//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef TELLDUS_CORE_COMMON_COMMON_H_
#define TELLDUS_CORE_COMMON_COMMON_H_

#ifdef _WINDOWS
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#include <ole2.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WINDOWS
#include <fstream>  // NOLINT(readability/streams)
#endif
#include <string>
#include "common/Strings.h"

inline void msleep( const int msec) {
#ifdef _WINDOWS
	Sleep(msec);
#else
	usleep(msec*1000);
#endif
}

inline void dlog(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	fflush(stdout);
}

inline void debuglogfilename(const int intMessage, const std::string strMessage, const std::string filename) {
#ifdef _WINDOWS
	static bool firstRun = true;
	std::ofstream file;

	if (firstRun) {
		file.open(filename.c_str(), std::ios::out);
		firstRun = false;
	} else {
		file.open(filename.c_str(), std::ios::out | std::ios::app);
	}

	__time32_t now = _time32(0);

	// Convert now to tm struct for local timezone
	struct tm localtm;
	_localtime32_s(&localtm, &now);
	char thetime[32];
	errno_t err = asctime_s(thetime, 32, &localtm);
	if (!err) {
		file << thetime << " [" << GetCurrentThreadId() << "] " << intMessage << " - " << strMessage << "\n";
		file.flush();
		file.close();
	}

#elif !defined(_MACOSX) && !defined(__FreeBSD__)
	pthread_t thread = pthread_self();
	printf("[%i] %i - %s\n", static_cast<int>(thread), intMessage, strMessage.c_str());
	fflush(stdout);
#else
	printf("%i - %s\n", intMessage, strMessage.c_str());
#endif
}

inline void debuglogservice(const int intMessage, const std::string strMessage) {
	std::string filename("C:/telldus_service_debug.txt");
	debuglogfilename(intMessage, strMessage, filename);
}

inline void debuglog(const int intMessage, const std::string strMessage) {
	std::string filename("C:/telldus_client_debug.txt");
	debuglogfilename(intMessage, strMessage, filename);
}

inline char *wrapStdString( const std::string &string) {
#ifdef _WINDOWS
	return reinterpret_cast<char *>(SysAllocStringByteLen(string.c_str(), (unsigned int)string.size()));
#else
	char *returnVal;
	returnVal = reinterpret_cast<char *>(malloc(sizeof(*returnVal) * (string.size()+1)));
	snprintf(returnVal, string.size()+1, "%s", string.c_str());
	return returnVal;
#endif
}

inline char *wrapStdWstring( const std::wstring &wstring) {
	return wrapStdString(TelldusCore::wideToString(wstring));
}

#endif  // TELLDUS_CORE_COMMON_COMMON_H_
