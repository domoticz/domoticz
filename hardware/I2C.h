#pragma once

#include "DomoticzHardware.h"

typedef union _i2c_data
{
  uint8_t  byte[2] ;
  uint16_t word ;
} i2c_data;

class I2C : public CDomoticzHardwareBase
{
public:
	enum _eI2CType
	{
		I2CTYPE_UNKNOWN = 0,
		I2CTYPE_BMP085,
		I2CTYPE_HTU21D,
		I2CTYPE_TSL2561,
		I2CTYPE_PCF8574,
		I2CTYPE_BME280,
		I2CTYPE_MCP23017
	};

	explicit I2C(const int ID, const _eI2CType DevType, const int Port);
	~I2C();
	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
	bool StartHardware();
	bool StopHardware();
	void HTU21D_ReadSensorDetails();
	void bmp_Read_BMP_SensorDetails();
	void bmp_Read_BME_SensorDetails();

	bool readBME280ID(const int fd, int &ChipID, int &Version);
	bool readBME280All(const int fd, float &temp, float &pressure, int &humidity);

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;

	std::string m_ActI2CBus;
	_eI2CType m_dev_type;


	bool i2c_test(const char *I2CBusName);
	int i2c_Open(const char *I2CBusName);

	int ReadInt(int fd, uint8_t *devValues, uint8_t startReg, uint8_t bytesToRead);
	int WriteCmd(int fd, uint8_t devAction);

	// BMP085 stuff
	//Forecast
	int bmp_CalculateForecast(const float pressure);
	int CalculateForcast(const float pressure);
	float m_LastPressure;
	int m_LastMinute;
	float m_pressureSamples[180];
	int m_minuteCount;
	bool m_firstRound;
	float m_pressureAvg[7];
	int m_LastForecast;
	unsigned char m_LastSendForecast;
	int bmp_Calibration(int fd);
	int bmp_WaitForConversion(int fd);
	int bmp_GetPressure(int fd, double *Pres);
	int bmp_GetTemperature(int fd, double *Temp);

	double   bmp_altitude(double p);
	double   bmp_qnh(double p, double StationAlt);
	double   bmp_ppl_DensityAlt(double PAlt, double Temp);

	// Calibration values - These are stored in the BMP085/180
	short int            bmp_ac1;
	short int            bmp_ac2;
	short int            bmp_ac3;
	unsigned short int   bmp_ac4;
	unsigned short int   bmp_ac5;
	unsigned short int   bmp_ac6;
	short int            bmp_b1;
	short int            bmp_b2;
	int                  bmp_b5;
	short int            bmp_mb;
	short int            bmp_mc;
	short int            bmp_md;

	// HTU21D stuff
	int HTU21D_checkCRC8(uint16_t data);
	int HTU21D_GetHumidity(int fd, float *Pres);
	int HTU21D_GetTemperature(int fd, float *Temp);

	// TSL2561 stuff
	void TSL2561_ReadSensorDetails();
	void TSL2561_Init();
	
	// PCF8574
	unsigned char	i2c_addr;
	void			PCF8574_ReadChipDetails();
	char			PCF8574_WritePin(char pin_number,char  value);
	char 			readByteI2C(int fd, char *byte, char i2c_addr);
	char 			writeByteI2C(int fd, char byte, char i2c_addr);

	// MCP23017
	void 			MCP23017_Init();
	void			MCP23017_ReadChipDetails();
	int			MCP23017_WritePin(char pin_number,char  value);
	int 			I2CWriteReg16(int fd, uint8_t reg, uint16_t value);
	int		 		I2CReadReg16(int fd, unsigned char reg, i2c_data *data);

};
