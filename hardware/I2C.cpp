/*
(Dev)   -  (Pi)
SDA     -  SDA
SCL     -  SCL
GND     -  GND
VCC     -  3.3V

This code supports:
- BMP085
- BMP180
- PCF8574, PCF8574A
- HTU21D
- TSL2561
- MCP23017

PS there are better density and QNH formulas.

Authors:
- GizMoCuz
- Eric Maasdorp
- Ondrej Lycka (ondrej.lycka@seznam.cz)
- Guillermo Estevez (gestevezluka@gmail.com)
*/

#include "stdafx.h"
#include "I2C.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef HAVE_LINUX_I2C
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
#include "../main/SQLHelper.h"

#define round(a) ( int ) ( a + .5 )

//#define INVERT_PCF8574_MCP23017

#define I2C_READ_INTERVAL 30
#define I2C_SENSOR_READ_INTERVAL 30
#define I2C_IO_EXPANDER_READ_INTERVAL 1

#define sleepms(ms)  usleep((ms)*1000)
// BMP085 & BMP180 Specific code
#define BMPx8x_I2CADDR           0x77
#define BMPx8x_CtrlMeas          0xF4
#define BMPx8x_TempConversion    0x2E
#define BMPx8x_PresConversion0   0x34
#define BMPx8x_Results           0xF6
#define BMPx8x_minDelay          4     //require 4.5ms *1000/700 'turbo mode fix'= 6.4-Retry =4.4
#define BMPx8x_RetryDelay        2     //min delay for temp 4+2=6ms, max 4+2*20=44ms for pressure
//Will stop waiting if conversion is complete

// BME280 Specific code
#define BMEx8x_I2CADDR				0x76
#define BMEx8x_ChipVersion			0xD0
#define BMEx8x_Data					0xF7
#define BMEx8x_CtrlMeas				0xF4
#define BMEx8x_Config				0xF5

#define BMEx8x_Hum_MSB				0xFD
#define BMEx8x_Hum_LSB				0xFE

#define BMEx8x_OverSampling_Temp	2
#define BMEx8x_OverSampling_hum		3
#define BMEx8x_OverSampling_Mode	1
#define BMEx8x_OverSampling_Control	BMEx8x_OverSampling_Temp<<5 | BMEx8x_OverSampling_hum<<2 | BMEx8x_OverSampling_Mode


const unsigned char BMPx8x_OverSampling = 3;
// HTU21D registers
#define HTU21D_ADDRESS							0x40	// I2C address
#define HTU21D_USER_REGISTER_WRITE				0xE6	// Write user register
#define HTU21D_USER_REGISTER_READ				0xE7	// Read  user register
#define HTU21D_SOFT_RESET						0xFE	// Soft Reset (takes 15ms). Switch sensor OFF & ON again. All registers set to default exept heater bit.
#define HTU21D_TEMP_COEFFICIENT					-0.15	// Temperature coefficient (from 0deg.C to 80deg.C)
#define HTU21D_CRC8_POLYNOMINAL					0x13100	// CRC8 polynomial for 16bit CRC8 x^8 + x^5 + x^4 + 1
#define HTU21D_TRIGGER_HUMD_MEASURE_HOLD		0xE5	// Trigger Humidity Measurement. Hold master (SCK line is blocked)
#define HTU21D_TRIGGER_TEMP_MEASURE_HOLD		0xE3	// Trigger Temperature Measurement. Hold master (SCK line is blocked)
#define HTU21D_TRIGGER_HUMD_MEASURE_NOHOLD		0xF5	// Trigger Humidity Measurement. No Hold master (allows other I2C communication on a bus while sensor is measuring)
#define HTU21D_TRIGGER_TEMP_MEASURE_NOHOLD		0xF3	// Trigger Temperature Measurement. No Hold master (allows other I2C communication on a bus while sensor is measuring)
#define HTU21D_TEMP_DELAY						70		// Maximum required measuring time for a complete temperature read
#define HTU21D_HUM_DELAY						36		// Maximum required measuring time for a complete humidity read

// TSL2561 registers
#define TSL2561_ADDRESS		0x39    // I2C address
#define TSL2561_INIT		0x03	// start integrations
#define TSL2561_Channel0	0xAC	// IR+Visible lux
#define TSL2561_Channel1	0xAE	// IR only lux

// MCP23017 registers
#define MCP23x17_IODIRA         0x00
#define MCP23x17_IPOLA          0x02
#define MCP23x17_GPINTENA       0x04
#define MCP23x17_DEFVALA        0x06
#define MCP23x17_INTCONA        0x08
#define MCP23x17_IOCON          0x0A
#define MCP23x17_GPPUA          0x0C
#define MCP23x17_INTFA          0x0E
#define MCP23x17_INTCAPA        0x10
#define MCP23x17_GPIOA          0x12
#define MCP23x17_OLATA          0x14

#define MCP23x17_IODIRB         0x01
#define MCP23x17_IPOLB          0x03
#define MCP23x17_GPINTENB       0x05
#define MCP23x17_DEFVALB        0x07
#define MCP23x17_INTCONB        0x09
#define MCP23x17_IOCONB         0x0B
#define MCP23x17_GPPUB          0x0D
#define MCP23x17_INTFB          0x0F
#define MCP23x17_INTCAPB        0x11
#define MCP23x17_GPIOB          0x13
#define MCP23x17_OLATB          0x15

// MCP23x17 - Bits in the IOCON register
#define IOCON_UNUSED    0x01
#define IOCON_INTPOL    0x02
#define IOCON_ODR       0x04
#define IOCON_HAEN      0x08
#define IOCON_DISSLW    0x10
#define IOCON_SEQOP     0x20
#define IOCON_MIRROR    0x40
#define IOCON_BANK_MODE 0x80



const char* szI2CTypeNames[] = {
	"I2C_Unknown",
	"I2C_BMP085/180",
	"I2C_HTU21D",
	"I2C_TSL2561",
	"I2C_PCF8574",
	"I2C_BME280",
	"I2C_MCP23017",
};

I2C::I2C(const int ID, const _eI2CType DevType, const int Port):
m_dev_type(DevType)
{
	if ((m_dev_type == I2CTYPE_PCF8574) || (m_dev_type == I2CTYPE_MCP23017)) {
		i2c_addr = Port;
	}

	m_stoprequested = false;
	m_HwdID = ID;
	m_ActI2CBus = "/dev/i2c-1";
	if (!i2c_test(m_ActI2CBus.c_str()))
	{
		m_ActI2CBus = "/dev/i2c-0";
	}
}

I2C::~I2C()
{
}

bool I2C::StartHardware()
{
	m_stoprequested = false;
	if ((m_dev_type == I2CTYPE_BMP085)|| (m_dev_type == I2CTYPE_BME280))
	{
		m_minuteCount = 0;
		m_firstRound = true;
		m_LastMinute = -1;
		m_LastForecast = -1;
		m_LastSendForecast = baroForecastNoInfo;
	}

	if (m_dev_type == I2CTYPE_MCP23017)
		MCP23017_Init();

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&I2C::Do_Work, this)));
	sOnConnected(this);
	m_bIsStarted = true;
	return (m_thread != NULL);
}

