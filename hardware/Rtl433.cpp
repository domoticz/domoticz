#include "stdafx.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

#include <boost/lexical_cast.hpp>

#include "Rtl433.h"

#define round(a) ( int ) ( a + .5 )

void removeCharsFromString(std::string &str, const char* charsToRemove ) {
   for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}

CRtl433::CRtl433(const int ID, const std::string &cmdline) :
	m_stoprequested(false),
	m_cmdline(cmdline)
{
	// Basic protection from malicious command line
	removeCharsFromString(m_cmdline, ":;/$()`<>|&");
	m_HwdID = ID;
	m_hPipe = NULL;
}

CRtl433::~CRtl433()
{
}

bool CRtl433::StartHardware()
{
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CRtl433::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	StartHeartbeatThread();
	return true;
}

bool CRtl433::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		if (m_hPipe)
		{
			boost::lock_guard<boost::mutex> l(m_pipe_mutex);
#ifdef WIN32
			_pclose(m_hPipe);
#else
			pclose(m_hPipe);
#endif
			m_hPipe = NULL;
		}
		m_thread->join();
	}

	m_bIsStarted = false;
	StopHeartbeatThread();
	return true;
}

std::vector<std::string> CRtl433::ParseCSVLine(const char *input)
{
	std::vector<std::string> line;
	std::string field;
	const char *s = input;
	while (*s)
	{
		if (*s == '\\')
		{
			s++;
			if (*s) {
				field += *s;
			}
		}
		else if (*s == ',')
		{
			line.push_back(field);
			field.clear();
		}
		else
		{
			field += *s;
		}
		s++;
	}
	return line;
}

