#include "TCPContex.h"

namespace net
{

	TCPContex::TCPContex()
	{
	}

	TCPContex::~TCPContex()
	{
		m_canAccept = false;
		release();
		m_acceptThread.join();
	}

	bool TCPContex::init(int acceptPort)
	{
		if (!initAccept(acceptPort)) return false;

		return true;
	}

	bool TCPContex::release()
	{
		closesocket(m_acceptPoint.soc);

		return true;
	}

	bool TCPContex::connectto(endpoint& ep)
	{
		Point p = Point(createuser(ep.ip, ep.port), tcpsocket());
		if (connect(p.soc, (sockaddr*)&p.user, sizeof(p.user)))
		{
			debugerr("erorr in connecting to endpoint: " << WSAGetLastError());
			socketclose(p.soc);
			ep.soc = 0;
			return false;
		}
		else
		{
			m_connectedto.push_back(p);
			ep.soc = p.soc;
		}

		return true;
	}

	void TCPContex::disconnect(endpoint& ep)
	{
		for (int i = 0; i < m_connectedto.size(); i++)
		{
			if (m_connectedto.at(i).soc == ep.soc)
			{
				m_connectedto.erase(m_connectedto.begin() + i);
			}
		}
		socketclose(ep.soc);
		ep.soc = 0;
	}

	bool TCPContex::ismsg(endpoint endpoint)
	{
		char buf[1];
		//MSG_PEEK	Peeks at the incoming data. The data is copied into the buffer, but is not removed from the input queue.
		//MSG_WAITALL	The receive request will complete only when one of the following events occurs: The buffer supplied by the caller is completely full...
		int bytesRecieved = recv(endpoint.soc, buf, 0, MSG_PEEK | MSG_WAITALL);//0
		if (bytesRecieved > 0)
		{
			return true;
		}
		else if (bytesRecieved == SOCKET_ERROR)
		{
			debugerr("error recieving from endpoint in check: " << WSAGetLastError());
			disconnect(endpoint);
			return false;
		}

		return true;
	}

	//stolen code
	//https://stackoverflow.com/questions/55384591/how-to-check-if-there-are-available-data-to-read-before-to-call-recv-in-a-blocki
	bool TCPContex::isDataAvailable(endpoint& ep)
	{
		Socket socket = ep.soc;

		if (socket == 0)
			return false;

		//error always return true if connection is closed
		//fix by returning if conn is closed: https://stackoverflow.com/questions/12402549/check-if-socket-is-connected-or-not
		//new bug? what if other program opens on that socket ???
		//http://www.on-time.com/rtos-32-docs/rtip-32/reference-manual/socket-api/select.htm

		fd_set sready;
		struct timeval nowait;

		FD_ZERO(&sready);
		FD_SET((unsigned int)socket, &sready);
		memset((char*)&nowait, 0, sizeof(nowait));

		bool res = select(socket + 1, &sready, NULL, NULL, &nowait);
		
		if (FD_ISSET(socket, &sready))
		{
			char size;
			int res2 = recv(ep.soc, &size, 1, MSG_PEEK);
			if (res2 < 1)
			{
				if (res2 == SOCKET_ERROR)
					disconnect(ep);

				return false;
			}

			return true;
		}
		else
			return false;
	}

	bool TCPContex::tcpsend(endpoint& ep, Message msg)
	{
		if (ep.soc == 0)
			return false;

		int i_aosm = msg.header->amountOfSubMsg;
		int tot_size = msg.header->size;

		int buff_size = 33 + 32 * i_aosm + tot_size;
		char* buffer = new char[buff_size];

		buffer[buff_size - 1] = '\0';

		std::string aosm = std::bitset<32>(i_aosm).to_string();
		memcpy(buffer, (void*)aosm.data(), 32);

		int counter = 32;
		for (int i = 0; i < i_aosm; i++)
		{
			int sosm = msg.message->at(i).subMessage->first.size;
			std::string aosm = std::bitset<32>(sosm).to_string();
			memcpy(&buffer[counter], (void*)aosm.data(), 32);
			counter += 32;

			memcpy(&buffer[counter], (void*)msg.message->at(i).subMessage->second.data(), sosm);
			counter += sosm;
		}

		if (send(ep.soc, buffer, buff_size, 0) == SOCKET_ERROR)
		{
			debugerr("couldnt send segment: " << WSAGetLastError());
			disconnect(ep);
			return false;
		}

		delete[] buffer;

		return true;
	}

