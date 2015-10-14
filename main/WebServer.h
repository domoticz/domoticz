#pragma once

#include <string>
#include <vector>
#include "../webserver/request.hpp"

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
class CWebServer
{
	typedef boost::function< void(const request& req, Json::Value &root) > webserver_response_function;
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
	std::string GetJSonPage(const request& req);
	std::string GetAppCache(const request& req);
	std::string GetCameraSnapshot(const request& req);
	std::string GetInternalCameraSnapshot(const request& req);
	std::string GetDatabaseBackup(const request& req);
	std::string Post_UploadCustomIcon(const request& req);

	char * PostSettings(const request& req);
	char * SetRFXCOMMode(const request& req);
	char * RFXComUpgradeFirmware(const request& req);
	char * SetRego6XXType(const request& req);
	char * SetS0MeterType(const request& req);
	char * SetLimitlessType(const request& req);
	char * SetOpenThermSettings(const request& req);
	char * SetP1USBType(const request& req);
	char * RestoreDatabase(const request& req);
	char * SBFSpotImportOldData(const request& req);

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
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const bool bSkipUserCheck);
private:
	void HandleCommand(const std::string &cparam, const request& req, Json::Value &root);
	void HandleRType(const std::string &rtype, const request& req, Json::Value &root);

	//Commands
	void Cmd_RFXComGetFirmwarePercentage(const request& req, Json::Value &root);
	void Cmd_GetLanguage(const request& req, Json::Value &root);
	void Cmd_GetThemes(const request& req, Json::Value &root);
	void Cmd_LoginCheck(const request& req, Json::Value &root);
	void Cmd_GetHardwareTypes(const request& req, Json::Value &root);
	void Cmd_AddHardware(const request& req, Json::Value &root);
	void Cmd_UpdateHardware(const request& req, Json::Value &root);
	void Cmd_DeleteHardware(const request& req, Json::Value &root);
	void Cmd_WOLGetNodes(const request& req, Json::Value &root);
	void Cmd_WOLAddNode(const request& req, Json::Value &root);
	void Cmd_WOLUpdateNode(const request& req, Json::Value &root);
	void Cmd_WOLRemoveNode(const request& req, Json::Value &root);
	void Cmd_WOLClearNodes(const request& req, Json::Value &root);
	void Cmd_PingerSetMode(const request& req, Json::Value &root);
	void Cmd_PingerGetNodes(const request& req, Json::Value &root);
	void Cmd_PingerAddNode(const request& req, Json::Value &root);
	void Cmd_PingerUpdateNode(const request& req, Json::Value &root);
	void Cmd_PingerRemoveNode(const request& req, Json::Value &root);
	void Cmd_PingerClearNodes(const request& req, Json::Value &root);
	void Cmd_KodiSetMode(const request& req, Json::Value &root);
	void Cmd_KodiGetNodes(const request& req, Json::Value &root);
	void Cmd_KodiAddNode(const request& req, Json::Value &root);
	void Cmd_KodiUpdateNode(const request& req, Json::Value &root);
	void Cmd_KodiRemoveNode(const request& req, Json::Value &root);
	void Cmd_KodiClearNodes(const request& req, Json::Value &root);
	void Cmd_KodiMediaCommand(const request& req, Json::Value &root);
	void Cmd_LMSSetMode(const request& req, Json::Value &root);
	void Cmd_LMSGetNodes(const request& req, Json::Value &root);
	void Cmd_LMSMediaCommand(const request& req, Json::Value &root);
	void Cmd_SaveFibaroLinkConfig(const request& req, Json::Value &root);
	void Cmd_GetFibaroLinkConfig(const request& req, Json::Value &root);
	void Cmd_GetFibaroLinks(const request& req, Json::Value &root);
	void Cmd_SaveFibaroLink(const request& req, Json::Value &root);
	void Cmd_DeleteFibaroLink(const request& req, Json::Value &root);
	void Cmd_GetDevicesForFibaroLink(const request& req, Json::Value &root);
	void Cmd_GetDeviceValueOptions(const request& req, Json::Value &root);
	void Cmd_GetDeviceValueOptionWording(const request& req, Json::Value &root);
	void Cmd_DeleteUserVariable(const request& req, Json::Value &root);
	void Cmd_SaveUserVariable(const request& req, Json::Value &root);
	void Cmd_UpdateUserVariable(const request& req, Json::Value &root);
	void Cmd_GetUserVariables(const request& req, Json::Value &root);
	void Cmd_GetUserVariable(const request& req, Json::Value &root);
	void Cmd_AllowNewHardware(const request& req, Json::Value &root);
	void Cmd_GetLog(const request& req, Json::Value &root);
	void Cmd_AddPlan(const request& req, Json::Value &root);
	void Cmd_UpdatePlan(const request& req, Json::Value &root);
	void Cmd_DeletePlan(const request& req, Json::Value &root);
	void Cmd_GetUnusedPlanDevices(const request& req, Json::Value &root);
	void Cmd_AddPlanActiveDevice(const request& req, Json::Value &root);
	void Cmd_GetPlanDevices(const request& req, Json::Value &root);
	void Cmd_DeletePlanDevice(const request& req, Json::Value &root);
	void Cmd_SetPlanDeviceCoords(const request& req, Json::Value &root);
	void Cmd_DeleteAllPlanDevices(const request& req, Json::Value &root);
	void Cmd_ChangePlanOrder(const request& req, Json::Value &root);
	void Cmd_ChangePlanDeviceOrder(const request& req, Json::Value &root);
	void Cmd_GetVersion(const request& req, Json::Value &root);
	void Cmd_GetAuth(const request& req, Json::Value &root);
	void Cmd_GetActualHistory(const request& req, Json::Value &root);
	void Cmd_GetNewHistory(const request& req, Json::Value &root);
	void Cmd_GetConfig(const request& req, Json::Value &root);
	void Cmd_SendNotification(const request& req, Json::Value &root);
	void Cmd_EmailCameraSnapshot(const request& req, Json::Value &root);
	void Cmd_UpdateDevice(const request& req, Json::Value &root);
	void Cmd_UpdateDevices(const request& req, Json::Value &root);
	void Cmd_SetThermostatState(const request& req, Json::Value &root);
	void Cmd_SystemShutdown(const request& req, Json::Value &root);
	void Cmd_SystemReboot(const request& req, Json::Value &root);
	void Cmd_ExcecuteScript(const request& req, Json::Value &root);
	void Cmd_GetCosts(const request& req, Json::Value &root);
	void Cmd_CheckForUpdate(const request& req, Json::Value &root);
	void Cmd_DownloadUpdate(const request& req, Json::Value &root);
	void Cmd_DownloadReady(const request& req, Json::Value &root);
	void Cmd_DeleteDatePoint(const request& req, Json::Value &root);
	void Cmd_AddTimer(const request& req, Json::Value &root);
	void Cmd_UpdateTimer(const request& req, Json::Value &root);
	void Cmd_DeleteTimer(const request& req, Json::Value &root);
	void Cmd_EnableTimer(const request& req, Json::Value &root);
	void Cmd_DisableTimer(const request& req, Json::Value &root);
	void Cmd_ClearTimers(const request& req, Json::Value &root);
	void Cmd_AddSceneTimer(const request& req, Json::Value &root);
	void Cmd_UpdateSceneTimer(const request& req, Json::Value &root);
	void Cmd_DeleteSceneTimer(const request& req, Json::Value &root);
	void Cmd_EnableSceneTimer(const request& req, Json::Value &root);
	void Cmd_DisableSceneTimer(const request& req, Json::Value &root);
	void Cmd_ClearSceneTimers(const request& req, Json::Value &root);
	void Cmd_GetSceneActivations(const request& req, Json::Value &root);
	void Cmd_AddSceneCode(const request& req, Json::Value &root);
	void Cmd_RemoveSceneCode(const request& req, Json::Value &root);
	void Cmd_ClearSceneCodes(const request& req, Json::Value &root);
	void Cmd_RenameScene(const request& req, Json::Value &root);
	void Cmd_SetSetpoint(const request& req, Json::Value &root);
	void Cmd_AddSetpointTimer(const request& req, Json::Value &root);
	void Cmd_UpdateSetpointTimer(const request& req, Json::Value &root);
	void Cmd_DeleteSetpointTimer(const request& req, Json::Value &root);
	void Cmd_ClearSetpointTimers(const request& req, Json::Value &root);
	void Cmd_GetSerialDevices(const request& req, Json::Value &root);
	void Cmd_GetDevicesList(const request& req, Json::Value &root);
	void Cmd_GetDevicesListOnOff(const request& req, Json::Value &root);
	void Cmd_RegisterWithPhilipsHue(const request& req, Json::Value &root);
	void Cmd_GetCustomIconSet(const request& req, Json::Value &root);
	void Cmd_DeleteCustomIcon(const request& req, Json::Value &root);
	void Cmd_UpdateCustomIcon(const request& req, Json::Value &root);
	void Cmd_RenameDevice(const request& req, Json::Value &root);
	void Cmd_SetUnused(const request& req, Json::Value &root);
	void Cmd_SaveHttpLinkConfig(const request& req, Json::Value &root);
	void Cmd_GetHttpLinkConfig(const request& req, Json::Value &root);
	void Cmd_GetHttpLinks(const request& req, Json::Value &root);
	void Cmd_SaveHttpLink(const request& req, Json::Value &root);
	void Cmd_DeleteHttpLink(const request& req, Json::Value &root);
	void Cmd_GetDevicesForHttpLink(const request& req, Json::Value &root);
	void Cmd_AddLogMessage(const request& req, Json::Value &root);
	void Cmd_ClearShortLog(const request& req, Json::Value &root);
	void Cmd_VacuumDatabase(const request& req, Json::Value &root);

	//RTypes
	void RType_HandleGraph(const request& req, Json::Value &root);
	void RType_LightLog(const request& req, Json::Value &root);
	void RType_TextLog(const request& req, Json::Value &root);
	void RType_Settings(const request& req, Json::Value &root);
	void RType_Events(const request& req, Json::Value &root);
	void RType_Hardware(const request& req, Json::Value &root);
	void RType_Devices(const request& req, Json::Value &root);
	void RType_Cameras(const request& req, Json::Value &root);
	void RType_Users(const request& req, Json::Value &root);
	void RType_Timers(const request& req, Json::Value &root);
	void RType_SceneTimers(const request& req, Json::Value &root);
	void RType_SetpointTimers(const request& req, Json::Value &root);
	void RType_GetTransfers(const request& req, Json::Value &root);
	void RType_TransferDevice(const request& req, Json::Value &root);
	void RType_Notifications(const request& req, Json::Value &root);
	void RType_Schedules(const request& req, Json::Value &root);
	void RType_GetSharedUserDevices(const request& req, Json::Value &root);
	void RType_SetSharedUserDevices(const request& req, Json::Value &root);
	void RType_SetUsed(const request& req, Json::Value &root);
	void RType_DeleteDevice(const request& req, Json::Value &root);
	void RType_AddScene(const request& req, Json::Value &root);
	void RType_DeleteScene(const request& req, Json::Value &root);
	void RType_UpdateScene(const request& req, Json::Value &root);
	void RType_CreateVirtualSensor(const request& req, Json::Value &root);
	void RType_CustomLightIcons(const request& req, Json::Value &root);
	void RType_Plans(const request& req, Json::Value &root);
	void RType_FloorPlans(const request& req, Json::Value &root);
	void RType_Scenes(const request& req, Json::Value &root);
	void RType_CreateEvohomeSensor(const request& req, Json::Value &root);
	void RType_BindEvohome(const request& req, Json::Value &root);
