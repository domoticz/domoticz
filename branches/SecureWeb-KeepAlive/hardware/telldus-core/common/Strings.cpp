//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "common/Strings.h"
#include <string.h>
#include <stdio.h>

#ifdef _WINDOWS
#include <windows.h>
#else
#include <iconv.h>
#endif
#include <algorithm>
#include <sstream>
#include <string>


#ifdef _MACOSX
#define WCHAR_T_ENCODING "UCS-4-INTERNAL"
#else
#define WCHAR_T_ENCODING "WCHAR_T"
#endif

#ifndef va_copy
#ifdef __va_copy
#define va_copy(a, b) __va_copy(a, b)
#else /* !__va_copy */
#define va_copy(a, b) ((a)=(b))
#endif /* __va_copy */
#endif /* va_copy */

std::wstring TelldusCore::charToWstring(const char *value) {
#ifdef _WINDOWS
	// Determine size
	int size = MultiByteToWideChar(CP_UTF8, 0, value, -1, NULL, 0);
	if (size == 0) {
		return L"";
	}
	wchar_t *buffer;
	buffer = new wchar_t[size];
	memset(buffer, 0, sizeof(wchar_t)*(size));

	int bytes = MultiByteToWideChar(CP_UTF8, 0, value, -1, buffer, size);
	std::wstring retval(buffer);
	delete[] buffer;
	return retval;

#else
	size_t utf8Length = strlen(value);
	size_t outbytesLeft = utf8Length*sizeof(wchar_t);

	// Copy the instring
	char *inString = new char[utf8Length+1];
	snprintf(inString, utf8Length+1, "%s", value);

	// Create buffer for output
	char *outString = reinterpret_cast<char*>(new wchar_t[utf8Length+1]);
	memset(outString, 0, sizeof(wchar_t)*(utf8Length+1));

#ifdef _FREEBSD
	const char *inPointer = inString;
#else
	char *inPointer = inString;
#endif
	char *outPointer = outString;

	iconv_t convDesc = iconv_open(WCHAR_T_ENCODING, "UTF-8");
	iconv(convDesc, &inPointer, &utf8Length, &outPointer, &outbytesLeft);
	iconv_close(convDesc);

	std::wstring retval( reinterpret_cast<wchar_t *>(outString) );

	// Cleanup
	delete[] inString;
	delete[] outString;

	return retval;
#endif
}

int TelldusCore::charToInteger(const char *input) {
	std::stringstream inputstream;
	inputstream << input;
	int retval;
	inputstream >> retval;
	return retval;
}

std::wstring TelldusCore::charUnsignedToWstring(const unsigned char value) {
	std::wstringstream st;
	st << value;
	return st.str();
}

/**
* This method doesn't support all locales
*/
bool TelldusCore::comparei(std::wstring stringA, std::wstring stringB) {
	transform(stringA.begin(), stringA.end(), stringA.begin(), toupper);
	transform(stringB.begin(), stringB.end(), stringB.begin(), toupper);

	return stringA == stringB;
}

std::wstring TelldusCore::intToWstring(int value) {
#ifdef _WINDOWS
	// no stream used
	// TODO(stefan): Make effective and safe...
	wchar_t numstr[21];  // enough to hold all numbers up to 64-bits
	_itow_s(value, numstr, sizeof(numstr), 10);
	std::wstring newstring(numstr);
	return newstring;
	// return TelldusCore::charToWstring(stdstring.c_str());
	// std::wstring temp = TelldusCore::charToWstring(stdstring.c_str());
	// std::wstring temp(stdstring.length(), L' ');
	// std::copy(stdstring.begin(), stdstring.end(), temp.begin());
	// return temp;
#else
	std::wstringstream st;
	st << value;
	return st.str();
#endif
}

std::string TelldusCore::intToString(int value) {
	// Not sure if this is neecssary (for ordinary stringstream that is)
#ifdef _WINDOWS
	char numstr[21];  // enough to hold all numbers up to 64-bits
	_itoa_s(value, numstr, sizeof(numstr), 10);
	std::string stdstring(numstr);
	return stdstring;
#else
	std::stringstream st;
	st << value;
	return st.str();
#endif
}

