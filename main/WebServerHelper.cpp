#include "stdafx.h"
#include "WebServerHelper.h"
#include "../main/SQLHelper.h"

namespace http {
	namespace server {

		typedef std::vector<CWebServer*>::iterator server_iterator;
#ifndef NOCLOUD
		typedef std::vector<CProxyManager*>::iterator proxy_iterator;
		extern CProxySharedData sharedData;
#endif

		CWebServerHelper::CWebServerHelper()
		{
#ifdef WWW_ENABLE_SSL
			secureServer_ = NULL;
#endif
			plainServer_ = NULL;

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
			plainServer_ = new CWebServer();
			serverCollection.push_back(plainServer_);
			bRet |= plainServer_->StartServer(web_settings, serverpath, bIgnoreUsernamePassword);
#ifdef WWW_ENABLE_SSL
			if (secure_web_settings.is_enabled()) {
				SSL_library_init();
				secureServer_ = new CWebServer();
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
				((CWebServer*) *it)->StopServer();
				if (*it) delete (*it);
			}
			serverCollection.clear();
			plainServer_ = NULL; // avoid double free
#ifdef WWW_ENABLE_SSL
			secureServer_ = NULL; // avoid double free
#endif

#ifndef NOCLOUD
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				((CProxyManager*) *it)->Stop();
				if (*it) delete (*it);
			}
			proxymanagerCollection.clear();
#endif
		}

#ifndef NOCLOUD
		void CWebServerHelper::RestartProxy() {
			sharedData.StopTCPClients();
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				(*it)->Stop();
				delete (*it);
			}

			// restart threads
			const unsigned int connections = GetNrMyDomoticzThreads();
			proxymanagerCollection.resize(connections);
			for (unsigned int i = 0; i < connections; i++) {
				proxymanagerCollection[i] = new CProxyManager(our_serverpath, plainServer_->m_pWebEm, m_pDomServ);
				proxymanagerCollection[i]->Start(i == 0);
			}
			_log.Log(LOG_STATUS, "Proxymanager started.");
		}

		boost::shared_ptr<CProxyClient> CWebServerHelper::GetProxyForMaster(DomoticzTCP *master) {
			if (proxymanagerCollection.size() > 0) {
				// todo: make this a random connection?
				return proxymanagerCollection[0]->GetProxyForMaster(master);
			}
			// we are not connected yet. save this master and connect later.
			sharedData.AddTCPClient(master);
			return boost::shared_ptr<CProxyClient>();
		}

		void CWebServerHelper::RemoveMaster(DomoticzTCP *master) {
			sharedData.RemoveTCPClient(master);
		}
#endif

		void CWebServerHelper::SetAuthenticationMethod(int amethod)
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
			const bool bFetchFavorites,
			const time_t LastUpdate,
			const std::string &username)
		{
			if (plainServer_) { // assert
				plainServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, bFetchFavorites, LastUpdate, username);
			}
#ifdef WWW_ENABLE_SSL
			else if (secureServer_) {
				secureServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, bFetchFavorites, LastUpdate, username);
			}
#endif
		}

		void CWebServerHelper::ReloadCustomSwitchIcons()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ReloadCustomSwitchIcons();
			 }
		}

#ifndef NOCLOUD
		int CWebServerHelper::GetNrMyDomoticzThreads()
		{
			int nrThreads = 1; // default value
			m_sql.GetPreferencesVar("MyDomoticzNrThreads", nrThreads);
			return nrThreads;
		}
#endif
	}

}
