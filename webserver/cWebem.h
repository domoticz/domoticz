#pragma once

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include "server.hpp"
#include "session_store.hpp"

namespace http {
	namespace server {
		enum _eUserRights
		{
			URIGHTS_VIEWER=0,
			URIGHTS_SWITCHER,
			URIGHTS_ADMIN
		};
		enum _eAuthenticationMethod
		{
			AUTH_LOGIN=0,
			AUTH_BASIC,
		};
		enum _eWebCompressionMode
		{
			WWW_USE_GZIP=0,
			WWW_USE_STATIC_GZ_FILES,
			WWW_FORCE_NO_GZIP_SUPPORT
		};
		typedef struct _tWebUserPassword
		{
			unsigned long ID;
			std::string Username;
			std::string Password;
			_eUserRights userrights;
			int TotSensors;
			int ActiveTabs;
		} WebUserPassword;

		typedef struct _tWebEmSession
		{
			std::string id;
			std::string remote_host;
			std::string auth_token;
			std::string username;
			int reply_status;
			time_t timeout;
			time_t expires;
			int rights;
			bool rememberme;
			bool isnew;
			bool forcelogin;
		} WebEmSession;

		typedef struct _tIPNetwork
		{
			bool bIsIPv6 = false;
			uint8_t Network[16] = { 0 };
			uint8_t Mask[16] = { 0 };
		} IPNetwork;

		// Parsed Authorization header
		struct ah {
			std::string method;
			std::string user;
			std::string response;
			std::string uri;
			std::string cnonce;
			std::string qop;
			std::string nc;
			std::string nonce;
			std::string ha1;
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
		typedef boost::function< void( std::string & content_part ) > webem_include_function;
		typedef boost::function< void( std::wstring & content_part_w ) > webem_include_function_w;
		typedef boost::function< void( WebEmSession & session, const request& req, std::string & redirecturi ) > webem_action_function;
		typedef boost::function< void( WebEmSession & session, const request & req, reply & rep ) > webem_page_function;


		/**

		The webem request handler.

		A specialization of the boost::asio request handler

		Application code should not use this class.

		*/
		class cWebemRequestHandler : public request_handler
		{
		public:
			/// Construct with a directory containing files to be served.
			cWebemRequestHandler( const std::string& doc_root, cWebem* webem ) :
				request_handler( doc_root, webem )
				{}

			/// Handle a request and produce a reply.
				void handle_request(const request &req, reply &rep) override;

			      private:
				char *strftime_t(const char *format, time_t rawtime);
				bool CompressWebOutput(const request &req, reply &rep);
				/// Websocket methods
				bool is_upgrade_request(WebEmSession &session, const request &req, reply &rep);
				std::string compute_accept_header(const std::string &websocket_key);
				bool CheckAuthentication(WebEmSession &session, const request &req, reply &rep);
				void send_authorization_request(reply &rep);
				void send_remove_cookie(reply &rep);
				std::string generateSessionID();
				void send_cookie(reply &rep, const WebEmSession &session);
				bool AreWeInLocalNetwork(const std::string &sHost, const request &req);
				int authorize(WebEmSession &session, const request &req, reply &rep);
				void Logout();
				int parse_auth_header(const request &req, struct ah *ah);
				std::string generateAuthToken(const WebEmSession &session, const request &req);
				bool checkAuthToken(WebEmSession &session);
				void removeAuthToken(const std::string &sessionId);
	};
		// forward declaration for friend declaration
		class CProxyClient;
		/**

		The webem embedded web server.

		*/
		class cWebem
		{
		friend class CProxyClient;
		public:
			cWebem(
				const server_settings & settings,
				const std::string& doc_root);
			~cWebem();
			void Run();
			void Stop();

			void RegisterIncludeCode(const char *idname, const webem_include_function &fun);

			void RegisterIncludeCodeW(const char *idname, const webem_include_function_w &fun);

			void RegisterPageCode(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication = false);
			void RegisterPageCodeW(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication = false);

			bool Include( std::string& reply );

			void RegisterActionCode(const char *idname, const webem_action_function &fun);

			void RegisterWhitelistURLString(const char* idname);
			void RegisterWhitelistCommandsString(const char* idname);

			bool IsAction(const request& req);
			bool CheckForAction(WebEmSession & session, request& req);

			bool IsPageOverride(const request& req, reply& rep);
			bool CheckForPageOverride(WebEmSession & session, request& req, reply& rep);

			void SetAuthenticationMethod(_eAuthenticationMethod amethod);
			void SetWebTheme(const std::string &themename);
			void SetWebRoot(const std::string &webRoot);
			void AddUserPassword(unsigned long ID, const std::string &username, const std::string &password, _eUserRights userrights, int activetabs);
			std::string ExtractRequestPath(const std::string& original_request_path);
			bool IsBadRequestPath(const std::string& original_request_path);

			void ClearUserPasswords();
			std::vector<_tWebUserPassword> m_userpasswords;
			void AddLocalNetworks(std::string network);
			void ClearLocalNetworks();
			std::vector<_tIPNetwork> m_localnetworks;
			void SetDigistRealm(const std::string &realm);
			std::string m_DigistRealm;
			void SetZipPassword(const std::string &password);

			//IPs that are allowed to pass proxy headers
			std::vector < std::string > myRemoteProxyIPs;
			void AddRemoteProxyIPs(const std::string &ipaddr);
			void ClearRemoteProxyIPs();

			// Session store manager
			void SetSessionStore(session_store_impl_ptr sessionStore);
			session_store_impl_ptr GetSessionStore();

			std::string m_zippassword;
			std::string GetPort();
			std::string GetWebRoot();
			WebEmSession * GetSession(const std::string & ssid);
			void AddSession(const WebEmSession & session);
			void RemoveSession(const WebEmSession & session);
			void RemoveSession(const std::string & ssid);
			std::vector<std::string> GetExpiredSessions();
			int CountSessions();
			_eAuthenticationMethod m_authmethod;
			//Whitelist url strings that bypass authentication checks (not used by basic-auth authentication)
			std::vector < std::string > myWhitelistURLs;
			std::vector < std::string > myWhitelistCommands;
			std::map<std::string, WebEmSession> m_sessions;
			server_settings m_settings;
			// actual theme selected
			std::string m_actTheme;

			void SetWebCompressionMode(_eWebCompressionMode gzmode);
			_eWebCompressionMode m_gzipmode;
		private:
			/// store map between include codes and application functions
			std::map < std::string, webem_include_function > myIncludes;
			/// store map between include codes and application functions returning UTF-16 strings
			std::map < std::string, webem_include_function_w > myIncludes_w;
			/// store map between action codes and application functions
			std::map < std::string, webem_action_function > myActions;
			/// store name walue pairs for form submit action
			std::map < std::string, webem_page_function > myPages;
			/// store map between pages and application functions
			std::map < std::string, webem_page_function > myPages_w;
			void CleanSessions();
			session_store_impl_ptr mySessionStore; /// session store
			/// request handler specialized to handle webem requests
			/// Rene: Beware: myRequestHandler should be declared BEFORE myServer
			cWebemRequestHandler myRequestHandler;
			/// boost::asio web server (RK: plain or secure)
			std::shared_ptr<server_base> myServer;
			// root of url for reverse proxy servers
			std::string m_webRoot;
			/// sessions management
			std::mutex m_sessionsMutex;
			boost::asio::io_service m_io_service;
			boost::asio::deadline_timer m_session_clean_timer;
			std::shared_ptr<std::thread> m_io_service_thread;
		};

	} // namespace server
} // namespace http
