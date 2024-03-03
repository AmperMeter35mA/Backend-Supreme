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
    void processData(net::Message& video)
    {
        net::SubMessage sm;
        char* arr;
        int size;

        ( (( ((sm << video) > size) << video) >> (char*&)arr) << video) > m_hour > m_year > m_month > m_day;


        m_buf.empty();//----------------
        //std::vector<uchar> buf(arr, &arr[size - 1]);
        m_buf = std::vector<uchar>(arr, &arr[size - 1]);

        m_frame = cv::imdecode(m_buf, 1);
        
        //buf.empty();
        delete[] arr;

        cv::imshow("test", m_frame);
        cv::waitKey(1);//*/

        m_frameCount++;

        // THIS SHOULD SEND CONFIRMATION OF CONTINUATON EVERY 1 OR 3 SECONDS AKA 15 OR 45 FRAMES

        /*net::Message ack;
        ack << true;

        net::tcpsend(client, ack);
        ack.destory();//*/
    }

    void handleSegment()
    {
        m_segmentCount++;
        m_segmentPassed = true;
    }

    void sendConfirmation(net::endpoint& client)
    {
        if (m_segmentCount >= 3)
        {
            net::Message confirmation;
            confirmation << true;
            net::tcpsend(client, confirmation);

            m_segmentCount = 0;
        }
    }

private:
    cv::Mat m_frame;
    cv::VideoWriter m_vidSaver;

private:
    std::vector<uchar> m_buf;

private:
    int m_hour = 0;
    int m_min = 0;

    int m_day = 0;
    int m_month = 0;
    int m_year = 0;

private:
    int m_frameCount = 0;
    int m_segmentCount = 0;
    bool m_segmentPassed = false;

private:
    friend class Database;

};

//
// //Timer - util
// Database - interacting with database and potentially interacting with fronternd program
// Video - saving to file and database and get cancer with nodejs
// //Arduino - interpretation
// //AutoRestart - autommaticaly restart at midnight
//

class ArduinoSystem
{
public:
    void processData(net::Message data)
    {
        net::SubMessage sm;
        (((sm << data) > net::intArr(*m_arduinoData, 7)) << data) > m_hour > m_min;

        m_temp1 = m_arduinoData[0];
        m_temp2 = m_arduinoData[1];
        m_weight = m_arduinoData[2];
        m_moist = m_arduinoData[5];
        m_soundActivity = m_arduinoData[6];

        int tempLight = m_arduinoData[3];
        int tempRain = m_arduinoData[4];

        log("\n" << m_hour << ":" << m_min << ", temperatura 1: " << m_arduinoData[0] << ", temperatura 2: " << m_arduinoData[1] << ", svetlost: " << m_arduinoData[3] << ", dez: " << m_arduinoData[4] << ", vlaga: " << m_arduinoData[5] << ", teza: " << m_arduinoData[2] << ", zvok: " << m_arduinoData[6] << "\n");

        if (tempRain < 350)
            m_rain = true;
        else
            m_rain = false;

        if (tempLight < 750)
            m_light = true;
        else
            m_light = false;
    }


private:
    int m_hour = 0;
    int m_min = 0;

    int m_temp1 = 0;
    int m_temp2 = 0;
    int m_weight = 0;
    int m_moist = 0;
    int m_soundActivity = 0;
    bool m_rain = false;
    bool m_light = true;

private:
    int m_arduinoData[7];

private:
    friend class Database;
};

class Database
{
public:
    bool init(std::string user, std::string password = "")
    {
        if (!(m_mysql = mysql_init(0)))
        {
            debugerr("mysql init failed");
            return false;
        }

        // MAKE THIS FUNCTION AUTO TRY TO CONNECT LIKE HOW CLIENT CONNECTION TO SERVER WORKS
        /*if (!mysql_real_connect(m_mysql, "localhost", user.c_str(), password.c_str(), "cpp_test", 3306, NULL, NULL))
        {
            debugerr("mysql connection failed");
            debugerr(mysql_error(m_mysql));
            return false;
        }//*/

        int tries = 0;
        while (!mysql_real_connect(m_mysql, "localhost", user.c_str(), password.c_str(), "cpp_test", 3306, NULL, NULL))
        {
            log("here");
            m_timer.waitFor(tries * 2000);
            if(tries++ > 5)
            {
                return false;
            }

            if (!(m_mysql = mysql_init(0)))
            {
                debugerr("mysql init failed");
                return false;
            }
            log("here2");
        }

        if (!execute("use cebelnjak;"))
            return false;


        m_hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\videopipe"),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
            1,
            1024 * 100,
            1024 * 100,
            NMPWAIT_USE_DEFAULT_WAIT,
            NULL);

