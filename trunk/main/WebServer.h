#pragma once

#include <string>
#include <vector>

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
	typedef boost::function< void(Json::Value &root) > webserver_response_function;
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
	std::string GetJSonPage();
	std::string GetAppCache();
	std::string GetCameraSnapshot();
	std::string GetInternalCameraSnapshot();
	std::string GetDatabaseBackup();
	std::string Post_UploadCustomIcon();

	char * PostSettings();
	char * SetRFXCOMMode();
	char * SetRego6XXType();
	char * SetS0MeterType();
	char * SetLimitlessType();
	char * SetOpenThermSettings();
	char * SetP1USBType();
	char * RestoreDatabase();
	char * SBFSpotImportOldData();

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
	void HandleCommand(const std::string &cparam, Json::Value &root);
	void HandleRType(const std::string &rtype, Json::Value &root);

	//Commands
	void Cmd_GetLanguage(Json::Value &root);
	void Cmd_GetThemes(Json::Value &root);
	void Cmd_LoginCheck(Json::Value &root);
	void Cmd_GetHardwareTypes(Json::Value &root);
	void Cmd_AddHardware(Json::Value &root);
	void Cmd_UpdateHardware(Json::Value &root);
	void Cmd_DeleteHardware(Json::Value &root);
	void Cmd_WOLGetNodes(Json::Value &root);
	void Cmd_WOLAddNode(Json::Value &root);
	void Cmd_WOLUpdateNode(Json::Value &root);
	void Cmd_WOLRemoveNode(Json::Value &root);
	void Cmd_WOLClearNodes(Json::Value &root);
	void Cmd_PingerSetMode(Json::Value &root);
	void Cmd_PingerGetNodes(Json::Value &root);
	void Cmd_PingerAddNode(Json::Value &root);
	void Cmd_PingerUpdateNode(Json::Value &root);
	void Cmd_PingerRemoveNode(Json::Value &root);
	void Cmd_PingerClearNodes(Json::Value &root);
	void Cmd_SaveFibaroLinkConfig(Json::Value &root);
	void Cmd_GetFibaroLinkConfig(Json::Value &root);
	void Cmd_GetFibaroLinks(Json::Value &root);
	void Cmd_SaveFibaroLink(Json::Value &root);
	void Cmd_DeleteFibaroLink(Json::Value &root);
	void Cmd_GetDevicesForFibaroLink(Json::Value &root);
	void Cmd_GetDeviceValueOptions(Json::Value &root);
	void Cmd_GetDeviceValueOptionWording(Json::Value &root);
	void Cmd_DeleteUserVariable(Json::Value &root);
	void Cmd_SaveUserVariable(Json::Value &root);
	void Cmd_UpdateUserVariable(Json::Value &root);
	void Cmd_GetUserVariables(Json::Value &root);
	void Cmd_GetUserVariable(Json::Value &root);
	void Cmd_AllowNewHardware(Json::Value &root);
	void Cmd_GetLog(Json::Value &root);
	void Cmd_AddPlan(Json::Value &root);
	void Cmd_UpdatePlan(Json::Value &root);
	void Cmd_DeletePlan(Json::Value &root);
	void Cmd_GetUnusedPlanDevices(Json::Value &root);
	void Cmd_AddPlanActiveDevice(Json::Value &root);
	void Cmd_GetPlanDevices(Json::Value &root);
	void Cmd_DeletePlanDevice(Json::Value &root);
	void Cmd_SetPlanDeviceCoords(Json::Value &root);
	void Cmd_DeleteAllPlanDevices(Json::Value &root);
	void Cmd_ChangePlanOrder(Json::Value &root);
	void Cmd_ChangePlanDeviceOrder(Json::Value &root);
	void Cmd_GetVersion(Json::Value &root);
	void Cmd_GetAuth(Json::Value &root);
	void Cmd_GetActualHistory(Json::Value &root);
	void Cmd_GetNewHistory(Json::Value &root);
	void Cmd_GetConfig(Json::Value &root);
	void Cmd_SendNotification(Json::Value &root);
	void Cmd_EmailCameraSnapshot(Json::Value &root);
	void Cmd_UpdateDevice(Json::Value &root);
	void Cmd_SetThermostatState(Json::Value &root);
	void Cmd_SystemShutdown(Json::Value &root);
	void Cmd_SystemReboot(Json::Value &root);
	void Cmd_ExcecuteScript(Json::Value &root);
	void Cmd_GetCosts(Json::Value &root);
	void Cmd_CheckForUpdate(Json::Value &root);
	void Cmd_DownloadUpdate(Json::Value &root);
	void Cmd_DownloadReady(Json::Value &root);
	void Cmd_DeleteDatePoint(Json::Value &root);
	void Cmd_AddTimer(Json::Value &root);
	void Cmd_UpdateTimer(Json::Value &root);
	void Cmd_DeleteTimer(Json::Value &root);
	void Cmd_ClearTimers(Json::Value &root);
	void Cmd_AddSceneTimer(Json::Value &root);
	void Cmd_UpdateSceneTimer(Json::Value &root);
	void Cmd_DeleteSceneTimer(Json::Value &root);
	void Cmd_ClearSceneTimers(Json::Value &root);
	void Cmd_SetSceneCode(Json::Value &root);
	void Cmd_RemoveSceneCode(Json::Value &root);
	void Cmd_SetSetpoint(Json::Value &root);
	void Cmd_AddSetpointTimer(Json::Value &root);
	void Cmd_UpdateSetpointTimer(Json::Value &root);
	void Cmd_DeleteSetpointTimer(Json::Value &root);
	void Cmd_ClearSetpointTimers(Json::Value &root);
	void Cmd_GetSerialDevices(Json::Value &root);
	void Cmd_GetDevicesList(Json::Value &root);
	void Cmd_GetDevicesListOnOff(Json::Value &root);
	void Cmd_RegisterWithPhilipsHue(Json::Value &root);
	void Cmd_GetCustomIconSet(Json::Value &root);
	void Cmd_DeleteCustomIcon(Json::Value &root);
	void Cmd_RenameDevice(Json::Value &root);
	void Cmd_SaveHttpLinkConfig(Json::Value &root);
	void Cmd_GetHttpLinkConfig(Json::Value &root);
	void Cmd_GetHttpLinks(Json::Value &root);
	void Cmd_SaveHttpLink(Json::Value &root);
	void Cmd_DeleteHttpLink(Json::Value &root);
	void Cmd_GetDevicesForHttpLink(Json::Value &root);

	//RTypes
	void RType_HandleGraph(Json::Value &root);
	void RType_LightLog(Json::Value &root);
	void RType_TextLog(Json::Value &root);
	void RType_Settings(Json::Value &root);
	void RType_Events(Json::Value &root);
	void RType_Hardware(Json::Value &root);
	void RType_Devices(Json::Value &root);
	void RType_Cameras(Json::Value &root);
	void RType_Users(Json::Value &root);
	void RType_Timers(Json::Value &root);
	void RType_SceneTimers(Json::Value &root);
	void RType_SetpointTimers(Json::Value &root);
	void RType_GetTransfers(Json::Value &root);
	void RType_TransferDevice(Json::Value &root);
	void RType_Notifications(Json::Value &root);
	void RType_Schedules(Json::Value &root);
	void RType_GetSharedUserDevices(Json::Value &root);
	void RType_SetSharedUserDevices(Json::Value &root);
	void RType_SetUsed(Json::Value &root);
	void RType_DeleteDevice(Json::Value &root);
	void RType_AddScene(Json::Value &root);
	void RType_DeleteScene(Json::Value &root);
	void RType_UpdateScene(Json::Value &root);
	void RType_CreateVirtualSensor(Json::Value &root);
	void RType_CustomLightIcons(Json::Value &root);
	void RType_Plans(Json::Value &root);
	void RType_FloorPlans(Json::Value &root);
	void RType_Scenes(Json::Value &root);
	void RType_CreateEvohomeSensor(Json::Value &root);
	void RType_BindEvohome(Json::Value &root);
