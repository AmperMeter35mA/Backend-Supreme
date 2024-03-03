#include <iostream>
#include <vector>

#include <Windows.h>


#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;


int main()
{
    char buf[92000];
    std::vector<unsigned char> buf2; buf2.resize(92000);
    DWORD num = 0;

    for (int i = 0; i < 92000; i++)
    {
        buf[i] = 'a';
        buf2.at('a');
    }

    HANDLE m_hPipe;

    m_hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\videopipe"),
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
        1,
        1024 * 100,
        1024 * 100,
        NMPWAIT_USE_DEFAULT_WAIT,
        NULL);

    if (m_hPipe == INVALID_HANDLE_VALUE)
    {
        log("pipe init failed");
        return false;
    }

    while (true)
    {
        if (m_hPipe != INVALID_HANDLE_VALUE)
        {
            bool res = WriteFile(m_hPipe, buf2.data(), 92000, &num, NULL);
            log("writing to pipe, success: " << res << ", size: " << num);
        }

        Sleep(470);
    }

    /*HANDLE hPipe;
    DWORD dwWritten;


    hPipe = CreateFile(TEXT("\\\\.\\pipe\\mynamedpipe"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hPipe,
            "connected\n",
            11,   // = length of string + terminating '\0' !!!
            &dwWritten,
            NULL);
        
        CloseHandle(hPipe);
    }//*/
}
