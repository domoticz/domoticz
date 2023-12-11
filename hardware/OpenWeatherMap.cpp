#include "stdafx.h"
#include "OpenWeatherMap.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#ifdef _DEBUG
	//#define DEBUG_OPENWEATHERMAP_READ
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

#define OWM_Get_Weather_Details "https://api.openweathermap.org/data/2.5/weather?"
#define OWM_icon_URL "https://openweathermap.org/img/wn/"	// for example 10d@4x.png
#define OWM_forecast_URL "https://openweathermap.org/city/"

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location, const int owmforecastscreen) :
	m_APIKey(APIKey),
	m_Location(Location),
	m_Language("en"),
	m_use_owminforecastscreen(owmforecastscreen)
{
	m_HwdID=ID;

	std::string sValue;
	if (m_sql.GetPreferencesVar("Language", sValue))
	{
		m_Language = sValue;
	}
}

bool COpenWeatherMap::ResolveLocation(const std::string& Location, double& latitude, double& longitude, uint32_t& cityid, const bool IsCityName)
{
	std::stringstream sURL;

	sURL << OWM_Get_Weather_Details;
	if (IsCityName)
		sURL << "q=" << Location;
	else
		sURL << "id=" << Location;
	sURL << "&appid=" << m_APIKey;
	sURL << "&units=metric" << "&lang=" << m_Language;
	std::string sResult;

	if (!HTTPClient::GET(sURL.str(), sResult))
	{
		Log(LOG_ERROR, "Error getting http data!");
		return false;
	}

#ifdef DEBUG_OPENWEATHERMAP_WRITE
	SaveString2Disk(sResult, "E:\\OpenWeatherMap_city.json");
#endif

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received (not JSON)!");
		return false;
	}
	if (root.empty())
	{
		Log(LOG_ERROR, "No data, empty response received!");
		return false;
	}

	if (root["cod"].empty() || root["id"].empty() || root["coord"].empty())
	{
		Log(LOG_ERROR, "Invalid data received, missing relevant fields (cod, id, coord)!");
		return false;
	}

	if (root["cod"].asString() != "200")
	{
		Log(LOG_ERROR, "Error code received (%s)!", root["cod"].asString().c_str());
		return false;
	}

	// Process current
	latitude = root["coord"]["lat"].asDouble();
	longitude = root["coord"]["lon"].asDouble();

	return true;
}