	bool TCPContex::tcpsend2(endpoint ep, Message message)//ORIGINAL ONE
	{
		if (ep.soc == -1)
			return false;

		int amountOfSubMsg = message.getHeader().amountOfSubMsg;

		std::string m = std::bitset<32>(amountOfSubMsg).to_string();
		const char* mm = m.c_str();
		if (send(ep.soc, mm, 32, 0) == SOCKET_ERROR)
		{
			debugerr("couldnt send segment: " << WSAGetLastError());
			return false;
		}

		for (int i = 0; i < message.getHeader().amountOfSubMsg; i++)
		{
			auto s = message.message->at(0).subMessage;
			int size = s->first.size;
			const int newSize = size + 1 + 32;
			char* buffer = new char[newSize];
			ZeroMemory(buffer, newSize);
			memcpy(&buffer[32], s->second.data(), size);
			buffer[newSize - 1] = '\0';

			//set first 32 bytes to bitset of size of user data
			std::string n = std::bitset<32>(s->second.size()).to_string();
			const char* ss = n.c_str();
			memcpy(buffer, ss, 32);

			if (send(ep.soc, buffer, newSize, 0) == SOCKET_ERROR)
			{
				debugerr("couldnt send segment: " << WSAGetLastError());
				return false;
			}

			delete[] buffer;
			delete s;
			message.message->erase(message.message->begin() + 0);
		}

		delete message.header;
		delete message.message;//still possibly error

		return true;
	}

	//http://www.on-time.com/rtos-32-docs/rtip-32/reference-manual/socket-api/recv.htm
	bool TCPContex::TCPContex::tcprecv(endpoint& ep, Message& message, bool async)
	{
		//if async mode is enabled, delete this line
		if (!isDataAvailable(ep) && !async)
			return false;

		char carr_aosm[33];
		carr_aosm[32] = '\0';
		int brcv = recv(ep.soc, carr_aosm, 32, MSG_WAITALL);

		if (brcv > 0)
		{
			int size = 0;
			int i_aosm = std::bitset<32>(carr_aosm).to_ulong();
			char* buffer;

			for (int i = 0; i < i_aosm; i++)
			{
				brcv = recv(ep.soc, carr_aosm, 32, MSG_WAITALL);
				size = std::bitset<32>(carr_aosm).to_ulong();

				buffer = new char[size];
				brcv = recv(ep.soc, buffer, size, MSG_WAITALL);

				if (brcv > 0)
				{
					message << net::charArr(*buffer, size);
				}
				else if (brcv == SOCKET_ERROR)
				{
					debug("error recieving from endpoint: " << WSAGetLastError());
					disconnect(ep);
					return false;
				}

				delete[] buffer;
			}
		}
		else if (brcv == 0 || brcv == SOCKET_ERROR)
		{
			brcv == 0 ? std::cout << "client disconnected" << std::endl : debug("error recieving from endpoint: " << WSAGetLastError());
			disconnect(ep);
			return false;
		}

		char temp;
		recv(ep.soc, &temp, 1, MSG_WAITALL);

		return true;
	}

