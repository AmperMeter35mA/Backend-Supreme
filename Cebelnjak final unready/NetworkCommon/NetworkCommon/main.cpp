#include "pch.h"

#include "Net.h"


class Timer
{
public:
    tm getLocalTime()
    {
        return m_loaclTime;
    }

    void updateLocalTime()
    {
        time_t t = time(0);
        m_loaclTime = *localtime(&t);
    }

    unsigned int getUniTime()
    {
        return time(0);
    }

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

private:
    tm m_loaclTime;
};


class VideoStream
{
public:
    bool openVideo(int device)
    {
        m_compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        m_compression_params.push_back(95);

        bool check = m_video.open(device);

        return check;
    }

    bool reopenDevice(int device)
    {
        return m_video.open(device);
    }

    bool sendVideoStream(net::endpoint& server)
    {
        if (m_video.read(m_frame))
        {
            Timer timer;
            timer.updateLocalTime();
            tm tLoc = timer.getLocalTime();
            std::string timeDate = std::to_string(tLoc.tm_hour) + ":" + std::to_string(tLoc.tm_min) + " " + std::to_string(tLoc.tm_mday) + "/" + std::to_string(tLoc.tm_mon + 1) + "/" + std::to_string(tLoc.tm_year + 1900);

            cv::putText(m_frame, timeDate, cv::Point(10, 20), cv::FONT_HERSHEY_DUPLEX, 0.4, CV_RGB(118, 185, 0));

            std::vector<uchar> buffer;
            cv::imencode(".jpg", m_frame, buffer, m_compression_params);

            net::Message message;
            (((message << "VID:") << (int)buffer.size()) << net::charArr(*((char*)buffer.data()), (int)buffer.size()) << tLoc.tm_hour < (tLoc.tm_year + 1900) < (tLoc.tm_mon + 1) < tLoc.tm_mday);

            if (server.soc != 0)
                net::tcpsend(server, message);
            else
                debug("server disconnected");

            message.destory();
        }
        else
        {
            log("capturing video failed");
            return false;
        }
    }

    void sendConfirmation(net::endpoint& server, int framesSent)
    {
        net::Message confirmation;
        (confirmation << "CON:") << framesSent;
        net::tcpsend(server, confirmation);
    }

