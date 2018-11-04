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
#include "Rtl433.h"

void removeCharsFromString(std::string &str, const char* charsToRemove ) {
   for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
      str.erase( remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
   }
}

CRtl433::CRtl433(const int ID, const std::string &cmdline) :
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
	RequestStart();

	m_thread = std::make_shared<std::thread>(&CRtl433::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	StartHeartbeatThread();
	return true;
}

bool CRtl433::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
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
	if (!m_cmdline.empty())
		_log.Log(LOG_STATUS, "Rtl433: Worker started... (Extra Arguments: %s)", m_cmdline.c_str());
	else
		_log.Log(LOG_STATUS, "Rtl433: Worker started...");

	bool bHaveReceivedData = false;
	while (!IsStopRequested(0))
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
			if (!IsStopRequested(0))
			{
				// sleep 30 seconds before retrying
#ifdef WIN32
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed. (%s)  https://cognito.me.uk/computers/rtl_433-windows-binary-32-bit)", szCommand.c_str());
#else
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed (%s). https://github.com/merbanan/rtl_433", szCommand.c_str());
#endif
				for (int i = 0; i < 30; i++)
				{
					if (IsStopRequested(1000))
						break;
				}
			}
			continue;
		}
#ifndef WIN32
		//Set to non-blocking mode
		int fd = fileno(m_hPipe);
		int flags;
		flags = fcntl(fd, F_GETFL, 0);
		flags |= O_NONBLOCK;
		fcntl(fd, F_SETFL, flags);
