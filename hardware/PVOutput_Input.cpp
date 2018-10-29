#include "stdafx.h"
#include "PVOutput_Input.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_PVOutputInput

CPVOutputInput::CPVOutputInput(const int ID, const std::string &SID, const std::string &Key) :
m_SID(SID),
m_KEY(Key)
{
	m_HwdID=ID;
	Init();
}

CPVOutputInput::~CPVOutputInput(void)
{
}

void CPVOutputInput::Init()
{
}

bool CPVOutputInput::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CPVOutputInput::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CPVOutputInput::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

#define PVOUTPUT_POLL_INTERVAL 5

void CPVOutputInput::Do_Work()
{
	int LastMinute=-1;
	_log.Log(LOG_STATUS,"PVOutput (Input): Worker started...");
	while (!IsStopRequested(1000))
	{
		time_t atime=mytime(NULL);
		m_LastHeartbeat = atime;
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if (((ltime.tm_min/PVOUTPUT_POLL_INTERVAL!=LastMinute))&&(ltime.tm_sec>20))
		{
			LastMinute=ltime.tm_min/PVOUTPUT_POLL_INTERVAL;
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"PVOutput (Input): Worker stopped...");
}

bool CPVOutputInput::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void CPVOutputInput::GetMeterDetails()
{
	if (m_SID.size()==0)
		return;
	if (m_KEY.size()==0)
		return;

	std::string sResult;

	std::stringstream sstr;
	sstr << "http://pvoutput.org/service/r2/getstatus.jsp?sid=" << m_SID << "&key=" << m_KEY;
	if (!HTTPClient::GET(sstr.str(),sResult))
	{
		_log.Log(LOG_ERROR,"PVOutput (Input): Error login!");
		return;
	}
	std::vector<std::string> splitresult;
	StringSplit(sResult,",",splitresult);
	if (splitresult.size()<9)
	{
		_log.Log(LOG_ERROR,"PVOutput (Input): Invalid Data received!");
		return;
	}

	double Usage=atof(splitresult[3].c_str());
	if (Usage < 0)
		Usage = 0;

	bool bHaveConsumption=false;
	double Consumption=0;
	if (splitresult[5]!="NaN")
	{
		Consumption=atof(splitresult[5].c_str());
		if (Consumption < 0)
			Consumption = 0;
		bHaveConsumption=true;
	}

	if (splitresult[6]!="NaN")
	{
		double Efficiency=atof(splitresult[6].c_str())*100.0;
		if (Efficiency>100.0)
			Efficiency=100.0;
		SendPercentageSensor(1, 0, 255, float(Efficiency), "Efficiency");
	}
	if (splitresult[7]!="NaN")
	{
		double Temperature=atof(splitresult[7].c_str());
		SendTempSensor(1, 255, float(Temperature), "Temperature");
	}
	if (splitresult[8]!="NaN")
	{
		double Voltage=atof(splitresult[8].c_str());
		if (Voltage >= 0)
			SendVoltageSensor(0, 1, 255, float(Voltage), "Voltage");
	}

	sstr.clear();
	sstr.str("");

	sstr << "http://pvoutput.org/service/r2/getstatistic.jsp?sid=" << m_SID << "&key=" << m_KEY << "&c=1";
	if (!HTTPClient::GET(sstr.str(),sResult))
	{
		_log.Log(LOG_ERROR,"PVOutput (Input): Error login!");
		return;
	}
	StringSplit(sResult,",",splitresult);
	if (splitresult.size()<11)
	{
		_log.Log(LOG_ERROR,"PVOutput (Input): Invalid Data received!");
		return;
	}

	double kWhCounterUsage=atof(splitresult[0].c_str());
	SendKwhMeter(0, 1, 255, Usage, kWhCounterUsage / 1000.0, "SolarMain");

	if (bHaveConsumption)
	{
		if (splitresult.size() > 11)
		{
			double kWhCounterConsumed = atof(splitresult[11].c_str());
			if (kWhCounterConsumed != 0)
			{
				SendKwhMeter(0, 2, 255, Consumption, kWhCounterConsumed / 1000.0, "SolarConsumed");
			}
		}
	}
}

