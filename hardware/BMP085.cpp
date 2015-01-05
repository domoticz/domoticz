/*
This code is based on:
Raspberry Pi Bosch BMP085 communication code.
By:      John Burns (www.john.geek.nz)
Date:    13 February 2013
License: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/

This is a derivative work based on:
	BMP085 Extended Example Code
	by: Jim Lindblom
	SparkFun Electronics
	date: 1/18/11
	license: CC BY-SA v3.0 - http://creativecommons.org/licenses/by-sa/3.0/
	Source: http://www.sparkfun.com/tutorial/Barometric/BMP085_Example_Code.pde

Circuit detail:
	Using a Spark Fun Barometric Pressure Sensor - BMP085 breakout board
	link: https://www.sparkfun.com/products/9694
	This comes with pull up resistors already on the i2c lines.
	BMP085 pins below are as marked on the Sparkfun BMP085 Breakout board

	SDA	- 	P1-03 / IC20-SDA
	SCL	- 	P1-05 / IC20_SCL
	XCLR	- 	Not Connected
	EOC	-	Not Connected
	GND	-	P1-06 / GND
	VCC	- 	P1-01 / 3.3V
	
	Note: Make sure you use P1-01 / 3.3V NOT the 5V pin.
*/
#include "stdafx.h"
#include "BMP085.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef __arm__
	#include <unistd.h>
	#include <sys/ioctl.h>
#endif
#include "smbus.h" 
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define I2C_SLAVE       0x0703  /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706  /* Use this slave address, even if it is in use */

#define BMP085_I2C_ADDRESS 0x77

#define BMP085_ULTRALOWPOWER 0
#define BMP085_STANDARD 1
#define BMP085_HIGHRES 2
#define BMP085_ULTRAHIGHRES 3
#define BMP085_CAL_AC1 0xAA // R Calibration data (16 bits)
#define BMP085_CAL_AC2 0xAC // R Calibration data (16 bits)
#define BMP085_CAL_AC3 0xAE // R Calibration data (16 bits)
#define BMP085_CAL_AC4 0xB0 // R Calibration data (16 bits)
#define BMP085_CAL_AC5 0xB2 // R Calibration data (16 bits)
#define BMP085_CAL_AC6 0xB4 // R Calibration data (16 bits)
#define BMP085_CAL_B1 0xB6 // R Calibration data (16 bits)
#define BMP085_CAL_B2 0xB8 // R Calibration data (16 bits)
#define BMP085_CAL_MB 0xBA // R Calibration data (16 bits)
#define BMP085_CAL_MC 0xBC // R Calibration data (16 bits)
#define BMP085_CAL_MD 0xBE // R Calibration data (16 bits)
#define BMP085_CONTROL 0xF4
#define BMP085_TEMPDATA 0xF6
#define BMP085_PRESSUREDATA 0xF6
#define BMP085_READTEMPCMD 0x2E
#define BMP085_READPRESSURECMD 0x34

const unsigned char BMP085_OVERSAMPLING_SETTING = 3;

#define I2C_READ_INTERVAL 30

CBMP085::CBMP085(const int ID)
{
	m_stoprequested=false;
	m_HwdID=ID;
}

CBMP085::~CBMP085()
{
}

bool CBMP085::StartHardware()
{
#ifndef __arm__
	return false;
#endif
	m_waitcntr=(I2C_READ_INTERVAL-5)*2;
	m_stoprequested=false;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CBMP085::Do_Work, this)));
	sOnConnected(this);
	m_bIsStarted=true;
	return (m_thread!=NULL);
}

bool CBMP085::StopHardware()
{
	m_stoprequested=true;
	if (m_thread!=NULL)
		m_thread->join();
	m_bIsStarted=false;
	return true;
}

void CBMP085::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CBMP085::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::millisec(500));
		if (m_stoprequested)
			break;
		m_waitcntr++;
		if (m_waitcntr>=I2C_READ_INTERVAL*2)
		{
			m_waitcntr=0;
			ReadSensorDetails();
		}

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);

		if (ltime.tm_sec % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
	}
	_log.Log(LOG_STATUS,"I2C: Worker stopped...");
}

// Open a connection to the bmp085
// Returns a file id, or -1 if not open/error
int CBMP085::bmp085_i2c_Begin()
{
	int fd=0;
#ifdef __arm__
	// Open port for reading and writing
	if ((fd = open("/dev/i2c-1", O_RDWR)) < 0)
		return -1;
	
	// Set the port options and set the address of the device
	if (ioctl(fd, I2C_SLAVE, BMP085_I2C_ADDRESS) < 0) {					
		close(fd);
		return -1;
	}
#endif
	return fd;
}

#ifdef __arm__

// Read two words from the BMP085 and supply it as a 16 bit integer
signed int bmp085_i2c_Read_Int(int fd, unsigned char address)
{
	signed int res = i2c_smbus_read_word_data(fd, address);
	if (res < 0) {
		return 0;
	}

	// Convert result to 16 bits and swap bytes
	res = ((res<<8) & 0xFF00) | ((res>>8) & 0xFF);

	return res;
}

//Write a byte to the BMP085
void bmp085_i2c_Write_Byte(int fd, unsigned char address, unsigned char value)
{
	i2c_smbus_write_byte_data(fd, address, value);
}

// Read a block of data BMP085
void bmp085_i2c_Read_Block(int fd, unsigned char address, unsigned char length, unsigned char *values)
{
	i2c_smbus_read_i2c_block_data(fd, address,length,values);
}
#endif

