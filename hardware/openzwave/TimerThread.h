//-----------------------------------------------------------------------------
//
//	TimerThread.h
//
//  Timer for scheduling future events
//
//	Copyright (c) 2017 h3ctrl <h3ctrl@gmail.com>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef _TIMERTHREAD_H_
#define _TIMERTHREAD_H_

#if __cplusplus >= 201103L || __APPLE__ || _MSC_VER
#include <functional>
using std::bind;
using std::function;
#else
#include <tr1/functional>
using std::tr1::bind;
using std::tr1::function;
#endif

#include "Defs.h"
#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/TimeStamp.h"

namespace OpenZWave
{
	class Driver;
	class Thread;
	class Timer;

	/** \brief The TimerThread class makes it possible to schedule events to happen
	 *  at a certain time in the future.
	 */
	class OPENZWAVE_EXPORT TimerThread
	{
		friend class Timer;
	//-----------------------------------------------------------------------------
	//  Timer based actions
	//-----------------------------------------------------------------------------
	public:
		/** A timer callback function. */
		typedef function<void(uint32)> TimerCallback;

		/**
		 * Constructor.
		 */
		TimerThread( Driver *_driver );

		/**
		 * Destructor.
		 */
		~TimerThread();

		struct TimerEventEntry
		{
			Timer *instance;
			TimeStamp timestamp;
			TimerCallback callback;
			uint32 id;
		};

		/**
		 * Main entry point for the timer thread. Wrapper around TimerThreadProc.
		 * \param _exitEvent Exit event indicating the thread should exit
		 * \param _context A TimerThread object
		 */
		static void TimerThreadEntryPoint( Event* _exitEvent, void* _context );

	private:
		//Driver*	m_driver;

		/**
		 * Schedule an event.
		 * \param _milliseconds The number of milliseconds before the event should happen
		 * \param _callback The function to be called when the time is reached
		 * \param _instance The Timer SubClass where this Event should be executed from
		 */
		TimerEventEntry* TimerSetEvent( int32 _milliseconds, TimerCallback _callback, Timer *_instance, uint32 id );

		/**
		 * Remove a Event
		 *
		 */
		void TimerDelEvent(TimerEventEntry *);

		/**
		 * Main class entry point for the timer thread. Contains the main timer loop.
		 * \param _exitEvent Exit event indicating the thread should exit
		 */
		void TimerThreadProc( Event* _exitEvent );


    /** A list of upcoming timer events */
		list<TimerEventEntry *> m_timerEventList;

		Event*				m_timerEvent;   // Event to signal new timed action requested
		Mutex*				m_timerMutex;   // Serialize access to class members
		int32					m_timerTimeout; // Time in milliseconds to wait until next event
	};

	/**
	 * \brief Timer SubClass for automatically registering/unregistering Timer Callbacks
	 * if the instance goes out of scope
	 *
	 */

	class OPENZWAVE_EXPORT Timer
	{
	public:
		/**
		 * \brief Constructor with the _driver this instance is associated with
		 * \param _driver The Driver that this instance is associated with
		 */
		Timer( Driver *_driver );
		/**
		 * \brief Default Constructor
		 */

		Timer();
		/**
		 * \brief Destructor
		 */
		~Timer();
		/**
		 * \brief Schedule an event.
		 * \param _milliseconds The number of milliseconds before the event should happen
		 * \param _callback The function to be called when the time is reached
		 * \param _id The ID of the Timer
		 */
		TimerThread::TimerEventEntry* TimerSetEvent( int32 _milliseconds, TimerThread::TimerCallback _callback, uint32 id );
		/**
		 *  \brief Delete All Events registered to this instance
		 */
		void TimerDelEvents();
		/**
		 * \brief Delete a Specific Event Registered to this instance
		 * \param te The TimerEventEntry Struct that was returned when Setting a Event
		 */
		void TimerDelEvent(TimerThread::TimerEventEntry *te);
		/**
		 * \brief Delete a Specific Event Registered to this instance
		 * \param id The ID of the Timer To Delete
		 */
		void TimerDelEvent(uint32 id);

		/**
		 * \brief Register the Driver Associated with this Instance
		 * \param _driver The Driver
		 */
		void SetDriver(Driver *_driver);
		/**
		 * \brief Called From the TimerThread Class to execute a callback
		 * \param te The TimerEventEntry structure for the callback to execute
		 */
		void TimerFireEvent(TimerThread::TimerEventEntry *te);
	private:
		Driver*	m_driver;
		list<TimerThread::TimerEventEntry *> m_timerEventList;


	};
} // namespace OpenZWave

#endif // _TIMERTHREAD_H_
