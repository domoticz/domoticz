//mainpage WEBEM
//
//Detailed class and method documentation of the WEBEM C++ embedded web server source code.
//
//Modified, extended etc by Robbert E. Peters/RTSS B.V.
#include "stdafx.h"
#include "cWebem.h"
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "reply.hpp"
#include "request.hpp"
#include "mime_types.hpp"
#include "Base64.h"
#include <stdarg.h>
#include <fstream>
#include <sstream>
#include "../main/Helper.h"
#include "../main/localtime_r.h"

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

int m_failcounter=0;

namespace http {
	namespace server {

	typedef std::multimap  < std::string, std::string> webem_mmp_name_value;
	typedef std::multimap  < std::string, std::string>::iterator webem_iter_name_value;

/**

Webem constructor

@param[in] address  IP address.  In general, use  "0.0.0.0"
@param[in] port     port to listen on for browser requests e.g. "8080"
@param[in] doc_root path to folder containing html e.g. "./"

*/
cWebem::cWebem(
	   const std::string& address,
	   const std::string& port,
	   const std::string& doc_root ) :
myRequestHandler( doc_root,this ), myPort( port ),
myServer( address, port, myRequestHandler )
{
	m_DigistRealm = "Domoticz.com";
	m_zippassword = "";
	m_actsessionid=0;
	m_authmethod=AUTH_LOGIN;
	m_bForceRelogin=false;
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
			/// get UTF8 version
			const char * get8() { return myString8; }
			/// free buffers
			~cUTF() { free(myString8); }
		};

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
			memcpy(myString8, dest.c_str(), len);
			*(myString8+len) = '\0';
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
			break;;
		q += 3;

		int reply_len = reply.length();

		// code identifying text generator
		std::string code = reply.substr( p+11, q-p-15 );

		// find the function associated with this code
		std::map < std::string, webem_include_function >::iterator pf = myIncludes.find( code );
		if( pf != myIncludes.end() ) {
			// insert generated text
			reply.insert( p, pf->second() );
		} else {
			// no function found, look for a wide character fuction
			std::map < std::string, webem_include_function_w >::iterator pf = myIncludes_w.find( code );
			if( pf != myIncludes_w.end() ) {
				// function found
				// get return string and convert from UTF-16 to UTF-8
				cUTF utf( pf->second() );
				// insert generated text
				reply.insert( p, utf.get8() );
			}
		}

		// adjust pointer into text for insertion
		p = q + reply.length() - reply_len;
	}
}
/**

Do not call from application code,
used by server  to handle form submissions.

*/
void cWebem::CheckForAction( request& req )
{
	// look for cWebem form action request
	std::string uri = req.uri;
	int q = 0;
	if( req.method != "POST" ) {
		q = uri.find(".webem");
		if( q == -1 )
			return;
	} else {
		q = uri.find(".webem");
		if( q == -1 )
			return;
	}

	// find function matching action code
	std::string code = uri.substr(1,q-1);
	std::map < std::string, webem_action_function >::iterator
		pfun = myActions.find(  code );
	if( pfun == myActions.end() )
		return;

	// decode the values

	if( req.method == "POST" ) {
		uri = req.content;
		q = 0;
	} else {
		q += 7;
	}


	myNameValues.clear();
	std::string name;
	std::string value;

	int p = q;
	int flag_done = 0;
	while( ! flag_done ) {
		q = uri.find("=",p);
		if (q==-1)
			return;
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

		myNameValues.insert( std::pair< std::string,std::string > ( name, value ) );
		p = q+1;
	}

	// call the function
	req.uri = pfun->second( this );

	return;
}

