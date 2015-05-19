//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <ftdi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <list>
#include <string>

#include "service/TellStick.h"
#include "service/Log.h"
#include "service/Settings.h"
#include "client/telldus-core.h"
#include "common/Thread.h"
#include "common/Mutex.h"
#include "common/Strings.h"
#include "common/common.h"

typedef struct _EVENT_HANDLE {
	pthread_cond_t eCondVar;
	pthread_mutex_t eMutex;
} EVENT_HANDLE;
typedef int DWORD;

class TellStick::PrivateData {
public:
	bool open, ignoreControllerConfirmation;
	int vid, pid;
	std::string serial, message;
	ftdi_context ftHandle;
	EVENT_HANDLE eh;
	bool running;
	TelldusCore::Mutex mutex;
};

TellStick::TellStick(int controllerId, TelldusCore::EventRef event, TelldusCore::EventRef updateEvent, const TellStickDescriptor &td )
	:Controller(controllerId, event, updateEvent) {
	d = new PrivateData;
	d->open = false;
	d->vid = td.vid;
	d->pid = td.pid;
	d->serial = td.serial;
	d->running = false;

	Settings set;
	d->ignoreControllerConfirmation = set.getSetting(L"ignoreControllerConfirmation") == L"true";

	ftdi_init(&d->ftHandle);
	ftdi_set_interface(&d->ftHandle, INTERFACE_ANY);

	Log::notice("Connecting to TellStick (%X/%X) with serial %s", d->vid, d->pid, d->serial.c_str());
	int ret = ftdi_usb_open_desc(&d->ftHandle, td.vid, td.pid, NULL, td.serial.c_str());
	if (ret < 0) {
		ftdi_deinit(&d->ftHandle);
		return;
	}
	d->open = true;
	ftdi_usb_reset( &d->ftHandle );
	ftdi_disable_bitbang( &d->ftHandle );
	ftdi_set_latency_timer(&d->ftHandle, 16);

	if (d->open) {
		if (td.pid == 0x0C31) {
			this->setBaud(9600);
		} else {
			this->setBaud(4800);
		}
		this->start();
	} else {
		Log::warning("Failed to open TellStick");
	}
}

TellStick::~TellStick() {
	Log::warning("Disconnected TellStick (%X/%X) with serial %s", d->vid, d->pid, d->serial.c_str());
	if (d->running) {
		stop();
	}

	if (d->open) {
		ftdi_usb_close(&d->ftHandle);
		ftdi_deinit(&d->ftHandle);
	}
	delete d;
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
	int success = ftdi_usb_reset( &d->ftHandle );
	if(success < 0) {
		return TELLSTICK_ERROR_UNKNOWN;  // -1 = FTDI reset failed, -2 = USB device unavailable
	}
	return TELLSTICK_SUCCESS;
}

void TellStick::run() {
	int dwBytesRead = 0;
	unsigned char buf[1024];     // = 0;

	pthread_mutex_init(&d->eh.eMutex, NULL);
	pthread_cond_init(&d->eh.eCondVar, NULL);

	{
		TelldusCore::MutexLocker locker(&d->mutex);
		d->running = true;
	}

	// Send a firmware version request
	unsigned char msg[] = "V+";
	ftdi_write_data( &d->ftHandle, msg, 2 );

	while(1) {
		// Is there any better way then sleeping between reads?
		msleep(100);
		TelldusCore::MutexLocker locker(&d->mutex);
		if (!d->running) {
			break;
		}
		memset(buf, 0, sizeof(buf));
		dwBytesRead = ftdi_read_data(&d->ftHandle, buf, sizeof(buf));
		if (dwBytesRead < 0) {
			// An error occured, avoid flooding by sleeping longer
			// Hopefully if will start working again
			msleep(1000);  // 1s
		}
		if (dwBytesRead < 1) {
			continue;
		}
		processData( reinterpret_cast<char *>(&buf) );
	}
}

