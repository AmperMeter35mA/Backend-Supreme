#include "pch.h"

#include "Net.h"


//void handleMessage(net::endpoint& client, net::Message& message);
//void handleSQL(net::endpoint& client, net::Message& message);
//void handleFile(net::endpoint& client, net::Message& message);
//void handleStream(net::endpoint& client, net::Message& message);

//PIPES
//HANDLE hPipe;//uncomment for SQL

enum class State
{
    NONE,
    MSG,
};

enum class InternalClientState
{
    NONE,
    PENDING_SEND_STREAM,
    PENDING_RECV_STREAM,
    DECLINED_STREAM,
    ACCEPTED_SEND_STREAM,
    ACCEPTED_RECV_STREAM,
    SEND_STREAM,
    RECV_STREAM
};

class Timer
{
public:
    unsigned long timeMillis()
    {
        m_now = clock();
        return m_now;
    }

    unsigned long timeSeconds()
    {
        return clock() / 1000;
    }

    void setTickRate(int tickRate)
    {
        m_delay = 1000.0 / tickRate;
    }

    bool checkForTick()
    {
        if (timeMillis() - m_prev >= m_delay)
        {
            m_prev = m_now;
            return true;
        }

        return false;
    }

    void waitFor(unsigned int milliSec)
    {
        unsigned int time = timeMillis();
        while (time + milliSec >= timeMillis());
    }

    void nonBlockingWaitFor(unsigned int waitTime)
    {
        m_wait = false;
        std::thread wait(&Timer::asyncWaitFor, this, waitTime);
    }

    bool isWaitingOver()
    {
        return m_wait;
    }

public:
    void asyncWaitFor(unsigned int milliSec)
    {
        unsigned int time = timeMillis();
        while (time + milliSec >= timeMillis());

        m_wait = true;
    }


private:
    double m_delay = 0;
    unsigned int m_now = 0;
    unsigned int m_prev = 0;

    bool m_wait = true;
};

class VideoStream
{
public:
    bool openVideo(std::string videoPath)
    {
        m_compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        m_compression_params.push_back(50);

        bool check = m_video.open(videoPath);

        return check;
    }

    bool sendVideoStream(net::endpoint& server)
    {
        if (m_video.read(m_frame))
        {
            /*int size = frame.rows * frame.cols * frame.channels();//frame.channels()
            arr = new char[size];
            for (int i = 0; i < size; i++)
            {
                arr[i] = (char)frame.data[i];
            }

            net::Message message;
            (message << "STR:" << frame.rows < frame.cols) << net::charArr(*((char*)arr), size);
            net::tcpsend(server, message);

            message.destory();
            delete[] arr;//*/

            std::vector<uchar> buffer;
            cv::imencode(".jpg", m_frame, buffer, m_compression_params);

            net::Message message;
            ((message << "STR:" << "PUT") << (int)buffer.size()) << net::charArr(*((char*)buffer.data()), buffer.size());
            
            if (server.soc != 0)
                net::tcpsend(server, message);
            else
                debug("server disconnected");

            message.destory();
        }
        else
        {
            //debugerr("failed to read frame");
            return false;
        }
    }

    void recvVideoStream(net::Message& message)
    {
        net::SubMessage sm;

        /*int rows, cols;
        (sm << message) > rows > cols;
        (sm << message) >> (char*&)arr;

        cv::imdecode(arr, )

        frame.create(rows, cols, CV_8UC3);
        //frame.create(rows, cols, CV_8UC3);
        for (int i = 0; i < rows * cols * 3; i++)
        {
            frame.data[i] = (uchar)arr[i];
        }//*/

        char* arr;
        int size;
        (sm << message) > size;
        (sm << message) >> (char*&)arr;

        std::vector<uchar> buf(arr, &arr[size - 1]);

        m_frame = cv::imdecode(buf, 1);

        //counter++;
        buf.empty();
        delete[] arr;

        //log("frames: " << counter);

        //THIS FUCKING SHOT HAS TO BE HERE FOR VIDEO TO PLAY?!??!?!?!
        if (cv::waitKey(1) == 27)
        {
            log("ending");
        }//*/

        cv::imshow(m_windowName, m_frame);//m_windowName
    }

