#include "NetCom.h"

namespace net
{

	typedef sockaddr_in User;
	typedef SOCKET Socket;

	NetCom::NetCom()
	{
	}

	NetCom::~NetCom()
	{
	}

	WSADATA NetCom::m_data;
	WORD NetCom::m_version;
	bool NetCom::isInit = false;
	bool NetCom::init()
	{
		if (!isInit)
		{
			m_version = MAKEWORD(2, 2);
			if (WSAStartup(m_version, &m_data))
			{
				debugerr("cannot start WSA");
				return false;
			}
		}

		return true;
	}

	bool NetCom::release()
	{
		WSACleanup();
		isInit = false;

		return true;
	}

	Socket NetCom::tcpsocket()
	{
		Socket soc = socket(AF_INET, SOCK_STREAM, 0);

		if (soc == INVALID_SOCKET)
		{
			debugerr("error creating socket: " << WSAGetLastError());
			return NULL;
		}

		return soc;
	}

	Socket NetCom::udpsocket()
	{
		Socket soc = socket(AF_INET, SOCK_DGRAM, 0);

		if (soc == INVALID_SOCKET)
		{
			debugerr("error creating socket: " << WSAGetLastError());
			return NULL;
		}

		return soc;
	}

	void NetCom::socketclose(Socket soc)
	{
		debug("connection ended");
		closesocket(soc);
	}

	User NetCom::createuser()
	{
		User user;
		user.sin_family = AF_INET;

		return user;
	}

	User NetCom::createuser(int port)
	{
		User user;
		user.sin_family = AF_INET;
		user.sin_port = htons(port);
		user.sin_addr.S_un.S_addr = ADDR_ANY;


		return user;
	}

	User NetCom::createuser(std::string ip, int port)
	{
		User user;
		user.sin_family = AF_INET;
		user.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &user.sin_addr);

		return user;
	}

	bool NetCom::tcpconnect(Socket soc, User* endpointuser)
	{
		if (connect(soc, (sockaddr*)endpointuser, sizeof(endpointuser)) == SOCKET_ERROR)
		{
			debugerr("erorr in connecting to endpoint: " << WSAGetLastError());
			return false;
		}

		return true;
	}

	bool NetCom::socketbind(Socket soc, User* user)
	{
		if (bind(soc, (sockaddr*)user, sizeof(*user)) == SOCKET_ERROR)
		{
			debugerr("erorr in binding socket " << WSAGetLastError());
			return false;
		}

		return true;
	}


	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------------------------------------------


	SubMessage& SubMessage::operator<<(charArr buf)
	{
		int size = buf.arr[1];
		subMessage->first.size = size;
		subMessage->second.resize(size);
		memcpy(subMessage->second.data(), (char*)buf.arr[0], size);
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

		//IDK
		delete subMessage.subMessage;
		//subMessage.subMessage->second.clear();

		subMessage = message->at(0);

		return *this;
	}

	Message& Message::operator<<(SubMessage& subMessage)
	{
		return *this << subMessage.subMessage->second.data();

		//return *this;
	}


	Message& Message::operator<<(charArr buf)
	{
		int size = buf.arr[1];
		header->size += size;
		header->amountOfSubMsg++;
		message->push_back(SubMessage());
		auto& msg = message->back().subMessage;
		msg->first.size = size;
		msg->second.resize(size);
		memcpy(msg->second.data(), (char*)buf.arr[0], size);
		//assert;

		return *this;
	}

	Message& Message::operator<(charArr buf)
	{
		int size = buf.arr[1];
		header->size += size;
		auto& msg = message->back().subMessage;
		int prevSize = msg->first.size;
		msg->first.size += size;
		msg->second.resize(msg->first.size);
		memcpy(msg->second.data() + prevSize, (char*)buf.arr[0], size);
		//assert;

		return *this;
	}

	Message& Message::operator<<(intArr buf)
	{
		int size = buf.arr[1];
		*this << ((int*)buf.arr[0])[0];
		for (int i = 1; i < size ; i++)
		{
			*this < ((int*)buf.arr[0])[i];
		}

		return *this;
	}

	Message& Message::operator<(intArr buf)
	{
		int size = buf.arr[1];
		for (int i = 0; i < size; i++)
		{
			*this < ((int*)buf.arr[0])[i];
		}

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

	SubMessage& SubMessage::operator>(charArr buf)
	{
		auto& v = subMessage->second;
		int size = buf.arr[1];
		memcpy((char*)buf.arr[0], v.data(), size);
		v.erase(v.begin(), v.begin() + size);
		subMessage->first.size -= size;

		return *this;
	}

	SubMessage& SubMessage::operator>(intArr buf)
	{
		int size = buf.arr[1];

		for (int i = 0; i < size; i++)
		{
			*this > ((int*)buf.arr[0])[i];
		}

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

	SubMessage& SubMessage::operator>(stringArr buf)
	{
		auto& v = subMessage->second;
		int size = buf.arr[1];
		//memcpy(&buf, v.data(), size);
		std::string& blyat = *((std::string*)buf.arr[0]);
		/*((std::string*)buf.arr[0])->*/blyat.resize(size);
		auto d = v.data();
		for (int i = 0; i < size; i++) blyat[i] = d[i];//((std::string*)buf.arr[0])
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
		std::string s = std::bitset<32>(buf).to_string();
		const char* ss = s.c_str();

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
	}

}