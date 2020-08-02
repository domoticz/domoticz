#include "stdafx.h"
#include "OpenWeatherMap.h"
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

#define OWM_onecall_URL "https://api.openweathermap.org/data/2.5/onecall?"
#define OWM_icon_URL "https://openweathermap.org/img/wn/"	// for example 10d@4x.png
#define OWM_forecast_URL "https://openweathermap.org/city/"

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location) :
	m_APIKey(APIKey),
	m_Location(Location),
	m_Language("en"),
	m_Lat(1),
	m_Lon(1),
	m_Interval(600)
{
	m_HwdID=ID;

	std::string sValue, sLatitude, sLongitude;
	std::vector<std::string> strarray;

	if (m_sql.GetPreferencesVar("Language", sValue))
	{
		m_Language = sValue;
	}

	_log.Debug(DEBUG_NORM, "OpenWeatherMap: Got location parameter %s", Location.c_str());

	if (Location == "0")
	{
		// Let's get the 'home' Location of this Domoticz instance from the Preferences
		if (m_sql.GetPreferencesVar("Location", sValue))
		{
			StringSplit(sValue, ";", strarray);

			if (strarray.size() == 2)
			{
				sLatitude = strarray[0];
				sLongitude = strarray[1];

				if (!((sLatitude == "1") && (sLongitude == "1")))
				{
					m_Lat = std::stod(sLatitude);
					m_Lon = std::stod(sLongitude);

					_log.Log(LOG_STATUS, "OpenWeatherMap: Using Domoticz home location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid Location found in Settings! (Check your Latitude/Longitude!)");
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			}
		}
	}
	else
	{
			StringSplit(Location, ",", strarray);

			if (strarray.size() == 2)
			{
				sLatitude = strarray[0];
				sLongitude = strarray[1];

				m_Lat = std::stod(sLatitude);
				m_Lon = std::stod(sLongitude);

				_log.Log(LOG_STATUS, "OpenWeatherMap: Using specified location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
			}
			else
			{
				_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid Location specified! (Check your Latitude , Longitude!)");
			}
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
	int sec_counter = m_Interval - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % m_Interval == 0)
		{
			try
			{
				GetMeterDetails();
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "OpenWeatherMap: Unhandled failure getting/parsing http data!");
			}
		}
	}
	_log.Log(LOG_STATUS,"OpenWeatherMap: Worker stopped...");
}