    bool requestVideoStream(net::endpoint& server, std::string streamName)
    {
        net::Message request;
        ((request << "STR") << "HEAD") << &streamName;
        net::tcpsend(server, request);

        Timer timer;
        unsigned long time = timer.timeSeconds();

        while (time + 3 >= timer.timeSeconds())
        {
            if (net::ismsg(server))
            {
                net::Message response = net::tcprecv(server);
                std::string confirm;
                (net::SubMessage() << response) >> confirm;
                if (confirm == "ACCEPT")
                    return true;

                return false;
            }
        }

        return false;
    }

    bool sendStreamingRequest(net::endpoint& server, std::string streamName)
    {
        net::Message request;
        ((request << "STR") << "POST") << &streamName;
        net::tcpsend(server, request);

        Timer timer;
        unsigned long time = timer.timeSeconds();

        while (time + 3 >= timer.timeSeconds())
        {
            if (net::ismsg(server))
            {
                net::Message response = net::tcprecv(server);
                std::string confirm;
                (net::SubMessage() << response) >> confirm;
                if (confirm == "ACCEPT")
                    return true;

                return false;
            }
        }

        return false;
    }

    void cancelStream(net::endpoint& server)
    {
        net::Message request;
        (request << "STR") << "CUT";
        net::tcpsend(server, request);
    }

    void acceptStream(net::endpoint& server)
    {
        net::Message request;
        request << "ACCEPT";
        net::tcpsend(server, request);
    }

    void closeWindow()
    {
        cv::destroyWindow(m_windowName);
    }

    void setWindowName(std::string windowName)
    {
        m_windowName = windowName;
    }

    std::string getWindowname()
    {
        return m_windowName;
    }

private:
    cv::VideoCapture m_video;
    cv::Mat m_frame;

    std::string m_windowName = "default";

private:
    std::vector<int> m_compression_params;
};

class AsyncCommandReader
{
public:
    AsyncCommandReader()
    {
        m_command = "null";
        m_readCommands.store(true);
    }

    void startCmdReader()
    {
        m_commandReader = std::thread(&AsyncCommandReader::getCmd, this);
    }

    void exit()
    {
        m_readCommands.store(false);
        m_command = "exit";

        m_commandReader.join();
    }

    void changePrompt(std::string prompt)
    {
        m_prompt = prompt;
    }

    std::string read()
    {
        //enable();

        if (m_newCommand)
        {
            std::lock_guard<std::mutex> guard(m_cmdMutex);
            m_newCommand = false;
            //disable();//IDK
            return m_command;
        }

        return "";
    }

    bool isNewCommand()
    {
        return m_newCommand;
    }

    void enable()
    {
        m_readCommands.store(true);
    }

    void disable()
    {
        m_readCommands.store(false);
    }

    //ONLY FOR CLIENT CODE - PROBABLY
    std::string waitFor(std::vector<std::string> commands)
    {
        enable();//added it with no idea if I should
        m_waitForInput = true;
        while (m_waitForInput)
        {
            std::string input = read();
            for (int i = 0; i < commands.size(); i++)
            {
                if (input == commands.at(i))
                    return input;
            }
        }

        return "";
    }

    //SERVER VERSION
    void handleCommandWaitForOnClient(std::vector<std::string> commands, std::string cmd, int clientID, InternalClientState& ics)
    {
        if (cmd != "" && cmd != "null")
        {
            int numCmdPart1 = -1;
            int index;

            for (index = 0; index < cmd.size(); index++)
            {
                if (cmd[index] == ' ')
                {
                    numCmdPart1 = std::stoi(cmd.substr(0, index));//POSSIBLE ERRORS
                    break;
                }
            }

            //MAKE IT JUST RETURN THE COMMAND AND MOVE THE BELOW CODE OUT THE FUNCTION
            if (numCmdPart1 == clientID)
                for (int i = 0; i < commands.size(); i++)
                {
                    //std::cout << cmdPart2.substr(index + 1, cmdPart2.size()) << std::endl;
                    if (cmd.substr(index + 1) == commands.at(i))
                    {
                        //CHANGE INTERNAL STATE, BOTH FOR STREAM AND FILE, AND POSSIBLY SEND RESPONSE IF DECLINED
                        if (ics == InternalClientState::PENDING_RECV_STREAM)
                            ics = InternalClientState::ACCEPTED_RECV_STREAM;
                        else if (ics == InternalClientState::PENDING_SEND_STREAM)
                            ics = InternalClientState::ACCEPTED_SEND_STREAM;
                        //DO IT FOR FILE TOO

                        return;
                    }
                }
        }
    }

