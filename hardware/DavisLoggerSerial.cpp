#include "stdafx.h"
#include "DavisLoggerSerial.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <ctime>

#ifdef _DEBUG
//#define DEBUG_DAVIS
#endif

#define RETRY_DELAY 30
#define DAVIS_READ_INTERVAL 30

#define round(a) ( int ) ( a + .5 )

CDavisLoggerSerial::CDavisLoggerSerial(const int ID, const std::string& devname, unsigned int baud_rate) :
	m_szSerialPort(devname)
{
	m_HwdID = ID;
	m_iBaudRate = baud_rate;
	m_stoprequested = false;
	m_retrycntr = RETRY_DELAY;
	m_statecounter = 0;
	m_state = DSTATE_WAKEUP;
}

CDavisLoggerSerial::~CDavisLoggerSerial(void)
{

}

bool CDavisLoggerSerial::StartHardware()
{
	StopHardware();
	m_retrycntr = RETRY_DELAY; //will force reconnect first thing
	m_stoprequested = false;
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CDavisLoggerSerial::Do_Work, this);

	return (m_thread != nullptr);

}

bool CDavisLoggerSerial::StopHardware()
{
	m_stoprequested = true;
	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}
	// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
	sleep_milliseconds(10);
	terminate();
	m_bIsStarted = false;
	return true;
}

bool CDavisLoggerSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, m_iBaudRate);
		_log.Log(LOG_STATUS, "Davis: Using serial port: %s", m_szSerialPort.c_str());
		m_statecounter = 0;
		m_state = DSTATE_WAKEUP;
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Davis: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Davis: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted = true;
	setReadCallback(boost::bind(&CDavisLoggerSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

bool CDavisLoggerSerial::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void CDavisLoggerSerial::Do_Work()
{
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);

		sec_counter++;

		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}
		if (m_stoprequested)
			break;

#ifdef DEBUG_DAVIS
		sOnConnected(this);
		HandleLoopData(NULL, 0);
		continue;
#endif

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				_log.Log(LOG_STATUS, "Davis: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				OpenSerialDevice();
			}
		}
		else
		{
			switch (m_state)
			{
			case DSTATE_WAKEUP:
				m_statecounter++;
				if (m_statecounter < 5) {
					write("\n", 1);
				}
				else {
					m_retrycntr = 0;
					//still did not receive a wakeup, lets try again
					terminate();
				}
				break;
			case DSTATE_LOOP:
				m_statecounter++;
				if (m_statecounter >= DAVIS_READ_INTERVAL)
				{
					m_statecounter = 0;
					write("LOOP 1\n", 7);
				}
				break;
			}
		}
	}
	_log.Log(LOG_STATUS, "Davis: Serial Worker stopped...");
}

void CDavisLoggerSerial::readCallback(const char *data, size_t len)
{
	std::lock_guard<std::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"Davis: received %ld bytes",len);

		switch (m_state)
		{
		case DSTATE_WAKEUP:
			if (len == 2) {
				_log.Log(LOG_NORM, "Davis: System is Awake...");
				m_state = DSTATE_LOOP;
				m_statecounter = DAVIS_READ_INTERVAL - 1;
			}
			break;
		case DSTATE_LOOP:
			if (len == 2)
				break; //could be a left over from the awake
			if (len != 100) {
				_log.Log(LOG_ERROR, "Davis: Invalid bytes received!...");
				//lets try again
				terminate();
			}
			else {
				if (!HandleLoopData((const unsigned char*)data, len))
				{
					//error in data, try again...
					terminate();
				}
			}
			break;
		}
	}
	catch (...)
	{

	}
}

