#include "stdafx.h"
#include "HTU21D.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef __arm__
	#include <linux/i2c-dev.h>
	#include <linux/i2c.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif
#include <math.h>
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define I2C_READ_INTERVAL 30

#define sleepms(ms)  usleep((ms)*1000)

// HTU21D registers
#define HTU21D_ADDRESS     				0x40    /* I2C address */
#define USER_REGISTER_WRITE 				0xE6    /* Write user register*/
#define USER_REGISTER_READ  				0xE7    /* Read  user register*/
#define SOFT_RESET          				0xFE    /* Soft Reset (takes 15ms). Switch sensor OFF & ON again. All registers set to default exept heater bit. */
#define TEMP_COEFFICIENT    				-0.15   /* Temperature coefficient (from 0deg.C to 80deg.C) */
#define CRC8_POLYNOMINAL    				0x13100 /* CRC8 polynomial for 16bit CRC8 x^8 + x^5 + x^4 + 1 */
#define TRIGGER_HUMD_MEASURE_HOLD   0xE5  /* Trigger Humidity Measurement. Hold master (SCK line is blocked) */
#define TRIGGER_TEMP_MEASURE_HOLD   0xE3  /* Trigger Temperature Measurement. Hold master (SCK line is blocked) */
#define TRIGGER_HUMD_MEASURE_NOHOLD 0xF5   /* Trigger Humidity Measurement. No Hold master (allows other I2C communication on a bus while sensor is measuring) */
#define TRIGGER_TEMP_MEASURE_NOHOLD 0xF3   /* Trigger Temperature Measurement. No Hold master (allows other I2C communication on a bus while sensor is measuring) */
#define TEMP_DELAY							 		70   /* Maximum required measuring time for a complete temperature read */
#define HUM_DELAY							 		  36   /* Maximum required measuring time for a complete humidity read */

CHTU21D::CHTU21D(const int ID)
{
	m_stoprequested=false;
	m_HwdID=ID;
	m_ActI2CBus = "/dev/i2c-1";
	if (!i2c_test(m_ActI2CBus.c_str()))
	{
		m_ActI2CBus = "/dev/i2c-0";
	}
}

CHTU21D::~CHTU21D()
{
}

bool CHTU21D::StartHardware()
{
#ifndef __arm__
	return false;
#endif
	m_stoprequested=false;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CHTU21D::Do_Work, this)));
	sOnConnected(this);
	m_bIsStarted=true;
	return (m_thread!=NULL);
}

bool CHTU21D::StopHardware()
{
	m_stoprequested=true;
	if (m_thread!=NULL)
		m_thread->join();
	m_bIsStarted=false;
	return true;
}

bool CHTU21D::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CHTU21D::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = I2C_READ_INTERVAL - 5;
  _log.Log(LOG_STATUS,"HTU21D: Worker started...");
	while (!m_stoprequested)
	{
		sleep_milliseconds(500);
		if (m_stoprequested)
			break;
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			if (sec_counter % I2C_READ_INTERVAL == 0)
			{
				try
				{
					ReadSensorDetails();
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "HTU21D: Error reading sensor data!...");
				}
			}
		}
	}
	_log.Log(LOG_STATUS,"HTU21D: Worker stopped...");
}

//returns true if it could be opened
bool CHTU21D::i2c_test(const char *I2CBusName)
{
#ifndef __arm__
	return false;
#else
	int fd;
	//Open port for reading and writing
	if ((fd = open(I2CBusName, O_RDWR)) < 0)
		return false;
	close(fd);
	return true;
#endif
}

// Returns a file id for the port/bus
int CHTU21D::i2c_Open(const char *I2CBusName)
{
#ifndef __arm__
	return -1;
#else
	int fd;
	//Open port for reading and writing
	if ((fd = open(I2CBusName, O_RDWR)) < 0)
	{
		_log.Log(LOG_ERROR, "HTU21D: Failed to open the i2c bus!...");
		_log.Log(LOG_ERROR, "HTU21D: Check to see if you have a bus: %s", I2CBusName);
		_log.Log(LOG_ERROR, "HTU21D: We might only be able to access this as root user");
		return -1;
	}
	return fd;
#endif
}

int CHTU21D::ReadInt(int fd, uint8_t *devValues, uint8_t bytesToRead)
{
#ifndef __arm__
	return -1;
#else
	int rc;
	struct i2c_rdwr_ioctl_data messagebuffer;

	//Build a register read command
	//Requires one complete message containing a reg address and command
	struct i2c_msg read_reg[1] = {
		{ HTU21D_ADDRESS, I2C_M_RD, bytesToRead, devValues }
	};

	messagebuffer.nmsgs = 1;                  //One message/action
	messagebuffer.msgs = read_reg;            //load the 'read__reg' message into the buffer
	rc = ioctl(fd, I2C_RDWR, &messagebuffer); //Send the buffer to the bus and returns a send status
	if (rc < 0){
		return rc;
	}
	//note that the return data is contained in the array pointed to by devValues (passed by-ref)
	return 0;
#endif
}

