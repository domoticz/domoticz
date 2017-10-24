#include "stdafx.h"
#include "Netatmo.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"

#define round(a) ( int ) ( a + .5 )

#ifdef _DEBUG
	//#define DEBUG_NetatmoWeatherStationR
#endif

#ifdef DEBUG_NetatmoWeatherStationW
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
#ifdef DEBUG_NetatmoWeatherStationR
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

struct _tNetatmoDevice
{
	std::string ID;
	std::string ModuleName;
	std::string StationName;
	std::vector<std::string> ModulesIDs;
	//Json::Value Modules;
};

CNetatmo::CNetatmo(const int ID, const std::string& username, const std::string& password) :
m_username(CURLEncode::URLEncode(username)),
m_password(CURLEncode::URLEncode(password)),
m_clientId("5588029e485a88af28f4a3c4"),
m_clientSecret("6vIpQVjNsL2A74Bd8tINscklLw2LKv7NhE9uW2")
{
	m_nextRefreshTs = mytime(NULL);
	m_isLogged = false;
	m_bIsHomeCoach = false;

	m_HwdID=ID;

	m_stoprequested=false;
	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_tSetpointUpdateTime = time(NULL);
	Init();
}

CNetatmo::~CNetatmo(void)
{
}

void CNetatmo::Init()
{
	m_RainOffset.clear();
	m_OldRainCounter.clear();
	m_bPollThermostat = true;
	m_bPollWeatherData = true;
	m_bFirstTimeThermostat = true;
	m_bFirstTimeWeatherData = true;
	m_bForceSetpointUpdate=false;
	m_bIsHomeCoach = false;
}

bool CNetatmo::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CNetatmo::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CNetatmo::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CNetatmo::Do_Work()
{
	int sec_counter = 600 - 5;
	bool bFirstTimeWS = true;
	bool bFirstTimeTH = true;
	_log.Log(LOG_STATUS, "Netatmo: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (!m_isLogged)
		{
			if (sec_counter % 30 == 0)
			{
				Login();
			}
		}
		if (m_isLogged)
		{
			if (RefreshToken())
			{
				if ((sec_counter % 600 == 0) || (bFirstTimeWS))
				{
					//Weather station data is updated every 10 minutes
					bFirstTimeWS = false;
					if ((m_bPollWeatherData)|| (sec_counter % 1200 == 0))
					{
						GetMeterDetails();
					}
				}
				if (m_bPollThermostat)
				{
					//Thermostat data is updated every hour
					if ((sec_counter % 600 == 0) || (bFirstTimeTH))
					{
						bFirstTimeTH = false;
						GetThermostatDetails();
					}
					if (m_bForceSetpointUpdate)
					{
						time_t atime = time(NULL);
						if (atime >= m_tSetpointUpdateTime)
						{
							m_bForceSetpointUpdate = false;
							GetThermostatDetails();
						}
					}
				}

			}
		}
	}
	_log.Log(LOG_STATUS,"Netatmo: Worker stopped...");
}

static unsigned int crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static unsigned int Crc32(unsigned int crc, const unsigned char *buf, size_t size)
{
	const unsigned char *p = buf;
	crc = crc ^ ~0U;
	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return crc ^ ~0U;
}

bool CNetatmo::Login()
{
	if (m_isLogged)
		return true;

	if (LoadRefreshToken())
	{
		if (RefreshToken(true))
		{
			m_isLogged = true;
			m_bPollThermostat = true;
			return true;
		}
	}
	std::stringstream sstr;
	sstr << "grant_type=password&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret << "&";
	sstr << "username=" << m_username << "&";
	sstr << "password=" << m_password << "&";
	sstr << "scope=read_station read_thermostat write_thermostat read_homecoach";

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.netatmo.net/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid/no data received...");
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		_log.Log(LOG_ERROR, "Netatmo: No access granted, check username/password...");
		return false;
	}

	//Initial Access Token
	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	//_log.Log(LOG_STATUS, "Access token: %s", m_accessToken.c_str());
	//_log.Log(LOG_STATUS, "RefreshToken: %s", m_refreshToken.c_str());
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(NULL) + expires;
	StoreRefreshToken();
	m_isLogged = true;
	return true;
}

