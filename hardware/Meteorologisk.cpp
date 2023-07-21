#include "stdafx.h"
#include "Meteorologisk.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#define round(a) (int)(a + .5)

#ifdef _DEBUG
//#define DEBUG_MeteorologiskR
//#define DEBUG_MeteorologiskW
#endif

#ifdef DEBUG_MeteorologiskW
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
#ifdef DEBUG_MeteorologiskR
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

#define Meteorologisk_URL "https://api.met.no/weatherapi/locationforecast/2.0/complete.json"
#define Meteorologisk_forecast_URL "https://darksky.net/#/f/"

CMeteorologisk::CMeteorologisk(const int ID, const std::string &Location)
	: m_Location(Location)
{
	m_HwdID = ID;
}

void CMeteorologisk::Init()
{
	std::stringstream sURL;
	m_Lat = 1;
	m_Lon = 1;

	// Try the hardware location first, then fallback on the global settings
	if (!m_Location.empty())
	{
		std::vector<std::string> strarray;
		if (m_Location != "0")
		{
			StringSplit(m_Location, ",", strarray);
			if (strarray.size() == 2)
			{
				m_Lat = strtod(strarray[0].c_str(), nullptr);
				m_Lon = strtod(strarray[1].c_str(), nullptr);
			}
			else
			{
				StringSplit(m_Location, ";", strarray);
				if (strarray.size() == 2)
				{
					m_Lat = strtod(strarray[0].c_str(), nullptr);
					m_Lon = strtod(strarray[1].c_str(), nullptr);
					Log(LOG_STATUS, "Found Location with semicolon as seperator instead of expected comma.");
				}
			}
		}
		else
		{
			int nValue;
			std::string sValue;
			if (!m_sql.GetPreferencesVar("Location", nValue, sValue))
			{
				Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			}
			else
			{
				StringSplit(sValue, ";", strarray);
				if (strarray.size() == 2)
				{
					m_Lat = strtod(strarray[0].c_str(), nullptr);
					m_Lon = strtod(strarray[1].c_str(), nullptr);
					Log(LOG_STATUS, "Using Domoticz default location from Settings as Location.");
				}
			}
		}
	}

	if (!(m_Lat == 1 || m_Lon == 1))
	{
		sURL << Meteorologisk_URL << "?lat=" << m_Lat << "&lon=" << m_Lon;
		m_URL = sURL.str();
	}

	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL.str(std::string());
	sURL.clear();
	sURL << Meteorologisk_forecast_URL << szLoc;
	m_ForecastURL = sURL.str();
	Debug(DEBUG_HARDWARE, "Meteorologisk: Set Forecast URL to: %s", m_ForecastURL.c_str());
}

