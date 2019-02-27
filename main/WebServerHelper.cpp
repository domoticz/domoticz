#include "stdafx.h"
#include "WebServerHelper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"

namespace http {
	namespace server {

		typedef std::vector<std::shared_ptr<CWebServer> >::iterator server_iterator;
#ifndef NOCLOUD
		extern CProxySharedData sharedData;
#endif

		CWebServerHelper::CWebServerHelper()
		{
			m_pDomServ = NULL;
		}

		CWebServerHelper::~CWebServerHelper()
		{
			StopServers();
		}
#ifdef WWW_ENABLE_SSL
		bool CWebServerHelper::StartServers(server_settings & web_settings, ssl_server_settings & secure_web_settings, const std::string &serverpath, const bool bIgnoreUsernamePassword, tcp::server::CTCPServer *sharedServer)
#else
		bool CWebServerHelper::StartServers(server_settings & web_settings, const std::string &serverpath, const bool bIgnoreUsernamePassword, tcp::server::CTCPServer *sharedServer)
#endif
		{
			bool bRet = false;

			m_pDomServ = sharedServer;

			our_serverpath = serverpath;
			plainServer_.reset(new CWebServer());
			serverCollection.push_back(plainServer_);
			bRet |= plainServer_->StartServer(web_settings, serverpath, bIgnoreUsernamePassword);
			our_listener_port = web_settings.listening_port;
#ifdef WWW_ENABLE_SSL
			if (secure_web_settings.is_enabled()) {
				SSL_library_init();
				secureServer_.reset(new CWebServer());
				bRet |= secureServer_->StartServer(secure_web_settings, serverpath, bIgnoreUsernamePassword);
				serverCollection.push_back(secureServer_);
			}
#endif

#ifndef NOCLOUD
			// start up the mydomoticz proxy client
			RestartProxy();
#endif

			return bRet;
		}

		void CWebServerHelper::StopServers()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->StopServer();
			}
			serverCollection.clear();
			plainServer_.reset();
#ifdef WWW_ENABLE_SSL
			secureServer_.reset();
#endif

#ifndef NOCLOUD
			proxymanager.Stop();
#endif
		}

#ifndef NOCLOUD
		void CWebServerHelper::RestartProxy() {
			sharedData.StopTCPClients();
			proxymanager.Stop();
			// restart
			cWebem *my_pWebEm = (plainServer_ != NULL ? plainServer_->m_pWebEm : (secureServer_ != NULL ? secureServer_->m_pWebEm : NULL));
			if (my_pWebEm == NULL) {
				_log.Log(LOG_ERROR, "No servers are configured. Hence mydomoticz will not be started either (if configured)");
				return;
			}
			if (proxymanager.Start(my_pWebEm, m_pDomServ)) {
				_log.Log(LOG_STATUS, "Proxymanager started.");
			}
		}

		CProxyClient *CWebServerHelper::GetProxyForMaster(DomoticzTCP *master) {
			return proxymanager.GetProxyForMaster(master);
		}

		void CWebServerHelper::RemoveMaster(DomoticzTCP *master) {
			sharedData.RemoveTCPClient(master);
		}
#endif

		void CWebServerHelper::SetWebCompressionMode(const _eWebCompressionMode gzmode)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetWebCompressionMode(gzmode);
			 }
		}

		void CWebServerHelper::SetAuthenticationMethod(const _eAuthenticationMethod amethod)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetAuthenticationMethod(amethod);
			 }
		}

		void CWebServerHelper::SetWebTheme(const std::string &themename)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetWebTheme(themename);
			 }
		}

		void CWebServerHelper::SetWebRoot(const std::string &webRoot)
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->SetWebRoot(webRoot);
			 }
			proxymanager.SetWebRoot(webRoot);
		}

		void CWebServerHelper::ClearUserPasswords()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ClearUserPasswords();
			 }
		}

		//JSon
		void CWebServerHelper::	GetJSonDevices(
			Json::Value &root,
			const std::string &rused,
			const std::string &rfilter,
			const std::string &order,
			const std::string &rowid,
			const std::string &planID,
			const std::string &floorID,
			const bool bDisplayHidden,
			const bool bDisplayDisabled,
			const bool bFetchFavorites,
			const time_t LastUpdate,
			const std::string &username,
			const std::string &hardwareid) // OTO
		{
			if (plainServer_) { // assert
				plainServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, bDisplayDisabled, bFetchFavorites, LastUpdate, username, hardwareid);
			}
#ifdef WWW_ENABLE_SSL
			else if (secureServer_) {
				secureServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, bDisplayDisabled, bFetchFavorites, LastUpdate, username, hardwareid);
			}
#endif
		}

		void CWebServerHelper::ReloadCustomSwitchIcons()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ReloadCustomSwitchIcons();
			 }
		}
	}

}
