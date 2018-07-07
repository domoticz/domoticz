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

//Configuration functions
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

void HTTPClient::LogError(const long response_code)
{
	switch (response_code)
	{
	case 400:
		_log.Debug(DEBUG_NORM, "HTTP 400: Bad Request");
		break;
	case 401:
		_log.Debug(DEBUG_NORM, "HTTP 401: Unauthorized. Authentication is required, has failed or has not been provided");
		break;
	case 403:
		_log.Debug(DEBUG_NORM, "HTTP 403: Forbidden. The request is valid, but the server is refusing action");
		break;
	case 404:
		_log.Debug(DEBUG_NORM, "HTTP 404: Not Found");
		break;
	case 500:
		_log.Debug(DEBUG_NORM, "HTTP 500: Internal Server Error");
		break;
	case 503:
		_log.Debug(DEBUG_NORM, "HTTP 503: Service Unavailable");
		break;
	default:
		_log.Debug(DEBUG_NORM, "HTTP return code is: %li", response_code);
		break;
	}
}

bool HTTPClient::GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const int TimeOut)
{
	std::vector<std::string> vHeaderData;
	return GETBinary(url, ExtraHeaders, response, vHeaderData, TimeOut);
}

bool HTTPClient::GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, std::vector<std::string> &vHeaderData, const int TimeOut)
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
		}

		if (headers != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_curl_headerdata);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &vHeaderData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			// Push response/error code to end of vHeaderData vector
			std::stringstream ss;
			if (res == CURLE_HTTP_RETURNED_ERROR)
			{
				long responseCode;
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
				ss << responseCode;
				LogError(responseCode);
			}
			else
				ss << res;
			vHeaderData.push_back(ss.str());
		}

		curl_easy_cleanup(curl);

		if (headers != NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res == CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
	return false;
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

bool HTTPClient::GETBinarySingleLine(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const int TimeOut)
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
		}

		if (headers != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data_single_line);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
		res = curl_easy_perform(curl);

		if (res == CURLE_HTTP_RETURNED_ERROR)
		{
			long responseCode;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
			LogError(responseCode);
		}

		curl_easy_cleanup(curl);

		if (headers != NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}
		if (
			(res == CURLE_WRITE_ERROR) &&
			(!response.empty())
			)
			res = CURLE_OK;
		return (res == CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
	return false;
}

bool HTTPClient::POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const bool bFollowRedirect)
{
	std::vector<std::string> vHeaderData;
	return POSTBinary(url, postdata, ExtraHeaders, response, vHeaderData, bFollowRedirect);
}

bool HTTPClient::POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, std::vector<std::string> &vHeaderData, const bool bFollowRedirect)
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
		if (!bFollowRedirect)
		{
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
		}

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
		}

		if (headers != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);

		// Push status code to end of vHeaderData vector
		long responseCode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
		std::stringstream ss;
		ss << responseCode;
		vHeaderData.push_back(ss.str());

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

bool HTTPClient::PUTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response)
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
		}

		if (headers != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);
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

bool HTTPClient::DeleteBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response)
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
		}

		if (headers != NULL)
		{
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);
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


bool HTTPClient::GET(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> ExtraHeaders;
	if (!GETBinary(url, ExtraHeaders, vHTTPResponse, -1))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}

bool HTTPClient::GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return GET(url, ExtraHeaders, response, vHeaderData, bIgnoreNoDataReturned);
}

bool HTTPClient::GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!GETBinary(url, ExtraHeaders, vHTTPResponse, vHeaderData, -1))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::GETSingleLine(const std::string &url, std::string &response, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> ExtraHeaders;
	if (!GETBinarySingleLine(url, ExtraHeaders, vHTTPResponse))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bFollowRedirect, const bool bIgnoreNoDataReturned)
{
	std::vector<std::string> vHeaderData;
	return POST(url, postdata, ExtraHeaders, response, vHeaderData, bFollowRedirect, bIgnoreNoDataReturned);
}

bool HTTPClient::POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, std::vector<std::string> &vHeaderData, const bool bFollowRedirect, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!POSTBinary(url, postdata, ExtraHeaders, vHTTPResponse, vHeaderData, bFollowRedirect))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::PUT(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!PUTBinary(url, postdata, ExtraHeaders, vHTTPResponse))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}

bool HTTPClient::Delete(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response, const bool bIgnoreNoDataReturned)
{
	response = "";
	std::vector<unsigned char> vHTTPResponse;
	if (!DeleteBinary(url, postdata, ExtraHeaders, vHTTPResponse))
		return false;
	if (!bIgnoreNoDataReturned)
	{
		if (vHTTPResponse.empty())
			return false;
	}
	response.insert(response.begin(), vHTTPResponse.begin(), vHTTPResponse.end());
	return true;
}
