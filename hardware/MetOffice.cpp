#include "stdafx.h"
#include "MetOffice.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fstream>
#include <iomanip>

// Met Office Weather DataHub (https://datahub.metoffice.gov.uk)
//
// Two separately subscribed products are used, each with its own API key:
//  - Site Specific "Global Spot" hourly forecast for a latitude/longitude
//  - Land Observations: hourly measurements from the nearest observing site
//
// The free plans are limited per day (resetting at 00:00 UTC): 55 calls for
// Global Spot and 360 for Land Observations. Everything below is arranged
// around staying well inside those limits.

#define MetOffice_Forecast_URL "https://data.hub.api.metoffice.gov.uk/sitespecific/v0/point/hourly"
#define MetOffice_Daily_Forecast_URL "https://data.hub.api.metoffice.gov.uk/sitespecific/v0/point/daily"
#define MetOffice_Observation_URL "https://data.hub.api.metoffice.gov.uk/observation-land/1/"

// The forecast is recalculated hourly, so more than one call an hour buys
// nothing: 24 of the 55 daily calls.
#define MetOffice_Forecast_Fetch_Interval 3600
// The optional forecast per day comes out of the same 55 calls: 8 more,
// which leaves 23 as headroom.
#define MetOffice_Daily_Forecast_Fetch_Interval (3 * 3600)
// Both are fetched at fixed times, counted from local midnight, so that one
// of the daily ones follows midnight closely: that is when the days move
// along and the new last one is not known yet. Not on the hour itself, and
// not at the same moment for everybody: 5 to 15 minutes past.
#define MetOffice_Fetch_Min_Offset (5 * 60)
#define MetOffice_Fetch_Offset_Spread (10 * 60)
// Today plus the six days after it
#define MetOffice_Daily_Forecast_Days 7
// Observations are hourly too, but appear some time after the hour.
#define MetOffice_Observation_Fetch_Interval 900
// A failing call may still count against the daily limit, so do not hammer.
#define MetOffice_Retry_Interval 1800
// The devices follow the cached forecast between two fetches.
#define MetOffice_Device_Update_Interval 300
// An observation older than this says nothing about the current weather.
#define MetOffice_Observation_Max_Age (3 * 3600)

extern std::string szUserDataFolder;

namespace
{
	constexpr int NODE_FC_TEMPHUMBARO = 1;
	constexpr int NODE_FC_WIND = 2;
	constexpr int NODE_FC_FEELSLIKE = 3;
	constexpr int NODE_FC_VISIBILITY = 4;
	constexpr int NODE_FC_DEWPOINT = 5;
	constexpr int NODE_FC_RAIN = 6;
	constexpr int NODE_FC_WEATHER = 7;
	constexpr int NODE_FC_ISRAINING = 8;
	constexpr int NODE_FC_UV = 9;
	constexpr int NODE_FC_RAINPROBABILITY = 10;

	constexpr int NODE_OBS_TEMP = 21;
	constexpr int NODE_OBS_WIND = 22;
	constexpr int NODE_OBS_VISIBILITY = 23;
	constexpr int NODE_OBS_WEATHER = 24;
	constexpr int NODE_OBS_PRESSURE = 25;

	// One of each per day, the day number is added
	constexpr int NODE_DAY_MAXTEMP = 100;
	constexpr int NODE_DAY_MINTEMP = 110;
	constexpr int NODE_DAY_WEATHER = 120;
	constexpr int NODE_DAY_RAINPROBABILITY = 130;
	constexpr int NODE_DAY_DATE = 140;

	const char *const CACHE_HOURLY = "forecast";
	const char *const CACHE_DAILY = "daily";

	struct WeatherCode
	{
		const char *description;
		uint8_t forecast;
	};

