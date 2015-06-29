//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <math.h>
#include <string>

#include "common/Socket.h"
#include "common/Mutex.h"
#include "common/Strings.h"

#define BUFSIZE 512
#if defined(_MACOSX) && !defined(SOCK_CLOEXEC)
	#define SOCK_CLOEXEC 0
#endif

namespace TelldusCore {

int connectWrapper(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return connect(sockfd, addr, addrlen);
}

class Socket::PrivateData {
public:
	SOCKET_T socket;
	bool connected;
	fd_set infds;
	Mutex mutex;
};

Socket::Socket() {
	d = new PrivateData;
	d->socket = 0;
	d->connected = false;
	FD_ZERO(&d->infds);
}

Socket::Socket(SOCKET_T socket) {
	d = new PrivateData;
	d->socket = socket;
	FD_ZERO(&d->infds);
	d->connected = true;
}

Socket::~Socket(void) {
	if(d->socket) {
		close(d->socket);
	}
	delete d;
}

void Socket::connect(const std::wstring &server) {
	struct sockaddr_un remote;
	socklen_t len;

	if ((d->socket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1) {
		return;
	}
#if defined(_MACOSX)
	int op = fcntl(d->socket, F_GETFD);
	fcntl(d->socket, F_SETFD, op | FD_CLOEXEC);  // OS X doesn't support SOCK_CLOEXEC yet
#endif
	std::string name = "/tmp/" + std::string(server.begin(), server.end());
	remote.sun_family = AF_UNIX;
	snprintf(remote.sun_path, sizeof(remote.sun_path), "%s", name.c_str());

	len = SUN_LEN(&remote);
	if (connectWrapper(d->socket, (struct sockaddr *)&remote, len) == -1) {
		return;
	}

	TelldusCore::MutexLocker locker(&d->mutex);
	d->connected = true;
}

bool Socket::isConnected() {
	TelldusCore::MutexLocker locker(&d->mutex);
	return d->connected;
}

std::wstring Socket::read() {
	return this->read(0);
}

std::wstring Socket::read(int timeout) {
	struct timeval tv;
	char inbuf[BUFSIZE];

	FD_SET(d->socket, &d->infds);
	std::string msg;
	while(isConnected()) {
		tv.tv_sec = floor(timeout / 1000.0);
		tv.tv_usec = timeout % 1000;

		int response = select(d->socket+1, &d->infds, NULL, NULL, &tv);
		if (response == 0 && timeout > 0) {
			return L"";
		} else if (response <= 0) {
			FD_SET(d->socket, &d->infds);
			continue;
		}

		int received = BUFSIZE;
		while(received >= (BUFSIZE - 1)) {
			memset(inbuf, '\0', sizeof(inbuf));
			received = recv(d->socket, inbuf, BUFSIZE - 1, 0);
			if(received > 0) {
				msg.append(std::string(inbuf));
			}
		}
		if (received < 0) {
			TelldusCore::MutexLocker locker(&d->mutex);
			d->connected = false;
		}
		break;
	}

	return TelldusCore::charToWstring(msg.c_str());
}

void Socket::stopReadWait() {
	TelldusCore::MutexLocker locker(&d->mutex);
	d->connected = false;
	// TODO(stefan): somehow signal the socket here?
}

void Socket::write(const std::wstring &msg) {
	std::string newMsg(TelldusCore::wideToString(msg));
	int sent = send(d->socket, newMsg.c_str(), newMsg.length(), 0);
	if (sent < 0) {
		TelldusCore::MutexLocker locker(&d->mutex);
		d->connected = false;
	}
}

}  // namespace TelldusCore
