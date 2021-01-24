//mainpage WEBEM
//
//Detailed class and method documentation of the WEBEM C++ embedded web server source code.
//
//Modified, extended etc by Robbert E. Peters/RTSS B.V.
#include "stdafx.h"
#include "cWebem.h"
#include <boost/bind/bind.hpp>
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
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#define SHORT_SESSION_TIMEOUT 600 // 10 minutes
#define LONG_SESSION_TIMEOUT (30 * 86400) // 30 days

#ifdef _WIN32
#define gmtime_r(timep, result) gmtime_s(result, timep)
#endif

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
			, m_settings(settings)
			, mySessionStore(nullptr)
			, myRequestHandler(doc_root, this)
			// Rene, make sure we initialize m_sessions first, before starting a server
			, myServer(server_factory::create(settings, myRequestHandler))
			, m_io_service()
			, m_session_clean_timer(m_io_service, boost::posix_time::minutes(1))
		{
			// associate handler to timer and schedule the first iteration
			m_session_clean_timer.async_wait(boost::bind(&cWebem::CleanSessions, this));
			m_io_service_thread = std::make_shared<std::thread>(boost::bind(&boost::asio::io_service::run, &m_io_service));
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


		/**

		Create a link between a string ID and a function to calculate the dynamic content of the string

		The function should return a pointer to a character buffer.  This should be contain only ASCII characters
		( Unicode code points 1 to 127 )

		@param[in] idname  string identifier
		@param[in] fun pointer to function which calculates the string to be displayed

		*/

		void cWebem::RegisterIncludeCode(const char *idname, const webem_include_function &fun)
		{
			myIncludes.insert(std::pair<std::string, webem_include_function >(std::string(idname), fun));
		}
		/**

		Create a link between a string ID and a function to calculate the dynamic content of the string

		The function should return a pointer to wide character buffer.  This should contain a wide character UTF-16 encoded unicode string.
		WEBEM will convert the string to UTF-8 encoding before sending to the browser.

		@param[in] idname  string identifier
		@param[in] fun pointer to function which calculates the string to be displayed

		*/

		void cWebem::RegisterIncludeCodeW(const char *idname, const webem_include_function_w &fun)
		{
			myIncludes_w.insert(std::pair<std::string, webem_include_function_w >(std::string(idname), fun));
		}

		void cWebem::RegisterPageCode(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication)
		{
			myPages.insert(std::pair<std::string, webem_page_function >(std::string(pageurl), fun));
			if (bypassAuthentication)
			{
				RegisterWhitelistURLString(pageurl);
			}
		}
		void cWebem::RegisterPageCodeW(const char *pageurl, const webem_page_function &fun, bool bypassAuthentication)
		{
			myPages_w.insert(std::pair<std::string, webem_page_function >(std::string(pageurl), fun));
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
		

		/**

		  Do not call from application code, used by server to include generated text.

		  @param[in/out] reply  text to include generated

		  The text is searched for "<!--#cWebemX-->".
		  The X can be any string not containing "-->"

		  If X has been registered with cWebem then the associated function
		  is called to generate text to be inserted.

		  returns true if any text is replaced


		*/
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
				std::map < std::string, webem_include_function >::iterator pf = myIncludes.find(code);
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
				else
				{
					// no function found, look for a wide character fuction
					std::map < std::string, webem_include_function_w >::iterator pf = myIncludes_w.find(code);
					if (pf != myIncludes_w.end())
					{
						// function found
						// get return string and convert from UTF-16 to UTF-8
						std::wstring content_part_w;
						try
						{
							pf->second(content_part_w);
						}
						catch (...)
						{

						}
						cUTF utf(content_part_w.c_str());
						// insert generated text
						reply.insert(p, utf.get8());
						res = true;
					}
				}

				// adjust pointer into text for insertion
				p = q + reply.length() - reply_len;
			}
			return res;
		}

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

		bool cWebem::IsAction(const request& req)
		{
			// look for cWebem form action request
			std::string uri = req.uri;
			size_t q = uri.find(".webem");
			if (q == std::string::npos)
				return false;
			return true;
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

			req.parameters.clear();

			std::string uri = ExtractRequestPath(req.uri);

			// find function matching action code
			size_t q = uri.find(".webem");
			std::string code = uri.substr(1, q - 1);
			std::map < std::string, webem_action_function >::iterator
				pfun = myActions.find(code);
			if (pfun == myActions.end())
				return false;

			// decode the values

			if (req.method == "POST")
			{
				const char *pContent_Type = request::get_req_header(&req, "Content-Type");
				if (pContent_Type)
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
								return true;
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
						//we should have at least one value
						if (req.parameters.empty())
							return false;
						// call the function
						try
						{
							pfun->second(session, req, req.uri);
						}
						catch (...)
						{

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
					if ((strstr(pContent_Type, "text/plain") != nullptr) || (strstr(pContent_Type, "application/json") != nullptr) ||
					    (strstr(pContent_Type, "application/xml") != nullptr))
					{
						//Raw data
						req.parameters.insert(std::pair< std::string, std::string >("data", req.content));
						// call the function
						try
						{
							pfun->second(session, req, req.uri);
						}
						catch (...)
						{

						}
						return true;
					}
				}
				uri = req.content;
				q = 0;
			}
			else
			{
				q += 7;
			}

			std::string name;
			std::string value;

			size_t p = q;
			int flag_done = 0;
			while (!flag_done)
			{
				q = uri.find('=', p);
				if (q == std::string::npos)
					return false;
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

				req.parameters.insert(std::pair< std::string, std::string >(name, value));
				p = q + 1;
			}

			// call the function
			try
			{
				pfun->second(session, req, req.uri);
			}
			catch (...)
			{

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

		bool cWebem::IsPageOverride(const request& req, reply& rep)
		{
			std::string request_path;
			if (!request_handler::url_decode(req.uri, request_path))
			{
				rep = reply::stock_reply(reply::bad_request);
				return false;
			}

			request_path = ExtractRequestPath(request_path);

			size_t paramPos = request_path.find_first_of('?');
			if (paramPos != std::string::npos)
			{
				request_path = request_path.substr(0, paramPos);
			}

			std::map < std::string, webem_page_function >::iterator
				pfun = myPages.find(request_path);

			if (pfun != myPages.end())
				return true;
			//check wchar_t
			std::map < std::string, webem_page_function >::iterator
				pfunW = myPages_w.find(request_path);
			if (pfunW != myPages_w.end())
				return true;
			return false;
		}

		bool cWebem::CheckForPageOverride(WebEmSession & session, request& req, reply& rep)
		{
			// Decode url to path.
			std::string request_path;
			if (!request_handler::url_decode(req.uri, request_path))
			{
				rep = reply::stock_reply(reply::bad_request);
				return false;
			}

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
					if (strstr(pContent_Type, "multipart/form-data") != nullptr)
					{
						std::string szContent = req.content;
						size_t pos;
						std::string szVariable, szContentType, szValue;

						//first line is our boundary
						pos = szContent.find("\r\n");
						if (pos == std::string::npos)
							return true;
						std::string szBoundary = szContent.substr(0, pos);
						szContent = szContent.substr(pos + 2);

						while (!szContent.empty())
						{
							//Next line will contain our variable name
							pos = szContent.find("\r\n");
							if (pos == std::string::npos)
								return true;
							szVariable = szContent.substr(0, pos);
							szContent = szContent.substr(pos + 2);
							if (szVariable.find("Content-Disposition") != 0)
								return true;
							pos = szVariable.find("name=\"");
							if (pos == std::string::npos)
								return true;
							szVariable = szVariable.substr(pos + 6);
							pos = szVariable.find('"');
							if (pos == std::string::npos)
								return true;
							szVariable = szVariable.substr(0, pos);
							//Next line could be empty, or a Content-Type, if its empty, it is just a string
							pos = szContent.find("\r\n");
							if (pos == std::string::npos)
								return true;
							szContentType = szContent.substr(0, pos);
							szContent = szContent.substr(pos + 2);
							if (
								(szContentType.find("application/octet-stream") != std::string::npos)
								|| (szContentType.find("application/json") != std::string::npos)
								|| (szContentType.find("application/x-zip") != std::string::npos)
								|| (szContentType.find("application/zip") != std::string::npos)
								|| (szContentType.find("Content-Type: text/xml") != std::string::npos)
								)
							{
								//Its a file/stream, next line should be empty
								pos = szContent.find("\r\n");
								if (pos == std::string::npos)
									return true;
								szContent = szContent.substr(pos + 2);
							}
							else
							{
								//next line should be empty
								if (!szContentType.empty())
									return true;//dont know this one
							}
							pos = szContent.find(szBoundary);
							if (pos == std::string::npos)
								return true;
							szValue = szContent.substr(0, pos - 2);
							req.parameters.insert(std::pair< std::string, std::string >(szVariable, szValue));

							szContent = szContent.substr(pos + szBoundary.size());
							pos = szContent.find("\r\n");
							if (pos == std::string::npos)
								return true;
							szContent = szContent.substr(pos + 2);
						}
						//we should have at least one value
						if (req.parameters.empty())
							return true;
					} // if (strstr(pContent_Type, "multipart/form-data") != NULL)
					else if (strstr(pContent_Type, "application/x-www-form-urlencoded") != nullptr)
					{
						std::string params = req.content;
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

			std::map < std::string, webem_page_function >::iterator
				pfun = myPages.find(request_path);

			if (pfun != myPages.end())
			{
				rep.status = reply::ok;
				try
				{
					pfun->second(session, req, rep);
				}
				catch (std::exception& e)
				{
					_log.Log(LOG_ERROR, "WebServer PO exception occurred : '%s'", e.what());
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "WebServer PO unknown exception occurred");
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
					if (!strMimeType.empty())
						strMimeType += ";charset=UTF-8";
					reply::add_header(&rep, "Cache-Control", "no-cache");
					reply::add_header(&rep, "Pragma", "no-cache");
					reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				}
				else
				{
					reply::add_header(&rep, "Cache-Control", "max-age=3600, public");
				}
				reply::add_header_content_type(&rep, strMimeType);
				return true;
			}

			//check wchar_t
			std::map < std::string, webem_page_function >::iterator
				pfunW = myPages_w.find(request_path);
			if (pfunW == myPages_w.end())
				return false;

			try
			{
				pfunW->second(session, req, rep);
			}
			catch (...)
			{

			}

			rep.status = reply::ok;
			reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
			reply::add_header(&rep, "Content-Type", strMimeType + "; charset=UTF-8");
			reply::add_header(&rep, "Cache-Control", "no-cache");
			reply::add_header(&rep, "Pragma", "no-cache");
			reply::add_header(&rep, "Access-Control-Allow-Origin", "*");

			return true;
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

			// If path ends in slash (i.e. is a directory) then add "index.html".
			if (request_path[request_path.size() - 1] == '/')
			{
				request_path += "index.html";
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

			if (request_path.find("/acttheme/") == 0)
			{
				request_path = m_actTheme + request_path.substr(9);
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

		void cWebem::AddUserPassword(const unsigned long ID, const std::string &username, const std::string &password, const _eUserRights userrights, const int activetabs)
		{
			_tWebUserPassword wtmp;
			wtmp.ID = ID;
			wtmp.Username = username;
			wtmp.Password = password;
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

		void cWebem::AddLocalNetworks(std::string network)
		{
			_tIPNetwork ipnetwork;
			ipnetwork.bIsIPv6 = (network.find(':') != std::string::npos);

			uint8_t iASize = (!ipnetwork.bIsIPv6) ? 4 : 16;
			int ii;

			if (network.empty())
			{
				//add local host
				char szLocalHostname[256];
				if (gethostname(szLocalHostname, sizeof(szLocalHostname)) == SOCKET_ERROR)
					return; //Could not retreive hostname
				network = szLocalHostname;
			}

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
					else if (inet_pton((!ipnetwork.bIsIPv6) ? AF_INET : AF_INET6, network.c_str(), &ipnetwork.Network)
						 != 1)
						return; //invalid address
					memset((void*)&ipnetwork.Mask, 0xFF, iASize);

					//Apply mask to network address
					for (ii = 0; ii < iASize; ii++)
						ipnetwork.Network[ii] = ipnetwork.Network[ii] & ipnetwork.Mask[ii];
				}
			}

			m_localnetworks.push_back(ipnetwork);
		}

		void cWebem::ClearLocalNetworks()
		{
			m_localnetworks.clear();
		}

		void cWebem::AddRemoteProxyIPs(const std::string &ipaddr)
		{
			myRemoteProxyIPs.push_back(ipaddr);
		}

		void cWebem::ClearRemoteProxyIPs()
		{
			myRemoteProxyIPs.clear();
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

		// Check the user's password, return 1 if OK
		static int check_password(struct ah *ah, const std::string &ha1, const std::string &realm)
		{
			if ((ah->nonce.empty()) && (!ah->response.empty()))
				return (ha1 == GenerateMD5Hash(ah->response));

			return 0;
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
			m_session_clean_timer.async_wait(boost::bind(&cWebem::CleanSessions, this));
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

				ah->user = decoded.substr(0, npos);
				ah->response = decoded.substr(npos + 1);
				return 1;
			}

			return 0;
		}

		// Authorize against the opened passwords file. Return 1 if authorized.
		int cWebemRequestHandler::authorize(WebEmSession & session, const request& req, reply& rep)
		{
			struct ah _ah;

			std::string uname;
			std::string upass;

			if (!parse_auth_header(req, &_ah))
			{
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
				if (request_handler::url_decode(tmpuname, uname))
				{
					if (request_handler::url_decode(tmpupass, upass))
					{
						uname = base64_decode(uname);
						upass = GenerateMD5Hash(base64_decode(upass));

						for (const auto &my : myWebem->m_userpasswords)
						{
							if (my.Username == uname)
							{
								if (my.Password != upass)
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
					}
				}
				m_failcounter++;
				return 0;
			}

			for (const auto &my : myWebem->m_userpasswords)
			{
				if (my.Username == _ah.user)
				{
					int bOK = check_password(&_ah, my.Password, myWebem->m_DigistRealm);
					if (!bOK)
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

		bool IsIPInRange(const std::string &ip, const _tIPNetwork &ipnetwork)
		{
			bool bIsIPv6 = (ip.find(':') != std::string::npos);
			if (ipnetwork.bIsIPv6 != bIsIPv6)
				return false;

			uint8_t IP[16] = { 0 };
			if (inet_pton((!bIsIPv6) ? AF_INET : AF_INET6, ip.c_str(), &IP) != 1)
				return true;

			int iASize = (!bIsIPv6) ? 4 : 16;
			for (int ii = 0; ii < iASize; ii++)
				if (ipnetwork.Network[ii] != (IP[ii] & ipnetwork.Mask[ii]))
					return false;

			return true;
		}

		//Returns true is the connected host is in the local network
		bool cWebemRequestHandler::AreWeInLocalNetwork(const std::string &sHost, const request& req)
		{
			//check if in local network(s)
			if (myWebem->m_localnetworks.empty())
				return false;
			if (sHost.size() < 3)
				return false;

			return std::any_of(myWebem->m_localnetworks.begin(), myWebem->m_localnetworks.end(),
					   [&](const _tIPNetwork &my) { return IsIPInRange(sHost, my); });
		}

		constexpr std::array<const char *, 12> months{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		constexpr std::array<const char *, 7> wkdays{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

		char *make_web_time(const time_t rawtime)
		{
			static char buffer[256];
			struct tm gmt;
#ifdef _WIN32
			if (gmtime_r(&rawtime, &gmt)) //windows returns errno_t, which returns zero when successful
#else
			if (gmtime_r(&rawtime, &gmt) == nullptr)
#endif
			{
				strcpy(buffer, "Thu, 1 Jan 1970 00:00:00 GMT");
			}
			else
			{
				sprintf(buffer, "%s, %02d %s %04d %02d:%02d:%02d GMT",
					wkdays[gmt.tm_wday],
					gmt.tm_mday,
					months[gmt.tm_mon],
					gmt.tm_year + 1900,
					gmt.tm_hour,
					gmt.tm_min,
					gmt.tm_sec);


			}
			return buffer;
		}

		void cWebemRequestHandler::send_remove_cookie(reply& rep)
		{
			std::stringstream sstr;
			sstr << "DMZSID=none";
			// RK, we removed path=/ so you can be logged in to two Domoticz's at the same time on https://my.domoticz.com/.
			sstr << "; HttpOnly; Expires=" << make_web_time(0);
			reply::add_header(&rep, "Set-Cookie", sstr.str(), false);
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

			_log.Debug(DEBUG_WEBSERVER, "[web:%s] generate new authentication token %s", myWebem->GetPort().c_str(), authToken.c_str());

			session_store_impl_ptr sstore = myWebem->GetSessionStore();
			if (sstore != nullptr)
			{
				WebEmStoredSession storedSession;
				storedSession.id = session.id;
				storedSession.auth_token = GenerateMD5Hash(authToken); // only save the hash to avoid a security issue if database is stolen
				storedSession.username = session.username;
				storedSession.expires = session.expires;
				storedSession.remote_host = session.remote_host; // to trace host
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

		void cWebemRequestHandler::send_authorization_request(reply& rep)
		{
			rep = reply::stock_reply(reply::unauthorized);
			rep.status = reply::unauthorized;
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
			const int sha1len = 20;
			// the GUID as specified in RFC 6455
			const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
			std::string combined = websocket_key + GUID;
			unsigned char sha1result[sha1len];
			sha1::calc((void *)combined.c_str(), combined.length(), sha1result);
			std::string accept = base64_encode(sha1result, sha1len);
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

/*
			std::string connection_header = h;
			if (!boost::iequals(connection_header, "upgrade"))
			{
				return false;
			};
*/
			// client MUST include Upgrade: websocket
			h = request::get_req_header(&req, "Upgrade");
			if (!h)
			{
				return false;
			}

			if (!CheckAuthentication(session, req, rep))
				return false;


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
			h = request::get_req_header(&req, "Origin");
			// request MUST include an origin header, even if we don't check it
			// we only "allow" connections from browser clients
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::forbidden);
				return true;
			}
			h = request::get_req_header(&req, "Sec-Websocket-Version");
			// request MUST include a version number
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::internal_server_error);
				return true;
			}
			int version = atoi(h);
			// we support versions 13 (and higher)
			if (version < 13)
			{
				rep = reply::stock_reply(reply::internal_server_error);
				return true;
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
				rep = reply::stock_reply(reply::internal_server_error);
				return true;
			}
			h = request::get_req_header(&req, "Sec-Websocket-Key");
			// request MUST include a sec-websocket-key header and we need to respond to it
			if (h == nullptr)
			{
				rep = reply::stock_reply(reply::internal_server_error);
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

		bool cWebemRequestHandler::CheckAuthentication(WebEmSession & session, const request& req, reply& rep)
		{
			session.rights = -1; // no rights
			session.id = "";

			if (myWebem->m_userpasswords.empty())
			{
				session.rights = 2;
			}

			if (AreWeInLocalNetwork(session.remote_host, req))
			{
				session.rights = 2;
			}

			//Check cookie if still valid
			const char* cookie_header = request::get_req_header(&req, "Cookie");
			if (cookie_header != nullptr)
			{
				std::string sSID;
				std::string sAuthToken;
				std::string szTime;
				bool expired = false;

				// Parse session id and its expiration date
				std::string scookie = cookie_header;
				size_t fpos = scookie.find("DMZSID=");
				if (fpos != std::string::npos)
				{
					scookie = scookie.substr(fpos);
					fpos = 0;
					size_t epos = scookie.find(';');
					if (epos != std::string::npos)
					{
						scookie = scookie.substr(0, epos);
					}
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
						send_authorization_request(rep);
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

					send_authorization_request(rep);
					return false;

				}
				// invalid cookie
				if (myWebem->m_authmethod != AUTH_BASIC)
				{
					// Check if we need to bypass authentication (not when using basic-auth)
					for (const auto &url : myWebem->myWhitelistURLs)
						if (req.uri.find(url) == 0)
							return true;

					std::string cmdparam;
					if (GetURICommandParameter(req.uri, cmdparam))
						for (const auto &cmd : myWebem->myWhitelistCommands)
							if (cmdparam.find(cmd) == 0)
								return true;

					// Force login form
					send_authorization_request(rep);
					return false;
				}
			}

			if ((session.rights == 2) && (session.id.empty()))
			{
				session.isnew = true;
				return true;
			}

			//patch to let always support basic authentication function for script calls
			if (req.uri.find("json.htm") != std::string::npos)
			{
				//Check first if we have a basic auth
				if (authorize(session, req, rep))
					return true;
			}

			if (myWebem->m_authmethod == AUTH_BASIC)
			{
				if (!authorize(session, req, rep))
				{
					if (m_failcounter > 0)
					{
						_log.Log(LOG_ERROR, "[web:%s] Failed authentication attempt, ignoring client request (remote address: %s)", myWebem->GetPort().c_str(), session.remote_host.c_str());
					}
					if (m_failcounter > 2)
					{
						m_failcounter = 0;
						rep = reply::stock_reply(reply::service_unavailable);
						return false;
					}
					send_authorization_request(rep);
					return false;
				}
				return true;
			}

			//Check if we need to bypass authentication (not when using basic-auth)
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

			send_authorization_request(rep);
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
				_log.Log(LOG_ERROR, "CheckAuthToken([%s_%s]) : no store defined", session.id.c_str(), session.auth_token.c_str());
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
				_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : session id not found", session.id.c_str(), session.auth_token.c_str());
				return false;
			}
			if (storedSession.auth_token != GenerateMD5Hash(session.auth_token))
			{
				_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : auth token mismatch", session.id.c_str(), session.auth_token.c_str());
				removeAuthToken(session.id);
				return false;
			}

			_log.Debug(DEBUG_WEBSERVER, "[web:%s] CheckAuthToken(%s_%s_%s) : user authenticated", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());

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
					_log.Debug(DEBUG_WEBSERVER, "[web:%s] CheckAuthToken(%s_%s) : cannot restore session, user not found or session expired", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str());
					removeAuthToken(session.id);
					return false;
				}

				WebEmSession* oldSession = myWebem->GetSession(session.id);
				if (oldSession == nullptr)
				{
					_log.Debug(DEBUG_WEBSERVER, "[web:%s] CheckAuthToken(%s_%s_%s) : restore session", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
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

		void cWebemRequestHandler::handle_request(const request& req, reply& rep)
		{
			_log.Debug(DEBUG_WEBSERVER, "web: Host:%s Uri;%s", req.host_address.c_str(), req.uri.c_str());

			// Initialize session
			WebEmSession session;
			session.remote_host = req.host_address;

			if (!myWebem->myRemoteProxyIPs.empty())
			{
				for (auto &myRemoteProxyIP : myWebem->myRemoteProxyIPs)
				{
					if (session.remote_host == myRemoteProxyIP)
					{
						const char *host_header = request::get_req_header(&req, "X-Forwarded-For");
						if (host_header != nullptr)
						{
							if (strstr(host_header, ",") != nullptr)
							{
								//Multiple proxies are used... this is not very common
								host_header = request::get_req_header(&req, "X-Real-IP"); //try our NGINX header
								if (!host_header)
								{
									_log.Log(LOG_ERROR, "Webserver: Multiple proxies are used (Or possible spoofing attempt), ignoring client request (remote address: %s)", session.remote_host.c_str());
									rep = reply::stock_reply(reply::forbidden);
									return;
								}
							}
							session.remote_host = host_header;
						}
					}
				}
			}

			session.reply_status = reply::ok;
			session.isnew = false;
			session.forcelogin = false;
			session.rememberme = false;

			rep.status = reply::ok;
			rep.bIsGZIP = false;

			bool isPage = myWebem->IsPageOverride(req, rep);
			bool isAction = myWebem->IsAction(req);

			// Respond to CORS Preflight request (for JSON API)
			if (req.method == "OPTIONS")
			{
				reply::add_header(&rep, "Content-Length", "0");
				reply::add_header(&rep, "Content-Type", "text/plain");
				reply::add_header(&rep, "Access-Control-Max-Age", "3600");
				reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
				reply::add_header(&rep, "Access-Control-Allow-Methods", "GET, POST");
				reply::add_header(&rep, "Access-Control-Allow-Headers", "Authorization, Content-Type");
				return;
			}

			// Check authentication on each page or action, if it exists.
			bool bCheckAuthentication = false;
			if (isPage || isAction)
			{
				bCheckAuthentication = true;
			}

			if (isPage && (req.uri.find("dologout") != std::string::npos))
			{
				//Remove session id based on cookie
				const char *cookie;
				cookie = request::get_req_header(&req, "Cookie");
				if (cookie != nullptr)
				{
					std::string scookie = cookie;
					size_t fpos = scookie.find("DMZSID=");
					size_t upos = scookie.find('_', fpos);
					if ((fpos != std::string::npos) && (upos != std::string::npos))
					{
						std::string sSID = scookie.substr(fpos + 7, upos - fpos - 7);
						_log.Debug(DEBUG_WEBSERVER, "Web: Logout : remove session %s", sSID.c_str());
						auto itt = myWebem->m_sessions.find(sSID);
						if (itt != myWebem->m_sessions.end())
						{
							myWebem->m_sessions.erase(itt);
						}
						removeAuthToken(sSID);
					}
				}
				session.username = "";
				session.rights = -1;
				session.forcelogin = true;
				bCheckAuthentication = false; // do not authenticate the user, just logout
				send_authorization_request(rep);
				return;
			}

			// Check if this is an upgrade request to a websocket connection
			if (is_upgrade_request(session, req, rep))
			{
				return;
			}
			// Check user authentication on each page or action, if it exists.
			if ((isPage || isAction || bCheckAuthentication) && !CheckAuthentication(session, req, rep))
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
				if (session.rights != 2)
				{
					rep = reply::stock_reply(reply::forbidden);
					return;
				}
				bHandledAction = myWebem->CheckForAction(session, requestCopy);
				if (!requestCopy.uri.empty())
				{
					if ((requestCopy.method == "POST") && (requestCopy.uri[0] != '/'))
					{
						//Send back as data instead of a redirect uri
						rep.status = reply::ok;
						rep.content = requestCopy.uri;
						reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
						reply::add_header(&rep, "Last-Modified", make_web_time(mytime(nullptr)), true);
						reply::add_header(&rep, "Content-Type", "application/json;charset=UTF-8");
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

					if (session.reply_status != reply::ok) // forbidden
					{
						rep = reply::stock_reply(static_cast<reply::status_type>(session.reply_status));
						return;
					}

					if (!rep.bIsGZIP)
					{
						CompressWebOutput(req, rep);
					}
				}
				else
				{
					modify_info mInfo;
					if (session.reply_status != reply::ok)
					{
						rep = reply::stock_reply(static_cast<reply::status_type>(session.reply_status));
						return;
					}
					if (rep.status != reply::ok) // bad request
					{
						return;
					}

					// do normal handling
					try
					{
						std::string uri = myWebem->ExtractRequestPath(requestCopy.uri);
						if (uri.find("/images/") == 0)
						{
							std::string theme_images_path = myWebem->m_actTheme + uri;
							if (file_exist((doc_root_ + theme_images_path).c_str()))
								requestCopy.uri = myWebem->GetWebRoot() + theme_images_path;
						}

						request_handler::handle_request(requestCopy, rep, mInfo);
					}
					catch (...)
					{
						rep = reply::stock_reply(reply::internal_server_error);
						return;
					}

					// find content type header
					std::string content_type;
					for (auto &header : rep.headers)
					{
						if (boost::iequals(header.name, "Content-Type"))
						{
							content_type = header.value;
							break;
						}
					}

					if (content_type == "text/html"
						|| content_type == "text/plain"
						|| content_type == "text/css"
						|| content_type == "text/javascript"
						|| content_type == "application/javascript"
						)
					{
						// check if content is not gzipped, include won't work with non-text content
						if (!rep.bIsGZIP)
						{
							// Find and include any special cWebem strings
							if (!myWebem->Include(rep.content))
							{
								if (mInfo.mtime_support && !mInfo.is_modified)
								{
									_log.Debug(DEBUG_WEBSERVER, "[web:%s] %s not modified (1).", myWebem->GetPort().c_str(), req.uri.c_str());
									rep = reply::stock_reply(reply::not_modified);
									return;
								}
							}

							// adjust content length header
							// ( Firefox ignores this, but apparently some browsers truncate display without it.
							// fix provided by http://www.codeproject.com/Members/jaeheung72 )

							reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));

							if (!mInfo.mtime_support)
							{
								reply::add_header(&rep, "Last-Modified", make_web_time(mytime(nullptr)),
										  true);
							}

							//check gzip support if yes, send it back in gzip format
							CompressWebOutput(req, rep);
						}

						// tell browser that we are using UTF-8 encoding
						reply::add_header(&rep, "Content-Type", content_type + ";charset=UTF-8");
					}
					else if (mInfo.mtime_support && !mInfo.is_modified)
					{
						rep = reply::stock_reply(reply::not_modified);
						_log.Debug(DEBUG_WEBSERVER, "[web:%s] %s not modified (2).", myWebem->GetPort().c_str(), req.uri.c_str());
						return;
					}
					else if (content_type.find("image/") != std::string::npos)
					{
						//Cache images
						reply::add_header(&rep, "Expires",
								  make_web_time(mytime(nullptr) + 3600 * 24 * 365)); // one year
					}
					else
					{
						// tell browser that we are using UTF-8 encoding
						reply::add_header(&rep, "Content-Type", content_type + ";charset=UTF-8");
					}
				}
			}

			// Set timeout to make session in use
			session.timeout = mytime(nullptr) + SHORT_SESSION_TIMEOUT;

			if ((session.isnew == true) &&
				(session.rights == 2) &&
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
				_log.Log(LOG_STATUS, "Incoming connection from: %s", session.remote_host.c_str());
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
			else if (session.forcelogin == true)
			{
				_log.Debug(DEBUG_WEBSERVER, "[web:%s] Logout : remove session %s", myWebem->GetPort().c_str(), session.id.c_str());

				myWebem->RemoveSession(session.id);
				removeAuthToken(session.id);
				if (myWebem->m_authmethod == AUTH_BASIC)
				{
					send_authorization_request(rep);
				}

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
