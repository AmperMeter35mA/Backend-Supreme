#include "pch.h"
#include "TCPContex.h"
#include "TCPFile.h"


namespace net
{
	class Net
	{
	public:
		Net();
		~Net();
		
		static bool Create(int tcpDefaultport);
		static void Release();
		static Net& Get();
	
	private:
		static Net* s_netInstance;

	public:
		TCPContex tcp;
		//UDPContex udp;
		TCPFile tcpfile;
	};

	bool init(int tcpDefaultport);//int udpDefaultport
	void release();
	void async();

	bool connectto(endpoint& endpoint);
	void disconnect(endpoint& endpoint);

	bool isNewConnection();

	endpoint acceptConnection();
	void declineConnection();

	bool ismsg(endpoint& endpoint);
	bool tcpsend(endpoint& endpoint, Message message);
	Message tcprecv(endpoint& endpoint);
	bool tcprecv(endpoint& endpoint, Message& message);

	Message async_tcprecv(endpoint& endpoint);
	bool async_tcprecv(endpoint& endpoint, Message& message);

	void sendFile(endpoint ep, std::string fpath);

}