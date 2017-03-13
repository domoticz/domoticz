#include "stdafx.h"
#include "NotificationPushsafer.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"

#include <stdio.h>
#include "../webserver/Base64.h"
#include <curl/curl.h>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


CNotificationPushsafer::CNotificationPushsafer() : CNotificationBase(std::string("pushsafer"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushsaferEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushsaferAPI"), _apikey);
	SetupConfig(std::string("PushsaferImage"), _apiuser);
}

CNotificationPushsafer::~CNotificationPushsafer()
{
}

bool CNotificationPushsafer::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message to Pushsafer
	std::string cSubject = (Subject == Text) ? "Domoticz" : Subject;

	bool bRet;
	std::string sResult;
	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;

	// Load image from URL and base64_encode it
	std::string base64ImageString;
	CURL* curlCtx = curl_easy_init();

	if (!_apiuser.empty()) {

		curl_easy_setopt(curlCtx, CURLOPT_URL, _apiuser.c_str());
		curl_easy_setopt(curlCtx, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curlCtx, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curlCtx, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curlCtx, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curlCtx, CURLOPT_WRITEDATA, &base64ImageString);

		CURLcode rc = curl_easy_perform(curlCtx);
		if (rc)
		{
			_log.Log(LOG_ERROR, "Pushsafer: Failed to download image > wrong URL");
		}

		curl_easy_cleanup(curlCtx);

		base64ImageString = base64_encode((const unsigned char*)base64ImageString.c_str(), base64ImageString.size());

		sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text << "&p=data:image/jpeg;base64," << base64ImageString;
	}
	else
	{
		sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text;
	}

	//Add the required Content Type
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
	
	bRet = HTTPClient::POST("https://www.pushsafer.com/api",sPostData.str(),ExtraHeaders,sResult);
	bool bSuccess = (sResult.find("\"success\":") != std::string::npos);
	if (!bSuccess)
		_log.Log(LOG_ERROR, "Pushsafer: %s", sResult.c_str());
	return (bRet && bSuccess);
}

bool CNotificationPushsafer::IsConfigured()
{
	return _apikey != "";
}