bool cWebem::CheckForPageOverride(const request& req, reply& rep)
{
	// Decode url to path.
	std::string request_path;
	if (!request_handler::url_decode(req.uri, request_path))
	{
		rep = reply::stock_reply(reply::bad_request);
		return false;
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

	myNameValues.clear();
	m_lastRequestPath=request_path;

	int paramPos=request_path.find_first_of('?');
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

			myNameValues.insert( std::pair< std::string,std::string > ( name, value ) );
			p = q+1;
		}

		request_path=request_path.substr(0,paramPos);
	}

	// Determine the file extension.
	std::size_t last_slash_pos = request_path.find_last_of("/");
	std::size_t last_dot_pos = request_path.find_last_of(".");
	std::string extension;
	if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
	{
		extension = request_path.substr(last_dot_pos + 1);
	}

	std::map < std::string, webem_page_function >::iterator
		pfun = myPages.find(  request_path );

	if (pfun!=myPages.end())
	{
		m_outputfilename="";
		rep.status = reply::ok;
		std::string retstr=pfun->second( );

		rep.content.append(retstr.c_str(), retstr.size());

		std::string strMimeType=mime_types::extension_to_type(extension);
		int extraheaders=0;
		if (m_outputfilename!="")
		{
			std::size_t last_dot_pos = m_outputfilename.find_last_of(".");
			if (last_dot_pos != std::string::npos)
			{
				extension = m_outputfilename.substr(last_dot_pos + 1);
				strMimeType=mime_types::extension_to_type(extension);
			}
			extraheaders=1;
		}

		rep.headers.resize(4+extraheaders);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = strMimeType;
		rep.headers[1].value += ";charset=UTF-8"; //ISO-8859-1
		rep.headers[2].name = "Cache-Control";
		rep.headers[2].value = "no-cache";
		rep.headers[3].name = "Pragma";
		rep.headers[3].value = "no-cache";

		if (m_outputfilename!="")
		{
			rep.headers[4].name = "Content-Disposition";
			rep.headers[4].value = "attachment; filename=" + m_outputfilename;
		}
		return true;
	}
	//check wchar_t
	std::map < std::string, webem_page_function_w >::iterator
		pfunW = myPages_w.find(  request_path );
	if (pfunW==myPages_w.end())
		return false;

	cUTF utf( pfunW->second( ) );

	rep.status = reply::ok;
	rep.content.append(utf.get8(), strlen(utf.get8()));
	rep.headers.resize(4);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = mime_types::extension_to_type(extension);
	rep.headers[1].value += ";charset=UTF-8";
	rep.headers[2].name = "Cache-Control";
	rep.headers[2].value = "no-cache";
	rep.headers[3].name = "Pragma";
	rep.headers[3].value = "no-cache";
	return true;
}

void cWebem::AddUserPassword(const unsigned long ID, const std::string username, const std::string password, const _eUserRights userrights)
{
	_tWebUserPassword wtmp;
	wtmp.ID=ID;
	wtmp.Username=username;
	wtmp.Password=password;
	wtmp.userrights=userrights;
	m_userpasswords.push_back(wtmp);
}

void cWebem::ClearUserPasswords()
{
	m_userpasswords.clear();
	m_sessionids.clear();
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
		inetwork=stdreplace(inetwork,"*","0");
		int a, b, c, d;
		if (sscanf(inetwork.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
			return;
		std::stringstream newnetwork;
		newnetwork << std::dec << a << "." << std::dec << b << "." << std::dec << c << "." << std::dec << d;
		inetwork=newnetwork.str();

		inetworkmask=stdreplace(inetworkmask,"*","999");
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

/**

  Find the value of a name set by a form submit action

*/
std::string& cWebem::FindValue( const char* name )
{
	static std::string ret;
	ret = "";
	webem_iter_name_value iter = myNameValues.find( name );
	if( iter != myNameValues.end() )
		ret = iter->second;

	return ret;
}

#ifndef HAVE_MD5
typedef struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
} MD5_CTX;

#if defined(__BYTE_ORDER) && (__BYTE_ORDER == 1234)
#define byteReverse(buf, len) // Do nothing
#else
static void byteReverse(unsigned char *buf, unsigned longs) {
	uint32_t t;
	do {
		t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
			((unsigned) buf[1] << 8 | buf[0]);
		*(uint32_t *) buf = t;
		buf += 4;
	} while (--longs);
}
#endif

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

// Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
// initialization constants.
static void MD5Init(MD5_CTX *ctx) {
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

static void MD5Transform(uint32_t buf[4], uint32_t const in[16]) {
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

static void MD5Update(MD5_CTX *ctx, unsigned char const *buf, unsigned len) {
	uint32_t t;

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
		ctx->bits[1]++;
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;

	if (t) {
		unsigned char *p = (unsigned char *) ctx->in + t;

		t = 64 - t;
		if (len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *) ctx->in);
		buf += t;
		len -= t;
	}

	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *) ctx->in);
		buf += 64;
		len -= 64;
	}

	memcpy(ctx->in, buf, len);
}

