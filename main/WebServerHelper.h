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
			void ReloadCorsPolicy();
			// Aggregate the recently-seen-clients snapshot across every server in
			// serverCollection (plain and, if enabled, secure). Each cWebem
			// instance now tracks its own clients rather than sharing one global
			// map, so a caller that wants the full picture across HTTP and HTTPS
			// has to combine them here.
			std::vector<connection::_tRemoteClients> GetRemoteClients() const;
			// called from OTGWBase()
			void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID,
					    const std::string &floorID, bool bDisplayHidden, bool bDisplayDisabled, bool bFetchFavorites, time_t LastUpdate, const std::string &username,
					    const std::string &hardwareid = "");
			// called from CSQLHelper
			void ReloadCustomSwitchIcons();
			CWebServer* GetAnyServer() const
		{
			if (plainServer_)
				return plainServer_.get();
#ifdef WWW_ENABLE_SSL
			if (secureServer_)
				return secureServer_.get();
#endif
			return nullptr;
		}
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
