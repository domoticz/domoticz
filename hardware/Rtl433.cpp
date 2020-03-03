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
	Registered 135 out of 141 device decoding protocols [ 1-4 6-8 10-17 19-26 29-64 67-141 ]
	std::string headerline = time,msg,codes,model,button,id,channel,battery_ok,temperature_C,mic,subtype,rid,humidity,state,status,brand,rain_rate,rain_rate_mm_h,rain_total,rain_mm,gust,average,direction,wind_max_m_s,wind_avg_m_s,wind_dir_deg,pressure_hPa,uv,power_W,energy_kWh,radio_clock,sequence,unit,group_call,command,dim,dim_value,wind_speed,wind_gust,wind_direction,wind_avg_km_h,wind_max_km_h,dipswitch,rbutton,device,setpoint_C,switch,cmd,cmd_id,tristate,direction_deg,speed,rain,msg_type,signal,sensor_code,uv_status,uv_index,lux,wm,seq,rainfall_mm,wind_speed_ms,gust_speed_ms,current,interval,learn,message_type,sensor_id,sequence_num,battery_low,wind_speed_mph,wind_speed_kph,rain_inch,rc,gust_speed_mph,wind_approach,flags,maybetemp,binding_countdown,depth,depth_cm,dev_id,sensor_type,power0,power1,power2,node,ct1,ct2,ct3,ct4,Vrms/batt,batt_Vrms,temp1_C,temp2_C,temp3_C,temp4_C,temp5_C,temp6_C,pulse,address,button1,button2,button3,button4,data,sid,group,transmit,moisture,type,pressure_kPa,battery_mV,pulses,pulsecount,energy,len,to,from,payload,event,alarm,tamper,heartbeat,temperature1_C,temperature2_C,temperature_1_C,temperature_2_C,test,probe,water,humidity_1,ptemperature_C,phumidity,newbattery,heating,heating_temp,uvi,light_lux,pm2_5_ug_m3,pm10_0_ug_m3,counter,code,repeat,maybe_battery,device_type,raw_message,switch1,switch2,switch3,switch4,switch5,extradata,house_id,module_id,sensor_count,alarms,sensor_value,battery_voltage,mode,version,type_string,failed,class,alert,secret_knock,relay,wind_dev_deg,exposure_mins,transmit_s,button_id,button_name,encrypted,misc,current_A,voltage_V,pairing,connected,gap,impulses,triggered,storage
	std::vector<std::string> headers = ParseCSVLine(headerline.c_str());
	std::string line = "2020-02-10 17:15:14,,,Oregon-THN132N,,183,1,1,20.800,,,,,,,OS,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,";
	std::string line = "2020-02-10 17:15:14,,,Oregon-THN132N,,183,1,1,20.800,,,,,,,OS,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
	std::string line = "2020-02-10 17:15:15,,,Oregon-CM180,,47856,,1,,,,,,,,OS,,,,,,,,,,,,,982,,,2,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
	std::string line = "2020-02-10 17:15:21,,,Oregon-RTGR328N,,241,3,0,17.500,,,,33,,,OS,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
	std::string line = "2020-02-10 17:15:27,,,Oregon-CM180,,47856,,1,,,,,,,,OS,,,,,,,,,,,,,1062,,,1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
	std::string line = "2020-02-10 17:15:32,,,Nexus-TH,,3,3,1,14.600,,,,54,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
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

	bool haveDepth = false;
	float depth = 0;

	bool haveWind_Speed = false;
	float wind_speed = 0;

	bool haveWind_Gust = false;
	float wind_gust = 0;

	bool haveWind_Dir = false;
	int wind_dir = 0;

	bool haveMoisture = false;
	int moisture = 0;

	bool havePower = false;
	float power = 0;

	bool haveEnergy = false;
	float energy = 0;

	bool haveSequence = false;
	int sequence = 0;

	if (!data["id"].empty())
	{
		id = atoi(data["id"].c_str());
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
	if (FindField(data, "battery_ok"))
	{
		if (data["battery_ok"] == "0") {
			batterylevel = 10;
			haveBattery = true;
		}
		else if (data["battery_ok"] == "1") {
			batterylevel = 100;
			haveBattery = true;
		}
	}

	if (FindField(data, "temperature_C"))
	{
		tempC = (float)atof(data["temperature_C"].c_str());
		haveTemp = true;
	}

	if (FindField(data, "humidity"))
	{
		if (data["humidity"] == "HH") // "HH" and "LL" are specific to WT_GT-02 and WT-GT-03 see issue 1996
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
	if (FindField(data, "pressure_kPa"))
	{
		pressure = 10.0f*(float)atof(data["pressure_kPa"].c_str()); // convert to hPA
		havePressure = true;
	}

	if (FindField(data, "rain_mm"))
	{
		rain = (float)atof(data["rain_mm"].c_str());
		haveRain = true;
	}

	if (FindField(data, "depth_cm"))
	{
		depth = (float)atof(data["depth_cm"].c_str());
		haveDepth = true;
	}

	if (FindField(data, "wind_avg_km_h")) // wind speed average (converting into m/s note that internal storage if 10.0f*m/s) 
	{
		wind_speed = ((float)atof(data["wind_avg_km_h"].c_str()))/3.6f;
		haveWind_Speed = true;
	}


	if (FindField(data, "wind_dir_deg"))
	{
		wind_dir = atoi(data["wind_dir_deg"].c_str()); // does domoticz assume it is degree ? (and not rad or something else)
		haveWind_Dir = true;
	}

	if (FindField(data, "wind_max_km_h")) // idem, converting to m/s
	{
		wind_gust = ((float)atof(data["wind_max_km_h"].c_str()))/3.6f;
		haveWind_Gust = true;
	}
        else if (FindField(data, "moisture"))
	{
		moisture = atoi(data["moisture"].c_str());
		haveMoisture = true;
	}
	else {
		if (FindField(data, "power_W")) // -- power_W,energy_kWh,radio_clock,sequence,
		{
			power = (float)atof(data["power_W"].c_str());
			havePower = true;
		}
		if (FindField(data, "energy_kWh")) // sensor type general subtype electric counter
		{
			energy = (float)atof(data["energy_kWh"].c_str());
			haveEnergy = true;
		}
		if (FindField(data, "sequence")) // please do not remove : to be added in future PR for data in sensor (for fiability reporting)
		{
			sequence = atoi(data["sequence"].c_str());
			haveSequence = true;
		}
	}

	std::string model = data["model"]; // new model format normalized from the 201 different devices presently supported by rtl_433

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
			(const uint8_t)unit,
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
	if (haveWind_Speed || haveWind_Gust || haveWind_Dir)
	{
		SendWind(sensoridx, batterylevel, wind_dir, wind_speed, wind_gust, tempC, 0, haveTemp, false, model);
		bHandled = true;
	}
	if (haveRain)
	{
		SendRainSensor(sensoridx, batterylevel, rain, model);
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
		SendWattMeter((uint8_t)sensoridx, (uint8_t)unit, batterylevel, power, model);
		bHandled = true;
	}
	if (haveEnergy && havePower)
	{
		//can remove this comment : _log.Log(LOG_STATUS, "Rtl433: : CM180 haveSequence(%d) sensoridx(%d) havePower(%d) haveEnergy(%d))", haveSequence, sensoridx, havePower, haveEnergy);
		sensoridx = sensoridx + 1;
		//can rmeove this comment : _log.Log(LOG_STATUS, "Rtl433: : CM180 sensoridx(%d) unit(%d) batterylevel(%d) power(%f) energy(%f) model(%s)", sensoridx, unit, batterylevel, power, energy, model.c_str());
		SendKwhMeter(sensoridx, unit, batterylevel, power, energy, model);
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

		std::string szFlags = "-F csv -M newmodel -C si " + m_cmdline; // newmodel used (-M newmodel) and international system used (-C si) -f 433.92e6 -f 868.24e6 -H 60 -d 0
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

bool CRtl433::WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/)
{
	//const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	return false;
}
