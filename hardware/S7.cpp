#ifdef WITH_S7

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
#include "S7.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include "../../yaml-cpp/yaml.h"

#include "../main/SQLHelper.h"
extern CSQLHelper m_sql;


#define SCALE 64
/*
 * Inspired by GPIO implementation, inspired by other hardware implementations such as PiFace and EnOcean
 */
S7::S7(const int ID, std::string address) {

	m_HwdID = ID;

	host = address;

	//Prepare a generic packet info for LIGHTING1 packet, so we do not have to do it for every packet
	//todo: remove this, may cause trouble?
	IOPinStatusPacket.LIGHTING1.packetlength =
			sizeof(IOPinStatusPacket.LIGHTING1) - 1;
	IOPinStatusPacket.LIGHTING1.housecode = 0;
	IOPinStatusPacket.LIGHTING1.packettype = pTypeLighting1;
	IOPinStatusPacket.LIGHTING1.subtype = sTypeIMPULS;
	IOPinStatusPacket.LIGHTING1.rssi = 12;
	IOPinStatusPacket.LIGHTING1.seqnbr = 0;
}

S7::~S7(void) {
}

std::vector<PLCDevice *> S7::GetDeviceList() {
	return devices;
}

PLCDevice* S7::GetDeviceById(int id) {
	return devices[id];
}

bool S7::getConfig() {
	try {

		//read config file
		YAML::Node config = YAML::LoadFile("S7-config.yaml");
		//get host, db

//		if (config["host"].IsNull()) {
//			_log.Log(LOG_ERROR, "S7: config does not contain host attribute");
//			return false;
//		}
//
//		this->host = config["host"].as<std::string>();

		if (config["db"].IsNull()) {
			_log.Log(LOG_ERROR, "S7: config does not contain db attribute");
			return false;
		}

		this->db = config["db"].as<int>();

		//read devices, place in list, assign sequential ids
		YAML::Node entries = config["devices"];

		if (entries.IsNull() || entries.size() == 0) {
			_log.Log(LOG_ERROR,
					"S7: config does not contain entries attribute");
			return false;
		}

		devices.reserve(entries.size());

		for (std::size_t i = 0; i < entries.size(); i++) {
			try {
				if ("lamp" == entries[i]["type"].as<std::string>()) {
					devices.push_back(
							new Lamp(entries[i]["name"].as<std::string>(), i,
									entries[i]["offset"].as<int>()));
				}
				if ("dimmer" == entries[i]["type"].as<std::string>()) {
					devices.push_back(
							new Dimmer(entries[i]["name"].as<std::string>(), i,
									entries[i]["offset"].as<int>()));
				}
			} catch (YAML::BadConversion& e) {
				_log.Log(LOG_ERROR, "S7: can not parse entry: %s", e.what());

			}
		}

		//determine min/max
		start = INT_MAX;
		int stop = 0;
		for (std::vector<PLCDevice*>::const_iterator i = devices.begin();
				i != devices.end(); ++i) {
			start = std::min(start, (*i)->start());
			stop = std::max(stop, (*i)->end());
		}

		size = stop - start;

		//allocate buffer
		buffer = new uint8_t[size];

		return true;
	} catch (YAML::Exception& e) {
		_log.Log(LOG_ERROR, "S7: config invalid: %s", e.what());
		return false;
	}
}

void writeNameAndSType(int m_HwdID, PLCDevice* dev) {
	m_sql.safe_query("UPDATE DeviceStatus SET Name='%s', SwitchType='%d' WHERE (HardwareID==%d) AND (Unit == %d)",dev->name.c_str(),dev->defaultSwitchType(),m_HwdID,dev->id);
}


bool S7::Connect() {
	connection = new TS7Client();
	int ret = connection->ConnectTo(host.c_str(), 0, 2);
	if (ret != 0) {
		char err[128];
		Cli_ErrorText(ret, err, 128);
		_log.Log(LOG_ERROR, "S7: can not connect: %s", err);
		return false;
	}

	connection->DBRead(db, start, size, buffer);

	for (std::vector<PLCDevice*>::const_iterator i = devices.begin();
			i != devices.end(); ++i) {
		(*i)->decode(this, buffer, false);
		writeNameAndSType(m_HwdID, (*i));
	}

	return true;

}

bool S7::Disconnect() {
	int ret = connection->Disconnect();
	if (ret != 0) {
		char err[128];
		Cli_ErrorText(ret, err, 128);
		_log.Log(LOG_ERROR, "S7: can not disconnect: %s", err);
		return false;
	}
	delete connection;
}

