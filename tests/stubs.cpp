#include <boost/current_function.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../webserver/cWebem.h"

#include "../hardware/ASyncSerial.h"

#include "../hardware/DomoticzHardware.h"
#include "../json/value.h"
#include "../main/RFXNames.h"
#include "../main/Scheduler.h"
#include "../main/IFTTT.h"
#include "../main/WindCalculation.h"
#include "../notifications/NotificationBase.h"

#include "stubs.h"

#include "../main/mainworker.h"


#include "../hardware/ASyncTCP.h"
#include "../hardware/EvohomeBase.h"
#include "../main/EventSystem.h"
#include "../main/IFTTT.h"
#include "../main/WebServerHelper.h"
#include "../smtpclient/SMTPClient.h"
#include "../tcpserver/TCPServer.h"

// NOTE: these includes are copied from mainworker.cpp

#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"
#include "../main/localtime_r.h"
#include "../push/FibaroPush.h"
#include "../push/GooglePubSubPush.h"
#include "../push/HttpPush.h"
#include "../push/InfluxPush.h"
#include "../main/SunRiseSet.h"

#include "../httpclient/HTTPClient.h"

// Hardware Devices
#include "../hardware/DomoticzTCP.h"
#include "../hardware/P1MeterBase.h"
#include "../hardware/P1MeterSerial.h"
#include "../hardware/P1MeterTCP.h"
#include "../hardware/RFXBase.h"
#include "../hardware/RFXComSerial.h"
#include "../hardware/RFXComTCP.h"
#include "../hardware/YouLess.h"
#include "../hardware/hardwaretypes.h"
#ifdef WITH_LIBUSB
#include "../hardware/TE923.h"
#include "../hardware/VolcraftCO20.h"
#endif
#include "../hardware/1Wire.h"
#include "../hardware/AccuWeather.h"
#include "../hardware/AnnaThermostat.h"
#include "../hardware/Arilux.h"
#include "../hardware/AtagOne.h"
#include "../hardware/BleBox.h"
#include "../hardware/Buienradar.h"
#include "../hardware/Comm5SMTCP.h"
#include "../hardware/Comm5Serial.h"
#include "../hardware/Comm5TCP.h"
#include "../hardware/CurrentCostMeterSerial.h"
#include "../hardware/CurrentCostMeterTCP.h"
#include "../hardware/Daikin.h"
#include "../hardware/DarkSky.h"
#include "../hardware/DavisLoggerSerial.h"
#include "../hardware/DenkoviDevices.h"
#include "../hardware/DenkoviTCPDevices.h"
#include "../hardware/DenkoviUSBDevices.h"
#include "../hardware/DomoticzInternal.h"
#include "../hardware/Dummy.h"
#include "../hardware/ETH8020.h"
#include "../hardware/Ec3kMeterTCP.h"
#include "../hardware/EcoCompteur.h"
#include "../hardware/EcoDevices.h"
#include "../hardware/EnOceanESP2.h"
#include "../hardware/EnOceanESP3.h"
#include "../hardware/EnphaseAPI.h"
#include "../hardware/EvohomeBase.h"
#include "../hardware/EvohomeScript.h"
#include "../hardware/EvohomeSerial.h"
#include "../hardware/EvohomeTCP.h"
#include "../hardware/EvohomeWeb.h"
#include "../hardware/FritzboxTCP.h"
#include "../hardware/GoodweAPI.h"
#include "../hardware/HEOS.h"
#include "../hardware/HardwareMonitor.h"
#include "../hardware/HarmonyHub.h"
#include "../hardware/Honeywell.h"
#include "../hardware/HttpPoller.h"
#include "../hardware/I2C.h"
#include "../hardware/ICYThermostat.h"
#include "../hardware/InComfort.h"
#include "../hardware/KMTronic433.h"
#include "../hardware/KMTronicSerial.h"
#include "../hardware/KMTronicTCP.h"
#include "../hardware/KMTronicUDP.h"
#include "../hardware/Kodi.h"

#ifndef SOCKET
#define SOCKET int
#include "../hardware/Limitless.h"
#undef SOCKET
#else
#include "../hardware/Limitless.h"
#endif



#include "../hardware/LogitechMediaServer.h"
#include "../hardware/MQTT.h"
#include "../hardware/Meteostick.h"
#include "../hardware/MochadTCP.h"
#include "../hardware/MultiFun.h"
#include "../hardware/MySensorsMQTT.h"
#include "../hardware/MySensorsSerial.h"
#include "../hardware/MySensorsTCP.h"
#include "../hardware/NefitEasy.h"
#include "../hardware/Nest.h"
#include "../hardware/NestOAuthAPI.h"
#include "../hardware/Netatmo.h"
#include "../hardware/OctoPrintMQTT.h"
#include "../hardware/OTGWSerial.h"
#include "../hardware/OTGWTCP.h"
#include "../hardware/OnkyoAVTCP.h"
#include "../hardware/OpenWeatherMap.h"
#include "../hardware/OpenWebNetTCP.h"
#include "../hardware/OpenWebNetUSB.h"
#include "../hardware/PVOutput_Input.h"
#include "../hardware/PanasonicTV.h"
#include "../hardware/PhilipsHue/PhilipsHue.h"
#include "../hardware/PiFace.h"
#include "../hardware/Pinger.h"
#include "../hardware/RAVEn.h"
#include "../hardware/RFLinkSerial.h"
#include "../hardware/RFLinkTCP.h"
#include "../hardware/Rego6XXSerial.h"
#include "../hardware/RelayNet.h"
#include "../hardware/Rtl433.h"
#include "../hardware/S0MeterSerial.h"
#include "../hardware/S0MeterTCP.h"
#include "../hardware/SBFSpot.h"
#include "../hardware/SatelIntegra.h"
#include "../hardware/SolarEdgeAPI.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#include "../hardware/SolarMaxTCP.h"

#include "../hardware/Sterbox.h"
#include "../hardware/SysfsGpio.h"
#include "../hardware/TTNMQTT.h"
#include "../hardware/Tado.h"
#include "../hardware/TeleinfoBase.h"
#include "../hardware/TeleinfoSerial.h"
#include "../hardware/Tellstick.h"
#include "../hardware/Thermosmart.h"
#include "../hardware/ToonThermostat.h"
#include "../hardware/USBtin.h"
#include "../hardware/USBtin_MultiblocV8.h"
#include "../hardware/WOL.h"
#include "../hardware/Winddelen.h"
#include "../hardware/Wunderground.h"
#include "../hardware/XiaomiGateway.h"
#include "../hardware/Yeelight.h"
#include "../hardware/ZiBlueSerial.h"
#include "../hardware/ZiBlueTCP.h"
#include "../hardware/eHouseTCP.h"
#include "../hardware/plugins/Plugins.h"

// load notifications configuration
#include "../notifications/NotificationHelper.h"

#ifdef WITH_GPIO
#include "../hardware/Gpio.h"
#include "../hardware/GpioPin.h"
#endif

namespace http::server {
struct server_settings;
} // namespace http::server
namespace tcp::server {
class CTCPClientBase;
} // namespace tcp::server

MainWorker m_mainworker;

CLogger _log;
bool g_bUseSyslog = false;
bool g_bRunAsDaemon = false;

std::string szUserDataFolder = "./"; // TODO: use build dir for now
std::string szWWWFolder = ".";       // TODO: use build dir for now
bool g_bUseEventTrigger = false;
bool g_bUseUpdater = false;
http::server::_eWebCompressionMode g_wwwCompressMode =
    http::server::WWW_USE_GZIP;

std::string szAppVersion = "???";
std::string szRandomUUID = "???";
std::string szStartupFolder = "./";
std::string szWebRoot = ".";

