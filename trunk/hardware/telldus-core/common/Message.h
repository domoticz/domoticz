//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_MESSAGE_H_
#define TELLDUS_CORE_COMMON_MESSAGE_H_

#include <string>

namespace TelldusCore {
	class Message : public std::wstring {
	public:
		Message();
		explicit Message(const std::wstring &functionName);
		~Message(void);

		void addArgument(const std::wstring &value);
		// void addSpecialArgument(const std::wstring &);
		// void addSpecialArgument(int);
		// void addSpecialArgument(const char *);
		void addArgument(int value);
		void addArgument(const char *value);

		static bool nextIsInt(const std::wstring &message);
		static bool nextIsString(const std::wstring &message);

		static std::wstring takeString(std::wstring *message);
		static int takeInt(std::wstring *message);

	private:
	};
}

#endif  // TELLDUS_CORE_COMMON_MESSAGE_H_