bool CMeteorologisk::StartHardware()
{
	Init();

	RequestStart();

	// Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CMeteorologisk::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

#define Meteorologisk_Poll_Interval 300

void CMeteorologisk::Do_Work()
{
	Log(LOG_STATUS, "Started...");
	Debug(DEBUG_NORM, "Metereologisk module started with Location parameters Latitude %f, Longitude %f!", m_Lat, m_Lon);

	int sec_counter = Meteorologisk_Poll_Interval - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % Meteorologisk_Poll_Interval == 0)
		{
			if (!m_URL.empty())
			{
				try
				{
					GetMeterDetails();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Unhandled failure getting/parsing data!");
				}
			}
			else
			{
				Log(LOG_STATUS, "Unable to properly run due to missing or incorrect Location parameters (Latitude, Longitude)!");
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool CMeteorologisk::WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

std::string CMeteorologisk::GetForecastURL()
{
	return m_ForecastURL;
}

void CMeteorologisk::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_MeteorologiskR
	sResult = ReadFile("E:\\Meteorologisk.json");
#else
	try
	{
		if (!HTTPClient::GET(m_URL, sResult))
		{
			Log(LOG_ERROR, "Error getting http data!.");
			Debug(DEBUG_RECEIVED, "Meteorologisk: Received .%s.", sResult.c_str());
			return;
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Recovered from crash during attempt to get http data!");
		return;
	}
#ifdef DEBUG_MeteorologiskW
	SaveString2Disk(sResult, "E:\\Meteorologisk.json");
#endif

#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received! Check Location, use Latitude, Longitude Coordinates (xx.yyyy,xx.yyyyy)!");
		Debug(DEBUG_NORM, "Meteorologisk: Received invalid JSON data .%s.", sResult.c_str());
		return;
	}
	Debug(DEBUG_RECEIVED, "Meteorologisk: Received JSON data .%s.", root.toStyledString().c_str());
	if (root["properties"].empty() == true || root["properties"]["timeseries"].empty() == true)
	{
		Log(LOG_ERROR, "Unexpected data structure received!");
		return;
	}

	Json::Value timeseries = root["properties"]["timeseries"];

	int iSelectedTimeserie = -1;
	time_t now = mytime(nullptr);

	for (int i = 0; i < (int)timeseries.size(); i++)
	{
		std::string stime = timeseries[i]["time"].asString();

		time_t parsedDate;
		struct tm parsedDateTm;

		if (ParseISOdatetime(parsedDate, parsedDateTm, stime))
		{
			double diff = difftime(mktime(&parsedDateTm), now);
			if (diff >= 0)
			{
				break;
			}
			iSelectedTimeserie = i;
		}
	}

	if ((iSelectedTimeserie < 0) || (iSelectedTimeserie >= (int)timeseries.size()))
	{
		Log(LOG_ERROR, "Invalid data received, or unknown location!");
		return;
	}
	std::string picked_datetime = timeseries[iSelectedTimeserie]["time"].asString();

	Json::Value instantData = timeseries[iSelectedTimeserie]["data"]["instant"]["details"];
	Json::Value nextOneHour = timeseries[iSelectedTimeserie]["data"]["next_1_hours"];

	float temperature = 0;
	int humidity = 0;
	float barometric = 0;
	int barometric_forcast = baroForecastNoInfo;

	// The api already provides the temperature in celcius.
	temperature = instantData["air_temperature"].asFloat();

	if (instantData["relative_humidity"].empty() == false)
	{
		humidity = round(instantData["relative_humidity"].asFloat());
	}
	if (instantData["air_pressure_at_sea_level"].empty() == false)
	{
		barometric = instantData["air_pressure_at_sea_level"].asFloat();
		if (barometric < 1000)
			barometric_forcast = baroForecastRain;
		else if (barometric < 1020)
			barometric_forcast = baroForecastCloudy;
		else if (barometric < 1030)
			barometric_forcast = baroForecastPartlyCloudy;
		else
			barometric_forcast = baroForecastSunny;

		if (!nextOneHour["summary"]["symbol_code"].empty())
		{
			// https://api.met.no/weatherapi/weathericon/2.0/documentation
			std::string forcasticon = nextOneHour["summary"]["symbol_code"].asString();
			if ((forcasticon == "partlycloudy_day") || (forcasticon == "partlycloudy_night"))
			{
				barometric_forcast = baroForecastPartlyCloudy;
			}
			else if (forcasticon == "cloudy")
			{
				barometric_forcast = baroForecastCloudy;
			}
			else if ((forcasticon == "clearsky_day") || (forcasticon == "clearsky_night"))
			{
				barometric_forcast = baroForecastSunny;
			}
			else if ((forcasticon == "rain") || (forcasticon == "snow"))
			{
				barometric_forcast = baroForecastRain;
			}
		}
	}

	if ((barometric != 0) && (humidity != 0))
	{
		// Add temp+hum+baro device
		SendTempHumBaroSensor(1, 255, temperature, humidity, barometric, barometric_forcast, "THB");
	}
	else if (humidity != 0)
	{
		// add temp+hum device
		SendTempHumSensor(1, 255, temperature, humidity, "TempHum");
	}
	else
	{
		// add temp device
		SendTempSensor(1, 255, temperature, "Temperature");
	}

	// Wind
	if (!instantData["wind_from_direction"].empty() && !instantData["wind_speed"].empty() && !instantData["wind_speed_of_gust"].empty())
	{
		int wind_direction = instantData["wind_from_direction"].asInt();
		float wind_gusts = instantData["wind_speed_of_gust"].asFloat();
		float wind_speed = instantData["wind_speed"].asFloat();

		float wind_chill;
		if ((temperature < 10.0) && (wind_speed >= 1.4))
			wind_chill = 0; // if we send 0 as chill, it will be calculated
		else
			wind_chill = temperature;

		SendWind(1, 255, wind_direction, wind_speed, wind_gusts, temperature, wind_chill, temperature != -999.9F, true, "Wind");
	}

	// UV
	if (instantData["ultraviolet_index_clear_sky"].empty() == false)
	{
		float UV = instantData["ultraviolet_index_clear_sky"].asFloat();
		if ((UV < 16) && (UV >= 0))
		{
			SendUVSensor(0, 1, 255, UV, "UV Index");
		}
	}

	// Cloud Cover
	if (instantData["cloud_area_fraction"].empty() == false)
	{
		float cloudcover = instantData["cloud_area_fraction"].asFloat();
		if (cloudcover >= 0.0F)
		{
			SendPercentageSensor(1, 0, 255, cloudcover, "Cloud Cover");
		}
	}

	// Rain
	if (!nextOneHour["details"]["precipitation_amount"].empty())
	{
		float rainrateph = nextOneHour["details"]["precipitation_amount"].asFloat();
		if ((rainrateph != -9999.00F) && (rainrateph >= 0.00F))
		{
			SendRainRateSensor(1, 255, rainrateph, "Rain");
		}
	}
}
