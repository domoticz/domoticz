#include "stdafx.h"
#include "NotificationPushalot.h"
#include "../httpclient/HTTPClient.h"

CNotificationPushalot::CNotificationPushalot() : CNotificationBase(std::string("pushalot"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushALotEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushALotAPI"), _apikey);
}

CNotificationPushalot::~CNotificationPushalot()
{
}

bool CNotificationPushalot::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	//send message to PushAlot
	bool bRet;
	std::stringstream sPostData;
	std::string IsImportant;
	std::string IsSilent;
	std::string sResult;

	// map priority to PushAlot 'IsSilent' & 'IsImportant'
	switch (Priority) {
	case -2: // Fall through to -1
	case -1:
		IsImportant = "False";
		IsSilent = "True";
		break;
	case 2: // Fall through to 1
	case 1:
		IsImportant = "True";
		IsSilent = "False";
		break;
	default:
		IsImportant = "False";
		IsSilent = "False";
		break;
	}

	sPostData << "AuthorizationToken=" << _apikey << "&IsImportant=" << IsImportant << "&IsSilent=" << IsSilent << "&Source=Domoticz&Body=" << Text;
	std::vector<std::string> ExtraHeaders;
	bRet = HTTPClient::POST("https://pushalot.com/api/sendmessage", sPostData.str(), ExtraHeaders, sResult);
	return bRet;
}

bool CNotificationPushalot::IsConfigured()
{
	return _apikey != "";
}