bool I2C::StopHardware()
{
	m_stoprequested = true;
	if (m_thread != NULL)
		m_thread->join();
	m_bIsStarted = false;
	return true;
}

bool I2C::WriteToHardware(const char *pdata, const unsigned char length)
{
	int rc = 0;
	if (m_dev_type != I2CTYPE_PCF8574 && m_dev_type != I2CTYPE_MCP23017)
		return false;

	const tRBUF *pCmd = reinterpret_cast<const tRBUF*>(pdata);
	if ((pCmd->LIGHTING2.packettype == pTypeLighting2)) {
		unsigned char pin_number = pCmd->LIGHTING2.unitcode; // in DB column "Unit" is used for identify pin number
		unsigned char  value = pCmd->LIGHTING2.cmnd;
#ifdef INVERT_PCF8574_MCP23017
		value = ~value & 0x01; // Invert Status
#endif
		if (m_dev_type == I2CTYPE_PCF8574) {
			if (PCF8574_WritePin(pin_number, value) < 0)
				return false;
			return true;
		}
		else if (m_dev_type == I2CTYPE_MCP23017) {
			if (MCP23017_WritePin(pin_number, value) < 0)
				return false;
			return true;
		}
	}
	return false;
}

void I2C::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;
	_log.Log(LOG_STATUS, "%s: Worker started...", szI2CTypeNames[m_dev_type]);

	if (m_dev_type == I2CTYPE_TSL2561)
	{
		TSL2561_Init();
	}

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
			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}
			try
			{
				if (sec_counter % I2C_IO_EXPANDER_READ_INTERVAL == 0)
				{
					if (m_dev_type == I2CTYPE_PCF8574)
					{
						PCF8574_ReadChipDetails();
					}
					else if (m_dev_type == I2CTYPE_MCP23017)
					{
						MCP23017_ReadChipDetails();
					}
				}
				if (sec_counter % I2C_SENSOR_READ_INTERVAL == 0)
				{
					if (m_dev_type == I2CTYPE_BMP085)
					{
						bmp_Read_BMP_SensorDetails();
					}
					else if (m_dev_type == I2CTYPE_HTU21D)
					{
						HTU21D_ReadSensorDetails();
					}
					else if (m_dev_type == I2CTYPE_TSL2561)
					{
						TSL2561_ReadSensorDetails();
					}
					else if (m_dev_type == I2CTYPE_BME280)
					{
						bmp_Read_BME_SensorDetails();
					}
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "%s: Error reading sensor data!...", szI2CTypeNames[m_dev_type]);
			}
		}
	}
	_log.Log(LOG_STATUS, "%s: Worker stopped...", szI2CTypeNames[m_dev_type]);
}

//returns true if it could be opened
bool I2C::i2c_test(const char *I2CBusName)
{
#ifndef HAVE_LINUX_I2C
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
int I2C::i2c_Open(const char *I2CBusName)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	int fd;
	//Open port for reading and writing
	if ((fd = open(I2CBusName, O_RDWR)) < 0)
	{
		_log.Log(LOG_ERROR, "%s: Failed to open the i2c bus!...", szI2CTypeNames[m_dev_type]);
		_log.Log(LOG_ERROR, "%s: Check to see if you have a bus: %s", szI2CTypeNames[m_dev_type], I2CBusName);
		_log.Log(LOG_ERROR, "%s: We might only be able to access this as root user", szI2CTypeNames[m_dev_type]);
		return -1;
	}
	return fd;
#endif
}

// PCF8574 and PCF8574A

void I2C::PCF8574_ReadChipDetails()
{	
#ifndef HAVE_LINUX_I2C
	return;
#else
	char buf = 0;
	int fd = i2c_Open(m_ActI2CBus.c_str()); // open i2c
	if (fd < 0) return; // Error opening i2c device!
	if ( readByteI2C(fd, &buf, i2c_addr) < 0 ) return; //read from i2c
#ifdef INVERT_PCF8574_MCP23017
	buf=~buf; // Invert Status
#endif
	for (char pin_number=0; pin_number<8; pin_number++){
		int DeviceID = (i2c_addr << 8) + pin_number; // DeviceID from i2c_address and pin_number
		unsigned char Unit = pin_number;
		char pin_mask=0x01<<pin_number;
		bool value=(buf & pin_mask);
		SendSwitch(DeviceID, Unit, 255, value, 0, ""); // create or update switch
	}
	close(fd);
#endif
}


char I2C::PCF8574_WritePin(char pin_number,char  value)
{	
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	_log.Log(LOG_NORM, "GPIO: WRITE TO PCF8574 pin:%d, value: %d, i2c_address:%d", pin_number, value, i2c_addr);
	char pin_mask=0x01<<pin_number; // create pin mask from pin number
	char buf_act = 0;
	char buf_new = 0;
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) return -1; // Error opening i2c device!
	if ( readByteI2C(fd, &buf_act, i2c_addr) < 0 ) return -2;
	lseek(fd,0,SEEK_SET); // after read back file cursor to begin (prepare to write new value on begin)
	if (value==1) buf_new = buf_act | pin_mask;	//prepare new value by combinate current value, mask and new value
	else buf_new = buf_act & ~pin_mask;
	if (buf_new!=buf_act) { // if value change write new value
		if (writeByteI2C(fd, buf_new, i2c_addr) < 0 ) {
			_log.Log(LOG_ERROR, "GPIO: %s: Error write to device!...", szI2CTypeNames[m_dev_type]);
			return -3;
		}
	}
	close(fd);
	return 1;
#endif
}

// MCP23017

