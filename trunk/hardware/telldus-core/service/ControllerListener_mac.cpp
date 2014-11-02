//
// Copyright (C) 2012 Telldus Technologies AB. All rights reserved.
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "service/ControllerListener.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <string>

class TellStickData {
public:
	io_object_t				notification;
	CFStringRef             serialNumber;
	UInt32					vid;
	UInt32					pid;
	TelldusCore::EventRef				event;
};

class ControllerListener::PrivateData {
public:
	IONotificationPortRef	gNotifyPort;
	CFRunLoopRef			gRunLoop;
	io_iterator_t			gAddedIter;
	TelldusCore::EventRef				event;
	bool running;

	void addUsbFilter(int vid, int pid);
	static void DeviceAdded(void *refCon, io_iterator_t iterator);
	static void DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument);
};

ControllerListener::ControllerListener(TelldusCore::EventRef event)
:Thread() {
	d = new PrivateData;
	d->event = event;
	d->running = true;
	d->gRunLoop = NULL;
	this->start();
}

ControllerListener::~ControllerListener() {
	d->running = false;
	if(d->gRunLoop != NULL)
		CFRunLoopStop(d->gRunLoop);

	this->wait();
	delete d;
}

void ControllerListener::run() {
	CFRunLoopSourceRef		runLoopSource;

	d->gNotifyPort = IONotificationPortCreate(kIOMasterPortDefault);
	runLoopSource = IONotificationPortGetRunLoopSource(d->gNotifyPort);

	d->gRunLoop = CFRunLoopGetCurrent();
	CFRunLoopAddSource(d->gRunLoop, runLoopSource, kCFRunLoopDefaultMode);

	d->addUsbFilter(0x1781, 0x0c30);
	d->addUsbFilter(0x1781, 0x0c31);

	// Race check, if destructor was called really close to thread init,
	// running might have gone false. Make sure we don't get stuck
	if (d->running) {
		CFRunLoopRun();
	}
}

void ControllerListener::PrivateData::addUsbFilter(int vid, int pid) {
	CFNumberRef				numberRef;
	CFMutableDictionaryRef 	matchingDict;

	matchingDict = IOServiceMatching(kIOUSBDeviceClassName);  // Interested in instances of class
	                                                          // IOUSBDevice and its subclasses
	if (matchingDict == NULL) {
		return;
	}

	// Create a CFNumber for the idVendor and set the value in the dictionary
	numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vid);
	CFDictionarySetValue(matchingDict, CFSTR(kUSBVendorID), numberRef);
	CFRelease(numberRef);

	// Create a CFNumber for the idProduct and set the value in the dictionary
	numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
	CFDictionarySetValue(matchingDict, CFSTR(kUSBProductID), numberRef);
	CFRelease(numberRef);

	// Now set up a notification to be called when a device is first matched by I/O Kit.
	IOServiceAddMatchingNotification(gNotifyPort,                // notifyPort
	                                 kIOFirstMatchNotification,  // notificationType
	                                 matchingDict,               // matching
	                                 PrivateData::DeviceAdded,   // callback
	                                 this,                       // refCon
	                                 &gAddedIter                 // notification
	                                 );
	// Iterate once to get already-present devices and arm the notification
	PrivateData::DeviceAdded(this, gAddedIter);
}

void ControllerListener::PrivateData::DeviceNotification(void *refCon, io_service_t service, natural_t messageType, void *messageArgument) {
	if (messageType != kIOMessageServiceIsTerminated) {
		return;
	}

	TellStickData *tsd = reinterpret_cast<TellStickData*> (refCon);
	if (!tsd) {
		return;
	}

	CFIndex size = CFStringGetLength(tsd->serialNumber);
	char *s = new char[size+1];
	CFStringGetCString(tsd->serialNumber, s, size+1, kCFStringEncodingASCII);
	std::string serial(s);  // Copy the string to the stack
	delete[] s;

	ControllerChangeEventData *data = new ControllerChangeEventData;
	data->vid = tsd->vid;
	data->pid = tsd->pid;
	data->inserted = false;
	tsd->event->signal(data);

	// Free the data we're no longer using now that the device is going away
	CFRelease(tsd->serialNumber);

	IOObjectRelease(tsd->notification);

	delete tsd;
}

void ControllerListener::PrivateData::DeviceAdded(void *refCon, io_iterator_t iterator) {
	io_service_t usbDevice;

	PrivateData *pd = reinterpret_cast<PrivateData*> (refCon);

	while ((usbDevice = IOIteratorNext(iterator))) {
		TellStickData *tsd = new TellStickData;
		tsd->event = pd->event;

		// Get the serial number
		CFStringRef serialRef = reinterpret_cast<CFStringRef>(IORegistryEntryCreateCFProperty(  usbDevice, CFSTR("USB Serial Number" ), kCFAllocatorDefault, 0 ));
		if (serialRef == NULL) {
			// No serial number, we cannot continue. Sorry
			continue;
		}

		CFNumberRef vidRef = reinterpret_cast<CFNumberRef> (IORegistryEntryCreateCFProperty(usbDevice, CFSTR("idVendor"), kCFAllocatorDefault, 0));
		if (vidRef) {
			CFNumberGetValue(vidRef, kCFNumberIntType, &(tsd->vid));
			CFRelease(vidRef);
		}

		CFNumberRef pidRef = reinterpret_cast<CFNumberRef> (IORegistryEntryCreateCFProperty(usbDevice, CFSTR("idProduct"), kCFAllocatorDefault, 0));
		if (pidRef) {
			CFNumberGetValue(pidRef, kCFNumberIntType, &(tsd->pid));
			CFRelease(pidRef);
		}

		CFStringRef serialNumberAsCFString = CFStringCreateCopy(kCFAllocatorDefault, serialRef);
		tsd->serialNumber = serialNumberAsCFString;
		CFRelease(serialRef);

		// Register for an interest notification of this device being removed. Use a reference to our
		// private data as the refCon which will be passed to the notification callback.
		IOServiceAddInterestNotification(pd->gNotifyPort, usbDevice, kIOGeneralInterest, DeviceNotification, tsd, &(tsd->notification));

		CFIndex size = CFStringGetLength(serialNumberAsCFString);
		char *s = new char[size+1];
		CFStringGetCString(serialNumberAsCFString, s, size+1, kCFStringEncodingASCII);
		std::string serial(s);  // Copy the string to the stack
		delete[] s;

		IOObjectRelease(usbDevice);

		ControllerChangeEventData *data = new ControllerChangeEventData;
		data->vid = tsd->vid;
		data->pid = tsd->pid;
		data->inserted = true;
		tsd->event->signal(data);
	}
}
