#include "stdafx.h"
#include "AccuWeather.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	//#define DEBUG_AccuWeatherR
	//#define DEBUG_AccuWeatherW
	//#define DEBUG_AccuWeatherW2
#endif

#if defined(DEBUG_AccuWeatherW) || defined(DEBUG_AccuWeatherW2)
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
#ifdef DEBUG_AccuWeatherR
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

CAccuWeather::CAccuWeather(const int ID, const std::string &APIKey, const std::string &Location) :
m_APIKey(APIKey),
m_Location(Location),
m_LocationKey("")
{
	m_HwdID=ID;
	m_stoprequested=false;
	Init();
}

CAccuWeather::~CAccuWeather(void)
{
}

void CAccuWeather::Init()
{
}

bool CAccuWeather::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CAccuWeather::Do_Work, this);
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CAccuWeather::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
    m_bIsStarted=false;
    return true;
}

void CAccuWeather::Do_Work()
{
	int sec_counter = 595;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 1800 == 0) //50 free calls a day.. thats not much guy's!
		{
			if (m_LocationKey.empty())
			{
				m_LocationKey = GetLocationKey();
				if (m_LocationKey.empty())
					continue;
			}
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"AccuWeather Worker stopped...");
}

bool CAccuWeather::WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/)
{
	return false;
}

std::string CAccuWeather::GetForecastURL()
{
	return m_ForecastURL;
}

std::string CAccuWeather::GetLocationKey()
{
	std::string sResult;
#ifdef DEBUG_AccuWeatherR
	sResult = ReadFile("E:\\AccuWeather_LocationSearch.json");
#else
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);

	sURL << "https://dataservice.accuweather.com/locations/v1/search?apikey=" << m_APIKey << "&q=" << szLoc;
	try
	{
		bool bret;
		std::string szURL = sURL.str();
		bret = HTTPClient::GET(szURL, sResult);
		if (!bret)
		{
			_log.Log(LOG_ERROR, "AccuWeather: Error getting http data!");
			return "";
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "AccuWeather: Error getting http data!");
		return "";
	}
#endif
#ifdef DEBUG_AccuWeatherW2
	SaveString2Disk(sResult, "E:\\AccuWeather_LocationSearch.json");
#endif
	try
	{
		Json::Value root;
		Json::Reader jReader;
		bool ret = jReader.parse(sResult, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "AccuWeather: Invalid data received!");
			return "";
		}
		if (!root.empty())
		{
			if (root.isArray())
				root = root[0];
			if (!root.isObject())
			{
				_log.Log(LOG_ERROR, "AccuWeather: Invalid data received, or unknown location!");
				return "";
			}
			if (root["Key"].empty())
			{
				_log.Log(LOG_ERROR, "AccuWeather: Invalid data received, or unknown location!");
				return "";
			}
			return root["Key"].asString();
		}
		else
		{
			_log.Log(LOG_ERROR, "AccuWeather: Invalid data received, unknown location or API key!");
			return "";
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "AccuWeather: Error parsing JSon data!");
	}
	return "";
}

void CAccuWeather::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_AccuWeatherR
	sResult=ReadFile("E:\\AccuWeather.json");
#else
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_LocationKey);
	sURL << "https://dataservice.accuweather.com/currentconditions/v1/" << szLoc << "?apikey=" << m_APIKey << "&details=true";
	try
	{
		bool bret;
		std::string szURL = sURL.str();
		bret = HTTPClient::GET(szURL, sResult);
		if (!bret)
		{
			_log.Log(LOG_ERROR, "AccuWeather: Error getting http data!");
			return;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "AccuWeather: Error getting http data!");
		return;
	}
#endif
#ifdef DEBUG_AccuWeatherW
	SaveString2Disk(sResult, "E:\\AccuWeather.json");
