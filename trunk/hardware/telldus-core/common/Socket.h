//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_SOCKET_H_
#define TELLDUS_CORE_COMMON_SOCKET_H_

#ifdef _WINDOWS
	#include <windows.h>
	typedef HANDLE SOCKET_T;
#else
	typedef int SOCKET_T;
#endif

#include <string>

namespace TelldusCore {
	class Socket {
	public:
		Socket();
		explicit Socket(SOCKET_T hPipe);
		virtual ~Socket(void);

		void connect(const std::wstring &server);
		bool isConnected();
		std::wstring read();
		std::wstring read(int timeout);
		void stopReadWait();
		void write(const std::wstring &msg);

	private:
		class PrivateData;
		PrivateData *d;
	};
}

#endif  // TELLDUS_CORE_COMMON_SOCKET_H_
