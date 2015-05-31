#include "stdafx.h"
#include "NotificationNma.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"

CNotificationNma::CNotificationNma() : CNotificationBase(std::string("nma"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("NMAEnabled"), &m_IsEnabled);
	SetupConfig(std::string("NMAAPI"), _apikey);
}

CNotificationNma::~CNotificationNma()
{
}

bool CNotificationNma::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message to NMA
	bool bRet;
	std::stringstream sPostData;
	std::string sResult;
	sPostData << "apikey=" << _apikey << "&application=Domoticz&event=" << Subject << "&description=" << Text << "&priority=" << Priority;
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://www.notifymyandroid.com/publicapi/notify",sPostData.str(),ExtraHeaders,sResult);
	bool bSuccess = (sResult.find("success code=\"200\"") != std::string::npos);
	if (!bSuccess)
		_log.Log(LOG_ERROR, "NMA: %s", sResult.c_str());
	return (bRet && bSuccess);
}

bool CNotificationNma::IsConfigured()
{
	return _apikey != "";
}