void I2C::MCP23017_Init()
{
#ifndef HAVE_LINUX_I2C
	return;
#else
	int fd = i2c_Open(m_ActI2CBus.c_str()); // open i2c
	if (fd < 0) return; // Error opening i2c device!
	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> >::const_iterator itt;
	unsigned char nvalue;
	uint16_t GPIO_reg = 0xFFFF;
	int unit;
	bool value;
	
	result = m_sql.safe_query("SELECT Unit, nValue FROM DeviceStatus WHERE (HardwareID = %d) AND (DeviceID like '000%02X%%');", m_HwdID, i2c_addr);
	if (!result.empty())
	{
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;
			unit = atoi(sd[0].c_str());
			nvalue = atoi(sd[1].c_str());

			nvalue ^= 1;
			//prepare new value by combinating current value, mask and new value
			if (nvalue==1) {
				GPIO_reg |= (0x01 << unit);
				value = false;
			}
			else if (nvalue==0) {
				GPIO_reg &= ~(0x01 << unit);
				value = true;
			}

			int DeviceID = (i2c_addr << 8) + (unsigned char)unit; 			// DeviceID from i2c_address and pin_number
			SendSwitch(DeviceID, unit, 255, value, 0, ""); 					// create or update switch
			//_log.Log(LOG_NORM, "SendSwitch(DeviceID: %d, unit: %d, value: %d", DeviceID, unit, value );
		}
	}
	else {
		for (char pin_number=0; pin_number<16; pin_number++){
			int DeviceID = (i2c_addr << 8) + pin_number; 			// DeviceID from i2c_address and pin_number
			SendSwitch(DeviceID, pin_number, 255, value, 0, ""); 			// create switch
		}
	}
	if (I2CWriteReg16(fd, MCP23x17_GPIOA, GPIO_reg) < 0 ) {	// write values from domoticz db to gpio register
		_log.Log(LOG_NORM, "I2C::MCP23017_Init. %s. Failed to write to I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
		return; 											// write to i2c failed
	}
	if (I2CWriteReg16(fd, MCP23x17_IODIRA, 0x0000) < 0 ){	// set all gpio pins on the port as output
		_log.Log(LOG_NORM, "I2C::MCP23017_Init. %s. Failed to write to I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
		return; 											// write to i2c failed
	}
	close(fd);
#endif
}

void I2C::MCP23017_ReadChipDetails()
{	
#ifndef HAVE_LINUX_I2C
	return;
#else
//	uint16_t data = 0;
	uint16_t cur_iodir = 0;
	i2c_data data;
	int rc;


	int fd = i2c_Open(m_ActI2CBus.c_str()); // open i2c
	if (fd < 0) return; // Error opening i2c device!

	rc = I2CReadReg16(fd, MCP23x17_IODIRA, &data); 				// get current iodir port value
	close(fd);

	if (rc < 0) {
		_log.Log(LOG_NORM, "I2C::MCP23017_ReadChipDetails. %s. Failed to read from I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
		return; //read from i2c failed
	}
	if ((data.word == 0xFFFF)){									// if oidir port is 0xFFFF means the chip has been reset
		_log.Log(LOG_NORM, "I2C::MCP23017_ReadChipDetails, Cur_iodir: 0xFFFF, call MCP23017_Init");
		MCP23017_Init();										// initialize gpio pins with switch status from db
	}
#endif
}

int I2C::MCP23017_WritePin(char pin_number,char  value)
{	
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	_log.Log(LOG_NORM, "GPIO: WRITE TO MCP23017 pin:%d, value: %d, i2c_address:%d", pin_number, value, i2c_addr);
	uint16_t pin_mask=0, iodir_mask=0;
	unsigned char gpio_port, iodir_port;
	uint16_t new_data = 0;
	i2c_data cur_data, cur_iodir;
	int rc;
	
	pin_mask=0x01<<(pin_number);
	iodir_mask &= ~(0x01 << (pin_number));
	
	// open i2c device
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) return -1; 								// Error opening i2c device!
	
	rc = I2CReadReg16(fd, MCP23x17_GPIOA, &cur_data); 		// get current gio port value
	if (rc < 0) {
		_log.Log(LOG_NORM, "I2C::MCP23017_WritePin. %s. Failed to read from I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
		close(fd);
		return -2; 						//read from i2c failed
	}
	rc = I2CReadReg16(fd, MCP23x17_IODIRA, &cur_iodir); 		// get current iodir port value
	if (rc < 0) {						//read from i2c failed
		_log.Log(LOG_NORM, "I2C::MCP23017_WritePin. %s. Failed to read from I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
		close(fd);
		return -2; 						//read from i2c failed
	}
	cur_iodir.word &= iodir_mask; 							// create mask for iodir register to set pin as output.
	
	if (value==1) new_data = cur_data.word | pin_mask;		//prepare new value by combinating current value, mask and new value
	else new_data = cur_data.word & ~pin_mask;
	
	if (new_data!=cur_data.word) { 							// if value change write new value
		if (I2CWriteReg16(fd, MCP23x17_GPIOA, new_data) < 0 ) {
			_log.Log(LOG_ERROR, "I2C::MCP23017_WritePin. %s: Failed to write to I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
			close(fd);
			return -3;
		}
		if (I2CWriteReg16(fd, MCP23x17_IODIRA, cur_iodir.word) < 0 ) {		// write to iodir register, set gpio pin as output
			_log.Log(LOG_ERROR, "I2C::MCP23017_WritePin. %s: Failed to write to I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
			close(fd);
			return -3;													// write to i2c failed
		}
	}
	close(fd);
	return 1;
#endif
}

// BMP085, BMP180, HTU and TSL common code

int I2C::ReadInt(int fd, uint8_t *devValues, uint8_t startReg, uint8_t bytesToRead)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	int rc;
	struct i2c_rdwr_ioctl_data messagebuffer;
	struct i2c_msg bmp_read_reg[2] = {
		{ BMPx8x_I2CADDR, 0, 1, &startReg },
		{ BMPx8x_I2CADDR, I2C_M_RD, bytesToRead, devValues }
	};
	struct i2c_msg htu_read_reg[1] = {
		{ HTU21D_ADDRESS, I2C_M_RD, bytesToRead, devValues }
	};
	struct i2c_msg tsl_read_reg[2] = {
		{ TSL2561_ADDRESS, 0, 1, &startReg },
		{ TSL2561_ADDRESS, I2C_M_RD, bytesToRead, devValues }
	};
	struct i2c_msg bme_read_reg[2] = {
		{ BMEx8x_I2CADDR, 0, 1, &startReg },
		{ BMEx8x_I2CADDR, I2C_M_RD, bytesToRead, devValues }
	};

	//Build a register read command
	//Requires a one complete message containing a command
	//and another complete message for the reply
	if (m_dev_type == I2CTYPE_BMP085)
	{
		messagebuffer.nmsgs = 2;                  //Two message/action
		messagebuffer.msgs = bmp_read_reg;            //load the 'read__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_HTU21D)
	{
		messagebuffer.nmsgs = 1;
		messagebuffer.msgs = htu_read_reg;            //load the 'read__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_TSL2561)
	{
		messagebuffer.nmsgs = 2;
		messagebuffer.msgs = tsl_read_reg;            //load the 'read__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_BME280)
	{
		messagebuffer.nmsgs = 2;                  //Two message/action
		messagebuffer.msgs = bme_read_reg;            //load the 'read__reg' message into the buffer
	}

	rc = ioctl(fd, I2C_RDWR, &messagebuffer); //Send the buffer to the bus and returns a send status
	if (rc < 0) {
		return rc;
	}
	//note that the return data is contained in the array pointed to by devValues (passed by-ref)
	return 0;
#endif
}

int I2C::WriteCmd(int fd, uint8_t devAction)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	int rc;
	struct i2c_rdwr_ioctl_data messagebuffer;
	uint8_t datatosend[2];
	struct i2c_msg bmp_write_reg[1] = {
		{ BMPx8x_I2CADDR, 0, 2, datatosend }
	};
	struct i2c_msg htu_write_reg[1] = {
		{ HTU21D_ADDRESS, 0, 1, datatosend }
	};
	struct i2c_msg tsl_write_reg[1] = {
		{ TSL2561_ADDRESS, 0, 1, datatosend }
	};
	struct i2c_msg bme_write_reg[1] = {
		{ BMEx8x_I2CADDR, 0, 2, datatosend }
	};

	if (m_dev_type == I2CTYPE_BMP085)
	{
		datatosend[0] = BMPx8x_CtrlMeas;
		datatosend[1] = devAction;
		//Build a register write command
		//Requires one complete message containing a reg address and command
		messagebuffer.msgs = bmp_write_reg;           //load the 'write__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_HTU21D)
	{
		datatosend[0] = devAction;
		//Build a register write command
		//Requires one complete message containing a reg address and command
		messagebuffer.msgs = htu_write_reg;           //load the 'write__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_TSL2561)
	{
		datatosend[0] = devAction;
		//Build a register write command
		//Requires one complete message containing a reg address and command
		messagebuffer.msgs = tsl_write_reg;           //load the 'write__reg' message into the buffer
	}
	else if (m_dev_type == I2CTYPE_BME280)
	{
		datatosend[0] = BMEx8x_CtrlMeas;
		datatosend[1] = devAction;
		//Build a register write command
		//Requires one complete message containing a reg address and command
		messagebuffer.msgs = bme_write_reg;           //load the 'write__reg' message into the buffer
	}

	messagebuffer.nmsgs = 1;                  //One message/action
	rc = ioctl(fd, I2C_RDWR, &messagebuffer); //Send the buffer to the bus and returns a send status
	if (rc < 0) {
		return rc;
	}
	return 0;
#endif
}

char I2C::readByteI2C(int fd, char *byte, char i2c_addr)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	// set I2C address to will be communicate (first address = chip on base board)
	if (ioctl(fd, I2C_SLAVE_FORCE, i2c_addr) < 0) {
		_log.Log(LOG_ERROR, "%s: Failed to acquire bus access and/or talk to slave with address %d", szI2CTypeNames[m_dev_type], i2c_addr);
		return -1;
	}
	//read from I2C device
	if (read(fd,byte,1) != 1) {
		_log.Log(LOG_ERROR, "%s: Failed to read from the i2c bus with address %d", szI2CTypeNames[m_dev_type], i2c_addr);
		return -2;
	}
	return 1;
#endif
}

char I2C::writeByteI2C(int fd, char byte, char i2c_addr)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	// set I2C address to will be communicate (first address = chip on base board)
	if (ioctl(fd, I2C_SLAVE_FORCE, i2c_addr) < 0) {
		_log.Log(LOG_ERROR, "%s: Failed to acquire bus access and/or talk to slave with address %d", szI2CTypeNames[m_dev_type], i2c_addr);
		return -1;
	}
	//write to I2C device
	if (write(fd,&byte,1) != 1) {
		_log.Log(LOG_ERROR, "%s: Failed write to the i2c bus with address %d", szI2CTypeNames[m_dev_type], i2c_addr);
		return -2;
	}
	return 1;
#endif
}

int I2C::I2CWriteReg16(int fd, uint8_t reg, uint16_t value)
{
	#ifndef HAVE_LINUX_I2C
		return -1;
	#else
		int rc;
		struct i2c_rdwr_ioctl_data messagebuffer;
		uint8_t datatosend[3];
		i2c_data bytes;
		bytes.word = value;
		struct i2c_msg write_reg[1] = {
			{ i2c_addr, 0, 3, datatosend } // flag absent == write, two registers, one for register address and one for the value to write
		};
		datatosend[0] = reg;						// address of register to write
		datatosend[1] = bytes.byte[0];						// value to write
		datatosend[2] = bytes.byte[1];						// value to write
		messagebuffer.msgs = write_reg;				//load the 'write_reg' message into the buffer
		messagebuffer.nmsgs = 1;
		rc = ioctl(fd, I2C_RDWR, &messagebuffer); //Send the buffer to the bus and returns a send status
		if (rc < 0) {
//			_log.Log(LOG_NORM, "I2C::I2CReadReg16. %s. Failed to write to I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
			return rc;
		}
		return 0;
	#endif
}

int I2C::I2CReadReg16(int fd, unsigned char reg, i2c_data *data )
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	int rc;
	//i2c_data data;
	struct i2c_rdwr_ioctl_data messagebuffer;
	struct i2c_msg read_reg[3] = {
		{ i2c_addr, 0, 1, &reg },					//flag absent == write, address of register to read
		{ i2c_addr, I2C_M_RD, 2, data->byte }		//flag I2C_M_RD == read
	};
	messagebuffer.nmsgs = 2;                  		//Two message/action
	messagebuffer.msgs = read_reg;            		//load the 'read__reg' message into the buffer
	return ioctl(fd, I2C_RDWR, &messagebuffer); 		//Send the buffer to the bus and returns a send status
//	if (rc < 0) {
//		_log.Log(LOG_NORM, "I2C::I2CReadReg16. %s, Failed to read from I2C device at address: 0x%x", szI2CTypeNames[m_dev_type], i2c_addr);
//		return rc;
//	}
	//return data.word ;	
#endif
}

// HTU21D functions
int I2C::HTU21D_checkCRC8(uint16_t data)
{
	unsigned int bit;
	for (bit = 0; bit < 16; bit++)
	{
		if (data & 0x8000)
		{
			data = (data << 1) ^ HTU21D_CRC8_POLYNOMINAL;
		}
		else
		{
			data <<= 1;
		}
	}
	data >>= 8;
	return data;
}

int I2C::HTU21D_GetHumidity(int fd, float *Hum)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	uint16_t rawHumidity;
	uint8_t rValues[3];
	uint8_t Checksum;

	if (WriteCmd(fd, (HTU21D_TRIGGER_HUMD_MEASURE_HOLD)) != 0) return -1;

	sleepms(HTU21D_HUM_DELAY);

	if (ReadInt(fd, rValues, 0, 3) != 0) return -1;
	rawHumidity = ((rValues[0] << 8) | rValues[1]);
	Checksum = rValues[2];
	if (HTU21D_checkCRC8(rawHumidity) != Checksum)
	{
		_log.Log(LOG_ERROR, "%s: Incorrect humidity checksum!...", szI2CTypeNames[m_dev_type]);
		return -1;
	}
	rawHumidity ^= 0x02;
	*Hum = -6 + 0.001907 * (float)rawHumidity;
	return 0;
#endif
}

int I2C::HTU21D_GetTemperature(int fd, float *Temp)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	uint16_t rawTemperature;
	uint8_t rValues[3];
	uint8_t Checksum;

	if (WriteCmd(fd, HTU21D_TRIGGER_TEMP_MEASURE_HOLD) != 0) {
		_log.Log(LOG_ERROR, "%s: Error writing I2C!...", szI2CTypeNames[m_dev_type]);
		return -1;
	}
	sleepms(HTU21D_TEMP_DELAY);
	if (ReadInt(fd, rValues, 0, 3) != 0) {
		_log.Log(LOG_ERROR, "%s: Error reading I2C!...", szI2CTypeNames[m_dev_type]);
		return -1;
	}
	rawTemperature = ((rValues[0] << 8) | rValues[1]);
	Checksum = rValues[2];
	if (HTU21D_checkCRC8(rawTemperature) != Checksum)
	{
		_log.Log(LOG_ERROR, "%s: Incorrect temperature checksum!...", szI2CTypeNames[m_dev_type]);
		return -1;
	}
	*Temp = -46.85 + 0.002681 * (float)rawTemperature;
	return 0;
#endif
}

void I2C::HTU21D_ReadSensorDetails()
{
	float temperature, humidity;

#ifndef HAVE_LINUX_I2C
	temperature = 21.3f;
	humidity = 45;
#ifndef _DEBUG
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!...", szI2CTypeNames[m_dev_type]);
	return;
#else
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!... Debug: just adding a value", szI2CTypeNames[m_dev_type]);
#endif
#else
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) {
		_log.Log(LOG_ERROR, "%s: Error opening device!...", szI2CTypeNames[m_dev_type]);
		return;
	}
	if (HTU21D_GetTemperature(fd, &temperature) < 0) {
		_log.Log(LOG_ERROR, "%s: Error reading temperature!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	if (HTU21D_GetHumidity(fd, &humidity) < 0) {
		_log.Log(LOG_ERROR, "%s: Error reading humidity!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	close(fd);
	if (temperature >= 0 && temperature <= 80)
		humidity = humidity + (25 - temperature) * HTU21D_TEMP_COEFFICIENT;
#endif

	SendTempHumSensor(1, 255, temperature, round(humidity), "TempHum");
}

// TSL2561 functions
void I2C::TSL2561_Init()
{
#ifdef HAVE_LINUX_I2C
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) {
		_log.Log(LOG_ERROR, "%s: Error opening device!...", szI2CTypeNames[m_dev_type]);
		return;
	}
	if (WriteCmd(fd, TSL2561_INIT) != 0) {
		_log.Log(LOG_ERROR, "%s: Error initializing device!...", szI2CTypeNames[m_dev_type]);
	}
	close(fd);
#endif
}

void I2C::TSL2561_ReadSensorDetails()
{
	float lux;
#ifndef HAVE_LINUX_I2C
	lux = 1984;
	#ifndef _DEBUG
		_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!...", szI2CTypeNames[m_dev_type]);
		return;
	#else
		_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!... Debug: just adding a value", szI2CTypeNames[m_dev_type]);
	#endif
#else
	uint8_t rValues[2];
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0) {
		_log.Log(LOG_ERROR, "%s: Error opening device!...", szI2CTypeNames[m_dev_type]);
		return;
	}
	if (ReadInt(fd, rValues, TSL2561_Channel0, 2) != 0) {
		_log.Log(LOG_ERROR, "%s: Error reading ch0!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	float ch0 = rValues[1] * 256.0 + rValues[0];
	if (ReadInt(fd, rValues, TSL2561_Channel1, 2) != 0) {
		_log.Log(LOG_ERROR, "%s: Error reading ch1!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	close(fd);
	float ch1 = rValues[1] * 256.0 + rValues[0];

	// Real Lux calculation for T,FN and CL packages
	float ratio = 0;
	if (ch0 != 0) ratio = ch1/ch0;
	if (ratio >= 0 && ratio < 0.50)
		lux = ch0 * (0.0304 - 0.062 * pow(ch1 / ch0, 1.4));
	else if (ratio >= 0.5 && ratio < 0.61)
		lux = 0.0224*ch0 - 0.031*ch1;
	else if (ratio >= 0.61 && ratio < 0.8)
		lux = 0.0128*ch0 - 0.0153*ch1;
	else if (ratio >= 0.8 && ratio < 1.3)
		lux = 0.00146*ch0 - 0.00112*ch1;
	else
		lux = 0;
	// final scaling with default gain
	lux *= 16;
#endif
	SendLuxSensor(0, 0, 255, lux, "Lux");
}

// BMP085 functions
int I2C::bmp_Calibration(int fd)
{
#ifdef HAVE_LINUX_I2C
	uint8_t rValue[22];
	//printf("Entering Calibration\n");
	if (ReadInt(fd, rValue, 0xAA, 22) == 0)
	{
		bmp_ac1 = ((rValue[0] << 8) | rValue[1]);
		bmp_ac2 = ((rValue[2] << 8) | rValue[3]);
		bmp_ac3 = ((rValue[4] << 8) | rValue[5]);
		bmp_ac4 = ((rValue[6] << 8) | rValue[7]);
		bmp_ac5 = ((rValue[8] << 8) | rValue[9]);
		bmp_ac6 = ((rValue[10] << 8) | rValue[11]);
		bmp_b1 = ((rValue[12] << 8) | rValue[13]);
		bmp_b2 = ((rValue[14] << 8) | rValue[15]);
		bmp_mb = ((rValue[16] << 8) | rValue[17]);
		bmp_mc = ((rValue[18] << 8) | rValue[19]);
		bmp_md = ((rValue[20] << 8) | rValue[21]);
		/*
		printf ("\nac1:0x%0x\n",bmp_ac1);
		printf ("ac2:0x%0x\n",bmp_ac2);
		printf ("ac3:0x%0x\n",bmp_ac3);
		printf ("ac4:0x%0x\n",bmp_ac4);
		printf ("ac5:0x%0x\n",bmp_ac5);
		printf ("ac6:0x%0x\n",bmp_ac6);
		printf ("b1:0x%0x\n",bmp_b1);
		printf ("b2:0x%0x\n",bmp_b2);
		printf ("mb:0x%0x\n",bmp_mb);
		printf ("mc:0x%0x\n",bmp_mc);
		printf ("md:0x%0x\n",bmp_md);
		*/
		return 0;
	}
#endif
	return -1;
}

int I2C::bmp_WaitForConversion(int fd)
{
#ifdef HAVE_LINUX_I2C
	uint8_t rValues[3];
	int counter = 0;
	//Delay can now be reduced by checking that bit 5 of Ctrl_Meas(0xF4) == 0
	do {
		sleepms(BMPx8x_RetryDelay);
		if (ReadInt(fd, rValues, BMPx8x_CtrlMeas, 1) != 0) return -1;
		counter++;
		//printf("GetPressure:\t Loop:%i\trValues:0x%0x\n",counter,rValues[0]);
	} while (((rValues[0] & 0x20) != 0) && counter < 20);
#endif
	return 0;
}

// Calculate calibrated pressure
// Value returned will be in hPa
int I2C::bmp_GetPressure(int fd, double *Pres)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	unsigned int up;
	uint8_t rValues[3];

	// Pressure conversion with oversampling 0x34+ BMPx8x_OverSampling 'bit shifted'
	if (WriteCmd(fd, (BMPx8x_PresConversion0 + (BMPx8x_OverSampling << 6))) != 0) return -1;

	//Delay gets longer the higher the oversampling must be at least 26 ms plus a bit for turbo
	//clock error ie 26 * 1000/700 or 38 ms
	//sleepms (BMPx8x_minDelay + (4<<BMPx8x_OverSampling));  //39ms at oversample = 3

	//Code is now 'turbo' over clock independent
	sleepms(BMPx8x_minDelay);
	if (bmp_WaitForConversion(fd) != 0) return -1;

	//printf ("\nDelay:%i\n",(BMPx8x_minDelay+(4<<BMPx8x_OverSampling)));
	if (ReadInt(fd, rValues, BMPx8x_Results, 3) != 0) return -1;
	up = (((unsigned int)rValues[0] << 16) | ((unsigned int)rValues[1] << 8) | (unsigned int)rValues[2]) >> (8 - BMPx8x_OverSampling);

	int x1, x2, x3, b3, b6, p;
	unsigned int b4, b7;

	b6 = bmp_b5 - 4000;
	x1 = (bmp_b2 * (b6 * b6) >> 12) >> 11;
	x2 = (bmp_ac2 * b6) >> 11;
	x3 = x1 + x2;
	b3 = (((((int)bmp_ac1) * 4 + x3) << BMPx8x_OverSampling) + 2) >> 2;

	x1 = (bmp_ac3 * b6) >> 13;
	x2 = (bmp_b1 * ((b6 * b6) >> 12)) >> 16;
	x3 = ((x1 + x2) + 2) >> 2;
	b4 = (bmp_ac4 * (unsigned int)(x3 + 32768)) >> 15;

	b7 = ((unsigned int)(up - b3) * (50000 >> BMPx8x_OverSampling));
	if (b7 < 0x80000000)
		p = (b7 << 1) / b4;
	else
		p = (b7 / b4) << 1;

	x1 = (p >> 8) * (p >> 8);
	x1 = (x1 * 3038) >> 16;
	x2 = (-7357 * p) >> 16;
	p += (x1 + x2 + 3791) >> 4;
	*Pres = ((double)p / 100);
	return 0;
#endif
}

// Calculate calibrated temperature
// Value returned will be in units of 0.1 deg C
int I2C::bmp_GetTemperature(int fd, double *Temp)
{
#ifndef HAVE_LINUX_I2C
	return -1;
#else
	unsigned int ut;
	uint8_t rValues[2];

	if (WriteCmd(fd, BMPx8x_TempConversion) != 0) return -1;
	//Code is now 'turbo' over clock independent
	sleepms(BMPx8x_minDelay);
	if (bmp_WaitForConversion(fd) != 0) return -1;

	if (ReadInt(fd, rValues, BMPx8x_Results, 2) != 0) return -1;
	ut = ((rValues[0] << 8) | rValues[1]);

	int x1, x2;
	x1 = (((int)ut - (int)bmp_ac6)*(int)bmp_ac5) >> 15;
	x2 = ((int)bmp_mc << 11) / (x1 + bmp_md);
	bmp_b5 = x1 + x2;

	double result = ((bmp_b5 + 8) >> 4);
	*Temp = result / 10;
	return 0;
#endif
}

double I2C::bmp_altitude(double p) {
	return 145437.86*(1 - pow((p / 1013.25), 0.190294496)); //return feet
															//return 44330*(1- pow((p/1013.25),0.190294496)); //return meters
}

double I2C::bmp_qnh(double p, double StationAlt) {
	return p / pow((1 - (StationAlt / 145437.86)), 5.255); //return hPa based on feet
														   //return p / pow((1-(StationAlt/44330)),5.255) ; //return hPa based on feet
}

double I2C::bmp_ppl_DensityAlt(double PAlt, double Temp) {
	double ISA = 15 - (1.98*(PAlt / 1000));
	return PAlt + (120 * (Temp - ISA)); //So,So density altitude
}

#define FC_BMP085_STABLE 0			//Stable weather
#define FC_BMP085_SUNNY 1			//Slowly rising HP stable good weather (Clear/Sunny)
#define FC_BMP085_CLOUDY_RAIN 2		//Slowly falling Low Pressure System, stable rainy weather (Cloudy/Rain)
#define FC_BMP085_UNSTABLE 3		//Quickly rising HP, not stable weather
#define FC_BMP085_THUNDERSTORM 4	//Quickly falling LP, Thunderstorm, not stable (Thunderstorm)
#define FC_BMP085_UNKNOWN 5			//

//Should be called every minute
int I2C::bmp_CalculateForecast(const float pressure)
{
	double dP_dt = 0;

	// Algorithm found here
	// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
	if (m_minuteCount > 180)
		m_minuteCount = 6;

	m_pressureSamples[m_minuteCount] = pressure;
	m_minuteCount++;

	if (m_minuteCount == 5) {
		// Avg pressure in first 5 min, value averaged from 0 to 5 min.
		m_pressureAvg[0] = ((m_pressureSamples[0] + m_pressureSamples[1]
			+ m_pressureSamples[2] + m_pressureSamples[3] + m_pressureSamples[4])
			/ 5);
	}
	else if (m_minuteCount == 35) {
		// Avg pressure in 30 min, value averaged from 0 to 5 min.
		m_pressureAvg[1] = ((m_pressureSamples[30] + m_pressureSamples[31]
			+ m_pressureSamples[32] + m_pressureSamples[33]
			+ m_pressureSamples[34]) / 5);
		float change = (m_pressureAvg[1] - m_pressureAvg[0]);
		if (m_firstRound) // first time initial 3 hour
			dP_dt = ((65.0 / 1023.0) * 2 * change); // note this is for t = 0.5hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 1.5); // divide by 1.5 as this is the difference in time from 0 value.
	}
	else if (m_minuteCount == 60) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[2] = ((m_pressureSamples[55] + m_pressureSamples[56]
			+ m_pressureSamples[57] + m_pressureSamples[58]
			+ m_pressureSamples[59]) / 5);
		float change = (m_pressureAvg[2] - m_pressureAvg[0]);
		if (m_firstRound) //first time initial 3 hour
			dP_dt = ((65.0 / 1023.0) * change); //note this is for t = 1 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 2); //divide by 2 as this is the difference in time from 0 value
	}
	else if (m_minuteCount == 95) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[3] = ((m_pressureSamples[90] + m_pressureSamples[91]
			+ m_pressureSamples[92] + m_pressureSamples[93]
			+ m_pressureSamples[94]) / 5);
		float change = (m_pressureAvg[3] - m_pressureAvg[0]);
		if (m_firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 1.5); // note this is for t = 1.5 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 2.5); // divide by 2.5 as this is the difference in time from 0 value
	}
	else if (m_minuteCount == 120) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[4] = ((m_pressureSamples[115] + m_pressureSamples[116]
			+ m_pressureSamples[117] + m_pressureSamples[118]
			+ m_pressureSamples[119]) / 5);
		float change = (m_pressureAvg[4] - m_pressureAvg[0]);
		if (m_firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 2); // note this is for t = 2 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 3); // divide by 3 as this is the difference in time from 0 value
	}
	else if (m_minuteCount == 155) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[5] = ((m_pressureSamples[150] + m_pressureSamples[151]
			+ m_pressureSamples[152] + m_pressureSamples[153]
			+ m_pressureSamples[154]) / 5);
		float change = (m_pressureAvg[5] - m_pressureAvg[0]);
		if (m_firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 2.5); // note this is for t = 2.5 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 3.5); // divide by 3.5 as this is the difference in time from 0 value
	}
	else if (m_minuteCount == 180) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[6] = ((m_pressureSamples[175] + m_pressureSamples[176]
			+ m_pressureSamples[177] + m_pressureSamples[178]
			+ m_pressureSamples[179]) / 5);
		float change = (m_pressureAvg[6] - m_pressureAvg[0]);
		if (m_firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 3); // note this is for t = 3 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 4); // divide by 4 as this is the difference in time from 0 value
		m_pressureAvg[0] = m_pressureAvg[5]; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
		m_firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
	}

	if (m_minuteCount < 35 && m_firstRound) //if time is less than 35 min on the first 3 hour interval.
		return FC_BMP085_UNKNOWN; // Unknown, more time needed
	else if (dP_dt < (-0.25))
		return FC_BMP085_THUNDERSTORM; // Quickly falling LP, Thunderstorm, not stable
	else if (dP_dt > 0.25)
		return FC_BMP085_UNSTABLE; // Quickly rising HP, not stable weather
	else if ((dP_dt >(-0.25)) && (dP_dt < (-0.05)))
		return FC_BMP085_CLOUDY_RAIN; // Slowly falling Low Pressure System, stable rainy weather
	else if ((dP_dt > 0.05) && (dP_dt < 0.25))
		return FC_BMP085_SUNNY; // Slowly rising HP stable good weather
	else if ((dP_dt >(-0.05)) && (dP_dt < 0.05))
		return FC_BMP085_STABLE; // Stable weather
	else
		return FC_BMP085_UNKNOWN; // Unknown
}

int I2C::CalculateForcast(const float pressure)
{
	int forecast = bmp_CalculateForecast(pressure);
	if (forecast != m_LastForecast)
	{
		m_LastForecast = forecast;
		switch (forecast)
		{
		case FC_BMP085_STABLE:			//Stable weather
			if (m_LastForecast == bmpbaroforecast_unknown)
				m_LastSendForecast = bmpbaroforecast_stable;
			break;
		case FC_BMP085_SUNNY:			//Slowly rising HP stable good weather (Clear/Sunny)
			m_LastSendForecast = bmpbaroforecast_sunny;
			break;
		case FC_BMP085_CLOUDY_RAIN:		//Slowly falling Low Pressure System, stable rainy weather (Cloudy/Rain)
			m_LastSendForecast = bmpbaroforecast_cloudy;
			if (m_LastSendForecast == bmpbaroforecast_cloudy)
			{
				if (pressure < 1010)
					m_LastSendForecast = bmpbaroforecast_rain;
			}
			break;
		case FC_BMP085_UNSTABLE:		//Quickly rising HP, not stable weather
			if (m_LastForecast == bmpbaroforecast_unknown)
				m_LastSendForecast = bmpbaroforecast_unstable;
			break;
		case FC_BMP085_THUNDERSTORM:	//Quickly falling LP, Thunderstorm, not stable (Thunderstorm)
			m_LastSendForecast = bmpbaroforecast_thunderstorm;
			break;
		case FC_BMP085_UNKNOWN:			//
		{
			int nforecast = bmpbaroforecast_cloudy;
			if (pressure <= 980)
				nforecast = bmpbaroforecast_thunderstorm;
			else if (pressure <= 995)
				nforecast = bmpbaroforecast_rain;
			else if (pressure >= 1029)
				nforecast = bmpbaroforecast_sunny;
			m_LastSendForecast = nforecast;
		}
		break;
		}
	}
	return m_LastSendForecast;
}

void I2C::bmp_Read_BMP_SensorDetails()
{
	double temperature, pressure;
	double altitude;

#ifndef HAVE_LINUX_I2C
#ifndef _DEBUG
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!...", szI2CTypeNames[m_dev_type]);
	return;
#else
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!... Debug: just adding a value", szI2CTypeNames[m_dev_type]);
#endif
	temperature = 21.3;
	pressure = 1021.22;
	altitude = 10.0;
#else
	int fd = i2c_Open(m_ActI2CBus.c_str());
	if (fd < 0)
		return;
	if (bmp_Calibration(fd) < 0) {
		_log.Log(LOG_ERROR, "%s: Error reading sensor data!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	if (bmp_GetTemperature(fd, &temperature) < 0) {
		_log.Log(LOG_ERROR, "%s: Error reading temperature!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	if (bmp_GetPressure(fd, &pressure) < 0) {
		_log.Log(LOG_ERROR, "%s: Error reading pressure!...", szI2CTypeNames[m_dev_type]);
		close(fd);
		return;
	}
	close(fd);
	altitude = bmp_altitude(pressure);
#endif

	_tTempBaro tsensor;
	tsensor.id1 = 1;
	tsensor.temp = float(temperature);
	tsensor.baro = float(pressure);
	tsensor.altitude = float(altitude);

	//this is probably not good, need to take the rising/falling of the pressure into account?
	//any help would be welcome!

	tsensor.forecast = CalculateForcast(((float)pressure) * 10.0f);
	sDecodeRXMessage(this, (const unsigned char *)&tsensor, NULL, 255);
}

bool I2C::readBME280ID(const int fd, int &ChipID, int &Version)
{
	uint8_t rValues[2];
	if (ReadInt(fd, rValues, BMEx8x_ChipVersion, 2) != 0) {
		_log.Log(LOG_ERROR, "%s: Error reading BME ChipVersion", szI2CTypeNames[m_dev_type]);
		return false;
	}
	ChipID = rValues[0];
	Version = rValues[1];
	return true;
}

int16_t getShort(uint8_t *data, int index)
{
	// return two bytes from data as a signed 16-bit value
	return uint16_t((data[index + 1] << 8) + data[index]);
}

uint16_t getUShort(uint8_t *data, int index)
{
	//return two bytes from data as an unsigned 16-bit value
	return uint16_t(data[index + 1] << 8) + data[index];
}

int8_t getChar(uint8_t *data, int index)
{
	// return one byte from data as a signed char
	int result = data[index];
	if (result > 127)
		result -= 256;
	return (int8_t)result;
}

uint8_t getUChar(uint8_t *data, int index)
{
	// return one byte from data as an unsigned char
	return data[index];
}

bool I2C::readBME280All(const int fd, float &temp, float &pressure, int &humidity)
{
	if (WriteCmd(fd, BMEx8x_OverSampling_Control) != 0) {
		_log.Log(LOG_ERROR, "%s: Error Writing to I2C register", szI2CTypeNames[m_dev_type]);
		return false;
	}

	uint8_t cal1[24] = { 0 };
	uint8_t cal2[1] = { 0 };
	uint8_t cal3[7] = { 0 };
	if (ReadInt(fd, cal1, 0x88, 24) != 0) {
		_log.Log(LOG_ERROR, "%s: Error Reading calibration data 1", szI2CTypeNames[m_dev_type]);
		return false;
	}
	if (ReadInt(fd, cal2, 0xA1, 1) != 0) {
		_log.Log(LOG_ERROR, "%s: Error Reading calibration data 2", szI2CTypeNames[m_dev_type]);
		return false;
	}
	if (ReadInt(fd, cal3, 0xE1, 7) != 0) {
		_log.Log(LOG_ERROR, "%s: Error Reading calibration data 3", szI2CTypeNames[m_dev_type]);
		return false;
	}

	// Convert byte data to word values
	uint16_t dig_T1 = getUShort(cal1, 0);
	int16_t dig_T2 = getShort(cal1, 2);
	int16_t dig_T3 = getShort(cal1, 4);

	uint16_t dig_P1 = getUShort(cal1, 6);
	int16_t dig_P2 = getShort(cal1, 8);
	int16_t dig_P3 = getShort(cal1, 10);
	int16_t dig_P4 = getShort(cal1, 12);
	int16_t dig_P5 = getShort(cal1, 14);
	int16_t dig_P6 = getShort(cal1, 16);
	int16_t dig_P7 = getShort(cal1, 18);
	int16_t dig_P8 = getShort(cal1, 20);
	int16_t dig_P9 = getShort(cal1, 22);

	uint8_t dig_H1 = getUChar(cal2, 0);
	int16_t dig_H2 = getShort(cal3, 0);
	uint8_t dig_H3 = getUChar(cal3, 2);

	int dig_H4 = getChar(cal3, 3);
	dig_H4 = (dig_H4 << 24) >> 20;
	dig_H4 = dig_H4 | (getChar(cal3, 4) & 0x0F);

	int dig_H5 = getChar(cal3, 5);
	dig_H5 = (dig_H5 << 24) >> 20;
	dig_H5 = dig_H5 | (getUChar(cal3, 4) >> 4 & 0x0F);

	int8_t dig_H6 = getChar(cal3, 6);

	// Read temperature/pressure/humidity
	uint8_t data[8] = { 0 };
	if (ReadInt(fd, data, BMEx8x_Data, 8) != 0) {
		_log.Log(LOG_ERROR, "%s: Error Reading sensor data", szI2CTypeNames[m_dev_type]);
		return false;
	}

	int pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
	int temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
	int hum_raw = (data[6] << 8) | data[7];

	// Refine temperature
	double var1 = ((((temp_raw >> 3) - (dig_T1 << 1)))*(dig_T2)) >> 11;
	double var2 = (((((temp_raw >> 4) - (dig_T1)) * ((temp_raw >> 4) - (dig_T1))) >> 12) * (dig_T3)) >> 14;
	double t_fine = var1 + var2;
	temp = float((int(t_fine * 5) + 128) >> 8);
	temp /= 100.0f;

	// Refine pressure and adjust for temperature
	var1 = t_fine / 2.0 - 64000.0;
	var2 = var1 * var1 * dig_P6 / 32768.0;
	var2 = var2 + var1 * dig_P5 * 2.0;
	var2 = var2 / 4.0 + dig_P4 * 65536.0;
	var1 = (dig_P3 * var1 * var1 / 524288.0 + dig_P2 * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0) * dig_P1;
	double dpressure;
	if (var1 == 0)
		dpressure = 0;
	else
	{
		dpressure = 1048576.0 - pres_raw;
		dpressure = ((dpressure - var2 / 4096.0) * 6250.0) / var1;
		var1 = dig_P9 * dpressure * dpressure / 2147483648.0;
		var2 = dpressure * dig_P8 / 32768.0;
		dpressure = dpressure + (var1 + var2 + dig_P7) / 16.0;
	}
	pressure = float(dpressure) / 100.0f;

	// Refine humidity
	double dhumidity = t_fine - 76800.0;
	dhumidity = (hum_raw - (dig_H4 * 64.0 + dig_H5 / 16384.0 * dhumidity)) * (dig_H2 / 65536.0 * (1.0 + dig_H6 / 67108864.0 * dhumidity * (1.0 + dig_H3 / 67108864.0 * dhumidity)));
	dhumidity = dhumidity * (1.0 - dig_H1 * dhumidity / 524288.0);
	if (dhumidity > 100)
		dhumidity = 100;
	else if (dhumidity < 0)
		dhumidity = 0;
	humidity = round(dhumidity);
	return true;
}

void I2C::bmp_Read_BME_SensorDetails()
{
	float temperature, pressure;
	int humidity;

#ifndef HAVE_LINUX_I2C
#ifndef _DEBUG
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!...", szI2CTypeNames[m_dev_type]);
	return;
#else
	_log.Log(LOG_ERROR, "%s: I2C is unsupported on this architecture!... Debug: just adding a value", szI2CTypeNames[m_dev_type]);
#endif
	temperature = 21.3f;
	pressure = 1021.22f;
	humidity = 70;
#else
	int fd = i2c_Open(m_ActI2CBus.c_str()); // open i2c
	if (fd < 0) {
		_log.Log(LOG_ERROR, "%s: Error opening device!...", szI2CTypeNames[m_dev_type]);
		return;
	}

	if (!readBME280All(fd, temperature, pressure, humidity))
	{
		close(fd);
		return;
	}
	close(fd);
#endif
	int forecast = CalculateForcast(((float)pressure) * 10.0f);
	//We are using the TempHumBaro Float type now, convert the forecast
	int nforecast = wsbaroforcast_some_clouds;
	if (pressure <= 980)
		nforecast = wsbaroforcast_heavy_rain;
	else if (pressure <= 995)
	{
		if (temperature > 1)
			nforecast = wsbaroforcast_rain;
		else
			nforecast = wsbaroforcast_snow;
	}
	else if (pressure >= 1029)
		nforecast = wsbaroforcast_sunny;
	switch (forecast)
	{
	case bmpbaroforecast_sunny:
		nforecast = wsbaroforcast_sunny;
		break;
	case bmpbaroforecast_cloudy:
		nforecast = wsbaroforcast_cloudy;
		break;
	case bmpbaroforecast_thunderstorm:
		nforecast = wsbaroforcast_heavy_rain;
		break;
	case bmpbaroforecast_rain:
		if (temperature > 1)
			nforecast = wsbaroforcast_rain;
		else
			nforecast = wsbaroforcast_snow;
		break;
	}

	SendTempHumBaroSensorFloat(1, 255, temperature, humidity, pressure, nforecast, "TempHumBaro");
}
