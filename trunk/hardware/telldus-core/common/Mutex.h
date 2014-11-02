//
// C++ Interface: Thread
//
// Description:
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2010
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_MUTEX_H_
#define TELLDUS_CORE_COMMON_MUTEX_H_

namespace TelldusCore {
	class Mutex {
	public:
		Mutex();
		virtual ~Mutex();

		virtual void lock();
		virtual void unlock();

	private:
		Mutex(const Mutex&);  // Disable copy
		Mutex& operator = (const Mutex&);
		class PrivateData;
		PrivateData *d;
	};
	class LoggedMutex : public Mutex {
	public:
		void lock();
		void unlock();
	};

	class MutexLocker {
	public:
		explicit MutexLocker(Mutex *m);
		~MutexLocker();
	private:
		Mutex *mutex;
	};
}

#endif  // TELLDUS_CORE_COMMON_MUTEX_H_