	// Met Office significant weather codes, shared by forecast and observations
	const std::map<int, WeatherCode> WeatherCodes = {
		// Not in the DataHub list, which starts at 0. The older DataPoint
		// service had it, and observations may still report it.
		{ -1, { "Trace rain", wsbaroforecast_cloudy } },
		{ 0, { "Clear night", wsbaroforecast_sunny } },
		{ 1, { "Sunny day", wsbaroforecast_sunny } },
		{ 2, { "Partly cloudy (night)", wsbaroforecast_some_clouds } },
		{ 3, { "Partly cloudy (day)", wsbaroforecast_some_clouds } },
		// 4 is "Not used" in the Met Office list
		{ 5, { "Mist", wsbaroforecast_cloudy } },
		{ 6, { "Fog", wsbaroforecast_cloudy } },
		{ 7, { "Cloudy", wsbaroforecast_cloudy } },
		{ 8, { "Overcast", wsbaroforecast_cloudy } },
		{ 9, { "Light rain shower (night)", wsbaroforecast_rain } },
		{ 10, { "Light rain shower (day)", wsbaroforecast_rain } },
		{ 11, { "Drizzle", wsbaroforecast_rain } },
		{ 12, { "Light rain", wsbaroforecast_rain } },
		{ 13, { "Heavy rain shower (night)", wsbaroforecast_heavy_rain } },
		{ 14, { "Heavy rain shower (day)", wsbaroforecast_heavy_rain } },
		{ 15, { "Heavy rain", wsbaroforecast_heavy_rain } },
		{ 16, { "Sleet shower (night)", wsbaroforecast_snow } },
		{ 17, { "Sleet shower (day)", wsbaroforecast_snow } },
		{ 18, { "Sleet", wsbaroforecast_snow } },
		{ 19, { "Hail shower (night)", wsbaroforecast_unstable } },
		{ 20, { "Hail shower (day)", wsbaroforecast_unstable } },
		{ 21, { "Hail", wsbaroforecast_unstable } },
		{ 22, { "Light snow shower (night)", wsbaroforecast_snow } },
		{ 23, { "Light snow shower (day)", wsbaroforecast_snow } },
		{ 24, { "Light snow", wsbaroforecast_snow } },
		{ 25, { "Heavy snow shower (night)", wsbaroforecast_heavy_snow } },
		{ 26, { "Heavy snow shower (day)", wsbaroforecast_heavy_snow } },
		{ 27, { "Heavy snow", wsbaroforecast_heavy_snow } },
		{ 28, { "Thunder shower (night)", wsbaroforecast_unstable } },
		{ 29, { "Thunder shower (day)", wsbaroforecast_unstable } },
		{ 30, { "Thunder", wsbaroforecast_unstable } },
	};

	std::string GetWeatherDescription(const int code)
	{
		auto it = WeatherCodes.find(code);
		if (it != WeatherCodes.end())
			return it->second.description;
		return "Unknown";
	}

	uint8_t GetWeatherForecast(const int code)
	{
		auto it = WeatherCodes.find(code);
		if (it != WeatherCodes.end())
			return it->second.forecast;
		return wsbaroforecast_unknown;
	}

	// Both APIs give UTC timestamps, "2026-11-16T15:00Z" or "2026-11-16T15:00:00Z".
	// Returns 0 when the string is not one of those.
	time_t ParseUTCTime(const std::string &sTime)
	{
		int year, month, day, hour, minute;
		if (sscanf(sTime.c_str(), "%4d-%2d-%2dT%2d:%2d", &year, &month, &day, &hour, &minute) != 5)
			return 0;
		if ((year < 1970) || (month < 1) || (month > 12) || (day < 1) || (day > 31) || (hour < 0) || (hour > 23) || (minute < 0) || (minute > 59))
			return 0;
		// Days since 1970-01-01 in the proleptic Gregorian calendar, which
		// avoids depending on timegm() or the process timezone.
		const int y = (month <= 2) ? year - 1 : year;
		const int era = y / 400;
		const int yoe = y - era * 400;
		const int doy = (153 * (month + ((month > 2) ? -3 : 9)) + 2) / 5 + day - 1;
		const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		const int64_t days = static_cast<int64_t>(era) * 146097 + doe - 719468;
		return static_cast<time_t>(days * 86400 + hour * 3600 + minute * 60);
	}

	// 16 point compass notation to degrees, -1 when not recognised
	int CompassToDegrees(const std::string &sDirection)
	{
		static const char *const points[] = { "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW" };
		for (int ii = 0; ii < 16; ii++)
		{
			if (sDirection == points[ii])
				return static_cast<int>(std::lround(ii * 22.5));
		}
		return -1;
	}

	// Day of the month where Domoticz runs
	int LocalDay(const time_t atime)
	{
		struct tm ltime;
		localtime_r(&atime, &ltime);
		return ltime.tm_mday;
	}

	float Interpolate(const float a, const float b, const float fraction)
	{
		return a + (b - a) * fraction;
	}

	// Wind direction is circular: go the short way round between two bearings
	int InterpolateDirection(const int a, const int b, const float fraction)
	{
		int diff = ((b - a) % 360 + 540) % 360 - 180;
		int result = static_cast<int>(std::lround(a + diff * fraction)) % 360;
		if (result < 0)
			result += 360;
		return result;
	}
} // namespace

CMetOffice::CMetOffice(const int ID, const std::string &ForecastAPIKey, const std::string &ObservationAPIKey, const std::string &Location, const bool bAddDailyForecast)
	: m_ForecastAPIKey(ForecastAPIKey)
	, m_ObservationAPIKey(ObservationAPIKey)
	, m_Location(Location)
	, m_bAddDailyForecast(bAddDailyForecast)
{
	m_HwdID = ID;
	stdstring_trim(m_ForecastAPIKey);
	stdstring_trim(m_ObservationAPIKey);
	stdstring_trim(m_Location);
	// The same one after a restart
	m_FetchOffset = MetOffice_Fetch_Min_Offset + static_cast<int>(std::hash<std::string>()(m_ForecastAPIKey) % MetOffice_Fetch_Offset_Spread);
}

