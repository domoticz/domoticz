#include "stdafx.h"
#include "Buienradar.h"
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

#define round(a) ( int ) ( a + .5 )

#define BUIENRADAR_URL "https://data.buienradar.nl/2.0/feed/json"
#define BUIENRADAR_ACTUAL_URL "https://observations.buienradar.nl/1.0/actual/weatherstation/"

#ifdef _DEBUG
#define DEBUG_BUIENRADARR
//#define DEBUG_BUIENRADARW
#endif

#ifdef DEBUG_BUIENRADARW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_BUIENRADARR
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

//Haversine formula to calculate distance between two points

#define earthRadiusKm 6371.0

// This function converts decimal degrees to radians
double deg2rad(double deg) {
	return (deg * 3.14159265358979323846264338327950288 / 180.0);
}

/**
 * Returns the distance between two points on the Earth.
 * Direct translation from http://en.wikipedia.org/wiki/Haversine_formula
 * @param lat1d Latitude of the first point in degrees
 * @param lon1d Longitude of the first point in degrees
 * @param lat2d Latitude of the second point in degrees
 * @param lon2d Longitude of the second point in degrees
 * @return The distance between the two points in kilometers
 */
double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d) {
	double lat1r, lon1r, lat2r, lon2r, u, v;
	lat1r = deg2rad(lat1d);
	lon1r = deg2rad(lon1d);
	lat2r = deg2rad(lat2d);
	lon2r = deg2rad(lon2d);
	u = sin((lat2r - lat1r) / 2);
	v = sin((lon2r - lon1r) / 2);
	return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

CBuienRadar::CBuienRadar(const int ID)
{
	m_HwdID = ID;
	Init();
}

CBuienRadar::~CBuienRadar(void)
{
}

void CBuienRadar::Init()
{
}

bool CBuienRadar::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CBuienRadar::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	if (!m_thread)
		return false;
	m_bIsStarted = true;
	sOnConnected(this);
	return true;
}

bool CBuienRadar::StopHardware()
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

void CBuienRadar::Do_Work()
{
#ifdef DEBUG_BUIENRADARR
	GetMeterDetails();
#endif
	int sec_counter = 590;
	_log.Log(LOG_STATUS, "BuienRadar: Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 10 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
#ifdef DEBUG_BUIENRADARR
		if (sec_counter % 10 == 0)
#else
		if (sec_counter % 600 == 0)
#endif
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "BuienRadar: Worker stopped...");
}

bool CBuienRadar::WriteToHardware(const char* pdata, const unsigned char length)
{
	return false;
}

std::string CBuienRadar::GetForecastURL()
{
	std::stringstream sURL;
	//std::string szLoc = CURLEncode::URLEncode(m_Location);
	//sURL << "https://api.weather.com/v3/location/point?geocode=" << szLoc << "&language=en-US&format=json&apiKey=" << m_APIKey;
	//sURL << "http://www.BuienRadar.com/cgi-bin/findweather/getForecast?query=" << szLoc;
	return sURL.str();
}

bool CBuienRadar::FindNearestStationID()
{
	std::string Latitude = "1";
	std::string Longitude = "1";
	int nValue;
	std::string sValue;
	if (!m_sql.GetPreferencesVar("Location", nValue, sValue))
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid Location found in Settings! (Check your Latitude/Longitude!)");
		return false;
	}
	std::vector<std::string> strarray;
	StringSplit(sValue, ";", strarray);

	if (strarray.size() != 2)
		return false;

	Latitude = strarray[0];
	Longitude = strarray[1];

	if ((Latitude == "1") && (Longitude == "1"))
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid Location found in Settings! (Check your Latitude/Longitude!)");
		return false;
	}

	double my_Latitude = std::stod(Latitude);
	double my_Longitude = std::stod(Longitude);

	std::string sResult;
#ifdef DEBUG_BUIENRADARR
	sResult = ReadFile("E:\\br.json");
#else
	bool bret = HTTPClient::GET(BUIENRADAR_URL, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Error getting http data! (Check your internet connection!)");
		return false;
	}
#ifdef DEBUG_BUIENRADARW
	SaveString2Disk(sResult, "E:\\br.json");
