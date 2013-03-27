#include "stdafx.h"
#include "TE923.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "TE923Tool.h"

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
	m_LastPollTime=time(NULL)-TE923_POLL_INTERVAL+2;
}

bool CTE923::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CTE923::Do_Work, this)));

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
    return true;
}

void CTE923::Do_Work()
{
	time_t atime;
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		atime=time(NULL);
		if (atime-m_LastPollTime>=TE923_POLL_INTERVAL)
		{
			GetSensorDetails();
			m_LastPollTime=time(NULL);
		}
	}
	_log.Log(LOG_NORM,"TE923 Worker stopped...");
}

void CTE923::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CTE923::GetSensorDetails()
{
	Te923DataSet_t data;
#ifndef _DEBUG
	CTE923Tool _te923tool;
	if (!_te923tool.OpenDevice())
	{
		return;
	}
	if (!_te923tool.GetData(&data))
	{
		//give it one more change!
		_te923tool.CloseDevice();
		boost::this_thread::sleep( boost::posix_time::milliseconds(500) );

		CTE923Tool _te923tool2;
		if (!_te923tool2.OpenDevice())
		{
			return;
		}
		if (!_te923tool2.GetData(&data))
		{
			_log.Log(LOG_ERROR, "Could not read weather data!");
			return;
		}
	}
#else
	FILE *fIn=fopen("weatherdata.bin","rb+");
	fread(&data,1,sizeof(Te923DataSet_t),fIn);
	fclose(fIn);
#endif

	int ii;

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
				tsen.TEMP_HUM_BARO.packetlength=sizeof(tsen);
				tsen.TEMP_HUM_BARO.packettype=pTypeTEMP_HUM_BARO;
				tsen.TEMP_HUM_BARO.subtype=sTypeTHBFloat;
				tsen.TEMP_HUM_BARO.battery_level=9;
				tsen.TEMP_HUM_BARO.rssi=6;
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

				sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO);//decode message
				m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP_HUM_BARO));
			}
			else
			{
				tsen.TEMP_HUM.packetlength=sizeof(tsen);
				tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
				tsen.TEMP_HUM.subtype=sTypeTH5;
				tsen.TEMP_HUM.battery_level=9;
				tsen.TEMP_HUM.rssi=6;
				tsen.TEMP_HUM.id1=0;
				tsen.TEMP_HUM.id2=ii;

				tsen.TEMP_HUM.tempsign=(data.t[ii]>=0)?0:1;
				int at10=round(abs(data.t[ii]*10.0f));
				tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
				at10-=(tsen.TEMP_HUM.temperatureh*256);
				tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
				tsen.TEMP_HUM.humidity=(BYTE)data.h[ii];
				tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);//decode message
				m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP_HUM));
			}
		}
		else if (data._t[ii]==0)
		{
			//Temp
			RBUF tsen;
			memset(&tsen,0,sizeof(RBUF));
			tsen.TEMP.packetlength=sizeof(tsen);
			tsen.TEMP.packettype=pTypeTEMP;
			tsen.TEMP.subtype=sTypeTEMP10;
			tsen.TEMP.battery_level=9;
			tsen.TEMP.rssi=6;
			tsen.TEMP.id1=0;
			tsen.TEMP.id2=ii;

			tsen.TEMP.tempsign=(data.t[ii]>=0)?0:1;
			int at10=round(abs(data.t[ii]*10.0f));
			tsen.TEMP.temperatureh=(BYTE)(at10/256);
			at10-=(tsen.TEMP.temperatureh*256);
			tsen.TEMP.temperaturel=(BYTE)(at10);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
			m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.TEMP));
		}
		else if (data._h[ii]==0)
		{
			//Hum
			RBUF tsen;
			memset(&tsen,0,sizeof(RBUF));
			tsen.HUM.packetlength=sizeof(tsen);
			tsen.HUM.packettype=pTypeHUM;
			tsen.HUM.subtype=sTypeHUM2;
			tsen.HUM.battery_level=9;
			tsen.HUM.rssi=6;
			tsen.HUM.id1=0;
			tsen.HUM.id2=ii;

			tsen.HUM.humidity=(BYTE)data.h[ii];
			tsen.HUM.humidity_status=Get_Humidity_Level(tsen.HUM.humidity);

			sDecodeRXMessage(this, (const unsigned char *)&tsen.HUM);//decode message
			m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.HUM));
		}
	}

	//Wind
	if (data._wDir==0)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.WIND.packetlength=sizeof(tsen);
		tsen.WIND.packettype=pTypeWIND;
		tsen.WIND.subtype=sTypeWINDNoTemp;
		tsen.WIND.battery_level=9;
		tsen.WIND.rssi=6;
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
		tsen.WIND.temperaturel;
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

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND);//decode message
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.WIND));
	}

	//Rain
	if (data._RainCount==0)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.RAIN.packetlength=sizeof(tsen);
		tsen.RAIN.packettype=pTypeRAIN;
		tsen.RAIN.subtype=sTypeRAIN3;
		tsen.RAIN.battery_level=9;
		tsen.RAIN.rssi=6;
		tsen.RAIN.id1=0;
		tsen.RAIN.id2=1;

		tsen.RAIN.rainrateh=0;
		tsen.RAIN.rainratel=0;

		int tr10=int((float(data.RainCount)*10.0f)*0.7f);

		tsen.RAIN.raintotal1=0;
		tsen.RAIN.raintotal2=(BYTE)(tr10/256);
		tr10-=(tsen.RAIN.raintotal2*256);
		tsen.RAIN.raintotal3=(BYTE)(tr10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN);//decode message
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.RAIN));
	}
	//UV
	if (data._uv==0)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.UV.packetlength=sizeof(tsen);
		tsen.UV.packettype=pTypeUV;
		tsen.UV.subtype=sTypeUV1;
		tsen.UV.battery_level=9;
		tsen.UV.rssi=6;
		tsen.UV.id1=0;
		tsen.UV.id2=1;

		tsen.UV.uv=(BYTE)round(data.uv*10);
		sDecodeRXMessage(this, (const unsigned char *)&tsen.UV);//decode message
		m_sharedserver.SendToAll((const char*)&tsen,sizeof(tsen.UV));
	}
}