bool CDavisLoggerSerial::HandleLoopData(const unsigned char *data, size_t len)
{
	const uint8_t *pData = data + 1;

#ifndef DEBUG_DAVIS
	if (len != 100)
		return false;

	if (
		(data[1] != 'L') ||
		(data[2] != 'O') ||
		(data[3] != 'O') ||
		(data[96] != 0x0a) ||
		(data[97] != 0x0d)
		)
		return false;
	//bool bIsRevA = (data[4]=='P');
#else
	//	FILE *fOut=fopen("davisrob.bin","wb+");
	//	fwrite(data,1,len,fOut);
	//	fclose(fOut);
	unsigned char szBuffer[200];
	FILE *fIn = fopen("E:\\davis2.bin", "rb+");
	//FILE *fIn=fopen("davisrob.bin","rb+");
	fread(&szBuffer, 1, 100, fIn);
	fclose(fIn);
	pData = szBuffer + 1;
	//bool bIsRevA(szBuffer[4]=='P');
#endif

	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));


	unsigned char tempIdx = 1;

	bool bBaroValid = false;
	float BaroMeter = 0;
	bool bInsideTemperatureValid = false;
	float InsideTemperature = 0;
	bool bInsideHumidityValid = false;
	int InsideHumidity = 0;
	bool bOutsideTemperatureValid = false;
	float OutsideTemperature = 0;
	bool bOutsideHumidityValid = false;
	int OutsideHumidity = 0;

	bool bWindDirectionValid = false;
	int WindDirection = 0;

	bool bWindSpeedValid = false;
	float WindSpeed = 0;
	//bool bWindSpeedAVR10Valid=false;
	//float WindSpeedAVR10=0;

	bool bUVValid = false;
	float UV = 0;

	//Barometer
	if ((pData[7] != 0xFF) && (pData[8] != 0xFF))
	{
		bBaroValid = true;
		BaroMeter = ((unsigned int)((pData[8] << 8) | pData[7])) / 29.53f; //in hPa
	}
	//Inside Temperature
	if ((pData[9] != 0xFF) || (pData[10] != 0x7F))
	{
		bInsideTemperatureValid = true;
		InsideTemperature = ((unsigned int)((pData[10] << 8) | pData[9])) / 10.f;
		InsideTemperature = (InsideTemperature - 32.0f) * 5.0f / 9.0f;
	}
	//Inside Humidity
	if (pData[11] != 0xFF)
	{
		InsideHumidity = pData[11];
		if (InsideHumidity < 101)
			bInsideHumidityValid = true;
	}

	if (bBaroValid&&bInsideTemperatureValid&&bInsideHumidityValid)
	{
		uint8_t forecastitem = pData[89];
		uint8_t forecast = 0;

		if ((forecastitem & 0x01) == 0x01)
			forecast = wsbaroforcast_rain;
		else if ((forecastitem & 0x02) == 0x02)
			forecast = wsbaroforcast_cloudy;
		else if ((forecastitem & 0x04) == 0x04)
			forecast = wsbaroforcast_some_clouds;
		else if ((forecastitem & 0x08) == 0x08)
			forecast = wsbaroforcast_sunny;
		else if ((forecastitem & 0x10) == 0x10)
			forecast = wsbaroforcast_snow;

		SendTempHumBaroSensorFloat(tempIdx++, 255, InsideTemperature, InsideHumidity, BaroMeter, forecast, "THB");
	}

	//Outside Temperature
	if ((pData[12] != 0xFF) || (pData[13] != 0x7F))
	{
		bOutsideTemperatureValid = true;
		//OutsideTemperature=((unsigned int)((pData[13] << 8) | pData[12])) / 10.f;
		//OutsideTemperature = (OutsideTemperature - 32.0f) * 5.0f / 9.0f;

		uint8_t msb = pData[13];
		uint8_t lsb = pData[12];

		uint16_t temp16 = ((msb << 8) | lsb);
		if (temp16 > 0x800) {
			// Negative values, convert to float from two complements int
			int temp_int = (temp16 | ~((1 << 16) - 1));
			OutsideTemperature = (float)temp_int / 10.0f;
		}
		else {
			OutsideTemperature = (float)temp16 / 10.0f;
		}

		// Convert to celsius
		OutsideTemperature = (OutsideTemperature - 32.0f) * 5.0f / 9.0f;
	}
	//Outside Humidity
	if (pData[33] != 0xFF)
	{
		OutsideHumidity = pData[33];
		if (OutsideHumidity < 101)
			bOutsideHumidityValid = true;
	}
	if (bOutsideTemperatureValid || bOutsideHumidityValid)
	{
		if ((bOutsideTemperatureValid) && (bOutsideHumidityValid))
		{
			//Temp+hum
			SendTempHumSensor(tempIdx++, 255, OutsideTemperature, OutsideHumidity, "Outside TempHum");
		}
		else if (bOutsideTemperatureValid)
		{
			//Temp
			SendTempSensor(tempIdx++, 255, OutsideTemperature, "Outside Temp");
		}
		else if (bOutsideHumidityValid)
		{
			//hum
			SendHumiditySensor(tempIdx++, 255, OutsideHumidity, "Outside Humidity");
		}
	}

	tempIdx = 10;
	//Add Extra Temp/Hum Sensors
	int iTmp;
	for (iTmp = 0; iTmp < 7; iTmp++)
	{
		bool bTempValid = false;
		bool bHumValid = false;
		float temp = 0;
		uint8_t hum = 0;

		if (pData[18 + iTmp] != 0xFF)
		{
			bTempValid = true;
			temp = pData[18 + iTmp] - 90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (pData[34 + iTmp] != 0xFF)
		{
			bHumValid = true;
			hum = pData[34 + iTmp];
		}
		if ((bTempValid) && (bHumValid))
		{
			//Temp+hum
			SendTempHumSensor(tempIdx++, 255, temp, hum, "Extra TempHum");
		}
		else if (bTempValid)
		{
			//Temp
			SendTempSensor(tempIdx++, 255, temp, "Extra Temp");
		}
		else if (bHumValid)
		{
			//hum
			SendHumiditySensor(tempIdx++, 255, hum, "Extra Humidity");
		}
	}

	tempIdx = 20;
	//Add Extra Soil Temp Sensors
	for (iTmp = 0; iTmp < 4; iTmp++)
	{
		bool bTempValid = false;
		float temp = 0;

		if (pData[25 + iTmp] != 0xFF)
		{
			bTempValid = true;
			temp = pData[25 + iTmp] - 90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (bTempValid)
		{
			SendTempSensor(tempIdx++, 255, temp, "Soil Temp");
		}
	}

	tempIdx = 30;
	//Add Extra Leaf Temp Sensors
	for (iTmp = 0; iTmp < 4; iTmp++)
	{
		bool bTempValid = false;
		float temp = 0;

		if (pData[29 + iTmp] != 0xFF)
		{
			bTempValid = true;
			temp = pData[29 + iTmp] - 90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (bTempValid)
		{
			SendTempSensor(tempIdx++, 255, temp, "Leaf Temp");
		}
	}

	//Wind Speed
	if (pData[14] != 0xFF)
	{
		bWindSpeedValid = true;
		WindSpeed = (pData[14])*(4.0f / 9.0f);
	}
	//Wind Speed AVR 10 minutes
	if (pData[15] != 0xFF)
	{
		//bWindSpeedAVR10Valid=true;
		//WindSpeedAVR10=(pData[15])*(4.0f/9.0f);
	}
	//Wind Direction
	if ((pData[16] != 0xFF) && (pData[17] != 0x7F))
	{
		bWindDirectionValid = true;
		WindDirection = ((unsigned int)((pData[17] << 8) | pData[16]));
	}

	if ((bWindSpeedValid) && (bWindDirectionValid))
	{
		memset(&tsen, 0, sizeof(RBUF));
		tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
		tsen.WIND.packettype = pTypeWIND;
		tsen.WIND.subtype = sTypeWINDNoTemp;
		tsen.WIND.battery_level = 9;
		tsen.WIND.rssi = 12;
		tsen.WIND.id1 = 0;
		tsen.WIND.id2 = 1;

		int aw = round(WindDirection);
		tsen.WIND.directionh = (BYTE)(aw / 256);
		aw -= (tsen.WIND.directionh * 256);
		tsen.WIND.directionl = (BYTE)(aw);

		tsen.WIND.av_speedh = 0;
		tsen.WIND.av_speedl = 0;
		int sw = round(WindSpeed*10.0f);
		tsen.WIND.av_speedh = (BYTE)(sw / 256);
		sw -= (tsen.WIND.av_speedh * 256);
		tsen.WIND.av_speedl = (BYTE)(sw);

		tsen.WIND.gusth = 0;
		tsen.WIND.gustl = 0;

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh = 0;
		tsen.WIND.chilll = 0;
		tsen.WIND.temperatureh = 0;
		tsen.WIND.temperaturel = 0;
		if (bOutsideTemperatureValid)
		{
			tsen.WIND.tempsign = (OutsideTemperature >= 0) ? 0 : 1;
			tsen.WIND.chillsign = (OutsideTemperature >= 0) ? 0 : 1;
			int at10 = round(std::abs(OutsideTemperature*10.0f));
			tsen.WIND.temperatureh = (BYTE)(at10 / 256);
			tsen.WIND.chillh = (BYTE)(at10 / 256);
			at10 -= (tsen.WIND.chillh * 256);
			tsen.WIND.temperaturel = (BYTE)(at10);
			tsen.WIND.chilll = (BYTE)(at10);
		}

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, NULL, 255);
	}

	//UV
	if (pData[43] != 0xFF)
	{
		UV = (pData[43]) / 10.0f;
		if (UV < 100)
			bUVValid = true;
	}
	if (bUVValid)
	{
		SendUVSensor(0, 1, 255, UV, "UV");
	}

	//Rain Rate
	if ((pData[41] != 0xFF) && (pData[42] != 0xFF))
	{
		//float rainRate=((unsigned int)((pData[42] << 8) | pData[41])) / 100.0f; //inches
		//rainRate*=25.4f; //mm
	}
	//Rain Day
	if ((pData[50] != 0xFF) && (pData[51] != 0xFF))
	{
		//float rainDay=((unsigned int)((pData[51] << 8) | pData[50])) / 100.0f; //inches
		//rainDay*=25.4f; //mm
	}
	//Rain Year
	if ((pData[54] != 0xFF) && (pData[55] != 0xFF))
	{
		float rainYear = ((unsigned int)((pData[55] << 8) | pData[54])) / 100.0f; //inches
		rainYear *= 25.4f; //mm

		SendRainSensor(1, 255, rainYear, "Rain");
	}

	//Solar Radiation
	if ((pData[44] != 0xFF) && (pData[45] != 0x7F))
	{
		unsigned int solarRadiation = ((unsigned int)((pData[45] << 8) | pData[44]));//Watt/M2
		_tGeneralDevice gdevice;
		gdevice.subtype = sTypeSolarRadiation;
		gdevice.floatval1 = float(solarRadiation);
		sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);

	}

	//Soil Moistures
	for (int iMoister = 0; iMoister < 4; iMoister++)
	{
		if (pData[62 + iMoister] != 0xFF)
		{
			int moister = pData[62 + iMoister];
			SendMoistureSensor(1 + iMoister, 255, moister, "Moisture");
		}
	}

	//Leaf Wetness
	for (int iLeaf = 0; iLeaf < 4; iLeaf++)
	{
		if (pData[66 + iLeaf] != 0xFF)
		{
			int leaf_wetness = pData[66 + iLeaf];

			_tGeneralDevice gdevice;
			gdevice.subtype = sTypeLeafWetness;
			gdevice.intval1 = leaf_wetness;
			gdevice.id = (uint8_t)(1 + iLeaf);
			sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
		}
	}

	return true;
}