	bool TCPContex::tcprecv2(endpoint& ep, Message& message)//ORIGINAL ONE
	{//first time error - nope
		if (isDataAvailable(ep))//deal with this//add a bool for if user want blocking or non blocking
		{
			//MSG_PEEK	Peeks at the incoming data. The data is copied into the buffer, but is not removed from the input queue.
			//MSG_WAITALL	The receive request will complete only when one of the following events occurs: The buffer supplied by the caller is completely full...
			Message* msg = new Message();
			char* buf;
			char size[33];
			int bytesRecieved = recv(ep.soc, size, 32, MSG_WAITALL);//MSG_PEEK//linux has MSG_DONTWAIT=?0?---?ioctlsocket()?https://stackoverflow.com/questions/29010214/winsock-msg-dontwait-equivalent
			//!!!!!!! on conn end its -1 and causes error
			if (bytesRecieved > 0)
			{
				size[32] = '\0';
				std::string bb = size;
				std::bitset<32> b(bb);
				int bytes = b.to_ulong();//!!!!!!! on conn end its -1 and causes error
				int amountOfMsg = bytes;
				for (int i = 0; i < amountOfMsg; i++)
				{
					bytesRecieved = recv(ep.soc, size, 32, MSG_WAITALL);//MSG_PEEK//linux has MSG_DONTWAIT=?0?---?ioctlsocket()?https://stackoverflow.com/questions/29010214/winsock-msg-dontwait-equivalent
					size[32] = '\0';
					bb = size;
					std::bitset<32> bm(bb);
					bytes = bm.to_ulong();
					buf = new char[bytes];

					bytesRecieved = recv(ep.soc, buf, bytes, MSG_WAITALL);//0//linux has MSG_DONTWAIT=?0?---?ioctlsocket()?https://stackoverflow.com/questions/29010214/winsock-msg-dontwait-equivalent
					if (bytesRecieved > 0)
					{
						*msg << charArr(*buf, bytes);//cannot pass array test code was not tested proprely, not possible to work
						recv(ep.soc, buf, 1, MSG_WAITALL);
						delete[] buf;

						//return true;
					}
					else if (bytesRecieved == SOCKET_ERROR)
					{
						debug("error recieving from endpoint: " << WSAGetLastError());
						disconnect(ep);
						delete[] buf;
						return false;
					}
				}
			}
			else if (bytesRecieved == 0 || bytesRecieved == SOCKET_ERROR)
			{
				bytesRecieved == 0 ? std::cout << "client disconnected" << std::endl : debug("error recieving from endpoint: " << WSAGetLastError());
				disconnect(ep);
				return false;
			}
			message = *msg;
		}

		return true;
	}

	bool TCPContex::initAccept(int acceptPort)
	{
		m_canAccept = true;

		m_acceptPort = acceptPort;
		m_acceptPoint = Point(createuser(m_acceptPort), tcpsocket());
		socketbind(m_acceptPoint.soc, &m_acceptPoint.user);
		if (listen(m_acceptPoint.soc, SOMAXCONN) == SOCKET_ERROR)
		{
			debugerr("listening socket error: " << WSAGetLastError());
			return false;
		}
		m_acceptThread = std::thread(&TCPContex::handleConAccept, this);

		return true;
	}

	void TCPContex::handleConAccept()
	{
		while (m_canAccept)
		{
			Point p = Point(createuser(), 0);
			int size = sizeof(p.user);
			p.soc = accept(m_acceptPoint.soc, (sockaddr*)&p.user, &size);
			if (p.soc == INVALID_SOCKET)
			{
				debugerr("error creating socket or terminating accept thread: " << WSAGetLastError());
			}
			m_newconnections.push_back(p);
		}
	}

	endpoint TCPContex::acceptconnection()
	{
		auto p = m_newconnections.at(0);
		m_newconnections.erase(m_newconnections.begin() + 0);
		m_connectedto.push_back(p);

		p = m_connectedto.back();

		char clientIp[NI_MAXHOST];
		char clientPort[NI_MAXSERV];
		ZeroMemory(clientIp, NI_MAXHOST);
		ZeroMemory(clientPort, NI_MAXSERV);
		getnameinfo((sockaddr*)&p.user, sizeof(p.user), clientIp, NI_MAXHOST, clientPort, NI_MAXSERV, 0);

		debugx("connected with: " << clientIp << ":" << clientPort);

		endpoint ep(std::string(clientIp), std::stoi(std::string(clientPort)));
		ep.soc = p.soc;

		return ep;
	}

	void TCPContex::declineconnection()
	{
		auto& p = m_newconnections.at(0);
		m_newconnections.erase(m_newconnections.begin() + 0);
		socketclose(p.soc);
	}

}
