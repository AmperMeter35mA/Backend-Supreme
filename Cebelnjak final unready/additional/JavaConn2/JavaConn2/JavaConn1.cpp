#include "pch.h"

#include "JavaConn1.h"



HANDLE hPipe;
char buffer[4097];
DWORD dwRead;

bool connect()
{
	//check PIPE_TYPE_MESSAGE on https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createnamedpipea
	// FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1024 * 16, 1024 * 16, NMPWAIT_USE_DEFAULT_WAIT, NULL);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	
	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				/* add terminating zero */
				buffer[dwRead] = '\0';//but is is really required?
				
				std::string test;
				for (int i = 0; i < dwRead - 1; i++)
				{
					test += buffer[i];
				}

				if (test == "connected\n")
				{
					return true;
				}
			}
		}
	}

	return false;
}

void disconnect()
{
	DisconnectNamedPipe(hPipe);
}




JNIEXPORT jboolean JNICALL Java_main_Main_connect(JNIEnv*, jclass)
{	
	return (jboolean)connect();
}