#include "stdafx.h"
#include "OpenMeteo.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <cerrno>
#include <iomanip>

#ifdef _DEBUG
//#define DEBUG_OpenMeteoR
//#define DEBUG_OpenMeteoW
#endif

#ifdef DEBUG_OpenMeteoW
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
#ifdef DEBUG_OpenMeteoR
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

#define OpenMeteo_API_URL "https://api.open-meteo.com/v1/forecast"
#define OpenMeteo_Poll_Interval 300

// WMO Weather interpretation codes to text description
static const std::map<int, std::string> WMO_Descriptions = {
	{ 0, "Clear sky" },
	{ 1, "Mainly clear" },
	{ 2, "Partly cloudy" },
	{ 3, "Overcast" },
	{ 45, "Fog" },
	{ 48, "Depositing rime fog" },
	{ 51, "Light drizzle" },
	{ 53, "Moderate drizzle" },
	{ 55, "Dense drizzle" },
	{ 56, "Light freezing drizzle" },
	{ 57, "Dense freezing drizzle" },
	{ 61, "Slight rain" },
	{ 63, "Moderate rain" },
	{ 65, "Heavy rain" },
	{ 66, "Light freezing rain" },
	{ 67, "Heavy freezing rain" },
	{ 71, "Slight snow" },
	{ 73, "Moderate snow" },
	{ 75, "Heavy snow" },
	{ 77, "Snow grains" },
	{ 80, "Slight rain showers" },
	{ 81, "Moderate rain showers" },
	{ 82, "Violent rain showers" },
	{ 85, "Slight snow showers" },
	{ 86, "Heavy snow showers" },
	{ 95, "Thunderstorm" },
	{ 96, "Thunderstorm with slight hail" },
	{ 99, "Thunderstorm with heavy hail" },
};

// WMO Weather code to barometric forecast mapping
static const std::map<int, uint8_t> WMO_To_Forecast = {
	{ 0, wsbaroforecast_sunny }, { 1, wsbaroforecast_sunny }, { 2, wsbaroforecast_some_clouds }, { 3, wsbaroforecast_cloudy },
	{ 45, wsbaroforecast_cloudy }, { 48, wsbaroforecast_cloudy },
	{ 51, wsbaroforecast_rain }, { 53, wsbaroforecast_rain }, { 55, wsbaroforecast_rain }, { 56, wsbaroforecast_rain }, { 57, wsbaroforecast_rain },
	{ 61, wsbaroforecast_rain }, { 63, wsbaroforecast_rain }, { 65, wsbaroforecast_heavy_rain }, { 66, wsbaroforecast_rain }, { 67, wsbaroforecast_heavy_rain },
	{ 71, wsbaroforecast_snow }, { 73, wsbaroforecast_snow }, { 75, wsbaroforecast_heavy_snow }, { 77, wsbaroforecast_snow },
	{ 80, wsbaroforecast_rain }, { 81, wsbaroforecast_rain }, { 82, wsbaroforecast_heavy_rain }, { 85, wsbaroforecast_snow }, { 86, wsbaroforecast_heavy_snow },
	{ 95, wsbaroforecast_unstable }, { 96, wsbaroforecast_unstable }, { 99, wsbaroforecast_unstable },
};

static std::string GetWMODescription(int code)
{
	auto it = WMO_Descriptions.find(code);
	if (it != WMO_Descriptions.end())
		return it->second;
	return "Unknown";
}

static uint8_t GetWMOForecast(int code)
{
	auto it = WMO_To_Forecast.find(code);
	if (it != WMO_To_Forecast.end())
		return it->second;
	return wsbaroforecast_unknown;
}

COpenMeteo::COpenMeteo(const int ID)
{
	m_HwdID = ID;
}

void COpenMeteo::Init()
{
	m_Lat = 0;
	m_Lon = 0;
	m_URL.clear();

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
	{
		Log(LOG_ERROR, "Invalid Location format! Expected 'lat;lon'");
		return;
	}

	char *endLat = nullptr, *endLon = nullptr;
	errno = 0;
	m_Lat = strtod(strarray[0].c_str(), &endLat);
	m_Lon = strtod(strarray[1].c_str(), &endLon);

	if (errno != 0 || (endLat && *endLat != '\0') || (endLon && *endLon != '\0'))
	{
		Log(LOG_ERROR, "Invalid Location coordinates! Could not parse '%s;%s' as numbers", strarray[0].c_str(), strarray[1].c_str());
		return;
	}

	if ((m_Lat < -90.0 || m_Lat > 90.0) || (m_Lon < -180.0 || m_Lon > 180.0))
	{
		Log(LOG_ERROR, "Location coordinates out of range (lat=%.6f, lon=%.6f)! Configure in Settings > System > Location", m_Lat, m_Lon);
		return;
	}

	if (m_Lat == 0.0 && m_Lon == 0.0)
	{
		Log(LOG_ERROR, "Location coordinates are 0,0! Configure in Settings > System > Location");
		return;
	}

	std::stringstream sURL;
	sURL << std::fixed << std::setprecision(6);
	sURL << OpenMeteo_API_URL
	     << "?latitude=" << m_Lat
	     << "&longitude=" << m_Lon
	     << "&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,rain,weather_code,cloud_cover,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m,visibility,soil_temperature_0cm,soil_moisture_0_to_1cm,uv_index,direct_radiation_instant"
	     << "&wind_speed_unit=ms";
	m_URL = sURL.str();

	Log(LOG_STATUS, "Using Domoticz location: lat=%f, lon=%f", m_Lat, m_Lon);
}

