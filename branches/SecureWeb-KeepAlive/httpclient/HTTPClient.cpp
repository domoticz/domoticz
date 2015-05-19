#include "stdafx.h"
#include "HTTPClient.h"
#include <curl/curl.h>

#include <iostream>
#include <fstream>

#ifndef O_LARGEFILE
	#define O_LARGEFILE 0
#endif

bool		HTTPClient::m_bCurlGlobalInitialized = false;
long		HTTPClient::m_iConnectionTimeout=30;
long		HTTPClient::m_iTimeout=90; //max, time that a download has to be finished?
std::string	HTTPClient::m_sUserAgent="domoticz/1.0";

size_t write_curl_data(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	std::vector<unsigned char>* pvHTTPResponse = (std::vector<unsigned char>*)userp;
	pvHTTPResponse->insert(pvHTTPResponse->end(),(unsigned char*)contents,(unsigned char*)contents+realsize);
	return realsize;
}

size_t write_curl_data_file(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	std::ofstream *outfile=(std::ofstream*)userp;
	outfile->write((const char*)contents,realsize);
	return realsize;
}

bool HTTPClient::CheckIfGlobalInitDone()
{
	if (!m_bCurlGlobalInitialized) {
		CURLcode res=curl_global_init(CURL_GLOBAL_ALL);
		if (res!=CURLE_OK)
			return false;
		m_bCurlGlobalInitialized=true;
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
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_curl_data);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, m_sUserAgent.c_str());
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, m_iConnectionTimeout);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT,m_iTimeout); 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "domocookie.txt"); 
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR,  "domocookie.txt"); 
}

//Configuration functions
void HTTPClient::SetConnectionTimeout(const long timeout)
{
	m_iConnectionTimeout=timeout;
}

void HTTPClient::SetTimeout(const long timeout)
{
	m_iTimeout=timeout;
}

void HTTPClient::SetUserAgent(const std::string &useragent)
{
	m_sUserAgent=useragent;
}

bool HTTPClient::GETBinary(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response, const int TimeOut)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl=curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		if (TimeOut != -1)
		{
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, TimeOut);
		}

		struct curl_slist *headers=NULL;
		if (ExtraHeaders.size()>0) {
			std::vector<std::string>::const_iterator itt;
			for (itt=ExtraHeaders.begin(); itt!=ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
		}

		if (headers!=NULL) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
		}

		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (headers!=NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res==CURLE_OK);
	}
	catch (...)
	{
		return false;		
	}
}

bool HTTPClient::GETBinaryToFile(const std::string &url, const std::string &outputfile)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;

		//open the output file for writing
		std::ofstream outfile;
		outfile.open(outputfile.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
		if (!outfile.is_open())
			return false;

		CURL *curl=curl_easy_init();
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

bool HTTPClient::POSTBinary(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::vector<unsigned char> &response)
{
	try
	{
		if (!CheckIfGlobalInitDone())
			return false;
		CURL *curl=curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);

		struct curl_slist *headers=NULL;
		if (ExtraHeaders.size()>0) {
			std::vector<std::string>::const_iterator itt;
			for (itt=ExtraHeaders.begin(); itt!=ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
		}

		if (headers!=NULL) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (headers!=NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res==CURLE_OK);
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
		CURL *curl=curl_easy_init();
		if (!curl)
			return false;

		CURLcode res;
		SetGlobalOptions(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		//curl_easy_setopt(curl, CURLOPT_PUT, 1);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");


		struct curl_slist *headers=NULL;
		if (ExtraHeaders.size()>0) {
			std::vector<std::string>::const_iterator itt;
			for (itt=ExtraHeaders.begin(); itt!=ExtraHeaders.end(); ++itt)
			{
				headers = curl_slist_append(headers, (*itt).c_str());
			}
		}

		if (headers!=NULL) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
		}

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (headers!=NULL) {
			curl_slist_free_all(headers); /* free the header list */
		}

		return (res==CURLE_OK);
	}
	catch (...)
	{
		return false;
	}
}


bool HTTPClient::GET(const std::string &url, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	std::vector<std::string> ExtraHeaders;
	if (!GETBinary(url,ExtraHeaders,vHTTPResponse))
		return false;
	response="";
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}

bool HTTPClient::GET(const std::string &url, const std::vector<std::string> &ExtraHeaders, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	if (!GETBinary(url,ExtraHeaders,vHTTPResponse))
		return false;
	response="";
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}

bool HTTPClient::POST(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	if (!POSTBinary(url,postdata,ExtraHeaders, vHTTPResponse))
		return false;
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}

bool HTTPClient::PUT(const std::string &url, const std::string &postdata, const std::vector<std::string> &ExtraHeaders, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	if (!PUTBinary(url,postdata,ExtraHeaders, vHTTPResponse))
		return false;
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}
