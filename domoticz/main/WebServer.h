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
	struct _tCustomIcon
	{
		std::string RootFile;
		std::string Title;
		std::string Description;
	};
	typedef boost::function< void(Json::Value &root) > webserver_response_function;
public:
	CWebServer(void);
	~CWebServer(void);
	bool StartServer(const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword);
	void StopServer();
	void RegisterCommandCode(const char* idname, webserver_response_function ResponseFunction);
	void RegisterRType(const char* idname, webserver_response_function ResponseFunction);

	char * DisplayVersion();
	char * DisplayHardwareCombo();
	char * DisplayDataPushDevicesCombo();
	char * DisplayDataPushOnOffDevicesCombo();
	char * DisplaySwitchTypesCombo();
	char * DisplayMeterTypesCombo();
	char * DisplayTimerTypesCombo();
	char * DisplayHardwareTypesCombo();
	char * DisplaySerialDevicesCombo();
	char * DisplayLanguageCombo();
	char * DisplayDevicesList();
	std::string GetJSonPage();
	std::string GetLanguage();
	std::string GetCameraSnapshot();
	std::string GetInternalCameraSnapshot();
	std::string GetDatabaseBackup();
	char * PostSettings();
	char * SetRFXCOMMode();
	char * SetRego6XXType();
	char * SetS0MeterType();
	char * SetLimitlessType();
	char * SetOpenThermSettings();
	char * SetP1USBType();
	char * RestoreDatabase();
	char * SMASpotImportOldData();

	cWebem *m_pWebEm;

	void LoadUsers();
	void SaveUsers();
	void AddUser(const unsigned long ID, const std::string &username, const std::string &password, const int userrights);
	void ClearUserPasswords();
	bool FindAdminUser();
	int FindUser(const char* szUserName);
	void SetAuthenticationMethod(int amethod);
	std::vector<_tWebUserPassword> m_users;

	//JSon
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate);
private:
	void HandleCommand(const std::string &cparam, Json::Value &root);
	void HandleRType(const std::string &rtype, Json::Value &root);

	//Commands
	void Cmd_LoginCheck(Json::Value &root);
	void Cmd_AddHardware(Json::Value &root);
	void Cmd_UpdateHardware(Json::Value &root);
	void Cmd_DeleteHardware(Json::Value &root);
	void Cmd_WOLGetNodes(Json::Value &root);
	void Cmd_WOLAddNode(Json::Value &root);
	void Cmd_WOLUpdateNode(Json::Value &root);
	void Cmd_WOLRemoveNode(Json::Value &root);
	void Cmd_WOLClearNodes(Json::Value &root);
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
	void Cmd_GetActualHistory(Json::Value &root);
	void Cmd_GetNewHistory(Json::Value &root);
	void Cmd_GetActiveTabs(Json::Value &root);
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

	//RTypes
	void RType_HandleGraph(Json::Value &root);
	void RType_LightLog(Json::Value &root);
	void RType_Settings(Json::Value &root);
	void RType_Events(Json::Value &root);
	void RType_Hardware(Json::Value &root);
	void RType_Devices(Json::Value &root);
	void RType_Cameras(Json::Value &root);
	void RType_Users(Json::Value &root);
	void RType_Timers(Json::Value &root);
	void RType_SceneTimers(Json::Value &root);
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
#ifdef WITH_OPENZWAVE
	//ZWave
	void ZWaveUpdateNode(Json::Value &root);
	void ZWaveDeleteNode(Json::Value &root);
	void ZWaveInclude(Json::Value &root);
	void ZWaveExclude(Json::Value &root);
	void ZWaveSoftReset(Json::Value &root);
	void ZWaveHardReset(Json::Value &root);
	void ZWaveNetworkHeal(Json::Value &root);
	void ZWaveNodeHeal(Json::Value &root);
	void ZWaveNetworkInfo(Json::Value &root);
	void ZWaveRemoveGroupNode(Json::Value &root);
	void ZWaveAddGroupNode(Json::Value &root);
	void ZWaveGroupInfo(Json::Value &root);
	void ZWaveCancel(Json::Value &root);
	void ApplyZWaveNodeConfig(Json::Value &root);
	void RequestZWaveNodeConfig(Json::Value &root);
	void ZWaveStateCheck(Json::Value &root);
	void ZWaveRequestNodeConfig(Json::Value &root);
	void ZWaveReceiveConfigurationFromOtherController(Json::Value &root);
	void ZWaveSendConfigurationToSecondaryController(Json::Value &root);
	void ZWaveTransferPrimaryRole(Json::Value &root);
	std::string ZWaveGetConfigFile();
	void ZWaveSetUserCodeEnrollmentMode(Json::Value &root);
	void ZWaveGetNodeUserCodes(Json::Value &root);
	void ZWaveRemoveUserCode(Json::Value &root);
	//RTypes
	void RType_OpenZWaveNodes(Json::Value &root);
#endif	
	boost::shared_ptr<boost::thread> m_thread;
	std::map < std::string, webserver_response_function > m_webcommands;
	std::map < std::string, webserver_response_function > m_webrtypes;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;
	time_t m_LastUpdateCheck;
	std::vector<_tCustomIcon> m_custom_light_icons;
};

} //server
}//http
