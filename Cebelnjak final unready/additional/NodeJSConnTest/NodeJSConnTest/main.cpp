

#include <iostream>
#include <thread>

#include <Windows.h>


#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;
#define assert __debugbreak();





HANDLE hPipe;
unsigned char data[1024 * 100];
bool allowRead = false;
std::thread reader;
bool runReader = true;


void readData();

bool init()
{
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

    reader = std::thread(readData);


    return true;
}

void readData()
{
    DWORD count;

    while (runReader)
    {
        //https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
        allowRead = false;
        ReadFile(hPipe, data, 1024 * 100, &count, NULL);
        log("read data: " << count);
        allowRead = true;
    }
}

unsigned char* getData()
{
    if (allowRead)
    {
        allowRead = false;
        return data;
    }

    return nullptr;
}

void stop()
{
    runReader = false;
    reader.join();
}


int main()
{
    if (!init())
    {
        log("init error");
        return -1;
    }

    while (true);

}