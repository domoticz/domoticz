#pragma once
#include "WebServer.h"
#include "../tcpserver/TCPServer.h"

namespace http {
	namespace server {
		class CWebServerHelper {
		public:
			CWebServerHelper();
			~CWebServerHelper();

			// called from mainworker():
#ifdef WWW_ENABLE_SSL
			bool StartServers(server_settings &web_settings, ssl_server_settings &secure_web_settings, iamserver::iam_settings & iam_settings, const std::string &serverpath, bool bIgnoreUsernamePassword);
#else
			bool StartServers(server_settings & web_settings, iamserver::iam_settings & iam_settings, const std::string &serverpath, const bool bIgnoreUsernamePassword);
#endif
			void StopServers();
			void SetWebCompressionMode(_eWebCompressionMode gzmode);
			void SetAllowPlainBasicAuth(const bool allow);
			void SetWebTheme(const std::string &themename);
			void SetWebRoot(const std::string &webRoot);
			void LoadUsers();
			void ClearUserPasswords();
			void ReloadTrustedNetworks();
			// called from OTGWBase()
			void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID,
					    const std::string &floorID, bool bDisplayHidden, bool bDisplayDisabled, bool bFetchFavorites, time_t LastUpdate, const std::string &username,
					    const std::string &hardwareid = "");
			// called from CSQLHelper
			void ReloadCustomSwitchIcons();
			std::string our_listener_port;
		private:
			std::shared_ptr<CWebServer> plainServer_;
#ifdef WWW_ENABLE_SSL
			std::shared_ptr<CWebServer> secureServer_;
#endif
			std::vector<std::shared_ptr<CWebServer> > serverCollection;

			std::string our_serverpath;
};

	} // end namespace server
} // end namespace http
