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

class DomoticzInternal : public CDomoticzHardwareBase
{
      public:
	explicit DomoticzInternal(int ID);
	~DomoticzInternal() override = default;

	bool WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/) override
	{
		// nothing to do yet
		return false;
	};

      private:
	bool StartHardware() override
	{
		// nothing to do yet
		m_bIsStarted = true;
		return true;
	};
	bool StopHardware() override
	{
		// nothing to do yet
		m_bIsStarted = false;
		return true;
	};
};

#endif /* HARDWARE_DOMOTICZINTERNAL_H_ */
