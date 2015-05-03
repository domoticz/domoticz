#include "stdafx.h"
#include "NotificationProwl.h"
#include "../httpclient/HTTPClient.h"

CNotificationProwl::CNotificationProwl() : CNotificationBase(std::string("prowl"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("ProwlEnabled"), &m_IsEnabled);
	SetupConfig(std::string("ProwlAPI"), _apikey);
}

CNotificationProwl::~CNotificationProwl()
{
}

bool CNotificationProwl::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message to Prowl
	bool bRet;
	std::stringstream sPostData;
	std::string sResult;
	sPostData << "apikey=" << _apikey << "&application=Domoticz&event=" << Subject << "&description=" << Text << "&priority=" << Priority;
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://api.prowlapp.com/publicapi/add",sPostData.str(),ExtraHeaders,sResult);
	/* todo: parse result */
	return bRet;
}

bool CNotificationProwl::IsConfigured()
{
	return _apikey != "";
}
