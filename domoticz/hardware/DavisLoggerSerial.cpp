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
/*
#ifdef _DEBUG
	unsigned char szBuffer[200];
	FILE *fIn=fopen("E:\\davis.bin","rb+");
	fread(&szBuffer,1,100,fIn);
	fclose(fIn);
	HandleLoopData((const unsigned char*)&szBuffer,100);
#endif
*/
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

bool CDavisLoggerSerial::HandleLoopData(const unsigned char *data, size_t len)
{
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
/*
#ifdef _DEBUG
	FILE *fOut=fopen("davis.bin","wb+");
	fwrite(data,1,len,fOut);
	fclose(fOut);
#endif
*/
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));

	bool bIsRevA = (data[4]=='P');

	const uint8_t *pData=data+1;

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
		tsen.TEMP_HUM_BARO.id2=1;

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
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP_HUM_BARO));
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

	//Wind Speed
	if (pData[14]!=0xFF)
	{
		bWindSpeedValid=true;
		WindSpeed=(pData[14])*(4.0f/9.0f);
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
		if (bOutsideTemperatureValid==0)
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
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.WIND));
	}

	//UV
	if (pData[43]!=0xFF)
	{
		UV=pData[43];
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
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.UV));
	}
	

	return true;
}

