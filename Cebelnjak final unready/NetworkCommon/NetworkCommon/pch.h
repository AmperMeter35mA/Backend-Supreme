#pragma once

#include <iostream>
#include <fstream>
#include <WS2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <bitset>
#include <time.h>
#include <atomic>
#include <mutex>
#include <fstream>

#pragma comment (lib, "ws2_32.lib")

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/videoio/videoio_c.h"

#define log(x) std::cout << x << std::endl;
#define logn(x) std::cout << x;

#define debug
#define debugon
#define debugx

//#define debug
#ifdef debug
	#define debug(x) std::cout << x << std::endl;
	#define debugerr(x) std::cerr << x << std::endl;
#else
	#define debug(x)
	#define debugerr(x)
#endif

//#define debion
#ifdef debugon
	#define debugx(x) std::cout << x << std::endl;
#else
	#define debugx(x)
#endif


#define assert __debugbreak();

//#define debugx
#ifdef debugx
	#define assertx __debugbreak();
#else
	#define assertx
#endif
