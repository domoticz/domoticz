#pragma once

#include <string>
#include <vector>

class MainWorker;

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
	bool StartServer(MainWorker *pMain, const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword);
	void StopServer();
	void RegisterCommandCode(const char* idname, webserver_response_function ResponseFunction);
	std::string& GetWebValue( const char* name );

	char * DisplayVersion();
	char * DisplayHardwareCombo();
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
	void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID);
private:
	void HandleCommand(const std::string &cparam, Json::Value &root);

	//Commands
	void CmdLoginCheck(Json::Value &root);
	void CmdAddHardware(Json::Value &root);
	void CmdUpdateHardware(Json::Value &root);
	void DeleteHardware(Json::Value &root);
#ifdef WITH_OPENZWAVE
	//ZWave
	void ZWaveUpdateNode(Json::Value &root);
	void ZWaveDeleteNode(Json::Value &root);
	void ZWaveInclude(Json::Value &root);
	void ZWaveExclude(Json::Value &root);
	void ZWaveSoftReset(Json::Value &root);
	void ZWaveHardReset(Json::Value &root);
	void ZWaveNetworkHeal(Json::Value &root);
	void ZWaveNetworkInfo(Json::Value &root);
	void ZWaveRemoveGroupNode(Json::Value &root);
	void ZWaveAddGroupNode(Json::Value &root);
	void ZWaveGroupInfo(Json::Value &root);
	void ZWaveCancel(Json::Value &root);
	void ApplyZWaveNodeConfig(Json::Value &root);
	void RequestZWaveNodeConfig(Json::Value &root);
	void ZWaveStateCheck(Json::Value &root);
	
private:
	void HandleCommand(const std::string &cparam, Json::Value &root);
	void ZWaveRequestNodeConfig(Json::Value &root);
	void ZWaveReceiveConfigurationFromOtherController(Json::Value &root);
	void ZWaveSendConfigurationToSecondaryController(Json::Value &root);
	void ZWaveTransferPrimaryRole(Json::Value &root);
#endif	

	MainWorker *m_pMain;
	boost::shared_ptr<boost::thread> m_thread;
	std::map < std::string, webserver_response_function > m_webcommands;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;
	time_t m_LastUpdateCheck;
	std::vector<_tCustomIcon> m_custom_light_icons;

};

} //server
}//http
