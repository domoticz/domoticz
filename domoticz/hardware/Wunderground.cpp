#include "stdafx.h"
#include "Wunderground.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/mynetwork.h"
#include "../httpclient/UrlEncode.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../main/RFXtrx.h"

#define round(a) ( int ) ( a + .5 )

//#define DEBUG_WUNDERGROUND

CWunderground::CWunderground(const int ID, const std::string APIKey, const std::string Location)
{
	m_HwdID=ID;
	m_LastMinute=26;
	m_APIKey=APIKey;
	m_Location=Location;
	m_stoprequested=false;
	Init();
}

CWunderground::~CWunderground(void)
{
}

void CWunderground::Init()
{
}

bool CWunderground::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CWunderground::Do_Work, this)));
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CWunderground::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    
    return true;
}

void CWunderground::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		time_t atime=time(NULL);
		struct tm ltime;
		localtime_r(&atime,&ltime);
		if ((ltime.tm_min/10!=m_LastMinute))
		{
			GetMeterDetails();
			m_LastMinute=ltime.tm_min/10;
		}
	}
	_log.Log(LOG_NORM,"Wunderground Worker stopped...");
}

void CWunderground::WriteToHardware(const char *pdata, const unsigned char length)
{

}

static std::string readWUndergroundTestFile( const char *path )
{
	FILE *file = fopen( path, "rb" );
	if ( !file )
		return std::string("");
	fseek( file, 0, SEEK_END );
	long size = ftell( file );
	fseek( file, 0, SEEK_SET );
	std::string text;
	char *buffer = new char[size+1];
	buffer[size] = 0;
	if ( fread( buffer, 1, size, file ) == (unsigned long)size )
		text = buffer;
	fclose( file );
	delete[] buffer;
	return text;
}

void CWunderground::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_WUNDERGROUND
	sResult=readWUndergroundTestFile("E:\\underground.json");
#else
	std::stringstream sURL;
	CURLEncode m_urlencoder;
	std::string szLoc=m_urlencoder.URLEncode(m_Location);
	sURL << "http://api.wunderground.com/api/" << m_APIKey << "/conditions/q/" << szLoc << ".json";
	bool bret;
	std::string szURL=sURL.str();
	bret=HTTPClient::GET(szURL,sResult);
	if (!bret)
	{
		_log.Log(LOG_ERROR,"Wunderground: Error getting http data!");
		return;
	}

