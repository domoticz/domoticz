#pragma once
#include "WebServer.h"
#include <vector>

namespace http {
	namespace server {

		class CWebServerHelper {
		public:
			CWebServerHelper();
			~CWebServerHelper();

			// called from mainworker():
			bool StartServers(const std::string &listenaddress, const std::string &listenport, const std::string &secure_listenport, const std::string &serverpath, const std::string &secure_cert_file, const std::string &secure_cert_passphrase, const bool bIgnoreUsernamePassword);
			void StopServers();
			void SetAuthenticationMethod(int amethod);
			void SetWebTheme(const std::string &themename);
			void ClearUserPasswords();
			// called from OTGWBase()
			void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const bool bSkipUserCheck);
			// called from CSQLHelper
			void ReloadCustomSwitchIcons();
		private:
			CWebServer *plainServer_;
#ifdef NS_ENABLE_SSL
			CWebServer *secureServer_;
#endif
			std::vector<CWebServer*> serverCollection;
};

	} // end namespace server
} // end namespace http