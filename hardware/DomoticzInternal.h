/*
 * DomoticzInternal.h
 *
 *  Created on: 5 janv. 2016
 *      Author: gaudryc
 */
#pragma once

#ifndef HARDWARE_DOMOTICZINTERNAL_H_
#define HARDWARE_DOMOTICZINTERNAL_H_

#include "DomoticzHardware.h"

// Internl use only (eg. Security Panel device)
// User can not attach device to this type of hardware

class DomoticzInternal: public CDomoticzHardwareBase {
public:
	explicit DomoticzInternal(const int ID);
	~DomoticzInternal();

	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) {
		// nothing to do yet
		return false;
	};

private:
	bool StartHardware() {
		// nothing to do yet
		m_bIsStarted = true;
		return true;
	};
	bool StopHardware() {
		// nothing to do yet
		m_bIsStarted = false;
		return true;
	};

};

#endif /* HARDWARE_DOMOTICZINTERNAL_H_ */
