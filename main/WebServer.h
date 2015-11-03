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
class CWebServer : public session_store
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
	bool StartServer(const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword, const std::string &secure_cert_file = "", const std::string &secure_cert_passphrase = "");
	void StopServer();
	void RegisterCommandCode(const char* idname, webserver_response_function ResponseFunction, bool bypassAuthentication=false);
	void RegisterRType(const char* idname, webserver_response_function ResponseFunction);

	char * DisplaySwitchTypesCombo();
	char * DisplayMeterTypesCombo();
	char * DisplayTimerTypesCombo();
	char * DisplayLanguageCombo();
	std::string GetJSonPage(WebEmSession & session, const request& req);
	std::string GetAppCache(WebEmSession & session, const request& req);
	std::string GetCameraSnapshot(WebEmSession & session, const request& req);
	std::string GetInternalCameraSnapshot(WebEmSession & session, const request& req);
	std::string GetDatabaseBackup(WebEmSession & session, const request& req);
	std::string Post_UploadCustomIcon(WebEmSession & session, const request& req);

	char * PostSettings(WebEmSession & session, const request& req);
	char * SetRFXCOMMode(WebEmSession & session, const request& req);
	char * RFXComUpgradeFirmware(WebEmSession & session, const request& req);
	char * SetRego6XXType(WebEmSession & session, const request& req);
	char * SetS0MeterType(WebEmSession & session, const request& req);
	char * SetLimitlessType(WebEmSession & session, const request& req);
	char * SetOpenThermSettings(WebEmSession & session, const request& req);
	char * SetP1USBType(WebEmSession & session, const request& req);
	char * RestoreDatabase(WebEmSession & session, const request& req);
	char * SBFSpotImportOldData(WebEmSession & session, const request& req);

	cWebem *m_pWebEm;

	void ReloadCustomSwitchIcons();

	void LoadUsers();
	void AddUser(const unsigned long ID, const std::string &username, const std::string &password, const int userrights, const int activetabs);
	void ClearUserPasswords();
	bool FindAdminUser();
	int FindUser(const char* szUserName);
	void SetAuthenticationMethod(int amethod);
	void SetWebTheme(const std::string &themename);
	std::vector<_tWebUserPassword> m_users;
	//JSon
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const std::string &username);

	// SessionStore interface
	const WebEmStoredSession GetSession(const std::string & sessionId);
	void StoreSession(const WebEmStoredSession & session);
	void RemoveSession(const std::string & sessionId);
	void CleanSessions();

private:
	void HandleCommand(const std::string &cparam, WebEmSession & session, const request& req, Json::Value &root);
	void HandleRType(const std::string &rtype, WebEmSession & session, const request& req, Json::Value &root);

	//Commands
	void Cmd_RFXComGetFirmwarePercentage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLanguage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetThemes(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_MySensorsRemoveNode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_MySensorsRemoveChild(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_GetDeviceValueOptions(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDeviceValueOptionWording(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_DeleteUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_SaveUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_UpdateUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetUserVariables(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetUserVariable(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AllowNewHardware(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetLog(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_ClearSetpointTimers(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetSerialDevices(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesList(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_GetDevicesListOnOff(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_RegisterWithPhilipsHue(WebEmSession & session, const request& req, Json::Value &root);
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
	void Cmd_GetDevicesForHttpLink(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_AddLogMessage(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ClearShortLog(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_VacuumDatabase(WebEmSession & session, const request& req, Json::Value &root);

	//RTypes
	void RType_HandleGraph(WebEmSession & session, const request& req, Json::Value &root);
	void RType_LightLog(WebEmSession & session, const request& req, Json::Value &root);
	void RType_TextLog(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Settings(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Events(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Hardware(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Devices(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Cameras(WebEmSession & session, const request& req, Json::Value &root);
	void RType_Users(WebEmSession & session, const request& req, Json::Value &root);
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
	std::string ZWaveGetConfigFile(WebEmSession & session, const request& req);
	std::string ZWaveCPPollXml(WebEmSession & session, const request& req);
	std::string ZWaveCPIndex(WebEmSession & session, const request& req);
	std::string ZWaveCPNodeGetConf(WebEmSession & session, const request& req);
	std::string ZWaveCPNodeGetValues(WebEmSession & session, const request& req);
	std::string ZWaveCPNodeSetValue(WebEmSession & session, const request& req);
	std::string ZWaveCPNodeSetButton(WebEmSession & session, const request& req);
	std::string ZWaveCPAdminCommand(WebEmSession & session, const request& req);
	std::string ZWaveCPNodeChange(WebEmSession & session, const request& req);
	std::string ZWaveCPSaveConfig(WebEmSession & session, const request& req);
	std::string ZWaveCPGetTopo(WebEmSession & session, const request& req);
	std::string ZWaveCPGetStats(WebEmSession & session, const request& req);
	void Cmd_ZWaveSetUserCodeEnrollmentMode(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveGetNodeUserCodes(WebEmSession & session, const request& req, Json::Value &root);
	void Cmd_ZWaveRemoveUserCode(WebEmSession & session, const request& req, Json::Value &root);
	//RTypes
	void RType_OpenZWaveNodes(WebEmSession & session, const request& req, Json::Value &root);
	int m_ZW_Hwidx;
#endif	
	boost::shared_ptr<boost::thread> m_thread;

	std::map < std::string, webserver_response_function > m_webcommands;
	std::map < std::string, webserver_response_function > m_webrtypes;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;
	std::vector<_tCustomIcon> m_custom_light_icons;
	std::map<int, int> m_custom_light_icons_lookup;
	bool m_bDoStop;

	void luaThread(lua_State *lua_state, const std::string &filename);
	static void luaStop(lua_State *L, lua_Debug *ar);
	void report_errors(lua_State *L, int status);
};

} //server
}//http
