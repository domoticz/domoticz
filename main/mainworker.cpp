#include "stdafx.h"
#include "mainworker.h"
#include "Helper.h"
#include "SunRiseSet.h"
#include "Logger.h"
#include "WebServerHelper.h"
#include "SQLHelper.h"
#include "../push/FibaroPush.h"
#include "../push/HttpPush.h"
#include "../push/InfluxPush.h"
#include "../push/GooglePubSubPush.h"
#include "../push/MQTTPush.h"

#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"
#include <boost/algorithm/string/join.hpp>
#include "../main/json_helper.h"

#include <algorithm>
#include <set>

#include "../mdns/mdns.hpp"

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
#include "../hardware/DavisLoggerSerial.h"
#include "../hardware/1Wire.h"
#include "../hardware/I2C.h"
#include "../hardware/Wunderground.h"
#include "../hardware/DarkSky.h"
#include "../hardware/VisualCrossing.h"
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
#include "../hardware/TeleinfoTCP.h"
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
#include "../hardware/MQTTAutoDiscover.h"
#include "../hardware/FritzboxTCP.h"
#include "../hardware/ETH8020.h"
#ifdef WITH_OPENZWAVE
#include "../hardware/OpenZWave.h"
#endif
#include "../hardware/RFLinkSerial.h"
#include "../hardware/RFLinkTCP.h"
#include "../hardware/RFLinkMQTT.h"
#include "../hardware/KMTronicSerial.h"
#include "../hardware/KMTronicTCP.h"
#include "../hardware/KMTronicUDP.h"
#include "../hardware/KMTronic433.h"
#include "../hardware/SolarMaxTCP.h"
#include "../hardware/Pinger.h"
#include "../hardware/Nest.h"
#include "../hardware/NestOAuthAPI.h"
#include "../hardware/Tado.h"
#include "../hardware/eVehicles/eVehicle.h"
#include "../hardware/Kodi.h"
#include "../hardware/Netatmo.h"
#include "../hardware/HttpPoller.h"
#include "../hardware/AnnaThermostat.h"
#include "../hardware/Winddelen.h"
#include "../hardware/SatelIntegra.h"
#include "../hardware/LogitechMediaServer.h"
#include "../hardware/Comm5TCP.h"
#include "../hardware/Comm5SMTCP.h"
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
#include "../hardware/DenkoviDevices.h"
#include "../hardware/DenkoviUSBDevices.h"
#include "../hardware/DenkoviTCPDevices.h"
#include "../hardware/AccuWeather.h"
#include "../hardware/BleBox.h"
#include "../hardware/Ec3kMeterTCP.h"
#include "../hardware/OpenWeatherMap.h"
#include "../hardware/Daikin.h"
#include "../hardware/HEOS.h"
#include "../hardware/MultiFun.h"
#include "../hardware/ZiBlueSerial.h"
#include "../hardware/ZiBlueTCP.h"
#include "../hardware/Yeelight.h"
#include "../hardware/XiaomiGateway.h"
#ifdef ENABLE_PYTHON
#include "../hardware/plugins/Plugins.h"
#endif
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
#include "../hardware/eHouseTCP.h"
#include "../hardware/EcoCompteur.h"
#include "../hardware/Honeywell.h"
#include "../hardware/TTNMQTT.h"
#include "../hardware/Buienradar.h"
#include "../hardware/OctoPrintMQTT.h"
#include "../hardware/Meteorologisk.h"
#include "../hardware/AirconWithMe.h"
#include "../hardware/AlfenEve.h"
#include "../hardware/Enever.h"
#include "../hardware/MitsubishiWF.h"

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

extern std::string szStartupFolder;
extern std::string szUserDataFolder;
extern std::string szWWWFolder;
extern std::string szAppVersion;
extern std::string szCertFile;
extern int iAppRevision;
extern std::string szWebRoot;
extern bool g_bUseUpdater;
extern http::server::_eWebCompressionMode g_wwwCompressMode;
extern http::server::CWebServerHelper m_webservers;
extern bool g_bUseEventTrigger;
extern bool bNoCleanupDev;
extern domoticz_mdns::mDNS m_mdns;
extern bool bEnableMDNS;

CFibaroPush m_fibaropush;
CGooglePubSubPush m_googlepubsubpush;
CHttpPush m_httppush;
CInfluxPush m_influxpush;
CMQTTPush m_mqttpush;

MainWorker::MainWorker()
{
	m_SecCountdown = -1;

	m_bStartHardware = false;
	m_hardwareStartCounter = 0;

	// Set default settings for web servers
	m_webserver_settings.listening_address = "::"; // listen to all network interfaces
	m_webserver_settings.listening_port = "8080";
#ifdef WWW_ENABLE_SSL
	m_secure_webserver_settings.listening_address = "::"; // listen to all network interfaces
	m_secure_webserver_settings.listening_port = "443";
	m_secure_webserver_settings.ssl_method = "tls";
	m_secure_webserver_settings.certificate_chain_file_path = szCertFile;
	m_secure_webserver_settings.ca_cert_file_path = m_secure_webserver_settings.certificate_chain_file_path; // not used
	m_secure_webserver_settings.cert_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.private_key_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.private_key_pass_phrase = "";
	m_secure_webserver_settings.ssl_options = "single_dh_use";
	m_secure_webserver_settings.tmp_dh_file_path = m_secure_webserver_settings.certificate_chain_file_path;
	m_secure_webserver_settings.verify_peer = false;
	m_secure_webserver_settings.verify_fail_if_no_peer_cert = false;
	m_secure_webserver_settings.verify_file_path = "";
	m_secure_webserver_settings.cipher_list = "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384";
#endif
	m_bIgnoreUsernamePassword = false;

	time_t atime = mytime(nullptr);
	m_LastHeartbeat = atime;
	m_LastSunriseSet = "";
	m_DayLength = "";

	m_bHaveDownloadedDomoticzUpdate = false;
	m_bHaveDownloadedDomoticzUpdateSuccessFull = false;
	m_bDoDownloadDomoticzUpdate = false;
	m_LastUpdateCheck = 0;
	m_bHaveUpdate = false;
	m_iRevision = 0;

	m_SecStatus = SECSTATUS_DISARMED;

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
		"SELECT ID, Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware ORDER BY ID ASC");
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			int ID = atoi(sd[0].c_str());
			std::string Name = sd[1];
			std::string sEnabled = sd[2];
			bool Enabled = (sEnabled == "1") ? true : false;
			_eHardwareTypes Type = (_eHardwareTypes)atoi(sd[3].c_str());
			uint32_t LogLevelEnabled = (uint32_t)atoi(sd[4].c_str());
			std::string Address = sd[5];
			uint16_t Port = (uint16_t)atoi(sd[6].c_str());
			std::string SerialPort = sd[7];
			std::string Username = sd[8];
			std::string Password = sd[9];
			std::string Extra = sd[10];
			int mode1 = atoi(sd[11].c_str());
			int mode2 = atoi(sd[12].c_str());
			int mode3 = atoi(sd[13].c_str());
			int mode4 = atoi(sd[14].c_str());
			int mode5 = atoi(sd[15].c_str());
			int mode6 = atoi(sd[16].c_str());
			int DataTimeout = atoi(sd[17].c_str());
			AddHardwareFromParams(ID, Name, Enabled, Type, LogLevelEnabled, Address, Port, SerialPort, Username, Password, Extra, mode1, mode2, mode3, mode4, mode5, mode6, DataTimeout,
				false);
		}
		m_hardwareStartCounter = 0;
		m_bStartHardware = true;
	}
}

void MainWorker::StartDomoticzHardware()
{
	for (const auto& device : m_hardwaredevices)
		if (!device->IsStarted())
			device->Start();
}

void MainWorker::StopDomoticzHardware()
{
	// Separate the Stop() from the device removal from the vector.
	// Some actions the hardware might take during stop (e.g updating a device) can cause deadlocks on the m_devicemutex
	std::vector<CDomoticzHardwareBase*> OrgHardwaredevices;
	{
		std::lock_guard<std::mutex> l(m_devicemutex);
		for (const auto& device : m_hardwaredevices)
			OrgHardwaredevices.push_back(device);
		m_hardwaredevices.clear();
	}

	for (auto& device : OrgHardwaredevices)
	{
#ifdef ENABLE_PYTHON
		m_pluginsystem.DeregisterPlugin(device->m_HwdID);
#endif
		device->Stop();
		delete device;
	}
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
		bFound = std::any_of(m_webthemes.begin(), m_webthemes.end(), [&](const std::string& s) { return s == sValue; });

	if (!bFound)
		m_sql.UpdatePreferencesVar("WebTheme", "default");
}

void MainWorker::AddDomoticzHardware(CDomoticzHardwareBase* pHardware)
{
	int devidx = FindDomoticzHardware(pHardware->m_HwdID);
	if (devidx != -1) //it is already there!, remove it
		RemoveDomoticzHardware(m_hardwaredevices[devidx]);
	std::lock_guard<std::mutex> l(m_devicemutex);
	pHardware->sDecodeRXMessage.connect([this](auto hw, auto rx, auto name, auto battery, auto userName) { DecodeRXMessage(hw, rx, name, battery, userName); });
	pHardware->sOnConnected.connect([this](auto hw) { OnHardwareConnected(hw); });
	m_hardwaredevices.push_back(pHardware);
}

void MainWorker::RemoveDomoticzHardware(CDomoticzHardwareBase* pHardware)
{
	// Separate the Stop() from the device removal from the vector.
	// Some actions the hardware might take during stop (e.g updating a device) can cause deadlocks on the m_devicemutex
	CDomoticzHardwareBase* pOrgHardware = nullptr;
	{
		std::lock_guard<std::mutex> l(m_devicemutex);
		for (auto itt = m_hardwaredevices.begin(); itt != m_hardwaredevices.end(); ++itt)
		{
			pOrgHardware = *itt;
			if (pOrgHardware == pHardware) {
				m_hardwaredevices.erase(itt);
				break;
			}
		}
	}

	if (pOrgHardware == pHardware)
	{
		try
		{
			pOrgHardware->Stop();
			delete pOrgHardware;
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "Mainworker: Exception: %s (%s:%d)", e.what(), std::string(__func__).substr(std::string(__func__).find_last_of("/\\") + 1).c_str(), __LINE__);
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "Mainworker: Exception catched! %s:%d", std::string(__func__).substr(std::string(__func__).find_last_of("/\\") + 1).c_str(), __LINE__);
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
	std::lock_guard<std::mutex> l(m_devicemutex);
	int ii = 0;
	for (const auto& device : m_hardwaredevices)
	{
		if (device->m_HwdID == HwdId)
			return ii;
		ii++;
	}
	return -1;
}

int MainWorker::FindDomoticzHardwareByType(const _eHardwareTypes HWType)
{
	std::lock_guard<std::mutex> l(m_devicemutex);
	int ii = 0;
	for (const auto& device : m_hardwaredevices)
	{
		if (device->HwdType == HWType)
			return device->m_HwdID;
		ii++;
	}
	return -1;
}

CDomoticzHardwareBase* MainWorker::GetHardware(int HwdId)
{
	std::lock_guard<std::mutex> l(m_devicemutex);
	for (const auto& device : m_hardwaredevices)
		if (device->m_HwdID == HwdId)
			return device;

	return nullptr;
}

CDomoticzHardwareBase* MainWorker::GetHardwareByIDType(const std::string& HwdId, const _eHardwareTypes HWType)
{
	if (HwdId.empty())
		return nullptr;
	int iHardwareID = atoi(HwdId.c_str());
	CDomoticzHardwareBase* pHardware = m_mainworker.GetHardware(iHardwareID);
	if (pHardware == nullptr)
		return nullptr;
	if (pHardware->HwdType != HWType)
		return nullptr;
	return pHardware;
}

CDomoticzHardwareBase* MainWorker::GetHardwareByType(const _eHardwareTypes HWType)
{
	std::lock_guard<std::mutex> l(m_devicemutex);
	for (const auto& device : m_hardwaredevices)
		if (device->HwdType == HWType)
			return device;
	return nullptr;
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

	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int year = ltime.tm_year + 1900;
	int month = ltime.tm_mon + 1;
	int day = ltime.tm_mday;

	double dLatitude = atof(Latitude.c_str());
	double dLongitude = atof(Longitude.c_str());

	SunRiseSet::_tSubRiseSetResults sresult;
	SunRiseSet::GetSunRiseSet(dLatitude, dLongitude, year, month, day, sresult);

	std::string sunrise = std_format("%02d:%02d:00", sresult.SunRiseHour, sresult.SunRiseMin);
	std::string sunset = std_format("%02d:%02d:00", sresult.SunSetHour, sresult.SunSetMin);
	std::string daylength = std_format("%02d:%02d:00", sresult.DaylengthHours, sresult.DaylengthMins);
	std::string sunatsouth = std_format("%02d:%02d:00", sresult.SunAtSouthHour, sresult.SunAtSouthMin);
	std::string civtwstart = std_format("%02d:%02d:00", sresult.CivilTwilightStartHour, sresult.CivilTwilightStartMin);
	std::string civtwend = std_format("%02d:%02d:00", sresult.CivilTwilightEndHour, sresult.CivilTwilightEndMin);
	std::string nauttwstart = std_format("%02d:%02d:00", sresult.NauticalTwilightStartHour, sresult.NauticalTwilightStartMin);
	std::string nauttwend = std_format("%02d:%02d:00", sresult.NauticalTwilightEndHour, sresult.NauticalTwilightEndMin);
	std::string asttwstart = std_format("%02d:%02d:00", sresult.AstronomicalTwilightStartHour, sresult.AstronomicalTwilightStartMin);
	std::string asttwend = std_format("%02d:%02d:00", sresult.AstronomicalTwilightEndHour, sresult.AstronomicalTwilightEndMin);

	m_scheduler.SetSunRiseSetTimes(sunrise, sunset, sunatsouth, civtwstart, civtwend, nauttwstart, nauttwend, asttwstart, asttwend); // Do not change the order

	bool bFirstTime = m_LastSunriseSet.empty();

	std::string riseset = sunrise.substr(0, sunrise.size() - 3) + ";" + sunset.substr(0, sunset.size() - 3) + ";" + sunatsouth.substr(0, sunatsouth.size() - 3) + ";" + civtwstart.substr(0, civtwstart.size() - 3) + ";" + civtwend.substr(0, civtwend.size() - 3) + ";" + nauttwstart.substr(0, nauttwstart.size() - 3) + ";" + nauttwend.substr(0, nauttwend.size() - 3) + ";" + asttwstart.substr(0, asttwstart.size() - 3) + ";" + asttwend.substr(0, asttwend.size() - 3) + ";" + daylength.substr(0, daylength.size() - 3); //make a short version
	if (m_LastSunriseSet != riseset)
	{
		m_DayLength = daylength;
		m_LastSunriseSet = riseset;

		// Now store all the time stamps e.g. "08:42;09:12" etc, found in m_LastSunriseSet into
		// a new vector after that we've first converted them to minutes after midnight.
		std::vector<std::string> strarray;
		std::vector<std::string> hourMinItem;
		StringSplit(m_LastSunriseSet, ";", strarray);
		m_SunRiseSetMins.clear();

		for (const auto& str : strarray)
		{
			StringSplit(str, ":", hourMinItem);
			int intMins = (atoi(hourMinItem[0].c_str()) * 60) + atoi(hourMinItem[1].c_str());
			m_SunRiseSetMins.push_back(intMins);
		}

		if (sunrise == sunset)
			if (m_DayLength == "00:00:00")
				_log.Log(LOG_NORM, "Sun below horizon in the space of 24 hours");
			else
				_log.Log(LOG_NORM, "Sun above horizon in the space of 24 hours");
		else
			_log.Log(LOG_NORM, "Sunrise: %s SunSet: %s", sunrise.c_str(), sunset.c_str());
		_log.Log(LOG_NORM, "Day length: %s Sun at south: %s", daylength.c_str(), sunatsouth.c_str());
		if (civtwstart == civtwend)
			_log.Log(LOG_NORM, "There is no civil twilight in the space of 24 hours");
		else
			_log.Log(LOG_NORM, "Civil twilight start: %s Civil twilight end: %s", civtwstart.c_str(), civtwend.c_str());
		if (nauttwstart == nauttwend)
			_log.Log(LOG_NORM, "There is no nautical twilight in the space of 24 hours");
		else
			_log.Log(LOG_NORM, "Nautical twilight start: %s Nautical twilight end: %s", nauttwstart.c_str(), nauttwend.c_str());
		if (asttwstart == asttwend)
			_log.Log(LOG_NORM, "There is no astronomical twilight in the space of 24 hours");
		else
			_log.Log(LOG_NORM, "Astronomical twilight start: %s Astronomical twilight end: %s", asttwstart.c_str(), asttwend.c_str());

		if (!bFirstTime)
			m_eventsystem.LoadEvents();
	}
	return true;
}

void MainWorker::SetWebserverSettings(const http::server::server_settings& settings)
{
	m_webserver_settings.set(settings);
}

void MainWorker::SetIamserverSettings(const iamserver::iam_settings& iamsettings)
{
	m_iamserver_settings.set(iamsettings);
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

void MainWorker::SetSecureWebserverSettings(const http::server::ssl_server_settings& ssl_settings)
{
	m_secure_webserver_settings.set(ssl_settings);
}
#endif

bool MainWorker::RestartHardware(const std::string& idx)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT Name, Enabled, Type, LogLevel, Address, Port, SerialPort, Username, Password, Extra, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6, DataTimeout FROM Hardware WHERE (ID=='%q')",
		idx.c_str());
	if (result.empty())
		return false;
	std::vector<std::string> sd = result[0];
	std::string Name = sd[0];
	std::string senabled = (sd[1] == "1") ? "true" : "false";
	_eHardwareTypes htype = (_eHardwareTypes)atoi(sd[2].c_str());
	uint32_t LogLevelEnabled = (uint32_t)atoi(sd[3].c_str());
	std::string address = sd[4];
	uint16_t port = (uint16_t)atoi(sd[5].c_str());
	std::string serialport = sd[6];
	std::string username = sd[7];
	std::string password = sd[8];
	std::string extra = sd[9];
	int Mode1 = atoi(sd[10].c_str());
	int Mode2 = atoi(sd[11].c_str());
	int Mode3 = atoi(sd[12].c_str());
	int Mode4 = atoi(sd[13].c_str());
	int Mode5 = atoi(sd[14].c_str());
	int Mode6 = atoi(sd[15].c_str());
	int DataTimeout = atoi(sd[16].c_str());
	return AddHardwareFromParams(atoi(idx.c_str()), Name, (senabled == "true") ? true : false, htype, LogLevelEnabled, address, port, serialport, username, password, extra, Mode1, Mode2, Mode3,
		Mode4, Mode5, Mode6, DataTimeout, true);
}

