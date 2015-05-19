//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/Timer.h"
#ifdef _WINDOWS
#else
#include <sys/time.h>
#include <errno.h>
#endif
#include "common/Mutex.h"

class Timer::PrivateData {
public:
	PrivateData() : interval(0), running(false) {}
	TelldusCore::EventRef event;
	int interval;
	bool running;
#ifdef _WINDOWS
	HANDLE cond;
	TelldusCore::Mutex mutex;
#else
	pthread_mutex_t waitMutex;
	pthread_cond_t cond;
#endif
};

Timer::Timer(TelldusCore::EventRef event)
	:TelldusCore::Thread(), d(new PrivateData) {
	d->event = event;
#ifdef _WINDOWS
	d->cond = CreateEventW(NULL, false, false, NULL);
#else
	pthread_cond_init(&d->cond, NULL);
	pthread_mutex_init(&d->waitMutex, NULL);
#endif
}

Timer::~Timer() {
	this->stop();
	this->wait();

#ifdef _WINDOWS
#else
	pthread_mutex_destroy(&d->waitMutex);
	pthread_cond_destroy(&d->cond);
	delete d;
#endif
}

void Timer::setInterval(int sec) {
	d->interval = sec;
}

void Timer::stop() {
#ifdef _WINDOWS
	TelldusCore::MutexLocker(&d->mutex);
	d->running = false;
	SetEvent(d->cond);
#else
	// Signal event
	pthread_mutex_lock(&d->waitMutex);
	if (d->running) {
		d->running = false;
		pthread_cond_signal(&d->cond);
	}
	pthread_mutex_unlock(&d->waitMutex);
#endif
}

void Timer::run() {
#ifdef _WINDOWS
	int interval = 0;
	{
		TelldusCore::MutexLocker(&d->mutex);
		d->running = true;
		interval = d->interval*1000;
	}
	while(1) {
		DWORD retval = WaitForSingleObject(d->cond, interval);
		if (retval == WAIT_TIMEOUT) {
			d->event->signal();
		}
		TelldusCore::MutexLocker(&d->mutex);
		if (!d->running) {
			break;
		}
	}
#else
	struct timespec ts;
	struct timeval tp;

	pthread_mutex_lock(&d->waitMutex);
	d->running = true;
	pthread_mutex_unlock(&d->waitMutex);

	while(1) {
		int rc = 0;
		gettimeofday(&tp, NULL);

		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += d->interval;

		pthread_mutex_lock( &d->waitMutex );
		if (d->running) {
			rc = pthread_cond_timedwait(&d->cond, &d->waitMutex, &ts);
		} else {
			pthread_mutex_unlock( &d->waitMutex );
			break;
		}
		pthread_mutex_unlock( &d->waitMutex );
		if (rc == ETIMEDOUT) {
			d->event->signal();
		}
	}
#endif
}
