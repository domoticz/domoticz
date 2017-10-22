#include "stdafx.h"
#include "DarkSky.h"
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
//#define DEBUG_DarkSkyR
//#define DEBUG_DarkSkyW
#endif

#ifdef DEBUG_DarkSkyW
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
#ifdef DEBUG_DarkSkyR
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

CDarkSky::CDarkSky(const int ID, const std::string &APIKey, const std::string &Location) :
m_APIKey(APIKey),
m_Location(Location)
{
	m_HwdID=ID;
	m_stoprequested=false;
	Init();
}

CDarkSky::~CDarkSky(void)
{
}

void CDarkSky::Init()
{
}

bool CDarkSky::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CDarkSky::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CDarkSky::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CDarkSky::Do_Work()
{
	_log.Log(LOG_STATUS, "DarkSky: Started...");

	int sec_counter = 290;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % 300 == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS,"DarkSky: Worker stopped...");
}

bool CDarkSky::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

std::string CDarkSky::GetForecastURL()
{
	std::stringstream sURL;
	std::string szLoc = CURLEncode::URLEncode(m_Location);
	sURL << "https://darksky.net/#/f/" << szLoc;
	return sURL.str();
}

void CDarkSky::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_DarkSkyR
	sResult=ReadFile("E:\\DarkSky.json");
#else
	std::stringstream sURL;
	std::string szLoc = m_Location;
	std::string szExclude = "minutely,hourly,daily,alerts,flags";
	sURL << "https://api.darksky.net/forecast/" << m_APIKey << "/" << szLoc << "?exclude=" << szExclude;
	try
	{
		bool bret;
		std::string szURL = sURL.str();
		bret = HTTPClient::GET(szURL, sResult);
		if (!bret)
		{
			_log.Log(LOG_ERROR, "DarkSky: Error getting http data!.");
			return;
		}
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "DarkSky: Error getting http data!");
		return;
	}
#ifdef DEBUG_DarkSkyW
	SaveString2Disk(sResult, "E:\\DarkSky.json");
#endif

