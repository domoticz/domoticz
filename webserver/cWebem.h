#pragma once

#include <map>
#include <boost/function.hpp>
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
		typedef struct _tWebUserPassword
		{
			unsigned long ID;
			std::string Username;
			std::string Password;

			_eUserRights userrights;
			int ActiveTabs;
		} WebUserPassword;

		typedef struct _tWebEmSession
		{
			std::string id;
			std::string remote_host;
			std::string auth_token;
			std::string username;
			time_t expires;
			int rights;
			bool rememberme;
			bool isnew;
			bool removecookie;
			bool forcelogin;
			std::string lastRequestPath;
			std::string outputfilename;
		} WebEmSession;

		typedef struct _tIPNetwork
		{
			uint32_t network;
			uint32_t mask;
			std::string hostname;
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
		typedef boost::function< char*() > webem_include_function;
		typedef boost::function< wchar_t*() > webem_include_function_w;
		typedef boost::function< char*( WebEmSession & session, const request& ) > webem_action_function;
		typedef boost::function< std::string( WebEmSession & session, const request& ) > webem_page_function;
		typedef boost::function< wchar_t*( WebEmSession & session, const request& ) > webem_page_function_w;


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
				request_handler( doc_root, webem ),
				m_doc_root ( doc_root ),
				myWebem(webem)
				{}

			/// Handle a request and produce a reply.
			virtual void handle_request( const std::string &sHost, const request& req, reply& rep);
		private:
			char *strftime_t(const char *format, const time_t rawtime);
			bool CompressWebOutput(const request& req, reply& rep);
			bool CheckAuthentication(WebEmSession & session, const request& req, reply& rep);
			void send_authorization_request(reply& rep);
			void send_remove_cookie(reply& rep);
			std::string generateSessionID();
			void send_cookie(reply& rep, const WebEmSession & session);
			bool AreWeInLocalNetwork(const std::string &sHost, const request& req);
			int authorize(WebEmSession & session, const request& req, reply& rep);
			void Logout();
			int parse_auth_header(const request& req, struct ah *ah);
			std::string generateAuthToken(const WebEmSession & session, const request & req);
			bool checkAuthToken(WebEmSession & session);
			void removeAuthToken(const std::string & sessionId);
			std::string m_doc_root;
			// Webem link to application code
			cWebem* myWebem;
		};

		/**

		The webem embedded web server.

		*/
		class cWebem
		{
		public:
			cWebem(
				const std::string& address,
				const std::string& port,
				const std::string& doc_root,
				const std::string& secure_cert_file,
				const std::string& secure_cert_passphrase);

			void Run();
			void Stop();

			void RegisterIncludeCode(
				const char* idname,
				webem_include_function fun );
			
			void RegisterIncludeCodeW(
				const char* idname,
				webem_include_function_w fun );

			void RegisterPageCode(
				const char* pageurl,
				webem_page_function fun );
			void RegisterPageCodeW(
				const char* pageurl,
				webem_page_function_w fun );

			void Include( std::string& reply );

			void RegisterActionCode(
				const char* idname,
				webem_action_function fun );

			void RegisterWhitelistURLString(const char* idname);

			bool IsAction(const request& req);
			bool CheckForAction(WebEmSession & session, request& req);

			bool IsPageOverride(const request& req, reply& rep);
			bool CheckForPageOverride(WebEmSession & session, request& req, reply& rep);

			void SetAuthenticationMethod(const _eAuthenticationMethod amethod);
			void SetWebTheme(const std::string &themename);
			void AddUserPassword(const unsigned long ID, const std::string &username, const std::string &password, const _eUserRights userrights, const int activetabs);
			void ClearUserPasswords();
			std::vector<_tWebUserPassword> m_userpasswords;
			void AddLocalNetworks(std::string network);
			void ClearLocalNetworks();
			std::vector<_tIPNetwork> m_localnetworks;
			void SetDigistRealm(std::string realm);
			std::string m_DigistRealm;
			void SetZipPassword(std::string password);

			// Session store manager
			void SetSessionStore(session_store* sessionStore);
			session_store* GetSessionStore();

			std::string m_zippassword;
			std::map<std::string,WebEmSession> m_sessions;
			_eAuthenticationMethod m_authmethod;
			//Whitelist url strings that bypass authentication checks (not used by basic-auth authentication)
			std::vector < std::string > myWhitelistURLs;
			// actual theme selected
			std::string m_actTheme;
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
			std::map < std::string, webem_page_function_w > myPages_w;
			/// request handler specialized to handle webem requests
			cWebemRequestHandler myRequestHandler;
			/// boost::asio web server (RK: plain or secure)
			server myServer;
			/// port server is listening on
			std::string myPort;
			/// session store
			session_store* mySessionStore;
		};

	}
}


