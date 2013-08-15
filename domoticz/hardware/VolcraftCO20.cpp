
#include "stdafx.h"
#ifndef WIN32
#include "VolcraftCO20.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "VolcraftCO20Tool.h"

#define VolcraftCO20_POLL_INTERVAL 30

#define round(a) ( int ) ( a + .5 )

//at this moment it does not work under windows... no idea why... help appriciated!
//for this we fake the data previously read by a station on unix (weatherdata.bin)

CVolcraftCO20::CVolcraftCO20(const int ID)
{
	m_HwdID=ID;
	m_stoprequested=false;
	Init();
}

CVolcraftCO20::~CVolcraftCO20(void)
{
}

void CVolcraftCO20::Init()
{
	m_LastPollTime=mytime(NULL)-VolcraftCO20_POLL_INTERVAL+2;
}

bool CVolcraftCO20::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CVolcraftCO20::Do_Work, this)));
	sOnConnected(this);

	return (m_thread!=NULL);
}

bool CVolcraftCO20::StopHardware()
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

void CVolcraftCO20::Do_Work()
{
	time_t atime;
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		atime=mytime(NULL);
		if (atime-m_LastPollTime>=VolcraftCO20_POLL_INTERVAL)
		{
			GetSensorDetails();
			m_LastPollTime=mytime(NULL);
		}
	}
	_log.Log(LOG_NORM,"Voltcraft CO-20 CO-20: Worker stopped...");
}

void CVolcraftCO20::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CVolcraftCO20::GetSensorDetails()
{
	CVolcraftCO20Tool _VolcraftCO20tool;
	if (!_VolcraftCO20tool.OpenDevice())
	{
		return;
	}
	unsigned short voc=_VolcraftCO20tool.GetVOC();
	if (voc==0)
	{
		//give it one more change!
		_VolcraftCO20tool.CloseDevice();
		boost::this_thread::sleep( boost::posix_time::milliseconds(500) );

		CVolcraftCO20Tool _VolcraftCO20tool2;
		if (!_VolcraftCO20tool2.OpenDevice())
		{
			return;
		}
		voc=_VolcraftCO20tool2.GetVOC();
		if (voc==0)
		{
			_log.Log(LOG_ERROR, "Voltcraft CO-20: Could not read sensor data!");
			return;
		}
	}
	else
		_VolcraftCO20tool.CloseDevice();
	if (voc==3000)
	{
		_log.Log(LOG_ERROR, "Voltcraft CO-20: Sensor data out of range!!");
		return;
	}
	//got the data
	_tAirQualityMeter meter;
	meter.len=sizeof(_tAirQualityMeter)-1;
	meter.type=pTypeAirQuality;
	meter.subtype=sTypeVoltcraft;
	meter.airquality=voc;
	meter.id1=1;
	sDecodeRXMessage(this, (const unsigned char *)&meter);//decode message
}

#endif
