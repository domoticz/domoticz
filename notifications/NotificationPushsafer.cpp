#include "stdafx.h"
#include "NotificationPushsafer.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "../httpclient/HTTPClient.h"
#include "../webserver/Base64.h"

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

	if (!_apiuser.empty()) {

		std::vector<unsigned char> camimage;
		std::vector<std::string> ExtraHeadersBinary;

		if (HTTPClient::GETBinary(CURLEncode::URLDecode(_apiuser.c_str()), ExtraHeadersBinary, camimage, 10))
		{
			std::string base64ImageString(camimage.begin(), camimage.end());
			base64ImageString = base64_encode((const unsigned char*)base64ImageString.c_str(), base64ImageString.size());
			sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text << "&p=data:image/jpeg;base64," << base64ImageString;
		}
		else
		{
			_log.Log(LOG_ERROR, "Pushsafer: can't download image > wrong url");
			sPostData << "i=12&k=" << _apikey << "&t=" << cSubject << "&m=" << Text;
		}

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
