//mainpage WEBEM
//
//Detailed class and method documentation of the WEBEM C++ embedded web server source code.
//
//Modified, extended etc by Robbert E. Peters/RTSS B.V.
#include "stdafx.h"
#include "cWebem.h"
#include "reply.hpp"
#include "request.hpp"
#include "mime_types.hpp"
#include "utf.hpp"
#include "Base64.h"
#include "sha1.hpp"
#include "GZipHelper.h"
#include <stdarg.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include "../main/Helper.h"
#include "../main/Logger.h"

#define JWT_DISABLE_BASE64
#include "../jwt-cpp/jwt.h"

#define SHORT_SESSION_TIMEOUT 600 // 10 minutes
#define LONG_SESSION_TIMEOUT (30 * 86400) // 30 days

#define websocket_protocol "domoticz"

int m_failcounter = 0;

namespace http {
	namespace server {

		/**
		Webem constructor

		@param[in] server_settings  Server settings (IP address, listening port, ssl options...)
		@param[in] doc_root path to folder containing html e.g. "./"
		*/
		cWebem::cWebem(const server_settings &settings, const std::string &doc_root)
			: m_DigistRealm("Domoticz.com")
			, m_authmethod(AUTH_LOGIN)
			, m_AllowPlainBasicAuth(false)
			, m_settings(settings)
			, mySessionStore(nullptr)
			, myRequestHandler(doc_root, this)
			// Rene, make sure we initialize m_sessions first, before starting a server
			, myServer(server_factory::create(settings, myRequestHandler))
			, m_io_service()
			, m_session_clean_timer(m_io_service, boost::posix_time::minutes(1))
		{
			// associate handler to timer and schedule the first iteration
			m_session_clean_timer.async_wait([this](auto &&) { CleanSessions(); });
			m_io_service_thread = std::make_shared<std::thread>([p = &m_io_service] { p->run(); });
			SetThreadName(m_io_service_thread->native_handle(), "Webem_ssncleaner");
		}

		cWebem::~cWebem()
		{
			// Remove reference to CWebServer before its deletion (fix a "pure virtual method called" exception on server termination)
			mySessionStore = nullptr;
			// Delete server (no need with smart pointer)
		}

		/**

		Start the server.

		This does not return.

		IMPORTANT: This method does not return. If application needs to continue, start new thread with call to this method.

		*/
		void cWebem::Run()
		{
			// Start Web server
			if (myServer != nullptr)
			{
				myServer->run();
			}
		}

		/**

		Stop and delete the internal server.

		IMPORTANT:  To start the server again, delete it and create a new cWebem instance.

		*/
		void cWebem::Stop()
		{
			// Stop session cleaner
			try
			{
				if (!m_io_service.stopped())
				{
					m_io_service.stop();
				}
				if (m_io_service_thread)
				{
					m_io_service_thread->join();
					m_io_service_thread.reset();
				}
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "[web:%s] exception thrown while stopping session cleaner", GetPort().c_str());
			}
			// Stop Web server
			if (myServer != nullptr)
			{
				myServer->stop();
			}
		}

		void cWebem::SetAuthenticationMethod(const _eAuthenticationMethod amethod)
		{
			m_authmethod = amethod;
		}

		void cWebem::SetWebCompressionMode(_eWebCompressionMode gzmode)
		{
			m_gzipmode = gzmode;
		}

		void cWebem::SetAllowPlainBasicAuth(const bool bAllow)
		{
			m_AllowPlainBasicAuth = bAllow;
		}

		/**

		Create a link between a string ID and a function to calculate the dynamic content of the string

		The function should return a pointer to a character buffer.  This should be contain only ASCII characters
		( Unicode code points 1 to 127 )

		@param[in] idname  string identifier
		@param[in] fun pointer to function which calculates the string to be displayed

		*/
		/* 20230525 No Longer in Use! Will be removed Soon!

		void cWebem::RegisterIncludeCode(const char *idname, const webem_include_function &fun)
		{
			myIncludes.insert(std::pair<std::string, webem_include_function >(std::string(idname), fun));
		}
		*/

		/**

		Create a link between a string ID and a function to calculate the dynamic content of the string

		The function should return a pointer to wide character buffer.  This should contain a wide character UTF-16 encoded unicode string.
		WEBEM will convert the string to UTF-8 encoding before sending to the browser.

		@param[in] idname  string identifier
		@param[in] fun pointer to function which calculates the string to be displayed

		*/

		void cWebem::RegisterPageCode(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication)
		{
			myPages.insert(std::pair<std::string, webem_page_function >(std::string(pageurl), fun));
			if (bypassAuthentication)
			{
				RegisterWhitelistURLString(pageurl);
			}
		}

		/**

		Specify link between form and application function to run when form submitted

		@param[in] idname string identifier
		@param[in] fun fpointer to function

		*/
		void cWebem::RegisterActionCode(const char *idname, const webem_action_function &fun)
		{
			myActions.insert(std::pair<std::string, webem_action_function >(std::string(idname), fun));
		}

		//Used by non basic-auth authentication (for example login forms) to bypass returning false when not authenticated
		void cWebem::RegisterWhitelistURLString(const char* idname)
		{
			myWhitelistURLs.push_back(idname);
		}
		void cWebem::RegisterWhitelistCommandsString(const char* idname)
		{
			myWhitelistCommands.push_back(idname);
		}

		// Show a Debug line with the registered functions, actions, includes, whitelist urls and commands
		void cWebem::DebugRegistrations()
		{
			_log.Debug(DEBUG_WEBSERVER, "cWebEm Registration: %d pages, %d actions, %d whitelist urls, %d whitelist commands",
				(int)myPages.size(), (int)myActions.size(), (int)myWhitelistURLs.size(), (int)myWhitelistCommands.size());
		}

		/**

		  Do not call from application code, used by server to include generated text.

		  @param[in/out] reply  text to include generated

		  The text is searched for "<!--#cWebemX-->".
		  The X can be any string not containing "-->"

		  If X has been registered with cWebem then the associated function
		  is called to generate text to be inserted.

		  returns true if any text is replaced


		*/
		/* 20230525 No longer in Use! Will be removed soon!
		bool cWebem::Include(std::string& reply)
		{
			bool res = false;
			size_t p = 0;
			while (true)
			{
				// find next request for generated text
				p = reply.find("<!--#embed", p);
				if (p == std::string::npos)
				{
					break;
				}
				size_t q = reply.find("-->", p);
				if (q == std::string::npos)
					break;
				q += 3;

				size_t reply_len = reply.length();

				// code identifying text generator
				std::string code = reply.substr(p + 11, q - p - 15);

				// find the function associated with this code
				auto pf = myIncludes.find(code);
				if (pf != myIncludes.end())
				{
					// insert generated text
					std::string content_part;
					try
					{
						pf->second(content_part);
					}
					catch (...)
					{

					}
					reply.insert(p, content_part);
					res = true;
				}

				// adjust pointer into text for insertion
				p = q + reply.length() - reply_len;
			}
			return res;
		}
		*/

		std::istream & safeGetline(std::istream & is, std::string & line)
		{
			std::string myline;
			if (getline(is, myline))
			{
				if (!myline.empty() && myline[myline.size() - 1] == '\r')
				{
					line = myline.substr(0, myline.size() - 1);
				}
				else
				{
					line = myline;
				}
			}
			return is;
		}

		bool cWebem::ExtractPostData(request &req, const char *pContent_Type)
		{
			if (strstr(pContent_Type, "multipart/form-data") != nullptr)
			{
				std::string szContent = req.content;
				size_t pos;
				std::string szVariable, szContentType, szValue;

				//first line is our boundary
				pos = szContent.find("\r\n");
				if (pos == std::string::npos)
					return false;
				std::string szBoundary = szContent.substr(0, pos);
				szContent = szContent.substr(pos + 2);

				while (!szContent.empty())
				{
					//Next line will contain our variable name
					pos = szContent.find("\r\n");
					if (pos == std::string::npos)
						return false;
					szVariable = szContent.substr(0, pos);
					szContent = szContent.substr(pos + 2);
					if (szVariable.find("Content-Disposition") != 0)
						return false;
					pos = szVariable.find("name=\"");
					if (pos == std::string::npos)
						return false;
					szVariable = szVariable.substr(pos + 6);
					pos = szVariable.find('"');
					if (pos == std::string::npos)
						return false;
					szVariable = szVariable.substr(0, pos);
					//Next line could be empty, or a Content-Type, if its empty, it is just a string
					pos = szContent.find("\r\n");
					if (pos == std::string::npos)
						return false;
					szContentType = szContent.substr(0, pos);
					szContent = szContent.substr(pos + 2);
					if (
						(szContentType.find("application/octet-stream") != std::string::npos) ||
						(szContentType.find("application/json") != std::string::npos) ||
						(szContentType.find("application/x-zip") != std::string::npos) ||
						(szContentType.find("application/zip") != std::string::npos) ||
						(szContentType.find("Content-Type: text/xml") != std::string::npos) ||
						(szContentType.find("Content-Type: text/x-hex") != std::string::npos) ||
						(szContentType.find("Content-Type: image/") != std::string::npos)
						)
					{
						//Its a file/stream, next line should be empty
						pos = szContent.find("\r\n");
						if (pos == std::string::npos)
							return false;
						szContent = szContent.substr(pos + 2);
					}
					else
					{
						//next line should be empty
						if (!szContentType.empty())
							return false;//dont know this one
					}
					pos = szContent.find(szBoundary);
					if (pos == std::string::npos)
						return false;
					szValue = szContent.substr(0, pos - 2);
					req.parameters.insert(std::pair< std::string, std::string >(szVariable, szValue));

					szContent = szContent.substr(pos + szBoundary.size());
					pos = szContent.find("\r\n");
					if (pos == std::string::npos)
						return false;
					szContent = szContent.substr(pos + 2);
				}
			}
			else if (strstr(pContent_Type, "application/x-www-form-urlencoded") != nullptr)
			{
				std::string params = req.content;
				std::string name;
				std::string value;

				size_t q = 0;
				size_t p = q;
				int flag_done = 0;
				const std::string& uri = params;
				while (!flag_done)
				{
					q = uri.find('=', p);
					if (q == std::string::npos)
					{
						break;
					}
					name = uri.substr(p, q - p);
					p = q + 1;
					q = uri.find('&', p);
					if (q != std::string::npos)
						value = uri.substr(p, q - p);
					else
					{
						value = uri.substr(p);
						flag_done = 1;
					}
					// the browser sends blanks as +
					while (true)
					{
						size_t p = value.find('+');
						if (p == std::string::npos)
							break;
						value.replace(p, 1, " ");
					}

					// now, url-decode only the value
					std::string decoded;
					request_handler::url_decode(value, decoded);
					req.parameters.insert(std::pair< std::string, std::string >(name, decoded));
					p = q + 1;
				}
			}
			else if ((strstr(pContent_Type, "text/plain") != nullptr) || (strstr(pContent_Type, "application/json") != nullptr) ||
				(strstr(pContent_Type, "application/xml") != nullptr))
			{
				//Raw data
				req.parameters.insert(std::pair< std::string, std::string >("data", req.content));
			}
			else
			{
				//Unknown content type
				_log.Debug(DEBUG_WEBSERVER, "[web:%s] Unable to process POST Data, unknown content type: %s", GetPort().c_str(), pContent_Type);
				return false;
			}
			return true;
		}