        if (m_hPipe == INVALID_HANDLE_VALUE)
        {
            log("pipe init failed");
            return false;
        }

        return true;
    }

    void release()
    {
        mysql_close(m_mysql);
    }

public:
    bool storeArduinoData(ArduinoSystem& arduino)
    {
        m_timer.updateLocalTime();
        tm tNow = m_timer.getLocalTime();

        std::string currentYear = std::to_string(tNow.tm_year + 1900);
        std::string currentMonth = (tNow.tm_mon + 1) < 10 ? "0" + std::to_string(tNow.tm_mon + 1) : std::to_string(tNow.tm_mon + 1);
        std::string currentDay = tNow.tm_mday < 10 ? "0" + std::to_string(tNow.tm_mday) : std::to_string(tNow.tm_mday);

        if (!execute("INSERT INTO panj VALUES('" + currentYear + "-" + currentMonth + "-" + currentDay + " " + std::to_string(arduino.m_hour) + ":" + std::to_string(arduino.m_min) + ":00', " + std::to_string(arduino.m_temp1) + ", " + std::to_string(arduino.m_temp2) + ", " + std::to_string(arduino.m_light) + ", " + std::to_string(arduino.m_rain) + ", " + std::to_string(arduino.m_moist) + ", " + std::to_string(arduino.m_weight) + ", " + std::to_string(arduino.m_soundActivity) + ");"))
            return false;

        return true;
    }

    bool processVideoData(VideoStream& video)
    {
        if (video.m_frameCount <= 15 && !video.m_segmentPassed && video.m_vidSaver.isOpened())
        {
            //if (!video.m_segmentPassed)// NO "!" BEFORE
            video.m_vidSaver.write(video.m_frame);
            /*cv::imshow("test", video.m_frame);
            cv::waitKey(1);//*/

            // PUT DATA INTO FILE BUFFER FOR NodeJS TO READ WITH A DLL

            if (m_hPipe != INVALID_HANDLE_VALUE)
            {
                bool res = WriteFile(m_hPipe, video.m_buf.data(), video.m_buf.size(), NULL, NULL);
                log("writing to pipe, success: " << res << ", size: " << video.m_buf.size());
            }
        }

        return true;
    }

    void fillMissingFrames(VideoStream& video)
    {
        if (video.m_frameCount <= 15)
        {
            if (video.m_segmentPassed && video.m_frameCount < 15)
                for (int i = 0; i < 15 - video.m_segmentCount; i++)
                    video.m_vidSaver.write(video.m_frame);
            //else if (video.m_frameCount < 15)
                //video.m_vidSaver.write(video.m_frame);
        }

        if (video.m_segmentPassed)
            video.m_frameCount = 0;

        video.m_segmentPassed = false;
    }

    bool updateVideoSaver(VideoStream& video)
    {
        if (m_currentVideoHour < video.m_hour || (m_currentVideoHour == 23 && video.m_hour == 0) || !video.m_vidSaver.isOpened())
        {
            m_currentVideoHour = video.m_hour;

            std::string videoPath = std::to_string(video.m_year) + "-" + std::to_string(video.m_month) + "-" + std::to_string(video.m_day) + " " + std::to_string(video.m_hour);
            

            std::string newVideoPath = videoPath + ".avi";
            LPCSTR lVideoPath = newVideoPath.c_str();
            
            int counter = 0;
            while (GetFileAttributesA(lVideoPath) != 0xffffffff)
            {
                newVideoPath = videoPath + "-" + std::to_string(++counter) + ".avi";
                lVideoPath = newVideoPath.c_str();
                if (counter > 100)
                    return false;
            }

            if (counter != 0)
                videoPath = newVideoPath;//*/
            else
                videoPath += ".avi";

            if (!video.m_vidSaver.open(videoPath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 15.0, cv::Size(video.m_frame.cols, video.m_frame.rows), true));//CV_FOURCC_MACRO('M', 'J', 'P', 'G')
            {
                log("failed to open video file, FALSE ACCUALLY IT WAS SUCCESFUL");

                if (!execute("INSERT INTO video VALUES('" + std::to_string(video.m_year) + "-" + std::to_string(video.m_month) + "-" + std::to_string(video.m_day) + " " + std::to_string(video.m_hour) + ":00:00', " + std::to_string(counter) + ", '" + videoPath + "');"))
                {
                    log(mysql_error(m_mysql));
                    return false;
                }

                return true;
            }

            log("created another file");
        }

        return true;
    }

    void updateArduinoAndVideoDataStorage()
    {
        m_timer.updateLocalTime();
        tm tNow = m_timer.getLocalTime();

        std::string yesterdaysYear = "";
        std::string yesterdaysMonth = "";
        std::string yesterdaysDay = "";

        if (tNow.tm_mday - 14 > 0)
        {
            yesterdaysYear = std::to_string(tNow.tm_year + 1900);
            yesterdaysMonth = std::to_string(tNow.tm_mon + 1);
            yesterdaysDay = std::to_string(tNow.tm_mday - 14);
        }
        else
        {
            int pMonth = tNow.tm_mon;
            if (pMonth < 1)
            {
                yesterdaysYear = std::to_string(tNow.tm_year + 1899);
                yesterdaysMonth = std::to_string(12);
                yesterdaysDay = std::to_string(31);
            }
            else
            {
                int pYear = tNow.tm_year + 1900;

                yesterdaysYear = std::to_string(pYear);
                yesterdaysMonth = std::to_string(pMonth);
                if (pMonth != 2)
                    yesterdaysDay = std::to_string(pMonth % 2 == 1 ? (31 + (tNow.tm_mday - 14)) : (30 + (tNow.tm_mday - 14)));
                else if (!(pYear % 4 == 0 && (pYear % 100 != 0 || pYear % 400 == 0)))
                    yesterdaysDay = std::to_string(28 + (tNow.tm_mday - 14));
                else
                    yesterdaysDay = std::to_string(29 + (tNow.tm_mday - 14));
            }
        }


        // DELETE ARDUINO DATA FROM 14 DAYS AGO
        if (!execute("DELETE FROM panj WHERE DATE(id_zapisa) <= '" + yesterdaysYear + "-" + yesterdaysMonth + "-" + yesterdaysDay + "';"))
            log("error in updating arduino data storage");


        // DELETE VIDEO DATA FROM DATABASE
        if (!execute("DELETE FROM video WHERE DATE(id_videa) <= '" + yesterdaysYear + "-" + yesterdaysMonth + "-" + yesterdaysDay + "';"))
            log("error in updating video data storage");

        // DELETE VIDEO DATA FROM 14 DAYS AGO
        for (int i = 0; i < 24; i++)
        {
            std::string fileName = yesterdaysYear + "-" + yesterdaysMonth + "-" + yesterdaysDay + " " + std::to_string(i);

            if (remove(fileName.c_str()))
            {
                log("error in deleting file: " << fileName);
            }

            std::string temp;
            for (int i = 1; i <= 100; i++)
            {
                temp = fileName + "-" + std::to_string(i);
                if (!remove(temp.c_str()))
                    break;
            }
        }

    }