    void cancelWait()
    {
        m_waitForInput = false;
    }

    void cancelWaitIn(unsigned int milliSec)
    {
        std::thread wait(&AsyncCommandReader::asyncCancelWait, this, milliSec);
    }

public:
    void getCmd()
    {
        std::string temp = "null";
        while (m_command != "exit")
        {
            if (m_readCommands.load())
            {
                std::cout << m_prompt;
                std::getline(std::cin, temp);

                std::lock_guard<std::mutex> guard(m_cmdMutex);
                m_command = temp;
                m_readCommands.store(false);
                m_newCommand = true;
            }
        }
    }

    void asyncCancelWait(unsigned int milliSec)
    {
        Timer timer;
        timer.waitFor(milliSec);
        m_waitForInput = false;
    }

private:
    std::string m_command;
    //std::atomic<const char*> cmd2;
    std::atomic<bool> m_readCommands;

    std::thread m_commandReader;
    std::mutex m_cmdMutex;

private:
    std::string m_prompt = "enter command: ";
    bool m_newCommand = false;
    bool m_waitForInput = true;

};

class MessageCommunication
{
public:
    MessageCommunication(State* state)
        : m_state(state) { }

public:
    void sendMessage(net::endpoint& server, AsyncCommandReader& cmdReader)
    {
        if (*m_state != State::MSG)
        {
            *m_state = State::MSG;
            cmdReader.changePrompt("msg: ");//if we change prompt here, getCmd thread will already have outputed previous one        
        }
        cmdReader.enable();
        std::string chat = cmdReader.read();
        

        if (chat != "" && chat != "/back")
        {
            net::Message message; message << "MSG" << chat.c_str();
            net::tcpsend(server, message);
            cmdReader.enable();
        }
        else if (chat == "/back")
        {
            *m_state = State::NONE;
            cmdReader.changePrompt("enter command: ");
            cmdReader.enable();
        }
    }

    void handleMessage(net::endpoint& client, net::Message& message)
    {
        net::SubMessage sm;
        std::string ss;
        (sm << message) >> ss;

        log("server message: " << ss);
    }

private:
    State* m_state;
};

//System class for client connection IDs
class Clients
{
public:
    int getIDFromIndex(int index)
    {
        m_currentID = m_IDs.at(index);
        m_currentIndex = index;

        return m_currentID;
    }

    net::endpoint& getEP(int clientID)
    {
        if(m_currentID == clientID)
            return m_endpoints.at(m_currentIndex);

        return m_endpoints.at(getIndexFromID(clientID));
    }

    VideoStream& getVS(int clientID)
    {
        if (m_currentID == clientID)
            return m_vidStrs.at(m_currentIndex);

        return m_vidStrs.at(getIndexFromID(clientID));
    }

    Timer& getT(int clientID)
    {
        if (m_currentID == clientID)
            return m_timers.at(m_currentIndex);

        return m_timers.at(getIndexFromID(clientID));
    }

    InternalClientState& getS(int clientID)
    {
        if (m_currentID == clientID)
            return m_internalStates.at(m_currentIndex);

        return m_internalStates.at(getIndexFromID(clientID));
    }

    void add()
    {
        if (freeIDs.size() != 0)
        {
            m_IDs.push_back(freeIDs.back());
            freeIDs.pop_back();
        }
        else
            m_IDs.push_back(m_highestID++);

        m_endpoints.push_back(net::acceptConnection());
        m_vidStrs.push_back(VideoStream());
        m_timers.push_back(Timer());
        m_internalStates.push_back(InternalClientState::NONE);
    }

    void remove(int clientID)
    {
        int currentIndex = m_currentIndex;
        if (m_currentID != clientID)
            currentIndex = getIndexFromID(clientID);
            
        m_IDs.erase(m_IDs.begin() + currentIndex);
        m_endpoints.erase(m_endpoints.begin() + currentIndex);
        m_vidStrs.erase(m_vidStrs.begin() + currentIndex);
        m_timers.erase(m_timers.begin() + currentIndex);

        freeIDs.push_back(currentIndex);

        if (size() > m_currentIndex)
            m_currentID = m_IDs.at(m_currentIndex);
        else if(m_IDs.size() != 0)
        {
            m_currentID = m_IDs.back();
        }
    }