#endif
		bool bFirstTime = true;
		time_t time_last_received = time(NULL);
		while (!IsStopRequested(100))
		{
			if (m_hPipe == NULL)
				break;
			//size_t bread = read(fd, (char*)&line, sizeof(line));
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
				for (std::vector<std::string>::iterator vi = values.begin(); vi != values.end(); ++vi)
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
				float depth_cm;
				bool hasdepth_cm = false;
				float depth;
				bool hasdepth = false;
				float wind_str;
				bool haswind_str = false;
				float wind_gst;
				bool haswind_gst = false;
				int wind_dir;
				bool haswind_dir = false;
				// attempt parsing field values
				//atoi/f functions return 0 if string conv fails.

				if (!data["id"].empty()) {
					id = atoi(data["id"].c_str());
					hasid = true;
				}


				if (!data["unit"].empty())
				{
					unit = atoi(data["unit"].c_str());
					hasunit = true;
				}


				if (!data["channel"].empty())
				{
					channel = atoi(data["channel"].c_str());
					haschannel = true;
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

				if (!data["temperature_C"].empty())
				{
					tempC = (float)atof(data["temperature_C"].c_str());
					hastempC = true;
				}

				if (!data["humidity"].empty())
 				{
					if (data["humidity"] == "HH")
					{
						humidity = 90;
						hashumidity = true;
					}
					else if (data["humidity"] == "LL")
					{
						humidity = 10;
						hashumidity = true;
					}
					else
					{
						humidity = atoi(data["humidity"].c_str());
						hashumidity = true;
					}
				}

				if (!data["pressure_hPa"].empty())
				{
					pressure = (float)atof(data["pressure_hPa"].c_str());
					haspressure = true;
				}

				if (!data["rain"].empty())
				{
					rain = (float)atof(data["rain"].c_str());
					hasrain = true;
				}
				if (!data["depth_cm"].empty())
				{
					depth_cm = (float)atof(data["depth_cm"].c_str());
					hasdepth_cm = true;
				}

				if (!data["depth"].empty())
				{
					depth = (float)atof(data["depth"].c_str());
					hasdepth = true;
				}

				if (!data["windstrength"].empty() || !data["wind_speed"].empty())
				{
					//Based on current knowledge it's not possible to have both windstrength and wind_speed at the same time.
					if (!data["windstrength"].empty())
					{
						wind_str = (float)atof(data["windstrength"].c_str());
					}
					else if (!data["wind_speed"].empty())
					{
						wind_str = (float)atof(data["wind_speed"].c_str());
					}
					haswind_str = true;
				}

				if (!data["winddirection"].empty() || !data["wind_direction"].empty())
				{
					//Based on current knowledge it's not possible to have both winddirection and wind_direction at the same time.
					if (!data["winddirection"].empty())
					{
						wind_dir = atoi(data["winddirection"].c_str());
					}
					else if (!data["wind_direction"].empty())
					{
						wind_dir = atoi(data["wind_direction"].c_str());
					}
					haswind_dir = true;
				}

				if (!data["wind_gust"].empty())
				{
					wind_gst = (float)atof(data["wind_gust"].c_str());
					haswind_gst = true;
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

				bool bValidTempHum = false;
				if (hastempC && hashumidity)
				{
					bValidTempHum = !((tempC == 0) && (humidity == 0));
				}

				bool bHaveSend = false;
				if (hastempC && hashumidity && haspressure && bValidTempHum)
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
				else if (haswind_str && haswind_dir && !haswind_gst && hastempC)
				{
					SendWind(sensoridx,
						batterylevel,
						wind_dir,
						wind_str,
						0,
						tempC,
						0,
						true,
						model);
					bHaveSend = true;
				}
				else if (haswind_str && haswind_dir && !haswind_gst && !hastempC)
				{
					SendWind(sensoridx,
						batterylevel,
						wind_dir,
						wind_str,
						0,
						0,
						0,
						false,
						model);
					bHaveSend = true;
				}
				else if (haswind_str && haswind_gst && haswind_dir && hastempC)
				{
					SendWind(sensoridx,
						batterylevel,
						wind_dir,
						wind_str,
						wind_gst,
						tempC,
						0,
						true,
						model);
					bHaveSend = true;
				}
				else if (haswind_str && haswind_gst && haswind_dir && !hastempC)
				{
					SendWind(sensoridx,
						batterylevel,
						wind_dir,
						wind_str,
						wind_gst,
						0,
						0,
						false,
						model);
					bHaveSend = true;
				}
				else if (hastempC && hashumidity && bValidTempHum)
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
				if (hasdepth_cm)
				{
					SendDistanceSensor(sensoridx, unit,
						batterylevel, depth_cm, model);
					bHaveSend = true;
				}
				if (hasdepth)
				{
					SendDistanceSensor(sensoridx, unit,
						batterylevel, depth, model);
					bHaveSend = true;
				}

				if (!bHaveSend)
				{
					// this is also logged when parsed data is invalid
					_log.Log(LOG_STATUS, "Rtl433: Unhandled sensor reading, please report: (%s)", line);
				}
				else
				{
					//Useful as some sensors will be skipped if temp is available
					//_log.Log(LOG_NORM, "Rtl433: Raw Data: (%s)", line);
				}
			}
			else { //fgets
				if ((errno == EWOULDBLOCK)|| (errno == EAGAIN)) {
					continue;
				}
				break; // bail out, subprocess has failed
			}
		} // while !IsStopRequested()
		if (m_hPipe)
		{
#ifdef WIN32
			_pclose(m_hPipe);
#else
			pclose(m_hPipe);
#endif
			m_hPipe = NULL;
		}
		if (!IsStopRequested(0)) {
			// sleep 30 seconds before retrying
			if (!bHaveReceivedData)
			{
#ifdef WIN32
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed. (%s)  https://cognito.me.uk/computers/rtl_433-windows-binary-32-bit)", szCommand.c_str());
#else
				_log.Log(LOG_STATUS, "Rtl433: rtl_433 startup failed. Make sure it's properly installed (%s). https://github.com/merbanan/rtl_433", szCommand.c_str());
#endif
			}
			else
			{
				_log.Log(LOG_STATUS, "Rtl433: Failure! Retrying in 30 seconds...");
			}
			for (int i = 0; i < 30; i++)
			{
				if (IsStopRequested(1000))
					break;
			}
		}
	} // while !IsStopRequested()
	_log.Log(LOG_STATUS, "Rtl433: Worker stopped...");
}

bool CRtl433::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	return false;
}
