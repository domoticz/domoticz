#pragma once

#include <string>
#include <vector>
#include "../json/config.h"
#include "../json/json.h"

class MainWorker;

namespace http {
	namespace server {
		class cWebem;
		struct _tWebUserPassword;
class CWebServer
{
public:
	CWebServer(void);
	~CWebServer(void);
	bool StartServer(MainWorker *pMain, std::string listenaddress, std::string listenport,std::string serverpath, bool bIgnoreUsernamePassword);
	void StopServer();

	char * DisplayVersion();
	char * DisplayHardwareCombo();
	char * DisplaySwitchTypesCombo();
	char * DisplayMeterTypesCombo();
	char * DisplayTimerTypesCombo();
	char * DisplayHardwareTypesCombo();
	char * DisplaySerialDevicesCombo();
	char * GetJSonPage();
	char * PostSettings();

	cWebem *m_pWebEm;

	void LoadUsers();
	void SaveUsers();
	int FindUser(const char* szUserName);
	std::vector<_tWebUserPassword> m_users;

private:
	MainWorker *m_pMain;
	boost::shared_ptr<boost::thread> m_thread;
	void Do_Work();
	std::string m_retstr;
	std::wstring m_wretstr;

	//JSon
	void GetJSonDevices(Json::Value &root, std::string rused, std::string rfilter, std::string order);
};

} //server
}//http
