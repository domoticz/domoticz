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

void removeCharsFromString(std::string &str, const char* charsToRemove) {
	for (unsigned int i = 0; i < strlen(charsToRemove); ++i) {
		str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end());
	}
}

CRtl433::CRtl433(const int ID, const std::string &cmdline) :
	m_cmdline(cmdline)
{
	// Basic protection from malicious command line
	removeCharsFromString(m_cmdline, ":;/$()`<>|&");
	m_HwdID = ID;
	m_hPipe = NULL;
	m_time_last_received = 0;
/*
	#ifdef _DEBUG
		std::string headerline = "time,msg,codes,model,button,id,channel,battery,temperature_C,mic,subtype,rid,humidity,state,status,brand,rain_rate,rain_rate_mm_h,rain_rate_in_h,rain_total,rain_mm,rain_in,gust,average,direction,wind_max_m_s,wind_avg_m_s,wind_dir_deg,pressure_hPa,uv,power_W,energy_kWh,unit,group_call,command,dim,dim_value,wind_speed,wind_gust,wind_direction,wind_avg_km_h,wind_max_km_h,dipswitch,rbutton,device,temperature_F,battery_ok,setpoint_C,switch,cmd,cmd_id,tristate,direction_deg,speed,rain,msg_type,signal,radio_clock,sensor_code,uv_status,uv_index,lux,wm,seq,rainfall_mm,wind_speed_ms,gust_speed_ms,current,interval,learn,sensor_id,battery_low,sequence_num,message_type,wind_speed_mph,wind_speed_kph,wind_avg_mi_h,rain_inch,rc,gust_speed_mph,wind_max_mi_h,wind_approach,flags,maybetemp,binding_countdown,depth,depth_cm,dev_id,power0,power1,power2,node,ct1,ct2,ct3,ct4,Vrms/batt,batt_Vrms,temp1_C,temp2_C,temp3_C,temp4_C,temp5_C,temp6_C,pulse,address,button1,button2,button3,button4,data,sid,group,transmit,moisture,type,pressure_PSI,battery_mV,pressure_kPa,pulses,energy,len,to,from,payload,event,heartbeat,temperature1_C,temperature2_C,temperature_1_C,temperature_2_C,test,probe,water,humidity_1,ptemperature_C,phumidity,newbattery,heating,heating_temp,uvi,light_lux,pm2_5_ug_m3,pm10_ug_m3,counter,code,alarm,repeat,maybe_battery,device_type,raw_message,switch1,switch2,switch3,switch4,switch5,extradata,house_id,module_id,sensor_type,sensor_count,alarms,sensor_value,battery_voltage,failed,class,alert,secret_knock,relay,wind_dev_deg,exposure_mins,transmit_s,device_id,button_id,button_name";
		std::vector<std::string> headers = ParseCSVLine(headerline.c_str());
		std::string line = "2019-07-03 16:22:59,,,LaCrosse WS,,32,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,909.090,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
		ParseLine(headers, line.c_str());
	#endif
*/
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

bool CRtl433::FindField(const std::map<std::string, std::string> &data, const std::string &field)
{
	return (data.find(field) != data.end());
}

bool CRtl433::ParseLine(const std::vector<std::string> &headers, const char *line)
{
	time_t atime = time(NULL);
	std::string slineRaw(line);
	if (slineRaw.find(',') != std::string::npos)
	{
		slineRaw = slineRaw.substr(slineRaw.find(',') + 1);
		if (slineRaw == m_sLastLine)
		{
			if (atime - m_time_last_received < 2)
				return true; //skip duplicate RF frames
		}
		m_sLastLine = slineRaw;
	}
	m_time_last_received = atime;
	std::vector<std::string> values = ParseCSVLine(line);

	if (values.size() != headers.size())
		return false; //should be equal

	// load values into a map
	std::map<std::string, std::string> data;
	std::vector<std::string>::const_iterator h = headers.begin();
	for (std::vector<std::string>::iterator vi = values.begin(); vi != values.end(); ++vi)
	{
		if (!(*vi).empty())
			data[*(h)] = *vi;
		h++;
	}
	int id = 0;

	bool haveUnit = false;
	int unit = 0;

	bool haveChannel = false;
	int channel = 0;

	bool haveBattery = false;
	int batterylevel = 255;

	bool haveTemp = false;
	float tempC = 0;

	bool haveHumidity = false;
	int humidity = 0;

	bool havePressure = false;
	float pressure = 0;

	bool haveRain = false;
	float rain = 0;

	bool haveDepth_CM = false;
	float depth_cm = 0;

	bool haveDepth = false;
	float depth = 0;

	bool haveWind_Strength = false;
	float wind_strength = 0;

	bool haveWind_Gust = false;
	float wind_gust = 0;

	bool haveWind_Dir = false;
	int wind_dir = 0;

	bool haveMoisture = false;
	int moisture = 0;

	bool havePower = false;
	float power = 0;

	if (!data["id"].empty())
	{
		id = atoi(data["id"].c_str());
	}
	else if (!data["rc"].empty())
	{
		id = atoi(data["rc"].c_str());
	}


	if (FindField(data, "unit"))
	{
		unit = atoi(data["unit"].c_str());
		haveUnit = true;
	}
	if (FindField(data, "channel"))
	{
		channel = atoi(data["channel"].c_str());
		haveChannel = true;
	}
	if (FindField(data, "battery"))
	{
		if (data["battery"] == "LOW") {
			batterylevel = 10;
			haveBattery = true;
		}
		else if (data["battery"] == "OK") {
			batterylevel = 100;
			haveBattery = true;
		}
	}

	if (FindField(data, "temperature_C"))
	{
		tempC = (float)atof(data["temperature_C"].c_str());
		haveTemp = true;
	}
	else if (FindField(data, "temperature_F"))
	{
		tempC = (float)ConvertToCelsius(atof(data["temperature_F"].c_str()));
		haveTemp = true;
	}

	if (FindField(data, "humidity"))
	{
		if (data["humidity"] == "HH")
		{
			humidity = 90;
			haveHumidity = true;
		}
		else if (data["humidity"] == "LL")
		{
			humidity = 10;
			haveHumidity = true;
		}
		else
		{
			humidity = atoi(data["humidity"].c_str());
			haveHumidity = true;
		}
	}

	if (FindField(data, "pressure_hPa"))
	{
		pressure = (float)atof(data["pressure_hPa"].c_str());
		havePressure = true;
	}

	if (FindField(data, "rain"))
	{
		rain = (float)atof(data["rain"].c_str());
		haveRain = true;
	}

	if (FindField(data, "rainfall_mm"))
	{
		rain = (float)atof(data["rainfall_mm"].c_str());
		haveRain = true;
	}

	if (FindField(data, "rain_total"))
	{
		rain = (float)atof(data["rain_total"].c_str());
		haveRain = true;
	}

	if (FindField(data, "depth_cm"))
	{
		depth_cm = (float)atof(data["depth_cm"].c_str());
		haveDepth_CM = true;
	}

	if (FindField(data, "depth"))
	{
		depth = (float)atof(data["depth"].c_str());
		haveDepth = true;
	}

	if (FindField(data, "windstrength") || FindField(data, "wind_speed"))
	{
		//Based on current knowledge it's not possible to have both wind strength and wind_speed at the same time.
		if (FindField(data, "windstrength"))
		{
			wind_strength = (float)atof(data["windstrength"].c_str());
		}
		else if (FindField(data, "wind_speed"))
		{
			wind_strength = (float)atof(data["wind_speed"].c_str());
		}
		haveWind_Strength = true;
	}
	else if (FindField(data, "average"))
	{
		wind_strength = (float)atof(data["average"].c_str());
		haveWind_Strength = true;
	}

	if (FindField(data, "winddirection") || FindField(data, "wind_direction"))
	{
		//Based on current knowledge it's not possible to have both wind direction and wind_direction at the same time.
		if (FindField(data, "winddirection"))
		{
			wind_dir = atoi(data["winddirection"].c_str());
		}
		else if (FindField(data, "wind_direction"))
		{
			wind_dir = atoi(data["wind_direction"].c_str());
		}
		haveWind_Dir = true;
	}
	else if (FindField(data, "direction"))
	{
		wind_dir = atoi(data["direction"].c_str());
		haveWind_Dir = true;
	}

	if (FindField(data, "wind_gust"))
	{
		wind_gust = (float)atof(data["wind_gust"].c_str());
		haveWind_Gust = true;
	}
	else if (FindField(data, "gust"))
	{
		wind_gust = (float)atof(data["gust"].c_str());
		haveWind_Gust = true;
	}
	else if (FindField(data, "moisture"))
	{
		moisture = atoi(data["moisture"].c_str());
		haveMoisture = true;
	}
	else if (FindField(data, "power_W"))
	{
		power = (float)atof(data["power_W"].c_str());
		havePower = true;
	}

	std::string model = data["model"];

	bool hasstate = FindField(data, "state") || FindField(data, "command");

	if (hasstate)
	{
		bool state = false;
		if (FindField(data, "state"))
			state = data["state"] == "ON";
		else if (FindField(data, "command"))
			state = data["command"] == "On";
		unsigned int switchidx = (id & 0xfffffff) | ((channel & 0xf) << 28);
		SendSwitch(switchidx,
			unit,
			batterylevel,
			state,
			0,
			model);
		return true;
	}

	unsigned int sensoridx = (id & 0xff) | ((channel & 0xff) << 8);

	if (haveTemp && haveHumidity)
	{
		//Check if it is valid
		bool bValidTempHum = !((tempC == 0) && (humidity == 0));
		if (!bValidTempHum)
			return false; //invalid temp+hum
	}

	bool bHandled = false;
	if (haveTemp && haveHumidity && havePressure)
	{
		int iForecast = 0;
		SendTempHumBaroSensor(sensoridx, batterylevel, tempC, humidity, pressure, iForecast, model);
		bHandled = true;
	}
	else if (haveTemp && haveHumidity)
	{
		SendTempHumSensor(sensoridx, batterylevel, tempC, humidity, model);
		bHandled = true;
	}
	else 
	{
		if (haveTemp)
		{
			SendTempSensor(sensoridx, batterylevel, tempC, model);
			bHandled = true;
		}
		if (haveHumidity)
		{
			SendHumiditySensor(sensoridx, batterylevel, humidity, model);
			bHandled = true;
		}
	}
	if (haveWind_Strength || haveWind_Gust || haveWind_Dir)
	{
		SendWind(sensoridx, batterylevel, wind_dir, wind_strength, wind_gust, tempC, 0, haveTemp, false, model);
		bHandled = true;
	}
	if (haveRain)
	{
		SendRainSensor(sensoridx, batterylevel, rain, model);
		bHandled = true;
	}
	if (haveDepth_CM)
	{
		SendDistanceSensor(sensoridx, unit, batterylevel, depth_cm, model);
		bHandled = true;
	}
	if (haveDepth)
	{
		SendDistanceSensor(sensoridx, unit, batterylevel, depth, model);
		bHandled = true;
	}
	if (haveMoisture)
	{
		SendMoistureSensor(sensoridx, batterylevel, moisture, model);
		bHandled = true;
	}
	if (havePower)
	{
		SendWattMeter(sensoridx, unit, batterylevel, power, model);
		bHandled = true;
	}

	return bHandled; //not handled (Yet!)
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
		std::string headerLine = "";
		m_sLastLine = "";

		std::string szFlags = "-F csv " + m_cmdline; // -f 433.92e6 -f 868.24e6 -H 60 -d 0
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
		m_time_last_received = time(NULL);
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
					headerLine = line;
					headers = ParseCSVLine(line);
					continue;
				}
				if (!ParseLine(headers, line))
				{
					// this is also logged when parsed data is invalid
					_log.Log(LOG_STATUS, "Rtl433: Unhandled sensor reading, please report: (%s|%s)", headerLine.c_str(), line);
				}
			}
			else { //fgets
				if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
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