bool COpenWeatherMap::StartHardware()
{
	std::string sValue, sLatitude, sLongitude;
	std::vector<std::string> strarray;
	uint32_t cityid;
	Debug(DEBUG_NORM, "Got location parameter %s", m_Location.c_str());

	if (m_Location.empty())
	{
		Log(LOG_ERROR, "No Location specified! (Check your StationId/CityId or Latitude, Longitude!)");
	}
	else
	{
		if (m_Location == "0")
		{
			// Let's get the 'home' Location of this Domoticz instance from the Preferences
			if (m_sql.GetPreferencesVar("Location", sValue))
			{
				StringSplit(sValue, ";", strarray);

				if (strarray.size() == 2)
				{
					sLatitude = strarray[0];
					sLongitude = strarray[1];

					if ((sLatitude == "1") && (sLongitude == "1"))
					{
						Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
						return false;
					}
					m_Lat = atof(sLatitude.c_str());
					m_Lon = atof(sLongitude.c_str());

					m_Location = sLatitude + "," + sLongitude;	// rewrite the default Location data to match what OWN expects

					Log(LOG_STATUS, "Using Domoticz home location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
				}
				else
				{
					Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
					return false;
				}
			}
		}
		else
		{
			StringSplit(m_Location, ",", strarray);
			if (strarray.size() == 2)
			{
				//It could be a City/Country or a Lat/Long (Londen,UK)
				sLatitude = strarray[0];
				sLongitude = strarray[1];

				double lat, lon;
				char* p;
				double converted = strtod(sLatitude.c_str(), &p);
				if (*p)
				{
					// conversion failed because the input wasn't a number
					// assume it is a city/country combination
					Log(LOG_STATUS, "Using specified location (City %s)!", m_Location.c_str());
					if (!ResolveLocation(m_Location, lat, lon, cityid))
					{
						Log(LOG_ERROR, "Unable to resolve City to Latitude/Longitude, please use Latitude,Longitude directly in hardware setup!)");
					}
					else
					{
						Log(LOG_STATUS, "City (%d)-> Lat/Long = %g,%g", cityid, lat, lon);
						m_Lat = lat;
						m_Lon = lon;
						m_CityID = cityid;
					}
				}
				else
				{
					//Assume it is a Lat/Long
					m_Lat = atof(sLatitude.c_str());
					m_Lon = atof(sLongitude.c_str());
					Log(LOG_STATUS, "Using specified location (Lon %s, Lat %s) Unable to find nearest City!", sLongitude.c_str(), sLatitude.c_str());
				}
			}
			else
			{
				//Could be a Station ID (Number) or a City
				char* p;
				long converted = strtol(m_Location.c_str(), &p, 10);
				double lat, lon;
				if (*p)
				{
					//City
					Log(LOG_STATUS, "Using specified location (City %s)!", m_Location.c_str());
					if (!ResolveLocation(m_Location, lat, lon, cityid))
					{
						Log(LOG_ERROR, "Unable to resolve City to Latitude/Longitude, please use Latitude,Longitude directly in hardware setup!)");
					}
					else
					{
						Log(LOG_STATUS, "City (%d)-> Lat/Long = %g,%g", cityid, lat, lon);
						m_Lat = lat;
						m_Lon = lon;
						m_CityID = cityid;
					}
				}
				else
				{
					//Station ID
					Log(LOG_STATUS, "Using specified location (Station ID %s)!", m_Location.c_str());
					if (!ResolveLocation(m_Location, lat, lon, cityid, false))
					{
						Log(LOG_ERROR, "Unable to resolve City to Latitude/Longitude, please use Latitude,Longitude directly in hardware setup!)");
					}
					else
					{
						Log(LOG_STATUS, "City (%d)-> Lat/Long = %g,%g", cityid, lat, lon);
						m_Lat = lat;
						m_Lon = lon;
						m_CityID = cityid;
					}
				}
			}
		}
	}

	if(m_use_owminforecastscreen)
	{
		if (m_CityID > 0)
		{
			std::stringstream ss;
			ss << OWM_forecast_URL << m_CityID;
			m_ForecastURL = ss.str();

			Log(LOG_STATUS, "Updating preferences for forecastscreen to use OpenWeatherMap!");
			m_sql.UpdatePreferencesVar("ForecastHardwareID",m_HwdID);
		}
	}
	else
	{
		int iValue;
		m_sql.GetPreferencesVar("ForecastHardwareID", iValue);
		if (iValue == m_HwdID)
		{
			// User has de-activated OWM for the forecast screen
			m_sql.UpdatePreferencesVar("ForecastHardwareID",0);
			Log(LOG_STATUS, "Updating preferences for forecastscreen to not use OpenWeatherMap anymore (back to default)!");
		}
	}

	RequestStart();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);
	Log(LOG_STATUS, "Started");
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

#define OpenWeatherMap_Poll_Interval 300

void COpenWeatherMap::Do_Work()
{
	int sec_counter = OpenWeatherMap_Poll_Interval - 3;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % OpenWeatherMap_Poll_Interval == 0)
		{
			try
			{
				GetMeterDetails();
			}
			catch (...)
			{
				Log(LOG_ERROR, "Unhandled failure getting/parsing http data!");
			}
		}
	}
	Log(LOG_STATUS,"Worker stopped...");
}

bool COpenWeatherMap::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

std::string COpenWeatherMap::GetForecastURL()
{
	return m_ForecastURL;
}

Json::Value COpenWeatherMap::GetForecastData()
{
	Json::Value root;

	root["url"] = m_ForecastURL;
	root["location"] = m_Location;
	root["lat"] = m_Lat;
	root["lon"] = m_Lon;
	root["appid"] = m_APIKey;
	root["cityid"] = m_CityID;

	Debug(DEBUG_NORM, "GetForecastData: \n%s", root.toStyledString().c_str());
	return root;
}

int COpenWeatherMap::GetForecastFromBarometricPressure(const float pressure, const float temp)
{
	int barometric_forecast = wsbaroforecast_unknown;
	if (pressure < 1000)
	{
		barometric_forecast = wsbaroforecast_rain;
		if (temp != -999.9F)
		{
			if (temp <= 0)
				barometric_forecast = wsbaroforecast_snow;
		}
	}
	else if (pressure < 1020)
		barometric_forecast = wsbaroforecast_cloudy;
	else if (pressure < 1030)
		barometric_forecast = wsbaroforecast_some_clouds;
	else
		barometric_forecast = wsbaroforecast_sunny;

	return barometric_forecast;
}

