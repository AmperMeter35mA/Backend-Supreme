#include "pch.h"

#include "Net.h"



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

private:
    double m_delay = 0;
    unsigned int m_now = 0;
    unsigned int m_prev = 0;
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
        //FAILS
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

            //int size = buffer.size();

            net::Message message;
            ((message << "STR" << "PUT") << (int)buffer.size()) << net::charArr(*((char*)buffer.data()), (int)buffer.size());

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

        //THIS FUCKING SHOT HAS TO BE HERE FOR VIDEO TO PLAY?!??!?!?!
        if (cv::waitKey(1) == 27)
        {
            log("ending");
        }//*/

        cv::imshow(m_windowName, m_frame);
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

        while (time + 10 >= timer.timeSeconds())
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
        (request << "STR") << "ACCEPT";
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


//USB Connection
class ArduinoInput
{
public:
    void sendUSB(net::endpoint& server)
    {
        const int bytes = 4;
        char buffer[bytes];// + 1]; buffer[bytes] = '\0';
        LPDWORD num = 0;

        if (ReadFile(m_usb_port, buffer, bytes - 1, num, NULL))
        {
            //buffer[3] = '\0';
            //log("input: " << (int)buffer[0] << " " << (int)buffer[1] << " " << (int)buffer[2] << std::endl);
            net::Message message;
            (message << "ARD:") << net::charArr(*buffer, 3);

            if (server.soc != 0)
                net::tcpsend(server, message);
            else
                debug("server disconnected");

            message.destory();
        }

    }

    char openDevice()//const char* Device, const unsigned int Bauds
    {
        // Open serial port
        m_usb_port = CreateFileA("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,/*FILE_ATTRIBUTE_NORMAL*/0, 0);
        if (m_usb_port == INVALID_HANDLE_VALUE) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                return -1; // Device not found

            // Error while opening the device
            return -2;
        }

        // Set parameters

        // Structure for the port parameters
        DCB dcbSerialParams;
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        // Get the port parameters
        if (!GetCommState(m_usb_port, &dcbSerialParams)) return -3;

        // Set the speed (Bauds)
        dcbSerialParams.BaudRate = CBR_9600;

        //select data size
        BYTE bytesize = 8;
        BYTE stopBits = 1;
        BYTE parity = 0;
        // configure byte size
        dcbSerialParams.ByteSize = bytesize;
        // configure stop bits
        dcbSerialParams.StopBits = stopBits;
        // configure parity
        dcbSerialParams.Parity = parity;

        // Write the parameters
        if (!SetCommState(m_usb_port, &dcbSerialParams)) return -5;

        // Set TimeOut

        COMMTIMEOUTS timeouts = { 0 };
        // Set the Timeout parameters
        timeouts.ReadIntervalTimeout = 0;
        // No TimeOut
        timeouts.ReadTotalTimeoutConstant = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.WriteTotalTimeoutConstant = MAXDWORD;
        timeouts.WriteTotalTimeoutMultiplier = 0;

        // Write the parameters
        if (!SetCommTimeouts(m_usb_port, &timeouts)) return -6;

        // Opening successfull
        return true;
    }

private:
    HANDLE m_usb_port;
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
    void sendMessage(net::endpoint& server, AsyncCommandReader& cmdReader)
    {
        cmdReader.changePrompt("msg: ");//if we change prompt here, getCmd thread will already have outputed previous one
        cmdReader.enable();
        std::string chat = cmdReader.read();
        while (chat != "/back")
        {
            //logn("msg: ");
            chat = cmdReader.read();
            if (chat != "" && chat != "/back")
            {
                net::Message message; message << "MSG" << chat.c_str();
                net::tcpsend(server, message);
                cmdReader.enable();
            }
        }
        cmdReader.changePrompt("enter command: ");
        cmdReader.enable();
    }

    void handleMessage(net::endpoint& client, net::Message& message)
    {
        net::SubMessage sm;
        std::string ss;
        (sm << message) >> ss;

        log("server message: " << ss);
    }

private:

};

void sendFile(net::endpoint& server);

