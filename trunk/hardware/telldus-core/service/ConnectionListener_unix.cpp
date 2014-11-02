//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

#include "service/ConnectionListener.h"
#include "common/Socket.h"

#if defined(_MACOSX) && !defined(SOCK_CLOEXEC)
#define SOCK_CLOEXEC 0
#endif

class ConnectionListener::PrivateData {
public:
	TelldusCore::EventRef waitEvent;
	std::string name;
	bool running;
};

ConnectionListener::ConnectionListener(const std::wstring &name, TelldusCore::EventRef waitEvent) {
	d = new PrivateData;
	d->waitEvent = waitEvent;

	d->name = "/tmp/" + std::string(name.begin(), name.end());
	d->running = true;

	this->start();
}

ConnectionListener::~ConnectionListener(void) {
	d->running = false;
	this->wait();
	unlink(d->name.c_str());
	delete d;
}

void ConnectionListener::run() {
	struct timeval tv = { 0, 0 };

	// Timeout for select

	SOCKET_T serverSocket;
	struct sockaddr_un name;
	serverSocket = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (serverSocket < 0) {
		return;
	}
#if defined(_MACOSX)
	int op = fcntl(serverSocket, F_GETFD);
	fcntl(serverSocket, F_SETFD, op | FD_CLOEXEC);  // OS X doesn't support SOCK_CLOEXEC yet
#endif
	name.sun_family = AF_LOCAL;
	memset(name.sun_path, '\0', sizeof(name.sun_path));
	strncpy(name.sun_path, d->name.c_str(), sizeof(name.sun_path));
	unlink(name.sun_path);
	int size = SUN_LEN(&name);
	bind(serverSocket, (struct sockaddr *)&name, size);
	listen(serverSocket, 5);

	// Change permissions to allow everyone
	chmod(d->name.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

	fd_set infds;
	FD_ZERO(&infds);
	FD_SET(serverSocket, &infds);

	while(d->running) {
		tv.tv_sec = 5;

		int response = select(serverSocket+1, &infds, NULL, NULL, &tv);
		if (response == 0) {
			FD_SET(serverSocket, &infds);
			continue;
		} else if (response < 0 ) {
			continue;
		}
		// Make sure it is a new connection
		if (!FD_ISSET(serverSocket, &infds)) {
			continue;
		}
		SOCKET_T clientSocket = accept(serverSocket, NULL, NULL);

		ConnectionListenerEventData *data = new ConnectionListenerEventData();
		data->socket = new TelldusCore::Socket(clientSocket);
		d->waitEvent->signal(data);
	}
	close(serverSocket);
}

