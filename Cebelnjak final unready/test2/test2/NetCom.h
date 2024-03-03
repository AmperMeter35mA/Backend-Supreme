#pragma once

#include "pch.h"

namespace net
{
	class NetCom
	{
	public:
		NetCom();
		~NetCom();

		typedef sockaddr_in User;
		typedef SOCKET Socket;

		static bool init();
		static bool release();

	protected:
		Socket tcpsocket();
		Socket udpsocket();
		void socketclose(Socket soc);

		User createuser();
		User createuser(int port);
		User createuser(std::string ip, int port);

		bool tcpconnect(Socket soc, User* endpointuser);
		bool socketbind(Socket soc, User* user);
		//no known way to disconnect or unbind

	private:
		static WSADATA m_data;
		static WORD m_version;
		static bool isInit;
	};

#define ARCHx64
#ifndef ARCHx64
	struct charArr
	{
		charArr(char& a, int size) { arr[0] = (int)&a; arr[1] = size; }
		int arr[2];
	};

	struct intArr
	{
		intArr(int& a, int size) { arr[0] = (int)&a; arr[1] = size; }
		int arr[2];
	};

	struct stringArr
	{
		stringArr(std::string& a, int size) { arr[0] = (int)&a; arr[1] = size; }
		int arr[2];
	};

	struct floatArr
	{
		floatArr(float& a, int size) { arr[0] = (int)&a; arr[1] = size; }
		int arr[2];
	};

	struct boolArr
	{
		boolArr(bool& a, int size) { arr[0] = (int)&a; arr[1] = size; }
		int arr[2];
	};
#else
	struct charArr
	{
		charArr(char& a, int size) { arr[0] = (long long)&a; arr[1] = (long long)size; }
		long long arr[2];
	};

	struct intArr
	{
		intArr(int& a, int size) { arr[0] = (long long)&a; arr[1] = (long long)size; }
		long long arr[2];
	};

	struct stringArr
	{
		stringArr(std::string& a, int size) { arr[0] = (long long)&a; arr[1] = (long long)size; }
		long long arr[2];
	};

	struct floatArr
	{
		floatArr(float& a, int size) { arr[0] = (long long)&a; arr[1] = (long long)size; }
		long long arr[2];
	};

	struct boolArr
	{
		boolArr(bool& a, int size) { arr[0] = (long long)&a; arr[1] = (long long)size; }
		long long arr[2];
	};
#endif



	typedef sockaddr_in User;
	typedef SOCKET Socket;

	struct endpoint
	{
		endpoint() {}
		endpoint(std::string i, int p)
			: ip(i), port(p) {}

		std::string ip;
		int port;
		Socket soc = 0;
	};



	struct Header
	{
		unsigned int size = 0;//might only be useful for the system not user
		int amountOfSubMsg = 0;//*SubMsg
		int amountOfNormalMsg = 0;
		//char crc = 0;//crc in udp? or check for if a new msg arrive
	};

	struct SubHeader
	{
		int size = 0;
		//type maybe
		//id maybe
	};

	class TCPcontext;
	class Message;
	class SubMessage
	{
	public:
		SubMessage(charArr buf)
		{
			subMessage = new std::pair<SubHeader, std::vector<char>>();
			*this << buf;
		}
		SubMessage()
		{
			subMessage = new std::pair<SubHeader, std::vector<char>>();
		}
		~SubMessage()
		{

		}

		SubMessage& operator<<(charArr buf);//for test
		SubMessage& operator<<(Message& message);
		SubMessage& operator<<(SubMessage& subMessage);

		SubMessage& operator>>(char*& buf);
		//SubMessage& operator>(char buf[]);
		SubMessage& operator>(charArr buf);

		SubMessage& operator>>(std::string& buf);
		SubMessage& operator>(stringArr buf);

		SubMessage& operator>(intArr buf);

		SubMessage& operator>>(int* buf);
		SubMessage& operator>(int& buf);
		SubMessage& operator>>(float* buf);
		SubMessage& operator>(float& buf);
		SubMessage& operator>>(bool* buf);
		SubMessage& operator>(bool& buf);

		SubHeader getSubHeader()
		{
			return subMessage->first;
		}

	private:
		std::pair<SubHeader, std::vector<char>>* subMessage;

		friend class Message;
		friend class TCPContex;
	};

	class Message
	{
	public:
		Message()
		{
			header = new Header();
			message = new std::vector<SubMessage>();
		}
		~Message()
		{
			//delete[] message;
		}

		Header getHeader()
		{
			return *header;
		}

