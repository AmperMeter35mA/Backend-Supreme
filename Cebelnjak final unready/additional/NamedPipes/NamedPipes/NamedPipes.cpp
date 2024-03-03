#include <iostream>
#include <Windows.h>


#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;


int main()
{
    HANDLE hPipe;
    unsigned char data[1024 * 100];
    DWORD count;

    hPipe = CreateFile(TEXT("\\\\.\\pipe\\videopipe"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        log("pipe was null");
        return false;
    }

    while (true)
    {
        //if (ConnectNamedPipe(hPipe, NULL))
        {
            //https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
            if (ReadFile(hPipe, data, 1024 * 100, &count, NULL))
            {
                log("read: " << count);
            }
        }
    }

    /*HANDLE hPipe;
    char buffer[4096];
    DWORD dwRead;


    hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\mynamedpipe"),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
        1,
        1024 * 16,
        1024 * 16,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    while (hPipe != INVALID_HANDLE_VALUE)
    {
        if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
        {
            while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
            {
                // add terminating zero
                buffer[dwRead] = '\0';

                // do something with data in buffer
                std::cout << buffer << std::endl;
            }
        }

        DisconnectNamedPipe(hPipe);
    }//*/
}
