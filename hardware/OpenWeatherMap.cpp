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
	//#define DEBUG_OPENWEATHERMAP
	//#define DEBUG_OPENWEATHERMAP_WRITE
#endif

#ifdef DEBUG_OPENWEATHERMAP_WRITE
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
#ifdef DEBUG_OPENWEATHERMAP_READ
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


#define round(a) ( int ) ( a + .5 )

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location) :
	m_APIKey(APIKey),
	m_Location(Location),
	m_Language("en")
{
	m_HwdID=ID;

	m_bHaveGPSCoordinated = (Location.find("lat=") != std::string::npos);


	std::string sValue;
	if (m_sql.GetPreferencesVar("Language", sValue))
	{
		m_Language = sValue;
	}
}

COpenWeatherMap::~COpenWeatherMap(void)
{
}

bool COpenWeatherMap::StartHardware()
{
	RequestStart();

	m_thread = std::make_shared<std::thread>(&COpenWeatherMap::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "OpenWeatherMap: Started");
	return (m_thread != nullptr);
}

bool COpenWeatherMap::StopHardware()
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

void COpenWeatherMap::Do_Work()
{
	int sec_counter = 595;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 600 == 0)
		{
			try
			{
				GetMeterDetails();
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "OpenWeatherMap: Error getting/parsing http data!");
			}
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

	sURL << "http://api.openweathermap.org/data/2.5/weather?";
	if (!m_bHaveGPSCoordinated)
		sURL << "q=";
	sURL << m_Location << "&APPID=" << m_APIKey << "&units=metric" << "&lang=" << m_Language;

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

#ifdef DEBUG_OPENWEATHERMAP_WRITE
	SaveString2Disk(sResult, "E:\\OpenWeatherMap.json");
#endif

	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR,"OpenWeatherMap: Invalid data received!");
		return;
	}

	if (root.size() < 1)
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Empty data received!");
		return;
	}
	if (root["cod"].empty())
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid data received!");
		return;
	}
	std::string rcod = root["cod"].asString();
	if (atoi(rcod.c_str()) != 200)
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Error received: %s (%s)",rcod.c_str(), root["message"].asString().c_str());
		return;
	}

	float temp = -999.9f;
	if (!root["main"].empty())
	{
		int humidity = 0;
		int barometric = 0;
		int barometric_forecast = baroForecastNoInfo;
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
				barometric_forecast = baroForecastRain;
			else if (barometric < 1020)
				barometric_forecast = baroForecastCloudy;
			else if (barometric < 1030)
				barometric_forecast = baroForecastPartlyCloudy;
			else
				barometric_forecast = baroForecastSunny;

			if (!root["weather"].empty())
			{
				if (!root["weather"][0].empty())
				{
					if (!root["weather"][0]["id"].empty())
					{
						int condition = root["weather"][0]["id"].asInt();
						if ((condition == 801) || (condition == 802))
							barometric_forecast = baroForecastPartlyCloudy;
						else if (condition == 803)
							barometric_forecast = baroForecastCloudy;
						else if ((condition == 800))
							barometric_forecast = baroForecastSunny;
						else if ((condition >= 300) && (condition < 700))
							barometric_forecast = baroForecastRain;
					}
				}
			}
		}
		if ((temp != -999.9f) && (humidity != 0) && (barometric != 0))
		{
			SendTempHumBaroSensor(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forecast, "THB");
		}
		else if ((temp != -999.9f) && (humidity != 0))
		{
			SendTempHumSensor(1, 255, temp, humidity, "TempHum");
		}
		else
		{
			if (temp != -999.9f)
				SendTempSensor(1, 255, temp, "Temperature");
			if (humidity != 0)
				SendHumiditySensor(1, 255, humidity, "Humidity");
		}
	}

	//Wind
	if (!root["wind"].empty())
	{
		int wind_degrees = -1;
		float windspeed_ms = -1;

		if (!root["wind"]["deg"].empty())
		{
			wind_degrees = root["wind"]["deg"].asInt();
		}
		if (!root["wind"]["speed"].empty())
		{
			windspeed_ms = root["wind"]["speed"].asFloat();
		}
		if ((wind_degrees != -1) && (windspeed_ms != -1))
		{
			bool bHaveTemp = (temp != -999.9f);
			float rTemp = (temp != -999.9f) ? temp : 0;
			SendWind(1, 255, wind_degrees, windspeed_ms, 0, rTemp, 0, bHaveTemp, false, "Wind");
		}
	}

	//Rain
	if (!root["rain"].empty())
	{
		if (!root["rain"]["3h"].empty())
		{
			float RainCount = static_cast<float>(atof(root["rain"]["3h"].asString().c_str()));
			if (RainCount >= 0.00f)
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

				int tr10 = int((float(RainCount)*10.0f));
				tsen.RAIN.raintotal1 = 0;
				tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
				tr10 -= (tsen.RAIN.raintotal2 * 256);
				tsen.RAIN.raintotal3 = (BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, "Rain", 255);
			}
		}
	}

	//Visibility
	if (!root["visibility"].empty())
	{
		if (root["visibility"].isInt())
		{
			float visibility = ((float)root["visibility"].asInt())/1000.0f;
			if (visibility >= 0)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype = sTypeVisibility;
				gdevice.floatval1 = visibility;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, "Visibility", 255);
			}
		}
	}

	//clouds
	if (!root["clouds"].empty())
	{
		if (!root["clouds"]["all"].empty())
		{
			float clouds = root["clouds"]["all"].asFloat();
			SendPercentageSensor(1, 0, 255, clouds, "Clouds %");
		}
	}

	//Forecast URL
	if (!root["id"].empty())
	{
		m_ForecastURL = "http://openweathermap.org/city/" + root["id"].asString();
	}
}

