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
#define OWM_Get_City_Details "https://api.openweathermap.org/data/2.5/weather?"
#define OWM_icon_URL "https://openweathermap.org/img/wn/"	// for example 10d@4x.png
#define OWM_forecast_URL "https://openweathermap.org/city/"

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location, const int adddayforecast, const int addhourforecast) :
	m_APIKey(APIKey),
	m_Location(Location),
	m_Language("en"),
	m_add_dayforecast(adddayforecast),
	m_add_hourforecast(addhourforecast)
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
	std::string sResult;
	std::stringstream sURL;

	sURL << OWM_Get_City_Details;
	if (IsCityName)
		sURL << "q=" << Location;
	else
		sURL << "id=" << Location;
	sURL << "&appid=" << m_APIKey;
	sURL << "&units=metric" << "&lang=" << m_Language;

	Debug(DEBUG_NORM, "Get data from %s", sURL.str().c_str());

	try
	{
		if (!HTTPClient::GET(sURL.str(), sResult))
		{
			Log(LOG_ERROR, "Error getting http data!");
			return false;
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error getting http data!");
		return false;
	}

#ifdef DEBUG_OPENWEATHERMAP_WRITE
	SaveString2Disk(sResult, "OpenWeatherMap_city.json");
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
	cityid = root["id"].asUInt();

	return true;
}

