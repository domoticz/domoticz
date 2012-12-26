// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#if defined WIN32
	#include "targetver.h"
#endif

#include <stdio.h>
//#include <stdlib.h>
#include <string>

// TODO: reference additional headers your program requires here

typedef unsigned char       BYTE;

#if !defined WIN32
	#include <sys/socket.h> // Needed for the socket functions
	#include <netdb.h>
	#include <arpa/inet.h>
	typedef int SOCKET;
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR   -1
	#define closesocket(s) close(s);
#else
	#pragma warning(disable : 4996)
	#include <winsock2.h>
#endif

#define WEBSERVER_DONT_USE_ZIP

#include <boost/thread.hpp>

