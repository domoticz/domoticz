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
#include "Base64.h"
#include "GZipHelper.h"
#include <stdarg.h>
#include <fstream>
#include <sstream>
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

//10 minutes
#define SESSION_TIMEOUT 600

int m_failcounter=0;

namespace http {
	namespace server {

/**

Webem constructor

@param[in] address  IP address.  In general, use  "0.0.0.0"
@param[in] port     port to listen on for browser requests e.g. "8080"
@param[in] doc_root path to folder containing html e.g. "./"

*/
cWebem::cWebem(
	   const std::string& address,
	   const std::string& port,
	   const std::string& doc_root,
	   const std::string& secure_cert_file,
	   const std::string& secure_cert_passphrase) :
myRequestHandler( doc_root,this ), myPort( port ),
myServer( address, port, myRequestHandler, secure_cert_file, secure_cert_passphrase ),
m_DigistRealm("Domoticz.com"),
m_zippassword(""),
m_actTheme("")
{
	m_authmethod=AUTH_LOGIN;
	mySessionStore = NULL;
	myNextSessionCleanup = mytime(NULL) + 15 * 60; // in 15 minutes
}

/**

Start the server.

This does not return.

If application needs to continue, start new thread with call to this method.

*/

void cWebem::Run() { myServer.run(); }

void cWebem::Stop() { myServer.stop(); }


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
void cWebem::RegisterPageCodeW( const char* pageurl, webem_page_function_w fun )
{
	myPages_w.insert( std::pair<std::string, webem_page_function_w >( std::string(pageurl), fun  ) );
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

		Conversion between UTF-8 and UTF-16 strings.

		UTF-8 is used by web pages.  It is a variable byte length encoding
		of UNICODE characters which is independant of the byte order in a computer word.

		UTF-16 is the native Windows UNICODE encoding.

		The class stores two copies of the string, one in each encoding,
		so should only exist briefly while conversion is done.

		This is a wrapper for the WideCharToMultiByte and MultiByteToWideChar
		*/
		class cUTF
		{
			char * myString8;			///< string in UTF-6
		public:
			/// Construct from UTF-16
			cUTF( const wchar_t * ws );
			///  Construct from UTF8
			cUTF( const char * s );
			// copy constructor
			cUTF();
			/// get UTF8 version
			const char * get8() { return myString8; }
			/// free buffers
			~cUTF() { if (myString8) free(myString8); }
		};

		cUTF::cUTF()
		{
			myString8 = NULL;
		}

		/// Construct from UTF-16
		cUTF::cUTF( const wchar_t * ws )
		{
			std::string dest;
			std::wstring src=ws;
			for (size_t i = 0; i < src.size(); i++)
			{
				wchar_t w = src[i];
				if (w <= 0x7f)
					dest.push_back((char)w);
				else if (w <= 0x7ff){
					dest.push_back(0xc0 | ((w >> 6)& 0x1f));
					dest.push_back(0x80| (w & 0x3f));
				}
				else if (w <= 0xffff){
					dest.push_back(0xe0 | ((w >> 12)& 0x0f));
					dest.push_back(0x80| ((w >> 6) & 0x3f));
					dest.push_back(0x80| (w & 0x3f));
				}
				else
					dest.push_back('?');
			}
			int len=dest.size();
			myString8 = (char * ) malloc( len + 1 );
			if (myString8)
			{
				memcpy(myString8, dest.c_str(), len);
				*(myString8 + len) = '\0';
			}
		}

/**

  Do not call from application code, used by server to include generated text.

  @param[in/out] reply  text to include generated

  The text is searched for "<!--#cWebemX-->".
  The X can be any string not containing "-->"

  If X has been registered with cWebem then the associated function
  is called to generate text to be inserted.


*/
void cWebem::Include( std::string& reply )
{
	int p = 0;
	while( 1 ) {
		// find next request for generated text
		p = reply.find("<!--#embed",p);
		if( p == -1 ) {
			break;
		}
		int q = reply.find("-->",p);
		if( q == -1 )
			break;
		q += 3;

		int reply_len = reply.length();

		// code identifying text generator
		std::string code = reply.substr( p+11, q-p-15 );

		// find the function associated with this code
		std::map < std::string, webem_include_function >::iterator pf = myIncludes.find( code );
		if( pf != myIncludes.end() ) {
			// insert generated text
			std::string ret;
			try
			{
				ret = pf->second();
			}
			catch (...)
			{
				
			}
			reply.insert( p, ret );
		} else {
			// no function found, look for a wide character fuction
			std::map < std::string, webem_include_function_w >::iterator pf = myIncludes_w.find( code );
			if( pf != myIncludes_w.end() ) {
				// function found
				// get return string and convert from UTF-16 to UTF-8
				std::wstring wret;
				try
				{
					wret = pf->second();
				}
				catch (...)
				{
					
				}
				cUTF utf( wret.c_str() );
				// insert generated text
				reply.insert( p, utf.get8() );
			}
		}

		// adjust pointer into text for insertion
		p = q + reply.length() - reply_len;
	}
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
	int q = uri.find(".webem");
	if (q == -1)
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

	std::string uri = req.uri;

	// find function matching action code
	int q = uri.find(".webem");
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
						(szContentType.find("Content-Type: text/xml") != std::string::npos)
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
				std::string ret;
				try
				{
					ret = pfun->second(session, req);
				}
				catch (...)
				{
					
				}
				req.uri = ret;
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

	int p = q;
	int flag_done = 0;
	while( ! flag_done ) {
		q = uri.find("=",p);
		if (q==-1)
			return true;
		name = uri.substr(p,q-p);
		p = q + 1;
		q = uri.find("&",p);
		if( q != -1 )
			value = uri.substr(p,q-p);
		else {
			value = uri.substr(p);
			flag_done = 1;
		}
		// the browser sends blanks as +
		while( 1 ) {
			int p = value.find("+");
			if( p == -1 )
				break;
			value.replace( p, 1, " " );
		}

		req.parameters.insert( std::pair< std::string,std::string > ( name, value ) );
		p = q+1;
	}

	// call the function
	std::string ret;
	try
	{
		ret = pfun->second(session, req);
	}
	catch (...)
	{
		
	}
	req.uri = ret;

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
	int paramPos = request_path.find_first_of('?');
	if (paramPos != std::string::npos)
	{
		request_path = request_path.substr(0, paramPos);
	}
	std::map < std::string, webem_page_function >::iterator
		pfun = myPages.find(request_path);

	if (pfun != myPages.end())
		return true;
	//check wchar_t
	std::map < std::string, webem_page_function_w >::iterator
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

	session.lastRequestPath = request_path;

	int paramPos = request_path.find_first_of('?');
	if (paramPos != std::string::npos)
	{
		request_path = request_path.substr(0,paramPos);
	}

	// Request path must be absolute and not contain "..".
	if (request_path.empty() || request_path[0] != '/'
		|| request_path.find("..") != std::string::npos)
	{
		rep = reply::stock_reply(reply::bad_request);
		return false;
	}

	// If path ends in slash (i.e. is a directory) then add "index.html".
	if (request_path[request_path.size() - 1] == '/')
	{
		request_path += "index.html";
	}
	req.parameters.clear();

	request_path = req.uri; // we need the raw request string to parse the get-request
	paramPos=request_path.find_first_of('?');
	if (paramPos!=std::string::npos)
	{
		std::string params=request_path.substr(paramPos+1);
		std::string name;
		std::string value;

		int q=0;
		int p = q;
		int flag_done = 0;
		std::string uri=params;
		while( ! flag_done ) {
			q = uri.find("=",p);
			if (q==-1)
			{
				break;
			}
			name = uri.substr(p,q-p);
			p = q + 1;
			q = uri.find("&",p);
			if( q != -1 )
				value = uri.substr(p,q-p);
			else {
				value = uri.substr(p);
				flag_done = 1;
			}
			// the browser sends blanks as +
			while( 1 ) {
				int p = value.find("+");
				if( p == -1 )
					break;
				value.replace( p, 1, " " );
			}

			// now, url-decode only the value
			std::string decoded;
			request_handler::url_decode(value, decoded);
			req.parameters.insert( std::pair< std::string,std::string > ( name, decoded ) );
			p = q+1;
		}

		request_path=request_path.substr(0,paramPos);
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
	if (req.uri.find("json.htm") != std::string::npos)
		extension = "json";
	else
	{
		std::size_t last_slash_pos = request_path.find_last_of("/");
		std::size_t last_dot_pos = request_path.find_last_of(".");
		if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
		{
			extension = request_path.substr(last_dot_pos + 1);
		}
	}

	std::map < std::string, webem_page_function >::iterator
		pfun = myPages.find(  request_path );

	if (pfun!=myPages.end())
	{
		session.outputfilename="";
		rep.status = reply::ok;
		std::string retstr;
		try
		{
			retstr = pfun->second(session, req);
		}
		catch (...)
		{
			
		}

		rep.content.append(retstr.c_str(), retstr.size());

		std::string strMimeType=mime_types::extension_to_type(extension);
		int extraheaders=0;
		if (session.outputfilename!="")
		{
			std::size_t last_dot_pos = session.outputfilename.find_last_of(".");
			if (last_dot_pos != std::string::npos)
			{
				extension = session.outputfilename.substr(last_dot_pos + 1);
				strMimeType=mime_types::extension_to_type(extension);
			}
			extraheaders=1;
		}
		if (req.keep_alive) {
			extraheaders += 2;
		}

		int iHeader = 0;
		if (!boost::algorithm::starts_with(strMimeType, "image"))
		{
			rep.headers.resize(5 + extraheaders);
			rep.headers[iHeader].name = "Content-Length";
			rep.headers[iHeader++].value = boost::lexical_cast<std::string>(rep.content.size());
			rep.headers[iHeader].name = "Content-Type";
			rep.headers[iHeader].value = strMimeType;
			rep.headers[iHeader++].value += ";charset=UTF-8"; //ISO-8859-1
			rep.headers[iHeader].name = "Cache-Control";
			rep.headers[iHeader++].value = "no-cache";
			rep.headers[iHeader].name = "Pragma";
			rep.headers[iHeader++].value = "no-cache";
			rep.headers[iHeader].name = "Access-Control-Allow-Origin";
			rep.headers[iHeader++].value = "*";
			if (req.keep_alive) {
				rep.headers[iHeader].name = "Connection";
				rep.headers[iHeader++].value = "Keep-Alive";
				rep.headers[iHeader].name = "Keep-Alive";
				rep.headers[iHeader++].value = "max=20, timeout=10";
			}
			if (session.outputfilename != "")
			{
				rep.headers[iHeader].name = "Content-Disposition";
				rep.headers[iHeader++].value = "attachment; filename=" + session.outputfilename;
			}
		}
		else
		{
			rep.headers.resize(3 + extraheaders);
			rep.headers[iHeader].name = "Content-Length";
			rep.headers[iHeader++].value = boost::lexical_cast<std::string>(rep.content.size());
			rep.headers[iHeader].name = "Content-Type";
			rep.headers[iHeader++].value = strMimeType;
			rep.headers[iHeader].name = "Cache-Control";
			rep.headers[iHeader++].value = "max-age=3600, public";
			if (req.keep_alive) {
				rep.headers[iHeader].name = "Connection";
				rep.headers[iHeader++].value = "Keep-Alive";
				rep.headers[iHeader].name = "Keep-Alive";
				rep.headers[iHeader++].value = "max=20, timeout=10";
			}
			if (session.outputfilename != "")
			{
				rep.headers[iHeader].name = "Content-Disposition";
				rep.headers[iHeader++].value = "attachment; filename=" + session.outputfilename;
			}
		}
		return true;
	}
	//check wchar_t
	std::map < std::string, webem_page_function_w >::iterator
		pfunW = myPages_w.find(  request_path );
	if (pfunW==myPages_w.end())
		return false;

	std::wstring wret;
	try
	{
		wret = pfunW->second(session, req);
	}
	catch (...)
	{
		
	}
	cUTF utf( wret.c_str() );

	int extraheaders = 0;
	if (req.keep_alive) {
		extraheaders += 2;
	}

	rep.status = reply::ok;
	rep.content.append(utf.get8(), strlen(utf.get8()));
	rep.headers.resize(5 + extraheaders);

	int iHeader = 0;
	rep.headers[iHeader].name = "Content-Length";
	rep.headers[iHeader++].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[iHeader].name = "Content-Type";
	rep.headers[iHeader].value = mime_types::extension_to_type(extension);
	rep.headers[iHeader++].value += ";charset=UTF-8";
	rep.headers[iHeader].name = "Cache-Control";
	rep.headers[iHeader++].value = "no-cache";
	rep.headers[iHeader].name = "Pragma";
	rep.headers[iHeader++].value = "no-cache";
	rep.headers[iHeader].name = "Access-Control-Allow-Origin";
	rep.headers[iHeader++].value = "*";
	if (req.keep_alive) {
		rep.headers[iHeader].name = "Connection";
		rep.headers[iHeader++].value = "Keep-Alive";
		rep.headers[iHeader].name = "Keep-Alive";
		rep.headers[iHeader++].value = "max=20, timeout=10";
	}
	return true;
}

void cWebem::SetWebTheme(const std::string &themename)
{
	m_actTheme = "/styles/"+themename;
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


	int pos=network.find_first_of("*");
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

void cWebem::SetSessionStore(session_store* sessionStore) {
	mySessionStore = sessionStore;
	if (mySessionStore != NULL) {
		mySessionStore->CleanSessions();
	}
}

session_store* cWebem::GetSessionStore() {
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

void cWebem::CleanTimedOutSessions() {

	// TODO : Check if a mutex and an atomic<unsigned long> are needed in case multiple requests are received at the same time.

	time_t now = mytime(NULL);
	if (myNextSessionCleanup >= now) {
		myNextSessionCleanup = now + 15 * 60; // in 15 minutes

		std::map<std::string, WebEmSession>::iterator itt;
		for (itt=m_sessions.begin(); itt!=m_sessions.end(); ++itt) {
			if (itt->second.timeout < now) {
				m_sessions.erase(itt);
			}
		}

	}
}

// Return 1 on success. Always initializes the ah structure.
int cWebemRequestHandler::parse_auth_header(const request& req, struct ah *ah)
{
	const char *auth_header;

	if ((auth_header = request::get_req_header(&req, "Authorization")) == NULL)
	{
		return 0;
	}

	//Only accept Basic Auth header
	if (!boost::icontains(auth_header, "Basic "))
		return 0;

	std::string decoded = base64_decode(auth_header + 6);
	int npos = decoded.find(':');
	if (npos == std::string::npos)
		return 0;

	ah->user = decoded.substr(0, npos);
	ah->response = decoded.substr(npos + 1);
	return 1;
}

// Authorize against the opened passwords file. Return 1 if authorized.
int cWebemRequestHandler::authorize(WebEmSession & session, const request& req, reply& rep)
{
	struct ah _ah;

	std::string uname="";
	std::string upass="";

	if (!parse_auth_header(req, &_ah))
	{
		int uPos=req.uri.find("username=");
		int pPos=req.uri.find("password=");
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

	std::string host=sHost;
	std::vector<_tIPNetwork>::const_iterator itt;

	if (host=="127.0.0.1")
	{
		//We could be using a proxy server
		//Check if we have the "X-Forwarded-For" (connection via proxy)
		const char *host_header=request::get_req_header(&req, "X-Forwarded-For");
		if (host_header!=NULL)
		{
			host=host_header;
			if (strstr(host_header,",")!=NULL)
			{
				//Multiple proxies are used... this is not very common
				host_header=request::get_req_header(&req, "X-Real-IP"); //try our NGINX header
				if (!host_header)
				{
					_log.Log(LOG_ERROR,"Webserver: Multiple proxies are used (Or possible spoofing attempt), ignoring client request (remote addresses: %s)",host.c_str());
					return false;
				}
				host=host_header;
			}
		}
	}

	/* RK, this doesn't work with IPv6 addresses.
	pos=host.find_first_of(":");
	if (pos!=std::string::npos)
		host=host.substr(0,pos);
	*/

	for (itt=myWebem->m_localnetworks.begin(); itt!=myWebem->m_localnetworks.end(); ++itt)
	{
		if (IsIPInRange(host,*itt))
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
	size_t ahsize = rep.headers.size();
	rep.headers.resize(ahsize + 1);
	rep.headers[ahsize].name = "Set-Cookie";
	std::stringstream sstr;
	sstr << "SID=none";
	sstr << "; path=/; Expires= " << make_web_time(0);
	rep.headers[ahsize].value = sstr.str();
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

#ifdef _DEBUG
	_log.Log(LOG_STATUS, "generate new session id token %s", sessionId.c_str());
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

#ifdef _DEBUG
	_log.Log(LOG_STATUS, "generate new authentication token %s", authToken.c_str());
#endif

	session_store* sstore = myWebem->GetSessionStore();
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
	int ahsize = rep.headers.size();
	rep.headers.resize(ahsize + 1);
	rep.headers[ahsize].name = "Set-Cookie";
	std::stringstream sstr;
	sstr << "SID=" << session.id << "_" << session.auth_token << "." << session.expires;
	sstr << "; path=/; Expires= " << make_web_time(session.expires);
	rep.headers[ahsize].value = sstr.str();
}

void cWebemRequestHandler::send_authorization_request(reply& rep)
{
	rep.status = reply::unauthorized;
	size_t ahsize = rep.headers.size();
	rep.headers.resize(ahsize+2);
	rep.headers[ahsize].name = "Content-Length";
	rep.headers[ahsize].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[ahsize+1].name = "WWW-Authenticate";

	char szAuthHeader[200];
	sprintf(szAuthHeader,
		"Basic "
		"realm=\"%s\"",
		myWebem->m_DigistRealm.c_str());
	rep.headers[ahsize+1].value = szAuthHeader;
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
				rep.content.clear();
				rep.content.append((char*)gzip.pgzip, gzip.Length);
				//Set new content length
				std::string szSize=boost::lexical_cast<std::string>(rep.content.size());

				rep.headers[0].value = szSize;
				//Add gzip header
				int ohsize=rep.headers.size();
				rep.headers.resize(ohsize+1);
				rep.headers[ohsize].name = "Content-Encoding";
				rep.headers[ohsize].value = "gzip";
			}
		}
	}
	return false;
}

bool cWebemRequestHandler::CheckAuthentication(WebEmSession & session, const request& req, reply& rep)
{
	session.rights = -1; // no rights
	if (session.forcelogin)
	{
		session.forcelogin = false;
		if (myWebem->m_authmethod == AUTH_BASIC)
		{
			send_authorization_request(rep);
			return false;
		}
	}

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
		int fpos = scookie.find("SID=");
		int upos = scookie.find("_");
		int ppos = scookie.find(".");
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
			std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(sSID);
			if (itt != myWebem->m_sessions.end() && (itt->second.expires < now)) {
				// Check if session stored in memory is not expired (prevent from spoofing expiration time)
				expired = true;
			}
			if (expired)
			{
				//expired session, remove session
				m_failcounter = 0;
				send_remove_cookie(rep);
				if (itt != myWebem->m_sessions.end())
				{
					// session exists (delete it from memory and database)
					myWebem->m_sessions.erase(itt);
					removeAuthToken(sSID);
					rep = reply::stock_reply(reply::unauthorized);
				}
				if (myWebem->m_authmethod == AUTH_BASIC)
				{
					send_authorization_request(rep);
				}
				return false;
			}
			if (itt != myWebem->m_sessions.end()) {
				// session already exists
				session = itt->second;
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
			rep = reply::stock_reply(reply::unauthorized);
			send_remove_cookie(rep);
			return false;

		} else {
			//invalid cookie
			session.removecookie = true;
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
				_log.Log(LOG_ERROR, "Webserver: Failed authentication attempt, ignoring client request (remote addresses: %s)", session.remote_host.c_str());
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
	std::vector < std::string >::const_iterator itt;
	for (itt = myWebem->myWhitelistURLs.begin(); itt != myWebem->myWhitelistURLs.end(); ++itt)
	{
		if (req.uri.find(*itt) != std::string::npos)
			return true;
	}

	rep = reply::stock_reply(reply::unauthorized);
	if (session.removecookie)
	{
		send_remove_cookie(rep);
	}
	return false;
}

/**
 * Check authentication token if exists and restore the user session if necessary
 */
bool cWebemRequestHandler::checkAuthToken(WebEmSession & session) {
	session_store* sstore = myWebem->GetSessionStore();
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

#ifdef _DEBUG
	//_log.Log(LOG_STATUS, "CheckAuthToken(%s_%s_%s) : user authenticated", session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
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
#ifdef _DEBUG
			_log.Log(LOG_ERROR, "CheckAuthToken(%s_%s) : cannot restore session, user not found or session expired", session.id.c_str(), session.auth_token.c_str());
#endif
			removeAuthToken(session.id);
			return false;
		}

		std::map<std::string, WebEmSession>::iterator itts = myWebem->m_sessions.find(session.id);
		if (itts == myWebem->m_sessions.end()) {
#ifdef _DEBUG
			_log.Log(LOG_STATUS, "CheckAuthToken(%s_%s_%s) : restore session", session.id.c_str(), session.auth_token.c_str(), session.username.c_str());
#endif
			myWebem->m_sessions[session.id] = session;
		}
	}

	return true;
}

void cWebemRequestHandler::removeAuthToken(const std::string & sessionId) {
	session_store* sstore = myWebem->GetSessionStore();
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
	//_log.Log(LOG_NORM, "www-request: %s", req.uri.c_str());

	// Initialize session
	WebEmSession session;
	session.remote_host = req.host;
	session.isnew = false;
	session.removecookie = false;
	session.forcelogin = false;
	session.rememberme = false;

	rep.bIsGZIP = false;

	bool isPage = myWebem->IsPageOverride(req, rep);
	bool isAction = myWebem->IsAction(req);

	// Check authentication on each page or action, if it exists.
	bool bCheckAuthentication = false;
	if (isPage || isAction) {
		bCheckAuthentication = true;
	}

	if (isPage && (req.uri.find("dologout") != std::string::npos))
	{
		//Remove session id based on cookie
		const char *cookie;
		cookie = request::get_req_header(&req, "Cookie");
		if (cookie != NULL)
		{
			std::string scookie = cookie;
			int fpos = scookie.find("SID=");
			int upos = scookie.find("_");
			if ((fpos != std::string::npos) && (upos != std::string::npos))
			{
				std::string sSID = scookie.substr(fpos + 4, upos-fpos-4);
				_log.Log(LOG_STATUS, "Logout : remove session %s", sSID.c_str());
				std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(sSID);
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
		session.removecookie = true;
		bCheckAuthentication = false; // do not authenticate the user, just logout
	}

	// Check user authentication on each page or action, if it exists.
	if (bCheckAuthentication && !CheckAuthentication(session, req, rep)) {
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
	}

	if (!myWebem->CheckForPageOverride(session, requestCopy, rep))
	{
		// do normal handling
		request_handler::handle_request(requestCopy, rep);

		if (rep.headers[1].value == "text/html" 
			|| rep.headers[1].value == "text/plain" 
			|| rep.headers[1].value == "text/css"
			|| rep.headers[1].value == "text/javascript"
			)
		{
			// Find and include any special cWebem strings
			myWebem->Include( rep.content );

			// adjust content length header
			// ( Firefox ignores this, but apparently some browsers truncate display without it.
			// fix provided by http://www.codeproject.com/Members/jaeheung72 )

			rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());

			// tell browser that we are using UTF-8 encoding
			rep.headers[1].value += ";charset=UTF-8";

			//check gzip support if yes, send it back in gzip format
			if (!rep.bIsGZIP)
				CompressWebOutput(req,rep);
		}
		else if (rep.headers[1].value.find("image/")!=std::string::npos)
		{
			//Cache images
			int theaders = rep.headers.size();
			rep.headers.resize(theaders + 2);

			rep.headers[theaders].name = "Date";
			rep.headers[theaders].value = strftime_t("%a, %d %b %Y %H:%M:%S GMT",mytime(NULL));

			rep.headers[theaders + 1].name = "Expires";
			rep.headers[theaders + 1].value = "Sat, 26 Dec 2099 11:40:31 GMT";
		}
	}
	else
	{
		if (!rep.bIsGZIP)
			CompressWebOutput(req,rep);
	}

	// Set timeout to make session in use
	session.timeout = mytime(NULL) + SESSION_TIMEOUT;
	// Clean up timed out sessions
	myWebem->CleanTimedOutSessions();

	if (session.isnew == true)
	{
		// Create a new session ID
		session.id = generateSessionID();
		session.expires = session.timeout;
		if (session.rememberme) {
			// Extend session by a year
			session.expires += (86400 * 365);
		}
		session.auth_token = generateAuthToken(session, req); // do it after expires to save it also
		session.isnew = false;
		myWebem->m_sessions[session.id] = session;
		send_cookie(rep, session);
	}
	else if (session.removecookie == true)
	{
		// Invalid cookie, unauthorized or logout
		removeAuthToken(session.id);
		send_remove_cookie(rep);
	}
	else if (session.id.size()>0)
	{
		// Renew session expiration and authentication token
		std::map<std::string, WebEmSession>::iterator itt = myWebem->m_sessions.find(session.id);
		if (itt != myWebem->m_sessions.end())
		{
			time_t now = mytime(NULL);
			if (myWebem->m_sessions[session.id].expires - 60 < now)
			{
				myWebem->m_sessions[session.id].expires = now + SESSION_TIMEOUT;
				myWebem->m_sessions[session.id].auth_token = generateAuthToken(myWebem->m_sessions[session.id], req); // do it after expires to save it also
				send_cookie(rep, myWebem->m_sessions[session.id]);
			}
		}
	}

}

} //namespace server {
} //namespace http {

