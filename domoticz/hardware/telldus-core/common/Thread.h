//
// C++ Interface: Thread
//
// Description:
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_THREAD_H_
#define TELLDUS_CORE_COMMON_THREAD_H_

#include <string>
#include "common/Mutex.h"

namespace TelldusCore {
	class ThreadPrivate;
	class Thread {
		public:
			Thread();
			virtual ~Thread();
			void start();
			void startAndLock(Mutex *lock);
			bool wait();

		protected:
			virtual void run() = 0;

		private:
			static void* exec( void *ptr );
			ThreadPrivate *d;
	};
}

#endif  // TELLDUS_CORE_COMMON_THREAD_H_
