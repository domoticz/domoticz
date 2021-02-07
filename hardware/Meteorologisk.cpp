#include "stdafx.h"
#include "Meteorologisk.h"
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

CMeteorologisk::CMeteorologisk(const int ID, const std::string &Location) :
	m_Location(Location)
{
	m_HwdID=ID;
	Init();
}

void CMeteorologisk::Init()
{
}

bool CMeteorologisk::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
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
	m_bIsStarted=false;
	return true;
}

void CMeteorologisk::Do_Work()
{
	Log(LOG_STATUS, "Started...");

	int sec_counter = 290;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % 300 == 0)
		{
			GetMeterDetails();
		}
	}
	Log(LOG_STATUS,"Worker stopped...");
}

bool CMeteorologisk::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

std::string CMeteorologisk::GetForecastURL()
{
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL << "https://darksky.net/#/f/" << szLoc;
	return sURL.str();
}

void CMeteorologisk::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_MeteorologiskR
	sResult=ReadFile("/tmp/Meteorologisk.json");
#else
	std::stringstream sURL;
	std::string szLoc = m_Location;
	std::string Latitude = "1";
	std::string Longitude = "1";

	// Try the hardware location first, then fallback on the global settings.s
	if(!szLoc.empty())
	{
		std::vector<std::string> strarray;
		StringSplit(szLoc, ";", strarray);

		Latitude = strarray[0];
		Longitude = strarray[1];
	}

	if(Latitude == "1" || Longitude == "1")
	{
		if(szLoc.empty()){
			Log(LOG_ERROR, "No location fount for this hardware. Falling back to global domoticz location settings.");
		} else {
			Log(LOG_ERROR, "Hardware location not correct. Should be of the form 'Latitude;Longitude'. Falling back to global domoticz location settings.");
		}

		int nValue;
		std::string sValue;
		if (!m_sql.GetPreferencesVar("Location", nValue, sValue))
		{
			Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			return;
		}

		std::vector<std::string> strarray;
		StringSplit(sValue, ";", strarray);

		if (strarray.size() != 2)
			return;

		Latitude = strarray[0];
		Longitude = strarray[1];

		if ((Latitude == "1") && (Longitude == "1"))
		{
			Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			return;
		}

		Log(LOG_STATUS, "using Domoticz location settings (%s,%s)", Latitude.c_str(), Longitude.c_str());
	} else
	{
		Log(LOG_STATUS, "using hardware location settings (%s,%s)", Latitude.c_str(), Longitude.c_str());
	}

	sURL << "https://api.met.no/weatherapi/locationforecast/2.0/.json?lat=" << Latitude << "&lon=" << Longitude;

	try
	{
		if (!HTTPClient::GET(sURL.str(), sResult))
		{
			Log(LOG_ERROR, "Error getting http data!.");
			return;
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error getting http data!");
		return;
	}
#ifdef DEBUG_MeteorologiskW
	SaveString2Disk(sResult, "/tmp/Meteorologisk.json");
#endif

#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR,"Invalid data received! Check Location, use a City or GPS Coordinates (xx.yyyy,xx.yyyyy)");
		return;
	}
	if (root["properties"].empty()==true || root["properties"]["timeseries"].empty()==true)
	{
		Log(LOG_ERROR,"Invalid data received, or unknown location!");
		return;
	}

	float temperature;
	int humidity=0;
	float barometric=0;
	int barometric_forcast=baroForecastNoInfo;

	Json::Value timeseries = root["properties"]["timeseries"];

	Json::Value selectedTimeserie = 0;
	time_t now = time(nullptr);

	for(int i = 0; i < (int)timeseries.size(); i++)
	{
		Json::Value timeserie = timeseries[i];
		std::string stime = timeserie["time"].asString();

		time_t parsedDate;
		struct tm parsedDateTm;

		if(ParseISOdatetime(parsedDate, parsedDateTm, stime))
		{
			double diff = difftime(mktime(&parsedDateTm), now);
			if(diff > 0)
			{
				selectedTimeserie = timeserie;
				i = timeseries.size();
			}
		}
	}

	if(selectedTimeserie == 0){
		Log(LOG_ERROR,"Invalid data received, or unknown location!");
		return;
	}

	Json::Value instantData = selectedTimeserie["data"]["instant"]["details"];
	Json::Value nextOneHour = selectedTimeserie["data"]["next_1_hours"];

	//The api already provides the temperature in celcius.
	temperature=instantData["air_temperature"].asFloat();

	if (instantData["relative_humidity"].empty()==false)
	{
		humidity=round(instantData["relative_humidity"].asFloat());
	}
	if (instantData["air_pressure_at_sea_level"].empty()==false)
	{
		barometric = instantData["air_pressure_at_sea_level"].asFloat();
		if (barometric<1000)
			barometric_forcast=baroForecastRain;
		else if (barometric<1020)
			barometric_forcast=baroForecastCloudy;
		else if (barometric<1030)
			barometric_forcast=baroForecastPartlyCloudy;
		else
			barometric_forcast=baroForecastSunny;

		if (!nextOneHour["summary"]["symbol_code"].empty())
		{
			//https://api.met.no/weatherapi/weathericon/2.0/documentation
			std::string forcasticon=nextOneHour["summary"]["symbol_code"].asString();
			if ((forcasticon=="partlycloudy_day")||(forcasticon=="partlycloudy_night"))
			{
				barometric_forcast=baroForecastPartlyCloudy;
			}
			else if (forcasticon=="cloudy")
			{
				barometric_forcast=baroForecastCloudy;
			}
			else if ((forcasticon=="clearsky_day")||(forcasticon=="clearsky_night"))
			{
				barometric_forcast=baroForecastSunny;
			}
			else if ((forcasticon=="rain")||(forcasticon=="snow"))
			{
				barometric_forcast=baroForecastRain;
			}
		}
	}

	if (barometric!=0)
	{
		//Add temp+hum+baro device
		SendTempHumBaroSensor(1, 255, temperature, humidity, barometric, barometric_forcast, "THB");
	}
	else if (humidity!=0)
	{
		//add temp+hum device
		SendTempHumSensor(1, 255, temperature, humidity, "TempHum");
	}
	else
	{
		//add temp device
		SendTempSensor(1, 255, temperature, "Temperature");
	}

	//Wind
	if(!instantData["wind_from_direction"].empty() && !instantData["wind_speed"].empty() && !instantData["wind_speed_of_gust"].empty())
	{
		int wind_direction = instantData["wind_from_direction"].asInt();
		float wind_gusts = instantData["wind_speed_of_gust"].asFloat();
		float wind_speed = instantData["wind_speed"].asFloat();

		float wind_chill;
		if ((temperature < 10.0) && (wind_speed >= 1.4))
			wind_chill = 0; //if we send 0 as chill, it will be calculated
		else
			wind_chill = temperature;

		SendWind(1, 255, wind_direction, wind_speed, wind_gusts, temperature, wind_chill, temperature != -999.9F, true, "Wind");
	}

	//UV
	if (instantData["ultraviolet_index_clear_sky"].empty() == false)
	{
		float UV = instantData["ultraviolet_index_clear_sky"].asFloat();
		if ((UV < 16) && (UV >= 0))
		{
			SendUVSensor(0, 1, 255, UV, "UV Index");
		}
	}

	//Cloud Cover
	if (instantData["cloud_area_fraction"].empty() == false)
	{
		float cloudcover = instantData["cloud_area_fraction"].asFloat();
		if (cloudcover >= 0.0F)
		{
			SendPercentageSensor(1, 0, 255, cloudcover, "Cloud Cover");
		}
	}

	//Rain
	if (!nextOneHour["details"]["precipitation_amount"].empty())
	{
		float rainrateph = nextOneHour["details"]["precipitation_amount"].asFloat();
		if ((rainrateph != -9999.00F) && (rainrateph >= 0.00F))
		{
			   SendRainRateSensor(1, 255, rainrateph, "Rain");
		}
	}
}