void CMetOffice::Init()
{
	m_Lat = 0;
	m_Lon = 0;
	m_bHaveLocation = false;
	m_Forecast.clear();
	m_ForecastFetched = 0;
	m_NextForecastAttempt = 0;
	m_DailyForecast.clear();
	m_DailyForecastFetched = 0;
	m_NextDailyForecastAttempt = 0;
	m_ObservationGeohash.clear();
	m_NextObservationAttempt = 0;

	if (m_ForecastAPIKey.empty() && m_ObservationAPIKey.empty())
	{
		Log(LOG_ERROR, "No API Key specified! (A Global Spot and/or a Land Observations key is needed)");
		return;
	}

	std::string sLocation = m_Location;
	char cSeparator = ',';
	const bool bUseHomeLocation = (sLocation.empty() || (sLocation == "0"));
	if (bUseHomeLocation)
	{
		// Use the 'home' location of this Domoticz instance
		int nValue;
		if (!m_sql.GetPreferencesVar("Location", nValue, sLocation))
		{
			Log(LOG_ERROR, "Invalid Location found in Settings! (Check your Latitude/Longitude!)");
			return;
		}
		cSeparator = ';';
	}

	std::vector<std::string> strarray;
	StringSplit(sLocation, std::string(1, cSeparator), strarray);
	if (strarray.size() != 2)
	{
		Log(LOG_ERROR, "Invalid Location! (Expected Latitude%cLongitude)", cSeparator);
		return;
	}

	char *endLat = nullptr, *endLon = nullptr;
	errno = 0;
	const double lat = strtod(strarray[0].c_str(), &endLat);
	const double lon = strtod(strarray[1].c_str(), &endLon);
	if ((errno != 0) || (endLat == strarray[0].c_str()) || (*endLat != '\0') || (endLon == strarray[1].c_str()) || (*endLon != '\0'))
	{
		Log(LOG_ERROR, "Invalid Location! (Could not parse '%s' as Latitude%cLongitude)", sLocation.c_str(), cSeparator);
		return;
	}
	if (bUseHomeLocation && (lat == 0.0) && (lon == 0.0))
	{
		Log(LOG_ERROR, "The Domoticz location is not set! (Set it in Settings > System > Location, or enter Latitude,Longitude in the hardware setup)");
		return;
	}
	// The forecast API accepts latitudes from -85 to 85
	if ((lat < -85.0) || (lat > 85.0) || (lon < -180.0) || (lon > 180.0) || ((lat == 0.0) && (lon == 0.0)))
	{
		Log(LOG_ERROR, "Location out of range (Latitude %g, Longitude %g)!", lat, lon);
		return;
	}

	m_Lat = lat;
	m_Lon = lon;
	m_bHaveLocation = true;
	Log(LOG_STATUS, "Using location Latitude %g, Longitude %g", m_Lat, m_Lon);

	// Restarting Domoticz must not cost any of the 55 daily calls, so the
	// forecasts are kept on disk and reused until they are due again.
	if (!m_ForecastAPIKey.empty())
	{
		const time_t atime = mytime(nullptr);
		std::string sData;
		time_t fetched;
		if (LoadCache(CACHE_HOURLY, MetOffice_Forecast_Fetch_Interval, sData, fetched) && ParseForecast(sData))
		{
			m_ForecastFetched = fetched;
			Log(LOG_STATUS, "Using the forecast retrieved %d minutes ago", static_cast<int>((atime - fetched) / 60));
		}
		if (m_bAddDailyForecast && LoadCache(CACHE_DAILY, MetOffice_Daily_Forecast_Fetch_Interval, sData, fetched) && ParseDailyForecast(sData))
		{
			m_DailyForecastFetched = fetched;
			Log(LOG_STATUS, "Using the daily forecast retrieved %d minutes ago", static_cast<int>((atime - fetched) / 60));
		}
	}
}

