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
public:
	CWebServer(void);
	~CWebServer(void);
	bool StartServer(MainWorker *pMain, const std::string &listenaddress, const std::string &listenport, const std::string &serverpath, const bool bIgnoreUsernamePassword);
	void StopServer();

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
	MainWorker *m_pMain;
	boost::shared_ptr<boost::thread> m_thread;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;
	time_t m_LastUpdateCheck;
	std::vector<_tCustomIcon> m_custom_light_icons;

};

} //server
}//http
