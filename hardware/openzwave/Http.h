//-----------------------------------------------------------------------------
//
//	Http.h
//
//	Simple HTTP Client Interface to download updated config files
//
//	Copyright (c) 2015 Justin Hammond <Justin@dynam.ac>
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

#ifndef _Http_H
#define _Http_H

#include "Defs.h"
#include "platform/Event.h"
#include "platform/Thread.h"
#include "platform/Mutex.h"

namespace OpenZWave
{
	class Driver;

	namespace Internal
	{
		/* This is a abstract class you can implement if you wish to override the built in HTTP Client
		 * Code in OZW with your own code.
		 *
		 * The built in Code uses threads to download updated files to a temporary file
		 * and then this class moves the files into the correct place.
		 *
		 */

		struct HttpDownload
		{
				string filename;
				string url;
				uint8 node;
				enum DownloadType
				{
					None,
					File,
					Config,
					MFSConfig
				};
				DownloadType operation;
				enum Status
				{
					Ok,
					Failed
				};
				Status transferStatus;

		};

		class i_HttpClient
		{
			public:
				i_HttpClient(Driver *);
				virtual ~i_HttpClient()
				{
				}
				;
				virtual bool StartDownload(HttpDownload *transfer) = 0;
				void FinishDownload(HttpDownload *transfer);
			private:
				Driver* m_driver;
		};

		/* this is OZW's implementation of a Http Client. It uses threads to download Config Files in the background.
		 *
		 */

		class HttpClient: public i_HttpClient
		{
			public:
				HttpClient(Driver *);
				~HttpClient();
				bool StartDownload(HttpDownload *transfer);
			private:

				static void HttpThreadProc(Internal::Platform::Event* _exitEvent, void* _context);
				//Driver* 	m_driver;
				Internal::Platform::Event* m_exitEvent;

				Internal::Platform::Thread* m_httpThread;
				bool m_httpThreadRunning;
				Internal::Platform::Mutex* m_httpMutex;
				list<HttpDownload *> m_httpDownlist;
				Internal::Platform::Event* m_httpDownloadEvent;

		};

	} // namespace Internal
} /* namespace OpenZWave */

#endif
