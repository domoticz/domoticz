#include "stdafx.h"
#include "NotificationNma.h"
#include "../httpclient/HTTPClient.h"

CNotificationNma::CNotificationNma() : CNotificationBase(std::string("nma"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY)
{
	SetupConfig(std::string("NMAAPI"), _apikey);
}

CNotificationNma::~CNotificationNma()
{
}

bool CNotificationNma::SendMessageImplementation(const std::string &Subject, const std::string &Text, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message to NMA
	bool bRet;
	std::stringstream sPostData;
	std::string sResult;
	sPostData << "apikey=" << _apikey  << "&application=Domoticz&event=" << Text << "&description=" << Text << "&priority=" << Priority;
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://www.notifymyandroid.com/publicapi/notify",sPostData.str(),ExtraHeaders,sResult);
	/* todo: parse result */
	return bRet;
}

bool CNotificationNma::IsConfigured()
{
	return _apikey != "";
}