static void MD5Final(unsigned char digest[16], MD5_CTX *ctx) {
	unsigned count;
	unsigned char *p;

	count = (ctx->bits[0] >> 3) & 0x3F;

	p = ctx->in + count;
	*p++ = 0x80;
	count = 64 - 1 - count;
	if (count < 8) {
		memset(p, 0, count);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *) ctx->in);
		memset(ctx->in, 0, 56);
	} else {
		memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	((uint32_t *) ctx->in)[14] = ctx->bits[0];
	((uint32_t *) ctx->in)[15] = ctx->bits[1];

	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);
	memcpy(digest, ctx->buf, 16);
	memset((char *) ctx, 0, sizeof(*ctx));
}
#endif // !HAVE_MD5

// Stringify binary data. Output buffer must be twice as big as input,
// because each byte takes 2 bytes in string representation
static void bin2str(char *to, const unsigned char *p, size_t len) {
	static const char *hex = "0123456789abcdef";

	for (; len--; p++) {
		*to++ = hex[p[0] >> 4];
		*to++ = hex[p[0] & 0x0f];
	}
	*to = '\0';
}

// Return stringified MD5 hash for list of strings. Buffer must be 33 bytes.
void mg_md5(char buf[33], ...) {
	unsigned char hash[16];
	const char *p;
	va_list ap;
	MD5_CTX ctx;

	MD5Init(&ctx);

	va_start(ap, buf);
	while ((p = va_arg(ap, const char *)) != NULL) {
		MD5Update(&ctx, (const unsigned char *) p, (unsigned) strlen(p));
	}
	va_end(ap);

	MD5Final(hash, &ctx);
	bin2str(buf, hash, sizeof(hash));
}

static int lowercase(const char *s) {
	return tolower(* (const unsigned char *) s);
}

static int mg_strncasecmp(const char *s1, const char *s2, size_t len)
{
	int diff = 0;

	if (len > 0)
		do {
			diff = lowercase(s1++) - lowercase(s2++);
		} while (diff == 0 && s1[-1] != '\0' && --len > 0);

	return diff;
}

static int mg_strcasecmp(const char *s1, const char *s2)
{
	int diff;

	do {
		diff = lowercase(s1++) - lowercase(s2++);
	} while (diff == 0 && s1[-1] != '\0');

	return diff;
}

// Skip the characters until one of the delimiters characters found.
// 0-terminate resulting word. Skip the delimiter and following whitespaces if any.
// Advance pointer to buffer to the next word. Return found 0-terminated word.
// Delimiters can be quoted with quotechar.
static char *skip_quoted(char **buf, const char *delimiters,
	const char *whitespace, char quotechar)
{
	char *p, *begin_word, *end_word, *end_whitespace;

	begin_word = *buf;
	end_word = begin_word + strcspn(begin_word, delimiters);

	// Check for quotechar
	if (end_word > begin_word) 
	{
		p = end_word - 1;
		while (*p == quotechar) 
		{
			// If there is anything beyond end_word, copy it
			if (*end_word == '\0') {
				*p = '\0';
				break;
			} else 
			{
				size_t end_off = strcspn(end_word + 1, delimiters);
				memmove (p, end_word, end_off + 1);
				p += end_off; // p must correspond to end_word - 1
				end_word += end_off + 1;
			}
		}
		for (p++; p < end_word; p++) 
		{
			*p = '\0';
		}
	}

	if (*end_word == '\0') {
		*buf = end_word;
	} else 
	{
		end_whitespace = end_word + 1 + strspn(end_word + 1, whitespace);

		for (p = end_word; p < end_whitespace; p++) 
		{
			*p = '\0';
		}

		*buf = end_whitespace;
	}

	return begin_word;
}