int CHTU21D::WriteCmd(int fd, uint8_t devAction)
{
#ifndef __arm__
	return -1;
#else
	int rc;
	struct i2c_rdwr_ioctl_data messagebuffer;
	uint8_t datatosend[2];

	datatosend[0] = devAction;
	//Build a register write command
	//Requires one complete message containing a reg address and command
	struct i2c_msg write_reg[1] = {
		{ HTU21D_ADDRESS, 0, 2, datatosend }
	};

	messagebuffer.nmsgs = 1;                  //One message/action
	messagebuffer.msgs = write_reg;           //load the 'write__reg' message into the buffer
	rc = ioctl(fd, I2C_RDWR, &messagebuffer); //Send the buffer to the bus and returns a send status
	if (rc < 0){
		return rc;
	}
	return 0;
#endif
}
/**************************************************************************/
/*
    Calculates CRC8 for 16 bit received Data
    NOTE:
    For more info about Cyclic Redundancy Check (CRC) see:
    http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
*/
/**************************************************************************/
int CHTU21D::checkCRC8(uint16_t data)
{
	unsigned int bit;
  for (bit = 0; bit < 16; bit++)
  {
		if (data & 0x8000)
    {
      data =  (data << 1) ^ CRC8_POLYNOMINAL;
    }
    else
    {
      data <<= 1;
    }
  }
  data >>= 8;
  return data;
}

int CHTU21D::GetHumidity(int fd, float *Hum)
{
#ifndef __arm__
	return -1;
#else
	uint16_t rawHumidity;
	uint8_t rValues[3];
	uint8_t Checksum;

	if (WriteCmd(fd, (TRIGGER_HUMD_MEASURE_HOLD)) != 0) return -1;

	sleepms(HUM_DELAY);

	if (ReadInt(fd, rValues, 3) != 0) return -1;
	rawHumidity = ((rValues[0] << 8) | rValues[1]);
	Checksum = rValues[2];
	if (checkCRC8(rawHumidity) != Checksum)
	{
		_log.Log(LOG_ERROR, "HTU21D: Incorrect humidity checksum!...");
    return -1;
  }
	rawHumidity ^=  0x02;
	*Hum = -6 + 0.001907 * (float)rawHumidity;
	return 0;
#endif
}

int CHTU21D::GetTemperature(int fd, float *Temp)
{
#ifndef __arm__
	return -1;
#else
	uint16_t rawTemperature;
	uint8_t rValues[3];
	uint8_t Checksum;

	if (WriteCmd(fd, TRIGGER_TEMP_MEASURE_HOLD) != 0) {
		_log.Log(LOG_ERROR, "HTU21D: Error writing I2C!...");
		return -1;
	}
	sleepms(TEMP_DELAY);
	if (ReadInt(fd, rValues, 3) != 0) {
		_log.Log(LOG_ERROR, "HTU21D: Error reading I2C!...");
		return -1;
	}
	rawTemperature = ((rValues[0] << 8) | rValues[1]);
	Checksum = rValues[2];
	if (checkCRC8(rawTemperature) != Checksum)
  {
		_log.Log(LOG_ERROR, "HTU21D: Incorrect temperature checksum!...");
    return -1;
  }
  *Temp = -46.85 + 0.002681 * (float)rawTemperature;
	return 0;
#endif
}

void CHTU21D::ReadSensorDetails()
{
	float temperature, humidity;

#ifndef __arm__
	temperature = 21.3;
	humidity = 45;
#else
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) {
		_log.Log(LOG_ERROR, "HTU21D: Error opening device!...");
		return;
	}
	if (GetTemperature(fd, &temperature) < 0) {
		_log.Log(LOG_ERROR, "HTU21D: Error reading temperature!...");
		close(fd);
		return;
	}
	if (GetHumidity(fd, &humidity) < 0) {
		_log.Log(LOG_ERROR, "HTU21D: Error reading humidity!...");
		close(fd);
		return;
	}
	close(fd);
	if (temperature >= 0 && temperature <= 80)
		humidity = humidity + (25 - temperature) * TEMP_COEFFICIENT;
#endif

  RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
	tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype = sTypeHTU21D_TC;
	tsen.TEMP_HUM.rssi = 12;
	tsen.TEMP_HUM.id1 = 1;
	tsen.TEMP_HUM.id2 = 1;
	tsen.TEMP_HUM.tempsign = (temperature >= 0) ? 0 : 1;
	int at10 = round(abs(temperature*10.0f));
	tsen.TEMP_HUM.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP_HUM.temperatureh * 256);
	tsen.TEMP_HUM.temperaturel = (BYTE)(at10);
	tsen.TEMP_HUM.humidity = (BYTE)humidity;
	tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);
  sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM, NULL, 255);

	memset(&tsen, 0, sizeof(RBUF));
	tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
	tsen.HUM.packettype = pTypeHUM;
	tsen.HUM.subtype = sTypeHTU21D;
	tsen.HUM.rssi = 12;
	tsen.HUM.id1 = 1;
	tsen.HUM.id2 = 2;
	tsen.HUM.humidity = (BYTE)humidity;
	tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM, NULL, 255);
}