bool COpenWeatherMap::StartHardware()
{
	std::string sValue, sLatitude, sLongitude;
	std::vector<std::string> strarray;
	uint32_t cityid;
	Debug(DEBUG_NORM, "Got location parameter %s", m_Location.c_str());
	Debug(DEBUG_NORM, "Starting with setting %d, %d", m_add_dayforecast, m_add_hourforecast);

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

					if (!((sLatitude == "1") && (sLongitude == "1")))
					{
						m_Lat = std::stod(sLatitude);
						m_Lon = std::stod(sLongitude);

						Log(LOG_STATUS, "Using Domoticz home location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
					}
					else
					{
						Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
					}
				}
				else
				{
					Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
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

				char* p;
				double converted = strtod(sLatitude.c_str(), &p);
				if (*p)
				{
					// conversion failed because the input wasn't a number
					// assume it is a city/country combination
					Log(LOG_STATUS, "Using specified location (City %s)!", m_Location.c_str());
					double lat, lon;
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
					m_Lat = std::stod(sLatitude);
					m_Lon = std::stod(sLongitude);

					Log(LOG_STATUS, "Using specified location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
				}
			}
			else if (m_Location.find("lat=") == 0)
			{
				StringSplit(m_Location, "&", strarray);
				if (strarray.size() == 2)
				{
					sLatitude = strarray[0];
					sLongitude = strarray[1];
					if ((sLatitude.find("lat=") == 0) && (sLongitude.find("lon=") == 0))
					{
						m_Lat = std::stod(sLatitude.substr(4));
						m_Lon = std::stod(sLongitude.substr(4));
						Log(LOG_STATUS, "Using specified location (Lon %s, Lat %s)!", sLongitude.c_str(), sLatitude.c_str());
					}
					else
						Log(LOG_ERROR, "Invalid Location specified! (Check your StationId/CityId or Latitude, Longitude!)");
				}
				else
					Log(LOG_ERROR, "Invalid Location specified! (Check your StationId/CityId or Latitude, Longitude!)");
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

	//Forecast URL
	if (m_CityID > 0)
	{
		std::stringstream ss;
		ss << OWM_forecast_URL << m_CityID;
		m_ForecastURL = ss.str();
	}

	RequestStart();

	m_thread = std::make_shared<std::thread>(&COpenWeatherMap::Do_Work, this);
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

std::string COpenWeatherMap::GetHourFromUTCtimestamp(const uint8_t hournr, const std::string &UTCtimestamp)
{
	std::string sHour = "Unknown";

	time_t t = (time_t)strtol(UTCtimestamp.c_str(), nullptr, 10);
	std::string sDate = ctime(&t);

	std::vector<std::string> strarray;

	StringSplit(sDate," ", strarray);
	if (!(strarray.size() >= 5 && strarray.size() <= 6))
	{
		Debug(DEBUG_NORM, "Unable to determine hour for hour %d from timestamp %s (got string %s)", hournr, UTCtimestamp.c_str(), sDate.c_str());
		return sHour;
	}

	size_t iHourPos = strarray.size() - 2;
	sHour = strarray[iHourPos];
	strarray.clear();
	StringSplit(sHour,":", strarray);
	if (strarray.size() != 3)
	{
		Debug(DEBUG_NORM, "Unable to determine hour for hour %d from timestamp %s (found hour %s)", hournr, UTCtimestamp.c_str(), sHour.c_str());
		return "Unknown";
	}

	sHour = strarray[0];
	Debug(DEBUG_NORM, "Determining hour for hour %d from timestamp %s (string %s)", hournr, UTCtimestamp.c_str(), sHour.c_str());

	return sHour;
}

std::string COpenWeatherMap::GetDayFromUTCtimestamp(const uint8_t daynr, const std::string &UTCtimestamp)
{
	std::string sDay = "Unknown";

	time_t t = (time_t)strtol(UTCtimestamp.c_str(), nullptr, 10);
	std::string sDate = ctime(&t);

	std::vector<std::string> strarray;

	StringSplit(sDate," ", strarray);
	if (!(strarray.size() >= 5 && strarray.size() <= 6))
	{
		Debug(DEBUG_NORM, "Unable to determine day for day %d from timestamp %s (got string %s)", daynr, UTCtimestamp.c_str(), sDate.c_str());
		return sDay;
	}
	
	switch (daynr) {
		case 0:
			sDay = "Today";
			break;
		case 1:
			sDay = "Tomorrow";
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			sDay = strarray[0];
			break;
		default:
			sDay = "Day " + std::to_string(daynr);
	}

	return sDay;
}

bool COpenWeatherMap::ProcessForecast(Json::Value &forecast, const std::string &period, const std::string &periodname, const uint8_t count, const int startNodeID)
{
	bool bResult = false;

	if (!forecast.empty())
	{
		try
		{
			float rainmm = 9999.00F;
			float uvi = -999.9F;
			float mintemp = -999.9F;
			float maxtemp = -999.9F;

			float pop = forecast["pop"].asFloat();
			float clouds = forecast["clouds"].asFloat();
			float windspeed_ms = forecast["wind_speed"].asFloat();
			int wind_degrees = forecast["wind_deg"].asInt();
			float barometric = forecast["pressure"].asFloat();
			uint8_t humidity = (uint8_t) forecast["humidity"].asUInt();
			std::string wdesc = forecast["weather"][0]["description"].asString();
			std::string wicon = forecast["weather"][0]["icon"].asString();

			// Min and max or just max temp?
			if (forecast["temp"].isObject())
			{
				mintemp = forecast["temp"]["min"].asFloat();
				maxtemp = forecast["temp"]["max"].asFloat();
			}
			else
			{
				maxtemp = forecast["temp"].asFloat();
			}
			int barometric_forecast = GetForecastFromBarometricPressure(barometric, maxtemp);

			//UV Index (only if present)
			if (!forecast["uvi"].empty())
			{
				uvi = forecast["uvi"].asFloat();
			}

			//Rain (only present if there is rain (or snow))
			if (!forecast["rain"].empty())
			{
				if (!forecast["rain"].isObject())
				{
					rainmm = forecast["rain"].asFloat();
				}
				else if (!forecast["rain"]["1h"].empty())
				{
					rainmm = forecast["rain"]["1h"].asFloat();
				}
			}
			if (!forecast["snow"].empty())
			{
				if (!forecast["snow"].isArray())
				{
					rainmm = rainmm + forecast["snow"].asFloat();
				}
				else if (!forecast["snow"]["1h"].empty())
				{
					rainmm = rainmm + forecast["snow"]["1h"].asFloat();
				}
			}

			int NodeID = startNodeID + count * 16;
			std::stringstream sName;

			sName << "TempHumBaro " << period << " " << (count + 0);
			SendTempHumBaroSensorFloat(NodeID, 255, maxtemp, humidity, barometric, barometric_forecast, sName.str());

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "Weather Description " << period << " " << (count + 0);
			SendTextSensor(NodeID, 1, 255, wdesc, sName.str());
			sName.str("");
			sName.clear();
			sName << "Weather Description " << period << " " << (count + 0) << " Icon";
			SendTextSensor(NodeID, 2, 255, wicon, sName.str());
			sName.str("");
			sName.clear();
			sName << "Weather Description " << period << " " << (count + 0) << " Name";
			SendTextSensor(NodeID, 3, 255, periodname, sName.str());

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "Minumum Temperature " << period << " " << (count + 0);
			if (mintemp != -999.9F)
			{
				SendTempSensor(NodeID, 255, mintemp, sName.str());
			}

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "Wind " << period << " " << (count + 0);
			SendWind(NodeID, 255, wind_degrees, windspeed_ms, 0, 0, 0, false, false, sName.str());

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "UV Index " << period << " " << (count + 0);
			if (uvi != -999.9F)
			{
				SendUVSensor(NodeID, 1, 255, uvi, sName.str());
			}

			NodeID++;
			// We do not have visibility forecasts

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "Clouds % " << period << " " << (count + 0);
			SendPercentageSensor(NodeID, 1, 255, clouds, sName.str());

			NodeID++;;
			if ((rainmm != 9999.00F) && (rainmm >= 0.00F))
			{
				sName.str("");
				sName.clear();
				sName << "Rain(Snow) " << period << " " << (count + 0);
				SendRainRateSensor(NodeID, 255, rainmm, sName.str());
			}

			NodeID++;;
			sName.str("");
			sName.clear();
			sName << "Precipitation " << period << " " << (count + 0);
			SendPercentageSensor(NodeID, 1, 255, (pop * 100), sName.str());

			bResult = true;
			Debug(DEBUG_NORM, "Processed forecast for period %d: %s - %f - %f - %f",count, wdesc.c_str(), maxtemp, mintemp, pop);
		}
		catch(const std::exception& e)
		{
			Debug(DEBUG_NORM, "Processing forecast crashed for period %d on %s",count, e.what());
		}
	}

	return bResult;
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
	std::stringstream sURL;

	sURL << OWM_onecall_URL;
	sURL << "lat=" << m_Lat << "&lon=" << m_Lon;
	sURL << "&exclude=minutely";
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
	SaveString2Disk(sResult, "OpenWeatherMap.json");
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
	if (root["current"].empty())
	{
		Log(LOG_ERROR, "Invalid data received, could not find current weather data!");
		return;
	}

	//Current values
	Json::Value current;
	float temp = -999.9F;
	float fltemp = -999.9F;

	current = root["current"];
	int humidity = 0;
	float barometric = 0;
	int barometric_forecast = 0;
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
		barometric_forecast = GetForecastFromBarometricPressure(barometric, temp);
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

			//we need to assume temp and chill temperatures are availabe to define subtype of wind device. 
			//It is possible that sometimes in the API a temperature is missing, but it should not change a device type.
			//Therefor set that temp to 0
			float wind_temp = (temp != -999.9F ? temp : 0);
			float wind_chill = (fltemp != -999.9F ? fltemp : 0); // Wind_chill is same as feels like temperature

			SendWind(4, 255, wind_degrees, windspeed_ms, windgust_ms, wind_temp, wind_chill, true, true, "Wind");
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
		float visibility = ((float)current["visibility"].asInt()) / 1000.0F;
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

	//Rain (only present if their is rain)
	float precipitation = 0;
	if (!current["rain"].empty() && !current["rain"]["1h"].empty())
	{
		precipitation = current["rain"]["1h"].asFloat();
	}

	//Snow (only present if their is snow), add together with rain as precipitation
	if (!current["snow"].empty() && !current["snow"]["1h"].empty())
	{
		precipitation += current["snow"]["1h"].asFloat();
	}
	SendRainRateSensor(8, 255, precipitation, "Precipitation");
	m_itIsRaining = precipitation > 0;
	SendSwitch(9, 1, 255, m_itIsRaining, 0, "Is it raining/snowing", m_Name);

	// Process daily forecast data if available
	if (root["daily"].empty())
	{
		if (m_add_dayforecast)
			Log(LOG_STATUS, "Could not find daily weather forecast data!");
	}
	else if (m_add_dayforecast)
	{
		Json::Value dailyfc;
		uint8_t iDay = 0;

		dailyfc = root["daily"];
		do
		{
			if (dailyfc[iDay]["dt"].empty())
			{
				Log(LOG_STATUS, "Processing daily forecast failed (unexpected structure)!");
				break;
			}
			std::string sDay = GetDayFromUTCtimestamp(iDay, dailyfc[iDay]["dt"].asString());
			Debug(DEBUG_NORM, "Processing daily forecast for %s (%s)", dailyfc[iDay]["dt"].asString().c_str(), sDay.c_str());

			Json::Value curday = dailyfc[iDay];

			if (!ProcessForecast(curday, "Day", sDay, iDay, 17))
			{
				Log(LOG_STATUS, "Processing daily forecast for day %d failed!", iDay);
			}
			iDay++;
		}
		while (!dailyfc[iDay].empty());
		Debug(DEBUG_NORM, "Processed %d daily forecasts",iDay);
	}

	// Process hourly forecast data if available
	if (root["hourly"].empty())
	{
		if (m_add_hourforecast)
			Log(LOG_STATUS, "Could not find hourly weather forecast data!");
	}
	else if (m_add_hourforecast)
	{
		Json::Value hourlyfc;
		uint8_t iHour = 0;

		hourlyfc = root["hourly"];
		do
		{
			if (hourlyfc[iHour]["dt"].empty())
			{
				Log(LOG_STATUS, "Processing hourly forecast failed (unexpected structure)!");
				break;
			}
			std::string sHour = GetHourFromUTCtimestamp(iHour, hourlyfc[iHour]["dt"].asString());
			Debug(DEBUG_NORM, "Processing hourly forecast for %s (%s)", hourlyfc[iHour]["dt"].asString().c_str(), sHour.c_str());

			Json::Value curhour = hourlyfc[iHour];

			if (!ProcessForecast(curhour, "Hour", sHour, iHour, 257))
			{
				Log(LOG_STATUS, "Processing hourly forecast for hour %d failed!", iHour);
			}
			iHour++;
		}
		while (!hourlyfc[iHour].empty());
		Debug(DEBUG_NORM, "Processed %d hourly forecasts",iHour);
	}
}