bool CBMP085::bmp085_Calibration()
{
#ifdef __arm__
	int fd = bmp085_i2c_Begin();
	if (fd < 0)
		return false;
	ac1 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC1);
	ac2 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC2);
	ac3 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC3);
	ac4 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC4);
	ac5 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC5);
	ac6 = bmp085_i2c_Read_Int(fd,BMP085_CAL_AC6);
	b1 = bmp085_i2c_Read_Int(fd,BMP085_CAL_B1);
	b2 = bmp085_i2c_Read_Int(fd,BMP085_CAL_B2);
	mb = bmp085_i2c_Read_Int(fd,BMP085_CAL_MB);
	mc = bmp085_i2c_Read_Int(fd,BMP085_CAL_MC);
	md = bmp085_i2c_Read_Int(fd,BMP085_CAL_MD);
	close(fd);
	return true;
#else
	return false;
#endif
}

// Read the uncompensated temperature value
signed int CBMP085::bmp085_ReadUT()
{
	signed int ut = 0;
#ifdef __arm__
	int fd = bmp085_i2c_Begin();

	// Write 0x2E into Register 0xF4
	// This requests a temperature reading
	bmp085_i2c_Write_Byte(fd,BMP085_CONTROL,0x2E);
	
	// Wait at least 4.5ms
	usleep(5000);

	// Read the two byte result from address 0xF6
	ut = bmp085_i2c_Read_Int(fd,BMP085_TEMPDATA);

	// Close the i2c file
	close (fd);
#endif	
	return ut;
}

// Read the uncompensated pressure value
unsigned int CBMP085::bmp085_ReadUP()
{
	unsigned int up = 0;
#ifdef __arm__
	int fd = bmp085_i2c_Begin();

	// Write 0x34+(BMP085_OVERSAMPLING_SETTING<<6) into register 0xF4
	// Request a pressure reading w/ oversampling setting
	bmp085_i2c_Write_Byte(fd,BMP085_CONTROL,0x34 + (BMP085_OVERSAMPLING_SETTING<<6));

	// Wait for conversion, delay time dependent on oversampling setting
	usleep((2 + (3<<BMP085_OVERSAMPLING_SETTING)) * 1000);

	// Read the three byte result from 0xF6
	// 0xF6 = MSB, 0xF7 = LSB and 0xF8 = XLSB
	unsigned char values[3];
	bmp085_i2c_Read_Block(fd, BMP085_PRESSUREDATA, 3, values);

	up = (((unsigned int) values[0] << 16) | ((unsigned int) values[1] << 8) | (unsigned int) values[2]) >> (8-BMP085_OVERSAMPLING_SETTING);

	// Close the i2c file
	close (fd);
#endif	
	return up;
}

int32_t CBMP085::computeB5(int32_t UT) 
{
	int32_t X1 = (UT - (int32_t)ac6) * ((int32_t)ac5) >> 15;
	int32_t X2 = ((int32_t)mc << 11) / (X1 + (int32_t)md);
	return X1 + X2;
}

// Calculate pressure given uncalibrated pressure
// Value returned will be in units of Pa
unsigned int CBMP085::bmp085_GetPressure()
{
	int32_t up, b3, b6, x1, x2, x3, p;
	uint32_t b4, b7;

	up = bmp085_ReadUP();
  
	b6 = b5 - 4000;
	// Calculate B3
	x1 = (b2 * (b6 * b6)>>12)>>11;
	x2 = (ac2 * b6)>>11;
	x3 = x1 + x2;
	b3 = (((((int)ac1)*4 + x3)<<BMP085_OVERSAMPLING_SETTING) + 2)>>2;
  
	// Calculate B4
	x1 = (ac3 * b6)>>13;
	x2 = (b1 * ((b6 * b6)>>12))>>16;
	x3 = ((x1 + x2) + 2)>>2;
	b4 = (ac4 * (unsigned int)(x3 + 32768))>>15;
  
	b7 = ((unsigned int)(up - b3) * (50000>>BMP085_OVERSAMPLING_SETTING));
	if (b7 < 0x80000000)
		p = (b7<<1)/b4;
	else
		p = (b7/b4)<<1;
	
	x1 = (p>>8) * (p>>8);
	x1 = (x1 * 3038)>>16;
	x2 = (-7357 * p)>>16;
	p += (x1 + x2 + 3791)>>4;
  
	return p;
}

// Calculate temperature given uncalibrated temperature
// Value returned will be in units of 0.1 deg C
int32_t CBMP085::bmp085_GetTemperature()
{
	int32_t ut = bmp085_ReadUT();
	b5 = computeB5(ut);
	int32_t result = ((b5 + 8) >> 4);

	return result;
}

void CBMP085::ReadSensorDetails()
{
	if (!bmp085_Calibration())
	{
		_log.Log(LOG_ERROR,"I2C: Could not open BMP085 device...");
		return;
	}

	int32_t temperature = bmp085_GetTemperature();
	unsigned int pressure = bmp085_GetPressure();

#ifndef __arm__
	temperature=213;
	pressure=102122;
#endif

	//Calculate Altitude
	double altitude = CalculateAltitudeFromPressure((double)pressure);
	_tTempBaro tsensor;
	tsensor.id1=1;
	tsensor.temp=float(((double)temperature)/10);
	tsensor.baro=float(((double)pressure)/100);
	tsensor.altitude=float(altitude);

	//this is probably not good, need to take the rising/falling of the pressure into account?
	//any help would be welcome!

	tsensor.forecast=baroForecastNoInfo;
	if (tsensor.baro<1000)
		tsensor.forecast=baroForecastRain;
	else if (tsensor.baro<1020)
		tsensor.forecast=baroForecastCloudy;
	else if (tsensor.baro<1030)
		tsensor.forecast=baroForecastPartlyCloudy;
	else
		tsensor.forecast=baroForecastSunny;
	sDecodeRXMessage(this, (const unsigned char *)&tsensor);//decode message
}
