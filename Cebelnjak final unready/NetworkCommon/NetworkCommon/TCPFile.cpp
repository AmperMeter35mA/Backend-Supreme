#include "TCPFile.h"
#include "Net.h"

namespace net
{

    //this needs to be callback/request send

    void net::TCPFile::sendFile(endpoint ep, std::string fpath)
    {
        std::ifstream file;
        file.open(fpath, std::ios::in | std::ios::ate | std::ios::binary);// | std::ios::binary//"files/" + chat.substr(5, chat.size())
        int size = file.tellg();
        file.seekg(0, std::ios::beg);

        {
            net::Message message;
            message << size;
            net::tcpsend(ep, message);
        }

        if (file.is_open())
        {
            const int dataSize = 4096;// 4096;1024
            char data[dataSize];
            int left = size % dataSize;

            for (int i = 0; i < size / dataSize; i++)
            {
                Message message;
                ZeroMemory(data, dataSize);
                file.read(data, dataSize);
                message << charArr(*data, 4096);
                Net::Get().tcp.tcpsend(ep, message);
            }

            if (left != 0)
            {
                Message message;
                ZeroMemory(data, dataSize);
                file.read(data, left);
                message << charArr(*data, left);
                //net::connectto(gameServer);
                Net::Get().tcp.tcpsend(ep, message);
            }

            file.close();
        }
    }

    /*void net::TCPFile::recvFile(endpoint ep, std::string fpath)
    {
        net::Header h = message.getHeader();
        for (int i = 0; i < h.amountOfSubMsg; i++)
        {
            //subMessage << net::getSubMessage(message, id);
            //subHeader = subMessage.getSubHeader();//.type
            //char* buf;// = new char[subHeader.size];
            //subMessage >> buf;//auto memory allocation of full subMessage
            //buf = new char[customSize];
            //subMessage > buf2;//user memory alloction of custom size
            char* buf;
            net::SubMessage sm;
            //sm << message >> buf;
            sm << message;
            net::SubHeader sh = sm.getSubHeader();
            sm >> buf;
            for (int j = 0; j < sh.size; j++)
            {
                //char b = buf[j];
                //logn(buf[j]);//buf[j]);
                file << buf[j];//buf[j];
            }
            file.flush();//very important
        }
    }*/

}