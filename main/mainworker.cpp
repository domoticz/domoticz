#include "stdafx.h"
#include "mainworker.h"
#include "Helper.h"
#include "SunRiseSet.h"
#include "localtime_r.h"
#include "Logger.h"
#include "WebServerHelper.h"
#include "SQLHelper.h"
#include "../push/FibaroPush.h"
#include "../push/HttpPush.h"
#include "../push/InfluxPush.h"
#include "../push/GooglePubSubPush.h"

#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"
#include <boost/algorithm/string/join.hpp>

#include <boost/crc.hpp>
#include <algorithm>
#include <set>

//Hardware Devices
#include "../hardware/hardwaretypes.h"
#include "../hardware/RFXBase.h"
#include "../hardware/RFXComSerial.h"
#include "../hardware/RFXComTCP.h"
#include "../hardware/DomoticzTCP.h"
#include "../hardware/P1MeterBase.h"
#include "../hardware/P1MeterSerial.h"
#include "../hardware/P1MeterTCP.h"
#include "../hardware/YouLess.h"
#ifdef WITH_LIBUSB
#include "../hardware/TE923.h"
#include "../hardware/VolcraftCO20.h"
#endif
#include "../hardware/Rego6XXSerial.h"
#include "../hardware/Razberry.h"
#ifdef WITH_OPENZWAVE
#include "../hardware/OpenZWave.h"
#endif
#include "../hardware/DavisLoggerSerial.h"
#include "../hardware/1Wire.h"
#include "../hardware/I2C.h"
#include "../hardware/Wunderground.h"
#include "../hardware/DarkSky.h"
#include "../hardware/HardwareMonitor.h"
#include "../hardware/Dummy.h"
#include "../hardware/Tellstick.h"
#include "../hardware/PiFace.h"
#include "../hardware/S0MeterSerial.h"
#include "../hardware/S0MeterTCP.h"
#include "../hardware/OTGWSerial.h"
#include "../hardware/OTGWTCP.h"
#include "../hardware/TeleinfoBase.h"
#include "../hardware/TeleinfoSerial.h"
#include "../hardware/Limitless.h"
#include "../hardware/MochadTCP.h"
#include "../hardware/EnOceanESP2.h"
#include "../hardware/EnOceanESP3.h"
#include "../hardware/SBFSpot.h"
#include "../hardware/PhilipsHue/PhilipsHue.h"
#include "../hardware/ICYThermostat.h"
#include "../hardware/WOL.h"
#include "../hardware/Meteostick.h"
#include "../hardware/PVOutput_Input.h"
#include "../hardware/ToonThermostat.h"
#include "../hardware/HarmonyHub.h"
#include "../hardware/EcoDevices.h"
#include "../hardware/EvohomeBase.h"
#include "../hardware/EvohomeScript.h"
#include "../hardware/EvohomeSerial.h"
#include "../hardware/EvohomeTCP.h"
#include "../hardware/EvohomeWeb.h"
#include "../hardware/MySensorsSerial.h"
#include "../hardware/MySensorsTCP.h"
#include "../hardware/MySensorsMQTT.h"
#include "../hardware/MQTT.h"
#include "../hardware/FritzboxTCP.h"
#include "../hardware/ETH8020.h"
#include "../hardware/RFLinkSerial.h"
#include "../hardware/RFLinkTCP.h"
#include "../hardware/KMTronicSerial.h"
#include "../hardware/KMTronicTCP.h"
#include "../hardware/KMTronicUDP.h"
#include "../hardware/KMTronic433.h"
#include "../hardware/SolarMaxTCP.h"
#include "../hardware/Pinger.h"
#include "../hardware/Nest.h"
#include "../hardware/Thermosmart.h"
#include "../hardware/Kodi.h"
#include "../hardware/Netatmo.h"
#include "../hardware/HttpPoller.h"
#include "../hardware/AnnaThermostat.h"
#include "../hardware/Winddelen.h"
#include "../hardware/SatelIntegra.h"
#include "../hardware/LogitechMediaServer.h"
#include "../hardware/Comm5TCP.h"
#include "../hardware/Comm5Serial.h"
#include "../hardware/CurrentCostMeterSerial.h"
#include "../hardware/CurrentCostMeterTCP.h"
#include "../hardware/SolarEdgeAPI.h"
#include "../hardware/DomoticzInternal.h"
#include "../hardware/NefitEasy.h"
#include "../hardware/PanasonicTV.h"
#include "../hardware/OpenWebNetTCP.h"
#include "../hardware/AtagOne.h"
#include "../hardware/Sterbox.h"
#include "../hardware/RAVEn.h"
#include "../hardware/DenkoviSmartdenLan.h"
#include "../hardware/DenkoviSmartdenIPInOut.h"
#include "../hardware/AccuWeather.h"
#include "../hardware/BleBox.h"
#include "../hardware/Ec3kMeterTCP.h"
#include "../hardware/OpenWeatherMap.h"
#include "../hardware/GoodweAPI.h"
#include "../hardware/Daikin.h"
#include "../hardware/HEOS.h"
#include "../hardware/MultiFun.h"
#include "../hardware/ZiBlueSerial.h"
#include "../hardware/ZiBlueTCP.h"
#include "../hardware/Yeelight.h"
#include "../hardware/XiaomiGateway.h"
#include "../hardware/plugins/Plugins.h"
#include "../hardware/Arilux.h"
#include "../hardware/OpenWebNetUSB.h"
#include "../hardware/InComfort.h"
#include "../hardware/RelayNet.h"
#include "../hardware/SysfsGpio.h"
#include "../hardware/Rtl433.h"
#include "../hardware/OnkyoAVTCP.h"
#include "../hardware/USBtin.h"
#include "../hardware/USBtin_MultiblocV8.h"
#include "../hardware/EnphaseAPI.h"

// load notifications configuration
#include "../notifications/NotificationHelper.h"

#ifdef WITH_GPIO
#include "../hardware/Gpio.h"
#include "../hardware/GpioPin.h"
#endif

#ifdef WIN32
#include "../msbuild/WindowsHelper.h"
#include "dirent_windows.h"
#else
#include <sys/utsname.h>
#include <dirent.h>
#endif

#include "mainstructs.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef _DEBUG
	//#define PARSE_RFXCOM_DEVICE_LOG
	//#define DEBUG_DOWNLOAD
	//#define DEBUG_RXQUEUE
#endif

#ifdef PARSE_RFXCOM_DEVICE_LOG
#include <iostream>
#include <fstream>
#endif

#define round(a) ( int ) ( a + .5 )

extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWWWFolder;
extern std::string szAppVersion;
extern std::string szWebRoot;

extern http::server::CWebServerHelper m_webservers;

CFibaroPush m_fibaropush;
CGooglePubSubPush m_googlepubsubpush;
CHttpPush m_httppush;
CInfluxPush m_influxpush;


namespace tcp {
	namespace server {
		class CTCPClient;
	} //namespace server
} //namespace tcp

MainWorker::MainWorker()
{
	m_SecCountdown = -1;
	m_stoprequested = false;
	m_stopRxMessageThread = false;
	m_verboselevel = EVBL_None;

	m_bStartHardware = false;
	m_hardwareStartCounter = 0;

	// Set default settings for web servers
	m_webserver_settings.listening_address = "::"; // listen to all network interfaces
	m_webserver_settings.listening_port = "8080";
#ifdef WWW_ENABLE_SSL
	m_secure_webserver_settings.listening_address = "::"; // listen to all network interfaces
	m_secure_webserver_settings.listening_port = "443";
	m_secure_webserver_settings.ssl_method = "sslv23";
	m_secure_webserver_settings.certificate_chain_file_path = "./server_cert.pem";
	m_secure_webserver_settings.ca_cert_file_path = m_secure_webserver_settings.certificate_chain_file_path; // not used
	m_secure_webserver_settings.cert_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.private_key_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.private_key_pass_phrase = "";
	m_secure_webserver_settings.options = "default_workarounds,no_sslv2,no_sslv3,no_tlsv1,no_tlsv1_1,single_dh_use";
	m_secure_webserver_settings.tmp_dh_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.verify_peer = false;
	m_secure_webserver_settings.verify_fail_if_no_peer_cert = false;
	m_secure_webserver_settings.verify_file_path = "";
#endif
	m_bIgnoreUsernamePassword = false;

	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);
	m_ScheduleLastMinute = ltime.tm_min;
	m_ScheduleLastHour = ltime.tm_hour;
	m_ScheduleLastMinuteTime = 0;
	m_ScheduleLastHourTime = 0;
	m_ScheduleLastDayTime = 0;
	m_LastSunriseSet = "";

	m_bHaveDownloadedDomoticzUpdate = false;
	m_bHaveDownloadedDomoticzUpdateSuccessFull = false;
	m_bDoDownloadDomoticzUpdate = false;
	m_LastUpdateCheck = 0;
	m_bHaveUpdate = false;
	m_iRevision = 0;

	m_rxMessageIdx = 1;
	m_bForceLogNotificationCheck = false;
}

MainWorker::~MainWorker()
{
	Stop();
}

void MainWorker::AddAllDomoticzHardware()
{
	//Add Hardware devices
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware ORDER BY ID ASC");
	if (result.size() > 0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			int ID = atoi(sd[0].c_str());
			std::string Name = sd[1];
			std::string sEnabled = sd[2];
			bool Enabled = (sEnabled == "1") ? true : false;
			_eHardwareTypes Type = (_eHardwareTypes)atoi(sd[3].c_str());
			std::string Address = sd[4];
			unsigned short Port = (unsigned short)atoi(sd[5].c_str());
			std::string SerialPort = sd[6];
			std::string Username = sd[7];
			std::string Password = sd[8];
			std::string Extra = sd[9];
			int mode1 = atoi(sd[10].c_str());
			int mode2 = atoi(sd[11].c_str());
			int mode3 = atoi(sd[12].c_str());
			int mode4 = atoi(sd[13].c_str());
			int mode5 = atoi(sd[14].c_str());
			int mode6 = atoi(sd[15].c_str());
			int DataTimeout = atoi(sd[16].c_str());
			std::string Mode1Str = sd[10];
			std::string Mode2Str = sd[11];
			std::string Mode3Str = sd[12];
			std::string Mode4Str = sd[13];
			std::string Mode5Str = sd[14];
			std::string Mode6Str = sd[15];
			AddHardwareFromParams(ID, Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, mode1, mode2, mode3, mode4, mode5, mode6, DataTimeout, false);
		}
		m_hardwareStartCounter = 0;
		m_bStartHardware = true;
	}
}

void MainWorker::StartDomoticzHardware()
{
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		if (!(*itt)->IsStarted())
		{
			(*itt)->Start();
		}
	}
}

void MainWorker::StopDomoticzHardware()
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
#ifdef ENABLE_PYTHON
		m_pluginsystem.DeregisterPlugin((*itt)->m_HwdID);
#endif
		(*itt)->Stop();
		delete (*itt);
	}
	m_hardwaredevices.clear();
}

void MainWorker::GetAvailableWebThemes()
{
	std::string ThemeFolder = szWWWFolder + "/styles/";
	m_webthemes.clear();
	DirectoryListing(m_webthemes, ThemeFolder, true, false);

	//check if current theme is found, if not, select default
	bool bFound = false;
	std::string sValue;
	if (m_sql.GetPreferencesVar("WebTheme", sValue))
	{
		std::vector<std::string>::const_iterator itt;
		for (itt = m_webthemes.begin(); itt != m_webthemes.end(); ++itt)
		{
			if (*itt == sValue)
			{
				bFound = true;
				break;
			}
		}
	}
	if (!bFound)
	{
		m_sql.UpdatePreferencesVar("WebTheme", "default");
	}
}

void MainWorker::SendResetCommand(CDomoticzHardwareBase *pHardware)
{
	pHardware->m_bEnableReceive = false;

	if (
		(pHardware->HwdType != HTYPE_RFXtrx315) &&
		(pHardware->HwdType != HTYPE_RFXtrx433) &&
		(pHardware->HwdType != HTYPE_RFXtrx868) &&
		(pHardware->HwdType != HTYPE_RFXLAN)
		)
	{
		//clear buffer, and enable receive
		pHardware->m_rxbufferpos = 0;
		pHardware->m_bEnableReceive = true;
		return;
	}
	pHardware->m_rxbufferpos = 0;
	//Send Reset
	SendCommand(pHardware->m_HwdID, cmdRESET, "Reset");
	//wait at least 500ms
	boost::this_thread::sleep(boost::posix_time::millisec(500));
	pHardware->m_rxbufferpos = 0;
	pHardware->m_bEnableReceive = true;

	SendCommand(pHardware->m_HwdID, cmdStartRec, "Start Receiver");
	boost::this_thread::sleep(boost::posix_time::millisec(50));

	SendCommand(pHardware->m_HwdID, cmdSTATUS, "Status");
}

void MainWorker::AddDomoticzHardware(CDomoticzHardwareBase *pHardware)
{
	int devidx = FindDomoticzHardware(pHardware->m_HwdID);
	if (devidx != -1) //it is already there!, remove it
	{
		RemoveDomoticzHardware(m_hardwaredevices[devidx]);
	}
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	pHardware->sDecodeRXMessage.connect(boost::bind(&MainWorker::DecodeRXMessage, this, _1, _2, _3, _4));
	pHardware->sOnConnected.connect(boost::bind(&MainWorker::OnHardwareConnected, this, _1));
	m_hardwaredevices.push_back(pHardware);
}

void MainWorker::RemoveDomoticzHardware(CDomoticzHardwareBase *pHardware)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		CDomoticzHardwareBase *pOrgDevice = *itt;
		if (pOrgDevice == pHardware) {
			try
			{
				pOrgDevice->Stop();
				delete pOrgDevice;
				m_hardwaredevices.erase(itt);
			}
			catch (...)
			{
			}
			return;
		}
	}
}

void MainWorker::RemoveDomoticzHardware(int HwdId)
{
	int dpos = FindDomoticzHardware(HwdId);
	if (dpos == -1)
		return;
#ifdef ENABLE_PYTHON
	m_pluginsystem.DeregisterPlugin(HwdId);
#endif
	RemoveDomoticzHardware(m_hardwaredevices[dpos]);
}

int MainWorker::FindDomoticzHardware(int HwdId)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->m_HwdID == HwdId)
		{
			return (itt - m_hardwaredevices.begin());
		}
	}
	return -1;
}

int MainWorker::FindDomoticzHardwareByType(const _eHardwareTypes HWType)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->HwdType == HWType)
		{
			return (itt - m_hardwaredevices.begin());
		}
	}
	return -1;
}

CDomoticzHardwareBase* MainWorker::GetHardware(int HwdId)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->m_HwdID == HwdId)
		{
			return (*itt);
		}
	}
	return NULL;
}

CDomoticzHardwareBase* MainWorker::GetHardwareByIDType(const std::string &HwdId, const _eHardwareTypes HWType)
{
	if (HwdId == "")
		return NULL;
	int iHardwareID = atoi(HwdId.c_str());
	CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
	if (pHardware == NULL)
		return NULL;
	if (pHardware->HwdType != HWType)
		return NULL;
	return pHardware;
}

CDomoticzHardwareBase* MainWorker::GetHardwareByType(const _eHardwareTypes HWType)
{
	boost::lock_guard<boost::mutex> l(m_devicemutex);
	std::vector<CDomoticzHardwareBase*>::iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		if ((*itt)->HwdType == HWType)
		{
			return (*itt);
		}
	}
	return NULL;
}

// sunset/sunrise
// http://www.earthtools.org/sun/<latitude>/<longitude>/<day>/<month>/<timezone>/<dst>
// example:
// http://www.earthtools.org/sun/52.214268/5.171002/11/11/99/1

bool MainWorker::GetSunSettings()
{
	int nValue;
	std::string sValue;
	std::vector<std::string> strarray;
	if (m_sql.GetPreferencesVar("Location", nValue, sValue))
		StringSplit(sValue, ";", strarray);

	if (strarray.size() != 2)
	{
		// No location entered in the settings, lets just reload our schedules and return
		// Load non sun settings timers
		m_scheduler.ReloadSchedules();
		return false;
	}

	std::string Latitude = strarray[0];
	std::string Longitude = strarray[1];

	time_t atime = mytime(NULL);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int year = ltime.tm_year + 1900;
	int month = ltime.tm_mon + 1;
	int day = ltime.tm_mday;

	double dLatitude = atof(Latitude.c_str());
	double dLongitude = atof(Longitude.c_str());

	SunRiseSet::_tSubRiseSetResults sresult;
	SunRiseSet::GetSunRiseSet(dLatitude, dLongitude, year, month, day, sresult);

	std::string sunrise;
	std::string sunset;

	char szRiseSet[30];
	sprintf(szRiseSet, "%02d:%02d:00", sresult.SunRiseHour, sresult.SunRiseMin);
	sunrise = szRiseSet;
	sprintf(szRiseSet, "%02d:%02d:00", sresult.SunSetHour, sresult.SunSetMin);
	sunset = szRiseSet;

	m_scheduler.SetSunRiseSetTimers(sunrise, sunset);
	std::string riseset = sunrise.substr(0, sunrise.size() - 3) + ";" + sunset.substr(0, sunrise.size() - 3); //make a short version
	if (m_LastSunriseSet != riseset)
	{
		m_LastSunriseSet = riseset;
		_log.Log(LOG_NORM, "Sunrise: %s SunSet:%s", sunrise.c_str(), sunset.c_str());

		// ToDo: add here some condition to avoid double events loading on application startup. check if m_LastSunriseSet was empty?
		m_eventsystem.LoadEvents(); // reloads all events from database to refresh blocky events sunrise/sunset what are already replaced with time

		// FixMe: only reload schedules relative to sunset/sunrise to prevent race conditions
		// m_scheduler.ReloadSchedules(); // force reload of all schedules to adjust for changed sunrise/sunset values
	}
	return true;
}

void MainWorker::SetVerboseLevel(eVerboseLevel Level)
{
	m_verboselevel = Level;
}

eVerboseLevel MainWorker::GetVerboseLevel()
{
	return m_verboselevel;
}

void MainWorker::SetWebserverSettings(const server_settings & settings)
{
	m_webserver_settings.set(settings);
}

std::string MainWorker::GetWebserverAddress()
{
	return m_webserver_settings.listening_address;
}

std::string MainWorker::GetWebserverPort()
{
	return m_webserver_settings.listening_port;
}

#ifdef WWW_ENABLE_SSL
std::string MainWorker::GetSecureWebserverPort()
{
	return m_secure_webserver_settings.listening_port;
}

void MainWorker::SetSecureWebserverSettings(const ssl_server_settings & ssl_settings)
{
	m_secure_webserver_settings.set(ssl_settings);
}
#endif

bool MainWorker::RestartHardware(const std::string &idx)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT Name, Enabled, Type, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware WHERE (ID=='%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];
	std::string Name = sd[0];
	std::string senabled = (sd[1] == "1") ? "true" : "false";
	_eHardwareTypes htype = (_eHardwareTypes)atoi(sd[2].c_str());
	std::string address = sd[3];
	unsigned short port = (unsigned short)atoi(sd[4].c_str());
	std::string serialport = sd[5];
	std::string username = sd[6];
	std::string password = sd[7];
	std::string extra = sd[8];
	int Mode1 = atoi(sd[9].c_str());
	int Mode2 = atoi(sd[10].c_str());
	int Mode3 = atoi(sd[11].c_str());
	int Mode4 = atoi(sd[12].c_str());
	int Mode5 = atoi(sd[13].c_str());
	int Mode6 = atoi(sd[14].c_str());
	int DataTimeout = atoi(sd[15].c_str());
	std::string Mode1Str = sd[9];
	std::string Mode2Str = sd[10];
	std::string Mode3Str = sd[11];
	std::string Mode4Str = sd[12];
	std::string Mode5Str = sd[13];
	std::string Mode6Str = sd[14];

	return AddHardwareFromParams(atoi(idx.c_str()), Name, (senabled == "true") ? true : false, htype, address, port, serialport, username, password, extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout, true);
}

bool MainWorker::AddHardwareFromParams(
	const int ID,
	const std::string &Name,
	const bool Enabled,
	const _eHardwareTypes Type,
	const std::string &Address, const unsigned short Port, const std::string &SerialPort,
	const std::string &Username, const std::string &Password,
	const std::string &Filename,
	const int Mode1,
	const int Mode2,
	const int Mode3,
	const int Mode4,
	const int Mode5,
	const int Mode6,
	const int DataTimeout,
	const bool bDoStart
)
{
	RemoveDomoticzHardware(ID);

	if (!Enabled)
		return true;

	CDomoticzHardwareBase *pHardware = NULL;

	switch (Type)
	{
	case HTYPE_RFXtrx315:
	case HTYPE_RFXtrx433:
	case HTYPE_RFXtrx868:
		pHardware = new RFXComSerial(ID, SerialPort, 38400);
		break;
	case HTYPE_P1SmartMeter:
		pHardware = new P1MeterSerial(ID, SerialPort, (Mode1 == 1) ? 115200 : 9600, (Mode2 != 0), Mode3);
		break;
	case HTYPE_Rego6XX:
		pHardware = new CRego6XXSerial(ID, SerialPort, Mode1);
		break;
	case HTYPE_DavisVantage:
		pHardware = new CDavisLoggerSerial(ID, SerialPort, 19200);
		break;
	case HTYPE_S0SmartMeterUSB:
		pHardware = new S0MeterSerial(ID, SerialPort, 9600);
		break;
	case HTYPE_S0SmartMeterTCP:
		//LAN
		pHardware = new S0MeterTCP(ID, Address, Port);
		break;
	case HTYPE_OpenThermGateway:
		pHardware = new OTGWSerial(ID, SerialPort, 9600, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
		break;
	case HTYPE_TeleinfoMeter:
		pHardware = new CTeleinfoSerial(ID, SerialPort, DataTimeout, Mode1, (Mode2 != 0), Mode3);
		break;
	case HTYPE_MySensorsUSB:
		pHardware = new MySensorsSerial(ID, SerialPort, Mode1);
		break;
	case HTYPE_KMTronicUSB:
		pHardware = new KMTronicSerial(ID, SerialPort);
		break;
	case HTYPE_KMTronic433:
		pHardware = new KMTronic433(ID, SerialPort);
		break;
	case HTYPE_OpenZWave:
#ifdef WITH_OPENZWAVE
		pHardware = new COpenZWave(ID, SerialPort);
#endif
		break;
	case HTYPE_EnOceanESP2:
		pHardware = new CEnOceanESP2(ID, SerialPort, Mode1);
		break;
	case HTYPE_EnOceanESP3:
		pHardware = new CEnOceanESP3(ID, SerialPort, Mode1);
		break;
	case HTYPE_Meteostick:
		pHardware = new Meteostick(ID, SerialPort, 115200);
		break;
	case HTYPE_EVOHOME_SERIAL:
		pHardware = new CEvohomeSerial(ID, SerialPort, Mode1, Filename);
		break;
	case HTYPE_EVOHOME_TCP:
		pHardware = new CEvohomeTCP(ID, Address, Port, Filename);
		break;
	case HTYPE_RFLINKUSB:
		pHardware = new CRFLinkSerial(ID, SerialPort);
		break;
	case HTYPE_ZIBLUEUSB:
		pHardware = new CZiBlueSerial(ID, SerialPort);
		break;
	case HTYPE_CurrentCostMeter:
		pHardware = new CurrentCostMeterSerial(ID, SerialPort, (Mode1 == 1) ? 57600 : 9600);
		break;
	case HTYPE_RAVEn:
		pHardware = new RAVEn(ID, SerialPort);
		break;
	case HTYPE_Comm5Serial:
		pHardware = new Comm5Serial(ID, SerialPort);
		break;
	case HTYPE_RFXLAN:
		//LAN
		pHardware = new RFXComTCP(ID, Address, Port);
		break;
	case HTYPE_Domoticz:
		//LAN
		pHardware = new DomoticzTCP(ID, Address, Port, Username, Password);
		break;
	case HTYPE_RazberryZWave:
		_log.Log(LOG_ERROR, "Razberry: Deprecated, support is removed! Use OpenZWave (see wiki)...");
		return false;
		break;
	case HTYPE_P1SmartMeterLAN:
		//LAN
		pHardware = new P1MeterTCP(ID, Address, Port, (Mode2 != 0), Mode3);
		break;
	case HTYPE_WOL:
		//LAN
		pHardware = new CWOL(ID, Address, Port);
		break;
	case HTYPE_OpenThermGatewayTCP:
		//LAN
		pHardware = new OTGWTCP(ID, Address, Port, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
		break;
	case HTYPE_MySensorsTCP:
		//LAN
		pHardware = new MySensorsTCP(ID, Address, Port);
		break;
	case HTYPE_MySensorsMQTT:
		//LAN
		pHardware = new MySensorsMQTT(ID, Name, Address, Port, Username, Password, Filename, Mode1);
		break;
	case HTYPE_RFLINKTCP:
		//LAN
		pHardware = new CRFLinkTCP(ID, Address, Port);
		break;
	case HTYPE_ZIBLUETCP:
		//LAN
		pHardware = new CZiBlueTCP(ID, Address, Port);
		break;
	case HTYPE_MQTT:
		//LAN
		pHardware = new MQTT(ID, Address, Port, Username, Password, Filename, Mode1);
		break;
	case HTYPE_FRITZBOX:
		//LAN
		pHardware = new FritzboxTCP(ID, Address, Port);
		break;
	case HTYPE_SOLARMAXTCP:
		//LAN
		pHardware = new SolarMaxTCP(ID, Address, Port);
		break;
	case HTYPE_LimitlessLights:
		//LAN
	{
		int rmode1 = Mode1;
		if (rmode1 == 0)
			rmode1 = 1;
		pHardware = new CLimitLess(ID, rmode1, Mode2, Address, Port);
	}
	break;
	case HTYPE_YouLess:
		//LAN
		pHardware = new CYouLess(ID, Address, Port, Password);
		break;
	case HTYPE_WINDDELEN:
		pHardware = new CWinddelen(ID, Address, Port, Mode1);
		break;
	case HTYPE_ETH8020:
		//LAN
		pHardware = new CETH8020(ID, Address, Port, Username, Password);
		break;
	case HTYPE_RelayNet:
		//LAN
		pHardware = new RelayNet(ID, Address, Port, Username, Password, Mode1 != 0, Mode2 != 0, Mode3, Mode4, Mode5);
		break;
	case HTYPE_KMTronicTCP:
		//LAN
		pHardware = new KMTronicTCP(ID, Address, Port, Username, Password);
		break;
	case HTYPE_KMTronicUDP:
		//UDP
		pHardware = new KMTronicUDP(ID, Address, Port);
		break;
	case HTYPE_NefitEastLAN:
		pHardware = new CNefitEasy(ID, Address, Port);
		break;
	case HTYPE_ECODEVICES:
		//LAN
		pHardware = new CEcoDevices(ID, Address, Port, Username, Password, DataTimeout, Mode1, Mode2);
		break;
	case HTYPE_1WIRE:
		//1-Wire file system
		pHardware = new C1Wire(ID, Mode1, Mode2, Filename);
		break;
	case HTYPE_Pinger:
		//System Alive Checker (Ping)
		pHardware = new CPinger(ID, Mode1, Mode2);
		break;
	case HTYPE_Kodi:
		//Kodi Media Player
		pHardware = new CKodi(ID, Mode1, Mode2);
		break;
	case HTYPE_PanasonicTV:
		//Panasonic Viera TV's
		pHardware = new CPanasonic(ID, Mode1, Mode2);
		break;
	case HTYPE_Mochad:
		//LAN
		pHardware = new MochadTCP(ID, Address, Port);
		break;
	case HTYPE_SatelIntegra:
		pHardware = new SatelIntegra(ID, Address, Port, Password, Mode1);
		break;
	case HTYPE_LogitechMediaServer:
		//Logitech Media Server
		pHardware = new CLogitechMediaServer(ID, Address, Port, Username, Password, Mode1, Mode2);
		break;
	case HTYPE_Sterbox:
		//LAN
		pHardware = new CSterbox(ID, Address, Port, Username, Password);
		break;
	case HTYPE_DenkoviSmartdenLan:
		//LAN
		pHardware = new CDenkoviSmartdenLan(ID, Address, Port, Password);
		break;
	case HTYPE_DenkoviSmartdenIPInOut:
		//LAN
		pHardware = new CDenkoviSmartdenIPInOut(ID, Address, Port, Password);
		break;
	case HTYPE_HEOS:
		//HEOS by DENON
		pHardware = new CHEOS(ID, Address, Port, Username, Password, Mode1, Mode2);
		break;
	case HTYPE_MultiFun:
		//MultiFun LAN
		pHardware = new MultiFun(ID, Address, Port);
		break;
#ifndef WIN32
	case HTYPE_TE923:
		//TE923 compatible weather station
#ifdef WITH_LIBUSB
		pHardware = new CTE923(ID);
#endif
		break;
	case HTYPE_VOLCRAFTCO20:
		//Voltcraft CO-20 Air Quality
#ifdef WITH_LIBUSB
		pHardware = new CVolcraftCO20(ID);
#endif
		break;
#endif
	case HTYPE_RaspberryBMP085:
		pHardware = new I2C(ID, I2C::I2CTYPE_BMP085, 0);
		break;
	case HTYPE_RaspberryHTU21D:
		pHardware = new I2C(ID, I2C::I2CTYPE_HTU21D, 0);
		break;
	case HTYPE_RaspberryTSL2561:
		pHardware = new I2C(ID, I2C::I2CTYPE_TSL2561, 0);
		break;
	case HTYPE_RaspberryPCF8574:
		pHardware = new I2C(ID, I2C::I2CTYPE_PCF8574, Port);
		break;
	case HTYPE_RaspberryBME280:
		pHardware = new I2C(ID, I2C::I2CTYPE_BME280, 0);
		break;
	case HTYPE_RaspberryMCP23017:
		_log.Log(LOG_NORM, "MainWorker::AddHardwareFromParams HTYPE_RaspberryMCP23017");
		pHardware = new I2C(ID, I2C::I2CTYPE_MCP23017, Port);
		break;
	case HTYPE_Wunderground:
		pHardware = new CWunderground(ID, Username, Password);
		break;
	case HTYPE_HTTPPOLLER:
		pHardware = new CHttpPoller(ID, Username, Password, Address, Filename, Port);
		break;
	case HTYPE_DarkSky:
		pHardware = new CDarkSky(ID, Username, Password);
		break;
	case HTYPE_AccuWeather:
		pHardware = new CAccuWeather(ID, Username, Password);
		break;
	case HTYPE_SolarEdgeAPI:
		pHardware = new SolarEdgeAPI(ID, Username);
		break;
	case HTYPE_Netatmo:
		pHardware = new CNetatmo(ID, Username, Password);
		break;
	case HTYPE_Daikin:
		pHardware = new CDaikin(ID, Address, Port, Username, Password);
		break;
	case HTYPE_SBFSpot:
		pHardware = new CSBFSpot(ID, Username);
		break;
	case HTYPE_ICYTHERMOSTAT:
		pHardware = new CICYThermostat(ID, Username, Password);
		break;
	case HTYPE_TOONTHERMOSTAT:
		pHardware = new CToonThermostat(ID, Username, Password, Mode1);
		break;
	case HTYPE_AtagOne:
		pHardware = new CAtagOne(ID, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
		break;
	case HTYPE_NEST:
		pHardware = new CNest(ID, Username, Password);
		break;
	case HTYPE_ANNATHERMOSTAT:
		pHardware = new CAnnaThermostat(ID, Address, Port, Username, Password);
		break;
	case HTYPE_THERMOSMART:
		pHardware = new CThermosmart(ID, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
		break;
	case HTYPE_Philips_Hue:
		pHardware = new CPhilipsHue(ID, Address, Port, Username, Mode1);
		break;
	case HTYPE_HARMONY_HUB:
		pHardware = new CHarmonyHub(ID, Address, Port);
		break;
	case HTYPE_PVOUTPUT_INPUT:
		pHardware = new CPVOutputInput(ID, Username, Password);
		break;
	case HTYPE_Dummy:
		pHardware = new CDummy(ID);
		break;
#ifdef WITH_TELLDUSCORE
	case HTYPE_Tellstick:
		pHardware = new CTellstick(ID, Mode1, Mode2);
		break;
#endif //WITH_TELLDUSCORE
	case HTYPE_EVOHOME_SCRIPT:
		pHardware = new CEvohomeScript(ID);
		break;
	case HTYPE_PiFace:
		pHardware = new CPiFace(ID);
		break;
	case HTYPE_System:
		pHardware = new CHardwareMonitor(ID);
		break;
	case HTYPE_RaspberryGPIO:
		//Raspberry Pi GPIO port access
#ifdef WITH_GPIO
		pHardware = new CGpio(ID, Mode1, Mode2, Mode3);
#endif
		break;
	case HTYPE_SysfsGpio:
#ifdef WITH_GPIO
		pHardware = new CSysfsGpio(ID, Mode1, Mode2);
#endif
		break;
	case HTYPE_Comm5TCP:
		//LAN
		pHardware = new Comm5TCP(ID, Address, Port);
		break;
	case HTYPE_CurrentCostMeterLAN:
		//LAN
		pHardware = new CurrentCostMeterTCP(ID, Address, Port);
		break;
	case HTYPE_DomoticzInternal:
		pHardware = new DomoticzInternal(ID);
		break;
	case HTYPE_OpenWebNetTCP:
		pHardware = new COpenWebNetTCP(ID, Address, Port, Password);
		break;
	case HTYPE_BleBox:
		pHardware = new BleBox(ID, Mode1);
		break;
	case HTYPE_OpenWeatherMap:
		pHardware = new COpenWeatherMap(ID, Username, Password);
		break;
	case HTYPE_Ec3kMeterTCP:
		pHardware = new Ec3kMeterTCP(ID, Address, Port);
		break;
	case HTYPE_GoodweAPI:
		pHardware = new GoodweAPI(ID, Username);
		break;
	case HTYPE_Yeelight:
		pHardware = new Yeelight(ID);
		break;
	case HTYPE_PythonPlugin:
#ifdef ENABLE_PYTHON
		pHardware = m_pluginsystem.RegisterPlugin(ID, Name, Filename);
#endif
		break;
	case HTYPE_XiaomiGateway:
		pHardware = new XiaomiGateway(ID);
		break;
	case HTYPE_Arilux:
		pHardware = new Arilux(ID);
		break;
	case HTYPE_OpenWebNetUSB:
		pHardware = new COpenWebNetUSB(ID, SerialPort, 115200);
		break;
	case HTYPE_IntergasInComfortLAN2RF:
		pHardware = new CInComfort(ID, Address, Port);
		break;
	case HTYPE_EVOHOME_WEB:
		pHardware = new CEvohomeWeb(ID, Username, Password, Mode1, Mode2, Mode3);
		break;
	case HTYPE_Rtl433:
		pHardware = new CRtl433(ID, Filename);
		break;
	case HTYPE_OnkyoAVTCP:
		pHardware = new OnkyoAVTCP(ID, Address, Port);
		break;
	case HTYPE_USBtinGateway:
		pHardware = new USBtin(ID, SerialPort, Mode1, Mode2);
		break;
	case HTYPE_EnphaseAPI:
		pHardware = new EnphaseAPI(ID, Address, Port);
		break;
	}

	if (pHardware)
	{
		pHardware->HwdType = Type;
		pHardware->Name = Name;
		pHardware->m_DataTimeout = DataTimeout;
		AddDomoticzHardware(pHardware);

		if (bDoStart)
			pHardware->Start();
		return true;
	}
	return false;
}

bool MainWorker::Start()
{
	if (!m_sql.OpenDatabase())
	{
		return false;
	}
	//set the log preference
	_log.GetLogPreference();

	HTTPClient::SetUserAgent(GenerateUserAgent());
	m_notifications.Init();
	GetSunSettings();
	GetAvailableWebThemes();
#ifdef ENABLE_PYTHON
	if (m_sql.m_bEnableEventSystem)
	{
		m_pluginsystem.StartPluginSystem();
	}
#endif
	AddAllDomoticzHardware();
	m_fibaropush.Start();
	m_httppush.Start();
	m_influxpush.Start();
	m_googlepubsubpush.Start();
#ifdef PARSE_RFXCOM_DEVICE_LOG
	if (m_bStartHardware == false)
		m_bStartHardware = true;
#endif
	// load notifications configuration
	m_notifications.LoadConfig();
	if (!StartThread())
		return false;
	return true;
}


bool MainWorker::Stop()
{
	if (m_rxMessageThread) {
		// Stop RxMessage thread before hardware to avoid NULL pointer exception
		m_stopRxMessageThread = true;
		UnlockRxMessageQueue();
		m_rxMessageThread->join();
		m_rxMessageThread.reset();
	}
	if (m_thread)
	{
		m_webservers.StopServers();
		m_sharedserver.StopServer();
		_log.Log(LOG_STATUS, "Stopping all hardware...");
		StopDomoticzHardware();
		m_scheduler.StopScheduler();
		m_eventsystem.StopEventSystem();
		m_fibaropush.Stop();
		m_httppush.Stop();
		m_influxpush.Stop();
		m_googlepubsubpush.Stop();
#ifdef ENABLE_PYTHON
		m_pluginsystem.StopPluginSystem();
#endif

		//    m_cameras.StopCameraGrabber();

		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
	return true;
}

bool MainWorker::StartThread()
{
	if (m_webserver_settings.is_enabled()
#ifdef WWW_ENABLE_SSL
		|| m_secure_webserver_settings.is_enabled()
#endif
		)
	{
		//Start WebServer
#ifdef WWW_ENABLE_SSL
		if (!m_webservers.StartServers(m_webserver_settings, m_secure_webserver_settings, szWWWFolder, m_bIgnoreUsernamePassword, &m_sharedserver))
#else
		if (!m_webservers.StartServers(m_webserver_settings, szWWWFolder, m_bIgnoreUsernamePassword, &m_sharedserver))
#endif
		{
#ifdef WIN32
			MessageBox(0, "Error starting webserver(s), check if ports are not in use!", MB_OK, MB_ICONERROR);
#endif
			return false;
		}
	}
	int nValue = 0;
	if (m_sql.GetPreferencesVar("AuthenticationMethod", nValue))
	{
		m_webservers.SetAuthenticationMethod(nValue);
	}
	std::string sValue;
	if (m_sql.GetPreferencesVar("WebTheme", sValue))
	{
		m_webservers.SetWebTheme(sValue);
	}

	m_webservers.SetWebRoot(szWebRoot);

	//Start Scheduler
	m_scheduler.StartScheduler();
	m_cameras.ReloadCameras();

	int rnvalue = 0;
	m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);
	if (rnvalue != 0)
	{
		char szPort[100];
		sprintf(szPort, "%d", rnvalue);
		m_sharedserver.sDecodeRXMessage.connect(boost::bind(&MainWorker::DecodeRXMessage, this, _1, _2, _3, _4));
		m_sharedserver.StartServer("::", szPort);

		LoadSharedUsers();
	}

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MainWorker::Do_Work, this)));
	m_rxMessageThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MainWorker::Do_Work_On_Rx_Messages, this)));

	return (m_thread != NULL) && (m_rxMessageThread != NULL);
}

#define HEX( x ) \
	std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)( x )

bool MainWorker::IsUpdateAvailable(const bool bIsForced)
{
	if (!bIsForced)
	{
		int nValue = 0;
		m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
		if (nValue != 1)
		{
			return false;
		}
	}

	utsname my_uname;
	if (uname(&my_uname) < 0)
		return false;

	m_szSystemName = my_uname.sysname;
	std::string machine = my_uname.machine;
	std::transform(m_szSystemName.begin(), m_szSystemName.end(), m_szSystemName.begin(), ::tolower);

	if (machine == "armv6l")
	{
		//Seems like old arm systems can also use the new arm build
		machine = "armv7l";
	}

#ifdef DEBUG_DOWNLOAD
	m_szSystemName = "linux";
	machine = "armv7l";
#endif

	if ((m_szSystemName != "windows") && (machine != "armv6l") && (machine != "armv7l") && (machine != "x86_64") && (machine != "aarch64"))
	{
		//Only Raspberry Pi (Wheezy)/Ubuntu/windows/osx for now!
		return false;
	}
	time_t atime = mytime(NULL);
	if (!bIsForced)
	{
		if (atime - m_LastUpdateCheck < 12 * 3600)
		{
			return m_bHaveUpdate;
		}
	}
	m_LastUpdateCheck = atime;

	int nValue;
	m_sql.GetPreferencesVar("ReleaseChannel", nValue);
	bool bIsBetaChannel = (nValue != 0);

	std::string szURL;
	if (!bIsBetaChannel)
	{
		szURL = "http://www.domoticz.com/download.php?channel=stable&type=version&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateURL = "http://www.domoticz.com/download.php?channel=stable&type=release&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateChecksumURL = "http://www.domoticz.com/download.php?channel=stable&type=checksum&system=" + m_szSystemName + "&machine=" + machine;
	}
	else
	{
		szURL = "http://www.domoticz.com/download.php?channel=beta&type=version&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateURL = "http://www.domoticz.com/download.php?channel=beta&type=release&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateChecksumURL = "http://www.domoticz.com/download.php?channel=beta&type=checksum&system=" + m_szSystemName + "&machine=" + machine;
	}

	std::string revfile;

	if (!HTTPClient::GET(szURL, revfile))
		return false;

	stdreplace(revfile, "\r\n", "\n");
	std::vector<std::string> strarray;
	StringSplit(revfile, "\n", strarray);
	if (strarray.size() < 1)
		return false;
	StringSplit(strarray[0], " ", strarray);
	if (strarray.size() != 3)
		return false;

	int version = atoi(szAppVersion.substr(szAppVersion.find(".") + 1).c_str());
	m_iRevision = atoi(strarray[2].c_str());
#ifdef DEBUG_DOWNLOAD
	m_bHaveUpdate = true;
#else
	m_bHaveUpdate = ((version != m_iRevision) && (version < m_iRevision));
#endif
	return m_bHaveUpdate;
}

bool MainWorker::StartDownloadUpdate()
{
#ifdef WIN32
	return false; //managed by web gui
#endif

	if (!IsUpdateAvailable(true))
		return false; //no new version available

	m_bHaveDownloadedDomoticzUpdate = false;
	m_bHaveDownloadedDomoticzUpdateSuccessFull = false;
	m_bDoDownloadDomoticzUpdate = true;
	return true;
}

void MainWorker::HandleAutomaticBackups()
{
	int nValue = 0;
	if (!m_sql.GetPreferencesVar("UseAutoBackup", nValue))
		return;
	if (nValue != 1)
		return;

	_log.Log(LOG_STATUS, "Starting automatic database backup procedure...");

	std::stringstream backup_DirH;
	std::stringstream backup_DirD;
	std::stringstream backup_DirM;

#ifdef WIN32
	std::string sbackup_DirH = szUserDataFolder + "backups\\hourly\\";
	std::string sbackup_DirD = szUserDataFolder + "backups\\daily\\";
	std::string sbackup_DirM = szUserDataFolder + "backups\\monthly\\";
#else
	std::string sbackup_DirH = szUserDataFolder + "backups/hourly/";
	std::string sbackup_DirD = szUserDataFolder + "backups/daily/";
	std::string sbackup_DirM = szUserDataFolder + "backups/monthly/";
#endif


	//create folders if they not exists
	mkdir_deep(sbackup_DirH.c_str(), 0755);
	mkdir_deep(sbackup_DirD.c_str(), 0755);
	mkdir_deep(sbackup_DirM.c_str(), 0755);

	time_t now = mytime(NULL);
	struct tm tm1;
	localtime_r(&now, &tm1);
	int hour = tm1.tm_hour;
	int day = tm1.tm_mday;
	int month = tm1.tm_mon;

	int lastHourBackup = -1;
	int lastDayBackup = -1;
	int lastMonthBackup = -1;

	m_sql.GetLastBackupNo("Hour", lastHourBackup);
	m_sql.GetLastBackupNo("Day", lastDayBackup);
	m_sql.GetLastBackupNo("Month", lastMonthBackup);

	DIR *lDir;
	//struct dirent *ent;
	if ((lastHourBackup == -1) || (lastHourBackup != hour)) {

		if ((lDir = opendir(sbackup_DirH.c_str())) != NULL)
		{
			std::stringstream sTmp;
			sTmp << "backup-hour-" << std::setw(2) << std::setfill('0') << hour << ".db";

			std::string OutputFileName = sbackup_DirH + sTmp.str();
			if (m_sql.BackupDatabase(OutputFileName)) {
				m_sql.SetLastBackupNo("Hour", hour);
			}
			else {
				_log.Log(LOG_ERROR, "Error writing automatic hourly backup file");
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "Error accessing automatic backup directories");
		}
	}
	if ((lastDayBackup == -1) || (lastDayBackup != day)) {

		if ((lDir = opendir(sbackup_DirD.c_str())) != NULL)
		{
			std::stringstream sTmp;
			sTmp << "backup-day-" << std::setw(2) << std::setfill('0') << day << ".db";

			std::string OutputFileName = sbackup_DirD + sTmp.str();
			if (m_sql.BackupDatabase(OutputFileName)) {
				m_sql.SetLastBackupNo("Day", day);
			}
			else {
				_log.Log(LOG_ERROR, "Error writing automatic daily backup file");
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "Error accessing automatic backup directories");
		}
	}
	if ((lastMonthBackup == -1) || (lastMonthBackup != month)) {
		if ((lDir = opendir(sbackup_DirM.c_str())) != NULL)
		{
			std::stringstream sTmp;
			sTmp << "backup-month-" << std::setw(2) << std::setfill('0') << month + 1 << ".db";

			std::string OutputFileName = sbackup_DirM + sTmp.str();
			if (m_sql.BackupDatabase(OutputFileName)) {
				m_sql.SetLastBackupNo("Month", month);
			}
			else {
				_log.Log(LOG_ERROR, "Error writing automatic monthly backup file");
			}
			closedir(lDir);
		}
		else {
			_log.Log(LOG_ERROR, "Error accessing automatic backup directories");
		}
	}
	_log.Log(LOG_STATUS, "Ending automatic database backup procedure...");
}

void MainWorker::ParseRFXLogFile()
{
#ifdef PARSE_RFXCOM_DEVICE_LOG
	std::vector<std::string> _lines;
	std::ifstream myfile("C:\\RFXtrxLog.txt");
	if (myfile.is_open())
	{
		while (myfile.good())
		{
			std::string _line;
			getline(myfile, _line);
			size_t tpos = _line.find("=");
			if (tpos != std::string::npos)
			{
				_line = _line.substr(tpos + 1);
				tpos = _line.find(" ");
				if (tpos == 0)
				{
					_line = _line.substr(1);
				}
			}
			stdreplace(_line, " ", "");
			_lines.push_back(_line);
		}
		myfile.close();
	}
	int HWID = 999;
	//m_sql.DeleteHardware("999");

	CDomoticzHardwareBase *pHardware = GetHardware(HWID);
	if (pHardware == NULL)
	{
		pHardware = new CDummy(HWID);
		AddDomoticzHardware(pHardware);
	}

	std::vector<std::string>::iterator itt;
	unsigned char rxbuffer[100];
	static const char* const lut = "0123456789ABCDEF";
	for (itt = _lines.begin(); itt != _lines.end(); ++itt)
	{
		std::string hexstring = *itt;
		if (hexstring.size() % 2 != 0)
			continue;//illegal
		int totbytes = hexstring.size() / 2;
		int ii = 0;
		for (ii = 0; ii < totbytes; ii++)
		{
			std::string hbyte = hexstring.substr((ii * 2), 2);

			char a = hbyte[0];
			const char* p = std::lower_bound(lut, lut + 16, a);
			if (*p != a) throw std::invalid_argument("not a hex digit");

			char b = hbyte[1];
			const char* q = std::lower_bound(lut, lut + 16, b);
			if (*q != b) throw std::invalid_argument("not a hex digit");

			unsigned char uchar = ((p - lut) << 4) | (q - lut);
			rxbuffer[ii] = uchar;
		}
		if (ii == 0)
			continue;
		if (CRFXBase::CheckValidRFXData((const uint8_t*)&rxbuffer))
		{
			pHardware->WriteToHardware((const char *)&rxbuffer, totbytes);
			DecodeRXMessage(pHardware, (const unsigned char *)&rxbuffer, NULL, 255);
			sleep_milliseconds(300);
		}
		else
		{
			_log.Log(LOG_ERROR, "Invalid data/length!");
		}
	}
#endif
}

void MainWorker::Do_Work()
{
	int second_counter = 0;
	while (!m_stoprequested)
	{
		//sleep 500 milliseconds
		sleep_milliseconds(500);

		if (m_bDoDownloadDomoticzUpdate)
		{
			m_bDoDownloadDomoticzUpdate = false;

			_log.Log(LOG_STATUS, "Starting Upgrade progress...");
#ifdef WIN32
			std::string outfile;

			//First download the checksum file
			outfile = szStartupFolder + "update.tgz.sha256sum";
			bool bHaveDownloadedChecksum = HTTPClient::GETBinaryToFile(m_szDomoticzUpdateChecksumURL.c_str(), outfile.c_str());
			if (bHaveDownloadedChecksum)
			{
				//Next download the actual update
				outfile = szStartupFolder + "update.tgz";
				m_bHaveDownloadedDomoticzUpdateSuccessFull = HTTPClient::GETBinaryToFile(m_szDomoticzUpdateURL.c_str(), outfile.c_str());
				if (!m_bHaveDownloadedDomoticzUpdateSuccessFull)
				{
					m_UpdateStatusMessage = "Problem downloading update file!";
				}
			}
			else
				m_UpdateStatusMessage = "Problem downloading checksum file!";
#else
			int nValue;
			m_sql.GetPreferencesVar("ReleaseChannel", nValue);
			bool bIsBetaChannel = (nValue != 0);

			std::string scriptname = szUserDataFolder + "scripts/download_update.sh";
			std::string strparm = szUserDataFolder;
			if (bIsBetaChannel)
				strparm += " /beta";

			std::string lscript = scriptname + " " + strparm;
			_log.Log(LOG_STATUS, "Starting: %s", lscript.c_str());
			int ret = system(lscript.c_str());
			m_bHaveDownloadedDomoticzUpdateSuccessFull = (ret == 0);
#endif
			m_bHaveDownloadedDomoticzUpdate = true;
		}

		second_counter++;
		if (second_counter < 2)
			continue;
		second_counter = 0;

		if (m_bStartHardware)
		{
			m_hardwareStartCounter++;
			if (m_hardwareStartCounter >= 2)
			{
				m_bStartHardware = false;
				StartDomoticzHardware();
#ifdef ENABLE_PYTHON
				m_pluginsystem.AllPluginsStarted();
#endif
				ParseRFXLogFile();
				m_eventsystem.SetEnabled(m_sql.m_bEnableEventSystem);
				m_eventsystem.StartEventSystem();
			}
		}
		if (m_devicestorestart.size() > 0)
		{
			std::vector<int>::const_iterator itt;
			for (itt = m_devicestorestart.begin(); itt != m_devicestorestart.end(); ++itt)
			{
				int hwid = (*itt);
				std::stringstream sstr;
				sstr << hwid;
				std::string idx = sstr.str();

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM Hardware WHERE (ID=='%q')",
					idx.c_str());
				if (result.size() > 0)
				{
					std::vector<std::string> sd = result[0];
					std::string Name = sd[0];
					_log.Log(LOG_ERROR, "Restarting: %s", Name.c_str());
					RestartHardware(idx);
				}
			}
			m_devicestorestart.clear();
		}

		if (m_SecCountdown > 0)
		{
			m_SecCountdown--;
			if (m_SecCountdown == 0)
			{
				SetInternalSecStatus();
			}
		}

		time_t atime = mytime(NULL);
		struct tm ltime;
		localtime_r(&atime, &ltime);

		if (ltime.tm_min != m_ScheduleLastMinute)
		{
			if (difftime(atime, m_ScheduleLastMinuteTime) > 30) //avoid RTC/NTP clock drifts
			{
				m_ScheduleLastMinuteTime = atime;
				m_ScheduleLastMinute = ltime.tm_min;

				tzset(); //this because localtime_r/localtime_s does not update for DST

				//check for 5 minute schedule
				if (ltime.tm_min % m_sql.m_ShortLogInterval == 0)
				{
					m_sql.ScheduleShortlog();
				}
				std::string szPwdResetFile = szStartupFolder + "resetpwd";
				if (file_exist(szPwdResetFile.c_str()))
				{
					m_webservers.ClearUserPasswords();
					m_sql.UpdatePreferencesVar("WebUserName", "");
					m_sql.UpdatePreferencesVar("WebPassword", "");
					std::remove(szPwdResetFile.c_str());
				}
				m_notifications.CheckAndHandleLastUpdateNotification();
			}
			if (_log.NotificationLogsEnabled())
			{
				if ((ltime.tm_min % 5 == 0) || (m_bForceLogNotificationCheck))
				{
					m_bForceLogNotificationCheck = false;
					HandleLogNotifications();
				}
			}
		}
		if (ltime.tm_hour != m_ScheduleLastHour)
		{
			if (difftime(atime, m_ScheduleLastHourTime) > 30 * 60) //avoid RTC/NTP clock drifts
			{
				m_ScheduleLastHourTime = atime;
				m_ScheduleLastHour = ltime.tm_hour;
				GetSunSettings();

				m_sql.CheckDeviceTimeout();
				m_sql.CheckBatteryLow();

				//check for daily schedule
				if (ltime.tm_hour == 0)
				{
					if (atime - m_ScheduleLastDayTime > 12 * 60 * 60)
					{
						m_ScheduleLastDayTime = atime;
						m_sql.ScheduleDay();
					}
				}
#ifdef WITH_OPENZWAVE
				if (ltime.tm_hour == 4)
				{
					//Heal the OpenZWave network
					boost::lock_guard<boost::mutex> l(m_devicemutex);
					std::vector<CDomoticzHardwareBase*>::iterator itt;
					for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
					{
						CDomoticzHardwareBase *pHardware = (*itt);
						if (pHardware->HwdType == HTYPE_OpenZWave)
						{
							COpenZWave *pZWave = (COpenZWave *)pHardware;
							pZWave->NightlyNodeHeal();
						}
					}
				}
#endif
				if ((ltime.tm_hour == 5) || (ltime.tm_hour == 17))
				{
					IsUpdateAvailable(true);//check for update
				}
				HandleAutomaticBackups();
			}
		}
		if (ltime.tm_sec % 30 == 0)
		{
			HeartbeatCheck();
		}
	}
	_log.Log(LOG_STATUS, "Mainworker Stopped...");
}

void MainWorker::SendCommand(const int HwdID, unsigned char Cmd, const char *szMessage)
{
	int hindex = FindDomoticzHardware(HwdID);
	if (hindex == -1)
		return;

	if (szMessage != NULL)
		if (_log.isTraceEnabled()) _log.Log(LOG_TRACE, "MAIN SendCommand: %s", szMessage);


	tRBUF cmd;
	cmd.ICMND.packetlength = 13;
	cmd.ICMND.packettype = 0;
	cmd.ICMND.subtype = 0;
	cmd.ICMND.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
	cmd.ICMND.cmnd = Cmd;
	cmd.ICMND.freqsel = 0;
	cmd.ICMND.xmitpwr = 0;
	cmd.ICMND.msg3 = 0;
	cmd.ICMND.msg4 = 0;
	cmd.ICMND.msg5 = 0;
	cmd.ICMND.msg6 = 0;
	cmd.ICMND.msg7 = 0;
	cmd.ICMND.msg8 = 0;
	cmd.ICMND.msg9 = 0;
	WriteToHardware(HwdID, (const char*)&cmd, sizeof(cmd.ICMND));
}

bool MainWorker::WriteToHardware(const int HwdID, const char *pdata, const unsigned char length)
{
	int hindex = FindDomoticzHardware(HwdID);

	if (hindex == -1)
		return false;

	return m_hardwaredevices[hindex]->WriteToHardware(pdata, length);
	if (_log.isTraceEnabled()) _log.Log(LOG_TRACE, "MAIN WriteToHardware %s", m_hardwaredevices[hindex]->Name.c_str());

}

void MainWorker::WriteMessageStart()
{
	_log.LogSequenceStart();
}

void MainWorker::WriteMessageEnd()
{
	_log.LogSequenceEnd(LOG_NORM);
}

void MainWorker::WriteMessage(const char *szMessage)
{
	_log.LogSequenceAdd(szMessage);
}

void MainWorker::WriteMessage(const char *szMessage, bool linefeed)
{
	if (linefeed)
		_log.LogSequenceAdd(szMessage);
	else
		_log.LogSequenceAddNoLF(szMessage);
}

void MainWorker::OnHardwareConnected(CDomoticzHardwareBase *pHardware)
{
	SendResetCommand(pHardware);
}

uint64_t MainWorker::PerformRealActionFromDomoticzClient(const unsigned char *pRXCommand, CDomoticzHardwareBase **pOriginalHardware)
{
	*pOriginalHardware = NULL;
	unsigned char devType = pRXCommand[1];
	unsigned char subType = pRXCommand[2];
	std::string ID = "";
	unsigned char Unit = 0;
	const tRBUF *pResponse = reinterpret_cast<const tRBUF *>(pRXCommand);
	char szTmp[300];
	std::vector<std::vector<std::string> > result;

	switch (devType) {
	case pTypeLighting1:
		sprintf(szTmp, "%d", pResponse->LIGHTING1.housecode);
		ID = szTmp;
		Unit = pResponse->LIGHTING1.unitcode;
		break;
	case pTypeLighting2:
		sprintf(szTmp, "%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
		ID = szTmp;
		Unit = pResponse->LIGHTING2.unitcode;
		break;
	case pTypeLighting5:
		if (subType != sTypeEMW100)
			sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
		else
			sprintf(szTmp, "%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
		ID = szTmp;
		Unit = pResponse->LIGHTING5.unitcode;
		break;
	case pTypeLighting6:
		sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2, pResponse->LIGHTING6.groupcode);
		ID = szTmp;
		Unit = pResponse->LIGHTING6.unitcode;
		break;
	case pTypeHomeConfort:
		sprintf(szTmp, "%02X%02X%02X%02X", pResponse->HOMECONFORT.id1, pResponse->HOMECONFORT.id2, pResponse->HOMECONFORT.id3, pResponse->HOMECONFORT.housecode);
		ID = szTmp;
		Unit = pResponse->HOMECONFORT.unitcode;
		break;
	case pTypeRadiator1:
		if (subType == sTypeSmartwaresSwitchRadiator)
		{
			sprintf(szTmp, "%X%02X%02X%02X", pResponse->RADIATOR1.id1, pResponse->RADIATOR1.id2, pResponse->RADIATOR1.id3, pResponse->RADIATOR1.id4);
			ID = szTmp;
			Unit = pResponse->RADIATOR1.unitcode;
		}
		break;
	case pTypeLimitlessLights:
	{
		_tLimitlessLights *pLed = (_tLimitlessLights *)pResponse;
		ID = "1";
		Unit = pLed->dunit;
	}
	break;
	case pTypeCurtain:
		sprintf(szTmp, "%d", pResponse->CURTAIN1.housecode);
		ID = szTmp;
		Unit = pResponse->CURTAIN1.unitcode;
		break;
	case pTypeBlinds:
		sprintf(szTmp, "%02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2, pResponse->BLINDS1.id3);
		ID = szTmp;
		Unit = pResponse->BLINDS1.unitcode;
		break;
	case pTypeRFY:
		sprintf(szTmp, "%02X%02X%02X", pResponse->RFY.id1, pResponse->RFY.id2, pResponse->RFY.id3);
		ID = szTmp;
		Unit = pResponse->RFY.unitcode;
		break;
	case pTypeSecurity1:
		sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
		ID = szTmp;
		Unit = 0;
		break;
	case pTypeSecurity2:
		sprintf(szTmp, "%02X%02X%02X%02X%02X%02X%02X%02X", pResponse->SECURITY2.id1, pResponse->SECURITY2.id2, pResponse->SECURITY2.id3, pResponse->SECURITY2.id4, pResponse->SECURITY2.id5, pResponse->SECURITY2.id6, pResponse->SECURITY2.id7, pResponse->SECURITY2.id8);
		ID = szTmp;
		Unit = 0;
		break;
	case pTypeChime:
		sprintf(szTmp, "%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
		ID = szTmp;
		Unit = pResponse->CHIME.sound;
		break;
	case pTypeThermostat:
	{
		const _tThermostat *pMeter = reinterpret_cast<const _tThermostat*>(pResponse);
		sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
		ID = szTmp;
		Unit = pMeter->dunit;
	}
	break;
	case pTypeThermostat2:
		ID = "1";
		Unit = pResponse->THERMOSTAT2.unitcode;
		break;
	case pTypeThermostat3:
		sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT3.unitcode1, pResponse->THERMOSTAT3.unitcode2, pResponse->THERMOSTAT3.unitcode3);
		ID = szTmp;
		Unit = 0;
		break;
	case pTypeThermostat4:
		sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT4.unitcode1, pResponse->THERMOSTAT4.unitcode2, pResponse->THERMOSTAT4.unitcode3);
		ID = szTmp;
		Unit = 0;
		break;
	case pTypeGeneralSwitch:
	{
		const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pResponse);
		sprintf(szTmp, "%08X", pSwitch->id);
		ID = szTmp;
		Unit = pSwitch->unitcode;
	}
	break;
	default:
		return -1;
	}

	if (ID != "")
	{
		// find our original hardware
		// if it is not a domoticz type, perform the actual command

		result = m_sql.safe_query(
			"SELECT HardwareID,ID,Name,StrParam1,StrParam2,nValue,sValue FROM DeviceStatus WHERE (DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			ID.c_str(), Unit, devType, subType);
		if (result.size() == 1)
		{
			std::vector<std::string> sd = result[0];

			CDomoticzHardwareBase *pHardware = GetHardware(atoi(sd[0].c_str()));
			if (pHardware != NULL)
			{
				if (pHardware->HwdType != HTYPE_Domoticz)
				{
					*pOriginalHardware = pHardware;
					pHardware->WriteToHardware((const char*)pRXCommand, pRXCommand[0] + 1);
					std::stringstream s_strid;
					s_strid << std::dec << sd[1];
					uint64_t ullID;
					s_strid >> ullID;
					return ullID;
				}
			}
		}
	}
	return -1;
}

void MainWorker::DecodeRXMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)
{
	if ((pHardware == NULL) || (pRXCommand == NULL))
		return;
	if ((pHardware->HwdType == HTYPE_Domoticz) && (pHardware->m_HwdID == 8765))
	{
		//Directly process the command
		boost::lock_guard<boost::mutex> l(m_decodeRXMessageMutex);
		ProcessRXMessage(pHardware, pRXCommand, defaultName, BatteryLevel);
	}
	else
	{
		// Submit command without waiting for the command to be processed
		PushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel);
	}
}

void MainWorker::PushRxMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)
{
	// Check command, submit it without waiting for it to be processed
	CheckAndPushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel, false);
}

void MainWorker::PushAndWaitRxMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)
{
	// Check command, submit it and wait for it to be processed
	CheckAndPushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel, true);
}

void MainWorker::CheckAndPushRxMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel, const bool wait)
{
	if ((pHardware == NULL) || (pRXCommand == NULL)) {
		_log.Log(LOG_ERROR, "RxQueue: cannot push message with undefined hardware (%s) or command (%s)",
			(pHardware == NULL) ? "null" : "not null",
			(pRXCommand == NULL) ? "null" : "not null");
		return;
	}
	if (pHardware->m_HwdID < 1) {
		_log.Log(LOG_ERROR, "RxQueue: cannot push message with invalid hardware id (id=%d, type=%d, name=%s)",
			pHardware->m_HwdID,
			pHardware->HwdType,
			pHardware->Name.c_str());
		return;
	}

	// Build queue item
	_tRxQueueItem rxMessage;
	if (defaultName != NULL)
	{
		rxMessage.Name = defaultName;
	}
	rxMessage.BatteryLevel = BatteryLevel;
	rxMessage.rxMessageIdx = m_rxMessageIdx++;
	rxMessage.hardwareId = pHardware->m_HwdID;
	// defensive copy of the command
	rxMessage.vrxCommand.resize(pRXCommand[0] + 1);
	rxMessage.vrxCommand.insert(rxMessage.vrxCommand.begin(), pRXCommand, pRXCommand + pRXCommand[0] + 1);
	rxMessage.crc = 0x0;
#ifdef DEBUG_RXQUEUE
	// CRC
	boost::crc_optimal<16, 0x1021, 0xFFFF, 0, false, false> crc_ccitt2;
	crc_ccitt2 = std::for_each(pRXCommand, pRXCommand + pRXCommand[0] + 1, crc_ccitt2);
	rxMessage.crc = crc_ccitt2();
#endif

	if (m_stopRxMessageThread) {
		// Server is stopping
		return;
	}

	// Trigger
	rxMessage.trigger = NULL; // Should be initialized to NULL if trigger is no used
	if (wait) { // add trigger to wait for the message to be processed
		rxMessage.trigger = new queue_element_trigger();
	}

#ifdef DEBUG_RXQUEUE
	_log.Log(LOG_STATUS, "RxQueue: push a rxMessage(%lu) (hrdwId=%d, hrdwType=%d, hrdwName=%s, type=%02X, subtype=%02X)",
		rxMessage.rxMessageIdx,
		pHardware->m_HwdID,
		pHardware->HwdType,
		pHardware->Name.c_str(),
		pRXCommand[1],
		pRXCommand[2]);
#endif

	// Push item to queue
	m_rxMessageQueue.push(rxMessage);

	if (rxMessage.trigger != NULL) {
#ifdef DEBUG_RXQUEUE
		_log.Log(LOG_STATUS, "RxQueue: wait for rxMessage(%lu) to be processed...", rxMessage.rxMessageIdx);
#endif
		while (!rxMessage.trigger->timed_wait(boost::posix_time::milliseconds(1000))) {
#ifdef DEBUG_RXQUEUE
			_log.Log(LOG_STATUS, "RxQueue: wait 1s for rxMessage(%lu) to be processed...", rxMessage.rxMessageIdx);
#endif
			if (m_stopRxMessageThread) {
				// Server is stopping
				break;
			}
		}
#ifdef DEBUG_RXQUEUE
		if (moreThanTimeout) {
			_log.Log(LOG_STATUS, "RxQueue: rxMessage(%lu) processed", rxMessage.rxMessageIdx);
		}
#endif
		delete rxMessage.trigger;
	}
}

void MainWorker::UnlockRxMessageQueue()
{
#ifdef DEBUG_RXQUEUE
	_log.Log(LOG_STATUS, "RxQueue: unlock queue using dummy message");
#endif
	// Push dummy message to unlock queue
	_tRxQueueItem rxMessage;
	rxMessage.rxMessageIdx = m_rxMessageIdx++;
	rxMessage.hardwareId = -1;
	rxMessage.trigger = NULL;
	rxMessage.BatteryLevel = 0;
	m_rxMessageQueue.push(rxMessage);
}

void MainWorker::Do_Work_On_Rx_Messages()
{
	_log.Log(LOG_STATUS, "RxQueue: queue worker started...");

	m_stopRxMessageThread = false;
	while (true) {
		if (m_stopRxMessageThread) {
			// Server is stopping
			break;
		}

		// Wait and pop next message or timeout
		_tRxQueueItem rxQItem;
		bool hasPopped = m_rxMessageQueue.timed_wait_and_pop<boost::posix_time::milliseconds>(rxQItem,
			boost::posix_time::milliseconds(5000));// (if no message for 2 seconds, returns anyway to check m_stopRxMessageThread)

		if (!hasPopped) {
			// Timeout occurred : queue is empty
#ifdef DEBUG_RXQUEUE
				//_log.Log(LOG_STATUS, "RxQueue: the queue has been empty for five seconds");
#endif
			continue;
		}
		if (rxQItem.hardwareId == -1) {
			// dummy message
#ifdef DEBUG_RXQUEUE
			_log.Log(LOG_STATUS, "RxQueue: dummy message popped");
#endif
			continue;
		}
		if (rxQItem.hardwareId < 1) {
			_log.Log(LOG_ERROR, "RxQueue: cannot process invalid hardware id: (%d)", rxQItem.hardwareId);
			// cannot process message with invalid id or null message
			if (rxQItem.trigger != NULL) rxQItem.trigger->popped();
			continue;
		}

		const CDomoticzHardwareBase *pHardware = GetHardware(rxQItem.hardwareId);

		// Check pointers
		if (pHardware == NULL) {
			_log.Log(LOG_ERROR, "RxQueue: cannot retrieve hardware with id: %d", rxQItem.hardwareId);
			if (rxQItem.trigger != NULL) rxQItem.trigger->popped();
			continue;
		}
		if (rxQItem.vrxCommand.empty()) {
			_log.Log(LOG_ERROR, "RxQueue: cannot retrieve command with id: %d", rxQItem.hardwareId);
			if (rxQItem.trigger != NULL) rxQItem.trigger->popped();
			continue;
		}

		const unsigned char *pRXCommand = &rxQItem.vrxCommand[0];

#ifdef DEBUG_RXQUEUE
		// CRC
		boost::uint16_t crc = rxQItem.crc;
		boost::crc_optimal<16, 0x1021, 0xFFFF, 0, false, false> crc_ccitt2;
		crc_ccitt2 = std::for_each(pRXCommand, pRXCommand + rxQItem.vrxCommand.size(), crc_ccitt2);
		if (crc != crc_ccitt2()) {
			_log.Log(LOG_ERROR, "RxQueue: cannot process invalid rxMessage(%lu) from hardware with id=%d (type %d)",
				rxQItem.rxMessageIdx,
				rxQItem.hardwareId,
				pHardware->HwdType);
			if (rxQItem.trigger != NULL) rxQItem.trigger->popped();
			continue;
		}

		_log.Log(LOG_STATUS, "RxQueue: process a rxMessage(%lu) (hrdwId=%d, hrdwType=%d, hrdwName=%s, type=%02X, subtype=%02X)",
			rxQItem.rxMessageIdx,
			pHardware->m_HwdID,
			pHardware->HwdType,
			pHardware->Name.c_str(),
			pRXCommand[1],
			pRXCommand[2]);
#endif
		ProcessRXMessage(pHardware, pRXCommand, rxQItem.Name.c_str(), rxQItem.BatteryLevel);
		if (rxQItem.trigger != NULL)
		{
			rxQItem.trigger->popped();
		}
	}

	_log.Log(LOG_STATUS, "RxQueue: queue worker stopped...");
}

void MainWorker::ProcessRXMessage(const CDomoticzHardwareBase *pHardware, const unsigned char *pRXCommand, const char *defaultName, const int BatteryLevel)
{
	// current date/time based on current system
	size_t Len = pRXCommand[0] + 1;

	int HwdID = pHardware->m_HwdID;
	_eHardwareTypes HwdType = pHardware->HwdType;

	const_cast<CDomoticzHardwareBase *>(pHardware)->SetHeartbeatReceived();

	uint64_t DeviceRowIdx = -1;
	std::string DeviceName = "";
	tcp::server::CTCPClient *pClient2Ignore = NULL;

	if (_log.isTraceEnabled()) {
		char  mes[sizeof(tRBUF) * 2 + 2];
		char * ptmes = mes;
		for (size_t i = 0; i < Len; i++) {
			sprintf(ptmes, "%02X", pRXCommand[i]);
			ptmes += 2;
		}
		*ptmes = 0;

		_log.Log(LOG_TRACE, "MAIN ProcessRX Msg %s", mes);
	}

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		if (pHardware->m_HwdID == 8765) //did we receive it from our master?
		{
			CDomoticzHardwareBase *pOrgHardware = NULL;
			switch (pRXCommand[1])
			{
			case pTypeLighting1:
			case pTypeLighting2:
			case pTypeLighting3:
			case pTypeLighting4:
			case pTypeLighting5:
			case pTypeLighting6:
			case pTypeLimitlessLights:
			case pTypeCurtain:
			case pTypeBlinds:
			case pTypeRFY:
			case pTypeSecurity1:
			case pTypeSecurity2:
			case pTypeChime:
			case pTypeThermostat:
			case pTypeThermostat2:
			case pTypeThermostat3:
			case pTypeThermostat4:
			case pTypeRadiator1:
			case pTypeGeneralSwitch:
			case pTypeHomeConfort:
			case pTypeFan:
				//we received a control message from a domoticz client,
				//and should actually perform this command ourself switch
				DeviceRowIdx = PerformRealActionFromDomoticzClient(pRXCommand, &pOrgHardware);
				if (DeviceRowIdx != -1)
				{
					if (pOrgHardware != NULL)
					{
						DeviceRowIdx = -1;
						pClient2Ignore = (tcp::server::CTCPClient*)pHardware->m_pUserData;
						pHardware = pOrgHardware;
						HwdID = pOrgHardware->m_HwdID;
					}
					WriteMessage("Control Command, ", (pOrgHardware == NULL));
				}
				break;
			}
		}
	}

	_tRxMessageProcessingResult procResult;
	procResult.DeviceName = "";
	procResult.DeviceRowIdx = -1;
	procResult.bProcessBatteryValue = true;
	if (DeviceRowIdx == -1)
	{
		switch (pRXCommand[1])
		{
		case pTypeInterfaceMessage:
			decode_InterfaceMessage(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeInterfaceControl:
			decode_InterfaceControl(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRecXmitMessage:
			decode_RecXmitMessage(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeUndecoded:
			decode_UNDECODED(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting1:
			decode_Lighting1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting2:
			decode_Lighting2(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting3:
			decode_Lighting3(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting4:
			decode_Lighting4(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting5:
			decode_Lighting5(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLighting6:
			decode_Lighting6(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeFan:
			decode_Fan(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeCurtain:
			decode_Curtain(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeBlinds:
			decode_BLINDS1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRFY:
			decode_RFY(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeSecurity1:
			decode_Security1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeSecurity2:
			decode_Security2(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeEvohome:
			decode_evohome1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeEvohomeZone:
		case pTypeEvohomeWater:
			decode_evohome2(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeEvohomeRelay:
			decode_evohome3(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeCamera:
			decode_Camera1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRemote:
			decode_Remote(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeThermostat: //own type
			decode_Thermostat(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeThermostat1:
			decode_Thermostat1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeThermostat2:
			decode_Thermostat2(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeThermostat3:
			decode_Thermostat3(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeThermostat4:
			decode_Thermostat4(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRadiator1:
			decode_Radiator1(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeTEMP:
			decode_Temp(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeHUM:
			decode_Hum(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeTEMP_HUM:
			decode_TempHum(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeTEMP_RAIN:
			decode_TempRain(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeBARO:
			decode_Baro(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeTEMP_HUM_BARO:
			decode_TempHumBaro(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeTEMP_BARO:
			decode_TempBaro(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRAIN:
			decode_Rain(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeWIND:
			decode_Wind(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeUV:
			decode_UV(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeDT:
			decode_DateTime(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeCURRENT:
			decode_Current(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeENERGY:
			decode_Energy(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeCURRENTENERGY:
			decode_Current_Energy(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeGAS:
			decode_Gas(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeWATER:
			decode_Water(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeWEIGHT:
			decode_Weight(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRFXSensor:
			decode_RFXSensor(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRFXMeter:
			decode_RFXMeter(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeP1Power:
			decode_P1MeterPower(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeP1Gas:
			decode_P1MeterGas(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeUsage:
			decode_Usage(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeYouLess:
			decode_YouLessMeter(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeAirQuality:
			decode_AirQuality(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRego6XXTemp:
			decode_Rego6XXTemp(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeRego6XXValue:
			decode_Rego6XXValue(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeFS20:
			decode_FS20(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLux:
			decode_Lux(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeGeneral:
			decode_General(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeChime:
			decode_Chime(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeBBQ:
			decode_BBQ(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypePOWER:
			decode_Power(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeLimitlessLights:
			decode_LimitlessLights(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeGeneralSwitch:
			decode_GeneralSwitch(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeHomeConfort:
			decode_HomeConfort(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		case pTypeCARTELECTRONIC:
			decode_Cartelectronic(HwdID, HwdType, reinterpret_cast<const tRBUF *>(pRXCommand), procResult);
			break;
		default:
			_log.Log(LOG_ERROR, "UNHANDLED PACKET TYPE:      FS20 %02X", pRXCommand[1]);
			return;
		}
		DeviceRowIdx = procResult.DeviceRowIdx;
		DeviceName = procResult.DeviceName;
	}

	if (DeviceRowIdx == -1)
		return;

	if ((BatteryLevel != -1) && (procResult.bProcessBatteryValue))
	{
		m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d WHERE (ID==%" PRIu64 ")", BatteryLevel, DeviceRowIdx);
	}

	if ((defaultName != NULL) && ((DeviceName == "Unknown") || (DeviceName.empty())))
	{
		if (strlen(defaultName) > 0)
		{
			DeviceName = defaultName;
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID==%" PRIu64 ")", defaultName, DeviceRowIdx);
		}
	}

	if (pHardware->m_bOutputLog)
	{
		std::string sdevicetype = RFX_Type_Desc(pRXCommand[1], 1);
		if (pRXCommand[1] == pTypeGeneral)
		{
			const _tGeneralDevice *pMeter = reinterpret_cast<const _tGeneralDevice*>(pRXCommand);
			sdevicetype += "/" + std::string(RFX_Type_SubType_Desc(pMeter->type, pMeter->subtype));
		}
		std::stringstream sTmp;
		sTmp << "(" << pHardware->Name << ") " << sdevicetype << " (" << DeviceName << ")";
		WriteMessageStart();
		WriteMessage(sTmp.str().c_str());
		WriteMessageEnd();
	}

	//Send to connected Sharing Users
	m_sharedserver.SendToAll(pHardware->m_HwdID, DeviceRowIdx, (const char*)pRXCommand, pRXCommand[0] + 1, pClient2Ignore);

	sOnDeviceReceived(pHardware->m_HwdID, DeviceRowIdx, DeviceName, pRXCommand);
}

void MainWorker::decode_InterfaceMessage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];

	WriteMessageStart();

	switch (pResponse->IRESPONSE.subtype)
	{
	case sTypeInterfaceCommand:
	{
		int mlen = pResponse->IRESPONSE.packetlength;
		WriteMessage("subtype           = Interface Response");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
		WriteMessage(szTmp);
		switch (pResponse->IRESPONSE.cmnd)
		{
		case cmdSTATUS:
		case cmdSETMODE:
		case cmd310:
		case cmd315:
		case cmd800:
		case cmd800F:
		case cmd830:
		case cmd830F:
		case cmd835:
		case cmd835F:
		case cmd895:
		{
			WriteMessage("response on cmnd  = ", false);
			switch (pResponse->IRESPONSE.cmnd)
			{
			case cmdSTATUS:
				WriteMessage("Get Status");
				break;
			case cmdSETMODE:
				WriteMessage("Set Mode");
				break;
			case cmd310:
				WriteMessage("Select 310MHz");
				break;
			case cmd315:
				WriteMessage("Select 315MHz");
				break;
			case cmd800:
				WriteMessage("Select 868.00MHz");
				break;
			case cmd800F:
				WriteMessage("Select 868.00MHz FSK");
				break;
			case cmd830:
				WriteMessage("Select 868.30MHz");
				break;
			case cmd830F:
				WriteMessage("Select 868.30MHz FSK");
				break;
			case cmd835:
				WriteMessage("Select 868.35MHz");
				break;
			case cmd835F:
				WriteMessage("Select 868.35MHz FSK");
				break;
			case cmd895:
				WriteMessage("Select 868.95MHz");
				break;
			default:
				WriteMessage("Error: unknown response");
				break;
			}

			m_sql.UpdateRFXCOMHardwareDetails(HwdID, pResponse->IRESPONSE.msg1, pResponse->IRESPONSE.msg2, pResponse->ICMND.msg3, pResponse->ICMND.msg4, pResponse->ICMND.msg5, pResponse->ICMND.msg6);

			switch (pResponse->IRESPONSE.msg1)
			{
			case recType310:
				WriteMessage("Transceiver type  = 310MHz");
				break;
			case recType315:
				WriteMessage("Receiver type     = 315MHz");
				break;
			case recType43392:
				WriteMessage("Receiver type     = 433.92MHz (receive only)");
				break;
			case trxType43392:
				WriteMessage("Transceiver type  = 433.92MHz");
				break;
			case recType86800:
				WriteMessage("Receiver type     = 868.00MHz");
				break;
			case recType86800FSK:
				WriteMessage("Receiver type     = 868.00MHz FSK");
				break;
			case recType86830:
				WriteMessage("Receiver type     = 868.30MHz");
				break;
			case recType86830FSK:
				WriteMessage("Receiver type     = 868.30MHz FSK");
				break;
			case recType86835:
				WriteMessage("Receiver type     = 868.35MHz");
				break;
			case recType86835FSK:
				WriteMessage("Receiver type     = 868.35MHz FSK");
				break;
			case recType86895:
				WriteMessage("Receiver type     = 868.95MHz");
				break;
			default:
				WriteMessage("Receiver type     = unknown");
				break;
			}
			int FWType = 0;
			int FWVersion = 0;
			if (mlen > 13)
			{
				FWType = pResponse->IRESPONSE.msg10;
				FWVersion = pResponse->IRESPONSE.msg2 + 1000;
			}
			else
			{
				FWVersion = pResponse->IRESPONSE.msg2;
				if ((pResponse->IRESPONSE.msg1 == recType43392) && (FWVersion < 162))
					FWType = 0; //Type1 RFXrec
				else if ((pResponse->IRESPONSE.msg1 == recType43392) && (FWVersion < 162))
					FWType = 1; //Type1
				else if ((pResponse->IRESPONSE.msg1 == recType43392) && ((FWVersion > 162) && (FWVersion < 225)))
					FWType = 2; //Type2
				else
					FWType = 3; //Ext
			}
			sprintf(szTmp, "Firmware version  = %d", FWVersion);
			WriteMessage(szTmp);
			WriteMessage("Firmware type     = ", false);
			switch (FWType)
			{
			case 0:
				strcpy(szTmp, "Type1 RX");
				break;
			case 1:
				strcpy(szTmp, "Type1");
				break;
			case 2:
				strcpy(szTmp, "Type2");
				break;
			case 3:
				strcpy(szTmp, "Ext");
				break;
			case 4:
				strcpy(szTmp, "Ext2");
				break;
			default:
				strcpy(szTmp, "?");
				break;
			}
			WriteMessage(szTmp);

			CRFXBase *pMyHardware = reinterpret_cast<CRFXBase*>(GetHardware(HwdID));
			if (pMyHardware)
			{
				std::stringstream sstr;
				sstr << szTmp << "/" << FWVersion;
				pMyHardware->m_Version = sstr.str();
			}


			sprintf(szTmp, "Hardware version  = %d.%d", pResponse->IRESPONSE.msg7, pResponse->IRESPONSE.msg8);
			WriteMessage(szTmp);

			if (pResponse->IRESPONSE.UNDECODEDenabled)
				WriteMessage("Undec             on");
			else
				WriteMessage("Undec             off");

			if (pResponse->IRESPONSE.X10enabled)
				WriteMessage("X10               enabled");
			else
				WriteMessage("X10               disabled");

			if (pResponse->IRESPONSE.ARCenabled)
				WriteMessage("ARC               enabled");
			else
				WriteMessage("ARC               disabled");

			if (pResponse->IRESPONSE.ACenabled)
				WriteMessage("AC                enabled");
			else
				WriteMessage("AC                disabled");

			if (pResponse->IRESPONSE.HEEUenabled)
				WriteMessage("HomeEasy EU       enabled");
			else
				WriteMessage("HomeEasy EU       disabled");

			if (pResponse->IRESPONSE.MEIANTECHenabled)
				WriteMessage("Meiantech/Atlantic enabled");
			else
				WriteMessage("Meiantech/Atlantic disabled");

			if (pResponse->IRESPONSE.OREGONenabled)
				WriteMessage("Oregon Scientific enabled");
			else
				WriteMessage("Oregon Scientific disabled");

			if (pResponse->IRESPONSE.ATIenabled)
				WriteMessage("ATI/Cartelectronic enabled");
			else
				WriteMessage("ATI/Cartelectronic disabled");

			if (pResponse->IRESPONSE.VISONICenabled)
				WriteMessage("Visonic           enabled");
			else
				WriteMessage("Visonic           disabled");

			if (pResponse->IRESPONSE.MERTIKenabled)
				WriteMessage("Mertik            enabled");
			else
				WriteMessage("Mertik            disabled");

			if (pResponse->IRESPONSE.LWRFenabled)
				WriteMessage("AD                enabled");
			else
				WriteMessage("AD                disabled");

			if (pResponse->IRESPONSE.HIDEKIenabled)
				WriteMessage("Hideki            enabled");
			else
				WriteMessage("Hideki            disabled");

			if (pResponse->IRESPONSE.LACROSSEenabled)
				WriteMessage("La Crosse         enabled");
			else
				WriteMessage("La Crosse         disabled");

			if (pResponse->IRESPONSE.FS20enabled)
				WriteMessage("FS20/Legrand      enabled");
			else
				WriteMessage("FS20/Legrand      disabled");

			if (pResponse->IRESPONSE.PROGUARDenabled)
				WriteMessage("ProGuard          enabled");
			else
				WriteMessage("ProGuard          disabled");

			if (pResponse->IRESPONSE.BLINDST0enabled)
				WriteMessage("BlindsT0          enabled");
			else
				WriteMessage("BlindsT0          disabled");

			if (pResponse->IRESPONSE.BLINDST1enabled)
				WriteMessage("BlindsT1          enabled");
			else
				WriteMessage("BlindsT1          disabled");

			if (pResponse->IRESPONSE.AEenabled)
				WriteMessage("AE                enabled");
			else
				WriteMessage("AE                disabled");

			if (pResponse->IRESPONSE.RUBICSONenabled)
				WriteMessage("RUBiCSON          enabled");
			else
				WriteMessage("RUBiCSON          disabled");

			if (pResponse->IRESPONSE.FINEOFFSETenabled)
				WriteMessage("FineOffset        enabled");
			else
				WriteMessage("FineOffset        disabled");

			if (pResponse->IRESPONSE.LIGHTING4enabled)
				WriteMessage("Lighting4         enabled");
			else
				WriteMessage("Lighting4         disabled");

			if (pResponse->IRESPONSE.RSLenabled)
				WriteMessage("Conrad RSL        enabled");
			else
				WriteMessage("Conrad RSL        disabled");

			if (pResponse->IRESPONSE.SXenabled)
				WriteMessage("ByronSX           enabled");
			else
				WriteMessage("ByronSX           disabled");

			if (pResponse->IRESPONSE.IMAGINTRONIXenabled)
				WriteMessage("IMAGINTRONIX      enabled");
			else
				WriteMessage("IMAGINTRONIX      disabled");

			if (pResponse->IRESPONSE.KEELOQenabled)
				WriteMessage("KEELOQ            enabled");
			else
				WriteMessage("KEELOQ            disabled");

			if (pResponse->IRESPONSE.HCEnabled)
				WriteMessage("Home Confort      enabled");
			else
				WriteMessage("Home Confort      disabled");
		}
		break;
		case cmdSAVE:
			WriteMessage("response on cmnd  = Save");
			break;
		}
		break;
	}
	break;
	case sTypeUnknownRFYremote:
		WriteMessage("subtype           = Unknown RFY remote! Use the Program command to create a remote in the RFXtrx433Ext");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
		WriteMessage(szTmp);
		break;
	case sTypeExtError:
		WriteMessage("subtype           = No RFXtrx433E hardware detected");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
		WriteMessage(szTmp);
		break;
	case sTypeRFYremoteList:
		if ((pResponse->ICMND.xmitpwr == 0) && (pResponse->ICMND.msg3 == 0) && (pResponse->ICMND.msg4 == 0) && (pResponse->ICMND.msg5 == 0))
		{
			sprintf(szTmp, "subtype           = RFY remote: %d is empty", pResponse->ICMND.freqsel);
			WriteMessage(szTmp);
		}
		else
		{
			sprintf(szTmp, "subtype           = RFY remote: %d, ID: %02d%02d%02d, unitnbr: %d",
				pResponse->ICMND.freqsel,
				pResponse->ICMND.xmitpwr,
				pResponse->ICMND.msg3,
				pResponse->ICMND.msg4,
				pResponse->ICMND.msg5);
			WriteMessage(szTmp);
		}
		break;
	case sTypeASAremoteList:
		if ((pResponse->ICMND.xmitpwr == 0) && (pResponse->ICMND.msg3 == 0) && (pResponse->ICMND.msg4 == 0) && (pResponse->ICMND.msg5 == 0))
		{
			sprintf(szTmp, "subtype           = ASA remote: %d is empty", pResponse->ICMND.freqsel);
			WriteMessage(szTmp);
		}
		else
		{
			sprintf(szTmp, "subtype           = ASA remote: %d, ID: %02d%02d%02d, unitnbr: %d",
				pResponse->ICMND.freqsel,
				pResponse->ICMND.xmitpwr,
				pResponse->ICMND.msg3,
				pResponse->ICMND.msg4,
				pResponse->ICMND.msg5);
			WriteMessage(szTmp);
		}
		break;
	case sTypeInterfaceWrongCommand:
		if (pResponse->IRESPONSE.cmnd == 0x07)
		{
			WriteMessage("Please upgrade your RFXTrx Firmware!");
		}
		else
		{
			sprintf(szTmp, "subtype          = Wrong command received from application (%d)", pResponse->IRESPONSE.cmnd);
			WriteMessage(szTmp);
			sprintf(szTmp, "Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
			WriteMessage(szTmp);
		}
		break;
	}
	WriteMessageEnd();
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_InterfaceControl(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	WriteMessageStart();
	switch (pResponse->IRESPONSE.subtype)
	{
	case sTypeInterfaceCommand:
		WriteMessage("subtype           = Interface Command");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->IRESPONSE.seqnbr);
		WriteMessage(szTmp);
		switch (pResponse->IRESPONSE.cmnd)
		{
		case cmdRESET:
			WriteMessage("reset the receiver/transceiver");
			break;
		case cmdSTATUS:
			WriteMessage("return firmware versions and configuration of the interface");
			break;
		case cmdSETMODE:
			WriteMessage("set configuration of the interface");
			break;
		case cmdSAVE:
			WriteMessage("save receiving modes of the receiver/transceiver in non-volatile memory");
			break;
		case cmdStartRec:
			WriteMessage("start RFXtrx receiver");
			break;
		case cmd310:
			WriteMessage("select 310MHz in the 310/315 transceiver");
			break;
		case cmd315:
			WriteMessage("select 315MHz in the 310/315 transceiver");
			break;
		case cmd800:
			WriteMessage("select 868.00MHz ASK in the 868 transceiver");
			break;
		case cmd800F:
			WriteMessage("select 868.00MHz FSK in the 868 transceiver");
			break;
		case cmd830:
			WriteMessage("select 868.30MHz ASK in the 868 transceiver");
			break;
		case cmd830F:
			WriteMessage("select 868.30MHz FSK in the 868 transceiver");
			break;
		case cmd835:
			WriteMessage("select 868.35MHz ASK in the 868 transceiver");
			break;
		case cmd835F:
			WriteMessage("select 868.35MHz FSK in the 868 transceiver");
			break;
		case cmd895:
			WriteMessage("select 868.95MHz in the 868 transceiver");
			break;
		}
		break;
	}
	WriteMessageEnd();
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_BateryLevel(bool bIsInPercentage, unsigned char level)
{
	if (bIsInPercentage)
	{
		switch (level)
		{
		case 0:
			WriteMessage("Battery       = 10%");
			break;
		case 1:
			WriteMessage("Battery       = 20%");
			break;
		case 2:
			WriteMessage("Battery       = 30%");
			break;
		case 3:
			WriteMessage("Battery       = 40%");
			break;
		case 4:
			WriteMessage("Battery       = 50%");
			break;
		case 5:
			WriteMessage("Battery       = 60%");
			break;
		case 6:
			WriteMessage("Battery       = 70%");
			break;
		case 7:
			WriteMessage("Battery       = 80%");
			break;
		case 8:
			WriteMessage("Battery       = 90%");
			break;
		case 9:
			WriteMessage("Battery       = 100%");
			break;
		}
	}
	else
	{
		if (level == 0)
		{
			WriteMessage("Battery       = Low");
		}
		else
		{
			WriteMessage("Battery       = OK");
		}
	}
}

unsigned char MainWorker::get_BateryLevel(const _eHardwareTypes HwdType, bool bIsInPercentage, unsigned char level)
{
	if (HwdType == HTYPE_OpenZWave)
	{
		bIsInPercentage = true;
	}
	unsigned char ret = 0;
	if (bIsInPercentage)
	{
		if (level >= 0 && level <= 9)
			ret = (level + 1) * 10;
	}
	else
	{
		if (level == 0)
		{
			ret = 0;
		}
		else
		{
			ret = 100;
		}
	}
	return ret;
}

void MainWorker::decode_Rain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeRAIN;
	unsigned char subType = pResponse->RAIN.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->RAIN.id1 * 256) + pResponse->RAIN.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->RAIN.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, pResponse->RAIN.subtype == sTypeRAIN1, pResponse->RAIN.battery_level & 0x0F);

	int Rainrate = (pResponse->RAIN.rainrateh * 256) + pResponse->RAIN.rainratel;

	float TotalRain = float((pResponse->RAIN.raintotal1 * 65535) + (pResponse->RAIN.raintotal2 * 256) + pResponse->RAIN.raintotal3) / 10.0f;

	if (subType != sTypeRAINWU)
	{
		Rainrate = 0;
		//Calculate our own rainrate
		std::vector<std::vector<std::string> > result;

		//Get our index
		result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", HwdID, ID.c_str(), Unit, devType, subType);
		if (result.size() != 0)
		{
			uint64_t ulID;
			std::stringstream s_str(result[0][0]);
			s_str >> ulID;

			//Get Counter from one Hour ago
			time_t now = mytime(NULL);
			now -= 3600; //subtract one hour
			struct tm ltime;
			localtime_r(&now, &ltime);

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query(
				"SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%04d-%02d-%02d %02d:%02d:%02d')",
				ulID, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
			if (result.size() == 1)
			{
				float totalRainFallLastHour = TotalRain - static_cast<float>(atof(result[0][0].c_str()));
				Rainrate = round(totalRainFallLastHour*100.0f);
			}
		}
	}

	sprintf(szTmp, "%d;%.1f", Rainrate, TotalRain);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleRainNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_RAIN, TotalRain);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (subType)
		{
		case sTypeRAIN1:
			WriteMessage("subtype       = RAIN1 - RGR126/682/918/928");
			break;
		case sTypeRAIN2:
			WriteMessage("subtype       = RAIN2 - PCR800");
			break;
		case sTypeRAIN3:
			WriteMessage("subtype       = RAIN3 - TFA");
			break;
		case sTypeRAIN4:
			WriteMessage("subtype       = RAIN4 - UPM RG700");
			break;
		case sTypeRAIN5:
			WriteMessage("subtype       = RAIN5 - LaCrosse WS2300");
			break;
		case sTypeRAIN6:
			WriteMessage("subtype       = RAIN6 - LaCrosse TX5");
			break;
		case sTypeRAINWU:
			WriteMessage("subtype       = Weather Underground (Total Rain)");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X : %02X", pResponse->RAIN.packettype, pResponse->RAIN.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->RAIN.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		if (pResponse->RAIN.subtype == sTypeRAIN1)
		{
			sprintf(szTmp, "Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}
		else if (pResponse->RAIN.subtype == sTypeRAIN2)
		{
			sprintf(szTmp, "Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Total rain    = %.1f mm", TotalRain);
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->RAIN.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->RAIN.subtype == sTypeRAIN1, pResponse->RAIN.battery_level & 0x0F);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Wind(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[300];
	unsigned char devType = pTypeWIND;
	unsigned char subType = pResponse->WIND.subtype;
	unsigned short windID = (pResponse->WIND.id1 * 256) + pResponse->WIND.id2;
	sprintf(szTmp, "%d", windID);
	std::string ID = szTmp;
	unsigned char Unit = 0;

	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->WIND.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, pResponse->WIND.subtype == sTypeWIND3, pResponse->WIND.battery_level & 0x0F);

	double dDirection;
	dDirection = (double)(pResponse->WIND.directionh * 256) + pResponse->WIND.directionl;
	dDirection = m_wind_calculator[windID].AddValueAndReturnAvarage(dDirection);

	std::string strDirection;
	if (dDirection > 348.75 || dDirection < 11.26)
		strDirection = "N";
	else if (dDirection < 33.76)
		strDirection = "NNE";
	else if (dDirection < 56.26)
		strDirection = "NE";
	else if (dDirection < 78.76)
		strDirection = "ENE";
	else if (dDirection < 101.26)
		strDirection = "E";
	else if (dDirection < 123.76)
		strDirection = "ESE";
	else if (dDirection < 146.26)
		strDirection = "SE";
	else if (dDirection < 168.76)
		strDirection = "SSE";
	else if (dDirection < 191.26)
		strDirection = "S";
	else if (dDirection < 213.76)
		strDirection = "SSW";
	else if (dDirection < 236.26)
		strDirection = "SW";
	else if (dDirection < 258.76)
		strDirection = "WSW";
	else if (dDirection < 281.26)
		strDirection = "W";
	else if (dDirection < 303.76)
		strDirection = "WNW";
	else if (dDirection < 326.26)
		strDirection = "NW";
	else if (dDirection < 348.76)
		strDirection = "NNW";
	else
		strDirection = "---";

	dDirection = round(dDirection);

	int intSpeed = (pResponse->WIND.av_speedh * 256) + pResponse->WIND.av_speedl;
	int intGust = (pResponse->WIND.gusth * 256) + pResponse->WIND.gustl;

	if (pResponse->WIND.subtype == sTypeWIND6)
	{
		//LaCrosse WS2300
		//This sensor is only reporting gust, speed=gust
		intSpeed = intGust;
	}

	m_wind_calculator[windID].SetSpeedGust(intSpeed, intGust);

	float temp = 0, chill = 0;
	if (pResponse->WIND.subtype == sTypeWIND4)
	{
		if (!pResponse->WIND.tempsign)
		{
			temp = float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0f;
		}
		else
		{
			temp = -(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0f);
		}
		if ((temp < -200) || (temp > 380))
		{
			WriteMessage(" Invalid Temperature");
			return;
		}

		float AddjValue = 0.0f;
		float AddjMulti = 1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;

		if (!pResponse->WIND.chillsign)
		{
			chill = float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0f;
		}
		else
		{
			chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0f);
		}
		chill += AddjValue;
	}
	else if (pResponse->WIND.subtype == sTypeWINDNoTemp)
	{
		if (!pResponse->WIND.tempsign)
		{
			temp = float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0f;
		}
		else
		{
			temp = -(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0f);
		}
		if ((temp < -200) || (temp > 380))
		{
			WriteMessage(" Invalid Temperature");
			return;
		}

		float AddjValue = 0.0f;
		float AddjMulti = 1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;

		if (!pResponse->WIND.chillsign)
		{
			chill = float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0f;
		}
		else
		{
			chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0f);
		}
		chill += AddjValue;
	}
	if (chill == 0)
	{
		float wspeedms = float(intSpeed) / 10.0f;
		if ((temp < 10.0) && (wspeedms >= 1.4))
		{
			float chillJatTI = 13.12f + 0.6215f*temp - 11.37f*pow(wspeedms*3.6f, 0.16f) + 0.3965f*temp*pow(wspeedms*3.6f, 0.16f);
			chill = chillJatTI;
		}
	}

	sprintf(szTmp, "%.2f;%s;%d;%d;%.1f;%.1f", dDirection, strDirection.c_str(), intSpeed, intGust, temp, chill);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	float wspeedms = float(intSpeed) / 10.0f;
	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_WIND, wspeedms);

	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, 0, true, false);


	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->WIND.subtype)
		{
		case sTypeWIND1:
			WriteMessage("subtype       = WIND1 - WTGR800");
			break;
		case sTypeWIND2:
			WriteMessage("subtype       = WIND2 - WGR800");
			break;
		case sTypeWIND3:
			WriteMessage("subtype       = WIND3 - STR918/928, WGR918");
			break;
		case sTypeWIND4:
			WriteMessage("subtype       = WIND4 - TFA");
			break;
		case sTypeWIND5:
			WriteMessage("subtype       = WIND5 - UPM WDS500");
			break;
		case sTypeWIND6:
			WriteMessage("subtype       = WIND6 - LaCrosse WS2300");
			break;
		case sTypeWIND7:
			WriteMessage("subtype       = WIND7 - Alecto WS4500");
			break;
		case sTypeWINDNoTemp:
			WriteMessage("subtype       = Weather Station");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->WIND.packettype, pResponse->WIND.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->WIND.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		sprintf(szTmp, "Direction     = %d degrees %s", int(dDirection), strDirection.c_str());
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype != sTypeWIND5)
		{
			sprintf(szTmp, "Average speed = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intSpeed) / 10.0f, (float(intSpeed) * 0.36f), (float(intSpeed) * 0.223693629f));
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Wind gust     = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intGust) / 10.0f, (float(intGust)* 0.36f), (float(intGust) * 0.223693629f));
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype == sTypeWIND4)
		{
			sprintf(szTmp, "Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp, "Chill         = %.1f C", chill);
		}
		if (pResponse->WIND.subtype == sTypeWINDNoTemp)
		{
			sprintf(szTmp, "Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp, "Chill         = %.1f C", chill);
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->WIND.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->WIND.subtype == sTypeWIND3, pResponse->WIND.battery_level & 0x0F);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Temp(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeTEMP;
	unsigned char subType = pResponse->TEMP.subtype;
	sprintf(szTmp, "%d", (pResponse->TEMP.id1 * 256) + pResponse->TEMP.id2);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->TEMP.id2;

	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->TEMP.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->TEMP.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	//Override battery level if hardware supports it
	if (HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level * 10;
	}
	else if ((HwdType == HTYPE_EnOceanESP2) || (HwdType == HTYPE_EnOceanESP3))
	{
		BatteryLevel = 255;
		SignalLevel = 12;
		Unit = (pResponse->TEMP.rssi << 4) | pResponse->TEMP.battery_level;
	}

	float temp;
	if (!pResponse->TEMP.tempsign)
	{
		temp = float((pResponse->TEMP.temperatureh * 256) + pResponse->TEMP.temperaturel) / 10.0f;
	}
	else
	{
		temp = -(float(((pResponse->TEMP.temperatureh & 0x7F) * 256) + pResponse->TEMP.temperaturel) / 10.0f);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	sprintf(szTmp, "%.1f", temp);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	bool bHandledNotification = false;
	unsigned char humidity = 0;
	if (pResponse->TEMP.subtype == sTypeTEMP5)
	{
		//check if we already had a humidity for this device, if so, keep it!
		char szTmp[300];
		std::vector<std::vector<std::string> > result;

		result = m_sql.safe_query(
			"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HwdID, ID.c_str(), 1, pTypeHUM, sTypeHUM1);
		if (result.size() == 1)
		{
			m_sql.GetAddjustment(HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, AddjValue, AddjMulti);
			temp += AddjValue;
			humidity = atoi(result[0][0].c_str());
			unsigned char humidity_status = atoi(result[0][1].c_str());
			sprintf(szTmp, "%.1f;%d;%d", temp, humidity, humidity_status);
			DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, SignalLevel, BatteryLevel, 0, szTmp, procResult.DeviceName);
			m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, humidity, true, true);
			float dewpoint = (float)CalculateDewPoint(temp, humidity);
			m_notifications.CheckAndHandleDewPointNotification(DevRowIdx, procResult.DeviceName, temp, dewpoint);
			bHandledNotification = true;
		}
	}

	if (!bHandledNotification)
		m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, humidity, true, false);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->TEMP.subtype)
		{
		case sTypeTEMP1:
			WriteMessage("subtype       = TEMP1 - THR128/138, THC138");
			sprintf(szTmp, "                channel %d", pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP2:
			WriteMessage("subtype       = TEMP2 - THC238/268,THN132,THWR288,THRN122,THN122,AW129/131");
			sprintf(szTmp, "                channel %d", pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP3:
			WriteMessage("subtype       = TEMP3 - THWR800");
			break;
		case sTypeTEMP4:
			WriteMessage("subtype       = TEMP4 - RTHN318");
			sprintf(szTmp, "                channel %d", pResponse->TEMP.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTEMP5:
			WriteMessage("subtype       = TEMP5 - LaCrosse TX2, TX3, TX4, TX17");
			break;
		case sTypeTEMP6:
			WriteMessage("subtype       = TEMP6 - TS15C");
			break;
		case sTypeTEMP7:
			WriteMessage("subtype       = TEMP7 - Viking 02811, Proove TSS330");
			break;
		case sTypeTEMP8:
			WriteMessage("subtype       = TEMP8 - LaCrosse WS2300");
			break;
		case sTypeTEMP9:
			if (pResponse->TEMP.id2 & 0xFF)
				WriteMessage("subtype       = TEMP9 - RUBiCSON 48659 stektermometer");
			else
				WriteMessage("subtype       = TEMP9 - RUBiCSON 48695");
			break;
		case sTypeTEMP10:
			WriteMessage("subtype       = TEMP10 - TFA 30.3133");
			break;
		case sTypeTEMP11:
			WriteMessage("subtype       = WT0122 pool sensor");
			break;
		case sTypeTEMP_SYSTEM:
			WriteMessage("subtype       = System");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP.packettype, pResponse->TEMP.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->TEMP.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->TEMP.id1 * 256) + pResponse->TEMP.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Temperature   = %.1f C", temp);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Hum(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeHUM;
	unsigned char subType = pResponse->HUM.subtype;
	sprintf(szTmp, "%d", (pResponse->HUM.id1 * 256) + pResponse->HUM.id2);
	std::string ID = szTmp;
	unsigned char Unit = 1;

	unsigned char SignalLevel = pResponse->HUM.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->HUM.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;
	//Override battery level if hardware supports it
	if (HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level;
	}

	unsigned char humidity = pResponse->HUM.humidity;
	if (humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}

	sprintf(szTmp, "%d", pResponse->HUM.humidity_status);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, humidity, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	bool bHandledNotification = false;
	float temp = 0;
	if (pResponse->HUM.subtype == sTypeHUM1)
	{
		//check if we already had a humidity for this device, if so, keep it!
		char szTmp[300];
		std::vector<std::vector<std::string> > result;

		result = m_sql.safe_query(
			"SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HwdID, ID.c_str(), 0, pTypeTEMP, sTypeTEMP5);
		if (result.size() == 1)
		{
			temp = static_cast<float>(atof(result[0][0].c_str()));
			float AddjValue = 0.0f;
			float AddjMulti = 1.0f;
			m_sql.GetAddjustment(HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, AddjValue, AddjMulti);
			temp += AddjValue;
			sprintf(szTmp, "%.1f;%d;%d", temp, humidity, pResponse->HUM.humidity_status);
			DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, SignalLevel, BatteryLevel, 0, szTmp, procResult.DeviceName);
			m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, humidity, true, true);
			float dewpoint = (float)CalculateDewPoint(temp, humidity);
			m_notifications.CheckAndHandleDewPointNotification(DevRowIdx, procResult.DeviceName, temp, dewpoint);
			bHandledNotification = true;
		}
	}
	if (!bHandledNotification)
		m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, humidity, false, true);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->HUM.subtype)
		{
		case sTypeHUM1:
			WriteMessage("subtype       = HUM1 - LaCrosse TX3");
			break;
		case sTypeHUM2:
			WriteMessage("subtype       = HUM2 - LaCrosse WS2300");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->HUM.packettype, pResponse->HUM.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->HUM.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->HUM.id1 * 256) + pResponse->HUM.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Humidity      = %d %%", pResponse->HUM.humidity);
		WriteMessage(szTmp);

		switch (pResponse->HUM.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->HUM.rssi);
		WriteMessage(szTmp);

		if ((pResponse->HUM.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_TempHum(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeTEMP_HUM;
	unsigned char subType = pResponse->TEMP_HUM.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->TEMP_HUM.id1 * 256) + pResponse->TEMP_HUM.id2);
	ID = szTmp;
	unsigned char Unit = 0;

	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->TEMP_HUM.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, pResponse->TEMP_HUM.subtype == sTypeTH8, pResponse->TEMP_HUM.battery_level);

	//Get Channel(Unit)
	switch (pResponse->TEMP_HUM.subtype)
	{
	case sTypeTH1:
	case sTypeTH2:
	case sTypeTH3:
	case sTypeTH4:
	case sTypeTH6:
	case sTypeTH8:
	case sTypeTH10:
	case sTypeTH11:
	case sTypeTH12:
	case sTypeTH13:
	case sTypeTH14:
		Unit = pResponse->TEMP_HUM.id2;
		break;
	case sTypeTH5:
	case sTypeTH9:
		//no channel
		break;
	case sTypeTH7:
		if (pResponse->TEMP_HUM.id1 < 0x40)
			Unit = 1;
		else if (pResponse->TEMP_HUM.id1 < 0x60)
			Unit = 2;
		else if (pResponse->TEMP_HUM.id1 < 0x80)
			Unit = 3;
		else if ((pResponse->TEMP_HUM.id1 > 0x9F) && (pResponse->TEMP_HUM.id1 < 0xC0))
			Unit = 4;
		else if (pResponse->TEMP_HUM.id1 < 0xE0)
			Unit = 5;
		break;
	}

	float temp;
	if (!pResponse->TEMP_HUM.tempsign)
	{
		temp = float((pResponse->TEMP_HUM.temperatureh * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_HUM.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0f);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	int Humidity = (int)pResponse->TEMP_HUM.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM.humidity_status;

	if (Humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}
	/*
		AddjValue=0.0f;
		AddjMulti=1.0f;
		m_sql.GetAddjustment2(HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
		Humidity+=int(AddjValue);
		if (Humidity>100)
			Humidity=100;
		if (Humidity<0)
			Humidity=0;
	*/
	sprintf(szTmp, "%.1f;%d;%d", temp, Humidity, HumidityStatus);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, Humidity, true, true);

	float dewpoint = (float)CalculateDewPoint(temp, Humidity);
	m_notifications.CheckAndHandleDewPointNotification(DevRowIdx, procResult.DeviceName, temp, dewpoint);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->TEMP_HUM.subtype)
		{
		case sTypeTH1:
			WriteMessage("subtype       = TH1 - THGN122/123/132,THGR122/228/238/268");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH2:
			WriteMessage("subtype       = TH2 - THGR810,THGN800");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH3:
			WriteMessage("subtype       = TH3 - RTGR328");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH4:
			WriteMessage("subtype       = TH4 - THGR328");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH5:
			WriteMessage("subtype       = TH5 - WTGR800");
			break;
		case sTypeTH6:
			WriteMessage("subtype       = TH6 - THGR918/928,THGRN228,THGN500");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH7:
			WriteMessage("subtype       = TH7 - Cresta, TFA TS34C");
			if (pResponse->TEMP_HUM.id1 < 0x40)
				WriteMessage("                channel 1");
			else if (pResponse->TEMP_HUM.id1 < 0x60)
				WriteMessage("                channel 2");
			else if (pResponse->TEMP_HUM.id1 < 0x80)
				WriteMessage("                channel 3");
			else if ((pResponse->TEMP_HUM.id1 > 0x9F) && (pResponse->TEMP_HUM.id1 < 0xC0))
				WriteMessage("                channel 4");
			else if (pResponse->TEMP_HUM.id1 < 0xE0)
				WriteMessage("                channel 5");
			else
				WriteMessage("                channel ??");
			break;
		case sTypeTH8:
			WriteMessage("subtype       = TH8 - WT260,WT260H,WT440H,WT450,WT450H");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH9:
			WriteMessage("subtype       = TH9 - Viking 02038, 02035 (02035 has no humidity), TSS320");
			break;
		case sTypeTH10:
			WriteMessage("subtype       = TH10 - Rubicson/IW008T/TX95");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH11:
			WriteMessage("subtype       = TH11 - Oregon EW109");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH12:
			WriteMessage("subtype       = TH12 - Imagintronix/Opus TX300");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH13:
			WriteMessage("subtype       = TH13 - Alecto WS1700 and compatibles");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTH14:
			WriteMessage("subtype       = TH14 - Alecto");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM.id2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP_HUM.packettype, pResponse->TEMP_HUM.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->TEMP_HUM.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		double tvalue = ConvertTemperature(temp, m_sql.m_tempsign[0]);
		sprintf(szTmp, "Temperature   = %.1f C", tvalue);
		WriteMessage(szTmp);
		sprintf(szTmp, "Humidity      = %d %%", Humidity);
		WriteMessage(szTmp);

		switch (pResponse->TEMP_HUM.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP_HUM.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->TEMP_HUM.subtype == sTypeTH8, pResponse->TEMP_HUM.battery_level);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_TempHumBaro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeTEMP_HUM_BARO;
	unsigned char subType = pResponse->TEMP_HUM_BARO.subtype;
	sprintf(szTmp, "%d", (pResponse->TEMP_HUM_BARO.id1 * 256) + pResponse->TEMP_HUM_BARO.id2);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->TEMP_HUM_BARO.id2;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->TEMP_HUM_BARO.rssi;
	unsigned char BatteryLevel;
	if ((pResponse->TEMP_HUM_BARO.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;
	//Override battery level if hardware supports it
	if (HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level;
	}

	float temp;
	if (!pResponse->TEMP_HUM_BARO.tempsign)
	{
		temp = float((pResponse->TEMP_HUM_BARO.temperatureh * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_HUM_BARO.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0f);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	unsigned char Humidity = pResponse->TEMP_HUM_BARO.humidity;
	unsigned char HumidityStatus = pResponse->TEMP_HUM_BARO.humidity_status;

	if (Humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}

	int barometer = (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol;

	int forcast = pResponse->TEMP_HUM_BARO.forecast;
	float fbarometer = (float)barometer;

	m_sql.GetAddjustment2(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	barometer += int(AddjValue);

	if (pResponse->TEMP_HUM_BARO.subtype == sTypeTHBFloat)
	{
		if ((barometer < 8000) || (barometer > 12000))
		{
			WriteMessage(" Invalid Barometer");
			return;
		}
		fbarometer = float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol) / 10.0f;
		fbarometer += AddjValue;
		sprintf(szTmp, "%.1f;%d;%d;%.1f;%d", temp, Humidity, HumidityStatus, fbarometer, forcast);
	}
	else
	{
		if ((barometer < 800) || (barometer > 1200))
		{
			WriteMessage(" Invalid Barometer");
			return;
		}
		sprintf(szTmp, "%.1f;%d;%d;%d;%d", temp, Humidity, HumidityStatus, barometer, forcast);
	}
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	//calculate Altitude
	//float seaLevelPressure=101325.0f;
	//float altitude = 44330.0f * (1.0f - pow(fbarometer / seaLevelPressure, 0.1903f));

	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, Humidity, true, true);

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_BARO, fbarometer);

	float dewpoint = (float)CalculateDewPoint(temp, Humidity);
	m_notifications.CheckAndHandleDewPointNotification(DevRowIdx, procResult.DeviceName, temp, dewpoint);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->TEMP_HUM_BARO.subtype)
		{
		case sTypeTHB1:
			WriteMessage("subtype       = THB1 - BTHR918, BTHGN129");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM_BARO.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTHB2:
			WriteMessage("subtype       = THB2 - BTHR918N, BTHR968");
			sprintf(szTmp, "                channel %d", pResponse->TEMP_HUM_BARO.id2);
			WriteMessage(szTmp);
			break;
		case sTypeTHBFloat:
			WriteMessage("subtype       = Weather Station");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->TEMP_HUM_BARO.packettype, pResponse->TEMP_HUM_BARO.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->TEMP_HUM_BARO.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = %d", (pResponse->TEMP_HUM_BARO.id1 * 256) + pResponse->TEMP_HUM_BARO.id2);
		WriteMessage(szTmp);

		double tvalue = ConvertTemperature(temp, m_sql.m_tempsign[0]);
		sprintf(szTmp, "Temperature   = %.1f C", tvalue);
		WriteMessage(szTmp);

		sprintf(szTmp, "Humidity      = %d %%", pResponse->TEMP_HUM_BARO.humidity);
		WriteMessage(szTmp);

		switch (pResponse->TEMP_HUM_BARO.humidity_status)
		{
		case humstat_normal:
			WriteMessage("Status        = Normal");
			break;
		case humstat_comfort:
			WriteMessage("Status        = Comfortable");
			break;
		case humstat_dry:
			WriteMessage("Status        = Dry");
			break;
		case humstat_wet:
			WriteMessage("Status        = Wet");
			break;
		}

		if (pResponse->TEMP_HUM_BARO.subtype == sTypeTHBFloat)
			sprintf(szTmp, "Barometer     = %.1f hPa", float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol) / 10.0f);
		else
			sprintf(szTmp, "Barometer     = %d hPa", (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol);
		WriteMessage(szTmp);

		switch (pResponse->TEMP_HUM_BARO.forecast)
		{
		case baroForecastNoInfo:
			WriteMessage("Forecast      = No information available");
			break;
		case baroForecastSunny:
			WriteMessage("Forecast      = Sunny");
			break;
		case baroForecastPartlyCloudy:
			WriteMessage("Forecast      = Partly Cloudy");
			break;
		case baroForecastCloudy:
			WriteMessage("Forecast      = Cloudy");
			break;
		case baroForecastRain:
			WriteMessage("Forecast      = Rain");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP_HUM_BARO.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP_HUM_BARO.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_TempBaro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeTEMP_BARO;
	unsigned char subType = sTypeBMP085;
	_tTempBaro *pTempBaro = (_tTempBaro*)pResponse;

	sprintf(szTmp, "%d", pTempBaro->id1);
	std::string ID = szTmp;
	unsigned char Unit = 1;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel;
	BatteryLevel = 100;

	float temp = pTempBaro->temp;
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	float fbarometer = pTempBaro->baro;
	int forcast = pTempBaro->forecast;
	m_sql.GetAddjustment2(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	fbarometer += AddjValue;

	sprintf(szTmp, "%.1f;%.1f;%d;%.2f", temp, fbarometer, forcast, pTempBaro->altitude);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, 0, true, false);
	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_BARO, fbarometer);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->TEMP_HUM_BARO.subtype)
		{
		case sTypeBMP085:
			WriteMessage("subtype       = BMP085 I2C");
			sprintf(szTmp, "                channel %d", pTempBaro->id1);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", devType, subType);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Temperature   = %.1f C", temp);
		WriteMessage(szTmp);

		sprintf(szTmp, "Barometer     = %.1f hPa", fbarometer);
		WriteMessage(szTmp);

		switch (pTempBaro->forecast)
		{
		case baroForecastNoInfo:
			WriteMessage("Forecast      = No information available");
			break;
		case baroForecastSunny:
			WriteMessage("Forecast      = Sunny");
			break;
		case baroForecastPartlyCloudy:
			WriteMessage("Forecast      = Partly Cloudy");
			break;
		case baroForecastCloudy:
			WriteMessage("Forecast      = Cloudy");
			break;
		case baroForecastRain:
			WriteMessage("Forecast      = Rain");
			break;
		}

		if (pResponse->TEMP_HUM_BARO.subtype == sTypeBMP085)
		{
			sprintf(szTmp, "Altitude   = %.2f meter", pTempBaro->altitude);
			WriteMessage(szTmp);
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_TempRain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];

	//We are (also) going to split this device into two separate sensors (temp + rain)

	unsigned char devType = pTypeTEMP_RAIN;
	unsigned char subType = pResponse->TEMP_RAIN.subtype;

	sprintf(szTmp, "%d", (pResponse->TEMP_RAIN.id1 * 256) + pResponse->TEMP_RAIN.id2);
	std::string ID = szTmp;
	int Unit = pResponse->TEMP_RAIN.id2;
	int cmnd = 0;

	unsigned char SignalLevel = pResponse->TEMP_RAIN.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->TEMP_RAIN.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	float temp;
	if (!pResponse->TEMP_RAIN.tempsign)
	{
		temp = float((pResponse->TEMP_RAIN.temperatureh * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0f;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_RAIN.temperatureh & 0x7F) * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0f);
	}

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, pTypeTEMP, sTypeTEMP3, AddjValue, AddjMulti);
	temp += AddjValue;

	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}
	float TotalRain = float((pResponse->TEMP_RAIN.raintotal1 * 256) + pResponse->TEMP_RAIN.raintotal2) / 10.0f;

	sprintf(szTmp, "%.1f;%.1f", temp, TotalRain);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	sprintf(szTmp, "%.1f", temp);
	uint64_t DevRowIdxTemp = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, pTypeTEMP, sTypeTEMP3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, 0, true, false);

	sprintf(szTmp, "%d;%.1f", 0, TotalRain);
	uint64_t DevRowIdxRain = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, pTypeRAIN, sTypeRAIN3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	m_notifications.CheckAndHandleRainNotification(DevRowIdx, procResult.DeviceName, pTypeRAIN, sTypeRAIN3, NTYPE_RAIN, TotalRain);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->TEMP_RAIN.subtype)
		{
		case sTypeTR1:
			WriteMessage("Subtype       = Alecto WS1200");
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->TEMP_RAIN.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->TEMP_RAIN.id1 * 256) + pResponse->TEMP_RAIN.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Temperature   = %.1f C", temp);
		WriteMessage(szTmp);
		sprintf(szTmp, "Total Rain    = %.1f mm", TotalRain);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->TEMP_RAIN.rssi);
		WriteMessage(szTmp);

		if ((pResponse->TEMP_RAIN.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}

	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_UV(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeUV;
	unsigned char subType = pResponse->UV.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->UV.id1 * 256) + pResponse->UV.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->UV.rssi;
	unsigned char BatteryLevel;
	if ((pResponse->UV.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;
	float Level = float(pResponse->UV.uv) / 10.0f;
	if (Level > 30)
	{
		WriteMessage(" Invalid UV");
		return;
	}
	float temp = 0;
	if (pResponse->UV.subtype == sTypeUV3)
	{
		if (!pResponse->UV.tempsign)
		{
			temp = float((pResponse->UV.temperatureh * 256) + pResponse->UV.temperaturel) / 10.0f;
		}
		else
		{
			temp = -(float(((pResponse->UV.temperatureh & 0x7F) * 256) + pResponse->UV.temperaturel) / 10.0f);
		}
		if ((temp < -200) || (temp > 380))
		{
			WriteMessage(" Invalid Temperature");
			return;
		}

		float AddjValue = 0.0f;
		float AddjMulti = 1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
	}

	sprintf(szTmp, "%.1f;%.1f", Level, temp);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, 0, true, false);

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_UV, Level);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->UV.subtype)
		{
		case sTypeUV1:
			WriteMessage("Subtype       = UV1 - UVN128, UV138");
			break;
		case sTypeUV2:
			WriteMessage("Subtype       = UV2 - UVN800");
			break;
		case sTypeUV3:
			WriteMessage("Subtype       = UV3 - TFA");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->UV.packettype, pResponse->UV.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->UV.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->UV.id1 * 256) + pResponse->UV.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Level         = %.1f UVI", Level);
		WriteMessage(szTmp);

		if (pResponse->UV.subtype == sTypeUV3)
		{
			sprintf(szTmp, "Temperature   = %.1f C", temp);
			WriteMessage(szTmp);
		}

		if (pResponse->UV.uv < 3)
			WriteMessage("Description = Low");
		else if (pResponse->UV.uv < 6)
			WriteMessage("Description = Medium");
		else if (pResponse->UV.uv < 8)
			WriteMessage("Description = High");
		else if (pResponse->UV.uv < 11)
			WriteMessage("Description = Very high");
		else
			WriteMessage("Description = Dangerous");

		sprintf(szTmp, "Signal level  = %d", pResponse->UV.rssi);
		WriteMessage(szTmp);

		if ((pResponse->UV.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lighting1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeLighting1;
	unsigned char subType = pResponse->LIGHTING1.subtype;
	sprintf(szTmp, "%d", pResponse->LIGHTING1.housecode);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->LIGHTING1.unitcode;
	unsigned char cmnd = pResponse->LIGHTING1.cmnd;
	unsigned char SignalLevel = pResponse->LIGHTING1.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->LIGHTING1.subtype)
		{
		case sTypeX10:
			WriteMessage("subtype       = X10");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sDim:
				WriteMessage("Dim");
				break;
			case light1_sBright:
				WriteMessage("Bright");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeARC:
			WriteMessage("subtype       = ARC");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			case light1_sChime:
				WriteMessage("Chime");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeAB400D:
		case sTypeWaveman:
		case sTypeEMW200:
		case sTypeIMPULS:
		case sTypeRisingSun:
		case sTypeEnergenie:
		case sTypeEnergenie5:
		case sTypeGDR2:
		case sTypeHQ:
			//decoding of these types is only implemented for use by simulate and verbose
			//these types are not received by the RFXtrx433
			switch (pResponse->LIGHTING1.subtype)
			{
			case sTypeAB400D:
				WriteMessage("subtype       = ELRO AB400");
				break;
			case sTypeWaveman:
				WriteMessage("subtype       = Waveman");
				break;
			case sTypeEMW200:
				WriteMessage("subtype       = EMW200");
				break;
			case sTypeIMPULS:
				WriteMessage("subtype       = IMPULS");
				break;
			case sTypeRisingSun:
				WriteMessage("subtype       = RisingSun");
				break;
			case sTypeEnergenie:
				WriteMessage("subtype       = Energenie-ENER010");
				break;
			case sTypeEnergenie5:
				WriteMessage("subtype       = Energenie 5-gang");
				break;
			case sTypeGDR2:
				WriteMessage("subtype       = COCO GDR2");
				break;
			case sTypeHQ:
				WriteMessage("subtype       = HQ COCO-20");
				break;
			}
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);

			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypePhilips:
			//decoding of this type is only implemented for use by simulate and verbose
			//this type is not received by the RFXtrx433
			WriteMessage("subtype       = Philips SBC");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING1.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "housecode     = %c", pResponse->LIGHTING1.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->LIGHTING1.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);

			switch (pResponse->LIGHTING1.cmnd)
			{
			case light1_sOff:
				WriteMessage("Off");
				break;
			case light1_sOn:
				WriteMessage("On");
				break;
			case light1_sAllOff:
				WriteMessage("All Off");
				break;
			case light1_sAllOn:
				WriteMessage("All On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING1.packettype, pResponse->LIGHTING1.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING1.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lighting2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeLighting2;
	unsigned char subType = pResponse->LIGHTING2.subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->LIGHTING2.unitcode;
	unsigned char cmnd = pResponse->LIGHTING2.cmnd;
	unsigned char level = pResponse->LIGHTING2.level;
	unsigned char SignalLevel = pResponse->LIGHTING2.rssi;

	sprintf(szTmp, "%d", level);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);

	bool isGroupCommand = ((cmnd == light2_sGroupOff) || (cmnd == light2_sGroupOn));
	unsigned char single_cmnd = cmnd;

	if (isGroupCommand)
	{
		single_cmnd = (cmnd == light2_sGroupOff) ? light2_sOff : light2_sOn;

		// We write the GROUP_CMD into the log to differentiate between manual turn off/on and group_off/group_on
		m_sql.UpdateValueLighting2GroupCmd(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);

		//set the status of all lights with the same code to on/off
		m_sql.Lighting2GroupCmd(ID, subType, single_cmnd);
	}

	if (DevRowIdx == -1) {
		// not found nothing to do
		return;
	}
	CheckSceneCode(DevRowIdx, devType, subType, single_cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->LIGHTING2.subtype)
		{
		case sTypeAC:
		case sTypeHEU:
		case sTypeANSLUT:
			switch (pResponse->LIGHTING2.subtype)
			{
			case sTypeAC:
				WriteMessage("subtype       = AC");
				break;
			case sTypeHEU:
				WriteMessage("subtype       = HomeEasy EU");
				break;
			case sTypeANSLUT:
				WriteMessage("subtype       = ANSLUT");
				break;
			}
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING2.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %s", ID.c_str());
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", Unit);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING2.cmnd)
			{
			case light2_sOff:
				WriteMessage("Off");
				break;
			case light2_sOn:
				WriteMessage("On");
				break;
			case light2_sSetLevel:
				sprintf(szTmp, "Set Level: %d", level);
				WriteMessage(szTmp);
				break;
			case light2_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light2_sGroupOn:
				WriteMessage("Group On");
				break;
			case light2_sSetGroupLevel:
				sprintf(szTmp, "Set Group Level: %d", level);
				WriteMessage(szTmp);
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING2.packettype, pResponse->LIGHTING2.subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

//not in dbase yet
void MainWorker::decode_Lighting3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessageStart();
	WriteMessage("");

	char szTmp[100];

	switch (pResponse->LIGHTING3.subtype)
	{
	case sTypeKoppla:
		WriteMessage("subtype       = Ikea Koppla");
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING3.seqnbr);
		WriteMessage(szTmp);
		WriteMessage("Command       = ", false);
		switch (pResponse->LIGHTING3.cmnd)
		{
		case 0x0:
			WriteMessage("Off");
			break;
		case 0x1:
			WriteMessage("On");
			break;
		case 0x20:
			sprintf(szTmp, "Set Level: %d", pResponse->LIGHTING3.channel10_9);
			WriteMessage(szTmp);
			break;
		case 0x21:
			WriteMessage("Program");
			break;
		default:
			if ((pResponse->LIGHTING3.cmnd >= 0x10) && (pResponse->LIGHTING3.cmnd < 0x18))
				WriteMessage("Dim");
			else if ((pResponse->LIGHTING3.cmnd >= 0x18) && (pResponse->LIGHTING3.cmnd < 0x20))
				WriteMessage("Bright");
			else
				WriteMessage("UNKNOWN");
			break;
		}
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING3.packettype, pResponse->LIGHTING3.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING3.rssi);
	WriteMessage(szTmp);
	WriteMessageEnd();
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Lighting4(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeLighting4;
	unsigned char subType = pResponse->LIGHTING4.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING4.cmd1, pResponse->LIGHTING4.cmd2, pResponse->LIGHTING4.cmd3);
	std::string ID = szTmp;
	int Unit = 0;
	unsigned char cmnd = 1; //only 'On' supported
	unsigned char SignalLevel = pResponse->LIGHTING4.rssi;
	sprintf(szTmp, "%d", (pResponse->LIGHTING4.pulseHigh * 256) + pResponse->LIGHTING4.pulseLow);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->LIGHTING4.subtype)
		{
		case sTypePT2262:
			WriteMessage("subtype       = PT2262");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING4.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "Code          = %02X%02X%02X", pResponse->LIGHTING4.cmd1, pResponse->LIGHTING4.cmd2, pResponse->LIGHTING4.cmd3);
			WriteMessage(szTmp);

			WriteMessage("S1- S24  = ", false);
			if ((pResponse->LIGHTING4.cmd1 & 0x80) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x40) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x20) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x10) == 0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);


			if ((pResponse->LIGHTING4.cmd1 & 0x08) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x04) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x02) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd1 & 0x01) == 0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);


			if ((pResponse->LIGHTING4.cmd2 & 0x80) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x40) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x20) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x10) == 0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);


			if ((pResponse->LIGHTING4.cmd2 & 0x08) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x04) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x02) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd2 & 0x01) == 0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);


			if ((pResponse->LIGHTING4.cmd3 & 0x80) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x40) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x20) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x10) == 0)
				WriteMessage("0 ", false);
			else
				WriteMessage("1 ", false);


			if ((pResponse->LIGHTING4.cmd3 & 0x08) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x04) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x02) == 0)
				WriteMessage("0", false);
			else
				WriteMessage("1", false);

			if ((pResponse->LIGHTING4.cmd3 & 0x01) == 0)
				WriteMessage("0");
			else
				WriteMessage("1");

			sprintf(szTmp, "Pulse         = %d usec", (pResponse->LIGHTING4.pulseHigh * 256) + pResponse->LIGHTING4.pulseLow);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING4.packettype, pResponse->LIGHTING4.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING4.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lighting5(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeLighting5;
	unsigned char subType = pResponse->LIGHTING5.subtype;
	if ((subType != sTypeEMW100) && (subType != sTypeLivolo) && (subType != sTypeLivoloAppliance) && (subType != sTypeRGB432W) && (subType != sTypeKangtai))
		sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	else
		sprintf(szTmp, "%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->LIGHTING5.unitcode;
	unsigned char cmnd = pResponse->LIGHTING5.cmnd;
	float flevel;
	if (subType == sTypeLivolo)
		flevel = (100.0f / 7.0f)*float(pResponse->LIGHTING5.level);
	else if (subType == sTypeLightwaveRF)
		flevel = (100.0f / 31.0f)*float(pResponse->LIGHTING5.level);
	else
		flevel = (100.0f / 7.0f)*float(pResponse->LIGHTING5.level);
	unsigned char SignalLevel = pResponse->LIGHTING5.rssi;

	bool bDoUpdate = true;
	if ((subType == sTypeTRC02) || (subType == sTypeTRC02_2) || (subType == sTypeAoke) || (subType == sTypeEurodomest))
	{
		if (
			(pResponse->LIGHTING5.cmnd != light5_sOff) &&
			(pResponse->LIGHTING5.cmnd != light5_sOn) &&
			(pResponse->LIGHTING5.cmnd != light5_sGroupOff) &&
			(pResponse->LIGHTING5.cmnd != light5_sGroupOn)
			)
		{
			bDoUpdate = false;
		}
	}
	uint64_t DevRowIdx = -1;
	if (bDoUpdate)
	{
		sprintf(szTmp, "%d", pResponse->LIGHTING5.level);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);
	}

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->LIGHTING5.subtype)
		{
		case sTypeLightwaveRF:
			WriteMessage("subtype       = LightwaveRF");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sMood1:
				WriteMessage("Group Mood 1");
				break;
			case light5_sMood2:
				WriteMessage("Group Mood 2");
				break;
			case light5_sMood3:
				WriteMessage("Group Mood 3");
				break;
			case light5_sMood4:
				WriteMessage("Group Mood 4");
				break;
			case light5_sMood5:
				WriteMessage("Group Mood 5");
				break;
			case light5_sUnlock:
				WriteMessage("Unlock");
				break;
			case light5_sLock:
				WriteMessage("Lock");
				break;
			case light5_sAllLock:
				WriteMessage("All lock");
				break;
			case light5_sClose:
				WriteMessage("Close inline relay");
				break;
			case light5_sStop:
				WriteMessage("Stop inline relay");
				break;
			case light5_sOpen:
				WriteMessage("Open inline relay");
				break;
			case light5_sSetLevel:
				sprintf(szTmp, "Set dim level to: %.2f %%", flevel);
				WriteMessage(szTmp);
				break;
			case light5_sColourPalette:
				if (pResponse->LIGHTING5.level == 0)
					WriteMessage("Color Palette (Even command)");
				else
					WriteMessage("Color Palette (Odd command)");
				break;
			case light5_sColourTone:
				if (pResponse->LIGHTING5.level == 0)
					WriteMessage("Color Tone (Even command)");
				else
					WriteMessage("Color Tone (Odd command)");
				break;
			case light5_sColourCycle:
				if (pResponse->LIGHTING5.level == 0)
					WriteMessage("Color Cycle (Even command)");
				else
					WriteMessage("Color Cycle (Odd command)");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeEMW100:
			WriteMessage("subtype       = EMW100");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sLearn:
				WriteMessage("Learn");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeBBSB:
			WriteMessage("subtype       = BBSB new");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeRSL:
			WriteMessage("subtype       = Conrad RSL");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeLivolo:
			WriteMessage("subtype       = Livolo Dimmer");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeLivoloAppliance:
			WriteMessage("subtype       = Livolo On/Off module");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sLivoloAllOff:
				WriteMessage("Off");
				break;
			case light5_sLivoloGang1Toggle:
				WriteMessage("Toggle");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeRGB432W:
			WriteMessage("subtype       = RGB432W");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sRGBoff:
				WriteMessage("Off");
				break;
			case light5_sRGBon:
				WriteMessage("On");
				break;
			case light5_sRGBbright:
				WriteMessage("Bright+");
				break;
			case light5_sRGBdim:
				WriteMessage("Bright-");
				break;
			case light5_sRGBcolorplus:
				WriteMessage("Color+");
				break;
			case light5_sRGBcolormin:
				WriteMessage("Color-");
				break;
			default:
				sprintf(szTmp, "Color =          = %d", pResponse->LIGHTING5.cmnd);
				WriteMessage(szTmp);
				break;
			}
			break;
		case sTypeTRC02:
		case sTypeTRC02_2:
			if (pResponse->LIGHTING5.subtype == sTypeTRC02)
				WriteMessage("subtype       = TRC02 (RGB)");
			else
				WriteMessage("subtype       = TRC02_2 (RGB)");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sRGBbright:
				WriteMessage("Bright+");
				break;
			case light5_sRGBdim:
				WriteMessage("Bright-");
				break;
			case light5_sRGBcolorplus:
				WriteMessage("Color+");
				break;
			case light5_sRGBcolormin:
				WriteMessage("Color-");
				break;
			default:
				sprintf(szTmp, "Color = %d", pResponse->LIGHTING5.cmnd);
				WriteMessage(szTmp);
				break;
			}
			break;
		case sTypeAoke:
			WriteMessage("subtype       = Aoke");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeEurodomest:
			WriteMessage("subtype       = Eurodomest");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeLegrandCAD:
			WriteMessage("subtype       = Legrand CAD");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			WriteMessage("Command       = Toggle");
			break;
		case sTypeAvantek:
			WriteMessage("subtype       = Avantek");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("Unknown");
				break;
			}
			break;
		case sTypeIT:
			WriteMessage("subtype       = Intertek,FA500,PROmax");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			case light5_sSetLevel:
				sprintf(szTmp, "Set dim level to: %.2f %%", flevel);
				WriteMessage(szTmp);
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeKangtai:
			WriteMessage("subtype       = Kangtai / Cotech");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING5.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "Unit          = %d", pResponse->LIGHTING5.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING5.cmnd)
			{
			case light5_sOff:
				WriteMessage("Off");
				break;
			case light5_sOn:
				WriteMessage("On");
				break;
			case light5_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light5_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING5.packettype, pResponse->LIGHTING5.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING5.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lighting6(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeLighting6;
	unsigned char subType = pResponse->LIGHTING6.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2, pResponse->LIGHTING6.groupcode);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->LIGHTING6.unitcode;
	unsigned char cmnd = pResponse->LIGHTING6.cmnd;
	unsigned char rfu = pResponse->LIGHTING6.seqnbr2;
	unsigned char SignalLevel = pResponse->LIGHTING6.rssi;

	sprintf(szTmp, "%d", rfu);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->LIGHTING6.subtype)
		{
		case sTypeBlyss:
			WriteMessage("subtype       = Blyss");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->LIGHTING6.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2);
			WriteMessage(szTmp);
			sprintf(szTmp, "groupcode     = %d", pResponse->LIGHTING6.groupcode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->LIGHTING6.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->LIGHTING6.cmnd)
			{
			case light6_sOff:
				WriteMessage("Off");
				break;
			case light6_sOn:
				WriteMessage("On");
				break;
			case light6_sGroupOff:
				WriteMessage("Group Off");
				break;
			case light6_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			sprintf(szTmp, "Command seqnbr= %d", pResponse->LIGHTING6.cmndseqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "seqnbr2       = %d", pResponse->LIGHTING6.seqnbr2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING6.packettype, pResponse->LIGHTING6.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING6.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Fan(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeFan;
	unsigned char subType = pResponse->FAN.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->FAN.id1, pResponse->FAN.id2, pResponse->FAN.id3);
	std::string ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = pResponse->FAN.cmnd;
	unsigned char SignalLevel = pResponse->FAN.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->FAN.subtype)
		{
		case sTypeSiemensSF01:
			WriteMessage("subtype       = Siemens SF01");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->FAN.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->FAN.id2, pResponse->FAN.id3);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->FAN.cmnd)
			{
			case fan_sTimer:
				WriteMessage("Timer");
				break;
			case fan_sMin:
				WriteMessage("-");
				break;
			case fan_sLearn:
				WriteMessage("Learn");
				break;
			case fan_sPlus:
				WriteMessage("+");
				break;
			case fan_sConfirm:
				WriteMessage("Confirm");
				break;
			case fan_sLight:
				WriteMessage("Light");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeItho:
			WriteMessage("subtype       = Itho CVE RFT");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->FAN.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->FAN.id1, pResponse->FAN.id2, pResponse->FAN.id3);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->FAN.cmnd)
			{
			case fan_Itho1:
				WriteMessage("1");
				break;
			case fan_Itho2:
				WriteMessage("2");
				break;
			case fan_Itho3:
				WriteMessage("3");
				break;
			case fan_IthoTimer:
				WriteMessage("Timer");
				break;
			case fan_IthoNotAtHome:
				WriteMessage("Not at Home");
				break;
			case fan_IthoLearn:
				WriteMessage("Learn");
				break;
			case fan_IthoEraseAll:
				WriteMessage("Erase all remotes");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		case sTypeLucciAir:
			WriteMessage("subtype       = Lucci Air");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->LIGHTING6.packettype, pResponse->LIGHTING6.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->LIGHTING6.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_HomeConfort(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeHomeConfort;
	unsigned char subType = pResponse->HOMECONFORT.subtype;
	sprintf(szTmp, "%02X%02X%02X%02X", pResponse->HOMECONFORT.id1, pResponse->HOMECONFORT.id2, pResponse->HOMECONFORT.id3, pResponse->HOMECONFORT.housecode);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->HOMECONFORT.unitcode;
	unsigned char cmnd = pResponse->HOMECONFORT.cmnd;
	unsigned char SignalLevel = pResponse->HOMECONFORT.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);

	bool isGroupCommand = ((cmnd == HomeConfort_sGroupOff) || (cmnd == HomeConfort_sGroupOn));
	unsigned char single_cmnd = cmnd;

	if (isGroupCommand)
	{
		single_cmnd = (cmnd == HomeConfort_sGroupOff) ? HomeConfort_sOff : HomeConfort_sOn;

		// We write the GROUP_CMD into the log to differentiate between manual turn off/on and group_off/group_on
		m_sql.UpdateValueHomeConfortGroupCmd(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);

		//set the status of all lights with the same code to on/off
		m_sql.HomeConfortGroupCmd(ID, subType, single_cmnd);
	}

	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->HOMECONFORT.subtype)
		{
		case sTypeHomeConfortTEL010:
			WriteMessage("subtype       = TEL-010");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->HOMECONFORT.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->HOMECONFORT.id1, pResponse->HOMECONFORT.id2, pResponse->HOMECONFORT.id3);
			WriteMessage(szTmp);
			sprintf(szTmp, "housecode     = %d", pResponse->HOMECONFORT.housecode);
			WriteMessage(szTmp);
			sprintf(szTmp, "unitcode      = %d", pResponse->HOMECONFORT.unitcode);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->HOMECONFORT.cmnd)
			{
			case HomeConfort_sOff:
				WriteMessage("Off");
				break;
			case HomeConfort_sOn:
				WriteMessage("On");
				break;
			case HomeConfort_sGroupOff:
				WriteMessage("Group Off");
				break;
			case HomeConfort_sGroupOn:
				WriteMessage("Group On");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->HOMECONFORT.packettype, pResponse->HOMECONFORT.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->HOMECONFORT.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_LimitlessLights(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[300];
	_tLimitlessLights *pLed = (_tLimitlessLights*)pResponse;

	unsigned char devType = pTypeLimitlessLights;
	unsigned char subType = pLed->subtype;
	if (pLed->id == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)pLed->id);
	std::string ID = szTmp;
	unsigned char Unit = pLed->dunit;
	unsigned char cmnd = pLed->command;
	unsigned char value = pLed->value;

	char szValueTmp[100];
	sprintf(szValueTmp, "%d", value);
	std::string sValue = szValueTmp;
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, 12, -1, cmnd, sValue.c_str(), procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (cmnd == Limitless_SetBrightnessLevel)
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT ID,Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HwdID, ID.c_str(), Unit, devType, subType);
		if (result.size() != 0)
		{
			uint64_t ulID;
			std::stringstream s_str(result[0][0]);
			s_str >> ulID;

			//store light level
			m_sql.safe_query(
				"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %" PRIu64 ")",
				value,
				ulID);
		}

	}

	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Chime(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeChime;
	unsigned char subType = pResponse->CHIME.subtype;
	sprintf(szTmp, "%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->CHIME.sound;
	unsigned char cmnd = pResponse->CHIME.sound;
	unsigned char SignalLevel = pResponse->CHIME.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->CHIME.subtype)
		{
		case sTypeByronSX:
			WriteMessage("subtype       = Byron SX");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			WriteMessage("Sound          = ", false);
			switch (pResponse->CHIME.sound)
			{
			case chime_sound0:
			case chime_sound4:
				WriteMessage("Tubular 3 notes");
				break;
			case chime_sound1:
			case chime_sound5:
				WriteMessage("Big Ben");
				break;
			case chime_sound2:
			case chime_sound6:
				WriteMessage("Tubular 3 notes");
				break;
			case chime_sound3:
			case chime_sound7:
				WriteMessage("Solo");
				break;
			default:
				WriteMessage("UNKNOWN?");
				break;
			}
			break;
		case sTypeByronMP001:
			WriteMessage("subtype       = Byron MP001");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			sprintf(szTmp, "Sound          = %d", pResponse->CHIME.sound);
			WriteMessage(szTmp);

			if ((pResponse->CHIME.id1 & 0x40) == 0x40)
				WriteMessage("Switch 1      = Off");
			else
				WriteMessage("Switch 1      = On");

			if ((pResponse->CHIME.id1 & 0x10) == 0x10)
				WriteMessage("Switch 2      = Off");
			else
				WriteMessage("Switch 2      = On");

			if ((pResponse->CHIME.id1 & 0x04) == 0x04)
				WriteMessage("Switch 3      = Off");
			else
				WriteMessage("Switch 3      = On");

			if ((pResponse->CHIME.id1 & 0x01) == 0x01)
				WriteMessage("Switch 4      = Off");
			else
				WriteMessage("Switch 4      = On");

			if ((pResponse->CHIME.id2 & 0x40) == 0x40)
				WriteMessage("Switch 5      = Off");
			else
				WriteMessage("Switch 5      = On");

			if ((pResponse->CHIME.id2 & 0x10) == 0x10)
				WriteMessage("Switch 6      = Off");
			else
				WriteMessage("Switch 6      = On");
			break;
		case sTypeSelectPlus:
			WriteMessage("subtype       = SelectPlus200689101");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			break;
		case sTypeSelectPlus3:
			WriteMessage("subtype       = SelectPlus200689103");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			break;
		case sTypeEnvivo:
			WriteMessage("subtype       = Envivo");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CHIME.packettype, pResponse->CHIME.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->CHIME.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}


void MainWorker::decode_UNDECODED(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];

	WriteMessage("UNDECODED ", false);

	switch (pResponse->UNDECODED.subtype)
	{
	case sTypeUac:
		WriteMessage("AC:", false);
		break;
	case sTypeUarc:
		WriteMessage("ARC:", false);
		break;
	case sTypeUati:
		WriteMessage("ATI:", false);
		break;
	case sTypeUhideki:
		WriteMessage("HIDEKI:", false);
		break;
	case sTypeUlacrosse:
		WriteMessage("LACROSSE:", false);
		break;
	case sTypeUad:
		WriteMessage("AD:", false);
		break;
	case sTypeUmertik:
		WriteMessage("MERTIK:", false);
		break;
	case sTypeUoregon1:
		WriteMessage("OREGON1:", false);
		break;
	case sTypeUoregon2:
		WriteMessage("OREGON2:", false);
		break;
	case sTypeUoregon3:
		WriteMessage("OREGON3:", false);
		break;
	case sTypeUproguard:
		WriteMessage("PROGUARD:", false);
		break;
	case sTypeUvisonic:
		WriteMessage("VISONIC:", false);
		break;
	case sTypeUnec:
		WriteMessage("NEC:", false);
		break;
	case sTypeUfs20:
		WriteMessage("FS20:", false);
		break;
	case sTypeUblinds:
		WriteMessage("Blinds:", false);
		break;
	case sTypeUrubicson:
		WriteMessage("RUBICSON:", false);
		break;
	case sTypeUae:
		WriteMessage("AE:", false);
		break;
	case sTypeUfineoffset:
		WriteMessage("FineOffset:", false);
		break;
	case sTypeUrgb:
		WriteMessage("RGB:", false);
		break;
	case sTypeUrfy:
		WriteMessage("RFY:", false);
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->UNDECODED.packettype, pResponse->UNDECODED.subtype);
		WriteMessage(szTmp);
		break;
	}
	std::stringstream sHexDump;
	unsigned char *pRXBytes = (unsigned char*)&pResponse->UNDECODED.msg1;
	for (int i = 0; i < pResponse->UNDECODED.packetlength - 3; i++)
	{
		sHexDump << HEX(pRXBytes[i]);
	}
	WriteMessage(sHexDump.str().c_str());
	procResult.DeviceRowIdx = -1;
}


void MainWorker::decode_RecXmitMessage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];

	switch (pResponse->RXRESPONSE.subtype)
	{
	case sTypeReceiverLockError:
		WriteMessage("subtype           = Receiver lock error");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->RXRESPONSE.seqnbr);
		WriteMessage(szTmp);
		break;
	case sTypeTransmitterResponse:
		WriteMessage("subtype           = Transmitter Response");
		sprintf(szTmp, "Sequence nbr      = %d", pResponse->RXRESPONSE.seqnbr);
		WriteMessage(szTmp);

		switch (pResponse->RXRESPONSE.msg)
		{
		case 0x0:
			WriteMessage("response          = ACK, data correct transmitted");
			break;
		case 0x1:
			WriteMessage("response          = ACK, but transmit started after 6 seconds delay anyway with RF receive data detected");
			break;
		case 0x2:
			WriteMessage("response          = NAK, transmitter did not lock on the requested transmit frequency");
			break;
		case 0x3:
			WriteMessage("response          = NAK, AC address zero in id1-id4 not allowed");
			break;
		default:
			sprintf(szTmp, "ERROR: Unexpected message type= %02X", pResponse->RXRESPONSE.msg);
			WriteMessage(szTmp);
			break;
		}
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RXRESPONSE.packettype, pResponse->RXRESPONSE.subtype);
		WriteMessage(szTmp);
		break;
	}
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Curtain(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeCurtain;
	unsigned char subType = pResponse->CURTAIN1.subtype;
	sprintf(szTmp, "%d", pResponse->CURTAIN1.housecode);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->CURTAIN1.unitcode;
	unsigned char cmnd = pResponse->CURTAIN1.cmnd;
	unsigned char SignalLevel = 9;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		char szTmp[100];

		switch (pResponse->CURTAIN1.subtype)
		{
		case sTypeHarrison:
			WriteMessage("subtype       = Harrison");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->CURTAIN1.packettype, pResponse->CURTAIN1.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->CURTAIN1.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "Housecode         = %d", pResponse->CURTAIN1.housecode);
		WriteMessage(szTmp);

		sprintf(szTmp, "Unit          = %d", pResponse->CURTAIN1.unitcode);
		WriteMessage(szTmp);

		WriteMessage("Command       = ", false);

		switch (pResponse->CURTAIN1.cmnd)
		{
		case curtain_sOpen:
			WriteMessage("Open");
			break;
		case curtain_sStop:
			WriteMessage("Stop");
			break;
		case curtain_sClose:
			WriteMessage("Close");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_BLINDS1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeBlinds;
	unsigned char subType = pResponse->BLINDS1.subtype;

	sprintf(szTmp, "%02X%02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2, pResponse->BLINDS1.id3, pResponse->BLINDS1.id4);

	std::string ID = szTmp;
	unsigned char Unit = pResponse->BLINDS1.unitcode;
	unsigned char cmnd = pResponse->BLINDS1.cmnd;
	unsigned char SignalLevel = pResponse->BLINDS1.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		char szTmp[100];

		switch (pResponse->BLINDS1.subtype)
		{
		case sTypeBlindsT0:
			WriteMessage("subtype       = Safy / RollerTrol / Hasta new");
			break;
		case sTypeBlindsT1:
			WriteMessage("subtype       = Hasta old");
			break;
		case sTypeBlindsT2:
			WriteMessage("subtype       = A-OK RF01");
			break;
		case sTypeBlindsT3:
			WriteMessage("subtype       = A-OK AC114");
			break;
		case sTypeBlindsT4:
			WriteMessage("subtype       = RAEX");
			break;
		case sTypeBlindsT5:
			WriteMessage("subtype       = Media Mount");
			break;
		case sTypeBlindsT6:
			WriteMessage("subtype       = DC106, YOOHA, Rohrmotor24 RMF");
			break;
		case sTypeBlindsT7:
			WriteMessage("subtype       = Forest");
			break;
		case sTypeBlindsT8:
			WriteMessage("subtype       = Chamberlain CS4330CN");
			break;
		case sTypeBlindsT9:
			WriteMessage("subtype       = Sunpery");
			break;
		case sTypeBlindsT10:
			WriteMessage("subtype       = Dolat DLM-1");
			break;
		case sTypeBlindsT11:
			WriteMessage("subtype       = ASP");
			break;
		case sTypeBlindsT12:
			WriteMessage("subtype       = Confexx");
			break;
		case sTypeBlindsT13:
			WriteMessage("subtype       = Screenline");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->BLINDS1.packettype, pResponse->BLINDS1.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->BLINDS1.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "id1-3         = %02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2, pResponse->BLINDS1.id3);
		WriteMessage(szTmp);

		if ((subType == sTypeBlindsT6) || (subType == sTypeBlindsT7))
		{
			sprintf(szTmp, "id4         = %02X", pResponse->BLINDS1.id4);
			WriteMessage(szTmp);
		}

		if (pResponse->BLINDS1.unitcode == 0)
			WriteMessage("Unit          = All");
		else
		{
			sprintf(szTmp, "Unit          = %d", pResponse->BLINDS1.unitcode);
			WriteMessage(szTmp);
		}

		WriteMessage("Command       = ", false);

		switch (pResponse->BLINDS1.cmnd)
		{
		case blinds_sOpen:
			WriteMessage("Open");
			break;
		case blinds_sStop:
			WriteMessage("Stop");
			break;
		case blinds_sClose:
			WriteMessage("Close");
			break;
		case blinds_sConfirm:
			WriteMessage("Confirm");
			break;
		case blinds_sLimit:
			WriteMessage("Set Limit");
			if (pResponse->BLINDS1.subtype == sTypeBlindsT4)
				WriteMessage("Set Upper Limit");
			else
				WriteMessage("Set Limit");
			break;
		case blinds_slowerLimit:
			WriteMessage("Set Lower Limit");
			break;
		case blinds_sDeleteLimits:
			WriteMessage("Delete Limits");
			break;
		case blinds_sChangeDirection:
			WriteMessage("Change Direction");
			break;
		case blinds_sLeft:
			WriteMessage("Left");
			break;
		case blinds_sRight:
			WriteMessage("Right");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->BLINDS1.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_RFY(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeRFY;
	unsigned char subType = pResponse->RFY.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->RFY.id1, pResponse->RFY.id2, pResponse->RFY.id3);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->RFY.unitcode;
	unsigned char cmnd = pResponse->RFY.cmnd;
	unsigned char SignalLevel = pResponse->RFY.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		char szTmp[100];

		switch (pResponse->RFY.subtype)
		{
		case sTypeRFY:
			WriteMessage("subtype       = RFY");
			break;
		case sTypeRFY2:
			WriteMessage("subtype       = RFY2");
			break;
		case sTypeRFYext:
			WriteMessage("subtype       = RFY-Ext");
			break;
		case sTypeASA:
			WriteMessage("subtype       = ASA");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->RFY.packettype, pResponse->RFY.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFY.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "id1-3         = %02X%02X%02X", pResponse->RFY.id1, pResponse->RFY.id2, pResponse->RFY.id3);
		WriteMessage(szTmp);

		if (pResponse->RFY.unitcode == 0)
			WriteMessage("Unit          = All");
		else
		{
			sprintf(szTmp, "Unit          = %d", pResponse->RFY.unitcode);
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "rfu1         = %02X", pResponse->RFY.rfu1);
		WriteMessage(szTmp);
		sprintf(szTmp, "rfu2         = %02X", pResponse->RFY.rfu2);
		WriteMessage(szTmp);
		sprintf(szTmp, "rfu3         = %02X", pResponse->RFY.rfu3);
		WriteMessage(szTmp);

		WriteMessage("Command       = ", false);

		switch (pResponse->RFY.cmnd)
		{
		case rfy_sStop:
			WriteMessage("Stop");
			break;
		case rfy_sUp:
			WriteMessage("Up");
			break;
		case rfy_sUpStop:
			WriteMessage("Up + Stop");
			break;
		case rfy_sDown:
			WriteMessage("Down");
			break;
		case rfy_sDownStop:
			WriteMessage("Down + Stop");
			break;
		case rfy_sUpDown:
			WriteMessage("Up + Down");
			break;
		case rfy_sListRemotes:
			WriteMessage("List remotes");
		case rfy_sProgram:
			WriteMessage("Program");
			break;
		case rfy_s2SecProgram:
			WriteMessage("2 seconds: Program");
			break;
		case rfy_s7SecProgram:
			WriteMessage("7 seconds: Program");
			break;
		case rfy_s2SecStop:
			WriteMessage("2 seconds: Stop");
			break;
		case rfy_s5SecStop:
			WriteMessage("5 seconds: Stop");
			break;
		case rfy_s5SecUpDown:
			WriteMessage("5 seconds: Up + Down");
			break;
		case rfy_sEraseThis:
			WriteMessage("Erase this remote");
			break;
		case rfy_sEraseAll:
			WriteMessage("Erase all remotes");
			break;
		case rfy_s05SecUp:
			WriteMessage("< 0.5 seconds: up");
			break;
		case rfy_s05SecDown:
			WriteMessage("< 0.5 seconds: down");
			break;
		case rfy_s2SecUp:
			WriteMessage("> 2 seconds: up");
			break;
		case rfy_s2SecDown:
			WriteMessage("> 2 seconds: down");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->RFY.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_evohome2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	const REVOBUF *pEvo = reinterpret_cast<const REVOBUF*>(pResponse);
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 255;//Unknown
	unsigned char BatteryLevel = 255;//Unknown

	//Get Device details
	std::vector<std::vector<std::string> > result;
	if (pEvo->EVOHOME2.type == pTypeEvohomeZone && pEvo->EVOHOME2.zone > 12) //Allow for additional Zone Temp devices which require DeviceID
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%x') AND (Type==%d)",
			HwdID, (int)RFX_GETID3(pEvo->EVOHOME2.id1, pEvo->EVOHOME2.id2, pEvo->EVOHOME2.id3), (int)pEvo->EVOHOME2.type);
	}
	else if (pEvo->EVOHOME2.zone)//if unit number is available the id3 will be the controller device id
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit == %d) AND (Type==%d)",
			HwdID, (int)pEvo->EVOHOME2.zone, (int)pEvo->EVOHOME2.type);
	}
	else//unit number not available then id3 should be the zone device id
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%x') AND (Type==%d)",
			HwdID, (int)RFX_GETID3(pEvo->EVOHOME2.id1, pEvo->EVOHOME2.id2, pEvo->EVOHOME2.id3), (int)pEvo->EVOHOME2.type);
	}
	if (result.size() < 1 && !pEvo->EVOHOME2.zone)
		return;

	CEvohomeBase *pEvoHW = reinterpret_cast<CEvohomeBase*>(GetHardware(HwdID));
	bool bNewDev = false;
	std::string name, szDevID;
	std::stringstream szID;
	unsigned char Unit;
	unsigned char dType;
	unsigned char dSubType;
	std::string szUpdateStat;
	if (result.size() > 0)
	{
		std::vector<std::string> sd = result[0];
		szDevID = sd[1];
		Unit = atoi(sd[2].c_str());
		dType = atoi(sd[3].c_str());
		dSubType = atoi(sd[4].c_str());
		szUpdateStat = sd[5];
		BatteryLevel = atoi(sd[6].c_str());
	}
	else
	{
		bNewDev = true;
		Unit = pEvo->EVOHOME2.zone;//should always be non zero
		dType = pEvo->EVOHOME2.type;
		dSubType = pEvo->EVOHOME2.subtype;

		szID << std::hex << (int)RFX_GETID3(pEvo->EVOHOME2.id1, pEvo->EVOHOME2.id2, pEvo->EVOHOME2.id3);
		szDevID = szID.str();

		if (!pEvoHW)
			return;
		if (dType == pTypeEvohomeWater)
			name = "Hot Water";
		else if (dType == pTypeEvohomeZone && !szDevID.empty())
			name = "Zone Temp";
		else
			name = pEvoHW->GetZoneName(Unit - 1);
		if (name.empty())
			return;
		szUpdateStat = "0.0;0.0;Auto";
	}

	if (pEvo->EVOHOME2.updatetype == CEvohomeBase::updBattery)
		BatteryLevel = pEvo->EVOHOME2.battery_level;
	else
	{
		if (dType == pTypeEvohomeWater && pEvo->EVOHOME2.updatetype == pEvoHW->updSetPoint)
			sprintf(szTmp, "%s", pEvo->EVOHOME2.temperature ? "On" : "Off");
		else
			sprintf(szTmp, "%.2f", pEvo->EVOHOME2.temperature / 100.0f);

		std::vector<std::string> strarray;
		StringSplit(szUpdateStat, ";", strarray);
		if (strarray.size() >= 3)
		{
			if (pEvo->EVOHOME2.updatetype == pEvoHW->updSetPoint)//SetPoint
			{
				strarray[1] = szTmp;
				if (pEvo->EVOHOME2.mode <= pEvoHW->zmTmp)//for the moment only update this if we get a valid setpoint mode as we can now send setpoint on its own
				{
					int nControllerMode = pEvo->EVOHOME2.controllermode;
					if (dType == pTypeEvohomeWater && (nControllerMode == pEvoHW->cmEvoHeatingOff || nControllerMode == pEvoHW->cmEvoAutoWithEco || nControllerMode == pEvoHW->cmEvoCustom))//dhw has no economy mode and does not turn off for heating off also appears custom does not support the dhw zone
						nControllerMode = pEvoHW->cmEvoAuto;
					if (pEvo->EVOHOME2.mode == pEvoHW->zmAuto || nControllerMode == pEvoHW->cmEvoHeatingOff)//if zonemode is auto (followschedule) or controllermode is heatingoff
						strarray[2] = pEvoHW->GetWebAPIModeName(nControllerMode);//the web front end ultimately uses these names for images etc.
					else
						strarray[2] = pEvoHW->GetZoneModeName(pEvo->EVOHOME2.mode);
					if (pEvo->EVOHOME2.mode == pEvoHW->zmTmp)
					{
						std::string szISODate(CEvohomeDateTime::GetISODate(pEvo->EVOHOME2));
						if (strarray.size() < 4) //add or set until
							strarray.push_back(szISODate);
						else
							strarray[3] = szISODate;
					}
					else if ((pEvo->EVOHOME2.mode == pEvoHW->zmAuto) && (HwdType == HTYPE_EVOHOME_WEB))
					{
						strarray[2] = "FollowSchedule";
						if ((pEvo->EVOHOME2.year != 0) && (pEvo->EVOHOME2.year != 0xFFFF))
						{
							std::string szISODate(CEvohomeDateTime::GetISODate(pEvo->EVOHOME2));
							if (strarray.size() < 4) //add or set until
								strarray.push_back(szISODate);
							else
								strarray[3] = szISODate;
						}

					}
					else
						if (strarray.size() >= 4) //remove until
							strarray.resize(3);
				}
			}
			else if (pEvo->EVOHOME2.updatetype == pEvoHW->updOverride)
			{
				strarray[2] = pEvoHW->GetZoneModeName(pEvo->EVOHOME2.mode);
				if (strarray.size() >= 4) //remove until
					strarray.resize(3);
			}
			else
				strarray[0] = szTmp;
			szUpdateStat = boost::algorithm::join(strarray, ";");
		}
	}
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, szDevID.c_str(), Unit, dType, dSubType, SignalLevel, BatteryLevel, cmnd, szUpdateStat.c_str(), procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	if (bNewDev)
	{
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")",
			name.c_str(), DevRowIdx);
		procResult.DeviceName = name;
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_evohome1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	const REVOBUF *pEvo = reinterpret_cast<const REVOBUF*>(pResponse);
	unsigned char devType = pTypeEvohome;
	unsigned char subType = pEvo->EVOHOME1.subtype;
	std::stringstream szID;
	if (HwdType == HTYPE_EVOHOME_SERIAL || HwdType == HTYPE_EVOHOME_TCP)
		szID << std::hex << (int)RFX_GETID3(pEvo->EVOHOME1.id1, pEvo->EVOHOME1.id2, pEvo->EVOHOME1.id3);
	else //GB3: web based evohome uses decimal device ID's
		szID << std::dec << (int)RFX_GETID3(pEvo->EVOHOME1.id1, pEvo->EVOHOME1.id2, pEvo->EVOHOME1.id3);
	std::string ID(szID.str());
	unsigned char Unit = 0;
	unsigned char cmnd = pEvo->EVOHOME1.status;
	unsigned char SignalLevel = 255;//Unknown
	unsigned char BatteryLevel = 255;//Unknown

	std::string szUntilDate;
	if (pEvo->EVOHOME1.mode == CEvohomeBase::cmTmp)//temporary
		szUntilDate = CEvohomeDateTime::GetISODate(pEvo->EVOHOME1);

	CEvohomeBase *pEvoHW = reinterpret_cast<CEvohomeBase*>(GetHardware(HwdID));

	//FIXME A similar check is also done in switchmodal do we want to forward the ooc flag and rely on this check entirely?
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q')",
		HwdID, ID.c_str());
	bool bNewDev = false;
	std::string name;
	if (result.size() > 0)
	{
		std::vector<std::string> sd = result[0];
		if (atoi(sd[7].c_str()) == cmnd && sd[8] == szUntilDate)
			return;
	}
	else
	{
		bNewDev = true;
		if (!pEvoHW)
			return;
		name = pEvoHW->GetControllerName();
		if (name.empty())
			return;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szUntilDate.c_str(), procResult.DeviceName, pEvo->EVOHOME1.action != 0);
	if (DevRowIdx == -1)
		return;
	if (bNewDev)
	{
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")",
			name.c_str(), DevRowIdx);
		procResult.DeviceName = name;
	}

	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");
	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pEvo->EVOHOME1.subtype)
		{
		case sTypeEvohome:
			WriteMessage("subtype       = Evohome");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pEvo->EVOHOME1.type, pEvo->EVOHOME1.subtype);
			WriteMessage(szTmp);
			break;
		}

		if (HwdType == HTYPE_EVOHOME_SERIAL || HwdType == HTYPE_EVOHOME_TCP)
			sprintf(szTmp, "id            = %02X:%02X:%02X", pEvo->EVOHOME1.id1, pEvo->EVOHOME1.id2, pEvo->EVOHOME1.id3);
		else //GB3: web based evohome uses decimal device ID's
			sprintf(szTmp, "id            = %u", (int)RFX_GETID3(pEvo->EVOHOME1.id1, pEvo->EVOHOME1.id2, pEvo->EVOHOME1.id3));
		WriteMessage(szTmp);
		sprintf(szTmp, "action        = %d", (int)pEvo->EVOHOME1.action);
		WriteMessage(szTmp);
		WriteMessage("status        = ");
		WriteMessage(pEvoHW->GetControllerModeName(pEvo->EVOHOME1.status));

		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_evohome3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	const REVOBUF *pEvo = reinterpret_cast<const REVOBUF*>(pResponse);
	unsigned char devType = pTypeEvohomeRelay;
	unsigned char subType = pEvo->EVOHOME1.subtype;
	std::stringstream szID;
	int nDevID = (int)RFX_GETID3(pEvo->EVOHOME3.id1, pEvo->EVOHOME3.id2, pEvo->EVOHOME3.id3);
	szID << std::hex << nDevID;
	std::string ID(szID.str());
	unsigned char Unit = pEvo->EVOHOME3.devno;
	unsigned char cmnd = (pEvo->EVOHOME3.demand > 0) ? light1_sOn : light1_sOff;
	sprintf(szTmp, "%d", pEvo->EVOHOME3.demand);
	std::string szDemand(szTmp);
	unsigned char SignalLevel = 255;//Unknown
	unsigned char BatteryLevel = 255;//Unknown

	if (Unit == 0xFF && nDevID == 0)
		return;
	//Get Device details (devno or devid not available)
	bool bNewDev = false;
	std::vector<std::vector<std::string> > result;
	if (Unit == 0xFF)
		result = m_sql.safe_query(
			"SELECT HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q')",
			HwdID, ID.c_str());
	else
		result = m_sql.safe_query(
			"SELECT HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit == '%d') AND (Type==%d) AND (DeviceID == '%q')",
			HwdID, (int)Unit, (int)pEvo->EVOHOME3.type, ID.c_str());
	if (result.size() > 0)
	{
		if (pEvo->EVOHOME3.demand == 0xFF)//we sometimes get a 0418 message after the initial device creation but it will mess up the logging as we don't have a demand
			return;
		unsigned char cur_cmnd = atoi(result[0][5].c_str());
		BatteryLevel = atoi(result[0][7].c_str());

		if (pEvo->EVOHOME3.updatetype == CEvohomeBase::updBattery)
		{
			BatteryLevel = pEvo->EVOHOME3.battery_level;
			szDemand = result[0][6];
			cmnd = (atoi(szDemand.c_str()) > 0) ? light1_sOn : light1_sOff;
		}
		if (Unit == 0xFF)
		{
			Unit = atoi(result[0][2].c_str());
			szDemand = result[0][6];
			if (cmnd == cur_cmnd)
				return;
		}
		else if (nDevID == 0)
		{
			ID = result[0][1];
		}
	}
	else
	{
		if (Unit == 0xFF || (nDevID == 0 && Unit > 12))
			return;
		bNewDev = true;
		if (pEvo->EVOHOME3.demand == 0xFF)//0418 allows us to associate unit and deviceid but no state information other messages only contain one or the other
			szDemand = "0";
		if (pEvo->EVOHOME3.updatetype == CEvohomeBase::updBattery)
			BatteryLevel = pEvo->EVOHOME3.battery_level;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szDemand.c_str(), procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (bNewDev && (Unit == 0xF9 || Unit == 0xFA || Unit == 0xFC || Unit <= 12))
	{
		if (Unit == 0xF9)
			procResult.DeviceName = "CH Valve";
		else if (Unit == 0xFA)
			procResult.DeviceName = "DHW Valve";
		else if (Unit == 0xFC)
		{
			if (pEvo->EVOHOME3.id1 >> 2 == CEvohomeID::devBridge) // Evohome OT Bridge
				procResult.DeviceName = "Boiler (OT Bridge)";
			else
				procResult.DeviceName = "Boiler";
		}
		else if (Unit <= 12)
			procResult.DeviceName = "Zone";
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")",
			procResult.DeviceName.c_str(), DevRowIdx);
	}

	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Security1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeSecurity1;
	unsigned char subType = pResponse->SECURITY1.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = pResponse->SECURITY1.status;
	unsigned char SignalLevel = pResponse->SECURITY1.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, false, pResponse->SECURITY1.battery_level & 0x0F);
	if (
		(pResponse->SECURITY1.subtype == sTypeKD101) ||
		(pResponse->SECURITY1.subtype == sTypeSA30) ||
		(pResponse->SECURITY1.subtype == sTypeDomoticzSecurity)
		)
	{
		//KD101 & SA30 do not support battery low indication
		BatteryLevel = 255;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->SECURITY1.subtype)
		{
		case sTypeSecX10:
			WriteMessage("subtype       = X10 security");
			break;
		case sTypeSecX10M:
			WriteMessage("subtype       = X10 security motion");
			break;
		case sTypeSecX10R:
			WriteMessage("subtype       = X10 security remote");
			break;
		case sTypeKD101:
			WriteMessage("subtype       = KD101 smoke detector");
			break;
		case sTypePowercodeSensor:
			WriteMessage("subtype       = Visonic PowerCode sensor - primary contact");
			break;
		case sTypePowercodeMotion:
			WriteMessage("subtype       = Visonic PowerCode motion");
			break;
		case sTypeCodesecure:
			WriteMessage("subtype       = Visonic CodeSecure");
			break;
		case sTypePowercodeAux:
			WriteMessage("subtype       = Visonic PowerCode sensor - auxiliary contact");
			break;
		case sTypeMeiantech:
			WriteMessage("subtype       = Meiantech/Atlantic/Aidebao");
			break;
		case sTypeSA30:
			WriteMessage("subtype       = Alecto SA30 smoke detector");
			break;
		case sTypeDomoticzSecurity:
			WriteMessage("subtype       = Security Panel");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->SECURITY1.packettype, pResponse->SECURITY1.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->SECURITY1.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "id1-3         = %02X:%02X:%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
		WriteMessage(szTmp);

		WriteMessage("status        = ", false);

		switch (pResponse->SECURITY1.status)
		{
		case sStatusNormal:
			WriteMessage("Normal");
			break;
		case sStatusNormalDelayed:
			WriteMessage("Normal Delayed");
			break;
		case sStatusAlarm:
			WriteMessage("Alarm");
			break;
		case sStatusAlarmDelayed:
			WriteMessage("Alarm Delayed");
			break;
		case sStatusMotion:
			WriteMessage("Motion");
			break;
		case sStatusNoMotion:
			WriteMessage("No Motion");
			break;
		case sStatusPanic:
			WriteMessage("Panic");
			break;
		case sStatusPanicOff:
			WriteMessage("Panic End");
			break;
		case sStatusArmAway:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group2 or ", false);
			WriteMessage("Arm Away");
			break;
		case sStatusArmAwayDelayed:
			WriteMessage("Arm Away Delayed");
			break;
		case sStatusArmHome:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group3 or ", false);
			WriteMessage("Arm Home");
			break;
		case sStatusArmHomeDelayed:
			WriteMessage("Arm Home Delayed");
			break;
		case sStatusDisarm:
			if (pResponse->SECURITY1.subtype == sTypeMeiantech)
				WriteMessage("Group1 or ", false);
			WriteMessage("Disarm");
			break;
		case sStatusLightOff:
			WriteMessage("Light Off");
			break;
		case sStatusLightOn:
			WriteMessage("Light On");
			break;
		case sStatusLight2Off:
			WriteMessage("Light 2 Off");
			break;
		case sStatusLight2On:
			WriteMessage("Light 2 On");
			break;
		case sStatusDark:
			WriteMessage("Dark detected");
			break;
		case sStatusLight:
			WriteMessage("Light Detected");
			break;
		case sStatusBatLow:
			WriteMessage("Battery low MS10 or XX18 sensor");
			break;
		case sStatusPairKD101:
			WriteMessage("Pair KD101");
			break;
		case sStatusNormalTamper:
			WriteMessage("Normal + Tamper");
			break;
		case sStatusNormalDelayedTamper:
			WriteMessage("Normal Delayed + Tamper");
			break;
		case sStatusAlarmTamper:
			WriteMessage("Alarm + Tamper");
			break;
		case sStatusAlarmDelayedTamper:
			WriteMessage("Alarm Delayed + Tamper");
			break;
		case sStatusMotionTamper:
			WriteMessage("Motion + Tamper");
			break;
		case sStatusNoMotionTamper:
			WriteMessage("No Motion + Tamper");
			break;
		}

		if (
			(pResponse->SECURITY1.subtype != sTypeKD101) &&		//KD101 & SA30 does not support battery low indication
			(pResponse->SECURITY1.subtype != sTypeSA30)
			)
		{
			if ((pResponse->SECURITY1.battery_level & 0xF) == 0)
				WriteMessage("battery level = Low");
			else
				WriteMessage("battery level = OK");
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->SECURITY1.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Security2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeSecurity2;
	unsigned char subType = pResponse->SECURITY2.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X%02X%02X%02X%02X%02X", pResponse->SECURITY2.id1, pResponse->SECURITY2.id2, pResponse->SECURITY2.id3, pResponse->SECURITY2.id4, pResponse->SECURITY2.id5, pResponse->SECURITY2.id6, pResponse->SECURITY2.id7, pResponse->SECURITY2.id8);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;// pResponse->SECURITY2.cmnd;
	unsigned char SignalLevel = pResponse->SECURITY2.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, false, pResponse->SECURITY2.battery_level & 0x0F);

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->SECURITY2.subtype)
		{
		case sTypeSec2Classic:
			WriteMessage("subtype       = Keeloq Classic");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->SECURITY2.packettype, pResponse->SECURITY2.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->SECURITY2.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "id1-8         = %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", pResponse->SECURITY2.id1, pResponse->SECURITY2.id2, pResponse->SECURITY2.id3, pResponse->SECURITY2.id4, pResponse->SECURITY2.id5, pResponse->SECURITY2.id6, pResponse->SECURITY2.id7, pResponse->SECURITY2.id8);
		WriteMessage(szTmp);

		if ((pResponse->SECURITY2.battery_level & 0xF) == 0)
			WriteMessage("battery level = Low");
		else
			WriteMessage("battery level = OK");

		sprintf(szTmp, "Signal level  = %d", pResponse->SECURITY2.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

//not in dbase yet
void MainWorker::decode_Camera1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessage("");

	//unsigned char devType=pTypeCamera;

	char szTmp[100];

	switch (pResponse->CAMERA1.subtype)
	{
	case sTypeNinja:
		WriteMessage("subtype       = X10 Ninja/Robocam");
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->CAMERA1.seqnbr);
		WriteMessage(szTmp);

		WriteMessage("Command       = ", false);

		switch (pResponse->CAMERA1.cmnd)
		{
		case camera_sLeft:
			WriteMessage("Left");
			break;
		case camera_sRight:
			WriteMessage("Right");
			break;
		case camera_sUp:
			WriteMessage("Up");
			break;
		case camera_sDown:
			WriteMessage("Down");
			break;
		case camera_sPosition1:
			WriteMessage("Position 1");
			break;
		case camera_sProgramPosition1:
			WriteMessage("Position 1 program");
			break;
		case camera_sPosition2:
			WriteMessage("Position 2");
			break;
		case camera_sProgramPosition2:
			WriteMessage("Position 2 program");
			break;
		case camera_sPosition3:
			WriteMessage("Position 3");
			break;
		case camera_sProgramPosition3:
			WriteMessage("Position 3 program");
			break;
		case camera_sPosition4:
			WriteMessage("Position 4");
			break;
		case camera_sProgramPosition4:
			WriteMessage("Position 4 program");
			break;
		case camera_sCenter:
			WriteMessage("Center");
			break;
		case camera_sProgramCenterPosition:
			WriteMessage("Center program");
			break;
		case camera_sSweep:
			WriteMessage("Sweep");
			break;
		case camera_sProgramSweep:
			WriteMessage("Sweep program");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp, "Housecode     = %d", pResponse->CAMERA1.housecode);
		WriteMessage(szTmp);
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CAMERA1.packettype, pResponse->CAMERA1.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp, "Signal level  = %d", pResponse->CAMERA1.rssi);
	WriteMessage(szTmp);
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Remote(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeRemote;
	unsigned char subType = pResponse->REMOTE.subtype;
	sprintf(szTmp, "%d", pResponse->REMOTE.id);
	std::string ID = szTmp;
	unsigned char Unit = pResponse->REMOTE.cmnd;
	unsigned char cmnd = light2_sOn;
	unsigned char SignalLevel = pResponse->REMOTE.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->REMOTE.subtype)
		{
		case sTypeATI:
			WriteMessage("subtype       = ATI Remote Wonder");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("Command       = A");
				break;
			case 0x1:
				WriteMessage("Command       = B");
				break;
			case 0x2:
				WriteMessage("Command       = power");
				break;
			case 0x3:
				WriteMessage("Command       = TV");
				break;
			case 0x4:
				WriteMessage("Command       = DVD");
				break;
			case 0x5:
				WriteMessage("Command       = ?");
				break;
			case 0x6:
				WriteMessage("Command       = Guide");
				break;
			case 0x7:
				WriteMessage("Command       = Drag");
				break;
			case 0x8:
				WriteMessage("Command       = VOL+");
				break;
			case 0x9:
				WriteMessage("Command       = VOL-");
				break;
			case 0xA:
				WriteMessage("Command       = MUTE");
				break;
			case 0xB:
				WriteMessage("Command       = CHAN+");
				break;
			case 0xC:
				WriteMessage("Command       = CHAN-");
				break;
			case 0xD:
				WriteMessage("Command       = 1");
				break;
			case 0xE:
				WriteMessage("Command       = 2");
				break;
			case 0xF:
				WriteMessage("Command       = 3");
				break;
			case 0x10:
				WriteMessage("Command       = 4");
				break;
			case 0x11:
				WriteMessage("Command       = 5");
				break;
			case 0x12:
				WriteMessage("Command       = 6");
				break;
			case 0x13:
				WriteMessage("Command       = 7");
				break;
			case 0x14:
				WriteMessage("Command       = 8");
				break;
			case 0x15:
				WriteMessage("Command       = 9");
				break;
			case 0x16:
				WriteMessage("Command       = txt");
				break;
			case 0x17:
				WriteMessage("Command       = 0");
				break;
			case 0x18:
				WriteMessage("Command       = snapshot ESC");
				break;
			case 0x19:
				WriteMessage("Command       = C");
				break;
			case 0x1A:
				WriteMessage("Command       = ^");
				break;
			case 0x1B:
				WriteMessage("Command       = D");
				break;
			case 0x1C:
				WriteMessage("Command       = TV/RADIO");
				break;
			case 0x1D:
				WriteMessage("Command       = <");
				break;
			case 0x1E:
				WriteMessage("Command       = OK");
				break;
			case 0x1F:
				WriteMessage("Command       = >");
				break;
			case 0x20:
				WriteMessage("Command       = <-");
				break;
			case 0x21:
				WriteMessage("Command       = E");
				break;
			case 0x22:
				WriteMessage("Command       = v");
				break;
			case 0x23:
				WriteMessage("Command       = F");
				break;
			case 0x24:
				WriteMessage("Command       = Rewind");
				break;
			case 0x25:
				WriteMessage("Command       = Play");
				break;
			case 0x26:
				WriteMessage("Command       = Fast forward");
				break;
			case 0x27:
				WriteMessage("Command       = Record");
				break;
			case 0x28:
				WriteMessage("Command       = Stop");
				break;
			case 0x29:
				WriteMessage("Command       = Pause");
				break;
			case 0x2C:
				WriteMessage("Command       = TV");
				break;
			case 0x2D:
				WriteMessage("Command       = VCR");
				break;
			case 0x2E:
				WriteMessage("Command       = RADIO");
				break;
			case 0x2F:
				WriteMessage("Command       = TV Preview");
				break;
			case 0x30:
				WriteMessage("Command       = Channel list");
				break;
			case 0x31:
				WriteMessage("Command       = Video Desktop");
				break;
			case 0x32:
				WriteMessage("Command       = red");
				break;
			case 0x33:
				WriteMessage("Command       = green");
				break;
			case 0x34:
				WriteMessage("Command       = yellow");
				break;
			case 0x35:
				WriteMessage("Command       = blue");
				break;
			case 0x36:
				WriteMessage("Command       = rename TAB");
				break;
			case 0x37:
				WriteMessage("Command       = Acquire image");
				break;
			case 0x38:
				WriteMessage("Command       = edit image");
				break;
			case 0x39:
				WriteMessage("Command       = Full screen");
				break;
			case 0x3A:
				WriteMessage("Command       = DVD Audio");
				break;
			case 0x70:
				WriteMessage("Command       = Cursor-left");
				break;
			case 0x71:
				WriteMessage("Command       = Cursor-right");
				break;
			case 0x72:
				WriteMessage("Command       = Cursor-up");
				break;
			case 0x73:
				WriteMessage("Command       = Cursor-down");
				break;
			case 0x74:
				WriteMessage("Command       = Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Command       = Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Command       = Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Command       = Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("Command       = V");
				break;
			case 0x79:
				WriteMessage("Command       = V-End");
				break;
			case 0x7C:
				WriteMessage("Command       = X");
				break;
			case 0x7D:
				WriteMessage("Command       = X-End");
				break;
			default:
				WriteMessage("Command       = unknown");
				break;
			}
			break;
		case sTypeATIplus:
			WriteMessage("subtype       = ATI Remote Wonder Plus");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);

			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("A", false);
				break;
			case 0x1:
				WriteMessage("B", false);
				break;
			case 0x2:
				WriteMessage("power", false);
				break;
			case 0x3:
				WriteMessage("TV", false);
				break;
			case 0x4:
				WriteMessage("DVD", false);
				break;
			case 0x5:
				WriteMessage("?", false);
				break;
			case 0x6:
				WriteMessage("Guide", false);
				break;
			case 0x7:
				WriteMessage("Drag", false);
				break;
			case 0x8:
				WriteMessage("VOL+", false);
				break;
			case 0x9:
				WriteMessage("VOL-", false);
				break;
			case 0xA:
				WriteMessage("MUTE", false);
				break;
			case 0xB:
				WriteMessage("CHAN+", false);
				break;
			case 0xC:
				WriteMessage("CHAN-", false);
				break;
			case 0xD:
				WriteMessage("1", false);
				break;
			case 0xE:
				WriteMessage("2", false);
				break;
			case 0xF:
				WriteMessage("3", false);
				break;
			case 0x10:
				WriteMessage("4", false);
				break;
			case 0x11:
				WriteMessage("5", false);
				break;
			case 0x12:
				WriteMessage("6", false);
				break;
			case 0x13:
				WriteMessage("7", false);
				break;
			case 0x14:
				WriteMessage("8", false);
				break;
			case 0x15:
				WriteMessage("9", false);
				break;
			case 0x16:
				WriteMessage("txt", false);
				break;
			case 0x17:
				WriteMessage("0", false);
				break;
			case 0x18:
				WriteMessage("Open Setup Menu", false);
				break;
			case 0x19:
				WriteMessage("C", false);
				break;
			case 0x1A:
				WriteMessage("^", false);
				break;
			case 0x1B:
				WriteMessage("D", false);
				break;
			case 0x1C:
				WriteMessage("FM", false);
				break;
			case 0x1D:
				WriteMessage("<", false);
				break;
			case 0x1E:
				WriteMessage("OK", false);
				break;
			case 0x1F:
				WriteMessage(">", false);
				break;
			case 0x20:
				WriteMessage("Max/Restore window", false);
				break;
			case 0x21:
				WriteMessage("E", false);
				break;
			case 0x22:
				WriteMessage("v", false);
				break;
			case 0x23:
				WriteMessage("F", false);
				break;
			case 0x24:
				WriteMessage("Rewind", false);
				break;
			case 0x25:
				WriteMessage("Play", false);
				break;
			case 0x26:
				WriteMessage("Fast forward", false);
				break;
			case 0x27:
				WriteMessage("Record", false);
				break;
			case 0x28:
				WriteMessage("Stop", false);
				break;
			case 0x29:
				WriteMessage("Pause", false);
				break;
			case 0x2A:
				WriteMessage("TV2", false);
				break;
			case 0x2B:
				WriteMessage("Clock", false);
				break;
			case 0x2C:
				WriteMessage("i", false);
				break;
			case 0x2D:
				WriteMessage("ATI", false);
				break;
			case 0x2E:
				WriteMessage("RADIO", false);
				break;
			case 0x2F:
				WriteMessage("TV Preview", false);
				break;
			case 0x30:
				WriteMessage("Channel list", false);
				break;
			case 0x31:
				WriteMessage("Video Desktop", false);
				break;
			case 0x32:
				WriteMessage("red", false);
				break;
			case 0x33:
				WriteMessage("green", false);
				break;
			case 0x34:
				WriteMessage("yellow", false);
				break;
			case 0x35:
				WriteMessage("blue", false);
				break;
			case 0x36:
				WriteMessage("rename TAB", false);
				break;
			case 0x37:
				WriteMessage("Acquire image", false);
				break;
			case 0x38:
				WriteMessage("edit image", false);
				break;
			case 0x39:
				WriteMessage("Full screen", false);
				break;
			case 0x3A:
				WriteMessage("DVD Audio", false);
				break;
			case 0x70:
				WriteMessage("Cursor-left", false);
				break;
			case 0x71:
				WriteMessage("Cursor-right", false);
				break;
			case 0x72:
				WriteMessage("Cursor-up", false);
				break;
			case 0x73:
				WriteMessage("Cursor-down", false);
				break;
			case 0x74:
				WriteMessage("Cursor-up-left", false);
				break;
			case 0x75:
				WriteMessage("Cursor-up-right", false);
				break;
			case 0x76:
				WriteMessage("Cursor-down-right", false);
				break;
			case 0x77:
				WriteMessage("Cursor-down-left", false);
				break;
			case 0x78:
				WriteMessage("Left Mouse Button", false);
				break;
			case 0x79:
				WriteMessage("V-End", false);
				break;
			case 0x7C:
				WriteMessage("Right Mouse Button", false);
				break;
			case 0x7D:
				WriteMessage("X-End", false);
				break;
			default:
				WriteMessage("unknown", false);
				break;
			}
			if ((pResponse->REMOTE.toggle & 1) == 1)
				WriteMessage("  (button press = odd)");
			else
				WriteMessage("  (button press = even)");
			break;
		case sTypeATIrw2:
			WriteMessage("subtype       = ATI Remote Wonder II");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			WriteMessage("Command type  = ", false);

			switch (pResponse->REMOTE.cmndtype & 0x0E)
			{
			case 0x0:
				WriteMessage("PC");
				break;
			case 0x2:
				WriteMessage("AUX1");
				break;
			case 0x4:
				WriteMessage("AUX2");
				break;
			case 0x6:
				WriteMessage("AUX3");
				break;
			case 0x8:
				WriteMessage("AUX4");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("A", false);
				break;
			case 0x1:
				WriteMessage("B", false);
				break;
			case 0x2:
				WriteMessage("power", false);
				break;
			case 0x3:
				WriteMessage("TV", false);
				break;
			case 0x4:
				WriteMessage("DVD", false);
				break;
			case 0x5:
				WriteMessage("?", false);
				break;
			case 0x7:
				WriteMessage("Drag", false);
				break;
			case 0x8:
				WriteMessage("VOL+", false);
				break;
			case 0x9:
				WriteMessage("VOL-", false);
				break;
			case 0xA:
				WriteMessage("MUTE", false);
				break;
			case 0xB:
				WriteMessage("CHAN+", false);
				break;
			case 0xC:
				WriteMessage("CHAN-", false);
				break;
			case 0xD:
				WriteMessage("1", false);
				break;
			case 0xE:
				WriteMessage("2", false);
				break;
			case 0xF:
				WriteMessage("3", false);
				break;
			case 0x10:
				WriteMessage("4", false);
				break;
			case 0x11:
				WriteMessage("5", false);
				break;
			case 0x12:
				WriteMessage("6", false);
				break;
			case 0x13:
				WriteMessage("7", false);
				break;
			case 0x14:
				WriteMessage("8", false);
				break;
			case 0x15:
				WriteMessage("9", false);
				break;
			case 0x16:
				WriteMessage("txt", false);
				break;
			case 0x17:
				WriteMessage("0", false);
				break;
			case 0x18:
				WriteMessage("Open Setup Menu", false);
				break;
			case 0x19:
				WriteMessage("C", false);
				break;
			case 0x1A:
				WriteMessage("^", false);
				break;
			case 0x1B:
				WriteMessage("D", false);
				break;
			case 0x1C:
				WriteMessage("TV/RADIO", false);
				break;
			case 0x1D:
				WriteMessage("<", false);
				break;
			case 0x1E:
				WriteMessage("OK", false);
				break;
			case 0x1F:
				WriteMessage(">", false);
				break;
			case 0x20:
				WriteMessage("Max/Restore window", false);
				break;
			case 0x21:
				WriteMessage("E", false);
				break;
			case 0x22:
				WriteMessage("v", false);
				break;
			case 0x23:
				WriteMessage("F", false);
				break;
			case 0x24:
				WriteMessage("Rewind", false);
				break;
			case 0x25:
				WriteMessage("Play", false);
				break;
			case 0x26:
				WriteMessage("Fast forward", false);
				break;
			case 0x27:
				WriteMessage("Record", false);
				break;
			case 0x28:
				WriteMessage("Stop", false);
				break;
			case 0x29:
				WriteMessage("Pause", false);
				break;
			case 0x2C:
				WriteMessage("i", false);
				break;
			case 0x2D:
				WriteMessage("ATI", false);
				break;
			case 0x3B:
				WriteMessage("PC", false);
				break;
			case 0x3C:
				WriteMessage("AUX1", false);
				break;
			case 0x3D:
				WriteMessage("AUX2", false);
				break;
			case 0x3E:
				WriteMessage("AUX3", false);
				break;
			case 0x3F:
				WriteMessage("AUX4", false);
				break;
			case 0x70:
				WriteMessage("Cursor-left", false);
				break;
			case 0x71:
				WriteMessage("Cursor-right", false);
				break;
			case 0x72:
				WriteMessage("Cursor-up", false);
				break;
			case 0x73:
				WriteMessage("Cursor-down", false);
				break;
			case 0x74:
				WriteMessage("Cursor-up-left", false);
				break;
			case 0x75:
				WriteMessage("Cursor-up-right", false);
				break;
			case 0x76:
				WriteMessage("Cursor-down-right", false);
				break;
			case 0x77:
				WriteMessage("Cursor-down-left", false);
				break;
			case 0x78:
				WriteMessage("Left Mouse Button", false);
				break;
			case 0x7C:
				WriteMessage("Right Mouse Button", false);
				break;
			default:
				WriteMessage("unknown", false);
				break;
			}
			if ((pResponse->REMOTE.toggle & 1) == 1)
				WriteMessage("  (button press = odd)");
			else
				WriteMessage("  (button press = even)");
			break;
		case sTypeMedion:
			WriteMessage("subtype       = Medion Remote");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);

			WriteMessage("Command       = ", false);

			switch (pResponse->REMOTE.cmnd)
			{
			case 0x0:
				WriteMessage("Mute");
				break;
			case 0x1:
				WriteMessage("B");
				break;
			case 0x2:
				WriteMessage("power");
				break;
			case 0x3:
				WriteMessage("TV");
				break;
			case 0x4:
				WriteMessage("DVD");
				break;
			case 0x5:
				WriteMessage("Photo");
				break;
			case 0x6:
				WriteMessage("Music");
				break;
			case 0x7:
				WriteMessage("Drag");
				break;
			case 0x8:
				WriteMessage("VOL-");
				break;
			case 0x9:
				WriteMessage("VOL+");
				break;
			case 0xA:
				WriteMessage("MUTE");
				break;
			case 0xB:
				WriteMessage("CHAN+");
				break;
			case 0xC:
				WriteMessage("CHAN-");
				break;
			case 0xD:
				WriteMessage("1");
				break;
			case 0xE:
				WriteMessage("2");
				break;
			case 0xF:
				WriteMessage("3");
				break;
			case 0x10:
				WriteMessage("4");
				break;
			case 0x11:
				WriteMessage("5");
				break;
			case 0x12:
				WriteMessage("6");
				break;
			case 0x13:
				WriteMessage("7");
				break;
			case 0x14:
				WriteMessage("8");
				break;
			case 0x15:
				WriteMessage("9");
				break;
			case 0x16:
				WriteMessage("txt");
				break;
			case 0x17:
				WriteMessage("0");
				break;
			case 0x18:
				WriteMessage("snapshot ESC");
				break;
			case 0x19:
				WriteMessage("DVD MENU");
				break;
			case 0x1A:
				WriteMessage("^");
				break;
			case 0x1B:
				WriteMessage("Setup");
				break;
			case 0x1C:
				WriteMessage("TV/RADIO");
				break;
			case 0x1D:
				WriteMessage("<");
				break;
			case 0x1E:
				WriteMessage("OK");
				break;
			case 0x1F:
				WriteMessage(">");
				break;
			case 0x20:
				WriteMessage("<-");
				break;
			case 0x21:
				WriteMessage("E");
				break;
			case 0x22:
				WriteMessage("v");
				break;
			case 0x23:
				WriteMessage("F");
				break;
			case 0x24:
				WriteMessage("Rewind");
				break;
			case 0x25:
				WriteMessage("Play");
				break;
			case 0x26:
				WriteMessage("Fast forward");
				break;
			case 0x27:
				WriteMessage("Record");
				break;
			case 0x28:
				WriteMessage("Stop");
				break;
			case 0x29:
				WriteMessage("Pause");
				break;
			case 0x2C:
				WriteMessage("TV");
				break;
			case 0x2D:
				WriteMessage("VCR");
				break;
			case 0x2E:
				WriteMessage("RADIO");
				break;
			case 0x2F:
				WriteMessage("TV Preview");
				break;
			case 0x30:
				WriteMessage("Channel list");
				break;
			case 0x31:
				WriteMessage("Video Desktop");
				break;
			case 0x32:
				WriteMessage("red");
				break;
			case 0x33:
				WriteMessage("green");
				break;
			case 0x34:
				WriteMessage("yellow");
				break;
			case 0x35:
				WriteMessage("blue");
				break;
			case 0x36:
				WriteMessage("rename TAB");
				break;
			case 0x37:
				WriteMessage("Acquire image");
				break;
			case 0x38:
				WriteMessage("edit image");
				break;
			case 0x39:
				WriteMessage("Full screen");
				break;
			case 0x3A:
				WriteMessage("DVD Audio");
				break;
			case 0x70:
				WriteMessage("Cursor-left");
				break;
			case 0x71:
				WriteMessage("Cursor-right");
				break;
			case 0x72:
				WriteMessage("Cursor-up");
				break;
			case 0x73:
				WriteMessage("Cursor-down");
				break;
			case 0x74:
				WriteMessage("Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("V");
				break;
			case 0x79:
				WriteMessage("V-End");
				break;
			case 0x7C:
				WriteMessage("X");
				break;
			case 0x7D:
				WriteMessage("X-End");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			break;
		case sTypePCremote:
			WriteMessage("subtype       = PC Remote");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->REMOTE.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->REMOTE.id);
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->REMOTE.cmnd)
			{
			case 0x2:
				WriteMessage("0");
				break;
			case 0x82:
				WriteMessage("1");
				break;
			case 0xD1:
				WriteMessage("MP3");
				break;
			case 0x42:
				WriteMessage("2");
				break;
			case 0xD2:
				WriteMessage("DVD");
				break;
			case 0xC2:
				WriteMessage("3");
				break;
			case 0xD3:
				WriteMessage("CD");
				break;
			case 0x22:
				WriteMessage("4");
				break;
			case 0xD4:
				WriteMessage("PC or SHIFT-4");
				break;
			case 0xA2:
				WriteMessage("5");
				break;
			case 0xD5:
				WriteMessage("SHIFT-5");
				break;
			case 0x62:
				WriteMessage("6");
				break;
			case 0xE2:
				WriteMessage("7");
				break;
			case 0x12:
				WriteMessage("8");
				break;
			case 0x92:
				WriteMessage("9");
				break;
			case 0xC0:
				WriteMessage("CH-");
				break;
			case 0x40:
				WriteMessage("CH+");
				break;
			case 0xE0:
				WriteMessage("VOL-");
				break;
			case 0x60:
				WriteMessage("VOL+");
				break;
			case 0xA0:
				WriteMessage("MUTE");
				break;
			case 0x3A:
				WriteMessage("INFO");
				break;
			case 0x38:
				WriteMessage("REW");
				break;
			case 0xB8:
				WriteMessage("FF");
				break;
			case 0xB0:
				WriteMessage("PLAY");
				break;
			case 0x64:
				WriteMessage("PAUSE");
				break;
			case 0x63:
				WriteMessage("STOP");
				break;
			case 0xB6:
				WriteMessage("MENU");
				break;
			case 0xFF:
				WriteMessage("REC");
				break;
			case 0xC9:
				WriteMessage("EXIT");
				break;
			case 0xD8:
				WriteMessage("TEXT");
				break;
			case 0xD9:
				WriteMessage("SHIFT-TEXT");
				break;
			case 0xF2:
				WriteMessage("TELETEXT");
				break;
			case 0xD7:
				WriteMessage("SHIFT-TELETEXT");
				break;
			case 0xBA:
				WriteMessage("A+B");
				break;
			case 0x52:
				WriteMessage("ENT");
				break;
			case 0xD6:
				WriteMessage("SHIFT-ENT");
				break;
			case 0x70:
				WriteMessage("Cursor-left");
				break;
			case 0x71:
				WriteMessage("Cursor-right");
				break;
			case 0x72:
				WriteMessage("Cursor-up");
				break;
			case 0x73:
				WriteMessage("Cursor-down");
				break;
			case 0x74:
				WriteMessage("Cursor-up-left");
				break;
			case 0x75:
				WriteMessage("Cursor-up-right");
				break;
			case 0x76:
				WriteMessage("Cursor-down-right");
				break;
			case 0x77:
				WriteMessage("Cursor-down-left");
				break;
			case 0x78:
				WriteMessage("Left mouse");
				break;
			case 0x79:
				WriteMessage("Left mouse-End");
				break;
			case 0x7B:
				WriteMessage("Drag");
				break;
			case 0x7C:
				WriteMessage("Right mouse");
				break;
			case 0x7D:
				WriteMessage("Right mouse-End");
				break;
			default:
				WriteMessage("unknown");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->REMOTE.packettype, pResponse->REMOTE.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->REMOTE.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Thermostat1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeThermostat1;
	unsigned char subType = pResponse->THERMOSTAT1.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->THERMOSTAT1.id1 * 256) + pResponse->THERMOSTAT1.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->THERMOSTAT1.rssi;
	unsigned char BatteryLevel = 255;

	unsigned char temp = pResponse->THERMOSTAT1.temperature;
	unsigned char set_point = pResponse->THERMOSTAT1.set_point;
	unsigned char mode = (pResponse->THERMOSTAT1.mode & 0x80);
	unsigned char status = (pResponse->THERMOSTAT1.status & 0x03);

	sprintf(szTmp, "%d;%d;%d;%d", temp, set_point, mode, status);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->THERMOSTAT1.subtype)
		{
		case sTypeDigimax:
			WriteMessage("subtype       = Digimax");
			break;
		case sTypeDigimaxShort:
			WriteMessage("subtype       = Digimax with short format");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT1.packettype, pResponse->THERMOSTAT1.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->THERMOSTAT1.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->THERMOSTAT1.id1 * 256) + pResponse->THERMOSTAT1.id2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Temperature   = %d C", pResponse->THERMOSTAT1.temperature);
		WriteMessage(szTmp);

		if (pResponse->THERMOSTAT1.subtype == sTypeDigimax)
		{
			sprintf(szTmp, "Set           = %d C", pResponse->THERMOSTAT1.set_point);
			WriteMessage(szTmp);

			if ((pResponse->THERMOSTAT1.mode & 0x80) == 0)
				WriteMessage("Mode          = heating");
			else
				WriteMessage("Mode          = Cooling");

			switch (pResponse->THERMOSTAT1.status & 0x03)
			{
			case 0:
				WriteMessage("Status        = no status available");
				break;
			case 1:
				WriteMessage("Status        = demand");
				break;
			case 2:
				WriteMessage("Status        = no demand");
				break;
			case 3:
				WriteMessage("Status        = initializing");
				break;
			}
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->THERMOSTAT1.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Thermostat2(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeThermostat2;
	unsigned char subType = pResponse->THERMOSTAT2.subtype;
	std::string ID;
	ID = "1";
	unsigned char Unit = pResponse->THERMOSTAT2.unitcode;
	unsigned char cmnd = pResponse->THERMOSTAT2.cmnd;
	unsigned char SignalLevel = pResponse->THERMOSTAT2.rssi;
	unsigned char BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->THERMOSTAT2.subtype)
		{
		case sTypeHE105:
			WriteMessage("subtype       = HE105");
			break;
		case sTypeRTS10:
			WriteMessage("subtype       = RTS10");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT2.packettype, pResponse->THERMOSTAT2.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->THERMOSTAT2.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "Unit code        = 0x%02X", pResponse->THERMOSTAT2.unitcode);
		WriteMessage(szTmp);

		switch (pResponse->THERMOSTAT2.cmnd)
		{
		case thermostat2_sOff:
			WriteMessage("Command       = Off");
			break;
		case thermostat2_sOn:
			WriteMessage("Command       = On");
			break;
		default:
			WriteMessage("Command       = unknown");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->THERMOSTAT2.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Thermostat3(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeThermostat3;
	unsigned char subType = pResponse->THERMOSTAT3.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT3.unitcode1, pResponse->THERMOSTAT3.unitcode2, pResponse->THERMOSTAT3.unitcode3);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = pResponse->THERMOSTAT3.cmnd;
	unsigned char SignalLevel = pResponse->THERMOSTAT3.rssi;
	unsigned char BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "");

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->THERMOSTAT3.subtype)
		{
		case sTypeMertikG6RH4T1:
			WriteMessage("subtype       = Mertik G6R-H4T1");
			break;
		case sTypeMertikG6RH4TB:
			WriteMessage("subtype       = Mertik G6R-H4TB");
			break;
		case sTypeMertikG6RH4TD:
			WriteMessage("subtype       = Mertik G6R-H4TD");
			break;
		case sTypeMertikG6RH4S:
			WriteMessage("subtype       = Mertik G6R-H4S");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT3.packettype, pResponse->THERMOSTAT3.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->THERMOSTAT3.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = 0x%02X%02X%02X", pResponse->THERMOSTAT3.unitcode1, pResponse->THERMOSTAT3.unitcode2, pResponse->THERMOSTAT3.unitcode3);
		WriteMessage(szTmp);

		switch (pResponse->THERMOSTAT3.cmnd)
		{
		case thermostat3_sOff:
			WriteMessage("Command       = Off");
			break;
		case thermostat3_sOn:
			WriteMessage("Command       = On");
			break;
		case thermostat3_sUp:
			WriteMessage("Command       = Up");
			break;
		case thermostat3_sDown:
			WriteMessage("Command       = Down");
			break;
		case thermostat3_sRunUp:
			if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
				WriteMessage("Command       = Run Up");
			else
				WriteMessage("Command       = 2nd Off");
			break;
		case thermostat3_sRunDown:
			if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
				WriteMessage("Command       = Run Down");
			else
				WriteMessage("Command       = 2nd On");
			break;
		case thermostat3_sStop:
			if (pResponse->THERMOSTAT3.subtype == sTypeMertikG6RH4T1)
				WriteMessage("Command       = Stop");
			else
				WriteMessage("Command       = unknown");
		default:
			WriteMessage("Command       = unknown");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->THERMOSTAT3.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Thermostat4(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeThermostat4;
	unsigned char subType = pResponse->THERMOSTAT4.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT4.unitcode1, pResponse->THERMOSTAT4.unitcode2, pResponse->THERMOSTAT4.unitcode3);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char SignalLevel = pResponse->THERMOSTAT4.rssi;
	unsigned char BatteryLevel = 255;
	sprintf(szTmp, "%d;%d;%d;%d;%d;%d",
		pResponse->THERMOSTAT4.beep,
		pResponse->THERMOSTAT4.fan1_speed,
		pResponse->THERMOSTAT4.fan2_speed,
		pResponse->THERMOSTAT4.fan3_speed,
		pResponse->THERMOSTAT4.flame_power,
		pResponse->THERMOSTAT4.mode
	);

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->THERMOSTAT4.subtype)
		{
		case sTypeMCZ1:
			WriteMessage("subtype       = MCZ pellet stove 1 fan model");
			break;
		case sTypeMCZ2:
			WriteMessage("subtype       = MCZ pellet stove 2 fan model");
			break;
		case sTypeMCZ3:
			WriteMessage("subtype       = MCZ pellet stove 3 fan model");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->THERMOSTAT4.packettype, pResponse->THERMOSTAT4.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->THERMOSTAT4.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = 0x%02X%02X%02X", pResponse->THERMOSTAT4.unitcode1, pResponse->THERMOSTAT4.unitcode2, pResponse->THERMOSTAT4.unitcode3);
		WriteMessage(szTmp);

		if (pResponse->THERMOSTAT4.beep)
			WriteMessage("Beep          = Yes");
		else
			WriteMessage("Beep          = No");

		if (pResponse->THERMOSTAT4.fan1_speed == 6)
			strcpy(szTmp, "Fan 1 Speed   = Auto");
		else
			sprintf(szTmp, "Fan 1 Speed   = %d", pResponse->THERMOSTAT4.fan1_speed);
		WriteMessage(szTmp);

		if (pResponse->THERMOSTAT4.fan2_speed == 6)
			strcpy(szTmp, "Fan 2 Speed   = Auto");
		else
			sprintf(szTmp, "Fan 2 Speed   = %d", pResponse->THERMOSTAT4.fan2_speed);
		WriteMessage(szTmp);

		if (pResponse->THERMOSTAT4.fan3_speed == 6)
			strcpy(szTmp, "Fan 3 Speed   = Auto");
		else
			sprintf(szTmp, "Fan 3 Speed   = %d", pResponse->THERMOSTAT4.fan3_speed);
		WriteMessage(szTmp);

		sprintf(szTmp, "Flame power   = %d", pResponse->THERMOSTAT4.flame_power);
		WriteMessage(szTmp);

		switch (pResponse->THERMOSTAT4.mode)
		{
		case thermostat4_sOff:
			WriteMessage("Command       = Off");
			break;
		case thermostat4_sManual:
			WriteMessage("Command       = Manual");
			break;
		case thermostat4_sAuto:
			WriteMessage("Command       = Auto");
			break;
		case thermostat4_sEco:
			WriteMessage("Command       = Eco");
			break;
		default:
			WriteMessage("Command       = unknown");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->THERMOSTAT3.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Radiator1(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeRadiator1;
	unsigned char subType = pResponse->RADIATOR1.subtype;
	std::string ID;
	sprintf(szTmp, "%X%02X%02X%02X", pResponse->RADIATOR1.id1, pResponse->RADIATOR1.id2, pResponse->RADIATOR1.id3, pResponse->RADIATOR1.id4);
	ID = szTmp;
	unsigned char Unit = pResponse->RADIATOR1.unitcode;
	unsigned char cmnd = pResponse->RADIATOR1.cmnd;
	unsigned char SignalLevel = pResponse->RADIATOR1.rssi;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%d.%d", pResponse->RADIATOR1.temperature, pResponse->RADIATOR1.tempPoint5);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->RADIATOR1.subtype)
		{
		case sTypeSmartwares:
			WriteMessage("subtype       = Smartwares");
			break;
		case sTypeSmartwaresSwitchRadiator:
			WriteMessage("subtype       = Smartwares Radiator Switch");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RADIATOR1.packettype, pResponse->RADIATOR1.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->THERMOSTAT3.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = %X%02X%02X%02X", pResponse->RADIATOR1.id1, pResponse->RADIATOR1.id2, pResponse->RADIATOR1.id3, pResponse->RADIATOR1.id4);
		WriteMessage(szTmp);
		sprintf(szTmp, "Unit          = %d", pResponse->RADIATOR1.unitcode);
		WriteMessage(szTmp);

		switch (pResponse->RADIATOR1.cmnd)
		{
		case Radiator1_sNight:
			WriteMessage("Command       = Night/Off");
			break;
		case Radiator1_sDay:
			WriteMessage("Command       = Day/On");
			break;
		case Radiator1_sSetTemp:
			WriteMessage("Command       = Set Temperature");
			break;
		default:
			WriteMessage("Command       = unknown");
			break;
		}

		sprintf(szTmp, "Temp          = %d.%d", pResponse->RADIATOR1.temperature, pResponse->RADIATOR1.tempPoint5);
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->RADIATOR1.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

//not in dbase yet
void MainWorker::decode_Baro(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessage("");

	//unsigned char devType=pTypeBARO;

	WriteMessage("Not implemented");

	procResult.DeviceRowIdx = -1;
}

//not in dbase yet
void MainWorker::decode_DateTime(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessage("");

	//unsigned char devType=pTypeDT;

	char szTmp[100];

	switch (pResponse->DT.subtype)
	{
	case sTypeDT1:
		WriteMessage("Subtype       = DT1 - RTGR328N");
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->DT.packettype, pResponse->DT.subtype);
		WriteMessage(szTmp);
		break;
	}
	sprintf(szTmp, "Sequence nbr  = %d", pResponse->DT.seqnbr);
	WriteMessage(szTmp);
	sprintf(szTmp, "ID            = %d", (pResponse->DT.id1 * 256) + pResponse->DT.id2);
	WriteMessage(szTmp);

	WriteMessage("Day of week   = ", false);

	switch (pResponse->DT.dow)
	{
	case 1:
		WriteMessage(" Sunday");
		break;
	case 2:
		WriteMessage(" Monday");
		break;
	case 3:
		WriteMessage(" Tuesday");
		break;
	case 4:
		WriteMessage(" Wednesday");
		break;
	case 5:
		WriteMessage(" Thursday");
		break;
	case 6:
		WriteMessage(" Friday");
		break;
	case 7:
		WriteMessage(" Saturday");
		break;
	}
	sprintf(szTmp, "Date yy/mm/dd = %02d/%02d/%02d", pResponse->DT.yy, pResponse->DT.mm, pResponse->DT.dd);
	WriteMessage(szTmp);
	sprintf(szTmp, "Time          = %02d:%02d:%02d", pResponse->DT.hr, pResponse->DT.min, pResponse->DT.sec);
	WriteMessage(szTmp);
	sprintf(szTmp, "Signal level  = %d", pResponse->DT.rssi);
	WriteMessage(szTmp);
	if ((pResponse->DT.battery_level & 0x0F) == 0)
		WriteMessage("Battery       = Low");
	else
		WriteMessage("Battery       = OK");
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Current(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeCURRENT;
	unsigned char subType = pResponse->CURRENT.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->CURRENT.id1 * 256) + pResponse->CURRENT.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->CURRENT.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, false, pResponse->CURRENT.battery_level & 0x0F);

	float CurrentChannel1 = float((pResponse->CURRENT.ch1h * 256) + pResponse->CURRENT.ch1l) / 10.0f;
	float CurrentChannel2 = float((pResponse->CURRENT.ch2h * 256) + pResponse->CURRENT.ch2l) / 10.0f;
	float CurrentChannel3 = float((pResponse->CURRENT.ch3h * 256) + pResponse->CURRENT.ch3l) / 10.0f;
	sprintf(szTmp, "%.1f;%.1f;%.1f", CurrentChannel1, CurrentChannel2, CurrentChannel3);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleAmpere123Notification(DevRowIdx, procResult.DeviceName, CurrentChannel1, CurrentChannel2, CurrentChannel3);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->CURRENT.subtype)
		{
		case sTypeELEC1:
			WriteMessage("subtype       = ELEC1 - OWL CM113, Electrisave, cent-a-meter");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CURRENT.packettype, pResponse->CURRENT.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->CURRENT.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->CURRENT.id1 * 256) + pResponse->CURRENT.id2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Count         = %d", pResponse->CURRENT.id2);//m_rxbuffer[5]);
		WriteMessage(szTmp);
		sprintf(szTmp, "Channel 1     = %.1f ampere", CurrentChannel1);
		WriteMessage(szTmp);
		sprintf(szTmp, "Channel 2     = %.1f ampere", CurrentChannel2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Channel 3     = %.1f ampere", CurrentChannel3);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->CURRENT.rssi);
		WriteMessage(szTmp);

		if ((pResponse->CURRENT.battery_level & 0xF) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Energy(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeENERGY;
	unsigned char subType = pResponse->ENERGY.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->ENERGY.id1 * 256) + pResponse->ENERGY.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->ENERGY.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, false, pResponse->ENERGY.battery_level & 0x0F);

	long instant;

	instant = (pResponse->ENERGY.instant1 * 0x1000000) + (pResponse->ENERGY.instant2 * 0x10000) + (pResponse->ENERGY.instant3 * 0x100) + pResponse->ENERGY.instant4;

	double total = (
		double(pResponse->ENERGY.total1) * 0x10000000000ULL +
		double(pResponse->ENERGY.total2) * 0x100000000ULL +
		double(pResponse->ENERGY.total3) * 0x1000000 +
		double(pResponse->ENERGY.total4) * 0x10000 +
		double(pResponse->ENERGY.total5) * 0x100 +
		double(pResponse->ENERGY.total6)
		) / 223.666;

	if (pResponse->ENERGY.subtype == sTypeELEC3)
	{
		if (total == 0)
		{
			//Retrieve last total from current record
			int nValue;
			std::string sValue;
			struct tm LastUpdateTime;
			if (!m_sql.GetLastValue(HwdID, ID.c_str(), Unit, devType, subType, nValue, sValue, LastUpdateTime))
				return;
			std::vector<std::string> strarray;
			StringSplit(sValue, ";", strarray);
			if (strarray.size() != 2)
				return;
			total = atof(strarray[1].c_str());
		}
	}

	//Translate this sensor type to the new kWh sensor type
	_tGeneralDevice gdevice;
	gdevice.intval1 = (pResponse->ENERGY.id1 * 256) + pResponse->ENERGY.id2;
	gdevice.subtype = sTypeKwh;
	gdevice.floatval1 = (float)instant;
	gdevice.floatval2 = (float)total;

	int voltage = 230;
	m_sql.GetPreferencesVar("ElectricVoltage", voltage);
	if (voltage != 230)
	{
		float mval = float(voltage) / 230.0f;
		gdevice.floatval1 *= mval;
		gdevice.floatval2 *= mval;
	}

	decode_General(HwdID, HwdType, (const tRBUF*)&gdevice, procResult, SignalLevel, BatteryLevel);
	procResult.bProcessBatteryValue = false;
}

void MainWorker::decode_Power(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypePOWER;
	unsigned char subType = pResponse->POWER.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->POWER.id1 * 256) + pResponse->POWER.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->POWER.rssi;
	unsigned char BatteryLevel = 255;

	int Voltage = pResponse->POWER.voltage;
	double current = ((pResponse->POWER.currentH * 256) + pResponse->POWER.currentL) / 100.0;
	double instant = ((pResponse->POWER.powerH * 256) + pResponse->POWER.powerL) / 10.0;// Watt
	double usage = ((pResponse->POWER.energyH * 256) + pResponse->POWER.energyL) / 100.0; //kWh
	double powerfactor = pResponse->POWER.pf / 100.0;
	int frequency = pResponse->POWER.freq; //Hz

	sprintf(szTmp, "%ld;%.2f", long(round(instant)), usage*1000.0);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (const float)instant);

	//Voltage
	sprintf(szTmp, "%.3f", (float)Voltage);
	std::string tmpDevName;
	uint64_t DevRowIdxAlt = m_sql.UpdateValue(HwdID, ID.c_str(), 1, pTypeGeneral, sTypeVoltage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName);
	if (DevRowIdxAlt == -1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdxAlt, tmpDevName, pTypeGeneral, sTypeVoltage, NTYPE_USAGE, (float)Voltage);

	//Powerfactor
	sprintf(szTmp, "%.2f", (float)powerfactor);
	DevRowIdxAlt = m_sql.UpdateValue(HwdID, ID.c_str(), 2, pTypeGeneral, sTypePercentage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName);
	if (DevRowIdxAlt == -1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdxAlt, tmpDevName, pTypeGeneral, sTypePercentage, NTYPE_PERCENTAGE, (float)powerfactor);

	//Frequency
	sprintf(szTmp, "%.2f", (float)frequency);
	DevRowIdxAlt = m_sql.UpdateValue(HwdID, ID.c_str(), 3, pTypeGeneral, sTypePercentage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName);
	if (DevRowIdxAlt == -1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdxAlt, tmpDevName, pTypeGeneral, sTypePercentage, NTYPE_PERCENTAGE, (float)frequency);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->POWER.subtype)
		{
		case sTypeELEC5:
			WriteMessage("subtype       = ELEC5 - Revolt");
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->POWER.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->POWER.id1 * 256) + pResponse->POWER.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Voltage       = %d Volt", Voltage);
		WriteMessage(szTmp);
		sprintf(szTmp, "Current       = %.2f Ampere", current);
		WriteMessage(szTmp);

		sprintf(szTmp, "Instant usage = %.2f Watt", instant);
		WriteMessage(szTmp);
		sprintf(szTmp, "total usage   = %.2f kWh", usage);
		WriteMessage(szTmp);

		sprintf(szTmp, "Power factor  = %.2f", powerfactor);
		WriteMessage(szTmp);
		sprintf(szTmp, "Frequency     = %d Hz", frequency);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->POWER.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Current_Energy(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeCURRENTENERGY;
	unsigned char subType = pResponse->CURRENT_ENERGY.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->CURRENT_ENERGY.id1 * 256) + pResponse->CURRENT_ENERGY.id2);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->CURRENT_ENERGY.rssi;
	unsigned char BatteryLevel = get_BateryLevel(HwdType, false, pResponse->CURRENT_ENERGY.battery_level & 0x0F);

	float CurrentChannel1 = float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0f;
	float CurrentChannel2 = float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0f;
	float CurrentChannel3 = float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0f;

	double usage = 0;

	if (pResponse->CURRENT_ENERGY.count != 0)
	{
		//no usage provided, get the last usage
		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query(
			"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HwdID, ID.c_str(), int(Unit), int(devType), int(subType));
		if (result2.size() > 0)
		{
			std::vector<std::string> strarray;
			StringSplit(result2[0][1], ";", strarray);
			if (strarray.size() == 4)
			{
				usage = atof(strarray[3].c_str());
			}
		}
	}
	else
	{
		usage = (
			double(pResponse->CURRENT_ENERGY.total1) * 0x10000000000ULL +
			double(pResponse->CURRENT_ENERGY.total2) * 0x100000000ULL +
			double(pResponse->CURRENT_ENERGY.total3) * 0x1000000 +
			double(pResponse->CURRENT_ENERGY.total4) * 0x10000 +
			double(pResponse->CURRENT_ENERGY.total5) * 0x100 +
			pResponse->CURRENT_ENERGY.total6
			) / 223.666;

		if (usage == 0)
		{
			//That should not be, let's get the previous value
			//no usage provided, get the last usage
			std::vector<std::vector<std::string> > result2;
			result2 = m_sql.safe_query(
				"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
				HwdID, ID.c_str(), int(Unit), int(devType), int(subType));
			if (result2.size() > 0)
			{
				std::vector<std::string> strarray;
				StringSplit(result2[0][1], ";", strarray);
				if (strarray.size() == 4)
				{
					usage = atof(strarray[3].c_str());
				}
			}
		}

		int voltage = 230;
		m_sql.GetPreferencesVar("ElectricVoltage", voltage);

		sprintf(szTmp, "%ld;%.2f", (long)round((CurrentChannel1 + CurrentChannel2 + CurrentChannel3)*voltage), usage);
		m_sql.UpdateValue(HwdID, ID.c_str(), Unit, pTypeENERGY, sTypeELEC3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	}
	sprintf(szTmp, "%.1f;%.1f;%.1f;%.3f", CurrentChannel1, CurrentChannel2, CurrentChannel3, usage);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleAmpere123Notification(DevRowIdx, procResult.DeviceName, CurrentChannel1, CurrentChannel2, CurrentChannel3);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->CURRENT_ENERGY.subtype)
		{
		case sTypeELEC4:
			WriteMessage("subtype       = ELEC4 - OWL CM180i");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->CURRENT_ENERGY.packettype, pResponse->CURRENT_ENERGY.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->CURRENT_ENERGY.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->CURRENT_ENERGY.id1 * 256) + pResponse->CURRENT_ENERGY.id2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Count         = %d", pResponse->CURRENT_ENERGY.count);
		WriteMessage(szTmp);
		float ampereChannel1, ampereChannel2, ampereChannel3;
		ampereChannel1 = float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0f;
		ampereChannel2 = float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0f;
		ampereChannel3 = float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0f;
		sprintf(szTmp, "Channel 1     = %.1f ampere", ampereChannel1);
		WriteMessage(szTmp);
		sprintf(szTmp, "Channel 2     = %.1f ampere", ampereChannel2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Channel 3     = %.1f ampere", ampereChannel3);
		WriteMessage(szTmp);

		if (pResponse->CURRENT_ENERGY.count == 0)
		{
			sprintf(szTmp, "total usage   = %.3f Wh", usage);
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->CURRENT_ENERGY.rssi);
		WriteMessage(szTmp);

		if ((pResponse->CURRENT_ENERGY.battery_level & 0xF) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

//not in dbase yet
void MainWorker::decode_Gas(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessage("");

	//unsigned char devType=pTypeGAS;

	WriteMessage("Not implemented");

	procResult.DeviceRowIdx = -1;
}

//not in dbase yet
void MainWorker::decode_Water(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	WriteMessage("");

	//unsigned char devType=pTypeWATER;

	WriteMessage("Not implemented");

	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Weight(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeWEIGHT;
	unsigned char subType = pResponse->WEIGHT.subtype;
	unsigned short weightID = (pResponse->WEIGHT.id1 * 256) + pResponse->WEIGHT.id2;
	std::string ID;
	sprintf(szTmp, "%d", weightID);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->WEIGHT.rssi;
	unsigned char BatteryLevel = 255;
	float weight = (float(pResponse->WEIGHT.weighthigh) * 25.6f) + (float(pResponse->WEIGHT.weightlow) / 10.0f);

	float AddjValue = 0.0f;
	float AddjMulti = 1.0f;
	m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	weight += AddjValue;

	sprintf(szTmp, "%.1f", weight);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->WEIGHT.subtype)
		{
		case sTypeWEIGHT1:
			WriteMessage("subtype       = BWR102");
			break;
		case sTypeWEIGHT2:
			WriteMessage("subtype       = GR101");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type = %02X:%02X", pResponse->WEIGHT.packettype, pResponse->WEIGHT.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->WEIGHT.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->WEIGHT.id1 * 256) + pResponse->WEIGHT.id2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Weight        = %.1f kg", (float(pResponse->WEIGHT.weighthigh) * 25.6f) + (float(pResponse->WEIGHT.weightlow) / 10));
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->WEIGHT.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_RFXSensor(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeRFXSensor;
	unsigned char subType = pResponse->RFXSENSOR.subtype;
	std::string ID;
	sprintf(szTmp, "%d", pResponse->RFXSENSOR.id);
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->RFXSENSOR.rssi;
	unsigned char BatteryLevel = 255;

	if ((HwdType == HTYPE_EnOceanESP2) || (HwdType == HTYPE_EnOceanESP3))
	{
		BatteryLevel = 255;
		SignalLevel = 12;
		Unit = (pResponse->RFXSENSOR.rssi << 4) | pResponse->RFXSENSOR.filler;
	}

	float temp;
	int volt = 0;
	switch (pResponse->RFXSENSOR.subtype)
	{
	case sTypeRFXSensorTemp:
	{
		if ((pResponse->RFXSENSOR.msg1 & 0x80) == 0) //positive temperature?
			temp = float((pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0f;
		else
			temp = -(float(((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
		float AddjValue = 0.0f;
		float AddjMulti = 1.0f;
		m_sql.GetAddjustment(HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
		sprintf(szTmp, "%.1f", temp);
	}
	break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
	{
		volt = (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2;
		if (
			(HwdType == HTYPE_RFXLAN) ||
			(HwdType == HTYPE_RFXtrx315) ||
			(HwdType == HTYPE_RFXtrx433) ||
			(HwdType == HTYPE_RFXtrx868)
			)
		{
			volt *= 10;
		}
		sprintf(szTmp, "%d", volt);
	}
	break;
	}
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	switch (pResponse->RFXSENSOR.subtype)
	{
	case sTypeRFXSensorTemp:
	{
		m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, temp, 0, true, false);
	}
	break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
	{
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, float(volt));
	}
	break;
	}

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->RFXSENSOR.subtype)
		{
		case sTypeRFXSensorTemp:
			WriteMessage("subtype       = Temperature");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);

			if ((pResponse->RFXSENSOR.msg1 & 0x80) == 0) //positive temperature?
			{
				sprintf(szTmp, "Temperature   = %.2f C", float((pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
				WriteMessage(szTmp);
			}
			else
			{
				sprintf(szTmp, "Temperature   = -%.2f C", float(((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0f);
				WriteMessage(szTmp);
			}
			break;
		case sTypeRFXSensorAD:
			WriteMessage("subtype       = A/D");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			sprintf(szTmp, "volt          = %d mV", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXSensorVolt:
			WriteMessage("subtype       = Voltage");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			sprintf(szTmp, "volt          = %d mV", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXSensorMessage:
			WriteMessage("subtype       = Message");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXSENSOR.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", pResponse->RFXSENSOR.id);
			WriteMessage(szTmp);
			switch (pResponse->RFXSENSOR.msg2)
			{
			case 0x1:
				WriteMessage("msg           = sensor addresses incremented");
				break;
			case 0x2:
				WriteMessage("msg           = battery low detected");
				break;
			case 0x81:
				WriteMessage("msg           = no 1-wire device connected");
				break;
			case 0x82:
				WriteMessage("msg           = 1-Wire ROM CRC error");
				break;
			case 0x83:
				WriteMessage("msg           = 1-Wire device connected is not a DS18B20 or DS2438");
				break;
			case 0x84:
				WriteMessage("msg           = no end of read signal received from 1-Wire device");
				break;
			case 0x85:
				WriteMessage("msg           = 1-Wire scratch pad CRC error");
				break;
			default:
				WriteMessage("ERROR: unknown message");
				break;
			}
			sprintf(szTmp, "msg           = %d", (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RFXSENSOR.packettype, pResponse->RFXSENSOR.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->RFXSENSOR.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_RFXMeter(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	uint64_t DevRowIdx = -1;
	char szTmp[100];
	unsigned char devType = pTypeRFXMeter;
	unsigned char subType = pResponse->RFXMETER.subtype;
	if (subType == sTypeRFXMeterCount)
	{
		std::string ID;
		sprintf(szTmp, "%d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
		ID = szTmp;
		unsigned char Unit = 0;
		unsigned char cmnd = 0;
		unsigned char SignalLevel = pResponse->RFXMETER.rssi;
		unsigned char BatteryLevel = 255;

		unsigned long counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
		//float RFXPwr = float(counter) / 1000.0f;

		sprintf(szTmp, "%lu", counter);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
	}

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		unsigned long counter;

		switch (pResponse->RFXMETER.subtype)
		{
		case sTypeRFXMeterCount:
			WriteMessage("subtype       = RFXMeter counter");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
			sprintf(szTmp, "Counter       = %lu", counter);
			WriteMessage(szTmp);
			sprintf(szTmp, "if RFXPwr     = %.3f kWh", float(counter) / 1000.0f);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterInterval:
			WriteMessage("subtype       = RFXMeter new interval time set");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			WriteMessage("Interval time = ", false);

			switch (pResponse->RFXMETER.count3)
			{
			case 0x1:
				WriteMessage("30 seconds");
				break;
			case 0x2:
				WriteMessage("1 minute");
				break;
			case 0x4:
				WriteMessage("6 minutes");
				break;
			case 0x8:
				WriteMessage("12 minutes");
				break;
			case 0x10:
				WriteMessage("15 minutes");
				break;
			case 0x20:
				WriteMessage("30 minutes");
				break;
			case 0x40:
				WriteMessage("45 minutes");
				break;
			case 0x80:
				WriteMessage("1 hour");
				break;
			}
			break;
		case sTypeRFXMeterCalib:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Calibrate mode for channel 1");
				break;
			case 0x40:
				WriteMessage("subtype       = Calibrate mode for channel 2");
				break;
			case 0x80:
				WriteMessage("subtype       = Calibrate mode for channel 3");
				break;
			}
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter = (((pResponse->RFXMETER.count2 & 0x3F) << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4) / 1000;
			sprintf(szTmp, "Calibrate cnt = %lu msec", counter);
			WriteMessage(szTmp);

			sprintf(szTmp, "RFXPwr        = %.3f kW", 1.0f / (float(16 * counter) / (3600000.0f / 62.5f)));
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterAddr:
			WriteMessage("subtype       = New address set, push button for next address");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterCounterReset:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 1 will be executed");
				break;
			case 0x40:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 2 will be executed");
				break;
			case 0x80:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else RESET COUNTER channel 3 will be executed");
				break;
			}
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterCounterSet:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Counter channel 1 is reset to zero");
				break;
			case 0x40:
				WriteMessage("subtype       = Counter channel 2 is reset to zero");
				break;
			case 0x80:
				WriteMessage("subtype       = Counter channel 3 is reset to zero");
				break;
			}
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
			sprintf(szTmp, "Counter       = %lu", counter);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetInterval:
			WriteMessage("subtype       = Push the button for next mode within 5 seconds or else SET INTERVAL MODE will be entered");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetCalib:
			switch (pResponse->RFXMETER.count2 & 0xC0)
			{
			case 0x0:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 1 will be executed");
				break;
			case 0x40:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 2 will be executed");
				break;
			case 0x80:
				WriteMessage("subtype       = Push the button for next mode within 5 seconds or else CALIBRATION mode for channel 3 will be executed");
				break;
			}
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterSetAddr:
			WriteMessage("subtype       = Push the button for next mode within 5 seconds or else SET ADDRESS MODE will be entered");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			break;
		case sTypeRFXMeterIdent:
			WriteMessage("subtype       = RFXMeter identification");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->RFXMETER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
			WriteMessage(szTmp);
			sprintf(szTmp, "FW version    = %02X", pResponse->RFXMETER.count3);
			WriteMessage(szTmp);
			WriteMessage("Interval time = ", false);

			switch (pResponse->RFXMETER.count4)
			{
			case 0x1:
				WriteMessage("30 seconds");
				break;
			case 0x2:
				WriteMessage("1 minute");
				break;
			case 0x4:
				WriteMessage("6 minutes");
				break;
			case 0x8:
				WriteMessage("12 minutes");
				break;
			case 0x10:
				WriteMessage("15 minutes");
				break;
			case 0x20:
				WriteMessage("30 minutes");
				break;
			case 0x40:
				WriteMessage("45 minutes");
				break;
			case 0x80:
				WriteMessage("1 hour");
				break;
			}
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->RFXMETER.packettype, pResponse->RFXMETER.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->RFXMETER.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_P1MeterPower(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tP1Power *p1Power = reinterpret_cast<const _tP1Power*>(pResponse);

	if (p1Power->len != sizeof(_tP1Power) - 1)
		return;

	unsigned char devType = p1Power->type;
	unsigned char subType = p1Power->subtype;
	std::string ID;
	sprintf(szTmp, "%d", p1Power->ID);
	ID = szTmp;

	unsigned char Unit = subType;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%u;%u;%u;%u;%u;%u",
		p1Power->powerusage1,
		p1Power->powerusage2,
		p1Power->powerdeliv1,
		p1Power->powerdeliv2,
		p1Power->usagecurrent,
		p1Power->delivcurrent
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (const float)p1Power->usagecurrent);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (p1Power->subtype)
		{
		case sTypeP1Power:
			WriteMessage("subtype       = P1 Smart Meter Power");

			sprintf(szTmp, "powerusage1 = %.3f kWh", float(p1Power->powerusage1) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp, "powerusage2 = %.3f kWh", float(p1Power->powerusage2) / 1000.0f);
			WriteMessage(szTmp);

			sprintf(szTmp, "powerdeliv1 = %.3f kWh", float(p1Power->powerdeliv1) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp, "powerdeliv2 = %.3f kWh", float(p1Power->powerdeliv2) / 1000.0f);
			WriteMessage(szTmp);

			sprintf(szTmp, "current usage = %03u Watt", p1Power->usagecurrent);
			WriteMessage(szTmp);
			sprintf(szTmp, "current deliv = %03u Watt", p1Power->delivcurrent);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", p1Power->type, p1Power->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_P1MeterGas(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tP1Gas *p1Gas = reinterpret_cast<const _tP1Gas*>(pResponse);
	if (p1Gas->len != sizeof(_tP1Gas) - 1)
		return;

	unsigned char devType = p1Gas->type;
	unsigned char subType = p1Gas->subtype;
	std::string ID;
	sprintf(szTmp, "%d", p1Gas->ID);
	ID = szTmp;
	unsigned char Unit = subType;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%u", p1Gas->gasusage);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (p1Gas->subtype)
		{
		case sTypeP1Gas:
			WriteMessage("subtype       = P1 Smart Meter Gas");

			sprintf(szTmp, "gasusage = %.3f m3", float(p1Gas->gasusage) / 1000.0f);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", p1Gas->type, p1Gas->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_YouLessMeter(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tYouLessMeter *pMeter = reinterpret_cast<const _tYouLessMeter*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;
	sprintf(szTmp, "%d", pMeter->ID1);
	std::string ID = szTmp;
	unsigned char Unit = subType;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%lu;%lu",
		pMeter->powerusage,
		pMeter->usagecurrent
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (const float)pMeter->usagecurrent);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeYouLess:
			WriteMessage("subtype       = YouLess Meter");

			sprintf(szTmp, "powerusage = %.3f kWh", float(pMeter->powerusage) / 1000.0f);
			WriteMessage(szTmp);
			sprintf(szTmp, "current usage = %03lu Watt", pMeter->usagecurrent);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Rego6XXTemp(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tRego6XXTemp *pRego = reinterpret_cast<const _tRego6XXTemp*>(pResponse);
	unsigned char devType = pRego->type;
	unsigned char subType = pRego->subtype;
	std::string ID = pRego->ID;
	unsigned char Unit = subType;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%.1f",
		pRego->temperature
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TEMPERATURE, pRego->temperature);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		WriteMessage("subtype       = Rego6XX Temp");

		sprintf(szTmp, "Temp = %.1f", pRego->temperature);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Rego6XXValue(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tRego6XXStatus *pRego = reinterpret_cast<const _tRego6XXStatus*>(pResponse);
	unsigned char devType = pRego->type;
	unsigned char subType = pRego->subtype;
	std::string ID = pRego->ID;
	unsigned char Unit = subType;
	int numValue = pRego->value;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%d",
		pRego->value
	);

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, numValue, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pRego->subtype)
		{
		case sTypeRego6XXStatus:
			WriteMessage("subtype       = Rego6XX Status");
			sprintf(szTmp, "Status = %d", pRego->value);
			WriteMessage(szTmp);
			break;
		case sTypeRego6XXCounter:
			WriteMessage("subtype       = Rego6XX Counter");
			sprintf(szTmp, "Counter = %d", pRego->value);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_AirQuality(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tAirQualityMeter *pMeter = reinterpret_cast<const _tAirQualityMeter*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;
	sprintf(szTmp, "%d", pMeter->id1);
	std::string ID = szTmp;
	unsigned char Unit = pMeter->id2;
	//unsigned char cmnd=0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->airquality, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (const float)pMeter->airquality);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeVoltcraft:
			WriteMessage("subtype       = Voltcraft CO-20");

			sprintf(szTmp, "CO2 = %d ppm", pMeter->airquality);
			WriteMessage(szTmp);
			if (pMeter->airquality < 700)
				strcpy(szTmp, "Quality = Excellent");
			else if (pMeter->airquality < 900)
				strcpy(szTmp, "Quality = Good");
			else if (pMeter->airquality < 1100)
				strcpy(szTmp, "Quality = Fair");
			else if (pMeter->airquality < 1600)
				strcpy(szTmp, "Quality = Mediocre");
			else
				strcpy(szTmp, "Quality = Bad");
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Usage(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tUsageMeter *pMeter = reinterpret_cast<const _tUsageMeter*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	unsigned char Unit = pMeter->dunit;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = 255;

	sprintf(szTmp, "%.1f", pMeter->fusage);

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->fusage);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeElectric:
			WriteMessage("subtype       = Electric");

			sprintf(szTmp, "Usage = %.1f W", pMeter->fusage);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lux(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tLightMeter *pMeter = reinterpret_cast<const _tLightMeter*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	unsigned char Unit = pMeter->dunit;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = pMeter->battery_level;

	sprintf(szTmp, "%.0f", pMeter->fLux);

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->fLux);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeLux:
			WriteMessage("subtype       = Lux");

			sprintf(szTmp, "Lux = %.1f W", pMeter->fLux);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Thermostat(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tThermostat *pMeter = reinterpret_cast<const _tThermostat*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	unsigned char Unit = pMeter->dunit;
	unsigned char cmnd = 0;
	unsigned char SignalLevel = 12;
	unsigned char BatteryLevel = pMeter->battery_level;

	switch (pMeter->subtype)
	{
	case sTypeThermSetpoint:
		sprintf(szTmp, "%.2f", pMeter->temp);
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
		WriteMessage(szTmp);
		return;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	if (pMeter->subtype == sTypeThermSetpoint)
	{
		m_notifications.CheckAndHandleTempHumidityNotification(DevRowIdx, procResult.DeviceName, pMeter->temp, 0, true, false);
	}

	//m_notifications.CheckAndHandleNotification(DevRowIdx, m_LastDeviceName,devType, subType, NTYPE_USAGE, pMeter->fLux);

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		double tvalue = ConvertTemperature(pMeter->temp, m_sql.m_tempsign[0]);
		switch (pMeter->subtype)
		{
		case sTypeThermSetpoint:
			WriteMessage("subtype       = SetPoint");
			sprintf(szTmp, "Temp = %.2f", tvalue);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_General(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult, const unsigned char SignalLevel, const unsigned char BatteryLevel)
{
	char szTmp[200];
	const _tGeneralDevice *pMeter = reinterpret_cast<const _tGeneralDevice*>(pResponse);
	unsigned char devType = pMeter->type;
	unsigned char subType = pMeter->subtype;

	if (
		(subType == sTypeVoltage) ||
		(subType == sTypeCurrent) ||
		(subType == sTypePercentage) ||
		(subType == sTypeWaterflow) ||
		(subType == sTypePressure) ||
		(subType == sTypeZWaveClock) ||
		(subType == sTypeZWaveThermostatMode) ||
		(subType == sTypeZWaveThermostatFanMode) ||
		(subType == sTypeFan) ||
		(subType == sTypeTextStatus) ||
		(subType == sTypeSoundLevel) ||
		(subType == sTypeBaro) ||
		(subType == sTypeDistance) ||
		(subType == sTypeSoilMoisture) ||
		(subType == sTypeCustom) ||
		(subType == sTypeKwh) ||
		(subType == sTypeZWaveAlarm)
		)
	{
		sprintf(szTmp, "%08X", (unsigned int)pMeter->intval1);
	}
	else
	{
		sprintf(szTmp, "%d", pMeter->id);

	}
	std::string ID = szTmp;
	unsigned char Unit = 1;
	unsigned char cmnd = 0;

	uint64_t DevRowIdx = -1;

	if (subType == sTypeVisibility)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		int meterType = 0;
		m_sql.GetMeterType(HwdID, ID.c_str(), Unit, devType, subType, meterType);
		float fValue = pMeter->floatval1;
		if (meterType == 1)
		{
			//miles
			fValue *= 0.6214f;
		}
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, fValue);
	}
	else if (subType == sTypeDistance)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		int meterType = 0;
		m_sql.GetMeterType(HwdID, ID.c_str(), Unit, devType, subType, meterType);
		float fValue = pMeter->floatval1;
		if (meterType == 1)
		{
			//inches
			fValue *= 0.393701f;
		}
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, fValue);
	}
	else if (subType == sTypeSolarRadiation)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeSoilMoisture)
	{
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->intval2, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (float)pMeter->intval2);
	}
	else if (subType == sTypeLeafWetness)
	{
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->intval1, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (float)pMeter->intval1);
	}
	else if (subType == sTypeVoltage)
	{
		sprintf(szTmp, "%.3f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeCurrent)
	{
		sprintf(szTmp, "%.3f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeBaro)
	{
		sprintf(szTmp, "%.02f;%d", pMeter->floatval1, pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypePressure)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypePercentage)
	{
		sprintf(szTmp, "%.2f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_PERCENTAGE, pMeter->floatval1);
	}
	else if (subType == sTypeWaterflow)
	{
		sprintf(szTmp, "%.2f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeFan)
	{
		sprintf(szTmp, "%d", pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_RPM, (float)pMeter->intval2);
	}
	else if (subType == sTypeSoundLevel)
	{
		sprintf(szTmp, "%d", pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, (float)pMeter->intval2);
	}
	else if (subType == sTypeZWaveClock)
	{
		int tintval = pMeter->intval2;
		int day = tintval / (24 * 60); tintval -= (day * 24 * 60);
		int hour = tintval / (60); tintval -= (hour * 60);
		int minute = tintval;
		sprintf(szTmp, "%d;%d;%d", day, hour, minute);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	}
	else if ((subType == sTypeZWaveThermostatMode) || (subType == sTypeZWaveThermostatFanMode))
	{
		cmnd = (unsigned char)pMeter->intval2;
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName);
	}
	else if (subType == sTypeKwh)
	{
		sprintf(szTmp, "%.3f;%.3f", pMeter->floatval1, pMeter->floatval2);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeAlert)
	{
		if (strcmp(pMeter->text, ""))
			sprintf(szTmp, "(%d) %s", pMeter->intval1, pMeter->text);
		else
			sprintf(szTmp, "%d", pMeter->intval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->intval1, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, static_cast<float>(pMeter->intval1));
	}
	else if (subType == sTypeCustom)
	{
		sprintf(szTmp, "%.4f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_USAGE, pMeter->floatval1);
	}
	else if (subType == sTypeZWaveAlarm)
	{
		Unit = pMeter->id;
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->intval2, procResult.DeviceName);
		if (DevRowIdx == -1)
			return;
		m_notifications.CheckAndHandleValueNotification(DevRowIdx, procResult.DeviceName, pMeter->intval2);
	}
	else if (subType == sTypeTextStatus)
	{
		DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->text, procResult.DeviceName);
	}

	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeVisibility:
			WriteMessage("subtype       = Visibility");
			sprintf(szTmp, "Visibility = %.1f km", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeDistance:
			WriteMessage("subtype       = Distance");
			sprintf(szTmp, "Distance = %.1f cm", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeSolarRadiation:
			WriteMessage("subtype       = Solar Radiation");
			sprintf(szTmp, "Radiation = %.1f Watt/m2", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeSoilMoisture:
			WriteMessage("subtype       = Soil Moisture");
			sprintf(szTmp, "Moisture = %d cb", pMeter->intval2);
			WriteMessage(szTmp);
			break;
		case sTypeLeafWetness:
			WriteMessage("subtype       = Leaf Wetness");
			sprintf(szTmp, "Wetness = %d", pMeter->intval1);
			WriteMessage(szTmp);
			break;
		case sTypeVoltage:
			WriteMessage("subtype       = Voltage");
			sprintf(szTmp, "Voltage = %.3f V", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeCurrent:
			WriteMessage("subtype       = Current");
			sprintf(szTmp, "Current = %.3f V", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypePressure:
			WriteMessage("subtype       = Pressure");
			sprintf(szTmp, "Pressure = %.1f bar", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeBaro:
			WriteMessage("subtype       = Barometric Pressure");
			sprintf(szTmp, "Pressure = %.1f hPa", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeFan:
			WriteMessage("subtype       = Fan");
			sprintf(szTmp, "Speed = %d RPM", pMeter->intval2);
			WriteMessage(szTmp);
			break;
		case sTypeSoundLevel:
			WriteMessage("subtype       = Sound Level");
			sprintf(szTmp, "Sound Level = %d dB", pMeter->intval2);
			WriteMessage(szTmp);
			break;
		case sTypeZWaveClock:
		{
			int tintval = pMeter->intval2;
			int day = tintval / (24 * 60); tintval -= (day * 24 * 60);
			int hour = tintval / (60); tintval -= (hour * 60);
			int minute = tintval;
			WriteMessage("subtype       = Thermostat Clock");
			sprintf(szTmp, "Clock = %s %02d:%02d", ZWave_Clock_Days(day), hour, minute);
			WriteMessage(szTmp);
		}
		break;
		case sTypeZWaveThermostatMode:
			WriteMessage("subtype       = Thermostat Mode");
			//sprintf(szTmp, "Mode = %d (%s)", pMeter->intval2, ZWave_Thermostat_Modes[pMeter->intval2]);
			sprintf(szTmp, "Mode = %d", pMeter->intval2);
			WriteMessage(szTmp);
			break;
		case sTypeZWaveThermostatFanMode:
			WriteMessage("subtype       = Thermostat Fan Mode");
			sprintf(szTmp, "Mode = %d (%s)", pMeter->intval2, ZWave_Thermostat_Fan_Modes[pMeter->intval2]);
			WriteMessage(szTmp);
			break;
		case sTypePercentage:
			WriteMessage("subtype       = Percentage");
			sprintf(szTmp, "Percentage = %.2f", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeWaterflow:
			WriteMessage("subtype       = Waterflow");
			sprintf(szTmp, "l/min = %.2f", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeKwh:
			WriteMessage("subtype       = kWh");
			sprintf(szTmp, "Instant = %.3f", pMeter->floatval1);
			WriteMessage(szTmp);
			sprintf(szTmp, "Counter = %.3f", pMeter->floatval2 / 1000.0f);
			WriteMessage(szTmp);
			break;
		case sTypeAlert:
			WriteMessage("subtype       = Alert");
			sprintf(szTmp, "Alert = %d", pMeter->intval1);
			WriteMessage(szTmp);
			break;
		case sTypeCustom:
			WriteMessage("subtype       = Custom Sensor");
			sprintf(szTmp, "Value = %g", pMeter->floatval1);
			WriteMessage(szTmp);
			break;
		case sTypeZWaveAlarm:
			WriteMessage("subtype       = Alarm");
			sprintf(szTmp, "Level = %d (0x%02X)", pMeter->intval2, pMeter->intval2);
			WriteMessage(szTmp);
			break;
		case sTypeTextStatus:
			WriteMessage("subtype       = Text");
			sprintf(szTmp, "Text = %s", pMeter->text);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
			WriteMessage(szTmp);
			break;
		}
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_GeneralSwitch(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[200];
	const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pResponse);
	unsigned char devType = pSwitch->type;
	unsigned char subType = pSwitch->subtype;

	sprintf(szTmp, "%08X", pSwitch->id);
	std::string ID = szTmp;
	unsigned char Unit = pSwitch->unitcode;
	unsigned char cmnd = pSwitch->cmnd;
	unsigned char level = pSwitch->level;
	unsigned char SignalLevel = pSwitch->rssi;

	sprintf(szTmp, "%d", level);
	uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	unsigned char check_cmnd = cmnd;
	if ((cmnd == gswitch_sGroupOff) || (cmnd == gswitch_sGroupOn))
		check_cmnd = (cmnd == gswitch_sGroupOff) ? gswitch_sOff : gswitch_sOn;
	CheckSceneCode(DevRowIdx, devType, subType, check_cmnd, szTmp);

	if ((cmnd == gswitch_sGroupOff) || (cmnd == gswitch_sGroupOn))
	{
		//set the status of all lights with the same code to on/off
		m_sql.GeneralSwitchGroupCmd(ID, subType, (cmnd == gswitch_sGroupOff) ? gswitch_sOff : gswitch_sOn);
	}

	procResult.DeviceRowIdx = DevRowIdx;
}

//BBQ sensor has two temperature sensors, add them as two temperature devices
void MainWorker::decode_BBQ(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	unsigned char devType = pTypeBBQ;
	unsigned char subType = pResponse->BBQ.subtype;

	sprintf(szTmp, "%d", 1);//(pResponse->BBQ.id1 * 256) + pResponse->BBQ.id2); //this because every time you turn the device on, you get a new ID
	std::string ID = szTmp;

	unsigned char Unit = pResponse->BBQ.id2;

	unsigned char cmnd = 0;
	unsigned char SignalLevel = pResponse->BBQ.rssi;
	unsigned char BatteryLevel = 0;
	if ((pResponse->BBQ.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	uint64_t DevRowIdx = 0;

	float temp1, temp2;
	temp1 = float((pResponse->BBQ.sensor1h * 256) + pResponse->BBQ.sensor1l);// / 10.0f;
	temp2 = float((pResponse->BBQ.sensor2h * 256) + pResponse->BBQ.sensor2l);// / 10.0f;

	sprintf(szTmp, "%.0f;%.0f", temp1, temp2);
	DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;
	if (m_verboselevel >= EVBL_ALL)
	{
		WriteMessageStart();
		switch (pResponse->BBQ.subtype)
		{
		case sTypeBBQ1:
			WriteMessage("subtype       = Maverick ET-732");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->BBQ.packettype, pResponse->BBQ.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->BBQ.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "ID            = %d", (pResponse->BBQ.id1 * 256) + pResponse->BBQ.id2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Sensor1 Temp  = %.1f C", temp1);
		WriteMessage(szTmp);
		sprintf(szTmp, "Sensor2 Temp  = %.1f C", temp2);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->BBQ.rssi);
		WriteMessage(szTmp);

		if ((pResponse->BBQ.battery_level & 0x0F) == 0)
			WriteMessage("Battery       = Low");
		else
			WriteMessage("Battery       = OK");
		WriteMessageEnd();
	}

	//Send the two sensors

	//Temp
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP6;
	tsen.TEMP.battery_level = 9;
	tsen.TEMP.rssi = SignalLevel;
	tsen.TEMP.id1 = 0;
	tsen.TEMP.id2 = 1;

	tsen.TEMP.tempsign = (temp1 >= 0) ? 0 : 1;
	int at10 = round(std::abs(temp1*10.0f));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	_tRxMessageProcessingResult tmpProcResult1;
	tmpProcResult1.DeviceName = "";
	tmpProcResult1.DeviceRowIdx = -1;
	decode_Temp(HwdID, HwdType, (const tRBUF*)&tsen.TEMP, tmpProcResult1);

	tsen.TEMP.id1 = 0;
	tsen.TEMP.id2 = 2;

	tsen.TEMP.tempsign = (temp2 >= 0) ? 0 : 1;
	at10 = round(std::abs(temp2*10.0f));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	_tRxMessageProcessingResult tmpProcResult2;
	tmpProcResult2.DeviceName = "";
	tmpProcResult2.DeviceRowIdx = -1;
	decode_Temp(HwdID, HwdType, (const tRBUF*)&tsen.TEMP, tmpProcResult2);

	procResult.DeviceRowIdx = DevRowIdx;
}

//not in dbase yet
void MainWorker::decode_FS20(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	//unsigned char devType=pTypeFS20;

	char szTmp[100];

	switch (pResponse->FS20.subtype)
	{
	case sTypeFS20:
		WriteMessage("subtype       = FS20");
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		switch (pResponse->FS20.cmd1 & 0x1F)
		{
		case 0x0:
			WriteMessage("Off");
			break;
		case 0x1:
			WriteMessage("dim level 1 = 6.25%");
			break;
		case 0x2:
			WriteMessage("dim level 2 = 12.5%");
			break;
		case 0x3:
			WriteMessage("dim level 3 = 18.75%");
			break;
		case 0x4:
			WriteMessage("dim level 4 = 25%");
			break;
		case 0x5:
			WriteMessage("dim level 5 = 31.25%");
			break;
		case 0x6:
			WriteMessage("dim level 6 = 37.5%");
			break;
		case 0x7:
			WriteMessage("dim level 7 = 43.75%");
			break;
		case 0x8:
			WriteMessage("dim level 8 = 50%");
			break;
		case 0x9:
			WriteMessage("dim level 9 = 56.25%");
			break;
		case 0xA:
			WriteMessage("dim level 10 = 62.5%");
			break;
		case 0xB:
			WriteMessage("dim level 11 = 68.75%");
			break;
		case 0xC:
			WriteMessage("dim level 12 = 75%");
			break;
		case 0xD:
			WriteMessage("dim level 13 = 81.25%");
			break;
		case 0xE:
			WriteMessage("dim level 14 = 87.5%");
			break;
		case 0xF:
			WriteMessage("dim level 15 = 93.75%");
			break;
		case 0x10:
			WriteMessage("On (100%)");
			break;
		case 0x11:
			WriteMessage("On ( at last dim level set)");
			break;
		case 0x12:
			WriteMessage("Toggle between Off and On (last dim level set)");
			break;
		case 0x13:
			WriteMessage("Bright one step");
			break;
		case 0x14:
			WriteMessage("Dim one step");
			break;
		case 0x15:
			WriteMessage("Start dim cycle");
			break;
		case 0x16:
			WriteMessage("Program(Timer)");
			break;
		case 0x17:
			WriteMessage("Request status from a bidirectional device");
			break;
		case 0x18:
			WriteMessage("Off for timer period");
			break;
		case 0x19:
			WriteMessage("On (100%) for timer period");
			break;
		case 0x1A:
			WriteMessage("On ( at last dim level set) for timer period");
			break;
		case 0x1B:
			WriteMessage("Reset");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}

		if ((pResponse->FS20.cmd1 & 0x80) == 0)
			WriteMessage("                command to receiver");
		else
			WriteMessage("                response from receiver");

		if ((pResponse->FS20.cmd1 & 0x40) == 0)
			WriteMessage("                unidirectional command");
		else
			WriteMessage("                bidirectional command");

		if ((pResponse->FS20.cmd1 & 0x20) == 0)
			WriteMessage("                additional cmd2 byte not present");
		else
			WriteMessage("                additional cmd2 byte present");

		if ((pResponse->FS20.cmd1 & 0x20) != 0)
		{
			sprintf(szTmp, "Cmd2          = %02X", pResponse->FS20.cmd2);
			WriteMessage(szTmp);
		}
		break;
	case sTypeFHT8V:
		WriteMessage("subtype       = FHT 8V valve");

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		if ((pResponse->FS20.cmd1 & 0x80) == 0)
			WriteMessage("new command");
		else
			WriteMessage("repeated command");

		if ((pResponse->FS20.cmd1 & 0x40) == 0)
			WriteMessage("                unidirectional command");
		else
			WriteMessage("                bidirectional command");

		if ((pResponse->FS20.cmd1 & 0x20) == 0)
			WriteMessage("                additional cmd2 byte not present");
		else
			WriteMessage("                additional cmd2 byte present");

		if ((pResponse->FS20.cmd1 & 0x10) == 0)
			WriteMessage("                battery empty beep not enabled");
		else
			WriteMessage("                enable battery empty beep");

		switch (pResponse->FS20.cmd1 & 0xF)
		{
		case 0x0:
			WriteMessage("                Synchronize now");
			sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0x1:
			WriteMessage("                open valve");
			break;
		case 0x2:
			WriteMessage("                close valve");
			break;
		case 0x6:
			WriteMessage("                open valve at percentage level");
			sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0x8:
			WriteMessage("                relative offset (cmd2 bit 7=direction, bit 5-0 offset value)");
			break;
		case 0xA:
			WriteMessage("                decalcification cycle");
			sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55f);
			WriteMessage(szTmp);
			break;
		case 0xC:
			WriteMessage("                synchronization active");
			sprintf(szTmp, "Cmd2          = count down is %d seconds", pResponse->FS20.cmd2 >> 1);
			WriteMessage(szTmp);
			break;
		case 0xE:
			WriteMessage("                test, drive valve and produce an audible signal");
			break;
		case 0xF:
			WriteMessage("                pair valve (cmd2 bit 7-1 is count down in seconds, bit 0=1)");
			sprintf(szTmp, "Cmd2          = count down is %d seconds", pResponse->FS20.cmd2 >> 1);
			WriteMessage(szTmp);
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}
		break;
	case sTypeFHT80:
		WriteMessage("subtype       = FHT80 door/window sensor");
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->FS20.seqnbr);
		WriteMessage(szTmp);
		sprintf(szTmp, "House code    = %02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
		WriteMessage(szTmp);
		sprintf(szTmp, "Address       = %02X", pResponse->FS20.addr);
		WriteMessage(szTmp);

		WriteMessage("Cmd1          = ", false);

		switch (pResponse->FS20.cmd1 & 0xF)
		{
		case 0x1:
			WriteMessage("sensor opened");
			break;
		case 0x2:
			WriteMessage("sensor closed");
			break;
		case 0xC:
			WriteMessage("synchronization active");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown command = %02X", pResponse->FS20.cmd1);
			WriteMessage(szTmp);
			break;
		}

		if ((pResponse->FS20.cmd1 & 0x80) == 0)
			WriteMessage("                new command");
		else
			WriteMessage("                repeated command");
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->FS20.packettype, pResponse->FS20.subtype);
		WriteMessage(szTmp);
		break;
	}

	sprintf(szTmp, "Signal level  = %d", pResponse->FS20.rssi);
	WriteMessage(szTmp);
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Cartelectronic(const int HwdID, const _eHardwareTypes HwdType, const tRBUF *pResponse, _tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	std::string ID;

	sprintf(szTmp, "%llu", ((unsigned long long)(pResponse->TIC.id1) << 32) + (pResponse->TIC.id2 << 24) + (pResponse->TIC.id3 << 16) + (pResponse->TIC.id4 << 8) + (pResponse->TIC.id5));
	ID = szTmp;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;

	unsigned char subType = pResponse->TIC.subtype;

	switch (subType)
	{
	case sTypeTIC:
		decode_CartelectronicTIC(HwdID, pResponse, procResult);
		WriteMessage("Cartelectronic TIC received");
		break;
	case sTypeCEencoder:
		decode_CartelectronicEncoder(HwdID, pResponse, procResult);
		WriteMessage("Cartelectronic Encoder received");
		break;
	default:
		WriteMessage("Cartelectronic protocol not supported");
		break;
	}
}

void MainWorker::decode_CartelectronicTIC(const int HwdID,
	const tRBUF *pResponse,
	_tRxMessageProcessingResult & procResult)
{
	//Contract Options
	typedef enum {
		OP_NOT_DEFINED,
		OP_BASE,
		OP_CREUSE,
		OP_EJP,
		OP_TEMPO
	} Contract;

	//Running time
	typedef enum {
		PER_NOT_DEFINED,
		PER_ALL_HOURS,             //TH..
		PER_LOWCOST_HOURS,         //HC..
		PER_HIGHCOST_HOURS,        //HP..
		PER_NORMAL_HOURS,          //HN..
		PER_MOBILE_PEAK_HOURS,     //PM..
		PER_BLUE_LOWCOST_HOURS,    //HCJB
		PER_WHITE_LOWCOST_HOURS,   //HCJW
		PER_RED_LOWCOST_HOURS,     //HCJR
		PER_BLUE_HIGHCOST_HOURS,   //HPJB
		PER_WHITE_HIGHCOST_HOURS,  //HPJW
		PER_RED_HIGHCOST_HOURS     //HPJR
	} PeriodTime;

	char szTmp[100];
	uint32_t counter1 = 0;
	uint32_t counter2 = 0;
	unsigned int apparentPower = 0;
	unsigned int counter1ApparentPower = 0;
	unsigned int counter2ApparentPower = 0;
	unsigned char unitCounter1 = 0;
	unsigned char unitCounter2 = 1;
	unsigned char cmnd = 0;
	std::string ID;

	unsigned char devType = pTypeGeneral;
	unsigned char subType = sTypeKwh;
	unsigned char SignalLevel = pResponse->TIC.rssi;

	unsigned char BatteryLevel = (pResponse->TIC.battery_level + 1) * 10;

	// If no error
	if ((pResponse->TIC.state & 0x04) == 0)
	{
		// Id of the counter
		uint64_t uint64ID = ((uint64_t)pResponse->TIC.id1 << 32) +
			((uint64_t)pResponse->TIC.id2 << 24) +
			((uint64_t)pResponse->TIC.id3 << 16) +
			((uint64_t)pResponse->TIC.id4 << 8) +
			(uint64_t)(pResponse->TIC.id5);

		sprintf(szTmp, "%.12" PRIu64, uint64ID);
		ID = szTmp;

		// Contract Type
		Contract contractType = (Contract)(pResponse->TIC.contract_type >> 4);

		// Period
		PeriodTime period = (PeriodTime)(pResponse->TIC.contract_type & 0x0f);

		// Apparent Power
		if ((pResponse->TIC.state & 0x02) != 0) // Only if this one is present
		{
			apparentPower = (pResponse->TIC.power_H << 8) + pResponse->TIC.power_L;

			sprintf(szTmp, "%u", apparentPower);
			uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 1, pTypeUsage, sTypeElectric, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);

			if (DevRowIdx == -1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, pTypeUsage, sTypeElectric, NTYPE_ENERGYINSTANT, (const float)apparentPower);
		}

		switch (contractType)
		{
			// BASE CONTRACT
		case OP_BASE:
		{
			unitCounter1 = 0;
			counter1ApparentPower = apparentPower;
		}
		break;

		// LOW/HIGH PERIOD
		case OP_CREUSE:
		{
			unitCounter1 = 0;

			if (period == PER_LOWCOST_HOURS)
			{
				counter1ApparentPower = apparentPower;
				counter2ApparentPower = 0;
			}
			if (period == PER_HIGHCOST_HOURS)
			{
				counter1ApparentPower = 0;
				counter2ApparentPower = apparentPower;
			}

			// Counter 2
			counter2 = (pResponse->TIC.counter2_0 << 24) + (pResponse->TIC.counter2_1 << 16) + (pResponse->TIC.counter2_2 << 8) + (pResponse->TIC.counter2_3);
			sprintf(szTmp, "%u;%d", counter2ApparentPower, counter2);

			uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);

			if (DevRowIdx == -1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (const float)counter2);
			//----------------------------
		}
		break;

		// EJP
		case OP_EJP:
		{
			unitCounter1 = 0;
			// Counter 2
			counter2 = (pResponse->TIC.counter2_0 << 24) + (pResponse->TIC.counter2_1 << 16) + (pResponse->TIC.counter2_2 << 8) + (pResponse->TIC.counter2_3);

			sprintf(szTmp, "%u;%d", counter2ApparentPower, counter2);
			uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);

			if (DevRowIdx == -1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (const float)counter2);
			//----------------------------
		}
		break;

		// TEMPO Contracts
		// Total of 6 counters. Theses counters depends of the time period received. For each period time, only 2 counters are sended.
		case OP_TEMPO:
		{
			switch (period)
			{
			case PER_BLUE_LOWCOST_HOURS:
				counter1ApparentPower = apparentPower;
				counter2ApparentPower = 0;
				unitCounter1 = 0;
				unitCounter2 = 1;
				break;
			case PER_BLUE_HIGHCOST_HOURS:
				counter1ApparentPower = 0;
				counter2ApparentPower = apparentPower;
				unitCounter1 = 0;
				unitCounter2 = 1;
				break;
			case PER_WHITE_LOWCOST_HOURS:
				counter1ApparentPower = apparentPower;
				counter2ApparentPower = 0;
				unitCounter1 = 2;
				unitCounter2 = 3;
				break;
			case PER_WHITE_HIGHCOST_HOURS:
				counter1ApparentPower = 0;
				counter2ApparentPower = apparentPower;
				unitCounter1 = 2;
				unitCounter2 = 3;
				break;
			case PER_RED_LOWCOST_HOURS:
				counter1ApparentPower = apparentPower;
				counter2ApparentPower = 0;
				unitCounter1 = 4;
				unitCounter2 = 5;
				break;
			case PER_RED_HIGHCOST_HOURS:
				counter1ApparentPower = 0;
				counter2ApparentPower = apparentPower;
				unitCounter1 = 4;
				unitCounter2 = 5;
				break;
			default:
				break;
			}

			// Counter 2
			counter2 = (pResponse->TIC.counter2_0 << 24) + (pResponse->TIC.counter2_1 << 16) + (pResponse->TIC.counter2_2 << 8) + (pResponse->TIC.counter2_3);

			sprintf(szTmp, "%u;%d", counter2ApparentPower, counter2);
			uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), unitCounter2, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);

			if (DevRowIdx == -1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (const float)counter2);
			//----------------------------
		}
		break;
		default:
			break;
		}

		// Counter 1
		counter1 = (pResponse->TIC.counter1_0 << 24) + (pResponse->TIC.counter1_1 << 16) + (pResponse->TIC.counter1_2 << 8) + (pResponse->TIC.counter1_3);

		sprintf(szTmp, "%u;%d", counter1ApparentPower, counter1);
		uint64_t DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), unitCounter1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);

		if (DevRowIdx == -1)
			return;

		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (const float)counter1);
		//----------------------------
	}
	else
	{
		WriteMessage("TeleInfo not connected");
	}
}

void MainWorker::decode_CartelectronicEncoder(const int HwdID,
	const tRBUF *pResponse,
	_tRxMessageProcessingResult & procResult)
{
	char szTmp[100];
	uint32_t counter1 = 0;
	uint32_t counter2 = 0;
	int apparentPower = 0;
	unsigned char Unit = 0;
	unsigned char cmnd = 0;
	uint64_t DevRowIdx = 0;
	std::string ID;

	unsigned char devType = pTypeRFXMeter;
	unsigned char subType = sTypeRFXMeterCount;
	unsigned char SignalLevel = pResponse->CEENCODER.rssi;

	unsigned char BatteryLevel = (pResponse->CEENCODER.battery_level + 1) * 10;

	// Id of the module
	sprintf(szTmp, "%d", ((uint32_t)(pResponse->CEENCODER.id1 << 24) + (pResponse->CEENCODER.id2 << 16) + (pResponse->CEENCODER.id3 << 8) + pResponse->CEENCODER.id4));
	ID = szTmp;

	// Counter 1
	counter1 = (pResponse->CEENCODER.counter1_0 << 24) + (pResponse->CEENCODER.counter1_1 << 16) + (pResponse->CEENCODER.counter1_2 << 8) + (pResponse->CEENCODER.counter1_3);

	sprintf(szTmp, "%d", counter1);
	DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYCOUNTER, (const float)counter1);

	// Counter 2
	counter2 = (pResponse->CEENCODER.counter2_0 << 24) + (pResponse->CEENCODER.counter2_1 << 16) + (pResponse->CEENCODER.counter2_2 << 8) + (pResponse->CEENCODER.counter2_3);

	sprintf(szTmp, "%d", counter2);
	DevRowIdx = m_sql.UpdateValue(HwdID, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName);
	if (DevRowIdx == -1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYCOUNTER, (const float)counter2);
}

bool MainWorker::GetSensorData(const uint64_t idx, int &nValue, std::string &sValue)
{
	std::vector<std::vector<std::string> > result;
	char szTmp[100];
	result = m_sql.safe_query("SELECT [Type],[SubType],[nValue],[sValue],[SwitchType] FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
	if (result.empty())
		return false;
	std::vector<std::string> sd = result[0];
	int devType = atoi(sd[0].c_str());
	int subType = atoi(sd[1].c_str());
	nValue = atoi(sd[2].c_str());
	sValue = sd[3];
	_eMeterType metertype = (_eMeterType)atoi(sd[4].c_str());

	//Special cases
	if ((devType == pTypeP1Power) && (subType == sTypeP1Power))
	{
		std::vector<std::string> results;
		StringSplit(sValue, ";", results);
		if (results.size() < 6)
			return false; //invalid data
		//Return usage or delivery
		long usagecurrent = atol(results[4].c_str());
		long delivcurrent = atol(results[5].c_str());
		std::stringstream ssvalue;
		if (delivcurrent > 0)
		{
			ssvalue << "-" << delivcurrent;
		}
		else
		{
			ssvalue << usagecurrent;
		}
		nValue = 0;
		sValue = ssvalue.str();
	}
	else if ((devType == pTypeP1Gas) && (subType == sTypeP1Gas))
	{
		float GasDivider = 1000.0f;
		//get lowest value of today
		time_t now = mytime(NULL);
		struct tm tm1;
		localtime_r(&now, &tm1);

		struct tm ltime;
		ltime.tm_isdst = tm1.tm_isdst;
		ltime.tm_hour = 0;
		ltime.tm_min = 0;
		ltime.tm_sec = 0;
		ltime.tm_year = tm1.tm_year;
		ltime.tm_mon = tm1.tm_mon;
		ltime.tm_mday = tm1.tm_mday;

		char szDate[40];
		sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

		std::vector<std::vector<std::string> > result2;
		strcpy(szTmp, "0");
		result2 = m_sql.safe_query("SELECT MIN(Value) FROM Meter WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q')", idx, szDate);
		if (result2.size() > 0)
		{
			std::vector<std::string> sd2 = result2[0];

			unsigned long long total_min_gas, total_real_gas;
			unsigned long long gasactual;

			std::stringstream s_str1(sd2[0]);
			s_str1 >> total_min_gas;
			std::stringstream s_str2(sValue);
			s_str2 >> gasactual;
			total_real_gas = gasactual - total_min_gas;
			float musage = float(total_real_gas) / GasDivider;
			sprintf(szTmp, "%.03f", musage);
		}
		else
		{
			sprintf(szTmp, "%.03f", 0.0f);
		}
		nValue = 0;
		sValue = szTmp;
	}
	else if (devType == pTypeRFXMeter)
	{
		float EnergyDivider = 1000.0f;
		float GasDivider = 100.0f;
		//float WaterDivider = 100.0f;
		int tValue;
		if (m_sql.GetPreferencesVar("MeterDividerEnergy", tValue))
		{
			EnergyDivider = float(tValue);
		}
		if (m_sql.GetPreferencesVar("MeterDividerGas", tValue))
		{
			GasDivider = float(tValue);
		}
		//if (m_sql.GetPreferencesVar("MeterDividerWater", tValue))
		//{
			//WaterDivider = float(tValue);
		//}

		//get value of today
		time_t now = mytime(NULL);
		struct tm tm1;
		localtime_r(&now, &tm1);

		struct tm ltime;
		ltime.tm_isdst = tm1.tm_isdst;
		ltime.tm_hour = 0;
		ltime.tm_min = 0;
		ltime.tm_sec = 0;
		ltime.tm_year = tm1.tm_year;
		ltime.tm_mon = tm1.tm_mon;
		ltime.tm_mday = tm1.tm_mday;

		char szDate[40];
		sprintf(szDate, "%04d-%02d-%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);

		std::vector<std::vector<std::string> > result2;
		strcpy(szTmp, "0");
		result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID='%" PRIu64 "' AND Date>='%q')", idx, szDate);
		if (result2.size() > 0)
		{
			std::vector<std::string> sd2 = result2[0];

			unsigned long long total_min, total_max, total_real;

			std::stringstream s_str1(sd2[0]);
			s_str1 >> total_min;
			std::stringstream s_str2(sd2[1]);
			s_str2 >> total_max;
			total_real = total_max - total_min;
			sprintf(szTmp, "%llu", total_real);

			float musage = 0;
			switch (metertype)
			{
			case MTYPE_ENERGY:
			case MTYPE_ENERGY_GENERATED:
				musage = float(total_real) / EnergyDivider;
				sprintf(szTmp, "%.03f", musage);
				break;
			case MTYPE_GAS:
				musage = float(total_real) / GasDivider;
				sprintf(szTmp, "%.03f", musage);
				break;
			case MTYPE_WATER:
				sprintf(szTmp, "%llu", total_real);
				break;
			case MTYPE_COUNTER:
				sprintf(szTmp, "%llu", total_real);
				break;
				/*
							default:
								strcpy(szTmp, "?");
								break;
				*/
			}
		}
		nValue = 0;
		sValue = szTmp;
	}
	return true;
}

bool MainWorker::SetRFXCOMHardwaremodes(const int HardwareID, const unsigned char Mode1, const unsigned char Mode2, const unsigned char Mode3, const unsigned char Mode4, const unsigned char Mode5, const unsigned char Mode6)
{
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;
	m_hardwaredevices[hindex]->m_rxbufferpos = 0;
	tRBUF Response;
	Response.ICMND.packetlength = sizeof(Response.ICMND) - 1;
	Response.ICMND.packettype = pTypeInterfaceControl;
	Response.ICMND.subtype = sTypeInterfaceCommand;
	Response.ICMND.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
	Response.ICMND.cmnd = cmdSETMODE;
	Response.ICMND.freqsel = Mode1;
	Response.ICMND.xmitpwr = Mode2;
	Response.ICMND.msg3 = Mode3;
	Response.ICMND.msg4 = Mode4;
	Response.ICMND.msg5 = Mode5;
	Response.ICMND.msg6 = Mode6;
	if (!WriteToHardware(HardwareID, (const char*)&Response, sizeof(Response.ICMND)))
		return false;
	PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&Response, NULL, -1);
	//Save it also
	SendCommand(HardwareID, cmdSAVE, "Save Settings");

	m_hardwaredevices[hindex]->m_rxbufferpos = 0;

	return true;
}

bool MainWorker::SwitchLightInt(const std::vector<std::string> &sd, std::string switchcmd, int level, int hue, const bool IsTesting)
{
	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	unsigned char ID1 = (unsigned char)((ID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((ID & 0x00FF0000) >> 16);
	unsigned char ID3 = (unsigned char)((ID & 0x0000FF00) >> 8);
	unsigned char ID4 = (unsigned char)((ID & 0x000000FF));

	int HardwareID = atoi(sd[0].c_str());

	if (_log.isTraceEnabled()) _log.Log(LOG_TRACE, "MAIN SwitchLightInt : switchcmd:%s level:%d HWid:%d  sd:%s %s %s %s %s %s", switchcmd.c_str(), level, HardwareID,
		sd[0].c_str(), sd[1].c_str(), sd[2].c_str(), sd[3].c_str(), sd[4].c_str(), sd[5].c_str());

	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
	{
		_log.Log(LOG_ERROR, "Switch command not send!, Hardware device disabled or not found!");
		return false;
	}
	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;

	if (pHardware->HwdType == HTYPE_DomoticzInternal)
	{
		//Special cases
		if (ID == 0x00148702)
		{
			int iSecStatus = 2;
			if (switchcmd == "Disarm")
				iSecStatus = 0;
			else if (switchcmd == "Arm Home")
				iSecStatus = 1;
			else if (switchcmd == "Arm Away")
				iSecStatus = 2;
			else
				return false;
			UpdateDomoticzSecurityStatus(iSecStatus);
			return true;
		}
	}

	unsigned char Unit = atoi(sd[2].c_str());
	unsigned char dType = atoi(sd[3].c_str());
	unsigned char dSubType = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());
	std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[10].c_str());

	//when asking for Toggle, just switch to the opposite value
	if (switchcmd == "Toggle") {
		switchcmd = (atoi(sd[7].c_str()) == 1 ? "Off" : "On");
	}

	//adjust level
	if (switchcmd=="Set Level" ||
		switchcmd=="On")
	{
		if (
			(level > 0) &&
			(switchtype != STYPE_Selector)
			)
		{
			level -= 1;
		}
		//when level = 0 and command is "Set Level", set switch command to Off
		if (level==0 && switchcmd=="Set Level")
			switchcmd="Off";
	}

	//when level = 0 and command is "On" replace level with "LastLevel"
	if (switchcmd=="On" && level == 0 && switchtype != STYPE_Selector)
	{
		//Get LastLevel
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
		"SELECT LastLevel FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, sd[1].c_str(), Unit, int(dType), int(dSubType));
		if (result.size() == 1)
		{
			level = atoi(result[0][0].c_str());
		}
	}

	//
	//	For plugins all the specific logic below is irrelevent
	//	so just send the full details to the plugin so that it can take appropriate action
	//
	if (pHardware->HwdType == HTYPE_PythonPlugin)
	{
#ifdef ENABLE_PYTHON
		((Plugins::CPlugin*)m_hardwaredevices[hindex])->SendCommand(Unit, switchcmd, level, hue);
#endif
		return true;
	}

	switch (dType)
	{
	case pTypeLighting1:
	{
		tRBUF lcmd;
		lcmd.LIGHTING1.packetlength = sizeof(lcmd.LIGHTING1) - 1;
		lcmd.LIGHTING1.packettype = dType;
		lcmd.LIGHTING1.subtype = dSubType;
		lcmd.LIGHTING1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.LIGHTING1.housecode = atoi(sd[1].c_str());
		lcmd.LIGHTING1.unitcode = Unit;
		lcmd.LIGHTING1.filler = 0;
		lcmd.LIGHTING1.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.LIGHTING1.cmnd, options))
			return false;
		if (switchtype == STYPE_Doorbell)
		{
			int rnvalue = 0;
			m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
			if (rnvalue == 0)
				lcmd.LIGHTING1.cmnd = light1_sChime;
			else
				lcmd.LIGHTING1.cmnd = light1_sOn;
		}

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING1)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeLighting2:
	{
		tRBUF lcmd;
		lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
		lcmd.LIGHTING2.packettype = dType;
		lcmd.LIGHTING2.subtype = dSubType;
		lcmd.LIGHTING2.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.LIGHTING2.id1 = ID1;
		lcmd.LIGHTING2.id2 = ID2;
		lcmd.LIGHTING2.id3 = ID3;
		lcmd.LIGHTING2.id4 = ID4;
		lcmd.LIGHTING2.unitcode = Unit;
		lcmd.LIGHTING2.filler = 0;
		lcmd.LIGHTING2.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.LIGHTING2.cmnd, options))
			return false;
		if (switchtype == STYPE_Doorbell) {
			int rnvalue = 0;
			m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
			if (rnvalue == 0)
				lcmd.LIGHTING2.cmnd = light2_sGroupOn;
			else
				lcmd.LIGHTING2.cmnd = light2_sOn;
			level = 15;
		}
		else if (switchtype == STYPE_X10Siren) {
			level = 15;
		}
		else if ((switchtype == STYPE_BlindsPercentage) || (switchtype == STYPE_BlindsPercentageInverted)) {
			if (lcmd.LIGHTING2.cmnd == light2_sSetLevel)
			{
				if (level == 15)
				{
					lcmd.LIGHTING2.cmnd = light2_sOn;
				}
				else if (level == 0)
				{
					lcmd.LIGHTING2.cmnd = light2_sOff;
				}
			}
		}
		else if (switchtype == STYPE_Media)
		{
			if (switchcmd == "Set Volume") {
				level = (level < 0) ? 0 : level;
				level = (level > 100) ? 100 : level;
			}
		}
		else
			level = (level > 15) ? 15 : level;

		lcmd.LIGHTING2.level = (unsigned char)level;
		//Special Teach-In for EnOcean Dimmers
		if ((pHardware->HwdType == HTYPE_EnOceanESP2) && (IsTesting) && (switchtype == STYPE_Dimmer))
		{
			CEnOceanESP2 *pEnocean = reinterpret_cast<CEnOceanESP2*>(pHardware);
			pEnocean->SendDimmerTeachIn((const char*)&lcmd, sizeof(lcmd.LIGHTING1));
		}
		else if ((pHardware->HwdType == HTYPE_EnOceanESP3) && (IsTesting) && (switchtype == STYPE_Dimmer))
		{
			CEnOceanESP3 *pEnocean = reinterpret_cast<CEnOceanESP3*>(pHardware);
			pEnocean->SendDimmerTeachIn((const char*)&lcmd, sizeof(lcmd.LIGHTING1));
		}
		else
		{
			if (switchtype != STYPE_Motion) //dont send actual motion off command
			{
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING2)))
					return false;
			}
		}

		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeLighting3:
		if (level > 9)
			level = 9;
		break;
	case pTypeLighting4:
	{
		tRBUF lcmd;
		lcmd.LIGHTING4.packetlength = sizeof(lcmd.LIGHTING4) - 1;
		lcmd.LIGHTING4.packettype = dType;
		lcmd.LIGHTING4.subtype = dSubType;
		lcmd.LIGHTING4.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.LIGHTING4.cmd1 = ID2;
		lcmd.LIGHTING4.cmd2 = ID3;
		lcmd.LIGHTING4.cmd3 = ID4;
		lcmd.LIGHTING4.filler = 0;
		lcmd.LIGHTING4.rssi = 12;

		//Get Pulse timing
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, sd[1].c_str(), Unit, int(dType), int(dSubType));
		if (result.size() == 1)
		{
			int pulsetimeing = atoi(result[0][0].c_str());
			lcmd.LIGHTING4.pulseHigh = pulsetimeing / 256;
			lcmd.LIGHTING4.pulseLow = pulsetimeing & 0xFF;
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING4)))
				return false;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
			}
			return true;
		}
		return false;
	}
	break;
	case pTypeLighting5:
	{
		int oldlevel = level;
		tRBUF lcmd;
		lcmd.LIGHTING5.packetlength = sizeof(lcmd.LIGHTING5) - 1;
		lcmd.LIGHTING5.packettype = dType;
		lcmd.LIGHTING5.subtype = dSubType;
		lcmd.LIGHTING5.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.LIGHTING5.id1 = ID2;
		lcmd.LIGHTING5.id2 = ID3;
		lcmd.LIGHTING5.id3 = ID4;
		lcmd.LIGHTING5.unitcode = Unit;
		lcmd.LIGHTING5.filler = 0;
		lcmd.LIGHTING5.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.LIGHTING5.cmnd, options))
			return false;
		if (switchtype == STYPE_Doorbell)
		{
			int rnvalue = 0;
			m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
			if (rnvalue == 0)
				lcmd.LIGHTING5.cmnd = light5_sGroupOn;
			else
				lcmd.LIGHTING5.cmnd = light5_sOn;
			level = 31;
		}
		else if (switchtype == STYPE_X10Siren)
		{
			level = 31;
		}
		if (level > 31)
			level = 31;
		lcmd.LIGHTING5.level = (unsigned char)level;
		if (dSubType == sTypeLivolo)
		{
			if ((switchcmd == "Set Level") && (level == 0))
			{
				switchcmd = "Off";
				GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.LIGHTING5.cmnd, options);
			}
			if (switchcmd != "Off")
			{
				//Special Case, turn off first
				unsigned char oldCmd = lcmd.LIGHTING5.cmnd;
				lcmd.LIGHTING5.cmnd = light5_sLivoloAllOff;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return false;
				lcmd.LIGHTING5.cmnd = oldCmd;
			}
			if (switchcmd == "Set Level")
			{
				//dim value we have to send multiple times
				for (int iDim = 0; iDim < level; iDim++)
				{
					if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
						return false;
				}
			}
			else
			{
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return false;
			}
		}
		else if ((dSubType == sTypeTRC02) || (dSubType == sTypeTRC02_2))
		{
			if (switchcmd != "Off")
			{
				if ((hue != -1) && (hue != 1000))
				{
					double dval;
					dval = (255.0 / 360.0)*float(hue);
					oldlevel = round(dval);
					switchcmd = "Set Color";
				}
			}
			if (((switchcmd == "Off") || (switchcmd == "On")) && (switchcmd != "Set Color"))
			{
				//Special Case, turn off first
				unsigned char oldCmd = lcmd.LIGHTING5.cmnd;
				lcmd.LIGHTING5.cmnd = light5_sRGBoff;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return false;
				lcmd.LIGHTING5.cmnd = oldCmd;
				sleep_milliseconds(100);
			}
			if ((switchcmd == "On") || (switchcmd == "Set Color"))
			{
				//turn on
				lcmd.LIGHTING5.cmnd = light5_sRGBon;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return false;
				sleep_milliseconds(100);

				if (switchcmd == "Set Color")
				{
					if ((oldlevel != -1) && (oldlevel != 1000))
					{
						double dval;
						dval = (78.0 / 255.0)*float(oldlevel);
						lcmd.LIGHTING5.cmnd = light5_sRGBcolormin + 1 + round(dval);
						if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
							return false;
					}
				}
			}
		}
		else
		{
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
				return false;
		}
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeLighting6:
	{
		tRBUF lcmd;
		lcmd.LIGHTING6.packetlength = sizeof(lcmd.LIGHTING6) - 1;
		lcmd.LIGHTING6.packettype = dType;
		lcmd.LIGHTING6.subtype = dSubType;
		lcmd.LIGHTING6.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.LIGHTING6.seqnbr2 = 0;
		lcmd.LIGHTING6.id1 = ID2;
		lcmd.LIGHTING6.id2 = ID3;
		lcmd.LIGHTING6.groupcode = ID4;
		lcmd.LIGHTING6.unitcode = Unit;
		lcmd.LIGHTING6.cmndseqnbr = m_hardwaredevices[hindex]->m_SeqNr % 4;
		lcmd.LIGHTING6.filler = 0;
		lcmd.LIGHTING6.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.LIGHTING6.cmnd, options))
			return false;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING6)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeHomeConfort:
	{
		tRBUF lcmd;
		lcmd.HOMECONFORT.packetlength = sizeof(lcmd.HOMECONFORT) - 1;
		lcmd.HOMECONFORT.packettype = dType;
		lcmd.HOMECONFORT.subtype = dSubType;
		lcmd.HOMECONFORT.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.HOMECONFORT.id1 = ID1;
		lcmd.HOMECONFORT.id2 = ID2;
		lcmd.HOMECONFORT.id3 = ID3;
		lcmd.HOMECONFORT.housecode = ID4;
		lcmd.HOMECONFORT.unitcode = Unit;
		lcmd.HOMECONFORT.filler = 0;
		lcmd.HOMECONFORT.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.HOMECONFORT.cmnd, options))
			return false;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.HOMECONFORT)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeFan:
	{
		tRBUF lcmd;
		lcmd.FAN.packetlength = sizeof(lcmd.FAN) - 1;
		lcmd.FAN.packettype = dType;
		lcmd.FAN.subtype = dSubType;
		lcmd.FAN.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.FAN.id1 = ID2;
		lcmd.FAN.id2 = ID3;
		lcmd.FAN.id3 = ID4;
		lcmd.FAN.filler = 0;
		lcmd.FAN.rssi = 12;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.FAN.cmnd, options))
			return false;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.FAN)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeLimitlessLights:
	{
		_tLimitlessLights lcmd;
		lcmd.len = sizeof(_tLimitlessLights) - 1;
		lcmd.type = dType;
		lcmd.subtype = dSubType;
		lcmd.id = ID;
		lcmd.dunit = Unit;

		if ((switchcmd == "On") || (switchcmd == "Set Level"))
		{
			if (hue != -1)
			{
				_tLimitlessLights lcmd2;
				lcmd2.len = sizeof(_tLimitlessLights) - 1;
				lcmd2.type = dType;
				lcmd2.subtype = dSubType;
				lcmd2.id = ID;
				lcmd2.dunit = Unit;
				if (hue != 1000)
				{
					double dval;
					dval = (255.0 / 360.0)*float(hue);
					int ival;
					ival = round(dval);
					lcmd2.value = ival;
					lcmd2.command = Limitless_SetRGBColour;
				}
				else
				{
					lcmd2.command = Limitless_SetColorToWhite;
				}
				if (!WriteToHardware(HardwareID, (const char*)&lcmd2, sizeof(_tLimitlessLights)))
					return false;
				sleep_milliseconds(100);
			}
		}

		lcmd.value = level;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.command, options))
			return false;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(_tLimitlessLights)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeSecurity1:
	{
		tRBUF lcmd;
		lcmd.SECURITY1.packetlength = sizeof(lcmd.SECURITY1) - 1;
		lcmd.SECURITY1.packettype = dType;
		lcmd.SECURITY1.subtype = dSubType;
		lcmd.SECURITY1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.SECURITY1.battery_level = 9;
		lcmd.SECURITY1.id1 = ID2;
		lcmd.SECURITY1.id2 = ID3;
		lcmd.SECURITY1.id3 = ID4;
		lcmd.SECURITY1.rssi = 12;
		switch (dSubType)
		{
		case sTypeKD101:
		case sTypeSA30:
		{
			if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.SECURITY1.status, options))
				return false;
			//send it twice
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return false;
			sleep_milliseconds(500);
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return false;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
			}
		}
		break;
		case sTypeSecX10M:
		case sTypeSecX10R:
		case sTypeSecX10:
		case sTypeMeiantech:
		{
			if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.SECURITY1.status, options))
				return false;
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return false;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
			}
		}
		break;
		}
		return true;
	}
	break;
	case pTypeSecurity2:
	{
		BYTE kCodes[9];
		if (sd[1].size() < 8 * 2)
		{
			return false;
		}
		for (int ii = 0; ii < 8; ii++)
		{
			std::string sHex = sd[1].substr(ii * 2, 2);
			std::stringstream s_strid;
			int iHex = 0;
			s_strid << std::hex << sHex;
			s_strid >> iHex;
			kCodes[ii] = (BYTE)iHex;

		}
		tRBUF lcmd;
		lcmd.SECURITY2.packetlength = sizeof(lcmd.SECURITY2) - 1;
		lcmd.SECURITY2.packettype = dType;
		lcmd.SECURITY2.subtype = dSubType;
		lcmd.SECURITY2.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.SECURITY2.id1 = kCodes[0];
		lcmd.SECURITY2.id2 = kCodes[1];
		lcmd.SECURITY2.id3 = kCodes[2];
		lcmd.SECURITY2.id4 = kCodes[3];
		lcmd.SECURITY2.id5 = kCodes[4];
		lcmd.SECURITY2.id6 = kCodes[5];
		lcmd.SECURITY2.id7 = kCodes[6];
		lcmd.SECURITY2.id8 = kCodes[7];

		lcmd.SECURITY2.id9 = 0;//bat full
		lcmd.SECURITY2.battery_level = 9;
		lcmd.SECURITY2.rssi = 12;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY2)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeCurtain:
	{
		tRBUF lcmd;
		lcmd.CURTAIN1.packetlength = sizeof(lcmd.CURTAIN1) - 1;
		lcmd.CURTAIN1.packettype = dType;
		lcmd.CURTAIN1.subtype = dSubType;
		lcmd.CURTAIN1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.CURTAIN1.housecode = atoi(sd[1].c_str());
		lcmd.CURTAIN1.unitcode = Unit;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.CURTAIN1.cmnd, options))
			return false;
		lcmd.CURTAIN1.filler = 0;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.CURTAIN1)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeBlinds:
	{
		tRBUF lcmd;
		lcmd.BLINDS1.packetlength = sizeof(lcmd.BLINDS1) - 1;
		lcmd.BLINDS1.packettype = dType;
		lcmd.BLINDS1.subtype = dSubType;
		lcmd.BLINDS1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.BLINDS1.id1 = ID1;
		lcmd.BLINDS1.id2 = ID2;
		lcmd.BLINDS1.id3 = ID3;
		lcmd.BLINDS1.id4 = 0;
		if ((dSubType == sTypeBlindsT0) || (dSubType == sTypeBlindsT1) || (dSubType == sTypeBlindsT3) || (dSubType == sTypeBlindsT8) || (dSubType == sTypeBlindsT12) || (dSubType == sTypeBlindsT13))
		{
			lcmd.BLINDS1.unitcode = Unit;
		}
		else if ((dSubType == sTypeBlindsT6) || (dSubType == sTypeBlindsT7) || (dSubType == sTypeBlindsT9))
		{
			lcmd.BLINDS1.unitcode = Unit;
			lcmd.BLINDS1.id4 = ID4;
		}
		else
		{
			lcmd.BLINDS1.unitcode = 0;
		}
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.BLINDS1.cmnd, options))
			return false;
		level = 15;
		lcmd.BLINDS1.filler = 0;
		lcmd.BLINDS1.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.BLINDS1)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeRFY:
	{
		tRBUF lcmd;
		lcmd.BLINDS1.packetlength = sizeof(lcmd.RFY) - 1;
		lcmd.BLINDS1.packettype = dType;
		lcmd.BLINDS1.subtype = dSubType;
		lcmd.RFY.id1 = ID2;
		lcmd.RFY.id2 = ID3;
		lcmd.RFY.id3 = ID4;
		lcmd.RFY.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.RFY.unitcode = Unit;

		if (IsTesting)
		{
			lcmd.RFY.cmnd = rfy_sProgram;
		}
		else
		{
			if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.RFY.cmnd, options))
				return false;
		}

		if (lcmd.BLINDS1.subtype == sTypeRFY2)
		{
			//Special case for protocol version 2
			lcmd.BLINDS1.subtype = sTypeRFY;
			if (lcmd.RFY.cmnd == rfy_sUp)
				lcmd.RFY.cmnd = rfy_s2SecUp;
			else if (lcmd.RFY.cmnd == rfy_sDown)
				lcmd.RFY.cmnd = rfy_s2SecDown;
		}

		level = 15;
		lcmd.RFY.filler = 0;
		lcmd.RFY.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RFY)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeChime:
	{
		tRBUF lcmd;
		lcmd.CHIME.packetlength = sizeof(lcmd.CHIME) - 1;
		lcmd.CHIME.packettype = dType;
		lcmd.CHIME.subtype = dSubType;
		lcmd.CHIME.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.CHIME.id1 = ID3;
		lcmd.CHIME.id2 = ID4;
		level = 15;
		lcmd.CHIME.sound = Unit;
		lcmd.CHIME.filler = 0;
		lcmd.CHIME.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.CHIME)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeThermostat2:
	{
		tRBUF lcmd;
		lcmd.THERMOSTAT2.packetlength = sizeof(lcmd.REMOTE) - 1;
		lcmd.THERMOSTAT2.packettype = dType;
		lcmd.THERMOSTAT2.subtype = dSubType;
		lcmd.THERMOSTAT2.unitcode = Unit;
		lcmd.THERMOSTAT2.cmnd = Unit;
		lcmd.THERMOSTAT2.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.THERMOSTAT2.cmnd, options))
			return false;

		lcmd.THERMOSTAT2.filler = 0;
		lcmd.THERMOSTAT2.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT2)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeThermostat3:
	{
		tRBUF lcmd;
		lcmd.THERMOSTAT3.packetlength = sizeof(lcmd.THERMOSTAT3) - 1;
		lcmd.THERMOSTAT3.packettype = dType;
		lcmd.THERMOSTAT3.subtype = dSubType;
		lcmd.THERMOSTAT3.unitcode1 = ID2;
		lcmd.THERMOSTAT3.unitcode2 = ID3;
		lcmd.THERMOSTAT3.unitcode3 = ID4;
		lcmd.THERMOSTAT3.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.THERMOSTAT3.cmnd, options))
			return false;
		level = 15;
		lcmd.THERMOSTAT3.filler = 0;
		lcmd.THERMOSTAT3.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT3)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeThermostat4:
	{
		_log.Log(LOG_ERROR, "Thermostat 4 not implemented yet!");
		/*
					tRBUF lcmd;
					lcmd.THERMOSTAT4.packetlength = sizeof(lcmd.THERMOSTAT3) - 1;
					lcmd.THERMOSTAT4.packettype = dType;
					lcmd.THERMOSTAT4.subtype = dSubType;
					lcmd.THERMOSTAT4.unitcode1 = ID2;
					lcmd.THERMOSTAT4.unitcode2 = ID3;
					lcmd.THERMOSTAT4.unitcode3 = ID4;
					lcmd.THERMOSTAT4.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
					if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.THERMOSTAT4.mode, options))
						return false;
					level = 15;
					lcmd.THERMOSTAT4.filler = 0;
					lcmd.THERMOSTAT4.rssi = 12;
					if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT4)))
						return false;
					if (!IsTesting) {
						//send to internal for now (later we use the ACK)
						PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
					}
		*/
		return true;
	}
	break;
	case pTypeRemote:
	{
		tRBUF lcmd;
		lcmd.REMOTE.packetlength = sizeof(lcmd.REMOTE) - 1;
		lcmd.REMOTE.packettype = dType;
		lcmd.REMOTE.subtype = dSubType;
		lcmd.REMOTE.id = ID4;
		lcmd.REMOTE.cmnd = Unit;
		lcmd.REMOTE.cmndtype = 0;
		lcmd.REMOTE.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.REMOTE.toggle = 0;
		lcmd.REMOTE.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.REMOTE)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeEvohomeRelay:
	{
		REVOBUF lcmd;
		memset(&lcmd, 0, sizeof(REVOBUF));
		lcmd.EVOHOME3.len = sizeof(lcmd.EVOHOME3) - 1;
		lcmd.EVOHOME3.type = pTypeEvohomeRelay;
		lcmd.EVOHOME3.subtype = sTypeEvohomeRelay;
		RFX_SETID3(ID, lcmd.EVOHOME3.id1, lcmd.EVOHOME3.id2, lcmd.EVOHOME3.id3)
			lcmd.EVOHOME3.devno = Unit;
		if (switchcmd == "On")
			lcmd.EVOHOME3.demand = 200;
		else
			lcmd.EVOHOME3.demand = level;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.EVOHOME3)))
			return false;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	}
	break;
	case pTypeRadiator1:
		tRBUF lcmd;
		lcmd.RADIATOR1.packetlength = sizeof(lcmd.RADIATOR1) - 1;
		lcmd.RADIATOR1.packettype = pTypeRadiator1;
		lcmd.RADIATOR1.subtype = sTypeSmartwares;
		lcmd.RADIATOR1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.RADIATOR1.id1 = ID1;
		lcmd.RADIATOR1.id2 = ID2;
		lcmd.RADIATOR1.id3 = ID3;
		lcmd.RADIATOR1.id4 = ID4;
		lcmd.RADIATOR1.unitcode = Unit;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.RADIATOR1.cmnd, options))
			return false;
		if (level > 15)
			level = 15;
		lcmd.RADIATOR1.temperature = 0;
		lcmd.RADIATOR1.tempPoint5 = 0;
		lcmd.RADIATOR1.filler = 0;
		lcmd.RADIATOR1.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RADIATOR1)))
			return false;

		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			lcmd.RADIATOR1.subtype = sTypeSmartwaresSwitchRadiator;
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&lcmd, NULL, -1);
		}
		return true;
	case pTypeGeneralSwitch:
	{

		_tGeneralSwitch gswitch;
		gswitch.type = dType;
		gswitch.subtype = dSubType;
		gswitch.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		gswitch.id = ID;
		gswitch.unitcode = Unit;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, gswitch.cmnd, options))
			return false;

		if ((switchtype != STYPE_Selector) && (dSubType != sSwitchGeneralSwitch))
		{
			level = (level > 99) ? 99 : level;
		}

		if (switchtype == STYPE_Doorbell) {
			int rnvalue = 0;
			m_sql.GetPreferencesVar("DoorbellCommand", rnvalue);
			if (rnvalue == 0)
				lcmd.LIGHTING2.cmnd = gswitch_sGroupOn;
			else
				lcmd.LIGHTING2.cmnd = gswitch_sOn;
			level = 15;
		}
		else if (switchtype == STYPE_Selector)
		{
			if ((switchcmd == "Set Level") || (switchcmd == "Set Group Level")) {
				std::map<std::string, std::string> statuses;
				GetSelectorSwitchStatuses(options, statuses);
				int maxLevel = statuses.size() * 10;

				level = (level < 0) ? 0 : level;
				level = (level > maxLevel) ? maxLevel : level;

				std::stringstream sslevel;
				sslevel << level;
				if (statuses[sslevel.str()].empty()) {
					_log.Log(LOG_ERROR, "Setting a wrong level value %d to Selector device %" PRIu64 "", level, ID);
				}
			}
		}
		else if (((switchtype == STYPE_BlindsPercentage) ||
			(switchtype == STYPE_BlindsPercentageInverted)) &&
			(gswitch.cmnd == gswitch_sSetLevel) && (level == 100))
			gswitch.cmnd = gswitch_sOn;

		gswitch.level = (unsigned char)level;
		gswitch.rssi = 12;
		if (switchtype != STYPE_Motion) //dont send actual motion off command
		{
			if (!WriteToHardware(HardwareID, (const char*)&gswitch, sizeof(_tGeneralSwitch)))
				return false;
		}
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const unsigned char *)&gswitch, NULL, -1);
		}
	}
	return true;
	}
	return false;
}

bool MainWorker::SwitchModal(const std::string &idx, const std::string &status, const std::string &action, const std::string &ooc, const std::string &until)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,nValue FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;
	std::vector<std::string> sd = result[0];

	int nStatus = 0;
	if (status == "Away")
		nStatus = CEvohomeBase::cmEvoAway;
	else if (status == "AutoWithEco")
		nStatus = CEvohomeBase::cmEvoAutoWithEco;
	else if (status == "DayOff")
		nStatus = CEvohomeBase::cmEvoDayOff;
	else if (status == "Custom")
		nStatus = CEvohomeBase::cmEvoCustom;
	else if (status == "Auto")
		nStatus = CEvohomeBase::cmEvoAuto;
	else if (status == "HeatingOff")
		nStatus = CEvohomeBase::cmEvoHeatingOff;

	int nValue = atoi(sd[7].c_str());
	if (ooc == "1" && nValue == nStatus)
		return false;//FIXME not an error ... status = (already set)

	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	unsigned char Unit = atoi(sd[2].c_str());
	unsigned char dType = atoi(sd[3].c_str());
	unsigned char dSubType = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());

	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;


	unsigned long ID;
	std::stringstream s_strid;
	if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		s_strid << std::hex << sd[1];
	else  //GB3: web based evohome uses decimal device ID's. We need to convert those to hex here to fit the 3-byte ID defined in the message struct
		s_strid << std::hex << std::dec << sd[1];
	s_strid >> ID;

	//Update Domoticz evohome Device
	REVOBUF tsen;
	memset(&tsen, 0, sizeof(REVOBUF));
	tsen.EVOHOME1.len = sizeof(tsen.EVOHOME1) - 1;
	tsen.EVOHOME1.type = pTypeEvohome;
	tsen.EVOHOME1.subtype = sTypeEvohome;
	RFX_SETID3(ID, tsen.EVOHOME1.id1, tsen.EVOHOME1.id2, tsen.EVOHOME1.id3)
		tsen.EVOHOME1.action = (action == "1") ? 1 : 0;
	tsen.EVOHOME1.status = nStatus;

	tsen.EVOHOME1.mode = until.empty() ? CEvohomeBase::cmPerm : CEvohomeBase::cmTmp;
	if (tsen.EVOHOME1.mode == CEvohomeBase::cmTmp)
		CEvohomeDateTime::DecodeISODate(tsen.EVOHOME1, until.c_str());
	WriteToHardware(HardwareID, (const char*)&tsen, sizeof(tsen.EVOHOME1));

	//the latency on the scripted solution is quite bad so it's good to see the update happening...ideally this would go to an 'updating' status (also useful to update database if we ever use this as a pure virtual device)
	PushRxMessage(pHardware, (const unsigned char *)&tsen, NULL, 255);
	return true;
}

bool MainWorker::SwitchLight(const std::string &idx, const std::string &switchcmd, const std::string &level, const std::string &hue, const std::string &ooc, const int ExtraDelay)
{
	uint64_t ID;
	std::stringstream s_str(idx);
	s_str >> ID;

	return SwitchLight(ID, switchcmd, atoi(level.c_str()), atoi(hue.c_str()), atoi(ooc.c_str()) != 0, ExtraDelay);
}

bool MainWorker::SwitchLight(const uint64_t idx, const std::string &switchcmd, const int level, const int hue, const bool ooc, const int ExtraDelay)
{
	//Get Device details
	if (_log.isTraceEnabled()) _log.Log(LOG_TRACE, "MAIN SwitchLight idx:%d cmd:%s lvl:%d ", (long)idx, switchcmd.c_str(), level);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID,DeviceID,Unit,Type,SubType,SwitchType,AddjValue2,nValue,sValue,Name,Options FROM DeviceStatus WHERE (ID == %" PRIu64 ")",
		idx);
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];

	//unsigned char dType = atoi(sd[3].c_str());
	//unsigned char dSubType = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());
	int iOnDelay = atoi(sd[6].c_str());
	int nValue = atoi(sd[7].c_str());
	std::string sValue = sd[8].c_str();
	std::string devName = sd[9].c_str();
	//std::string sOptions = sd[10].c_str();

	bool bIsOn = IsLightSwitchOn(switchcmd);
	if (ooc)//Only on change
	{
		int nNewVal = bIsOn ? 1 : 0;//Is that everything we need here
		if ((switchtype == STYPE_Selector) && (nValue == nNewVal) && (level == atoi(sValue.c_str()))) {
			return true;
		}
		else if (nValue == nNewVal) {
			return true;//FIXME no return code for already set
		}
	}
	//Check if we have an On-Delay, if yes, add it to the tasker
	if (((bIsOn) && (iOnDelay != 0)) || ExtraDelay)
	{
		if (ExtraDelay != 0)
		{
			_log.Log(LOG_NORM, "Delaying switch [%s] action (%s) for %d seconds", devName.c_str(), switchcmd.c_str(), ExtraDelay);
		}
		m_sql.AddTaskItem(_tTaskItem::SwitchLightEvent(static_cast<float>(iOnDelay + ExtraDelay), idx, switchcmd, level, hue, "Switch with Delay"));
		return true;
	}
	else
		return SwitchLightInt(sd, switchcmd, level, hue, false);
}

bool MainWorker::SetSetPoint(const std::string &idx, const float TempValue, const int newMode, const std::string &until)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1 FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		s_strid << std::hex << sd[1];
	else //GB3: web based evohome uses decimal device ID's. We need to convert those to hex here to fit the 3-byte ID defined in the message struct
		s_strid << std::hex << std::dec << sd[1];
	s_strid >> ID;


	unsigned char Unit = atoi(sd[2].c_str());
	unsigned char dType = atoi(sd[3].c_str());
	unsigned char dSubType = atoi(sd[4].c_str());
	//_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());

	if (pHardware->HwdType == HTYPE_EVOHOME_SCRIPT || pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_WEB || pHardware->HwdType == HTYPE_EVOHOME_TCP)
	{
		REVOBUF tsen;
		memset(&tsen, 0, sizeof(tsen.EVOHOME2));
		tsen.EVOHOME2.len = sizeof(tsen.EVOHOME2) - 1;
		tsen.EVOHOME2.type = dType;
		tsen.EVOHOME2.subtype = dSubType;
		RFX_SETID3(ID, tsen.EVOHOME2.id1, tsen.EVOHOME2.id2, tsen.EVOHOME2.id3)

			tsen.EVOHOME2.zone = Unit;//controller is 0 so let our zones start from 1...
		tsen.EVOHOME2.updatetype = CEvohomeBase::updSetPoint;//setpoint
		tsen.EVOHOME2.temperature = static_cast<int16_t>((dType == pTypeEvohomeWater) ? TempValue : TempValue*100.0f);
		tsen.EVOHOME2.mode = newMode;
		if (newMode == CEvohomeBase::zmTmp)
			CEvohomeDateTime::DecodeISODate(tsen.EVOHOME2, until.c_str());
		WriteToHardware(HardwareID, (const char*)&tsen, sizeof(tsen.EVOHOME2));

		//Pass across the current controller mode if we're going to update as per the hw device
		result = m_sql.safe_query(
			"SELECT Name,DeviceID,nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==0)",
			HardwareID);
		if (result.size() > 0)
		{
			sd = result[0];
			tsen.EVOHOME2.controllermode = atoi(sd[2].c_str());
		}
		//the latency on the scripted solution is quite bad so it's good to see the update happening...ideally this would go to an 'updating' status (also useful to update database if we ever use this as a pure virtual device)
		PushAndWaitRxMessage(pHardware, (const unsigned char*)&tsen, NULL, -1);
	}
	return true;
}

bool MainWorker::SetSetPointInt(const std::vector<std::string> &sd, const float TempValue)
{
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	unsigned char ID1 = (unsigned char)((ID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((ID & 0x00FF0000) >> 16);
	unsigned char ID3 = (unsigned char)((ID & 0x0000FF00) >> 8);
	unsigned char ID4 = (unsigned char)((ID & 0x000000FF));

	unsigned char Unit = atoi(sd[2].c_str());
	unsigned char dType = atoi(sd[3].c_str());
	unsigned char dSubType = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());

	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;
	//
	//	For plugins all the specific logic below is irrelevent
	//	so just send the full details to the plugin so that it can take appropriate action
	//
	if (pHardware->HwdType == HTYPE_PythonPlugin)
	{
#ifdef ENABLE_PYTHON
		((Plugins::CPlugin*)pHardware)->SendCommand(Unit, "Set Level", TempValue);
#endif
	}
	else if (
		(pHardware->HwdType == HTYPE_OpenThermGateway) ||
		(pHardware->HwdType == HTYPE_OpenThermGatewayTCP) ||
		(pHardware->HwdType == HTYPE_ICYTHERMOSTAT) ||
		(pHardware->HwdType == HTYPE_TOONTHERMOSTAT) ||
		(pHardware->HwdType == HTYPE_AtagOne) ||
		(pHardware->HwdType == HTYPE_NEST) ||
		(pHardware->HwdType == HTYPE_ANNATHERMOSTAT) ||
		(pHardware->HwdType == HTYPE_THERMOSMART) ||
		(pHardware->HwdType == HTYPE_EVOHOME_SCRIPT) ||
		(pHardware->HwdType == HTYPE_EVOHOME_SERIAL) ||
		(pHardware->HwdType == HTYPE_EVOHOME_TCP) ||
		(pHardware->HwdType == HTYPE_EVOHOME_WEB) ||
		(pHardware->HwdType == HTYPE_Netatmo) ||
		(pHardware->HwdType == HTYPE_NefitEastLAN) ||
		(pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF) ||
		(pHardware->HwdType == HTYPE_OpenWebNetTCP)
		)
	{
		if (pHardware->HwdType == HTYPE_OpenThermGateway)
		{
			OTGWSerial *pGateway = reinterpret_cast<OTGWSerial*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_OpenThermGatewayTCP)
		{
			OTGWTCP *pGateway = reinterpret_cast<OTGWTCP*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_ICYTHERMOSTAT)
		{
			CICYThermostat *pGateway = reinterpret_cast<CICYThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_TOONTHERMOSTAT)
		{
			CToonThermostat *pGateway = reinterpret_cast<CToonThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_AtagOne)
		{
			CAtagOne *pGateway = reinterpret_cast<CAtagOne*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_NEST)
		{
			CNest *pGateway = reinterpret_cast<CNest*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_ANNATHERMOSTAT)
		{
			CAnnaThermostat *pGateway = reinterpret_cast<CAnnaThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_THERMOSMART)
		{
			CThermosmart *pGateway = reinterpret_cast<CThermosmart*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_Netatmo)
		{
			CNetatmo *pGateway = reinterpret_cast<CNetatmo*>(pHardware);
			pGateway->SetSetpoint(ID2, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_NefitEastLAN)
		{
			CNefitEasy *pGateway = reinterpret_cast<CNefitEasy*>(pHardware);
			pGateway->SetSetpoint(ID2, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_EVOHOME_SCRIPT || pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_WEB || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		{
			SetSetPoint(sd[7], TempValue, CEvohomeBase::zmPerm, "");
		}
		else if (pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF)
		{
			CInComfort *pGateway = reinterpret_cast<CInComfort*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_OpenWebNetTCP)
		{
			COpenWebNetTCP *pGateway = reinterpret_cast<COpenWebNetTCP*>(pHardware);
			return pGateway->SetSetpoint(ID4, TempValue);
		}
	}
	else
	{
		if (dType == pTypeRadiator1)
		{
			tRBUF lcmd;
			lcmd.RADIATOR1.packetlength = sizeof(lcmd.RADIATOR1) - 1;
			lcmd.RADIATOR1.packettype = dType;
			lcmd.RADIATOR1.subtype = dSubType;
			lcmd.RADIATOR1.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
			lcmd.RADIATOR1.id1 = ID1;
			lcmd.RADIATOR1.id2 = ID2;
			lcmd.RADIATOR1.id3 = ID3;
			lcmd.RADIATOR1.id4 = ID4;
			lcmd.RADIATOR1.unitcode = Unit;
			lcmd.RADIATOR1.filler = 0;
			lcmd.RADIATOR1.rssi = 12;
			lcmd.RADIATOR1.cmnd = Radiator1_sSetTemp;

			char szTemp[20];
			sprintf(szTemp, "%.1f", TempValue);
			std::vector<std::string> strarray;
			StringSplit(szTemp, ".", strarray);
			lcmd.RADIATOR1.temperature = (unsigned char)atoi(strarray[0].c_str());
			lcmd.RADIATOR1.tempPoint5 = (unsigned char)atoi(strarray[1].c_str());
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RADIATOR1)))
				return false;
			PushAndWaitRxMessage(pHardware, (const unsigned char*)&lcmd, NULL, -1);
		}
		else
		{
			float tempDest = TempValue;
			unsigned char tSign = m_sql.m_tempsign[0];
			if (tSign == 'F')
			{
				//Convert to Celsius
				tempDest = static_cast<float>(ConvertToCelsius(tempDest));
			}

			_tThermostat tmeter;
			tmeter.subtype = sTypeThermSetpoint;
			tmeter.id1 = ID1;
			tmeter.id2 = ID2;
			tmeter.id3 = ID3;
			tmeter.id4 = ID4;
			tmeter.dunit = 1;
			tmeter.temp = tempDest;
			if (!WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tThermostat)))
				return false;
			if (pHardware->HwdType == HTYPE_Dummy)
			{
				//Also set it in the database, ad this devices does not send updates
				_log.Log(LOG_TRACE, "MAIN SetPoint command Idx=%s : Temp=%f", sd[7].c_str(), TempValue);
				PushAndWaitRxMessage(pHardware, (const unsigned char*)&tmeter, NULL, -1);
			}
		}
	}
	return true;
}

bool MainWorker::SetSetPoint(const std::string &idx, const float TempValue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,ID FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];
	return SetSetPointInt(sd, TempValue);
}

bool MainWorker::SetClockInt(const std::vector<std::string> &sd, const std::string &clockstr)
{
#ifdef WITH_OPENZWAVE
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		std::vector<std::string> splitresults;
		StringSplit(clockstr, ";", splitresults);
		if (splitresults.size() != 3)
			return false;
		int day = atoi(splitresults[0].c_str());
		int hour = atoi(splitresults[1].c_str());
		int minute = atoi(splitresults[2].c_str());

		_tGeneralDevice tmeter;
		tmeter.subtype = sTypeZWaveClock;
		tmeter.intval1 = ID;
		tmeter.intval2 = (day*(24 * 60)) + (hour * 60) + minute;
		if (!WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tGeneralDevice)))
			return false;
	}
#endif
	return true;
}

bool MainWorker::SetClock(const std::string &idx, const std::string &clockstr)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];
	return SetClockInt(sd, clockstr);
}

bool MainWorker::SetZWaveThermostatModeInt(const std::vector<std::string> &sd, const int tMode)
{
#ifdef WITH_OPENZWAVE
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		_tGeneralDevice tmeter;
		tmeter.subtype = sTypeZWaveThermostatMode;
		tmeter.intval1 = ID;
		tmeter.intval2 = tMode;
		if (!WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tGeneralDevice)))
			return false;
	}
#endif
	return true;
}

bool MainWorker::SetZWaveThermostatFanModeInt(const std::vector<std::string> &sd, const int fMode)
{
#ifdef WITH_OPENZWAVE
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		_tGeneralDevice tmeter;
		tmeter.subtype = sTypeZWaveThermostatFanMode;
		tmeter.intval1 = ID;
		tmeter.intval2 = fMode;
		if (!WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tGeneralDevice)))
			return false;
	}
#endif
	return true;
}

bool MainWorker::SetZWaveThermostatMode(const std::string &idx, const int tMode)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];
	return SetZWaveThermostatModeInt(sd, tMode);
}

bool MainWorker::SetZWaveThermostatFanMode(const std::string &idx, const int fMode)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;

	std::vector<std::string> sd = result[0];
	return SetZWaveThermostatFanModeInt(sd, fMode);
}

bool MainWorker::SetThermostatState(const std::string &idx, const int newState)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.size() < 1)
		return false;
	int HardwareID = atoi(result[0][0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);
	if (pHardware == NULL)
		return false;
	if (pHardware->HwdType == HTYPE_TOONTHERMOSTAT)
	{
		CToonThermostat *pGateway = reinterpret_cast<CToonThermostat*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_AtagOne)
	{
		//CAtagOne *pGateway = reinterpret_cast<CAtagOne*>(pHardware);
		//pGateway->SetProgramState(newState);
		return true;
	}
	else if (pHardware->HwdType == HTYPE_NEST)
	{
		CNest *pGateway = reinterpret_cast<CNest*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	else if (pHardware->HwdType == HTYPE_ANNATHERMOSTAT)
	{
		CAnnaThermostat *pGateway = reinterpret_cast<CAnnaThermostat*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	else if (pHardware->HwdType == HTYPE_THERMOSMART)
	{
		CThermosmart *pGateway = reinterpret_cast<CThermosmart *>(pHardware);
		//pGateway->SetProgramState(newState);
		return true;
	}
	else if (pHardware->HwdType == HTYPE_Netatmo)
	{
		CNetatmo *pGateway = reinterpret_cast<CNetatmo *>(pHardware);
		int tIndex = atoi(idx.c_str());
		pGateway->SetProgramState(tIndex, newState);
		return true;
	}
	else if (pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF)
	{
		CInComfort *pGateway = reinterpret_cast<CInComfort*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	return false;
}


bool MainWorker::SwitchScene(const std::string &idx, const std::string &switchcmd)
{
	uint64_t ID;
	std::stringstream s_str(idx);
	s_str >> ID;

	return SwitchScene(ID, switchcmd);
}

//returns if a device activates a scene
bool MainWorker::DoesDeviceActiveAScene(const uint64_t DevRowIdx, const int Cmnd)
{
	//check for scene code
	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> >::const_iterator itt;

	result = m_sql.safe_query("SELECT Activators, SceneType FROM Scenes WHERE (Activators!='')");
	if (result.size() > 0)
	{
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			int SceneType = atoi(sd[1].c_str());

			std::vector<std::string> arrayActivators;
			StringSplit(sd[0], ";", arrayActivators);
			std::vector<std::string>::const_iterator ittAct;
			for (ittAct = arrayActivators.begin(); ittAct != arrayActivators.end(); ++ittAct)
			{
				std::string sCodeCmd = *ittAct;

				std::vector<std::string> arrayCode;
				StringSplit(sCodeCmd, ":", arrayCode);

				std::string sID = arrayCode[0];
				std::string sCode = "";
				if (arrayCode.size() == 2)
				{
					sCode = arrayCode[1];
				}

				uint64_t aID;
				std::stringstream sstr;
				sstr << sID;
				sstr >> aID;
				if (aID == DevRowIdx)
				{
					if ((SceneType == SGTYPE_GROUP) || (sCode.empty()))
						return true;
					int iCode = atoi(sCode.c_str());
					if (iCode == Cmnd)
						return true;
				}
			}
		}
	}
	return false;
}

bool MainWorker::SwitchScene(const uint64_t idx, std::string switchcmd)
{
	//Get Scene details
	std::vector<std::vector<std::string> > result;
	int nValue = (switchcmd == "On") ? 1 : 0;

	//first set actual scene status
	std::string Name = "Unknown?";
	_eSceneGroupType scenetype = SGTYPE_SCENE;
	std::string onaction = "";
	std::string offaction = "";
	std::string status = "";

	//Get Scene Name
	result = m_sql.safe_query("SELECT Name, SceneType, OnAction, OffAction, nValue FROM Scenes WHERE (ID == %" PRIu64 ")", idx);
	if (result.size() > 0)
	{
		std::vector<std::string> sds = result[0];
		Name = sds[0];
		scenetype = (_eSceneGroupType)atoi(sds[1].c_str());
		onaction = sds[2];
		offaction = sds[3];
		status = sds[4];

		//when asking for Toggle, just switch to the opposite value
		if (switchcmd == "Toggle") {
			nValue = (atoi(status.c_str()) == 0 ? 1 : 0);
			switchcmd = (nValue == 1 ? "On" : "Off");
		}

		m_sql.HandleOnOffAction((nValue == 1), onaction, offaction);
	}

	m_sql.safe_query("INSERT INTO SceneLog (SceneRowID, nValue) VALUES ('%" PRIu64 "', '%d')", idx, nValue);

	std::string szLastUpdate = TimeToString(NULL, TF_DateTime);
	m_sql.safe_query("UPDATE Scenes SET nValue=%d, LastUpdate='%q' WHERE (ID == %" PRIu64 ")",
		nValue,
		szLastUpdate.c_str(),
		idx);

	//Check if we need to email a snapshot of a Camera
	std::string emailserver;
	int n2Value;
	if (m_sql.GetPreferencesVar("EmailServer", n2Value, emailserver))
	{
		if (emailserver != "")
		{
			result = m_sql.safe_query(
				"SELECT CameraRowID, DevSceneDelay FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID==%" PRIu64 ") AND (DevSceneWhen==%d)",
				idx,
				!nValue
			);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator ittCam;
				for (ittCam = result.begin(); ittCam != result.end(); ++ittCam)
				{
					std::vector<std::string> sd = *ittCam;
					std::string camidx = sd[0];
					int delay = atoi(sd[1].c_str());
					std::string subject;
					if (scenetype == SGTYPE_SCENE)
						subject = Name + " Activated";
					else
						subject = Name + " Status: " + switchcmd;
					m_sql.AddTaskItem(_tTaskItem::EmailCameraSnapshot(static_cast<float>(delay + 1), camidx, subject));
				}
			}
		}
	}

	_log.Log(LOG_NORM, "Activating Scene/Group: [%s]", Name.c_str());

	bool bEventTrigger = true;
	if (m_sql.m_bEnableEventSystem)
		bEventTrigger = m_eventsystem.UpdateSceneGroup(idx, nValue, szLastUpdate);

	//now switch all attached devices, and only the onces that do not trigger a scene
	result = m_sql.safe_query(
		"SELECT DeviceRowID, Cmd, Level, Hue, OnDelay, OffDelay FROM SceneDevices WHERE (SceneRowID == %" PRIu64 ") ORDER BY [Order] ASC", idx);
	if (result.size() < 1)
		return true; //no devices in the scene

	std::vector<std::vector<std::string> >::const_iterator itt;
	for (itt = result.begin(); itt != result.end(); ++itt)
	{
		std::vector<std::string> sd = *itt;

		int cmd = atoi(sd[1].c_str());
		int level = atoi(sd[2].c_str());
		int hue = atoi(sd[3].c_str());
		int ondelay = atoi(sd[4].c_str());
		int offdelay = atoi(sd[5].c_str());
		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType, nValue, sValue, Name FROM DeviceStatus WHERE (ID == '%q')", sd[0].c_str());
		if (result2.size() > 0)
		{
			std::vector<std::string> sd2 = result2[0];
			unsigned char rnValue = atoi(sd2[6].c_str());
			std::string sValue = sd2[7];
			unsigned char Unit = atoi(sd2[2].c_str());
			unsigned char dType = atoi(sd2[3].c_str());
			unsigned char dSubType = atoi(sd2[4].c_str());
			std::string DeviceName = sd2[8];
			_eSwitchType switchtype = (_eSwitchType)atoi(sd2[5].c_str());

			//Check if this device will not activate a scene
			uint64_t dID;
			std::stringstream sdID;
			sdID << sd[0];
			sdID >> dID;
			if (DoesDeviceActiveAScene(dID, cmd))
			{
				_log.Log(LOG_ERROR, "Skipping sensor '%s' because this triggers another scene!", DeviceName.c_str());
				continue;
			}

			std::string lstatus = switchcmd;
			int llevel = 0;
			bool bHaveDimmer = false;
			bool bHaveGroupCmd = false;
			int maxDimLevel = 0;

			GetLightStatus(dType, dSubType, switchtype, cmd, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

			if (scenetype == SGTYPE_GROUP)
			{
				lstatus = ((switchcmd == "On") || (switchcmd == "Group On") || (switchcmd == "Chime") || (switchcmd == "All On")) ? "On" : "Off";
			}
			_log.Log(LOG_NORM, "Activating Scene/Group Device: %s (%s)", DeviceName.c_str(), lstatus.c_str());


			int ilevel = maxDimLevel - 1;

			if (
				((switchtype == STYPE_Dimmer) ||
				(switchtype == STYPE_BlindsPercentage) ||
					(switchtype == STYPE_BlindsPercentageInverted) ||
					(switchtype == STYPE_Selector)
					) && (maxDimLevel != 0))
			{
				if (lstatus == "On")
				{
					lstatus = "Set Level";
					float fLevel = (maxDimLevel / 100.0f)*level;
					if (fLevel > 100)
						fLevel = 100;
					ilevel = round(fLevel) + 1;
				}
				if (switchtype == STYPE_Selector) {
					if (lstatus != "Set Level") {
						ilevel = 0;
					}
					ilevel = round(ilevel / 10.0f) * 10; // select only multiples of 10
					if (ilevel == 0) {
						lstatus = "Off";
					}
				}
			}

			int idx = atoi(sd[0].c_str());
			if (switchtype != STYPE_PushOn)
			{
				int delay = (lstatus == "Off") ? offdelay : ondelay;
				if (m_sql.m_bEnableEventSystem && !bEventTrigger)
					m_eventsystem.SetEventTrigger(idx, m_eventsystem.REASON_DEVICE, static_cast<float>(delay));
				SwitchLight(idx, lstatus, ilevel, hue, false, delay);
				if (scenetype == SGTYPE_SCENE)
				{
					if ((lstatus != "Off") && (offdelay > 0))
					{
						//switch with on delay, and off delay
						if (m_sql.m_bEnableEventSystem && !bEventTrigger)
							m_eventsystem.SetEventTrigger(idx, m_eventsystem.REASON_DEVICE, static_cast<float>(ondelay + offdelay));
						SwitchLight(idx, "Off", ilevel, hue, false, ondelay + offdelay);
					}
				}
			}
			else
			{
				if (m_sql.m_bEnableEventSystem && !bEventTrigger)
					m_eventsystem.SetEventTrigger(idx, m_eventsystem.REASON_DEVICE, static_cast<float>(ondelay));
				SwitchLight(idx, "On", ilevel, hue, false, ondelay);
			}
			sleep_milliseconds(50);
		}
	}
	return true;
}

void MainWorker::CheckSceneCode(const uint64_t DevRowIdx, const unsigned char dType, const unsigned char dSubType, const int nValue, const char* sValue)
{
	//check for scene code
	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> >::const_iterator itt;

	result = m_sql.safe_query("SELECT ID, Activators, SceneType FROM Scenes WHERE (Activators!='')");
	if (result.size() > 0)
	{
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;

			std::vector<std::string> arrayActivators;
			StringSplit(sd[1], ";", arrayActivators);
			std::vector<std::string>::const_iterator ittAct;
			for (ittAct = arrayActivators.begin(); ittAct != arrayActivators.end(); ++ittAct)
			{
				std::string sCodeCmd = *ittAct;

				std::vector<std::string> arrayCode;
				StringSplit(sCodeCmd, ":", arrayCode);

				std::string sID = arrayCode[0];
				std::string sCode = "";
				if (arrayCode.size() == 2)
				{
					sCode = arrayCode[1];
				}

				uint64_t aID;
				std::stringstream sstr;
				sstr << sID;
				sstr >> aID;
				if (aID == DevRowIdx)
				{
					uint64_t ID;
					std::stringstream s_str(sd[0]);
					s_str >> ID;
					int scenetype = atoi(sd[2].c_str());

					if ((scenetype == SGTYPE_SCENE) && (!sCode.empty()))
					{
						//Also check code
						int iCode = atoi(sCode.c_str());
						if (iCode != nValue)
							continue;
					}

					std::string lstatus = "";
					int llevel = 0;
					bool bHaveDimmer = false;
					bool bHaveGroupCmd = false;
					int maxDimLevel = 0;

					GetLightStatus(dType, dSubType, STYPE_OnOff, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
					std::string switchcmd = (IsLightSwitchOn(lstatus) == true) ? "On" : "Off";

					m_sql.AddTaskItem(_tTaskItem::SwitchSceneEvent(0.2f, ID, switchcmd, "SceneTrigger"));
				}
			}
		}
	}
}

void MainWorker::LoadSharedUsers()
{
	std::vector<tcp::server::_tRemoteShareUser> users;

	std::vector<std::vector<std::string> > result;
	std::vector<std::vector<std::string> > result2;
	std::vector<std::vector<std::string> >::const_iterator itt;
	std::vector<std::vector<std::string> >::const_iterator itt2;

	result = m_sql.safe_query("SELECT ID, Username, Password FROM USERS WHERE ((RemoteSharing==1) AND (Active==1))");
	if (result.size() > 0)
	{
		for (itt = result.begin(); itt != result.end(); ++itt)
		{
			std::vector<std::string> sd = *itt;
			tcp::server::_tRemoteShareUser suser;
			suser.Username = base64_decode(sd[1]);
			suser.Password = sd[2];

			//Get User Devices
			result2 = m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == '%q')",
				sd[0].c_str());
			if (result2.size() > 0)
			{
				for (itt2 = result2.begin(); itt2 != result2.end(); ++itt2)
				{
					std::vector<std::string> sd2 = *itt2;
					uint64_t ID;
					std::stringstream s_str(sd2[0]);
					s_str >> ID;
					suser.Devices.push_back(ID);
				}
			}
			users.push_back(suser);
		}
	}
	m_sharedserver.SetRemoteUsers(users);
	m_sharedserver.stopAllClients();
}

void MainWorker::SetInternalSecStatus()
{
	m_eventsystem.WWWUpdateSecurityState(m_SecStatus);

	//Update Domoticz Security Device
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.SECURITY1.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.SECURITY1.packettype = pTypeSecurity1;
	tsen.SECURITY1.subtype = sTypeDomoticzSecurity;
	tsen.SECURITY1.battery_level = 9;
	tsen.SECURITY1.rssi = 12;
	tsen.SECURITY1.id1 = 0x14;
	tsen.SECURITY1.id2 = 0x87;
	tsen.SECURITY1.id3 = 0x02;
	tsen.SECURITY1.seqnbr = 1;
	if (m_SecStatus == SECSTATUS_DISARMED)
		tsen.SECURITY1.status = sStatusNormal;
	else if (m_SecStatus == SECSTATUS_ARMEDHOME)
		tsen.SECURITY1.status = sStatusArmHome;
	else
		tsen.SECURITY1.status = sStatusArmAway;

	if (m_verboselevel >= EVBL_ALL)
	{
		_log.Log(LOG_NORM, "(System) Domoticz Security Status");
	}

	CDomoticzHardwareBase *pHardware = GetHardwareByType(HTYPE_DomoticzInternal);
	PushAndWaitRxMessage(pHardware, (const unsigned char *)&tsen, "Domoticz Security Panel", -1);
}

void MainWorker::UpdateDomoticzSecurityStatus(const int iSecStatus)
{
	m_SecCountdown = -1; //cancel possible previous delay
	m_SecStatus = iSecStatus;

	m_sql.UpdatePreferencesVar("SecStatus", iSecStatus);

	int nValue = 0;
	m_sql.GetPreferencesVar("SecOnDelay", nValue);

	if ((nValue == 0) || (iSecStatus == SECSTATUS_DISARMED))
	{
		//Do it Directly
		SetInternalSecStatus();
	}
	else
	{
		//Schedule It
		m_SecCountdown = nValue;
	}
}

void MainWorker::ForceLogNotificationCheck()
{
	m_bForceLogNotificationCheck = true;
}

void MainWorker::HandleLogNotifications()
{
	std::list<CLogger::_tLogLineStruct> _loglines = _log.GetNotificationLogs();
	if (_loglines.empty())
		return;
	//Assemble notification message

	std::stringstream sstr;
	std::list<CLogger::_tLogLineStruct>::const_iterator itt;
	std::string sTopic;

	if (_loglines.size() > 1)
	{
		sTopic = "Domoticz: Multiple errors received in the last 5 minutes";
		sstr << "Multiple errors received in the last 5 minutes:<br><br>";
	}
	else
	{
		itt = _loglines.begin();
		sTopic = "Domoticz: " + itt->logmessage;
	}

	for (itt = _loglines.begin(); itt != _loglines.end(); ++itt)
	{
		sstr << itt->logmessage << "<br>";
	}
	m_sql.AddTaskItem(_tTaskItem::SendEmail(1, sTopic, sstr.str()));
}

void MainWorker::HeartbeatUpdate(const std::string &component)
{
	boost::lock_guard<boost::mutex> l(m_heartbeatmutex);
	time_t now = time(0);
	std::map<std::string, time_t >::iterator itt = m_componentheartbeats.find(component);
	if (itt != m_componentheartbeats.end()) {
		itt->second = now;
	}
	else {
		m_componentheartbeats[component] = now;
	}
}

void MainWorker::HeartbeatRemove(const std::string &component)
{
	boost::lock_guard<boost::mutex> l(m_heartbeatmutex);
	std::map<std::string, time_t >::iterator itt = m_componentheartbeats.find(component);
	if (itt != m_componentheartbeats.end()) {
		m_componentheartbeats.erase(itt);
	}
}

void MainWorker::HeartbeatCheck()
{
	boost::lock_guard<boost::mutex> l(m_heartbeatmutex);
	boost::lock_guard<boost::mutex> l2(m_devicemutex);

	m_devicestorestart.clear();

	time_t now;
	mytime(&now);

	std::map<std::string, time_t>::const_iterator iterator;
	for (iterator = m_componentheartbeats.begin(); iterator != m_componentheartbeats.end(); ++iterator) {
		double dif = difftime(now, iterator->second);
		//_log.Log(LOG_STATUS, "%s last checking  %.2lf seconds ago", iterator->first.c_str(), dif);
		if (dif > 60)
		{
			_log.Log(LOG_ERROR, "%s thread seems to have ended unexpectedly", iterator->first.c_str());
		}
	}

	//Check hardware heartbeats
	std::vector<CDomoticzHardwareBase*>::const_iterator itt;
	for (itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
	{
		CDomoticzHardwareBase *pHardware = (CDomoticzHardwareBase *)(*itt);
		if (!pHardware->m_bSkipReceiveCheck)
		{
			//Skip Dummy Hardware
			bool bDoCheck = (pHardware->HwdType != HTYPE_Dummy) && (pHardware->HwdType != HTYPE_Domoticz) && (pHardware->HwdType != HTYPE_EVOHOME_SCRIPT);
			if (bDoCheck)
			{
				//Check Thread Timeout
				double diff = difftime(now, pHardware->m_LastHeartbeat);
				//_log.Log(LOG_STATUS, "%d last checking  %.2lf seconds ago", iterator->first, dif);
				if (diff > 60)
				{
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT Name FROM Hardware WHERE (ID='%d')", pHardware->m_HwdID);
					if (result.size() == 1)
					{
						std::vector<std::string> sd = result[0];
						_log.Log(LOG_ERROR, "%s hardware (%d) thread seems to have ended unexpectedly", sd[0].c_str(), pHardware->m_HwdID);
					}
				}
			}

			if (pHardware->m_DataTimeout > 0)
			{
				//Check Receive Timeout
				double diff = difftime(now, pHardware->m_LastHeartbeatReceive);
				if (diff > pHardware->m_DataTimeout)
				{
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT Name FROM Hardware WHERE (ID='%d')", pHardware->m_HwdID);
					if (result.size() == 1)
					{
						std::vector<std::string> sd = result[0];

						std::string sDataTimeout = "";
						int totNum = 0;
						if (pHardware->m_DataTimeout < 60) {
							totNum = pHardware->m_DataTimeout;
							sDataTimeout = "Seconds";
						}
						else if (pHardware->m_DataTimeout < 3600) {
							totNum = pHardware->m_DataTimeout / 60;
							if (totNum == 1) {
								sDataTimeout = "Minute";
							}
							else {
								sDataTimeout = "Minutes";
							}
						}
						else if (pHardware->m_DataTimeout < 86400) {
							totNum = pHardware->m_DataTimeout / 3600;
							if (totNum == 1) {
								sDataTimeout = "Hour";
							}
							else {
								sDataTimeout = "Hours";
							}
						}
						else {
							totNum = pHardware->m_DataTimeout / 60;
							if (totNum == 1) {
								sDataTimeout = "Day";
							}
							else {
								sDataTimeout = "Days";
							}
						}

						_log.Log(LOG_ERROR, "%s hardware (%d) nothing received for more than %d %s!....", sd[0].c_str(), pHardware->m_HwdID, totNum, sDataTimeout.c_str());
						m_devicestorestart.push_back(pHardware->m_HwdID);
					}
				}
			}

		}
	}
}

bool MainWorker::UpdateDevice(const int HardwareID, const std::string &DeviceID, const int unit, const int devType, const int subType, const int nValue, const std::string &sValue, const int signallevel, const int batterylevel, const bool parseTrigger)
{
	CDomoticzHardwareBase *pHardware = GetHardware(HardwareID);

	unsigned long ID = 0;
	std::stringstream s_strid;
	s_strid << std::hex << DeviceID;
	s_strid >> ID;

	if (pHardware)
	{
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT ID,Name FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			HardwareID, DeviceID.c_str(), unit, devType, subType);

		uint64_t dID = 0;
		std::string dName = "";

		if (!result.empty())
		{
			std::vector<std::string> sd = result[0];
			std::stringstream s_strid;
			s_strid << sd[0];
			s_strid >> dID;
			dName = sd[1];
		}

		if (devType == pTypeLighting2)
		{
			//Update as Lighting 2
			unsigned long ID;
			std::stringstream s_strid;
			s_strid << std::hex << DeviceID;
			s_strid >> ID;
			unsigned char ID1 = (unsigned char)((ID & 0xFF000000) >> 24);
			unsigned char ID2 = (unsigned char)((ID & 0x00FF0000) >> 16);
			unsigned char ID3 = (unsigned char)((ID & 0x0000FF00) >> 8);
			unsigned char ID4 = (unsigned char)((ID & 0x000000FF));

			tRBUF lcmd;
			memset(&lcmd, 0, sizeof(RBUF));
			lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
			lcmd.LIGHTING2.packettype = pTypeLighting2;
			lcmd.LIGHTING2.subtype = subType;
			lcmd.LIGHTING2.id1 = ID1;
			lcmd.LIGHTING2.id2 = ID2;
			lcmd.LIGHTING2.id3 = ID3;
			lcmd.LIGHTING2.id4 = ID4;
			lcmd.LIGHTING2.unitcode = (unsigned char)unit;
			lcmd.LIGHTING2.cmnd = (unsigned char)nValue;
			lcmd.LIGHTING2.level = (unsigned char)atoi(sValue.c_str());
			lcmd.LIGHTING2.filler = 0;
			lcmd.LIGHTING2.rssi = signallevel;
			DecodeRXMessage(pHardware, (const unsigned char *)&lcmd.LIGHTING2, NULL, batterylevel);
			return true;
		}
		else if (devType == pTypeGeneral)
		{
			if (subType == sTypePercentage)
			{
				_tGeneralDevice gDevice;
				gDevice.subtype = sTypePercentage;
				gDevice.id = unit;
				gDevice.floatval1 = (float)atof(sValue.c_str());
				gDevice.intval1 = static_cast<int>(ID);
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeVoltage)
			{
				_tGeneralDevice gDevice;
				gDevice.subtype = sTypeVoltage;
				gDevice.id = unit;
				gDevice.floatval1 = (float)atof(sValue.c_str());
				gDevice.intval1 = static_cast<int>(ID);
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeWaterflow)
			{
				unsigned long ID;
				std::stringstream s_strid;
				s_strid << std::hex << DeviceID;
				s_strid >> ID;
				_tGeneralDevice gDevice;
				gDevice.subtype = sTypeWaterflow;
				gDevice.id = unit;
				gDevice.floatval1 = (float)atof(sValue.c_str());
				gDevice.intval1 = static_cast<int>(ID);
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeSoundLevel)
			{
				_tGeneralDevice gDevice;
				gDevice.subtype = sTypeSoundLevel;
				gDevice.id = unit;
				gDevice.intval1 = static_cast<int>(ID);
				gDevice.intval2 = atoi(sValue.c_str());
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeCustom)
			{
				_tGeneralDevice gDevice;
				gDevice.subtype = sTypeCustom;
				gDevice.id = unit;
				gDevice.floatval1 = (float)atof(sValue.c_str());
				gDevice.intval1 = static_cast<int>(ID);
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeSoilMoisture)
			{
				_tGeneralDevice gDevice;
				gDevice.subtype = subType;
				gDevice.id = unit;
				gDevice.intval1 = static_cast<int>(ID);
				gDevice.intval2 = nValue;
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
			else if (subType == sTypeKwh)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size() == 2)
				{
					_tGeneralDevice gDevice;
					gDevice.subtype = subType;
					gDevice.id = unit;
					gDevice.intval1 = static_cast<int>(ID);
					gDevice.floatval1 = static_cast<float>(atof(strarray[0].c_str()));
					gDevice.floatval2 = static_cast<float>(atof(strarray[1].c_str()));
					DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
					return true;
				}
			}
			else if (subType == sTypeAlert)
			{
				std::string devname = "Unknown";
				uint64_t devidx = m_sql.UpdateValue(
					HardwareID,
					DeviceID.c_str(),
					(const unsigned char)unit,
					(const unsigned char)devType,
					(const unsigned char)subType,
					signallevel,//signal level,
					batterylevel,//battery level
					nValue,
					sValue.c_str(),
					devname,
					false
				);
				if (devidx == -1)
					return false;
				m_notifications.CheckAndHandleNotification(devidx, devname, devType, subType, NTYPE_USAGE, static_cast<float>(nValue));
				return true;
			}
			else if (subType == sTypeTextStatus)
			{
				std::string sstatus = sValue;
				if (sstatus.size() > 63)
					sstatus = sstatus.substr(0, 63);
				_tGeneralDevice gDevice;
				gDevice.subtype = subType;
				gDevice.id = unit;
				gDevice.intval1 = static_cast<int>(ID);
				strcpy(gDevice.text, sstatus.c_str());
				DecodeRXMessage(pHardware, (const unsigned char *)&gDevice, NULL, batterylevel);
				return true;
			}
		}
		else if ((devType == pTypeAirQuality) && (subType == sTypeVoltcraft))
		{
			if (dID != 0)
				m_notifications.CheckAndHandleNotification(dID, dName, devType, subType, NTYPE_USAGE, (const float)nValue);
		}
		else if (devType == pTypeTEMP)
		{
			if (dID != 0)
			{
				m_notifications.CheckAndHandleNotification(dID, dName, devType, subType, NTYPE_TEMPERATURE, (float)atof(sValue.c_str()));
			}
		}
		else if (devType == pTypeTEMP_HUM)
		{
			if (dID != 0)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size() == 3)
				{
					float Temp = (float)atof(strarray[0].c_str());
					int Hum = atoi(strarray[1].c_str());
					m_notifications.CheckAndHandleTempHumidityNotification(dID, dName, Temp, Hum, true, true);
					float dewpoint = (float)CalculateDewPoint(Temp, Hum);
					m_notifications.CheckAndHandleDewPointNotification(dID, dName, Temp, dewpoint);
				}
			}
		}
		else if (devType == pTypeTEMP_HUM_BARO)
		{
			if (dID != 0)
			{
				std::vector<std::string> strarray;
				StringSplit(sValue, ";", strarray);
				if (strarray.size() == 5)
				{
					float Temp = (float)atof(strarray[0].c_str());
					int Hum = atoi(strarray[1].c_str());
					m_notifications.CheckAndHandleTempHumidityNotification(dID, dName, Temp, Hum, true, true);
					float dewpoint = (float)CalculateDewPoint(Temp, Hum);
					m_notifications.CheckAndHandleDewPointNotification(dID, dName, Temp, dewpoint);
					m_notifications.CheckAndHandleNotification(dID, dName, devType, subType, NTYPE_BARO, (float)atof(strarray[3].c_str()));
				}
			}
		}
		/*
				else if (devType == pTypeGeneralSwitch)
				{
					_tGeneralSwitch gswitch;
					gswitch.subtype = subType;
					gswitch.id = ID;
					gswitch.unitcode = unit;
					gswitch.cmnd = nValue;
					gswitch.level = (unsigned char)atoi(sValue.c_str());;
					gswitch.battery_level = batterylevel;
					gswitch.rssi = signallevel;
					gswitch.seqnbr = 0;
					DecodeRXMessage(pHardware, (const unsigned char *)&gswitch, NULL, batterylevel);
					return true;
				}
		*/
	}

	std::string devname = "Unknown";
	uint64_t devidx = m_sql.UpdateValue(
		HardwareID,
		DeviceID.c_str(),
		(const unsigned char)unit,
		(const unsigned char)devType,
		(const unsigned char)subType,
		signallevel,//signal level,
		batterylevel,//battery level
		nValue,
		sValue.c_str(),
		devname,
		false
	);
	if (devidx == -1)
		return false;

	// signal connected devices (MQTT, fibaro, http push ... ) about the web update
	if ((pHardware) && (parseTrigger))
	{
		sOnDeviceReceived(pHardware->m_HwdID, devidx, devname, NULL);
	}

	std::stringstream sidx;
	sidx << devidx;

	if (
		((devType == pTypeThermostat) && (subType == sTypeThermSetpoint)) ||
		((devType == pTypeRadiator1) && (subType == sTypeSmartwares))
		)
	{
		_log.Log(LOG_NORM, "Sending SetPoint to device....");
		SetSetPoint(sidx.str(), static_cast<float>(atof(sValue.c_str())));
	}
	else if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatMode))
	{
		_log.Log(LOG_NORM, "Sending Thermostat Mode to device....");
		SetZWaveThermostatMode(sidx.str(), nValue);
	}
	else if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatFanMode))
	{
		_log.Log(LOG_NORM, "Sending Thermostat Fan Mode to device....");
		SetZWaveThermostatFanMode(sidx.str(), nValue);
	}
	return true;
}
