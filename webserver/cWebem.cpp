//mainpage WEBEM
//
//Detailed class and method documentation of the WEBEM C++ embedded web server source code.
//
//Modified, extended etc by Robbert E. Peters/RTSS B.V.
#include "stdafx.h"
#include "cWebem.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // uuid generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include "reply.hpp"
#include "request.hpp"
#include "mime_types.hpp"
#include "utf.hpp"
#include "Base64.h"
#include "GZipHelper.h"
#include <stdarg.h>
#include <fstream>
#include <sstream>
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#define SHORT_SESSION_TIMEOUT 600 // 10 minutes
#define LONG_SESSION_TIMEOUT (30 * 86400) // 30 days

int m_failcounter=0;

namespace http {
	namespace server {

/**
Webem constructor

@param[in] server_settings  Server settings (IP address, listening port, ssl options...)
@param[in] doc_root path to folder containing html e.g. "./"
*/
cWebem::cWebem(
		const server_settings & settings,
		const std::string& doc_root) :
				m_io_service(),
				m_settings(settings),
				myRequestHandler(doc_root, this),
				m_DigistRealm("Domoticz.com"),
				m_session_clean_timer(m_io_service, boost::posix_time::minutes(1)),
				m_io_service_thread(boost::bind(&boost::asio::io_service::run, &m_io_service)),
				myServer(server_factory::create(settings, myRequestHandler)) {
	m_authmethod = AUTH_LOGIN;
	mySessionStore = NULL;
	// associate handler to timer and schedule the first iteration
	m_session_clean_timer.async_wait(boost::bind(&cWebem::CleanSessions, this));
}

cWebem::~cWebem() {
	// Remove reference to CWebServer before its deletion (fix a "pure virtual method called" exception on server termination)
	mySessionStore = NULL;
	// Delete server (no need with smart pointer)
}

/**

Start the server.

This does not return.

IMPORTANT: This method does not return. If application needs to continue, start new thread with call to this method.

*/
void cWebem::Run() {
	// Start Web server
	if (myServer != NULL) {
		myServer->run();
	}
}

/**

Stop and delete the internal server.

IMPORTANT:  To start the server again, delete it and create a new cWebem instance.

*/
void cWebem::Stop() {
	// Stop session cleaner
	try {
		if (!m_io_service.stopped()) {
			m_io_service.stop();
			m_io_service_thread.join();
		}
	} catch (...) {
		_log.Log(LOG_ERROR, "[web:%s] exception thrown while stopping session cleaner", GetPort().c_str());
	}
	// Stop Web server
	if (myServer != NULL) {
		myServer->stop();
	}
}

void cWebem::SetAuthenticationMethod(const _eAuthenticationMethod amethod)
{
	m_authmethod=amethod;
}

/**

Create a link between a string ID and a function to calculate the dynamic content of the string

The function should return a pointer to a character buffer.  This should be contain only ASCII characters
( Unicode code points 1 to 127 )

@param[in] idname  string identifier
@param[in] fun pointer to function which calculates the string to be displayed

*/

void cWebem::RegisterIncludeCode( const char* idname, webem_include_function fun )
{
	myIncludes.insert( std::pair<std::string, webem_include_function >( std::string(idname), fun  ) );
}
/**

Create a link between a string ID and a function to calculate the dynamic content of the string

The function should return a pointer to wide character buffer.  This should contain a wide character UTF-16 encoded unicode string.
WEBEM will convert the string to UTF-8 encoding before sending to the browser.

@param[in] idname  string identifier
@param[in] fun pointer to function which calculates the string to be displayed

*/

void cWebem::RegisterIncludeCodeW( const char* idname, webem_include_function_w fun )
{
	myIncludes_w.insert( std::pair<std::string, webem_include_function_w >( std::string(idname), fun  ) );
}


void cWebem::RegisterPageCode( const char* pageurl, webem_page_function fun )
{
	myPages.insert( std::pair<std::string, webem_page_function >( std::string(pageurl), fun  ) );
}
void cWebem::RegisterPageCodeW( const char* pageurl, webem_page_function fun )
{
	myPages_w.insert( std::pair<std::string, webem_page_function >( std::string(pageurl), fun  ) );
}


/**

Specify link between form and application function to run when form submitted

@param[in] idname string identifier
@param[in] fun fpointer to function

*/
void cWebem::RegisterActionCode( const char* idname, webem_action_function fun )
{
	myActions.insert( std::pair<std::string, webem_action_function >( std::string(idname), fun  ) );
}

//Used by non basic-auth authentication (for example login forms) to bypass returning false when not authenticated
void cWebem::RegisterWhitelistURLString(const char* idname)
{
	myWhitelistURLs.push_back(idname);
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
bool cWebem::Include( std::string& reply )
{
	bool res = false;
	size_t p = 0;
	while( 1 ) {
		// find next request for generated text
		p = reply.find("<!--#embed",p);
		if( p == std::string::npos ) {
			break;
		}
		size_t q = reply.find("-->",p);
		if( q == std::string::npos )
			break;
		q += 3;

		size_t reply_len = reply.length();

		// code identifying text generator
		std::string code = reply.substr( p+11, q-p-15 );

		// find the function associated with this code
		std::map < std::string, webem_include_function >::iterator pf = myIncludes.find( code );
		if( pf != myIncludes.end() ) {
			// insert generated text
			std::string content_part;
			try
			{
				pf->second(content_part);
			}
			catch (...)
			{
				
			}
			reply.insert( p, content_part );
			res = true;
		} else {
			// no function found, look for a wide character fuction
			std::map < std::string, webem_include_function_w >::iterator pf = myIncludes_w.find( code );
			if( pf != myIncludes_w.end() ) {
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
				cUTF utf( content_part_w.c_str() );
				// insert generated text
				reply.insert( p, utf.get8() );
				res = true;
			}
		}

		// adjust pointer into text for insertion
		p = q + reply.length() - reply_len;
	}
	return res;
}

std::istream & safeGetline( std::istream & is, std::string & line ) {
	std::string myline;
	if ( getline( is, myline ) ) {
		if ( myline.size() && myline[myline.size()-1] == '\r' ) {
			line = myline.substr( 0, myline.size() - 1 );
		}
		else {
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
bool cWebem::CheckForAction(WebEmSession & session, request& req )
{
	// look for cWebem form action request
	if (!IsAction(req))
		return true;

	req.parameters.clear();

	std::string uri = ExtractRequestPath(req.uri);
	
	// find function matching action code
	size_t q = uri.find(".webem");
	std::string code = uri.substr(1,q-1);
	std::map < std::string, webem_action_function >::iterator
		pfun = myActions.find(  code );
	if( pfun == myActions.end() )
		return true;

	// decode the values

	if( req.method == "POST" )
	{
		const char *pContent_Type=request::get_req_header(&req,"Content-Type");
		if (pContent_Type)
		{
			if (strstr(pContent_Type,"multipart/form-data")!=NULL)
			{
				std::string szContent = req.content;
				size_t pos;
				std::string szVariable,szContentType,szValue;

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
					pos = szVariable.find("\"");
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
						(szContentType.find("application/octet-stream") != std::string::npos) ||
						(szContentType.find("application/json") != std::string::npos) ||
						(szContentType.find("Content-Type: text/xml") != std::string::npos) ||
						(szContentType.find("Content-Type: text/x-hex") != std::string::npos)
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

					szContent=szContent.substr(pos + szBoundary.size());
					pos = szContent.find("\r\n");
					if (pos == std::string::npos)
						return true;
					szContent = szContent.substr(pos + 2);
				}
				//we should have at least one value
				if (req.parameters.empty())
					return true;
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
	} else {
		q += 7;
	}

	std::string name;
	std::string value;

	size_t p = q;
	int flag_done = 0;
	while( ! flag_done ) {
		q = uri.find("=",p);
		if (q == std::string::npos)
			return true;
		name = uri.substr(p,q-p);
		p = q + 1;
		q = uri.find("&",p);
		if( q != std::string::npos )
			value = uri.substr(p,q-p);
		else {
			value = uri.substr(p);
			flag_done = 1;
		}
		// the browser sends blanks as +
		while( 1 ) {
			size_t p = value.find("+");
			if( p == std::string::npos )
				break;
			value.replace( p, 1, " " );
		}

		req.parameters.insert( std::pair< std::string,std::string > ( name, value ) );
		p = q+1;
	}

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

bool cWebem::IsPageOverride(const request& req, reply& rep)
{
	std::string request_path;
	if (!request_handler::url_decode(req.uri, request_path))
	{
		rep = reply::stock_reply(reply::bad_request);
		return false;
	}
	
	request_path = ExtractRequestPath(request_path);

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
	size_t paramPos=request_path2.find_first_of('?');
	if (paramPos!=std::string::npos)
	{
		std::string params=request_path2.substr(paramPos+1);
		std::string name;
		std::string value;

		size_t q=0;
		size_t p = q;
		int flag_done = 0;
		std::string uri=params;
		while( ! flag_done ) {
			q = uri.find("=",p);
			if (q == std::string::npos)
			{
				break;
			}
			name = uri.substr(p,q-p);
			p = q + 1;
			q = uri.find("&",p);
			if( q != std::string::npos)
				value = uri.substr(p,q-p);
			else {
				value = uri.substr(p);
				flag_done = 1;
			}
			// the browser sends blanks as +
			while( 1 ) {
				size_t p = value.find("+");
				if( p == std::string::npos)
					break;
				value.replace( p, 1, " " );
			}

			// now, url-decode only the value
			std::string decoded;
			request_handler::url_decode(value, decoded);
			req.parameters.insert( std::pair< std::string,std::string > ( name, decoded ) );
			p = q+1;
		}
	}
	if (req.method == "POST") {
		const char *pContent_Type = request::get_req_header(&req, "Content-Type");
		if (pContent_Type)
		{
			if (strstr(pContent_Type, "multipart") != NULL)
			{
				const char *pBoundary = strstr(pContent_Type, "boundary=");
				if (pBoundary != NULL)
				{
					std::string szBoundary = std::string("--") + (pBoundary + 9);
					//Find boundary in content
					std::istringstream ss(req.content);
					std::string csubstr;
					int ii = 0;
					std::string vName = "";
					while (!ss.eof())
					{
						safeGetline(ss, csubstr);
						if (ii == 0)
						{
							//Boundary
							if (csubstr != szBoundary)
							{
								rep = reply::stock_reply(reply::bad_request);
								return false;
							}
							ii++;
						}
						else if (ii == 1)
						{
							if (csubstr.find("Content-Disposition:") != std::string::npos)
							{
								size_t npos = csubstr.find("name=\"");
								if (npos == std::string::npos)
								{
									rep = reply::stock_reply(reply::bad_request);
									return false;
								}
								vName = csubstr.substr(npos + 6);
								npos = vName.find("\"");
								if (npos == std::string::npos)
								{
									rep = reply::stock_reply(reply::bad_request);
									return false;
								}
								vName = vName.substr(0, npos);
								ii++;
							}
						}
						else if (ii == 2)
						{
							if (csubstr.size() == 0)
							{
								ii++;
								//2 empty lines, rest is data
								std::string szContent;
								size_t bpos = size_t(ss.tellg());
								szContent = req.content.substr(bpos, ss.rdbuf()->str().size() - bpos - szBoundary.size() - 6);
								req.parameters.insert(std::pair< std::string, std::string >(vName, szContent));
								break;
							}
						}
					}
				}
			}
		}
	}

	// Determine the file extension.
	std::string extension;
	if (req.uri.find("json.htm") != std::string::npos) {
		extension = "json";
	} else {
		std::size_t last_slash_pos = request_path.find_last_of("/");
		std::size_t last_dot_pos = request_path.find_last_of(".");
		if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
		{
			extension = request_path.substr(last_dot_pos + 1);
		}
	}
	std::string strMimeType = mime_types::extension_to_type(extension);

	std::map < std::string, webem_page_function >::iterator
		pfun = myPages.find(  request_path );

	if (pfun!=myPages.end())
	{
		rep.status = reply::ok;
		try
		{
			pfun->second(session, req, rep);
		}
		catch (std::exception& e) {
			_log.Log(LOG_ERROR, "WebServer PO exception occurred : '%s'", e.what());
		}
		catch (...) {
			_log.Log(LOG_ERROR, "WebServer PO unknown exception occurred");
		}
		std::string attachment;
		size_t num = rep.headers.size();
		for (size_t h = 0; h < num; h++) {
			if (boost::iequals(rep.headers[h].name, "Content-Disposition")) {
				attachment = rep.headers[h].value.substr(rep.headers[h].value.find("=") + 1);
				std::size_t last_dot_pos = attachment.find_last_of(".");
				if (last_dot_pos != std::string::npos)
				{
					extension = attachment.substr(last_dot_pos + 1);
					strMimeType=mime_types::extension_to_type(extension);
				}
				break;
			}
		}

		if (!boost::algorithm::starts_with(strMimeType, "image"))
		{
			reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
			reply::add_header(&rep, "Content-Type", strMimeType + ";charset=UTF-8");
			reply::add_header(&rep, "Cache-Control", "no-cache");
			reply::add_header(&rep, "Pragma", "no-cache");
			reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
		}
		else
		{
			reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
			reply::add_header(&rep, "Content-Type", strMimeType);
			reply::add_header(&rep, "Cache-Control", "max-age=3600, public");
		}
		return true;
	}

	//check wchar_t
	std::map < std::string, webem_page_function >::iterator
		pfunW = myPages_w.find(  request_path );
	if (pfunW==myPages_w.end())
		return false;

	try
	{
		pfunW->second(session, req, rep);
	}
	catch (...)
	{
		
	}

	rep.status = reply::ok;
	reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
	reply::add_header(&rep, "Content-Type", strMimeType + "; charset=UTF-8");
	reply::add_header(&rep, "Cache-Control", "no-cache");
	reply::add_header(&rep, "Pragma", "no-cache");
	reply::add_header(&rep, "Access-Control-Allow-Origin", "*");

	return true;
}

void cWebem::SetWebTheme(const std::string &themename)
{
	m_actTheme = "/styles/"+themename;
}

void cWebem::SetWebRoot(const std::string &webRoot)
{
	// remove trailing slash if required
	if(m_webRoot.size() > 0 && m_webRoot[m_webRoot.size() - 1] != '/')
	{
		m_webRoot = webRoot.substr(0, webRoot.size() - 1);
	}
	else
	{
		m_webRoot = webRoot;
	}
	// put slash at the front if required
	if(m_webRoot.size() > 0 && m_webRoot[0] != '/')
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

	if (request_path.find(m_webRoot + "/@login")==0)
	{
		request_path=m_webRoot + "/";
	}

	// If path ends in slash (i.e. is a directory) then add "index.html".
	if (request_path[request_path.size() - 1] == '/')
	{
		request_path += "index.html";
	}
	
	if(m_webRoot.size() > 0)
	{
		// remove web root if present otherwise
		// create invalid request
		if (request_path.find(m_webRoot) == 0)
		{
			request_path = request_path.substr(m_webRoot.size());
		}
		else
		{
			request_path= "";
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
	if(m_webRoot.size() > 0)
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
	wtmp.ID=ID;
	wtmp.Username=username;
	wtmp.Password=password;
	wtmp.userrights=userrights;
	wtmp.ActiveTabs = activetabs;
	m_userpasswords.push_back(wtmp);
}

void cWebem::ClearUserPasswords()
{
	m_userpasswords.clear();

	boost::mutex::scoped_lock lock(m_sessionsMutex);
	m_sessions.clear(); //TODO : check if it is really necessary
}

void cWebem::AddLocalNetworks(std::string network)
{
	_tIPNetwork ipnetwork;

	if (network=="")
	{
		//add local host
		char ac[256];
		if (gethostname(ac, sizeof(ac)) != SOCKET_ERROR) {
			ipnetwork.hostname=ac;
			std::transform(ipnetwork.hostname.begin(), ipnetwork.hostname.end(), ipnetwork.hostname.begin(), ::tolower);
			m_localnetworks.push_back(ipnetwork);
		}
		return;
	}
	std::string inetwork=network;
	std::string inetworkmask=network;
	std::string mask=network;


	size_t pos=network.find_first_of("*");
	if (pos>0)
	{
		stdreplace(inetwork,"*","0");
		int a, b, c, d;
		if (sscanf(inetwork.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
			return;
		std::stringstream newnetwork;
		newnetwork << std::dec << a << "." << std::dec << b << "." << std::dec << c << "." << std::dec << d;
		inetwork=newnetwork.str();

		stdreplace(inetworkmask,"*","999");
		int e, f, g, h;
		if (sscanf(inetworkmask.c_str(), "%d.%d.%d.%d", &e, &f, &g, &h) != 4)
			return;

		std::stringstream newmask;
		if (e!=999) newmask << "255"; else newmask << "0";
		newmask << ".";
		if (f!=999) newmask << "255"; else newmask << "0";
		newmask << ".";
		if (g!=999) newmask << "255"; else newmask << "0";
		newmask << ".";
		if (h!=999) newmask << "255"; else newmask << "0";
		mask=newmask.str();

		ipnetwork.network=IPToUInt(inetwork);
		ipnetwork.mask=IPToUInt(mask);
	}
	else
	{
		pos=network.find_first_of("/");
		if (pos>0)
		{
			unsigned char keepbits = (unsigned char)atoi(network.substr(pos+1).c_str());
			uint32_t imask = keepbits > 0 ? 0x00 - (1<<(32 - keepbits)) : 0xFFFFFFFF;
			inetwork=network.substr(0,pos);
			ipnetwork.network=IPToUInt(inetwork);
			ipnetwork.mask=imask;
		}
		else
		{
			//Is it an IP address of hostname?
			boost::system::error_code ec;
			boost::asio::ip::address::from_string( network, ec );
			if ( ec )
			{
				//only allow ip's, localhost is covered above
				return;
				//ipnetwork.hostname=network;
			}
			else
			{
				//single IP?
				ipnetwork.network=IPToUInt(inetwork);
				ipnetwork.mask=IPToUInt("255.255.255.255");
			}
		}
	}

	m_localnetworks.push_back(ipnetwork);
}

void cWebem::ClearLocalNetworks()
{
	m_localnetworks.clear();
}

void cWebem::SetDigistRealm(std::string realm)
{
	m_DigistRealm=realm;
}

void cWebem::SetZipPassword(std::string password)
{
	m_zippassword = password;
}

void cWebem::SetSessionStore(session_store_impl_ptr sessionStore) {
	mySessionStore = sessionStore;
}

session_store_impl_ptr cWebem::GetSessionStore() {
	return mySessionStore;
}

// Check the user's password, return 1 if OK
static int check_password(struct ah *ah, const std::string &ha1, const std::string &realm)
{
	if (
		(ah->nonce.size() == 0) &&
		(ah->response.size() != 0)
		)
	{
		return (ha1 == GenerateMD5Hash(ah->response));
	}
	return 0;
}

const std::string cWebem::GetPort() {
	return m_settings.listening_port;
}

WebEmSession * cWebem::GetSession(const std::string & ssid) {
	boost::mutex::scoped_lock lock(m_sessionsMutex);
	std::map<std::string, WebEmSession>::iterator itt = m_sessions.find(ssid);
	if (itt != m_sessions.end()) {
		return &itt->second;
	}
	return NULL;
}
void cWebem::AddSession(const WebEmSession & session) {
	boost::mutex::scoped_lock lock(m_sessionsMutex);
	m_sessions[session.id] = session;
}

void cWebem::RemoveSession(const WebEmSession & session) {
	RemoveSession(session.id);
}

void cWebem::RemoveSession(const std::string & ssid) {
	boost::mutex::scoped_lock lock(m_sessionsMutex);
	std::map<std::string, WebEmSession>::iterator itt = m_sessions.find(ssid);
	if (itt != m_sessions.end()) {
		m_sessions.erase(itt);
	}
}

int cWebem::CountSessions() {
	boost::mutex::scoped_lock lock(m_sessionsMutex);
	return (int) m_sessions.size();
}

void cWebem::CleanSessions() {
#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] cleaning sessions...", GetPort().c_str());
#endif
	int before = CountSessions();
	// Clean up timed out sessions from memory
	std::vector<std::string> ssids;
	{
		boost::mutex::scoped_lock lock(m_sessionsMutex);
		time_t now = mytime(NULL);
		std::map<std::string, WebEmSession>::iterator itt;
		for (itt = m_sessions.begin(); itt != m_sessions.end(); ++itt) {
			if (itt->second.timeout < now) {
				ssids.push_back(itt->second.id);
			}
		}
	}
	std::vector<std::string>::iterator ssitt;
	for (ssitt = ssids.begin(); ssitt != ssids.end(); ++ssitt) {
		std::string ssid = *ssitt;
		RemoveSession(ssid);
	}
	int after = CountSessions();
	std::stringstream ss;
	{
		boost::mutex::scoped_lock lock(m_sessionsMutex);
		std::map<std::string, WebEmSession>::iterator itt;
		int i = 0;
		for (itt = m_sessions.begin(); itt != m_sessions.end(); ++itt) {
			if (i > 0) {
				ss << ",";
			}
			ss << itt->second.id;
			i += 1;
		}
	}
	// Clean up expired sessions from database in order to avoid to wait for the domoticz restart (long time running instance)
	if (mySessionStore != NULL) {
		this->mySessionStore->CleanSessions();
	}
	// Schedule next cleanup
	m_session_clean_timer.expires_at(m_session_clean_timer.expires_at() + boost::posix_time::minutes(15));
	m_session_clean_timer.async_wait(boost::bind(&cWebem::CleanSessions, this));
}

// Return 1 on success. Always initializes the ah structure.
int cWebemRequestHandler::parse_auth_header(const request& req, struct ah *ah)
{
	const char *auth_header;

	if ((auth_header = request::get_req_header(&req, "Authorization")) == NULL) {
		return 0;
	}

	// X509 Auth header
	if (boost::icontains(auth_header, "/CN="))
	{
		// DN looks like: /C=Country/ST=State/L=City/O=Org/OU=OrganizationUnit/CN=username/emailAddress=user@mail.com
		std::string dn = auth_header;
		size_t spos, epos;

		spos = dn.find("/CN=");
		epos = dn.find("/", spos + 1);
		if (spos != std::string::npos) {
				if (epos == std::string::npos) {
						epos = dn.size();
				}
				ah->user = dn.substr(spos + 4, epos - spos - 4);
		}

		spos = dn.find("/emailAddress=");
		epos = dn.find("/", spos + 1);
		if (spos != std::string::npos) {
				if (epos == std::string::npos) {
						epos = dn.size();
				}
				ah->response = dn.substr(spos + 14, epos - spos - 14);
		}

		if (ah->user.empty()) { // TODO: Should ah->response be not empty ?
			return 0;
		}
		return 1;
	}
	// Basic Auth header
	else if (boost::icontains(auth_header, "Basic "))
	{
		std::string decoded = base64_decode(auth_header + 6);
		size_t npos = decoded.find(':');
		if (npos == std::string::npos) {
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

	std::string uname="";
	std::string upass="";

	if (!parse_auth_header(req, &_ah))
	{
		size_t uPos=req.uri.find("username=");
		size_t pPos=req.uri.find("password=");
		if (
			(uPos==std::string::npos)||
			(pPos==std::string::npos)
			)
		{
			return 0;
		}
		size_t ulen=strlen("username=");
		std::string tmpuname=req.uri.substr(uPos+ulen,pPos-uPos-ulen-1);
		std::string tmpupass=req.uri.substr(pPos+strlen("username="));
		if (request_handler::url_decode(tmpuname,uname))
		{
			if (request_handler::url_decode(tmpupass,upass))
			{
				uname=base64_decode(uname);
				upass = GenerateMD5Hash(base64_decode(upass));

				std::vector<_tWebUserPassword>::iterator itt;
				for (itt=myWebem->m_userpasswords.begin(); itt!=myWebem->m_userpasswords.end(); ++itt)
				{
					if (itt->Username == uname)
					{
						if (itt->Password!=upass)
						{
							m_failcounter++;
							return 0;
						}
						session.isnew = true;
						session.username = itt->Username;
						session.rights = itt->userrights;
						session.rememberme = false;
						m_failcounter=0;
						return 1;
					}
				}
			}
		}
		m_failcounter++;
		return 0;
	}

	std::vector<_tWebUserPassword>::iterator itt;
	for (itt=myWebem->m_userpasswords.begin(); itt!=myWebem->m_userpasswords.end(); ++itt)
	{
		if (itt->Username == _ah.user)
		{
			int bOK = check_password(&_ah, itt->Password, myWebem->m_DigistRealm);
			if (!bOK)
			{
				m_failcounter++;
				return 0;
			}
			session.isnew = true;
			session.username = itt->Username;
			session.rights = itt->userrights;
			session.rememberme = false;
			m_failcounter=0;
			return 1;
		}
	}
	m_failcounter++;
	return 0;
}

bool IsIPInRange(const std::string &ip, const _tIPNetwork &ipnetwork) 
{
	if (ipnetwork.hostname.size()!=0)
	{
		return (ip==ipnetwork.hostname);
	}
	uint32_t ip_addr = IPToUInt(ip);
	if (ip_addr==0)
		return false;

	uint32_t net_lower = (ipnetwork.network & ipnetwork.mask);
	uint32_t net_upper = (net_lower | (~ipnetwork.mask));

	if (ip_addr >= net_lower &&
		ip_addr <= net_upper)
		return true;
	return false;
}

//Returns true is the connected host is in the local network
bool cWebemRequestHandler::AreWeInLocalNetwork(const std::string &sHost, const request& req)
{
	//check if in local network(s)
	if (myWebem->m_localnetworks.size()==0)
		return false;
	if (sHost.size()<3)
		return false;

	std::vector<_tIPNetwork>::const_iterator itt;

	/* RK, this doesn't work with IPv6 addresses.
	pos=host.find_first_of(":");
	if (pos!=std::string::npos)
		host=host.substr(0,pos);
	*/

	for (itt=myWebem->m_localnetworks.begin(); itt!=myWebem->m_localnetworks.end(); ++itt)
	{
		if (IsIPInRange(sHost,*itt))
		{
			return true;
		}
	}
	return false;
}

const char * months[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char * wkdays[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *make_web_time(const time_t rawtime)
{
	static char buffer[256];
	struct tm *gmt=gmtime(&rawtime);
	sprintf(buffer, "%s, %02d %s %04d %02d:%02d:%02d GMT",
		wkdays[gmt->tm_wday],
		gmt->tm_mday,
		months[gmt->tm_mon],
		gmt->tm_year + 1900,
		gmt->tm_hour,
		gmt->tm_min,
		gmt->tm_sec);
	return buffer;
}

void cWebemRequestHandler::send_remove_cookie(reply& rep)
{
	std::stringstream sstr;
	sstr << "SID=none";
	sstr << "; HttpOnly; path=/; Expires=" << make_web_time(0);
	reply::add_header(&rep, "Set-Cookie", sstr.str(), false);
}

std::string cWebemRequestHandler::generateSessionID()
{
	// Session id should not be predictable
	boost::uuids::random_generator gen;
	std::stringstream ss;
	std::string randomValue;
	
	boost::uuids::uuid u = gen();
	ss << u;
	randomValue = ss.str();

	std::string sessionId = GenerateMD5Hash(base64_encode((const unsigned char*)randomValue.c_str(), randomValue.size()));

#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] generate new session id token %s", myWebem->GetPort().c_str(), sessionId.c_str());
#endif

	return sessionId;
}

std::string cWebemRequestHandler::generateAuthToken(const WebEmSession & session, const request & req)
{
	// Authentication token should not be predictable
	boost::uuids::random_generator gen;
	std::stringstream ss;
	std::string randomValue;

	boost::uuids::uuid u = gen();
	ss << u;
	randomValue = ss.str();

	std::string authToken = base64_encode((const unsigned char*)randomValue.c_str(), randomValue.size());

#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] generate new authentication token %s", myWebem->GetPort().c_str(), authToken.c_str());
#endif

	session_store_impl_ptr sstore = myWebem->GetSessionStore();
	if (sstore != NULL) {
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
	sstr << "SID=" << session.id << "_" << session.auth_token << "." << session.expires;
	sstr << "; HttpOnly; path=/; Expires=" << make_web_time(session.expires);
	reply::add_header(&rep, "Set-Cookie", sstr.str(), false);
}

void cWebemRequestHandler::send_authorization_request(reply& rep)
{
	rep = reply::stock_reply(reply::unauthorized);
	rep.status = reply::unauthorized;
	send_remove_cookie(rep);
	if (myWebem->m_authmethod == AUTH_BASIC) {
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
	std::string request_path;
	if (!url_decode(req.uri, request_path))
		return false;
	if (
		(request_path.find(".png")!=std::string::npos)||
		(request_path.find(".jpg")!=std::string::npos)
		)
	{
		//don't compress 'compressed' images
		return false;
	}

	const char *encoding_header;
	//check gzip support if yes, send it back in gzip format
	if ((encoding_header = request::get_req_header(&req, "Accept-Encoding")) != NULL)
	{
		//see if we support gzip
		bool bHaveGZipSupport=(strstr(encoding_header,"gzip")!=NULL);
		if (bHaveGZipSupport)
		{
			CA2GZIP gzip((char*)rep.content.c_str(), rep.content.size());
			if ((gzip.Length>0)&&(gzip.Length<(int)rep.content.size()))
			{
				rep.bIsGZIP = true; // flag for later
				rep.content.clear();
				rep.content.append((char*)gzip.pgzip, gzip.Length);
				//Set new content length
				reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
				//Add gzip header
				reply::add_header(&rep, "Content-Encoding", "gzip");
				return true;
			}
		}
	}
	return false;
}

static void GetURICommandParameter(const std::string &uri, std::string &cmdparam)
{
	cmdparam = uri;
	size_t ppos1 = uri.find("&param=");
	size_t ppos2 = uri.find("?param=");
	if (
		(ppos1 == std::string::npos) &&
		(ppos2 == std::string::npos)
		)
		return;
	size_t ppos = ppos1;
	if (ppos == std::string::npos)
		ppos = ppos2;
	else
	{
		if ((ppos2 < ppos) && (ppos != std::string::npos))
			ppos = ppos2;
	}
	cmdparam = uri.substr(ppos + 7);
	ppos = cmdparam.find("&");
	if (ppos != std::string::npos)
	{
		cmdparam = cmdparam.substr(0, ppos);
	}
}

bool cWebemRequestHandler::CheckAuthentication(WebEmSession & session, const request& req, reply& rep)
{
	session.rights = -1; // no rights

	if (myWebem->m_userpasswords.size() == 0)
	{
		session.rights = 2;
		return true;//no username/password we are admin
	}

	if (AreWeInLocalNetwork(session.remote_host, req))
	{
		session.rights = 2;
		return true;//we are in the local network, no authentication needed, we are admin
	}

	//Check cookie if still valid
	const char* cookie_header = request::get_req_header(&req, "Cookie");
	if (cookie_header != NULL)
	{
		std::string sSID;
		std::string sAuthToken;
		std::string szTime;
		bool expired = false;

		// Parse session id and its expiration date
		std::string scookie = cookie_header;
		size_t fpos = scookie.find("SID=");
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
		size_t upos = scookie.find("_", fpos);
		size_t ppos = scookie.find(".", upos);
		time_t now = mytime(NULL);
		if ((fpos != std::string::npos) && (upos != std::string::npos) && (ppos != std::string::npos))
		{
			sSID = scookie.substr(fpos + 4, upos-fpos-4);
			sAuthToken = scookie.substr(upos + 1, ppos-upos-1);
			szTime = scookie.substr(ppos + 1);

			time_t stime;
			std::stringstream sstr;
			sstr << szTime;
			sstr >> stime;

			expired = stime < now;
		}

		if (!(sSID.empty() || sAuthToken.empty() || szTime.empty())) {
			WebEmSession* oldSession = myWebem->GetSession(sSID);
			if ((oldSession != NULL) && (oldSession->expires < now)) {
				// Check if session stored in memory is not expired (prevent from spoofing expiration time)
				expired = true;
			}
			if (expired)
			{
				//expired session, remove session
				m_failcounter = 0;
				if (oldSession != NULL)
				{
					// session exists (delete it from memory and database)
					myWebem->RemoveSession(sSID);
					removeAuthToken(sSID);
				}
				send_authorization_request(rep);
				return false;
			}
			if (oldSession != NULL) {
				// session already exists
				session = *oldSession;
			} else {
				// Session does not exists
				session.id = sSID;
			}
			session.auth_token = sAuthToken;
			// Check authen_token and restore session
			if (checkAuthToken(session)) {
				// user is authenticated
				return true;
			}

			send_authorization_request(rep);
			return false;

		} else {
			// invalid cookie
			if (myWebem->m_authmethod != AUTH_BASIC) {
				//Check if we need to bypass authentication (not when using basic-auth)
				std::string cmdparam;
				GetURICommandParameter(req.uri, cmdparam);
				std::vector < std::string >::const_iterator itt;
				for (itt = myWebem->myWhitelistURLs.begin(); itt != myWebem->myWhitelistURLs.end(); ++itt)
				{
					if (*itt == cmdparam)
						return true;
				}
				// Force login form
				send_authorization_request(rep);
				return false;
			}
		}
	}

	//patch to let always support basic authentication function for script calls
	if (req.uri.find("json.htm") != std::string::npos)
	{
		//Check first if we have a basic auth
		if (authorize(session, req, rep))
		{
			return true;
		}
	}

	if (myWebem->m_authmethod == AUTH_BASIC)
	{
		if (!authorize(session, req, rep))
		{
			if (m_failcounter > 0) {
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
	std::string cmdparam;
	GetURICommandParameter(req.uri, cmdparam);
	std::vector < std::string >::const_iterator itt;
	for (itt = myWebem->myWhitelistURLs.begin(); itt != myWebem->myWhitelistURLs.end(); ++itt)
	{
		if (*itt == cmdparam)
			return true;
	}

	send_authorization_request(rep);
	return false;
}

/**
 * Check authentication token if exists and restore the user session if necessary
 */
bool cWebemRequestHandler::checkAuthToken(WebEmSession & session) {
	session_store_impl_ptr sstore = myWebem->GetSessionStore();
	if (sstore == NULL) {
		_log.Log(LOG_ERROR, "CheckAuthToken([%s_%s]) : no store defined", session.id.c_str(), session.auth_token.c_str());
		return true;
	}

	if (session.id.empty() || session.auth_token.empty()) {
		_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : session id or auth token is empty", session.id.c_str(), session.auth_token.c_str());
		return false;
	}
	WebEmStoredSession storedSession = sstore->GetSession(session.id);
	if (storedSession.id.empty()) {
		_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : session id not found", session.id.c_str(), session.auth_token.c_str());
		return false;
	}
	if (storedSession.auth_token != GenerateMD5Hash(session.auth_token)) {
		_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : auth token mismatch", session.id.c_str(), session.auth_token.c_str());
		removeAuthToken(session.id);
		return false;
	}

#ifdef DEBUG_WWW
	_log.Log(LOG_STATUS, "[web:%s] CheckAuthToken(%s_%s_%s) : user authenticated", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
#endif

	if (session.username.empty()) {
		// Restore session if user exists and session does not already exist
		bool userExists = false;
		bool sessionExpires = false;
		session.username = storedSession.username;
		session.expires = storedSession.expires;
		std::vector<_tWebUserPassword>::iterator ittu;
		for (ittu=myWebem->m_userpasswords.begin(); ittu!=myWebem->m_userpasswords.end(); ++ittu) {
			if (ittu->Username == session.username) { // the user still exists
				userExists = true;
				session.rights = ittu->userrights;
				break;
			}
		}

		time_t now = mytime(NULL);
		sessionExpires = session.expires < now;

		if (!userExists || sessionExpires) {
#ifdef DEBUG_WWW
			_log.Log(LOG_ERROR, "[web:%s] CheckAuthToken(%s_%s) : cannot restore session, user not found or session expired", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str());
#endif
			removeAuthToken(session.id);
			return false;
		}

		WebEmSession* oldSession = myWebem->GetSession(session.id);
		if (oldSession == NULL) {
#ifdef DEBUG_WWW
			_log.Log(LOG_STATUS, "[web:%s] CheckAuthToken(%s_%s_%s) : restore session", myWebem->GetPort().c_str(), session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
#endif
			myWebem->AddSession(session);
		}
	}

	return true;
}

void cWebemRequestHandler::removeAuthToken(const std::string & sessionId) {
	session_store_impl_ptr sstore = myWebem->GetSessionStore();
	if (sstore != NULL) {
		sstore->RemoveSession(sessionId);
	}
}

char *cWebemRequestHandler::strftime_t(const char *format, const time_t rawtime)
{
	static char buffer[1024];
	struct tm ltime;
	localtime_r(&rawtime,&ltime);
	strftime(buffer, sizeof(buffer), format, &ltime);
	return buffer;
}

void cWebemRequestHandler::handle_request(const request& req, reply& rep)
{
	if (_log.isTraceEnabled())	  
		_log.Log(LOG_TRACE, "WEBH : Host:%s Uri;%s", req.host_address.c_str(), req.uri.c_str());

	// Initialize session
	WebEmSession session;
	session.remote_host = req.host_address;

	if (session.remote_host == "127.0.0.1")
	{
		//We could be using a proxy server
		//Check if we have the "X-Forwarded-For" (connection via proxy)
		const char *host_header = request::get_req_header(&req, "X-Forwarded-For");
		if (host_header != NULL)
		{
			session.remote_host = host_header;
			if (strstr(host_header, ",") != NULL)
			{
				//Multiple proxies are used... this is not very common
				host_header = request::get_req_header(&req, "X-Real-IP"); //try our NGINX header
				if (!host_header)
				{
					_log.Log(LOG_ERROR, "Webserver: Multiple proxies are used (Or possible spoofing attempt), ignoring client request (remote address: %s)", session.remote_host.c_str());
					rep = reply::stock_reply(reply::forbidden);
					return;
				}
				session.remote_host = host_header;
			}
		}
	}

	session.reply_status = reply::ok;
	session.isnew = false;
	session.forcelogin = false;
	session.rememberme = false;

	rep.bIsGZIP = false;

	bool isPage = myWebem->IsPageOverride(req, rep);
	bool isAction = myWebem->IsAction(req);

	// Respond to CORS Preflight request (for JSON API)
	if (req.method == "OPTIONS")
	{
		rep.status = reply::ok;
		reply::add_header(&rep, "Content-Length", "0");
		reply::add_header(&rep, "Content-Type", "text/plain");
		reply::add_header(&rep, "Access-Control-Max-Age", "3600");
		reply::add_header(&rep, "Access-Control-Allow-Origin", "*");
		reply::add_header(&rep, "Access-Control-Allow-Methods", "GET, POST");
		reply::add_header(&rep, "Access-Control-Allow-Headers", "Authorization, Content-Type");
		return;
	}

	// Check authentication on each page or action, if it exists.
	if ((isPage || isAction) && !CheckAuthentication(session, req, rep)) {
		return;
	}

	// Copy the request to be able to fill its parameters attribute
	request requestCopy = req;

	// Run action if exists
	if (isAction) {
		// Post actions only allowed when authenticated and user has admin rights
		if (session.rights != 2) {
			rep = reply::stock_reply(reply::forbidden);
			return;
		}
		myWebem->CheckForAction(session, requestCopy);
		if (!requestCopy.uri.empty())
		{
			if ((requestCopy.method == "POST") && (requestCopy.uri[0] != '/'))
			{
				//Send back as data instead of a redirect uri
				rep.status = reply::ok;
				rep.content = requestCopy.uri;
				reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
				reply::add_header(&rep, "Last-Modified", make_web_time(mytime(NULL)), true);
				reply::add_header(&rep, "Content-Type", "application/json;charset=UTF-8");
				return;
			}
		}
	}

	modify_info mInfo;
	if (!myWebem->CheckForPageOverride(session, requestCopy, rep))
	{
		if (session.reply_status != reply::ok)
		{
			rep = reply::stock_reply(static_cast<reply::status_type>(session.reply_status));
			return;
		}
		// do normal handling
		try
		{
			if (requestCopy.uri.find("/images/") == 0)
			{
				std::string theme_images_path = myWebem->m_actTheme + requestCopy.uri;
				if (file_exist((doc_root_ + theme_images_path).c_str()))
					requestCopy.uri = theme_images_path;
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
		for (unsigned int h = 0; h < rep.headers.size(); h++) {
			if (boost::iequals(rep.headers[h].name, "Content-Type")) {
				content_type = rep.headers[h].value;
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
			if (!rep.bIsGZIP) {
				// Find and include any special cWebem strings
				if (!myWebem->Include(rep.content)) {
					if (mInfo.mtime_support && !mInfo.is_modified) {
#ifdef DEBUG_WWW
						_log.Log(LOG_STATUS, "[web:%s] %s not modified (1).", myWebem->GetPort().c_str(), req.uri.c_str());
#endif
						rep = reply::stock_reply(reply::not_modified);
						return;
					}
				}

				// adjust content length header
				// ( Firefox ignores this, but apparently some browsers truncate display without it.
				// fix provided by http://www.codeproject.com/Members/jaeheung72 )

				reply::add_header(&rep, "Content-Length", boost::lexical_cast<std::string>(rep.content.size()));
				reply::add_header(&rep, "Last-Modified", make_web_time(mytime(NULL)), true);

				//check gzip support if yes, send it back in gzip format
				CompressWebOutput(req, rep);
			}

			// tell browser that we are using UTF-8 encoding
			reply::add_header(&rep, "Content-Type", content_type + ";charset=UTF-8");
		}
		else if (content_type.find("image/")!=std::string::npos)
		{
			if (mInfo.mtime_support && !mInfo.is_modified) {
				rep = reply::stock_reply(reply::not_modified);
#ifdef DEBUG_WWW
				_log.Log(LOG_STATUS, "[web:%s] %s not modified (2).", myWebem->GetPort().c_str(), req.uri.c_str());
#endif
				return;
			}
			//Cache images
			reply::add_header(&rep, "Date", strftime_t("%a, %d %b %Y %H:%M:%S GMT", mytime(NULL)));
			reply::add_header(&rep, "Expires", "Sat, 26 Dec 2099 11:40:31 GMT");
		}
		else {
			if (mInfo.mtime_support && !mInfo.is_modified) {
				rep = reply::stock_reply(reply::not_modified);
#ifdef DEBUG_WWW
				_log.Log(LOG_STATUS, "[web:%s] %s not modified (3).", myWebem->GetPort().c_str(), req.uri.c_str());
#endif
				return;
			}
		}
	}
	else
	{
		if (session.reply_status != reply::ok)
		{
			rep = reply::stock_reply(static_cast<reply::status_type>(session.reply_status));
			return;
		}

		if (!rep.bIsGZIP) {
			CompressWebOutput(req, rep);
		}
	}

	// Set timeout to make session in use
	session.timeout = mytime(NULL) + SHORT_SESSION_TIMEOUT;

	if (session.isnew == true) {
		// Create a new session ID
		session.id = generateSessionID();
		session.expires = session.timeout;
		if (session.rememberme) {
			// Extend session by 30 days
			session.expires += LONG_SESSION_TIMEOUT;
		}
		session.auth_token = generateAuthToken(session, req); // do it after expires to save it also
		session.isnew = false;
		myWebem->AddSession(session);
		send_cookie(rep, session);

	} else if (session.forcelogin == true) {
#ifdef DEBUG_WWW
		_log.Log(LOG_STATUS, "[web:%s] Logout : remove session %s", myWebem->GetPort().c_str(), session.id.c_str());
#endif
		myWebem->RemoveSession(session.id);
		removeAuthToken(session.id);
		if (myWebem->m_authmethod == AUTH_BASIC) {
			send_authorization_request(rep);
		}

	} else if (session.id.size() > 0) {
		// Renew session expiration and authentication token
		WebEmSession* memSession = myWebem->GetSession(session.id);
		if (memSession != NULL)
		{
			time_t now = mytime(NULL);
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

} //namespace server {
} //namespace http {

