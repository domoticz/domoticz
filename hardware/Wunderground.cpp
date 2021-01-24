#include "stdafx.h"
#include "Wunderground.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

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

CWunderground::CWunderground(const int ID, const std::string &APIKey, const std::string &Location)
	: m_bForceSingleStation(false)
	, m_bFirstTime(true)
	, m_APIKey(APIKey)
	, m_Location(Location)
{
	m_HwdID = ID;
	Init();
}

void CWunderground::Init()
{
	m_bFirstTime = true;
}

bool CWunderground::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CWunderground::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	if (!m_thread)
		return false;
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}

bool CWunderground::StopHardware()
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

void CWunderground::Do_Work()
{
#ifdef DEBUG_WUNDERGROUNDR
	GetMeterDetails();
#endif
	int sec_counter = 590;
	_log.Log(LOG_STATUS, "Wunderground: Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 10 == 0) {
			m_LastHeartbeat = mytime(nullptr);
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
	sURL << "https://api.weather.com/v3/location/point?geocode=" << szLoc << "&language=en-US&format=json&apiKey=" << m_APIKey;
	sURL << "http://www.wunderground.com/cgi-bin/findweather/getForecast?query=" << szLoc;
	return sURL.str();
}

std::string CWunderground::GetWeatherStationFromGeo()
{
	std::string sValue;
	int nValue;
	if (m_sql.GetPreferencesVar("Location", nValue, sValue))
	{
		std::vector<std::string> strarray;
		StringSplit(sValue, ";", strarray);

		if (strarray.size() == 2)
		{
			std::string Latitude = strarray[0];
			std::string Longitude = strarray[1];

			std::string sResult;
#ifdef DEBUG_WUNDERGROUNDR
			sResult = ReadFile("E:\\wu_location.json");
#else
			std::stringstream sURL;
			sURL << "https://api.weather.com/v3/location/near?geocode=" << Latitude << "," << Longitude << "&product=pws&format=json&apiKey=" << m_APIKey;
			if (!HTTPClient::GET(sURL.str(), sResult))
			{
				_log.Log(LOG_ERROR, "Wunderground: Error getting location/near result! (Check API key!)");
				return "";
			}
#ifdef DEBUG_WUNDERGROUNDW
			SaveString2Disk(sResult, "E:\\wu_location.json");
#endif
#endif
			Json::Value root;

			bool ret = ParseJSon(sResult, root);
			if ((!ret) || (!root.isObject()))
			{
				_log.Log(LOG_ERROR, "WUnderground: Problem getting location/near result. Invalid data received! (Check Station ID!)");
				return "";
			}

			bool bValid = true;
			if (root["location"].empty() == true)
			{
				bValid = false;
			}
			else if (root["location"]["stationId"].empty())
			{
				bValid = false;
			}
			if (!bValid)
			{
				_log.Log(LOG_ERROR, "WUnderground: Problem getting location/near result.Invalid data received, or no data returned!");
				return "";
			}
			if (!root["location"]["stationId"].empty())
			{
				std::string szFirstStation = root["location"]["stationId"][0].asString();
				return szFirstStation;
			}
			_log.Log(LOG_ERROR, "WUnderground: Problem getting location/near result. No stations returned!");
		}
	}
	return "";
}

void CWunderground::GetMeterDetails()
{
	if (m_Location.find(',') != std::string::npos)
	{
		std::string newLocation = GetWeatherStationFromGeo();
		if (newLocation.empty())
			return;
		m_Location = newLocation;
	}
	if (m_Location.empty())
		return;

	std::string sResult;
#ifdef DEBUG_WUNDERGROUNDR
	sResult= ReadFile("E:\\wu.json");
#else
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL << "https://api.weather.com/v2/pws/observations/current?stationId=" << szLoc << "&format=json&units=s&numericPrecision=decimal&apiKey=" << m_APIKey;
	if (!HTTPClient::GET(sURL.str(), sResult))
	{
		_log.Log(LOG_ERROR,"Wunderground: Error getting http data! (Check API key!)");
		return;
	}
#ifdef DEBUG_WUNDERGROUNDW
	SaveString2Disk(sResult, "E:\\wu.json");
#endif
#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR,"WUnderground: Invalid data received! (Check Station ID!)");
		return;
	}

	bool bValid = true;
	if (root["observations"].empty() == true)
	{
		bValid = false;
	}
	else if (root["observations"][0]["country"].empty())
	{
		bValid = false;
	}
	else if (m_Location.find(root["observations"][0]["stationID"].asString()) == std::string::npos)
	{
		bValid = false;
	}
	if (!bValid)
	{
		_log.Log(LOG_ERROR, "WUnderground: Invalid data received, or no data returned!");
		return;
	}

	root = root["observations"][0];

	if (!m_bFirstTime)
	{
		time_t tlocal = time(nullptr);
		time_t tobserver = (time_t)root["epoch"].asInt();
		if (difftime(tlocal, tobserver) >= 1800)
		{
			//When we don't get any valid data in 30 minutes, we also stop using the values
			_log.Log(LOG_ERROR, "WUnderground: Receiving old data from WU! (No new data return for more than 30 minutes)");
			return;
		}
	}
	m_bFirstTime = false;

	std::string tmpstr;
	float temp;
	int humidity=0;
	int barometric=0;
	int barometric_forcast=baroForecastNoInfo;

	temp=root["metric_si"]["temp"].asFloat();

	if (root["humidity"].empty()==false)
	{
		humidity = root["humidity"].asInt();
	}
	if (root["metric_si"]["pressure"].empty()==false)
	{
		barometric=atoi(root["metric_si"]["pressure"].asString().c_str());
		if (barometric<1000)
			barometric_forcast=baroForecastRain;
		else if (barometric<1020)
			barometric_forcast=baroForecastCloudy;
		else if (barometric<1030)
			barometric_forcast=baroForecastPartlyCloudy;
		else
			barometric_forcast=baroForecastSunny;
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

	if (root["winddir"].empty()==false)
	{
		wind_degrees=atoi(root["winddir"].asString().c_str());
	}
	if (root["metric_si"]["windSpeed"].empty()==false)
	{
		if ((root["metric_si"]["windSpeed"] != "N/A") && (root["metric_si"]["windSpeed"] != "--"))
		{
			windspeed_ms = static_cast<float>(atof(root["metric_si"]["windSpeed"].asString().c_str()));
		}
	}
	if (root["metric_si"]["windGust"].empty()==false)
	{
		if ((root["metric_si"]["windGust"] != "N/A") && (root["metric_si"]["windGust"] != "--"))
		{
			windgust_ms = static_cast<float>(atof(root["metric_si"]["windGust"].asString().c_str()));
		}
	}
	if (root["metric_si"]["windChill"].empty()==false)
	{
		if ((root["metric_si"]["windChill"] != "N/A") && (root["metric_si"]["windChill"] != "--"))
		{
			wind_chill = static_cast<float>(atof(root["metric_si"]["windChill"].asString().c_str()));
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
		int sw = round(windspeed_ms * 10.0F);
		tsen.WIND.av_speedh=(BYTE)(sw/256);
		sw-=(tsen.WIND.av_speedh*256);
		tsen.WIND.av_speedl=(BYTE)(sw);

		tsen.WIND.gusth=0;
		tsen.WIND.gustl=0;
		int gw = round(windgust_ms * 10.0F);
		tsen.WIND.gusth=(BYTE)(gw/256);
		gw-=(tsen.WIND.gusth*256);
		tsen.WIND.gustl=(BYTE)(gw);

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh=0;
		tsen.WIND.chilll=0;
		tsen.WIND.temperatureh=0;
		tsen.WIND.temperaturel=0;

		tsen.WIND.tempsign=(wind_temp>=0)?0:1;
		int at10 = round(std::abs(wind_temp * 10.0F));
		tsen.WIND.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.WIND.temperatureh*256);
		tsen.WIND.temperaturel=(BYTE)(at10);

		tsen.WIND.chillsign=(wind_chill>=0)?0:1;
		at10 = round(std::abs(wind_chill * 10.0F));
		tsen.WIND.chillh=(BYTE)(at10/256);
		at10-=(tsen.WIND.chillh*256);
		tsen.WIND.chilll=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, nullptr, 255, nullptr);
	}

	//UV
	if (root["uv"].empty() == false)
	{
		if ((root["uv"] != "N/A") && (root["uv"] != "--"))
		{
			float UV = static_cast<float>(atof(root["uv"].asString().c_str()));
			if ((UV < 16) && (UV >= 0))
			{
				SendUVSensor(0, 1, 255, UV, "UV");
			}
		}
	}

	//Rain
	if (root["metric_si"]["precipTotal"].empty() == false)
	{
		if ((root["metric_si"]["precipTotal"] != "N/A") && (root["metric_si"]["precipTotal"] != "--"))
		{
			float RainCount = static_cast<float>(atof(root["metric_si"]["precipTotal"].asString().c_str()));
			if ((RainCount != -9999.00F) && (RainCount >= 0.00F))
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

				if (root["metric_si"]["precipRate"].empty() == false)
				{
					if ((root["metric_si"]["precipRate"] != "N/A") && (root["metric_si"]["precipRate"] != "--"))
					{
						float rainrateph = static_cast<float>(atof(root["metric_si"]["precipRate"].asString().c_str()));
						if (rainrateph != -9999.00F)
						{
							int at10 = round(std::abs(rainrateph * 100.0F));
							tsen.RAIN.rainrateh = (BYTE)(at10 / 256);
							at10 -= (tsen.RAIN.rainrateh * 256);
							tsen.RAIN.rainratel = (BYTE)(at10);
						}
					}
				}

				int tr10 = int((float(RainCount) * 10.0F));
				tsen.RAIN.raintotal1 = 0;
				tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
				tr10 -= (tsen.RAIN.raintotal2 * 256);
				tsen.RAIN.raintotal3 = (BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, nullptr, 255, nullptr);
			}
		}
	}

	//Visibility (seems not to exist anymore!?)
	if (root["visibility"].empty() == false)
	{
		if ((root["visibility"] != "N/A") && (root["visibility"] != "--"))
		{
			float visibility = static_cast<float>(atof(root["visibility"].asString().c_str()));
			if (visibility >= 0)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype = sTypeVisibility;
				gdevice.floatval1 = visibility;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, nullptr, 255, nullptr);
			}
		}
	}

	//Solar Radiation
	if (root["solarRadiation"].empty() == false)
	{
		float radiation = static_cast<float>(atof(root["solarRadiation"].asString().c_str()));
		if (radiation >= 0.0F)
		{
			_tGeneralDevice gdevice;
			gdevice.subtype = sTypeSolarRadiation;
			gdevice.floatval1 = radiation;
			sDecodeRXMessage(this, (const unsigned char *)&gdevice, nullptr, 255, nullptr);
		}
	}
}