#ifdef WITH_OPENZWAVE
	//ZWave
	void Cmd_ZWaveUpdateNode(Json::Value &root);
	void Cmd_ZWaveDeleteNode(Json::Value &root);
	void Cmd_ZWaveInclude(Json::Value &root);
	void Cmd_ZWaveExclude(Json::Value &root);
	void Cmd_ZWaveSoftReset(Json::Value &root);
	void Cmd_ZWaveHardReset(Json::Value &root);
	void Cmd_ZWaveNetworkHeal(Json::Value &root);
	void Cmd_ZWaveNodeHeal(Json::Value &root);
	void Cmd_ZWaveNetworkInfo(Json::Value &root);
	void Cmd_ZWaveRemoveGroupNode(Json::Value &root);
	void Cmd_ZWaveAddGroupNode(Json::Value &root);
	void Cmd_ZWaveGroupInfo(Json::Value &root);
	void Cmd_ZWaveCancel(Json::Value &root);
	void Cmd_ApplyZWaveNodeConfig(Json::Value &root);
	void Cmd_RequestZWaveNodeConfig(Json::Value &root);
	void Cmd_ZWaveStateCheck(Json::Value &root);
	void Cmd_ZWaveRequestNodeConfig(Json::Value &root);
	void Cmd_ZWaveReceiveConfigurationFromOtherController(Json::Value &root);
	void Cmd_ZWaveSendConfigurationToSecondaryController(Json::Value &root);
	void Cmd_ZWaveTransferPrimaryRole(Json::Value &root);
	std::string ZWaveGetConfigFile();
	std::string ZWaveCPPollXml();
	std::string ZWaveCPIndex();
	std::string ZWaveCPNodeGetConf();
	std::string ZWaveCPNodeGetValues();
	std::string ZWaveCPNodeSetValue();
	std::string ZWaveCPNodeSetButton();
	std::string ZWaveCPAdminCommand();
	std::string ZWaveCPNodeChange();
	std::string ZWaveCPSaveConfig();
	std::string ZWaveCPGetTopo();
	std::string ZWaveCPGetStats();
	void Cmd_ZWaveSetUserCodeEnrollmentMode(Json::Value &root);
	void Cmd_ZWaveGetNodeUserCodes(Json::Value &root);
	void Cmd_ZWaveRemoveUserCode(Json::Value &root);
	//RTypes
	void RType_OpenZWaveNodes(Json::Value &root);
	int m_ZW_Hwidx;
#endif	
	boost::shared_ptr<boost::thread> m_thread;

	std::map < std::string, webserver_response_function > m_webcommands;
	std::map < std::string, webserver_response_function > m_webrtypes;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;
	time_t m_LastUpdateCheck;
	std::vector<_tCustomIcon> m_custom_light_icons;
	std::map<int, int> m_custom_light_icons_lookup;
	bool m_bDoStop;

	bool m_bHaveUpdate;
	int m_iRevision;
};

} //server
}//http
