#pragma once

#include <string>
#include "../webserver/cWebem.h"
#include "../webserver/request.hpp"
#include "../webserver/session_store.hpp"
#include "../iamserver/iam_settings.hpp"

struct lua_State;
struct lua_Debug;

namespace Json
{
	class Value;
} // namespace Json

namespace http {
	namespace server {
		class cWebem;
		struct _tWebUserPassword;
class CWebServer : public session_store, public std::enable_shared_from_this<CWebServer>
{
	typedef std::function<void(WebEmSession &session, const request &req, Json::Value &root)> webserver_response_function;

      public:
	struct _tCustomIcon
	{
		int idx;
		std::string RootFile;
		std::string Title;
		std::string Description;
	};
	CWebServer();
	~CWebServer() override;
	bool StartServer(server_settings &settings, const std::string &serverpath, bool bIgnoreUsernamePassword);
	void StopServer();
	void RegisterCommandCode(const char *idname, const webserver_response_function &ResponseFunction, bool bypassAuthentication = false);

	void GetJSonPage(WebEmSession & session, const request& req, reply & rep);
	void GetCameraSnapshot(WebEmSession & session, const request& req, reply & rep);
	void GetInternalCameraSnapshot(WebEmSession & session, const request& req, reply & rep);
	void GetFloorplanImage(WebEmSession& session, const request& req, reply& rep);
	void GetServiceWorker(WebEmSession& session, const request& req, reply& rep);
	void GetDatabaseBackup(WebEmSession & session, const request& req, reply & rep);

	void GetOauth2AuthCode(WebEmSession &session, const request &req, reply &rep);
	void PostOauth2AccessToken(WebEmSession &session, const request &req, reply &rep);
	void GetOpenIDConfiguration(WebEmSession &session, const request &req, reply &rep);

