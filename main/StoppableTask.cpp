#include "stdafx.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#include <inttypes.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#ifdef _WIN32
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct sockaddr_un {
	unsigned short sun_family;
	char sun_path[UNIX_PATH_MAX];
};

/* socketpair.c
 * Copyright 2007, 2010 by Nathan C. Myers <ncm@cantrip.org>
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice, this
 *     list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *     The name of the author must not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#endif

StoppableTask::StoppableTask() : m_stopfd{INVALID_SOCKET, INVALID_SOCKET}
{
#ifndef _WIN32
	// Nice and simple except on Windows, where we have to use the standard
	// trick (lifted from ncm, credited above) to build a socketpair.
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_stopfd) == 0)
	{
		fcntl(m_stopfd[0], F_SETFL, O_NONBLOCK);
		fcntl(m_stopfd[1], F_SETFL, O_NONBLOCK);
	}
#else
	// Create socketpair for stop signalling.
	union {
		struct sockaddr_un unaddr;
		struct sockaddr_in inaddr;
		struct sockaddr addr;
	} a;
	SOCKET listener;
	int e, attempt;
	int domains[] = {AF_UNIX, AF_INET};
	socklen_t addrlen;
	int reuse = 1;

	/* AF_UNIX/SOCK_STREAM became available in Windows 10
	 * ( https://devblogs.microsoft.com/commandline/af_unix-comes-to-windows )
	 *
	 * However, Windows doesn't support abstract sockets, so we have to make
	 * a pathname for it (and delete it immediately once it's bound, so we
	 * never leave anything around). Using szUserDataFolder should be fine
	 * for this: we should always be able to write there.
	 *
	 * We keep the fallback to AF_INET just in case, as it does no harm.
	 */
	for (attempt = 0; attempt < 2; attempt++) {
		int domain = domains[attempt];

		listener = socket(domain, SOCK_STREAM, domain == AF_INET ? IPPROTO_TCP : 0);
		if (listener == INVALID_SOCKET)
			continue;

		memset(&a, 0, sizeof(a));
		if (domain == AF_UNIX) {
			/* XX: Abstract sockets (filesystem-independent) don't work, contrary to
			 * the claims of the aforementioned blog post:
			 * https://github.com/microsoft/WSL/issues/4240#issuecomment-549663217
			 *
			 * So we must use a named path, and that comes with all the attendant
			 * problems of permissions and collisions. Using temp folder
			 * with high-res time and PID in the filename.
			 */
			LARGE_INTEGER ticks;
			DWORD n;

			n = GetTempPathA(UNIX_PATH_MAX, a.unaddr.sun_path);
			if (n == 0 || n >= UNIX_PATH_MAX)
				continue;

			/* Use high-res timer ticks and PID for unique filename */
			QueryPerformanceCounter(&ticks);
			snprintf(a.unaddr.sun_path + n, UNIX_PATH_MAX - n,
				 "%" PRIx64 "-%lu.$$$", ticks.QuadPart, GetCurrentProcessId());
			a.unaddr.sun_family = AF_UNIX;
			addrlen = sizeof(a.unaddr);

			if (bind(listener, &a.addr, addrlen) == SOCKET_ERROR)
				goto fallback;
		} else {
			a.inaddr.sin_family = AF_INET;
			a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			a.inaddr.sin_port = 0;
			addrlen = sizeof(a.inaddr);

			if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
				       (char *) &reuse, (socklen_t) sizeof(reuse)) == -1)
				goto fallback;

			if (bind(listener, &a.addr, addrlen) == SOCKET_ERROR)
				goto fallback;

			memset(&a, 0, sizeof(a));
			if (getsockname(listener, &a.addr, &addrlen) == SOCKET_ERROR)
				goto fallback;

			// win32 getsockname may only set the port number, p=0.0005.
			// ( https://docs.microsoft.com/windows/win32/api/winsock/nf-winsock-getsockname ):
			a.inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			a.inaddr.sin_family = AF_INET;
		}

		if (listen(listener, 1) == SOCKET_ERROR)
			goto fallback;

		m_stopfd[0] = socket(domain, SOCK_STREAM, domain == AF_INET ? IPPROTO_TCP : 0);
		if (m_stopfd[0] == INVALID_SOCKET)
			goto fallback;
		if (connect(m_stopfd[0], &a.addr, addrlen) == SOCKET_ERROR)
			goto fallback;

		// For AF_UNIX sockets, delete the listening socket immediately so we
		// never leave anything behind. We only need the connected pair.
		if (domain == AF_UNIX)
			DeleteFile(a.unaddr.sun_path);

		m_stopfd[1] = accept(listener, NULL, NULL);
		if (m_stopfd[1] == INVALID_SOCKET)
			goto fallback;

		closesocket(listener);
		break;  // Success - exit loop

	fallback:
		// This socket type didn't work; close the sockets and try the next.
		e = WSAGetLastError();
		closesocket(listener);
		closesocket(m_stopfd[0]);
		closesocket(m_stopfd[1]);
		m_stopfd[0] = m_stopfd[1] = INVALID_SOCKET;
		WSASetLastError(e);
	}

	if (m_stopfd[0] != INVALID_SOCKET)
	{
		u_long mode = 1;
		ioctlsocket(m_stopfd[0], FIONBIO, &mode);
		ioctlsocket(m_stopfd[1], FIONBIO, &mode);
	}
#endif
}

StoppableTask::StoppableTask(StoppableTask &&obj) noexcept
	: m_stopfd{obj.m_stopfd[0], obj.m_stopfd[1]}
{
	obj.m_stopfd[0] = obj.m_stopfd[1] = INVALID_SOCKET;
}

StoppableTask &StoppableTask::operator=(StoppableTask &&obj) noexcept
{
	if (m_stopfd[0] != INVALID_SOCKET) closesocket(m_stopfd[0]);
	if (m_stopfd[1] != INVALID_SOCKET) closesocket(m_stopfd[1]);
	m_stopfd[0] = obj.m_stopfd[0];
	m_stopfd[1] = obj.m_stopfd[1];
	obj.m_stopfd[0] = obj.m_stopfd[1] = INVALID_SOCKET;
	return *this;
}

StoppableTask::~StoppableTask()
{
	if (m_stopfd[0] != INVALID_SOCKET) closesocket(m_stopfd[0]);
	if (m_stopfd[1] != INVALID_SOCKET) closesocket(m_stopfd[1]);
}

bool StoppableTask::IsStopRequested(const int timeMS)
{
	if (m_stopfd[0] == INVALID_SOCKET)
		return false;

	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(m_stopfd[0], &rfds);

	struct timeval tv;
	tv.tv_sec = timeMS / 1000;
	tv.tv_usec = (timeMS % 1000) * 1000;

	int ret = select(static_cast<int>(m_stopfd[0] + 1), &rfds, nullptr, nullptr, &tv);
	if (ret > 0 && FD_ISSET(m_stopfd[0], &rfds))
		return true;
	return false;
}

void StoppableTask::RequestStop()
{
	if (m_stopfd[1] != INVALID_SOCKET)
	{
		char c = 1;
		(void)send(m_stopfd[1], &c, 1, 0);
	}
}

void StoppableTask::RequestStart()
{
	// Drain any pending stop signals
	if (m_stopfd[0] != INVALID_SOCKET)
	{
		char c;
		while (recv(m_stopfd[0], &c, 1, 0) > 0);
	}
}

int StoppableTask::GetStopFd()
{
	return (m_stopfd[0] == INVALID_SOCKET) ? -1 : (int)m_stopfd[0];
}

