#include "stdafx.h"
#include "DavisLoggerSerial.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

#include <ctime>

//#define DEBUG_DAVIS

#define RETRY_DELAY 30
#define DAVIS_READ_INTERVAL 30

#define round(a) ( int ) ( a + .5 )

CDavisLoggerSerial::CDavisLoggerSerial(const int ID, const std::string& devname, unsigned int baud_rate)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
	m_iBaudRate=baud_rate;
	m_stoprequested=false;
	m_bIsStarted=false;

}

CDavisLoggerSerial::~CDavisLoggerSerial(void)
{
}

bool CDavisLoggerSerial::StartHardware()
{
	m_retrycntr=RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDavisLoggerSerial::Do_Work, this)));

	return (m_thread!=NULL);

}

bool CDavisLoggerSerial::StopHardware()
{
	m_stoprequested=true;
	m_thread->join();
	// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
	boost::this_thread::sleep(boost::posix_time::milliseconds(10));
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	return true;
}

bool CDavisLoggerSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort,m_iBaudRate);
		_log.Log(LOG_NORM,"Davis: Using serial port: %s", m_szSerialPort.c_str());
		m_statecounter=0;
		m_state=DSTATE_WAKEUP;
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"Davis: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"Davis: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CDavisLoggerSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CDavisLoggerSerial::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CDavisLoggerSerial::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		if (m_stoprequested)
			break;

#ifdef DEBUG_DAVIS
		sOnConnected(this);
		HandleLoopData(NULL,0);
		continue;
#endif

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"Davis: serial setup retry in %d seconds...", RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
			}
		}
		else
		{
			switch (m_state)
			{
			case DSTATE_WAKEUP:
				m_statecounter++;
				if (m_statecounter<5) {
					write("\n",1);
				}
				else {
					m_retrycntr=0;
					//still did not receive a wakeup, lets try again
					try {
						clearReadCallback();
						close();
						doClose();
						setErrorStatus(true);
					} catch(...)
					{
						//Don't throw from a Stop command
					}
				}
				break;
			case DSTATE_LOOP:
				m_statecounter++;
				if (m_statecounter>=DAVIS_READ_INTERVAL)
				{
					m_statecounter=0;
					write("LOOP 1\n", 7);
				}
				break;
			}
		}
	}
	_log.Log(LOG_NORM,"Davis: Serial Worker stopped...");
} 

void CDavisLoggerSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"Davis: received %ld bytes",len);

		switch (m_state)
		{
		case DSTATE_WAKEUP:
			if (len==2) {
				_log.Log(LOG_NORM,"Davis: System is Awake...");
				m_state=DSTATE_LOOP;
				m_statecounter=DAVIS_READ_INTERVAL-1;
			}
			break;
		case DSTATE_LOOP:
			if (len==2)
				break; //could be a left over from the awake
			if (len!=100) {
				_log.Log(LOG_ERROR,"Davis: Invalid bytes received!...");
				//lets try again
				try {
					clearReadCallback();
					close();
					doClose();
					setErrorStatus(true);
				} catch(...)
				{
					//Don't throw from a Stop command
				}
			}
			else {
				if (!HandleLoopData((const unsigned char*)data,len))
				{
					//error in data, try again...
					try {
						clearReadCallback();
						close();
						doClose();
						setErrorStatus(true);
					} catch(...)
					{
						//Don't throw from a Stop command
					}
				}
			}
			break;
		}
		//onRFXMessage((const unsigned char *)data,len);
	}
	catch (...)
	{

	}
}

void CDavisLoggerSerial::UpdateTempHumSensor(const unsigned char Idx, const float Temp, const int Hum)
{
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
	tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
	tsen.TEMP_HUM.subtype=sTypeTH5;
	tsen.TEMP_HUM.battery_level=9;
	tsen.TEMP_HUM.rssi=6;
	tsen.TEMP_HUM.id1=0;
	tsen.TEMP_HUM.id2=Idx;

	tsen.TEMP_HUM.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP_HUM.temperatureh*256);
	tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
	tsen.TEMP_HUM.humidity=(BYTE)Hum;
	tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);//decode message
}