bool MainWorker::AddHardwareFromParams(
	const int ID,
	const std::string& Name,
	const bool Enabled,
	const _eHardwareTypes Type,
	const uint32_t LogLevelEnabled,
	const std::string& Address, const uint16_t Port, const std::string& SerialPort,
	const std::string& Username, const std::string& Password,
	const std::string& Extra,
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

	CDomoticzHardwareBase* pHardware = nullptr;

	switch (Type)
	{
	case HTYPE_RFXtrx315:
	case HTYPE_RFXtrx433:
	case HTYPE_RFXtrx868:
		pHardware = new RFXComSerial(ID, SerialPort, 38400, (CRFXBase::_eRFXAsyncType)atoi(Extra.c_str()));
		break;
	case HTYPE_RFXLAN:
		pHardware = new RFXComTCP(ID, Address, Port, (CRFXBase::_eRFXAsyncType)atoi(Extra.c_str()));
		break;
	case HTYPE_P1SmartMeter:
		pHardware = new P1MeterSerial(ID, SerialPort, (Mode1 == 1) ? 115200 : 9600, (Mode2 != 0), Mode3, Password);
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
		pHardware = new CEvohomeSerial(ID, SerialPort, Mode1, Extra);
		break;
	case HTYPE_EVOHOME_TCP:
		pHardware = new CEvohomeTCP(ID, Address, Port, Extra);
		break;
	case HTYPE_RFLINKUSB:
		pHardware = new CRFLinkSerial(ID, SerialPort);
		break;
	case HTYPE_RFLINKMQTT:
		pHardware = new CRFLinkMQTT(ID, Address, Port, Username, Password, Extra, Mode2, Mode1, Mode4 != 0);
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
	case HTYPE_Domoticz:
		//LAN
		pHardware = new DomoticzTCP(ID, Address, Port, Username, Password);
		break;
	case HTYPE_P1SmartMeterLAN:
		//LAN
		pHardware = new P1MeterTCP(ID, Address, Port, (Mode2 != 0), Mode3, Password);
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
		pHardware = new MySensorsMQTT(ID, Name, Address, Port, Username, Password, Extra, Mode2, Mode1, Mode3 != 0);
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
		pHardware = new MQTT(ID, Address, Port, Username, Password, Extra, Mode2, Mode1, std::string("Domoticz") + GenerateUUID() + std::to_string(ID), Mode3 != 0);
		break;
	case HTYPE_eHouseTCP:
		//eHouse LAN, WiFi,Pro and other via eHousePRO gateway
		pHardware = new eHouseTCP(ID, Address, Port, Password, Mode1, Mode2, Mode3, Mode4, Mode5, Mode6);
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
		pHardware = new C1Wire(ID, Mode1, Mode2, Extra);
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
		pHardware = new CPanasonic(ID, Mode1, Mode2, Mode3);
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
		pHardware = new CLogitechMediaServer(ID, Address, Port, Username, Password, Mode1);
		break;
	case HTYPE_Sterbox:
		//LAN
		pHardware = new CSterbox(ID, Address, Port, Username, Password);
		break;
	case HTYPE_DenkoviHTTPDevices:
		//LAN
		pHardware = new CDenkoviDevices(ID, Address, Port, Password, Mode1, Mode2);
		break;
	case HTYPE_DenkoviUSBDevices:
		//USB
		pHardware = new CDenkoviUSBDevices(ID, SerialPort, Mode1);
		break;
	case HTYPE_DenkoviTCPDevices:
		//LAN
		pHardware = new CDenkoviTCPDevices(ID, Address, Port, Mode1, Mode2, Mode3);
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
		pHardware = new I2C(ID, I2C::I2CTYPE_BMP085, Address, SerialPort, Mode1);
		break;
	case HTYPE_RaspberryHTU21D:
		pHardware = new I2C(ID, I2C::I2CTYPE_HTU21D, Address, SerialPort, Mode1);
		break;
	case HTYPE_RaspberryTSL2561:
		pHardware = new I2C(ID, I2C::I2CTYPE_TSL2561, Address, SerialPort, Mode1);
		break;
	case HTYPE_RaspberryPCF8574:
		pHardware = new I2C(ID, I2C::I2CTYPE_PCF8574, Address, SerialPort, Mode1);
		break;
	case HTYPE_RaspberryBME280:
		pHardware = new I2C(ID, I2C::I2CTYPE_BME280, Address, SerialPort, Mode1);
		break;
	case HTYPE_RaspberryMCP23017:
		pHardware = new I2C(ID, I2C::I2CTYPE_MCP23017, Address, SerialPort, Mode1);
		break;
	case HTYPE_Wunderground:
		pHardware = new CWunderground(ID, Username, Password);
		break;
	case HTYPE_HTTPPOLLER:
		pHardware = new CHttpPoller(ID, Username, Password, Address, Extra, Port);
		break;
	case HTYPE_DarkSky:
		pHardware = new CDarkSky(ID, Username, Password);
		break;
	case HTYPE_VisualCrossing:
		pHardware = new CVisualCrossing(ID, Username, Password);
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
		pHardware = new CDaikin(ID, Address, Port, Username, Password, Mode1);
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
	case HTYPE_Nest_OAuthAPI:
		pHardware = new CNestOAuthAPI(ID, Username, Extra);
		break;
	case HTYPE_ANNATHERMOSTAT:
		pHardware = new CAnnaThermostat(ID, Address, Port, Username, Password);
		break;
	case HTYPE_Tado:
		pHardware = new CTado(ID);
		break;
	case HTYPE_Tesla:
		pHardware = new CeVehicle(ID, CeVehicle::Tesla, Username, Password, Mode1, Mode2, Mode3, Extra);
		break;
	case HTYPE_Mercedes:
		pHardware = new CeVehicle(ID, CeVehicle::Mercedes, Username, Password, Mode1, Mode2, Mode3, Extra);
		break;
	case HTYPE_Honeywell:
		pHardware = new CHoneywell(ID, Username, Password, Extra);
		break;
	case HTYPE_Philips_Hue:
		pHardware = new CPhilipsHue(ID, Address, Port, Username, Mode1, Mode2);
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
	case HTYPE_Tellstick:
	{
		CTellstick* tellstick;
		if (CTellstick::Create(&tellstick, ID, Mode1, Mode2)) {
			pHardware = tellstick;
		}
	}
	break;
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
		pHardware = new COpenWebNetTCP(ID, Address, Port, Password, Mode1, Mode2);
		break;
	case HTYPE_BleBox:
		pHardware = new BleBox(ID, Mode1, Mode2);
		break;
	case HTYPE_OpenWeatherMap:
		pHardware = new COpenWeatherMap(ID, Username, Password, Mode1, Mode2, Mode3, Mode4, Mode5);
		break;
	case HTYPE_Ec3kMeterTCP:
		pHardware = new Ec3kMeterTCP(ID, Address, Port);
		break;
	case HTYPE_Yeelight:
		pHardware = new Yeelight(ID);
		break;
	case HTYPE_PythonPlugin:
#ifdef ENABLE_PYTHON
		pHardware = m_pluginsystem.RegisterPlugin(ID, Name, Extra);
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
		pHardware = new CInComfort(ID, Address, Port, Username, Password);
		break;
	case HTYPE_EVOHOME_WEB:
		pHardware = new CEvohomeWeb(ID, Username, Password, Mode1, Mode2, Mode3);
		break;
	case HTYPE_Rtl433:
		pHardware = new CRtl433(ID, Extra);
		break;
	case HTYPE_OnkyoAVTCP:
		pHardware = new OnkyoAVTCP(ID, Address, Port);
		break;
	case HTYPE_USBtinGateway:
		pHardware = new USBtin(ID, SerialPort, Mode1, Mode2);
		break;
	case HTYPE_EnphaseAPI:
		pHardware = new EnphaseAPI(ID, Address, Port, Mode1, Mode2, Mode4, Mode3, Username, Password, Extra);
		break;
	case HTYPE_Comm5SMTCP:
		pHardware = new Comm5SMTCP(ID, Address, Port);
		break;
	case HTYPE_EcoCompteur:
		pHardware = new CEcoCompteur(ID, Address, Port);
		break;
	case HTYPE_TTN_MQTT:
		pHardware = new CTTNMQTT(ID, Address, Port, Username, Password, Extra);
		break;
	case HTYPE_BuienRadar:
		pHardware = new CBuienRadar(ID, Mode1, Mode2, Password);
		break;
	case HTYPE_OctoPrint:
		pHardware = new COctoPrintMQTT(ID, Address, Port, Username, Password, Extra);
		break;
	case HTYPE_Meteorologisk:
		pHardware = new CMeteorologisk(ID, Password); //Password is location here.
		break;
	case HTYPE_AirconWithMe:
		pHardware = new CAirconWithMe(ID, Address, Port, Username, Password);
		break;
	case HTYPE_TeleinfoMeterTCP:
		pHardware = new CTeleinfoTCP(ID, Address, Port, DataTimeout, (Mode2 != 0), Mode3);
		break;
	case HTYPE_MQTTAutoDiscovery:
		pHardware = new MQTTAutoDiscover(ID, Name, Address, Port, Username, Password, Extra, Mode2);
		break;
	case HTYPE_AlfenEveCharger:
		pHardware = new AlfenEve(ID, Address, 443, 30, Username, Password);
		break;
	case HTYPE_EneverPriceFeeds:
		pHardware = new Enever(ID, Username, Extra);
		break;
	case HTYPE_MitsubishiWF:
		pHardware = new MitsubishiWF(ID, Address);
		break;

	}

	if (pHardware)
	{
		pHardware->HwdType = Type;
		pHardware->m_LogLevelEnabled = LogLevelEnabled;
		pHardware->m_Name = Name;
		pHardware->m_ShortName = Hardware_Short_Desc(Type);
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
	utsname my_uname;
	if (uname(&my_uname) == 0)
	{
		m_szSystemName = my_uname.sysname;
		std::transform(m_szSystemName.begin(), m_szSystemName.end(), m_szSystemName.begin(), ::tolower);
	}

	if (!m_sql.OpenDatabase())
	{
		return false;
	}

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
	m_mqttpush.Start();
	m_googlepubsubpush.Start();
#ifdef PARSE_RFXCOM_DEVICE_LOG
	if (m_bStartHardware == false)
		m_bStartHardware = true;
#endif
	// load notifications configuration
	m_notifications.LoadConfig();

	if (m_webserver_settings.is_enabled()
#ifdef WWW_ENABLE_SSL
		|| m_secure_webserver_settings.is_enabled()
#endif
		)
	{
		//Start WebServer
#ifdef WWW_ENABLE_SSL
		if (!m_webservers.StartServers(m_webserver_settings, m_secure_webserver_settings, m_iamserver_settings, szWWWFolder, m_bIgnoreUsernamePassword))
#else
		if (!m_webservers.StartServers(m_webserver_settings, m_iamserver_settings, szWWWFolder, m_bIgnoreUsernamePassword))
#endif
		{
#ifdef WIN32
			MessageBox(0, "Error starting webserver(s), check if ports are not in use!", MB_OK, MB_ICONERROR);
#endif
			return false;
		}
	}
	int nValue = 0;
	if (m_sql.GetPreferencesVar("AllowPlainBasicAuth", nValue))
	{
		m_webservers.SetAllowPlainBasicAuth(static_cast<bool>(nValue));
	}
	std::string sValue;
	if (m_sql.GetPreferencesVar("WebTheme", sValue))
	{
		m_webservers.SetWebTheme(sValue);
	}

	m_webservers.SetWebRoot(szWebRoot);
	m_webservers.SetWebCompressionMode(g_wwwCompressMode);

	//Start Scheduler
	m_scheduler.StartScheduler();
	m_cameras.ReloadCameras();

	int rnvalue = 0;
	m_sql.GetPreferencesVar("RemoteSharedPort", rnvalue);
	if (rnvalue != 0)
	{
		std::string sPort = std_format("%d", rnvalue);
		m_sharedserver.sDecodeRXMessage.connect([this](auto hw, auto rx, auto name, auto battery, auto user) { DecodeRXMessage(hw, rx, name, battery, user); });
		m_sharedserver.StartServer("::", sPort.c_str());

		LoadSharedUsers();
	}

	if (bEnableMDNS)
	{
		if (
			m_webserver_settings.listening_port.empty()
#ifdef WWW_ENABLE_SSL
			&& m_secure_webserver_settings.listening_port.empty()
#endif
			)
		{
			_log.Log(LOG_STATUS, "Mainworker: mDNS enabled, but webserver ports are disabled. Not starting service!");
		}
		else
		{
			std::string sValue;
			std::string szInstanceName = "Domoticz";
			if (m_sql.GetPreferencesVar("Title", sValue))
			{
				szInstanceName = sValue;
			}
			stdlower(szInstanceName);

			m_mdns.setServiceHostname(szInstanceName);
			m_mdns.setServicePort(atoi(m_webserver_settings.listening_port.c_str()));
#ifdef WWW_ENABLE_SSL
			if (m_secure_webserver_settings.is_enabled())
			{
				m_mdns.setServicePort(atoi(m_secure_webserver_settings.listening_port.c_str()));
			}
#endif
			m_mdns.addServiceTxtRecord("app", "Domoticz");
			m_mdns.addServiceTxtRecord("version", szAppVersion);
			m_mdns.addServiceTxtRecord("path", "/");
			m_mdns.startService();
		}
	}

	HandleHourPrice();

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadName(m_thread->native_handle(), "MainWorker");
	m_rxMessageThread = std::make_shared<std::thread>([this] { Do_Work_On_Rx_Messages(); });
	SetThreadName(m_rxMessageThread->native_handle(), "MainWorkerRxMsg");
	return (m_thread != nullptr) && (m_rxMessageThread != nullptr);
}

bool MainWorker::Stop()
{
	if (m_thread)
	{
		m_notificationsystem.NotifyWait(Notification::DZ_STOP, Notification::STATUS_INFO); // blocking call
	}

	if (m_rxMessageThread) {
		// Stop RxMessage thread before hardware to avoid NULL pointer exception
		m_TaskRXMessage.RequestStop();
		UnlockRxMessageQueue();
		m_rxMessageThread->join();
		m_rxMessageThread.reset();
	}
	if (m_thread)
	{
		_log.Log(LOG_STATUS, "Stopping all hardware...");
		StopDomoticzHardware();
		m_webservers.StopServers();
		m_sharedserver.StopServer();
		m_scheduler.StopScheduler();
		m_eventsystem.StopEventSystem();
		m_notificationsystem.Stop();
		m_fibaropush.Stop();
		m_httppush.Stop();
		m_influxpush.Stop();
		m_mqttpush.Stop();
		m_googlepubsubpush.Stop();
#ifdef ENABLE_PYTHON
		m_pluginsystem.StopPluginSystem();
#endif
		if (m_mdns.isServiceRunning())	// Stop mDNS service
			m_mdns.stopService();

		//    m_cameras.StopCameraGrabber();

		HTTPClient::Cleanup();

		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	return true;
}

#define HEX( x ) \
	std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (int)( x )

bool MainWorker::IsUpdateAvailable(const bool bIsForced)
{
	bool bUseUpdater = g_bUseUpdater;
	if (!bIsForced)
	{
		int nValue = 0;
		m_sql.GetPreferencesVar("UseAutoUpdate", nValue);
		if (nValue != 1)
		{
			bUseUpdater = false;
		}
	}

	utsname my_uname;
	if (uname(&my_uname) < 0)
		return false;

	std::string machine = my_uname.machine;
	if (machine == "armv6l" || (machine == "aarch64" && sizeof(void*) == 4))
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
	time_t atime = mytime(nullptr);
	if (!bIsForced)
	{
		if (atime - m_LastUpdateCheck < 12 * 3600)
		{
			if (!bUseUpdater)
				return false;
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
		szURL = "https://www.domoticz.com/download.php?channel=stable&type=version&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateURL = "https://www.domoticz.com/download.php?channel=stable&type=release&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateChecksumURL = "https://www.domoticz.com/download.php?channel=stable&type=checksum&system=" + m_szSystemName + "&machine=" + machine;
	}
	else
	{
		szURL = "https://www.domoticz.com/download.php?channel=beta&type=version&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateURL = "https://www.domoticz.com/download.php?channel=beta&type=release&system=" + m_szSystemName + "&machine=" + machine;
		m_szDomoticzUpdateChecksumURL = "https://www.domoticz.com/download.php?channel=beta&type=checksum&system=" + m_szSystemName + "&machine=" + machine;
	}

	std::string revfile;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Unique_ID: " + m_sql.m_UniqueID);
	ExtraHeaders.push_back("App_Version: " + szAppVersion);
	ExtraHeaders.push_back("App_Revision: " + std::to_string(iAppRevision));
	ExtraHeaders.push_back("System_Name: " + m_szSystemName);
	ExtraHeaders.push_back("Machine: " + machine);
	ExtraHeaders.push_back("Type: " + std::string(!bIsBetaChannel ? "Stable" : "Beta"));

	if (!HTTPClient::GET(szURL, ExtraHeaders, revfile))
		return false;

	stdreplace(revfile, "\r\n", "\n");
	std::vector<std::string> strarray;
	StringSplit(revfile, "\n", strarray);
	if (strarray.empty())
		return false;
	StringSplit(strarray[0], " ", strarray);
	if (strarray.size() != 3)
		return false;

	m_iRevision = atoi(strarray[2].c_str());
#ifdef DEBUG_DOWNLOAD
	m_bHaveUpdate = true;
#else
	m_bHaveUpdate = ((iAppRevision != m_iRevision) && (iAppRevision < m_iRevision));
#endif
	if (!bUseUpdater)
		return false;

	return m_bHaveUpdate;
}

bool MainWorker::StartDownloadUpdate()
{
#ifndef DEBUG_DOWNLOAD
#ifdef WIN32
	return false; //managed by web gui
#endif
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

	time_t now = mytime(nullptr);
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

	std::string szInstanceName = "domoticz";
	std::string szVar;
	if (m_sql.GetPreferencesVar("Title", szVar))
	{
		stdreplace(szVar, " ", "_");
		stdreplace(szVar, "/", "_");
		stdreplace(szVar, "\\", "_");
		if (!szVar.empty()) {
			szInstanceName = szVar;
		}
	}

	DIR* lDir;
	Notification::_eStatus backupStatus;
	//struct dirent *ent;


	if ((lastHourBackup == -1) || (lastHourBackup != hour)) {

		if ((lDir = opendir(sbackup_DirH.c_str())) != nullptr)
		{
			Json::Value backupInfo;
			std::stringstream sTmp;
			sTmp << "backup-hour-" << std::setw(2) << std::setfill('0') << hour << "-" << szInstanceName << ".db";

			backupInfo["type"] = "Hour";
			backupInfo["location"] = sbackup_DirH + sTmp.str();
			if (m_sql.BackupDatabase(backupInfo["location"].asString())) {
				m_sql.SetLastBackupNo(backupInfo["type"].asString().c_str(), hour);

				backupStatus = Notification::STATUS_OK;
			}
			else {
				backupStatus = Notification::STATUS_ERROR;
				_log.Log(LOG_ERROR, "Error writing automatic hourly backup file");
			}
			closedir(lDir);
			backupInfo["duration"] = difftime(mytime(nullptr), now);
			m_mainworker.m_notificationsystem.Notify(Notification::DZ_BACKUP_DONE, backupStatus, JSonToRawString(backupInfo));
		}
		else
		{
			_log.Log(LOG_ERROR, "Error accessing automatic backup directories");
		}
	}
	if ((lastDayBackup == -1) || (lastDayBackup != day)) {

		if ((lDir = opendir(sbackup_DirD.c_str())) != nullptr)
		{
			now = mytime(nullptr);
			Json::Value backupInfo;
			std::stringstream sTmp;
			sTmp << "backup-day-" << std::setw(2) << std::setfill('0') << day << "-" << szInstanceName << ".db";

			backupInfo["type"] = "Day";
			backupInfo["location"] = sbackup_DirD + sTmp.str();
			if (m_sql.BackupDatabase(backupInfo["location"].asString())) {
				m_sql.SetLastBackupNo(backupInfo["type"].asString().c_str(), day);
				backupStatus = Notification::STATUS_OK;
			}
			else {
				backupStatus = Notification::STATUS_ERROR;
				_log.Log(LOG_ERROR, "Error writing automatic daily backup file");
			}
			closedir(lDir);
			backupInfo["duration"] = difftime(mytime(nullptr), now);
			m_mainworker.m_notificationsystem.Notify(Notification::DZ_BACKUP_DONE, backupStatus, JSonToRawString(backupInfo));
		}
		else
		{
			_log.Log(LOG_ERROR, "Error accessing automatic backup directories");
		}
	}
	if ((lastMonthBackup == -1) || (lastMonthBackup != month)) {
		if ((lDir = opendir(sbackup_DirM.c_str())) != nullptr)
		{
			now = mytime(nullptr);
			Json::Value backupInfo;
			std::stringstream sTmp;
			sTmp << "backup-month-" << std::setw(2) << std::setfill('0') << month + 1 << "-" << szInstanceName << ".db";

			backupInfo["type"] = "Month";
			backupInfo["location"] = sbackup_DirM + sTmp.str();
			if (m_sql.BackupDatabase(backupInfo["location"].asString())) {
				m_sql.SetLastBackupNo(backupInfo["type"].asString().c_str(), month);
				backupStatus = Notification::STATUS_OK;
			}
			else {
				backupStatus = Notification::STATUS_ERROR;
				_log.Log(LOG_ERROR, "Error writing automatic monthly backup file");
			}
			closedir(lDir);
			backupInfo["duration"] = difftime(mytime(nullptr), now);
			m_mainworker.m_notificationsystem.Notify(Notification::DZ_BACKUP_DONE, backupStatus, JSonToRawString(backupInfo));
		}
		else
		{
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
			if ((tpos == std::string::npos) || (tpos < 10))
				continue;
			if (_line[tpos - 4] != ':')
				continue;
			_line = _line.substr(tpos + 1);
			tpos = _line.find(':');
			if (tpos != std::string::npos)
			{
				_line = _line.substr(tpos + 1);
			}
			stdreplace(_line, " ", "");
			_lines.push_back(_line);
		}
		myfile.close();
	}
	int HWID = 999;
	//m_sql.DeleteHardware("999");

	RFXComTCP* pHardware = (RFXComTCP*)GetHardware(HWID);
	if (pHardware == NULL)
	{
		pHardware = new RFXComTCP(HWID, "127.0.0.1", 1234, CRFXBase::ATYPE_P1_DSMR_5);
		//pHardware->sDecodeRXMessage.connect(boost::bind(&MainWorker::DecodeRXMessage, this, _1, _2, _3, _4, _5));
		AddDomoticzHardware(pHardware);
		pHardware->m_bEnableReceive = true;
		pHardware->m_Name = pHardware->m_ShortName = "RFXCom debug";
	}

	unsigned char rxbuffer[600];
	static const char* const lut = "0123456789ABCDEF";
	for (const auto& hexstring : _lines)
	{
		if (hexstring.size() % 2 != 0)
			continue;//illegal
		size_t totbytes = hexstring.size() / 2;
		size_t ii = 0;
		for (ii = 0; ii < totbytes; ii++)
		{
			std::string hbyte = hexstring.substr((ii * 2), 2);

			char a = hbyte[0];
			const char* p = std::lower_bound(lut, lut + 16, a);
			if (*p != a) throw std::invalid_argument("not a hex digit");

			char b = hbyte[1];
			const char* q = std::lower_bound(lut, lut + 16, b);
			if (*q != b) throw std::invalid_argument("not a hex digit");

			unsigned char uchar = static_cast<unsigned char>(((p - lut) << 4) | (q - lut));
			rxbuffer[ii] = uchar;
		}
		if (ii == 0)
			continue;
		pHardware->onInternalMessage((const uint8_t*)&rxbuffer, ii);
		sleep_milliseconds(100);
	}
#endif
}

void MainWorker::Do_Work()
{
	int second_counter = 0;
	int minute_counter = 0;
	int heartbeat_counter = 0;

	time_t atime = mytime(nullptr);
	struct tm ltime;
	localtime_r(&atime, &ltime);

	int _ScheduleLastMinute = ltime.tm_min;
	int _ScheduleLastHour = ltime.tm_hour;
	time_t _ScheduleLastMinuteTime = 0;
	time_t _ScheduleLastHourTime = 0;
	time_t _ScheduleLastDayTime = 0;


	while (!IsStopRequested(500))
	{
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
				m_notificationsystem.Start();
				m_eventsystem.SetEnabled(m_sql.m_bEnableEventSystem);
				m_eventsystem.StartEventSystem();
				m_notificationsystem.Notify(Notification::DZ_START, Notification::STATUS_INFO);
			}
		}
		if (!m_devicestorestart.empty())
		{
			for (const auto& hwid : m_devicestorestart)
			{
				std::stringstream sstr;
				sstr << hwid;
				std::string idx = sstr.str();

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM Hardware WHERE (ID=='%q')",
					idx.c_str());
				if (!result.empty())
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
				SetInternalSecStatus("System");
			}
		}

		atime = mytime(nullptr);
		struct tm ltime;
		localtime_r(&atime, &ltime);

		if (ltime.tm_hour != _ScheduleLastHour)
		{
			HandleHourPrice();
		}

		if (ltime.tm_min != _ScheduleLastMinute)
		{
			minute_counter++;
			if (difftime(atime, _ScheduleLastMinuteTime) > 30) //avoid RTC/NTP clock drifts
			{
				_ScheduleLastMinuteTime = atime;
				_ScheduleLastMinute = ltime.tm_min;

				tzset(); //this because localtime_r/localtime_s does not update for DST

				//check for 5 minute schedule
				if (ltime.tm_min % m_sql.m_ShortLogInterval == 0)
				{
					HandleHourPrice();
					if (!bNoCleanupDev)
						m_sql.ScheduleShortlog();
				}
				std::string szPwdResetFile = szStartupFolder + "resetpwd";
				if (file_exist(szPwdResetFile.c_str()))
				{
					m_webservers.ClearUserPasswords();
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
			//Check for updates every 12 hours (every 720 seconds)
			if (minute_counter % 720 == 0)
			{
				IsUpdateAvailable(true);
			}
			if (ltime.tm_hour != _ScheduleLastHour)
			{
				if (difftime(atime, _ScheduleLastHourTime) > 30 * 60) //avoid RTC/NTP clock drifts
				{
					_ScheduleLastHourTime = atime;
					_ScheduleLastHour = ltime.tm_hour;
					GetSunSettings();

					m_sql.CheckDeviceTimeout();
					m_sql.CheckBatteryLow();

					//check for daily schedule
					if (ltime.tm_hour == 0)
					{
						if (atime - _ScheduleLastDayTime > 12 * 60 * 60)
						{
							_ScheduleLastDayTime = atime;
							if (!bNoCleanupDev)
								m_sql.ScheduleDay();
						}
					}
#ifdef WITH_OPENZWAVE
					if (ltime.tm_hour == 4)
					{
						//Heal the OpenZWave network
						std::lock_guard<std::mutex> l(m_devicemutex);
						for (const auto& pHardware : m_hardwaredevices)
						{
							if (pHardware->HwdType == HTYPE_OpenZWave)
							{
								COpenZWave* pZWave = dynamic_cast<COpenZWave*>(pHardware);
								pZWave->NightlyNodeHeal();
							}
						}
					}
#endif
					HandleAutomaticBackups();
				}
			}
		}
		if (heartbeat_counter++ > 12)
		{
			heartbeat_counter = 0;
			m_LastHeartbeat = mytime(nullptr);
			HeartbeatCheck();
		}
	}
	_log.Log(LOG_STATUS, "Mainworker Stopped...");
}

bool MainWorker::WriteToHardware(const int HwdID, const char* pdata, const uint8_t length)
{
	int hindex = FindDomoticzHardware(HwdID);

	if (hindex == -1)
		return false;

	return m_hardwaredevices[hindex]->WriteToHardware(pdata, length);
}

void MainWorker::WriteMessageStart()
{
	_log.LogSequenceStart();
}

void MainWorker::WriteMessageEnd()
{
	_log.LogSequenceEnd(LOG_NORM);
}

void MainWorker::WriteMessage(const char* szMessage)
{
	_log.LogSequenceAdd(szMessage);
}

void MainWorker::WriteMessage(const char* szMessage, bool linefeed)
{
	if (linefeed)
		_log.LogSequenceAdd(szMessage);
	else
		_log.LogSequenceAddNoLF(szMessage);
}

void MainWorker::OnHardwareConnected(CDomoticzHardwareBase* pHardware)
{
	if (
		(pHardware->HwdType != HTYPE_RFXtrx315) &&
		(pHardware->HwdType != HTYPE_RFXtrx433) &&
		(pHardware->HwdType != HTYPE_RFXtrx868) &&
		(pHardware->HwdType != HTYPE_RFXLAN)
		)
	{
		//enable receive
		pHardware->m_bEnableReceive = true;
		return;
	}
	CRFXBase* pRFXBase = reinterpret_cast<CRFXBase*>(pHardware);
	pRFXBase->SendResetCommand();
}

void MainWorker::DecodeRXMessage(const CDomoticzHardwareBase* pHardware, const uint8_t* pRXCommand, const char* defaultName, const int BatteryLevel, const char* userName)
{
	if ((pHardware == nullptr) || (pRXCommand == nullptr))
		return;
	if ((pHardware->HwdType == HTYPE_Domoticz) && (pHardware->m_HwdID == 8765))
	{
		//Directly process the command
		std::lock_guard<std::mutex> l(m_decodeRXMessageMutex);
		ProcessRXMessage(pHardware, pRXCommand, defaultName, BatteryLevel, userName);
	}
	else
	{
		// Submit command without waiting for the command to be processed
		PushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel, userName);
	}
}

void MainWorker::PushRxMessage(const CDomoticzHardwareBase* pHardware, const uint8_t* pRXCommand, const char* defaultName, const int BatteryLevel, const char* userName)
{
	// Check command, submit it without waiting for it to be processed
	CheckAndPushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel, userName, false);
}

void MainWorker::PushAndWaitRxMessage(const CDomoticzHardwareBase* pHardware, const uint8_t* pRXCommand, const char* defaultName, const int BatteryLevel, const char* userName)
{
	// Check command, submit it and wait for it to be processed
	CheckAndPushRxMessage(pHardware, pRXCommand, defaultName, BatteryLevel, userName, true);
}