bool CNetatmo::RefreshToken(const bool bForce)
{
	if (m_refreshToken.empty())
	{
		return false;
	}
	if (!bForce)
	{
		if (!m_isLogged)
			return false;
		if ((mytime(NULL) - 15) < m_nextRefreshTs)
			return true; //no need to refresh the token yet
	}

	// Time to refresh the token
	std::stringstream sstr;
	sstr << "grant_type=refresh_token&";
	sstr << "refresh_token=" << m_refreshToken << "&";
	sstr << "client_id=" << m_clientId << "&";
	sstr << "client_secret=" << m_clientSecret;

	std::string httpData = sstr.str();
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::string httpUrl("https://api.netatmo.net/oauth2/token");
	std::string sResult;
	bool ret = HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return false;
	}

	Json::Value root;
	Json::Reader jReader;
	ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid/no data received...");
		//Force login next time
		m_isLogged = false;
		return false;
	}

	if (root["access_token"].empty() || root["expires_in"].empty() || root["refresh_token"].empty())
	{
		//Force login next time
		_log.Log(LOG_ERROR, "Netatmo: No access granted, forcing login again...");
		m_isLogged = false;
		return false;
	}

	m_accessToken = root["access_token"].asString();
	m_refreshToken = root["refresh_token"].asString();
	int expires = root["expires_in"].asInt();
	m_nextRefreshTs = mytime(NULL) + expires;
	//StoreRefreshToken();
	return true;
}

bool CNetatmo::LoadRefreshToken()
{
	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query("SELECT Extra FROM Hardware WHERE (ID==%d)",m_HwdID);
	if (result.empty())
		return false;
	std::string refreshToken = result[0][0];
	if (refreshToken.empty())
		return false;
	m_refreshToken = refreshToken;
	return true;
}

void CNetatmo::StoreRefreshToken()
{
	if (m_refreshToken.empty())
		return;
	m_sql.safe_query("UPDATE Hardware SET Extra='%q' WHERE (ID == %d)", m_refreshToken.c_str(), m_HwdID);
}

int CNetatmo::GetBatteryLevel(const std::string &ModuleType, const int battery_percent)
{
    int batValue = 255;

    // Others are plugged
    if ((ModuleType == "NAModule1") || (ModuleType == "NAModule2") || (ModuleType == "NAModule3") || (ModuleType == "NAModule4") || (ModuleType == "NATherm1"))
        batValue = battery_percent;
    
    return batValue;
}

