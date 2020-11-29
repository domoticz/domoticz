#include "stdafx.h"
#include "Buienradar.h"
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
#include <sstream>
#include <iomanip>

#define BUIENRADAR_URL "https://data.buienradar.nl/2.0/feed/json"
#define BUIENRADAR_ACTUAL_URL "https://observations.buienradar.nl/1.0/actual/weatherstation/" //station_id
#define BUIENRADAR_GRAFIEK_URL "https://www.buienradar.nl/nederland/weerbericht/weergrafieken/" //station_id
//#define BUIENRARA_GRAFIEK_HISTORY_URL "https://graphdata.buienradar.nl/1.0/actualarchive/weatherstation/6370?startDate=2019-09-15"

#define BUIENRADAR_RAIN "https://gpsgadget.buienradar.nl/data/raintext/?lat=" // + m_szMyLatitude + "&lon=" + m_szMyLongitude;

#define RAINSHOWER_LEADTIME 3
#define RAINSHOWER_DURATION 4
#define RAINSHOWER_AVERAGE_MMH 5
#define RAINSHOWER_MAX_MMH 6

#ifdef _DEBUG
// #define DEBUG_BUIENRADARR
// #define DEBUG_BUIENRADARW
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
	std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return "";
	std::string sResult((std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));
	file.close();
	return sResult;
}
#endif

CBuienRadar::CBuienRadar(const int ID, const int iForecast, const int iThreshold, const std::string& Location)
{
	m_HwdID = ID;
	m_iForecast = (iForecast >= 5) ? iForecast : 15;
	m_iThreshold = (iThreshold > 0) ? iThreshold : 25;

	if (!Location.empty())
	{
		std::vector<std::string> strarray;
		StringSplit(Location, ",", strarray);
		if (strarray.size() == 1)
		{
			if (strarray[0] != "undefined")
			{
				try {
					m_iStationID = std::stoi(strarray[0]);
					m_stationidprovided=true;
				}
				catch (const std::exception& e) {
					Log(LOG_ERROR, "Bad Station ID provided: %s (%s), please recheck hardware setup.", strarray[0].c_str(), e.what());
					return;
				}
			}
		}
		else if (strarray.size() == 2)
		{
			m_szMyLatitude = strarray[0];
			m_szMyLongitude = strarray[1];
		}
		else
		{
			Log(LOG_ERROR, "Location config Invalid! (Use either Station_ID or Latitude,Lontitude)");
		}
	}
}

void CBuienRadar::Init()
{
	struct tm ltime;
	time_t now = mytime(nullptr);
	localtime_r(&now, &ltime);
	m_itIsRaining = false;
}

bool CBuienRadar::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
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
	GetRainPrediction();