		void destory()
		{
			delete header;
			for (int i = 0; i < message->size(); i++)
			{
				message->at(i).subMessage->second.clear();
				delete message->at(i).subMessage;
			}
			message->clear();
			delete message;
		}

		Message& operator>>(SubMessage& subMessage);
		Message& operator<<(SubMessage& subMessage);

		//main
		Message& operator<<(charArr buf);
		Message& operator<(charArr buf);


		Message& operator<<(intArr buf);
		Message& operator<(intArr buf);//implement


		Message& operator<<(const char* buf);
		Message& operator<(const char* buf);


		Message& operator<<(int buf);
		Message& operator<(int buf);
		Message& operator<<(float buf);
		Message& operator<(float buf);
		Message& operator<<(bool buf);
		Message& operator<(bool buf);


	private:
		Header* header;
		std::vector<SubMessage>* message;

		friend class SubMessage;
		friend class TCPContex;
	};
/*
	SubMessage& SubMessage::operator<<(char buf[])
	{
		int size = sizeof(buf) - 1;
		subMessage->first.size = size;
		subMessage->second.resize(size);
		memcpy(subMessage->second.data(), buf, size);
		//assert;

		return *this;
	}

	SubMessage& SubMessage::operator<<(Message& message)
	{
		message >> *this;
		return *this;
	}

	SubMessage& SubMessage::operator<<(SubMessage& subMessage)
	{
		//do implementation once *this = copy(submessage);

		return *this;
	}

	Message& Message::operator>>(SubMessage& subMessage)
	{
		SubHeader sh = message->at(0).getSubHeader();
		if (sh.size == 0)
		{
			header->amountOfSubMsg--;
			message->erase(message->begin() + 0);
		}

		int count = 0;
		for (int i = 0; i < header->amountOfSubMsg; i++) count += message->at(i).getSubHeader().size;
		header->size -= (header->size - count);

		subMessage = message->at(0);

		return *this;
	}

	Message& Message::operator<<(SubMessage& subMessage)
	{
		return *this << subMessage.subMessage->second.data();

		//return *this;
	}


	Message& Message::operator<<(char buf[])
	{
		int size = sizeof(buf) - 1;
		header->size += size;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = size;
		msg->second.resize(size);
		memcpy(msg->second.data(), buf, size);
		//assert;

		return *this;
	}

	Message& Message::operator<(char buf[])
	{
		int size = sizeof(buf) - 1;
		header->size += size;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size += size;
		msg->second.resize(msg->first.size);
		memcpy(msg->second.data() + prevSize, buf, size);
		//assert;

		return *this;
	}

	SubMessage& SubMessage::operator>>(char*& buf)
	{
		auto& v = subMessage->second;
		int size = v.size();
		buf = new char[size];
		memcpy(buf, v.data(), size);
		v.clear();
		subMessage->first.size -= size;

		return *this;
	}

	Message& Message::operator<<(const char* buf)
	{
		int size = strlen(buf);
		header->size += size;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = size;
		msg->second.resize(size);
		memcpy(msg->second.data(), buf, size);

		return *this;
	}

	Message& Message::operator<(const char* buf)
	{
		int size = strlen(buf);
		header->size += size;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size += size;
		msg->second.resize(msg->first.size);
		memcpy(msg->second.data() + prevSize, buf, size);

		return *this;
	}

	SubMessage& SubMessage::operator>(char* buf)
	{
		auto& v = subMessage->second;
		int size = sizeof(buf) - 1;
		memcpy(buf, v.data(), size);
		v.erase(v.begin(), v.begin() + size);
		subMessage->first.size -= size;

		return *this;
	}

	SubMessage& SubMessage::operator>>(std::string& buf)
	{
		auto& v = subMessage->second;
		int size = v.size();
		buf.resize(size);
		//memcpy(&buf, v.data(), size);
		auto d = v.data();
		for (int i = 0; i < size; i++) buf[i] = d[i];
		v.clear();
		subMessage->first.size -= size;

		return *this;
	}

	SubMessage& SubMessage::operator>(std::string& buf)
	{
		auto& v = subMessage->second;
		int size = buf.size();
		//memcpy(&buf, v.data(), size);
		auto d = v.data();
		for (int i = 0; i < size; i++) buf[i] = d[i];
		v.erase(v.begin(), v.begin() + size);
		subMessage->first.size -= size;
		//assert;

		return *this;
	}

	Message& Message::operator<<(int buf)
	{
		std::string s = std::bitset<32>(buf).to_string();
		const char* ss = s.c_str();

		int size = 32;// strlen(ss);
		header->size += size;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = size;
		msg->second.resize(size);
		memcpy(msg->second.data(), ss, size);

		return *this;
	}

	Message& Message::operator<(int buf)
	{
		//std::string s = std::bitset<32>(buf).to_string();
		const char* ss = std::bitset<32>(buf).to_string().c_str();//s.c_str();

		int size = 32;//strlen(ss);
		header->size += size;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size += size;
		msg->second.resize(msg->first.size);
		memcpy(msg->second.data() + prevSize, ss, size);
		//for (int i = 0; i < ; i++);

		return *this;
	}

	SubMessage& SubMessage::operator>>(int* buf)
	{
		auto& v = subMessage->second;
		int size = v.size();
		buf = new int[size];

		for (int i = 0; i < size; i++)
		{
			auto msg = v.data();
			std::string s; s.resize(32);
			memcpy(&s, msg, 32);
			std::bitset<32> b(s);
			buf[i] = b.to_ulong();
			v.erase(v.begin() + 0, v.begin() + 32);
			subMessage->first.size -= 32;
		}

		return *this;
	}

	SubMessage& SubMessage::operator>(int& buf)
	{
		auto msg = subMessage->second.data();
		std::string s; s.resize(32);
		//assert;
		//memcpy(&s, msg, 32);
		for (int i = 0; i < 32; i++) s[i] = msg[i];
		//assert;
		std::bitset<32> b(s);
		//assert;
		buf = b.to_ulong();
		//assert;
		auto& v = subMessage->second;
		v.erase(v.begin() + 0, v.begin() + 32);
		subMessage->first.size -= 32;

		return *this;
	}

	Message& Message::operator<<(float buf)
	{
		int ibuf = *(int*)&buf;

		std::string s = std::bitset<32>(ibuf).to_string();
		const char* ss = s.c_str();

		int size = strlen(ss);
		header->size += size;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = size;
		msg->second.resize(size);
		memcpy(msg->second.data(), ss, size);

		return *this;
	}

	Message& Message::operator<(float buf)
	{
		int ibuf = *(int*)&buf;

		std::string s = std::bitset<32>(ibuf).to_string();
		const char* ss = s.c_str();

		int size = strlen(ss);
		header->size += size;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size += size;
		msg->second.resize(msg->first.size);
		memcpy(msg->second.data() + prevSize, ss, size);

		return *this;
	}

	SubMessage& SubMessage::operator>>(float* buf)
	{
		auto& v = subMessage->second;
		int size = v.size();
		buf = new float[size];

		for (int i = 0; i < size; i++)
		{
			auto msg = v.data();
			std::string s; s.resize(32);
			//memcpy(&s, msg, 32);
			for (int i = 0; i < 32; i++) s[i] = msg[i];
			std::bitset<32> b(s);
			int bb = b.to_ulong();
			buf[i] = *((float*)(&bb));
			v.erase(v.begin() + 0, v.begin() + 32);
			subMessage->first.size -= 32;
		}

		return *this;
	}

	SubMessage& SubMessage::operator>(float& buf)
	{
		auto msg = subMessage->second.data();
		std::string s; s.resize(32);
		//memcpy(&s, msg, 32);
		for (int i = 0; i < 32; i++) s[i] = msg[i];
		std::bitset<32> b(s);
		int bb = b.to_ulong();
		buf = *((float*)(&bb));
		auto& v = subMessage->second;
		v.erase(v.begin() + 0, v.begin() + 32);
		subMessage->first.size -= 32;

		return *this;
	}

	Message& Message::operator<<(bool buf)
	{
		char b = buf ? '1' : '0';
		header->size++;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = 1;
		msg->second.resize(1);
		*msg->second.data() = b;

		return *this;
	}

	Message& Message::operator<(bool buf)
	{
		char b = buf ? '1' : '0';
		header->size++;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size++;
		msg->second.resize(msg->first.size);
		*(msg->second.data() + prevSize) = b;

		return *this;
	}

	SubMessage& SubMessage::operator>>(bool* buf)
	{
		auto& v = subMessage->second;
		int size = v.size();
		buf = new bool[size];

		for (int i = 0; i < size; i++)
		{
			buf[i] = v.at(0) == 1 ? true : false;
			v.erase(v.begin());
			subMessage->first.size--;
		}

		return *this;
	}

	SubMessage& SubMessage::operator>(bool& buf)
	{
		auto& v = subMessage->second;
		buf = (v.at(0) == '1') ? true : false;
		v.erase(v.begin());
		subMessage->first.size--;

		return *this;
	}*/

}