void COpenWeatherMap::GetMeterDetails()
{
	if (m_Lat == 0)
		return;

	std::string sResult;
#ifdef DEBUG_OPENWEATHERMAP_READ
	sResult = ReadFile("E:\\OpenWeatherMap.json");
#else
	std::stringstream sURL;

	sURL << OWM_Get_Weather_Details;
	sURL << "lat=" << m_Lat << "&lon=" << m_Lon;
	sURL << "&appid=" << m_APIKey;
	sURL << "&units=metric" << "&lang=" << m_Language;

	Debug(DEBUG_NORM, "Get data from %s", sURL.str().c_str());

	try
	{
		if (!HTTPClient::GET(sURL.str(), sResult))
		{
			Log(LOG_ERROR, "Error getting http data!");
			return;
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error getting http data!");
		return;
	}

#ifdef DEBUG_OPENWEATHERMAP_WRITE
	SaveString2Disk(sResult, "E:\\OpenWeatherMap.json");
#endif
#endif
	Json::Value root;

	bool ret=ParseJSon(sResult,root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR,"Invalid data received (not JSON)!");
		return;
	}
	if (root.empty())
	{
		Log(LOG_ERROR, "No data, empty response received!");
		return;
	}

	// Process current
	if (root["main"].empty())
	{
		Log(LOG_ERROR, "Invalid data received, could not find current weather data!");
		return;
	}

	//Current values
	float temp = -999.9F;
	float fltemp = -999.9F;

	int humidity = 0;
	float barometric = 0;
	int barometric_forecast = 0;
	if (!root["main"]["temp"].empty())
	{
		temp = root["main"]["temp"].asFloat();
	}
	if (!root["main"]["feels_like"].empty())
	{
		fltemp = root["main"]["feels_like"].asFloat();
	}
	if (!root["main"]["humidity"].empty())
	{
		humidity = root["main"]["humidity"].asInt();
	}
	if (!root["main"]["pressure"].empty())
	{
		barometric = root["main"]["pressure"].asFloat();
		barometric_forecast = GetForecastFromBarometricPressure(barometric, temp);
	}
	if ((temp != -999.9F) && (humidity != 0) && (barometric != 0))
	{
		SendTempHumBaroSensorFloat(1, 255, temp, humidity, barometric, barometric_forecast, "TempHumBaro");
	}
	else if ((temp != -999.9F) && (humidity != 0))
	{
		SendTempHumSensor(1, 255, temp, humidity, "TempHum");
	}
	else
	{
		if (temp != -999.9F)
			SendTempSensor(1, 255, temp, "Temperature");
		if (humidity != 0)
			SendHumiditySensor(1, 255, humidity, "Humidity");
	}

	// Feel temperature
	if (fltemp != -999.9F)
	{
		SendTempSensor(3, 255, fltemp, "Feel Temperature");
	}

	if (!root["weather"].empty())
	{
		if (!root["weather"][0].empty())
		{
			if (!root["weather"][0]["id"].empty())
			{
				int condition = root["weather"][0]["id"].asInt();
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
			if (!root["weather"][0]["description"].empty())
			{
				std::string weatherdescription = root["weather"][0]["description"].asString();
				SendTextSensor(2, 1, 255, weatherdescription, "Weather Description");
			}
		}
	}


	//Wind
	if (!root["wind"].empty())
	{
		int16_t wind_degrees = -1;
		float windgust_ms = 0;

		float windspeed_ms = root["wind"]["speed"].asFloat();

		if (!root["wind"]["gust"].empty())
		{
			windgust_ms = root["wind"]["gust"].asFloat();
		}

		if (!root["wind"]["deg"].empty())
		{
			wind_degrees = root["wind"]["deg"].asInt();

			//we need to assume temp and chill temperatures are availabe to define subtype of wind device. 
			//It is possible that sometimes in the API a temperature is missing, but it should not change a device type.
			//Therefor set that temp to 0
			float wind_temp = (temp != -999.9F ? temp : 0);
			float wind_chill = (fltemp != -999.9F ? fltemp : 0); // Wind_chill is same as feels like temperature

			SendWind(4, 255, wind_degrees, windspeed_ms, windgust_ms, wind_temp, wind_chill, true, true, "Wind");
		}
	}

	//UV
	if (!root["uvi"].empty())
	{
		float uvi = root["uvi"].asFloat();
		if ((uvi < 16) && (uvi >= 0))
		{
			SendUVSensor(5, 1, 255, uvi, "UV Index");
		}
	}

	//Visibility
	if (!root["visibility"].empty() && root["visibility"].isInt())
	{
		float visibility = ((float)root["visibility"].asInt()) / 1000.0F;
		if (visibility >= 0)
		{
			SendVisibilitySensor(6, 1, 255, visibility, "Visibility");
		}
	}

	//clouds
	if (!root["clouds"].empty())
	{
		float clouds = root["clouds"]["all"].asFloat();
		SendPercentageSensor(7, 1, 255, clouds, "Clouds %");
	}

	//Rain (only present if their is rain)
	float precipitation = 0;
	if (!root["rain"].empty() && !root["rain"]["1h"].empty())
	{
		precipitation = root["rain"]["1h"].asFloat();
	}

	//Snow (only present if their is snow), add together with rain as precipitation
	if (!root["snow"].empty() && !root["snow"]["1h"].empty())
	{
		precipitation += root["snow"]["1h"].asFloat();
	}
	SendRainRateSensor(8, 255, precipitation, "Precipitation");
	m_itIsRaining = precipitation > 0;
	SendSwitch(9, 1, 255, m_itIsRaining, 0, "Is it raining/snowing", m_Name);

}
