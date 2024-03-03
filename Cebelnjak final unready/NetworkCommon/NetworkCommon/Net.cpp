#include "Net.h"

namespace net
{
	Net* Net::s_netInstance;

	Net::Net()
	{
	}

	Net::~Net()
	{
	}

	bool Net::Create(int tcpDefaultport)
	{
		s_netInstance = new Net();
		if (!NetCom::init())
		{
			debugerr("NetCom init failed");

			return false;
		}
		//Net::Get().tcp.init(tcpDefaultport);//off when in client mode
	}

	void Net::Release()
	{
		delete s_netInstance;
	}

	Net& Net::Get()
	{
		return *s_netInstance;
	}

	bool init(int tcpDefaultport)
	{
		return Net::Create(tcpDefaultport);
	}

	void release()
	{
		Net::Release();
	}

	bool connectto(endpoint& endpoint)
	{

		return Net::Get().tcp.connectto(endpoint);
	}

	void disconnect(endpoint& endpoint)
	{
		Net::Get().tcp.disconnect(endpoint);
	}

	bool isNewConnection()
	{
		return  Net::Get().tcp.isnewconn();
	}

	endpoint acceptConnection()
	{
		return Net::Get().tcp.acceptconnection();
	}

	void declineConnection()
	{
		Net::Get().tcp.declineconnection();
	}

	bool ismsg(endpoint& endpoint)
	{
		return Net::Get().tcp.isDataAvailable(endpoint);
	}

	bool tcpsend(endpoint& endpoint, Message message)
	{
		return Net::Get().tcp.tcpsend(endpoint, message);
		//return the return value
		//return true;
	}

	Message tcprecv(endpoint& endpoint)
	{
		Message message;
		if (Net::Get().tcp.tcprecv(endpoint, message, false))
		{
			return message;
		}

		return message;
	}

	bool tcprecv(endpoint& endpoint, Message& message)
	{
		return Net::Get().tcp.tcprecv(endpoint, message, false);
	}

	Message async_tcprecv(endpoint& endpoint)
	{
		Message message;
		if (Net::Get().tcp.tcprecv(endpoint, message, true))
		{
			return message;
		}

		return message;
	}

	bool async_tcprecv(endpoint& endpoint, Message& message)
	{
		return Net::Get().tcp.tcprecv(endpoint, message, true);
	}

	void sendFile(endpoint ep, std::string fpath)
	{
		Net::Get().tcpfile.sendFile(ep, fpath);
	}

}
