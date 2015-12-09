
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
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CTE923::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);

	return (m_thread!=NULL);
}

bool CTE923::StopHardware()
{
	/*
    m_stoprequested=true;
	if (m_thread)
		m_thread->join();
	return true;
    */
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
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
		boost::this_thread::sleep( boost::posix_time::milliseconds(500) );

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
		if ((data._t[ii]==0)&&(data._h[ii]==0))
		{
			//Temp+Hum
			RBUF tsen;
			memset(&tsen,0,sizeof(RBUF));

			//special case if baro is present for first sensor
			if ((ii==0)&&(data._press == 0 ))
			{
				tsen.TEMP_HUM_BARO.packetlength=sizeof(tsen.TEMP_HUM_BARO)-1;
				tsen.TEMP_HUM_BARO.packettype=pTypeTEMP_HUM_BARO;
				tsen.TEMP_HUM_BARO.subtype=sTypeTHBFloat;

				tsen.TEMP_HUM_BARO.battery_level=9;
				tsen.TEMP_HUM_BARO.rssi=12;
				tsen.TEMP_HUM_BARO.id1=0;
				tsen.TEMP_HUM_BARO.id2=ii;

				tsen.TEMP_HUM_BARO.tempsign=(data.t[ii]>=0)?0:1;
				int at10=round(abs(data.t[ii]*10.0f));
				tsen.TEMP_HUM_BARO.temperatureh=(BYTE)(at10/256);
				at10-=(tsen.TEMP_HUM_BARO.temperatureh*256);
				tsen.TEMP_HUM_BARO.temperaturel=(BYTE)(at10);
				tsen.TEMP_HUM_BARO.humidity=(BYTE)data.h[ii];
				tsen.TEMP_HUM_BARO.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

				int ab10=round(data.press*10.0f);
				tsen.TEMP_HUM_BARO.baroh=(BYTE)(ab10/256);
				ab10-=(tsen.TEMP_HUM_BARO.baroh*256);
				tsen.TEMP_HUM_BARO.barol=(BYTE)(ab10);
				tsen.TEMP_HUM_BARO.forecast=data.forecast;

				sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO, NULL, (dev.battery[ii - 1]) ? 100 : 0);
			}
			else
			{
				tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
				tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
				tsen.TEMP_HUM.subtype=sTypeTH5;
				if (dev.battery[ii-1])
					tsen.TEMP_HUM.battery_level=9;
				else
					tsen.TEMP_HUM.battery_level=0;
				tsen.TEMP_HUM.rssi=12;
				tsen.TEMP_HUM.id1=0;
				tsen.TEMP_HUM.id2=ii;

				tsen.TEMP_HUM.tempsign=(data.t[ii]>=0)?0:1;
				int at10=round(abs(data.t[ii]*10.0f));
				tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
				at10-=(tsen.TEMP_HUM.temperatureh*256);
				tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
				tsen.TEMP_HUM.humidity=(BYTE)data.h[ii];
				tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM, NULL, -1);
			}
		}
		else if (data._t[ii]==0)
		{
			//Temp
			RBUF tsen;
			memset(&tsen,0,sizeof(RBUF));
			tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
			tsen.TEMP.packettype=pTypeTEMP;
			tsen.TEMP.subtype=sTypeTEMP10;
			if (ii>0)
			{
				if (dev.battery[ii-1])
					tsen.TEMP.battery_level=9;
				else
					tsen.TEMP.battery_level=0;
			}
			else
			{
				tsen.TEMP.battery_level=9;
			}
			tsen.TEMP.rssi=12;
			tsen.TEMP.id1=0;
			tsen.TEMP.id2=ii;

			tsen.TEMP.tempsign=(data.t[ii]>=0)?0:1;
			int at10=round(abs(data.t[ii]*10.0f));
			tsen.TEMP.temperatureh=(BYTE)(at10/256);
			at10-=(tsen.TEMP.temperatureh*256);
			tsen.TEMP.temperaturel=(BYTE)(at10);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP, NULL, -1);
		}
		else if (data._h[ii]==0)
		{
			//Hum
			RBUF tsen;
			memset(&tsen,0,sizeof(RBUF));
			tsen.HUM.packetlength=sizeof(tsen.HUM)-1;
			tsen.HUM.packettype=pTypeHUM;
			tsen.HUM.subtype=sTypeHUM2;
			if (ii>0)
			{
				if (dev.battery[ii-1])
					tsen.HUM.battery_level=9;
				else
					tsen.HUM.battery_level=0;
			}
			else
			{
				tsen.HUM.battery_level=9;
			}
			tsen.HUM.rssi=12;
			tsen.HUM.id1=0;
			tsen.HUM.id2=ii;

			tsen.HUM.humidity=(BYTE)data.h[ii];
			tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM, NULL, -1);
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
			int at10=round(abs(data.wChill*10.0f));
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
	}
	//UV
	if (data._uv==0)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.UV.packetlength=sizeof(tsen.UV)-1;
		tsen.UV.packettype=pTypeUV;
		tsen.UV.subtype=sTypeUV1;
		if (dev.batteryUV)
			tsen.UV.battery_level=9;
		else
			tsen.UV.battery_level=0;
		tsen.UV.rssi=12;
		tsen.UV.id1=0;
		tsen.UV.id2=1;

		tsen.UV.uv=(BYTE)round(data.uv*10);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.UV, NULL, -1);
	}
}
#endif //WITH_LIBUSB
#endif //WIN32
