//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/TelldusWinService_win.h"

#include <Dbt.h>
#include <algorithm>
#include <string>

#include "service/Log.h"
#include "service/TelldusMain.h"

int g_argc;
char **g_argv;


static const GUID GUID_DEVINTERFACE_USBRAW = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };

TelldusWinService::TelldusWinService()
	:tm(0) {
	tm = new TelldusMain();
}

TelldusWinService::~TelldusWinService() {
	delete tm;
}

void TelldusWinService::stop() {
	tm->stop();
}

DWORD WINAPI TelldusWinService::serviceControlHandler( DWORD controlCode, DWORD dwEventType, LPVOID lpEventData ) {
	switch ( controlCode ) {
		case SERVICE_CONTROL_INTERROGATE:
			SetServiceStatus( serviceStatusHandle, &serviceStatus );
			return NO_ERROR;

		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			stop();
			serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
			SetServiceStatus( serviceStatusHandle, &serviceStatus );

			return NO_ERROR;
		case SERVICE_CONTROL_POWEREVENT:
			if (dwEventType == PBT_APMSUSPEND) {
				tm->suspend();
			} else if (dwEventType == PBT_APMRESUMEAUTOMATIC) {
				tm->resume();
			}
			return NO_ERROR;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI TelldusWinService::deviceNotificationHandler( DWORD controlCode, DWORD dwEventType, LPVOID lpEventData ) {
	if (controlCode != SERVICE_CONTROL_DEVICEEVENT) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	if (dwEventType != DBT_DEVICEARRIVAL && dwEventType != DBT_DEVICEREMOVECOMPLETE) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	PDEV_BROADCAST_DEVICEINTERFACE pDevInf = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(lpEventData);
	if (!pDevInf) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}

	std::wstring name(pDevInf->dbcc_name);
	transform(name.begin(), name.end(), name.begin(), toupper);

	// Parse VID
	size_t posStart = name.find(L"VID_");
	if (posStart == std::wstring::npos) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	posStart += 4;
	size_t posEnd = name.find(L'&', posStart);
	if (posEnd == std::wstring::npos) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	std::wstring strVID = name.substr(posStart, posEnd-posStart);

	// Parse PID
	posStart = name.find(L"PID_");
	if (posStart == std::wstring::npos) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	posStart += 4;
	posEnd = name.find(L'#', posStart);
	if (posEnd == std::wstring::npos) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	std::wstring strPID = name.substr(posStart, posEnd-posStart);

	int vid = static_cast<int>(strtol(std::string(strVID.begin(), strVID.end()).c_str(), NULL, 16));
	int pid = static_cast<int>(strtol(std::string(strPID.begin(), strPID.end()).c_str(), NULL, 16));

	if (dwEventType == DBT_DEVICEARRIVAL) {
		tm->deviceInsertedOrRemoved(vid, pid, true);
	} else {
		tm->deviceInsertedOrRemoved(vid, pid, false);
	}

	return NO_ERROR;
}

DWORD WINAPI TelldusWinService::serviceControlHandler( DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext ) {
	TelldusWinService *instance = reinterpret_cast<TelldusWinService *>(lpContext);
	if (!instance) {
		return ERROR_CALL_NOT_IMPLEMENTED;
	}
	if (controlCode == SERVICE_CONTROL_DEVICEEVENT) {
		return instance->deviceNotificationHandler(controlCode, dwEventType, lpEventData);
	}
	return instance->serviceControlHandler(controlCode, dwEventType, lpEventData);
}

void WINAPI TelldusWinService::serviceMain( DWORD argc, TCHAR* argv[] ) {
	TelldusWinService instance;

	// Enable debug if we hade this supplied
	for(unsigned int i = 1; i < argc; ++i) {
		if (wcscmp(argv[i], L"--debug") == 0) {
			Log::setDebug();
		}
	}

	// initialise service status
	instance.serviceStatus.dwServiceType = SERVICE_WIN32;
	instance.serviceStatus.dwCurrentState = SERVICE_STOPPED;
	instance.serviceStatus.dwControlsAccepted = 0;
	instance.serviceStatus.dwWin32ExitCode = NO_ERROR;
	instance.serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
	instance.serviceStatus.dwCheckPoint = 0;
	instance.serviceStatus.dwWaitHint = 0;

	instance.serviceStatusHandle = RegisterServiceCtrlHandlerEx( serviceName, TelldusWinService::serviceControlHandler, &instance );

	if ( instance.serviceStatusHandle ) {
		// service is starting
		instance.serviceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus( instance.serviceStatusHandle, &instance.serviceStatus );

		// running
		instance.serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		// Register for power management notification
		instance.serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_POWEREVENT;
		instance.serviceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus( instance.serviceStatusHandle, &instance.serviceStatus );

		// Register for device notification
		DEV_BROADCAST_DEVICEINTERFACE devInterface;
		ZeroMemory( &devInterface, sizeof(devInterface) );
		devInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		devInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		devInterface.dbcc_classguid = GUID_DEVINTERFACE_USBRAW;
		HDEVNOTIFY deviceNotificationHandle = RegisterDeviceNotificationW(instance.serviceStatusHandle, &devInterface, DEVICE_NOTIFY_SERVICE_HANDLE);

		Log::notice("TelldusService started");

		// Start our main-loop
		instance.tm->start();

		Log::notice("TelldusService stopping");
		Log::destroy();

		// service was stopped
		instance.serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus( instance.serviceStatusHandle, &instance.serviceStatus );

		// service is now stopped
		instance.serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		instance.serviceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus( instance.serviceStatusHandle, &instance.serviceStatus );
	}
}