bool CNetatmo::ParseDashboard(const Json::Value &root, const int DevIdx, const int ID, const std::string &name, const std::string &ModuleType, const int battery_percent, const int rssiLevel)
{
	bool bHaveTemp = false;
	bool bHaveHum = false;
	bool bHaveBaro = false;
	bool bHaveCO2 = false;
	bool bHaveRain = false;
	bool bHaveSound = false;
	bool bHaveWind = false;
	bool bHaveSetpoint = false;

	float temp,sp_temp;
	int hum;
	float baro;
	int co2;
	float rain;
	int sound;

	int wind_angle = 0;
	int wind_gust_angle = 0;
	float wind_strength = 0;
	float wind_gust = 0;

	int batValue = GetBatteryLevel(ModuleType, battery_percent);

	if (!root["Temperature"].empty())
	{
		bHaveTemp = true;
		temp = root["Temperature"].asFloat();
	}
	else if (!root["temperature"].empty())
	{
		bHaveTemp = true;
		temp = root["temperature"].asFloat();
	}
	if (!root["Sp_Temperature"].empty())
	{
		bHaveSetpoint = true;
		sp_temp = root["Temperature"].asFloat();
	}
	else if (!root["setpoint_temp"].empty())
	{
		bHaveSetpoint = true;
		sp_temp = root["setpoint_temp"].asFloat();
	}
	if (!root["Humidity"].empty())
	{
		bHaveHum = true;
		hum = root["Humidity"].asInt();
	}
	if (!root["Pressure"].empty())
	{
		bHaveBaro = true;
		baro = root["Pressure"].asFloat();
	}
/*
	if (!root["AbsolutePressure"].empty())
	{
		float apressure = root["AbsolutePressure"].asFloat();
	}
*/
	if (!root["Noise"].empty())
	{
		bHaveSound = true;
		sound = root["Noise"].asInt();
	}
	if (!root["CO2"].empty())
	{
		bHaveCO2 = true;
		co2 = root["CO2"].asInt();
	}
	if (!root["sum_rain_24"].empty())
	{
		bHaveRain = true;
		rain = root["sum_rain_24"].asFloat();
	}
	if (!root["WindAngle"].empty())
	{
		if (
			(!root["WindAngle"].empty()) &&
			(!root["WindStrength"].empty()) &&
			(!root["GustAngle"].empty()) &&
			(!root["GustStrength"].empty())
			)
		{
			bHaveWind = true;
			wind_angle = root["WindAngle"].asInt();
			wind_gust_angle = root["GustAngle"].asInt();
			wind_strength = root["WindStrength"].asFloat()/ 3.6f;
			wind_gust = root["GustStrength"].asFloat() / 3.6f;
		}
	}

	if (bHaveTemp && bHaveHum && bHaveBaro)
	{
		int nforecast = CalculateBaroForecast(baro);
		if (temp < 0)
		{
			if (
				(nforecast == wsbaroforcast_rain) ||
				(nforecast == wsbaroforcast_heavy_rain)
				)
			{
				nforecast = wsbaroforcast_snow;
			}
		}
		if (nforecast == wsbaroforcast_unknown)
		{
			nforecast = wsbaroforcast_some_clouds;
			float pressure = baro;
			if (pressure <= 980)
				nforecast = wsbaroforcast_heavy_rain;
			else if (pressure <= 995)
			{
				if (temp > 1)
					nforecast = wsbaroforcast_rain;
				else
					nforecast = wsbaroforcast_snow;
			}
			else if (pressure >= 1029)
				nforecast = wsbaroforcast_sunny;
		}
		SendTempHumBaroSensorFloat(ID, batValue, temp, hum, baro, nforecast, name, rssiLevel);
	}
	else if (bHaveTemp && bHaveHum)
	{
		SendTempHumSensor(ID, batValue, temp, hum, name, rssiLevel);
	}
	else if (bHaveTemp)
	{
		SendTempSensor(ID, batValue, temp, name, rssiLevel);
	}

	if (bHaveSetpoint)
	{
		std::string sName = "SetPoint " + name;
		SendSetPointSensor(DevIdx, 0, ID & 0xFF, sp_temp, sName);
	}

	if (bHaveRain)
	{
		bool bRefetchData = (m_RainOffset.find(ID) == m_RainOffset.end());
		if (!bRefetchData)
		{
			bRefetchData = ((m_RainOffset[ID] == 0) && (m_OldRainCounter[ID] == 0));
		}
		if (bRefetchData)
		{
			//get last rain counter from the database
			bool bExists=false;
			m_RainOffset[ID] = GetRainSensorValue(ID, bExists);
			m_RainOffset[ID] -= rain;
			if (m_RainOffset[ID] < 0)
				m_RainOffset[ID] = 0;
			if (m_OldRainCounter.find(ID) == m_OldRainCounter.end())
			{
				m_OldRainCounter[ID] = 0;
			}
		}
		if (rain < m_OldRainCounter[ID])
		{
			//daily counter went to zero
			m_RainOffset[ID] += m_OldRainCounter[ID];
		}
		m_OldRainCounter[ID] = rain;
		SendRainSensor(ID, batValue, m_RainOffset[ID] + m_OldRainCounter[ID], name, rssiLevel);
	}

	if (bHaveCO2)
	{
		SendAirQualitySensor((ID & 0xFF00) >> 8, ID & 0xFF, batValue, co2, name);
	}

	if (bHaveSound)
	{
		SendSoundSensor(ID, batValue, sound, name);
	}
	
	if (bHaveWind)
	{
		SendWind(ID, batValue, wind_angle, wind_strength, wind_gust, 0, 0, false, name, rssiLevel);
	}
	return true;
}