bool CMetOffice::StartHardware()
{
	Init();

	RequestStart();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CMetOffice::StopHardware()
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

void CMetOffice::Do_Work()
{
	Log(LOG_STATUS, "Started...");

	if (!m_bHaveLocation)
		Log(LOG_STATUS, "Unable to run due to missing or incorrect API Key/Location parameters!");

	int sec_counter = 0;
	time_t lastDeviceUpdate = 0;
	time_t lastDailyDeviceUpdate = 0;
	time_t lastObservationFetch = 0;

	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
			m_LastHeartbeat = mytime(nullptr);

		if (!m_bHaveLocation)
			continue;
		// Looking at the clock every few seconds is plenty
		if (sec_counter % 5 != 0)
			continue;

		const time_t atime = mytime(nullptr);
		try
		{
			if (!m_ForecastAPIKey.empty())
			{
				if ((m_ForecastFetched < FetchSlotStart(atime, MetOffice_Forecast_Fetch_Interval)) && (atime >= m_NextForecastAttempt))
				{
					FetchForecast();
					lastDeviceUpdate = 0;
				}
				if (atime - lastDeviceUpdate >= MetOffice_Device_Update_Interval)
				{
					lastDeviceUpdate = atime;
					UpdateForecastDevices();
				}
				if (m_bAddDailyForecast)
				{
					if ((m_DailyForecastFetched < FetchSlotStart(atime, MetOffice_Daily_Forecast_Fetch_Interval)) && (atime >= m_NextDailyForecastAttempt))
					{
						FetchDailyForecast();
						lastDailyDeviceUpdate = 0;
					}
					// Which day is 'Day 0' changes at midnight: move the days
					// along right then, from what is already there.
					if ((lastDailyDeviceUpdate == 0) || (LocalDay(atime) != LocalDay(lastDailyDeviceUpdate)) || (atime - lastDailyDeviceUpdate >= MetOffice_Forecast_Fetch_Interval))
					{
						lastDailyDeviceUpdate = atime;
						UpdateDailyForecastDevices();
					}
				}
			}
			if (!m_ObservationAPIKey.empty())
			{
				if ((atime - lastObservationFetch >= MetOffice_Observation_Fetch_Interval) && (atime >= m_NextObservationAttempt))
				{
					lastObservationFetch = atime;
					FetchObservation();
				}
			}
		}
		catch (const std::exception &e)
		{
			Log(LOG_ERROR, "Unhandled failure getting/parsing data! (%s)", e.what());
		}
		catch (...)
		{
			Log(LOG_ERROR, "Unhandled failure getting/parsing data!");
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool CMetOffice::WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

bool CMetOffice::HTTPGet(const char *szWhat, const std::string &sURL, const std::string &sAPIKey, std::string &sResult)
{
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("apikey: " + sAPIKey);
	ExtraHeaders.push_back("Accept: application/json");

	std::vector<std::string> vHeaderData;
	Debug(DEBUG_HARDWARE, "Get data from %s", sURL.c_str());
	try
	{
		if (HTTPClient::GET(sURL, ExtraHeaders, sResult, vHeaderData))
			return true;
	}
	catch (...)
	{
		Log(LOG_ERROR, "Recovered from crash during attempt to get http data!");
		return false;
	}

	std::string sStatus = vHeaderData.empty() ? std::string("no response") : vHeaderData[0];
	stdstring_trim(sStatus);
	const char *szHint = "";
	if ((sStatus.find(" 401") != std::string::npos) || (sStatus.find(" 403") != std::string::npos))
		szHint = ", check the API Key and its subscription";
	else if (sStatus.find(" 429") != std::string::npos)
		szHint = ", the daily call limit has been reached";
	Log(LOG_ERROR, "Error getting the %s! (%s%s)", szWhat, sStatus.c_str(), szHint);
	return false;
}

std::string CMetOffice::CacheFile(const char *szKind) const
{
	return szUserDataFolder + "metoffice_" + szKind + "_" + std::to_string(m_HwdID) + ".json";
}

bool CMetOffice::LoadCache(const char *szKind, const time_t maxAge, std::string &sData, time_t &fetched)
{
	std::ifstream infile(CacheFile(szKind), std::ios::in | std::ios::binary);
	if (!infile.is_open())
		return false;
	std::stringstream ss;
	ss << infile.rdbuf();
	infile.close();

	Json::Value root;
	if (!ParseJSon(ss.str(), root) || !root.isObject())
		return false;
	if (!root["fetched"].isNumeric() || !root["latitude"].isNumeric() || !root["longitude"].isNumeric() || !root["data"].isString())
		return false;
	// A forecast for another location is of no use
	if ((std::fabs(root["latitude"].asDouble() - m_Lat) > 1e-6) || (std::fabs(root["longitude"].asDouble() - m_Lon) > 1e-6))
		return false;

	fetched = static_cast<time_t>(root["fetched"].asInt64());
	const time_t atime = mytime(nullptr);
	if ((fetched > atime) || (atime - fetched >= maxAge))
		return false;

	sData = root["data"].asString();
	return true;
}

void CMetOffice::SaveCache(const char *szKind, const std::string &sData, const time_t fetched)
{
	Json::Value root;
	root["fetched"] = static_cast<Json::Int64>(fetched);
	root["latitude"] = m_Lat;
	root["longitude"] = m_Lon;
	root["data"] = sData;

	std::ofstream outfile(CacheFile(szKind), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!outfile.is_open())
	{
		Log(LOG_ERROR, "Unable to write %s, a restart will use an extra API call!", CacheFile(szKind).c_str());
		return;
	}
	outfile << JSonToRawString(root);
}

bool CMetOffice::ParseForecast(const std::string &sData)
{
	Json::Value root;
	if (!ParseJSon(sData, root) || !root.isObject())
	{
		Log(LOG_ERROR, "Invalid forecast data received!");
		return false;
	}
	const Json::Value &features = root["features"];
	if (!features.isArray() || features.empty() || !features[0]["properties"]["timeSeries"].isArray())
	{
		Log(LOG_ERROR, "No forecast time series in API response!");
		return false;
	}

	static const char *const requiredFields[] = { "screenTemperature",	  "screenRelativeHumidity", "mslp",	  "windSpeed10m",
						      "windDirectionFrom10m", "windGustSpeed10m",	    "visibility", "precipitationRate" };

	std::vector<ForecastStep> steps;
	for (const auto &entry : features[0]["properties"]["timeSeries"])
	{
		if (!entry.isObject() || !entry["time"].isString())
			continue;
		ForecastStep step;
		step.time = ParseUTCTime(entry["time"].asString());
		if (step.time == 0)
			continue;

		bool bComplete = true;
		for (const auto &field : requiredFields)
		{
			if (!entry[field].isNumeric())
				bComplete = false;
		}
		if (!bComplete)
			continue;

		step.temperature = entry["screenTemperature"].asFloat();
		step.humidity = entry["screenRelativeHumidity"].asFloat();
		step.pressure = entry["mslp"].asFloat() / 100.0F; // Pa -> hPa
		step.windspeed = entry["windSpeed10m"].asFloat();
		step.winddirection = entry["windDirectionFrom10m"].asInt();
		step.windgust = entry["windGustSpeed10m"].asFloat();
		step.visibility = entry["visibility"].asFloat();
		step.rainrate = entry["precipitationRate"].asFloat();
		step.dewpoint = entry["screenDewPointTemperature"].isNumeric() ? entry["screenDewPointTemperature"].asFloat() : NAN;
		step.feelslike = entry["feelsLikeTemperature"].isNumeric() ? entry["feelsLikeTemperature"].asFloat() : NAN;
		step.uvindex = entry["uvIndex"].isNumeric() ? entry["uvIndex"].asFloat() : NAN;
		step.rainprobability = entry["probOfPrecipitation"].isNumeric() ? entry["probOfPrecipitation"].asFloat() : NAN;
		step.weathercode = entry["significantWeatherCode"].isNumeric() ? entry["significantWeatherCode"].asInt() : -100;
		steps.push_back(step);
	}
	if (steps.empty())
	{
		Log(LOG_ERROR, "No usable forecast time steps in API response!");
		return false;
	}
	std::sort(steps.begin(), steps.end(), [](const ForecastStep &a, const ForecastStep &b) { return a.time < b.time; });

	m_Forecast = std::move(steps);
	return true;
}

void CMetOffice::FetchForecast()
{
	const time_t atime = mytime(nullptr);
	// Unless it works out, wait before trying again
	m_NextForecastAttempt = atime + MetOffice_Retry_Interval;

	std::stringstream sURL;
	sURL << std::fixed << std::setprecision(6);
	sURL << MetOffice_Forecast_URL << "?latitude=" << m_Lat << "&longitude=" << m_Lon << "&excludeParameterMetadata=true";

	std::string sResult;
	if (!HTTPGet("forecast", sURL.str(), m_ForecastAPIKey, sResult))
		return;
	if (!ParseForecast(sResult))
		return;

	m_ForecastFetched = atime;
	m_NextForecastAttempt = 0;
	SaveCache(CACHE_HOURLY, sResult, atime);
}

// The forecast has one step per hour. Rather than showing the same values for
// an hour and then jumping, the devices follow a straight line between the
// two steps around the current time.
void CMetOffice::UpdateForecastDevices()
{
	if (m_Forecast.empty())
		return;

	const time_t atime = mytime(nullptr);

	size_t idx = 0;
	while ((idx + 1 < m_Forecast.size()) && (m_Forecast[idx + 1].time <= atime))
		idx++;

	const ForecastStep &a = m_Forecast[idx];
	if ((atime < a.time - 3600) || (atime > a.time + 3600))
	{
		// Only reachable when fetching has been failing for two days,
		// or with a badly wrong system clock
		Debug(DEBUG_HARDWARE, "The cached forecast does not cover the current time");
		return;
	}
	const ForecastStep &b = (idx + 1 < m_Forecast.size()) ? m_Forecast[idx + 1] : a;

	float fraction = 0;
	if ((b.time > a.time) && (atime > a.time))
		fraction = std::min(1.0F, static_cast<float>(atime - a.time) / static_cast<float>(b.time - a.time));
	// Codes, and anything else that cannot be averaged, come from the nearest step
	const ForecastStep &nearest = (fraction < 0.5F) ? a : b;

	const float temperature = Interpolate(a.temperature, b.temperature, fraction);
	const int humidity = static_cast<int>(std::lround(Interpolate(a.humidity, b.humidity, fraction)));
	const float pressure = Interpolate(a.pressure, b.pressure, fraction);
	const float windspeed = Interpolate(a.windspeed, b.windspeed, fraction);
	const float windgust = Interpolate(a.windgust, b.windgust, fraction);
	const int winddirection = InterpolateDirection(a.winddirection, b.winddirection, fraction);
	const float visibility = Interpolate(a.visibility, b.visibility, fraction);
	const float rainrate = Interpolate(a.rainrate, b.rainrate, fraction);

	SendTempHumBaroSensorFloat(NODE_FC_TEMPHUMBARO, 255, temperature, humidity, pressure, GetWeatherForecast(nearest.weathercode), "TempHumBaro");

	float wind_chill;
	if ((temperature < 10.0F) && (windspeed >= 1.4F))
		wind_chill = 0; // if we send 0, it will be calculated
	else
		wind_chill = temperature;
	SendWind(NODE_FC_WIND, 255, winddirection, windspeed, windgust, temperature, wind_chill, true, true, "Wind");

	if (!std::isnan(a.feelslike) && !std::isnan(b.feelslike))
		SendTempSensor(NODE_FC_FEELSLIKE, 255, Interpolate(a.feelslike, b.feelslike, fraction), "Feel Temperature");

	SendVisibilitySensor(NODE_FC_VISIBILITY, 1, 255, visibility / 1000.0F, "Visibility");

	if (!std::isnan(a.dewpoint) && !std::isnan(b.dewpoint))
		SendTempSensor(NODE_FC_DEWPOINT, 255, Interpolate(a.dewpoint, b.dewpoint, fraction), "Dew Point");

	SendRainRateSensor(NODE_FC_RAIN, 255, rainrate, "Rain");

	if (nearest.weathercode != -100)
		SendTextSensor(NODE_FC_WEATHER, 1, 255, GetWeatherDescription(nearest.weathercode), "Weather");

	SendSwitch(NODE_FC_ISRAINING, 1, 255, (nearest.rainrate > 0.0F), 0, "Is It Raining", m_Name);

	if (!std::isnan(nearest.uvindex) && (nearest.uvindex >= 0) && (nearest.uvindex < 16))
		SendUVSensor(NODE_FC_UV, 1, 255, nearest.uvindex, "UV Index");

	if (!std::isnan(nearest.rainprobability))
		SendPercentageSensor(NODE_FC_RAINPROBABILITY, 1, 255, nearest.rainprobability, "Rain Probability");
}

bool CMetOffice::ParseDailyForecast(const std::string &sData)
{
	Json::Value root;
	if (!ParseJSon(sData, root) || !root.isObject())
	{
		Log(LOG_ERROR, "Invalid daily forecast data received!");
		return false;
	}
	const Json::Value &features = root["features"];
	if (!features.isArray() || features.empty() || !features[0]["properties"]["timeSeries"].isArray())
	{
		Log(LOG_ERROR, "No daily forecast time series in API response!");
		return false;
	}

	std::vector<DailyStep> steps;
	for (const auto &entry : features[0]["properties"]["timeSeries"])
	{
		if (!entry.isObject() || !entry["time"].isString())
			continue;
		// "2026-11-17T00:00Z", the day it is about
		const std::string sTime = entry["time"].asString();
		if ((sTime.size() < 10) || (ParseUTCTime(sTime) == 0))
			continue;

		// The day values go once the day is over, the night ones remain
		DailyStep step;
		step.date = sTime.substr(0, 10);
		step.maxtemp = entry["dayMaxScreenTemperature"].isNumeric() ? entry["dayMaxScreenTemperature"].asFloat() : NAN;
		step.mintemp = entry["nightMinScreenTemperature"].isNumeric() ? entry["nightMinScreenTemperature"].asFloat() : NAN;
		if (entry["daySignificantWeatherCode"].isNumeric())
			step.weathercode = entry["daySignificantWeatherCode"].asInt();
		else if (entry["nightSignificantWeatherCode"].isNumeric())
			step.weathercode = entry["nightSignificantWeatherCode"].asInt();
		if (entry["dayProbabilityOfPrecipitation"].isNumeric())
			step.rainprobability = entry["dayProbabilityOfPrecipitation"].asFloat();
		else if (entry["nightProbabilityOfPrecipitation"].isNumeric())
			step.rainprobability = entry["nightProbabilityOfPrecipitation"].asFloat();
		else
			step.rainprobability = NAN;
		steps.push_back(step);
	}
	if (steps.empty())
	{
		Log(LOG_ERROR, "No usable daily forecast time steps in API response!");
		return false;
	}

	m_DailyForecast = std::move(steps);
	return true;
}

void CMetOffice::FetchDailyForecast()
{
	const time_t atime = mytime(nullptr);
	// Unless it works out, wait before trying again
	m_NextDailyForecastAttempt = atime + MetOffice_Retry_Interval;

	std::stringstream sURL;
	sURL << std::fixed << std::setprecision(6);
	sURL << MetOffice_Daily_Forecast_URL << "?latitude=" << m_Lat << "&longitude=" << m_Lon << "&excludeParameterMetadata=true";

	std::string sResult;
	if (!HTTPGet("daily forecast", sURL.str(), m_ForecastAPIKey, sResult))
		return;
	if (!ParseDailyForecast(sResult))
		return;

	m_DailyForecastFetched = atime;
	m_NextDailyForecastAttempt = 0;
	SaveCache(CACHE_DAILY, sResult, atime);
}

// The forecasts are fetched at fixed times of the day: a little after local
// midnight and every 'interval' seconds from there. This gives the start of
// the period the current time is in; a forecast from before it is due again.
time_t CMetOffice::FetchSlotStart(const time_t atime, const int interval) const
{
	struct tm ltime;
	localtime_r(&atime, &ltime);
	ltime.tm_hour = 0;
	ltime.tm_min = 0;
	ltime.tm_sec = 0;
	ltime.tm_isdst = -1;
	const time_t first = mktime(&ltime) + m_FetchOffset;
	if (atime < first)
		return first - interval; // the last one of yesterday
	return first + ((atime - first) / interval) * interval;
}

// 'Day 0' is today's date where Domoticz runs, 'Day 1' the one after and so
// on, so what a device is about changes at local midnight. Someone looking at
// it from another timezone, or just around midnight, cannot tell which date
// that is, so every day has a 'Date Day N' saying which date its values are
// for. It is only updated together with them: between midnight and the fetch
// that follows it the last day is not known yet and still holds what is now
// the forecast of the day before it, which shows as two days with one date.
// The minimum temperature is the one of the night that follows the day.
void CMetOffice::UpdateDailyForecastDevices()
{
	if (m_DailyForecast.empty())
		return;

	const time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	for (int iDay = 0; iDay < MetOffice_Daily_Forecast_Days; iDay++)
	{
		// Noon, so that a daylight saving change cannot land on another date
		struct tm tday = ltime;
		tday.tm_hour = 12;
		tday.tm_min = 0;
		tday.tm_sec = 0;
		tday.tm_mday += iDay;
		tday.tm_isdst = -1;
		if (mktime(&tday) == static_cast<time_t>(-1))
			continue;
		char szDate[40];
		snprintf(szDate, sizeof(szDate), "%04d-%02d-%02d", tday.tm_year + 1900, tday.tm_mon + 1, tday.tm_mday);

		auto it = std::find_if(m_DailyForecast.begin(), m_DailyForecast.end(), [&szDate](const DailyStep &step) { return step.date == szDate; });
		if (it == m_DailyForecast.end())
			continue;

		// All of a day or nothing: a device that kept its value while the
		// date next to it moved on would be showing it for the wrong date
		if (std::isnan(it->maxtemp) || std::isnan(it->mintemp) || std::isnan(it->rainprobability) || (it->weathercode == -100))
			continue;

		const std::string sDay = " Day " + std::to_string(iDay);
		SendTempSensor(NODE_DAY_MAXTEMP + iDay, 255, it->maxtemp, "Maximum Temperature" + sDay);
		SendTempSensor(NODE_DAY_MINTEMP + iDay, 255, it->mintemp, "Minimum Temperature" + sDay);
		SendTextSensor(NODE_DAY_WEATHER + iDay, 1, 255, GetWeatherDescription(it->weathercode), "Weather" + sDay);
		SendPercentageSensor(NODE_DAY_RAINPROBABILITY + iDay, 1, 255, it->rainprobability, "Rain Probability" + sDay);
		// The date last: once it says a date, the values are those of that date
		SendTextSensor(NODE_DAY_DATE + iDay, 1, 255, szDate, "Date" + sDay);
	}
}

// Observations are requested by the geohash of an observing site, which has
// to be looked up from the location first.
bool CMetOffice::ResolveObservationSite()
{
	// The API refuses coordinates with more than two decimals
	std::stringstream sURL;
	sURL << std::fixed << std::setprecision(2);
	sURL << MetOffice_Observation_URL << "nearest?lat=" << m_Lat << "&lon=" << m_Lon;

	std::string sResult;
	if (!HTTPGet("observation site", sURL.str(), m_ObservationAPIKey, sResult))
		return false;

	Json::Value root;
	if (!ParseJSon(sResult, root) || !root.isArray() || root.empty() || !root[0]["geohash"].isString() || root[0]["geohash"].asString().empty())
	{
		Log(LOG_ERROR, "No observation site found near Latitude %.2f, Longitude %.2f!", m_Lat, m_Lon);
		return false;
	}
	const std::string sGeohash = root[0]["geohash"].asString();
	// It becomes part of a URL
	if (sGeohash.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz") != std::string::npos)
	{
		Log(LOG_ERROR, "Invalid observation site received!");
		return false;
	}
	m_ObservationGeohash = sGeohash;

	std::string sArea = root[0]["area"].isString() ? root[0]["area"].asString() : "";
	Log(LOG_STATUS, "Using observation site %s%s%s", m_ObservationGeohash.c_str(), sArea.empty() ? "" : ", ", sArea.c_str());
	return true;
}

void CMetOffice::FetchObservation()
{
	const time_t atime = mytime(nullptr);

	if (m_ObservationGeohash.empty())
	{
		if (!ResolveObservationSite())
		{
			m_NextObservationAttempt = atime + MetOffice_Retry_Interval;
			return;
		}
	}

	std::string sResult;
	if (!HTTPGet("observations", std::string(MetOffice_Observation_URL) + m_ObservationGeohash, m_ObservationAPIKey, sResult))
	{
		m_NextObservationAttempt = atime + MetOffice_Retry_Interval;
		return;
	}

	Json::Value root;
	if (!ParseJSon(sResult, root) || !root.isArray())
	{
		Log(LOG_ERROR, "Invalid observation data received!");
		m_NextObservationAttempt = atime + MetOffice_Retry_Interval;
		return;
	}

	// Find the most recent observation, the order is not documented
	const Json::Value *pLatest = nullptr;
	time_t latest = 0;
	for (const auto &entry : root)
	{
		if (!entry.isObject() || !entry["datetime"].isString())
			continue;
		const time_t obstime = ParseUTCTime(entry["datetime"].asString());
		if (obstime > latest)
		{
			latest = obstime;
			pLatest = &entry;
		}
	}
	if (pLatest == nullptr)
	{
		Log(LOG_STATUS, "No observations available for site %s", m_ObservationGeohash.c_str());
		return;
	}
	if (atime - latest > MetOffice_Observation_Max_Age)
	{
		Log(LOG_STATUS, "Ignoring outdated observation of %s", (*pLatest)["datetime"].asString().c_str());
		return;
	}
	const Json::Value &obs = *pLatest;

	// What a site measures differs: many report temperature and humidity
	// only, so every value is optional.
	const bool bHaveTemp = obs["temperature"].isNumeric();
	const bool bHaveHum = obs["humidity"].isNumeric();
	const bool bHaveBaro = obs["mslp"].isNumeric();
	const bool bHaveCode = obs["weather_code"].isNumeric();
	const float temperature = bHaveTemp ? obs["temperature"].asFloat() : 0;
	const int humidity = bHaveHum ? obs["humidity"].asInt() : 0;
	const float pressure = bHaveBaro ? obs["mslp"].asFloat() : 0;
	const int weathercode = bHaveCode ? obs["weather_code"].asInt() : -100;

	if (bHaveTemp && bHaveHum && bHaveBaro)
		SendTempHumBaroSensorFloat(NODE_OBS_TEMP, 255, temperature, humidity, pressure, GetWeatherForecast(weathercode), "Observed TempHumBaro");
	else if (bHaveTemp && bHaveHum)
		SendTempHumSensor(NODE_OBS_TEMP, 255, temperature, humidity, "Observed TempHum");
	else if (bHaveTemp)
		SendTempSensor(NODE_OBS_TEMP, 255, temperature, "Observed Temperature");
	else if (bHaveHum)
		SendHumiditySensor(NODE_OBS_TEMP, 255, humidity, "Observed Humidity");

	if (bHaveBaro && !(bHaveTemp && bHaveHum))
		SendBaroSensor(NODE_OBS_PRESSURE, 1, 255, pressure, GetWeatherForecast(weathercode), "Observed Pressure");

	if (obs["wind_speed"].isNumeric() && obs["wind_direction"].isString())
	{
		const int winddirection = CompassToDegrees(obs["wind_direction"].asString());
		if (winddirection >= 0)
		{
			const float windspeed = obs["wind_speed"].asFloat();
			const float windgust = obs["wind_gust"].isNumeric() ? obs["wind_gust"].asFloat() : windspeed;
			float wind_chill;
			if (bHaveTemp && (temperature < 10.0F) && (windspeed >= 1.4F))
				wind_chill = 0; // if we send 0, it will be calculated
			else
				wind_chill = temperature;
			SendWind(NODE_OBS_WIND, 255, winddirection, windspeed, windgust, temperature, wind_chill, bHaveTemp, bHaveTemp, "Observed Wind");
		}
	}

	if (obs["visibility"].isNumeric())
		// A visibility device is identified by its ChildID alone, so it has
		// to differ from the one of the forecast
		SendVisibilitySensor(NODE_OBS_VISIBILITY, 2, 255, obs["visibility"].asFloat() / 1000.0F, "Observed Visibility");

	if (bHaveCode)
		SendTextSensor(NODE_OBS_WEATHER, 1, 255, GetWeatherDescription(weathercode), "Observed Weather");
}
