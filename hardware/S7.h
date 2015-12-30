/*
This code implements basic functionality for siemens S7 PLC's

    Copyright (C) 2015 Wouter <w.deborger@gmail.com>

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

*/
#pragma once

#include "DomoticzHardware.h"
#include "../snap7/snap7.h"
#include "../main/RFXtrx.h"


#include <string>
#include <stdint.h>

class S7;

class PLCDevice{
public:
	PLCDevice(std::string name, int id,int offset);

	virtual void decode(S7 *target, uint8_t buffer[], bool diff) = 0;
	virtual bool WriteToHardware(TS7Client *client, const char *pdata, const unsigned char length) = 0;

	// in bytes
	virtual int start();
	// byte after last
	virtual int end() = 0;

	std::string name;
	//in bits
	int offset;
	int id;

	//types
	virtual int defaultSwitchType() = 0;

};

class Lamp: public PLCDevice {
public:
	Lamp(std::string name, int id,int offset);
	virtual int end();

	bool state;

	virtual void decode(S7 *target, uint8_t buffer[], bool diff);

	//false is off, 0 is off
	void set(TS7Client * client, bool stat);


	virtual bool WriteToHardware(TS7Client *client, const char *pdata, const unsigned char length);


	virtual int defaultSwitchType();
};


class Dimmer: public PLCDevice {
public:
	Dimmer(std::string name, int id,int offset);
	virtual int end();

	uint16_t state;

	virtual void decode(S7 *target, uint8_t buffer[], bool diff);

	//false is off, 0 is off
	void set(TS7Client * client,uint16_t state);


	virtual bool WriteToHardware(TS7Client *client, const char *pdata, const unsigned char length);

	virtual int defaultSwitchType();

};


class S7 : public CDomoticzHardwareBase
{
public:
	S7(const int ID,std::string Address);
	~S7();

	bool WriteToHardware(const char *pdata, const unsigned char length);
	
	std::vector<PLCDevice*> GetDeviceList();
	PLCDevice* GetDeviceById(int id);

private:
	bool StartHardware();
	bool Connect();
	bool Disconnect();
	bool getConfig();
	bool StopHardware();

	void Do_Work();
	
	// List of GPIO pin numbers, ordered as listed
	std::vector<PLCDevice*> devices;
	
	boost::shared_ptr<boost::thread> m_thread;


	uint8_t *buffer;
	std::string host;

	int db;
	int start;
	int size;
	TS7Client *connection;

	friend class Lamp;
	friend class Dimmer;
	tRBUF IOPinStatusPacket;


};
