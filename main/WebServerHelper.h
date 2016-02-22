#pragma once
#include "WebServer.h"
#ifndef NOCLOUD
#include "../webserver/proxyclient.h"
#include "../hardware/DomoticzTCP.h"
#endif
#include "../tcpserver/TCPServer.h"
#include <vector>

namespace http {
	namespace server {

		class CWebServerHelper {
		public:
			CWebServerHelper();
			~CWebServerHelper();

			// called from mainworker():
			bool StartServers(const std::string &listenaddress, const std::string &listenport, const std::string &secure_listenport, const std::string &serverpath, const std::string &secure_cert_file, const std::string &secure_cert_passphrase, const bool bIgnoreUsernamePassword, tcp::server::CTCPServer *sharedServer);
			void StopServers();
#ifndef NOCLOUD
			void RestartProxy();
			boost::shared_ptr<CProxyClient> GetProxyForMaster(DomoticzTCP *master);
			void RemoveMaster(DomoticzTCP *master);
#endif
			void SetAuthenticationMethod(int amethod);
			void SetWebTheme(const std::string &themename);
			void SetWebRoot(const std::string &webRoot);
			void ClearUserPasswords();
			// called from OTGWBase()
			void GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const std::string &username);
			// called from CSQLHelper
			void ReloadCustomSwitchIcons();
		private:
			CWebServer *plainServer_;
#ifdef NS_ENABLE_SSL
			CWebServer *secureServer_;
#endif
			tcp::server::CTCPServer *m_pDomServ;
			std::vector<CWebServer*> serverCollection;

			std::string our_serverpath;
#ifndef NOCLOUD
			std::vector<CProxyManager*> proxymanagerCollection;
			int GetNrMyDomoticzThreads();

#endif
};

	} // end namespace server
} // end namespace http
