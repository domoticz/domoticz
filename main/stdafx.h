#pragma once
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#if defined WIN32
	#include <SDKDDKVer.h>
#endif


// TODO: reference additional headers your program requires here

typedef unsigned char       BYTE;

#if !defined WIN32
#include <stdio.h>
//#include <stdlib.h>
#include <string>
	#include <sys/socket.h> // Needed for the socket functions
	#include <sys/time.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	typedef int SOCKET;
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR   -1
	#define closesocket(s) close(s);
#else
	#pragma warning(disable : 4996)
	#include <winsock2.h>
	#if defined _M_IX86
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#elif defined _M_IA64
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#elif defined _M_X64
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#else
	#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#endif
#endif

#if defined(__FreeBSD__)
	#include <netinet/in.h>
#endif

#define WEBSERVER_DONT_USE_ZIP

// system, runtime, STL
#include <algorithm>
#include <iomanip>
#include <map>
#include <mutex>
#include <vector>
#include <sstream>
#include <string>
#include <thread>
#include <cmath>

// third-party
#include <json/json.h>
#include "RFXtrx.h"

// rarely changing project-specific
#include "concurrent_queue.h"
#include "localtime_r.h"
#include "StoppableTask.h"
