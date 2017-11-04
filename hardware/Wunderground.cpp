#include "stdafx.h"
#include "Wunderground.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	//#define DEBUG_WUNDERGROUNDR
	//#define DEBUG_WUNDERGROUNDW
#endif

#ifdef DEBUG_WUNDERGROUNDW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_WUNDERGROUNDR
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

CWunderground::CWunderground(const int ID, const std::string &APIKey, const std::string &Location) :
m_APIKey(APIKey),
m_Location(Location),
m_bForceSingleStation(false),
m_bFirstTime(true)
{
	m_HwdID = ID;
	m_stoprequested = false;
	Init();
}

CWunderground::~CWunderground(void)
{
}

void CWunderground::Init()
{
	m_bFirstTime = true;
}

bool CWunderground::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CWunderground::Do_Work, this)));
	if (!m_thread)
		return false;
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}

bool CWunderground::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CWunderground::Do_Work()
{
	int sec_counter = 590;
	_log.Log(LOG_STATUS, "Wunderground: Worker started...");

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 10 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
#ifdef DEBUG_WUNDERGROUNDR
		if (sec_counter % 10 == 0)
#else
		if (sec_counter % 600 == 0)
#endif
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"Wunderground: Worker stopped...");
}

bool CWunderground::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

std::string CWunderground::GetForecastURL()
{
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL << "http://www.wunderground.com/cgi-bin/findweather/getForecast?query=" << szLoc;
	return sURL.str();
}

void CWunderground::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_WUNDERGROUNDR
	sResult= ReadFile("E:\\wu.json");
#else
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL << "http://api.wunderground.com/api/" << m_APIKey << "/conditions/q/" << szLoc << ".json";
	bool bret;
	std::string szURL=sURL.str();
	bret=HTTPClient::GET(szURL,sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR,"Wunderground: Error getting http data!");
		return;
	}