bool COpenMeteo::StartHardware()
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

bool COpenMeteo::StopHardware()
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

void COpenMeteo::Do_Work()
{
	Log(LOG_STATUS, "Started...");

	int sec_counter = OpenMeteo_Poll_Interval - 5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % 12 == 0)
		{
			m_LastHeartbeat = mytime(nullptr);
		}
		if (sec_counter % OpenMeteo_Poll_Interval == 0)
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
				Log(LOG_STATUS, "Unable to run due to missing or incorrect Location parameters!");
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool COpenMeteo::WriteToHardware(const char * /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

void COpenMeteo::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_OpenMeteoR
	sResult = ReadFile("E:\\OpenMeteo.json");
#else
	try
	{
		std::vector<std::string> ExtraHeaders;
		ExtraHeaders.push_back("User-Agent: Domoticz/1.0");

		if (!HTTPClient::GET(m_URL, ExtraHeaders, sResult))
		{
			Log(LOG_ERROR, "Error getting http data!");
			return;
		}
	}
	catch (...)
	{
		Log(LOG_ERROR, "Recovered from crash during attempt to get http data!");
		return;
	}
#ifdef DEBUG_OpenMeteoW
	SaveString2Disk(sResult, "E:\\OpenMeteo.json");
#endif

#endif
	Json::Value root;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		Log(LOG_ERROR, "Invalid data received!");
		return;
	}

	if (root["current"].empty())
	{
		Log(LOG_ERROR, "No 'current' data in API response!");
		return;
	}

	Json::Value current = root["current"];

	// Validate mandatory fields
	static const char *requiredFields[] = {
		"temperature_2m", "relative_humidity_2m", "apparent_temperature", "precipitation",
		"rain", "weather_code", "cloud_cover", "surface_pressure",
		"wind_speed_10m", "wind_direction_10m", "wind_gusts_10m", "visibility"
	};
	for (const auto &field : requiredFields)
	{
		if (current[field].isNull())
		{
			Log(LOG_ERROR, "Missing required field '%s' in API response", field);
			return;
		}
	}

	float temperature = current["temperature_2m"].asFloat();
	int humidity = current["relative_humidity_2m"].asInt();
	float apparent_temperature = current["apparent_temperature"].asFloat();
	float precipitation = current["precipitation"].asFloat();
	float rain = current["rain"].asFloat();
	int weather_code = current["weather_code"].asInt();
	float cloud_cover = current["cloud_cover"].asFloat();
	float pressure = current["surface_pressure"].asFloat();
	float wind_speed = current["wind_speed_10m"].asFloat();
	int wind_direction = current["wind_direction_10m"].asInt();
	float wind_gusts = current["wind_gusts_10m"].asFloat();
	float visibility = current["visibility"].asFloat();

	uint8_t barometric_forecast = GetWMOForecast(weather_code);
	std::string weather_description = GetWMODescription(weather_code);

	// Node 1: Temp+Hum+Baro
	if (pressure != 0 && humidity != 0)
	{
		SendTempHumBaroSensorFloat(1, 255, temperature, humidity, pressure, barometric_forecast, "TempHumBaro");
	}

	// Node 2: Wind
	float wind_chill;
	if ((temperature < 10.0F) && (wind_speed >= 1.4F))
		wind_chill = 0; // if we send 0, it will be calculated
	else
		wind_chill = temperature;

	SendWind(2, 255, wind_direction, wind_speed, wind_gusts, temperature, wind_chill, true, true, "Wind");

	// Node 3: Feel Temperature (apparent temperature)
	SendTempSensor(3, 255, apparent_temperature, "Feel Temperature");

	// Node 4: Visibility (API returns meters, convert to km * 10 for Domoticz)
	float visibility_km = visibility / 1000.0F;
	SendVisibilitySensor(4, 1, 255, visibility_km, "Visibility");

	// Node 5: Cloud Cover (%)
	SendPercentageSensor(5, 1, 255, cloud_cover, "Cloud Cover");

	// Node 6: Rain
	SendRainRateSensor(6, 255, rain, "Rain");

	// Node 7: Weather description text
	SendTextSensor(7, 1, 255, weather_description, "Weather");

	// Node 8: Is it raining switch
	bool is_raining = (rain > 0.0F) || (precipitation > 0.0F);
	SendSwitch(8, 1, 255, is_raining, 0, "Is It Raining", m_Name);

	// Node 9: UV Index
	if (!current["uv_index"].empty())
	{
		float uv_index = current["uv_index"].asFloat();
		if ((uv_index >= 0) && (uv_index < 16))
		{
			SendUVSensor(9, 1, 255, uv_index, "UV Index");
		}
	}

	// Node 10: Solar Radiation (W/m²)
	if (!current["direct_radiation_instant"].empty())
	{
		float solar_radiation = current["direct_radiation_instant"].asFloat();
		SendSolarRadiationSensor(10, 255, solar_radiation, "Solar Radiation");
	}

	// Node 11: Soil Temperature (0 cm surface)
	if (!current["soil_temperature_0cm"].empty())
	{
		float soil_temp = current["soil_temperature_0cm"].asFloat();
		SendTempSensor(11, 255, soil_temp, "Soil Temperature");
	}

	// Node 12: Soil Moisture (0-1 cm, volumetric m³/m³ converted to %)
	if (!current["soil_moisture_0_to_1cm"].empty())
	{
		float soil_moisture = current["soil_moisture_0_to_1cm"].asFloat() * 100.0F;
		SendPercentageSensor(12, 1, 255, soil_moisture, "Soil Moisture");
	}
}
