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
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const bool bDisplayHidden, const time_t LastUpdate);
private:
	void HandleCommand(const std::string &cparam, Json::Value &root);
	void HandleRType(const std::string &rtype, Json::Value &root);

	//Commands
	void CmdLoginCheck(Json::Value &root);
	void CmdAddHardware(Json::Value &root);
	void CmdUpdateHardware(Json::Value &root);
	void DeleteHardware(Json::Value &root);
	void WOLGetNodes(Json::Value &root);
	void WOLAddNode(Json::Value &root);
	void WOLUpdateNode(Json::Value &root);
	void WOLRemoveNode(Json::Value &root);
	void WOLClearNodes(Json::Value &root);
	void SaveFibaroLinkConfig(Json::Value &root);
	void GetFibaroLinkConfig(Json::Value &root);
	void GetFibaroLinks(Json::Value &root);
	void SaveFibaroLink(Json::Value &root);
	void DeleteFibaroLink(Json::Value &root);
	void GetDevicesForFibaroLink(Json::Value &root);
	void GetDeviceValueOptions(Json::Value &root);
	void GetDeviceValueOptionWording(Json::Value &root);
	void DeleteUserVariable(Json::Value &root);
	void SaveUserVariable(Json::Value &root);
	void UpdateUserVariable(Json::Value &root);
	void GetUserVariables(Json::Value &root);
	void GetUserVariable(Json::Value &root);

	//RTypes
	void RType_HandleGraph(Json::Value &root);

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
