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

#define DATE_TIME_ISO  "%FT%TZ"     // 2020-05-11T16:00:00Z

#ifdef _DEBUG
#define DEBUG_MeteorologiskR
#define DEBUG_MeteorologiskW
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

CMeteorologisk::CMeteorologisk(const int ID, const std::string &APIKey, const std::string &Location) :
m_APIKey(APIKey),
m_Location(Location)
{
	m_HwdID=ID;
	Init();
}

CMeteorologisk::~CMeteorologisk(void)
{
}

void CMeteorologisk::Init()
{
}

bool CMeteorologisk::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CMeteorologisk::Do_Work, this);
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
			m_LastHeartbeat = mytime(NULL);
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

    if(!szLoc.empty()){
        std::vector<std::string> strarray;
        StringSplit(szLoc, ";", strarray);

        Latitude = strarray[0];
        Longitude = strarray[1];
    }

    if(Latitude == "1" || Longitude == "1"){
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
    } else {
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
	/*
	std::string tmpstr2 = root.toStyledString();
    FILE *fOut = fopen("/tmp/Meteorologisk.json", "wb+");
	fwrite(tmpstr2.c_str(), 1, tmpstr2.size(), fOut);
	fclose(fOut);
	*/

    float temperature;
	int humidity=0;
    float barometric=0;
	int barometric_forcast=baroForecastNoInfo;

    Json::Value timeseries = root["properties"]["timeseries"];

    Json::Value selectedTimeserie = NULL;
    time_t now = time(NULL);

    for(uint i=0; i<timeseries.size(); i++)
    {
        struct tm tm = {0};
        Json::Value timeserie = timeseries[i];
        const char* stime = timeserie["time"].asCString();
        char* s = strptime(stime, DATE_TIME_ISO, &tm);

        if(s!=NULL)
        {
            double diff = difftime(mktime(&tm), now);
            if(diff > 0)
            {
                selectedTimeserie = timeserie;
                i = timeseries.size();
            }
        }
    }

    if(selectedTimeserie == NULL){
        //TODO: error message
    }

    Json::Value instantData = selectedTimeserie["data"]["instant"]["details"];

    //The api already provides the temperature in celcius.
    temperature=instantData["air_temperature"].asFloat();

    if (instantData["relative_humidity"].empty()==false)
	{
        humidity=round(instantData["relative_humidity"].asFloat());
	}
    if (instantData["air_pressure_at_sea_level"].empty()==false)
	{
        barometric=atof(instantData["air_pressure_at_sea_level"].asString().c_str());
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
        SendTempHumBaroSensor(1, 255, temperature, humidity, static_cast<float>(barometric), barometric_forcast, "THB");
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
    float wind_degrees=-1;
	float windspeed_ms=0;
	float windgust_ms=0;
    float wind_temp=temperature;
    float wind_chill=temperature;
	//int windgust=1;
	//float windchill=-1;

    if (instantData["wind_from_direction"].empty()==false)
	{
        wind_degrees=atof(instantData["wind_from_direction"].asString().c_str());
	}
    if (instantData["wind_speed"].empty()==false)
	{
        windspeed_ms = atof(instantData["wind_speed"].asString().c_str());
	}
    if (instantData["wind_speed_of_gust"].empty()==false)
    {
        windgust_ms=atof(instantData["wind_speed_of_gust"].asString().c_str());
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

		tsen.WIND.chillsign=(wind_chill>=0)?0:1;
		at10=round(std::abs(wind_chill*10.0f));
		tsen.WIND.chillh=(BYTE)(at10/256);
		at10-=(tsen.WIND.chillh*256);
		tsen.WIND.chilll=(BYTE)(at10);

		sDecodeRXMessage(this, (const unsigned char *)&tsen.WIND, NULL, 255);
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

	//Rain
	if (root["currently"]["precipIntensity"].empty()==false)
	{
		if ((root["currently"]["precipIntensity"] != "N/A") && (root["currently"]["precipIntensity"] != "--"))
		{
			float rainrateph = static_cast<float>(atof(root["currently"]["precipIntensity"].asString().c_str()))*25.4f; //inches to mm
			if ((rainrateph !=-9999.00f)&&(rainrateph >=0.00f))
			{
				SendRainRateSensor(1, 255, rainrateph, "Rain");
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
			float radiation = static_cast<float>(atof(root["currently"]["ozone"].asString().c_str()));
			if (radiation>=0.0f)
			{
				SendCustomSensor(1, 0, 255, radiation, "Ozone Sensor", "DU"); //(dobson units)
			}
		}
	}
	//Cloud Cover
	if (root["currently"]["cloudCover"].empty() == false)
	{
		if ((root["currently"]["cloudCover"] != "N/A") && (root["currently"]["cloudCover"] != "--"))
		{
			float cloudcover = static_cast<float>(atof(root["currently"]["cloudCover"].asString().c_str()));
			if (cloudcover >= 0.0f)
			{
				SendPercentageSensor(1, 0, 255, cloudcover * 100.0f, "Cloud Cover");
			}
		}
	}

}