void MainWorker::CheckAndPushRxMessage(const CDomoticzHardwareBase* pHardware, const uint8_t* pRXCommand, const char* defaultName, const int BatteryLevel, const char* userName, const bool wait)
{
	if ((pHardware == nullptr) || (pRXCommand == nullptr))
	{
		_log.Log(LOG_ERROR, "RxQueue: cannot push message with undefined hardware (%s) or command (%s)",
			(pHardware == nullptr) ? "null" : "not null", (pRXCommand == nullptr) ? "null" : "not null");
		return;
	}
	if (pHardware->m_HwdID < 1) {
		_log.Log(LOG_ERROR, "RxQueue: cannot push message with invalid hardware id (id=%d, type=%d, name=%s)",
			pHardware->m_HwdID,
			pHardware->HwdType,
			pHardware->m_Name.c_str());
		return;
	}

	// Build queue item
	_tRxQueueItem rxMessage;
	if (defaultName != nullptr)
	{
		rxMessage.Name = defaultName;
	}
	rxMessage.BatteryLevel = BatteryLevel;
	if (userName != nullptr)
		rxMessage.UserName = userName;
	rxMessage.rxMessageIdx = m_rxMessageIdx++;
	rxMessage.hardwareId = pHardware->m_HwdID;
	// defensive copy of the command
	rxMessage.vrxCommand.insert(rxMessage.vrxCommand.begin(), pRXCommand, pRXCommand + pRXCommand[0] + 1);
	rxMessage.crc = 0x0;
#ifdef DEBUG_RXQUEUE
	// CRC
	rxMessage.crc = crc16ccitt(pRXCommand, pRXCommand[0] + 1);
#endif

	if (m_TaskRXMessage.IsStopRequested(0)) {
		// Server is stopping
		return;
	}

	// Trigger
	rxMessage.trigger = nullptr; // Should be initialized to NULL if trigger is no used
	if (wait) { // add trigger to wait for the message to be processed
		rxMessage.trigger = new queue_element_trigger();
	}

#ifdef DEBUG_RXQUEUE
	_log.Log(LOG_STATUS, "RxQueue: push a rxMessage(%lu) (hrdwId=%d, hrdwType=%d, hrdwName=%s, type=%02X, subtype=%02X)",
		rxMessage.rxMessageIdx,
		pHardware->m_HwdID,
		pHardware->HwdType,
		pHardware->m_Name.c_str(),
		pRXCommand[1],
		pRXCommand[2]);
#endif

	// Push item to queue
	m_rxMessageQueue.push(rxMessage);

	if (rxMessage.trigger != nullptr)
	{
#ifdef DEBUG_RXQUEUE
		_log.Log(LOG_STATUS, "RxQueue: wait for rxMessage(%lu) to be processed...", rxMessage.rxMessageIdx);
#endif
		while (!rxMessage.trigger->timed_wait(std::chrono::duration<int>(1))) {
#ifdef DEBUG_RXQUEUE
			_log.Log(LOG_STATUS, "RxQueue: wait 1s for rxMessage(%lu) to be processed...", rxMessage.rxMessageIdx);
#endif
			if (m_TaskRXMessage.IsStopRequested(0)) {
				// Server is stopping
				break;
			}
		}
#ifdef DEBUG_RXQUEUE
		if (wait) {
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
	rxMessage.trigger = nullptr;
	rxMessage.BatteryLevel = 0;
	m_rxMessageQueue.push(rxMessage);
}

void MainWorker::Do_Work_On_Rx_Messages()
{
	_log.Log(LOG_STATUS, "RxQueue: queue worker started...");

	while (!m_TaskRXMessage.IsStopRequested(0))
	{
		// Wait and pop next message or timeout
		_tRxQueueItem rxQItem;
		bool hasPopped = m_rxMessageQueue.timed_wait_and_pop<std::chrono::duration<int> >(rxQItem, std::chrono::duration<int>(5));
		// (if no message for 5 seconds, returns anyway to check m_TaskRXMessage.IsStopRequested)

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
			if (rxQItem.trigger != nullptr)
				rxQItem.trigger->popped();
			continue;
		}

		const CDomoticzHardwareBase* pHardware = GetHardware(rxQItem.hardwareId);

		// Check pointers
		if (pHardware == nullptr)
		{
			_log.Log(LOG_ERROR, "RxQueue: cannot retrieve hardware with id: %d", rxQItem.hardwareId);
			if (rxQItem.trigger != nullptr)
				rxQItem.trigger->popped();
			continue;
		}
		if (rxQItem.vrxCommand.empty()) {
			_log.Log(LOG_ERROR, "RxQueue: cannot retrieve command with id: %d", rxQItem.hardwareId);
			if (rxQItem.trigger != nullptr)
				rxQItem.trigger->popped();
			continue;
		}

		const uint8_t* pRXCommand = &rxQItem.vrxCommand[0];

#ifdef DEBUG_RXQUEUE
		// CRC
		uint16_t crc = crc16ccitt(pRXCommand, rxQItem.vrxCommand.size());
		if (rxQItem.crc != crc) {
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
			pHardware->m_Name.c_str(),
			pRXCommand[1],
			pRXCommand[2]);
#endif
		ProcessRXMessage(pHardware, pRXCommand, rxQItem.Name.c_str(), rxQItem.BatteryLevel, rxQItem.UserName.c_str());
		if (rxQItem.trigger != nullptr)
		{
			rxQItem.trigger->popped();
		}
	}

	_log.Log(LOG_STATUS, "RxQueue: queue worker stopped...");
}

void MainWorker::ProcessRXMessage(const CDomoticzHardwareBase* pHardware, const uint8_t* pRXCommand, const char* defaultName, const int BatteryLevel, const char* userName)
{
	// current date/time based on current system
	//size_t Len = pRXCommand[0] + 1;

	uint64_t DeviceRowIdx = (uint64_t)-1;
	std::string DeviceName;

	_tRxMessageProcessingResult procResult;
	procResult.Username = userName;
	if (DeviceRowIdx == (uint64_t)-1)
	{
		switch (pRXCommand[1])
		{
		case pTypeInterfaceMessage:
			decode_InterfaceMessage(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeInterfaceControl:
			decode_InterfaceControl(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRecXmitMessage:
			decode_RecXmitMessage(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeUndecoded:
			decode_UNDECODED(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting1:
			decode_Lighting1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting2:
			decode_Lighting2(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting3:
			decode_Lighting3(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting4:
			decode_Lighting4(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting5:
			decode_Lighting5(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLighting6:
			decode_Lighting6(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeFan:
			decode_Fan(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeCurtain:
			decode_Curtain(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeBlinds:
			decode_BLINDS1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRFY:
			decode_RFY(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeSecurity1:
			decode_Security1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeSecurity2:
			decode_Security2(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeEvohome:
			decode_evohome1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeEvohomeZone:
		case pTypeEvohomeWater:
			decode_evohome2(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeEvohomeRelay:
			decode_evohome3(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeCamera:
			decode_Camera1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRemote:
			decode_Remote(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeSetpoint: //own type
			decode_Thermostat(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeThermostat1:
			decode_Thermostat1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeThermostat2:
			decode_Thermostat2(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeThermostat3:
			decode_Thermostat3(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeThermostat4:
			decode_Thermostat4(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRadiator1:
			decode_Radiator1(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeTEMP:
			decode_Temp(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeHUM:
			decode_Hum(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeTEMP_HUM:
			decode_TempHum(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeTEMP_RAIN:
			decode_TempRain(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeBARO:
			decode_Baro(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeTEMP_HUM_BARO:
			decode_TempHumBaro(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeTEMP_BARO:
			decode_TempBaro(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRAIN:
			decode_Rain(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeWIND:
			decode_Wind(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeUV:
			decode_UV(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeDT:
			decode_DateTime(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeCURRENT:
			decode_Current(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeENERGY:
			decode_Energy(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeCURRENTENERGY:
			decode_Current_Energy(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeGAS:
			decode_Gas(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeWATER:
			decode_Water(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeWEIGHT:
			decode_Weight(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRFXSensor:
			decode_RFXSensor(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRFXMeter:
			decode_RFXMeter(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeP1Power:
			decode_P1MeterPower(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeP1Gas:
			decode_P1MeterGas(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeUsage:
			decode_Usage(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeYouLess:
			decode_YouLessMeter(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeAirQuality:
			decode_AirQuality(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRego6XXTemp:
			decode_Rego6XXTemp(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeRego6XXValue:
			decode_Rego6XXValue(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeFS20:
			decode_FS20(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLux:
			decode_Lux(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeGeneral:
			decode_General(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeChime:
			decode_Chime(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeBBQ:
			decode_BBQ(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypePOWER:
			decode_Power(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeColorSwitch:
			decode_ColorSwitch(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeGeneralSwitch:
			decode_GeneralSwitch(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeHomeConfort:
			decode_HomeConfort(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeCARTELECTRONIC:
			decode_Cartelectronic(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeASYNCPORT:
			decode_ASyncPort(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeASYNCDATA:
			decode_ASyncData(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeWEATHER:
			decode_Weather(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeSOLAR:
			decode_Solar(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeHunter:
			decode_Hunter(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLEVELSENSOR:
			decode_LevelSensor(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeLIGHTNING:
			decode_LightningSensor(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeDDxxxx:
			decode_DDxxxx(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		case pTypeHoneywell_AL:
			decode_Honeywell(pHardware, reinterpret_cast<const tRBUF*>(pRXCommand), procResult);
			break;
		default:
			_log.Log(LOG_ERROR, "UNHANDLED PACKET TYPE:      FS20 %02X", pRXCommand[1]);
			return;
		}
		DeviceRowIdx = procResult.DeviceRowIdx;
		DeviceName = procResult.DeviceName;
	}

	if (DeviceRowIdx == (uint64_t)-1)
		return;

	if ((BatteryLevel != -1) && (procResult.bProcessBatteryValue))
	{
		m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d WHERE (ID==%" PRIu64 ")", BatteryLevel, DeviceRowIdx);
		m_eventsystem.UpdateBatteryLevel(DeviceRowIdx, BatteryLevel); //GizMoCuz, temporarily... 
	}

	if ((defaultName != nullptr) && ((DeviceName == "Unknown") || (DeviceName.empty())))
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
			const _tGeneralDevice* pMeter = reinterpret_cast<const _tGeneralDevice*>(pRXCommand);
			sdevicetype += "/" + std::string(RFX_Type_SubType_Desc(pMeter->type, pMeter->subtype));
		}
		std::string szLogString = sdevicetype + " (" + DeviceName + ")";
		const_cast<CDomoticzHardwareBase*>(pHardware)->Log(LOG_NORM, szLogString);
	}

	sOnDeviceReceived(pHardware->m_HwdID, DeviceRowIdx, DeviceName, pRXCommand);
}

void MainWorker::decode_InterfaceMessage(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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
		case trxType310:
		case trxType315:
		case recType43392:
		case trxType43392:
		case trxType868:
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
			case trxType310:
				WriteMessage("Select 310MHz");
				break;
			case trxType315:
				WriteMessage("Select 315MHz");
				break;
			case recType43392:
				WriteMessage("Select 433.92MHz");
				break;
			case trxType43392:
				WriteMessage("Select 433.92MHz (E)");
				break;
			case trxType868:
				WriteMessage("Select 868.00MHz");
				break;
			default:
				WriteMessage("Error: unknown response");
				break;
			}

			m_sql.UpdateRFXCOMHardwareDetails(pHardware->m_HwdID, pResponse->IRESPONSE.msg1, pResponse->IRESPONSE.msg2, pResponse->ICMND.msg3, pResponse->ICMND.msg4, pResponse->ICMND.msg5, pResponse->ICMND.msg6);

			switch (pResponse->IRESPONSE.msg1)
			{
			case trxType310:
				WriteMessage("Transceiver type  = 310MHz");
				break;
			case trxType315:
				WriteMessage("Receiver type     = 315MHz");
				break;
			case recType43392:
				WriteMessage("Receiver type     = 433.92MHz (receive only)");
				break;
			case trxType43392:
				WriteMessage("Transceiver type  = 433.92MHz");
				break;
			case trxType868:
				WriteMessage("Receiver type     = 868.00MHz");
				break;
			default:
				WriteMessage("Receiver type     = unknown");
				break;
			}
			int FWType = 0;
			int FWVersion = 0;
			int NoiseLevel = 0;
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

			if (
				(pResponse->IRESPONSE.msg1 == recType43392) ||
				(pResponse->IRESPONSE.msg1 == trxType43392)
				)
			{
				WriteMessage("Firmware type     = ", false);
				switch (FWType)
				{
				case FWtyperec:
					strcpy(szTmp, "Type1 RX");
					break;
				case FWtype1:
					strcpy(szTmp, "Type1");
					break;
				case FWtype2:
					strcpy(szTmp, "Type2");
					break;
				case FWtypeExt:
					strcpy(szTmp, "Ext");
					break;
				case FWtypeExt2:
					strcpy(szTmp, "Ext2");
					break;
				case FWtypePro1:
					strcpy(szTmp, "Pro1");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				case FWtypePro2:
					strcpy(szTmp, "Pro2");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				case FWtypeProXL1:
					strcpy(szTmp, "Pro XL1");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				case FWtypeProXL2:
					strcpy(szTmp, "Pro XL2");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				case FWtypeRFX433:
					strcpy(szTmp, "RFM69 433");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				case FWtypeRFX868:
					strcpy(szTmp, "RFM69 868");
					NoiseLevel = static_cast<int>(pResponse->IRESPONSE.msg11);
					break;
				default:
					strcpy(szTmp, "?");
					break;
				}
				WriteMessage(szTmp);
			}

			CRFXBase* pMyHardware = (CRFXBase*)pHardware;
			if (pMyHardware)
			{
				pMyHardware->m_Version = std::string(szTmp) + std::string("/") + std::to_string(FWVersion);

				if (FWType >= FWtypePro1)
				{
					pMyHardware->m_NoiseLevel = NoiseLevel;
					sprintf(szTmp, "Noise Level: %d", pMyHardware->m_NoiseLevel);
					WriteMessage(szTmp);
				}
				if (
					(FWType == FWtypeProXL1) || (FWType == FWtypeProXL2))
				{
					pMyHardware->SetAsyncType(pMyHardware->m_AsyncType);
				}
			}


			sprintf(szTmp, "Hardware version  = %d.%d", pResponse->IRESPONSE.msg7, pResponse->IRESPONSE.msg8);
			WriteMessage(szTmp);

			if (pResponse->IRESPONSE.msg1 != trxType868)
			{
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

				if (pResponse->IRESPONSE.LEGRANDenabled)
					WriteMessage("Legrand           enabled");
				else
					WriteMessage("Legrand           disabled");

				if (pResponse->IRESPONSE.MSG4Reserved5)
					WriteMessage("MSG4Reserved5     enabled");
				else
					WriteMessage("MSG4Reserved5     disabled");

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
			else
			{
				//868
				if (pResponse->IRESPONSE868.UNDECODEDenabled)
					WriteMessage("Undec             on");
				else
					WriteMessage("Undec             off");

				if (pResponse->IRESPONSE868.ALECTOenabled)
					WriteMessage("Alecto ACH2010    enabled");

				if (pResponse->IRESPONSE868.FINEOFFSETenabled)
					WriteMessage("Alecto WS5500     enabled");

				if (pResponse->IRESPONSE868.LACROSSEenabled)
					WriteMessage("LA Crosse         enabled");

				if (pResponse->IRESPONSE868.DAVISEUenabled)
					WriteMessage("Davis EU          enabled");

				if (pResponse->IRESPONSE868.DAVISUSenabled)
					WriteMessage("Davis US          enabled");

				if (pResponse->IRESPONSE868.DAVISAUenabled)
					WriteMessage("Davis AU          enabled");

				if (pResponse->IRESPONSE868.FS20enabled)
					WriteMessage("FS20              enabled");

				if (pResponse->IRESPONSE868.LWRFenabled)
					WriteMessage("LightwaveRF       enabled");

				if (pResponse->IRESPONSE868.EDISIOenabled)
					WriteMessage("Edisio            enabled");

				if (pResponse->IRESPONSE868.VISONICenabled)
					WriteMessage("Visonic           enabled");

				if (pResponse->IRESPONSE868.MEIANTECHenabled)
					WriteMessage("Meiantech         enabled");

				if (pResponse->IRESPONSE868.KEELOQenabled)
					WriteMessage("Keeloq            enabled");

				if (pResponse->IRESPONSE868.PROGUARDenabled)
					WriteMessage("Proguard          enabled");

				if (pResponse->IRESPONSE868.ITHOenabled)
					WriteMessage("Itho CVE RFT      enabled");

				if (pResponse->IRESPONSE868.ITHOecoenabled)
					WriteMessage("Itho CVE ECO RFT  enabled");

				if (pResponse->IRESPONSE868.HONEYWELLenabled)
					WriteMessage("Honeywell Chime   enabled");

			}
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

void MainWorker::decode_InterfaceControl(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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
		case trxType310:
			WriteMessage("select 310MHz in the 310/315 transceiver");
			break;
		case trxType315:
			WriteMessage("select 315MHz in the 310/315 transceiver");
			break;
		case recType43392:
		case trxType43392:
			WriteMessage("select 433.92MHz in the 433 transceiver");
			break;
		case trxType868:
			WriteMessage("select 868MHz in the 868 transceiver");
			break;
		}
		break;
	}
	WriteMessageEnd();
	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_BateryLevel(bool bIsInPercentage, uint8_t level)
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

uint8_t MainWorker::get_BateryLevel(const _eHardwareTypes HwdType, bool bIsInPercentage, uint8_t level)
{
	if (HwdType == HTYPE_OpenZWave)
	{
		bIsInPercentage = true;
	}
	uint8_t ret = 0;
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

void MainWorker::decode_Rain(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeRAIN;
	uint8_t subType = pResponse->RAIN.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->RAIN.id1 * 256) + pResponse->RAIN.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->RAIN.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, subType == sTypeRAIN1 || subType == sTypeRAIN9, pResponse->RAIN.battery_level & 0x0F);

	int Rainrate = 0;
	float TotalRain = 0;
	if (subType == sTypeRAIN9)
	{
		uint16_t rainCount = (pResponse->RAIN.raintotal2 * 256) + pResponse->RAIN.raintotal3 + 10;
		TotalRain = roundf(float(rainCount * 2.54F)) / 10.0F;
	}
	else if (subType == sTypeRAIN10)
	{
		uint16_t rainCount = (pResponse->RAIN.raintotal2 * 256) + pResponse->RAIN.raintotal3 + 10;
		TotalRain = roundf(float(rainCount * 0.001F)) / 10.0F;
	}
	else {
		Rainrate = (pResponse->RAIN.rainrateh * 256) + pResponse->RAIN.rainratel;
		TotalRain = float((pResponse->RAIN.raintotal1 * 65535) + (pResponse->RAIN.raintotal2 * 256) + pResponse->RAIN.raintotal3) / 10.0F;
	}

	if (subType == sTypeRAINByRate)
	{
		//calculate new Total
		TotalRain = 0;

		std::vector<std::vector<std::string>> result;

		//Get our index
		result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", pHardware->m_HwdID, ID.c_str(), Unit, devType, subType);
		if (!result.empty())
		{
			uint64_t ulID = std::stoull(result[0][0]);

			time_t now = mytime(nullptr);
			struct tm ltime;
			localtime_r(&now, &ltime);

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query(
				"SELECT Rate, Date FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%04d-%02d-%02d') ORDER BY ROWID ASC",
				ulID, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
			if (!result.empty())
			{
				time_t countTime;
				struct tm midnightTime;
				getMidnight(countTime, midnightTime);

				for (const auto& sd : result)
				{
					float rate = (float)atof(sd[0].c_str()) / 10000.0F;
					std::string date = sd[1];

					time_t rowtime;
					struct tm rowTimetm;
					ParseSQLdatetime(rowtime, rowTimetm, date);

					int pastSeconds = (int)(rowtime - countTime);

					float rateAdd = (rate / (float)3600 * (float)pastSeconds);
					TotalRain += rateAdd;

					countTime = rowtime;
				}
			}
		}
	}
	else if (subType != sTypeRAINWU)
	{
		Rainrate = 0;
		//Calculate our own rainrate
		std::vector<std::vector<std::string>> result;

		//Get our index
		result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", pHardware->m_HwdID, ID.c_str(), Unit, devType, subType);
		if (!result.empty())
		{
			uint64_t ulID = std::stoull(result[0][0]);

			//Get Counter from one Hour ago
			time_t now = mytime(nullptr);
			now -= 3600; //subtract one hour
			struct tm ltime;
			localtime_r(&now, &ltime);

			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query(
				"SELECT MIN(Total) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%04d-%02d-%02d %02d:%02d:%02d')",
				ulID, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
			if (result.size() == 1)
			{
				float totalRainFallLastHour = TotalRain - static_cast<float>(atof(result[0][0].c_str()));
				Rainrate = ground(totalRainFallLastHour * 100.0F);
			}
		}
	}

	sprintf(szTmp, "%d;%.1f", Rainrate, TotalRain);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeRAIN7:
			WriteMessage("subtype       = RAIN7 - Alecto");
			break;
		case sTypeRAIN8:
			WriteMessage("subtype       = RAIN8 - Davis");
			break;
		case sTypeRAIN9:
			WriteMessage("subtype       = RAIN9 - TFA 30.3233.01");
			break;
		case sTypeRAIN10:
			WriteMessage("subtype       = RAIN10 - WH5360,WH5360B,WH40");
			break;
		case sTypeRAINWU:
			WriteMessage("subtype       = Weather Underground (Total Rain)");
			break;
		case sTypeRAINByRate:
			WriteMessage("subtype       = DarkSky for example (Only rate, no total, no counter) rate in mm/hour x 10000, so all decimals will fit");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X : %02X", pResponse->RAIN.packettype, subType);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->RAIN.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "ID            = %s", ID.c_str());
		WriteMessage(szTmp);

		if (subType == sTypeRAIN1)
		{
			sprintf(szTmp, "Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}
		else if (subType == sTypeRAIN2)
		{
			sprintf(szTmp, "Rain rate     = %d mm/h", Rainrate);
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Total rain    = %.1f mm", TotalRain);
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->RAIN.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(subType == sTypeRAIN1, pResponse->RAIN.battery_level & 0x0F);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Wind(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[300];
	uint8_t devType = pTypeWIND;
	uint8_t subType = pResponse->WIND.subtype;
	uint16_t windID = (pResponse->WIND.id1 * 256) + pResponse->WIND.id2;
	sprintf(szTmp, "%d", windID);
	std::string ID = szTmp;
	uint8_t Unit = 0;

	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->WIND.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, pResponse->WIND.subtype == sTypeWIND3, pResponse->WIND.battery_level & 0x0F);

	float AddjValue = 0.0F; //Temp adjustment
	float AddjMulti = 1.0F; //Wind Speed/Gust adjustment
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);

	float AddjValue2 = 0.0F; //Wind direction adjustment
	float AddjMulti2 = 1.0F;
	m_sql.GetAddjustment2(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue2, AddjMulti2);

	double dDirection;
	dDirection = (double)(pResponse->WIND.directionh * 256) + pResponse->WIND.directionl;

	//Apply user defined offset
	dDirection = std::fmod(dDirection + AddjValue2, 360.0);

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

	dDirection = ground(dDirection);

	int intSpeed = (pResponse->WIND.av_speedh * 256) + pResponse->WIND.av_speedl;
	int intGust = (pResponse->WIND.gusth * 256) + pResponse->WIND.gustl;

	//Apply user defind multiplication
	intSpeed = int(float(intSpeed) * AddjMulti);
	intGust = int(float(intGust) * AddjMulti);

	if (pResponse->WIND.subtype == sTypeWIND6)
	{
		//LaCrosse WS2300
		//This sensor is only reporting gust, speed=gust
		intSpeed = intGust;
	}

	m_wind_calculator[windID].SetSpeedGust(intSpeed, intGust);

	float temp = 0, chill = 0;
	if (subType != sTypeWINDNoTempNoChill)
	{
		if (pResponse->WIND.subtype == sTypeWIND4)
		{
			if (!pResponse->WIND.tempsign)
			{
				temp = float((pResponse->WIND.temperatureh * 256) + pResponse->WIND.temperaturel) / 10.0F;
			}
			else
			{
				temp = -(float(((pResponse->WIND.temperatureh & 0x7F) * 256) + pResponse->WIND.temperaturel) / 10.0F);
			}
			if ((temp < -200) || (temp > 380))
			{
				WriteMessage(" Invalid Temperature");
				return;
			}

			temp += AddjValue;

			if (!pResponse->WIND.chillsign)
			{
				chill = float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0F;
			}
			else
			{
				chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0F);
			}
			chill += AddjValue;
		}
		else if (pResponse->WIND.subtype == sTypeWINDNoTemp)
		{
			temp += AddjValue;

			if (!pResponse->WIND.chillsign)
			{
				chill = float((pResponse->WIND.chillh * 256) + pResponse->WIND.chilll) / 10.0F;
			}
			else
			{
				chill = -(float(((pResponse->WIND.chillh) & 0x7F) * 256 + pResponse->WIND.chilll) / 10.0F);
			}
			chill += AddjValue;
		}
		if (chill == 0)
		{
			float wspeedms = float(intSpeed) / 10.0F;
			if ((temp < 10.0) && (wspeedms >= 1.4))
			{
				float chillJatTI = 13.12F + 0.6215F * temp - 11.37F * pow(wspeedms * 3.6F, 0.16F) + 0.3965F * temp * pow(wspeedms * 3.6F, 0.16F);
				chill = chillJatTI;
			}
			else
			{
				chill = temp;
			}
		}
	}

	sprintf(szTmp, "%.2f;%s;%d;%d;%.1f;%.1f", dDirection, strDirection.c_str(), intSpeed, intGust, temp, chill);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(chill), _tTrendCalculator::TAVERAGE_TEMP);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeWINDNoTempNoChill:
			WriteMessage("subtype       = Wind (No Temp or Chill sensors");
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
			sprintf(szTmp, "Average speed = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intSpeed) / 10.0F, (float(intSpeed) * 0.36F), (float(intSpeed) * 0.223693629F));
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Wind gust     = %.1f mtr/sec, %.2f km/hr, %.2f mph", float(intGust) / 10.0F, (float(intGust) * 0.36F), (float(intGust) * 0.223693629F));
		WriteMessage(szTmp);

		if (pResponse->WIND.subtype == sTypeWIND4)
		{
			sprintf(szTmp, "Temperature   = %.1f C", temp);
			WriteMessage(szTmp);

			sprintf(szTmp, "Chill         = %.1f C", chill);
			WriteMessage(szTmp);
		}
		if (pResponse->WIND.subtype == sTypeWINDNoTemp)
		{
			sprintf(szTmp, "Chill         = %.1f C", chill);
			WriteMessage(szTmp);
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->WIND.rssi);
		WriteMessage(szTmp);

		decode_BateryLevel(pResponse->WIND.subtype == sTypeWIND3, pResponse->WIND.battery_level & 0x0F);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Temp(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeTEMP;
	uint8_t subType = pResponse->TEMP.subtype;
	sprintf(szTmp, "%d", (pResponse->TEMP.id1 * 256) + pResponse->TEMP.id2);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->TEMP.id2;

	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->TEMP.rssi;
	uint8_t BatteryLevel = 0;
	if ((pResponse->TEMP.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	//Override battery level if hardware supports it
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level * 10;
	}
	else if ((pHardware->HwdType == HTYPE_EnOceanESP2) || (pHardware->HwdType == HTYPE_EnOceanESP3))
	{
		// WARNING
		// battery_level & rssi fields fields are used here to transmit ID_BYTE0 value from EnOcean device
		// Set BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
		BatteryLevel = 255;
		SignalLevel = 12;
		// Set Unit = ID_BYTE0
		Unit = (pResponse->TEMP.rssi << 4) | pResponse->TEMP.battery_level;
	}

	float temp;
	if (!pResponse->TEMP.tempsign)
	{
		temp = float((pResponse->TEMP.temperatureh * 256) + pResponse->TEMP.temperaturel) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->TEMP.temperatureh & 0x7F) * 256) + pResponse->TEMP.temperaturel) / 10.0F);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	sprintf(szTmp, "%.1f", temp);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);

	bool bHandledNotification = false;
	uint8_t humidity = 0;
	if (pResponse->TEMP.subtype == sTypeTEMP5)
	{
		//check if we already had a humidity for this device, if so, keep it!
		char szTmp[300];
		std::vector<std::vector<std::string> > result;

		result = m_sql.safe_query(
			"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			pHardware->m_HwdID, ID.c_str(), 1, pTypeHUM, sTypeHUM1);
		if (result.size() == 1)
		{
			m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, AddjValue, AddjMulti);
			temp += AddjValue;
			humidity = atoi(result[0][0].c_str());
			uint8_t humidity_status = atoi(result[0][1].c_str());
			sprintf(szTmp, "%.1f;%d;%d", temp, humidity, humidity_status);
			DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, SignalLevel, BatteryLevel, 0, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
			m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, pTypeTEMP_HUM, sTypeTH_LC_TC, szTmp);

			bHandledNotification = true;
		}
	}

	if (!bHandledNotification)
		m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, temp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Hum(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeHUM;
	uint8_t subType = pResponse->HUM.subtype;
	sprintf(szTmp, "%d", (pResponse->HUM.id1 * 256) + pResponse->HUM.id2);
	std::string ID = szTmp;
	uint8_t Unit = 1;

	uint8_t SignalLevel = pResponse->HUM.rssi;
	uint8_t BatteryLevel = 0;
	if ((pResponse->HUM.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	//Override battery level if hardware supports it
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level;
	}

	uint8_t humidity = pResponse->HUM.humidity;
	if (humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}

	sprintf(szTmp, "%d", pResponse->HUM.humidity_status);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, humidity, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
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
			pHardware->m_HwdID, ID.c_str(), 0, pTypeTEMP, sTypeTEMP5);
		if (result.size() == 1)
		{
			temp = static_cast<float>(atof(result[0][0].c_str()));
			float AddjValue = 0.0F;
			float AddjMulti = 1.0F;
			m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, AddjValue, AddjMulti);
			temp += AddjValue;
			sprintf(szTmp, "%.1f;%d;%d", temp, humidity, pResponse->HUM.humidity_status);
			DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 2, pTypeTEMP_HUM, sTypeTH_LC_TC, SignalLevel, BatteryLevel, 0, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
			m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, pTypeTEMP_HUM, sTypeTH_LC_TC, subType, szTmp);
			bHandledNotification = true;
		}
	}
	if (!bHandledNotification)
		m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, (const int)humidity);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeHUM3:
			WriteMessage("subtype       = HUM3 - Inovalley S80 plant humidity sensor");
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

void MainWorker::decode_TempHum(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeTEMP_HUM;
	uint8_t subType = pResponse->TEMP_HUM.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->TEMP_HUM.id1 * 256) + pResponse->TEMP_HUM.id2);
	ID = szTmp;
	uint8_t Unit = 0;

	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->TEMP_HUM.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, pResponse->TEMP_HUM.subtype == sTypeTH8, pResponse->TEMP_HUM.battery_level);

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
		temp = float((pResponse->TEMP_HUM.temperatureh * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_HUM.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM.temperaturel) / 10.0F);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	int Humidity = (int)pResponse->TEMP_HUM.humidity;
	uint8_t HumidityStatus = pResponse->TEMP_HUM.humidity_status;

	if (Humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}
	/*
	AddjValue=0.0f;
	AddjMulti=1.0f;
	m_sql.GetAddjustment2(pHardware->m_HwdID, ID.c_str(),Unit,devType,subType,AddjValue,AddjMulti);
	Humidity+=int(AddjValue);
	if (Humidity>100)
	Humidity=100;
	if (Humidity<0)
	Humidity=0;
	*/
	sprintf(szTmp, "%.1f;%d;%d", temp, Humidity, HumidityStatus);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_TempHumBaro(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeTEMP_HUM_BARO;
	uint8_t subType = pResponse->TEMP_HUM_BARO.subtype;
	sprintf(szTmp, "%d", (pResponse->TEMP_HUM_BARO.id1 * 256) + pResponse->TEMP_HUM_BARO.id2);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->TEMP_HUM_BARO.id2;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->TEMP_HUM_BARO.rssi;
	uint8_t BatteryLevel;
	if ((pResponse->TEMP_HUM_BARO.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	//Override battery level if hardware supports it
	if (pHardware->HwdType == HTYPE_OpenZWave)
	{
		BatteryLevel = pResponse->TEMP.battery_level;
	}

	float temp;
	if (!pResponse->TEMP_HUM_BARO.tempsign)
	{
		temp = float((pResponse->TEMP_HUM_BARO.temperatureh * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_HUM_BARO.temperatureh & 0x7F) * 256) + pResponse->TEMP_HUM_BARO.temperaturel) / 10.0F);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	uint8_t Humidity = pResponse->TEMP_HUM_BARO.humidity;
	uint8_t HumidityStatus = pResponse->TEMP_HUM_BARO.humidity_status;

	if (Humidity > 100)
	{
		WriteMessage(" Invalid Humidity");
		return;
	}

	int barometer = (pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol;

	int forcast = pResponse->TEMP_HUM_BARO.forecast;
	float fbarometer = (float)barometer;

	m_sql.GetAddjustment2(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	barometer += int(AddjValue);

	if (pResponse->TEMP_HUM_BARO.subtype == sTypeTHBFloat)
	{
		if ((barometer < 8000) || (barometer > 12000))
		{
			WriteMessage(" Invalid Barometer");
			return;
		}
		fbarometer = float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol) / 10.0F;
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
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);

	//calculate Altitude
	//float seaLevelPressure=101325.0f;
	//float altitude = 44330.0f * (1.0f - pow(fbarometer / seaLevelPressure, 0.1903f));

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
			sprintf(szTmp, "Barometer     = %.1f hPa", float((pResponse->TEMP_HUM_BARO.baroh * 256) + pResponse->TEMP_HUM_BARO.barol) / 10.0F);
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

void MainWorker::decode_TempBaro(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeTEMP_BARO;
	uint8_t subType = sTypeBMP085;
	_tTempBaro* pTempBaro = (_tTempBaro*)pResponse;

	sprintf(szTmp, "%d", pTempBaro->id1);
	std::string ID = szTmp;
	uint8_t Unit = 1;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel;
	BatteryLevel = 100;

	float temp = pTempBaro->temp;
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	float fbarometer = pTempBaro->baro;
	int forcast = pTempBaro->forecast;
	m_sql.GetAddjustment2(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	fbarometer += AddjValue;

	sprintf(szTmp, "%.1f;%.1f;%d;%.2f", temp, fbarometer, forcast, pTempBaro->altitude);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_TempRain(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];

	//We are (also) going to split this device into two separate sensors (temp + rain)

	uint8_t devType = pTypeTEMP_RAIN;
	uint8_t subType = pResponse->TEMP_RAIN.subtype;

	sprintf(szTmp, "%d", (pResponse->TEMP_RAIN.id1 * 256) + pResponse->TEMP_RAIN.id2);
	std::string ID = szTmp;
	int Unit = pResponse->TEMP_RAIN.id2;
	int cmnd = 0;

	uint8_t SignalLevel = pResponse->TEMP_RAIN.rssi;
	uint8_t BatteryLevel = 0;
	if ((pResponse->TEMP_RAIN.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	float temp;
	if (!pResponse->TEMP_RAIN.tempsign)
	{
		temp = float((pResponse->TEMP_RAIN.temperatureh * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->TEMP_RAIN.temperatureh & 0x7F) * 256) + pResponse->TEMP_RAIN.temperaturel) / 10.0F);
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, pTypeTEMP, sTypeTEMP3, AddjValue, AddjMulti);
	temp += AddjValue;

	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}
	float TotalRain = float((pResponse->TEMP_RAIN.raintotal1 * 256) + pResponse->TEMP_RAIN.raintotal2) / 10.0F;

	sprintf(szTmp, "%.1f;%.1f", temp, TotalRain);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	uint64_t tID = ((uint64_t)(pHardware->m_HwdID & 0x7FFFFFFF) << 32) | (DevRowIdx & 0x7FFFFFFF);
	m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);

	sprintf(szTmp, "%.1f", temp);
	uint64_t DevRowIdxTemp = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, pTypeTEMP, sTypeTEMP3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	m_notifications.CheckAndHandleNotification(DevRowIdxTemp, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, pTypeTEMP, sTypeTEMP3, temp);

	sprintf(szTmp, "%d;%.1f", 0, TotalRain);
	uint64_t DevRowIdxRain = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, pTypeRAIN, sTypeRAIN3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	m_notifications.CheckAndHandleNotification(DevRowIdxRain, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, pTypeRAIN, sTypeRAIN3, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_UV(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeUV;
	uint8_t subType = pResponse->UV.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->UV.id1 * 256) + pResponse->UV.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->UV.rssi;
	uint8_t BatteryLevel;
	if ((pResponse->UV.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;
	float Level = float(pResponse->UV.uv) / 10.0F;
	float AddjValue2 = 0.0F;
	float AddjMulti2 = 1.0F;
	m_sql.GetAddjustment2(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue2, AddjMulti2);
	Level *= AddjMulti2;
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
			temp = float((pResponse->UV.temperatureh * 256) + pResponse->UV.temperaturel) / 10.0F;
		}
		else
		{
			temp = -(float(((pResponse->UV.temperatureh & 0x7F) * 256) + pResponse->UV.temperaturel) / 10.0F);
		}
		if ((temp < -200) || (temp > 380))
		{
			WriteMessage(" Invalid Temperature");
			return;
		}

		float AddjValue = 0.0F;
		float AddjMulti = 1.0F;
		m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
	}

	sprintf(szTmp, "%.1f;%.1f", Level, temp);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

		if (Level < 3)
			WriteMessage("Description = Low");
		else if (Level < 6)
			WriteMessage("Description = Medium");
		else if (Level < 8)
			WriteMessage("Description = High");
		else if (Level < 11)
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

void MainWorker::decode_FS20(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	//uint8_t devType=pTypeFS20;

	char szTmp[100];
	uint8_t devType = pTypeFS20;
	uint8_t subType = pResponse->FS20.subtype;

	sprintf(szTmp, "%02X%02X", pResponse->FS20.hc1, pResponse->FS20.hc2);
	std::string ID = szTmp;

	uint8_t Unit = pResponse->FS20.addr;
	uint8_t cmnd = pResponse->FS20.cmd1;
	uint8_t SignalLevel = pResponse->FS20.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
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
				sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55F);
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
				sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55F);
				WriteMessage(szTmp);
				break;
			case 0x8:
				WriteMessage("                relative offset (cmd2 bit 7=direction, bit 5-0 offset value)");
				break;
			case 0xA:
				WriteMessage("                decalcification cycle");
				sprintf(szTmp, "Cmd2          = valve position: %02X is %.2f %%", pResponse->FS20.cmd2, float(pResponse->FS20.cmd2) / 2.55F);
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
		WriteMessageEnd();
	}

	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Lighting1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeLighting1;
	uint8_t subType = pResponse->LIGHTING1.subtype;
	sprintf(szTmp, "%d", pResponse->LIGHTING1.housecode);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->LIGHTING1.unitcode;
	uint8_t cmnd = pResponse->LIGHTING1.cmnd;
	uint8_t SignalLevel = pResponse->LIGHTING1.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeOase:
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
			case sTypeOase:
				WriteMessage("subtype       = Oase Inscenio");
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

void MainWorker::decode_Lighting2(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeLighting2;
	uint8_t subType = pResponse->LIGHTING2.subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pResponse->LIGHTING2.id1, pResponse->LIGHTING2.id2, pResponse->LIGHTING2.id3, pResponse->LIGHTING2.id4);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->LIGHTING2.unitcode;
	uint8_t cmnd = pResponse->LIGHTING2.cmnd;
	uint8_t level = pResponse->LIGHTING2.level;
	uint8_t SignalLevel = pResponse->LIGHTING2.rssi;

	sprintf(szTmp, "%d", level);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

	bool isGroupCommand = ((cmnd == light2_sGroupOff) || (cmnd == light2_sGroupOn));
	uint8_t single_cmnd = cmnd;

	if (isGroupCommand)
	{
		single_cmnd = (cmnd == light2_sGroupOff) ? light2_sOff : light2_sOn;

		// We write the GROUP_CMD into the log to differentiate between manual turn off/on and group_off/group_on
		m_sql.UpdateValueLighting2GroupCmd(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

		//set the status of all lights with the same code to on/off
		m_sql.Lighting2GroupCmd(ID, subType, single_cmnd);
	}

	if (DevRowIdx == (uint64_t)-1) {
		// not found nothing to do
		return;
	}
	CheckSceneCode(DevRowIdx, devType, subType, single_cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
			case gswitch_sStop:
				WriteMessage("Stop");
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
void MainWorker::decode_Lighting3(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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

void MainWorker::decode_Lighting4(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeLighting4;
	uint8_t subType = pResponse->LIGHTING4.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING4.cmd1, pResponse->LIGHTING4.cmd2, pResponse->LIGHTING4.cmd3);
	std::string ID = szTmp;
	int Unit = 0;
	uint8_t cmnd = 1; //only 'On' supported
	uint8_t SignalLevel = pResponse->LIGHTING4.rssi;
	sprintf(szTmp, "%d", (pResponse->LIGHTING4.pulseHigh * 256) + pResponse->LIGHTING4.pulseLow);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Lighting5(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeLighting5;
	uint8_t subType = pResponse->LIGHTING5.subtype;
	if ((subType != sTypeEMW100) && (subType != sTypeLivolo) && (subType != sTypeLivolo1to10) && (subType != sTypeRGB432W) && (subType != sTypeKangtai))
		sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING5.id1, pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	else
		sprintf(szTmp, "%02X%02X", pResponse->LIGHTING5.id2, pResponse->LIGHTING5.id3);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->LIGHTING5.unitcode;
	uint8_t cmnd = pResponse->LIGHTING5.cmnd;
	float flevel;
	if (subType == sTypeLivolo)
		flevel = (100.0F / 7.0F) * float(pResponse->LIGHTING5.level);
	else if (subType == sTypeLightwaveRF)
		flevel = (100.0F / 31.0F) * float(pResponse->LIGHTING5.level);
	else
		flevel = (100.0F / 7.0F) * float(pResponse->LIGHTING5.level);
	uint8_t SignalLevel = pResponse->LIGHTING5.rssi;

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
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
		CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);
	}

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeLivolo1to10:
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

void MainWorker::decode_Lighting6(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeLighting6;
	uint8_t subType = pResponse->LIGHTING6.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->LIGHTING6.id1, pResponse->LIGHTING6.id2, pResponse->LIGHTING6.groupcode);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->LIGHTING6.unitcode;
	uint8_t cmnd = pResponse->LIGHTING6.cmnd;
	uint8_t rfu = pResponse->LIGHTING6.seqnbr2;
	uint8_t SignalLevel = pResponse->LIGHTING6.rssi;

	sprintf(szTmp, "%d", rfu);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Fan(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeFan;
	uint8_t subType = pResponse->FAN.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->FAN.id1, pResponse->FAN.id2, pResponse->FAN.id3);
	std::string ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = pResponse->FAN.cmnd;
	uint8_t SignalLevel = pResponse->FAN.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeSeavTXS4:
			WriteMessage("subtype       = SEAV TXS4");
			break;
		case sTypeWestinghouse:
			WriteMessage("subtype       = Westinghouse");
			break;
		case sTypeLucciAirDC:
			WriteMessage("subtype       = Lucci Air DC");
			break;
		case sTypeCasafan:
			WriteMessage("subtype       = Casafan");
			break;
		case sTypeFT1211R:
			WriteMessage("subtype       = FT1211R");
			break;
		case sTypeFalmec:
			WriteMessage("subtype       = Falmec");
			break;
		case sTypeLucciAirDCII:
			WriteMessage("subtype       = Lucci Air DC II");
			break;
		case sTypeIthoECO:
			WriteMessage("subtype       = Itho ECO");
			break;
		case sTypeNovy:
			WriteMessage("subtype       = Novy");
			break;
		case sTypeOrcon:
			WriteMessage("subtype       = Orcon");
			break;
		case sTypeIthoHRU400:
			WriteMessage("subtype       = Itho HRU400");
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

void MainWorker::decode_HomeConfort(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeHomeConfort;
	uint8_t subType = pResponse->HOMECONFORT.subtype;
	sprintf(szTmp, "%02X%02X%02X%02X", pResponse->HOMECONFORT.id1, pResponse->HOMECONFORT.id2, pResponse->HOMECONFORT.id3, pResponse->HOMECONFORT.housecode);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->HOMECONFORT.unitcode;
	uint8_t cmnd = pResponse->HOMECONFORT.cmnd;
	uint8_t SignalLevel = pResponse->HOMECONFORT.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());

	bool isGroupCommand = ((cmnd == HomeConfort_sGroupOff) || (cmnd == HomeConfort_sGroupOn));
	uint8_t single_cmnd = cmnd;

	if (isGroupCommand)
	{
		single_cmnd = (cmnd == HomeConfort_sGroupOff) ? HomeConfort_sOff : HomeConfort_sOn;

		// We write the GROUP_CMD into the log to differentiate between manual turn off/on and group_off/group_on
		m_sql.UpdateValueHomeConfortGroupCmd(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

		//set the status of all lights with the same code to on/off
		m_sql.HomeConfortGroupCmd(ID, subType, single_cmnd);
	}

	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_ColorSwitch(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[300];
	const _tColorSwitch* pLed = reinterpret_cast<const _tColorSwitch*>(pResponse);
	uint8_t devType = pTypeColorSwitch;
	uint8_t subType = pLed->subtype;
	if (pLed->id == 1)
		sprintf(szTmp, "%d", 1);
	else
		sprintf(szTmp, "%08X", (unsigned int)pLed->id);
	std::string ID = szTmp;
	uint8_t Unit = pLed->dunit;
	uint8_t cmnd = pLed->command;
	uint32_t value = pLed->value;
	_tColor color = pLed->color;

	char szValueTmp[100];
	sprintf(szValueTmp, "%u", value);
	std::string sValue = szValueTmp;

	uint64_t DevRowIdx = (uint64_t)-1;

	auto result = m_sql.safe_query(
		"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
		pHardware->m_HwdID, ID.c_str(), Unit, devType, subType);
	if (!result.empty())
	{
		DevRowIdx = std::stoull(result[0][0]);
	}
	else
	{
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, 12, -1, cmnd, sValue.c_str(), procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}

	// TODO: Why don't we let database update be handled by CSQLHelper::UpdateValue?
	if (cmnd == Color_SetBrightnessLevel || cmnd == Color_SetColor)
	{
		//store light level
		m_sql.safe_query(
			"UPDATE DeviceStatus SET LastLevel='%d' WHERE (ID = %" PRIu64 ")",
			value,
			DevRowIdx);
	}

	if (cmnd == Color_SetColor)
	{
		//store color in database
		m_sql.safe_query(
			"UPDATE DeviceStatus SET Color='%q' WHERE (ID = %" PRIu64 ")",
			color.toJSONString().c_str(),
			DevRowIdx);
	}

	DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, 12, -1, cmnd, sValue.c_str(), procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Chime(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeChime;
	uint8_t subType = pResponse->CHIME.subtype;

	if (pResponse->CHIME.subtype == sTypeByronBY)
		sprintf(szTmp, "%02X%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2, pResponse->CHIME.sound);
	else
		sprintf(szTmp, "%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);

	std::string ID = szTmp;
	uint8_t Unit = pResponse->CHIME.sound;
	uint8_t cmnd = pResponse->CHIME.sound;
	uint8_t SignalLevel = pResponse->CHIME.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeByronBY:
			WriteMessage("subtype       = SelectPlus200689103");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2, pResponse->CHIME.sound);
			WriteMessage(szTmp);
			break;
		case sTypeEnvivo:
			WriteMessage("subtype       = Envivo");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->CHIME.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %02X%02X", pResponse->CHIME.id1, pResponse->CHIME.id2);
			WriteMessage(szTmp);
			break;
		case sTypeAlfawise:
			WriteMessage("subtype       = Alfawise");
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


void MainWorker::decode_UNDECODED(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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
	uint8_t* pRXBytes = (uint8_t*)&pResponse->UNDECODED.msg1;
	for (int i = 0; i < pResponse->UNDECODED.packetlength - 3; i++)
		sHexDump << HEX(pRXBytes[i]);
	WriteMessage(sHexDump.str().c_str());
	procResult.DeviceRowIdx = -1;
}


void MainWorker::decode_RecXmitMessage(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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

void MainWorker::decode_Curtain(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeCurtain;
	uint8_t subType = pResponse->CURTAIN1.subtype;
	sprintf(szTmp, "%d", pResponse->CURTAIN1.housecode);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->CURTAIN1.unitcode;
	uint8_t cmnd = pResponse->CURTAIN1.cmnd;
	uint8_t SignalLevel = 9;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_BLINDS1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeBlinds;
	uint8_t subType = pResponse->BLINDS1.subtype;

	sprintf(szTmp, "%02X%02X%02X%02X", pResponse->BLINDS1.id1, pResponse->BLINDS1.id2, pResponse->BLINDS1.id3, pResponse->BLINDS1.id4);

	std::string ID = szTmp;
	uint8_t Unit = pResponse->BLINDS1.unitcode;
	uint8_t cmnd = pResponse->BLINDS1.cmnd;
	uint8_t SignalLevel = pResponse->BLINDS1.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		case sTypeBlindsT14:
			WriteMessage("subtype       = Hualite");
			break;
		case sTypeBlindsT15:
			WriteMessage("subtype       = RFU");
			break;
		case sTypeBlindsT16:
			WriteMessage("subtype       = Zemismart");
			break;
		case sTypeBlindsT17:
			WriteMessage("subtype       = Gaposa");
			break;
		case sTypeBlindsT18:
			WriteMessage("subtype       = Cherubini");
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
		case blinds_sLowerLimit:
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

void MainWorker::decode_RFY(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeRFY;
	uint8_t subType = pResponse->RFY.subtype;
	sprintf(szTmp, "%02X%02X%02X", pResponse->RFY.id1, pResponse->RFY.id2, pResponse->RFY.id3);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->RFY.unitcode;
	uint8_t cmnd = pResponse->RFY.cmnd;
	uint8_t SignalLevel = pResponse->RFY.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_evohome1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	if (
		pHardware->HwdType != HTYPE_EVOHOME_SERIAL
		&& pHardware->HwdType != HTYPE_EVOHOME_SCRIPT
		&& pHardware->HwdType != HTYPE_EVOHOME_WEB
		&& pHardware->HwdType != HTYPE_EVOHOME_TCP
		)
		return; //not for us!

	char szTmp[100];
	const _tEVOHOME1* pEvo = reinterpret_cast<const _tEVOHOME1*>(pResponse);
	uint8_t devType = pTypeEvohome;
	uint8_t subType = pEvo->subtype;
	std::stringstream szID;
	if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		szID << std::hex << (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3);
	else //GB3: web based evohome uses decimal device ID's
		szID << std::dec << (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3);
	std::string ID(szID.str());
	uint8_t Unit = 0;
	uint8_t cmnd = pEvo->status;
	uint8_t SignalLevel = 255;//Unknown
	uint8_t BatteryLevel = 255;//Unknown

	std::string szUntilDate;
	if (pEvo->mode == CEvohomeBase::cmTmp)//temporary
		szUntilDate = CEvohomeDateTime::GetISODate(pEvo);

	CEvohomeBase* pEvoHW = (CEvohomeBase*)pHardware;

	//FIXME A similar check is also done in switch modal do we want to forward the ooc flag and rely on this check entirely?
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q')",
		pHardware->m_HwdID, ID.c_str());
	bool bNewDev = false;
	std::string name;
	if (!result.empty())
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

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szUntilDate.c_str(), procResult.DeviceName, pEvo->action != 0, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	if (bNewDev)
	{
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")",
			name.c_str(), DevRowIdx);
		procResult.DeviceName = name;
	}

	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);
	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (pEvo->subtype)
		{
		case sTypeEvohome:
			WriteMessage("subtype       = Evohome");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pEvo->type, pEvo->subtype);
			WriteMessage(szTmp);
			break;
		}

		if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
			sprintf(szTmp, "id            = %02X:%02X:%02X", pEvo->id1, pEvo->id2, pEvo->id3);
		else //GB3: web based evohome uses decimal device ID's
			sprintf(szTmp, "id            = %u", (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3));
		WriteMessage(szTmp);
		sprintf(szTmp, "action        = %d", (int)pEvo->action);
		WriteMessage(szTmp);
		WriteMessage("status        = ");
		WriteMessage(pEvoHW->GetControllerModeName(pEvo->status));

		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_evohome2(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	if (
		pHardware->HwdType != HTYPE_EVOHOME_SERIAL
		&& pHardware->HwdType != HTYPE_EVOHOME_SCRIPT
		&& pHardware->HwdType != HTYPE_EVOHOME_WEB
		&& pHardware->HwdType != HTYPE_EVOHOME_TCP
		)
		return; //not for us!

	char szTmp[100];
	const _tEVOHOME2* pEvo = reinterpret_cast<const _tEVOHOME2*>(pResponse);
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 255;//Unknown
	uint8_t BatteryLevel = 255;//Unknown

	//Get Device details
	std::vector<std::vector<std::string> > result;
	if (pEvo->type == pTypeEvohomeZone && pEvo->zone > 12) //Allow for additional Zone Temp devices which require DeviceID
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%x') AND (Type==%d)",
			pHardware->m_HwdID, (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3), (int)pEvo->type);
	}
	else if (pEvo->zone)//if unit number is available the id3 will be the controller device id
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit == %d) AND (Type==%d)",
			pHardware->m_HwdID, (int)pEvo->zone, (int)pEvo->type);
	}
	else//unit number not available then id3 should be the zone device id
	{
		result = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%x') AND (Type==%d)",
			pHardware->m_HwdID, (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3), (int)pEvo->type);
	}
	if (result.empty() && !pEvo->zone)
		return;

	CEvohomeBase* pEvoHW = (CEvohomeBase*)pHardware;
	bool bNewDev = false;
	std::string name, szDevID;
	std::stringstream szID;
	uint8_t Unit;
	uint8_t dType;
	uint8_t dSubType;
	std::string szUpdateStat;
	if (!result.empty())
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
		Unit = pEvo->zone;//should always be non zero
		dType = pEvo->type;
		dSubType = pEvo->subtype;

		szID << std::hex << (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3);
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

	if (pEvo->updatetype == CEvohomeBase::updBattery)
		BatteryLevel = pEvo->battery_level;
	else
	{
		if (dType == pTypeEvohomeWater && pEvo->updatetype == pEvoHW->updSetPoint && !pEvo->temperature)
			sprintf(szTmp, "%s", "Off");
		else
			sprintf(szTmp, "%.2f", pEvo->temperature / 100.0F);

		std::vector<std::string> strarray;
		StringSplit(szUpdateStat, ";", strarray);
		if (strarray.size() >= 3)
		{
			if (pEvo->updatetype == pEvoHW->updSetPoint)//SetPoint
			{
				strarray[1] = szTmp;
				if (pEvo->mode <= pEvoHW->zmTmp)//for the moment only update this if we get a valid setpoint mode as we can now send setpoint on its own
				{
					int nControllerMode = pEvo->controllermode;
					if (dType == pTypeEvohomeWater && (nControllerMode == pEvoHW->cmEvoHeatingOff || nControllerMode == pEvoHW->cmEvoAutoWithEco || nControllerMode == pEvoHW->cmEvoCustom))//dhw has no economy mode and does not turn off for heating off also appears custom does not support the dhw zone
						nControllerMode = pEvoHW->cmEvoAuto;
					if (pEvo->mode == pEvoHW->zmAuto || nControllerMode == pEvoHW->cmEvoHeatingOff)//if zonemode is auto (followschedule) or controllermode is heatingoff
						strarray[2] = pEvoHW->GetWebAPIModeName(nControllerMode);//the web front end ultimately uses these names for images etc.
					else
						strarray[2] = pEvoHW->GetZoneModeName(pEvo->mode);
					if (pEvo->mode == pEvoHW->zmTmp)
					{
						std::string szISODate(CEvohomeDateTime::GetISODate(pEvo));
						if (strarray.size() < 4) //add or set until
							strarray.push_back(szISODate);
						else
							strarray[3] = szISODate;
					}
					else if ((pEvo->mode == pEvoHW->zmAuto) && (pHardware->HwdType == HTYPE_EVOHOME_WEB))
					{
						strarray[2] = "FollowSchedule";
						if ((pEvo->year != 0) && (pEvo->year != 0xFFFF))
						{
							std::string szISODate(CEvohomeDateTime::GetISODate(pEvo));
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
			else if (pEvo->updatetype == pEvoHW->updOverride)
			{
				strarray[2] = pEvoHW->GetZoneModeName(pEvo->mode);
				if (strarray.size() >= 4) //remove until
					strarray.resize(3);
			}
			else
				strarray[0] = szTmp;
			szUpdateStat = boost::algorithm::join(strarray, ";");
		}
	}
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, szDevID.c_str(), Unit, dType, dSubType, SignalLevel, BatteryLevel, cmnd, szUpdateStat.c_str(), procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	if (bNewDev)
	{
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q' WHERE (ID == %" PRIu64 ")",
			name.c_str(), DevRowIdx);
		procResult.DeviceName = name;
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_evohome3(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	const _tEVOHOME3* pEvo = reinterpret_cast<const _tEVOHOME3*>(pResponse);
	uint8_t devType = pTypeEvohomeRelay;
	uint8_t subType = pEvo->subtype;
	std::stringstream szID;
	int nDevID = (int)RFX_GETID3(pEvo->id1, pEvo->id2, pEvo->id3);
	szID << std::hex << nDevID;
	std::string ID(szID.str());
	uint8_t Unit = pEvo->devno;
	uint8_t cmnd = (pEvo->demand > 0) ? light1_sOn : light1_sOff;
	sprintf(szTmp, "%d", pEvo->demand);
	std::string szDemand(szTmp);
	uint8_t SignalLevel = 255;//Unknown
	uint8_t BatteryLevel = 255;//Unknown

	if (Unit == 0xFF && nDevID == 0)
		return;
	//Get Device details (devno or devid not available)
	bool bNewDev = false;
	std::vector<std::vector<std::string> > result;
	if (Unit == 0xFF)
		result = m_sql.safe_query(
			"SELECT HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == '%q')",
			pHardware->m_HwdID, ID.c_str());
	else
		result = m_sql.safe_query(
			"SELECT HardwareID,DeviceID,Unit,Type,SubType,nValue,sValue,BatteryLevel "
			"FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit == '%d') AND (Type==%d) AND (DeviceID == '%q')",
			pHardware->m_HwdID, (int)Unit, (int)pEvo->type, ID.c_str());
	if (!result.empty())
	{
		if (pEvo->demand == 0xFF)//we sometimes get a 0418 message after the initial device creation but it will mess up the logging as we don't have a demand
			return;
		uint8_t cur_cmnd = atoi(result[0][5].c_str());
		BatteryLevel = atoi(result[0][7].c_str());

		if (pEvo->updatetype == CEvohomeBase::updBattery)
		{
			BatteryLevel = pEvo->battery_level;
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
		if (pEvo->demand == 0xFF)//0418 allows us to associate unit and deviceid but no state information other messages only contain one or the other
			szDemand = "0";
		if (pEvo->updatetype == CEvohomeBase::updBattery)
			BatteryLevel = pEvo->battery_level;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szDemand.c_str(), procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (bNewDev && (Unit == 0xF9 || Unit == 0xFA || Unit == 0xFC || Unit <= 12))
	{
		if (Unit == 0xF9)
			procResult.DeviceName = "CH Valve";
		else if (Unit == 0xFA)
			procResult.DeviceName = "DHW Valve";
		else if (Unit == 0xFC)
		{
			if (pEvo->id1 >> 2 == CEvohomeID::devBridge) // Evohome OT Bridge
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

	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Security1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeSecurity1;
	uint8_t subType = pResponse->SECURITY1.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->SECURITY1.id1, pResponse->SECURITY1.id2, pResponse->SECURITY1.id3);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = pResponse->SECURITY1.status;
	uint8_t SignalLevel = pResponse->SECURITY1.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->SECURITY1.battery_level & 0x0F);
	if (
		(pResponse->SECURITY1.subtype == sTypeKD101) ||
		(pResponse->SECURITY1.subtype == sTypeSA30) ||
		(pResponse->SECURITY1.subtype == sTypeRM174RF) ||
		(pResponse->SECURITY1.subtype == sTypeDomoticzSecurity)
		)
	{
		//KD101 & SA30 do not support battery low indication
		BatteryLevel = 255;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Security2(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeSecurity2;
	uint8_t subType = pResponse->SECURITY2.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X%02X%02X%02X%02X%02X", pResponse->SECURITY2.id1, pResponse->SECURITY2.id2, pResponse->SECURITY2.id3, pResponse->SECURITY2.id4, pResponse->SECURITY2.id5, pResponse->SECURITY2.id6, pResponse->SECURITY2.id7, pResponse->SECURITY2.id8);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;// pResponse->SECURITY2.cmnd;
	uint8_t SignalLevel = pResponse->SECURITY2.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->SECURITY2.battery_level & 0x0F);

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
void MainWorker::decode_Camera1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	WriteMessage("");

	//uint8_t devType=pTypeCamera;

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

void MainWorker::decode_Remote(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeRemote;
	uint8_t subType = pResponse->REMOTE.subtype;
	sprintf(szTmp, "%d", pResponse->REMOTE.id);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->REMOTE.cmnd;
	uint8_t cmnd = light2_sOn;
	uint8_t SignalLevel = pResponse->REMOTE.rssi;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Thermostat(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tSetpoint* pMeter = reinterpret_cast<const _tSetpoint*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	uint8_t Unit = pMeter->dunit;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = pMeter->battery_level;

	switch (pMeter->subtype)
	{
	case sTypeSetpoint:
		sprintf(szTmp, "%.2f", pMeter->value);
		break;
	default:
		sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pMeter->type, pMeter->subtype);
		WriteMessage(szTmp);
		return;
	}

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, pMeter->value);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		double tvalue = ConvertTemperature(pMeter->value, m_sql.m_tempsign[0]);
		switch (pMeter->subtype)
		{
		case sTypeSetpoint:
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

void MainWorker::decode_Thermostat1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeThermostat1;
	uint8_t subType = pResponse->THERMOSTAT1.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->THERMOSTAT1.id1 * 256) + pResponse->THERMOSTAT1.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->THERMOSTAT1.rssi;
	uint8_t BatteryLevel = 255;

	uint8_t temp = pResponse->THERMOSTAT1.temperature;
	uint8_t set_point = pResponse->THERMOSTAT1.set_point;
	uint8_t mode = (pResponse->THERMOSTAT1.mode & 0x80);
	uint8_t status = (pResponse->THERMOSTAT1.status & 0x03);

	sprintf(szTmp, "%d;%d;%d;%d", temp, set_point, mode, status);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Thermostat2(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeThermostat2;
	uint8_t subType = pResponse->THERMOSTAT2.subtype;
	std::string ID;
	ID = "1";
	uint8_t Unit = pResponse->THERMOSTAT2.unitcode;
	uint8_t cmnd = pResponse->THERMOSTAT2.cmnd;
	uint8_t SignalLevel = pResponse->THERMOSTAT2.rssi;
	uint8_t BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Thermostat3(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeThermostat3;
	uint8_t subType = pResponse->THERMOSTAT3.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT3.unitcode1, pResponse->THERMOSTAT3.unitcode2, pResponse->THERMOSTAT3.unitcode3);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = pResponse->THERMOSTAT3.cmnd;
	uint8_t SignalLevel = pResponse->THERMOSTAT3.rssi;
	uint8_t BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, "", procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Thermostat4(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeThermostat4;
	uint8_t subType = pResponse->THERMOSTAT4.subtype;
	std::string ID;
	sprintf(szTmp, "%02X%02X%02X", pResponse->THERMOSTAT4.unitcode1, pResponse->THERMOSTAT4.unitcode2, pResponse->THERMOSTAT4.unitcode3);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t SignalLevel = pResponse->THERMOSTAT4.rssi;
	uint8_t BatteryLevel = 255;
	sprintf(szTmp, "%d;%d;%d;%d;%d;%d",
		pResponse->THERMOSTAT4.beep,
		pResponse->THERMOSTAT4.fan1_speed,
		pResponse->THERMOSTAT4.fan2_speed,
		pResponse->THERMOSTAT4.fan3_speed,
		pResponse->THERMOSTAT4.flame_power,
		pResponse->THERMOSTAT4.mode
	);

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Radiator1(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeRadiator1;
	uint8_t subType = pResponse->RADIATOR1.subtype;
	std::string ID;
	sprintf(szTmp, "%X%02X%02X%02X", pResponse->RADIATOR1.id1, pResponse->RADIATOR1.id2, pResponse->RADIATOR1.id3, pResponse->RADIATOR1.id4);
	ID = szTmp;
	uint8_t Unit = pResponse->RADIATOR1.unitcode;
	uint8_t cmnd = pResponse->RADIATOR1.cmnd;
	uint8_t SignalLevel = pResponse->RADIATOR1.rssi;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%d.%d", pResponse->RADIATOR1.temperature, pResponse->RADIATOR1.tempPoint5);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
void MainWorker::decode_Baro(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	//uint8_t devType=pTypeBARO;

	WriteMessageStart();
	WriteMessage("Not implemented");
	WriteMessageEnd();
	procResult.DeviceRowIdx = -1;
}

//not in dbase yet
void MainWorker::decode_DateTime(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	WriteMessage("");

	//uint8_t devType=pTypeDT;

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

void MainWorker::decode_Current(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeCURRENT;
	uint8_t subType = pResponse->CURRENT.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->CURRENT.id1 * 256) + pResponse->CURRENT.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->CURRENT.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->CURRENT.battery_level & 0x0F);

	float CurrentChannel1 = float((pResponse->CURRENT.ch1h * 256) + pResponse->CURRENT.ch1l) / 10.0F;
	float CurrentChannel2 = float((pResponse->CURRENT.ch2h * 256) + pResponse->CURRENT.ch2l) / 10.0F;
	float CurrentChannel3 = float((pResponse->CURRENT.ch3h * 256) + pResponse->CURRENT.ch3l) / 10.0F;
	sprintf(szTmp, "%.1f;%.1f;%.1f", CurrentChannel1, CurrentChannel2, CurrentChannel3);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		sprintf(szTmp, "Count         = %d", pResponse->CURRENT.count);
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

void MainWorker::decode_Energy(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint8_t subType = pResponse->ENERGY.subtype;
	uint8_t SignalLevel = pResponse->ENERGY.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->ENERGY.battery_level & 0x0F);

	long instant = (pResponse->ENERGY.instant1 * 0x1000000) + (pResponse->ENERGY.instant2 * 0x10000) + (pResponse->ENERGY.instant3 * 0x100) + pResponse->ENERGY.instant4;

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
			char szTmp[20];
			std::string ID;
			sprintf(szTmp, "%08X", (pResponse->ENERGY.id1 * 256) + pResponse->ENERGY.id2);
			ID = szTmp;

			//Retrieve last total from current record
			int nValue;
			subType = sTypeKwh; // sensor type changed during recording
			uint8_t devType = pTypeGeneral; // Device reported as General and not Energy
			uint8_t Unit = 1; // in decode_general() Unit is set to 1
			std::string sValue;
			struct tm LastUpdateTime;
			if (!m_sql.GetLastValue(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, nValue, sValue, LastUpdateTime))
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
	gdevice.rssi = SignalLevel;
	gdevice.battery_level = BatteryLevel;

	int voltage = 230;
	m_sql.GetPreferencesVar("ElectricVoltage", voltage);
	if (voltage != 230)
	{
		float mval = float(voltage) / 230.0F;
		gdevice.floatval1 *= mval;
		gdevice.floatval2 *= mval;
	}

	decode_General(pHardware, (const tRBUF*)&gdevice, procResult);
	procResult.bProcessBatteryValue = false;
}

void MainWorker::decode_Power(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypePOWER;
	uint8_t subType = pResponse->POWER.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->POWER.id1 * 256) + pResponse->POWER.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->POWER.rssi;
	uint8_t BatteryLevel = 255;

	float Voltage = (float)pResponse->POWER.voltage;
	double current = ((pResponse->POWER.currentH * 256) + pResponse->POWER.currentL) / 100.0;
	double instant = ((pResponse->POWER.powerH * 256) + pResponse->POWER.powerL) / 10.0;// Watt
	double usage = ((pResponse->POWER.energyH * 256) + pResponse->POWER.energyL) / 100.0; //kWh
	double powerfactor = pResponse->POWER.pf / 100.0;
	float frequency = (float)pResponse->POWER.freq; //Hz

	sprintf(szTmp, "%ld;%.2f", long(ground(instant)), usage * 1000.0);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	//Voltage
	sprintf(szTmp, "%.3f", Voltage);
	std::string tmpDevName;
	uint64_t DevRowIdxAlt = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 1, pTypeGeneral, sTypeVoltage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName, true, procResult.Username.c_str());
	if (DevRowIdxAlt == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, tmpDevName, 1, pTypeGeneral, sTypeVoltage, Voltage);

	//Powerfactor
	sprintf(szTmp, "%.2f", (float)powerfactor);
	DevRowIdxAlt = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 2, pTypeGeneral, sTypePercentage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName, true, procResult.Username.c_str());
	if (DevRowIdxAlt == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, tmpDevName, 2, pTypeGeneral, sTypePercentage, (float)powerfactor);

	//Frequency
	sprintf(szTmp, "%.2f", (float)frequency);
	DevRowIdxAlt = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 3, pTypeGeneral, sTypePercentage, SignalLevel, BatteryLevel, cmnd, szTmp, tmpDevName, true, procResult.Username.c_str());
	if (DevRowIdxAlt == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, tmpDevName, 3, pTypeGeneral, sTypePercentage, frequency);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

		sprintf(szTmp, "Voltage       = %g Volt", Voltage);
		WriteMessage(szTmp);
		sprintf(szTmp, "Current       = %.2f Ampere", current);
		WriteMessage(szTmp);

		sprintf(szTmp, "Instant usage = %.2f Watt", instant);
		WriteMessage(szTmp);
		sprintf(szTmp, "total usage   = %.2f kWh", usage);
		WriteMessage(szTmp);

		sprintf(szTmp, "Power factor  = %.2f", powerfactor);
		WriteMessage(szTmp);
		sprintf(szTmp, "Frequency     = %g Hz", frequency);
		WriteMessage(szTmp);

		sprintf(szTmp, "Signal level  = %d", pResponse->POWER.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Current_Energy(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeCURRENTENERGY;
	uint8_t subType = pResponse->CURRENT_ENERGY.subtype;
	std::string ID;
	sprintf(szTmp, "%d", (pResponse->CURRENT_ENERGY.id1 * 256) + pResponse->CURRENT_ENERGY.id2);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->CURRENT_ENERGY.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->CURRENT_ENERGY.battery_level & 0x0F);

	float CurrentChannel1 = float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0F;
	float CurrentChannel2 = float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0F;
	float CurrentChannel3 = float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0F;

	double usage = 0;

	if (pResponse->CURRENT_ENERGY.count != 0)
	{
		//no usage provided, get the last usage
		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query(
			"SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)",
			pHardware->m_HwdID, ID.c_str(), int(Unit), int(devType), int(subType));
		if (!result2.empty())
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
				pHardware->m_HwdID, ID.c_str(), int(Unit), int(devType), int(subType));
			if (!result2.empty())
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

		sprintf(szTmp, "%ld;%.2f", (long)ground((CurrentChannel1 + CurrentChannel2 + CurrentChannel3) * voltage), usage);
		m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, pTypeENERGY, sTypeELEC3, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	}
	sprintf(szTmp, "%.1f;%.1f;%.1f;%.3f", CurrentChannel1, CurrentChannel2, CurrentChannel3, usage);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		ampereChannel1 = float((pResponse->CURRENT_ENERGY.ch1h * 256) + pResponse->CURRENT_ENERGY.ch1l) / 10.0F;
		ampereChannel2 = float((pResponse->CURRENT_ENERGY.ch2h * 256) + pResponse->CURRENT_ENERGY.ch2l) / 10.0F;
		ampereChannel3 = float((pResponse->CURRENT_ENERGY.ch3h * 256) + pResponse->CURRENT_ENERGY.ch3l) / 10.0F;
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
void MainWorker::decode_Gas(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	WriteMessage("");

	//uint8_t devType=pTypeGAS;

	WriteMessage("Not implemented");

	procResult.DeviceRowIdx = -1;
}

//not in dbase yet
void MainWorker::decode_Water(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	WriteMessage("");

	//uint8_t devType=pTypeWATER;

	WriteMessage("Not implemented");

	procResult.DeviceRowIdx = -1;
}

void MainWorker::decode_Weight(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeWEIGHT;
	uint8_t subType = pResponse->WEIGHT.subtype;
	uint16_t weightID = (pResponse->WEIGHT.id1 * 256) + pResponse->WEIGHT.id2;
	std::string ID;
	sprintf(szTmp, "%d", weightID);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->WEIGHT.rssi;
	uint8_t BatteryLevel = 255;
	float weight = (float(pResponse->WEIGHT.weighthigh) * 25.6F) + (float(pResponse->WEIGHT.weightlow) / 10.0F);

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	weight += AddjValue;

	sprintf(szTmp, "%.1f", weight);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, weight);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
		sprintf(szTmp, "Weight        = %.1f kg", (float(pResponse->WEIGHT.weighthigh) * 25.6F) + (float(pResponse->WEIGHT.weightlow) / 10));
		WriteMessage(szTmp);
		sprintf(szTmp, "Signal level  = %d", pResponse->WEIGHT.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_RFXSensor(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeRFXSensor;
	uint8_t subType = pResponse->RFXSENSOR.subtype;
	std::string ID;
	sprintf(szTmp, "%d", pResponse->RFXSENSOR.id);
	ID = szTmp;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->RFXSENSOR.rssi;
	uint8_t BatteryLevel = 255;

	if ((pHardware->HwdType == HTYPE_EnOceanESP2) || (pHardware->HwdType == HTYPE_EnOceanESP3))
	{
		// WARNING
		// filler & rssi fields fields are used here to transmit ID_BYTE0 value from EnOcean device
		// Set BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
		BatteryLevel = 255;
		SignalLevel = 12;
		// Set Unit = ID_BYTE0
		Unit = (pResponse->RFXSENSOR.rssi << 4) | pResponse->RFXSENSOR.filler;
	}

	float temp = 0;
	int volt = 0;
	switch (pResponse->RFXSENSOR.subtype)
	{
	case sTypeRFXSensorTemp:
	{
		if ((pResponse->RFXSENSOR.msg1 & 0x80) == 0) //positive temperature?
			temp = float((pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0F;
		else
			temp = -(float(((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0F);
		float AddjValue = 0.0F;
		float AddjMulti = 1.0F;
		m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
		temp += AddjValue;
		sprintf(szTmp, "%.1f", temp);
	}
	break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
	{
		volt = (pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2;
		if (
			(pHardware->HwdType == HTYPE_RFXLAN) ||
			(pHardware->HwdType == HTYPE_RFXtrx315) ||
			(pHardware->HwdType == HTYPE_RFXtrx433) ||
			(pHardware->HwdType == HTYPE_RFXtrx868)
			)
		{
			volt *= 10;
		}
		sprintf(szTmp, "%d", volt);
	}
	break;
	}
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	switch (pResponse->RFXSENSOR.subtype)
	{
	case sTypeRFXSensorTemp:
		m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, temp);
		break;
	case sTypeRFXSensorAD:
	case sTypeRFXSensorVolt:
		m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, (float)volt);
		break;
	}

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
				sprintf(szTmp, "Temperature   = %.2f C", float((pResponse->RFXSENSOR.msg1 * 256) + pResponse->RFXSENSOR.msg2) / 100.0F);
				WriteMessage(szTmp);
			}
			else
			{
				sprintf(szTmp, "Temperature   = -%.2f C", float(((pResponse->RFXSENSOR.msg1 & 0x7F) * 256) + pResponse->RFXSENSOR.msg2) / 100.0F);
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

void MainWorker::decode_RFXMeter(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint64_t DevRowIdx = -1;
	char szTmp[100];
	uint8_t devType = pTypeRFXMeter;
	uint8_t subType = pResponse->RFXMETER.subtype;
	if (subType == sTypeRFXMeterCount)
	{
		std::string ID;
		sprintf(szTmp, "%d", (pResponse->RFXMETER.id1 * 256) + pResponse->RFXMETER.id2);
		ID = szTmp;
		uint8_t Unit = 0;
		uint8_t cmnd = 0;
		uint8_t SignalLevel = pResponse->RFXMETER.rssi;
		uint8_t BatteryLevel = 255;

		unsigned long counter = (pResponse->RFXMETER.count1 << 24) + (pResponse->RFXMETER.count2 << 16) + (pResponse->RFXMETER.count3 << 8) + pResponse->RFXMETER.count4;
		//float RFXPwr = float(counter) / 1000.0f;

		sprintf(szTmp, "%lu", counter);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
			sprintf(szTmp, "if RFXPwr     = %.3f kWh", float(counter) / 1000.0F);
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

			sprintf(szTmp, "RFXPwr        = %.3f kW", 1.0F / (float(16 * counter) / (3600000.0F / 62.5F)));
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

void MainWorker::decode_P1MeterPower(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tP1Power* p1Power = reinterpret_cast<const _tP1Power*>(pResponse);

	if (p1Power->len != sizeof(_tP1Power) - 1)
		return;

	uint8_t devType = p1Power->type;
	uint8_t subType = p1Power->subtype;
	std::string ID;
	sprintf(szTmp, "%d", p1Power->ID);
	ID = szTmp;

	uint8_t Unit = subType;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%u;%u;%u;%u;%u;%u",
		p1Power->powerusage1,
		p1Power->powerusage2,
		p1Power->powerdeliv1,
		p1Power->powerdeliv2,
		p1Power->usagecurrent,
		p1Power->delivcurrent
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (p1Power->subtype)
		{
		case sTypeP1Power:
			WriteMessage("subtype       = P1 Smart Meter Power");

			sprintf(szTmp, "powerusage1 = %.3f kWh", float(p1Power->powerusage1) / 1000.0F);
			WriteMessage(szTmp);
			sprintf(szTmp, "powerusage2 = %.3f kWh", float(p1Power->powerusage2) / 1000.0F);
			WriteMessage(szTmp);

			sprintf(szTmp, "powerdeliv1 = %.3f kWh", float(p1Power->powerdeliv1) / 1000.0F);
			WriteMessage(szTmp);
			sprintf(szTmp, "powerdeliv2 = %.3f kWh", float(p1Power->powerdeliv2) / 1000.0F);
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

void MainWorker::decode_P1MeterGas(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tP1Gas* p1Gas = reinterpret_cast<const _tP1Gas*>(pResponse);
	if (p1Gas->len != sizeof(_tP1Gas) - 1)
		return;

	uint8_t devType = p1Gas->type;
	uint8_t subType = p1Gas->subtype;
	std::string ID;
	sprintf(szTmp, "%d", p1Gas->ID);
	ID = szTmp;
	uint8_t Unit = subType;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%u", p1Gas->gasusage);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (p1Gas->subtype)
		{
		case sTypeP1Gas:
			WriteMessage("subtype       = P1 Smart Meter Gas");

			sprintf(szTmp, "gasusage = %.3f m3", float(p1Gas->gasusage) / 1000.0F);
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

void MainWorker::decode_YouLessMeter(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const CYouLess::YouLessMeter* pMeter = reinterpret_cast<const CYouLess::YouLessMeter*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	sprintf(szTmp, "%d", pMeter->ID1);
	std::string ID = szTmp;
	uint8_t Unit = subType;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%lu;%lu",
		pMeter->powerusage,
		pMeter->usagecurrent
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeYouLess:
			WriteMessage("subtype       = YouLess Meter");

			sprintf(szTmp, "powerusage = %.3f kWh", float(pMeter->powerusage) / 1000.0F);
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

void MainWorker::decode_Rego6XXTemp(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tRego6XXTemp* pRego = reinterpret_cast<const _tRego6XXTemp*>(pResponse);
	uint8_t devType = pRego->type;
	uint8_t subType = pRego->subtype;
	std::string ID = pRego->ID;
	uint8_t Unit = subType;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%.1f",
		pRego->temperature
	);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, pRego->temperature);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		WriteMessage("subtype       = Rego6XX Temp");

		sprintf(szTmp, "Temp = %.1f", pRego->temperature);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Rego6XXValue(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tRego6XXStatus* pRego = reinterpret_cast<const _tRego6XXStatus*>(pResponse);
	uint8_t devType = pRego->type;
	uint8_t subType = pRego->subtype;
	std::string ID = pRego->ID;
	uint8_t Unit = subType;
	int numValue = pRego->value;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%d",
		pRego->value
	);

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, numValue, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_AirQuality(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tAirQualityMeter* pMeter = reinterpret_cast<const _tAirQualityMeter*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	sprintf(szTmp, "%d", pMeter->id1);
	std::string ID = szTmp;
	uint8_t Unit = pMeter->id2;
	//uint8_t cmnd=0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = 255;

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, pMeter->airquality, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, (const int)pMeter->airquality);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (pMeter->subtype)
		{
		case sTypeVoc:
			WriteMessage("subtype       = Voc");

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

void MainWorker::decode_Usage(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tUsageMeter* pMeter = reinterpret_cast<const _tUsageMeter*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	uint8_t Unit = pMeter->dunit;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = pMeter->rssi;
	uint8_t BatteryLevel = 255;

	sprintf(szTmp, "%.1f", pMeter->fusage);

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, pMeter->fusage);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_Lux(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tLightMeter* pMeter = reinterpret_cast<const _tLightMeter*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	sprintf(szTmp, "%X%02X%02X%02X", pMeter->id1, pMeter->id2, pMeter->id3, pMeter->id4);
	std::string ID = szTmp;
	uint8_t Unit = pMeter->dunit;
	uint8_t cmnd = 0;
	uint8_t SignalLevel = 12;
	uint8_t BatteryLevel = pMeter->battery_level;

	sprintf(szTmp, "%.0f", pMeter->fLux);

	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, pMeter->fLux);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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

void MainWorker::decode_General(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tGeneralDevice* pMeter = reinterpret_cast<const _tGeneralDevice*>(pResponse);
	uint8_t devType = pMeter->type;
	uint8_t subType = pMeter->subtype;
	uint8_t SignalLevel = pMeter->rssi;
	uint8_t BatteryLevel = pMeter->battery_level;

	if (
		(subType == sTypeVoltage)
		|| (subType == sTypeCurrent)
		|| (subType == sTypePercentage)
		|| (subType == sTypeWaterflow)
		|| (subType == sTypePressure)
#ifdef WITH_OPENZWAVE
		|| (subType == sTypeZWaveThermostatMode)
		|| (subType == sTypeZWaveThermostatFanMode)
		|| (subType == sTypeZWaveThermostatOperatingState)
		|| (subType == sTypeZWaveAlarm)
#endif
		|| (subType == sTypeFan)
		|| (subType == sTypeTextStatus)
		|| (subType == sTypeSoundLevel)
		|| (subType == sTypeBaro)
		|| (subType == sTypeDistance)
		|| (subType == sTypeSoilMoisture)
		|| (subType == sTypeCustom)
		|| (subType == sTypeKwh)
		)
	{
		sprintf(szTmp, "%08X", (unsigned int)pMeter->intval1);
	}
	else
	{
		sprintf(szTmp, "%d", pMeter->id);
	}

	std::string ID = szTmp;
	uint8_t Unit = 1;
	uint8_t cmnd = 0;
	strcpy(szTmp, "");

	uint64_t DevRowIdx = -1;

	if (subType == sTypeVisibility)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeDistance)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeSolarRadiation)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeSoilMoisture)
	{
		cmnd = pMeter->intval2;
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeLeafWetness)
	{
		cmnd = pMeter->intval1;
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeVoltage)
	{
		sprintf(szTmp, "%.3f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeCurrent)
	{
		sprintf(szTmp, "%.3f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeBaro)
	{
		sprintf(szTmp, "%.02f;%d", pMeter->floatval1, pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypePressure)
	{
		sprintf(szTmp, "%.1f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypePercentage)
	{
		sprintf(szTmp, "%.2f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeWaterflow)
	{
		sprintf(szTmp, "%.2f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeFan)
	{
		sprintf(szTmp, "%d", pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeSoundLevel)
	{
		sprintf(szTmp, "%d", pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
#ifdef WITH_OPENZWAVE
	else if ((subType == sTypeZWaveThermostatMode) || (subType == sTypeZWaveThermostatFanMode) || (subType == sTypeZWaveThermostatOperatingState))
	{
		cmnd = (uint8_t)pMeter->intval2;
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	}
	else if (subType == sTypeZWaveAlarm)
	{
		Unit = pMeter->id;
		cmnd = pMeter->intval2;
		strcpy(szTmp, pMeter->text);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
#endif
	else if (subType == sTypeKwh)
	{
		sprintf(szTmp, "%.3f;%.3f", pMeter->floatval1, pMeter->floatval2);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	}
	else if (subType == sTypeAlert)
	{
		if (strcmp(pMeter->text, ""))
			sprintf(szTmp, "(%d) %s", pMeter->intval1, pMeter->text);
		else
			sprintf(szTmp, "%d", pMeter->intval1);
		cmnd = pMeter->intval1;
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeCustom)
	{
		sprintf(szTmp, "%.4f", pMeter->floatval1);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
		if (DevRowIdx == (uint64_t)-1)
			return;
	}
	else if (subType == sTypeTextStatus)
	{
		strcpy(szTmp, pMeter->text);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	}
	else if (subType == sTypeCounterIncremental)
	{
		sprintf(szTmp, "%d", pMeter->intval2);
		DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	}

	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, cmnd, szTmp);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
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
			sprintf(szTmp, "Counter = %.3f", pMeter->floatval2 / 1000.0F);
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

void MainWorker::decode_GeneralSwitch(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[200];
	const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pResponse);
	uint8_t devType = pSwitch->type;
	uint8_t subType = pSwitch->subtype;

	sprintf(szTmp, "%08X", pSwitch->id);
	std::string ID = szTmp;
	uint8_t Unit = pSwitch->unitcode;
	uint8_t cmnd = pSwitch->cmnd;
	uint8_t level = pSwitch->level;
	uint8_t SignalLevel = pSwitch->rssi;

	sprintf(szTmp, "%d", level);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	uint8_t check_cmnd = cmnd;
	if ((cmnd == gswitch_sGroupOff) || (cmnd == gswitch_sGroupOn))
		check_cmnd = (cmnd == gswitch_sGroupOff) ? gswitch_sOff : gswitch_sOn;
	CheckSceneCode(DevRowIdx, devType, subType, check_cmnd, szTmp, procResult.DeviceName);

	if ((cmnd == gswitch_sGroupOff) || (cmnd == gswitch_sGroupOn))
	{
		//set the status of all lights with the same code to on/off
		m_sql.GeneralSwitchGroupCmd(ID, subType, (cmnd == gswitch_sGroupOff) ? gswitch_sOff : gswitch_sOn);
	}

	procResult.DeviceRowIdx = DevRowIdx;
}

//BBQ sensor has two temperature sensors, add them as two temperature devices
void MainWorker::decode_BBQ(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeBBQ;
	uint8_t subType = pResponse->BBQ.subtype;

	sprintf(szTmp, "%d", 1);//(pResponse->BBQ.id1 * 256) + pResponse->BBQ.id2); //this because every time you turn the device on, you get a new ID
	std::string ID = szTmp;

	//The transmitter and receiver are negotiating a new ID every 15/20 minutes,
	//for this we need to work with fixed ID's
	uint8_t Unit = 1;// pResponse->BBQ.id2;

	uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->BBQ.rssi;
	uint8_t BatteryLevel = 0;
	if ((pResponse->BBQ.battery_level & 0x0F) == 0)
		BatteryLevel = 0;
	else
		BatteryLevel = 100;

	uint64_t DevRowIdx = 0;

	float temp1, temp2;
	temp1 = float((pResponse->BBQ.sensor1h * 256) + pResponse->BBQ.sensor1l);// / 10.0f;
	temp2 = float((pResponse->BBQ.sensor2h * 256) + pResponse->BBQ.sensor2l);// / 10.0f;

	sprintf(szTmp, "%.0f;%.0f", temp1, temp2);
	DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (pResponse->BBQ.subtype)
		{
		case sTypeBBQ1:
			WriteMessage("subtype       = Maverick ET-732/733");
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
	int at10 = ground(std::abs(temp1 * 10.0F));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	_tRxMessageProcessingResult tmpProcResult1;
	tmpProcResult1.DeviceName = "";
	tmpProcResult1.DeviceRowIdx = -1;
	decode_Temp(pHardware, (const tRBUF*)&tsen.TEMP, tmpProcResult1);

	tsen.TEMP.id1 = 0;
	tsen.TEMP.id2 = 2;

	tsen.TEMP.tempsign = (temp2 >= 0) ? 0 : 1;
	at10 = ground(std::abs(temp2 * 10.0F));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	_tRxMessageProcessingResult tmpProcResult2;
	tmpProcResult2.DeviceName = "";
	tmpProcResult2.DeviceRowIdx = -1;
	decode_Temp(pHardware, (const tRBUF*)&tsen.TEMP, tmpProcResult2);

	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Cartelectronic(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	std::string ID;

	sprintf(szTmp, "%" PRIu64, ((uint64_t)(pResponse->TIC.id1) << 32) + (pResponse->TIC.id2 << 24) + (pResponse->TIC.id3 << 16) + (pResponse->TIC.id4 << 8) + (pResponse->TIC.id5));
	ID = szTmp;
	//uint8_t Unit = 0;
	//uint8_t cmnd = 0;

	uint8_t subType = pResponse->TIC.subtype;

	switch (subType)
	{
	case sTypeTIC:
		decode_CartelectronicTIC(pHardware, pResponse, procResult);
		WriteMessage("Cartelectronic TIC received");
		break;
	case sTypeCEencoder:
		decode_CartelectronicEncoder(pHardware, pResponse, procResult);
		WriteMessage("Cartelectronic Encoder received");
		break;
	default:
		WriteMessage("Cartelectronic protocol not supported");
		break;
	}
}

void MainWorker::decode_ASyncPort(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();

		char szTmp[100];
		//uint8_t subType = pResponse->ASYNCPORT.subtype;

		switch (pResponse->ASYNCPORT.subtype)
		{
		case sTypeASYNCconfig:
			WriteMessage("subtype       = Async port configure");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
			WriteMessage(szTmp);
			break;
		}

		sprintf(szTmp, "Sequence nbr  = %d", pResponse->ASYNCPORT.seqnbr);
		WriteMessage(szTmp);

		WriteMessage("Command     = ", false);
		switch (pResponse->ASYNCPORT.cmnd)
		{
		case asyncdisable:
			WriteMessage("Disable");
			break;
		case asyncreceiveP1:
			WriteMessage("Enable P1 Receive");
			break;
		case asyncreceiveTeleinfo:
			WriteMessage("Enable Teleinfo Recieve");
			break;
		case asyncreceiveRAW:
			WriteMessage("Enable Raw Receive");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown command type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
			WriteMessage(szTmp);
			break;
		}

		if (pResponse->ASYNCPORT.cmnd != asyncdisable)
		{
			WriteMessage("Baudrate    = ", false);
			switch (pResponse->ASYNCPORT.baudrate)
			{
			case asyncbaud110:
				WriteMessage("110");
				break;
			case asyncbaud300:
				WriteMessage("300");
				break;
			case asyncbaud600:
				WriteMessage("600");
				break;
			case asyncbaud1200:
				WriteMessage("1200");
				break;
			case asyncbaud2400:
				WriteMessage("2400");
				break;
			case asyncbaud4800:
				WriteMessage("4800");
				break;
			case asyncbaud9600:
				WriteMessage("9600");
				break;
			case asyncbaud14400:
				WriteMessage("14400");
				break;
			case asyncbaud19200:
				WriteMessage("19200");
				break;
			case asyncbaud38400:
				WriteMessage("38400");
				break;
			case asyncbaud57600:
				WriteMessage("57600");
				break;
			case asyncbaud115200:
				WriteMessage("115200");
				break;
			default:
				sprintf(szTmp, "ERROR: Unknown baudrate type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
				WriteMessage(szTmp);
				break;
			}
			WriteMessage("Parity      = ", false);
			switch (pResponse->ASYNCPORT.parity)
			{
			case asyncParityNo:
				WriteMessage("None");
				break;
			case asyncParityOdd:
				WriteMessage("Odd");
				break;
			case asyncParityEven:
				WriteMessage("Even");
				break;
			default:
				sprintf(szTmp, "ERROR: Unknown partity type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
				WriteMessage(szTmp);
				break;
			}
			WriteMessage("Databits    = ", false);
			switch (pResponse->ASYNCPORT.databits)
			{
			case asyncDatabits7:
				WriteMessage("7");
				break;
			case asyncDatabits8:
				WriteMessage("8");
				break;
			default:
				sprintf(szTmp, "ERROR: Unknown databits type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
				WriteMessage(szTmp);
				break;
			}
			WriteMessage("Stopbits    = ", false);
			switch (pResponse->ASYNCPORT.stopbits)
			{
			case asyncStopbits1:
				WriteMessage("1");
				break;
			case asyncStopbits2:
				WriteMessage("2");
				break;
			default:
				sprintf(szTmp, "ERROR: Unknown stopbits type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
				WriteMessage(szTmp);
				break;
			}
			WriteMessage("Polarity    = ", false);
			switch (pResponse->ASYNCPORT.polarity)
			{
			case asyncPolarityNormal:
				WriteMessage("Normal");
				break;
			case asyncPolarityInvers:
				WriteMessage("Inverted");
				break;
			default:
				sprintf(szTmp, "ERROR: Unknown stopbits type for Packet type= %02X:%02X", pResponse->ASYNCPORT.packettype, pResponse->ASYNCPORT.subtype);
				WriteMessage(szTmp);
				break;
			}
		}
		WriteMessageEnd();
	}
}

void MainWorker::decode_ASyncData(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	if (
		(pHardware->m_HwdID == 999) ||
		(pHardware->HwdType == HTYPE_RFXtrx315) ||
		(pHardware->HwdType == HTYPE_RFXtrx433) ||
		(pHardware->HwdType == HTYPE_RFXtrx868) ||
		(pHardware->HwdType == HTYPE_RFXLAN)
		)
	{
		CRFXBase* pRFXBase = (CRFXBase*)pHardware;
		const uint8_t* pData = reinterpret_cast<const uint8_t*>(&pResponse->ASYNCDATA.datachar[0]);
		int Len = pResponse->ASYNCDATA.packetlength - 3;
		pRFXBase->Parse_Async_Data(pData, Len);
	}
}

void MainWorker::decode_CartelectronicTIC(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
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
	uint8_t unitCounter1 = 0;
	uint8_t unitCounter2 = 1;
	uint8_t cmnd = 0;
	std::string ID;

	uint8_t devType = pTypeGeneral;
	uint8_t subType = sTypeKwh;
	uint8_t SignalLevel = pResponse->TIC.rssi;

	uint8_t BatteryLevel = (pResponse->TIC.battery_level + 1) * 10;

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
			uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 1, pTypeUsage, sTypeElectric, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

			if (DevRowIdx == (uint64_t)-1)
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

			uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

			if (DevRowIdx == (uint64_t)-1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (float)counter2);
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
			uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

			if (DevRowIdx == (uint64_t)-1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (float)counter2);
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
			uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), unitCounter2, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

			if (DevRowIdx == (uint64_t)-1)
				return;

			m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (float)counter2);
			//----------------------------
		}
		break;
		default:
			break;
		}

		// Counter 1
		counter1 = (pResponse->TIC.counter1_0 << 24) + (pResponse->TIC.counter1_1 << 16) + (pResponse->TIC.counter1_2 << 8) + (pResponse->TIC.counter1_3);

		sprintf(szTmp, "%u;%d", counter1ApparentPower, counter1);
		uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), unitCounter1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());

		if (DevRowIdx == (uint64_t)-1)
			return;

		m_notifications.CheckAndHandleNotification(DevRowIdx, procResult.DeviceName, devType, subType, NTYPE_TODAYENERGY, (float)counter1);
		//----------------------------
	}
	else
	{
		WriteMessage("TeleInfo not connected");
	}
}

void MainWorker::decode_CartelectronicEncoder(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint32_t counter1 = 0;
	uint32_t counter2 = 0;
	//int apparentPower = 0;
	uint8_t Unit = 0;
	uint8_t cmnd = 0;
	uint64_t DevRowIdx = 0;
	std::string ID;

	uint8_t devType = pTypeRFXMeter;
	uint8_t subType = sTypeRFXMeterCount;
	uint8_t SignalLevel = pResponse->CEENCODER.rssi;

	uint8_t BatteryLevel = (pResponse->CEENCODER.battery_level + 1) * 10;

	// Id of the module
	sprintf(szTmp, "%d", ((uint32_t)(pResponse->CEENCODER.id1 << 24) + (pResponse->CEENCODER.id2 << 16) + (pResponse->CEENCODER.id3 << 8) + pResponse->CEENCODER.id4));
	ID = szTmp;

	// Counter 1
	counter1 = (pResponse->CEENCODER.counter1_0 << 24) + (pResponse->CEENCODER.counter1_1 << 16) + (pResponse->CEENCODER.counter1_2 << 8) + (pResponse->CEENCODER.counter1_3);

	sprintf(szTmp, "%d", counter1);
	DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, Unit, devType, subType, (float)counter1);

	// Counter 2
	counter2 = (pResponse->CEENCODER.counter2_0 << 24) + (pResponse->CEENCODER.counter2_1 << 16) + (pResponse->CEENCODER.counter2_2 << 8) + (pResponse->CEENCODER.counter2_3);

	sprintf(szTmp, "%d", counter2);
	DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), 1, devType, subType, SignalLevel, BatteryLevel, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	m_notifications.CheckAndHandleNotification(DevRowIdx, pHardware->m_HwdID, ID, procResult.DeviceName, 1, devType, subType, (float)counter2);
}

void MainWorker::decode_Weather(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint16_t windID = (pResponse->WEATHER.id1 * 256) + pResponse->WEATHER.id2;
	char szTmp[100];
	sprintf(szTmp, "%d", windID);
	std::string ID = szTmp;

	//uint8_t devType = pTypeWEATHER;
	uint8_t subType = pResponse->WEATHER.subtype;
	//uint8_t Unit = 0;
	//uint8_t cmnd = 0;
	uint8_t SignalLevel = pResponse->WEATHER.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->WEATHER.battery_level & 0x0F);

	procResult.DeviceRowIdx = -1;

	CRFXBase* pRFXDevice = (CRFXBase*)pHardware;

	//Wind
	int intDirection = (pResponse->WEATHER.directionhigh * 256) + pResponse->WEATHER.directionlow;
	float intSpeed = float((pResponse->WEATHER.av_speedhigh * 256) + pResponse->WEATHER.av_speedlow) / 10.0F;
	float intGust = float((pResponse->WEATHER.gusthigh * 256) + pResponse->WEATHER.gustlow) / 10.0F;

	float temp = 0, chill = 0;
	if (!pResponse->WEATHER.temperaturesign)
	{
		temp = float((pResponse->WEATHER.temperaturehigh * 256) + pResponse->WEATHER.temperaturelow) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->WEATHER.temperaturehigh & 0x7F) * 256) + pResponse->WEATHER.temperaturelow) / 10.0F);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage(" Invalid Temperature");
		return;
	}

	bool bHaveChill = false;
	if (1 == 0)// subType == sTypeWEATHERx)
	{
		if (!pResponse->WEATHER.chillsign)
		{
			chill = float((pResponse->WEATHER.chillhigh * 256) + pResponse->WEATHER.chilllow) / 10.0F;
		}
		else
		{
			chill = -(float(((pResponse->WEATHER.chillhigh) & 0x7F) * 256 + pResponse->WEATHER.chilllow) / 10.0F);
		}
		bHaveChill = true;
	}
	pRFXDevice->SendWind(windID, BatteryLevel, intDirection, intSpeed, intGust, temp, chill, true, bHaveChill, procResult.DeviceName, SignalLevel);

	int Humidity = (int)pResponse->WEATHER.humidity;
	pRFXDevice->SendTempHumSensor(windID, BatteryLevel, temp, Humidity, procResult.DeviceName, SignalLevel);

	//Rain
	float TotalRain = 0;
	switch (subType)
	{
	case sTypeWEATHER0:
		TotalRain = float((pResponse->WEATHER.raintotal2 * 256) + (pResponse->WEATHER.raintotal3)) * 0.1F;
		break;
	case sTypeWEATHER1:
		TotalRain = float((pResponse->WEATHER.raintotal2 * 256) + (pResponse->WEATHER.raintotal3)) * 0.3F;
		break;
	case sTypeWEATHER2:
		TotalRain = float((pResponse->WEATHER.raintotal2 * 256) + (pResponse->WEATHER.raintotal3)) * 0.254F;
		break;
	}
	pRFXDevice->SendRainSensor(windID, BatteryLevel, TotalRain, procResult.DeviceName, SignalLevel);

	//UV
	float UV = (float)pResponse->WEATHER.uv;
	switch (subType)
	{
	case sTypeWEATHER0:
	case sTypeWEATHER2:
		UV = UV / 10.0F;
		break;
	}
	pRFXDevice->SendUVSensor(windID, 1, BatteryLevel, UV, procResult.DeviceName, SignalLevel);

	//Solar
	float radiation = -1;
	if (subType == sTypeWEATHER0)
		radiation = (float)((pResponse->WEATHER.solarhigh * 256) + pResponse->WEATHER.solarlow) * 0.078925F;
	else if (subType == sTypeWEATHER2)
		radiation = (float)((pResponse->WEATHER.solarhigh * 256) + pResponse->WEATHER.solarlow);
	if (radiation != -1)
		pRFXDevice->SendSolarRadiationSensor((const uint8_t)windID, BatteryLevel, radiation, procResult.DeviceName);
}

void MainWorker::decode_Solar(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint8_t SignalLevel = pResponse->SOLAR.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->SOLAR.battery_level & 0x0F);

	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeSolarRadiation;
	gdevice.intval1 = (pResponse->SOLAR.id1 * 256) + pResponse->SOLAR.id2;
	gdevice.id = (uint8_t)gdevice.intval1;
	gdevice.floatval1 = float((pResponse->SOLAR.solarhigh * 256) + float(pResponse->SOLAR.solarlow)) / 100.F;

	if (gdevice.floatval1 > 1361)
		return; // https://en.wikipedia.org/wiki/Solar_irradiance

	gdevice.rssi = SignalLevel;
	gdevice.battery_level = BatteryLevel;
	decode_General(pHardware, (const tRBUF*)&gdevice, procResult);
	procResult.bProcessBatteryValue = false;
}

void MainWorker::decode_Hunter(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint8_t devType = pResponse->HUNTER.packettype;
	uint8_t subType = pResponse->HUNTER.subtype;
	uint8_t SignalLevel = pResponse->HUNTER.rssi;
	uint8_t BatteryLevel = 255;

	uint8_t Unit = 0;
	uint8_t cmnd = pResponse->HUNTER.cmnd;
	uint64_t DevRowIdx = 0;
	std::string ID;

	char szTmp[50];
	sprintf(szTmp, "%02X%02X%02X%02X%02X%02X", pResponse->HUNTER.id1, pResponse->HUNTER.id2, pResponse->HUNTER.id3, pResponse->HUNTER.id4, pResponse->HUNTER.id5, pResponse->HUNTER.id6);
	ID = szTmp;

	DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, BatteryLevel, cmnd, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		switch (pResponse->HUNTER.subtype)
		{
		case sTypeHunterfan:
			WriteMessage("subtype       = Hunter Fan");
			sprintf(szTmp, "Sequence nbr  = %d", pResponse->HUNTER.seqnbr);
			WriteMessage(szTmp);
			sprintf(szTmp, "ID            = %s", ID.c_str());
			WriteMessage(szTmp);
			WriteMessage("Command       = ", false);
			switch (pResponse->HUNTER.cmnd)
			{
			case HunterOff:
				WriteMessage("Off");
				break;
			case HunterLight:
				WriteMessage("Light");
				break;
			case HunterSpeed1:
				WriteMessage("Low");
				break;
			case HunterSpeed2:
				WriteMessage("Medium");
				break;
			case HunterSpeed3:
				WriteMessage("High");
				break;
			case HunterProgram:
				WriteMessage("Learn");
				break;
			default:
				WriteMessage("UNKNOWN");
				break;
			}
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

void MainWorker::decode_LevelSensor(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[50];

	uint8_t devType = pTypeLEVELSENSOR;
	uint8_t subType = pResponse->LEVELSENSOR.subtype;

	sprintf(szTmp, "%d", (pResponse->LEVELSENSOR.id1 * 256) + pResponse->LEVELSENSOR.id2);
	std::string ID = szTmp;
	uint8_t Unit = pResponse->TEMP.id2;


	uint8_t SignalLevel = pResponse->LEVELSENSOR.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->LEVELSENSOR.battery_level & 0x0F);

	float temp = 0;
	if (!pResponse->LEVELSENSOR.temperaturesign)
	{
		temp = float((pResponse->LEVELSENSOR.temperaturehigh * 256) + pResponse->LEVELSENSOR.temperaturelow) / 10.0F;
	}
	else
	{
		temp = -(float(((pResponse->LEVELSENSOR.temperaturehigh & 0x7F) * 256) + pResponse->LEVELSENSOR.temperaturelow) / 10.0F);
	}
	if ((temp < -200) || (temp > 380))
	{
		WriteMessage("ERROR: Invalid Temperature");
		return;
	}

	float AddjValue = 0.0F;
	float AddjMulti = 1.0F;
	m_sql.GetAddjustment(pHardware->m_HwdID, ID.c_str(), Unit, devType, subType, AddjValue, AddjMulti);
	temp += AddjValue;

	sprintf(szTmp, "%.1f", temp);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, pTypeTEMP, sTypeTEMP1, SignalLevel, BatteryLevel, 0, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;

	//Depth
	_tGeneralDevice gdevice;
	gdevice.subtype = sTypeDistance;
	gdevice.intval1 = (pResponse->LEVELSENSOR.id1 * 256) + pResponse->LEVELSENSOR.id2;
	gdevice.id = (uint8_t)gdevice.intval1;
	gdevice.floatval1 = float((pResponse->LEVELSENSOR.depth1 * 256) + float(pResponse->LEVELSENSOR.depth2));
	gdevice.rssi = SignalLevel;
	gdevice.battery_level = BatteryLevel;
	decode_General(pHardware, (const tRBUF*)&gdevice, procResult);
	procResult.bProcessBatteryValue = false;
}

void MainWorker::decode_LightningSensor(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	uint64_t DevRowIdx = 0;

	uint8_t devType = pTypeLIGHTNING;
	uint8_t subType = pResponse->LIGHTNING.subtype;

	uint16_t NodeID = (pResponse->LIGHTNING.id2 << 8) | pResponse->LIGHTNING.id3;
	uint8_t Unit = pResponse->LIGHTNING.id1;


	uint8_t SignalLevel = pResponse->LIGHTNING.rssi;
	uint8_t BatteryLevel = get_BateryLevel(pHardware->HwdType, false, pResponse->LIGHTNING.battery_level & 0x0F);

	int distance = pResponse->LIGHTNING.distance;

	_tGeneralDevice gdevice;
	gdevice.rssi = SignalLevel;
	gdevice.battery_level = BatteryLevel;
	gdevice.intval1 = NodeID;
	gdevice.id = (uint8_t)gdevice.intval1;

	bool bNewSensor = false;

	//Strikes (resetted daily, or turnover... we need to keep track of this)
	const int strike_count = pResponse->LIGHTNING.strike_cnt;
	gdevice.subtype = sTypeCounterIncremental;

	int new_count = strike_count;

	//Get previous send value
	std::vector<std::vector<std::string> > result;
	std::string lookupName = "Prev.Lightning_Strikes_" + std::to_string(gdevice.id);
	result = m_sql.safe_query("SELECT Timeout, ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q')", pHardware->m_HwdID, lookupName.c_str());
	if (!result.empty())
	{
		int old_count = atoi(result[0][0].c_str());
		if (old_count <= new_count)
		{
			new_count = strike_count - old_count;
		}
		m_sql.safe_query("UPDATE WOLNodes SET Timeout=%d WHERE (ID == %q)", strike_count, result[0][1].c_str());
	}
	else
	{
		bNewSensor = true;
		m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, Timeout) VALUES (%d, '%q', %d)", pHardware->m_HwdID, lookupName.c_str(), strike_count);
	}
	if ((new_count > 0) || (bNewSensor))
	{
		gdevice.intval2 = new_count;
		//gdevice.subtype = sTypeTextStatus;
		//strcpy_s(gdevice.text, std::to_string(strike_count).c_str());
		procResult.DeviceName = "Lightning Strike Count";
		decode_General(pHardware, (const tRBUF*)&gdevice, procResult);
	}

	//Distance (meters)
	if (distance == 63)
	{
		if (!bNewSensor)
			return; //invalid/no strike
		distance = 0;
	}
	gdevice.subtype = sTypeVisibility;
	gdevice.floatval1 = static_cast<float>(distance);
	procResult.DeviceName = "Lightning Distance";
	decode_General(pHardware, (const tRBUF*)&gdevice, procResult);
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_DDxxxx(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pTypeDDxxxx;
	uint8_t subType = pResponse->DDXXXX.subtype;

	sprintf(szTmp, "%02X%02X%02X%02X", pResponse->DDXXXX.id1, pResponse->DDXXXX.id2, pResponse->DDXXXX.id3, pResponse->DDXXXX.id4);

	std::string ID = szTmp;
	uint8_t Unit = pResponse->DDXXXX.unitcode;
	uint8_t cmnd = pResponse->DDXXXX.cmnd;
	uint8_t SignalLevel = pResponse->DDXXXX.rssi;

	sprintf(szTmp, "%d", pResponse->DDXXXX.percent);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		char szTmp[100];

		switch (pResponse->DDXXXX.subtype)
		{
		case sTypeDDxxxx:
			WriteMessage("subtype       = Brel");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->DDXXXX.packettype, pResponse->DDXXXX.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->DDXXXX.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "id1-4         = %02X%02X%02X%02X", pResponse->DDXXXX.id1, pResponse->DDXXXX.id2, pResponse->DDXXXX.id3, pResponse->DDXXXX.id4);
		WriteMessage(szTmp);

		if (pResponse->DDXXXX.unitcode == 0)
			WriteMessage("Unit          = All");
		else
			sprintf(szTmp, "Unit          = %d", pResponse->DDXXXX.unitcode + 1);
		WriteMessage(szTmp);

		WriteMessage("Command       = ", false);

		switch (pResponse->DDXXXX.cmnd)
		{
		case DDxxxx_Up:
			WriteMessage("Open/Up");
			break;
		case DDxxxx_Down:
			WriteMessage("Close/Down");
			break;
		case DDxxxx_Stop:
			WriteMessage("Stop");
			break;
		case DDxxxx_P2:
			WriteMessage("Pair");
			break;
		case DDxxxx_Percent:
			sprintf(szTmp, "Percent = %d", pResponse->DDXXXX.percent);
			WriteMessage(szTmp);
			break;
		case DDxxxx_Angle:
			WriteMessage("Angle");
			break;
		case DDxxxx_PercentAngle:
			WriteMessage("Percent Angle");
			break;
		case DDxxxx_HoldUp:
			WriteMessage("Hold Up");
			break;
		case DDxxxx_HoldUpDown:
			WriteMessage("Hold Down");
			break;
		case DDxxxx_HoldStop:
			WriteMessage("Hold Stop");
			break;
		case DDxxxx_HoldStopUp:
			WriteMessage("Hold Stop Up");
			break;
		case DDxxxx_HoldStopDown:
			WriteMessage("Hold Stop Down");
			break;
		default:
			WriteMessage("UNKNOWN");
			break;
		}
		sprintf(szTmp, "Signal level  = %d", pResponse->DDXXXX.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

void MainWorker::decode_Honeywell(const CDomoticzHardwareBase* pHardware, const tRBUF* pResponse, _tRxMessageProcessingResult& procResult)
{
	char szTmp[100];
	uint8_t devType = pResponse->HONEYWELL_AL.packettype;
	uint8_t subType = pResponse->HONEYWELL_AL.subtype;

	sprintf(szTmp, "%02X%02X%02X", pResponse->HONEYWELL_AL.id1, pResponse->HONEYWELL_AL.id2, pResponse->HONEYWELL_AL.id3);

	std::string ID = szTmp;
	uint8_t Unit = (pResponse->HONEYWELL_AL.knock << 4) | pResponse->HONEYWELL_AL.alert;
	uint8_t cmnd = 1;
	uint8_t SignalLevel = pResponse->HONEYWELL_AL.rssi;

	sprintf(szTmp, "%d", 0);
	uint64_t DevRowIdx = m_sql.UpdateValue(pHardware->m_HwdID, 0, ID.c_str(), Unit, devType, subType, SignalLevel, -1, cmnd, szTmp, procResult.DeviceName, true, procResult.Username.c_str());
	if (DevRowIdx == (uint64_t)-1)
		return;
	CheckSceneCode(DevRowIdx, devType, subType, cmnd, szTmp, procResult.DeviceName);

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		WriteMessageStart();
		char szTmp[100];

		switch (pResponse->HONEYWELL_AL.subtype)
		{
		case sTypeSeries5:
			WriteMessage("subtype       = Series5");
			break;
		case sTypePIR:
			WriteMessage("subtype       = PIR");
			break;
		default:
			sprintf(szTmp, "ERROR: Unknown Sub type for Packet type= %02X:%02X:", pResponse->HONEYWELL_AL.packettype, pResponse->HONEYWELL_AL.subtype);
			WriteMessage(szTmp);
			break;
		}
		sprintf(szTmp, "Sequence nbr  = %d", pResponse->HONEYWELL_AL.seqnbr);
		WriteMessage(szTmp);

		sprintf(szTmp, "id1-3         = %02X%02X%02X", pResponse->HONEYWELL_AL.id1, pResponse->HONEYWELL_AL.id2, pResponse->HONEYWELL_AL.id3);
		WriteMessage(szTmp);

		switch (pResponse->HONEYWELL_AL.alert)
		{
		case 0:
			WriteMessage("Alert       = Normal");
			break;
		case 1:
			WriteMessage("Alert       = Right halo light pattern");
			break;
		case 2:
			WriteMessage("Alert       = Left halo light pattern");
			break;
		case 3:
			WriteMessage("Alert       = High volume alarm");
			break;
		}

		switch (pResponse->HONEYWELL_AL.knock)
		{
		case 0:
			WriteMessage("Knock       = Yes");
			break;
		case 1:
			WriteMessage("Knock       = No");
			break;
		}

		sprintf(szTmp, "Signal level  = %d", pResponse->HONEYWELL_AL.rssi);
		WriteMessage(szTmp);
		WriteMessageEnd();
	}
	procResult.DeviceRowIdx = DevRowIdx;
}

bool MainWorker::GetSensorData(const uint64_t idx, int& nValue, std::string& sValue)
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
		float GasDivider = 1000.0F;
		//get lowest value of today
		time_t now = mytime(nullptr);
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
		if (!result2.empty())
		{
			std::vector<std::string> sd2 = result2[0];
			uint64_t total_min_gas = std::stoull(sd2[0].c_str());
			uint64_t gasactual = std::stoull(sValue);
			uint64_t total_real_gas = gasactual - total_min_gas;
			float musage = float(total_real_gas) / GasDivider;
			sprintf(szTmp, "%.03f", musage);
		}
		else
		{
			sprintf(szTmp, "%.03f", 0.0F);
		}
		nValue = 0;
		sValue = szTmp;
	}
	else if (devType == pTypeRFXMeter)
	{
		float EnergyDivider = 1000.0F;
		float GasDivider = 100.0F;
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
		time_t now = mytime(nullptr);
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
		if (!result2.empty())
		{
			std::vector<std::string> sd2 = result2[0];

			uint64_t total_min = std::stoull(sd2[0]);
			uint64_t total_max = std::stoull(sd2[1]);
			uint64_t total_real = total_max - total_min;
			sprintf(szTmp, "%" PRIu64, total_real);

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
				sprintf(szTmp, "%" PRIu64, total_real);
				break;
			case MTYPE_COUNTER:
				sprintf(szTmp, "%" PRIu64, total_real);
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

MainWorker::eSwitchLightReturnCode MainWorker::SwitchLightInt(const std::vector<std::string>& sd, std::string switchcmd, int level, const _tColor color, const bool IsTesting, const std::string& User)
{
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
	{
		_log.Log(LOG_ERROR, "Switch command not send!, Hardware device disabled or not found!");
		return SL_ERROR;
	}
	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return SL_ERROR;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
	uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
	uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
	uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

	_log.Debug(DEBUG_NORM, "MAIN SwitchLightInt : switchcmd:%s level:%d HWid:%d  sd:%s %s %s %s %s %s", switchcmd.c_str(), level, HardwareID,
		sd[0].c_str(), sd[1].c_str(), sd[2].c_str(), sd[3].c_str(), sd[4].c_str(), sd[5].c_str());


	std::string deviceID = sd[1];
	int Unit = atoi(sd[2].c_str());
	int dType = atoi(sd[3].c_str());
	int dSubType = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());

	std::string lstatus;
	int llevel = 0;
	bool bHaveDimmer = false;
	bool bHaveGroupCmd = false;
	int maxDimLevel = 0;
	int nValue = atoi(sd[7].c_str());
	std::string sValue = sd[8];
	GetLightStatus(dType, dSubType, switchtype, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);

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
				return SL_ERROR;
			UpdateDomoticzSecurityStatus(iSecStatus, User);
			return SL_OK;
		}
	}

	std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sd[10]);

	const bool bIsBlinds = (
		switchtype == STYPE_Blinds
		|| switchtype == STYPE_BlindsPercentage
		|| switchtype == STYPE_BlindsPercentageWithStop
		|| switchtype == STYPE_VenetianBlindsEU
		|| switchtype == STYPE_VenetianBlindsUS
		);

	// If dimlevel is 0 or no dimlevel, turn switch off
	if ((level <= 0 && switchcmd == "Set Level") && (!bIsBlinds))
		switchcmd = "Off";
	// If command is Off, we retrieve/store our previous level below
	if ((switchcmd == "Off") && (!bIsBlinds))
		level = (switchtype != STYPE_Selector) ? -1 : 0;

	//when level is invalid or command is "On", replace level with "LastLevel"
	if (switchcmd == "On" || level < 0)
	{
		//Get LastLevel
		std::vector<std::vector<std::string> > result;
		result = m_sql.safe_query(
			"SELECT LastLevel FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit=%d AND Type=%d AND SubType=%d)", HardwareID, sd[1].c_str(), Unit, int(dType), int(dSubType));
		if (result.size() == 1)
		{
			level = atoi(result[0][0].c_str());
			if (
				(switchcmd == "On")
				&& (level > 0)
				&& (switchtype == STYPE_Dimmer)
				)
			{
				switchcmd = "Set Level";
			}
		}

		//level here is from 0-100, convert it to device range
		if ((maxDimLevel != 0) && (switchtype != STYPE_Selector))
		{
			float fLevel = (maxDimLevel / 100.0F) * level;
			if (fLevel > 100)
				fLevel = 100;
			level = ground(fLevel);
		}
		_log.Debug(DEBUG_NORM, "MAIN SwitchLightInt : switchcmd==\"On\" || level < 0, new level:%d", level);
	}
	level = std::max(level, 0);

	if (bIsBlinds)
	{
		std::string szOrgCommand = switchcmd;
		bool bReverseState = false;
		bool bReversePosition = false;

		auto itt = options.find("ReverseState");
		if (itt != options.end())
			bReverseState = (itt->second == "true");
		itt = options.find("ReversePosition");
		if (itt != options.end())
			bReversePosition = (itt->second == "true");

		if ((bReversePosition) && (switchcmd == "Set Level"))
		{
			level = 100 - level;
		}

		if ((switchcmd == "Off") || (switchcmd == "Set Level" && level == 0))
			switchcmd = "Close";
		else if ((switchcmd == "On") || (switchcmd == "Set Level" && level == 100))
			switchcmd = "Open";

		if (switchcmd == "Open")
			level = 100;
		else if (switchcmd == "Close")
			level = 0;

		if ((bReverseState) && (szOrgCommand != "Set Level"))
		{
			if (switchcmd == "Open")
				switchcmd = "Close";
			else if (switchcmd == "Close")
				switchcmd = "Open";
		}
	}

	//when asking for Toggle, just switch to the opposite value
	if (switchcmd == "Toggle") {
		//Flip the status
		if (IsLightSwitchOn(lstatus) == true)
			switchcmd = (!bIsBlinds) ? "Off" : "Close";
		else
			switchcmd = (!bIsBlinds) ? "On" : "Open";
	}

	//
	//	For plugins all the specific logic below is irrelevent
	//	so just send the full details to the plugin so that it can take appropriate action
	//
	if (pHardware->HwdType == HTYPE_PythonPlugin)
	{
#ifdef ENABLE_PYTHON
		// Special case when color is passed from timer or scene
		if ((switchcmd == "On") || (switchcmd == "Set Level"))
		{
			if (color.mode != ColorModeNone)
			{
				switchcmd = "Set Color";
			}
		}
		((Plugins::CPlugin*)m_hardwaredevices[hindex])->SendCommand(sd[1], Unit, switchcmd, level, color);
#endif
		return SL_OK;
	}
	if (pHardware->HwdType == HTYPE_MQTTAutoDiscovery)
	{
		// Special case when color is passed from timer or scene
		if ((switchcmd == "Set Level") && (color.mode != ColorModeNone))
		{
			switchcmd = "Set Color";
		}
		bool bret = ((MQTTAutoDiscover*)m_hardwaredevices[hindex])->SendSwitchCommand(sd[1], sd[9], Unit, switchcmd, level, color, User);
		return (bret == true) ? SL_OK : SL_ERROR;
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
			return SL_ERROR;
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
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
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
		else if (
			(switchtype == STYPE_BlindsPercentage)
			|| (switchtype == STYPE_BlindsPercentageWithStop)
			)
		{
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

		lcmd.LIGHTING2.level = (uint8_t)level;

		if ((pHardware->HwdType == HTYPE_EnOceanESP2) && (IsTesting) && (switchtype == STYPE_Dimmer))
		{ // Special Teach-In for EnOcean ESP2 dimmers
			CEnOceanESP2* pEnocean = dynamic_cast<CEnOceanESP2*>(pHardware);
			pEnocean->SendDimmerTeachIn((const char*)&lcmd, sizeof(lcmd.LIGHTING1));
		}
		else if (switchtype != STYPE_Motion)
		{ // Don't send actual motion off command
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING2)))
				return SL_ERROR;
		}

		if (!IsTesting)
		{ //send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeLighting3:
	{
		if (level > 9)
			level = 9;
	}
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
				return SL_ERROR;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
			}
			return SL_OK;
		}
		return SL_ERROR;
	}
	break;
	case pTypeLighting5:
	{
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
			return SL_ERROR;
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
		lcmd.LIGHTING5.level = (uint8_t)level;
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
				uint8_t oldCmd = lcmd.LIGHTING5.cmnd;
				lcmd.LIGHTING5.cmnd = light5_sLivoloAllOff;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return SL_ERROR;
				lcmd.LIGHTING5.cmnd = oldCmd;
			}
			if (switchcmd == "Set Level")
			{
				//dim value we have to send multiple times
				for (int iDim = 0; iDim < level; iDim++)
					if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
						return SL_ERROR;
			}
			else
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return SL_ERROR;
		}
		else if ((dSubType == sTypeTRC02) || (dSubType == sTypeTRC02_2))
		{
			//int oldlevel = level;
			if (switchcmd != "Off")
			{
				if (color.mode == ColorModeRGB)
				{
					switchcmd = "Set Color";
				}
			}
			if ((switchcmd == "Off") ||
				(switchcmd == "On") ||      //Special Case, turn off first to ensure light is in normal mode
				(switchcmd == "Set Color"))
			{
				uint8_t oldCmd = lcmd.LIGHTING5.cmnd;
				lcmd.LIGHTING5.cmnd = light5_sRGBoff;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return SL_ERROR;
				lcmd.LIGHTING5.cmnd = oldCmd;
				sleep_milliseconds(100);
			}
			if ((switchcmd == "On") || (switchcmd == "Set Color"))
			{
				//turn on
				lcmd.LIGHTING5.cmnd = light5_sRGBon;
				if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
					return SL_ERROR;
				sleep_milliseconds(100);

				if (switchcmd == "Set Color")
				{
					if (color.mode == ColorModeRGB)
					{
						float hsb[3];
						rgb2hsb(color.r, color.g, color.b, hsb);
						switchcmd = "Set Color";

						float dval = 126.0F * hsb[0]; // Color Range is 0x06..0x84
						lcmd.LIGHTING5.cmnd = light5_sRGBcolormin + 1 + ground(dval);
						if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
							return SL_ERROR;
					}
				}
			}
		}
		else
		{
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING5)))
				return SL_ERROR;
		}
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.LIGHTING6)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeFS20:
	{
		tRBUF lcmd;
		lcmd.FS20.packetlength = sizeof(lcmd.FS20) - 1;
		lcmd.FS20.packettype = dType;
		lcmd.FS20.subtype = dSubType;
		lcmd.FS20.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.FS20.hc1 = ID3;
		lcmd.FS20.hc2 = ID4;
		lcmd.FS20.addr = Unit;
		lcmd.FS20.filler = 0;
		lcmd.FS20.rssi = 12;
		lcmd.FS20.cmd2 = 0;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.FS20.cmd1, options))
			return SL_ERROR;
		level = (level > 15) ? 15 : level;

		if (level > 0)
		{
			lcmd.FS20.cmd1 = fs20_sDimlevel_1 + level;
		}

		if (switchtype != STYPE_Motion) //dont send actual motion off command
		{
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.FS20)))
				return SL_ERROR;
		}

		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.HOMECONFORT)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.FAN)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeColorSwitch:
	{
		_tColorSwitch lcmd;
		lcmd.subtype = dSubType;
		lcmd.id = ID;
		lcmd.dunit = Unit;
		lcmd.color = color;
		level = std::min(100, level);
		level = std::max(0, level);
		lcmd.value = level;

		//Special case when color is passed from timer or scene
		if ((switchcmd == "On") || (switchcmd == "Set Level"))
		{
			if (color.mode != ColorModeNone)
			{
				switchcmd = "Set Color";
			}
		}
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.command, options))
			return SL_ERROR;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(_tColorSwitch)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
				return SL_ERROR;
			//send it twice
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return SL_ERROR;
			sleep_milliseconds(500);
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return SL_ERROR;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
			}
		}
		break;
		case sTypeSecX10M:
		case sTypeSecX10R:
		case sTypeSecX10:
		case sTypeMeiantech:
		{
			if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.SECURITY1.status, options))
				return SL_ERROR;
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY1)))
				return SL_ERROR;
			if (!IsTesting) {
				//send to internal for now (later we use the ACK)
				PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
			}
		}
		break;
		}
		return SL_OK;
	}
	break;
	case pTypeSecurity2:
	{
		BYTE kCodes[9];
		if (sd[1].size() < 8 * 2)
		{
			return SL_ERROR;
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
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeHunter:
	{
		BYTE kCodes[6];
		if (sd[1].size() < 6 * 2)
		{
			return SL_ERROR;
		}
		for (int ii = 0; ii < 6; ii++)
		{
			std::string sHex = sd[1].substr(ii * 2, 2);
			std::stringstream s_strid;
			int iHex = 0;
			s_strid << std::hex << sHex;
			s_strid >> iHex;
			kCodes[ii] = (BYTE)iHex;
		}
		tRBUF lcmd;
		lcmd.HUNTER.packetlength = sizeof(lcmd.HUNTER) - 1;
		lcmd.HUNTER.packettype = dType;
		lcmd.HUNTER.subtype = dSubType;
		lcmd.HUNTER.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.HUNTER.id1 = kCodes[0];
		lcmd.HUNTER.id2 = kCodes[1];
		lcmd.HUNTER.id3 = kCodes[2];
		lcmd.HUNTER.id4 = kCodes[3];
		lcmd.HUNTER.id5 = kCodes[4];
		lcmd.HUNTER.id6 = kCodes[5];
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.HUNTER.cmnd, options))
			return SL_ERROR;
		lcmd.HUNTER.rssi = 12;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.SECURITY2)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
		lcmd.CURTAIN1.filler = 0;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.CURTAIN1)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
		if (
			(dSubType == sTypeBlindsT0)
			|| (dSubType == sTypeBlindsT1)
			|| (dSubType == sTypeBlindsT3)
			|| (dSubType == sTypeBlindsT8)
			|| (dSubType == sTypeBlindsT12)
			|| (dSubType == sTypeBlindsT13)
			|| (dSubType == sTypeBlindsT14)
			|| (dSubType == sTypeBlindsT15)
			|| (dSubType == sTypeBlindsT16)
			|| (dSubType == sTypeBlindsT17)
			|| (dSubType == sTypeBlindsT18)
			)
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
			return SL_ERROR;
		level = 15;
		lcmd.BLINDS1.filler = 0;
		lcmd.BLINDS1.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.BLINDS1)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeRFY:
	{
		tRBUF lcmd;
		lcmd.RFY.packetlength = sizeof(lcmd.RFY) - 1;
		lcmd.RFY.packettype = dType;
		lcmd.RFY.subtype = dSubType;
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
				return SL_ERROR;
		}

		bool bIsRFY2 = (lcmd.RFY.subtype == sTypeRFY2);

		if (bIsRFY2)
		{
			//Special case for protocol version 2
			lcmd.RFY.subtype = sTypeRFY;
			if (lcmd.RFY.cmnd == rfy_sUp)
				lcmd.RFY.cmnd = rfy_s2SecUp;
			else if (lcmd.RFY.cmnd == rfy_sDown)
				lcmd.RFY.cmnd = rfy_s2SecDown;
		}

		level = 15;
		lcmd.RFY.filler = 0;
		lcmd.RFY.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RFY)))
			return SL_ERROR;
		if (!IsTesting) {
			if (bIsRFY2)
				lcmd.RFY.subtype = sTypeRFY2;
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeDDxxxx:
	{
		tRBUF lcmd;
		lcmd.DDXXXX.packetlength = sizeof(lcmd.DDXXXX) - 1;
		lcmd.DDXXXX.packettype = dType;
		lcmd.DDXXXX.subtype = dSubType;
		lcmd.DDXXXX.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.DDXXXX.id1 = ID1;
		lcmd.DDXXXX.id2 = ID2;
		lcmd.DDXXXX.id3 = ID3;
		lcmd.DDXXXX.id4 = ID4;
		lcmd.DDXXXX.unitcode = Unit;
		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, lcmd.DDXXXX.cmnd, options))
			return SL_ERROR;
		lcmd.DDXXXX.filler = 0;
		lcmd.DDXXXX.rssi = 12;

		level = std::min(100, level);
		level = std::max(0, level);
		if (
			(lcmd.DDXXXX.cmnd != DDxxxx_Percent)
			&& (lcmd.DDXXXX.cmnd != DDxxxx_PercentAngle)
			)
		{
			level = 0;
		}
		else
		{
			lcmd.DDXXXX.percent = level;
		}

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.DDXXXX)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeChime:
	{
		level = 15;
		tRBUF lcmd;
		lcmd.CHIME.packetlength = sizeof(lcmd.CHIME) - 1;
		lcmd.CHIME.packettype = dType;
		lcmd.CHIME.subtype = dSubType;
		lcmd.CHIME.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		if (dSubType == sTypeByronBY)
		{
			lcmd.CHIME.id1 = ID2;
			lcmd.CHIME.id2 = ID3;
			lcmd.CHIME.sound = ID4;
		}
		else
		{
			lcmd.CHIME.id1 = ID3;
			lcmd.CHIME.id2 = ID4;
			lcmd.CHIME.sound = Unit;
		}
		lcmd.CHIME.id4 = 0;
		lcmd.CHIME.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.CHIME)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;

		lcmd.THERMOSTAT2.filler = 0;
		lcmd.THERMOSTAT2.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT2)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
			return SL_ERROR;
		level = 15;
		lcmd.THERMOSTAT3.filler = 0;
		lcmd.THERMOSTAT3.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT3)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
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
		return SL_ERROR;
		level = 15;
		lcmd.THERMOSTAT4.filler = 0;
		lcmd.THERMOSTAT4.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.THERMOSTAT4)))
		return SL_ERROR;
		if (!IsTesting) {
		//send to internal for now (later we use the ACK)
		PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t *)&lcmd, NULL, -1);
		}
		*/
		return SL_OK;
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
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeEvohomeRelay:
	{
		_tEVOHOME3 lcmd;
		memset(&lcmd, 0, sizeof(_tEVOHOME3));
		lcmd.len = sizeof(_tEVOHOME3) - 1;
		lcmd.type = pTypeEvohomeRelay;
		lcmd.subtype = sTypeEvohomeRelay;
		RFX_SETID3(ID, lcmd.id1, lcmd.id2, lcmd.id3)
			lcmd.devno = Unit;
		if (switchcmd == "On")
			lcmd.demand = 200;
		else
			lcmd.demand = level;

		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(_tEVOHOME3)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeRadiator1:
	{
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
			return SL_ERROR;
		if (level > 15)
			level = 15;
		lcmd.RADIATOR1.temperature = 0;
		lcmd.RADIATOR1.tempPoint5 = 0;
		lcmd.RADIATOR1.filler = 0;
		lcmd.RADIATOR1.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RADIATOR1)))
			return SL_ERROR;

		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			lcmd.RADIATOR1.subtype = sTypeSmartwaresSwitchRadiator;
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeHoneywell_AL:
	{
		level = 15;
		tRBUF lcmd;
		lcmd.HONEYWELL_AL.packetlength = sizeof(lcmd.HONEYWELL_AL) - 1;
		lcmd.HONEYWELL_AL.packettype = dType;
		lcmd.HONEYWELL_AL.subtype = dSubType;
		lcmd.HONEYWELL_AL.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		lcmd.HONEYWELL_AL.knock = (Unit & 0xF0) >> 4;
		lcmd.HONEYWELL_AL.alert = Unit & 0x0F;
		lcmd.CHIME.rssi = 12;
		if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.HONEYWELL_AL)))
			return SL_ERROR;
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&lcmd, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	case pTypeGeneralSwitch:
	{
		tRBUF lcmd;
		_tGeneralSwitch gswitch;
		gswitch.type = dType;
		gswitch.subtype = dSubType;
		gswitch.seqnbr = m_hardwaredevices[hindex]->m_SeqNr++;
		gswitch.id = ID;
		gswitch.unitcode = Unit;

		if (!GetLightCommand(dType, dSubType, switchtype, switchcmd, gswitch.cmnd, options))
			return SL_ERROR;

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
				int maxLevel = static_cast<int>(statuses.size() - 1) * 10;

				if (
					(level < 0)
					|| (level > maxLevel)
					)
				{
					_log.Log(LOG_ERROR, "Setting a wrong level value %d to Selector device %lu", level, ID);
					return SL_ERROR;
				}
			}
		}
#ifdef WITH_OPENZWAVE
		else if (pHardware->HwdType == HTYPE_OpenZWave)
		{
			if (
				(switchtype == STYPE_BlindsPercentage)
				|| (switchtype == STYPE_BlindsPercentageWithStop)
				|| (switchtype == STYPE_VenetianBlindsUS)
				|| (switchtype == STYPE_VenetianBlindsEU)
				)
			{
				if (
					(gswitch.cmnd == gswitch_sSetLevel)
					&& (level == 100)
					)
				{
					//For Multilevel switches, 255 (0xFF) means Restore to most recent (non-zero) level,
					//which is perfect for dimmers, but for blinds (and using the slider), we set it to 99%
					//this should be done in the openzwave class, but it is deprecated and will be removed soon
					level = 99;
					if (gswitch.cmnd == gswitch_sOpen)
					{
						gswitch.cmnd = gswitch_sSetLevel;
					}
				}
			}
		}
#endif
		gswitch.level = (uint8_t)level;
		gswitch.rssi = 12;
		if (switchtype != STYPE_Motion) //dont send actual motion off command
		{
			if (!WriteToHardware(HardwareID, (const char*)&gswitch, sizeof(_tGeneralSwitch)))
				return SL_ERROR;
		}
		if (!IsTesting) {
			//send to internal for now (later we use the ACK)
			PushAndWaitRxMessage(m_hardwaredevices[hindex], (const uint8_t*)&gswitch, nullptr, -1, User.c_str());
		}
		return SL_OK;
	}
	break;
	}
	return SL_ERROR;
}

bool MainWorker::SwitchEvoModal(const std::string& idx, const std::string& status, const std::string& action, const std::string& ooc, const std::string& until)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,nValue FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	std::vector<std::string> sd = result[0];

	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	//uint8_t Unit = atoi(sd[2].c_str());
	//uint8_t dType = atoi(sd[3].c_str());
	//uint8_t dSubType = atoi(sd[4].c_str());
	//_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SwitchEvoModal(idx, status, action, ooc, until);
	}

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

	unsigned long ID;
	std::stringstream s_strid;
	if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		s_strid << std::hex << sd[1];
	else  //GB3: web based evohome uses decimal device ID's. We need to convert those to hex here to fit the 3-byte ID defined in the message struct
		s_strid << std::hex << std::dec << sd[1];
	s_strid >> ID;

	//Update Domoticz evohome Device
	_tEVOHOME1 tsen;
	memset(&tsen, 0, sizeof(_tEVOHOME1));
	tsen.len = sizeof(_tEVOHOME1) - 1;
	tsen.type = pTypeEvohome;
	tsen.subtype = sTypeEvohome;
	RFX_SETID3(ID, tsen.id1, tsen.id2, tsen.id3)
		tsen.action = (action == "1") ? 1 : 0;
	tsen.status = nStatus;

	tsen.mode = until.empty() ? CEvohomeBase::cmPerm : CEvohomeBase::cmTmp;
	if (tsen.mode == CEvohomeBase::cmTmp)
		CEvohomeDateTime::DecodeISODate(tsen, until.c_str());
	WriteToHardware(HardwareID, (const char*)&tsen, sizeof(_tEVOHOME1));

	//the latency on the scripted solution is quite bad so it's good to see the update happening...ideally this would go to an 'updating' status (also useful to update database if we ever use this as a pure virtual device)
	PushRxMessage(pHardware, (const uint8_t*)&tsen, nullptr, 255, nullptr);
	return true;
}

MainWorker::eSwitchLightReturnCode MainWorker::SwitchLight(const std::string& idx, const std::string& switchcmd, const std::string& level, const std::string& color, const std::string& ooc, const int ExtraDelay, const std::string& User)
{
	uint64_t ID = std::stoull(idx);
	int ilevel = -1;
	if (!level.empty())
		ilevel = atoi(level.c_str());

	return SwitchLight(ID, switchcmd, ilevel, _tColor(color), atoi(ooc.c_str()) != 0, ExtraDelay, User);
}

MainWorker::eSwitchLightReturnCode MainWorker::SwitchLight(const uint64_t idx, const std::string& switchcmd, const int level, _tColor color, const bool ooc, const int ExtraDelay, const std::string& User)
{
	//Get Device details
	_log.Debug(DEBUG_NORM, "MAIN SwitchLight idx:%" PRIu64 " cmd:%s lvl:%d ", idx, switchcmd.c_str(), level);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID,DeviceID,Unit,Type,SubType,SwitchType,AddjValue2,nValue,sValue,Name,Options FROM DeviceStatus WHERE (ID == %" PRIu64 ")",
		idx);
	if (result.empty())
		return SL_ERROR;

	std::vector<std::string> sd = result[0];
	std::string hwdid = sd[0];
	std::string devid = sd[1];
	int dtype = atoi(sd[3].c_str());
	int subtype = atoi(sd[4].c_str());
	_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());
	int iOnDelay = atoi(sd[6].c_str());
	int nValue = atoi(sd[7].c_str());
	std::string sValue = sd[8];
	std::string devName = sd[9];

	//std::string sOptions = sd[10].c_str();
	// ----------- If needed convert to GeneralSwitch type (for o.a. RFlink) -----------
	CDomoticzHardwareBase* pBaseHardware = m_mainworker.GetHardware(atoi(hwdid.c_str()));
	if (pBaseHardware != nullptr)
	{
		if (pBaseHardware->HwdType == HTYPE_ZIBLUEUSB || pBaseHardware->HwdType == HTYPE_ZIBLUETCP)
		{
			ConvertToGeneralSwitchType(devid, dtype, subtype);
			sd[1] = devid;
			sd[3] = std::to_string(dtype);
			sd[4] = std::to_string(subtype);
		}
	}
	bool bIsOn = IsLightSwitchOn(switchcmd);
	if (ooc)//Only on change
	{
		int nNewVal = bIsOn ? 1 : 0;//Is that everything we need here
		if ((switchtype == STYPE_Selector) && (nValue == nNewVal) && (level == atoi(sValue.c_str()))) {
			return SL_OK_NO_ACTION;
		}
		if (nValue == nNewVal)
		{
			return SL_OK_NO_ACTION;
		}
	}
	//Check if we have an On-Delay, if yes, add it to the tasker
	if (((bIsOn) && (iOnDelay != 0)) || ExtraDelay)
	{
		if (iOnDelay + ExtraDelay != 0)
		{
			_log.Log(LOG_NORM, "Delaying switch [%s] action (%s) for %d seconds", devName.c_str(), switchcmd.c_str(), iOnDelay + ExtraDelay);
		}
		m_sql.AddTaskItem(_tTaskItem::SwitchLightEvent(static_cast<float>(iOnDelay + ExtraDelay), idx, switchcmd, level, color, "Switch with Delay", User));
		return SL_OK;
	}

	int HardwareID = atoi(hwdid.c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
	{
		_log.Log(LOG_ERROR, "Switch command not send!, Hardware device disabled or not found!");
		return SL_ERROR;
	}
	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return SL_ERROR;
	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP *pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return (MainWorker::eSwitchLightReturnCode)pDomoticz->SwitchLight(idx, switchcmd, level, color, ooc, User);
	}

	return SwitchLightInt(sd, switchcmd, level, color, false, User);
}

//Seems this is only called for EvoHome, so this function needs to be moved to the EvoHome (base)class!
//(and modify Scheduler scripts)
bool MainWorker::SetSetPointEvo(const std::string& idx, const float TempValue, const std::string& newMode, const std::string& until)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,ID,Options FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	std::vector<std::string> sd = result[0];
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType != HTYPE_EVOHOME_SCRIPT && pHardware->HwdType != HTYPE_EVOHOME_SERIAL && pHardware->HwdType != HTYPE_EVOHOME_WEB && pHardware->HwdType != HTYPE_EVOHOME_TCP)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetSetPointEvo(idx, TempValue, newMode, until);
	}

	int nEvoMode = 0;
	if (newMode == "PermanentOverride" || newMode.empty())
		nEvoMode = CEvohomeBase::zmPerm;
	else if (newMode == "TemporaryOverride")
		nEvoMode = CEvohomeBase::zmTmp;

	//_log.Log(LOG_DEBUG, "Set point %s %f '%s' '%s'", idx.c_str(), TempValue, newMode.c_str(), until.c_str());

	unsigned long ID;
	std::stringstream s_strid;
	if (pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		s_strid << std::hex << sd[1];
	else //GB3: web based evohome uses decimal device ID's. We need to convert those to hex here to fit the 3-byte ID defined in the message struct
		s_strid << std::hex << std::dec << sd[1];
	s_strid >> ID;


	uint8_t Unit = atoi(sd[2].c_str());
	uint8_t dType = atoi(sd[3].c_str());
	uint8_t dSubType = atoi(sd[4].c_str());
	//_eSwitchType switchtype=(_eSwitchType)atoi(sd[5].c_str());

	if (pHardware->HwdType == HTYPE_EVOHOME_SCRIPT || pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_WEB || pHardware->HwdType == HTYPE_EVOHOME_TCP)
	{
		_tEVOHOME2 tsen;
		memset(&tsen, 0, sizeof(_tEVOHOME2));
		tsen.len = sizeof(_tEVOHOME2) - 1;
		tsen.type = dType;
		tsen.subtype = dSubType;
		RFX_SETID3(ID, tsen.id1, tsen.id2, tsen.id3)
			tsen.zone = Unit;//controller is 0 so let our zones start from 1...
		tsen.updatetype = CEvohomeBase::updSetPoint;//setpoint
		tsen.temperature = static_cast<int16_t>((dType == pTypeEvohomeWater) ? TempValue : TempValue * 100.0F);
		tsen.mode = nEvoMode;
		if (nEvoMode == CEvohomeBase::zmTmp)
			CEvohomeDateTime::DecodeISODate(tsen, until.c_str());
		WriteToHardware(HardwareID, (const char*)&tsen, sizeof(_tEVOHOME2));

		//Pass across the current controller mode if we're going to update as per the hw device
		result = m_sql.safe_query(
			"SELECT Name,DeviceID,nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==0)",
			HardwareID);
		if (!result.empty())
		{
			sd = result[0];
			tsen.controllermode = atoi(sd[2].c_str());
		}
		//the latency on the scripted solution is quite bad so it's good to see the update happening...ideally this would go to an 'updating' status (also useful to update database if we ever use this as a pure virtual device)
		PushAndWaitRxMessage(pHardware, (const uint8_t*)&tsen, nullptr, -1, nullptr);
	}
	return true;
}

bool MainWorker::SetSetPoint(const std::string& idx, const float TempValue)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType,StrParam1,ID,Options FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	std::vector<std::string> sd = result[0];

	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetSetPoint(idx, TempValue);
	}

	return SetSetPointInt(sd, TempValue);
}

bool MainWorker::SetSetPointInt(const std::vector<std::string>& sd, const float TempValue)
{
	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	bool ret = true;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << sd[1];
	s_strid >> ID;
	uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
	uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
	uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
	uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

	uint8_t Unit = atoi(sd[2].c_str());
	uint8_t dType = atoi(sd[3].c_str());
	uint8_t dSubType = atoi(sd[4].c_str());
	//_eSwitchType switchtype = (_eSwitchType)atoi(sd[5].c_str());

	_tSetpoint tmeter;
	tmeter.subtype = sTypeSetpoint;
	tmeter.id1 = ID1;
	tmeter.id2 = ID2;
	tmeter.id3 = ID3;
	tmeter.id4 = ID4;
	tmeter.dunit = Unit;

	if ((dType == pTypeSetpoint) && (dSubType == sTypeSetpoint))
	{
		std::string sOptions = sd[8].c_str();
		std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sOptions);
		std::string value_unit = options["ValueUnit"];

		if (
			(value_unit.empty())
			|| (value_unit == "�C")
			|| (value_unit == "�F")
			|| (value_unit == "C")
			|| (value_unit == "F")
			)
		{
			tmeter.value = (m_sql.m_tempsign[0] != 'F') ? TempValue : static_cast<float>(ConvertToCelsius(TempValue));
		}
		else
			tmeter.value = TempValue;
	}
	else
	{
		tmeter.value = (m_sql.m_tempsign[0] != 'F') ? TempValue : static_cast<float>(ConvertToCelsius(TempValue));
	}


	if (pHardware->HwdType == HTYPE_PythonPlugin)
	{
#ifdef ENABLE_PYTHON
		((Plugins::CPlugin*)pHardware)->SendCommand(sd[1], Unit, "Set Level", TempValue);
		return true;
#endif
	}
	else if (
		(pHardware->HwdType == HTYPE_OpenThermGateway)
		|| (pHardware->HwdType == HTYPE_OpenThermGatewayTCP)
		|| (pHardware->HwdType == HTYPE_ICYTHERMOSTAT)
		|| (pHardware->HwdType == HTYPE_TOONTHERMOSTAT)
		|| (pHardware->HwdType == HTYPE_AtagOne)
		|| (pHardware->HwdType == HTYPE_NEST)
		|| (pHardware->HwdType == HTYPE_Nest_OAuthAPI)
		|| (pHardware->HwdType == HTYPE_ANNATHERMOSTAT)
		|| (pHardware->HwdType == HTYPE_Tado)
		|| (pHardware->HwdType == HTYPE_EVOHOME_SCRIPT)
		|| (pHardware->HwdType == HTYPE_EVOHOME_SERIAL)
		|| (pHardware->HwdType == HTYPE_EVOHOME_TCP)
		|| (pHardware->HwdType == HTYPE_EVOHOME_WEB)
		|| (pHardware->HwdType == HTYPE_Netatmo)
		|| (pHardware->HwdType == HTYPE_NefitEastLAN)
		|| (pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF)
		|| (pHardware->HwdType == HTYPE_OpenWebNetTCP)
		|| (pHardware->HwdType == HTYPE_MQTT)
		|| (pHardware->HwdType == HTYPE_MQTTAutoDiscovery)
		|| (pHardware->HwdType == HTYPE_AlfenEveCharger)
		)
	{
		if (pHardware->HwdType == HTYPE_OpenThermGateway)
		{
			OTGWSerial* pGateway = dynamic_cast<OTGWSerial*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_OpenThermGatewayTCP)
		{
			OTGWTCP* pGateway = dynamic_cast<OTGWTCP*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_ICYTHERMOSTAT)
		{
			CICYThermostat* pGateway = dynamic_cast<CICYThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_TOONTHERMOSTAT)
		{
			CToonThermostat* pGateway = dynamic_cast<CToonThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_AtagOne)
		{
			CAtagOne* pGateway = dynamic_cast<CAtagOne*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_NEST)
		{
			CNest* pGateway = dynamic_cast<CNest*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_Nest_OAuthAPI)
		{
			CNestOAuthAPI* pGateway = dynamic_cast<CNestOAuthAPI*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_ANNATHERMOSTAT)
		{
			CAnnaThermostat* pGateway = dynamic_cast<CAnnaThermostat*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_Tado)
		{
			CTado* pGateway = dynamic_cast<CTado*>(pHardware);
			pGateway->SetSetpoint(ID2, ID3, ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_Netatmo)
		{
			CNetatmo* pGateway = dynamic_cast<CNetatmo*>(pHardware);
			pGateway->SetSetpoint(ID, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_NefitEastLAN)
		{
			CNefitEasy* pGateway = dynamic_cast<CNefitEasy*>(pHardware);
			pGateway->SetSetpoint(ID2, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_EVOHOME_SCRIPT || pHardware->HwdType == HTYPE_EVOHOME_SERIAL || pHardware->HwdType == HTYPE_EVOHOME_WEB || pHardware->HwdType == HTYPE_EVOHOME_TCP)
		{
			return SetSetPointEvo(sd[7], TempValue, "PermanentOverride", "");
		}
		else if (pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF)
		{
			CInComfort* pGateway = dynamic_cast<CInComfort*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_OpenWebNetTCP)
		{
			COpenWebNetTCP* pGateway = dynamic_cast<COpenWebNetTCP*>(pHardware);
			ret = pGateway->SetSetpoint(ID, TempValue);
		}
		else if (pHardware->HwdType == HTYPE_MQTTAutoDiscovery)
		{
			MQTTAutoDiscover* pGateway = dynamic_cast<MQTTAutoDiscover*>(pHardware);
			return pGateway->SetSetpoint(sd[1], TempValue);
		}
		else if (pHardware->HwdType == HTYPE_AlfenEveCharger)
		{
			AlfenEve* pGateway = dynamic_cast<AlfenEve*>(pHardware);
			pGateway->SetSetpoint(ID4, TempValue);
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
			lcmd.RADIATOR1.temperature = (uint8_t)atoi(strarray[0].c_str());
			lcmd.RADIATOR1.tempPoint5 = (uint8_t)atoi(strarray[1].c_str());
			if (!WriteToHardware(HardwareID, (const char*)&lcmd, sizeof(lcmd.RADIATOR1)))
				return false;
			PushAndWaitRxMessage(pHardware, (const uint8_t*)&lcmd, nullptr, -1, nullptr);
			return true;
		}
		else
		{
			if (!WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tSetpoint)))
				return false;
		}
	}
	if (!ret)
		return false;
	//Also put it in the database, not all devices are awake (battery operated nodes)
	PushAndWaitRxMessage(pHardware, (const uint8_t*)&tmeter, nullptr, -1, nullptr);
	return true;
}

bool MainWorker::SetThermostatState(const std::string& idx, const int newState)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;
	int HardwareID = atoi(result[0][0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetThermostatState(idx, newState);
	}

	if (pHardware->HwdType == HTYPE_TOONTHERMOSTAT)
	{
		CToonThermostat* pGateway = dynamic_cast<CToonThermostat*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_AtagOne)
	{
		//CAtagOne *pGateway = dynamic_cast<CAtagOne*>(pHardware);
		//pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_NEST)
	{
		CNest* pGateway = dynamic_cast<CNest*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_Nest_OAuthAPI)
	{
		CNestOAuthAPI* pGateway = dynamic_cast<CNestOAuthAPI*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_ANNATHERMOSTAT)
	{
		CAnnaThermostat* pGateway = dynamic_cast<CAnnaThermostat*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_Netatmo)
	{
		CNetatmo* pGateway = dynamic_cast<CNetatmo*>(pHardware);
		int tIndex = atoi(idx.c_str());
		pGateway->SetProgramState(tIndex, newState);
		return true;
	}
	if (pHardware->HwdType == HTYPE_IntergasInComfortLAN2RF)
	{
		CInComfort* pGateway = dynamic_cast<CInComfort*>(pHardware);
		pGateway->SetProgramState(newState);
		return true;
	}
	return false;
}


bool MainWorker::SetTextDevice(const std::string& idx, const std::string& text)
{
	//Get Device details
	auto result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	std::vector<std::string> sd = result[0];

	int HardwareID = atoi(sd[0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;

	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetTextDevice(idx, text);
	}
	else if (pHardware->HwdType == HTYPE_MQTTAutoDiscovery)
	{
		MQTTAutoDiscover* pGateway = dynamic_cast<MQTTAutoDiscover*>(pHardware);
		return pGateway->SetTextDevice(sd[1], text);
	}
	m_sql.safe_query("UPDATE DeviceStatus SET sValue='%q' WHERE (ID == '%q')", text.c_str(), idx.c_str());
	m_sql.UpdateLastUpdate(idx);

	return false;
}

#ifdef WITH_OPENZWAVE
bool MainWorker::SetZWaveThermostatMode(const std::string& idx, const int tMode)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	int HardwareID = atoi(result[0][0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;
	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetZWaveThermostatMode(idx, tMode);
	}

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << result[0][1];
	s_strid >> ID;

	if (pHardware->HwdType != HTYPE_OpenZWave)
		return false;
	_tGeneralDevice tmeter;
	tmeter.subtype = sTypeZWaveThermostatMode;
	tmeter.intval1 = ID;
	tmeter.intval2 = tMode;
	return WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tGeneralDevice));
}

bool MainWorker::SetZWaveThermostatFanMode(const std::string& idx, const int fMode)
{
	//Get Device details
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query(
		"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType FROM DeviceStatus WHERE (ID == '%q')",
		idx.c_str());
	if (result.empty())
		return false;

	int HardwareID = atoi(result[0][0].c_str());
	int hindex = FindDomoticzHardware(HardwareID);
	if (hindex == -1)
		return false;
	CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
	if (pHardware == nullptr)
		return false;

	if (pHardware->HwdType == HTYPE_Domoticz)
	{
		DomoticzTCP* pDomoticz = static_cast<DomoticzTCP*>(pHardware);
		return pDomoticz->SetZWaveThermostatFanMode(idx, fMode);
	}
	if (pHardware->HwdType != HTYPE_OpenZWave)
		return false;

	unsigned long ID;
	std::stringstream s_strid;
	s_strid << std::hex << result[0][1];
	s_strid >> ID;

	_tGeneralDevice tmeter;
	tmeter.subtype = sTypeZWaveThermostatFanMode;
	tmeter.intval1 = ID;
	tmeter.intval2 = fMode;
	return WriteToHardware(HardwareID, (const char*)&tmeter, sizeof(_tGeneralDevice));
}

#endif

//returns if a device activates a scene
bool MainWorker::DoesDeviceActiveAScene(const uint64_t DevRowIdx, const int Cmnd)
{
	//check for scene code
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT Activators, SceneType FROM Scenes WHERE (Activators!='')");
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			int SceneType = atoi(sd[1].c_str());

			std::vector<std::string> arrayActivators;
			StringSplit(sd[0], ";", arrayActivators);
			for (const auto& sCodeCmd : arrayActivators)
			{
				std::vector<std::string> arrayCode;
				StringSplit(sCodeCmd, ":", arrayCode);

				std::string sID = arrayCode[0];
				std::string sCode;
				if (arrayCode.size() == 2)
				{
					sCode = arrayCode[1];
				}

				uint64_t aID = std::stoull(sID);
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

bool MainWorker::SwitchScene(const std::string& idx, const std::string& switchcmd, const std::string& User)
{
	uint64_t ID = std::stoull(idx);

	return SwitchScene(ID, switchcmd, User);
}

bool MainWorker::SwitchScene(const uint64_t idx, std::string switchcmd, const std::string& User)
{
	std::vector<std::vector<std::string> > result;
	int nValue = (switchcmd == "On") ? 1 : 0;

	std::string Name = "Unknown?";
	_eSceneGroupType scenetype = SGTYPE_SCENE;
	std::string onaction;
	std::string offaction;
	std::string status;

	//Get Scene details
	result = m_sql.safe_query("SELECT Name, SceneType, OnAction, OffAction, nValue FROM Scenes WHERE (ID == %" PRIu64 ")", idx);
	if (!result.empty())
	{
		std::vector<std::string> sds = result[0];
		Name = sds[0];
		scenetype = (_eSceneGroupType)atoi(sds[1].c_str());
		onaction = sds[2];
		offaction = sds[3];
		status = sds[4];

		if (scenetype == SGTYPE_GROUP)
		{
			std::vector<std::string> validCmdArray{ "On", "Off", "Toggle", "Group On", "Chime", "All On" };
			if (std::find(validCmdArray.begin(), validCmdArray.end(), switchcmd) == validCmdArray.end())
				return false;
			//when asking for Toggle, just switch to the opposite value
			if (switchcmd == "Toggle") {
				nValue = (atoi(status.c_str()) == 0 ? 1 : 0);
				switchcmd = (nValue == 1 ? "On" : "Off");
			}
		}
		else
		{
			if (switchcmd != "On")
				return false; //A Scene can only be turned On
		}
		m_sql.HandleOnOffAction((nValue == 1), onaction, offaction);
	}

	m_sql.safe_query("INSERT INTO SceneLog (SceneRowID, nValue, User) VALUES ('%" PRIu64 "', '%d', '%q')", idx, nValue, User.c_str());

	std::string sLastUpdate = TimeToString(nullptr, TF_DateTime);
	m_sql.safe_query("UPDATE Scenes SET nValue=%d, LastUpdate='%q' WHERE (ID == %" PRIu64 ")",
		nValue,
		sLastUpdate.c_str(),
		idx);

	//Check if we need to email a snapshot of a Camera
	std::string emailserver;
	int n2Value;
	if (m_sql.GetPreferencesVar("EmailServer", n2Value, emailserver))
	{
		if (!emailserver.empty())
		{
			result = m_sql.safe_query(
				"SELECT CameraRowID, DevSceneDelay FROM CamerasActiveDevices WHERE (DevSceneType==1) AND (DevSceneRowID==%" PRIu64 ") AND (DevSceneWhen==%d)",
				idx,
				!nValue
			);
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
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
		bEventTrigger = m_eventsystem.UpdateSceneGroup(idx, nValue, sLastUpdate);

	// Notify listeners
	sOnSwitchScene(idx, Name);

	//now switch all attached devices, and only the onces that do not trigger a scene
	result = m_sql.safe_query(
		"SELECT DeviceRowID, Cmd, Level, Color, OnDelay, OffDelay FROM SceneDevices WHERE (SceneRowID == %" PRIu64 ") ORDER BY [Order] ASC", idx);
	if (result.empty())
		return true; //no devices in the scene

	for (const auto& sd : result)
	{
		int cmd = atoi(sd[1].c_str());
		int level = atoi(sd[2].c_str());
		_tColor color(sd[3]);
		int ondelay = atoi(sd[4].c_str());
		int offdelay = atoi(sd[5].c_str());
		std::vector<std::vector<std::string> > result2;
		result2 = m_sql.safe_query(
			"SELECT HardwareID, DeviceID,Unit,Type,SubType,SwitchType, nValue, sValue, Name FROM DeviceStatus WHERE (ID == '%q')", sd[0].c_str());
		if (!result2.empty())
		{
			std::vector<std::string> sd2 = result2[0];
			//uint8_t rnValue = atoi(sd2[6].c_str());
			std::string sValue = sd2[7];
			//uint8_t Unit = atoi(sd2[2].c_str());
			uint8_t dType = atoi(sd2[3].c_str());
			uint8_t dSubType = atoi(sd2[4].c_str());
			std::string DeviceName = sd2[8];
			_eSwitchType switchtype = (_eSwitchType)atoi(sd2[5].c_str());

			//Check if this device will not activate a scene
			uint64_t dID = std::stoull(sd[0]);
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


			int ilevel = maxDimLevel - 1; // Why -1?

			if (
				(
					(switchtype == STYPE_Dimmer)
					|| (switchtype == STYPE_BlindsPercentage)
					|| (switchtype == STYPE_BlindsPercentageWithStop)
					|| (switchtype == STYPE_Selector)
					)
				&& (maxDimLevel != 0)
				)
			{
				if ((lstatus == "On") || (lstatus == "Open"))
				{
					lstatus = "Set Level";
					float fLevel = (maxDimLevel / 100.0F) * level;
					if (fLevel > 100)
						fLevel = 100;
					ilevel = ground(fLevel);
				}
				if (switchtype == STYPE_Selector) {
					if (lstatus != "Set Level") {
						ilevel = 0;
					}
					ilevel = ground(ilevel / 10.0F) * 10; // select only multiples of 10
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
				SwitchLight(idx, lstatus, ilevel, color, false, delay, User);
				if (scenetype == SGTYPE_SCENE)
				{
					if ((lstatus != "Off") && (offdelay > 0))
					{
						//switch with on delay, and off delay
						if (m_sql.m_bEnableEventSystem && !bEventTrigger)
							m_eventsystem.SetEventTrigger(idx, m_eventsystem.REASON_DEVICE, static_cast<float>(ondelay + offdelay));
						SwitchLight(idx, "Off", ilevel, color, false, ondelay + offdelay, User);
					}
				}
			}
			else
			{
				if (m_sql.m_bEnableEventSystem && !bEventTrigger)
					m_eventsystem.SetEventTrigger(idx, m_eventsystem.REASON_DEVICE, static_cast<float>(ondelay));
				SwitchLight(idx, "On", ilevel, color, false, ondelay, User);
			}
			sleep_milliseconds(50);
		}
	}
	return true;
}

void MainWorker::CheckSceneCode(const uint64_t DevRowIdx, const uint8_t dType, const uint8_t dSubType, const int nValue, const char* sValue, const std::string& User)
{
	//check for scene code
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT ID, Activators, SceneType FROM Scenes WHERE (Activators!='')");
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			std::vector<std::string> arrayActivators;
			StringSplit(sd[1], ";", arrayActivators);
			for (const auto& sCodeCmd : arrayActivators)
			{
				std::vector<std::string> arrayCode;
				StringSplit(sCodeCmd, ":", arrayCode);

				std::string sID = arrayCode[0];
				std::string sCode;
				if (arrayCode.size() == 2)
				{
					sCode = arrayCode[1];
				}

				uint64_t aID = std::stoull(sID);
				if (aID == DevRowIdx)
				{
					uint64_t ID = std::stoull(sd[0]);
					int scenetype = atoi(sd[2].c_str());
					int rnValue = nValue;

					if ((scenetype == SGTYPE_SCENE) && (!sCode.empty()))
					{
						//Also check code
						int iCode = atoi(sCode.c_str());
						if (iCode != nValue)
							continue;
						rnValue = 1; //A Scene can only be activated (On)
					}

					std::string lstatus;
					int llevel = 0;
					bool bHaveDimmer = false;
					bool bHaveGroupCmd = false;
					int maxDimLevel = 0;

					GetLightStatus(dType, dSubType, STYPE_OnOff, rnValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
					std::string switchcmd = (IsLightSwitchOn(lstatus) == true) ? "On" : "Off";

					m_sql.AddTaskItem(_tTaskItem::SwitchSceneEvent(0.2F, ID, switchcmd, "SceneTrigger", User));
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

	result = m_sql.safe_query("SELECT ID, Username, Password FROM USERS WHERE ((RemoteSharing==1) AND (Active==1))");
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			tcp::server::_tRemoteShareUser suser;
			suser.Username = base64_decode(sd[1]);
			suser.Password = sd[2];

			//Get User Devices
			result2 = m_sql.safe_query("SELECT DeviceRowID FROM SharedDevices WHERE (SharedUserID == '%q')", sd[0].c_str());
			if (!result2.empty())
			{
				for (const auto& sd2 : result2)
				{
					uint64_t ID = std::stoull(sd2[0]);
					suser.Devices.push_back(ID);
				}
			}
			users.push_back(suser);
		}
	}
	m_sharedserver.SetRemoteUsers(users);
	m_sharedserver.stopAllClients();
}

void MainWorker::SetInternalSecStatus(const std::string& User)
{
	m_eventsystem.WWWUpdateSecurityState(m_SecStatus);

	//Update Domoticz Security Device
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.SECURITY1.packetlength = sizeof(tsen.SECURITY1) - 1;
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

	if (_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
	{
		_log.Log(LOG_NORM, "(System) Domoticz Security Status");
	}

	CDomoticzHardwareBase* pHardware = GetHardwareByType(HTYPE_DomoticzInternal);
	PushAndWaitRxMessage(pHardware, (const uint8_t*)&tsen, "Domoticz Security Panel", -1, User.c_str());
}

void MainWorker::UpdateDomoticzSecurityStatus(const int iSecStatus, const std::string& User)
{
	m_SecCountdown = -1; //cancel possible previous delay
	m_SecStatus = iSecStatus;

	m_sql.UpdatePreferencesVar("SecStatus", iSecStatus);

	int nValue = 0;
	m_sql.GetPreferencesVar("SecOnDelay", nValue);

	if ((nValue == 0) || (iSecStatus == SECSTATUS_DISARMED))
	{
		//Do it Directly
		SetInternalSecStatus(User);
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
	std::string sTopic;

	if (_loglines.size() > 1)
	{
		sTopic = "Domoticz: Multiple errors received in the last 5 minutes";
		sstr << "Multiple errors received in the last 5 minutes:<br><br>";
	}
	else
	{
		auto itt = _loglines.begin();
		sTopic = "Domoticz: " + itt->logmessage;
	}

	for (const auto& str : _loglines)
		sstr << str.logmessage << "<br>";
	m_sql.AddTaskItem(_tTaskItem::SendEmail(1, sTopic, sstr.str()));
}

void MainWorker::HeartbeatUpdate(const std::string& component, bool critical /*= true*/)
{
	std::lock_guard<std::mutex> l(m_heartbeatmutex);
	time_t now = time(nullptr);
	auto itt = m_componentheartbeats.find(component);
	if (itt != m_componentheartbeats.end()) {
		itt->second.first = now;
	}
	else {
		m_componentheartbeats[component] = std::make_pair(now, critical);
	}
}

void MainWorker::HeartbeatRemove(const std::string& component)
{
	std::lock_guard<std::mutex> l(m_heartbeatmutex);
	auto itt = m_componentheartbeats.find(component);
	if (itt != m_componentheartbeats.end()) {
		m_componentheartbeats.erase(itt);
	}
}

void MainWorker::HeartbeatCheck()
{
	std::lock_guard<std::mutex> l(m_heartbeatmutex);
	std::lock_guard<std::mutex> l2(m_devicemutex);

	m_devicestorestart.clear();

	time_t now;
	mytime(&now);

	for (const auto& heartbeat : m_componentheartbeats)
	{
		double diff = difftime(now, heartbeat.second.first);
		if (diff > 60)
		{
			_log.Log(LOG_ERROR, "%s thread seems to have ended unexpectedly (last update %f seconds ago)",
				heartbeat.first.c_str(), diff);
			/* GizMoCuz: This causes long operations to crash (Like Issue #3011)
						if (heartbeat.second.second) // If the stalled component is marked as critical, call abort /
			raise signal
						{
							if (!IsDebuggerPresent())
							{
			#ifdef WIN32
								abort();
			#else
								raise(SIGUSR1);
			#endif
							}
						}
			*/
		}
	}

	//Check hardware heartbeats
	for (const auto& pHardware : m_hardwaredevices)
	{
		if (!pHardware->m_bSkipReceiveCheck)
		{
			//Check Thread Timeout
			bool bDoCheck = (pHardware->HwdType != HTYPE_Dummy) && (pHardware->HwdType != HTYPE_EVOHOME_SCRIPT);
			if (bDoCheck)
			{
				double diff = difftime(now, pHardware->m_LastHeartbeat);
				//_log.Log(LOG_STATUS, "%s last checking  %.2lf seconds ago", pHardware->m_Name.c_str(), diff);
				if (diff > 60)
				{
					_log.Log(LOG_ERROR, "%s hardware (%d) thread seems to have ended unexpectedly", pHardware->m_Name.c_str(), pHardware->m_HwdID);
					
				}
			}

			//Check received data timeout
			if (pHardware->m_DataTimeout > 0)
			{
				//Check Receive Timeout
				double diff = difftime(now, pHardware->m_LastHeartbeatReceive);
				bool bHaveDataTimeout = (diff > pHardware->m_DataTimeout);
				if (bHaveDataTimeout)
				{
					std::string sDataTimeout;
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
						totNum = pHardware->m_DataTimeout / 86400;
						if (totNum == 1) {
							sDataTimeout = "Day";
						}
						else {
							sDataTimeout = "Days";
						}
					}

					_log.Log(LOG_ERROR, "%s hardware (%d) nothing received for more than %d %s!....", pHardware->m_Name.c_str(), pHardware->m_HwdID, totNum, sDataTimeout.c_str());
					m_devicestorestart.push_back(pHardware->m_HwdID);
				}
			}
		}
	}
}

bool MainWorker::UpdateDevice(const int DevIdx, const int nValue, const std::string& sValue, const std::string& userName, const int signallevel, const int batterylevel, const bool parseTrigger)
{
	// Get the raw device parameters
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT HardwareID, OrgHardwareID, DeviceID, Unit, Type, SubType FROM DeviceStatus WHERE (ID==%d)", DevIdx);
	if (result.empty())
		return false;

	int HardwareID = std::stoi(result[0][0]);
	int OrgHardwareID = std::stoi(result[0][1]);
	std::string DeviceID = result[0][2];
	int unit = std::stoi(result[0][3]);
	int devType = std::stoi(result[0][4]);
	int subType = std::stoi(result[0][5]);

	return UpdateDevice(HardwareID, OrgHardwareID, DeviceID, unit, devType, subType, nValue, sValue, userName, signallevel, batterylevel, parseTrigger);
}

bool MainWorker::UpdateDevice(const int HardwareID, const int OrgHardwareID, const std::string& DeviceID, const int unit, const int devType, const int subType, const int nValue, std::string sValue,
	const std::string& userName, const int signallevel, const int batterylevel, const bool parseTrigger)
{
	g_bUseEventTrigger = parseTrigger;

	try
	{
		// Prevent hazardous modification of DB from JSON calls
		std::string devname = "Unknown";
		uint64_t devidx = m_sql.GetDeviceIndex(HardwareID, OrgHardwareID, DeviceID, unit, devType, subType, devname);
		if (devidx == (uint64_t)-1)
			return false;
		std::stringstream sidx;
		sidx << devidx;

		if (
			((devType == pTypeSetpoint) && (subType == sTypeSetpoint))
			|| ((devType == pTypeRadiator1) && (subType == sTypeSmartwares))
			)
		{
			SetSetPoint(sidx.str(), static_cast<float>(atof(sValue.c_str())));

#ifdef ENABLE_PYTHON
			// notify plugin
			m_pluginsystem.DeviceModified(devidx);
#endif
			m_notifications.CheckAndHandleNotification(devidx, HardwareID, DeviceID, devname, unit, devType, subType, nValue, sValue);

			// signal connected devices (MQTT, fibaro, http push ... ) about the update
			// sOnDeviceReceived(HardwareID, devidx, devname, nullptr);
			g_bUseEventTrigger = true;
			return true;
		}

		unsigned long ID = 0;
		std::stringstream s_strid;
		s_strid << std::hex << DeviceID;
		s_strid >> ID;

		CDomoticzHardwareBase* pHardware = GetHardware(HardwareID);
		if (pHardware)
		{
			if (devType == pTypeLighting2)
			{
				// Update as Lighting 2
				unsigned long ID;
				std::stringstream s_strid;
				s_strid << std::hex << DeviceID;
				s_strid >> ID;
				uint8_t ID1 = (uint8_t)((ID & 0xFF000000) >> 24);
				uint8_t ID2 = (uint8_t)((ID & 0x00FF0000) >> 16);
				uint8_t ID3 = (uint8_t)((ID & 0x0000FF00) >> 8);
				uint8_t ID4 = (uint8_t)((ID & 0x000000FF));

				tRBUF lcmd;
				memset(&lcmd, 0, sizeof(RBUF));
				lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
				lcmd.LIGHTING2.packettype = pTypeLighting2;
				lcmd.LIGHTING2.subtype = subType;
				lcmd.LIGHTING2.id1 = ID1;
				lcmd.LIGHTING2.id2 = ID2;
				lcmd.LIGHTING2.id3 = ID3;
				lcmd.LIGHTING2.id4 = ID4;
				lcmd.LIGHTING2.unitcode = (uint8_t)unit;
				lcmd.LIGHTING2.cmnd = (uint8_t)nValue;
				lcmd.LIGHTING2.level = (uint8_t)atoi(sValue.c_str());
				lcmd.LIGHTING2.filler = 0;
				lcmd.LIGHTING2.rssi = signallevel;
				DecodeRXMessage(pHardware, (const uint8_t*)&lcmd.LIGHTING2, nullptr, batterylevel, userName.c_str());
				g_bUseEventTrigger = true;
				return true;
			}

			if (
				(devType == pTypeTEMP)
				|| (devType == pTypeTEMP_HUM)
				|| (devType == pTypeTEMP_HUM_BARO)
				|| (devType == pTypeBARO)
				)
			{
				float temp = 12345.0F;

				// Adjustment value
				float AddjValue = 0.0F;
				float AddjMulti = 1.0F;
				m_sql.GetAddjustment(HardwareID, DeviceID.c_str(), unit, devType, subType, AddjValue, AddjMulti);

				char szTmp[100];
				std::vector<std::string> strarray;

				if (devType == pTypeTEMP)
				{
					temp = static_cast<float>(atof(sValue.c_str()));
					temp += AddjValue;
					sprintf(szTmp, "%.2f", temp);
					sValue = szTmp;
				}
				else if (devType == pTypeTEMP_HUM)
				{
					StringSplit(sValue, ";", strarray);
					if (strarray.size() == 3)
					{
						temp = static_cast<float>(atof(strarray[0].c_str()));
						temp += AddjValue;
						sprintf(szTmp, "%.2f;%s;%s", temp, strarray[1].c_str(), strarray[2].c_str());
						sValue = szTmp;
					}
				}
				else if (devType == pTypeTEMP_HUM_BARO)
				{
					StringSplit(sValue, ";", strarray);
					if (strarray.size() == 5)
					{
						temp = static_cast<float>(atof(strarray[0].c_str()));
						float fbarometer = static_cast<float>(atof(strarray[3].c_str()));
						temp += AddjValue;

						AddjValue = 0.0F;
						AddjMulti = 1.0F;
						m_sql.GetAddjustment2(HardwareID, DeviceID.c_str(), unit, devType, subType, AddjValue, AddjMulti);
						fbarometer += AddjValue;

						if (subType == sTypeTHBFloat)
						{
							sprintf(szTmp, "%.2f;%s;%s;%.1f;%s", temp, strarray[1].c_str(), strarray[2].c_str(), fbarometer, strarray[4].c_str());
						}
						else
						{
							sprintf(szTmp, "%.2f;%s;%s;%d;%s", temp, strarray[1].c_str(), strarray[2].c_str(), (int)rint(fbarometer), strarray[4].c_str());
						}
						sValue = szTmp;
					}
				}
				// Calculate temperature trend
				if (temp != 12345.0F)
				{
					uint64_t tID = ((uint64_t)(HardwareID & 0x7FFFFFFF) << 32) | (devidx & 0x7FFFFFFF);
					m_trend_calculator[tID].AddValueAndReturnTendency(static_cast<double>(temp), _tTrendCalculator::TAVERAGE_TEMP);
				}
			}
		}

		devidx = m_sql.UpdateValue(HardwareID, OrgHardwareID, DeviceID.c_str(), (const uint8_t)unit, (const uint8_t)devType, (const uint8_t)subType,
			signallevel,	 // signal level,
			batterylevel, // battery level
			nValue, sValue.c_str(), devname,
			false,
			userName.c_str());
		if (devidx == (uint64_t)-1)
		{
			g_bUseEventTrigger = true;
			return false;
		}

		if (pHardware)
		{
			if ((pHardware->HwdType == HTYPE_MySensorsUSB) || (pHardware->HwdType == HTYPE_MySensorsTCP) || (pHardware->HwdType == HTYPE_MySensorsMQTT))
			{
				unsigned long ID;
				std::stringstream s_strid;
				s_strid << std::hex << DeviceID;
				s_strid >> ID;
				uint8_t NodeID = (uint8_t)((ID & 0x0000FF00) >> 8);
				uint8_t ChildID = (uint8_t)((ID & 0x000000FF));

				MySensorsBase* pMySensorDevice = dynamic_cast<MySensorsBase*>(pHardware);
				pMySensorDevice->SendTextSensorValue(NodeID, ChildID, sValue);
			}
		}

#ifdef ENABLE_PYTHON
		// notify plugin
		m_pluginsystem.DeviceModified(devidx);
#endif
		// signal connected devices (MQTT, fibaro, http push ... ) about the update
		sOnDeviceReceived(HardwareID, devidx, devname, nullptr);
#ifdef WITH_OPENZWAVE
		if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatMode))
		{
			_log.Log(LOG_NORM, "Sending Thermostat Mode to device....");
			SetZWaveThermostatMode(sidx.str(), nValue);
		}
		else if ((devType == pTypeGeneral) && (subType == sTypeZWaveThermostatFanMode))
		{
			_log.Log(LOG_NORM, "Sending Thermostat Fan Mode to device....");
			SetZWaveThermostatFanMode(sidx.str(), nValue);
		}
		else if (pHardware)
#else
		if (pHardware)
#endif
		{
			// Handle Notification
			m_notifications.CheckAndHandleNotification(devidx, HardwareID, DeviceID, devname, unit, devType, subType, nValue, sValue);
		}

		g_bUseEventTrigger = true;

		return true;
	}
	catch (const std::exception& e)
	{
		_log.Log(LOG_ERROR, "MainWorker::UpdateDevice exception occurred : '%s'", e.what());
	}
	g_bUseEventTrigger = true;
	return false;
}

void MainWorker::HandleHourPrice()
{
	int iHP_E_Idx = 0;
	int iHP_G_Idx = 0;
	m_sql.GetPreferencesVar("HourIdxElectricityDevice", iHP_E_Idx);
	m_sql.GetPreferencesVar("HourIdxGasDevice", iHP_G_Idx);

	int SensorTimeOut = 60;
	m_sql.GetPreferencesVar("SensorTimeout", SensorTimeOut);

	time_t atime = mytime(nullptr);
	struct tm tm1;
	localtime_r(&atime, &tm1);

	float fHourPriceE = 0.0f;
	float fHourPriceG = 0.0f;

	if (iHP_E_Idx != 0)
	{
		if (iHP_E_Idx == 0x98765)
		{
			int iP1Hardware = m_mainworker.FindDomoticzHardwareByType(HTYPE_P1SmartMeter);
			if (iP1Hardware == -1)
				iP1Hardware = m_mainworker.FindDomoticzHardwareByType(HTYPE_P1SmartMeterLAN);

			int nValue = 0;
			m_sql.GetPreferencesVar("CostEnergy", nValue);
			float priceT1 = (float)(nValue) / 10000.0F;

			m_sql.GetPreferencesVar("CostEnergyT2", nValue);
			float priceT2 = (float)(nValue) / 10000.0F;

			if (iP1Hardware != -1)
			{
				P1MeterBase* pP1Meter = dynamic_cast<P1MeterBase*>(m_mainworker.GetHardware(iP1Hardware));
				if (pP1Meter != nullptr)
				{
					if (pP1Meter->m_current_tariff == 1)
						fHourPriceE = priceT1;
					else
						fHourPriceE = priceT2;
				}
			}
			else
			{
				fHourPriceE = priceT1;
			}
		}
		else
		{
			auto result = m_sql.safe_query("SELECT HardwareID, Type, SubType, sValue, LastUpdate, AddjValue2 FROM DeviceStatus WHERE (ID==%d)", iHP_E_Idx);
			if (!result.empty())
			{
				uint8_t hwdID = std::stoi(result[0][0]);
				const CDomoticzHardwareBase* pHardware = GetHardware(hwdID);
				if (pHardware != nullptr)
				{
					if (pHardware->HwdType == HTYPE_EneverPriceFeeds)
					{
						//Make sure the prices are actual
						Enever* pEnever = dynamic_cast<Enever*>(const_cast<CDomoticzHardwareBase*>(pHardware));
						pEnever->ActualizePrices();
						result = m_sql.safe_query("SELECT HardwareID, Type, SubType, sValue, LastUpdate, AddjValue2 FROM DeviceStatus WHERE (ID==%d)", iHP_E_Idx);
					}
				}

				uint8_t devType = std::stoi(result[0][1]);
				uint8_t subType = std::stoi(result[0][2]);
				std::string sValue = result[0][3];
				std::string sLastUpdate = result[0][4];

				struct tm ntime;
				time_t checktime;
				ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

				if (difftime(atime, checktime) < SensorTimeOut * 60)
				{
					if ((devType == pTypeGeneral) && (subType == sTypeCustom))
					{
						fHourPriceE = static_cast<float>(atof(sValue.c_str()));
					}
					else if ((devType == pTypeGeneral) && (subType == sTypeManagedCounter))
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							float AddjValue2 = std::stof(result[0][5]);
							if (AddjValue2 == 0)
								AddjValue2 = 1;
							fHourPriceG = static_cast<float>(atof(strarray[1].c_str())) / AddjValue2;
						}
					}
				}
			}
		}
	}
	if (iHP_G_Idx != 0)
	{
		if (iHP_G_Idx == 0x98765)
		{
			int nValue = 0;
			m_sql.GetPreferencesVar("CostGas", nValue);
			fHourPriceG = (float)(nValue) / 10000.0F;
		}
		else
		{
			auto result = m_sql.safe_query("SELECT Type, SubType, sValue, LastUpdate, AddjValue2 FROM DeviceStatus WHERE (ID==%d)", iHP_G_Idx);
			if (!result.empty())
			{
				uint8_t devType = std::stoi(result[0][0]);
				uint8_t subType = std::stoi(result[0][1]);
				std::string sValue = result[0][2];
				std::string sLastUpdate = result[0][3];

				struct tm ntime;
				time_t checktime;
				ParseSQLdatetime(checktime, ntime, sLastUpdate, tm1.tm_isdst);

				if (difftime(atime, checktime) < SensorTimeOut * 60)
				{
					if ((devType == pTypeGeneral) && (subType == sTypeCustom))
					{
						fHourPriceG = static_cast<float>(atof(sValue.c_str()));
					}
					else if ((devType == pTypeGeneral) && (subType == sTypeManagedCounter))
					{
						std::vector<std::string> strarray;
						StringSplit(sValue, ";", strarray);
						if (strarray.size() == 2)
						{
							float AddjValue2 = std::stof(result[0][4]);
							if (AddjValue2 == 0)
								AddjValue2 = 1;
							fHourPriceG = static_cast<float>(atof(strarray[1].c_str())) / AddjValue2;
						}
					}
				}
			}
		}
	}
	m_hourPriceElectricity.timestamp = atime;
	m_hourPriceGas.timestamp = atime;
	m_hourPriceWater.timestamp = atime;

	m_hourPriceElectricity.price = fHourPriceE;
	m_hourPriceGas.price = fHourPriceG;

	int nWaterPrice = 0;
	m_sql.GetPreferencesVar("CostWater", nWaterPrice);
	m_hourPriceWater.price = static_cast<float>(nWaterPrice) / 10000.0F;
}
