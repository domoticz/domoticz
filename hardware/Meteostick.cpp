#include "stdafx.h"
#include "Meteostick.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "P1MeterBase.h"
#include "hardwaretypes.h"
#include <string>
#include <algorithm>
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#include <ctime>

#define RETRY_DELAY 30

#define round(a) ( int ) ( a + .5 )

#define USE_868_Mhz
#define RAIN_IN_MM


//
//Class Meteostick
//
Meteostick::Meteostick(const int ID, const std::string& devname, const unsigned int baud_rate):
	m_szSerialPort(devname)
{
	m_HwdID=ID;
	m_iBaudRate=baud_rate;
	m_state = MSTATE_INIT;
	m_bufferpos = 0;
	for (int ii = 0; ii < MAX_IDS; ii++)
	{
		m_LastOutsideTemp[ii] = 12345;
		m_LastOutsideHum[ii] = 0;
		m_ActRainCounter[ii] = -1;
		m_LastRainValue[ii] = -1;
	}
}

bool Meteostick::StartHardware()
{
	RequestStart();

	m_retrycntr = RETRY_DELAY; //will force reconnect first thing

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return true;
}

bool Meteostick::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

bool Meteostick::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS, "Meteostick: Using serial port: %s", m_szSerialPort.c_str());
		open(
			m_szSerialPort,
			m_iBaudRate,
			boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
			boost::asio::serial_port_base::character_size(8)
			);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR, "Meteostick:Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Meteostick:Error opening serial port!!!");
		return false;
	}
	m_state = MSTATE_INIT;
	m_bIsStarted = true;
	m_bufferpos = 0;

	for (int ii = 0; ii < MAX_IDS; ii++)
	{
		m_LastOutsideTemp[ii]	= 12345;
		m_LastOutsideHum[ii]	= 0;
		m_ActRainCounter[ii]	= -1;
		m_LastRainValue[ii]		= -1;
	}
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	sOnConnected(this);
	return true;
}


void Meteostick::Do_Work()
{
	int sec_counter = 0;
	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				_log.Log(LOG_STATUS, "Meteostick: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr >= RETRY_DELAY)
			{
				m_retrycntr = 0;
				OpenSerialDevice();
			}
		}
	}
	terminate();

	_log.Log(LOG_STATUS, "Meteostick: Worker stopped...");
}


void Meteostick::readCallback(const char *data, size_t len)
{
	if (!m_bIsStarted)
		return;

	if (!m_bEnableReceive)
		return; //receiving not enabled

	ParseData((const unsigned char*)data, static_cast<int>(len));
}

bool Meteostick::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void Meteostick::ParseData(const unsigned char *pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const unsigned char c = pData[ii];
		if (c == 0x0d)
		{
			ii++;
			continue;
		}

		if (c == 0x0a || m_bufferpos == sizeof(m_buffer)-1)
		{
			// discard newline, close string, parse line and clear it.
			if (m_bufferpos > 0) m_buffer[m_bufferpos] = 0;
			ParseLine();
			m_bufferpos = 0;
		}
		else
		{
			m_buffer[m_bufferpos] = c;
			m_bufferpos++;
		}
		ii++;
	}
}

void Meteostick::SendTempBaroSensorInt(const unsigned char Idx, const float Temp, const float Baro, const std::string &defaultname)
{
	//Calculate Pressure
	float altitude = 188.0F; // Should be custom defined for each user

	float dTempGradient = 0.0065F;
	float dTempAtSea = (Temp - (-273.15F)) + dTempGradient * altitude;
	float dBasis = 1 - dTempGradient * altitude / dTempAtSea;
	float dExponent = 0.03416F / dTempGradient;
	float dPressure = Baro / std::pow(dBasis,dExponent);

	SendTempBaroSensor(Idx, 255, Temp, dPressure, defaultname);
}

