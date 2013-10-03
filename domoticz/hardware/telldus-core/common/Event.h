//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef TELLDUS_CORE_COMMON_EVENT_H_
#define TELLDUS_CORE_COMMON_EVENT_H_


#ifndef _WINDOWS
	#include <tr1/memory>
	typedef void* EVENT_T;
#else
	#include <windows.h>
	#include <memory>
	typedef HANDLE EVENT_T;
#endif
#include "common/Thread.h"

namespace TelldusCore {
	class EventHandler;

	class EventData {
	public:
		virtual ~EventData();
		virtual bool isValid() const;
	};

	class EventDataBase : public EventData {
	public:
		virtual bool isValid() const;
	};

	typedef std::tr1::shared_ptr<EventData> EventDataRef;

	class EventBase {
	public:
		virtual ~EventBase();

		void popSignal();
		bool isSignaled();
		void signal();
		virtual void signal(EventData *eventData);
		EventDataRef takeSignal();

	protected:
		explicit EventBase(EventHandler *handler);
		void clearHandler();
		virtual void clearSignal() = 0;
		EventHandler *handler() const;
		virtual void sendSignal() = 0;

	private:
		class PrivateData;
		PrivateData *d;
	};

	class Event : public EventBase {
	public:
		virtual ~Event();

	protected:
		explicit Event(EventHandler *handler);
		EVENT_T retrieveNative();
		virtual void clearSignal();
		virtual void sendSignal();

	private:
		class PrivateData;
		PrivateData *d;

		friend class EventHandler;
	};

	typedef std::tr1::shared_ptr<Event> EventRef;
}

#endif  // TELLDUS_CORE_COMMON_EVENT_H_
