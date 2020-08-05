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
	m_Interval(600),
	m_itIsRaining(false)
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
				if (strarray.size() == 1 && std::stoi(Location) > 0)
				{
					_log.Log(LOG_ERROR, "OpenWeatherMap: StationID/CityID usage not (yet) implemented. Please specify Latitude, Longitude!");
				}
				else
				{
					_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid Location specified! (Check your StationId/CityId or Latitude, Longitude!)");
				}
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

	bool ret=ParseJSon(sResult,root);
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

	// Process current
	if (root["current"].empty())
	{
		_log.Log(LOG_ERROR, "OpenWeatherMap: Invalid data received, could not find current weather data!");
		return;
	}
	else
	{
		Json::Value current;
		float temp = -999.9f;
		float fltemp = -999.9f;

		current = root["current"];
		int humidity = 0;
		float barometric = 0;
		int barometric_forecast = wsbaroforecast_unknown;
		if (!current["temp"].empty())
		{
			temp = current["temp"].asFloat();
		}
		if (!current["feels_like"].empty())
		{
			fltemp = current["feels_like"].asFloat();
		}
		if (!current["humidity"].empty())
		{
			humidity = current["humidity"].asInt();
		}
		if (!current["pressure"].empty())
		{
			barometric = current["pressure"].asFloat();
			if (barometric < 1000)
			{
				barometric_forecast = wsbaroforecast_rain;
				if (temp != -999.9f)
				{
					if (temp <= 0)
						barometric_forecast = wsbaroforecast_snow;
				}
			}
			else if (barometric < 1020)
				barometric_forecast = wsbaroforecast_cloudy;
			else if (barometric < 1030)
				barometric_forecast = wsbaroforecast_some_clouds;
			else
				barometric_forecast = wsbaroforecast_sunny;

			if (!current["weather"].empty())
			{
				if (!current["weather"][0].empty())
				{
					if (!current["weather"][0]["id"].empty())
					{
						int condition = current["weather"][0]["id"].asInt();
						/* We do not use it at the moment, does not feel ok
						if ((condition == 801) || (condition == 802))
							barometric_forecast = baroForecastPartlyCloudy;
						else if (condition == 803)
							barometric_forecast = baroForecastCloudy;
						else if ((condition == 800))
							barometric_forecast = baroForecastSunny;
						else if ((condition >= 300) && (condition < 700))
							barometric_forecast = baroForecastRain;
						*/
					}
					if (!current["weather"][0]["description"].empty())
					{
						std::string weatherdescription = current["weather"][0]["description"].asString();
						SendTextSensor(2, 1, 255, weatherdescription, "Weather Description");
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

		// Feel temperature
		if (fltemp != -999.9f)
		{
			SendTempSensor(3, 255, fltemp, "Feel Temperature");
		}

		//Wind
		if (!current["wind_speed"].empty())
		{
			int16_t wind_degrees = -1;
			float windgust_ms = 0;

			float windspeed_ms = current["wind_speed"].asFloat();

			if (!current["wind_gust"].empty())
			{
				windgust_ms = current["wind_gust"].asFloat();
			}

			if (!current["wind_deg"].empty())
			{
				wind_degrees = current["wind_deg"].asInt();

				bool bHaveTemp = (temp != -999.9f);
				float rTemp = (bHaveTemp ? temp : 0);
				float rFlTemp = rTemp;

				if ((rTemp < 10.0) && (windspeed_ms >= 1.4))
					rFlTemp = 0; //if we send 0 as chill, it will be calculated
				SendWind(4, 255, wind_degrees, windspeed_ms, windgust_ms, rTemp, rFlTemp, bHaveTemp, true, "Wind");
			}
		}

		//UV
		if (!current["uvi"].empty())
		{
			float uvi = current["uvi"].asFloat();
			if ((uvi < 16) && (uvi >= 0))
			{
				SendUVSensor(5, 1, 255, uvi, "UV Index");
			}
		}

		//Visibility
		if (!current["visibility"].empty() && current["visibility"].isInt())
		{
			float visibility = ((float)current["visibility"].asInt())/1000.0f;
			if (visibility >= 0)
			{
				SendVisibilitySensor(6, 1, 255, visibility, "Visibility");
			}
		}

		//clouds
		if (!current["clouds"].empty())
		{
			float clouds = current["clouds"].asFloat();
			SendPercentageSensor(7, 1, 255, clouds, "Clouds %");
		}

		//Rain (only present if their is rain (or snow))
		float rainmm = 0;
		if (!current["rain"].empty() && !current["rain"]["1h"].empty())
		{
			float rainmm = static_cast<float>(atof(current["rain"]["1h"].asString().c_str()));
		}
		else
		{	// Maybe it is not raining but snowing... we show this as 'rain' as well (for now at least)
			if (!current["snow"].empty() && !current["snow"]["1h"].empty())
			{
				float rainmm = static_cast<float>(atof(current["snow"]["1h"].asString().c_str()));
			}
		}
		SendRainRateSensor(8, 255, rainmm, "Rain");
		m_itIsRaining = rainmm > 0;
		SendSwitch(9, 1, 255, m_itIsRaining, 0, "Is it Raining");
	}

	// Process daily forecast data if available
	if (root["daily"].empty())
	{
		_log.Log(LOG_STATUS, "OpenWeatherMap: Could not find daily weather forecast data!");
	}
	else
	{
		Json::Value dailyfc;
		uint8_t iDay = 0;
		uint64_t UTCfctime;

		dailyfc = root["daily"];
		do
		{
			if (dailyfc[iDay]["dt"].empty())
			{
				_log.Log(LOG_STATUS, "OpenWeatherMap: Processing daily forecast failed (unexpected structure)!");
				break;
			}
			else
			{
				UTCfctime = dailyfc[iDay]["dt"].asUInt64();

				_log.Debug(DEBUG_NORM, "OpenWeatherMap: Processing daily forecast for %lu",UTCfctime);

				try
				{
					float pop = dailyfc[iDay]["pop"].asFloat();
					float uvi = dailyfc[iDay]["uvi"].asFloat();
					float clouds = dailyfc[iDay]["clouds"].asFloat();
					float windspeed_ms = dailyfc[iDay]["wind_speed"].asFloat();
					float barometric = dailyfc[iDay]["pressure"].asFloat();
					uint8_t humidity = (uint8_t) dailyfc[iDay]["humidity"].asUInt();
					float mintemp = dailyfc[iDay]["temp"]["min"].asFloat();
					float maxtemp = dailyfc[iDay]["temp"]["max"].asFloat();
					std::string wdesc = dailyfc[iDay]["weather"][0]["description"].asString();
					std::string wicon = dailyfc[iDay]["weather"][0]["icon"].asString();
					int barometric_forecast = wsbaroforecast_unknown;
					if (barometric < 1000)
					{
						barometric_forecast = wsbaroforecast_rain;
						if (maxtemp != -999.9f)
						{
							if (maxtemp <= 0)
								barometric_forecast = wsbaroforecast_snow;
						}
					}
					else if (barometric < 1020)
						barometric_forecast = wsbaroforecast_cloudy;
					else if (barometric < 1030)
						barometric_forecast = wsbaroforecast_some_clouds;
					else
						barometric_forecast = wsbaroforecast_sunny;

					int NodeID = 17 + iDay * 16;
					std::stringstream sName;

					sName << "TempHumBaro Day " << (iDay + 1);
					SendTempHumBaroSensorFloat(NodeID, 255, maxtemp, humidity, static_cast<float>(barometric), barometric_forecast, sName.str().c_str());

					NodeID++;;
					sName.str("");
					sName.clear();
					sName << "Weather Description Day " << (iDay + 1);
					SendTextSensor(NodeID, 1, 255, wdesc, sName.str().c_str());
					sName.str("");
					sName.clear();
					sName << "Weather Description  Day " << (iDay + 1) << " Icon";
					SendTextSensor(NodeID, 2, 255, wicon, sName.str().c_str());

					NodeID++;;
					sName.str("");
					sName.clear();
					sName << "Minumum Temperature Day " << (iDay + 1);
					SendTempSensor(NodeID, 255, mintemp, sName.str().c_str());

					_log.Debug(DEBUG_NORM, "OpenWeatherMap: Processed forecast for day %i: %s - %f - %f - %f",iDay, wdesc.c_str(), maxtemp,mintemp, pop);
				}
				catch(const std::exception& e)
				{
					_log.Debug(DEBUG_NORM, "OpenWeatherMap: Processing daily forecast crashed for day %i on %s",iDay, e.what());
				}
			}
			iDay++;
		}
		while (!dailyfc[iDay].empty());
		_log.Debug(DEBUG_NORM, "OpenWeatherMap: Processed %i daily forecasts",iDay);
	}

	//Forecast URL
	if (!root["lat"].empty())
	{
		m_ForecastURL = OWM_forecast_URL + root["lat"].asString();
	}

}
