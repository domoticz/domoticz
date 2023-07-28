#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "server.hpp"
#include "session_store.hpp"

namespace http
{
	namespace server
	{
		enum _eUserRights
		{
			URIGHTS_VIEWER = 0,
			URIGHTS_SWITCHER,
			URIGHTS_ADMIN,
			URIGHTS_CLIENTID=255
		};
		enum _eAuthenticationMethod
		{
			AUTH_LOGIN = 0,
			AUTH_BASIC,
		};
		enum _eWebCompressionMode
		{
			WWW_USE_GZIP = 0,
			WWW_USE_STATIC_GZ_FILES,
			WWW_FORCE_NO_GZIP_SUPPORT
		};
		typedef struct _tWebUserPassword
		{
			unsigned long ID;
			std::string Username;
			std::string Password;
			std::string Mfatoken;
			std::string PrivKey;
			std::string PubKey;
			_eUserRights userrights = URIGHTS_VIEWER;
			int TotSensors = 0;
			int ActiveTabs = 0;
		} WebUserPassword;

		typedef struct _tWebEmSession
		{
			std::string id;
			std::string remote_host;
			std::string local_host;
			std::string remote_port;
			std::string local_port;
			std::string auth_token;
			std::string username;
			int reply_status = 0;
			time_t timeout = 0;
			time_t expires = 0;
			int rights = 0;
			bool rememberme = false;
			bool isnew = false;
		} WebEmSession;

		typedef struct _tIPNetwork
		{
			bool bIsIPv6 = false;
			std::string ip_string;
			uint8_t Network[16] = { 0 };
			uint8_t Mask[16] = { 0 };
		} IPNetwork;

		// Parsed Authorization header (RFC2617)
		struct ah {
			std::string method;		// HTTP request method
			std::string user;		// Username
			std::string response;	// Response with the request-digest
			std::string uri;		// Digest-Uri
			std::string cnonce;		// Client Nonce
			std::string qop;		// Quality of Protection
			std::string nc;			// Nonce Count
			std::string nonce;		// Nonce
			std::string ha1;		// A1 = unq(username-value) ":" unq(realm-value) ":" passwd
		};

		/**

		The link between the embedded web server and the application code


		* Copyright (c) 2008 by James Bremner
		* All rights reserved.
		*
		* Use license: Modified from standard BSD license.
		*
		* Redistribution and use in source and binary forms are permitted
		* provided that the above copyright notice and this paragraph are
		* duplicated in all such forms and that any documentation, advertising
		* materials, Web server pages, and other materials related to such
		* distribution and use acknowledge that the software was developed
		* by James Bremner. The name "James Bremner" may not be used to
		* endorse or promote products derived from this software without
		* specific prior written permission.
		*
		* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
		* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
		* WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.



		*/
		class cWebem;
		typedef std::function<void(std::string &content_part)> webem_include_function;
		typedef std::function<void(WebEmSession &session, const request &req, std::string &redirecturi)> webem_action_function;
		typedef std::function<void(WebEmSession &session, const request &req, reply &rep)> webem_page_function;

		/**

		The webem request handler.

		A specialization of the boost::asio request handler

		Application code should not use this class.

		*/
		class cWebemRequestHandler : public request_handler
		{
		      public:
			/// Construct with a directory containing files to be served.
			cWebemRequestHandler(const std::string &doc_root, cWebem *webem)
				: request_handler(doc_root, webem)
			{
			}

			/// Handle a request and produce a reply.
			void handle_request(const request &req, reply &rep) override;
			bool CheckUserAuthorization(std::string &user, const request &req);

				private:
			char *strftime_t(const char *format, time_t rawtime);
			bool CompressWebOutput(const request &req, reply &rep);
			/// Websocket methods
			bool is_upgrade_request(WebEmSession &session, const request &req, reply &rep);
			std::string compute_accept_header(const std::string &websocket_key);
			bool CheckAuthByPass(const request& req);
			bool CheckAuthentication(WebEmSession &session, const request &req, reply &rep);
			bool CheckUserAuthorization(std::string &user, struct ah *ah);
			bool AllowBasicAuth();
			void send_authorization_request(reply &rep);
			void send_remove_cookie(reply &rep);
			std::string generateSessionID();
			void send_cookie(reply &rep, const WebEmSession &session);
			bool parse_cookie(const request &req, std::string &sSID, std::string &sAuthToken, std::string &szTime, bool &expired);
			bool AreWeInTrustedNetwork(const std::string &sHost);
			bool IsIPInRange(const std::string &ip, const _tIPNetwork &ipnetwork, const bool &bIsIPv6);
			int authorize(WebEmSession &session, const request &req, reply &rep);
			void Logout();
			int parse_auth_header(const request &req, struct ah *ah);
			std::string generateAuthToken(const WebEmSession &session, const request &req);
			bool checkAuthToken(WebEmSession &session);
			void removeAuthToken(const std::string &sessionId);
			int check_password(struct ah *ah, const std::string &ha1);
		};

