/*
This code implements basic I/O hardware via the Raspberry Pi's GPIO port, using the wiringpi library.
WiringPi is Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
It must be installed beforehand following instructions at http://wiringpi.com/download-and-install/

    Copyright (C) 2014 Vicne <vicnevicne@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.

This is a derivative work based on the samples included with wiringPi where distributed 
under the GNU Lesser General Public License version 3
Source: http://wiringpi.com
*/
#pragma once

#include "DomoticzHardware.h"
#include "GpioPin.h"
#include "../main/RFXtrx.h"

class CGpio : public CDomoticzHardwareBase
{
public:
	CGpio(const int ID);
	~CGpio();

	bool WriteToHardware(const char *pdata, const unsigned char length);
	
	static bool InitPins();
	static std::vector<CGpioPin> GetPinList();
	static CGpioPin* GetPPinById(int id);

private:
	bool StartHardware();
	bool StopHardware();

	void Do_Work();
	void ProcessInterrupt(int gpioId);
	
	// List of GPIO pin numbers, ordered as listed
	static std::vector<CGpioPin> pins;
	
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_waitcntr;
	tRBUF IOPinStatusPacket;

};
