#include "stdafx.h"
#include "WebServerHelper.h"


namespace http {
	namespace server {

		typedef std::vector<CWebServer*>::iterator server_iterator;

		CWebServerHelper::CWebServerHelper()
		{
#ifdef NS_ENABLE_SSL
			secureServer_ = NULL;
#endif
			plainServer_ = NULL;
		}

		CWebServerHelper::~CWebServerHelper()
		{
#ifdef NS_ENABLE_SSL
			if (secureServer_)
			{
				delete secureServer_;
				secureServer_ = NULL;
			}
#endif
			if (plainServer_)
			{
				delete plainServer_;
				plainServer_ = NULL;
			}
		}

		bool CWebServerHelper::StartServers(const std::string &listenaddress, const std::string &listenport, const std::string &secure_listenport, const std::string &serverpath, const std::string &secure_cert_file, const std::string &secure_cert_passphrase, const bool bIgnoreUsernamePassword)
		{
			bool bRet = false;
			plainServer_ = new CWebServer();
			serverCollection.push_back(plainServer_);
			bRet |= plainServer_->StartServer(listenaddress, listenport, serverpath, bIgnoreUsernamePassword);
#ifdef NS_ENABLE_SSL
			if (!secure_listenport.empty()) {
				secureServer_ = new CWebServer();
				bRet |= secureServer_->StartServer(listenaddress, secure_listenport, serverpath, bIgnoreUsernamePassword, secure_cert_file, secure_cert_passphrase);
				serverCollection.push_back(secureServer_);
			}
#endif
			return bRet;
		}

		void CWebServerHelper::StopServers()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->StopServer();
			 }
			serverCollection.clear();
		}

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

		void CWebServerHelper::ClearUserPasswords()
		{
			for (server_iterator it = serverCollection.begin(); it != serverCollection.end(); ++it) {
				(*it)->ClearUserPasswords();
			 }
		}

		//JSon
		void CWebServerHelper::GetJSonDevices(Json::Value &root, const std::string &rused, const std::string &rfilter, const std::string &order, const std::string &rowid, const std::string &planID, const std::string &floorID, const bool bDisplayHidden, const time_t LastUpdate)
		{
			if (plainServer_) { // assert
				plainServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate);
			}
#ifdef NS_ENABLE_SSL
			else if (secureServer_) {
				secureServer_->GetJSonDevices(root, rused, rfilter, order, rowid, planID, floorID, bDisplayHidden, LastUpdate);
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