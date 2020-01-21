#include "stdafx.h"
#include "HTTPClient.h"
#include <curl/curl.h>
#include "../main/Logger.h"

#include <algorithm>
#include <iostream>
#include <fstream>

#ifndef O_LARGEFILE
	#define O_LARGEFILE 0
#endif

extern std::string szUserDataFolder;

bool		HTTPClient::m_bCurlGlobalInitialized = false;
bool		HTTPClient::m_bVerifyHost = false;
bool		HTTPClient::m_bVerifyPeer = false;
long		HTTPClient::m_iConnectionTimeout = 10;
long		HTTPClient::m_iTimeout = 90; //max, time that a download has to be finished?
std::string	HTTPClient::m_sUserAgent = "domoticz/1.0";


/************************************************************************
 *									*
 * Curl writeback functions						*
 *									*
 ************************************************************************/

size_t write_curl_headerdata(void *contents, size_t size, size_t nmemb, void *userp) // called once for each header
{
	size_t realsize = size * nmemb;
	std::vector<std::string>* pvHeaderData = (std::vector<std::string>*)userp;
	pvHeaderData->push_back(std::string((unsigned char*)contents, (std::find((unsigned char*)contents, (unsigned char*)contents + realsize, '\r'))));
	return realsize;
}

size_t write_curl_data(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::vector<unsigned char>* pvHTTPResponse = (std::vector<unsigned char>*)userp;
	pvHTTPResponse->insert(pvHTTPResponse->end(), (unsigned char*)contents, (unsigned char*)contents + realsize);
	return realsize;
}

size_t write_curl_data_file(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::ofstream *outfile = (std::ofstream*)userp;
	outfile->write((const char*)contents, realsize);
	return realsize;
}

size_t write_curl_data_single_line(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	std::vector<unsigned char>* pvHTTPResponse = (std::vector<unsigned char>*)userp;
	size_t ii=0;
	while (ii< realsize)
	{
		unsigned char *pData = (unsigned char*)contents + ii;
		if ((pData[0] == '\n') || (pData[0] == '\r'))
			return 0;
		pvHTTPResponse->push_back(pData[0]);
		ii++;
	}
	return realsize;
}


/************************************************************************
 *									*
 * Private functions							*
 *									*
 ************************************************************************/

bool HTTPClient::CheckIfGlobalInitDone()
{
	if (!m_bCurlGlobalInitialized)
	{
		CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
		if (res != CURLE_OK)
			return false;
		m_bCurlGlobalInitialized = true;
	}
	return true;
}

void HTTPClient::Cleanup()
{
	if (m_bCurlGlobalInitialized)
	{
		curl_global_cleanup();
	}
}

void HTTPClient::SetGlobalOptions(void *curlobj)
{
	CURL *curl=(CURL *)curlobj;
	curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC | CURLAUTH_DIGEST);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, m_sUserAgent.c_str());
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_iConnectionTimeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_iTimeout);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, m_bVerifyPeer ? 1L : 0);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, m_bVerifyHost ? 2L : 0); //allow self signed certificates
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	std::string domocookie = szUserDataFolder + "domocookie.txt";
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, domocookie.c_str());
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, domocookie.c_str());
}

struct _tHTTPErrors {
	const uint16_t http_code;
	const char* szMeaning;
};

const _tHTTPErrors HTTPCodes[] = {
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 306, "(Unused)" },
	{ 307, "Temporary Redirect" },
	{ 308, "Permanent Redirect(experimental)" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Timeout" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request - URI Too Long" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested Range Not Satisfiable" },
	{ 417, "Expectation Failed" },
	{ 418, "I'm a teapot (RFC 2324)" },
	{ 420, "Enhance Your Calm(Twitter)" },
	{ 422, "Unprocessable Entity(WebDAV)" },
	{ 423, "Locked(WebDAV)" },
	{ 424, "Failed Dependency(WebDAV)" },
	{ 425, "Reserved for WebDAV" },
	{ 426, "Upgrade Required" },
	{ 428, "Precondition Required" },
	{ 429, "Too Many Requests" },
	{ 431, "Request Header Fields Too Large" },
	{ 444, "No Response(Nginx)" },
	{ 449, "Retry With(Microsoft)" },
	{ 450, "Blocked by Windows Parental Controls(Microsoft)" },
	{ 451, "Unavailable For Legal Reasons" },
	{ 499, "Client Closed Request(Nginx)" },
	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Timeout" },
	{ 505, "HTTP Version Not Supported" },
	{ 506, "Variant Also Negotiates(Experimental)" },
	{ 507, "Insufficient Storage(WebDAV)" },
	{ 508, "Loop Detected(WebDAV)" },
	{ 509, "Bandwidth Limit Exceeded(Apache)" },
	{ 510, "Not Extended" },
	{ 511, "Network Authentication Required" },
	{ 598, "Network read timeout error" },
	{ 599, "Network connect timeout error" },
	{ 0, nullptr }
};

