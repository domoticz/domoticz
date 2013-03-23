#include "stdafx.h"
#include "HTTPClient.h"
#if defined WIN32
	#include "../curl/curl.h"
#else
	#include <curl/curl.h>
#endif

bool		HTTPClient::m_bCurlGlobalInitialized = false;
long		HTTPClient::m_iConnectionTimeout=30;
long		HTTPClient::m_iTimeout=10;
std::string	HTTPClient::m_sUserAgent="domoticz/1.0";

size_t write_curl_data(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	std::vector<unsigned char>* pvHTTPResponse = (std::vector<unsigned char>*)userp;
	pvHTTPResponse->insert(pvHTTPResponse->end(),(unsigned char*)contents,(unsigned char*)contents+realsize);
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

void HTTPClient::SetUserAgent(const std::string useragent)
{
	m_sUserAgent=useragent;
}

bool HTTPClient::GETBinary(const std::string url, std::vector<unsigned char> &response)
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
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return (res==CURLE_OK);
}

bool HTTPClient::POSTBinary(const std::string url, const std::string postdata, std::vector<unsigned char> &response)
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
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	return (res==CURLE_OK);
}

bool HTTPClient::GET(const std::string url, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	if (!GETBinary(url,vHTTPResponse))
		return false;
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}

bool HTTPClient::POST(const std::string url, const std::string postdata, std::string &response)
{
	std::vector<unsigned char> vHTTPResponse;
	if (!POSTBinary(url,postdata,vHTTPResponse))
		return false;
	response.insert( response.begin(), vHTTPResponse.begin(), vHTTPResponse.end() );
	return true;
}
