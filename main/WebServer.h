#pragma once

#include <string>
#include <vector>
#include "../webserver/cWebem.h"
#include "../webserver/request.hpp"
#include "../webserver/session_store.hpp"

struct lua_State;
struct lua_Debug;

namespace Json
{
	class Value;
};

namespace http {
	namespace server {
		class cWebem;
		struct _tWebUserPassword;
class CWebServer : public session_store, public boost::enable_shared_from_this<CWebServer>
{
	typedef boost::function< void(WebEmSession & session, const request& req, Json::Value &root) > webserver_response_function;
public:
	struct _tCustomIcon
	{
		int idx;
		std::string RootFile;
		std::string Title;
		std::string Description;
	};
	CWebServer(void);
	~CWebServer(void);
	bool StartServer(server_settings & settings, const std::string &serverpath, const bool bIgnoreUsernamePassword);
	void StopServer();
	void RegisterCommandCode(const char* idname, webserver_response_function ResponseFunction, bool bypassAuthentication=false);
	void RegisterRType(const char* idname, webserver_response_function ResponseFunction);

	void DisplaySwitchTypesCombo(std::string & content_part);
	void DisplayMeterTypesCombo(std::string & content_part);
	void DisplayTimerTypesCombo(std::string & content_part);
	void DisplayLanguageCombo(std::string & content_part);
	void GetJSonPage(WebEmSession & session, const request& req, reply & rep);
	void GetAppCache(WebEmSession & session, const request& req, reply & rep);
	void GetCameraSnapshot(WebEmSession & session, const request& req, reply & rep);
	void GetInternalCameraSnapshot(WebEmSession & session, const request& req, reply & rep);
	void GetDatabaseBackup(WebEmSession & session, const request& req, reply & rep);
	void Post_UploadCustomIcon(WebEmSession & session, const request& req, reply & rep);

