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

Rtl433::Rtl433(const int ID) :
	m_stoprequested(false)
{
	m_HwdID = ID;

}

Rtl433::~Rtl433()
{
}


bool Rtl433::StartHardware()
{
	m_threadReceive = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&Rtl433::ReceiveThread, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	StartHeartbeatThread();
	return true;
}

bool Rtl433::StopHardware()
{
	if (m_threadReceive)
		{
			m_stoprequested = true;
			m_threadReceive->join();
		}

	m_bIsStarted=false;
	StopHeartbeatThread();
	return true;
}

std::vector<std::string> Rtl433::ParseCSVLine(const char *input)
{
	std::vector<std::string> line;
	std::string field;
	const char *s = input;
	while(*s) {
		if(*s == '\\') {
			s++;
			if(*s) {
				field += *s;
			}
		} else if (*s == ',') {
			line.push_back(field);
			field.clear();
		} else {
			field += *s;
		}
		s++;
	}
	return line;
}

void Rtl433::ReceiveThread()
{
	sleep_milliseconds(1000);
	while(!m_stoprequested) {
		char line[1024];
		FILE *pipe = popen("rtl_433 -F csv -q 2>/dev/null", "r");
		if(fgets(line,sizeof(line)-1,pipe)) {
			std::vector<std::string> headers = ParseCSVLine(line);

			while(!m_stoprequested && // suspicious pattern from 1Wire driver, but seems to work
			      fgets(line,sizeof(line)-1,pipe)) {
				std::vector<std::string> values = ParseCSVLine(line);
				// load field values into a map
				std::map<std::string,std::string> data;
				std::vector<std::string>::iterator h = headers.begin();
				for(std::vector<std::string>::iterator vi = values.begin(); vi != values.end(); vi++) {
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
				// attempt parsing field values
				try {
					id = boost::lexical_cast<int>(data["id"]);
					hasid = true;
				} catch (boost::bad_lexical_cast e) {
				}
				try {
					unit = boost::lexical_cast<int>(data["unit"]);
					hasunit = true;
				} catch (boost::bad_lexical_cast e) {
				}
				try {
					channel = boost::lexical_cast<int>(data["channel"]);
					haschannel = true;
				} catch (boost::bad_lexical_cast e) {
				}
				if (data["battery"]=="LOW") {
					batterylevel = 10;
					hasbattery = true;
				}
				if (data["battery"]=="OK") {
					batterylevel = 100;
					hasbattery = true;
				}
				try {
				  tempC = boost::lexical_cast<float>(data["temperature_C"]);
					hastempC = true;
				} catch (boost::bad_lexical_cast e) {
				}
				try {
					humidity = boost::lexical_cast<int>(data["humidity"]);
					hashumidity = true;
				} catch (boost::bad_lexical_cast e) {
				}
				std::string model = data["model"];
				bool hasstate =
					(data["state"] != "") ||
					(data["command"] != "");
				bool state =
					data["state"] == "ON" ||
					data["command"] == "On";

				// Domoticz seems to have different idx length per type of device
				unsigned int sensoridx =
					(id       & 0xff ) |
					((channel & 0xff ) << 8);

				unsigned int switchidx =
					(id       & 0xfffffff ) |
					((channel & 0xf ) << 28);

				// Select type of data to send depending on fields we have
				if(hastempC && hashumidity) {
					SendTempHumSensor(sensoridx,
							  batterylevel,
							  tempC,
							  humidity,
							  model);
					continue;
				}
				if(hastempC && !hashumidity) {
					SendTempSensor(sensoridx,
						       batterylevel,
						       tempC,
						       model);
					continue;
				}
				if(!hastempC && hashumidity) {
					SendHumiditySensor(sensoridx,
							   batterylevel,
							   humidity,
							   model);
					continue;
				}
				if(hasstate) {
					SendSwitch(switchidx,
						   unit,
						   batterylevel,
						   state,
						   0,
						   model);
					continue;
				}
			} // while gets
		} // if header
		pclose(pipe);
		if(!m_stoprequested) {
			// sleep before retrying
			_log.Log(LOG_STATUS, "rtl_433 failed. Make sure it's properly installed. (https://github.com/merbanan/rtl_433)");
			for(int i=0;i<60 && !m_stoprequested;i++) {
				sleep_milliseconds(1000);
			}
		}
	} // while !stoprequested
	_log.Log(LOG_STATUS, "Rtl433: receiving thread terminating");
}
bool Rtl433::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen= reinterpret_cast<const tRBUF*>(pdata);
	return false;
}