int main2()
{
    net::init(54001);
    //net::async();
    Timer timer;
    AsyncCommandReader cmdReader;

    MessageCommunication msgComm;
    ArduinoInput arduino;
    VideoStream vidStr;

    net::endpoint server("127.0.0.1", 54001);//("89.143.91.205", 80);127.0.0.1
    

    if (net::connectto(server))
    {
        cmdReader.startCmdReader();
        std::string cmd = "null";

        while (cmd != "exit")
        {
            cmd = "null";//probably not required
            cmd = cmdReader.read();

            //RECV
            if (net::ismsg(server) && cmd != "exit")
            {
                net::Message message = net::tcprecv(server);
                net::SubMessage sm; std::string ss;
                (sm << message) >> ss;

                if (ss == "MSG")
                {
                    msgComm.handleMessage(server, message);
                }
                else if (ss == "STR")
                {
                    std::string secondParam;
                    (sm << message) >> secondParam;
                    log(secondParam);
                    //localCMD = command.load();
                    cmd = cmdReader.waitFor({ "exit" , "decline", "accept"});
                    if (cmd == "exit")
                    {
                        break;
                    }
                    else if (cmd.substr(0, 7) == "decline")
                    {
                        //handleAcceptStream(false)
                        vidStr.cancelStream(server);
                    }
                    else if (cmd.substr(0, 6) == "accept")
                    {
                        vidStr.acceptStream(server);
                        if (secondParam == "POST")
                        {
                            //accept
                            vidStr.acceptStream(server);
                            //recv
                            //while loop
                                //recv
                                //send CUT before stopping sending
                                //if revc CUT stop sending

                            bool handleStream = true;
                            while (handleStream)
                            {
                                if (GetKeyState(27))
                                {
                                    handleStream = false;
                                    //stopVideoStream
                                    vidStr.cancelStream(server);
                                }
                                if (net::ismsg(server))
                                {
                                    net::Message stream = net::tcprecv(server);

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
                        else if (secondParam == "HEAD")
                        {
                            //accept
                            vidStr.acceptStream(server);
                            log("accepted stream last");
                            //third param = stream name
                            //while loop
                                //send
                                //send CUT before stopping sending
                                //if revc CUT stop sending

                            bool handleStream = true;
                            while (handleStream)
                            {
                                if (GetKeyState(27))
                                {
                                    handleStream = false;
                                    //stopVideoStream
                                    vidStr.cancelStream(server);
                                }
                                if (net::ismsg(server))
                                {
                                    net::Message stream = net::tcprecv(server);

                                    std::string response;
                                    (net::SubMessage() << stream) >> response;
                                    if (response != "CUT")
                                    {
                                        vidStr.sendVideoStream(server);
                                    }
                                    else
                                    {
                                        handleStream = false;
                                    }

                                    stream.destory();
                                }
                            }

                        }
                    }
                }
                else if (ss == "FIL")
                {
                    std::string secondParam;
                    (sm << message) >> secondParam;
                    log(secondParam);

                    cmd = cmdReader.waitFor({ "exit" , "decline", "accept" });
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

            //std::string localCMD = command.load();

            //SEND
            if (cmd == "start client system")
            {
                timer.setTickRate(60);//24
                if (!arduino.openDevice())
                {
                    debugerr("failed to open USB port to Arduino");
                    return -1;
                }
                if (!vidStr.openVideo("F:/..............csgo clips/communist.mp4"))//"F:/..............csgo clips/communist.mp4"
                {
                    debugerr("failed to open video");
                    return -2;
                }
                vidStr.setWindowName("communist");

                while (!GetKeyState(27))
                {
                    if (timer.checkForTick())
                    {
                        arduino.sendUSB(server);
                        vidStr.sendVideoStream(server);
                        //sendAuioStream();
                    }
                }
            }
            else if (cmd.substr(0, 3) == "msg")
            {
                //cmdReader.changePrompt("msg: ");//look in sendMessage function for explanation
                msgComm.sendMessage(server, cmdReader);
                //cmdReader.changePrompt("enter command: ");
            }
            else if (cmd.substr(0, 3) == "get")
            {
                if (cmd.substr(4, 6) == "stream")
                {
                    //sendRequest
                    if(vidStr.requestVideoStream(server, "communist"))
                    {
                        //getVideoStream
                        bool handleStream = true;
                        while (handleStream)
                        {
                            if (GetKeyState(27))
                            {
                                handleStream = false;
                                    //stopVideoStream
                                    vidStr.cancelStream(server);
                            }
                            if (net::ismsg(server))
                            {
                                net::Message stream = net::tcprecv(server);
                                std::string response;
                                (net::SubMessage() << stream) >> response;
                                stream.destory();

                                if (response != "CUT")
                                    vidStr.recvVideoStream(stream);
                                else
                                    handleStream = false;
                            }
                        }
                    }
                    vidStr.closeWindow();
                    vidStr.cancelStream(server);//???
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
                    if (vidStr.sendStreamingRequest(server, "communist"))
                    {
                        if (!vidStr.openVideo("F:/..............csgo clips/communist.mp4"))
                            vidStr.cancelStream(server);//???
                        log("request accepted");
                        //sendVideoStream
                        bool handleStream = true;
                        while (handleStream)
                        {
                            if (GetKeyState(27))
                            {
                                handleStream = false;
                                //stopVideoStream
                                vidStr.cancelStream(server);
                            }
                            else
                            {
                                std::string response = "";
                                if (net::ismsg(server))
                                {
                                    //IMPLEMENT DISCONNECT CODE FROM SERVER CODE
                                    net::Message stream = net::tcprecv(server);
                                    (net::SubMessage() << stream) >> response;
                                    stream.destory();
                                }
                                if (response != "CUT")
                                    vidStr.sendVideoStream(server);
                                else
                                    handleStream = false;
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
            if(!cmdReader.isNewCommand())
                cmdReader.enable();
        
        }

        net::disconnect(server);
        cmdReader.exit();
    }

    net::release();

    return 1;
}

void sendFile(net::endpoint& server)
{
    std::string fileName; std::getline(std::cin, fileName);
    std::ifstream file;
    file.open(fileName, std::ios::in | std::ios::ate | std::ios::binary);// | std::ios::binary//"files/" + chat.substr(5, chat.size())
    int size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file.is_open())
    {
        const int dataSize = 4096;// 4096;1024
        char data[dataSize];
        int left = size % dataSize;

        for (int i = 0; i < size / dataSize; i++)
        {
            net::Message message;
            ZeroMemory(data, dataSize);
            file.read(data, dataSize);
            message << "FIL:" << net::charArr(*data, 4096);
            net::tcpsend(server, message);
        }

        if (left != 0)
        {
            net::Message message;
            ZeroMemory(data, dataSize);
            file.read(data, left);
            message << "FIL:" << net::charArr(*data, left);
            //net::connectto(gameServer);
            net::tcpsend(server, message);
        }

        file.close();
    }
}