bool CNetatmo::WriteToHardware(const char *pdata, const unsigned char length)
{
	if ((m_thermostatDeviceID.empty()) || (m_thermostatModuleID.empty()))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
		return false;
	}

	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	if (pCmd->LIGHTING2.packettype != pTypeLighting2)
		return false; //later add RGB support, if someone can provide access

	int node_id = pCmd->LIGHTING2.id4;
	int therm_idx = pCmd->LIGHTING2.unitcode;

	bool bIsOn = (pCmd->LIGHTING2.cmnd == light2_sOn);

	if ((node_id == 3) && (therm_idx >= 0))
	{
		//Away
		return SetAway(therm_idx - 1, bIsOn);
	}

	return false;
}

bool CNetatmo::SetAway(const int idx, const bool bIsAway)
{
	return SetProgramState(idx, (bIsAway == true) ? 1 : 0);
}

bool CNetatmo::SetProgramState(const int idx, const int newState)
{
	if (idx >= (int)m_thermostatDeviceID.size())
		return false;
	if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
		return false;
	}
	if (!m_isLogged == true)
	{
		if (!Login())
			return false;
	}
	std::string thermState;
	switch (newState)
	{
	case 0:
		thermState = "program"; //The Thermostat is currently following its weekly schedule
		break;
	case 1:
		thermState = "away"; //The Thermostat is currently applying the away temperature
		break;
	case 2:
		thermState = "hg"; //he Thermostat is currently applying the frost-guard temperature
		break;
	case 3:
		thermState = "off"; //The Thermostat is off
		break;
	default:
		_log.Log(LOG_ERROR, "NetatmoThermostat: Invalid thermostat state!");
		return false;
	}
	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	std::stringstream sstr;
	sstr << "access_token=" << m_accessToken;
	sstr << "&device_id=" << m_thermostatDeviceID[idx];
	sstr << "&module_id=" << m_thermostatModuleID[idx];
	sstr << "&setpoint_mode=" << thermState;

	std::string httpData = sstr.str();

	std::string httpUrl("https://api.netatmo.net/api/setthermpoint");
	std::string sResult;

	if (!HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint state!");
		return false;
	}
	GetThermostatDetails();
	return true;
}

void CNetatmo::SetSetpoint(const int idx, const float temp)
{
	if (idx >= (int)m_thermostatDeviceID.size())
		return;
	if ((m_thermostatDeviceID[idx].empty()) || (m_thermostatModuleID[idx].empty()))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: No thermostat found in online devices!");
		return;
	}
	if (!m_isLogged == true)
	{
		if (!Login())
			return;
	}

	std::vector<std::string> ExtraHeaders;

	ExtraHeaders.push_back("Host: api.netatmo.net");
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

	float tempDest = temp;
	unsigned char tSign = m_sql.m_tempsign[0];

	if (tSign == 'F')
	{
		//convert back to Celsius
		tempDest = static_cast<float>(ConvertToCelsius(tempDest));
	}

	time_t now = mytime(NULL);
	struct tm etime;
	localtime_r(&now, &etime);
	time_t end_time;
	int isdst = etime.tm_isdst;
	bool goodtime = false;
	while (!goodtime) {
		etime.tm_isdst = isdst;
		etime.tm_hour += 1;
		end_time = mktime(&etime);
		goodtime = (etime.tm_isdst == isdst);
		isdst = etime.tm_isdst;
		if (!goodtime) {
			localtime_r(&now, &etime);
		}
	}

	std::stringstream sstr;
	sstr << "access_token=" << m_accessToken;
	sstr << "&device_id=" << m_thermostatDeviceID[idx];
	sstr << "&module_id=" << m_thermostatModuleID[idx];
	sstr << "&setpoint_mode=manual";
	sstr << "&setpoint_temp=" << tempDest;
	sstr << "&setpoint_endtime=" << end_time;

	std::string httpData = sstr.str();

	std::string httpUrl("https://api.netatmo.net/api/setthermpoint");
	std::string sResult;

	if (!HTTPClient::POST(httpUrl, httpData, ExtraHeaders, sResult))
	{
		_log.Log(LOG_ERROR, "NetatmoThermostat: Error setting setpoint!");
		return;
	}
	GetThermostatDetails();
	m_tSetpointUpdateTime = time(NULL) + 60;
	m_bForceSetpointUpdate = true;
}