void HTTPClient::LogError(const long response_code)
{
	const _tHTTPErrors* pCodes = (const _tHTTPErrors*)&HTTPCodes;

	while (pCodes[0].http_code != 0)
	{
		if (pCodes[0].http_code == response_code)
		{
			_log.Debug(DEBUG_NORM, "HTTP %d: %s", pCodes[0].http_code, pCodes[0].szMeaning);
			return;
		}
		pCodes++;
	}
	_log.Debug(DEBUG_NORM, "HTTP return code is: %li", response_code);
}


/************************************************************************
 *									*
 * Configuration functions						*
 *									*
 ************************************************************************/

void HTTPClient::SetConnectionTimeout(const long timeout)
{
	m_iConnectionTimeout = timeout;
}

void HTTPClient::SetTimeout(const long timeout)
{
	m_iTimeout = timeout;
}

void HTTPClient::SetSecurityOptions(const bool verifypeer, const bool verifyhost)
{
	m_bVerifyPeer = verifypeer;
	m_bVerifyHost = verifyhost;
}

void HTTPClient::SetUserAgent(const std::string &useragent)
{
	m_sUserAgent = useragent;
}


/************************************************************************
 *									*
 * binary methods with access to return header data			*
 *									*
 ************************************************************************/

bool HTTPClient::GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, std::vector<std::string> &vHeaderData, const long TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);

		struct curl_slist *headers = NULL;
		if (ExtraHeaders.size() > 0)
		{
			std::vector<std::string>::const_iterator itt;
			for (itt = ExtraHeaders.begin(); itt != ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_curl_headerdata);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vHeaderData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		res = curl_easy_perform(curl);

		bool bOK = false;
		if (res == CURLE_OK)
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

			bOK = ((http_code) && (http_code < 400));
			if (!bOK)
			{
				LogError(http_code);
			}
		}
		else
		{
			if (res != CURLE_HTTP_RETURNED_ERROR)
			{
				//Need to generate a header
				std::stringstream ss;
				ss << "HTTP/1.1 " << res << " " << curl_easy_strerror(res);
				vHeaderData.push_back(ss.str());
			}
		}

		curl_easy_cleanup(curl);

		if (headers != NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}

		return bOK;
	}
	catch (...)
	{
		return false;
	}
	return false;
}


bool HTTPClient::POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, std::vector<std::string> &vHeaderData, const bool bFollowRedirect, const long TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);
		if (!bFollowRedirect)
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_curl_headerdata);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vHeaderData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);

		struct curl_slist *headers = NULL;
		if (ExtraHeaders.size() > 0)
		{
			std::vector<std::string>::const_iterator itt;
			for (itt = ExtraHeaders.begin(); itt != ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			if (res == CURLE_HTTP_RETURNED_ERROR)
			{
				//HTTP status code is already inside the header
				long responseCode;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				LogError(responseCode);
			}
			else
			{
				//Need to generate a header
				std::stringstream ss;
				ss << "HTTP/1.1 " << res << " " << curl_easy_strerror(res);
				vHeaderData.push_back(ss.str());
			}
		}

		curl_easy_cleanup(curl);

		if (headers != NULL)
		{
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res == CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
}

bool HTTPClient::PUTBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response,
std::vector<std::string> &vHeaderData, const long TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		//curl_easy_setopt(curl, CURLOPT_PUT, 1);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

		struct curl_slist *headers = NULL;
		if (ExtraHeaders.size() > 0)
		{
			std::vector<std::string>::const_iterator itt;
			for (itt = ExtraHeaders.begin(); itt != ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, putdata.c_str());
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			if (res == CURLE_HTTP_RETURNED_ERROR)
			{
				//HTTP status code is already inside the header
				long responseCode;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				LogError(responseCode);
			}
			else
			{
				//Need to generate a header
				std::stringstream ss;
				ss << "HTTP/1.1 " << res << " " << curl_easy_strerror(res);
				vHeaderData.push_back(ss.str());
			}
		}

		curl_easy_cleanup(curl);

		if (headers != NULL)
		{
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res == CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
}

bool HTTPClient::DeleteBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response,
std::vector<std::string> &vHeaderData, const long TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		//curl_easy_setopt(curl, CURLOPT_PUT, 1);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

		struct curl_slist *headers = NULL;
		if (ExtraHeaders.size() > 0)
		{
			std::vector<std::string>::const_iterator itt;
			for (itt = ExtraHeaders.begin(); itt != ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, putdata.c_str());
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			if (res == CURLE_HTTP_RETURNED_ERROR)
			{
				//HTTP status code is already inside the header
				long responseCode;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				LogError(responseCode);
			}
			else
			{
				//Need to generate a header
				std::stringstream ss;
				ss << "HTTP/1.1 " << res << " " << curl_easy_strerror(res);
				vHeaderData.push_back(ss.str());
			}
		}

		curl_easy_cleanup(curl);

		if (headers != NULL)
		{
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res == CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
}


/************************************************************************
 *									*
 * binary methods without access to return header data			*
 *									*
 ************************************************************************/

bool HTTPClient::GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const long TimeOut)
{
	std::vector<std::string> vHeaderData;
	return GETBinary(url, ExtraHeaders, response, vHeaderData, TimeOut);
}