#endif
#endif

	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received! (Check Station ID!)");
		return false;
	}

	if (root["actual"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received, or no data returned!");
		return false;
	}
	if (root["actual"]["stationmeasurements"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received, or no data returned!");
		return false;
	}


	double shortest_distance_km = 200.0;//start with 200 km
	std::string nearest_station_name("?");
	std::string nearest_station_regio("?");

	for (const auto itt : root["actual"]["stationmeasurements"])
	{
		if (itt["temperature"].empty())
			continue;

		double lat = itt["lat"].asDouble();
		double lon = itt["lon"].asDouble();

		double distance_km = distanceEarth(
			my_Latitude, my_Longitude,
			lat, lon
		);
		if (distance_km < shortest_distance_km)
		{
			shortest_distance_km = distance_km;
			m_iNearestStationID = itt["stationid"].asInt();
			nearest_station_name = itt["stationname"].asString();
			nearest_station_regio = itt["regio"].asString();
		}
	}
	if (m_iNearestStationID == -1)
	{
		_log.Log(LOG_ERROR, "BuienRadar: No (nearby) station found!");
		return false;
	}
	_log.Log(LOG_STATUS, "BuienRadar: Nearest station: %s (%s), ID: %d", nearest_station_name.c_str(), nearest_station_regio.c_str(), m_iNearestStationID);
	return true;
}

void CBuienRadar::GetMeterDetails()
{
	if (m_iNearestStationID == -1)
	{
		//Because BuienRadar always sends back results for all stations it knows,
		//we need to find our nearest station
		if (!FindNearestStationID())
		{
			return;
		}
	}
	std::string sResult;
#ifdef DEBUG_BUIENRADARR
	sResult = ReadFile("E:\\br_actual.json");
#else
	std::string szUrl = BUIENRADAR_ACTUAL_URL + std::to_string(m_iNearestStationID);
	bool bret = HTTPClient::GET(szUrl, sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Error getting http data! (Check your internet connection!)");
		return;
	}
#ifdef DEBUG_BUIENRADARW
	SaveString2Disk(sResult, "E:\\br_actual.json");
#endif
#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received! (Check Station ID!)");
		return;
	}

	if (root["stationid"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received, or no data returned!");
		return;
	}
	if (root["temperature"].empty() == true)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received, or no data returned!");
		return;
	}

	int stationID = root["stationid"].asInt();
	if (stationID != m_iNearestStationID)
	{
		_log.Log(LOG_ERROR, "BuienRadar: Invalid data received, or no data returned!");
		return;
	}

	//timestamp : "2019-08-22T08:30:00"
	//iconurl : "https://www.buienradar.nl/resources/images/icons/weather/30x30/a.png"
	//graphUrl : "https://www.buienradar.nl/nederland/weerbericht/weergrafieken/a"

	float temp = -999.9f;
	int humidity = 0;
	float barometric = 0;
	int barometric_forcast = wsbaroforcast_unknown;

	if (!root["temperature"].empty())
	{
		temp = root["temperature"].asFloat();
	}
	if (!root["humidity"].empty())
	{
		humidity = root["humidity"].asInt();
	}
	if (!root["airpressure"].empty())
	{
		barometric = root["airpressure"].asFloat();
		if (barometric < 1000)
		{
			barometric_forcast = wsbaroforcast_rain;
			if (temp != -999.9f)
			{
				if (temp <= 0)
					barometric_forcast = wsbaroforcast_snow;
			}
		}
		else if (barometric < 1020)
			barometric_forcast = wsbaroforcast_cloudy;
		else if (barometric < 1030)
			barometric_forcast = wsbaroforcast_some_clouds;
		else
			barometric_forcast = wsbaroforcast_sunny;
	}
	if (
		(temp != -999.9f)
		&& (humidity != 0)
		&& (barometric != 0)
		)
	{
		SendTempHumBaroSensorFloat(1, 255, temp, humidity, barometric, barometric_forcast, "TempHumBaro");
	}
	else if (
		(temp != -999.9f)
		&& (humidity != 0)
		)
	{
		SendTempHumSensor(1, 255, temp, humidity, "TempHum");
	}
	else if (temp != -999.9f)
	{
		SendTempSensor(1, 255, temp, "Temp");
	}

	if (!root["groundtemperature"].empty())
	{
		float temp = root["groundtemperature"].asFloat();
		SendTempSensor(2, 255, temp, "Ground Temperature (10 cm)");
	}
	if (!root["feeltemperature"].empty())
	{
		float temp = root["feeltemperature"].asFloat();
		SendTempSensor(3, 255, temp, "Feel Temperature");
	}


	if (!root["weatherdescription"].empty())
	{
		std::string weatherdescription = root["weatherdescription"].asString();
		SendTextSensor(1, 1, 255, weatherdescription, "Weather Description");
	}

	if (!root["winddirectiondegrees"].empty())
	{
		int wind_direction = root["winddirectiondegrees"].asInt();
		float wind_gusts = root["windgusts"].asFloat();
		float wind_speed = root["windspeed "].asFloat();

		float wind_chill;
		if ((temp < 10.0) && (wind_speed >= 1.4))
			wind_chill = 0; //if we send 0 as chill, it will be calculated
		else
			wind_chill = temp;

		SendWind(1, 255, wind_direction, wind_speed, wind_gusts, temp, wind_chill, temp != -999.9f, true, "Wind");
	}

	if (!root["visibility"].empty())
	{
		float visibility = root["visibility"].asFloat() / 1000.0F;
		SendVisibilitySensor(1, 1, 255, visibility, "Visibility");
	}

	if (!root["sunpower"].empty())
	{
		float sunpower = root["sunpower"].asFloat() / 100.0F;
		SendUVSensor(1, 1, 255, sunpower, "UV");
	}

	float total_rain_today = -1;
	float total_rain_last_hour = 0;
	if (!root["precipitation"].empty())
	{
		total_rain_today = root["precipitation"].asFloat();
	}
	else if (!root["precipation"].empty())
	{
		total_rain_today = root["precipation"].asFloat();
	}
	if (!root["rainFallLastHour"].empty())
	{
		total_rain_last_hour = root["rainFallLastHour"].asFloat();
	}
	if (total_rain_today != -1)
	{
		SendRainSensorWU(1, 255, total_rain_today, total_rain_last_hour, "Rain");
	}
}