bool S7::StartHardware() {

	if (!this->getConfig()) {
		_log.Log(LOG_ERROR, "S7: no config found.");
		return false;
	}
	if (!this->Connect()) {
		_log.Log(LOG_ERROR, "S7: could not connect.");
		return false;
	}

	//Start worker thread that will be responsible for interrupt handling
	m_thread = boost::shared_ptr<boost::thread>(
			new boost::thread(boost::bind(&S7::Do_Work, this)));

	sOnConnected(this);

	return (m_thread != NULL);
}

bool S7::StopHardware() {
	if (m_thread != NULL) {
		m_thread->interrupt();
		m_thread->join();
	}

	m_bIsStarted = false;

	Disconnect();

	free(this->buffer);

	return true;
}

bool S7::WriteToHardware(const char *pdata, const unsigned char length) {

	tRBUF *pCmd = (tRBUF*) pdata;

	int id;

	if (pCmd->LIGHTING1.packettype == pTypeLighting1) {
		unsigned char housecode = (pCmd->LIGHTING1.housecode);
		if (housecode != 0) {
			_log.Log(LOG_NORM, "S7: wrong housecode %d",
					static_cast<int>(housecode));
			return false;
		}
		id = pCmd->LIGHTING1.unitcode;
	} else if (pCmd->LIGHTING1.packettype == pTypeLighting2) {
		id = pCmd->LIGHTING2.unitcode;
	} else {
		_log.Log(LOG_NORM,
				"S7: WriteToHardware packet type %d or subtype %d unknown",
				pCmd->LIGHTING1.packettype, pCmd->LIGHTING1.subtype);
		return false;
	}

	_log.Log(LOG_NORM, "S7: WriteToHardware housecode %d, packetlength %d",
			pCmd->LIGHTING1.housecode, pCmd->LIGHTING1.packetlength);

	PLCDevice *p = this->GetDeviceById(id);

	return p->WriteToHardware(this->connection, pdata, length);

}

bool Lamp::WriteToHardware(TS7Client *client, const char *pdata,
		const unsigned char length) {

	tRBUF *pCmd = (tRBUF*) pdata;

	if ((pCmd->LIGHTING1.packettype == pTypeLighting1)
			&& (pCmd->LIGHTING1.subtype == sTypeIMPULS)) {
		unsigned char housecode = (pCmd->LIGHTING1.housecode);
		int gpioId = pCmd->LIGHTING1.unitcode;
		_log.Log(LOG_NORM, "S7: device %s (#%d) state was %d", name.c_str(),
				gpioId, state);

		bool oldvalue = state;
		bool newValue = static_cast<bool>(pCmd->LIGHTING1.cmnd);
		this->set(client, newValue);

		_log.Log(LOG_NORM,
				"GPIO: WriteToHardware housecode %d, device %d, previously %d, set %d",
				static_cast<int>(housecode), static_cast<int>(gpioId), state,
				newValue);

	} else {
		_log.Log(LOG_NORM,
				"GPIO: WriteToHardware packet type %d or subtype %d unknown",
				pCmd->LIGHTING1.packettype, pCmd->LIGHTING1.subtype);
		return false;
	}
	return true;

}

void S7::Do_Work() {
	boost::posix_time::milliseconds duration(12000);

	_log.Log(LOG_NORM, "GPIO: Worker started...");

	while (true) {
		sleep_milliseconds(100);

		SetHeartbeatReceived();

		int err = connection->DBRead(db, start, size, buffer);

		if (err != 0) {
			char errt[128];
			Cli_ErrorText(err, errt, 128);
			_log.Log(LOG_ERROR, "S7: can not connect: %s", errt);
			bool rec = Connect();
			if (!rec) {
				sleep_milliseconds(1000);
			}
		} else {

			for (std::vector<PLCDevice*>::const_iterator i = devices.begin();
					i != devices.end(); ++i) {
				(*i)->decode(this, buffer, true);
			}
		}

	}
	_log.Log(LOG_NORM, "S7: Worker stopped...");
}

PLCDevice::PLCDevice(std::string name, int id, int offset) {
	this->name = name;
	this->offset = offset;
	this->id = id;
}

int PLCDevice::start() {
	return offset / 8;
}

Lamp::Lamp(std::string name, int id, int offset) :
		PLCDevice::PLCDevice(name, id, offset) {
	this->state = false;
}
;

int Lamp::defaultSwitchType(){
	return sTypeX10;
}


int Lamp::end() {
	return start() + 1;
}