void Meteostick::SendWindSensor(const unsigned char Idx, const float Temp, const float Speed, const int Direction, const std::string &defaultname)
{
	RBUF tsen;

	memset(&tsen, 0, sizeof(RBUF));
	tsen.WIND.packetlength = sizeof(tsen.WIND) - 1;
	tsen.WIND.packettype = pTypeWIND;
	tsen.WIND.subtype = sTypeWINDNoTemp;
	tsen.WIND.battery_level = 9;
	tsen.WIND.rssi = 12;
	tsen.WIND.id1 = 0;
	tsen.WIND.id2 = Idx;

	int aw = round(Direction);
	tsen.WIND.directionh = (BYTE)(aw / 256);
	aw -= (tsen.WIND.directionh * 256);
	tsen.WIND.directionl = (BYTE)(aw);

	tsen.WIND.av_speedh = 0;
	tsen.WIND.av_speedl = 0;
	int sw = round(Speed * 10.0F);
	tsen.WIND.av_speedh = (BYTE)(sw / 256);
	sw -= (tsen.WIND.av_speedh * 256);
	tsen.WIND.av_speedl = (BYTE)(sw);

	tsen.WIND.gusth = 0;
	tsen.WIND.gustl = 0;

	tsen.WIND.chillh = 0;
	tsen.WIND.chilll = 0;

	float dWindSpeed = Speed * 3.6F;
	float dWindChill = Temp;
	if (dWindSpeed > 5 && Temp < 10)
	{
		float dBasis = dWindSpeed;
		float dExponent = 0.16F;
		float dWind = std::pow(dBasis,dExponent);
		dWindChill = (13.12F + 0.6215F * Temp - 11.37F * dWind + 0.3965F * Temp * dWind);
	}
	dWindChill *= 10.0F;
	tsen.WIND.chillsign = (dWindChill >= 0) ? 0 : 1;
	tsen.WIND.chillh = (BYTE)(dWindChill / 256);
	dWindChill -= (tsen.WIND.chillh * 256);
	tsen.WIND.chilll = (BYTE)(dWindChill);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, defaultname.c_str(), 255, nullptr);
}

void Meteostick::SendLeafWetnessRainSensor(const unsigned char Idx, const unsigned char Channel, const int Wetness, const std::string &defaultname)
{
	int finalID = (Idx * 10) + Channel;
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeLeafWetness;
	gdevice.intval1 = Wetness;
	gdevice.id = (uint8_t)finalID;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), 255, nullptr);
}

void Meteostick::SendSoilMoistureSensor(const unsigned char Idx, const unsigned char Channel, const int Moisture, const std::string &defaultname)
{
	int finalID = (Idx * 10) + Channel;
	SendMoistureSensor(finalID,255, Moisture, defaultname);
}

void Meteostick::SendSolarRadiationSensor(const unsigned char Idx, const float Radiation, const std::string &defaultname)
{
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeSolarRadiation;
	gdevice.id = static_cast<int>(Idx);
	gdevice.floatval1 = Radiation;
	sDecodeRXMessage(this, (const unsigned char *)&gdevice, defaultname.c_str(), 255, nullptr);
}