		/**
		The webem embedded web server.
		*/
		class cWebem
		{
		      public:
			cWebem(const server_settings &settings, const std::string &doc_root);
			~cWebem();
			void Run();
			void Stop();

			// 20230525 No longer in Use! Will be removed soon!
			//void RegisterIncludeCode(const char *idname, const webem_include_function &fun);
			//bool Include(std::string &reply);

			void RegisterPageCode(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication = false);

			void RegisterActionCode(const char *idname, const webem_action_function &fun);

			void RegisterWhitelistURLString(const char *idname);
			void RegisterWhitelistCommandsString(const char *idname);

			void DebugRegistrations();

			bool ExtractPostData(request &req, const char *pContent_Type);

			bool IsAction(const request &req);
			bool CheckForAction(WebEmSession &session, request &req);

			bool IsPageOverride(const request &req, reply &rep);
			bool CheckForPageOverride(WebEmSession &session, request &req, reply &rep);

			void SetAuthenticationMethod(_eAuthenticationMethod amethod);
			void SetWebTheme(const std::string &themename);
			void SetWebRoot(const std::string &webRoot);
			void AddUserPassword(unsigned long ID, const std::string &username, const std::string &password, const std::string &mfatoken, _eUserRights userrights, int activetabs, const std::string &privkey = "", const std::string &pubkey = "");
			std::string ExtractRequestPath(const std::string &original_request_path);
			bool IsBadRequestPath(const std::string &original_request_path);

			bool GenerateJwtToken(std::string &jwttoken, const std::string &clientid, const std::string &clientsecret, const std::string &user, const uint32_t exptime, const Json::Value jwtpayload = "");
			bool FindAuthenticatedUser(std::string &user, const request &req, reply &rep);
			bool CheckVHost(const request &req);
			bool findRealHostBehindProxies(const request &req, std::string &realhost);
			static bool isValidIP(std::string& ip);

			void ClearUserPasswords();
			std::vector<_tWebUserPassword> m_userpasswords;
			void AddTrustedNetworks(std::string network);
			void ClearTrustedNetworks();
			std::vector<_tIPNetwork> m_localnetworks;
			void SetDigistRealm(const std::string &realm);
			std::string m_DigistRealm;
			void SetAllowPlainBasicAuth(const bool bAllow);
			bool m_AllowPlainBasicAuth;
			void SetZipPassword(const std::string &password);

			// Session store manager
			void SetSessionStore(session_store_impl_ptr sessionStore);
			session_store_impl_ptr GetSessionStore();

			std::string m_zippassword;
			std::string GetPort();
			std::string GetWebRoot();
			WebEmSession *GetSession(const std::string &ssid);
			void AddSession(const WebEmSession &session);
			void RemoveSession(const WebEmSession &session);
			void RemoveSession(const std::string &ssid);
			std::vector<std::string> GetExpiredSessions();
			int CountSessions();
			_eAuthenticationMethod m_authmethod;
			// Whitelist url strings that bypass authentication checks (not used by basic-auth authentication)
			std::vector<std::string> myWhitelistURLs;
			std::vector<std::string> myWhitelistCommands;
			std::map<std::string, WebEmSession> m_sessions;
			server_settings m_settings;
			// actual theme selected
			std::string m_actTheme;

			void SetWebCompressionMode(_eWebCompressionMode gzmode);
			_eWebCompressionMode m_gzipmode;

		      private:
			/// store map between include codes and application functions (20230525 No longer in use! Will be removed soon!)
			// std::map<std::string, webem_include_function> myIncludes;
			/// store map between action codes and application functions
			std::map<std::string, webem_action_function> myActions;
			/// store name walue pairs for form submit action
			std::map<std::string, webem_page_function> myPages;

			void CleanSessions();
			bool sumProxyHeader(const std::string &sHeader, const request &req, std::vector<std::string> &vHeaderLines);
			bool parseProxyHeader(const std::vector<std::string> &vHeaderLines, std::vector<std::string> &vHosts);
			bool parseForwardedProxyHeader(const std::vector<std::string> &vHeaderLines, std::vector<std::string> &vHosts);
			session_store_impl_ptr mySessionStore; /// session store
			/// request handler specialized to handle webem requests
			/// Rene: Beware: myRequestHandler should be declared BEFORE myServer
			cWebemRequestHandler myRequestHandler;
			/// boost::asio web server (RK: plain or secure)
			std::shared_ptr<server_base> myServer;
			// root of url
			std::string m_webRoot;
			/// sessions management
			std::mutex m_sessionsMutex;
			boost::asio::io_service m_io_service;
			boost::asio::deadline_timer m_session_clean_timer;
			std::shared_ptr<std::thread> m_io_service_thread;
		};

	} // namespace server
} // namespace http
