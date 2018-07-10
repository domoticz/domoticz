
#include "stdafx.h"
#ifndef WIN32
#ifdef WITH_LIBUSB
#include "TE923.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "TE923Tool.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define TE923_POLL_INTERVAL 30

#define round(a) ( int ) ( a + .5 )

//at this moment it does not work under windows... no idea why... help appriciated!
//for this we fake the data previously read by a station on unix (weatherdata.bin)

CTE923::CTE923(const int ID)
{
	m_HwdID=ID;
	m_stoprequested=false;
	Init();
}

CTE923::~CTE923(void)
{
}

void CTE923::Init()
{
}

bool CTE923::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CTE923::Do_Work, this);
	m_bIsStarted=true;
	sOnConnected(this);

	return (m_thread != nullptr);
}

bool CTE923::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
    return true;
}

void CTE923::Do_Work()
{
	int sec_counter=TE923_POLL_INTERVAL-4;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			mytime(&m_LastHeartbeat);
		}

		if (sec_counter % TE923_POLL_INTERVAL == 0)
		{
			GetSensorDetails();
		}
	}
	_log.Log(LOG_STATUS,"TE923: Worker stopped...");
}

bool CTE923::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CTE923::GetSensorDetails()
{
	Te923DataSet_t data;
	Te923DevSet_t dev;
#ifndef _DEBUG
	CTE923Tool _te923tool;
	if (!_te923tool.OpenDevice())
	{
		return;
	}
	if (!_te923tool.GetData(&data,&dev))
	{
		//give it one more change!
		_te923tool.CloseDevice();
		sleep_milliseconds(500);

		CTE923Tool _te923tool2;
		if (!_te923tool2.OpenDevice())
		{
			return;
		}
		if (!_te923tool2.GetData(&data,&dev))
		{
			_log.Log(LOG_ERROR, "TE923: Could not read weather data!");
			return;
		}
		else
			_te923tool2.CloseDevice();
	}
	else
		_te923tool.CloseDevice();
#else
	FILE *fIn=fopen("weatherdata.bin","rb+");
	fread(&data,1,sizeof(Te923DataSet_t),fIn);
	fclose(fIn);
#endif

	if (data._press != 0 )
	{
		_log.Log(LOG_ERROR, "TE923: No Barometric pressure in weather station, reading skipped!");
		return;
	}
	if ((data.press<800)||(data.press>1200))
	{
		_log.Log(LOG_ERROR, "TE923: Invalid weather station data received (baro)!");
		return;
	}

	int ii;
	for (ii=0; ii<6; ii++)
	{
		if (data._t[ii]==0)
		{
			if ((data.t[ii]<-60)||(data.t[ii]>60))
			{
				_log.Log(LOG_ERROR, "TE923: Invalid weather station data received (temp)!");
				return;
			}
		}
	}

	//Add temp sensors
	for (ii=0; ii<6; ii++)
	{
		int BatLevel = 100;
		if (ii > 0)
		{
			if (dev.battery[ii - 1])
				BatLevel = 100;
			else
				BatLevel = 0;
		}

		if ((data._t[ii]==0)&&(data._h[ii]==0))
		{
			//Temp+Hum
			//special case if baro is present for first sensor
			if ((ii==0)&&(data._press == 0 ))
			{
				SendTempHumBaroSensorFloat(ii, BatLevel, data.t[ii], data.h[ii], data.press, data.forecast, "THB");
			}
			else
			{
				SendTempHumSensor(ii, BatLevel, data.t[ii], data.h[ii], "TempHum");
			}
		}
		else if (data._t[ii]==0)
		{
			//Temp
			SendTempSensor(ii, BatLevel, data.t[ii], "Temperature");
		}
		else if (data._h[ii]==0)
		{
			//Hum
			SendHumiditySensor(ii, BatLevel, data.h[ii], "Humidity");
		}
	}

	//Wind
	if (data._wDir==0)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.WIND.packetlength=sizeof(tsen.WIND)-1;
		tsen.WIND.packettype=pTypeWIND;
		tsen.WIND.subtype=sTypeWINDNoTemp;
		dev.batteryWind = 1;
		if (dev.batteryWind)
			tsen.WIND.battery_level=9;
		else
			tsen.WIND.battery_level=0;
		tsen.WIND.rssi=12;
		tsen.WIND.id1=0;
		tsen.WIND.id2=1;

		float winddir=float(data.wDir)*22.5f;
		int aw=round(winddir);
		tsen.WIND.directionh=(BYTE)(aw/256);
		aw-=(tsen.WIND.directionh*256);
		tsen.WIND.directionl=(BYTE)(aw);

		tsen.WIND.av_speedh=0;
		tsen.WIND.av_speedl=0;
		if (data._wSpeed==0)
		{
			int sw=round(data.wSpeed*10.0f);
			tsen.WIND.av_speedh=(BYTE)(sw/256);
			sw-=(tsen.WIND.av_speedh*256);
			tsen.WIND.av_speedl=(BYTE)(sw);
		}
		tsen.WIND.gusth=0;
		tsen.WIND.gustl=0;
		if (data._wGust==0)
		{
			int gw=round(data.wGust*10.0f);
			tsen.WIND.gusth=(BYTE)(gw/256);
			gw-=(tsen.WIND.gusth*256);
			tsen.WIND.gustl=(BYTE)(gw);
		}

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh=0;
		tsen.WIND.chilll=0;
		tsen.WIND.temperatureh=0;
		tsen.WIND.temperaturel=0;
		if (data._wChill==0)
		{
			tsen.WIND.tempsign=(data.wChill>=0)?0:1;
			tsen.WIND.chillsign=(data.wChill>=0)?0:1;
			int at10=round(std::abs(data.wChill*10.0f));
			tsen.WIND.temperatureh=(BYTE)(at10/256);
			tsen.WIND.chillh=(BYTE)(at10/256);
			at10-=(tsen.WIND.chillh*256);
			tsen.WIND.temperaturel=(BYTE)(at10);
			tsen.WIND.chilll=(BYTE)(at10);
		}

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, NULL, -1);
	}

	//Rain
	if (data._RainCount==0)
	{
		int BatLevel = (dev.batteryRain) ? 100 : 0;
		SendRainSensor(1, BatLevel, float(data.RainCount) / 0.7f, "Rain");
/*
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.RAIN.packetlength=sizeof(tsen.RAIN)-1;
		tsen.RAIN.packettype=pTypeRAIN;
		tsen.RAIN.subtype=sTypeRAIN3;
		if (dev.batteryRain)
			tsen.RAIN.battery_level=9;
		else
			tsen.RAIN.battery_level=0;
		tsen.RAIN.rssi=12;
		tsen.RAIN.id1=0;
		tsen.RAIN.id2=1;

		tsen.RAIN.rainrateh=0;
		tsen.RAIN.rainratel=0;

		int tr10=int((float(data.RainCount)*10.0f)*0.7f);

		tsen.RAIN.raintotal1=0;
		tsen.RAIN.raintotal2=(BYTE)(tr10/256);
		tr10-=(tsen.RAIN.raintotal2*256);
		tsen.RAIN.raintotal3=(BYTE)(tr10);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, NULL, -1);
*/
	}
	//UV
	if (data._uv==0)
	{
		SendUVSensor(0, 1, 255, data.uv, "UV");
	}
}
#endif //WITH_LIBUSB
#endif //WIN32