#endif
	Json::Value root;

	Json::Reader jReader;
	bool ret=jReader.parse(sResult,root);
	if (!ret)
	{
		_log.Log(LOG_ERROR,"WUnderground: Invalid data received!");
		return;
	}
	if (root["current_observation"].empty()==true)
	{
		_log.Log(LOG_ERROR,"WUnderground: Invalid data received, or unknown location!");
		return;
	}
	std::string tmpstr;
	int pos;
	float temp;
	int humidity=0;
	int barometric=0;
	temp=root["current_observation"]["temp_c"].asFloat();

	if (root["current_observation"]["relative_humidity"].empty()==false)
	{
		tmpstr=root["current_observation"]["relative_humidity"].asString();
		pos=tmpstr.find("%");
		if (pos==std::string::npos)
		{
			_log.Log(LOG_ERROR,"WUnderground: Invalid data received!");
			return;
		}
		humidity=atoi(tmpstr.substr(0,pos).c_str());
	}
	if (root["current_observation"]["pressure_mb"].empty()==false)
	{
		barometric=atoi(root["current_observation"]["pressure_mb"].asString().c_str());
	}

	if (barometric!=0)
	{
		//Add temp+hum+baro device
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP_HUM_BARO.packetlength=sizeof(tsen.TEMP_HUM_BARO)-1;
		tsen.TEMP_HUM_BARO.packettype=pTypeTEMP_HUM_BARO;
		tsen.TEMP_HUM_BARO.subtype=sTypeTHB1;
		tsen.TEMP_HUM_BARO.battery_level=9;
		tsen.TEMP_HUM_BARO.rssi=6;
		tsen.TEMP_HUM_BARO.id1=0;
		tsen.TEMP_HUM_BARO.id2=1;

		tsen.TEMP_HUM_BARO.tempsign=(temp>=0)?0:1;
		int at10=round(abs(temp*10.0f));
		tsen.TEMP_HUM_BARO.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.TEMP_HUM_BARO.temperatureh*256);
		tsen.TEMP_HUM_BARO.temperaturel=(BYTE)(at10);
		tsen.TEMP_HUM_BARO.humidity=(BYTE)humidity;
		tsen.TEMP_HUM_BARO.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

		int ab10=round(barometric);
		tsen.TEMP_HUM_BARO.baroh=(BYTE)(ab10/256);
		ab10-=(tsen.TEMP_HUM_BARO.baroh*256);
		tsen.TEMP_HUM_BARO.barol=(BYTE)(ab10);
		tsen.TEMP_HUM_BARO.forecast=baroForecastNoInfo;
		if (barometric<1000)
			tsen.TEMP_HUM_BARO.forecast=baroForecastRain;
		else if (barometric<1020)
			tsen.TEMP_HUM_BARO.forecast=baroForecastCloudy;
		else if (barometric<1030)
			tsen.TEMP_HUM_BARO.forecast=baroForecastPartlyCloudy;
		else
			tsen.TEMP_HUM_BARO.forecast=baroForecastSunny;

		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM_BARO);//decode message
	}
	else if (humidity!=0)
	{
		//add temp+hum device
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
		tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
		tsen.TEMP_HUM.subtype=sTypeTH5;
		tsen.TEMP_HUM.battery_level=9;
		tsen.TEMP_HUM.rssi=6;
		tsen.TEMP_HUM.id1=0;
		tsen.TEMP_HUM.id2=1;

		tsen.TEMP_HUM.tempsign=(temp>=0)?0:1;
		int at10=round(abs(temp*10.0f));
		tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.TEMP_HUM.temperatureh*256);
		tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
		tsen.TEMP_HUM.humidity=(BYTE)humidity;
		tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);//decode message
	}
	else
	{
		//add temp device
		RBUF tsen;
		memset(&tsen,0,sizeof(RBUF));
		tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
		tsen.TEMP.packettype=pTypeTEMP;
		tsen.TEMP.subtype=sTypeTEMP10;
		tsen.TEMP.battery_level=9;
		tsen.TEMP.rssi=6;
		tsen.TEMP.id1=0;
		tsen.TEMP.id2=1;

		tsen.TEMP.tempsign=(temp>=0)?0:1;
		int at10=round(abs(temp*10.0f));
		tsen.TEMP.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.TEMP.temperatureh*256);
		tsen.TEMP.temperaturel=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);//decode message
	}

	//Wind
	int wind_degrees=-1;
	float wind_mph=-1;
	float wind_gust_mph=-1;
	float windspeed_ms=0;
	float windgust_ms=0;
	float wind_temp=temp;
	float wind_chill=temp;
	int windgust=1;
	float windchill=-1;

	if (root["current_observation"]["wind_degrees"].empty()==false)
	{
		wind_degrees=atoi(root["current_observation"]["wind_degrees"].asString().c_str());
	}
	if (root["current_observation"]["wind_mph"].empty()==false)
	{
		wind_mph=(float)atof(root["current_observation"]["wind_mph"].asString().c_str());
		//convert to m/s
		windspeed_ms=wind_mph*0.44704f;
	}
	if (root["current_observation"]["wind_gust_mph"].empty()==false)
	{
		wind_gust_mph=(float)atof(root["current_observation"]["wind_gust_mph"].asString().c_str());
		//convert to m/s
		windgust_ms=wind_gust_mph*0.44704f;
	}
	if (root["current_observation"]["feelslike_c"].empty()==false)
	{
		if (root["current_observation"]["feelslike_c"]!="N/A")
		{
			wind_chill=(float)atof(root["current_observation"]["feelslike_c"].asString().c_str());
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
		tsen.WIND.rssi=6;
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
		tsen.WIND.temperaturel;

		tsen.WIND.tempsign=(wind_temp>=0)?0:1;
		int at10=round(abs(wind_temp*10.0f));
		tsen.WIND.temperatureh=(BYTE)(at10/256);
		at10-=(tsen.WIND.temperatureh*256);
		tsen.WIND.temperaturel=(BYTE)(at10);

		tsen.WIND.chillsign=(wind_temp>=0)?0:1;
		at10=round(abs(wind_chill*10.0f));
		tsen.WIND.chillh=(BYTE)(at10/256);
		at10-=(tsen.WIND.chillh*256);
		tsen.WIND.chilll=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND);//decode message
	}

	//UV
	if (root["current_observation"]["UV"].empty()==false)
	{
		if (root["current_observation"]["UV"]!="N/A")
		{
			float UV=(float)atof(root["current_observation"]["UV"].asString().c_str());
			if (UV!=-1)
			{
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.UV.packetlength=sizeof(tsen.UV)-1;
				tsen.UV.packettype=pTypeUV;
				tsen.UV.subtype=sTypeUV1;
				tsen.UV.battery_level=9;
				tsen.UV.rssi=6;
				tsen.UV.id1=0;
				tsen.UV.id2=1;

				tsen.UV.uv=(BYTE)round(UV*10);
				sDecodeRXMessage(this, (const unsigned char *)&tsen.UV);//decode message
			}
		}
	}

	//Rain
	if (root["current_observation"]["precip_today_metric"].empty()==false)
	{
		if (root["current_observation"]["precip_today_metric"]!="N/A")
		{
			float RainCount=(float)atof(root["current_observation"]["precip_today_metric"].asString().c_str());
			if (RainCount!=-9999.00f)
			{
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.RAIN.packetlength=sizeof(tsen.RAIN)-1;
				tsen.RAIN.packettype=pTypeRAIN;
				tsen.RAIN.subtype=sTypeRAIN3;
				tsen.RAIN.battery_level=9;
				tsen.RAIN.rssi=6;
				tsen.RAIN.id1=0;
				tsen.RAIN.id2=1;

				tsen.RAIN.rainrateh=0;
				tsen.RAIN.rainratel=0;

				if (root["current_observation"]["precip_1hr_metric"].empty()==false)
				{
					if (root["current_observation"]["precip_1hr_metric"]!="N/A")
					{
						float rainrateph=(float)atof(root["current_observation"]["precip_1hr_metric"].asString().c_str());
						if (rainrateph!=-9999.00f)
						{
							int at10=round(abs(rainrateph*10.0f));
							tsen.RAIN.rainrateh=(BYTE)(at10/256);
							at10-=(tsen.RAIN.rainrateh*256);
							tsen.RAIN.rainratel=(BYTE)(at10);
						}
					}
				}

				int tr10=int((float(RainCount)*10.0f));
				tsen.RAIN.raintotal1=0;
				tsen.RAIN.raintotal2=(BYTE)(tr10/256);
				tr10-=(tsen.RAIN.raintotal2*256);
				tsen.RAIN.raintotal3=(BYTE)(tr10);

				sDecodeRXMessage(this, (const unsigned char *)&tsen.RAIN);//decode message
			}
		}
	}

	//Visibility
	if (root["current_observation"]["visibility_km"].empty()==false)
	{
		if (root["current_observation"]["visibility_km"]!="N/A")
		{
			float visibility=(float)atof(root["current_observation"]["visibility_km"].asString().c_str());
			_tGeneralDevice gdevice;
			gdevice.subtype=sTypeVisibility;
			gdevice.floatval1=visibility;
			sDecodeRXMessage(this, (const unsigned char *)&gdevice);
		}
	}
}

