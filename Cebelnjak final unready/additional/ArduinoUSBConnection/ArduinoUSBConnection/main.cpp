#include <iostream>
#include <thread>

#include <windows.h>

#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;

char openDevice(HANDLE& usb_port);

int main()
{
    HANDLE usb_port;
    openDevice(usb_port);

    const int bytes = 26;
    char buffer[bytes]; buffer[bytes - 1] = '\0';// + 1]; buffer[bytes] = '\0';
	LPDWORD num = 0;

	while (true) { 
		if (ReadFile(usb_port, buffer, bytes - 1, num, NULL))
		{
            //log((int)buffer[0] << " " << (int)buffer[1] << " " << (int)buffer[2] << " nigga " << (int)buffer[11] << " " << (int)buffer[12] << " " << (int)buffer[13]);
            //buffer[3] = '\0';
            if ((int)buffer[0] == 1
                && (int)buffer[1] == 2
                && (int)buffer[2] == 3
                && (int)buffer[22] == 4
                && (int)buffer[23] == 5
                && (int)buffer[24] == 6)
            {
                int temp1 = (int)buffer[3];
                int temp2 = (int)buffer[4];
                int weight = (int)((unsigned char)buffer[21]);

                int light = (int)((unsigned char)buffer[8]);
                light = (light << 8) | (int)((unsigned char)buffer[7]);
                light = (light << 8) | (int)((unsigned char)buffer[6]);
                light = (light << 8) | (int)((unsigned char)buffer[5]);

                int rain = (int)((unsigned char)buffer[12]);
                rain = (rain << 8) | (int)((unsigned char)buffer[11]);
                rain = (rain << 8) | (int)((unsigned char)buffer[10]);
                rain = (rain << 8) | (int)((unsigned char)buffer[9]);

                int moist = (int)((unsigned char)buffer[16]);
                moist = (moist << 8) | (int)((unsigned char)buffer[15]);
                moist = (moist << 8) | (int)((unsigned char)buffer[14]);
                moist = (moist << 8) | (int)((unsigned char)buffer[13]);

                int sound = (int)((unsigned char)buffer[20]);
                sound = (sound << 8) | (int)((unsigned char)buffer[19]);
                sound = (sound << 8) | (int)((unsigned char)buffer[18]);
                sound = (sound << 8) | (int)((unsigned char)buffer[17]);

			    log("input: " << "temperatura 1: " << temp1 << ", temperatura 2: " << temp2 << ", svetlost: " << light << ", dez: " << rain << ", vlaga: " << moist << ", teza: " << weight << ", zvok: " << sound << std::endl);
            }
		}
		else
		{
			//log("nothing");
            
            // THIS IS SO IT AUTOMATICALLY RESTARTS THE READING IF ARDUINO GETS DISCONNECTED
            //log("here"); // WHEN IT IS DISCONNECTED THIS RUNS MANY TIMES PER SECOND
            openDevice(usb_port);
		}
	
	}

}

char openDevice(HANDLE &usb_port)//const char* Device, const unsigned int Bauds
{
    // Open serial port
    usb_port = CreateFileA("\\\\.\\COM3", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,/*FILE_ATTRIBUTE_NORMAL*/0, 0);
    if (usb_port == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return -1; // Device not found

        // Error while opening the device
        return -2;
    }

    // Set parameters

    // Structure for the port parameters
    DCB dcbSerialParams;
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    // Get the port parameters
    if (!GetCommState(usb_port, &dcbSerialParams)) return -3;

    // Set the speed (Bauds)
    dcbSerialParams.BaudRate = CBR_9600;

    //select data size
    BYTE bytesize = 8;
    BYTE stopBits = 1;
    BYTE parity = 0;
    // configure byte size
    dcbSerialParams.ByteSize = bytesize;
    // configure stop bits
    dcbSerialParams.StopBits = stopBits;
    // configure parity
    dcbSerialParams.Parity = parity;

    // Write the parameters
    if (!SetCommState(usb_port, &dcbSerialParams)) return -5;

    // Set TimeOut

    COMMTIMEOUTS timeouts = { 0 };
    // Set the Timeout parameters
    timeouts.ReadIntervalTimeout = 0;
    // No TimeOut
    timeouts.ReadTotalTimeoutConstant = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = MAXDWORD;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    // Write the parameters
    if (!SetCommTimeouts(usb_port, &timeouts)) return -6;

    // Opening successfull
    return 1;
}

//ARDUINO READ DOESNT WORK
/*char b = 'n';
char a = 'a';
bool c = true;
while (c)
{
    if (!WriteFile(usb_port, &a, 1, num, NULL))//!ReadFile(usb_port, &b, 1, num, NULL)) || !
    {
        openDevice(usb_port);
        log("failed: " << (int)b);
    }
    else
    {
        log("lol");
        ReadFile(usb_port, &b, 1, num, NULL);
        log("success");
        c = false;
        log(b);
        if (b != 'b')
            c = true;
    }
}

log("ack");//*/

//CLEAR FILE FOR READING
/*for(int i = 0; i < 10; i++)
{
    ReadFile(usb_port, buffer, bytes - 1, num, NULL);
}

log("THROUGH ...");//*/

/*LPDWORD n = 0;
char a = 'd';
while (a != 'a')
{
    if(!ReadFile(usb_port, &a, 1, n, NULL))
    {
        openDevice(usb_port);
    }
}
if (!WriteFile(usb_port, &a, 1, n, NULL))
    log("ack failed");

log("ack");//*/