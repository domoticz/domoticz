//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_CLIENT_CLIENT_H_
#define TELLDUS_CORE_CLIENT_CLIENT_H_

#include "client/telldus-core.h"
#include "client/CallbackDispatcher.h"
#include "common/Message.h"
#include "common/Thread.h"

namespace TelldusCore {
	class Client : public Thread {
	public:
		~Client(void);

		static Client *getInstance();
		static void close();

		int registerEvent(CallbackStruct::CallbackType type, void *eventFunction, void *context );
		void stopThread(void);
		int unregisterCallback( int callbackId );

		int getSensor(char *protocol, int protocolLen, char *model, int modelLen, int *id, int *dataTypes);
		int getController(int *controllerId, int *controllerType, char *name, int nameLen, int *available);

		static bool getBoolFromService(const Message &msg);
		static int getIntegerFromService(const Message &msg);
		static std::wstring getWStringFromService(const Message &msg);

	protected:
			void run(void);

	private:
		Client();
		static std::wstring sendToService(const Message &msg);

		class PrivateData;
		PrivateData *d;
		static Client *instance;
	};
}

#endif  // TELLDUS_CORE_CLIENT_CLIENT_H_