#endif
	int sec_counter = 593;
	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		time_t now = mytime(nullptr);
		m_LastHeartbeat = now;

		if (sec_counter % 600 == 0)
		{
			//Every 10 minutes
			GetMeterDetails();
		}
		if (sec_counter % 300 == 0)
		{
			//Every 5 minutes
			GetRainPrediction();
		}
		else
		{
			// Decrease rainshower every minute (if it is not zer)
			if ((m_rainShowerLeadTime > 0) && (sec_counter % 60 == 0))
			{
				m_rainShowerLeadTime--;
				SendCustomSensor(RAINSHOWER_LEADTIME, 1, 255, static_cast<float>(m_rainShowerLeadTime), "Expected Rainshower Leadtime", "minutes");
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool CBuienRadar::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

std::string CBuienRadar::GetForecastURL()
{
	std::string szURL = "https://gpsgadget.buienradar.nl/gadget/forecastandstation/" + std::to_string(m_iStationID);
	return szURL;
}

bool CBuienRadar::GetStationDetails()
{
	std::string sResult;

#ifdef DEBUG_BUIENRADARR
	sResult = ReadFile("E:\\br.json");
#else
	if (!HTTPClient::GET(BUIENRADAR_URL, sResult))
	{
		Log(LOG_ERROR, "Error getting http data! (Check your internet connection!)");
		return false;
	}
#ifdef DEBUG_BUIENRADARW
	SaveString2Disk(sResult, "E:\\br.json");
#endif
#endif

	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received! (Check Station ID!)");
		return false;
	}

	if (root["actual"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received (actual empty), or no data returned!");
		return false;
	}
	if (root["actual"]["stationmeasurements"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received (station measurement empty), or no data returned!");
		return false;
	}

	if (m_iStationID == 0)
	{
		std::string Latitude = "1";
		std::string Longitude = "1";
		int nValue;
		std::string sValue;
		double MyLatitude;
		double MyLongitude;

		if (m_szMyLatitude.empty())
		{
			// No station ID provided, so we are going to look for the closest one
			if (!m_sql.GetPreferencesVar("Location", nValue, sValue))
			{
				Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
				return false;
			}
			std::vector<std::string> strarray;
			StringSplit(sValue, ";", strarray);

			if (strarray.size() != 2)
				return false;

			Latitude = strarray[0];
			Longitude = strarray[1];

			MyLatitude = std::stod(Latitude);
			MyLongitude = std::stod(Longitude);

			if ((Latitude == "1") && (Longitude == "1"))
			{
				Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
				return false;
			}
			m_szMyLatitude = Latitude;
			m_szMyLongitude = Longitude;
			Log(LOG_STATUS, "using Domoticz location settings (%s,%s)", Latitude.c_str(), Longitude.c_str());
		}
		else
		{
			MyLatitude = std::stod(m_szMyLatitude);
			MyLongitude = std::stod(m_szMyLongitude);
			Log(LOG_STATUS, "using Configured location settings (%s,%s)", m_szMyLatitude.c_str(), m_szMyLongitude.c_str());
		}

		double shortest_distance_km = 200.0;//start with 200 km
		double shortest_station_lat = 0;
		double shortest_station_lon = 0;

		for (const auto &measurement : root["actual"]["stationmeasurements"])
		{
			if (measurement["temperature"].empty())
				continue;

			double lat = measurement["lat"].asDouble();
			double lon = measurement["lon"].asDouble();

			double distance_km = distanceEarth(
				MyLatitude, MyLongitude,
				lat, lon
			);
			if (distance_km < shortest_distance_km)
			{
				shortest_distance_km = distance_km;
				shortest_station_lat = lat;
				shortest_station_lon = lon;
				m_iStationID = measurement["stationid"].asInt();
				m_sStationName = measurement["stationname"].asString();
				m_sStationRegion = measurement["regio"].asString();
			}
		}
		if (m_iStationID == 0)
		{
			Log(LOG_ERROR, "No (nearby) station found!");
			return false;
		}
		Log(LOG_STATUS, "Nearest Station: %s (%s), Distance: %.1f km, ID: %d, Lat/Lon: %g,%g", m_sStationName.c_str(), m_sStationRegion.c_str(), shortest_distance_km, m_iStationID, shortest_station_lat, shortest_station_lon);
		return true;
	}

	// StationID was provided, find it in the list
	for (const auto &measurement : root["actual"]["stationmeasurements"])
	{
		if (measurement["temperature"].empty())
			continue;

		int StationID = measurement["stationid"].asInt();

		if (StationID == m_iStationID)
		{
			// Station Found, set name and region
			m_sStationName = measurement["stationname"].asString();
			m_sStationRegion = measurement["regio"].asString();
			m_szMyLatitude = std::to_string(measurement["lat"].asDouble());
			m_szMyLongitude = std::to_string(measurement["lon"].asDouble());
			Log(LOG_STATUS, "Using Station: %s (%s), ID: %d, Lat/Lon: %g,%g", m_sStationName.c_str(), m_sStationRegion.c_str(), m_iStationID, std::stod(m_szMyLatitude), std::stod(m_szMyLongitude));
			return true;
		}
	}
	Log(LOG_ERROR, "Configured StationID (%d) not found at Buienradar site (or does not contain a temperature sensor), please check Hardware setup!", m_iStationID);
	return false;
}

void CBuienRadar::GetMeterDetails()
{
	if (m_sStationName.empty())
	{
		// Station Name not set, get station details
		if (!GetStationDetails())
		{
			return;
		}
	}

	std::string sResult;
#ifdef DEBUG_BUIENRADARR
	sResult = ReadFile("E:\\br_actual.json");
#else
	std::string szUrl = BUIENRADAR_ACTUAL_URL + std::to_string(m_iStationID);
	if (!HTTPClient::GET(szUrl, sResult))
	{
		Log(LOG_ERROR, "Problem Connecting to Buienradar! (Check your Internet Connection!)");
		return;
	}
#ifdef DEBUG_BUIENRADARW
	SaveString2Disk(sResult, "E:\\br_actual.json");
#endif
#endif
	Json::Value root;
	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received! (Check Station ID!)");
		return;
	}

	if (root["timestamp"].empty() == true || (root["stationid"].empty() == true && root["stationId"].empty() == true))
	{
		Log(LOG_ERROR, "Invalid data received (timestamp or staionid missing) or no data returned!");
		return;
	}
	if (root["temperature"].empty() == true)
	{
		Log(LOG_ERROR, "Invalid data received (temperature missing) or no data returned!");
		return;
	}

	int stationID = root["stationid"].asInt();
	if (stationID != m_iStationID)
	{
		Log(LOG_ERROR, "Invalid data received (invalid stationid %d / %d ) or no data returned!", stationID, m_iStationID);
		return;
	}

	//iconurl : "https://www.buienradar.nl/resources/images/icons/weather/30x30/a.png"
	//graphUrl : "https://www.buienradar.nl/nederland/weerbericht/weergrafieken/a"

	// Update Location details if a configured station is used
	if (m_stationidprovided)
	{
		if (!root["lon"].empty())
		{
			if (root["lon"].asFloat()>0)
			{
				m_szMyLongitude = std::to_string(root["lon"].asDouble());
			}
		}

		if (!root["lat"].empty())
		{
			if (root["lat"].asFloat()>0)
			{
				m_szMyLatitude = std::to_string(root["lat"].asDouble());
			}
		}
	}

	float temp = -999.9F;
	int humidity = 0;
	float barometric = 0;
	uint8_t barometric_forecast = wsbaroforecast_unknown;

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
			barometric_forecast = wsbaroforecast_rain;
			if (temp != -999.9F)
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
	}
	if ((temp != -999.9F) && (humidity != 0) && (barometric != 0))
	{
		SendTempHumBaroSensorFloat(1, 255, temp, humidity, barometric, barometric_forecast, "TempHumBaro");
	}
	else if ((temp != -999.9F) && (humidity != 0))
	{
		SendTempHumSensor(1, 255, temp, humidity, "TempHum");
	}
	else if (temp != -999.9F)
	{
		SendTempSensor(1, 255, temp, "Temp");
	}

	if (!root["groundtemperature"].empty())
	{
		float tempGround = root["groundtemperature"].asFloat();
		SendTempSensor(2, 255, tempGround, "Ground Temperature (10 cm)");
	}
	if (!root["feeltemperature"].empty())
	{
		float tempFeel = root["feeltemperature"].asFloat();
		SendTempSensor(3, 255, tempFeel, "Feel Temperature");
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
		float wind_speed = root["windspeed"].asFloat();

		float wind_chill;
		if ((temp < 10.0) && (wind_speed >= 1.4))
			wind_chill = 0; //if we send 0 as chill, it will be calculated
		else
			wind_chill = temp;

		SendWind(1, 255, wind_direction, wind_speed, wind_gusts, temp, wind_chill, temp != -999.9F, true, "Wind");
	}

	if (!root["visibility"].empty())
	{
		float visibility = root["visibility"].asFloat() / 1000.0F;
		SendVisibilitySensor(1, 1, 255, visibility, "Visibility");
	}

	if (!root["sunpower"].empty())
	{
		float sunpower = root["sunpower"].asFloat();
		SendCustomSensor(2, 1, 255, sunpower, "Sun Power", "watt/m2");
	}

	if (!root["precipitationmm"].empty())
	{
		float precipitation = root["precipitationmm"].asFloat();
		SendRainRateSensor(1, 255, precipitation, "Rain");
		m_itIsRaining = precipitation > 0;
		SendSwitch(1, 1, 255, m_itIsRaining, 0, "Is it Raining", m_Name);
	}
}

void CBuienRadar::GetRainPrediction()
{
	if (m_szMyLatitude.empty() || m_szMyLongitude.empty() ||  std::stoi(m_szMyLatitude)<1 || std::stoi(m_szMyLongitude)<1)
		return;

	std::string sResult;
#ifdef DEBUG_BUIENRADARR
	sResult = ReadFile("E:\\br_rain.txt");
#else
	int totRetry = 0;
	bool bret = false;
	while ((!bret) && (totRetry < 2))
	{
		std::string szUrl = BUIENRADAR_RAIN + m_szMyLatitude + "&lon=" + m_szMyLongitude;
		bret = HTTPClient::GET(szUrl, sResult);
		if (!bret)
		{
			totRetry++;
			sleep_seconds(2);
		}
	}
	if (!bret)
	{
		Log(LOG_ERROR, "Problem Connecting to Buienradar! (Check your Internet Connection!)");
		return;
	}
	if (sResult.empty())
	{
		// Log(LOG_ERROR, "Problem getting Rainprediction: no prediction available at Buienradar");
		// no data to process, so don't update the sensros
		return;
	}

#ifdef DEBUG_BUIENRADARW
	SaveString2Disk(sResult, "E:\\br_rain.txt");
#endif
#endif

	time_t now = mytime(nullptr);
	struct tm ltime;
	localtime_r(&now, &ltime);

	int startTime = (ltime.tm_hour * 60) + ltime.tm_min;
#ifdef DEBUG_BUIENRADARR
	startTime = (13 * 60) + 30;
#endif
	int endTime = startTime + m_iForecast;
	int endTimeHour = startTime + 60;

	std::istringstream iStream(sResult);
	std::string sLine;

	double total_rain_in_duration = 0;
	int total_rain_values_in_duration = 0;
	double total_rain_next_hour = 0;
	int total_rain_values_next_hour = 0;
	int rain_time = 0;

	//values are between 0 (no rain) till 255 (heavy rain)
	//mm/h = 10^((value -109)/32), 77 = 0.1 mm.h

	// vars for next rainshower calculation
	int line = 0;
	bool shower_detected = m_itIsRaining;
	int start_of_rainshower;
	int end_of_rainshower = 0;
	if (shower_detected)
	{
		start_of_rainshower = startTime;
	}
	else
	{
		start_of_rainshower = 0;
	}
	double total_rainmmh_in_next_rainshower = 0;
	int total_rainvalues_in_next_rainshower = 0;
	double max_rainmmh_in_next_rainshower = 0;
	double avg_rainmmh_in_next_rainshower = 0;

	while (!iStream.eof())
	{
		std::getline(iStream, sLine);
		if (!sLine.empty())
		{
			std::vector<std::string> strarray;
			StringSplit(sLine, "|", strarray);
			if (strarray.size() != 2)
				return;

			int rain_value = std::stoi(strarray[0]);

			if (line == 0)
			{
				StringSplit(strarray[1], ":", strarray);
				if (strarray.size() != 2)
					return;

				int hour = std::stoi(strarray[0]);
				int min = std::stoi(strarray[1]);

				rain_time = (hour * 60) + min;

				// Check for 0.00 (in this case the difference between the timestamp is negative)
				if ((rain_time - startTime) < 0)
				{
					rain_time += 24 * 60;
				}

				// check for invalid measurement, the next prediction should be within 5 minutes, for safety we check for a timestamp max 10 minutes in the futere
				if ((rain_time - startTime) > 10)
				{
					// setting line to -1 to make sure line is ignored and we start again at the next line
					line = -1;
				}

			}
			else
			{
				rain_time += 5;
			}

			if (line >= 0)
			{
				// calculate statistics
				if (shower_detected)
				{
					if (end_of_rainshower == 0)
					{
						if (rain_value == 0)
						{
							// End Of RainShower detected
							end_of_rainshower = rain_time;
						}
						else
						{
							// add stats
							double rain_rate = pow(10, ((double)(rain_value - 109) / 32));
							total_rainvalues_in_next_rainshower++;
							total_rainmmh_in_next_rainshower += rain_rate;
							if (rain_rate > max_rainmmh_in_next_rainshower)
							{
								max_rainmmh_in_next_rainshower = rain_rate;
							}
						}
					}
				}
				else if ((start_of_rainshower == 0) && (rain_value > 0))
				{
					// Start Of RainShower Detected
					start_of_rainshower = rain_time;
					shower_detected = true;

					// add stats
					double rain_rate = pow(10, ((double)(rain_value - 109) / 32));

					total_rainvalues_in_next_rainshower++;
					total_rainmmh_in_next_rainshower += rain_rate;
					if (rain_rate > max_rainmmh_in_next_rainshower)
					{
						max_rainmmh_in_next_rainshower = rain_rate;
					}
				}

				if ((rain_time >= startTime) && (rain_time <= endTimeHour))
				{
					total_rain_next_hour += rain_value;
					total_rain_values_next_hour++;
				}

				if ((rain_time >= startTime) && (rain_time <= endTime))
				{
					total_rain_in_duration += rain_value;
					total_rain_values_in_duration++;
				}
			}
		}
		line++;
	}

	if (line > 12) // ignore the outcome if we have less then 12 valid measurements (this would mean the data is more than an hour old)
	{
		// Correct for conditions if Sstart or End Of Rainshower was not found.
		if (m_itIsRaining)
		{
			// We started with a rain condition so only checking if an end of shower was found
			if (end_of_rainshower == 0)
			{
				// No end of rain deteted, correcting endttime to last measurement
				end_of_rainshower = rain_time;
			}
		}
		else if (start_of_rainshower == 0)
		{
			// No Start of Rain detect, correcting StartOfRainShower and EndOfRainShower to starttime (which will result in duration of 0)
			start_of_rainshower = startTime;
			end_of_rainshower = startTime;
		}
		else if (end_of_rainshower == 0)
		{
			// Start was detected, but End was not, Assuming EndOfRainshower is next meausurement after the last measurement (+5 min)
			end_of_rainshower = rain_time + 5;
		}

		if (total_rainvalues_in_next_rainshower != 0)
		{
			avg_rainmmh_in_next_rainshower = total_rainmmh_in_next_rainshower / total_rainvalues_in_next_rainshower;
		}

		m_rainShowerLeadTime = start_of_rainshower - startTime;
		SendCustomSensor(RAINSHOWER_LEADTIME, 1, 255, static_cast<float>(round_digits(m_rainShowerLeadTime, 1)), "Next Rainshower Leadtime", "min");
		SendCustomSensor(RAINSHOWER_DURATION, 1, 255, static_cast<float>(round_digits(end_of_rainshower - start_of_rainshower, 1)), "Next Rainshower Duration", "min");
		SendCustomSensor(RAINSHOWER_AVERAGE_MMH, 1, 255, static_cast<float>(round_digits(avg_rainmmh_in_next_rainshower, 1)), "Next Rainshower Avg Rainrate", "mm/h");
		SendCustomSensor(RAINSHOWER_MAX_MMH, 1, 255, static_cast<float>(round_digits(max_rainmmh_in_next_rainshower, 1)), "Next Rainshower Max Rainrate", "mm/h");

		if (total_rain_values_in_duration)
		{
			double rain_avg = total_rain_in_duration / total_rain_values_in_duration;
			//double rain_mm_hour = pow(10, ((rain_avg - 109) / 32));
			double rain_perc = (rain_avg == 0) ? 0 : (rain_avg * 0.392156862745098);
			SendPercentageSensor(1, 1, 255, static_cast<float>(rain_perc), "Rain Intensity");
			SendSwitch(2, 1, 255, (rain_perc >= m_iThreshold), 0, "Possible Rain", m_Name);
		}
		if (total_rain_values_next_hour)
		{
			double rain_avg = total_rain_next_hour / total_rain_values_next_hour;
			double rain_mm_hour = (rain_avg == 0) ? 0 : pow(10, ((rain_avg - 109) / 32));
			SendCustomSensor(1, 1, 255, static_cast<float>(round_digits(rain_mm_hour, 1)), "Rainfall next Hour", "mm/h");
		}
	}
	else
	{
		Log(LOG_ERROR, "RainPrediction has old data");
	}
}