	void SetRFXCOMMode(WebEmSession & session, const request& req, std::string & redirect_uri);
	void RFXComUpgradeFirmware(WebEmSession & session, const request& req, std::string & redirect_uri);
	void UploadFloorplanImage(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetRego6XXType(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetS0MeterType(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetLimitlessType(WebEmSession & session, const request& req, std::string & redirect_uri);

	void SetOpenThermSettings(WebEmSession & session, const request& req, std::string & redirect_uri);
	void Cmd_SendOpenThermCommand(WebEmSession & session, const request& req, Json::Value &root);

	void ReloadPiFace(WebEmSession & session, const request& req, std::string & redirect_uri);
	void RestoreDatabase(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SBFSpotImportOldData(WebEmSession & session, const request& req, std::string & redirect_uri);

	cWebem *m_pWebEm;

	void ReloadCustomSwitchIcons();

	void LoadUsers();
	void AddUser(unsigned long ID, const std::string &username, const std::string &password, const std::string& mfatoken, int userrights, int activetabs, const std::string &pemfile = "");
	void ClearUserPasswords();
	bool FindAdminUser();
	int CountAdminUsers();
	int FindUser(const char* szUserName);
	int FindClient(const char* szClientName);

	void SetWebCompressionMode(_eWebCompressionMode gzmode);
	void SetAllowPlainBasicAuth(const bool allow);
	void SetWebTheme(const std::string &themename);
	void SetWebRoot(const std::string &webRoot);
	void SetIamSettings(const iamserver::iam_settings &iamsettings);

	std::vector<_tWebUserPassword> m_users;
	//JSon
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID,
			    const std::string &floorID, bool bDisplayHidden, bool bDisplayDisabled, bool bFetchFavorites, time_t LastUpdate, const std::string &username,
			    const std::string &hardwareid = ""); // OTO

	// SessionStore interface
	WebEmStoredSession GetSession(const std::string &sessionId) override;
	void StoreSession(const WebEmStoredSession & session) override;
	void RemoveSession(const std::string & sessionId) override;
	void CleanSessions() override;
	void RemoveUsersSessions(const std::string& username, const WebEmSession & exceptSession);
	std::string PluginHardwareDesc(int HwdID);

private:
	bool HandleCommandParam(const std::string &cparam, WebEmSession & session, const request& req, Json::Value &root);
    void GroupBy(Json::Value &root, std::string dbasetable, uint64_t idx, std::string sgroupby, bool bUseValuesOrCounter, std::function<std::string (std::string)> counterExpr, std::function<std::string (std::string)> valueExpr, std::function<std::string (double)> sumToResult);
    void AddTodayValueToResult(Json::Value &root, const std::string &sgroupby, const std::string &today, const double todayValue, const std::string &formatString);

	bool IsIdxForUser(const WebEmSession *pSession, int Idx);

	//OAuth2/OIDC support functions
	std::string GenerateOAuth2RefreshToken(const std::string &username, const int refreshexptime);
	bool ValidateOAuth2RefreshToken(const std::string &refreshtoken, std::string &username);
	void InvalidateOAuth2RefreshToken(const std::string &refreshtoken);
	void PresentOauth2LoginDialog(reply &rep, const std::string &sApp, const std::string &sError);
	bool VerifySHA1TOTP(const std::string &code, const std::string &key);

	//Commands
	void Cmd_RFXComGetFirmwarePercentage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetTimerTypes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLanguages(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSwitchTypes(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetMeterTypes(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetThemes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetTitle(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LoginCheck(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PostSettings(WebEmSession &session, const request &req, Json::Value &root);
	void Cmd_GetHardwareTypes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_WOLGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_WOLAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_WOLUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_WOLRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_WOLClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsGetChilds(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsRemoveChild(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsUpdateChild(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PingerClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_KodiMediaCommand(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LMSSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LMSGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LMSGetPlaylists(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LMSMediaCommand(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LMSDeleteUnusedDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveFibaroLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetFibaroLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetFibaroLinks(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveFibaroLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteFibaroLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesForFibaroLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveInfluxLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetInfluxLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetInfluxLinks(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveInfluxLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteInfluxLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesForInfluxLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDeviceValueOptions(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDeviceValueOptionWording(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetUserVariables(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AllowNewHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdatePlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeletePlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetUnusedPlanDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddPlanActiveDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetPlanDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeletePlanDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetPlanDeviceCoords(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteAllPlanDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ChangePlanOrder(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ChangePlanDeviceOrder(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetVersion(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetAuth(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetMyProfile(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_UpdateMyProfile(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetUptime(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetActualHistory(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetNewHistory(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetConfig(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetLocation(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetForecastConfig(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_SendNotification(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EmailCameraSnapshot(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetThermostatState(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SystemShutdown(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SystemReboot(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ExcecuteScript(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ApplicationUpdate(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_GetCosts(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CheckForUpdate(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CustomEvent(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_DownloadUpdate(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DownloadReady(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteDataPoint(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteDateRange(WebEmSession &session, const request &req, Json::Value &root);
	void Cmd_SetActiveTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnableTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DisableTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddSceneTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateSceneTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteSceneTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnableSceneTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DisableSceneTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearSceneTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSceneActivations(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddSceneCode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RemoveSceneCode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearSceneCodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RenameScene(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetSetpoint(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddSetpointTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateSetpointTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteSetpointTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnableSetpointTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DisableSetpointTimer(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearSetpointTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSerialDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesList(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesListOnOff(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueRegister(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueGetGroups(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueAddGroup(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueDeleteGroup(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueGroupAddLight(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PhilipsHueGroupRemoveLight(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetCustomIconSet(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UploadCustomIcon(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteCustomIcon(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateCustomIcon(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RenameDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetDeviceUsed(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveHttpLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetHttpLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetHttpLinks(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveHttpLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteHttpLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetGooglePubSubLinks(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveGooglePubSubLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteGooglePubSubLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddLogMessage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearShortLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_VacuumDatabase(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_PanasonicMediaCommand(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddMobileDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateMobileDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteMobileDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_HEOSSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_HEOSMediaCommand(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_OnkyoEiscpCommand(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddYeeLight(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddArilux(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_BleBoxSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxAutoSearchingNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxUpdateFirmware(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_GetTimerPlans(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DuplicateTimerPlan(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_AddCamera(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateCamera(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteCamera(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_GetApplications(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddApplication(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateApplication(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteApplication(WebEmSession & session, const request& req, Json::Value &root);

	// Plugin functions
	void Cmd_PluginCommand(WebEmSession & session, const request& req, Json::Value &root);
	void PluginList(Json::Value &root);
#ifdef ENABLE_PYTHON
	void PluginLoadConfig();
#endif

	//Migrated RTypes
	void Cmd_GetUsers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSettings(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSceneLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetScenes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddScene(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteScene(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateScene(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetMobiles(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetCameras(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetCamerasUser(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSchedules(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSceneTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSetpointTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetPlans(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetFloorPlans(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLightLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetTextLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetTransfers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DoTransferDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_Events(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetNotifications(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CreateRFLinkDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CreateMappedSensor(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CreateDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CreateEvohomeSensor(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BindEvohome(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CustomLightIcons(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSharedUserDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetSharedUserDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_HandleGraph(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RemoteWebClientsLog(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_SetUsed(WebEmSession & session, const request& req, Json::Value &root);

	//Migrated ActionCodes
	void Cmd_SetCurrentCostUSBType(WebEmSession& session, const request& req, Json::Value& root);

	void Cmd_ClearUserDevices(WebEmSession& session, const request& req, Json::Value& root);

	//MQTT-AD
	void Cmd_MQTTAD_GetConfig(WebEmSession& session, const request& req, Json::Value& root);
	void Cmd_MQTTAD_UpdateNumber(WebEmSession& session, const request& req, Json::Value& root);

	//EnOcean helpers cmds
	void Cmd_EnOceanGetManufacturers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanGetRORGs(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanGetProfiles(WebEmSession & session, const request& req, Json::Value &root);

	//EnOcean ESP3 cmds
	void Cmd_EnOceanESP3EnableLearnMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanESP3IsNodeTeachedIn(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanESP3CancelTeachIn(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_EnOceanESP3ControllerReset(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_EnOceanESP3UpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanESP3DeleteNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EnOceanESP3GetNodes(WebEmSession & session, const request& req, Json::Value &root);

    void Cmd_TellstickApplySettings(WebEmSession &session, const request &req, Json::Value &root);
	std::shared_ptr<std::thread> m_thread;

	std::map < std::string, webserver_response_function > m_webcommands;	//Commands
	void Do_Work();
	std::vector<_tCustomIcon> m_custom_light_icons;
	std::map<int, int> m_custom_light_icons_lookup;
	bool m_bDoStop;
	std::string m_server_alias;
	uint8_t m_failcount;
	iamserver::iam_settings m_iamsettings;

	struct _tUserAccessCode
	{
		int ID;
		int clientID;
		time_t AuthTime;
		uint64_t ExpTime;
		std::string UserName;
		std::string AuthCode;
		std::string Scope;
		std::string RedirectUri;
		std::string CodeChallenge;
	};

	std::vector<_tUserAccessCode> m_accesscodes;

};

	} // namespace server
} // namespace http