void Meteostick::ParseLine()
{
	if (m_bufferpos < 1)
		return;
	std::string sLine((char*)&m_buffer);

	std::vector<std::string> results;
	StringSplit(sLine, " ", results);
	if (results.empty())
		return; //invalid data

	switch (m_state)
	{
	case MSTATE_INIT:
		if (sLine.find("# MeteoStick Version") == 0) {
			_log.Log(LOG_STATUS, sLine);
			return;
		}
		if (results[0] == "?")
		{
			//Turn off filters
			write("f0\n");
			m_state = MSTATE_FILTERS;
		}
		return;
	case MSTATE_FILTERS:
		//Set output to 'computer values'
		write("o1\n");
		m_state = MSTATE_VALUES;
		return;
	case MSTATE_VALUES:
#ifdef USE_868_Mhz
		//Set listen frequency to 868Mhz
		write("m1\n");
#else
		//Set listen frequency to 915Mhz
		write("m0\n");
#endif
		m_state = MSTATE_DATA;
		return;
	}

	if (m_state != MSTATE_DATA)
		return;

	if (results.size() < 3)
		return;

	unsigned char rCode = results[0][0];
	if (rCode == '#')
		return;

//#ifdef _DEBUG
	_log.Log(LOG_NORM, sLine);
//#endif

	switch (rCode)
	{
	case 'B':
		//temperature in Celsius, pressure in hPa
		if (results.size() >= 3)
		{
			float temp = static_cast<float>(atof(results[1].c_str()));
			float baro = static_cast<float>(atof(results[2].c_str()));

			SendTempBaroSensorInt(0, temp, baro, "Meteostick Temp+Baro");
		}
		break;
	case 'W':
		//current wind speed in m / s, wind direction in degrees
		if (results.size() >= 5)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			if (m_LastOutsideTemp[ID%MAX_IDS] != 12345)
			{
				float speed = static_cast<float>(atof(results[2].c_str()));
				int direction = static_cast<int>(atoi(results[3].c_str()));
				SendWindSensor(ID, m_LastOutsideTemp[ID%MAX_IDS], speed, direction, "Wind");
			}
		}
		break;
	case 'T':
		//temperature in degree Celsius, humidity in percent
		if (results.size() >= 5)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			float temp = static_cast<float>(atof(results[2].c_str()));
			int hum = static_cast<int>(atoi(results[3].c_str()));

			SendTempHumSensor(ID, 255, temp, hum, "Outside Temp+Hum");
			m_LastOutsideTemp[ID%MAX_IDS] = temp;
			m_LastOutsideHum[ID%MAX_IDS] = hum;
		}
		break;
	case 'R':
		//Rain
		//counter value (value 0 - 255), ticks, 1 tick = 0.2mm or 0.01in
		//it only has a small counter, so we should make the total counter ourselfses
		if (results.size() >= 4)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			int raincntr = atoi(results[2].c_str());
			float Rainmm = 0;
			if (m_LastRainValue[ID%MAX_IDS] != -1)
			{
				int cntr_diff = (raincntr - m_LastRainValue[ID%MAX_IDS])&255;
				Rainmm = float(cntr_diff);
#ifdef RAIN_IN_MM
				//one tick is one mm
				Rainmm *= 0.2F; // convert to mm;
#else
				//one tick is 0.01 inch, we need to convert this also to mm
				//Rainmm *= 0.01f; //convert to inch
				//Rainmm *= 25.4f; //convert to mm
				//or directly
				Rainmm *= 0.254;
#endif
			}
			m_LastRainValue[ID%MAX_IDS] = raincntr;

			if (m_ActRainCounter[ID%MAX_IDS] == -1)
			{
				//Get Last stored Rain counter
				bool bExists = false;
				float rcounter= GetRainSensorValue(ID,bExists);
				m_ActRainCounter[ID%MAX_IDS] = rcounter;
			}
			m_ActRainCounter[ID%MAX_IDS] += Rainmm;
			SendRainSensor(ID, 255, m_ActRainCounter[ID%MAX_IDS], "Rain");
		}
		break;
	case 'S':
		//solar radiation, solar radiation in W / qm
		if (results.size() >= 4)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			float Radiation = static_cast<float>(atof(results[2].c_str()));
			SendSolarRadiationSensor(ID, Radiation, "Solar Radiation");
		}
		break;
	case 'U':
		//UV index
		if (results.size() >= 4)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			float UV = static_cast<float>(atof(results[2].c_str()));
			CDomoticzHardwareBase::SendUVSensor(0, ID, 255, UV, "UV");
		}
		break;
	case 'L':
		//wetness data of a leaf station
		//channel number (1 - 4), leaf wetness (0-15)
		if (results.size() >= 5)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			unsigned char Channel = (unsigned char)atoi(results[2].c_str());
			unsigned char Wetness = (unsigned char)atoi(results[3].c_str());
			SendLeafWetnessRainSensor(ID, Channel, Wetness, "Leaf Wetness");
		}
		break;
	case 'M':
		//soil moisture of a soil station
		//channel number (1 - 4), Soil moisture in cbar(0 - 200)
		if (results.size() >= 5)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			unsigned char Channel = (unsigned char)atoi(results[2].c_str());
			unsigned char Moisture = (unsigned char)atoi(results[3].c_str());
			SendSoilMoistureSensor(ID, Channel, Moisture, "Soil Moisture");
		}
		break;
	case 'O':
		//soil / leaf temperature of a soil / leaf station
		//channel number (1 - 4), soil / leaf temperature in degrees Celsius
		if (results.size() >= 5)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			unsigned char Channel = (unsigned char)atoi(results[2].c_str());
			float temp = static_cast<float>(atof(results[3].c_str()));
			unsigned char finalID = (ID * 10) + Channel;
			SendTempSensor(finalID, 255, temp, "Soil/Leaf Temp");
		}
		break;
	case 'P':
		//solar panel power in(0 - 100)
		if (results.size() >= 4)
		{
			unsigned char ID = (unsigned char)atoi(results[1].c_str());
			float Percentage = static_cast<float>(atof(results[2].c_str()));
			SendPercentageSensor(ID, 0, 255, Percentage, "power of solar panel");
		}
		break;
	default:
		_log.Log(LOG_STATUS, "Unknown Type: %c", rCode);
		break;
	}

}
