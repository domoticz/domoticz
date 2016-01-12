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
#ifdef NS_ENABLE_SSL
			secureServer_ = NULL;
#endif
			plainServer_ = NULL;

			m_pDomServ = NULL;
		}

		CWebServerHelper::~CWebServerHelper()
		{
#ifdef NS_ENABLE_SSL
			if (secureServer_) delete secureServer_;
#endif
			if (plainServer_) delete plainServer_;

#ifndef NOCLOUD
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				delete (*it);
			}
#endif
		}

		bool CWebServerHelper::StartServers(const std::string &listenaddress, const std::string &listenport, const std::string &secure_listenport, const std::string &serverpath, const std::string &secure_cert_file, const std::string &secure_cert_passphrase, const bool bIgnoreUsernamePassword, tcp::server::CTCPServer *sharedServer)
		{
			bool bRet = false;

			m_pDomServ = sharedServer;

#ifdef NS_ENABLE_SSL
			SSL_library_init();
			serverCollection.resize(secure_listenport.empty() ? 1 : 2);
#else
			serverCollection.resize(1);
#endif
			our_serverpath = serverpath;
			plainServer_ = new CWebServer();
			serverCollection[0] = plainServer_;
			bRet |= plainServer_->StartServer(listenaddress, listenport, serverpath, bIgnoreUsernamePassword);
#ifdef NS_ENABLE_SSL
			if (!secure_listenport.empty()) {
				secureServer_ = new CWebServer();
				bRet |= secureServer_->StartServer(listenaddress, secure_listenport, serverpath, bIgnoreUsernamePassword, secure_cert_file, secure_cert_passphrase);
				serverCollection[1] = secureServer_;
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
#ifndef NOCLOUD
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				(*it)->Stop();
			}
#endif
		}

#ifndef NOCLOUD
		void CWebServerHelper::RestartProxy() {
			sharedData.StopTCPClients();
			for (proxy_iterator it = proxymanagerCollection.begin(); it != proxymanagerCollection.end(); ++it) {
				(*it)->Stop();
				// todo: This seems to crash on a Pi (fatal signal 6). Windows goes fine.
				// stop old threads first
				//delete (*it);
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

		CProxyClient *CWebServerHelper::GetProxyForMaster(DomoticzTCP *master) {
			if (proxymanagerCollection.size() > 0) {
				// todo: make this a random connection?
				return proxymanagerCollection[0]->GetProxyForMaster(master);
			}
			// we are not connected yet. save this master and connect later.
			sharedData.AddTCPClient(master);
			return NULL;
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
		void CWebServerHelper::	GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate, const std::string &username)
		{
			if (plainServer_) { // assert
				plainServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate, username);
			}
#ifdef NS_ENABLE_SSL
			else if (secureServer_) {
				secureServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate, username);
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