void Lamp::decode(S7 *target, uint8_t buffer[], bool diff) {
	uint8_t b = buffer[offset / 8];
	b = (b >> (offset % 8)) & 1;

	bool oldstate = state;
	state = b;
	if (!diff || oldstate != state) {

		if (state) {
			target->IOPinStatusPacket.LIGHTING1.cmnd = light1_sOn;
		} else {
			target->IOPinStatusPacket.LIGHTING1.cmnd = light1_sOff;
		}

		unsigned char seqnr = target->IOPinStatusPacket.LIGHTING1.seqnbr;
		seqnr++;
		target->IOPinStatusPacket.LIGHTING1.seqnbr = seqnr;
		target->IOPinStatusPacket.LIGHTING1.unitcode = id;
		target->sDecodeRXMessage(target,
				(const unsigned char *) &target->IOPinStatusPacket,NULL, 255);
	}
}

void Lamp::set(TS7Client *client, bool stat) {
	this->state = stat;
	char byte = offset / 8;
	uint8_t mydb[2];
	client->DBRead(1, byte, 1, &mydb);
	if (stat) {
		mydb[0] |= (1 << (offset % 8));
	} else {
		mydb[0] &= (1 << (offset % 8));
	}
	client->DBWrite(1, byte, 1, &mydb);
}

Dimmer::Dimmer(std::string name, int id, int offset) :
		PLCDevice::PLCDevice(name, id, offset) {
	this->state = 0;
}
;

int Dimmer::end() {
	return start() + 2;
}




int Dimmer::defaultSwitchType(){
	return sTypePhilips;
}


void Dimmer::decode(S7 *target, uint8_t buffer[], bool diff) {

	uint8_t a = buffer[offset / 8];
	uint8_t b = buffer[(offset / 8) + 1];

	uint16_t oldstate = state;
	state = a << 8 | b;
	if (!diff || oldstate != state) {

		unsigned char seqnr = target->IOPinStatusPacket.LIGHTING1.seqnbr;
		seqnr++;

		BYTE cmd;
		BYTE level;
		if (state == 0) {
			cmd = light2_sOff;
			level = 0;
		} else {
			cmd = light2_sSetLevel;
			level = state /SCALE;
		}

		tRBUF lcmd;
		memset(&lcmd, 0, sizeof(RBUF));
		lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
		lcmd.LIGHTING2.packettype = pTypeLighting2;
		lcmd.LIGHTING2.subtype = sTypeZWaveSwitch;
		lcmd.LIGHTING2.seqnbr = seqnr;
		lcmd.LIGHTING2.id1 = 0;
		lcmd.LIGHTING2.id2 = 0;
		lcmd.LIGHTING2.id3 = 0;
		lcmd.LIGHTING2.id4 = 0;
		lcmd.LIGHTING2.unitcode = id;
		lcmd.LIGHTING2.cmnd = cmd;
		lcmd.LIGHTING2.level = level;
		lcmd.LIGHTING2.filler = 0;
		lcmd.LIGHTING2.rssi = 0;

		target->sDecodeRXMessage(target,
				(const unsigned char *) &lcmd.LIGHTING2,NULL, 255);

	}
}

void Dimmer::set(TS7Client * client, uint16_t stat) {
	this->state = stat*SCALE;
	char byte = offset / 8;
	uint8_t mydb[3];
	mydb[0] = state >> 8;
	mydb[1] = state & 0xff;

	client->DBWrite(1, byte, 2, &mydb);
}

bool Dimmer::WriteToHardware(TS7Client *client, const char *pdata,
		const unsigned char length) {
	tRBUF *pCmd = (tRBUF*) pdata;
	_log.Log(LOG_NORM,
			"S7: WriteToHardware packet type %d or subtype %d unknown; cmd %d; level %d ",
			pCmd->LIGHTING2.packettype, pCmd->LIGHTING2.subtype,
			pCmd->LIGHTING2.cmnd, pCmd->LIGHTING2.level);



	if (pCmd->LIGHTING2.packettype == pTypeLighting2) {
		_log.Log(LOG_NORM, "S7: device %s (#%d) state was %d", name.c_str(), id,
				state);

		uint16_t oldvalue = state;
		uint16_t newvalue;
		switch (pCmd->LIGHTING2.cmnd) {
		case light2_sOn:
			newvalue = 15;
			break;
		case light2_sOff:
			newvalue = 0;
			break;
		case light2_sSetLevel:
			newvalue = pCmd->LIGHTING2.level+1;
			break;
		}
		set(client,newvalue);
		_log.Log(LOG_NORM,
				"S7: WriteToHardware device %d, previously %d, level: %d",
				 static_cast<int>(id), state,
				 newvalue);

	} else {
		_log.Log(LOG_NORM,
				"GPIO: WriteToHardware packet type %d or subtype %d unknown",
				pCmd->LIGHTING1.packettype, pCmd->LIGHTING1.subtype);
		return false;
	}
	return true;

	// light2_sOn
	// light2_sOff
	// light2_sSetLevel
	return true;

}

#endif
