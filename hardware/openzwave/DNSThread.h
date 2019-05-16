//-----------------------------------------------------------------------------
//
//	DNSThread.h
//
//	Async DNS Lookups
//
//	Copyright (c) 2016 Justin Hammond <justin@dynam.ac>
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

#ifndef _DNSThread_H
#define _DNSThread_H

#include <string>
#include <map>
#include <list>

#include "Defs.h"
#include "Driver.h"
#include "platform/Event.h"
#include "platform/Mutex.h"
#include "platform/TimeStamp.h"
#include "platform/DNS.h"

namespace OpenZWave
{
	class Event;
	class Mutex;
	class Controller;
	class Thread;


	enum LookupType {
		DNS_Lookup_ConfigRevision = 1
	};

	struct DNSLookup {
			uint8 NodeID;
			string lookup;
			string result;
			DNSError status;
			LookupType type;

	};


	/** \brief the DNSThread provides Async DNS lookups for checking revision numbers of
	 *  Config Files against the official database
	 */
	class OPENZWAVE_EXPORT DNSThread
	{
			friend class Driver;
		private:
			DNSThread(Driver *);
			virtual ~DNSThread();

			/**
			 *  Entry point for DNSThread
			 */
			static void DNSThreadEntryPoint( Event* _exitEvent, void* _context );
			/**
			 *  DNSThreadProc for DNSThread.  This is where all the "action" takes place.
			 */
			void DNSThreadProc( Event* _exitEvent );

			/* submit a Request to the DNS List */
			bool sendRequest(DNSLookup *);

			/* process the most recent request recieved */
			void processResult();

			Driver*			  m_driver;
			Mutex*			  m_dnsMutex;
			list<DNSLookup *> m_dnslist;
			list<DNSLookup *> m_dnslistinprogress;
			Event*			  m_dnsRequestEvent;
			DNS				  m_dnsresolver;


	}; /* class DNSThread */
} /* namespace OpenZWave */


#endif