#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR,"DarkSky: Invalid data received! Check Location, use a City or GPS Coordinates (xx.yyyy,xx.yyyyy)");
		return;
	}
	if (root["currently"].empty()==true)
	{
		_log.Log(LOG_ERROR,"DarkSky: Invalid data received, or unknown location!");
		return;
	}
	/*
	std::string tmpstr2 = root.toStyledString();
	FILE *fOut = fopen("E:\\DarkSky.json", "wb+");
	fwrite(tmpstr2.c_str(), 1, tmpstr2.size(), fOut);
	fclose(fOut);
	*/

	float temp;
	int humidity=0;
	int barometric=0;
	int barometric_forcast=baroForecastNoInfo;

	temp=root["currently"]["temperature"].asFloat();
	//Convert to celcius
	temp=float((temp-32)*(5.0/9.0));

	if (root["currently"]["humidity"].empty()==false)
	{
		humidity=round(root["currently"]["humidity"].asFloat()*100.0f);
	}
	if (root["currently"]["pressure"].empty()==false)
	{
		barometric=atoi(root["currently"]["pressure"].asString().c_str());
		if (barometric<1000)
			barometric_forcast=baroForecastRain;
		else if (barometric<1020)
			barometric_forcast=baroForecastCloudy;
		else if (barometric<1030)
			barometric_forcast=baroForecastPartlyCloudy;
		else
			barometric_forcast=baroForecastSunny;

		if (root["currently"]["icon"].empty()==false)
		{
			std::string forcasticon=root["currently"]["icon"].asString();
			if ((forcasticon=="partly-cloudy-day")||(forcasticon=="partly-cloudy-night"))
			{
				barometric_forcast=baroForecastPartlyCloudy;
			}
			else if (forcasticon=="cloudy")
			{
				barometric_forcast=baroForecastCloudy;
			}
			else if ((forcasticon=="clear-day")||(forcasticon=="clear-night"))
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
		SendTempHumBaroSensor(1, 255, temp, humidity, static_cast<float>(barometric), barometric_forcast, "THB");
	}
	else if (humidity!=0)
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
	int wind_degrees=-1;
	float windspeed_ms=0;
	float windgust_ms=0;
	float wind_temp=temp;
	float wind_chill=temp;
	int windgust=1;
	float windchill=-1;

	if (root["currently"]["windBearing"].empty()==false)
	{
		wind_degrees=atoi(root["currently"]["windBearing"].asString().c_str());
	}
	if (root["currently"]["windSpeed"].empty()==false)
	{
		if ((root["currently"]["windSpeed"] != "N/A") && (root["currently"]["windSpeed"] != "--"))
		{
			float temp_wind_mph = static_cast<float>(atof(root["currently"]["windSpeed"].asString().c_str()));
			if (temp_wind_mph!=-9999.00f)
			{
				//convert to m/s
				windspeed_ms=temp_wind_mph*0.44704f;
			}
		}
	}
	if (root["currently"]["windGust"].empty()==false)
	{
		if ((root["currently"]["windGust"] != "N/A") && (root["currently"]["windGust"] != "--"))
		{
			float temp_wind_gust_mph = static_cast<float>(atof(root["currently"]["windGust"].asString().c_str()));
			if (temp_wind_gust_mph!=-9999.00f)
			{
				//convert to m/s
				windgust_ms=temp_wind_gust_mph*0.44704f;
			}
		}
	}
	if (root["currently"]["apparentTemperature"].empty()==false)
	{
		if ((root["currently"]["apparentTemperature"] != "N/A") && (root["currently"]["apparentTemperature"] != "--"))
		{
			wind_chill = static_cast<float>(atof(root["currently"]["apparentTemperature"].asString().c_str()));
			//Convert to celcius
			wind_chill=float((wind_chill-32)*(5.0/9.0));
		}
	}
	if (wind_degrees!=-1)
	{
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.WIND.packetlength=sizeof(tsen.WIND)-1;
		tsen.WIND.packettype=pTypeWIND;
		tsen.WIND.subtype=sTypeWIND4;
		tsen.WIND.battery_level=9;
		tsen.WIND.rssi=12;
		tsen.WIND.id1=0;
		tsen.WIND.id2=1;

		float winddir=float(wind_degrees);
		int aw=round(winddir);
		tsen.WIND.directionh=(BYTE)(aw/256);
		aw-=(tsen.WIND.directionh*256);
		tsen.WIND.directionl=(BYTE)(aw);

		tsen.WIND.av_speedh=0;
		tsen.WIND.av_speedl=0;
		int sw=round(windspeed_ms*10.0f);
		tsen.WIND.av_speedh=(BYTE)(sw/256);
		sw-=(tsen.WIND.av_speedh*256);
		tsen.WIND.av_speedl=(BYTE)(sw);

		tsen.WIND.gusth=0;
		tsen.WIND.gustl=0;
		int gw=round(windgust_ms*10.0f);
		tsen.WIND.gusth=(BYTE)(gw/256);
		gw-=(tsen.WIND.gusth*256);
		tsen.WIND.gustl=(BYTE)(gw);

		//this is not correct, why no wind temperature? and only chill?
		tsen.WIND.chillh=0;
		tsen.WIND.chilll=0;
		tsen.WIND.temperatureh=0;
		tsen.WIND.temperaturel=0;

		tsen.WIND.tempsign=(wind_temp>=0)?0:1;
		int at10=round(std::abs(wind_temp*10.0f));
		tsen.WIND.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.WIND.temperatureh*256);
		tsen.WIND.temperaturel=(BYTE)(at10);

		tsen.WIND.chillsign=(wind_temp>=0)?0:1;
		at10=round(std::abs(wind_chill*10.0f));
		tsen.WIND.chillh=(BYTE)(at10/256);
		at10-=(tsen.WIND.chillh*256);
		tsen.WIND.chilll=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, NULL, 255);
	}

	//UV
	if (root["currently"]["uvIndex"].empty() == false)
	{
		if ((root["currently"]["uvIndex"] != "N/A") && (root["currently"]["uvIndex"] != "--"))
		{
			float UV = root["currently"]["uvIndex"].asFloat();
			if ((UV < 16) && (UV >= 0))
			{
				SendUVSensor(0, 1, 255, UV, "UV Index");
			}
		}
	}

	//Rain
	if (root["currently"]["precipIntensity"].empty()==false)
	{
		if ((root["currently"]["precipIntensity"] != "N/A") && (root["currently"]["precipIntensity"] != "--"))
		{
			float RainCount = static_cast<float>(atof(root["currently"]["precipIntensity"].asString().c_str()))*25.4f; //inches to mm
			if ((RainCount!=-9999.00f)&&(RainCount>=0.00f))
			{
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.RAIN.packetlength=sizeof(tsen.RAIN)-1;
				tsen.RAIN.packettype=pTypeRAIN;
				tsen.RAIN.subtype=sTypeRAINWU;
				tsen.RAIN.battery_level=9;
				tsen.RAIN.rssi=12;
				tsen.RAIN.id1=0;
				tsen.RAIN.id2=1;

				tsen.RAIN.rainrateh=0;
				tsen.RAIN.rainratel=0;

				int tr10=int((float(RainCount)*10.0f));
				tsen.RAIN.raintotal1=0;
				tsen.RAIN.raintotal2=(BYTE)(tr10/256);
				tr10-=(tsen.RAIN.raintotal2*256);
				tsen.RAIN.raintotal3=(BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN, NULL, 255);
			}
		}
	}

	//Visibility
	if (root["currently"]["visibility"].empty()==false)
	{
		if ((root["currently"]["visibility"] != "N/A") && (root["currently"]["visibility"] != "--"))
		{
			float visibility = static_cast<float>(atof(root["currently"]["visibility"].asString().c_str()))*1.60934f; //miles to km
			if (visibility>=0)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype=sTypeVisibility;
				gdevice.floatval1=visibility;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
			}
		}
	}
	//Solar Radiation
	if (root["currently"]["ozone"].empty()==false)
	{
		if ((root["currently"]["ozone"] != "N/A") && (root["currently"]["ozone"] != "--"))
		{
			float radiation = static_cast<float>(atof(root["currently"]["ozone"].asString().c_str()));	//this is in dobson units, need to convert to Watt/m2? (2.69×(10^20) ?
			if (radiation>=0.0f)
			{
				_tGeneralDevice gdevice;
				gdevice.subtype=sTypeSolarRadiation;
				gdevice.floatval1=radiation;
				sDecodeRXMessage(this, (const unsigned char *)&gdevice, NULL, 255);
			}
		}
	}

}