    bool waitForConfirmation(net::endpoint& server)
    {
        bool ack = false;//true//false

        if (net::ismsg(server))
        {
            net::Message ackmsg = net::tcprecv(server);
            net::SubMessage acksmsg;

            (acksmsg << ackmsg) > ack;

            ackmsg.destory();
        }

        return ack;
    }

public:
    void showVideo()
    {
        cv::imshow(m_windowName, m_frame);

        cv::waitKey(1);
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
    bool sendArduinoData(net::endpoint& server, int hour, int min)
    {
        std::lock_guard<std::mutex> guard(m_dataMutex);

        net::Message message;
        ((message << "ARD:") << net::intArr(*m_data, 7)) << hour < min;
        //(message << "ARD:") << net::intArr(*m_data, 7);

        if (server.soc != 0)
        {
            if(!net::tcpsend(server, message))
                return false;

            //m_updated = false;
            return true;
        }
        else
            debug("server disconnected");

        message.destory();

        return false;
    }

    void readData()
    {
        const int bytes = 26;
        char buffer[bytes]; buffer[bytes - 1] = '\0';// + 1]; buffer[bytes] = '\0';
        LPDWORD num = 0;

        m_lastUpdateTime = m_timer.timeMillis();
        log("m_lastUpdateTime was initialized first time");

        while (m_runDataThread)
        {
            if (ReadFile(m_usb_port, buffer, bytes - 1, num, NULL) && !m_reopenFile)
            {
                //log((int)buffer[0] << " " << (int)buffer[1] << " " << (int)buffer[2] << " nigga " << (int)buffer[11] << " " << (int)buffer[12] << " " << (int)buffer[13]);
                //buffer[3] = '\0';
                if ((int)buffer[0] == 1
                    && (int)buffer[1] == 2
                    && (int)buffer[2] == 3
                    && (int)buffer[22] == 4
                    && (int)buffer[23] == 5
                    && (int)buffer[24] == 6)
                {
                    // THIS FUNCTION IS ON A THREAD, SO BLOCK ACCES TO THIS ARRAY WITH MUTEX HERE
                    std::lock_guard<std::mutex> guard(m_dataMutex);

                    m_data[0] = (int)buffer[3];//temp1
                    m_data[1] = (int)buffer[4];//temp2
                    m_data[2] = (int)((unsigned char)buffer[21]);//weight

                    m_data[3] = (int)((unsigned char)buffer[8]);//light
                    m_data[3] = (m_data[3] << 8) | (int)((unsigned char)buffer[7]);
                    m_data[3] = (m_data[3] << 8) | (int)((unsigned char)buffer[6]);
                    m_data[3] = (m_data[3] << 8) | (int)((unsigned char)buffer[5]);

                    m_data[4] = (int)((unsigned char)buffer[12]);//rain
                    m_data[4] = (m_data[4] << 8) | (int)((unsigned char)buffer[11]);
                    m_data[4] = (m_data[4] << 8) | (int)((unsigned char)buffer[10]);
                    m_data[4] = (m_data[4] << 8) | (int)((unsigned char)buffer[9]);

                    m_data[5] = (int)((unsigned char)buffer[16]);//moist
                    m_data[5] = (m_data[5] << 8) | (int)((unsigned char)buffer[15]);
                    m_data[5] = (m_data[5] << 8) | (int)((unsigned char)buffer[14]);
                    m_data[5] = (m_data[5] << 8) | (int)((unsigned char)buffer[13]);

                    m_data[6] = (int)((unsigned char)buffer[20]);//sound
                    m_data[6] = (m_data[6] << 8) | (int)((unsigned char)buffer[19]);
                    m_data[6] = (m_data[6] << 8) | (int)((unsigned char)buffer[18]);
                    m_data[6] = (m_data[6] << 8) | (int)((unsigned char)buffer[17]);

                    m_updated = true;
                    m_lastUpdateTime = m_timer.timeMillis(); log("m_lastUpdateTime was initialized");
                    m_reopenFile = false;
                }
                else
                {
                    m_updated = false;
                    m_reopenFile = true;
                    log("reading arduino data failed: wrong structure");
                }
            }
            else
            {
                m_updated = false;
                //m_reopenFile = true;
                m_reopenFile = !openDevice();
                log("reopening device");
            }
        }

    }

    bool shouldRestart()
    {
        if (m_updated)
            return false;

        if (m_timer.timeMillis() - m_lastUpdateTime > 10000)
            return true;

        return false;
    }

    bool isUpdated()
    {
        return m_updated;
    }

    void stopDataThread()
    {
        m_runDataThread = false;
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

private:
    int m_data[7];//temp 1, temp 2, weight, light, rain, moist, sound
    bool m_updated = false;//false
    bool m_reopenFile = false;
    bool m_runDataThread = true;
    std::mutex m_dataMutex;

private:
    Timer m_timer;
    int m_lastUpdateTime = 0;
};


class AutoConnect
{
public:
    bool establishConnection(net::endpoint& server)
    {
        if (m_delayTime <= m_timer.timeMillis())
        {
            m_isConnected = net::connectto(server);
        }
        else
        {
            return false;
        }
        
        if (!m_isConnected)
        {
            m_tries++;
            unsigned int time = m_timer.timeMillis();

            if (!(m_tries * 2000 > 40000))
            {
                m_delayTime = time + (m_tries * 2000);
            }
            else
            {
                m_delayTime = time + 1800000;
            }

            return false;
        }

        m_tries = 0;

        return true;
    }

    bool isConnected(net::endpoint server)
    {
        if (server.soc > 0)
        {
            return true;
        }
        else
        {
            m_isConnected = false;
            return false;
        }
    }

private:
    bool m_isConnected = false;
    int m_tries = 0;
    unsigned int m_delayTime = 0;

private:
    Timer m_timer;
};


class AutoRestart
{
public:
    AutoRestart(int restartHour, int restartMinute)
        : m_restartHour(restartHour), m_restartMinute(restartMinute)
    {
        m_programAllowNextRestartTime = m_timer.getUniTime() + 4200000;//hour and 10 min
    }

public:
    void checkForRestart()
    {
        //log("\n" << m_timer.getUniTime() << " : " << m_programAllowNextRestartTime);
        while (m_runRestartThread)
        {
            m_timer.updateLocalTime();
            tm tLoc = m_timer.getLocalTime();
            if ((tLoc.tm_hour == m_restartHour) && (tLoc.tm_min >= m_restartMinute) && (m_timer.getUniTime() >= m_programAllowNextRestartTime))
            {
                m_shouldRestart = true;
                m_runRestartThread = false;

                log("ordered to restart id 2");
                restart(10000);
            }

            m_shouldRestart = false;
        }
    }

    bool shouldRestart()
    {
        return m_shouldRestart;
    }

    void stopRestartTherad()
    {
        m_runRestartThread = false;
    }

    void restart(int milliSec = 0)
    {
        //KEEP COMMENTED WHILE TESTING
        if(milliSec) 
            m_timer.waitFor(milliSec);

        //system("c:\\windows\\system32\\shutdown /r /t 3");//restart in 3 seconds
        log("SYSTEM RESTARTING");
    }

private:
    int m_restartHour = 0;
    int m_restartMinute = 0;

private:
    int m_programAllowNextRestartTime = 0;
    bool m_shouldRestart = false;
    bool m_runRestartThread = true;

private:
    Timer m_timer;

};



int main()
{
    net::init(54001);
    

    Timer timer;





    
    /*// FOR TESTING THE FPS AND imencode FUNCTION TIME, TO MAKE IT MULTITHREADED

    int FPS = 0;
    int PTIME = timer.timeMillis();
    int TIME = timer.timeMillis();//

    std::vector<int> m_compression_params;
    m_compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);//IMWRITE_JPEG_LUMA_QUALITY
    m_compression_params.push_back(90);//90

    cv::VideoCapture vid(0);//"F:/..............csgo clips/communist.mp4"//0
    if (!vid.isOpened())
    {
        log("err in opening vid");
        return -1;
    }

    cv::Mat frame;

    while (1)
    {
        if (!vid.read(frame))
        {
            //break;
        }
        int tStart = timer.timeMillis();

        timer.updateLocalTime();
        tm tLoc = timer.getLocalTime();
        std::string timeDate = std::to_string(tLoc.tm_hour) + ":" + std::to_string(tLoc.tm_min) + " " + std::to_string(tLoc.tm_mday) + "/" + std::to_string(tLoc.tm_mon + 1) + "/" + std::to_string(tLoc.tm_year + 1900);
        cv::putText(frame, timeDate, cv::Point(10, 20), cv::FONT_HERSHEY_DUPLEX, 0.45, CV_RGB(118, 185, 0), 1.5);

        std::vector<uchar> buffer;


        int tNow = timer.timeMillis();
        cv::imencode(".jpg", frame, buffer, m_compression_params);//".jpg"//.png
        int delta = timer.timeMillis() - tNow;

        int size1 = frame.total() * frame.elemSize();
        int size2 = buffer.size();

        int tNow2 = timer.timeMillis();
        frame = cv::imdecode(buffer, 1);
        int delta2 = timer.timeMillis() - tNow2;

        buffer.empty();//

        cv::imshow("test", frame);

        if (cv::waitKey(1) == 27)// APPERENTLY THIS GIVES IT TIME TO PROCESS IMAGE SHOWING AND EVENTS AND TO LITTLE TIME MAKES THAT IMPOSSIBLE ??? SO YOU NEED TO GUESS ???
        {
            //break;
        }

        FPS++;

        TIME = timer.timeMillis();
        if (TIME - PTIME >= 1000)
        {
            PTIME = TIME;
            log("\nfps " << FPS << "\n");
            FPS = 0;
        }

        //int delta3 = timer.timeMillis() - tStart;
        log(delta << " " << delta2 << " " << timer.timeMillis() - tStart << " " << timer.timeMillis() - tStart);
        //
    }//*/







    ArduinoInput arduino;
    VideoStream vidStr;

    AutoConnect serverConn;
    AutoRestart autoRestart(0, 0);

    net::endpoint server("127.0.0.1", 54001);//("89.143.91.205", 80);127.0.0.1

    net::connectto(server);


    timer.setTickRate(15);//24//60//15
    if (!vidStr.openVideo(0))//"F:/..............csgo clips/communist.mp4"//0
    {
        debugerr("failed to open video");
        return -2;
    }
    //vidStr.setWindowName("test");// USELESS

    bool shouldSendVideo = true;

    arduino.openDevice();

    timer.updateLocalTime();
    tm localTime = timer.getLocalTime();
    int hour = localTime.tm_hour;
    int min = localTime.tm_min;

    bool lastUpdate = false;

    bool shouldRestart = false;
    bool shouldRun = true;


    // MAKE A WHILE LOOP IN IT
    std::thread arduinoReaderThread(&ArduinoInput::readData, &arduino);

    //CHECK FOR RESTART
    //MAKE IT ASYNCHRRONOUS SO IF THE MAIN PROGRAM FUCKING DIES IT STILL RESTARTS EVENTUALY
    std::thread autoRestartThread(&AutoRestart::checkForRestart, &autoRestart);

    int FPS = 0;
    int PTIME = timer.timeMillis();
    int TIME = timer.timeMillis();//*/
    int COUNTER = 0;

    while (shouldRun)
    {
        shouldRestart = autoRestart.shouldRestart();

        shouldRun = !shouldRestart;

        if (!serverConn.isConnected(server) && !shouldRestart)
        {
            if (serverConn.establishConnection(server))
            {
                shouldSendVideo = true;
            }
            //log("lol");
        }
        else
        {



            if (!shouldRestart)
            {

                // PROCESS DATA


                timer.updateLocalTime();
                localTime = timer.getLocalTime();

                // MAYBE WOULD BE NICE TO PUT THIS IN A CLASS
                if (localTime.tm_hour > hour || localTime.tm_min > min || (localTime.tm_hour == 0 && localTime.tm_hour < hour))//localTime.tm_hour > hour || localTime.tm_min > min || (localTime.tm_hour == 0 && localTime.tm_hour < hour)
                {
                    hour = localTime.tm_hour;
                    min = localTime.tm_min;

                    /*// PUT ON ANOTHER THREAD, BECAUSE readFile BLOCKS
                    if (!arduino.readData())
                    {
                        // HANDLE ERROR
                    }//*/

                    // USE A CHECK IF THERE IS A NEW VALUE // MAYBE EXPAND THIS FUNCTION FOR BETTER CHECKING
                    if (arduino.isUpdated())
                    {
                        if (!arduino.sendArduinoData(server, hour, min))
                        {
                            // HANDLE ERROR
                            log("error in sending");
                            net::disconnect(server);
                        }
                    }

                }

                if (arduino.shouldRestart())
                {
                    log("error in updating values");
                    log("ordered to restart id 1")

                    // HANDLE ERROR // RESTART
                    shouldRestart = true;
                    autoRestart.stopRestartTherad();

                    net::disconnect(server);

                    arduino.stopDataThread();

                    shouldRun = false;
                }



                // VIDEO STREAM // MOVE IT ABOVE ARDUINO SO IT DOENST INTERFERE WITH THE DISCONNECT OF PREVIOUS ERRORS


                //GetKeyState(27);

                if (timer.checkForTick())
                {
                    // READ AND SEND STREAM
                        // HANDLE ERRORS // VIDEO CAPTURING, ...
                    if (shouldSendVideo)//shouldSendVideo//false//true
                        if (vidStr.sendVideoStream(server))
                        {
                            shouldSendVideo = false;
                            vidStr.showVideo();

                            FPS++;
                        }
                        else
                        {
                            // HANDLE ERROR
                            vidStr.reopenDevice(0);
                        }

                    //vidStr.showVideo();
                    // 
                    // IF NOT DISCONNECTED WAIT FOR CONFIRAMATION
                        // IF NO CONFIRMATION WITHIN "SOME TIME", CONTINUE AND LET OTHER CODE CHECK FOR DISCONNECTS ...
                    if (serverConn.isConnected(server))
                    {
                        // THIS SHOULD REVIEVE CONFIRMATION OF CONTINUATON EVERY 1 OR 3 SECONDS AKA 15 OR 45 FRAMES

                        // WAIT FOR CONFIRMATION
                        if (COUNTER < 3)
                        {
                            shouldSendVideo = true;
                        }
                        else
                        {
                            if (vidStr.waitForConfirmation(server))
                            {
                                shouldSendVideo = true;
                                COUNTER = 0;
                            }
                        }


                        //shouldSendVideo = true;
                        // HANDLE WHAT HAPPENS IF IT DOESNT COME // MAYBE SEND REQUEST FOR CONFIRMATION ON A TIMEOUT TO MAYBE RESTART
                    }//*/
                    else
                    {
                        shouldSendVideo = false;
                    }
                }//*/

                    
                TIME = timer.timeMillis();
                if (TIME - PTIME >= 1000)
                {
                    PTIME = TIME;
                    
                    vidStr.sendConfirmation(server, FPS);
                    //log("fps " << FPS);
                    FPS = 0;

                    COUNTER++;
                }//*/




            }
            else
            {
                log("ordered to restart");

                shouldRun = false;

                net::disconnect(server);

                arduino.stopDataThread();

                autoRestart.stopRestartTherad();

                // AND PROSUMABLY OTHER VIDEO STUFF
            }
        }
    }

    //net::disconnect(server);

    log("releasing program");

    net::release();

    arduino.stopDataThread();// MAYBE NOT REQUIRED
    arduinoReaderThread.join();

    autoRestart.stopRestartTherad();// MAYBE NOT REQUIRED
    autoRestartThread.join();

    //if (shouldRestart)//OR DO IT ALWAYS?
    //{
    autoRestart.restart();
    //}
}

/*
    //
    // ATUO CONNECT //https://www.youtube.com/watch?v=M1E3roUNTMY
    // AUTO RESTART //https://www.youtube.com/watch?v=ZBztLmQ856s
    // TIME SYNC
    //


    //time_t t = time(0);
    //tm* dateTime = localtime(&t);
    //https://www.youtube.com/watch?v=M1E3roUNTMY
    //dateTime.yera gives years since 1900
    //gmtime(&t) gives you UTC time

    //system("c:\\windows\\system32\\shutdown /r /t 3");//restart in 3 seconds
    //https://www.youtube.com/watch?v=ZBztLmQ856s
//*/

/*std::vector<int> m_compression_params;
m_compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
m_compression_params.push_back(95);

cv::VideoCapture vid(0);
if (!vid.isOpened())
{
    log("err in opening vid");
    return -1;
}

cv::Mat frame;

while (1)
{
    if (!vid.read(frame))
    {
        //break;
    }
    Timer timer;
    timer.updateLocalTime();
    tm tLoc = timer.getLocalTime();
    std::string timeDate = std::to_string(tLoc.tm_hour) + ":" + std::to_string(tLoc.tm_min) + " " + std::to_string(tLoc.tm_mday) + "/" + std::to_string(tLoc.tm_mon + 1) + "/" + std::to_string(tLoc.tm_year + 1900);

    cv::putText(frame, timeDate, cv::Point(10, 20), cv::FONT_HERSHEY_DUPLEX, 0.4, CV_RGB(118, 185, 0));

    std::vector<uchar> buffer;
    cv::imencode(".jpg", frame, buffer, m_compression_params);

    frame = cv::imdecode(buffer, 1);


    buffer.empty();

    cv::imshow("test", frame);

    if (cv::waitKey(1) == 27)// APPERENTLY THIS GIVES IT TIME TO PROCESS IMAGE SHOWING AND EVENTS AND TO LITTLE TIME MAKES THAT IMPOSSIBLE ??? SO YOU NEED TO GUESS ???
    {
        //break;
    }
}//*/

/*cv::VideoCapture vid(0);
if (!vid.isOpened())
{
    log("err in opening vid");
    return -1;
}

cv::Mat frame;

while (1)
{
    if (!vid.read(frame))
    {
        break;
    }

    cv::imshow("test", frame);

    if (cv::waitKey(1) == 27)
    {
        break;
    }
}//*/