private:
    bool execute(std::string query)
    {
        if (mysql_query(m_mysql, query.c_str()))//"INSERT INTO Test(polje) VALUES('text1');"
        {
            log("mysql query failed");
            log(mysql_error(m_mysql));
            return false;
        }

        mysql_store_result(m_mysql);
        return true;
    }

private:
    int m_currentVideoHour = -1;// SO IT STARTS EVEN IF LAUNCHED AT MIDNIGHT

private:
    Timer m_timer;

private:
    MYSQL* m_mysql;

    HANDLE m_hPipe;

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
        if (milliSec)
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

    net::endpoint client;

    AutoRestart autoRestart(0, 0);


    Database db;
    if (!db.init("root", "Admin123"))
    {
        log("database init failed");
        return -1;
    }

    ArduinoSystem arduino;
    VideoStream video;

    int hour = 0;
    int min = 0;

    std::thread autoRestartThread(&AutoRestart::checkForRestart, &autoRestart);

    db.updateArduinoAndVideoDataStorage();

    bool shouldRestart = false;
    bool shouldRun = true;
    while (shouldRun)
    {
        shouldRestart = autoRestart.shouldRestart();
        shouldRun = !shouldRestart;

        if (!shouldRestart)
        {
            /*if (client.soc == 0)
            {
                video.reset();
            }//*/

            while (net::isNewConnection())
            {
                client = net::acceptConnection();
            }

            if (net::ismsg(client))
            {
                net::Message message = net::tcprecv(client);
                net::SubMessage sm; std::string ss;
                (sm << message) >> ss;

                if (ss == "ARD:")
                {
                    //log("\nARDUINO OUTPUT\n");

                    /*(sm << message) > net::intArr(*arduinoData, 7);
                    (sm << message) > hour > min;//*/
                    //(((sm << message) > net::intArr(*arduinoData, 7)) << message) > hour > min;
                    //log("\n" << hour << ":" << min << ", temperatura 1: " << arduinoData[0] << ", temperatura 2: " << arduinoData[1] << ", svetlost: " << arduinoData[3] << ", dez: " << arduinoData[4] << ", vlaga: " << arduinoData[5] << ", teza: " << arduinoData[2] << ", zvok: " << arduinoData[6] << "\n");
                    arduino.processData(message);
                    db.storeArduinoData(arduino);
                }
                else if (ss == "VID:")
                {
                    /*char* arr;
                    int size;
                    ((sm << message) > size);
                    (sm << message) >> (char*&)arr;
                    (sm << message) > hour > min;

                    std::vector<uchar> buf(arr, &arr[size - 1]);
                    cv::Mat m_frame = cv::imdecode(buf, 1);
                    buf.empty();
                    delete[] arr;
                    cv::imshow("test", m_frame);
                    cv::waitKey(1);

                    net::Message ack;
                    ack << true;

                    net::tcpsend(client, ack);
                    ack.destory();//*/

                    video.processData(message);
                    db.updateVideoSaver(video);
                    db.processVideoData(video);
                }
                else if (ss == "CON:")
                {
                    video.handleSegment();
                    db.fillMissingFrames(video);// USEFULL WHEN YOU MOVE THE CODE FROM processVideoData INTO THIS FUNCTION
                    video.sendConfirmation(client);
                }

                message.destory();

            }
        }
        else
        {
            net::disconnect(client);
        }
    }

    db.release();

    autoRestart.stopRestartTherad();// MAYBE NOT REQUIRED
    autoRestartThread.join();

    net::release();
}