int TellStick::send( const std::string &strMessage ) {
	if (!d->open) {
		return TELLSTICK_ERROR_NOT_FOUND;
	}

	bool c = true;
	unsigned char *tempMessage = new unsigned char[strMessage.size()];
	memcpy(tempMessage, strMessage.c_str(), strMessage.size());

	// This lock does two things
	//  1 Prevents two calls from different threads to this function
	//  2 Prevents our running thread from receiving the data we are interested in here
	TelldusCore::MutexLocker locker(&d->mutex);

	int ret;
	ret = ftdi_write_data( &d->ftHandle, tempMessage, strMessage.length() );
	if(ret < 0) {
		c = false;
	} else if(ret != strMessage.length()) {
		Log::debug("Weird send length? retval %i instead of %d\n", ret, static_cast<int>(strMessage.length()));
	}

	delete[] tempMessage;

	if(!c) {
		Log::debug("Broken pipe on send");
		return TELLSTICK_ERROR_BROKEN_PIPE;
	}

	if(strMessage.compare("N+") == 0 && ((pid() == 0x0C31 && firmwareVersion() < 5) || (pid() == 0x0C30 && firmwareVersion() < 6))) {
		// these firmware versions doesn't implement ack to noop, just check that the noop can be sent correctly
		return TELLSTICK_SUCCESS;
	}

	if(d->ignoreControllerConfirmation) {
		// allow TellStick to finish its air-sending
		msleep(1000);
		return TELLSTICK_SUCCESS;
	}

	int retrycnt = 250;
	unsigned char in;
	while(--retrycnt) {
		ret = ftdi_read_data( &d->ftHandle, &in, 1);
		if (ret > 0) {
			if (in == '\n') {
				return TELLSTICK_SUCCESS;
			}
		} else if(ret == 0) {  // No data available
			usleep(100);
		} else {  // Error
			Log::debug("Broken pipe on read");
			return TELLSTICK_ERROR_BROKEN_PIPE;
		}
	}

	return TELLSTICK_ERROR_COMMUNICATION;
}

void TellStick::setBaud(int baud) {
	int ret = ftdi_set_baudrate(&d->ftHandle, baud);
	if(ret != 0) {
		fprintf(stderr, "set Baud failed, retval %i\n", ret);
	}
}

std::list<TellStickDescriptor> TellStick::findAll() {
	std::list<TellStickDescriptor> tellstick = findAllByVIDPID(0x1781, 0x0C30);

	std::list<TellStickDescriptor> duo = findAllByVIDPID(0x1781, 0x0C31);
	for(std::list<TellStickDescriptor>::const_iterator it = duo.begin(); it != duo.end(); ++it) {
		tellstick.push_back(*it);
	}

	return tellstick;
}

bool TellStick::stillConnected() const {
	ftdi_context ftdic;
	struct ftdi_device_list *devlist, *curdev;
	char serialBuffer[10];
	ftdi_init(&ftdic);
	bool found = false;

	int ret = ftdi_usb_find_all(&ftdic, &devlist, d->vid, d->pid);
	if (ret > 0) {
		for (curdev = devlist; curdev != NULL; curdev = curdev->next) {
			ret = ftdi_usb_get_strings(&ftdic, curdev->dev, NULL, 0, NULL, 0, serialBuffer, 10);
			if (ret != 0) {
				continue;
			}
			if (d->serial.compare(serialBuffer) == 0) {
				found = true;
				break;
			}
		}
	}

	ftdi_list_free(&devlist);
	ftdi_deinit(&ftdic);
	return found;
}

std::list<TellStickDescriptor> TellStick::findAllByVIDPID( int vid, int pid ) {
	std::list<TellStickDescriptor> retval;

	ftdi_context ftdic;
	struct ftdi_device_list *devlist, *curdev;
	char serialBuffer[10];
	ftdi_init(&ftdic);

	int ret = ftdi_usb_find_all(&ftdic, &devlist, vid, pid);
	if (ret > 0) {
		for (curdev = devlist; curdev != NULL; curdev = curdev->next) {
			ret = ftdi_usb_get_strings(&ftdic, curdev->dev, NULL, 0, NULL, 0, serialBuffer, 10);
			if (ret != 0) {
				continue;
			}
			TellStickDescriptor td;
			td.vid = vid;
			td.pid = pid;
			td.serial = serialBuffer;
			retval.push_back(td);
		}
	}
	ftdi_list_free(&devlist);
	ftdi_deinit(&ftdic);

	return retval;
}

void TellStick::stop() {
	if (d->running) {
		{
			TelldusCore::MutexLocker locker(&d->mutex);
				d->running = false;
		}
		// Unlock the wait-condition

		pthread_cond_broadcast(&d->eh.eCondVar);
	}
	this->wait();
}
