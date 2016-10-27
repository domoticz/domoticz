#include "stdafx.h"
#include "Seahu.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/localtime_r.h"
#include "hardwaretypes.h"
// for I2c 
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
//#ifdef __arm__
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#endif
// not used include
//#include "../json/json.h"
//#include "../main/SQLHelper.h"
//#include "../main/WebServer.h"
//#include "../webserver/cWebem.h"

#define I2C_READ_INTERVAL 1
#define SEAHU_ID_ADD 0x4000   // base pseudorandom DeviceID for SEAHU devices

// PCF8574 (8-bit I/O expaner for I2C bus)
#define PCF8574_ADDRESS_1		0x20    /* I2C address for I/O expaner on base board - control relays and optical isolated input and output*/
#define PCF8574_ADDRESS_2		0x24    /* I2C address for I/O expaner on display board - control LCD-light and keys*/
#define SEAHU_I2C_BUS_PATH	"/dev/i2c-1"	/* path to kernel i2c bus. If use another version raspberrryPI tehn v3 may by must change to "/dev/i2c-0" */

typedef struct {
        char i2c_addr ; /* i2c address for PCF8574 chip */
        char mask;      /* 8-bit mask for select pin bit */
} SEAHU_I2C_PIN;

SEAHU_I2C_PIN seahuPins[16]={
	{ PCF8574_ADDRESS_1, 0x01 },
	{ PCF8574_ADDRESS_1, 0x02 },
	{ PCF8574_ADDRESS_1, 0x04 },
	{ PCF8574_ADDRESS_1, 0x08 },
	{ PCF8574_ADDRESS_1, 0x10 },
	{ PCF8574_ADDRESS_1, 0x20 },
	{ PCF8574_ADDRESS_1, 0x40 },
	{ PCF8574_ADDRESS_1, 0x80 },
	{ PCF8574_ADDRESS_2, 0x01 },
	{ PCF8574_ADDRESS_2, 0x02 },
	{ PCF8574_ADDRESS_2, 0x04 },
	{ PCF8574_ADDRESS_2, 0x08 },
	{ PCF8574_ADDRESS_2, 0x10 },
	{ PCF8574_ADDRESS_2, 0x20 },
	{ PCF8574_ADDRESS_2, 0x40 },
	{ PCF8574_ADDRESS_2, 0x80 }
};

CSeahu::CSeahu(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;

}

CSeahu::~CSeahu()
{
	m_bIsStarted=false;
}

void CSeahu::Init()
{
	// prepare create devices
	// my human name parametrs of function SendSwitch:
	//	( DeviceID, Unit , BatteryLevel, on-off , ?level? , name )
	SendSwitchIfNotExists(SEAHU_ID_ADD+0, 0, 255, 0, 0, "relay-1");
	SendSwitchIfNotExists(SEAHU_ID_ADD+1, 1, 255, 0, 0, "relay-2");
	SendSwitchIfNotExists(SEAHU_ID_ADD+2, 2, 255, 0, 0, "relay-3");
	SendSwitchIfNotExists(SEAHU_ID_ADD+3, 3, 255, 0, 0, "relay-4");
	SendSwitchIfNotExists(SEAHU_ID_ADD+4, 4, 255, 0, 0, "out-1");
	SendSwitchIfNotExists(SEAHU_ID_ADD+5, 5, 255, 0, 0, "out-2");
	SendSwitchIfNotExists(SEAHU_ID_ADD+6, 6, 255, 0, 0, "in-1");
	SendSwitchIfNotExists(SEAHU_ID_ADD+7, 7, 255, 0, 0, "in-2");
	SendSwitchIfNotExists(SEAHU_ID_ADD+8, 8, 255, 0, 0, "key-LEFT");
	SendSwitchIfNotExists(SEAHU_ID_ADD+9, 9, 255, 0, 0, "key-RIGHT");
	SendSwitchIfNotExists(SEAHU_ID_ADD+10, 10, 255, 0, 0, "key-UP");
	SendSwitchIfNotExists(SEAHU_ID_ADD+11, 11, 255, 0, 0, "key-DOWN");
	SendSwitchIfNotExists(SEAHU_ID_ADD+12, 12, 255, 0, 0, "key-SELECT");
	SendSwitchIfNotExists(SEAHU_ID_ADD+13, 13, 255, 0, 0, "key-ESC");
	SendSwitchIfNotExists(SEAHU_ID_ADD+14, 14, 255, 0, 0, "LCD-light");
	SendSwitchIfNotExists(SEAHU_ID_ADD+15, 15, 255, 0, 0, "beep");
	// default setting of PIO expander (same PIO is used as INPUT (keyboard and optoisolatd input) with open colector, therefore must be set to 1)
	// and this device my by controled by other programs, then when ini procces should't change output PIO.
	char buf = 0;	
	int fd = openI2C(SEAHU_I2C_BUS_PATH);
	if (fd < 0) return;
	// base board
	if ( readByteI2C(fd, &buf, PCF8574_ADDRESS_1 ) < 0 ) return;
	buf=buf|0x30;
	if (writeByteI2C(fd, buf, PCF8574_ADDRESS_1) < 0 ) return;
	// display board
	if ( readByteI2C(fd, &buf, PCF8574_ADDRESS_2 ) < 0 ) return;
	buf=buf|0x3F;
	if (writeByteI2C(fd, buf, PCF8574_ADDRESS_2) < 0 ) return;
	close(fd);
}