// Check the user's password, return 1 if OK
static int check_password(
	const char *method, const char *ha1, const char *uri,
	const char *nonce, const char *nc, const char *cnonce,
	const char *qop, const char *response) 
{
	char ha2[32 + 1], expected_response[32 + 1];

	if (
		(nonce==NULL)&&
		(response != NULL)
		)
	{
		return (strcmp(ha1,response)==0);
	}

	// Some of the parameters may be NULL
	if (method == NULL || nonce == NULL || nc == NULL || cnonce == NULL ||
		qop == NULL || response == NULL) {
			return 0;
	}

	// NOTE(lsm): due to a bug in MSIE, we do not compare the URI
	// TODO(lsm): check for authentication timeout
	if (// strcmp(dig->uri, c->ouri) != 0 ||
		strlen(response) != 32
		// || now - strtoul(dig->nonce, NULL, 10) > 3600
		) {
			return 0;
	}

	mg_md5(ha2, method, ":", uri, NULL);
	mg_md5(expected_response, ha1, ":", nonce, ":", nc,
		":", cnonce, ":", qop, ":", ha2, NULL);

	return mg_strcasecmp(response, expected_response) == 0;
}

// Return 1 on success. Always initializes the ah structure.
int cWebemRequestHandler::parse_auth_header(const request& req, char *buf,	size_t buf_size, struct ah *ah) 
{
	char *name, *value, *s;
	const char *auth_header;

	(void) memset(ah, 0, sizeof(*ah));
	if ((auth_header = req.get_req_header(&req, "Authorization")) == NULL)
	{
		return 0;
	}

	if (mg_strncasecmp(auth_header, "Basic ", 6) == 0) {
		//Basic authentication
		strcpy(buf, auth_header + 6);
		s = buf;

		std::string decoded = base64_decode(s);
		int npos=decoded.find(':');
		if (npos==std::string::npos)
			return 0;
		strcpy(buf,decoded.c_str());
		buf[npos]=0;
		ah->user=buf;
		myWebem->m_actualuser=buf;
		ah->response=buf+npos+1;

		return 1;
	}

	if (mg_strncasecmp(auth_header, "Digest ", 7) != 0) {
			return 0;
	}

	// Make modifiable copy of the auth header
	strcpy(buf, auth_header + 7);
	s = buf;

	// Parse authorization header
	for (;;) 
	{
		// Gobble initial spaces
		while (isspace(* (unsigned char *) s)) 
		{
			s++;
		}
		name = skip_quoted(&s, "=", " ", 0);
		// Value is either quote-delimited, or ends at first comma or space.
		if (s[0] == '\"') {
			s++;
			value = skip_quoted(&s, "\"", " ", '\\');
			if (s[0] == ',') {
				s++;
			}
		} else {
			value = skip_quoted(&s, ", ", " ", 0);  // IE uses commas, FF uses spaces
		}
		if (*name == '\0') {
			break;
		}

		if (!strcmp(name, "username")) {
			ah->user = value;
			myWebem->m_actualuser=value;
		} else if (!strcmp(name, "cnonce")) {
			ah->cnonce = value;
		} else if (!strcmp(name, "response")) {
			ah->response = value;
		} else if (!strcmp(name, "uri")) {
			ah->uri = value;
		} else if (!strcmp(name, "qop")) {
			ah->qop = value;
		} else if (!strcmp(name, "nc")) {
			ah->nc = value;
		} else if (!strcmp(name, "nonce")) {
			ah->nonce = value;
		}
	}

	// CGI needs it as REMOTE_USER
	if (ah->user != NULL) 
	{
		//conn->request_info.remote_user = mg_strdup(ah->user);
	} 
	else 
	{
		return 0;
	}

	return 1;
}


