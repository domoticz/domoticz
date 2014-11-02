//
// C++ Implementation: TellStick
//
// Description:
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <string.h>
#include <stdlib.h>
#include <string>
#include <list>
#include "common/common.h"
#include "common/Mutex.h"
#include "common/Strings.h"
#include "service/Log.h"
#include "service/Settings.h"
#include "service/TellStick.h"
#include "../client/telldus-core.h"

#include "service/ftd2xx.h"

class TellStick::PrivateData {
public:
	bool open, running, ignoreControllerConfirmation;
	int vid, pid;
	std::string serial, message;
	FT_HANDLE ftHandle;
	TelldusCore::Mutex mutex;

#ifdef _WINDOWS
	HANDLE eh;
#else
// #include <unistd.h>
	struct {
		pthread_cond_t eCondVar;
		pthread_mutex_t eMutex;
	} eh;
#endif
};

TellStick::TellStick(int controllerId, TelldusCore::EventRef event, TelldusCore::EventRef updateEvent, const TellStickDescriptor &td )
	:Controller(controllerId, event, updateEvent) {
	d = new PrivateData;
#ifdef _WINDOWS
	d->eh = CreateEvent( NULL, false, false, NULL );
#else
	pthread_mutex_init(&d->eh.eMutex, NULL);
	pthread_cond_init(&d->eh.eCondVar, NULL);
#endif
	d->open = false;
	d->running = false;
	d->vid = td.vid;
	d->pid = td.pid;
	d->serial = td.serial;
	Settings set;
	d->ignoreControllerConfirmation = set.getSetting(L"ignoreControllerConfirmation") == L"true";

	char *tempSerial = new char[td.serial.size()+1];
#ifdef _WINDOWS
	strcpy_s(tempSerial, td.serial.size()+1, td.serial.c_str());
#else
	snprintf(tempSerial, td.serial.size()+1, "%s", td.serial.c_str());
	FT_SetVIDPID(td.vid, td.pid);
#endif
	Log::notice("Connecting to TellStick (%X/%X) with serial %s", d->vid, d->pid, d->serial.c_str());
	FT_STATUS ftStatus = FT_OpenEx(tempSerial, FT_OPEN_BY_SERIAL_NUMBER, &d->ftHandle);
	delete[] tempSerial;
	if (ftStatus == FT_OK) {
		d->open = true;
		FT_SetFlowControl(d->ftHandle, FT_FLOW_NONE, 0, 0);
		FT_SetTimeouts(d->ftHandle, 5000, 0);
	}

	if (d->open) {
		if (td.pid == 0x0C31) {
			setBaud(9600);
		} else {
			setBaud(4800);
		}
		this->start();
	} else {
		Log::warning("Failed to open TellStick");
	}
}

TellStick::~TellStick() {
	Log::warning("Disconnected TellStick (%X/%X) with serial %s", d->vid, d->pid, d->serial.c_str());
	if (d->running) {
		TelldusCore::MutexLocker locker(&d->mutex);
		d->running = false;
#ifdef _WINDOWS
		SetEvent(d->eh);
#else
		pthread_cond_broadcast(&d->eh.eCondVar);
#endif
	}
	this->wait();
	if (d->open) {
		FT_Close(d->ftHandle);
	}
	delete d;
}

void TellStick::setBaud( int baud ) {
	FT_SetBaudRate(d->ftHandle, baud);
}

int TellStick::pid() const {
	return d->pid;
}

int TellStick::vid() const {
	return d->vid;
}

std::string TellStick::serial() const {
	return d->serial;
}

bool TellStick::isOpen() const {
	return d->open;
}

bool TellStick::isSameAsDescriptor(const TellStickDescriptor &td) const {
	if (td.vid != d->vid) {
		return false;
	}
	if (td.pid != d->pid) {
		return false;
	}
	if (td.serial != d->serial) {
		return false;
	}
	return true;
}

void TellStick::processData( const std::string &data ) {
	for (unsigned int i = 0; i < data.length(); ++i) {
		if (data[i] == 13) {  // Skip \r
			continue;
		} else if (data[i] == 10) {  // \n found
			if (d->message.substr(0, 2).compare("+V") == 0) {
				setFirmwareVersion(TelldusCore::charToInteger(d->message.substr(2).c_str()));
			} else if (d->message.substr(0, 2).compare("+R") == 0) {
				this->publishData(d->message.substr(2));
			} else if(d->message.substr(0, 2).compare("+W") == 0) {
				this->decodePublishData(d->message.substr(2));
			}
			d->message.clear();
		} else {  // Append the character
			d->message.append( 1, data[i] );
		}
	}
}

int TellStick::reset() {
#ifndef _WINDOWS
	return TELLSTICK_SUCCESS;  // nothing to be done on other platforms
#else
	int success = FT_CyclePort( d->ftHandle );
	if(success == FT_OK) {
		return TELLSTICK_SUCCESS;
	}
	return TELLSTICK_ERROR_UNKNOWN;
#endif
}