bool CSeahu::StartHardware()
{
	Init();
	m_stoprequested = false;
	
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSeahu::Do_Work, this)));
	sOnConnected(this);
	m_bIsStarted=true;
	return true;
}

bool CSeahu::StopHardware()
{
	m_stoprequested = true;
	if (m_thread != NULL)
		m_thread->join();
	m_bIsStarted=false;
	return true;
}

bool CSeahu::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if ((pCmd->LIGHTING2.packettype == pTypeLighting2)) {
		_log.Log(LOG_NORM,"GPIO: packetlength %d", pCmd->LIGHTING2.packetlength);
		_log.Log(LOG_NORM,"GPIO: packettype %d", pCmd->LIGHTING2.packettype);
		_log.Log(LOG_NORM,"GPIO: subtype %d", pCmd->LIGHTING2.subtype);
		_log.Log(LOG_NORM,"GPIO: seqnbr %d", pCmd->LIGHTING2.seqnbr);
		_log.Log(LOG_NORM,"GPIO: id1 %d", pCmd->LIGHTING2.id1);
		_log.Log(LOG_NORM,"GPIO: id2 %d", pCmd->LIGHTING2.id2);
		_log.Log(LOG_NORM,"GPIO: id3 %d", pCmd->LIGHTING2.id3);
		_log.Log(LOG_NORM,"GPIO: id4 %d", pCmd->LIGHTING2.id4);
		_log.Log(LOG_NORM,"GPIO: unitcode %d", pCmd->LIGHTING2.unitcode); // in DB columb "Unit" used for identify number switch on board
		_log.Log(LOG_NORM,"GPIO: cmnd %d", pCmd->LIGHTING2.cmnd);
		_log.Log(LOG_NORM,"GPIO: level %d", pCmd->LIGHTING2.level);
		char gpioId = pCmd->LIGHTING2.unitcode;
		char  value = pCmd->LIGHTING2.cmnd;
		_log.Log(LOG_NORM,"GPIO: Write to pi %d value: %d", gpioId, value);
		if (WritePin( gpioId, value)<0) return false;
		else return true;
	}
	else {
		_log.Log(LOG_NORM,"GPIO: WriteToHardware packet type %d or subtype %d unknown", pCmd->LIGHTING1.packettype, pCmd->LIGHTING1.subtype);
		return false;
	}
}

void CSeahu::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;
	_log.Log(LOG_STATUS, "%s: Worker started...", device.c_str());
	while (!m_stoprequested)
	{
		sleep_milliseconds(100);
		if (m_stoprequested)
			break;
		msec_counter++;
		if (msec_counter == 10)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 10 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			if (sec_counter % I2C_READ_INTERVAL == 0)
			{
				try
				{
					ReadSwitchs();
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "%s: Error reading sensor data!...", device.c_str());
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "%s: Worker stopped...", device.c_str());
}

void CSeahu::ReadSwitchs()
{	
	char buf = 0;
	int fd = openI2C(SEAHU_I2C_BUS_PATH);
	if (fd < 0) return;
	// paramers of function SendSwitch: 
	//	( NodeID , ChildID , BatteryLevel , bOn , Level , defaultname )
	// eqivalent structure LIGHTING2 :
	//	( NodeID->id1,id2,id3,id4 , ChildID->unitcode , BatteryLevel->? , bOn->cmd , Level , defaultname->? , packettype=pTypeLighting2 , subtype=sTypeAC )
	// eqivalent DB cloumb in DeviceStatus table:
	//	( NodeID->DeviceID , ChildID->Unit , BatteryLevel->BatteryLevel, bOn->nValue , Level->?LastLevel, defaultname->Name , Type=pTypeLighting2 , SubType=sTypeAC )
	// my human name parametrs of function SendSwitch:
	//	( DeviceID, Unit , BatteryLevel, on-off , ?level? , name )
	// send new value to switch records
	if ( readByteI2C(fd, &buf, PCF8574_ADDRESS_1) < 0 ) return;
	SendSwitch(SEAHU_ID_ADD+0, 0, 255, (bool) (buf & 0x01), 0, "relay-1");
	SendSwitch(SEAHU_ID_ADD+1, 1, 255, (bool) (buf & 0x02), 0, "relay-2");
	SendSwitch(SEAHU_ID_ADD+2, 2, 255, (bool) (buf & 0x04), 0, "relay-3");
	SendSwitch(SEAHU_ID_ADD+3, 3, 255, (bool) (buf & 0x08), 0, "relay-4");
	SendSwitch(SEAHU_ID_ADD+4, 4, 255, (bool) (buf & 0x10), 0, "out-1");
	SendSwitch(SEAHU_ID_ADD+5, 5, 255, (bool) (buf & 0x20), 0, "out-2");
	SendSwitch(SEAHU_ID_ADD+6, 6, 255, (bool) (buf & 0x40), 0, "in-1");
	SendSwitch(SEAHU_ID_ADD+7, 7, 255, (bool) (buf & 0x80), 0, "in-2");
	if ( readByteI2C(fd, &buf, PCF8574_ADDRESS_2) < 0 ) return;
	// send new value to switch records
	SendSwitch(SEAHU_ID_ADD+8, 8, 255,   (bool) (buf & 0x01), 0, "key-LEFT");
	SendSwitch(SEAHU_ID_ADD+9, 9, 255, (bool) (buf & 0x02), 0, "key-RIGHT");
	SendSwitch(SEAHU_ID_ADD+10, 10, 255, (bool) (buf & 0x04), 0, "key-UP");
	SendSwitch(SEAHU_ID_ADD+11, 11, 255, (bool) (buf & 0x08), 0, "key-DOWN");
	SendSwitch(SEAHU_ID_ADD+12, 12, 255, (bool) (buf & 0x10), 0, "key-SELECT");
	SendSwitch(SEAHU_ID_ADD+13, 13, 255, (bool) (buf & 0x20), 0, "key-ESC");
	SendSwitch(SEAHU_ID_ADD+14, 14, 255, (bool) (buf & 0x40), 0, "LCD-light");
	SendSwitch(SEAHU_ID_ADD+15, 15, 255, (bool) (buf & 0x80), 0, "beep");
	close(fd);
}