// Authorize against the opened passwords file. Return 1 if authorized.
int cWebemRequestHandler::authorize(const request& req)
{
	char buf[8191];
	struct ah _ah;

	std::string uname="";
	std::string upass="";

	if (!parse_auth_header(req, buf, sizeof(buf), &_ah))
	{
		int uPos=req.uri.find("username=");
		int pPos=req.uri.find("password=");
		if (
			(uPos==std::string::npos)||
			(pPos==std::string::npos)
			)
		{
			if (myWebem->m_guestuser!="")
			{
				myWebem->m_actualuser=myWebem->m_guestuser;
				return 1;
			}
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
				upass=base64_decode(upass);

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
						myWebem->m_actualuser=uname;
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
		if (itt->Username == uname)
		{
			if (itt->Password!=upass)
			{
				m_failcounter++;
				return 0;
			}
			m_failcounter=0;
			return 1;
		}
		else if (itt->Username == _ah.user)
		{
			int bOK=check_password
				(
				req.method.c_str(),
				itt->Password.c_str(), _ah.uri, _ah.nonce, _ah.nc, _ah.cnonce, _ah.qop,_ah.response
				);
			if (!bOK)
			{
				m_failcounter++;
				return 0;
			}
			m_failcounter=0;
			return 1;
		}
	}
	m_failcounter++;
	return 0;
}

bool IsIPInRange(const std::string ip, const _tIPNetwork ipnetwork) 
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
bool cWebemRequestHandler::AreWeInLocalNetwork(const request& req)
{
	//check if in local network(s)

	if (myWebem->m_localnetworks.size()==0)
		return false;

	const char *host_header;
	if ((host_header = request::get_req_header(&req, "Host")) != NULL)
	{
		std::string host=host_header;
		int pos=host.find_first_of(":");
		if (pos!=std::string::npos)
			host=host.substr(0,pos);

		std::vector<_tIPNetwork>::const_iterator itt;
		for (itt=myWebem->m_localnetworks.begin(); itt!=myWebem->m_localnetworks.end(); ++itt)
		{
			if (IsIPInRange(host,*itt))
			{
				return true;
			}
		}
	}
	return false;
}

// Return 1 if request is authorized, 0 otherwise.
int cWebemRequestHandler::check_authorization(const request& req)
{
	myWebem->m_actualuser="";
	myWebem->m_actsessionid=0;
	if (myWebem->m_userpasswords.size()==0)
		return 1;//no username/password

	if (AreWeInLocalNetwork(req))
		return 1;//we are in the local network, no authentication needed

	const char* cookie_header = request::get_req_header(&req, "Cookie");
	if (cookie_header!=NULL)
	{
		std::string scookie=cookie_header;
		int fpos=scookie.find("SID=");
		if (fpos!=std::string::npos)
		{
			std::stringstream sstr;

			int dpos=scookie.find(";");
			std::string sidstr;
			if (dpos==std::string::npos)
				sstr << scookie.substr(fpos+4).c_str();
			else
				sstr << scookie.substr(fpos+4,dpos-fpos-4).c_str();

			time_t SID;
			sstr >> SID;
			std::map<time_t,WebEmSession>::iterator itt = myWebem->m_sessionids.find(SID);
			if (itt != myWebem->m_sessionids.end()) 
			{
				if (
					(req.uri.find("login.html")!=std::string::npos)||
					(req.uri.find("dologout")!=std::string::npos)
					)
				{
					myWebem->m_sessionids.erase(itt);
					myWebem->m_bForceRelogin=true;
				} 
				else
				{
					time_t acttime=mytime(NULL);
					if (acttime-myWebem->m_sessionids[SID].lasttouch<10*60)
					{
						myWebem->m_actsessionid=SID;
						myWebem->m_actualuser=myWebem->m_sessionids[SID].username;
						return 1;
					}
					else
					{
						//timeout, remove session
						myWebem->m_sessionids.erase(itt);
					}
				}
			}
		}
	}
	return authorize(req);
}

void cWebemRequestHandler::send_authorization_request(reply& rep)
{
	rep.status = reply::unauthorized;
	rep.headers.resize(2);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "WWW-Authenticate";

	char szAuthHeader[200];
	sprintf(szAuthHeader,
		"Basic "
		"realm=\"%s\"",
		myWebem->m_DigistRealm.c_str());
	rep.headers[1].value=szAuthHeader;
}

void cWebemRequestHandler::send_authorization_page(reply& rep)
{
	std::string loginpage=doc_root_ + "/login.html";
	std::ifstream is(loginpage.c_str(), std::ios::in | std::ios::binary);
	if (!is)
	{
		rep = reply::stock_reply(reply::not_found);
	}
	else
	{
		// Fill out the reply to be sent to the client.
		rep.status = reply::ok;
		char buf[512];
		while (is.read(buf, sizeof(buf)).gcount() > 0)
			rep.content.append(buf, (unsigned int)is.gcount());

	}
	rep.headers.resize(4);
	rep.headers[0].name = "Content-Length";
	rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
	rep.headers[1].name = "Content-Type";
	rep.headers[1].value = mime_types::extension_to_type("html");
	rep.headers[1].value += ";charset=UTF-8";

	rep.headers[2].name = "Cache-Control";
	rep.headers[2].value= "no-cache, must-revalidate";
	rep.headers[3].name = "Expires";
	rep.headers[3].value= "Sat, 26 Jul 1997 05:00:00 GMT";
}

void cWebemRequestHandler::check_cookie(const request& req, reply& rep)
{
	if (AreWeInLocalNetwork(req))
		return;
	if (myWebem->m_actualuser.size()==0)
		return;

	if (myWebem->m_userpasswords.size()>0)
	{
		if (rep.headers[1].value.find("text/html")!=std::string::npos)
		{
			//serve cookies only on html pages
			const char *cookie;
			cookie = request::get_req_header(&req, "Cookie");
			bool bHaveSID=false;
			std::string scookie="";
			int fpos=0;
			if (cookie!=NULL)
			{
				scookie=cookie;
				fpos=scookie.find("SID=");
				bHaveSID=fpos!=std::string::npos;
			}
			if (!bHaveSID)
			{
				//Add new session ID
				rep.headers.resize(4);
				rep.headers[3].name="Set-Cookie";
				std::stringstream sstr;
				time_t sessionid=mytime(NULL);
				_tWebEmSession usession;
				usession.username=myWebem->m_actualuser;
				usession.lasttouch=sessionid;
				sstr << "SID=" << std::dec << sessionid;
				myWebem->m_sessionids[sessionid]=usession;
				rep.headers[3].value=sstr.str();
			}
			else
			{
				//check if we need to re-use this SID
				int dpos=scookie.find(";");
				std::string sidstr;
				if (dpos==std::string::npos)
					sidstr=scookie.substr(fpos+4);
				else
					sidstr=scookie.substr(fpos+4,dpos-fpos-4);
				int fpos=scookie.find("SID=");
				if (fpos!=std::string::npos)
				{
					std::stringstream sstr;
					sstr << sidstr;
					time_t SID;
					sstr >> SID;
					std::map<time_t,WebEmSession>::iterator itt = myWebem->m_sessionids.find(SID);
					if (itt == myWebem->m_sessionids.end()) 
					{
						time_t atime=mytime(NULL);
						_tWebEmSession usession;
						usession.username=myWebem->m_actualuser;
						usession.lasttouch=atime;
						myWebem->m_sessionids[SID]=usession;
					}
				}

			}
		}
	}
}

void cWebemRequestHandler::handle_request( const request& req, reply& rep)
{
	if ((!check_authorization(req))||(myWebem->m_bForceRelogin))
	{
		myWebem->m_bForceRelogin=false;
		if (m_failcounter==4)
		{
			m_failcounter=0;
			rep = reply::stock_reply(reply::forbidden);
			return;
		}
		else
		{
			if (myWebem->m_authmethod==AUTH_LOGIN)
			{
				if ((req.uri.find(".htm")!=std::string::npos)||(req.uri.find("#")!=std::string::npos)||(req.uri.find(".")==std::string::npos))
				{
					send_authorization_page(rep);
					return;
				}
			}
			else
			{
				send_authorization_request(rep);
				return;
			}
		}
	}

	// check for webem action request
	request req_modified = req;

	myWebem->CheckForAction( req_modified );

	if (!myWebem->CheckForPageOverride(req, rep))
	{
		// do normal handling
		request_handler::handle_request( req_modified, rep);

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
		}
	}
	check_cookie(req,rep);

	if (myWebem->m_actsessionid!=0)
	{
		myWebem->m_sessionids[myWebem->m_actsessionid].lasttouch=mytime(NULL);
	}
	
}

} //namespace server {
} //namespace http {