/*
std::wstring TelldusCore::intToWStringSafe(int value){
	#ifdef _WINDOWS
		//no stream used
	//TODO! Make effective and safe...
		char numstr[21]; // enough to hold all numbers up to 64-bits
		itoa(value, numstr, 10);
		std::string stdstring(numstr);
		return TelldusCore::charToWstring(stdstring.c_str());
		//std::wstring temp = TelldusCore::charToWstring(stdstring.c_str());
		//std::wstring temp(stdstring.length(), L' ');
		//std::copy(stdstring.begin(), stdstring.end(), temp.begin());
		//return temp;
	#else
		return TelldusCore::intToWString(value);
	#endif
}
*/

uint64_t TelldusCore::hexTo64l(const std::string data) {
#ifdef _WINDOWS
	return _strtoui64(data.c_str(), NULL, 16);
#elif defined(_MACOSX)
	return strtoq(data.c_str(), NULL, 16);
#else
	return strtoull(data.c_str(), NULL, 16);
#endif
}

int TelldusCore::wideToInteger(const std::wstring &input) {
	std::wstringstream inputstream;
	inputstream << input;
	int retval;
	inputstream >> retval;
	return retval;
}

std::string TelldusCore::wideToString(const std::wstring &input) {
#ifdef _WINDOWS
	// Determine size
	int size = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
	if (size == 0) {
		return "";
	}
	char *buffer;
	buffer = new char[size];
	memset(buffer, 0, sizeof(*buffer)*size);

	int bytes = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, buffer, size, NULL, NULL);
	std::string retval(buffer);
	delete[] buffer;
	return retval;

#else
	size_t wideSize = sizeof(wchar_t)*input.length();
	// We cannot know how many wide character there is yet
	size_t outbytesLeft = wideSize+sizeof(char);  // NOLINT(runtime/sizeof)

	// Copy the instring
	char *inString = reinterpret_cast<char*>(new wchar_t[input.length()+1]);
	memcpy(inString, input.c_str(), wideSize+sizeof(wchar_t));

	// Create buffer for output
	char *outString = new char[outbytesLeft];
	memset(outString, 0, sizeof(*outString)*(outbytesLeft));

#ifdef _FREEBSD
	const char *inPointer = inString;
#else
	char *inPointer = inString;
#endif
	char *outPointer = outString;

	iconv_t convDesc = iconv_open("UTF-8", WCHAR_T_ENCODING);
	iconv(convDesc, &inPointer, &wideSize, &outPointer, &outbytesLeft);
	iconv_close(convDesc);

	std::string retval(outString);

	// Cleanup
	delete[] inString;
	delete[] outString;

	return retval;
#endif
}

std::string TelldusCore::formatf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	std::string retval = sformatf(format, ap);
	va_end(ap);
	return retval;
}

std::string TelldusCore::sformatf(const char *format, va_list ap) {
	// This code is based on code from the Linux man-pages project (man vsprintf)
	int n;
	int size = 100;     /* Guess we need no more than 100 bytes. */
	char *p, *np;

	if ((p = reinterpret_cast<char*>(malloc(size))) == NULL) {
		return "";
	}

	while (1) {
		/* Try to print in the allocated space. */
		va_list ap2;
		va_copy(ap2, ap);
		n = vsnprintf(p, size, format, ap2);
		va_end(ap2);

		/* If that worked, return the string. */
		if (n > -1 && n < size) {
			std::string retval(p);
			free(p);
			return retval;
		}

		/* Else try again with more space. */

		if (n > -1) {   /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		} else {        /* glibc 2.0 */
			size *= 2;  /* twice the old size */
		}
		if ((np = reinterpret_cast<char *>(realloc (p, size))) == NULL) {
			free(p);
			return "";
		} else {
			p = np;
		}
	}
}
