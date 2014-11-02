//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <AccCtrl.h>
#include <Aclapi.h>
#include <windows.h>

#include "common/common.h"
#include "common/Socket.h"

#define BUFSIZE 512

namespace TelldusCore {

class Socket::PrivateData {
public:
	HANDLE hPipe;
	HANDLE readEvent;
	bool connected;
	bool running;
};

Socket::Socket() {
	d = new PrivateData;
	d->hPipe = INVALID_HANDLE_VALUE;
	d->connected = false;
	d->running = true;
}

Socket::Socket(SOCKET_T hPipe) {
	d = new PrivateData;
	d->hPipe = hPipe;
	d->connected = true;
	d->running = true;
}


Socket::~Socket(void) {
	d->running = false;
	SetEvent(d->readEvent);  // signal for break
	if (d->hPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(d->hPipe);
		d->hPipe = 0;
	}
	delete d;
}

void Socket::connect(const std::wstring &server) {
	BOOL fSuccess = false;

	std::wstring name(L"\\\\.\\pipe\\" + server);
	d->hPipe = CreateFile(
		(const wchar_t *)name.c_str(),           // pipe name
		GENERIC_READ |  // read and write access
		GENERIC_WRITE,
		0,              // no sharing
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe
		FILE_FLAG_OVERLAPPED,  // default attributes
		NULL);          // no template file

	if (d->hPipe == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		d->hPipe,  // pipe handle
		&dwMode,   // new pipe mode
		NULL,      // don't set maximum bytes
		NULL);     // don't set maximum time

	if (!fSuccess) {
		return;
	}
	d->connected = true;
}

void Socket::stopReadWait() {
	d->running = false;
	SetEvent(d->readEvent);
}

std::wstring Socket::read() {
	return read(INFINITE);
}

std::wstring Socket::read(int timeout) {
	wchar_t buf[BUFSIZE];
	int result;
	DWORD cbBytesRead = 0;
	OVERLAPPED oOverlap;

	memset(&oOverlap, 0, sizeof(OVERLAPPED));

	d->readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	oOverlap.hEvent = d->readEvent;
	BOOL fSuccess = false;
	std::wstring returnString;
	bool moreData = true;

	while(moreData) {
		moreData = false;
		memset(&buf, 0, sizeof(buf));

		ReadFile( d->hPipe, &buf, sizeof(buf)-sizeof(wchar_t), &cbBytesRead, &oOverlap);

		result = WaitForSingleObject(oOverlap.hEvent, timeout);

		if(!d->running) {
			CancelIo(d->hPipe);
			WaitForSingleObject(oOverlap.hEvent, INFINITE);
			d->readEvent = 0;
			CloseHandle(oOverlap.hEvent);
			return L"";
		}

		if (result == WAIT_TIMEOUT) {
			CancelIo(d->hPipe);
			// Cancel, we still need to cleanup
		}
		fSuccess = GetOverlappedResult(d->hPipe, &oOverlap, &cbBytesRead, true);

		if (!fSuccess) {
			DWORD err = GetLastError();
			
			if(err == ERROR_MORE_DATA) {
				moreData = true;
			} else {
				buf[0] = 0;
			}
			if (err == ERROR_BROKEN_PIPE) {
				d->connected = false;
				break;
			}
		}
		returnString.append(buf);
	}
	d->readEvent = 0;
	CloseHandle(oOverlap.hEvent);
	return returnString;
}

void Socket::write(const std::wstring &msg) {
	OVERLAPPED oOverlap;
	DWORD bytesWritten = 0;
	int result;
	BOOL fSuccess = false;

	memset(&oOverlap, 0, sizeof(OVERLAPPED));

	HANDLE writeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	oOverlap.hEvent = writeEvent;

	BOOL writeSuccess = WriteFile(d->hPipe, msg.data(), (DWORD)msg.length()*sizeof(wchar_t), &bytesWritten, &oOverlap);
	result = GetLastError();
	if (writeSuccess || result == ERROR_IO_PENDING) {
		result = WaitForSingleObject(writeEvent, 30000);
		if (result == WAIT_TIMEOUT) {
			CancelIo(d->hPipe);
			WaitForSingleObject(oOverlap.hEvent, INFINITE);
			CloseHandle(writeEvent);
			CloseHandle(d->hPipe);
			d->hPipe = 0;
			d->connected = false;
			return;
		}
		fSuccess = GetOverlappedResult(d->hPipe, &oOverlap, &bytesWritten, TRUE);
	}

	CloseHandle(writeEvent);
	if (!fSuccess) {
		CloseHandle(d->hPipe);
		d->hPipe = 0;
		d->connected = false;
		return;
	}
}

bool Socket::isConnected() {
	return d->connected;
}

}  // namespace TelldusCore
