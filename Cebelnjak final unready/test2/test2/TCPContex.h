#pragma once

#include "pch.h"

#include "NetCom.h"

namespace net
{
	class TCPContex : public NetCom
	{
	public:
		TCPContex();
		~TCPContex();

		bool init(int acceptPort);
		bool release();

	public:
		bool connectto(endpoint& ep);
		void disconnect(endpoint& ep);

		bool ismsg(endpoint endpoint);
		bool isDataAvailable(endpoint& ep);

		bool tcpsend2(endpoint ep, Message msg);//--------------------------------------------TEST
		bool tcpsend(endpoint& ep, Message message);
		bool tcprecv2(endpoint& ep, Message& message);//---------------------------------------TEST
		bool tcprecv(endpoint& ep, Message& message, bool async);//returns size of message

		endpoint acceptconnection();
		void declineconnection();

		bool isnewconn()
		{
			if (m_newconnections.size() != 0) return true;
			return false;
		}

	private:
		bool initAccept(int acceptPort);
		void handleConAccept();

	private:
		int m_acceptPort;

		struct Point
		{
			Point() {}
			Point(User u, Socket s)
				: user(u), soc(s) {}

			User user;
			Socket soc;
		} m_acceptPoint;

		std::thread m_acceptThread;

		//connections
		std::vector<Point> m_newconnections;
		//interthread communication
		bool m_canUse = false;
		bool m_canAccept = false;

	private:
		std::vector<Point> m_connectedto;
	};
}