#ifdef DEBUG_WUNDERGROUNDW
	SaveString2Disk(sResult, "E:\\wu.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR,"WUnderground: Invalid data received!");
		return;
	}

	bool bValid = true;
	if (root["response"].empty() == true)
	{
		bValid = false;
	}
	else if (!root["response"]["error"].empty())
	{
		bValid = false;
		if (!root["response"]["error"]["description"].empty())
		{
			_log.Log(LOG_ERROR, "WUnderground: Error: %s", root["response"]["error"]["description"].asString().c_str());
			return;
		}
	}
	else if (root["current_observation"].empty()==true)
	{
		bValid = false;
		return;
	}
	else if (root["current_observation"]["temp_c"].empty() == true)
	{
		bValid = false;
	}
	else if (m_bForceSingleStation && root["current_observation"]["station_id"].empty())
	{
		bValid = false;
	}
	else if (m_bForceSingleStation && m_Location.find(root["current_observation"]["station_id"].asString()) == std::string::npos)
	{
		bValid = false;
	}
	else if (root["current_observation"]["observation_epoch"].empty() == true)
	{
		bValid = false;
	}
	else if (root["current_observation"]["local_epoch"].empty() == true)
	{
		bValid = false;
	}
	else
	{
		if (!m_bFirstTime)
		{
			time_t tlocal = static_cast<time_t>(atoll(root["current_observation"]["local_epoch"].asString().c_str()));
			time_t tobserver = static_cast<time_t>(atoll(root["current_observation"]["observation_epoch"].asString().c_str()));
			if (difftime(tlocal, tobserver) >= 1800)
			{
				//When we don't get any valid data in 30 minutes, we also stop using the values
				_log.Log(LOG_ERROR, "WUnderground: Receiving old data from WU! (No new data return for more than 30 minutes)");
				return;
			}
		}
		m_bFirstTime = false;
	}
	if (!bValid)
	{
		_log.Log(LOG_ERROR, "WUnderground: Invalid data received, or no data returned!");
		return;
	}

	std::string tmpstr;
	float temp;
	int humidity=0;
	int barometric=0;
	int barometric_forcast=baroForecastNoInfo;


	temp=root["current_observation"]["temp_c"].asFloat();

	if (root["current_observation"]["relative_humidity"].empty()==false)
	{
		tmpstr=root["current_observation"]["relative_humidity"].asString();
		size_t pos=tmpstr.find("%");
		if (pos==std::string::npos)
		{
			_log.Log(LOG_ERROR,"WUnderground: Invalid data received!");
			return;
		}
		humidity=atoi(tmpstr.substr(0,pos).c_str());
	}
	if (root["current_observation"]["pressure_mb"].empty()==false)
	{
		barometric=atoi(root["current_observation"]["pressure_mb"].asString().c_str());
		if (barometric<1000)
			barometric_forcast=baroForecastRain;
		else if (barometric<1020)
			barometric_forcast=baroForecastCloudy;
		else if (barometric<1030)
			barometric_forcast=baroForecastPartlyCloudy;
		else
			barometric_forcast=baroForecastSunny;

		if (root["current_observation"]["icon"].empty()==false)
		{
			std::string forcasticon=root["current_observation"]["icon"].asString();
			if (forcasticon=="partlycloudy")
			{
				barometric_forcast=baroForecastPartlyCloudy;
			}
			else if (forcasticon=="cloudy")
			{
				barometric_forcast=baroForecastCloudy;
			}
			else if (forcasticon=="sunny")
			{
				barometric_forcast=baroForecastSunny;
			}
			else if (forcasticon=="clear")
			{
				barometric_forcast=baroForecastSunny;
			}			
			else if (forcasticon=="rain")
			{
				barometric_forcast=baroForecastRain;
			}
		}
	}

	if (barometric!=0)
	{
		//Add temp+hum+baro device
		SendTempHumBaroSensor(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forcast, "THB");
	}
	else if (humidity!=0)
	{
		//add temp+hum device
		SendTempHumSensor(1, 255, temp, humidity, "TempHum");
	}
	else
	{
		//add temp device
		SendTempSensor(1, 255, temp, "Temperature");
	}

	//Wind
	int wind_degrees=-1;
	float windspeed_ms=0;
	float windgust_ms=0;
	float wind_temp=temp;
	float wind_chill=temp;
	int windgust=1;
	float windchill=-1;

	if (root["current_observation"]["wind_degrees"].empty()==false)
	{
		wind_degrees=atoi(root["current_observation"]["wind_degrees"].asString().c_str());
	}
	if (root["current_observation"]["wind_mph"].empty()==false)
	{
		if ((root["current_observation"]["wind_mph"] != "N/A") && (root["current_observation"]["wind_mph"] != "--"))
		{
			float temp_wind_mph = static_cast<float>(atof(root["current_observation"]["wind_mph"].asString().c_str()));
			if (temp_wind_mph!=-9999.00f)
			{
				//convert to m/s
				windspeed_ms=temp_wind_mph*0.44704f;
			}
		}
	}
	if (root["current_observation"]["wind_gust_mph"].empty()==false)
	{
		if ((root["current_observation"]["wind_gust_mph"] != "N/A") && (root["current_observation"]["wind_gust_mph"] != "--"))
		{
			float temp_wind_gust_mph = static_cast<float>(atof(root["current_observation"]["wind_gust_mph"].asString().c_str()));
			if (temp_wind_gust_mph!=-9999.00f)
			{
				//convert to m/s
				windgust_ms=temp_wind_gust_mph*0.44704f;
			}
		}
	}
	if (root["current_observation"]["feelslike_c"].empty()==false)
	{
		if ((root["current_observation"]["feelslike_c"] != "N/A") && (root["current_observation"]["feelslike_c"] != "--"))
		{
			wind_chill = static_cast<float>(atof(root["current_observation"]["feelslike_c"].asString().c_str()));
		}
	}
	if (wind_degrees!=-1)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.WIND.packetlength=sizeof(tsen.WIND)-1;
		tsen.WIND.packettype=pTypeWIND;
		tsen.WIND.subtype=sTypeWIND4;
		tsen.WIND.battery_level=9;
		tsen.WIND.rssi=12;
		tsen.WIND.id1=0;
		tsen.WIND.id2=1;

		float winddir=float(wind_degrees);
		int aw=round(winddir);
		tsen.WIND.directionh=(BYTE)(aw/256);
		aw-=(tsen.WIND.directionh*256);
		tsen.WIND.directionl=(BYTE)(aw);

		tsen.WIND.av_speedh=0;
		tsen.WIND.av_speedl=0;
		int sw=round(windspeed_ms*10.0f);
		tsen.WIND.av_speedh=(BYTE)(sw/256);
		sw-=(tsen.WIND.av_speedh*256);
		tsen.WIND.av_speedl=(BYTE)(sw);

		tsen.WIND.gusth=0;
		tsen.WIND.gustl=0;
		int gw=round(windgust_ms*10.0f);
		tsen.WIND.gusth=(BYTE)(gw/256);
		gw-=(tsen.WIND.gusth*256);
		tsen.WIND.gustl=(BYTE)(gw);

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh=0;
		tsen.WIND.chilll=0;
		tsen.WIND.temperatureh=0;
		tsen.WIND.temperaturel=0;

		tsen.WIND.tempsign=(wind_temp>=0)?0:1;
		int at10=round(std::abs(wind_temp*10.0f));
		tsen.WIND.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.WIND.temperatureh*256);
		tsen.WIND.temperaturel=(BYTE)(at10);

		tsen.WIND.chillsign=(wind_temp>=0)?0:1;
		at10=round(std::abs(wind_chill*10.0f));
		tsen.WIND.chillh=(BYTE)(at10/256);
		at10-=(tsen.WIND.chillh*256);
		tsen.WIND.chilll=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, NULL, 255);
	}

	//UV
	if (root["current_observation"].empty() == false)
	{
		if (root["current_observation"]["UV"].empty() == false)
		{
			if ((root["current_observation"]["UV"] != "N/A") && (root["current_observation"]["UV"] != "--"))
			{
				float UV = static_cast<float>(atof(root["current_observation"]["UV"].asString().c_str()));
				if ((UV < 16) && (UV >= 0))
				{
					SendUVSensor(0, 1, 255, UV, "UV");
				}
			}
		}
	}

	//Rain
	if (root["current_observation"]["precip_today_metric"].empty() == false)
	{
		if ((root["current_observation"]["precip_today_metric"] != "N/A") && (root["current_observation"]["precip_today_metric"] != "--"))
		{
			float RainCount = static_cast<float>(atof(root["current_observation"]["precip_today_metric"].asString().c_str()));
			if ((RainCount != -9999.00f) && (RainCount >= 0.00f))
			{
				RBUF tsen;
				memset(&tsen, 0, sizeof(RBUF));
				tsen.RAIN.packetlength = sizeof(tsen.RAIN) - 1;
				tsen.RAIN.packettype = pTypeRAIN;
				tsen.RAIN.subtype = sTypeRAINWU;
				tsen.RAIN.battery_level = 9;
				tsen.RAIN.rssi = 12;
				tsen.RAIN.id1 = 0;
				tsen.RAIN.id2 = 1;

				tsen.RAIN.rainrateh = 0;
				tsen.RAIN.rainratel = 0;

				if (root["current_observation"]["precip_1hr_metric"].empty() == false)
				{
					if ((root["current_observation"]["precip_1hr_metric"] != "N/A") && (root["current_observation"]["precip_1hr_metric"] != "--"))
					{
						float rainrateph = static_cast<float>(atof(root["current_observation"]["precip_1hr_metric"].asString().c_str()));
						if (rainrateph != -9999.00f)
						{
							int at10 = round(std::abs(rainrateph*100.0f));
							tsen.RAIN.rainrateh = (BYTE)(at10 / 256);
							at10 -= (tsen.RAIN.rainrateh * 256);
							tsen.RAIN.rainratel = (BYTE)(at10);
						}
					}
				}

				int tr10 = int((float(RainCount)*10.0f));
				tsen.RAIN.raintotal1 = 0;
				tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
				tr10 -= (tsen.RAIN.raintotal2 * 256);
				tsen.RAIN.raintotal3 = (BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, NULL, 255);
			}
		}
	}

	//Visibility
	if (root["current_observation"]["visibility_km"].empty() == false)
	{
		if ((root["current_observation"]["visibility_km"] != "N/A") && (root["current_observation"]["visibility_km"] != "--"))
		{
			float visibility = static_cast<float>(atof(root["current_observation"]["visibility_km"].asString().c_str()));
			if (visibility >= 0)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype = sTypeVisibility;
				gdevice.floatval1 = visibility;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
			}
		}
	}

	//Solar Radiation
	if (root["current_observation"]["solarradiation"].empty() == false)
	{
		if ((root["current_observation"]["solarradiation"] != "N/A") && (root["current_observation"]["solarradiation"] != "--"))
		{
			float radiation = static_cast<float>(atof(root["current_observation"]["solarradiation"].asString().c_str()));
			if (radiation >= 0.0f)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype = sTypeSolarRadiation;
				gdevice.floatval1 = radiation;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
			}
		}
	}
}