void CDavisLoggerSerial::UpdateTempSensor(const unsigned char Idx, const float Temp)
{
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=6;
	tsen.TEMP.id1=0;
	tsen.TEMP.id2=Idx;

	tsen.TEMP.tempsign=(Temp>=0)?0:1;
	int at10=round(abs(Temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
}

void CDavisLoggerSerial::UpdateHumSensor(const unsigned char Idx, const int Hum)
{
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
	tsen.HUM.packettype=pTypeHUM;
	tsen.HUM.subtype=sTypeHUM2;
	tsen.HUM.battery_level=9;
	tsen.HUM.rssi=6;
	tsen.HUM.id1=0;
	tsen.HUM.id2=Idx;

	tsen.HUM.humidity=(BYTE)Hum;
	tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);//decode message
}

bool CDavisLoggerSerial::HandleLoopData(const unsigned char *data, size_t len)
{
	const uint8_t *pData=data+1;

#ifndef DEBUG_DAVIS
	if (len!=100)
		return false;

	if (
		(data[1]!='L')||
		(data[2]!='O')||
		(data[3]!='O')||
		(data[96]!=0x0a)||
		(data[97]!=0x0d)
		)
		return false;
	bool bIsRevA = (data[4]=='P');
#else
//	FILE *fOut=fopen("davisrob.bin","wb+");
//	fwrite(data,1,len,fOut);
//	fclose(fOut);
	unsigned char szBuffer[200];
	FILE *fIn=fopen("E:\\davis3.bin","rb+");
	//FILE *fIn=fopen("davisrob.bin","rb+");
	fread(&szBuffer,1,100,fIn);
	fclose(fIn);
	pData=szBuffer+1;
	bool bIsRevA(szBuffer[4]=='P');
#endif

	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));


	unsigned char tempIdx=1;

	bool bBaroValid=false;
	float BaroMeter=0;
	bool bInsideTemperatureValid=false;
	float InsideTemperature=0;
	bool bInsideHumidityValid=false;
	int InsideHumidity=0;
	bool bOutsideTemperatureValid=false;
	float OutsideTemperature=0;
	bool bOutsideHumidityValid=false;
	int OutsideHumidity=0;

	bool bWindDirectionValid=false;
	int WindDirection=0;

	bool bWindSpeedValid=false;
	float WindSpeed=0;
	bool bWindSpeedAVR10Valid=false;
	float WindSpeedAVR10=0;

	bool bUVValid=false;
	float UV=0;

	//Barometer
	if ((pData[7]!=0xFF)&&(pData[8]!=0xFF))
	{
		bBaroValid=true;
		BaroMeter=((unsigned int)((pData[8] << 8) | pData[7])) / 29.53f; //in hPa
	}
	//Inside Temperature
	if ((pData[9]!=0xFF)||(pData[10]!=0x7F))
	{
		bInsideTemperatureValid=true;
		InsideTemperature=((unsigned int)((pData[10] << 8) | pData[9])) / 10.f;
		InsideTemperature = (InsideTemperature - 32.0f) * 5.0f / 9.0f;
	}
	//Inside Humidity
	if (pData[11]!=0xFF)
	{
		InsideHumidity=pData[11];
		if (InsideHumidity<101)
			bInsideHumidityValid=true;
	}

	if (bBaroValid&&bInsideTemperatureValid&&bInsideHumidityValid)
	{
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP_HUM_BARO.packetlength=sizeof(tsen.TEMP_HUM_BARO)-1;
		tsen.TEMP_HUM_BARO.packettype=pTypeTEMP_HUM_BARO;
		tsen.TEMP_HUM_BARO.subtype=sTypeTHBFloat;
		tsen.TEMP_HUM_BARO.battery_level=9;
		tsen.TEMP_HUM_BARO.rssi=6;
		tsen.TEMP_HUM_BARO.id1=0;
		tsen.TEMP_HUM_BARO.id2=tempIdx++;

		tsen.TEMP_HUM_BARO.tempsign=(InsideTemperature>=0)?0:1;
		int at10=round(abs(InsideTemperature*10.0f));
		tsen.TEMP_HUM_BARO.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.TEMP_HUM_BARO.temperatureh*256);
		tsen.TEMP_HUM_BARO.temperaturel=(BYTE)(at10);
		tsen.TEMP_HUM_BARO.humidity=(BYTE)InsideHumidity;
		tsen.TEMP_HUM_BARO.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

		int ab10=round(BaroMeter*10.0f);
		tsen.TEMP_HUM_BARO.baroh=(BYTE)(ab10/256);
		ab10-=(tsen.TEMP_HUM_BARO.baroh*256);
		tsen.TEMP_HUM_BARO.barol=(BYTE)(ab10);

		uint8_t forecastitem=pData[89];
		int forecast=0;

		if ((forecastitem&0x01)==0x01)
			forecast=wsbaroforcast_rain;
		else if ((forecastitem&0x02)==0x02)
			forecast=wsbaroforcast_cloudy;
		else if ((forecastitem&0x04)==0x04)
			forecast=wsbaroforcast_some_clouds;
		else if ((forecastitem&0x08)==0x08)
			forecast=wsbaroforcast_sunny;
		else if ((forecastitem&0x10)==0x10)
			forecast=wsbaroforcast_snow;

		tsen.TEMP_HUM_BARO.forecast=forecast;

		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO);//decode message
	}

	//Outside Temperature
	if ((pData[12]!=0xFF)||(pData[13]!=0x7F))
	{
		bOutsideTemperatureValid=true;
		OutsideTemperature=((unsigned int)((pData[13] << 8) | pData[12])) / 10.f;

		OutsideTemperature = (OutsideTemperature - 32.0f) * 5.0f / 9.0f;
	}
	//Outside Humidity
	if (pData[33]!=0xFF)
	{
		OutsideHumidity=pData[33];
		if (OutsideHumidity<101)
			bOutsideHumidityValid=true;
	}
	if (bOutsideTemperatureValid||bOutsideHumidityValid)
	{
		//add outside sensor
		memset(&tsen,0,sizeof(RBUF));

		if ((bOutsideTemperatureValid)&&(bOutsideHumidityValid))
		{
			//Temp+hum
			UpdateTempHumSensor(tempIdx++,OutsideTemperature,OutsideHumidity);
		}
		else if (bOutsideTemperatureValid)
		{
			//Temp
			UpdateTempSensor(tempIdx++,OutsideTemperature);
		}
		else if (bOutsideHumidityValid)
		{
			//hum
			UpdateHumSensor(tempIdx++,OutsideHumidity);
		}
	}

	tempIdx=10;
	//Add Extra Temp/Hum Sensors
	int iTmp;
	for (int iTmp=0; iTmp<7; iTmp++)
	{
		bool bTempValid=false;
		bool bHumValid=false;
		float temp=0;
		uint8_t hum=0;

		if (pData[18+iTmp]!=0xFF)
		{
			bTempValid=true;
			temp=pData[18+iTmp]-90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (pData[34+iTmp]!=0xFF)
		{
			bHumValid=true;
			hum=pData[34+iTmp];
		}
		if ((bTempValid)&&(bHumValid))
		{
			//Temp+hum
			UpdateTempHumSensor(tempIdx++,temp,hum);
		}
		else if (bTempValid)
		{
			//Temp
			UpdateTempSensor(tempIdx++,temp);
		}
		else if (bHumValid)
		{
			//hum
			UpdateHumSensor(tempIdx++,hum);
		}
	}

	tempIdx=20;
	//Add Extra Soil Temp Sensors
	for (iTmp=0; iTmp<4; iTmp++)
	{
		bool bTempValid=false;
		float temp=0;

		if (pData[25+iTmp]!=0xFF)
		{
			bTempValid=true;
			temp=pData[25+iTmp]-90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (bTempValid)
		{
			UpdateTempSensor(tempIdx++,temp);
		}
	}

	tempIdx=30;
	//Add Extra Leaf Temp Sensors
	for (iTmp=0; iTmp<4; iTmp++)
	{
		bool bTempValid=false;
		float temp=0;

		if (pData[29+iTmp]!=0xFF)
		{
			bTempValid=true;
			temp=pData[29+iTmp]-90.0f;
			temp = (temp - 32.0f) * 5.0f / 9.0f;
		}
		if (bTempValid)
		{
			UpdateTempSensor(tempIdx++,temp);
		}
	}

	//Wind Speed
	if (pData[14]!=0xFF)
	{
		bWindSpeedValid=true;
		WindSpeed=(pData[14])*(4.0f/9.0f);
	}
	//Wind Speed AVR 10 minutes
	if (pData[15]!=0xFF)
	{
		bWindSpeedAVR10Valid=true;
		WindSpeedAVR10=(pData[15])*(4.0f/9.0f);
	}
	//Wind Direction
	if ((pData[16]!=0xFF)&&(pData[17]!=0x7F))
	{
		bWindDirectionValid=true;
		WindDirection=((unsigned int)((pData[17] << 8) | pData[16]));
	}

	if ((bWindSpeedValid)&&(bWindDirectionValid))
	{
		memset(&tsen,0,sizeof(RBUF));
		tsen.WIND.packetlength=sizeof(tsen.WIND)-1;
		tsen.WIND.packettype=pTypeWIND;
		tsen.WIND.subtype=sTypeWINDNoTemp;
		tsen.WIND.battery_level=9;
		tsen.WIND.rssi=6;
		tsen.WIND.id1=0;
		tsen.WIND.id2=1;

		int aw=round(WindDirection);
		tsen.WIND.directionh=(BYTE)(aw/256);
		aw-=(tsen.WIND.directionh*256);
		tsen.WIND.directionl=(BYTE)(aw);

		tsen.WIND.av_speedh=0;
		tsen.WIND.av_speedl=0;
		int sw=round(WindSpeed*10.0f);
		tsen.WIND.av_speedh=(BYTE)(sw/256);
		sw-=(tsen.WIND.av_speedh*256);
		tsen.WIND.av_speedl=(BYTE)(sw);

		tsen.WIND.gusth=0;
		tsen.WIND.gustl=0;

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh=0;
		tsen.WIND.chilll=0;
		tsen.WIND.temperatureh=0;
		tsen.WIND.temperaturel;
		if (bOutsideTemperatureValid)
		{
			tsen.WIND.tempsign=(OutsideTemperature>=0)?0:1;
			tsen.WIND.chillsign=(OutsideTemperature>=0)?0:1;
			int at10=round(abs(OutsideTemperature*10.0f));
			tsen.WIND.temperatureh=(BYTE)(at10/256);
			tsen.WIND.chillh=(BYTE)(at10/256);
			at10-=(tsen.WIND.chillh*256);
			tsen.WIND.temperaturel=(BYTE)(at10);
			tsen.WIND.chilll=(BYTE)(at10);
		}

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND);//decode message
	}

	//UV
	if (pData[43]!=0xFF)
	{
		UV=(pData[43])/10.0f;
		if (UV<100)
			bUVValid=true;
	}
	if (bUVValid)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.UV.packetlength=sizeof(tsen.UV)-1;
		tsen.UV.packettype=pTypeUV;
		tsen.UV.subtype=sTypeUV1;
		tsen.UV.battery_level=9;
		tsen.UV.rssi=6;
		tsen.UV.id1=0;
		tsen.UV.id2=1;

		tsen.UV.uv=(BYTE)round(UV*10);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.UV);//decode message
	}
	
	//Rain Rate
	if ((pData[41]!=0xFF)&&(pData[42]!=0xFF))
	{
		float rainRate=((unsigned int)((pData[42] << 8) | pData[41])) / 100.0f; //inches
		rainRate*=25.4f; //mm
	}
	//Rain Day
	if ((pData[50]!=0xFF)&&(pData[51]!=0xFF))
	{
		float rainDay=((unsigned int)((pData[51] << 8) | pData[50])) / 100.0f; //inches
		rainDay*=25.4f; //mm
	}
	//Rain Year
	if ((pData[54]!=0xFF)&&(pData[55]!=0xFF))
	{
		float rainYear=((unsigned int)((pData[55] << 8) | pData[54])) / 100.0f; //inches
		rainYear*=25.4f; //mm

		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.RAIN.packetlength=sizeof(tsen.RAIN)-1;
		tsen.RAIN.packettype=pTypeRAIN;
		tsen.RAIN.subtype=sTypeRAIN3;
		tsen.RAIN.battery_level=9;
		tsen.RAIN.rssi=6;
		tsen.RAIN.id1=0;
		tsen.RAIN.id2=1;

		tsen.RAIN.rainrateh=0;
		tsen.RAIN.rainratel=0;

		int tr10=int(float(rainYear)*10.0f);

		tsen.RAIN.raintotal1=0;
		tsen.RAIN.raintotal2=(BYTE)(tr10/256);
		tr10-=(tsen.RAIN.raintotal2*256);
		tsen.RAIN.raintotal3=(BYTE)(tr10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN);//decode message
	}

	//Solar Radiation
	if ((pData[44]!=0xFF)&&(pData[45]!=0x7F))
	{
		unsigned int solarRadiation=((unsigned int)((pData[45] << 8) | pData[44]));//Watt/M2
	}

	//Soil Moistures
	for (int iMoister=0; iMoister<4; iMoister++)
	{
		if (pData[62+iMoister]!=0xFF)
		{
			int moister=pData[62+iMoister];
		}
	}

	return true;
}