#endif

	try
	{
		Json::Value root;
		Json::Reader jReader;
		bool ret = jReader.parse(sResult, root);
		if (!ret)
		{
			_log.Log(LOG_ERROR, "AccuWeather: Invalid data received!");
			return;
		}

		if (root.size() < 1)
		{
			_log.Log(LOG_ERROR, "AccuWeather: Invalid data received!");
			return;
		}
		root = root[0];

		if (root["LocalObservationDateTime"].empty())
		{
			_log.Log(LOG_ERROR, "AccuWeather: Invalid data received, or unknown location!");
			return;
		}

		float temp = 0;
		int humidity = 0;
		int barometric = 0;
		int barometric_forcast = baroForecastNoInfo;

		if (!root["Temperature"].empty())
		{
			temp = root["Temperature"]["Metric"]["Value"].asFloat();
		}

		if (!root["RelativeHumidity"].empty())
		{
			humidity = root["RelativeHumidity"].asInt();
		}
		if (!root["Pressure"].empty())
		{
			barometric = atoi(root["Pressure"]["Metric"]["Value"].asString().c_str());
			if (barometric < 1000)
				barometric_forcast = baroForecastRain;
			else if (barometric < 1020)
				barometric_forcast = baroForecastCloudy;
			else if (barometric < 1030)
				barometric_forcast = baroForecastPartlyCloudy;
			else
				barometric_forcast = baroForecastSunny;

			if (!root["WeatherIcon"].empty())
			{
				int forcasticon = atoi(root["WeatherIcon"].asString().c_str());
				switch (forcasticon)
				{
				case 1:
				case 2:
				case 3:
					barometric_forcast = baroForecastSunny;
					break;
				case 4:
				case 5:
				case 6:
					barometric_forcast = baroForecastCloudy;
					break;
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:
				case 20:
				case 21:
				case 22:
				case 23:
				case 24:
				case 25:
				case 26:
				case 27:
				case 28:
				case 29:
				case 39:
				case 40:
				case 41:
				case 42:
				case 43:
				case 44:
					barometric_forcast = baroForecastRain;
					break;
				case 12:
				case 13:
				case 14:
				case 15:
				case 16:
				case 17:
				case 18:
				case 19:
					barometric_forcast = baroForecastCloudy;
					break;
				}
			}
		}

		if (barometric != 0)
		{
			//Add temp+hum+baro device
			SendTempHumBaroSensor(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forcast, "THB");
		}
		else if (humidity != 0)
		{
			//add temp+hum device
			SendTempHumSensor(1, 255, temp, humidity, "TempHum");
		}
		else
		{
			//add temp device
			SendTempSensor(1, 255, temp, "Temperature");
		}

		//Wind
		if (!root["Wind"].empty())
		{
			int wind_degrees = -1;
			float windspeed_ms = 0;
			float windgust_ms = 0;
			//float wind_temp = temp;
			float wind_chill = temp;

			if (!root["Wind"]["Direction"].empty())
			{
				wind_degrees = root["Wind"]["Direction"]["Degrees"].asInt();
			}
			if (!root["Wind"]["Speed"].empty())
			{
				windspeed_ms = root["Wind"]["Speed"]["Metric"]["Value"].asFloat() / 3.6f; //km/h to m/s
			}
			if (!root["WindGust"].empty())
			{
				if (!root["WindGust"]["Speed"].empty())
				{
					windgust_ms = root["WindGust"]["Speed"]["Metric"]["Value"].asFloat() / 3.6f; //km/h to m/s
				}
			}
			if (!root["RealFeelTemperature"].empty())
			{
				wind_chill = root["RealFeelTemperature"]["Metric"]["Value"].asFloat();
			}
			if (wind_degrees != -1)
			{
				SendWind(1, 255, wind_degrees, windspeed_ms, windgust_ms, temp, wind_chill, true, "Wind");
			}
		}

		//UV
		if (!root["UVIndex"].empty())
		{
			float UV = static_cast<float>(atof(root["UVIndex"].asString().c_str()));
			if ((UV < 16) && (UV >= 0))
			{
				SendUVSensor(0, 1, 255, UV, "UV");
			}
		}

		//Rain
		if (!root["PrecipitationSummary"].empty())
		{
			if (!root["PrecipitationSummary"]["Precipitation"].empty())
			{
				float RainCount = static_cast<float>(atof(root["PrecipitationSummary"]["Precipitation"]["Metric"]["Value"].asString().c_str()));
				if ((RainCount != -9999.00f) && (RainCount >= 0.00f))
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

					if (!root["PrecipitationSummary"]["PastHour"].empty())
					{
						float rainrateph = static_cast<float>(atof(root["PrecipitationSummary"]["PastHour"]["Metric"]["Value"].asString().c_str()));
						if (rainrateph != -9999.00f)
						{
							int at10 = round(std::abs(rainrateph*10.0f));
							tsen.RAIN.rainrateh = (BYTE)(at10 / 256);
							at10 -= (tsen.RAIN.rainrateh * 256);
							tsen.RAIN.rainratel = (BYTE)(at10);
						}
					}

					int tr10 = int((float(RainCount)*10.0f));
					tsen.RAIN.raintotal1 = 0;
					tsen.RAIN.raintotal2 = (BYTE)(tr10 / 256);
					tr10 -= (tsen.RAIN.raintotal2 * 256);
					tsen.RAIN.raintotal3 = (BYTE)(tr10);

					sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, NULL, 255);
				}
			}
		}

		//Visibility
		if (!root["Visibility"].empty())
		{
			if (!root["Visibility"]["Metric"].empty())
			{
				float visibility = root["Visibility"]["Metric"]["Value"].asFloat();
				if (visibility >= 0)
				{
					_tGeneralDevice gdevice;
					gdevice.subtype = sTypeVisibility;
					gdevice.floatval1 = visibility;
					sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
				}
			}
		}

		//Forecast URL
		if (!root["Link"].empty())
		{
			m_ForecastURL = root["Link"].asString();
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "AccuWeather: Error parsing JSon data!");
	}
}