	void PostSettings(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetRFXCOMMode(WebEmSession & session, const request& req, std::string & redirect_uri);
	void RFXComUpgradeFirmware(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetRego6XXType(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetS0MeterType(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetLimitlessType(WebEmSession & session, const request& req, std::string & redirect_uri);

	void SetOpenThermSettings(WebEmSession & session, const request& req, std::string & redirect_uri);
	void Cmd_SendOpenThermCommand(WebEmSession & session, const request& req, Json::Value &root);

	void ReloadPiFace(WebEmSession & session, const request& req, std::string & redirect_uri);
	void RestoreDatabase(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SBFSpotImportOldData(WebEmSession & session, const request& req, std::string & redirect_uri);
	void SetCurrentCostUSBType(WebEmSession & session, const request& req, std::string & redirect_uri);

	void EventCreate(WebEmSession & session, const request& req, std::string & redirect_uri);

	cWebem *m_pWebEm;

	void ReloadCustomSwitchIcons();

	void LoadUsers();
	void AddUser(const unsigned long ID, const std::string &username, const std::string &password, const int userrights, const int activetabs);
	void ClearUserPasswords();
	bool FindAdminUser();
	int FindUser(const char* szUserName);
	void SetAuthenticationMethod(int amethod);
	void SetWebTheme(const std::string &themename);
	void SetWebRoot(const std::string &webRoot);
	std::vector<_tWebUserPassword> m_users;
	//JSon
	void GetJSonDevices(
		Json::Value &root,
		const std::string &rused,
		const std::string &rfilter,
		const std::string &order,
		const std::string &rowid,
		const std::string &planID,
		const std::string &floorID,
		const bool bDisplayHidden,
		const bool bDisplayDisabled,
		const bool bFetchFavorites,
		const time_t LastUpdate,
		const std::string &username,
		const std::string &hardwareid = ""); // OTO

	// SessionStore interface
	const WebEmStoredSession GetSession(const std::string & sessionId);
	void StoreSession(const WebEmStoredSession & session);
	void RemoveSession(const std::string & sessionId);
	void CleanSessions();
	void RemoveUsersSessions(const std::string& username, const WebEmSession & exceptSession);
	std::string PluginHardwareDesc(int HwdID);

private:
	void HandleCommand(const std::string &cparam, WebEmSession & session, const request& req, Json::Value &root);
	void HandleRType(const std::string &rtype, WebEmSession & session, const request& req, Json::Value &root);

	//Commands
	void Cmd_RFXComGetFirmwarePercentage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLanguage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetThemes(WebEmSession & session, const request& req, Json::Value &root);
        void Cmd_GetTitle(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_LoginCheck(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_SaveUserVariable(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_GetUptime(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetActualHistory(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetNewHistory(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SendNotification(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_EmailCameraSnapshot(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetThermostatState(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SystemShutdown(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SystemReboot(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ExcecuteScript(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetCosts(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_CheckForUpdate(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DownloadUpdate(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DownloadReady(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteDatePoint(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_DeleteCustomIcon(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateCustomIcon(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RenameDevice(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SetUnused(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_AddYeeLight(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddArilux(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_BleBoxSetMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxGetNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxAddNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxClearNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxAutoSearchingNodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_BleBoxUpdateFirmware(WebEmSession & session, const request& req, Json::Value &root);
	
	void Cmd_GetTimerPlans(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateTimerPlan(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteTimerPlan(WebEmSession & session, const request& req, Json::Value &root);

	void Cmd_AddCamera(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateCamera(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteCamera(WebEmSession & session, const request& req, Json::Value &root);

	// Plugin functions
	void Cmd_PluginCommand(WebEmSession & session, const request& req, Json::Value &root);
	void PluginList(Json::Value &root);
#ifdef ENABLE_PYTHON
	void PluginLoadConfig();
#endif

	//RTypes
	void RType_HandleGraph(WebEmSession & session, const request& req, Json::Value &root);
	void RType_LightLog(WebEmSession & session, const request& req, Json::Value &root);
	void RType_TextLog(WebEmSession & session, const request& req, Json::Value &root);
	void RType_SceneLog(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Settings(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Events(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Hardware(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Devices(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Cameras(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Users(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Mobiles(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Timers(WebEmSession & session, const request& req, Json::Value &root);
	void RType_SceneTimers(WebEmSession & session, const request& req, Json::Value &root);
	void RType_SetpointTimers(WebEmSession & session, const request& req, Json::Value &root);
	void RType_GetTransfers(WebEmSession & session, const request& req, Json::Value &root);
	void RType_TransferDevice(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Notifications(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Schedules(WebEmSession & session, const request& req, Json::Value &root);
	void RType_GetSharedUserDevices(WebEmSession & session, const request& req, Json::Value &root);
	void RType_SetSharedUserDevices(WebEmSession & session, const request& req, Json::Value &root);
	void RType_SetUsed(WebEmSession & session, const request& req, Json::Value &root);
	void RType_DeleteDevice(WebEmSession & session, const request& req, Json::Value &root);
	void RType_AddScene(WebEmSession & session, const request& req, Json::Value &root);
	void RType_DeleteScene(WebEmSession & session, const request& req, Json::Value &root);
	void RType_UpdateScene(WebEmSession & session, const request& req, Json::Value &root);
	void RType_CreateVirtualSensor(WebEmSession & session, const request& req, Json::Value &root);
	void RType_CustomLightIcons(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Plans(WebEmSession & session, const request& req, Json::Value &root);
	void RType_FloorPlans(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Scenes(WebEmSession & session, const request& req, Json::Value &root);
	void RType_CreateEvohomeSensor(WebEmSession & session, const request& req, Json::Value &root);
	void RType_BindEvohome(WebEmSession & session, const request& req, Json::Value &root);
	void RType_CreateRFLinkDevice(WebEmSession & session, const request& req, Json::Value &root);
#ifdef WITH_OPENZWAVE
	//ZWave
	void Cmd_ZWaveUpdateNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveDeleteNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveInclude(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveExclude(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveIsNodeIncluded(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveIsNodeExcluded(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveSoftReset(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveHardReset(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveNetworkHeal(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveNodeHeal(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveNetworkInfo(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveRemoveGroupNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveAddGroupNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveGroupInfo(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveCancel(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ApplyZWaveNodeConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveStateCheck(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveRequestNodeConfig(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveReceiveConfigurationFromOtherController(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveSendConfigurationToSecondaryController(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveTransferPrimaryRole(WebEmSession & session, const request& req, Json::Value &root);
	void ZWaveGetConfigFile(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPPollXml(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPIndex(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPNodeGetConf(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPNodeGetValues(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPNodeSetValue(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPNodeSetButton(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPAdminCommand(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPNodeChange(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPSaveConfig(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPGetTopo(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPGetStats(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPSetGroup(WebEmSession & session, const request& req, reply & rep);
	void ZWaveCPSceneCommand(WebEmSession & session, const request& req, reply & rep);
	void Cmd_ZWaveSetUserCodeEnrollmentMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveGetNodeUserCodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveRemoveUserCode(WebEmSession & session, const request& req, Json::Value &root);
	void ZWaveCPTestHeal(WebEmSession & session, const request& req, reply & rep);
	//RTypes
	void RType_OpenZWaveNodes(WebEmSession & session, const request& req, Json::Value &root);
	int m_ZW_Hwidx;
#endif
#ifdef WITH_TELLDUSCORE
    void Cmd_TellstickApplySettings(WebEmSession &session, const request &req, Json::Value &root);
#endif
	boost::shared_ptr<boost::thread> m_thread;

	std::map < std::string, webserver_response_function > m_webcommands;
	std::map < std::string, webserver_response_function > m_webrtypes;
	void Do_Work();
	std::vector<_tCustomIcon> m_custom_light_icons;
	std::map<int, int> m_custom_light_icons_lookup;
	bool m_bDoStop;
	std::string m_server_alias;
};

} //server
}//http