char CSeahu::ReadPin(char gpioId, char *buf)
{	
	int fd = openI2C(SEAHU_I2C_BUS_PATH);
	if (fd < 0) return -1;
	if ( readByteI2C(fd, buf, seahuPins[gpioId].i2c_addr) < 0 ) return -2;
	if ( *buf & seahuPins[gpioId].mask ) *buf=1;
	else *buf=0;
	close(fd);
	return *buf;
}

char CSeahu::WritePin(char gpioId,char  value)
{	
	_log.Log(LOG_ERROR, "WRITE SEAHU DEVICE n.%d value %d", gpioId, value);
	char buf = 0;	
	int fd = openI2C(SEAHU_I2C_BUS_PATH);
	if (fd < 0) return -1;
	if ( readByteI2C(fd, &buf, seahuPins[gpioId].i2c_addr) < 0 ) return -2;
	lseek(fd,0,SEEK_SET); // jen pro zkouseni pri zapisu do souboru (protoze pri ceteni se posune kurzor, tak ho vratim zpatky)
	_log.Log(LOG_ERROR, "actual value byte %d", buf);
	if (value==1) buf = buf | seahuPins[gpioId].mask;	//prepare new value by combinate curent value, mask and new value
	else buf = buf & ~seahuPins[gpioId].mask;
	_log.Log(LOG_ERROR, "new value byte %d", buf);

	if (writeByteI2C(fd, buf, seahuPins[gpioId].i2c_addr) < 0 ) return -3;
	close(fd);
	_log.Log(LOG_ERROR, "WRITE ON SEAHU DEVICE n.%d value %d is OK", gpioId, value);
	return 1;
}

int CSeahu::openI2C(const char *path)
{
#ifndef __arm__
	return -1;
#else
	// open device
	int fd = open(path, O_RDWR);
	//int fd = open("pins.txt", O_RDWR); //for test withou i2c device
	if (fd < 0) {
		_log.Log(LOG_ERROR, "%s:%d Error opening device!...", device.c_str(), fd);
		return fd;
	}
	return fd;
#endif
}


char CSeahu::readByteI2C(int file, char *byte, char i2c_addr)
{
#ifndef __arm__
	return -1;
#else
	// set I2C address to will be comunicate (frist addres = chip on base board)
	if (ioctl(file, I2C_SLAVE_FORCE, i2c_addr) < 0) {
		_log.Log(LOG_ERROR, "%s: Failed to acquire bus access and/or talk to slave with address %d", device.c_str(), i2c_addr);
		return -1;
	}
	//read from I2C device
	if (read(file,byte,1) != 1) {
		_log.Log(LOG_ERROR, "%s: Failed to read from the i2c bus with address %d", device.c_str(), i2c_addr);
		return -2;
	}
	return 1;
#endif
}

char CSeahu::writeByteI2C(int file, char byte, char i2c_addr)
{
#ifndef __arm__
	return false;
#else
	// set I2C address to will be comunicate (frist addres = chip on base board)
	if (ioctl(file, I2C_SLAVE_FORCE, i2c_addr) < 0) {
		_log.Log(LOG_ERROR, "%s: Failed to acquire bus access and/or talk to slave with address %d", device.c_str(), i2c_addr);
		return -1;
	}
	//write to I2C device
	if (write(file,&byte,1) != 1) {
		_log.Log(LOG_ERROR, "%s: Failed write to the i2c bus with address %d", device.c_str(), i2c_addr);
		return -2;
	}
	return 1;
#endif
}