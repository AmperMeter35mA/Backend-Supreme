#pragma once

#include "pch.h"

#include "TCPContex.h"

namespace net
{

	class TCPFile
	{
	public:
		void sendFile(endpoint ep, std::string fpath);
		void recvFile(endpoint ep, std::string fpath);
	};

}