#include "stdafx.h"
#include "PVOutput_Input.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include <json/json.h>
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../httpclient/HTTPClient.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_PVOutputInput

CPVOutputInput::CPVOutputInput(const int ID, const std::string& SID, const std::string& Key) :
	m_SID(SID),
	m_KEY(Key)
{
	m_HwdID = ID;
	Init();
}

void CPVOutputInput::Init()
{
	m_bHadConsumption = false;
}

bool CPVOutputInput::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
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
	m_bIsStarted = false;
	return true;
}

#define PVOUTPUT_POLL_INTERVAL 5

void CPVOutputInput::Do_Work()
{
	int LastMinute = -1;
	_log.Log(LOG_STATUS, "PVOutput (Input): Worker started...");
	while (!IsStopRequested(1000))
	{
		time_t atime = mytime(nullptr);
		m_LastHeartbeat = atime;
		struct tm ltime;
		localtime_r(&atime, &ltime);
		if (((ltime.tm_min / PVOUTPUT_POLL_INTERVAL != LastMinute)) && (ltime.tm_sec > 20))
		{
			LastMinute = ltime.tm_min / PVOUTPUT_POLL_INTERVAL;
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "PVOutput (Input): Worker stopped...");
}

bool CPVOutputInput::WriteToHardware(const char* pdata, const unsigned char length)
{
	return false;
}

void CPVOutputInput::GetMeterDetails()
{
	if (m_SID.empty())
		return;
	if (m_KEY.empty())
		return;

	std::string sResult;

	std::stringstream sstr;
	sstr << "https://pvoutput.org/service/r2/getstatus.jsp?sid=" << m_SID << "&key=" << m_KEY;
	if (!HTTPClient::GET(sstr.str(), sResult))
	{
		_log.Log(LOG_ERROR, "PVOutput (Input): Error login!");
		return;
	}
	std::vector<std::string> splitresult;
	StringSplit(sResult, ",", splitresult);
	if (splitresult.size() < 9)
	{
		_log.Log(LOG_ERROR, "PVOutput (Input): Invalid Data received!");
		return;
	}
	/*
		0-Date	yyyymmdd	date	20100830	r1
		1-Time	hh : mm	time	14 : 10	r1
		2-Energy Generation	number	watt hours	12936	r1
		3-Power Generation	number	watt	202	r1
		4-Energy Consumption	number	watt hours	19832	r1
		5-Power Consumption	number	watt	459	r1
		6-Normalised Output	number	kW / kW	0.083	r1
		7-Temperature	decimal	celsius	15.3	r2
		8-Voltage
	*/

	double Usage = 0;
	if (splitresult[3] != "NaN")
	{
		Usage = atof(splitresult[3].c_str());
	}
	if (Usage < 0)
		Usage = 0;

	double Consumption = 0;
	if (splitresult[5] != "NaN")
	{
		Consumption = atof(splitresult[5].c_str());
		if (Consumption < 0)
			Consumption = 0;
		m_bHadConsumption = true;
	}

	if (splitresult[6] != "NaN")
	{
		double Efficiency = atof(splitresult[6].c_str()) * 100.0;
		if (Efficiency > 100.0)
			Efficiency = 100.0;
		SendPercentageSensor(1, 0, 255, float(Efficiency), "Efficiency");
	}
	if (splitresult[7] != "NaN")
	{
		double Temperature = atof(splitresult[7].c_str());
		SendTempSensor(1, 255, float(Temperature), "Temperature");
	}
	if (splitresult[8] != "NaN")
	{
		double Voltage = atof(splitresult[8].c_str());
		if (Voltage >= 0)
			SendVoltageSensor(0, 1, 255, float(Voltage), "Voltage");
	}

	sstr.clear();
	sstr.str("");

	sstr << "https://pvoutput.org/service/r2/getstatistic.jsp?sid=" << m_SID << "&key=" << m_KEY << "&c=1&df=19700101&dt=26000101";
	if (!HTTPClient::GET(sstr.str(), sResult))
	{
		_log.Log(LOG_ERROR, "PVOutput (Input): Error login!");
		return;
	}
	StringSplit(sResult, ",", splitresult);
	if (splitresult.size() < 11)
	{
		_log.Log(LOG_ERROR, "PVOutput (Input): Invalid Data received!");
		return;
	}
/*
	0-Energy Generated	number	watt hours	246800
	1-Energy Exported	number	watt hours	246800
	2-Average Generation	number	watt hours	8226
	3-Minimum Generation	number	watt hours	2000
	4-Maximum Generation	number	watt hours	11400
	5-Average Efficiency	number	kWh/kW	3.358
	6-Outputs	number	-	27
	7-Actual Date From	yyyymmdd	date	20100901
	8-Actual Date To	yyyymmdd	date	20100927
	9-Record Efficiency	number	kWh/kW	4.653
	10-Record Date	yyyymmdd	date	20100916
	11-Energy Consumed	number	watt hours	10800
	12-Peak Energy Import	number	watt hours	5000
	13-Off Peak Energy Import	number	watt hours	1000
	14-Shoulder Energy Import	number	watt hours	4000
	15-High Shoulder Energy Import	number	watt hours	800
	16-Average Consumption	number	watt hours	1392
	17-Minimum Consumption	number	watt hours	10
	18-Maximum Consumption	number	watt hours	2890
	19-Credit Amount	decimal	currency	37.29
	20-Debit Amount	decimal	currency	40.81
*/
	double kWhCounterUsage = 0;
	if (splitresult[0] != "NaN")
	{
		kWhCounterUsage = atof(splitresult[0].c_str());
	}
	SendKwhMeter(0, 1, 255, Usage, kWhCounterUsage / 1000.0, "SolarMain");

	if (m_bHadConsumption)
	{
		if (splitresult.size() > 11)
		{
			double kWhCounterConsumed = 0;
			if (splitresult[11] != "NaN")
			{
				kWhCounterConsumed = atof(splitresult[11].c_str());
			}
			
			if (kWhCounterConsumed != 0)
			{
				SendKwhMeter(0, 2, 255, Consumption, kWhCounterConsumed / 1000.0, "SolarConsumed");
			}
		}
	}
}

