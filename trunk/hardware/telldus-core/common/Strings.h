//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_STRINGS_H_
#define TELLDUS_CORE_COMMON_STRINGS_H_

#include <stdarg.h>
#ifdef _MSC_VER
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif
#include <string>

namespace TelldusCore {
	std::wstring charToWstring(const char *value);
	int charToInteger(const char *value);
	std::wstring charUnsignedToWstring(const unsigned char value);

	bool comparei(std::wstring stringA, std::wstring stringB);
	std::wstring intToWstring(int value);
	// std::wstring intToWStringSafe(int value);
	std::string intToString(int value);
	uint64_t hexTo64l(const std::string data);
	std::string wideToString(const std::wstring &input);

	int wideToInteger(const std::wstring &input);

	std::string formatf(const char *format, ...);
	std::string sformatf(const char *format, va_list ap);
}

#endif  // TELLDUS_CORE_COMMON_STRINGS_H_