#ifdef WITH_OPENZWAVE
	//ZWave
	void Cmd_ZWaveUpdateNode(const request& req, Json::Value &root);
	void Cmd_ZWaveDeleteNode(const request& req, Json::Value &root);
	void Cmd_ZWaveInclude(const request& req, Json::Value &root);
	void Cmd_ZWaveExclude(const request& req, Json::Value &root);
	void Cmd_ZWaveIsNodeIncluded(const request& req, Json::Value &root);
	void Cmd_ZWaveIsNodeExcluded(const request& req, Json::Value &root);
	void Cmd_ZWaveSoftReset(const request& req, Json::Value &root);
	void Cmd_ZWaveHardReset(const request& req, Json::Value &root);
	void Cmd_ZWaveNetworkHeal(const request& req, Json::Value &root);
	void Cmd_ZWaveNodeHeal(const request& req, Json::Value &root);
	void Cmd_ZWaveNetworkInfo(const request& req, Json::Value &root);
	void Cmd_ZWaveRemoveGroupNode(const request& req, Json::Value &root);
	void Cmd_ZWaveAddGroupNode(const request& req, Json::Value &root);
	void Cmd_ZWaveGroupInfo(const request& req, Json::Value &root);
	void Cmd_ZWaveCancel(const request& req, Json::Value &root);
	void Cmd_ApplyZWaveNodeConfig(const request& req, Json::Value &root);
	void Cmd_ZWaveStateCheck(const request& req, Json::Value &root);
	void Cmd_ZWaveRequestNodeConfig(const request& req, Json::Value &root);
	void Cmd_ZWaveReceiveConfigurationFromOtherController(const request& req, Json::Value &root);
	void Cmd_ZWaveSendConfigurationToSecondaryController(const request& req, Json::Value &root);
	void Cmd_ZWaveTransferPrimaryRole(const request& req, Json::Value &root);
	std::string ZWaveGetConfigFile(const request& req);
	std::string ZWaveCPPollXml(const request& req);
	std::string ZWaveCPIndex(const request& req);
	std::string ZWaveCPNodeGetConf(const request& req);
	std::string ZWaveCPNodeGetValues(const request& req);
	std::string ZWaveCPNodeSetValue(const request& req);
	std::string ZWaveCPNodeSetButton(const request& req);
	std::string ZWaveCPAdminCommand(const request& req);
	std::string ZWaveCPNodeChange(const request& req);
	std::string ZWaveCPSaveConfig(const request& req);
	std::string ZWaveCPGetTopo(const request& req);
	std::string ZWaveCPGetStats(const request& req);
	void Cmd_ZWaveSetUserCodeEnrollmentMode(const request& req, Json::Value &root);
	void Cmd_ZWaveGetNodeUserCodes(const request& req, Json::Value &root);
	void Cmd_ZWaveRemoveUserCode(const request& req, Json::Value &root);
	//RTypes
	void RType_OpenZWaveNodes(const request& req, Json::Value &root);
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