bool COpenWeatherMap::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
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

	sURL << OWM_onecall_URL;
	sURL << "lat=" << m_Lat << "&lon=" << m_Lon;
	sURL << "&appid=" << m_APIKey;
	sURL << "&units=metric" << "&lang=" << m_Language;

	_log.Debug(DEBUG_NORM, "OpenWeatherMap: Get data from %s", sURL.str().c_str());

	try
	{
		if (!HTTPClient::GET(sURL.str(), sResult))
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
	SaveString2Disk(sResult, "OpenWeatherMap.json");
#endif

	Json::Value root;

	bool ret= ParseJSon(sResult,root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR,"OpenWeatherMap: Invalid data received (not JSON)!");
		return;
	}
	if (root.size() < 1)
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: No data, empty response received!");
		return;
	}
	if (root["current"].empty())
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid data received, could not find current weather data!");
		return;
	}

	float temp = -999.9f;
	float fltemp = -999.9f;
	if (!root["current"].empty())
	{
		int humidity = 0;
		int barometric = 0;
		int barometric_forecast = baroForecastNoInfo;
		if (!root["current"]["temp"].empty())
		{
			temp = root["current"]["temp"].asFloat();
		}
		if (!root["current"]["feels_like"].empty())
		{
			fltemp = root["current"]["feels_like"].asFloat();
		}
		if (!root["current"]["humidity"].empty())
		{
			humidity = root["current"]["humidity"].asInt();
		}
		if (!root["current"]["pressure"].empty())
		{
			barometric = atoi(root["current"]["pressure"].asString().c_str());
			if (barometric < 1000)
				barometric_forecast = baroForecastRain;
			else if (barometric < 1020)
				barometric_forecast = baroForecastCloudy;
			else if (barometric < 1030)
				barometric_forecast = baroForecastPartlyCloudy;
			else
				barometric_forecast = baroForecastSunny;

			if (!root["current"]["weather"].empty())
			{
				if (!root["current"]["weather"][0].empty())
				{
					if (!root["current"]["weather"][0]["id"].empty())
					{
						int condition = root["current"]["weather"][0]["id"].asInt();
						if ((condition == 801) || (condition == 802))
							barometric_forecast = baroForecastPartlyCloudy;
						else if (condition == 803)
							barometric_forecast = baroForecastCloudy;
						else if ((condition == 800))
							barometric_forecast = baroForecastSunny;
						else if ((condition >= 300) && (condition < 700))
							barometric_forecast = baroForecastRain;
					}
					if (!root["current"]["weather"][0]["description"].empty())
					{
						std::string weatherdescription = root["current"]["weather"][0]["description"].asString();
						SendTextSensor(1, 1, 255, weatherdescription, "Weather Description");
					}
				}
			}
		}
		if ((temp != -999.9f) && (humidity != 0) && (barometric != 0))
		{
			SendTempHumBaroSensorFloat(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forecast, "TempHumBaro");
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

	// Feel temperature
	if (fltemp != -999.9f)
	{
		SendTempSensor(3, 255, fltemp, "Feel Temperature");
	}

	//Wind
	if (!root["current"]["wind_speed"].empty())
	{
		int16_t wind_degrees = -1;
		float windspeed_ms = -1;
		float windgust_ms = 0;

		if (!root["current"]["wind_deg"].empty())
		{
			wind_degrees = root["current"]["wind_deg"].asInt();
		}
		if (!root["current"]["wind_speed"].empty())
		{
			windspeed_ms = root["current"]["wind_speed"].asFloat();
		}
		if (!root["current"]["wind_gust"].empty())
		{
			windgust_ms = root["current"]["wind_gust"].asFloat();
		}
		if ((wind_degrees != -1) && (windspeed_ms != -1))
		{
			bool bHaveTemp = (temp != -999.9f);
			float rTemp = (bHaveTemp ? temp : 0);
			float rFlTemp = rTemp;

			if ((rTemp < 10.0) && (windspeed_ms >= 1.4))
				rFlTemp = 0; //if we send 0 as chill, it will be calculated
			SendWind(1, 255, wind_degrees, windspeed_ms, windgust_ms, rTemp, rFlTemp, bHaveTemp, true, "Wind");
		}
	}

	//UV
	if (!root["current"]["uvi"].empty())
	{
		float uvi = root["current"]["uvi"].asFloat();
		if ((uvi < 16) && (uvi >= 0))
		{
			SendUVSensor(0, 1, 255, uvi, "UV Index");
		}
	}

	//Rain
	if (!root["current"]["rain"].empty())
	{
		if (!root["current"]["rain"]["1h"].empty())
		{
			float RainCount = static_cast<float>(atof(root["current"]["rain"]["1h"].asString().c_str()));
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
	if (!root["current"]["visibility"].empty())
	{
		if (root["current"]["visibility"].isInt())
		{
			float visibility = ((float)root["current"]["visibility"].asInt())/1000.0f;
			if (visibility >= 0)
			{
				SendVisibilitySensor(1, 1, 255, visibility, "Visibility");
			}
		}
	}

	//clouds
	if (!root["current"]["clouds"].empty())
	{
		float clouds = root["current"]["clouds"].asFloat();
		SendPercentageSensor(1, 0, 255, clouds, "Clouds %");
	}

	//Forecast URL
	if (!root["lat"].empty())
	{
		m_ForecastURL = OWM_forecast_URL + root["lat"].asString();
	}

}
