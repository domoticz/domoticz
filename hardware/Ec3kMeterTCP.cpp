#include "stdafx.h"
#include "Ec3kMeterTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
//#include "../main/mainworker.h"
#include "../hardware/hardwaretypes.h"
#include "../json/json.h"

#include <sstream>

/*
  Extract readings from the json messages posted by the
  Energy count 3000/ NETPBSEM4 / La crosse energy meters
  collected by the EC3K software originally from
        https://github.com/avian2/ec3k
  The required server is added to the fork at:
	https://github.com/llagendijk/ec3k
  The server version of this software sends json messages
  with the following contents:

  {
    "data": {
      "energy": 52810582,
      "power_current": 34.4,
      "reset_counter": 1,
      "time_total": 1356705,
      "time_on": 1356613,
      "power_max": 42.2
    },
    "id": "e1a2"
  }
*/

#define RETRY_DELAY 30
#define SENSOR_ID "id"
#define DATA "data"
#define WS "energy"
#define W_CURRENT "power_current"
#define W_MAX "power_max"
#define TIME_ON "time_on"
#define TIME_TOTAL "time_total"
#define RESET_COUNT "reset_counter"

Ec3kMeterTCP::Ec3kMeterTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_bDoRestart=false;
	m_stoprequested=false;
	m_usIPPort=usIPPort;
	m_retrycntr = RETRY_DELAY;
	m_limiter = new(Ec3kLimiter);
}

Ec3kMeterTCP::~Ec3kMeterTCP(void)
{
}

bool Ec3kMeterTCP::StartHardware()
{
	m_stoprequested=false;
	m_bDoRestart=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Ec3kMeterTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool Ec3kMeterTCP::StopHardware()
{
	m_stoprequested=true;
	try {
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}

	m_bIsStarted=false;
	return true;
}

void Ec3kMeterTCP::OnConnect()
{
	_log.Log(LOG_STATUS,"Ec3kMeter: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	m_bDoRestart=false;
	m_bIsStarted=true;

	sOnConnected(this);
}

void Ec3kMeterTCP::OnDisconnect()
{
	_log.Log(LOG_STATUS,"Ec3kMeter: disconnected");
}

void Ec3kMeterTCP::Do_Work()
{
	bool bFirstTime=true;
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (bFirstTime)
		{
			bFirstTime=false;
			connect(m_szIPAddress,m_usIPPort);
		}
		else
		{
			if ((m_bDoRestart) && (sec_counter % 30 == 0))
			{
				connect(m_szIPAddress,m_usIPPort);
			}
			update();
		}
	}
	_log.Log(LOG_STATUS,"Ec3kMeter: TCP/IP Worker stopped...");
} 

void Ec3kMeterTCP::OnData(const unsigned char *pData, size_t length)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	ParseData(pData,length);
}

void Ec3kMeterTCP::OnError(const std::exception e)
{
	_log.Log(LOG_ERROR,"Ec3kMeter: Error: %s",e.what());
}

void Ec3kMeterTCP::OnError(const boost::system::error_code& error)
{
	if (
		(error == boost::asio::error::address_in_use) ||
		(error == boost::asio::error::connection_refused) ||
		(error == boost::asio::error::access_denied) ||
		(error == boost::asio::error::host_unreachable) ||
		(error == boost::asio::error::timed_out)
		)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: Can not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	}
	else if (
		(error == boost::asio::error::eof) ||
		(error == boost::asio::error::connection_reset)
		)
	{
		_log.Log(LOG_STATUS, "Ec3kMeter: Connection reset!");
	}
	else
		_log.Log(LOG_ERROR, "Ec3kMeter: %s", error.message().c_str());
}

bool Ec3kMeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!mIsConnected)
	{
		return false;
	}
	return true;
}


void Ec3kMeterTCP::ParseData(const unsigned char *pData, int Len)
{
	std::string buffer;
	buffer.assign((char *)pData, Len);

	// Validty check on the received json

	Json::Value root;
	Json::Reader jReader;

	bool ret = jReader.parse(buffer, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: invalid data received!");
		return;
	}
	if (root[SENSOR_ID].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: id not found in telegram");
		return;
	}
	if (root[DATA].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: data not found in telegram");
		return;
	}

	Json::Value data = root["data"];
	if (data[WS].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: energy (ws) not found in telegram");
		return;
	}

	if (data[W_CURRENT].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: current consumption not found in telegram");
		return;
	}
	if (data[W_MAX].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: maximum consumption not found in telegram");
		return;
	}
	if (data[TIME_ON].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: time on not found in telegram");
		return;
	}
	if (data[TIME_TOTAL].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: total time not found in telegram");
		return;
	}
	if (data[RESET_COUNT].empty() == true)
	{
		_log.Log(LOG_ERROR, "Ec3kMeter: reset count not found in telegram");
		return;
	}

	// extract values from json

	std::stringstream ssId;
	ssId << std::hex << root[SENSOR_ID].asString();
	//int id = root[SENSOR_ID].asInt();
	int id;
	ssId >> id;

	// update only when the update interval has elapsed 
	if (m_limiter->update(id))
	{
		int ws = data[WS].asInt();
		float w_current = data[W_CURRENT].asFloat();
		float w_max = data[W_MAX].asFloat();
		//int s_time_on = data[TIME_ON].asInt();
		//int s_time_total = data[TIME_TOTAL].asInt();
		//int reset_count = data[RESET_COUNT].asInt();

		// create suitable default names and send data

		std::stringstream sensorNameKwhSS;
		sensorNameKwhSS << "EC3K meter " << std::hex << id << " Usage";
		const std::string sensorNameKwh = sensorNameKwhSS.str();
		SendKwhMeter(id, 1, 255, w_current, (double)ws / 3600 / 1000, sensorNameKwh);

		std::stringstream sensorNameWMaxSS;
		sensorNameWMaxSS << "EC3K meter " << std::hex << id << " maximum";
		const std::string sensorNameWMax = sensorNameWMaxSS.str();
		SendWattMeter(id, 2, 255, w_max, sensorNameWMax);

		// TODO: send times + reset_count?
	}
}

Ec3kLimiter::Ec3kLimiter(void)
{
  no_meters = 0;
}

Ec3kLimiter::~Ec3kLimiter(void)
{
}

bool Ec3kLimiter::update(int id)
{
	int i;
	for (i = 0; i < no_meters; i++)
	{
		if (meters[i].id == id)
		{
			// Allow update after at least update interval  seconds
			if ((time(NULL) - meters[i].last_update) >= EC3K_UPDATE_INTERVAL)
			{
				meters[i].last_update = time(NULL);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	// Store new meter and allow update
	meters[no_meters].id = id;
	meters[no_meters].last_update = time(NULL);
	no_meters++;
	return true;
}

