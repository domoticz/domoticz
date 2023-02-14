#include "stdafx.h"
#include "WebServerHelper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"

namespace http {
	namespace server {

		CWebServerHelper::CWebServerHelper()
		{
		}

		CWebServerHelper::~CWebServerHelper()
		{
			StopServers();
		}
#ifdef WWW_ENABLE_SSL
		bool CWebServerHelper::StartServers(server_settings & web_settings, ssl_server_settings & secure_web_settings, iamserver::iam_settings & iam_settings, const std::string &serverpath, const bool bIgnoreUsernamePassword)
#else
		bool CWebServerHelper::StartServers(server_settings & web_settings, iamserver::iam_settings & iam_settings, const std::string &serverpath, const bool bIgnoreUsernamePassword)
#endif
		{
			bool bRet = false;

			our_serverpath = serverpath;
			plainServer_.reset(new CWebServer());
			if (iam_settings.is_enabled())
				plainServer_->SetIamSettings(iam_settings);
			bRet |= plainServer_->StartServer(web_settings, serverpath, bIgnoreUsernamePassword);
			if (bRet) {
				serverCollection.push_back(plainServer_);
				our_listener_port = web_settings.listening_port;
			}
#ifdef WWW_ENABLE_SSL
			if (secure_web_settings.is_enabled()) {
#if (OPENSSL_VERSION_NUMBER < 0x10100000L) || defined(LIBRESSL_VERSION_NUMBER)
				SSL_library_init();
#endif
				secureServer_.reset(new CWebServer());
				if (iam_settings.is_enabled())
					secureServer_->SetIamSettings(iam_settings);
				bRet |= secureServer_->StartServer(secure_web_settings, serverpath, bIgnoreUsernamePassword);
				if (bRet) {
					serverCollection.push_back(secureServer_);
				}
			}
#endif
			return bRet;
		}

		void CWebServerHelper::StopServers()
		{
			for (auto &it : serverCollection)
			{
				it->StopServer();
			}
			serverCollection.clear();
			plainServer_.reset();
#ifdef WWW_ENABLE_SSL
			secureServer_.reset();
#endif
		}

		void CWebServerHelper::SetWebCompressionMode(const _eWebCompressionMode gzmode)
		{
			for (auto &it : serverCollection)
			{
				it->SetWebCompressionMode(gzmode);
			}
		}

		void CWebServerHelper::SetAllowPlainBasicAuth(const bool allow)
		{
			for (auto &it : serverCollection)
			{
				it->SetAllowPlainBasicAuth(allow);
			}
		}

		void CWebServerHelper::SetWebTheme(const std::string &themename)
		{
			for (auto &it : serverCollection)
			{
				it->SetWebTheme(themename);
			}
		}

		void CWebServerHelper::SetWebRoot(const std::string &webRoot)
		{
			for (auto &it : serverCollection)
			{
				it->SetWebRoot(webRoot);
			}
		}

		void CWebServerHelper::LoadUsers()
		{
			for (auto &it : serverCollection)
			{
				it->LoadUsers();
			}
		}
		
		void CWebServerHelper::ClearUserPasswords()
		{
			for (auto &it : serverCollection)
			{
				it->ClearUserPasswords();
			}
		}

		void CWebServerHelper::ReloadTrustedNetworks()
		{
			std::string TrustedNetworks;
			m_sql.GetPreferencesVar("WebLocalNetworks", TrustedNetworks);

			for (auto &it : serverCollection)
			{
				if (it->m_pWebEm == nullptr)
					continue;
				it->m_pWebEm->ClearTrustedNetworks();

				std::vector<std::string> strarray;
				StringSplit(TrustedNetworks, ";", strarray);
				for (const auto &str : strarray)
					it->m_pWebEm->AddTrustedNetworks(str);
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
			for (auto &it : serverCollection)
			{
				it->ReloadCustomSwitchIcons();
			}
		}
	} // namespace server

} // namespace http