void CRtl433::Do_Work()
{
	sleep_milliseconds(1000);
	_log.Log(LOG_STATUS, "Rtl433: Worker started... (%s)",m_cmdline.c_str());

	bool bHaveReceivedData = false;
	while (!m_stoprequested)
	{
		char line[2048];
		std::vector<std::string> headers;
		std::string sLastLine = "";

		std::string szFlags = "-F csv -q -I 2 " + m_cmdline; // -f 433.92e6 -f 868.24e6 -H 60 -d 0
#ifdef WIN32
		std::string szCommand = "C:\\rtl_433.exe " + szFlags;
		m_hPipe = _popen(szCommand.c_str(), "r");
#else
		std::string szCommand = "rtl_433 " + szFlags + " 2>/dev/null";
		m_hPipe = popen(szCommand.c_str(), "r");
#endif
		if (m_hPipe == NULL)
		{
			if (!m_stoprequested) {
				// sleep 30 seconds before retrying
#ifdef WIN32
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed. (%s)  https://cognito.me.uk/computers/rtl_433-windows-binary-32-bit)",szCommand.c_str());
#else
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed (%s). https://github.com/merbanan/rtl_433",szCommand.c_str());
#endif
				for (int i = 0; i < 30 && !m_stoprequested; i++) {
					sleep_milliseconds(1000);
				}
			}
			continue;
		}

		bool bFirstTime = true;
		time_t time_last_received = time(NULL);
		while (!m_stoprequested)
		{
			sleep_milliseconds(100);
			boost::lock_guard<boost::mutex> l(m_pipe_mutex);
			if (m_hPipe == NULL)
				continue;
			line[0] = 0;
			if (fgets(line, sizeof(line) - 1, m_hPipe) != NULL)
			{
				bHaveReceivedData = true;

				if (bFirstTime)
				{
					bFirstTime = false;
					headers = ParseCSVLine(line);
					continue;
				}
				time_t atime = time(NULL);
				std::string slineRaw(line);
				if (slineRaw.find(',') != std::string::npos)
				{
					slineRaw = slineRaw.substr(slineRaw.find(',') + 1);
					if (slineRaw == sLastLine)
					{
						if (atime - time_last_received < 2)
							continue; //skip duplicate RF frames
					}
					sLastLine = slineRaw;
				}
				time_last_received = atime;
				std::vector<std::string> values = ParseCSVLine(line);
				// load field values into a map
				std::map<std::string, std::string> data;
				std::vector<std::string>::iterator h = headers.begin();
				for (std::vector<std::string>::iterator vi = values.begin(); vi != values.end(); vi++)
				{
					std::string header = *(h++);
					data[header] = *vi;
				}
				int id = 0;
				bool hasid = false;
				int unit = 0;
				bool hasunit = false;
				int channel = 0;
				bool haschannel = false;
				int batterylevel = 255;
				bool hasbattery = false;
				float tempC;
				bool hastempC = false;
				int humidity;
				bool hashumidity = false;
				float pressure;
				bool haspressure = false;
				float rain;
				bool hasrain = false;

				// attempt parsing field values
				try {
					if (!data["id"].empty()) {
						id = boost::lexical_cast<int>(data["id"]);
						hasid = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}
				try {
					if (!data["unit"].empty())
					{
						unit = boost::lexical_cast<int>(data["unit"]);
						hasunit = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}
				try {
					if (!data["channel"].empty())
					{
						channel = boost::lexical_cast<int>(data["channel"]);
						haschannel = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}
				if (!data["battery"].empty())
				{
					if (data["battery"] == "LOW") {
						batterylevel = 10;
						hasbattery = true;
					}
					if (data["battery"] == "OK") {
						batterylevel = 100;
						hasbattery = true;
					}
				}
				try {
					if (!data["temperature_C"].empty())
					{
						tempC = boost::lexical_cast<float>(data["temperature_C"]);
						hastempC = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}
				try {
					if (!data["humidity"].empty())
					{
						humidity = round(boost::lexical_cast<float>(data["humidity"]));
						hashumidity = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
						_log.Log(LOG_STATUS, "Rtl433: bad cast -- humidity");
				}
				try {
					if (!data["pressure"].empty())
					{
						pressure = boost::lexical_cast<float>(data["pressure"]);
						haspressure = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}
				try {
					if (!data["rain"].empty())
					{
						rain = boost::lexical_cast<float>(data["rain"]);
						hasrain = true;
					}
				}
				catch (boost::bad_lexical_cast e) {
				}

				std::string model = data["model"];

				bool hasstate =
					(!data["state"].empty()) ||
					(!data["command"].empty());
				bool state =
					data["state"] == "ON" ||
					data["command"] == "On";

				if (hasstate)
				{
					unsigned int switchidx = (id & 0xfffffff) | ((channel & 0xf) << 28);
					SendSwitch(switchidx,
						unit,
						batterylevel,
						state,
						0,
						model);
					continue;
				}

				unsigned int sensoridx = (id & 0xff) | ((channel & 0xff) << 8);
				bool bHaveSend = false;
				if (hastempC && hashumidity && haspressure)
				{
					int iForecast = 0;
					SendTempHumBaroSensor(sensoridx,
						batterylevel,
						tempC,
						humidity,
						pressure,
						iForecast,
						model);
					bHaveSend = true;
				}
				else if (hastempC && hashumidity)
				{
					SendTempHumSensor(sensoridx,
						batterylevel,
						tempC,
						humidity,
						model);
					bHaveSend = true;
				}
				else if (hastempC && !hashumidity)
				{
					SendTempSensor(sensoridx,
						batterylevel,
						tempC,
						model);
					bHaveSend = true;
				}
				else if (!hastempC && hashumidity)
				{
					SendHumiditySensor(sensoridx,
						batterylevel,
						humidity,
						model);
					bHaveSend = true;
				}

				if (hasrain)
				{
					SendRainSensor(sensoridx,
						batterylevel,
						rain,
						model);
					bHaveSend = true;
				}

				if (!bHaveSend)
				{
					_log.Log(LOG_STATUS, "Rtl433: Unhandled sensor type, please report: (%s)", line);
				}
			} else { //fgets
			  break; // bail out, subprocess has failed
			}
		} // while !m_stoprequested
		if (m_hPipe)
		{
#ifdef WIN32
			_pclose(m_hPipe);
#else
			pclose(m_hPipe);
#endif
			m_hPipe = NULL;
		}
		if (!m_stoprequested) {
			// sleep 30 seconds before retrying
			if (!bHaveReceivedData)
			{
#ifdef WIN32
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed. (%s)  https://cognito.me.uk/computers/rtl_433-windows-binary-32-bit)",szCommand.c_str());
#else
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed (%s). https://github.com/merbanan/rtl_433",szCommand.c_str());
#endif
			}
			else
			{
				_log.Log(LOG_STATUS, "Rtl433: Failure! Retrying in 30 seconds...");
			}
			for (int i = 0; i < 30 && !m_stoprequested; i++) {
				sleep_milliseconds(1000);
			}
		}
	} // while !stoprequested
	_log.Log(LOG_STATUS, "Rtl433: Worker stopped...");
}

bool CRtl433::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	return false;
}