extern "C" {
auto MD5(const unsigned char * /*d*/, size_t /*n*/, unsigned char * /*md*/)
    -> unsigned char * {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
}

CDomoticzHardwareBase::CDomoticzHardwareBase() = default;

CDomoticzHardwareBase::~CDomoticzHardwareBase() = default;

auto CDomoticzHardwareBase::CustomCommand(unsigned long /*unused*/,
                                          const std::string &
                                          /*unused*/) -> bool {
  return false;
}

void CDomoticzHardwareBase::SetHeartbeatReceived() {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDomoticzHardwareBase::Restart() -> bool {
  if (StopHardware()) {
    m_bIsStarted = StartHardware();
    return m_bIsStarted;
  }
  return false;
}

auto CDomoticzHardwareBase::SetThreadNameInt(
    const std::thread::native_handle_type &thread) -> int {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendAirQualitySensor(
    const uint8_t /*NodeID*/, const uint8_t /*ChildID*/,
    const int /*BatteryLevel*/, const int /*AirQuality*/,
    const std::string & /*defaultname*/) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendRGBWSwitch(
    const int /*NodeID*/, const uint8_t /*ChildID*/, const int /*BatteryLevel*/,
    const int /*Level*/, const bool /*bIsRGBW*/,
    const std::string & /*defaultname*/) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendPercentageSensor(
    const int /*NodeID*/, const uint8_t /*ChildID*/, const int /*BatteryLevel*/,
    const float /*Percentage*/, const std::string & /*defaultname*/) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendTempSensor(const int /*NodeID*/,
                                           const int /*BatteryLevel*/,
                                           const float /*temperature*/,
                                           const std::string & /*defaultname*/,
                                           const int /*RssiLevel*/) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendSwitch(
    const int /*NodeID*/, const uint8_t /*ChildID*/, const int /*BatteryLevel*/,
    const bool /*bOn*/, const double /*Level*/,
    const std::string & /*defaultname*/, const int /*RssiLevel*/ /* =12 */) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::Log(const _eLogLevel level,
                                const std::string &sLogline) {
  std::cerr << "Log(" << level << "): " << sLogline << std::endl;
}

void CDomoticzHardwareBase::Log(const _eLogLevel level, const char *logline,
                                ...) {
  va_list ap;
  va_start(ap, logline);
  std::array<char, 2048 * 3> buff;
  vsnprintf(buff.data(), buff.max_size(), logline, ap);
  va_end(ap);
  std::cerr << "Log(" << level << "): " << buff.data() << std::endl;
}

CSQLHelper m_sql;

auto CEvohomeBase::GetWebAPIModeName(uint8_t nControllerMode) -> const char * {
  return "foobar (Stubbed value)";
}

SMTPClient::SMTPClient() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

SMTPClient::~SMTPClient() = default;

void SMTPClient::SetTo(const std::string &To) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void SMTPClient::SetCredentials(const std::string &Username,
                                const std::string &Password) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void SMTPClient::SetServer(const std::string &Server, const int Port) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void SMTPClient::SetSubject(const std::string &Subject) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void SMTPClient::SetHTMLBody(const std::string &body) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SMTPClient::SendEmail() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void SMTPClient::SetFrom(const std::string &From) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEventSystem::CEventSystem() { m_bEnabled = false; }

CEventSystem::~CEventSystem() = default;

void CEventSystem::GetCurrentUserVariables() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::GetCurrentStates() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::GetCurrentScenesGroups() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::RemoveSingleState(const uint64_t ulDevID,
                                     const _eReason reason) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::ProcessDevice(
    const int HardwareID, const uint64_t ulDevID, const unsigned char unit,
    const unsigned char devType, const unsigned char subType,
    const unsigned char signallevel, const unsigned char batterylevel,
    const int nValue, const char *sValue, const std::string &devname) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::UpdateUserVariable(const uint64_t ulDevID,
                                      const std::string &varValue,
                                      const std::string &lastUpdate) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::SetEventTrigger(const uint64_t ulDevID,
                                   const _eReason reason,
                                   const float fDelayTime) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::TriggerURL(const std::string &result,
                              const std::vector<std::string> &headerData,
                              const std::string &callback) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEventSystem::CustomCommand(const uint64_t idx,
                                 const std::string &sCommand) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

http::server::CWebServerHelper::CWebServerHelper() { m_pDomServ = nullptr; }

http::server::CWebServerHelper::~CWebServerHelper() = default;

#ifndef NOCLOUD
http::server::CProxyManager::CProxyManager() {}

http::server::CProxyManager::~CProxyManager() {}
#endif

void http::server::CWebServer::ReloadCustomSwitchIcons() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::ReloadCustomSwitchIcons() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

#ifndef NOCLOUD
#ifdef WWW_ENABLE_SSL
#define PROXY_PORT 443
#define PROXY_SECURE true
#else
#define PROXY_PORT 80
#define PROXY_SECURE false
#endif

http::server::CProxyClient::CProxyClient() : ASyncTCP(PROXY_SECURE) {
  m_pDomServ = NULL;
}

http::server::CProxyClient::~CProxyClient() {}

void http::server::CProxyClient::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CProxyClient::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CProxyClient::OnData(const unsigned char *pData,
                                        size_t length) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CProxyClient::OnError(
    const boost::system::error_code &error) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

#endif

ASyncTCP::ASyncTCP(const bool secure)
#ifdef WWW_ENABLE_SSL
    : mSecure(secure)
#endif
{
#ifdef WWW_ENABLE_SSL
  mContext.set_verify_mode(boost::asio::ssl::verify_none);
  if (mSecure) {
    mSslSocket.reset(new boost::asio::ssl::stream<boost::asio::ip::tcp::socket>(
        mIos, mContext));
  }
#endif
}

 ASyncTCP::~ASyncTCP() = default;

http::server::CWebServerHelper m_webservers;

CNotificationHelper m_notifications;

CNotificationHelper::CNotificationHelper() {
  m_NotificationSwitchInterval = 0;
  m_NotificationSensorInterval = 12 * 3600;
}

CNotificationHelper::~CNotificationHelper() = default;

auto CNotificationHelper::SendMessage(
    const uint64_t Idx, const std::string &Name, const std::string &Subsystems,
    const std::string &Subject, const std::string &Text,
    const std::string &ExtraData, const int Priority, const std::string &Sound,
    const bool bFromNotification) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t DevRowIdx, const int HardwareID, const std::string &ID,
    const std::string &sName, const unsigned char unit,
    const unsigned char cType, const unsigned char cSubType, const int nValue)
    -> bool {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
  return true;
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t DevRowIdx, const int HardwareID, const std::string &ID,
    const std::string &sName, const unsigned char unit,
    const unsigned char cType, const unsigned char cSubType, const float fValue)
    -> bool {
  return true;
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t DevRowIdx, const int HardwareID, const std::string &ID,
    const std::string &sName, const unsigned char unit,
    const unsigned char cType, const unsigned char cSubType,
    const std::string &sValue) -> bool {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
  return true;
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t DevRowIdx, const int HardwareID, const std::string &ID,
    const std::string &sName, const unsigned char unit,
    const unsigned char cType, const unsigned char cSubType, const int nValue,
    const std::string &sValue) -> bool {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
  return true;
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t DevRowIdx, const int HardwareID, const std::string &ID,
    const std::string &sName, const unsigned char unit,
    const unsigned char cType, const unsigned char cSubType, const int nValue,
    const std::string &sValue, const float fValue) -> bool {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
  return true;
}

auto CNotificationHelper::CheckAndHandleNotification(
    const uint64_t Idx, const std::string &devicename,
    const unsigned char devType, const unsigned char subType,
    const _eNotificationTypes ntype, const float mvalue) -> bool {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
  return true;
}

auto CNotificationBase::SendMessageEx(
    const uint64_t Idx, const std::string &Name, const std::string &Subject,
    const std::string &Text, const std::string &ExtraData, const int Priority,
    const std::string &Sound, const bool bFromNotification) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNotificationHelper::SendMessageEx(
    const uint64_t Idx, const std::string &Name, const std::string &Subsystems,
    const std::string &Subject, const std::string &Text,
    const std::string &ExtraData, int Priority, const std::string &Sound,
    const bool bFromNotification) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNotificationHelper::CheckAndHandleSwitchNotification(
    const uint64_t Idx, const std::string &devicename,
    const _eNotificationTypes ntype, const int llevel) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNotificationHelper::ReloadNotifications() {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNotificationHelper::CheckAndHandleSwitchNotification(
    const uint64_t Idx, const std::string &devicename,
    const _eNotificationTypes ntype) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void _tWindCalculator::GetMMSpeedGust(int &MinSpeed, int &MaxSpeed,
                                      int &MinGust, int &MaxGust) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto IFTTT::Send_IFTTT_Trigger(const std::string &eventid,
                               const std::string &svalue1,
                               const std::string &svalue2,
                               const std::string &svalue3) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto HTTPClient::GET(const std::string &url,
                     const std::vector<std::string> &ExtraHeaders,
                     std::string &response,
                     std::vector<std::string> &vHeaderData,
                     const bool bIgnoreNoDataReturned) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto HTTPClient::POST(const std::string &url, const std::string &postdata,
                      const std::vector<std::string> &ExtraHeaders,
                      std::string &response,
                      std::vector<std::string> &vHeaderData,
                      const bool bFollowRedirect,
                      const bool bIgnoreNoDataReturned) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CCameraHandler::CCameraHandler() { m_seconds_counter = 0; }

CCameraHandler::~CCameraHandler() = default;

auto CCameraHandler::EmailCameraSnapshot(const std::string &CamIdx,
                                         const std::string &subject) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CScheduler::CScheduler() {
  m_tSunRise = 0;
  m_tSunSet = 0;
  m_tSunAtSouth = 0;
  m_tCivTwStart = 0;
  m_tCivTwEnd = 0;
  m_tNautTwStart = 0;
  m_tNautTwEnd = 0;
  m_tAstTwStart = 0;
  m_tAstTwEnd = 0;
  srand((int)mytime(nullptr));
}

CScheduler::~CScheduler() = default;

tcp::server::CTCPServer::CTCPServer() {
  m_pTCPServer = nullptr;
#ifndef NOCLOUD
  m_pProxyServer = NULL;
#endif
}

tcp::server::CTCPServer::CTCPServer(const int /*ID*/) {
  m_pTCPServer = nullptr;
#ifndef NOCLOUD
  m_pProxyServer = NULL;
#endif
}

tcp::server::CTCPServer::~CTCPServer() = default;

Arilux::Arilux(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
Arilux::~Arilux() = default;

C1Wire::C1Wire(const int ID, const int sensorThreadPeriod,
               const int switchThreadPeriod, const std::string &path)
    : m_system(nullptr),
      m_sensorThreadPeriod(sensorThreadPeriod),
      m_switchThreadPeriod(switchThreadPeriod),
      m_path(path),
      m_bSensorFirstTime(true),
      m_bSwitchFirstTime(true) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CAccuWeather::CAccuWeather(const int ID, const std::string& APIKey,
                           const std::string& Location) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CAnnaThermostat::CAnnaThermostat(const int ID, const std::string &IPAddress,
                                 const unsigned short usIPPort,
                                 const std::string &Username,
                                 const std::string &Password)
    : m_IPAddress(std::move(IPAddress)),
      m_IPPort(usIPPort),
      m_UserName(CURLEncode::URLEncode(Username)),
      m_Password(CURLEncode::URLEncode(Password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CAnnaThermostat::SetProgramState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CAnnaThermostat::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CAtagOne::CAtagOne(const int ID, const std::string &Username,
                   const std::string &Password, const int Mode1,
                   const int Mode2, const int Mode3, const int Mode4,
                   const int Mode5, const int Mode6)
    : m_UserName(std::move(Username)), m_Password(std::move(Password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CAtagOne::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CBuienRadar::CBuienRadar(int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CCameraHandler::ReloadCameras() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDaikin::CDaikin(const int ID, const std::string& IPAddress,
                 const unsigned short usIPPort, const std::string &username,
                 const std::string &password)
    : m_szIPAddress(std::move(IPAddress)),
      m_Username(CURLEncode::URLEncode(username)),
      m_Password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDarkSky::CDarkSky(const int ID, const std::string &APIKey,
                   const std::string &Location)
    : m_APIKey(std::move(APIKey)), m_Location(std::move(Location)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDavisLoggerSerial::CDavisLoggerSerial(const int ID, const std::string &devname,
                                       unsigned int baud_rate)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDenkoviDevices::CDenkoviDevices(const int ID, const std::string& IPAddress,
                                 const unsigned short usIPPort,
                                 const std::string &password,
                                 const int pollInterval, const int model)
    : m_szIPAddress(std::move(IPAddress)),
      m_Password(CURLEncode::URLEncode(password)),
      m_pollInterval(pollInterval) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDenkoviTCPDevices::CDenkoviTCPDevices(const int ID,
                                       const std::string &IPAddress,
                                       const unsigned short usIPPort,
                                       const int pollInterval, const int model,
                                       const int slaveId)
    : m_szIPAddress(std::move(IPAddress)), m_pollInterval(pollInterval) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDenkoviUSBDevices::CDenkoviUSBDevices(const int ID, const std::string &comPort,
                                       const int model)
    : m_szSerialPort(std::move(comPort)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendRainSensor(
    int, int, float,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendSolarRadiationSensor(
    unsigned char, int, float,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendTempHumSensor(
    int, int, float, int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendUVSensor(
    int, int, int, float,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDomoticzHardwareBase::SendWind(
    int, int, int, float, float, float, float, bool, bool,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDomoticzHardwareBase::Start() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDomoticzHardwareBase::Stop() -> bool {
  return true;
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CDummy::CDummy(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEcoCompteur::CEcoCompteur(const int ID, const std::string& url,
                           const unsigned short port)
    : m_url(std::move(url)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEcoDevices::CEcoDevices(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort, const std::string& username,
                         const std::string& password, const int datatimeout,
                         const int model, const int ratelimit)
    : m_szIPAddress(std::move(IPAddress)),
      m_username(std::move(username)),
      m_password(std::move(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEnOceanESP2::CEnOceanESP2(const int ID, const std::string &devname,
                           const int type) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEnOceanESP2::SendDimmerTeachIn(char const *, unsigned char) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEnOceanESP3::CEnOceanESP3(const int ID, const std::string &devname,
                           const int type) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEnOceanESP3::SendDimmerTeachIn(char const *, unsigned char) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

 CETH8020::CETH8020(const int ID, const std::string& IPAddress,
                   const unsigned short usIPPort, const std::string &username,
                   const std::string &password)
    : m_szIPAddress(std::move(IPAddress)),
      m_Username(CURLEncode::URLEncode(username)),
      m_Password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::LoadEvents() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::SetEnabled(bool) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::StartEventSystem() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::StopEventSystem() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEventSystem::UpdateSceneGroup(
    unsigned long, int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

bool CEventSystem::Update(const Notification::_eType type, const Notification::_eStatus status, const std::string &eventdata) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEventSystem::WWWUpdateSecurityState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeBase::GetControllerModeName(unsigned char) -> const char * {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeBase::GetControllerName() -> std::string {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeBase::GetZoneModeName(unsigned char) -> const char * {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeBase::GetZoneName(unsigned char) -> std::string {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeScript::CEvohomeScript(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeSerial::CEvohomeSerial(const int ID, const std::string &szSerialPort,
                               const int baudrate,
                               const std::string &UserContID)
    : CEvohomeRadio(ID, UserContID) {
  if (baudrate != 0) {
    m_iBaudRate = baudrate;
  } else {
    // allow migration of hardware created before baud rate was configurable
    m_iBaudRate = 115200;
  }
  m_szSerialPort = szSerialPort;

  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEvohomeSerial::Do_Work() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeSerial::OpenSerialDevice() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEvohomeSerial::ReadCallback(const char *data, size_t len) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEvohomeSerial::Do_Send(std::string str) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeTCP::CEvohomeTCP(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort,
                         const std::string &UserContID)
    :

      CEvohomeRadio(ID, UserContID),
      m_szIPAddress(std::move(IPAddress)),
      m_usIPPort(usIPPort) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CEvohomeTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CEvohomeTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CEvohomeTCP::Do_Work() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CEvohomeTCP::OnData(const unsigned char *pData, size_t length) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CEvohomeTCP::Do_Send(std::string str) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEvohomeTCP::OnError(const boost::system::error_code &error) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeWeb::CEvohomeWeb(const int ID, const std::string& Username,
                         const std::string& Password, const unsigned int refreshrate,
                         const int UseFlags, const unsigned int installation)
    : m_username(std::move(Username)),
      m_password(std::move(Password)),
      m_refreshrate(refreshrate) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CFibaroPush::CFibaroPush() = default;

void CFibaroPush::Start() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CFibaroPush::Stop() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CGooglePubSubPush::CGooglePubSubPush() = default;

void CGooglePubSubPush::Start() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CGooglePubSubPush::Stop() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHardwareMonitor::CHardwareMonitor(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHarmonyHub::CHarmonyHub(const int ID, const std::string& IPAddress,
                         const unsigned int port)
    : m_szHarmonyAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHoneywell::CHoneywell(
    int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}


CHttpPush::CHttpPush() = default;

void CHttpPush::Start() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CHttpPush::Stop() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CICYThermostat::CICYThermostat(const int ID, const std::string& Username,
                               const std::string& Password)
    : m_UserName(std::move(Username)), m_Password(std::move(Password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CICYThermostat::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CInComfort::CInComfort(const int ID, const std::string& IPAddress, unsigned short) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CInComfort::SetProgramState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CInComfort::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CInfluxPush::CInfluxPush() = default;

auto CInfluxPush::Start() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CInfluxPush::Stop() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CKodi::CKodi(int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CLimitLess::CLimitLess(const int ID, const int LedType, const int BridgeType,
                       const std::string& IPAddress, const unsigned short usIPPort) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CLogitechMediaServer::CLogitechMediaServer(const int ID,
                                           const std::string &IPAddress,
                                           const int Port,
                                           const std::string &User,
                                           const std::string &Pwd,
                                           const int PollIntervalsec) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNefitEasy::CNefitEasy(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNefitEasy::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNest::CNest(int,
             std::__cxx11::basic_string<char, std::char_traits<char>,
                                        std::allocator<char>> const &,
             std::__cxx11::basic_string<char, std::char_traits<char>,
                                        std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNestOAuthAPI::CNestOAuthAPI(const int ID, const std::string& apikey,
                             const std::string &extradata) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNestOAuthAPI::SetProgramState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNestOAuthAPI::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNest::SetProgramState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

 void CNest::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNetatmo::CNetatmo(const int ID, const std::string &username,
                   const std::string &password)
    : m_clientId("5588029e485a88af28f4a3c4"),
      m_clientSecret("6vIpQVjNsL2A74Bd8tINscklLw2LKv7NhE9uW2"),
      m_username(CURLEncode::URLEncode(username)),
      m_password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNetatmo::SetProgramState(int, int) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNetatmo::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNotificationHelper::CheckAndHandleLastUpdateNotification() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CNotificationHelper::Init() { ReloadNotifications(); }

void CNotificationHelper::LoadConfig() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

Comm5Serial::Comm5Serial(const int ID, const std::string& devname,
                         unsigned int baudRate /*= 115200*/)
    : m_szSerialPort(std::move(devname)), m_baudRate(baudRate) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Comm5Serial::WriteToHardware(const char *pdata,
                                  const unsigned char /*length*/) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
auto Comm5Serial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
auto Comm5Serial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

Comm5SMTCP::Comm5SMTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  m_HwdID = ID;
  m_usIPPort = usIPPort;
  initSensorData = true;
  m_bReceiverStarted = false;
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Comm5SMTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
auto Comm5SMTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::Do_Work() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::ParseData(const unsigned char *data, const size_t len) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::querySensorState() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
auto Comm5SMTCP::WriteToHardware(const char * /*pdata*/,
                                 const unsigned char /*length*/) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5SMTCP::OnError(const boost::system::error_code &error) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Comm5SMTCP::OnData(const unsigned char *pData, size_t length) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}


COpenWebNetTCP::COpenWebNetTCP(const int ID, const std::string &IPAddress,
                               const unsigned short usIPPort,
                               const std::string &ownPassword,
                               const int ownScanTime)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetTCP::SetSetpoint(int, float) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

COpenWebNetUSB::COpenWebNetUSB(
    int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    unsigned int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CPanasonic::CPanasonic(int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CPhilipsHue::CPhilipsHue(const int ID, const std::string &IPAddress,
                         const unsigned short Port, const std::string &Username,
                         const int PollInterval, const int Options)
    : m_IPAddress(std::move(IPAddress)), m_UserName(std::move(Username)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CPiFace::CPiFace(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CPinger::CPinger(int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CPVOutputInput::CPVOutputInput(const int ID, const std::string& SID, const std::string& Key)
    : m_SID(std::move(SID)), m_KEY(std::move(Key)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRego6XXSerial::CRego6XXSerial(const int ID, const std::string& devname,
                               const int type)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRFLinkSerial::CRFLinkSerial(const int ID, const std::string& devname)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRFLinkTCP::CRFLinkTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFXBase::Parse_Async_Data(unsigned char const *, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFXBase::SendResetCommand() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFXBase::SetAsyncType(CRFXBase::_eRFXAsyncType) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRtl433::CRtl433(const int ID, const std::string& cmdline)
    : m_cmdline(std::move(cmdline)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CSBFSpot::CSBFSpot(int,
                   std::__cxx11::basic_string<char, std::char_traits<char>,
                                              std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CScheduler::ReloadSchedules() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CScheduler::SetSunRiseSetTimers(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CScheduler::StartScheduler() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CScheduler::StopScheduler() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}


CSysfsGpio::CSysfsGpio(int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CTado::CTado(const int ID, const std::string& username, const std::string& password)
    : m_TadoUsername(std::move(username)), m_TadoPassword(std::move(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTado::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CTeleinfoSerial::CTeleinfoSerial(
    int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int, unsigned int, bool, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTellstick::Create(CTellstick **, int, int, int) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CThermosmart::CThermosmart(
    int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    int, int, int, int, int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CThermosmart::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CToonThermostat::CToonThermostat(const int ID, const std::string& Username,
                                 const std::string& Password, const int &Agreement)
    : m_UserName(std::move(Username)),
      m_Password(std::move(Password)),
      m_Agreement(Agreement) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CToonThermostat::SetProgramState(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CToonThermostat::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CTTNMQTT::CTTNMQTT(const int ID, const std::string& IPAddress,
                   const unsigned short usIPPort, const std::string& Username,
                   const std::string& Password, const std::string& CAfilename)
    : mosqdz::mosquittodz((std::string("Domoticz-TTN") + szRandomUUID).c_str()),
      m_szIPAddress(std::move(IPAddress)),
      m_UserName(std::move(Username)),
      m_Password(std::move(Password)),
      m_CAFilename(std::move(CAfilename)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CurrentCostMeterSerial::CurrentCostMeterSerial(const int ID,
                                               const std::string& devname,
                                               unsigned int baudRate)
    : m_szSerialPort(std::move(devname)), m_baudRate(baudRate) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CurrentCostMeterTCP::CurrentCostMeterTCP(const int ID, const std::string& IPAddress,
                                         const unsigned short usIPPort)
    : m_retrycntr(RETRY_DELAY),
      m_szIPAddress(std::move(IPAddress)),
      m_usIPPort(usIPPort),
      m_socket(INVALID_SOCKET) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CWinddelen::CWinddelen(const int ID, const std::string& IPAddress,
                       const unsigned short usTotParts,
                       const unsigned short usMillID)
    : m_szMillName(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CWOL::CWOL(const int ID, const std::string& BroadcastAddress,
           const unsigned short Port)
    : m_broadcast_address(std::move(BroadcastAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CWunderground::CWunderground(const int ID, const std::string& APIKey,
                             const std::string& Location)
    :
      m_bForceSingleStation(false),
      m_bFirstTime(true),
      m_APIKey(std::move(APIKey)),
      m_Location(std::move(Location))
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CYouLess::CYouLess(const int ID, const std::string& IPAddress,
                   const unsigned short usIPPort, const std::string &password)
    : m_szIPAddress(std::move(IPAddress)),
      m_Password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}


CZiBlueTCP::CZiBlueTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

DomoticzInternal::DomoticzInternal(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

DomoticzTCP::DomoticzTCP(const int ID, const std::string &IPAddress,
                         const unsigned short usIPPort,
                         const std::string &username,
                         const std::string &password)
    : m_szIPAddress(std::move(IPAddress)),
      m_username(std::move(username)),
      m_password(std::move(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

Ec3kMeterTCP::Ec3kMeterTCP(const int ID, const std::string& IPAddress,
                           const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
eHouseTCP::eHouseTCP(const int ID, const std::string& IPAddress,
                     const unsigned short IPPort, const std::string &userCode,
                     const int pollInterval, const unsigned char AutoDiscovery,
                     const unsigned char EnableAlarms,
                     const unsigned char EnablePro, const int opta,
                     const int optb)
    : m_modelIndex(-1),
      m_data32(false),
      m_socket(INVALID_SOCKET),
      m_IPPort(IPPort),
      m_IPAddress(std::move(IPAddress)),
      m_pollInterval(pollInterval) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

EnphaseAPI::EnphaseAPI(const int ID, const std::string& IPAddress,
                       const unsigned short /*usIPPort*/)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

FritzboxTCP::FritzboxTCP(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

GoodweAPI::GoodweAPI(const int ID, const std::string& userName,
                     const int ServerLocation)
    : m_UserName(std::move(userName)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

GoodweAPI::~GoodweAPI() = default;

void HTTPClient::Cleanup() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto HTTPClient::GET(std::__cxx11::basic_string<char, std::char_traits<char>,
                                                std::allocator<char>> const &,
                     std::__cxx11::basic_string<char, std::char_traits<char>,
                                                std::allocator<char>> &,
                     bool) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void HTTPClient::SetUserAgent(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::ClearUserPasswords() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::SetAuthenticationMethod(
    http::server::_eAuthenticationMethod) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::SetWebCompressionMode(
    http::server::_eWebCompressionMode) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::SetWebRoot(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void http::server::CWebServerHelper::SetWebTheme(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

#ifdef WWW_ENABLE_SSL
bool http::server::CWebServerHelper::StartServers(
    http::server::server_settings &, http::server::ssl_server_settings &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    bool, tcp::server::CTCPServer *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
#else
auto http::server::CWebServerHelper::StartServers(
    server_settings &web_settings, const std::string &serverpath,
    const bool bIgnoreUsernamePassword, tcp::server::CTCPServer *sharedServer)
    -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
#endif

void http::server::CWebServerHelper::StopServers() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

I2C::I2C(const int ID, const _eI2CType DevType, const std::string &Address,
         const std::string &SerialPort, const int Mode1)
    : m_dev_type(DevType),
      m_i2c_addr((uint8_t)atoi(Address.c_str())),
      m_ActI2CBus(std::move(SerialPort)),
      m_invert_data((bool)Mode1) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

KMTronic433::KMTronic433(const int ID, const std::string &devname) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

KMTronicSerial::KMTronicSerial(const int ID, const std::string &devname) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

KMTronicTCP::KMTronicTCP(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort,
                         const std::string &username,
                         const std::string &password)
    : m_szIPAddress(std::move(IPAddress)),
      m_Username(CURLEncode::URLEncode(username)),
      m_Password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

KMTronicTCP::~KMTronicTCP() = default;

KMTronicUDP::KMTronicUDP(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

Meteostick::Meteostick(const int ID, const std::string& devname,
                       const unsigned int baud_rate)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MochadTCP::MochadTCP(const int ID, const std::string& IPAddress,
                     const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MQTT::MQTT(const int ID, const std::string& IPAddress, const unsigned short usIPPort,
           const std::string& Username, const std::string& Password, const std::string& CAfilename,
           const int TLS_Version, const int Topics,
           const std::string &MQTTClientID)
    :
      mosqdz::mosquittodz(MQTTClientID.c_str()) ,
      m_szIPAddress(std::move(IPAddress)),
      m_UserName(std::move(Username)),
      m_Password(std::move(Password)),
      m_CAFilename(std::move(CAfilename))
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MultiFun::MultiFun(const int ID, const std::string& IPAddress,
                   const unsigned short IPPort)
    : m_IPPort(IPPort),
      m_IPAddress(std::move(IPAddress)),
      m_socket(nullptr),
      m_LastAlarms(0),
      m_LastWarnings(0),
      m_LastDevices(0),
      m_LastState(0),
      m_LastQuickAccess(0) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsBase::SendTextSensorValue(const int nodeID, const int childID,
                                        const std::string &tvalue) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

#define TOPIC_OUT "domoticz/out"
#define TOPIC_IN "domoticz/in"
MySensorsMQTT::MySensorsMQTT(const int ID, const std::string &Name,
                             const std::string &IPAddress,
                             const unsigned short usIPPort,
                             const std::string &Username,
                             const std::string &Password,
                             const std::string &CAfilename,
                             const int TLS_Version, const int Topics)
    : MQTT(ID, IPAddress, usIPPort, Username, Password, CAfilename, TLS_Version,
           (int)MQTT::PT_out,
           (std::string("Domoticz-MySensors") + szRandomUUID).c_str()),
      MyTopicIn(TOPIC_IN),
      MyTopicOut(TOPIC_OUT) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MySensorsSerial::MySensorsSerial(const int ID, const std::string &devname,
                                 const int Mode1)
    : m_retrycntr(RETRY_DELAY) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MySensorsTCP::MySensorsTCP(const int ID, const std::string& IPAddress,
                           const unsigned short usIPPort)
    :
      m_retrycntr(RETRY_DELAY),
      m_szIPAddress(std::move(IPAddress)),
      m_usIPPort(usIPPort)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

OnkyoAVTCP::OnkyoAVTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OTGWBase::SetSetpoint(int, float) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

OTGWSerial::OTGWSerial(const int ID, const std::string &devname,
                       const unsigned int baud_rate, const int Mode1,
                       const int Mode2, const int Mode3, const int Mode4,
                       const int Mode5, const int Mode6) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

OTGWTCP::OTGWTCP(const int ID, const std::string& IPAddress,
                 const unsigned short usIPPort, const int Mode1,
                 const int Mode2, const int Mode3, const int Mode4,
                 const int Mode5, const int Mode6)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

P1MeterTCP::P1MeterTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort, const bool disable_crc,
                       const int ratelimit)
    : m_szIPAddress(std::move(IPAddress)), m_usIPPort(usIPPort) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

RAVEn::RAVEn(const int ID, const std::string& devname)
    : device_(std::move(devname)),
      m_wptr(m_buffer),
      m_currUsage(0),
      m_totalUsage(0) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

RelayNet::RelayNet(const int ID, const std::string& IPAddress,
                   const unsigned short usIPPort, const std::string &username,
                   const std::string &password, const bool pollInputs,
                   const bool pollRelays, const int pollInterval,
                   const int inputCount, const int relayCount)
    : m_szIPAddress(std::move(IPAddress)),
      m_username(CURLEncode::URLEncode(username)),
      m_password(CURLEncode::URLEncode(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

RFXComSerial::RFXComSerial(const int ID, const std::string& devname,
                           unsigned int baud_rate,
                           const _eRFXAsyncType AsyncType)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

RFXComTCP::RFXComTCP(const int ID, const std::string& IPAddress,
                     const unsigned short usIPPort,
                     const _eRFXAsyncType AsyncType)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

S0MeterSerial::S0MeterSerial(
    int,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    unsigned int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

S0MeterTCP::S0MeterTCP(const int ID, const std::string& IPAddress,
                       const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

SolarEdgeAPI::SolarEdgeAPI(const int ID, const std::string& APIKey)
    : m_APIKey(std::move(APIKey)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

SolarMaxTCP::SolarMaxTCP(const int ID, const std::string& IPAddress,
                         const unsigned short usIPPort)
    : m_szIPAddress(std::move(IPAddress)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SunRiseSet::GetSunRiseSet(double, double, int, int, int,
                               SunRiseSet::_tSubRiseSetResults &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void tcp::server::CTCPServer::SendToAll(int, unsigned long, char const *,
                                        unsigned long,
                                        tcp::server::CTCPClientBase const *) {
  // using namespace std::literals;
  // throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void tcp::server::CTCPServer::SetRemoteUsers(
    std::vector<tcp::server::_tRemoteShareUser,
                std::allocator<tcp::server::_tRemoteShareUser>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto tcp::server::CTCPServer::StartServer(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &,
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void tcp::server::CTCPServer::stopAllClients() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void tcp::server::CTCPServer::StopServer() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto _tWindCalculator::AddValueAndReturnAvarage(double) -> double {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void _tWindCalculator::SetSpeedGust(int, int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

_tWindCalculator::_tWindCalculator() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

USBtin::USBtin(const int ID, const std::string& devname, unsigned int BusCanType,
               unsigned int DebugMode)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

XiaomiGateway::XiaomiGateway(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

Yeelight::Yeelight(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CBaroForecastCalculator::~CBaroForecastCalculator() = default;

C1Wire::~C1Wire() = default;
CAccuWeather::~CAccuWeather() = default;
CAnnaThermostat::~CAnnaThermostat() = default;
CAtagOne::~CAtagOne() = default;
CBuienRadar::~CBuienRadar() = default;
CDaikin::~CDaikin() = default;
CDarkSky::~CDarkSky() = default;
CDavisLoggerSerial::~CDavisLoggerSerial() = default;
CDenkoviDevices::~CDenkoviDevices() = default;
CDenkoviTCPDevices::~CDenkoviTCPDevices() = default;
CDenkoviUSBDevices::~CDenkoviUSBDevices() = default;
CDummy::~CDummy() = default;
CEcoCompteur::~CEcoCompteur() = default;
CEcoDevices::~CEcoDevices() = default;
CEnOceanESP2::~CEnOceanESP2() = default;
CEnOceanESP3::~CEnOceanESP3() = default;
CETH8020::~CETH8020() = default;
CEvohomeScript::~CEvohomeScript() = default;
CEvohomeWeb::~CEvohomeWeb() = default;
CHardwareMonitor::~CHardwareMonitor() = default;
CHarmonyHub::~CHarmonyHub() = default;
CHoneywell::~CHoneywell() = default;
CICYThermostat::~CICYThermostat() = default;
CInComfort::~CInComfort() = default;
CKodi::~CKodi() = default;
CLimitLess::~CLimitLess() = default;
CLogitechMediaServer::~CLogitechMediaServer() = default;
CNefitEasy::~CNefitEasy() = default;
CNest::~CNest() = default;
CNestOAuthAPI::~CNestOAuthAPI() = default;
CNetatmo::~CNetatmo() = default;
COpenWebNetTCP::~COpenWebNetTCP() = default;
COpenWebNetUSB::~COpenWebNetUSB() = default;
CPanasonic::~CPanasonic() = default;
CPhilipsHue::~CPhilipsHue() = default;
CPiFace::~CPiFace() = default;
CPinger::~CPinger() = default;
CPVOutputInput::~CPVOutputInput() = default;
CRego6XXSerial::~CRego6XXSerial() = default;
CRFLinkSerial::~CRFLinkSerial() = default;
CRFLinkTCP::~CRFLinkTCP() = default;
CRtl433::~CRtl433() = default;
CSBFSpot::~CSBFSpot() = default;
CSysfsGpio::~CSysfsGpio() = default;
CTado::~CTado() = default;
CTeleinfoSerial::~CTeleinfoSerial() = default;
CThermosmart::~CThermosmart() = default;
CToonThermostat::~CToonThermostat() = default;
CTTNMQTT::~CTTNMQTT() = default;
CurrentCostMeterSerial::~CurrentCostMeterSerial() = default;
CurrentCostMeterTCP::~CurrentCostMeterTCP() = default;
CWinddelen::~CWinddelen() = default;
CWOL::~CWOL() = default;
CWunderground::~CWunderground() = default;
CYouLess::~CYouLess() = default;
CZiBlueTCP::~CZiBlueTCP() = default;
DomoticzInternal::~DomoticzInternal() = default;
DomoticzTCP::~DomoticzTCP() = default;
Ec3kMeterTCP::~Ec3kMeterTCP() = default;
eHouseTCP::~eHouseTCP() = default;
EnphaseAPI::~EnphaseAPI() = default;
FritzboxTCP::~FritzboxTCP() = default;
I2C::~I2C() = default;
KMTronic433::~KMTronic433() = default;
KMTronicSerial::~KMTronicSerial() = default;
KMTronicUDP::~KMTronicUDP() = default;
Meteostick::~Meteostick() = default;
MochadTCP::~MochadTCP() = default;
MQTT::~MQTT() = default;
MultiFun::~MultiFun() = default;
MySensorsMQTT::~MySensorsMQTT() = default;
MySensorsSerial::~MySensorsSerial() = default;
MySensorsTCP::~MySensorsTCP() = default;
OnkyoAVTCP::~OnkyoAVTCP() = default;
OTGWSerial::~OTGWSerial() = default;
OTGWTCP::~OTGWTCP() = default;
P1MeterSerial::~P1MeterSerial() = default;
P1MeterTCP::~P1MeterTCP() = default;
RAVEn::~RAVEn() = default;
RelayNet::~RelayNet() = default;
RFXComSerial::~RFXComSerial() = default;
RFXComTCP::~RFXComTCP() = default;
S0MeterSerial::~S0MeterSerial() = default;
S0MeterTCP::~S0MeterTCP() = default;
SatelIntegra::~SatelIntegra() = default;
SolarEdgeAPI::~SolarEdgeAPI() = default;
SolarMaxTCP::~SolarMaxTCP() = default;
USBtin::~USBtin() = default;
XiaomiGateway::~XiaomiGateway() = default;
Yeelight::~Yeelight() = default;

auto Arilux::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Arilux::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Arilux::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

#include "../hardware/ASyncSerial.cpp"

auto C1Wire::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto C1Wire::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto C1Wire::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAccuWeather::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAccuWeather::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAccuWeather::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAnnaThermostat::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAnnaThermostat::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAnnaThermostat::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAtagOne::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAtagOne::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CAtagOne::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CBasePush::CBasePush() = default;

auto CBuienRadar::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CBuienRadar::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CBuienRadar::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDaikin::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDaikin::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDaikin::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDarkSky::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDarkSky::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDarkSky::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDavisLoggerSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDavisLoggerSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDavisLoggerSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviDevices::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviDevices::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviDevices::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDenkoviTCPDevices::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDenkoviTCPDevices::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDenkoviTCPDevices::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CDenkoviTCPDevices::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviTCPDevices::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviTCPDevices::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviTCPDevices::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviUSBDevices::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviUSBDevices::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDenkoviUSBDevices::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDummy::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDummy::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CDummy::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoCompteur::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoCompteur::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoCompteur::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoDevices::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoDevices::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEcoDevices::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP2::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP2::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP2::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP3::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP3::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEnOceanESP3::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CETH8020::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CETH8020::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CETH8020::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeBase::~CEvohomeBase() = default;

CEvohomeBase::CEvohomeBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeRadio::~CEvohomeRadio() = default;
auto CEvohomeRadio::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CEvohomeRadio::Idle_Work() {}

void CEvohomeRadio::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeRadio::WriteToHardware(const char *pdata,
                                    const unsigned char length) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CEvohomeRadio::CEvohomeRadio(
    int, std::__cxx11::basic_string<char, std::char_traits<char>,
                                    std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeScript::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeScript::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeScript::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeWeb::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeWeb::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CEvohomeWeb::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHardwareMonitor::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHardwareMonitor::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHarmonyHub::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHarmonyHub::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHarmonyHub::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHEOS::CHEOS(const int ID, const std::string &IPAddress,
             const unsigned short usIPPort, const std::string &User,
             const std::string &Pwd, const int PollIntervalsec,
             const int PingTimeoutms)
    : m_IP(std::move(IPAddress)),
      m_User(std::move(User)),
      m_Pwd(std::move(Pwd)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHEOS::~CHEOS() = default;

void CHEOS::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CHEOS::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CHEOS::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CHEOS::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHEOS::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHEOS::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHEOS::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHoneywell::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHoneywell::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHoneywell::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHttpPoller::CHttpPoller(const int ID, const std::string &username,
                         const std::string &password, const std::string &url,
                         const std::string &extradata,
                         const unsigned short refresh)
    : m_username(CURLEncode::URLEncode(username)),
      m_password(CURLEncode::URLEncode(password)),
      m_url(std::move(url)),
      m_refresh(refresh) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CHttpPoller::~CHttpPoller() = default;

auto CHttpPoller::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHttpPoller::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CHttpPoller::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CICYThermostat::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CICYThermostat::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CICYThermostat::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CInComfort::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CInComfort::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CInComfort::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CIOPort::~CIOPort() = default;

CIOPort::CIOPort() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CKodi::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CKodi::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CKodi::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLimitLess::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLimitLess::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLimitLess::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLogitechMediaServer::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLogitechMediaServer::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CLogitechMediaServer::WriteToHardware(char const *, unsigned char)
    -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNefitEasy::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNefitEasy::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNefitEasy::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNestOAuthAPI::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNestOAuthAPI::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNestOAuthAPI::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNest::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNest::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNest::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNetatmo::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNetatmo::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CNetatmo::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

COpenWeatherMap::COpenWeatherMap(const int ID, const std::string& APIKey,
                                 const std::string& Location)
    : m_APIKey(std::move(APIKey)),
      m_Location(std::move(Location)),
      m_Language("en") {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

COpenWeatherMap::~COpenWeatherMap() = default;

auto COpenWeatherMap::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWeatherMap::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWeatherMap::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetUSB::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetUSB::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto COpenWebNetUSB::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPanasonic::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPanasonic::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPanasonic::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPhilipsHue::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPhilipsHue::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPhilipsHue::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPiFace::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPiFace::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPiFace::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPinger::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPinger::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPinger::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPVOutputInput::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPVOutputInput::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CPVOutputInput::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRego6XXSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRego6XXSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRego6XXSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRFLinkBase::~CRFLinkBase() = default;

CRFLinkBase::CRFLinkBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkBase::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkSerial::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFLinkTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFLinkTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFLinkTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CRFLinkTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRFLinkTCP::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CRFXBase::~CRFXBase() = default;

CRFXBase::CRFXBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRtl433::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRtl433::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CRtl433::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSBFSpot::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSBFSpot::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSBFSpot::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CSterbox::CSterbox(const int ID, const std::string& IPAddress,
                   const unsigned short usIPPort, const std::string& username,
                   const std::string& password)
    : m_szIPAddress(std::move(IPAddress)),
      m_Username(std::move(username)),
      m_Password(std::move(password)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CSterbox::~CSterbox() = default;

auto CSterbox::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSterbox::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSterbox::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSysfsGpio::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSysfsGpio::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CSysfsGpio::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTado::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTado::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTado::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CTeleinfoBase::~CTeleinfoBase() = default;

CTeleinfoBase::CTeleinfoBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTeleinfoSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTeleinfoSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTeleinfoSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CThermosmart::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CThermosmart::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CThermosmart::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CToonThermostat::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CToonThermostat::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CToonThermostat::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::on_connect(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::on_disconnect(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::on_message(mosquitto_message const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::on_subscribe(int, int, int const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::SendHeartbeat() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTTNMQTT::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CTTNMQTT::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CTTNMQTT::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CurrentCostMeterBase::~CurrentCostMeterBase() = default;

CurrentCostMeterBase::CurrentCostMeterBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterSerial::WriteToHardware(char const *, unsigned char)
    -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CurrentCostMeterTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWinddelen::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWinddelen::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWinddelen::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWOL::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWOL::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWOL::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWunderground::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWunderground::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CWunderground::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CYouLess::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CYouLess::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CYouLess::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CZiBlueBase::CZiBlueBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CZiBlueBase::~CZiBlueBase() = default;

auto CZiBlueBase::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CZiBlueSerial::CZiBlueSerial(const int ID, const std::string& devname)
    : m_szSerialPort(std::move(devname)) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CZiBlueSerial::~CZiBlueSerial() = default;

auto CZiBlueSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueSerial::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueSerial::WriteInt(unsigned char const *, unsigned long) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CZiBlueTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CZiBlueTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CZiBlueTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void CZiBlueTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueTCP::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto CZiBlueTCP::WriteInt(const uint8_t *pData, const size_t length) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void DomoticzTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void DomoticzTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void DomoticzTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void DomoticzTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto DomoticzTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto DomoticzTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto DomoticzTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Ec3kMeterTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Ec3kMeterTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Ec3kMeterTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Ec3kMeterTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Ec3kMeterTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Ec3kMeterTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Ec3kMeterTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto eHouseTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto eHouseTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto eHouseTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto EnphaseAPI::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto EnphaseAPI::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto EnphaseAPI::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void FritzboxTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void FritzboxTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void FritzboxTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void FritzboxTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto FritzboxTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto FritzboxTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto FritzboxTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto GoodweAPI::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto GoodweAPI::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto GoodweAPI::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto I2C::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto I2C::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto I2C::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronic433::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronic433::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronic433::WriteInt(unsigned char const *, unsigned long, bool) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

KMTronicBase::~KMTronicBase() = default;

KMTronicBase::KMTronicBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicBase::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicSerial::WriteInt(unsigned char const *, unsigned long, bool)
    -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicTCP::WriteInt(unsigned char const *, unsigned long, bool) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicUDP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicUDP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicUDP::WriteInt(unsigned char const *, unsigned long, bool) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto KMTronicUDP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Meteostick::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Meteostick::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Meteostick::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MochadTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MochadTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MochadTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MochadTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MochadTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MochadTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MochadTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

mosqdz::mosquittodz::~mosquittodz() = default;

mosqdz::mosquittodz::mosquittodz(char const *, bool) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_connect(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_disconnect(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_error() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_log(int, char const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_message(mosquitto_message const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::on_subscribe(int, int, int const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::SendHeartbeat() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MQTT::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MQTT::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MQTT::WriteInt(std::__cxx11::basic_string<char, std::char_traits<char>,
                                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MultiFun::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MultiFun::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MultiFun::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

MySensorsBase::~MySensorsBase() = default;

MySensorsBase::MySensorsBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsBase::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsMQTT::on_connect(int) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsMQTT::on_message(mosquitto_message const *) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsMQTT::SendHeartbeat() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsMQTT::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsMQTT::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsMQTT::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsSerial::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto MySensorsTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void MySensorsTCP::WriteInt(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OnkyoAVTCP::CustomCommand(
    unsigned long, std::__cxx11::basic_string<char, std::char_traits<char>,
                                              std::allocator<char>> const &)
    -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OnkyoAVTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OnkyoAVTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OnkyoAVTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OnkyoAVTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OnkyoAVTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OnkyoAVTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OnkyoAVTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

OTGWBase::~OTGWBase() = default;

OTGWBase::OTGWBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWBase::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWSerial::WriteInt(unsigned char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OTGWTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OTGWTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OTGWTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void OTGWTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto OTGWTCP::WriteInt(unsigned char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

P1MeterBase::~P1MeterBase() = default;

P1MeterBase::P1MeterBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

P1MeterSerial::P1MeterSerial(const int ID, const std::string& devname,
                             const unsigned int baud_rate,
                             const bool disable_crc, const int ratelimit)
    : m_szSerialPort(std::move(devname)) {
  m_HwdID = ID;
  m_iBaudRate = baud_rate;
  m_bDisableCRC = disable_crc;
  m_ratelimit = ratelimit;
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

P1MeterSerial::P1MeterSerial(
    const std::string &devname, unsigned int baud_rate,
    boost::asio::serial_port_base::parity opt_parity,
    boost::asio::serial_port_base::character_size opt_csize,
    boost::asio::serial_port_base::flow_control opt_flow,
    boost::asio::serial_port_base::stop_bits opt_stop)
    : AsyncSerial(devname, baud_rate, opt_parity, opt_csize, opt_flow,
                  opt_stop),
      m_iBaudRate(baud_rate) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void P1MeterTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void P1MeterTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void P1MeterTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void P1MeterTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto P1MeterTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RAVEn::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RAVEn::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RAVEn::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RelayNet::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RelayNet::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RelayNet::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RelayNet::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RelayNet::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RelayNet::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RelayNet::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RFXComTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RFXComTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RFXComTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void RFXComTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto RFXComTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

S0MeterBase::~S0MeterBase() = default;

S0MeterBase::S0MeterBase() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterSerial::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterSerial::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterSerial::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void S0MeterTCP::OnConnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void S0MeterTCP::OnData(unsigned char const *, unsigned long) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void S0MeterTCP::OnDisconnect() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void S0MeterTCP::OnError(boost::system::error_code const &) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto S0MeterTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

SatelIntegra::SatelIntegra(const int ID, const std::string& IPAddress,
                           const unsigned short IPPort,
                           const std::string &userCode, const int pollInterval)
    : m_modelIndex(-1),
      m_data32(false),
      m_socket(INVALID_SOCKET),
      m_IPPort(IPPort),
      m_IPAddress(std::move(IPAddress)),
      m_pollInterval(pollInterval) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SatelIntegra::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SatelIntegra::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SatelIntegra::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

serial::Serial::~Serial() = default;

serial::Serial::Serial(std::__cxx11::basic_string<char, std::char_traits<char>,
                                                  std::allocator<char>> const &,
                       unsigned int, serial::Timeout, serial::bytesize_t,
                       serial::parity_t, serial::stopbits_t,
                       serial::flowcontrol_t) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarEdgeAPI::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarEdgeAPI::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarEdgeAPI::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarMaxTCP::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarMaxTCP::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto SolarMaxTCP::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

USBtin_MultiblocV8::~USBtin_MultiblocV8() = default;

USBtin_MultiblocV8::USBtin_MultiblocV8() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto USBtin_MultiblocV8::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto USBtin::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto USBtin::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto USBtin::writeFrame(
    std::__cxx11::basic_string<char, std::char_traits<char>,
                               std::allocator<char>> const &) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto XiaomiGateway::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto XiaomiGateway::StopHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto XiaomiGateway::WriteToHardware(char const *, unsigned char) -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Yeelight::StartHardware() -> bool {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Yeelight::StopHardware() -> bool {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Yeelight::WriteToHardware(char const *, unsigned char) -> bool {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CIOPinState::CIOPinState() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
};

CIOPinState::~CIOPinState() = default;
;

CIOCount::CIOCount() = default;

CIOCount::~CIOCount() = default;

Comm5TCP::Comm5TCP(int, const std::string&, unsigned short) {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Comm5TCP::StartHardware() -> bool {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Comm5TCP::StopHardware() -> bool {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

auto Comm5TCP::WriteToHardware(const char *, const unsigned char /*length*/)
    -> bool {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void Comm5TCP::Do_Work() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::enableNotifications() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::OnConnect() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::OnData(const unsigned char *, size_t) {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::OnDisconnect() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::OnError(const boost::system::error_code &) {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::ParseData(const unsigned char *, const size_t) {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::processSensorData(const std::string &) {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void Comm5TCP::queryRelayState() {
      using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNotificationObserver::CNotificationObserver() {}
CNotificationObserver::~CNotificationObserver() {}
CNotificationSystem::CNotificationSystem() {}

void CNotificationSystem::Start() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
void CNotificationSystem::Stop() {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
bool CNotificationSystem::NotifyWait(const Notification::_eType type,
                                     const Notification::_eStatus status,
                                     const std::string &eventdata) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

CNotificationSystem::~CNotificationSystem() {}

void CNotificationSystem::Notify(const Notification::_eType type,
                                 const Notification::_eStatus status,
                                 const std::string &eventdata) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

COctoPrintMQTT::COctoPrintMQTT(const int ID, const std::string &IPAddress,
                               const unsigned short usIPPort,
                               const std::string &Username,
                               const std::string &Password,
                               const std::string &CAfilename) {
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

COctoPrintMQTT::~COctoPrintMQTT() {}

bool COctoPrintMQTT::StartHardware()
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

bool COctoPrintMQTT::StopHardware()
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::WriteInt(const std::string &sendStr)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::on_connect(int rc)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::on_disconnect(int rc)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::on_message(const struct mosquitto_message *message)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}

void COctoPrintMQTT::SendHeartbeat()
{
  using namespace std::literals;
  throw std::runtime_error("stubbed: "s + BOOST_CURRENT_FUNCTION);
}