bool HTTPClient::GETBinarySingleLine(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const long TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
		{
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);
		}

		struct curl_slist *headers = NULL;
		if (ExtraHeaders.size() > 0)
		{
			std::vector<std::string>::const_iterator itt;
			for (itt = ExtraHeaders.begin(); itt != ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data_single_line);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		res = curl_easy_perform(curl);

		if (
			(res == CURLE_WRITE_ERROR) &&
			(!response.empty())
			)
		{
			res = CURLE_OK;
		}

		bool bOK = false;
		if (res == CURLE_OK)
		{
			long http_code = 0;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

			bOK = ((http_code) && (http_code < 400));
			if (!bOK)
			{
				LogError(http_code);
			}
		}

		curl_easy_cleanup(curl);

		if (headers != NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}
		return bOK;
	}
	catch (...)
	{
		return false;
	}
	return false;
}

bool HTTPClient::POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const bool bFollowRedirect, const long TimeOut)
{
	std::vector<std::string> vHeaderData;
	return POSTBinary(url, postdata, ExtraHeaders, response, vHeaderData, bFollowRedirect, TimeOut);
}

bool HTTPClient::PUTBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const long TimeOut)
{
	std::vector<std::string> vHeaderData;
	return PUTBinary(url, putdata, ExtraHeaders, response, vHeaderData, TimeOut);
}

bool HTTPClient::DeleteBinary(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const long TimeOut)
{
	std::vector<std::string> vHeaderData;
	return DeleteBinary(url, putdata, ExtraHeaders, response, vHeaderData, TimeOut);
}


/************************************************************************
 *									*
 * methods with access to the return headers				*
 *									*
 ************************************************************************/

bool HTTPClient::GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	bool bOK = GETBinary(url, ExtraHeaders, vHTTPResponse, vHeaderData, -1);
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	if (!bOK)
		return false;
	if (!bIgnoreNoDataReturned && vHTTPResponse.empty())
		return false;
	return true;
}

bool HTTPClient::POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bFollowRedirect, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!POSTBinary(url, postdata, ExtraHeaders, vHTTPResponse, vHeaderData, bFollowRedirect))
		return false;
	if (!bIgnoreNoDataReturned && vHTTPResponse.empty())
		return false;
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::PUT(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!PUTBinary(url, putdata, ExtraHeaders, vHTTPResponse, vHeaderData))
		return false;
	if (!bIgnoreNoDataReturned && vHTTPResponse.empty())
		return false;
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::Delete(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!DeleteBinary(url, putdata, ExtraHeaders, vHTTPResponse, vHeaderData))
		return false;
	if (!bIgnoreNoDataReturned && vHTTPResponse.empty())
		return false;
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}


/************************************************************************
 *									*
 * methods with optional headers					*
 *									*
 ************************************************************************/

bool HTTPClient::GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return GET(url, ExtraHeaders, response, vHeaderData, bIgnoreNoDataReturned);
}

bool HTTPClient::POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bFollowRedirect, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return POST(url, postdata, ExtraHeaders, response, vHeaderData, bFollowRedirect, bIgnoreNoDataReturned);
}

bool HTTPClient::PUT(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return PUT(url, putdata, ExtraHeaders, response, vHeaderData, bIgnoreNoDataReturned);
}

bool HTTPClient::Delete(const std::string &url, const std::string &putdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return Delete(url, putdata, ExtraHeaders, response, vHeaderData, bIgnoreNoDataReturned);
}


/************************************************************************
 *									*
 * simple access methods						*
 *									*
 ************************************************************************/

bool HTTPClient::GET(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> ExtraHeaders;
	return GET(url, ExtraHeaders, response, bIgnoreNoDataReturned);
}

bool HTTPClient::GETSingleLine(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> ExtraHeaders;
	if (!GETBinarySingleLine(url, ExtraHeaders, vHTTPResponse))
		return false;
	if (!bIgnoreNoDataReturned && vHTTPResponse.empty())
		return false;
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::GETBinaryToFile(const std::string &url, const std::string &outputfile)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;

		//open the output file for writing
		std::ofstream outfile;
		outfile.open(outputfile.c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
		if (!outfile.is_open())
			return false;

		CURL *curl = curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data_file);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&outfile);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		outfile.close();

		return (res==CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
}