		bool cWebem::IsAction(const request& req)
		{
			// look for cWebem form action request
			std::string uri = req.uri;
			size_t q = uri.find(".webem");
			if (q != std::string::npos && req.method == "POST")
				return true;
			return false;
		}

		/**
		Do not call from application code,
		used by server to handle form submissions.

		returns false is authentication is invalid

		*/
		bool cWebem::CheckForAction(WebEmSession & session, request& req)
		{
			// look for cWebem form action request
			if (!IsAction(req))
				return false;

			std::string uri = ExtractRequestPath(req.uri);

			// find function matching action code
			size_t q = uri.find(".webem");
			std::string code = uri.substr(1, q - 1);
			auto pfun = myActions.find(code);
			if (pfun == myActions.end())
				return false;

			// decode the values
			const char *pContent_Type = request::get_req_header(&req, "Content-Type");
			if (pContent_Type)
			{
				req.parameters.clear();

				bool bExtracted = ExtractPostData(req, pContent_Type);

				// parameters have been extracted, so now execute
				// we should have at least one value
				if (bExtracted && !req.parameters.empty())
				{
					// call the function
					try
					{
						pfun->second(session, req, req.uri);
					}
					catch (...)
					{
						return false;
					}
					if ((req.uri[0] == '/') && (m_webRoot.length() > 0))
					{
						// possible incorrect root reference
						size_t q = req.uri.find(m_webRoot);
						if (q != 0)
						{
							std::string olduri = req.uri;
							req.uri = m_webRoot + olduri;
						}
					}
					return true;
				}
			}

			return false;
		}

		bool cWebem::IsPageOverride(const request& req, reply& rep)
		{
			std::string request_path;
			request_handler::url_decode(req.uri, request_path);
			request_path = ExtractRequestPath(request_path);

			size_t paramPos = request_path.find_first_of('?');
			if (paramPos != std::string::npos)
			{
				request_path = request_path.substr(0, paramPos);
			}

			auto pfun = myPages.find(request_path);
			if (pfun != myPages.end())
				return true;
			return false;
		}

		bool cWebem::CheckForPageOverride(WebEmSession & session, request& req, reply& rep)
		{
			std::string request_path;
			request_handler::url_decode(req.uri, request_path);
			request_path = ExtractRequestPath(request_path);

			req.parameters.clear();

			std::string request_path2 = req.uri; // we need the raw request string to parse the get-request
			size_t paramPos = request_path2.find_first_of('?');
			if (paramPos != std::string::npos)
			{
				std::string params = request_path2.substr(paramPos + 1);
				std::string name;
				std::string value;

				size_t q = 0;
				size_t p = q;
				int flag_done = 0;
				const std::string &uri = params;
				while (!flag_done)
				{
					q = uri.find('=', p);
					if (q == std::string::npos)
					{
						break;
					}
					name = uri.substr(p, q - p);
					p = q + 1;
					q = uri.find('&', p);
					if (q != std::string::npos)
						value = uri.substr(p, q - p);
					else
					{
						value = uri.substr(p);
						flag_done = 1;
					}
					// the browser sends blanks as +
					while (true)
					{
						size_t p = value.find('+');
						if (p == std::string::npos)
							break;
						value.replace(p, 1, " ");
					}

					// now, url-decode only the value
					std::string decoded;
					request_handler::url_decode(value, decoded);
					req.parameters.insert(std::pair< std::string, std::string >(name, decoded));
					p = q + 1;
				}
			}
			if (req.method == "POST")
			{
				const char *pContent_Type = request::get_req_header(&req, "Content-Type");
				if (pContent_Type)
				{
					// Extract the POST data into the parameters
					bool bExtracted = ExtractPostData(req, pContent_Type);
				}
			}

			// Determine the file extension.
			std::string extension;
			if (req.uri.find("json.htm") != std::string::npos)
			{
				extension = "json";
			}
			else
			{
				std::size_t last_slash_pos = request_path.find_last_of('/');
				std::size_t last_dot_pos = request_path.find_last_of('.');
				if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
				{
					extension = request_path.substr(last_dot_pos + 1);
				}
			}
			std::string strMimeType = mime_types::extension_to_type(extension);

			auto pfun = myPages.find(request_path);
			if (pfun != myPages.end())
			{
				rep.status = reply::ok;
				try
				{
					pfun->second(session, req, rep);
				}
				catch (std::exception& e)
				{
					_log.Log(LOG_ERROR, "[web:%s] PO exception occurred : '%s'", GetPort().c_str(), e.what());
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "[web:%s] PO unknown exception occurred", GetPort().c_str());
				}
				std::string attachment;
				for (const auto &header : rep.headers)
				{
					if (boost::iequals(header.name, "Content-Disposition"))
					{
						attachment = header.value.substr(header.value.find('=') + 1);
						std::size_t last_dot_pos = attachment.find_last_of('.');
						if (last_dot_pos != std::string::npos)
						{
							extension = attachment.substr(last_dot_pos + 1);
							strMimeType = mime_types::extension_to_type(extension);
						}
						break;
					}
				}

				reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
				if (!boost::algorithm::starts_with(strMimeType, "image"))
				{
					reply::add_header(&rep, "Cache-Control", "no-cache");
					reply::add_header(&rep, "Pragma", "no-cache");
					reply::add_cors_headers(&rep);
				}
				else
				{
					reply::add_header(&rep, "Cache-Control", "max-age=3600, public");
				}
				reply::add_header_content_type(&rep, strMimeType);
				if (m_settings.is_secure())
					reply::add_security_headers(&rep);
				return true;
			}

