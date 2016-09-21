#include "stdafx.h"
#include "NotificationPushsafer.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"

CNotificationPushsafer::CNotificationPushsafer() : CNotificationBase(std::string("pushsafer"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushsaferEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushsaferAPI"), _apikey);
}

CNotificationPushsafer::~CNotificationPushsafer()
{
}

bool CNotificationPushsafer::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message to Pushsafer
	std::string cSubject = (Subject == Text) ? "Domoticz" : Subject;

	bool bRet;
	std::string sResult;
	std::stringstream sPostData;
	std::vector<std::string> ExtraHeaders;
	
	sPostData << "k=" << _apikey << "&t=" << cSubject << "&m=" << Text;

	//Add the required Access Token and Content Type
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