bool CNetatmo::ParseNetatmoGetResponse(const std::string &sResult, const bool bIsThermostat)
{
	Json::Value root;
	Json::Reader jReader;
	bool ret = jReader.parse(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		_log.Log(LOG_STATUS, "Netatmo: Invalid data received...");
		return false;
	}
	bool bHaveDevices = true;
	if (root["body"].empty())
	{
		bHaveDevices = false;
	}
	else if (root["body"]["devices"].empty())
	{
		bHaveDevices = false;
	}
	else if (!root["body"]["devices"].isArray())
	{
		bHaveDevices = false;
	}
	if (!bHaveDevices)
	{
		if ((!bIsThermostat) && (!m_bFirstTimeWeatherData) && (m_bPollWeatherData))
		{
			//Do not warn if we check if we have a Thermostat device
			_log.Log(LOG_STATUS, "Netatmo: No Weather Station devices found...");
		}
		return false;
	}

	std::vector<_tNetatmoDevice> _netatmo_devices;

	int iDevIndex = 0;
	for (Json::Value::iterator itDevice = root["body"]["devices"].begin(); itDevice != root["body"]["devices"].end(); ++itDevice)
	{
		Json::Value device = *itDevice;
		if (!device["_id"].empty())
		{
			std::string id = device["_id"].asString();
			std::string type = device["type"].asString();
			std::string name = device["module_name"].asString();
			std::string station_name;
			if (!device["station_name"].empty())
				station_name = device["station_name"].asString();
			else if (!device["name"].empty())
				station_name = device["name"].asString();

			stdreplace(name, "'", "");
			stdreplace(station_name, "'", "");

			_tNetatmoDevice nDevice;
			nDevice.ID = id;
			nDevice.ModuleName = name;
			nDevice.StationName = station_name;

			if (!device["modules"].empty())
			{
				if (device["modules"].isArray())
				{
					//Add modules for this device
					for (Json::Value::iterator itModule = device["modules"].begin(); itModule != device["modules"].end(); ++itModule)
					{
						Json::Value module = *itModule;
						if (module.isObject())
						{
							//New Method (getstationsdata and getthermostatsdata)
							if (module["_id"].empty())
							{
								iDevIndex++;
								continue;
							}
							std::string mid = module["_id"].asString();
							std::string mtype = module["type"].asString();
							std::string mname = module["module_name"].asString();
							if (mname.empty())
								mname = nDevice.ModuleName;
							int mbattery_percent = 0;
							if (!module["battery_percent"].empty())
							{
								mbattery_percent = module["battery_percent"].asInt();
							}
                            int mrf_status = 0;
                            if (!module["rf_status"].empty())
                            {
                                // 90=low, 60=highest
                                mrf_status = ( 90 - module["rf_status"].asInt())/3;
                                if (mrf_status > 10){
                                    mrf_status = 10;
                                }
                            }
							int crcId = Crc32(0, (const unsigned char *)mid.c_str(), mid.length());
							if (!module["dashboard_data"].empty())
							{
								ParseDashboard(module["dashboard_data"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
							}
							else if (!module["measured"].empty())
							{
								ParseDashboard(module["measured"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
								if (mtype == "NATherm1")
								{
									m_thermostatDeviceID[iDevIndex] = nDevice.ID;
									m_thermostatModuleID[iDevIndex] = mid;

									if (!module["setpoint"].empty())
									{
										if (!module["setpoint"]["setpoint_mode"].empty())
										{
											std::string setpoint_mode = module["setpoint"]["setpoint_mode"].asString();
											bool bIsAway = (setpoint_mode == "away");
											std::string aName = "Away " + mname;
											SendSwitch(3, 1 + iDevIndex, 255, bIsAway, 0, aName);
										}
										//Check if setpoint was just set, and if yes, overrule the previous setpoint
										if (!module["setpoint"]["setpoint_temp"].empty())
										{
											ParseDashboard(module["setpoint"], iDevIndex, crcId, mname, mtype, mbattery_percent, mrf_status);
										}
									}
								}
							}
						}
						else
							nDevice.ModulesIDs.push_back(module.asString());
					}
				}
			}
			_netatmo_devices.push_back(nDevice);

			int battery_percent = 0;
			if (!device["battery_percent"].empty())
			{
				battery_percent = device["battery_percent"].asInt();
			}
            int wifi_status = 0;
            if (!device["wifi_status"].empty())
            {
                // 86=bad, 56=good
                wifi_status = ( 86 - device["wifi_status"].asInt())/3;
                if (wifi_status > 10){
                    wifi_status = 10;
                }
            }
			int crcId = Crc32(0, (const unsigned char *)id.c_str(), id.length());
			if (!device["dashboard_data"].empty())
			{
				ParseDashboard(device["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, wifi_status);
			}
		}
		iDevIndex++;
	}

	if (root["body"]["modules"].empty())
	{
		//No additional modules defined
		return (!_netatmo_devices.empty());
	}
	if (!root["body"]["modules"].isArray())
	{
		//No additional modules defined
		return (!_netatmo_devices.empty());
	}
	for (Json::Value::iterator itModule = root["body"]["modules"].begin(); itModule != root["body"]["modules"].end(); ++itModule)
	{
		Json::Value module = *itModule;
		if (module["_id"].empty())
			continue;
		std::string id = module["_id"].asString();
		std::string type = module["type"].asString();
		std::string name = "Unknown";

		//Find the corresponding _tNetatmoDevice
		_tNetatmoDevice nDevice;
		bool bHaveFoundND = false;
		int iDevIndex = 0;
		std::vector<_tNetatmoDevice>::const_iterator ittND;
		for (ittND = _netatmo_devices.begin(); ittND != _netatmo_devices.end(); ++ittND)
		{
			std::vector<std::string>::const_iterator ittNM;
			for (ittNM = ittND->ModulesIDs.begin(); ittNM != ittND->ModulesIDs.end(); ++ittNM)
			{
				if (*ittNM == id)
				{
					nDevice = *ittND;
					iDevIndex = (ittND - _netatmo_devices.begin());
					bHaveFoundND = true;
					break;
				}
			}
			if (bHaveFoundND == true)
				break;
		}

		if (!module["module_name"].empty())
		{
			name = module["module_name"].asString();
		}
		else
		{
			if (bHaveFoundND)
			{
				if (!nDevice.ModuleName.empty())
					name = nDevice.ModuleName;
				else if (!nDevice.StationName.empty())
					name = nDevice.StationName;
			}
		}

		if (type == "NATherm1")
		{
			continue; //handled above
		}

		int battery_percent = 0;
		if (!module["battery_percent"].empty())
		{
			battery_percent = module["battery_percent"].asInt();
		}
        int rf_status = 0;
        if (!module["rf_status"].empty())
        {
            // 90=low, 60=highest
            rf_status = ( 90 - module["rf_status"].asInt())/3;
            if (rf_status > 10){
                rf_status = 10;
            }
        }
		stdreplace(name, "'", " ");

		//std::set<std::string> dataTypes;
		//for (Json::Value::iterator itDataType = module["data_type"].begin(); itDataType != module["data_type"].end(); ++itDataType)
		//{
		//	dataTypes.insert((*itDataType).asCString());
		//}
		int crcId = Crc32(0, (const unsigned char *)id.c_str(), id.length());
		if (!module["dashboard_data"].empty())
		{
			ParseDashboard(module["dashboard_data"], iDevIndex, crcId, name, type, battery_percent, rf_status);
		}
	}
	return (!_netatmo_devices.empty());
}

std::string CNetatmo::MakeRequestURL(const bool bIsHomeCoach)
{
	std::stringstream sstr;
	//	sstr2 << "https://api.netatmo.net/api/devicelist";
	if (bIsHomeCoach)
		sstr << "https://api.netatmo.net/api/gethomecoachsdata";
	else
		sstr << "https://api.netatmo.net/api/getstationsdata";


	sstr << "?";
	sstr << "access_token=" << m_accessToken;
	sstr << "&" << "get_favorites=" << "true";
	return sstr.str();
}

void CNetatmo::GetMeterDetails()
{
	if (!m_isLogged)
		return;

	std::string httpUrl = MakeRequestURL(m_bIsHomeCoach);

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

#ifdef DEBUG_NetatmoWeatherStationR
	//sResult = ReadFile("E:\\netatmo_mdetails.json");
	sResult = ReadFile("E:\\netatmo_getstationdata.json");
	bool ret = true;
#else
	bool ret=HTTPClient::GET(httpUrl, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
		return;
	}
#endif
#ifdef DEBUG_NetatmoWeatherStationW
	SaveString2Disk(sResult, "E:\\netatmo_getstationdata.json");
#endif

	//Check for error
	Json::Value root;
	Json::Reader jReader;
	bool bRet = jReader.parse(sResult, root);
	if ((!bRet) || (!root.isObject()))
	{
		_log.Log(LOG_ERROR, "Netatmo: Invalid data received...");
		return;
	}

	if (!root["error"].empty())
	{
		//We received an error
		_log.Log(LOG_ERROR, "Netatmo: %s", root["error"]["message"].asString().c_str());
		m_isLogged = false;
		return;
	}

	if (!ParseNetatmoGetResponse(sResult, false))
	{
		if (m_bFirstTimeWeatherData)
		{
			m_bFirstTimeWeatherData = false;
			//Check if the user has an Home Coach device
			httpUrl = MakeRequestURL(true);
			bool ret = HTTPClient::GET(httpUrl, ExtraHeaders, sResult);
			if (!ret)
			{
				_log.Log(LOG_ERROR, "Netatmo: Error connecting to Server...");
				return;
			}
			bool bRet = jReader.parse(sResult, root);
			if ((!bRet) || (!root.isObject()))
			{
				_log.Log(LOG_ERROR, "Netatmo: Invalid data received...");
				return;
			}
			if (!root["error"].empty())
			{
				//We received an error
				_log.Log(LOG_ERROR, "Netatmo: %s", root["error"]["message"].asString().c_str());
				m_isLogged = false;
				return;
			}
			m_bIsHomeCoach = ParseNetatmoGetResponse(sResult, false);
			if (!m_bIsHomeCoach)
			{
				m_bPollWeatherData = false;
			}
		}
	}
	m_bFirstTimeWeatherData = false;
}

void CNetatmo::GetThermostatDetails()
{
	if (!m_isLogged)
		return;

	std::stringstream sstr2;
	sstr2 << "https://api.netatmo.net/api/getthermostatsdata";
	sstr2 << "?";
	sstr2 << "access_token=" << m_accessToken;
	std::string httpUrl = sstr2.str();

	std::vector<std::string> ExtraHeaders;
	std::string sResult;

#ifdef DEBUG_NetatmoWeatherStationR
	sResult = ReadFile("E:\\netatmo_mdetails_thermostat_0002.json");
	bool ret = true;
#else
	bool ret = HTTPClient::GET(httpUrl, ExtraHeaders, sResult);
	if (!ret)
	{
		_log.Log(LOG_STATUS, "Netatmo: Error connecting to Server...");
		return;
	}
#endif
#ifdef DEBUG_NetatmoWeatherStationW
	static int cntr = 1;
	char szFileName[255];
	sprintf(szFileName, "E:\\netatmo_mdetails_thermostat_%04d.json", cntr++);
	SaveString2Disk(sResult, szFileName);
#endif
	if (!ParseNetatmoGetResponse(sResult,true))
	{
		if (m_bFirstTimeThermostat)
		{
			m_bFirstTimeThermostat = false;
			m_bPollThermostat = false;
		}
	}
}

