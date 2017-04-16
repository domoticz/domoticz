#include "stdafx.h"
#include "OpenWeatherMap.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#ifdef _DEBUG
	#define DEBUG_OPENWEATHERMAP
#endif

#define round(a) ( int ) ( a + .5 )

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location) :
	m_APIKey(APIKey),
	m_Location(Location),
	m_Language("en")
{
	_log.Log(LOG_STATUS, "OpenWeatherMap: Create instance");
	m_HwdID=ID;
	m_stoprequested=false;

	std::string sValue;
	if (m_sql.GetPreferencesVar("Language", sValue))
	{
		m_Language = sValue;
#ifdef DEBUG_OPENWEATHERMAP
		_log.Log(LOG_STATUS, "OpenWeatherMap: Language set to %s", sValue.c_str());
#endif
	}
}

COpenWeatherMap::~COpenWeatherMap(void)
{
	_log.Log(LOG_STATUS, "OpenWeatherMap: Destroy instance");
}

bool COpenWeatherMap::StartHardware()
{
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWeatherMap::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool COpenWeatherMap::StopHardware()
{
	m_stoprequested = true;
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void COpenWeatherMap::Do_Work()
{
	int sec_counter = 585;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 600 == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"OpenWeatherMap: Worker stopped...");
}

bool COpenWeatherMap::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

std::string COpenWeatherMap::GetForecastURL()
{
	return m_ForecastURL;
}

void COpenWeatherMap::GetMeterDetails()
{
	std::string sResult;
	std::stringstream sURL;

	sURL << "http://api.openweathermap.org/data/2.5/weather?" << m_Location << "&APPID=" << m_APIKey << "&units=metric" << "&lang=" << m_Language;
	
#ifdef DEBUG_OPENWEATHERMAP
	_log.Log(LOG_STATUS, "OpenWeatherMap: Get data from %s", sURL);
#endif

	try
	{
		bool bret;
		std::string szURL = sURL.str();
		bret = HTTPClient::GET(szURL, sResult);
		if (!bret)
		{
			_log.Log(LOG_ERROR, "OpenWeatherMap: Error getting http data!");
			return;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Error getting http data!");
		return;
	}

#ifdef DEBUG_OPENWEATHERMAP
	_log.Log(LOG_STATUS, "OpenWeatherMap: Parsing json");
#endif

	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"OpenWeatherMap: Invalid data received!");
		return;
	}

	if (root.size() < 1)
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Empty data received!");
		return;
	}

	if (root["cod"].asString() != "200")
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid cod received!");
		return;
	}

	float temp=0;
	int humidity=0;
	int barometric=0;
	int barometric_forcast=baroForecastNoInfo;

	if (!root["main"].empty())
	{

		if (!root["main"]["temp"].empty())
		{
			temp = root["main"]["temp"].asFloat();
		}

		if (!root["main"]["humidity"].empty())
		{
			humidity = root["main"]["humidity"].asInt();
		}
		if (!root["main"]["pressure"].empty())
		{
			barometric = atoi(root["main"]["pressure"].asString().c_str());
			if (barometric < 1000)
				barometric_forcast = baroForecastRain;
			else if (barometric < 1020)
				barometric_forcast = baroForecastCloudy;
			else if (barometric < 1030)
				barometric_forcast = baroForecastPartlyCloudy;
			else
				barometric_forcast = baroForecastSunny;
		}

		if (barometric != 0)
		{
			SendTempHumBaroSensor(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forcast, "THB");
		}
		else if (humidity != 0)
		{
			SendTempHumSensor(1, 255, temp, humidity, "TempHum");
		}
		else
		{
			SendTempSensor(1, 255, temp, "Temperature");
		}
	}

	//Wind
	if (!root["wind"].empty())
	{
		int wind_degrees = 0;
		float windspeed_ms = 0;

		if (!root["wind"]["deg"].empty())
		{
			wind_degrees = root["wind"]["deg"].asInt();
		}
		if (!root["wind"]["speed"].empty())
		{
			windspeed_ms = root["wind"]["speed"].asFloat();
		}

		SendWind(1, 255, wind_degrees, windspeed_ms, 0, 0, 0, false, "Wind");
	}

	//Rain
	if (!root["rain"].empty())
	{
			float RainCount = static_cast<float>(atof(root["rain"]["3h"].asString().c_str()));
			if (RainCount>=0.00f)
			{
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.RAIN.packetlength=sizeof(tsen.RAIN)-1;
				tsen.RAIN.packettype=pTypeRAIN;
				tsen.RAIN.subtype=sTypeRAINWU;
				tsen.RAIN.battery_level=9;
				tsen.RAIN.rssi=12;
				tsen.RAIN.id1=0;
				tsen.RAIN.id2=1;

				tsen.RAIN.rainrateh=0;
				tsen.RAIN.rainratel=0;

				int tr10=int((float(RainCount)*10.0f));
				tsen.RAIN.raintotal1=0;
				tsen.RAIN.raintotal2=(BYTE)(tr10/256);
				tr10-=(tsen.RAIN.raintotal2*256);
				tsen.RAIN.raintotal3=(BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, "Rain", 255);
			}
	}

	//Visibility
	if (!root["visibility"].empty())
	{
		float visibility = root["visibility"]["value"].asFloat();
		if (visibility >= 0)
		{
			_tGeneralDevice gdevice;
			gdevice.subtype = sTypeVisibility;
			gdevice.floatval1 = visibility;
			sDecodeRXMessage(this, (const unsigned char *)&gdevice, "Visibility", 255);
		}
	}

	// TODO - not working on this moment on www site
	//Forecast URL
	//if (!root["Link"].empty())
	//{
	//	m_ForecastURL = root["Link"].asString();
	//}
}