void TellStick::run() {
	d->running = true;
	DWORD dwBytesInQueue = 0;
	DWORD dwBytesRead = 0;
	char *buf = 0;

	// Send a firmware version request
	char msg[] = "V+";
	FT_Write(d->ftHandle, msg, (DWORD)strlen(msg), &dwBytesRead);

	while(1) {
#ifdef _WINDOWS
		FT_SetEventNotification(d->ftHandle, FT_EVENT_RXCHAR, d->eh);
		WaitForSingleObject(d->eh, INFINITE);
#else
		FT_SetEventNotification(d->ftHandle, FT_EVENT_RXCHAR, (PVOID)&d->eh);
		pthread_mutex_lock(&d->eh.eMutex);
		pthread_cond_wait(&d->eh.eCondVar, &d->eh.eMutex);
		pthread_mutex_unlock(&d->eh.eMutex);
#endif

		TelldusCore::MutexLocker locker(&d->mutex);
		if (!d->running) {
			break;
		}
		FT_GetQueueStatus(d->ftHandle, &dwBytesInQueue);
		if (dwBytesInQueue < 1) {
			continue;
		}
		buf = reinterpret_cast<char*>(malloc(sizeof(buf) * (dwBytesInQueue+1)));
		memset(buf, 0, dwBytesInQueue+1);
		FT_Read(d->ftHandle, buf, dwBytesInQueue, &dwBytesRead);
		processData( buf );
		free(buf);
	}
}

int TellStick::send( const std::string &strMessage ) {
	if (!d->open) {
		return TELLSTICK_ERROR_NOT_FOUND;
	}

	// This lock does two things
	//  1 Prevents two calls from different threads to this function
	//  2 Prevents our running thread from receiving the data we are interested in here
	TelldusCore::MutexLocker locker(&d->mutex);

	char *tempMessage = reinterpret_cast<char *>(malloc(sizeof(std::string::value_type) * (strMessage.size()+1)));
#ifdef _WINDOWS
	strcpy_s(tempMessage, strMessage.size()+1, strMessage.c_str());
#else
	snprintf(tempMessage, strMessage.size()+1, "%s", strMessage.c_str());
#endif

	ULONG bytesWritten, bytesRead;
	char in;
	FT_STATUS ftStatus;
	ftStatus = FT_Write(d->ftHandle, tempMessage, (DWORD)strMessage.length(), &bytesWritten);
	free(tempMessage);

	if(ftStatus != FT_OK) {
		Log::debug("Broken pipe on send");
		return TELLSTICK_ERROR_BROKEN_PIPE;
	}

	if(strMessage.compare("N+") == 0 && ((pid() == 0x0C31 && firmwareVersion() < 5) || (pid() == 0x0C30 && firmwareVersion() < 6))) {
		// these firmware versions doesn't implement ack to noop, just check that the noop can be sent correctly
		return TELLSTICK_SUCCESS;
	}
	if(d->ignoreControllerConfirmation) {
		// wait for TellStick to finish its air-sending
		msleep(1000);
		return TELLSTICK_SUCCESS;
	}

	while(1) {
		ftStatus = FT_Read(d->ftHandle, &in, 1, &bytesRead);
		if (ftStatus == FT_OK) {
			if (bytesRead == 1) {
				if (in == '\n') {
					return TELLSTICK_SUCCESS;
				} else {
					continue;
				}
			} else {  // Timeout
				return TELLSTICK_ERROR_COMMUNICATION;
			}
		} else {  // Error
			Log::debug("Broken pipe on read");
			return TELLSTICK_ERROR_BROKEN_PIPE;
		}
	}
}

bool TellStick::stillConnected() const {
	FT_STATUS ftStatus;
	DWORD numDevs;
	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus != FT_OK) {
		return false;
	}
	if (numDevs <= 0) {
		return false;
	}
	for (int i = 0; i < static_cast<int>(numDevs); i++) {
		FT_HANDLE ftHandleTemp;
		DWORD flags;
		DWORD id;
		DWORD type;
		DWORD locId;
		char serialNumber[16];
		char description[64];
		// get information for device i
		ftStatus = FT_GetDeviceInfoDetail(i, &flags, &type, &id, &locId, serialNumber, description, &ftHandleTemp);
		if (ftStatus != FT_OK) {
			continue;
		}
		if (d->serial.compare(serialNumber) == 0) {
			return true;
		}
	}
	return false;
}

std::list<TellStickDescriptor> TellStick::findAll() {
	std::list<TellStickDescriptor> tellstick = findAllByVIDPID(0x1781, 0x0C30);

	std::list<TellStickDescriptor> duo = findAllByVIDPID(0x1781, 0x0C31);
	for(std::list<TellStickDescriptor>::const_iterator it = duo.begin(); it != duo.end(); ++it) {
		tellstick.push_back(*it);
	}

	return tellstick;
}

std::list<TellStickDescriptor> TellStick::findAllByVIDPID( int vid, int pid ) {
	std::list<TellStickDescriptor> retval;

	FT_STATUS ftStatus = FT_OK;
	DWORD dwNumberOfDevices = 0;

#ifndef _WINDOWS
	FT_SetVIDPID(vid, pid);
#endif

	ftStatus = FT_CreateDeviceInfoList(&dwNumberOfDevices);
	if (ftStatus != FT_OK) {
		return retval;
	}
	if (dwNumberOfDevices > 0) {
		FT_DEVICE_LIST_INFO_NODE *devInfo;
		// allocate storage for list based on dwNumberOfDevices
		devInfo = reinterpret_cast<FT_DEVICE_LIST_INFO_NODE*>(malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*dwNumberOfDevices));  // get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo, &dwNumberOfDevices);
		if (ftStatus == FT_OK) {
			unsigned int id = (vid << 16) | pid;
			for (unsigned int i = 0; i < dwNumberOfDevices; i++) {
				if (devInfo[i].ID != id) {
					continue;
				}
				TellStickDescriptor td;
				td.vid = vid;
				td.pid = pid;
				td.serial = devInfo[i].SerialNumber;
				retval.push_back(td);
			}
		}
		free(devInfo);
	}
	return retval;
}
