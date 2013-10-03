//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ConnectionListener.h"

#include <AccCtrl.h>
#include <Aclapi.h>
#include <windows.h>

#include "common/Event.h"
#include "common/Socket.h"

#define BUFSIZE 512

class ConnectionListener::PrivateData {
public:
	std::wstring pipename;
	SECURITY_ATTRIBUTES sa;
	HANDLE hEvent;
	bool running;
	TelldusCore::EventRef waitEvent;
};

ConnectionListener::ConnectionListener(const std::wstring &name, TelldusCore::EventRef waitEvent) {
	d = new PrivateData;
	d->hEvent = 0;

	d->running = true;
	d->waitEvent = waitEvent;
	d->pipename = L"\\\\.\\pipe\\" + name;

	PSECURITY_DESCRIPTOR pSD = NULL;
	PACL pACL = NULL;
	EXPLICIT_ACCESS ea;
	PSID pEveryoneSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL) {
		return;
	}

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
		LocalFree(pSD);
		return;
	}

	if(!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID)) {
		LocalFree(pSD);
	}

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = STANDARD_RIGHTS_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance= NO_INHERITANCE;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

	// Add the ACL to the security descriptor.
	if (!SetSecurityDescriptorDacl(pSD,
				TRUE,     // bDaclPresent flag
				pACL,
				FALSE)) {   // not a default DACL
		LocalFree(pSD);
		FreeSid(pEveryoneSID);
	}


	d->sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	d->sa.lpSecurityDescriptor = pSD;
	d->sa.bInheritHandle = false;

	start();
}

ConnectionListener::~ConnectionListener(void) {
	d->running = false;
	if (d->hEvent) {
		SetEvent(d->hEvent);
	}
	wait();
	delete d;
}

void ConnectionListener::run() {
	HANDLE hPipe;
	OVERLAPPED oOverlap;
	DWORD cbBytesRead;

	memset(&oOverlap, 0, sizeof(OVERLAPPED));

	d->hEvent = CreateEvent(NULL, true, false, NULL);
	oOverlap.hEvent = d->hEvent;
	bool recreate = true;

	while (1) {
		BOOL alreadyConnected = false;
		if (recreate) {
			hPipe = CreateNamedPipe(
				(const wchar_t *)d->pipename.c_str(),             // pipe name
				PIPE_ACCESS_DUPLEX |       // read/write access
				FILE_FLAG_OVERLAPPED,	   // Overlapped mode
				PIPE_TYPE_MESSAGE |        // message type pipe
				PIPE_READMODE_MESSAGE |    // message-read mode
				PIPE_WAIT,                 // blocking mode
				PIPE_UNLIMITED_INSTANCES,  // max. instances
				BUFSIZE,                   // output buffer size
				BUFSIZE,                   // input buffer size
				0,                         // client time-out
				&d->sa);                   // default security attribute

			if (hPipe == INVALID_HANDLE_VALUE) {
				return;
			}

			ConnectNamedPipe(hPipe, &oOverlap);
			alreadyConnected = GetLastError() == ERROR_PIPE_CONNECTED;
			recreate = false;
		}
		if(!alreadyConnected) {
			DWORD result = WaitForSingleObject(oOverlap.hEvent, 1000);
			if (!d->running) {
				CancelIo(hPipe);
				WaitForSingleObject(oOverlap.hEvent, INFINITE);
				break;
			}

			if(result == WAIT_TIMEOUT) {
				// CloseHandle(hPipe);
				continue;
			}
			BOOL connected = GetOverlappedResult(hPipe, &oOverlap, &cbBytesRead, false);

			if (!connected) {
				CloseHandle(hPipe);
				return;
			}
		}
		ConnectionListenerEventData *data = new ConnectionListenerEventData();
		ResetEvent(oOverlap.hEvent);
		data->socket = new TelldusCore::Socket(hPipe);
		d->waitEvent->signal(data);

		recreate = true;
	}

	CloseHandle(d->hEvent);
	CloseHandle(hPipe);
}
