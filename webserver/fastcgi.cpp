#include "stdafx.h"
#include "fastcgi.hpp"
#include <fstream>
#include <sstream>
#include "../httpclient/UrlEncode.h"
#include "../main/Helper.h"

//(c) 2016 GizMoCuz

//To be done!
//ibfcgi-dev << source also contains win32 project
//The below is not FastCGI (yet)

namespace http {
	namespace server {

uint16_t fastcgi_parser::request_id_ = 1;
		
//http://www.mit.edu/~yandros/doc/specs/fcgi-spec.html
struct _tFCGI_Header {
	uint8_t version;
	uint8_t type;
	uint8_t requestIdB1;
	uint8_t requestIdB0;
	uint8_t contentLengthB1;
	uint8_t contentLengthB0;
	uint8_t paddingLength; //We recommend that records be placed on boundaries that are multiples of eight bytes. The fixed-length portion of a FCGI_Record is eight bytes.
	uint8_t reserved;
};

/*
* Number of bytes in a FCGI_Header.  Future versions of the protocol
* will not reduce this number.
*/
#define FCGI_HEADER_LEN  8

/*
* Value for version component of FCGI_Header
*/
#define FCGI_VERSION_1           1

#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

/*
* Value for requestId component of FCGI_Header
*/
#define FCGI_NULL_REQUEST_ID     0

struct _tFCGI_BeginRequestBody {
	uint8_t roleB1;
	uint8_t roleB0;
	uint8_t flags;
	uint8_t reserved[5];
};

struct _tFCGI_BeginRequestRecord {
	_tFCGI_Header header;
	_tFCGI_BeginRequestBody body;
} ;

/*
* Mask for flags component of FCGI_BeginRequestBody
*/
#define FCGI_KEEP_CONN  1

/*
* Values for role component of FCGI_BeginRequestBody
*/
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

struct _tFCGI_EndRequestBody {
	uint8_t appStatusB3;
	uint8_t appStatusB2;
	uint8_t appStatusB1;
	uint8_t appStatusB0;
	uint8_t protocolStatus;
	uint8_t reserved[3];
};

struct _tFCGI_EndRequestRecord {
	_tFCGI_Header header;
	_tFCGI_EndRequestBody body;
};

/*
* Values for protocolStatus component of FCGI_EndRequestBody
*/
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

/*
* Variable names for FCGI_GET_VALUES / FCGI_GET_VALUES_RESULT records
*/
#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

struct _tFCGI_UnknownTypeBody {
	uint8_t type;
	uint8_t reserved[7];
};

struct _tFCGI_UnknownTypeRecord {
	_tFCGI_Header header;
	_tFCGI_UnknownTypeBody body;
};

std::string ExecuteCommandAndReturnRaw(const std::string &szCommand)
{
	std::string ret;

	try
	{
		FILE *fp;

		/* Open the command for reading. */
#ifdef WIN32
		fp = _popen(szCommand.c_str(), "r");
#else
		fp = popen(szCommand.c_str(), "r");
#endif
		if (fp != NULL)
		{
			char path[1035];
			/* Read the output a line at a time - output it. */
			while (fgets(path, sizeof(path) - 1, fp) != NULL)
			{
				ret += path;
			}
			/* close */
#ifdef WIN32
			_pclose(fp);
#else
			pclose(fp);
#endif
		}
	}
	catch (...)
	{

	}
	return ret;
}

extern std::istream & safeGetline(std::istream & is, std::string & line);

bool fastcgi_parser::handlePHP(const server_settings &settings, const std::string &script_path, const request &req, reply &rep, modify_info &mInfo)
{
	std::string full_path = settings.www_root + script_path;
	std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
	if (!is)
	{
		rep = reply::stock_reply(reply::not_found);
		return false;
	}
	is.close();

	std::multimap<std::string, std::string> parameters;

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
		std::string uri = params;
		while (!flag_done) {
			q = uri.find("=", p);
			if (q == std::string::npos)
			{
				break;
			}
			name = uri.substr(p, q - p);
			p = q + 1;
			q = uri.find("&", p);
			if (q != std::string::npos)
				value = uri.substr(p, q - p);
			else {
				value = uri.substr(p);
				flag_done = 1;
			}
			// the browser sends blanks as +
			while (1) {
				size_t p = value.find("+");
				if (p == std::string::npos)
					break;
				value.replace(p, 1, " ");
			}
			parameters.insert(std::pair< std::string, std::string >(name, value));
			p = q + 1;
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
								parameters.insert(std::pair< std::string, std::string >(vName, szContent));
								break;
							}
						}
					}
				}
			}
		}
	}

	//
	
	_tFCGI_Header gfci;
	gfci.version = 1;
	gfci.type = 1;
	gfci.requestIdB1 = (request_id_ & 0xFF00 >> 8);
	gfci.requestIdB0 = request_id_ & 0xFF;
	gfci.paddingLength = 0;
	request_id_++;

	std::string szQueryString;

	std::string str_params;
	std::multimap<std::string, std::string>::const_iterator itt;
	for (itt = parameters.begin(); itt != parameters.end(); ++itt)
	{
		if (!str_params.empty())
			str_params += " ";

		str_params.append(itt->first);
		str_params.append("=");
		str_params.append(itt->second);

		if (!szQueryString.empty())
			szQueryString += "&";
		szQueryString.append(itt->first);
		szQueryString.append("=");
		szQueryString.append(CURLEncode::URLEncode(itt->second));
	}

	std::string fullexecmd = settings.php_cgi_path + " " + full_path;
	if (!str_params.empty())
		fullexecmd = fullexecmd + " " + str_params;

	std::map<std::string, std::string> fcgi_params;
	fcgi_params["SCRIPT_FILENAME"] = settings.www_root + script_path;
	fcgi_params["QUERY_STRING"] = szQueryString;
	fcgi_params["REQUEST_METHOD"] = "GET";
	fcgi_params["CONTENT_TYPE"] = "";
	fcgi_params["CONTENT_LENGTH"] = "";
	fcgi_params["SCRIPT_NAME"] = script_path;
	fcgi_params["REQUEST_URI"] = script_path;
	fcgi_params["DOCUMENT_URI"] = script_path;
	fcgi_params["DOCUMENT_ROOT"] = settings.www_root;
	fcgi_params["SERVER_PROTOCOL"] = "HTTP/1.1";
	fcgi_params["REQUEST_SCHEME"] = "http";
	fcgi_params["GATEWAY_INTERFACE"] = "CGI/1.1";
	fcgi_params["SERVER_SOFTWARE"] = "Domoticz";
	fcgi_params["REMOTE_ADDR"] = req.host_address;
	fcgi_params["REMOTE_PORT"] = req.host_port;
	fcgi_params["SERVER_ADDR"] = settings.listening_address;
	fcgi_params["SERVER_PORT"] = settings.listening_port;
	fcgi_params["SERVER_NAME"] = "localhost";
	fcgi_params["REDIRECT_STATUS"] = "200";


	fullexecmd += " SERVER_SOFTWARE=Domoticz";
	fullexecmd += " SERVER_NAME=localhost";
	fullexecmd += " SERVER_ADDR=" + CURLEncode::URLEncode(settings.listening_address);
	fullexecmd += " SERVER_PORT=" + settings.listening_port;
	fullexecmd += " REMOTE_ADDR=" + req.host_address;
	fullexecmd += " REMOTE_PORT=" + req.host_port;

	std::vector<header>::const_iterator ittHeader;
	for (ittHeader = req.headers.begin(); ittHeader != req.headers.end(); ++ittHeader)
	{
		std::string rName = "HTTP_" + ittHeader->name;
		stdreplace(rName, "-", "_");
		stdupper(rName);
		fullexecmd += " " + rName + "=" + CURLEncode::URLEncode(ittHeader->value);

		fcgi_params[rName] = CURLEncode::URLEncode(ittHeader->value);
	}

	std::string pret = ExecuteCommandAndReturnRaw(fullexecmd);
	if (pret.empty())
	{
		rep = reply::stock_reply(reply::not_found);
		return false;
	}
	rep.status = reply::ok;
	//Add the headers
	bool bDoneWithHeaders = false;
	while (!bDoneWithHeaders)
	{
		size_t tpos = pret.find('\n');
		if (tpos == std::string::npos)
		{
			rep = reply::stock_reply(reply::internal_server_error);
			return false;
		}

		if (tpos == 0)
		{
			bDoneWithHeaders = true;
			pret = pret.substr(tpos + 1);
		}
		else
		{
			std::string theader = pret.substr(0, tpos);
			pret = pret.substr(tpos + 1);

			//Check if we have a status return
			if (theader.find("Status") == 0)
			{
				tpos = theader.find(':');
				if (tpos == std::string::npos)
					continue;
				theader = theader.substr(tpos + 1);
				if (theader[0] == ' ')
					theader = theader.substr(1);
				tpos = theader.find(' ');
				if (tpos == std::string::npos)
					continue;
				std::string errcode = theader.substr(0, tpos);
				rep = reply::stock_reply((reply::status_type)atoi(errcode.c_str()));
				return true;
			}

			tpos = theader.find(':');
			if (tpos == std::string::npos)
				continue;
			std::string hfirst = theader.substr(0, tpos);
			std::string hlast = theader.substr(tpos + 1);
			if (hlast.empty())
				continue;
			if (hlast[0] == ' ')
				hlast = hlast.substr(1);
			reply::add_header(&rep, hfirst, hlast);
		}
	}
	if (!pret.empty())
	{
		rep.content.append(pret);
		reply::add_header(&rep, "Content-Length", std::to_string(rep.content.size()));
	}
	mInfo.delay_status = true;
	return true;
}

}
}

