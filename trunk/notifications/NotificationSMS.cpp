#include "stdafx.h"
#include "NotificationSMS.h"
#include "../httpclient/HTTPClient.h"

CNotificationSMS::CNotificationSMS() : CNotificationBase(std::string("chimitsms"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY)
{
	SetupConfig(std::string("SMSApiKey"), _apikey);
}

CNotificationSMS::~CNotificationSMS()
{
}

bool CNotificationSMS::SendMessageImplementation(const std::string &Subject, const std::string &Text, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message by sms
	bool bRet;
	std::string sResult;
	std::stringstream sPostData;

	sPostData << "ApiKey=" << _apikey << "&source=Domoticz&text=" << Text;
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://sms.chimit.nl/cgi-bin/domoticz.cgi", sPostData.str(), ExtraHeaders, sResult);
	/* todo: parse result */
	return bRet;
}

bool CNotificationSMS::IsConfigured()
{
	return (_apikey != "");
}