/*
        m_timer.updateLocalTime();
        tm tNow = m_timer.getLocalTime();

        std::string yesterdaysYear = "";
        std::string yesterdaysMonth = "";
        std::string yesterdaysDay = "";

        if (tNow.tm_mday - 1 > 0)
        {
            yesterdaysYear = std::to_string(tNow.tm_year + 1900);
            yesterdaysMonth = std::to_string(tNow.tm_mon + 1);
            yesterdaysDay = std::to_string(tNow.tm_mday - 1);
        }
        else
        {
            int pMonth = tNow.tm_mon - 1;
            if (pMonth > 0)
            {
                yesterdaysYear = std::to_string(tNow.tm_year + 1899);
                yesterdaysMonth = std::to_string(12);
                yesterdaysDay = std::to_string(31);
            }
            else
            {
                int pYear = tNow.tm_year + 1900;

                yesterdaysYear = std::to_string(tNow.tm_year + 1900);
                yesterdaysMonth = std::to_string(pMonth);
                if(pMonth != 2)
                    yesterdaysDay = std::to_string(pMonth % 2 == 1 ? 32 : 30);
                else if(!(pYear % 4 == 0 && (pYear % 100 != 0 || pYear % 400 == 0)))
                    yesterdaysDay = std::to_string(28);
                else
                    yesterdaysDay = std::to_string(29);
            }
        }
//*/