#include "stdafx.h"
#include "NotificationPushover.h"
#include "../httpclient/HTTPClient.h"

CNotificationPushover::CNotificationPushover() : CNotificationBase(std::string("pushover"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushoverEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushoverAPI"), _apikey);
	SetupConfig(std::string("PushoverUser"), _apiuser);
}

CNotificationPushover::~CNotificationPushover()
{
}

bool CNotificationPushover::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	bool bRet;
	std::string sResult;
	std::stringstream sPostData;

	sPostData << "token=" << _apikey << "&user=" << _apiuser << "&priority=" << Priority << "&title=" << Subject << "&message=" << Text;

	if (Priority == 2) {
		sPostData << "&retry=300&expire=3600";
	}
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://api.pushover.net/1/messages.json",sPostData.str(),ExtraHeaders,sResult);
	return bRet;
}

bool CNotificationPushover::IsConfigured()
{
	return _apikey != "" && _apiuser != "";
}