			return false;
		}

		void cWebem::SetWebTheme(const std::string &themename)
		{
			m_actTheme = "/styles/" + themename;
		}

		void cWebem::SetWebRoot(const std::string &webRoot)
		{
			// remove trailing slash if required
			if (!webRoot.empty() && webRoot[webRoot.size() - 1] == '/')
			{
				m_webRoot = webRoot.substr(0, webRoot.size() - 1);
			}
			else
			{
				m_webRoot = webRoot;
			}
			// put slash at the front if required
			if (!m_webRoot.empty() && m_webRoot[0] != '/')
			{
				m_webRoot = "/" + webRoot;
			}
		}

		std::string cWebem::ExtractRequestPath(const std::string& original_request_path)
		{
			std::string request_path(original_request_path);
			size_t paramPos = request_path.find_first_of('?');
			if (paramPos != std::string::npos)
			{
				request_path = request_path.substr(0, paramPos);
			}

			if (request_path.find(m_webRoot + "/@login") == 0)
			{
				request_path = m_webRoot + "/";
			}

			if (!m_webRoot.empty())
			{
				// remove web root if present otherwise
				// create invalid request
				if (request_path.find(m_webRoot) == 0)
				{
					request_path = request_path.substr(m_webRoot.size());
				}
				else
				{
					request_path = "";
				}
			}

			return request_path;
		}

		bool cWebem::IsBadRequestPath(const std::string& request_path)
		{
			// Request path must be absolute and not contain "..".
			if (request_path.empty() || request_path[0] != '/'
				|| request_path.find("..") != std::string::npos)
			{
				return true;
			}

			// don't allow access to control files
			if (request_path.find(".htpasswd") != std::string::npos)
			{
				return true;
			}

			// if we have a web root set the request must start with it
			if (!m_webRoot.empty())
			{
				if (request_path.find(m_webRoot) != 0)
				{
					return true;
				}
			}

			return false;
		}

		void cWebem::AddUserPassword(const unsigned long ID, const std::string &username, const std::string &password, const std::string &mfatoken, const _eUserRights userrights, const int activetabs, const std::string &privkey, const std::string &pubkey)
		{
			_tWebUserPassword wtmp;
			wtmp.ID = ID;
			wtmp.Username = username;
			wtmp.Password = password;
			wtmp.Mfatoken = mfatoken;
			wtmp.PrivKey = privkey;
			wtmp.PubKey = pubkey;
			wtmp.userrights = userrights;
			wtmp.ActiveTabs = activetabs;
			wtmp.TotSensors = 0;
			m_userpasswords.push_back(wtmp);
		}

		void cWebem::ClearUserPasswords()
		{
			m_userpasswords.clear();

			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			m_sessions.clear(); //TODO : check if it is really necessary
		}

		constexpr std::array<uint8_t, 8> ip_bit_8_array{
			0b00000000, //
			0b10000000, //
			0b11000000, //
			0b11100000, //
			0b11110000, //
			0b11111000, //
			0b11111100, //
			0b11111110, //
		};

		void cWebem::AddTrustedNetworks(std::string network)
		{
			if (network.empty())
			{
				_log.Log(LOG_STATUS, "[web:%s] Empty trusted network string provided! Skipping...", GetPort().c_str());
				return;
			}

			_tIPNetwork ipnetwork;
			ipnetwork.bIsIPv6 = (network.find(':') != std::string::npos);

			uint8_t iASize = (!ipnetwork.bIsIPv6) ? 4 : 16;
			int ii;

			_log.Debug(DEBUG_WEBSERVER, "[web:%s] Adding IPv%s network (%s) to list of trusted networks.", GetPort().c_str(), (ipnetwork.bIsIPv6 ? "6" : "4"), network.c_str());

			if (network.find('*') != std::string::npos)
			{
				std::vector<std::string> results;
				StringSplit(network, (!ipnetwork.bIsIPv6) ? "." : ":" , results);
				if (results.size() < 2)
					return;

				uint8_t wPos = 0;
				int wptr = 0;
				std::string szNetwork;
				while (wPos < (uint8_t)results.size())
				{
					bool bIsMask = (results[wPos] == "*");
					ipnetwork.Mask[wptr++] = (!bIsMask) ? 255 : 0;
					if (ipnetwork.bIsIPv6)
					{
						ipnetwork.Mask[wptr++] = (!bIsMask) ? 255 : 0;
					}
					if (!szNetwork.empty())
						szNetwork += (!ipnetwork.bIsIPv6) ? "." : ":";
					szNetwork += (!bIsMask) ? results[wPos] : "0";
					wPos++;
				}
				int totOctets = (!ipnetwork.bIsIPv6) ? 4 : 8;
				while (wPos < totOctets)
				{
					ipnetwork.Mask[wptr++] = 0;
					if (ipnetwork.bIsIPv6)
						ipnetwork.Mask[wptr++] = 0;
					if (!szNetwork.empty())
						szNetwork += (!ipnetwork.bIsIPv6) ? "." : ":";
					szNetwork += "0";
					wPos++;
				}
				
				if (inet_pton((!ipnetwork.bIsIPv6) ? AF_INET : AF_INET6, szNetwork.c_str(), &ipnetwork.Network) != 1)
					return; //invalid address

				//Apply mask to network address
				for (ii = 0; ii < iASize; ii++)
					ipnetwork.Network[ii] = ipnetwork.Network[ii] & ipnetwork.Mask[ii];
			}
			else
			{
				size_t pos = network.find_first_of('/');
				if (pos != std::string::npos)
				{
					std::string szNetwork = network.substr(0, pos);
					std::string szMask = network.substr(pos + 1);
					if (szNetwork.empty() || szMask.empty())
						return;

					if (inet_pton((!ipnetwork.bIsIPv6) ? AF_INET : AF_INET6, szNetwork.c_str(), &ipnetwork.Network) != 1)
						return; //invalid address

					uint8_t iBitcount = std::stoi(szMask);

					if (!ipnetwork.bIsIPv6)
					{
						if (iBitcount > 32)
							return;
					}
					else if (iBitcount > 128)
						return;

					uint8_t tot_c_bytes = iBitcount / 8;
					uint8_t tot_r_bits = iBitcount % 8;

					memset((void*)&ipnetwork.Mask, 0xFF, tot_c_bytes);
					if (tot_r_bits)
						ipnetwork.Mask[tot_c_bytes % 16] = ip_bit_8_array[tot_r_bits];

					//Apply mask to network address
					for (ii = 0; ii < iASize; ii++)
						ipnetwork.Network[ii] = ipnetwork.Network[ii] & ipnetwork.Mask[ii];
				}
				else
				{
					//Single IP or Hostname
					struct addrinfo* addr = nullptr;
					if (getaddrinfo(network.c_str(), "0", nullptr, &addr) == 0)
					{
						struct sockaddr_in* saddr = (((struct sockaddr_in*)addr->ai_addr));
						uint8_t* pAddress = nullptr;
						if (saddr->sin_family == AF_INET)
						{
							ipnetwork.bIsIPv6 = false;
							iASize = 4;
							pAddress = (uint8_t*)&saddr->sin_addr;
						}
						else if (saddr->sin_family == AF_INET6)
						{
							ipnetwork.bIsIPv6 = true;
							iASize = 16;
							struct sockaddr_in6* saddr6 = (((struct sockaddr_in6*)addr->ai_addr));
							pAddress = (uint8_t*)&saddr6->sin6_addr;
						}
						else
							return;
						memcpy(&ipnetwork.Network, pAddress, iASize);
					}
					else if (inet_pton((!ipnetwork.bIsIPv6) ? AF_INET : AF_INET6, network.c_str(), &ipnetwork.Network) != 1)
						return; //invalid address

					memset((void*)&ipnetwork.Mask, 0xFF, iASize);
					ipnetwork.ip_string = network;
				}
			}

			m_localnetworks.push_back(ipnetwork);
		}

		void cWebem::ClearTrustedNetworks()
		{
			m_localnetworks.clear();
		}

		void cWebem::SetDigistRealm(const std::string &realm)
		{
			m_DigistRealm = realm;
		}

		void cWebem::SetZipPassword(const std::string &password)
		{
			m_zippassword = password;
		}

		void cWebem::SetSessionStore(session_store_impl_ptr sessionStore)
		{
			mySessionStore = sessionStore;
		}

		session_store_impl_ptr cWebem::GetSessionStore()
		{
			return mySessionStore;
		}

		std::string cWebem::GetPort()
		{
			return m_settings.listening_port;
		}

		std::string cWebem::GetWebRoot()
		{
			return m_webRoot;
		}

		WebEmSession * cWebem::GetSession(const std::string & ssid)
		{
			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			auto itt = m_sessions.find(ssid);
			if (itt != m_sessions.end())
				return &itt->second;

			return nullptr;
		}

		void cWebem::AddSession(const WebEmSession & session)
		{
			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			m_sessions[session.id] = session;
		}

		void cWebem::RemoveSession(const WebEmSession & session)
		{
			RemoveSession(session.id);
		}

		void cWebem::RemoveSession(const std::string & ssid)
		{
			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			auto itt = m_sessions.find(ssid);
			if (itt != m_sessions.end())
				m_sessions.erase(itt);
		}

		int cWebem::CountSessions()
		{
			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			return (int)m_sessions.size();
		}

		std::vector<std::string> cWebem::GetExpiredSessions()
		{
			std::unique_lock<std::mutex> lock(m_sessionsMutex);
			std::vector<std::string> ret;
			time_t now = mytime(nullptr);
			for (const auto &session : m_sessions)
			{
				if (session.second.timeout < now)
					ret.push_back(session.second.id);
			}
			return ret;
		}

		void cWebem::CleanSessions()
		{
			_log.Debug(DEBUG_WEBSERVER, "[web:%s] cleaning sessions...", GetPort().c_str());

			// Clean up timed out sessions from memory
			std::vector<std::string> expired_ssids = GetExpiredSessions();
			for (const auto &ssid : expired_ssids)
			{
				RemoveSession(ssid);
			}
			// Clean up expired sessions from database in order to avoid to wait for the domoticz restart (long time running instance)
			if (mySessionStore != nullptr)
			{
				mySessionStore->CleanSessions();
			}
			// Schedule next cleanup
			m_session_clean_timer.expires_at(m_session_clean_timer.expires_at() + boost::posix_time::minutes(15));
			m_session_clean_timer.async_wait([this](auto &&) { CleanSessions(); });
		}

		bool cWebem::isValidIP(std::string &ip)
		{
			if (ip.empty())
				return false;

			std::string cleanIP = stdstring_trimws(ip);
			bool bIsIPv6 = (cleanIP.find(':') != std::string::npos);
			// IPv6 and IPv4 addresses can be written as quoted strings
			if (cleanIP.front() == '"' && cleanIP.back() == '"')
			{
				cleanIP = cleanIP.substr(1,cleanIP.size()-2);	// Remove quotes from begin and end
			}
			if (bIsIPv6)
			{
				// IPv6 addresses can be written as quoted strings and between brackets (See RFC5952)
				if (cleanIP.front() == '[' && cleanIP.back() == ']')
				{
					cleanIP = cleanIP.substr(1,cleanIP.size()-2);	// Remove brackets from begin and end
				}
				// Link-local IPv6 addresses could have a 'zone-index' identifiyng which interface is used
				// on a machine which has multiple interface. Can be discarded for checking
				if ((cleanIP.find("fe80::") == 0) && (cleanIP.find('%') != std::string::npos))
				{
					cleanIP = cleanIP.substr(0,cleanIP.find('%'));
				}
			}
		#ifndef WIN32
			else
			{
				// Convert from 'IPv4 numbers-and-dots notation' to 'IPv4 dotted-decimal notation' (or sometimes called: IPv4 dotted-quad notation)
				struct in_addr addr;
				if (inet_aton(cleanIP.c_str(), &addr) != 1)
					return false;
				char str[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &addr, str, INET_ADDRSTRLEN) == nullptr)
					return false;
				cleanIP.assign(str);
			}
		#endif
			uint8_t uiIP[16] = { 0 };
			if (inet_pton((!bIsIPv6) ? AF_INET : AF_INET6, cleanIP.c_str(), &uiIP) == 1)
			{
				// It seems to be a valid IPv4 or IPv6, let's try to rewrite it to correct presentation format
				char str[INET6_ADDRSTRLEN];
				if (inet_ntop((!bIsIPv6) ? AF_INET : AF_INET6, &uiIP, str, INET6_ADDRSTRLEN) != nullptr)
				{
					cleanIP.assign(str);
					ip = cleanIP;
					return true;	// Valid IPv4 or IPv6 in presentation format
				}
			}

			return false;
		}

		bool cWebem::findRealHostBehindProxies(const request &req, std::string &realhost)
		{
			// Checking for 3 possible headers (in order of handling)
			// "Forwarded"	RFC7239  (https://www.rfc-editor.org/rfc/rfc7239)
			// "X-Forwarded-For" The defacto standard header used by many web/proxy servers (https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-Forwarded-For)
			// "X-Real-IP"	The (old) default header used by NGINX  (http://nginx.org/en/docs/http/ngx_http_realip_module.html#real_ip_header)
			//
			// These headers can occur multiple times, so need to be 'squashed' together
			// And a single line can contain multiple (comma separated) values in order

			std::vector<std::string> headers;
			std::vector<std::string> hosts;

			if (sumProxyHeader("forwarded", req, headers))
			{
				// We found one or more Forwarded headers that need to be processed into a list of Hosts
				if (!parseForwardedProxyHeader(headers, hosts))
				{
					return false;
				}
			}
			else if (sumProxyHeader("x-forwarded-for", req, headers))
			{
				// We found one or more X-Forwarded-For headers that need to be processed into a list of Hosts
				if (!parseProxyHeader(headers, hosts))
				{
					return false;
				}
			}
			else if (sumProxyHeader("x-real-ip", req, headers))
			{
				// We found one or more X-Real-IP headers that need to be processed into a list of Hosts
				if (!parseProxyHeader(headers, hosts))
				{
					return false;
				}
			}
			else
			{
				return true;
			}

			realhost = hosts[0];	// Even if we found a chain of hosts, we always use the first (= origin)
			return true;
		}

		bool cWebem::sumProxyHeader(const std::string &sHeader, const request &req, std::vector<std::string> &vHeaderLines)
		{
			std::string sHeaderName;
			for (const auto &header : req.headers)
			{
				sHeaderName = header.name;
				std::transform(sHeaderName.begin(), sHeaderName.end(), sHeaderName.begin(), ::tolower);
				if (sHeaderName.find(sHeader)==0)
				{
					vHeaderLines.push_back(header.value);
				}
			}

			return !vHeaderLines.empty();		// Assuming the function is called with an empty vHeaderLines to begin with
		}

		bool cWebem::parseProxyHeader(const std::vector<std::string> &vHeaderLines, std::vector<std::string> &vHosts)
		{
			for (const auto sLine : vHeaderLines)
			{
				std::vector<std::string> vLineParts;
				StringSplit(sLine, ",", vLineParts);
				for (std::string sPart : vLineParts)
				{
					if (isValidIP(sPart))
						vHosts.push_back(sPart);
					else {
						size_t dpos = sPart.find_last_of(':');
						if (dpos != std::string::npos)
						{
							//Strip off the port number
							sPart = sPart.substr(0, dpos);
							if (isValidIP(sPart))
								vHosts.push_back(sPart);
						}
					}
				}
			}

			return !vHosts.empty();		// Assuming the function is called with an empty vHosts to begin with
		}

		bool cWebem::parseForwardedProxyHeader(const std::vector<std::string> &vHeaderLines, std::vector<std::string> &vHosts)
		{
			for (const auto sLine : vHeaderLines)
			{
				std::vector<std::string> vLineParts;
				StringSplit(sLine, ",", vLineParts);
				for (std::string sPart : vLineParts)
				{
					stdstring_trimws(sPart);
					if (std::size_t isPos = sPart.find("for=") != std::string::npos)
					{
						isPos = isPos + 3;
						std::size_t iePos = sLine.length();
						if (sPart.find(";", isPos) != std::string::npos)
						{
							iePos = sPart.find(";", isPos);
						}
						std::string sSub = sPart.substr(isPos, (iePos - isPos));
						if(isValidIP(sSub))
							vHosts.push_back(sSub);
					}
				}
			}

			return !vHosts.empty();		// Assuming the function is called with an empty vHosts to begin with
		}

		bool cWebem::CheckVHost(const request &req)
		{
			if (m_settings.vhostname.empty() || !m_settings.is_secure())	// Only do vhost checking for Secure (https) server
				return true;

			std::string sHost;
			std::string vHost = m_settings.vhostname;

			// When a vhostname is given, only respond to request addressed to it
			const char *cHost = req.get_req_header(&req, "Host");
			if (cHost != nullptr)
			{
				std::string scHost(cHost);
				size_t iPos = scHost.find_first_of(":");
				if (iPos != std::string::npos)
					sHost = scHost.substr(0,iPos);
				else
					sHost = scHost;
			}
			else
			{
				_log.Debug(DEBUG_WEBSERVER, "[web:%s] Unable to verify vhostname as Host header is missing in request!", GetPort().c_str());
				return false;
			}

			bool bStatus = (sHost.compare(vHost) == 0);
			_log.Debug(DEBUG_WEBSERVER, "[web:%s] Checking vhostname (%s) with request (%s) = %d", GetPort().c_str(), vHost.c_str(), sHost.c_str(), bStatus);
			return bStatus;
		}

		bool cWebem::FindAuthenticatedUser(std::string &user, const request &req, reply &rep)
		{
			bool bStatus = myRequestHandler.CheckUserAuthorization(user, req);

			if (user.empty())
			{
				rep = reply::stock_reply(reply::unauthorized, m_settings.is_secure());
				reply::add_cors_headers(&rep);
				char szAuthHeader[200];
				sprintf(szAuthHeader,
					"Basic "
					"realm=\"%s\"",
					m_DigistRealm.c_str());
				reply::add_header(&rep, "WWW-Authenticate", szAuthHeader);
			}

			return bStatus;
		}

		bool cWebemRequestHandler::CheckUserAuthorization(std::string &user, const request &req)
		{
			struct ah _ah;

			if(!parse_auth_header(req, &_ah))
				return false;

			return CheckUserAuthorization(user, &_ah);
		}

		bool cWebemRequestHandler::CheckUserAuthorization(std::string &user, struct ah *ah)
		{
			// Check if valid password has been provided for the user
			for (const auto &my : myWebem->m_userpasswords)
			{
				if (my.Username == ah->user && my.userrights != URIGHTS_CLIENTID)
				{
					user = ah->user;	// At least we know it is an existing User
					if (check_password(ah, my.Password))
					{
						ah->qop = std::to_string(my.userrights);
						return true;
					}
				}
			}
			return false;
		}

		// Return 1 on success. Always initializes the ah structure.
		int cWebemRequestHandler::parse_auth_header(const request& req, struct ah *ah)
		{
			const char *auth_header;

			if ((auth_header = request::get_req_header(&req, "Authorization")) == nullptr)
			{
				return 0;
			}

			// X509 Auth header
			if (boost::icontains(auth_header, "/CN="))
			{
				// DN looks like: /C=Country/ST=State/L=City/O=Org/OU=OrganizationUnit/CN=username/emailAddress=user@mail.com
				std::string dn = auth_header;
				size_t spos, epos;

				spos = dn.find("/CN=");
				epos = dn.find('/', spos + 1);
				if (spos != std::string::npos)
				{
					if (epos == std::string::npos)
					{
						epos = dn.size();
					}
					ah->user = dn.substr(spos + 4, epos - spos - 4);
				}

				spos = dn.find("/emailAddress=");
				epos = dn.find('/', spos + 1);
				if (spos != std::string::npos)
				{
					if (epos == std::string::npos)
					{
						epos = dn.size();
					}
					ah->response = dn.substr(spos + 14, epos - spos - 14);
				}

				if (ah->user.empty()) // TODO: Should ah->response be not empty ?
				{
					return 0;
				}
				ah->method = "X509";
				_log.Debug(DEBUG_AUTH, "[X509] Found a X509 Auth Header (%s)", ah->user.c_str());
				return 1;
			}
			// Basic Auth header
			if (boost::icontains(auth_header, "Basic "))
			{
				std::string decoded = base64_decode(auth_header + 6);
				size_t npos = decoded.find(':');
				if (npos == std::string::npos)
				{
					return 0;
				}

				ah->method = "BASIC";
				ah->user = decoded.substr(0, npos);
				ah->response = decoded.substr(npos + 1);
				_log.Debug(DEBUG_AUTH, "[Basic] Found a Basic Auth Header (%s)", ah->user.c_str());
				return 1;
			}
			// Bearer Auth header
			if (boost::icontains(auth_header, "Bearer "))
			{
				std::string sToken = auth_header + 7;

				// Might be a JWT token, find the first dot
				size_t npos = sToken.find('.');
				if (npos != std::string::npos)
				{
					// Base64decode the first piece to check
					std::string tokentype = base64url_decode(sToken.substr(0, npos));
					if(tokentype.find("JWT") != std::string::npos)
					{
						// We found the text JWT, now let's really check if it as a valid JWT Token
						// Step 1: Check if the JWT has an algorithm in the header AND an issuer (iss) claim in the payload
						auto decodedJWT = jwt::decode(sToken, &base64url_decode);
						if(!decodedJWT.has_algorithm())
						{
							_log.Debug(DEBUG_AUTH,"[JWT] Token does not contain an algorithm!");
							return 0;
						}
						if(!(decodedJWT.has_audience() && decodedJWT.has_issuer()))
						{
							_log.Debug(DEBUG_AUTH,"[JWT] Token does not contain an intended audience and/or issuer!");
							return 0;
						}
						// Step 2: Find the audience = our ClientID (~ the Username of the Domoticz User where the userright = ClientID)
						std::string clientid = decodedJWT.get_audience().cbegin()->data();	// Assumption: only 1 element in the AUD set!
						std::string JWTsubject = decodedJWT.get_subject();
						_log.Debug(DEBUG_AUTH,"[JWT] Token audience : %s", clientid.c_str());

						std::string clientsecret;
						std::string clientpubkey;
						std::string client_key_id;
						bool clientispublic = false;
						// Check if the audience has been registered as a User (type CLIENTID)
						for (const auto &my : myWebem->m_userpasswords)
						{
							if (my.Username == clientid)
							{
								if (my.userrights == URIGHTS_CLIENTID || clientid.compare(JWTsubject) == 0)
								{
									clientsecret = my.Password;
									clientpubkey = my.PubKey;
									client_key_id = std::to_string(my.ID);
									clientispublic = my.ActiveTabs;
									break;
								}
							}
						}
						if (client_key_id.empty() || (clientsecret.empty() && clientpubkey.empty()))
						{
							_log.Debug(DEBUG_AUTH, "[JWT] Unable to verify token as no ClientID for the audience has been found!");
							return 0;
						}
						// Step 3: Using the (hashed :( ) password of the ClientID as our ClientSecret to verify the JWT signature
						std::string JWTalgo = decodedJWT.get_algorithm();
						std::error_code ec;
						auto JWTverifyer = jwt::verify().with_issuer(myWebem->m_DigistRealm).with_audience(clientid);
						if (JWTalgo.compare("HS256") == 0)
						{
							JWTverifyer.allow_algorithm(jwt::algorithm::hs256{ clientsecret });
						}
						else if (JWTalgo.compare("HS384") == 0)
						{
							JWTverifyer.allow_algorithm(jwt::algorithm::hs384{ clientsecret });
						}
						else if (JWTalgo.compare("HS512") == 0)
						{
							JWTverifyer.allow_algorithm(jwt::algorithm::hs512{ clientsecret });
						}
						else if (JWTalgo.compare("RS256") == 0)
						{
							JWTverifyer.allow_algorithm(jwt::algorithm::rs256{ clientpubkey });
						}
						else if (JWTalgo.compare("PS256") == 0)
						{
							JWTverifyer.allow_algorithm(jwt::algorithm::ps256{ clientpubkey });
						}
						else
						{
							_log.Debug(DEBUG_AUTH, "[JWT] This token is signed with an unsupported algorithm (%s)!", JWTalgo.c_str());
							return 0;
						}
						JWTverifyer.expires_at_leeway(60);	// 60 seconds leeway time in case clocks are NOT fully (NTP) synced
						JWTverifyer.not_before_leeway(60);
						JWTverifyer.issued_at_leeway(60);
						JWTverifyer.verify(decodedJWT, ec);
						if(ec)
						{
							_log.Debug(DEBUG_AUTH, "[JWT] Token not valid! (%s)", ec.message().c_str());
							return 0;
						}
						// Step 4: Now also check if other mandatory claims (nbf, exp, sub) have been provided
						if(!(decodedJWT.has_expires_at() && decodedJWT.has_not_before() && decodedJWT.has_issued_at() && decodedJWT.has_subject() && decodedJWT.has_key_id()))
						{
							_log.Debug(DEBUG_AUTH, "[JWT] Mandatory claims KID, NBF, EXP, IAT, SUB are missing!");
							return 0;
						}
						// Step 5: See of the subject (intended user) is available and exists in the User table
						std::string key_id = decodedJWT.get_key_id();
						for (const auto &my : myWebem->m_userpasswords)
						{
							if (my.Username == JWTsubject)
							{
								if (my.userrights != URIGHTS_CLIENTID)
								{
									if (key_id.compare(client_key_id) == 0)
									{
										_log.Debug(DEBUG_AUTH,"[JWT] Decoded valid user (%s)", JWTsubject.c_str());
										ah->method = "JWT";
										ah->user = JWTsubject;
										ah->response = my.Password;
										ah->qop = std::to_string(my.userrights);		// Not really intended in original structure but works for passing the userrights
										return 1;
									}
									else
									{
										_log.Debug(DEBUG_AUTH, "[JWT] KID does not match (%s)!", client_key_id.c_str());
										return 0;
									}
								}
							}
						}
						_log.Debug(DEBUG_AUTH, "[JWT] Token contains non-existing user (%s)!", JWTsubject.c_str());
						return 0;
					}
				}
				// No dot found and/or not a JWT, so assume non-JWT type of Bearer token
				ah->method = "Bearer";
				ah->user = "";				// No clue how to deduce the user from the Bearer token provided
				ah->response = sToken; // Let's provide the found token as the 'password'
				_log.Debug(DEBUG_AUTH, "[Bearer] Found a Token (%s)", sToken.c_str());
				return 1;
			}
			return 0;
		}

		// Check the user's password, return 1 if OK
		int cWebemRequestHandler::check_password(struct ah *ah, const std::string &ha1)
		{
			if ((ah->nonce.empty()) && (!ah->response.empty()))
				return (ha1 == GenerateMD5Hash(ah->response));

			return 0;
		}

		// Authorize against the internal Userlist. Credentials coming via Authorization header or URL parameters.
		// Return 1 if authorized.
		// Only used when webserver Authentication method is set to Auth_Basic.
		int cWebemRequestHandler::authorize(WebEmSession & session, const request& req, reply& rep)
		{
			struct ah _ah;

			std::string uname;
			std::string upass;

			if (!parse_auth_header(req, &_ah))
			{	// No username, password (or other identification) found in Authorization header. Check URI for user parameters.
				size_t uPos = req.uri.find("username=");
				size_t pPos = req.uri.find("password=");
				if (
					(uPos == std::string::npos) ||
					(pPos == std::string::npos)
					)
				{
					return 0;
				}
				uPos += 9; //strlen("username=")
				pPos += 9; //strlen("password=")
				size_t uEnd = req.uri.find('&', uPos);
				size_t pEnd = req.uri.find('&', pPos);
				std::string tmpuname;
				std::string tmpupass;
				if (uEnd == std::string::npos)
					tmpuname = req.uri.substr(uPos);
				else
					tmpuname = req.uri.substr(uPos, uEnd - uPos);
				if (pEnd == std::string::npos)
					tmpupass = req.uri.substr(pPos);
				else
					tmpupass = req.uri.substr(pPos, pEnd - pPos);
				if (request_handler::url_decode(tmpuname, uname) && request_handler::url_decode(tmpupass, upass))
				{	// Found parameters, so lets use these to check
					_ah.user = base64_decode(uname);
					_ah.response = base64_decode(upass);
				}
				else
				{
					m_failcounter++;
					return 0;
				}
			}

			// Check if valid password has been provided for the user
			for (const auto &my : myWebem->m_userpasswords)
			{
				if (my.Username == _ah.user)
				{
					int bOK = check_password(&_ah, my.Password);
					_log.Debug(DEBUG_AUTH, "[Authorize] User %s password check: %d", _ah.user.c_str(), bOK);
					if (!bOK || my.userrights == URIGHTS_CLIENTID)	// User with ClientID 'rights' are not real users!
					{
						m_failcounter++;
						return 0;
					}
					session.isnew = true;
					session.username = my.Username;
					session.rights = my.userrights;
					session.rememberme = false;
					m_failcounter = 0;
					return 1;
				}
			}
			m_failcounter++;
			return 0;
		}

		bool cWebem::GenerateJwtToken(std::string &jwttoken, const std::string &clientid, const std::string &clientsecret, const std::string &user, const uint32_t exptime, const Json::Value jwtpayload)
		{
			bool bOk = false;
			// Did we get a 'plain' clientsecret or an already MD5Hashed one?
			std::string hashedsecret = clientsecret;
			if (!(clientsecret.length() == 32 && isHexRepresentation(clientsecret)))	// 2 * MD5_DIGEST_LENGTH
			{
				hashedsecret = GenerateMD5Hash(clientsecret);
			}
			// Check if the clientID exists and we have a valid clientSecret for it (used when generating Tokens for registered clients)
			for (const auto &my : myRequestHandler.Get_myWebem()->m_userpasswords)
			{
				if (my.Username == clientid)
				{
					if (my.userrights == URIGHTS_CLIENTID)	// The 'user' should have CLIENTID rights to be a real Client
					{
						if ((my.Password == hashedsecret) || (my.ActiveTabs == 1))	// We 'abuse' the Users ActiveTabs as the Application Public 'boolean'
						{
							_log.Debug(DEBUG_AUTH, "[JWT] Generate Token for %s using clientid %s (privKey %d)!", user.c_str(), clientid.c_str(), my.ActiveTabs);
							auto JWT = jwt::create()
								.set_type("JWT")
								.set_key_id(std::to_string(my.ID))
								.set_issuer(m_DigistRealm)
								.set_issued_at(std::chrono::system_clock::now())
								.set_not_before(std::chrono::system_clock::now())
								.set_expires_at(std::chrono::system_clock::now() + std::chrono::seconds{exptime})
								.set_audience(clientid)
								.set_subject(user)
								.set_id(GenerateUUID());
							if (!jwtpayload.empty())
							{
								for (auto const& id : jwtpayload.getMemberNames())
								{
									if(!(jwtpayload[id].isNull()))
									{
										if(jwtpayload[id].isNumeric())
										{
											double dVal(jwtpayload[id].asDouble());
											JWT.set_payload_claim(id, picojson::value(dVal));
										}
										else if(jwtpayload[id].isString())
										{
											std::string sVal(jwtpayload[id].asString());
											JWT.set_payload_claim(id, picojson::value(sVal));
										}
										else if(jwtpayload[id].isArray())
										{
											std::vector<std::string> aStrList;
											aStrList.reserve(jwtpayload[id].size());
											std::transform(jwtpayload[id].begin(), jwtpayload[id].end(), std::back_inserter(aStrList),[](const auto& s) { return s.asString(); });
											JWT.set_payload_claim(id, jwt::claim(aStrList.begin(), aStrList.end()));
										}
									}
								}
							}
							if (my.ActiveTabs)
							{
								jwttoken = JWT.sign(jwt::algorithm::ps256{"", my.PrivKey, "", ""}, &base64url_encode);
							}
							else
							{
								jwttoken = JWT.sign(jwt::algorithm::hs256{my.Password}, &base64url_encode);
							}
							bOk = true;
						}
					}
				}
			}

			return bOk;
		}

		bool cWebemRequestHandler::IsIPInRange(const std::string &ip, const _tIPNetwork &ipnetwork, const bool &bIsIPv6)
		{
			if (ipnetwork.bIsIPv6 != bIsIPv6)
				return false;	// No need to check the IP address and the network are not both IPv4 or IPv6

			uint8_t IP[16] = { 0 };
			inet_pton((!bIsIPv6) ? AF_INET : AF_INET6, ip.c_str(), &IP);	// We already checked if this works in the caller routine

			// Determine if the IP address is within the localnetwork range
			int iASize = (!bIsIPv6) ? 4 : 16;
			for (int ii = 0; ii < iASize; ii++)
				if (ipnetwork.Network[ii] != (IP[ii] & ipnetwork.Mask[ii]))
					return false;

			// As all segments of the given IP fit within the given network range, otherwise we wouldn't be here
			_log.Debug(DEBUG_WEBSERVER,"[web:%s] IP (%s) is within Trusted network range!",myWebem->GetPort().c_str(), ip.c_str());
			return true;
		}

		//Returns true is the connected host is in the trusted network
		bool cWebemRequestHandler::AreWeInTrustedNetwork(const std::string &sHost)
		{
			//Are there any local networks to check against?
			if (myWebem->m_localnetworks.empty())
				return false;

			//Is the given 'host' a valid IP address?
			std::string sCleanHost = sHost;
			if (!cWebem::isValidIP(sCleanHost))			{
				_log.Log(LOG_STATUS,"[web:%s] Given host (%s) is not a valid Ipv4 or IPv6 address! Unable to check if in Trusted Network!", myWebem->GetPort().c_str() ,sHost.c_str());
				return false;	// The IP address is not a valid IPv4 or IPv6 address
			}
			bool bIsIPv6 = (sCleanHost.find(':') != std::string::npos);

			return std::any_of(myWebem->m_localnetworks.begin(), myWebem->m_localnetworks.end(),
					   [&](const _tIPNetwork &my) { return IsIPInRange(sCleanHost, my, bIsIPv6); });
		}

		std::string cWebemRequestHandler::generateSessionID()
		{
			// Session id should not be predictable
			std::string randomValue = GenerateUUID();

			std::string sessionId = GenerateMD5Hash(base64_encode(randomValue));

			_log.Debug(DEBUG_WEBSERVER, "[web:%s] generate new session id token %s", myWebem->GetPort().c_str(), sessionId.c_str());

			return sessionId;
		}

		std::string cWebemRequestHandler::generateAuthToken(const WebEmSession & session, const request & req)
		{
			// Authentication token should not be predictable
			std::string randomValue = GenerateUUID();

			std::string authToken = base64_encode(randomValue);

			_log.Debug(DEBUG_WEBSERVER, "[web:%s] generate new authentication token %s for user %s", myWebem->GetPort().c_str(), authToken.c_str(), session.username.c_str());

			session_store_impl_ptr sstore = myWebem->GetSessionStore();
			if (sstore != nullptr)
			{
				WebEmStoredSession storedSession;
				storedSession.id = session.id;
				storedSession.auth_token = GenerateMD5Hash(authToken); // only save the hash to avoid a security issue if database is stolen
				storedSession.username = session.username;
				storedSession.expires = session.expires;
				storedSession.remote_host = session.remote_host; // to trace host
				storedSession.local_host = session.local_host; // to trace host
				storedSession.remote_port = session.remote_port; // to trace host
				storedSession.local_port = session.local_port; // to trace host
				sstore->StoreSession(storedSession); // only one place to do that
			}

			return authToken;
		}

		void cWebemRequestHandler::send_cookie(reply& rep, const WebEmSession & session)
		{
			std::stringstream sstr;
			sstr << "DMZSID=" << session.id << "_" << session.auth_token << "." << session.expires;
			sstr << "; HttpOnly; path=/; Expires=" << make_web_time(session.expires);
			reply::add_header(&rep, "Set-Cookie", sstr.str(), false);
		}

		void cWebemRequestHandler::send_remove_cookie(reply& rep)
		{
			std::stringstream sstr;
			sstr << "DMZSID=none";
			// RK, we removed path=/ so you can be logged in to two Domoticz's at the same time on https://my.domoticz.com/.
			sstr << "; HttpOnly; Expires=" << make_web_time(0);
			reply::add_header(&rep, "Set-Cookie", sstr.str(), false);
		}

		bool cWebemRequestHandler::parse_cookie(const request& req, std::string& sSID, std::string& sAuthToken, std::string& szTime, bool& expired)
		{
			bool bCookie = false;
			sSID.clear();
			sAuthToken.clear();
			szTime.clear();
			expired = false;

			//Check if cookie available and still valid
			const char* cookie_header = request::get_req_header(&req, "Cookie");
			if (cookie_header != nullptr)
			{
				// Parse session id and its expiration date
				std::string scookie = cookie_header;
				size_t fpos = scookie.find("DMZSID=");
				if (fpos != std::string::npos)
				{
					bCookie = true;
					scookie = scookie.substr(fpos);
					fpos = 0;
					size_t epos = scookie.find(';');	// Check if there are more cookies in this Header (and ignore those)
					if (epos != std::string::npos)
					{
						scookie = scookie.substr(0, epos);
					}
					size_t upos = scookie.find('_', fpos);
					size_t ppos = scookie.find('.', upos);
					time_t now = mytime(nullptr);
					if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
					{
						sSID = scookie.substr(fpos + 7, upos - fpos - 7);
						sAuthToken = scookie.substr(upos + 1, ppos - upos - 1);
						szTime = scookie.substr(ppos + 1);

						time_t stime;
						std::stringstream sstr;
						sstr << szTime;
						sstr >> stime;

						expired = stime < now;
					}
				}
			}
			return bCookie;
		}

		void cWebemRequestHandler::send_authorization_request(reply& rep)
		{
			rep = reply::stock_reply(reply::unauthorized, myWebem->m_settings.is_secure());
			reply::add_cors_headers(&rep);
			send_remove_cookie(rep);
			if (myWebem->m_authmethod == AUTH_BASIC)
			{
				char szAuthHeader[200];
				sprintf(szAuthHeader,
					"Basic "
					"realm=\"%s\"",
					myWebem->m_DigistRealm.c_str());
				reply::add_header(&rep, "WWW-Authenticate", szAuthHeader);
			}
		}

		bool cWebemRequestHandler::CompressWebOutput(const request& req, reply& rep)
		{
			if (myWebem->m_gzipmode != WWW_USE_GZIP)
				return false;

			std::string request_path;
			if (!url_decode(req.uri, request_path))
				return false;
			if (
				(request_path.find(".png") != std::string::npos) ||
				(request_path.find(".jpg") != std::string::npos)
				)
			{
				//don't compress 'compressed' images
				return false;
			}

			const char *encoding_header;
			//check gzip support if yes, send it back in gzip format
			if ((encoding_header = request::get_req_header(&req, "Accept-Encoding")) != nullptr)
			{
				//see if we support gzip
				bool bHaveGZipSupport = (strstr(encoding_header, "gzip") != nullptr);
				if (bHaveGZipSupport)
				{
					CA2GZIP gzip((char*)rep.content.c_str(), (int)rep.content.size());
					if ((gzip.Length > 0) && (gzip.Length < (int)rep.content.size()))
					{
						rep.bIsGZIP = true; // flag for later
						rep.content.clear();
						rep.content.append((char*)gzip.pgzip, gzip.Length);
						//Set new content length
						reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
						//Add gzip header
						reply::add_header(&rep, "Content-Encoding", "gzip");
						return true;
					}
				}
			}
			return false;
		}

		std::string cWebemRequestHandler::compute_accept_header(const std::string &websocket_key)
		{
			// the length of an sha1 hash
			#define SHA1_LENGTH 20
			// the GUID as specified in RFC 6455
			const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			std::string combined = websocket_key + GUID;
			unsigned char sha1result[SHA1_LENGTH];
			sha1::calc((void *)combined.c_str(), combined.length(), sha1result);
			std::string accept = base64_encode_buf(sha1result, SHA1_LENGTH);
			return accept;
		}

		bool cWebemRequestHandler::is_upgrade_request(WebEmSession & session, const request& req, reply& rep)
		{
			// request method should be GET
			if (req.method != "GET")
			{
				return false;
			}
			// http version should be 1.1 at least
			if (((req.http_version_major * 10) + req.http_version_minor) < 11)
			{
				return false;
			}
			const char *h;
			// client MUST include Connection: Upgrade header
			h = request::get_req_header(&req, "Connection");
			if (!h)
			{
				return false;
			}

			// client MUST include Upgrade: websocket
			h = request::get_req_header(&req, "Upgrade");
			if (!h)
			{
				return false;
			}

			std::string upgrade_header = h;
			if (!boost::iequals(upgrade_header, "websocket"))
			{
				return false;
			};

			// we only have one service until now
			if (req.uri.find("/json") == std::string::npos)
			{
				// todo: request uri could be an absolute URI as well!!!
				rep = reply::stock_reply(reply::not_found);
				return true;
			}
			h = request::get_req_header(&req, "Host");
			// request MUST include a host header, even if we don't check it
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::forbidden);
				return true;
			}
			// request MUST include an origin header, even if we don't check it
			// we only "allow" connections from browser clients
			h = request::get_req_header(&req, "Origin");
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::bad_request);
				return true;
			}
			// request MUST include a version number
			h = request::get_req_header(&req, "Sec-Websocket-Version");
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::bad_request);
				return true;
			}
			else
			{
				int version = atoi(h);
				// we support versions 13 (and higher)
				if (version < 13)
				{
					rep = reply::stock_reply(reply::bad_request);
					return true;
				}
			}

			h = request::get_req_header(&req, "Sec-Websocket-Protocol");
			// check if a protocol is given, and it includes the {websocket_protocol}.
			if (!h)
			{
				return false;
			}
			std::string protocol_header = h;
			if (protocol_header.find(websocket_protocol) == std::string::npos)
			{
				rep = reply::stock_reply(reply::bad_request);
				return true;
			}
			h = request::get_req_header(&req, "Sec-Websocket-Key");
			// request MUST include a sec-websocket-key header and we need to respond to it
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::bad_request);
				return true;
			}
			std::string websocket_key = h;
			rep = reply::stock_reply(reply::switching_protocols);
			reply::add_header(&rep, "Connection", "Upgrade");
			reply::add_header(&rep, "Upgrade", "websocket");

			std::string accept = compute_accept_header(websocket_key);
			if (accept.empty())
			{
				rep = reply::stock_reply(reply::internal_server_error);
				return true;
			}
			reply::add_header(&rep, "Sec-Websocket-Accept", accept);
			// we only speak the {websocket_protocol} subprotocol
			reply::add_header(&rep, "Sec-Websocket-Protocol", websocket_protocol);
			return true;
		}

		static bool GetURICommandParameter(const std::string &uri, std::string &cmdparam)
		{
			if (uri.find("type=command") == std::string::npos)
				return false;
			size_t ppos1 = uri.find("&param=");
			size_t ppos2 = uri.find("?param=");
			if (
				(ppos1 == std::string::npos) &&
				(ppos2 == std::string::npos)
				)
				return false;
			cmdparam = uri;
			size_t ppos = ppos1;
			if (ppos == std::string::npos)
				ppos = ppos2;
			else
			{
				if ((ppos2 < ppos) && (ppos != std::string::npos))
					ppos = ppos2;
			}
			cmdparam = uri.substr(ppos + 7);
			ppos = cmdparam.find('&');
			if (ppos != std::string::npos)
			{
				cmdparam = cmdparam.substr(0, ppos);
			}
			return true;
		}

		bool cWebemRequestHandler::CheckAuthByPass(const request& req)
		{
			//Check if we need to bypass authentication for this request (URL or command)
			for (const auto &url : myWebem->myWhitelistURLs)
				if (req.uri.find(url) == 0)
					return true;

			std::string cmdparam;
			if (GetURICommandParameter(req.uri, cmdparam))
			{
				for (const auto &cmd : myWebem->myWhitelistCommands)
					if (cmdparam.find(cmd) == 0)
						return true;
			}

			return false;
		}

		bool cWebemRequestHandler::AllowBasicAuth()
		{
			if (myWebem->m_settings.is_secure())		// Basic Auth is allowed when used over HTTPS (SSL Encrypted communication)
				return true;
			else if (myWebem->m_AllowPlainBasicAuth)	// Allow Basic Auth over non HTTPS
				return true;

			return false;
		}

		bool cWebemRequestHandler::CheckAuthentication(WebEmSession & session, const request& req, reply& rep)
		{
			bool bTrustedNetwork = false;

			session.rights = -1; // no rights
			session.id = "";
			session.username = "";
			session.auth_token = "";

			if (myWebem->m_userpasswords.empty())
			{
				_log.Log(LOG_ERROR, "No (active) users in the system! There should be at least 1 active Admin user!");
			}
			else if (AreWeInTrustedNetwork(session.remote_host))
			{
				for (const auto &my : myWebem->m_userpasswords)
				{
					if (my.userrights == URIGHTS_ADMIN) // we found an admin
					{
						session.username = my.Username;
						session.rights = my.userrights;
						break;
					}
				}
				if (session.rights == -1)
					_log.Debug(DEBUG_AUTH, "[Auth Check] Trusted network exception detected, but no Admin User found!");
				bTrustedNetwork = true;
			}

			//Check for valid Authorization headers (JWT Token, Basis Authentication, etc.) and use these offered credentials
			struct ah _ah;
			if (parse_auth_header(req, &_ah))
			{
				if (_ah.method == "JWT")
				{
					_log.Debug(DEBUG_AUTH, "[Auth Check] Found JWT Authorization token: Method %s, Userdata %s, rights %s", _ah.method.c_str(), _ah.user.c_str(), _ah.qop.c_str());
					session.isnew = false;
					session.rememberme = false;
					session.username = _ah.user;
					session.rights = std::atoi(_ah.qop.c_str());
					return true;
				}
				else if (_ah.method == "BASIC")
				{
					if (req.uri.find("json.htm") != std::string::npos)	// Exception for the main API endpoint so scripts can execute them with 'just' Basic AUTH
					{
						if (AllowBasicAuth())	// Check if Basic Auth is allowed either over HTTPS or when explicitly enabled
						{
							if (CheckUserAuthorization(_ah.user, &_ah))
							{
								_log.Debug(DEBUG_AUTH, "[Auth Check] Found Basic Authorization for API call: Method %s, Userdata %s, rights %s", _ah.method.c_str(), _ah.user.c_str(), _ah.qop.c_str());
								session.isnew = false;
								session.rememberme = false;
								session.username = _ah.user;
								session.rights = std::atoi(_ah.qop.c_str());
								return true;
							}
							else
							{	// Clear the session as we could be in a Trusted Network BUT have invalid Basic Auth
								_log.Debug(DEBUG_AUTH, "[Auth Check] Invalid Basic Authorization for API call!");
								session.username = "";
								session.rights = -1;
								return false;
							}
						}
						else
						{	// Clear the session as we could be in a Trusted Network BUT rejected Basic Auth
							_log.Debug(DEBUG_AUTH, "[Auth Check] Basic Authorization rejected as it is not done over HTTPS or not explicitly allowed over HTTP!");
							session.username = "";
							session.rights = -1;
							return false;
						}
					}
					else
					{
						_log.Debug(DEBUG_AUTH, "[Auth Check] Basic Authorization ignored as this is not a call to the API!");
					}
				}
			}

			//Check if cookie available and still valid
			std::string sSID;
			std::string sAuthToken;
			std::string szTime;
			bool expired = false;
			if(parse_cookie(req, sSID, sAuthToken, szTime, expired))
			{
				time_t now = mytime(nullptr);
				if (session.rights == 2)
				{
					if (!sSID.empty())
					{
						WebEmSession* oldSession = myWebem->GetSession(sSID);
						if (oldSession == nullptr)
						{
							session.id = sSID;
							session.auth_token = sAuthToken;
							expired = (!checkAuthToken(session));
						}
						else
						{
							session = *oldSession;
							expired = (oldSession->expires < now);
						}
					}
					if (sSID.empty() || expired)
						session.isnew = true;
					return true;
				}

				if (!(sSID.empty() || sAuthToken.empty() || szTime.empty()))
				{
					WebEmSession* oldSession = myWebem->GetSession(sSID);
					if ((oldSession != nullptr) && (oldSession->expires < now))
					{
						// Check if session stored in memory is not expired (prevent from spoofing expiration time)
						expired = true;
					}
					if (expired)
					{
						//expired session, remove session
						m_failcounter = 0;
						if (oldSession != nullptr)
						{
							// session exists (delete it from memory and database)
							myWebem->RemoveSession(sSID);
							removeAuthToken(sSID);
						}
						return false;
					}
					if (oldSession != nullptr)
					{
						// session already exists
						session = *oldSession;
					}
					else
					{
						// Session does not exists
						session.id = sSID;
					}
					session.auth_token = sAuthToken;
					// Check authen_token and restore session
					if (checkAuthToken(session))
					{
						// user is authenticated
						return true;
					}

					return false;

				}
				// invalid cookie
				if (CheckAuthByPass(req))
					return true;

				// Force login form
				return false;
			}

			// Not sure why this is here? Isn't this the case for all situation where the session ID is empty? Not only with admins
			if ((session.rights == URIGHTS_ADMIN) && (session.id.empty()))
			{
				session.isnew = true;
				return true;
			}

			return false;
		}

		/**
		 * Check authentication token if exists and restore the user session if necessary
		 */
		bool cWebemRequestHandler::checkAuthToken(WebEmSession & session)
		{
			session_store_impl_ptr sstore = myWebem->GetSessionStore();
			if (sstore == nullptr)
			{
				_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : no store defined", session.id.c_str(), session.auth_token.c_str());
				return true;
			}

			if (session.id.empty() || session.auth_token.empty())
			{
				_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : session id or auth token is empty", session.id.c_str(), session.auth_token.c_str());
				return false;
			}
			WebEmStoredSession storedSession = sstore->GetSession(session.id);
			if (storedSession.id.empty())
			{
				_log.Debug(DEBUG_AUTH, "[web:%s] CheckAuthToken(%s_%s) : session id not found", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str());
				return false;
			}
			if (storedSession.auth_token != GenerateMD5Hash(session.auth_token))
			{
				_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : auth token mismatch", session.id.c_str(), session.auth_token.c_str());
				removeAuthToken(session.id);
				return false;
			}

			_log.Debug(DEBUG_AUTH, "[web:%s] CheckAuthToken(%s_%s_%s) : user authenticated", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());

			if (session.rights == 2)
			{
				// we are already admin - restore session from db
				session.expires = storedSession.expires;
				time_t now = mytime(nullptr);
				if (session.expires < now)
				{
					removeAuthToken(session.id);
					return false;
				}
				session.timeout = now + SHORT_SESSION_TIMEOUT;
				myWebem->AddSession(session);
				return true;
			}

			if (session.username.empty())
			{
				// Restore session if user exists and session does not already exist
				bool userExists = false;
				bool sessionExpires = false;
				session.username = storedSession.username;
				session.expires = storedSession.expires;
				for (const auto &my : myWebem->m_userpasswords)
				{
					if (my.Username == session.username) // the user still exists
					{
						userExists = true;
						session.rights = my.userrights;
						break;
					}
				}

				time_t now = mytime(nullptr);
				sessionExpires = session.expires < now;

				if (!userExists || sessionExpires)
				{
					_log.Debug(DEBUG_AUTH, "[web:%s] CheckAuthToken(%s_%s) : cannot restore session, user not found or session expired", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str());
					removeAuthToken(session.id);
					return false;
				}

				WebEmSession* oldSession = myWebem->GetSession(session.id);
				if (oldSession == nullptr)
				{
					_log.Debug(DEBUG_AUTH, "[web:%s] CheckAuthToken(%s_%s_%s) : restore session", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
					myWebem->AddSession(session);
				}
			}

			return true;
		}

		void cWebemRequestHandler::removeAuthToken(const std::string & sessionId)
		{
			session_store_impl_ptr sstore = myWebem->GetSessionStore();
			if (sstore != nullptr)
			{
				sstore->RemoveSession(sessionId);
			}
		}

		char *cWebemRequestHandler::strftime_t(const char *format, const time_t rawtime)
		{
			static char buffer[1024];
			struct tm ltime;
			localtime_r(&rawtime, &ltime);
			strftime(buffer, sizeof(buffer), format, &ltime);
			return buffer;
		}

		std::map<std::string, connection::_tRemoteClients> m_remote_web_clients;

		void cWebemRequestHandler::handle_request(const request& req, reply& rep)
		{
			if(_log.IsDebugLevelEnabled(DEBUG_WEBSERVER))
			{
				_log.Debug(DEBUG_WEBSERVER, "[web:%s] Host:%s Uri:%s", myWebem->GetPort().c_str(), req.host_remote_address.c_str(), req.uri.c_str());
				if(_log.IsDebugLevelEnabled(DEBUG_RECEIVED))
				{
					std::string sHeaders;
					for (const auto &header : req.headers)
					{
						sHeaders += header.name + ": " + header.value + "\n";
					}
					_log.Debug(DEBUG_RECEIVED, "[web:%s] Request Headers:\n%s", myWebem->GetPort().c_str(), sHeaders.c_str());
				}
			}

			if(!myWebem->CheckVHost(req))
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Decode url to path.
			std::string request_path;
			if (!request_handler::url_decode(req.uri, request_path))
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// Initialize session
			WebEmSession session;
			session.remote_host = req.host_remote_address;
			session.remote_port = req.host_remote_port;
			session.local_host = req.host_local_address;
			session.local_port = req.host_local_port;
			session.rights = -1;

			// Let's examine possible proxies, etc.
			std::string realHost;
			bool bUseRealHost = false;
			if(!myWebem->findRealHostBehindProxies(req, realHost))
			{
				_log.Log(LOG_ERROR, "[web:%s]: Unable to determine origin due to improper proxy header(s) (values) being used (Possible spoofing attempt!?), dropping client request (remote address: %s)", myWebem->GetPort().c_str(), session.remote_host.c_str());
				rep = reply::stock_reply(reply::forbidden);
				return;
			}
			else if (!realHost.empty())
			{
				if (AreWeInTrustedNetwork(session.remote_host))
				{	// We only use Proxy header information if the connection Domotic receives comes from a Trusted network
					session.remote_host = realHost;		// replace the host of the connection with the originating host behind the proxies
					rep.originHost = realHost;
					bUseRealHost = true;
				}
			}

			std::string remoteClientKey = session.remote_host + session.local_port;
			auto itt_rc = m_remote_web_clients.find(remoteClientKey);
			if (itt_rc == m_remote_web_clients.end())
			{
				connection::_tRemoteClients rc;
				rc.host_remote_endpoint_address_ = session.remote_host;
				rc.host_local_endpoint_port_ = session.local_port;
				m_remote_web_clients[remoteClientKey] = rc;
				itt_rc = m_remote_web_clients.find(remoteClientKey);
			}
			itt_rc->second.last_seen = mytime(nullptr);
			itt_rc->second.host_last_request_uri_ = req.uri;

			session.reply_status = reply::ok;
			session.isnew = false;
			session.rememberme = false;

			rep.status = reply::ok;
			rep.bIsGZIP = false;

			// Respond to CORS Preflight request (for JSON API)
			if (req.method == "OPTIONS")
			{
				reply::add_header(&rep, "Content-Length", "0");
				reply::add_header(&rep, "Content-Type", "text/plain");
				reply::add_header(&rep, "Access-Control-Max-Age", "3600");
				reply::add_header(&rep, "Access-Control-Allow-Methods", "GET, POST");
				reply::add_header(&rep, "Access-Control-Allow-Headers", "Authorization, Content-Type");
				reply::add_cors_headers(&rep);
				return;
			}

			bool isPage = myWebem->IsPageOverride(req, rep);
			bool isAction = myWebem->IsAction(req);		// This is used but will be removed in the future and replaced by the JSON API commands

			if (isPage && (req.uri.find("dologout") != std::string::npos))
			{
				//Remove session id based on cookie
				std::string sSID;
				std::string sAuthToken;
				std::string szTime;
				bool expired = false;

				session.username = "";
				session.rights = -1;
				rep = reply::stock_reply(reply::no_content);
				if(bUseRealHost)
					rep.originHost = realHost;
				if(parse_cookie(req, sSID, sAuthToken, szTime, expired))
				{
					_log.Debug(DEBUG_AUTH, "[web:%s] Logout : remove session %s", myWebem->GetPort().c_str(), sSID.c_str());
					myWebem->RemoveSession(sSID);
					removeAuthToken(sSID);
					send_remove_cookie(rep);
				}
				return;
			}

			// Check if this is an upgrade request to a websocket connection
			bool isUpgradeRequest = is_upgrade_request(session, req, rep);

			// Does the request needs to be Authorized?
			bool needsAuthentication = (!CheckAuthByPass(req));
			bool isAuthenticated = CheckAuthentication(session, req, rep);

			_log.Debug(DEBUG_AUTH,"[web:%s] isPage %d isAction %d isUpgrade %d needsAuthentication %d isAuthenticated %d (%s)", myWebem->GetPort().c_str(), isPage, isAction, isUpgradeRequest, needsAuthentication, isAuthenticated, session.username.c_str());

			// Check user authentication on each page or action, if it exists.
			if ((isPage || isAction || isUpgradeRequest) && needsAuthentication && !isAuthenticated)
			{
				_log.Debug(DEBUG_WEBSERVER, "[web:%s] Did not find suitable Authorization!", myWebem->GetPort().c_str());
				send_authorization_request(rep);
				if(bUseRealHost)
					rep.originHost = realHost;
				return;
			}
			if (isUpgradeRequest)	// And authorized, which has been checked above
			{
				return;
			}

			// Copy the request to be able to fill its parameters attribute
			request requestCopy = req;

			bool bHandledAction = false;
			// Run action if exists
			if (isAction)
			{
				// Post actions only allowed when authenticated and user has admin rights
				if (session.rights != URIGHTS_ADMIN)
				{
					rep = reply::stock_reply(reply::forbidden);
					return;
				}
				bHandledAction = myWebem->CheckForAction(session, requestCopy);
				if (bHandledAction && !requestCopy.uri.empty())
				{
					if ((requestCopy.method == "POST") && (requestCopy.uri[0] != '/'))
					{
						//Send back as data instead of a redirect uri
						rep.status = reply::ok;
						rep.content = requestCopy.uri;
						reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
						reply::add_header(&rep, "Last-Modified", make_web_time(mytime(nullptr)), true);
						reply::add_header_content_type(&rep, "application/json");
						return;
					}
				}
			}

			if (!bHandledAction)
			{
				if (myWebem->CheckForPageOverride(session, requestCopy, rep))
				{
					if (rep.status == reply::status_type::download_file)
						return;

					if (!rep.bIsGZIP)
					{
						CompressWebOutput(req, rep);
					}
				}
				else
				{
					// do normal handling
					modify_info mInfo;
					try
					{
						if (myWebem->m_actTheme.find("default") == std::string::npos)
						{
							// A theme is being used (not default) so some theme specific processing might be neccessary
							std::string uri = myWebem->ExtractRequestPath(requestCopy.uri);
							if (uri.find("/images/") == 0)
							{
								std::string theme_images_path = myWebem->m_actTheme + uri;
								if (file_exist((doc_root_ + theme_images_path).c_str()))
								{
									requestCopy.uri = myWebem->GetWebRoot() + theme_images_path;
									_log.Debug(DEBUG_WEBSERVER, "[web:%s] modified images request to (%s).", uri.c_str(), requestCopy.uri.c_str());
								}
							}
							else if (uri.find("/styles/") == 0)
							{
								std::string theme_styles_path = myWebem->m_actTheme + uri.substr(15);
								if (file_exist((doc_root_ + theme_styles_path).c_str()))
								{
									requestCopy.uri = myWebem->GetWebRoot() + theme_styles_path;
									_log.Debug(DEBUG_WEBSERVER, "[web:%s] modified request to (%s).", uri.c_str(), requestCopy.uri.c_str());
								}
							}
						}

						request_handler::handle_request(requestCopy, rep, mInfo);
					}
					catch (...)
					{
						rep = reply::stock_reply(reply::internal_server_error);
						return;
					}
				}
			}

			// Set timeout to make session in use
			session.timeout = mytime(nullptr) + SHORT_SESSION_TIMEOUT;

			if ((session.isnew == true) &&
				(session.rights == URIGHTS_ADMIN) &&
				(req.uri.find("json.htm") != std::string::npos) &&
				(req.uri.find("logincheck") == std::string::npos)
				)
			{
				// client is possibly a script that does not send cookies - see if we have the IP address registered as a session ID
				WebEmSession* memSession = myWebem->GetSession(session.remote_host);
				time_t now = mytime(nullptr);
				if (memSession != nullptr)
				{
					if (memSession->expires < now)
					{
						myWebem->RemoveSession(session.remote_host);
					}
					else
					{
						session.isnew = false;
						if (memSession->expires - (SHORT_SESSION_TIMEOUT / 2) < now)
						{
							memSession->expires = now + SHORT_SESSION_TIMEOUT;

							// unsure about the point of the forced removal of 'live' sessions and restore from
							// database but these 'fake' sessions are memory only and can't be restored that way.
							// Should I do a RemoveSession() followed by a AddSession()?
							// For now: keep 'timeout' in sync with 'expires'
							memSession->timeout = memSession->expires;
						}
					}
				}

				if (session.isnew == true)
				{
					// register a 'fake' IP based session so we can reference that if the client returns here
					session.id = session.remote_host;
					session.rights = -1; // predictable session ID must have no rights
					session.expires = session.timeout;
					myWebem->AddSession(session);
					session.rights = 2; // restore session rights
				}
			}

			if (session.isnew == true)
			{
				_log.Log(LOG_STATUS, "[web:%s] Incoming connection from: %s", myWebem->GetPort().c_str(), session.remote_host.c_str());
				// Create a new session ID
				session.id = generateSessionID();
				session.expires = session.timeout;
				if (session.rememberme)
				{
					// Extend session by 30 days
					session.expires += LONG_SESSION_TIMEOUT;
				}
				session.auth_token = generateAuthToken(session, req); // do it after expires to save it also
				session.isnew = false;
				myWebem->AddSession(session);
				send_cookie(rep, session);
			}
			else if (!session.id.empty())
			{
				// Renew session expiration and authentication token
				WebEmSession* memSession = myWebem->GetSession(session.id);
				if (memSession != nullptr)
				{
					time_t now = mytime(nullptr);
					// Renew session expiration date if half of session duration has been exceeded ("dont remember me" sessions, 10 minutes)
					if (memSession->expires - (SHORT_SESSION_TIMEOUT / 2) < now)
					{
						memSession->expires = now + SHORT_SESSION_TIMEOUT;
						memSession->auth_token = generateAuthToken(*memSession, req); // do it after expires to save it also
						send_cookie(rep, *memSession);
					}
					// Renew session expiration date if half of session duration has been exceeded ("remember me" sessions, 30 days)
					else if ((memSession->expires > SHORT_SESSION_TIMEOUT + now) && (memSession->expires - (LONG_SESSION_TIMEOUT / 2) < now))
					{
						memSession->expires = now + LONG_SESSION_TIMEOUT;
						memSession->auth_token = generateAuthToken(*memSession, req); // do it after expires to save it also
						send_cookie(rep, *memSession);
					}
				}
			}
		}

	} // namespace server
} // namespace http