    int size()
    {
        return m_IDs.size();
    }

private:
    int getIndexFromID(int clientID)
    {
        for (int i = 0; i < size(); i++)
        {
            if (m_IDs.at(i) == clientID)
            {
                return i;
            }
        }

        return 0;
    }

private:
    std::vector<int> m_IDs;
    std::vector<net::endpoint> m_endpoints;
    std::vector<VideoStream> m_vidStrs;
    std::vector<Timer> m_timers;
    std::vector<InternalClientState> m_internalStates;

private:
    int m_highestID = 0;
    std::vector<int> freeIDs;

    int m_currentIndex = -1;
    int m_currentID = -1;

};

int main2()
{
    //UNCOMMENT FOR FILE
    //file.open("idk.png", std::ios::out | std::ios::beg | std::ios::binary);
    
    //UNCOMMENT FOR PIPES
    //hPipe = CreateFile(TEXT("\\\\.\\pipe\\mynamedpipe"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    net::init(54001);

    //
    // MAKE ANOTHER GENERAL STATE FOR CLIENT
    //
    State state = State::NONE;

    Clients clients;

    AsyncCommandReader cmdReader;
    MessageCommunication msgComm(&state);

    //net::endpoint client;
    //Timer timer;
    //VideoStream vidStr;

    std::string cmd = "null";

    cmdReader.startCmdReader();

    //std::vector<net::endpoint> endpoints;
    while (cmd != "exit")
    {

        bool shouldAccept = true;

        while (net::isNewConnection() && shouldAccept)
        {
            //net::endpoint connection = net::getNewConnection();
            //do stuff with it

            clients.add();
            //or
            //net::declineConnection();

            //net::enableDataReading(connection);

            //net::getAmountOfConnections();
            //shouldAccept = false;
        }

        cmd = "null";//probably not required
        cmd = cmdReader.read();

        for (int i = 0; i < clients.size(); i++)
        {
            //RECV
            //net::endpoint& client = endpoints.at(clientID);
            int clientID = clients.getIDFromIndex(i);

            net::endpoint& client = clients.getEP(clientID);
            Timer& timer = clients.getT(clientID);
            VideoStream& vidStr = clients.getVS(clientID);
            InternalClientState& internalState = clients.getS(clientID);


            if (client.soc == 0)
            {
                clients.remove(clientID);
                continue;
            }

            if (cmd == "exit")
                break;

            //
            // WHILE IN State::MSG, RECIEVING PART OF THE PROGRAM SHOULD NOT TAKE COMMANDS
            //      - SENDING PART ALREADY DOESN'T
            //

            //DONE, ONLY FOR DESCRIPTION
            //MOVE THIS UP -- PASS IN state AND clientID AND SET IT TO State::RECV_STREAM WHEN COMMAND HAS BEEN RECORDED
            if (state == State::NONE && (internalState == InternalClientState::PENDING_RECV_STREAM || internalState == InternalClientState::PENDING_SEND_STREAM))
                cmdReader.handleCommandWaitForOnClient({ "exit" , "decline", "accept" }, cmd, clientID, internalState);
            //DONE
            //MOVE ACCEPT OR DECLINE UP WITH waitFor... OF IN waitFor...
            else if (internalState == InternalClientState::DECLINED_STREAM)//(cmd.substr(0, 7) == "decline")
            {
                vidStr.cancelStream(client);
                internalState = InternalClientState::NONE;
            }
            else if (internalState == InternalClientState::ACCEPTED_RECV_STREAM)
            {
                vidStr.acceptStream(client);
                internalState = InternalClientState::RECV_STREAM;
            }
            else if (internalState == InternalClientState::ACCEPTED_SEND_STREAM)
            {
                vidStr.acceptStream(client);
                internalState = InternalClientState::SEND_STREAM;
            }
            
            //RECV
            if (net::ismsg(client))// && cmd != "exit"
            {
                net::Message message = net::tcprecv(client);
                net::SubMessage sm; std::string ss;
                (sm << message) >> ss;
                
                if (ss == "MSG")
                {
                    msgComm.handleMessage(client, message);
                }
                else if (ss == "STR")
                {
                    //
                    // REWRITE COMMAND READING AND MESSAGE PROCESSING SO THEY ARE APPART, COMMAND READING IS NOW OUTSIDE if(net::ismsg(client)) STATEMENT
                    //      - AND BASICALLY CREATE SOME KIND OF EVENT SYTSTEM FOR cmdReader.waitFor()
                    //      - MOVE MESSAGE PROCESSING CODE IN THE CLASS AND CONTROL IT WITH INTERNAL STATE, WHICH IS CONTROLLED BY COMMANDS
                    //
                    std::string secondParam;
                    (sm << message) >> secondParam;
                    if(internalState == InternalClientState::NONE)
                        log("client: " << clientID << " requested: " << secondParam);//IDK ABOUT THIS HERE IS WHERE I STOPPED

                    //THIS MIGHT NOT BE NEEDED
                    if (cmd == "exit")
                    {
                        break;
                    }
                    else if (internalState == InternalClientState::RECV_STREAM)//(secondParam == "POST")
                    {
                        //recv
                        //while loop
                            //recv
                            //send CUT before stopping sending
                            //if revc CUT stop sending
                        if (false)////GetKeyState(27)
                        {
                            //stopVideoStream
                            vidStr.cancelStream(client);
                            internalState = InternalClientState::NONE;
                        }
                        else
                        {
                            //net::Message stream = net::tcprecv(client);
                            std::string response;
                            //(net::SubMessage() << stream) >> response;
                            //stream.destory();

                            if (secondParam != "CUT")
                                vidStr.recvVideoStream(message);
                            else
                                internalState = InternalClientState::NONE;
                        }

                    }
                    else if (internalState == InternalClientState::SEND_STREAM)//(secondParam == "HEAD")
                    {
                        //third param = stream name
                        //while loop
                            //send
                            //send CUT before stopping sending
                            //if revc CUT stop sending

                        if (GetKeyState(27))//IF COMMAND IS ENTERED
                        {
                            vidStr.cancelStream(client);
                            internalState = InternalClientState::NONE;
                        }
                        else
                        {
                            std::string response = "";
                            if (net::ismsg(client))
                            {
                                net::Message stream = net::tcprecv(client);
                                (net::SubMessage() << stream) >> response;
                                stream.destory();
                            }

                            if (response != "CUT")
                                vidStr.sendVideoStream(client);
                            else
                                internalState = InternalClientState::NONE;
                        }
                    }
                    else if (internalState == InternalClientState::NONE)
                    {
                        if (secondParam == "POST")
                            internalState = InternalClientState::PENDING_RECV_STREAM;
                        else if (secondParam == "HEAD")
                            internalState = InternalClientState::PENDING_SEND_STREAM;
                    }
                }
                else if (ss == "FIL")
                {
                    std::string secondParam;
                    (sm << message) >> secondParam;
                    log(secondParam);

                    if (cmd == "exit")
                    {
                        break;
                    }
                    else if (cmd.substr(0, 7) == "decline")
                    {
                        //handleAcceptFile(false)
                    }
                    else if (cmd.substr(0, 6) == "accept")
                    {
                        //handleAcceptFile(true)
                        //getFile(client, message);
                    }
                }

                message.destory();

                cmd = "null";
                cmdReader.disable();//it should be disable???
            }

            

            //SEND
            if (cmd.substr(0, 3) == "msg" || state == State::MSG)
            {
                //cmdReader.changePrompt("msg: ");//look in sendMessage function for explanation
                msgComm.sendMessage(client, cmdReader);
                //cmdReader.changePrompt("enter command: ");
            }
            else if (cmd.substr(0, 3) == "get")
            {
                if (cmd.substr(4, 6) == "stream")
                {
                    //sendRequest
                    if (vidStr.requestVideoStream(client, "communist"))
                    {
                        //getVideoStream
                        bool handleStream = true;
                        while (handleStream)
                        {
                            if (GetKeyState(27))
                            {
                                handleStream = false;
                                //stopVideoStream
                                vidStr.cancelStream(client);
                            }
                            if (net::ismsg(client))
                            {
                                net::Message stream = net::tcprecv(client);

                                std::string response;
                                (net::SubMessage() << stream) >> response;
                                if (response != "CUT")
                                {
                                    vidStr.recvVideoStream(stream);
                                }
                                else
                                {
                                    handleStream = false;
                                }

                                stream.destory();
                            }
                        }
                    }
                    vidStr.closeWindow();
                }
                else if (cmd.substr(4, 4) == "file")
                {
                    //sendRequest
                    //getFile
                }
            }
            else if (cmd.substr(0, 3) == "put")
            {
                if (cmd.substr(4, 6) == "stream")
                {
                    //sendRequest
                    if (vidStr.sendStreamingRequest(client, "communist"))
                    {
                        //sendVideoStream
                        bool handleStream = true;
                        while (handleStream)
                        {
                            if (GetKeyState(27))
                            {
                                handleStream = false;
                                //stopVideoStream
                                vidStr.cancelStream(client);
                            }
                            if (net::ismsg(client))
                            {
                                net::Message stream = net::tcprecv(client);

                                std::string response;
                                (net::SubMessage() << stream) >> response;
                                if (response != "CUT")
                                {
                                    vidStr.sendVideoStream(client);
                                }
                                else
                                {
                                    handleStream = false;
                                }

                                stream.destory();
                            }
                        }
                    }

                    //vidStr.closeWindow();
                }
                else if (cmd.substr(4, 4) == "file")
                {
                    //sendRequest
                    //putFile
                }
            }

            //cmd = "null";
            if (!cmdReader.isNewCommand())
                cmdReader.enable();





            /*if (net::ismsg(client))
            {
                net::Message message = net::tcprecv(client);
                net::SubMessage sm; std::string ss;
                (sm << message) >> ss;

                if (ss == "MSG")
                {
                    handleMessage(client, message);
                }
                //else if (ss == "SQL")
                //{
                //    handleSQL(client, message);
                //}
                else if (ss == "FIL")
                {
                    handleFile(client, message);
                }
                else if (ss == "STR")
                {
                    handleStream(client, message);
                }
                //else if (ss == "CMD:")
                //{
                //    //commands ?? arent they just messages
                //}

                message.destory();

            }//*/

            //SEND
            //not now...
        }
    }


    //CloseHandle(hPipe);

    cmdReader.exit();

    net::release();

    return 1;
}
/*
void handleMessage(net::endpoint& client, net::Message& message)
{
    net::SubMessage sm;
    std::string ss;
    (sm << message) >> ss;
    log(ss);
}

/*
//PIPES
DWORD dwWritten;
//or CallNamedPipe https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createnamedpipea
void handleSQL(net::endpoint& client, net::Message& message)
{
    net::SubMessage sm;
    std::string ss;

    (sm << message) >> ss;
    //std::string ss2 = ss.substr(4, ss.length());
    const char* s = ss.c_str();// ss2.c_str();
    log(ss.c_str() << " " << strlen(s));
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hPipe, s, strlen(s), &dwWritten, NULL);// = length of string + terminating '\0' !!!
    }
}
*//*

std::ofstream file;
void handleFile(net::endpoint& client, net::Message& message)
{
    net::SubMessage sm;
    char* buf;

    sm << message;
    int size = sm.getSubHeader().size;
    sm >> buf;

    for (int j = 0; j < size; j++)
    {
        //char b = buf[j];
        //logn(buf[j]);//buf[j]);
        file << buf[j];//buf[j];
    }
    file.flush();//very important
}

//OPENCV
cv::Mat frame;
int counter = 0;
char* arr;
void handleStream(net::endpoint& client, net::Message& message)
{
    net::SubMessage sm;

    /*int rows, cols;
    (sm << message) > rows > cols;
    (sm << message) >> (char*&)arr;

    cv::imdecode(arr, )

    frame.create(rows, cols, CV_8UC3);
    //frame.create(rows, cols, CV_8UC3);
    for (int i = 0; i < rows * cols * 3; i++)
    {
        frame.data[i] = (uchar)arr[i];
    }//*//*

    int size;
    (sm << message) > size;
    (sm << message) >> (char*&)arr;

    std::vector<uchar> buf(arr, &arr[size - 1]);

    frame = cv::imdecode(buf, 1);

    counter++;
    buf.empty();
    delete[] arr;

    log("frames: " << counter);

    if (cv::waitKey(1) == 27)
    {
        log("ending");
    }

    cv::imshow("communist", frame);
}

//*/