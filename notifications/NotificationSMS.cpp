#include "stdafx.h"
#include "NotificationSMS.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

CNotificationSMS::CNotificationSMS() : CNotificationBase(std::string("clickatell"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("ClickatellEnabled"), &m_IsEnabled);
	SetupConfigBase64(std::string("ClickatellAPI"), _clickatellApi);
	SetupConfigBase64(std::string("ClickatellTo"), _clickatellTo);
}

CNotificationSMS::~CNotificationSMS()
{
}

bool CNotificationSMS::SendMessageImplementation(
	const uint64_t Idx,
	const std::string& Name,
	const std::string& Subject,
	const std::string& Text,
	const std::string& ExtraData,
	const int Priority,
	const std::string& Sound,
	const bool bFromNotification)
{
	//send message by Clickatell SMS
	bool bRet = false;

	std::string thisTo = CURLEncode::URLDecode(_clickatellTo);

	stdreplace(thisTo, "+", "");
	stdreplace(thisTo, " ", "");

	// One can send to multiple destination addresses by delimiting the addresses with commas
	stdreplace(thisTo, ";", ",");

	thisTo = stdstring_trim(thisTo);
	if (thisTo.empty())
		return false;

	std::string sResult;
	std::stringstream sPostData;

	sPostData
		<< "apiKey=" << _clickatellApi
		<< "&to=" << thisTo
		<< "&content=" << Text;

	std::vector<std::string> ExtraHeaders;
	bRet |= HTTPClient::POST("https://platform.clickatell.com/messages/http/send", sPostData.str(), ExtraHeaders, sResult);
	if (sResult.find("ERR:") != std::string::npos)
	{
		//We have an error
		bRet = false;
		_log.Log(LOG_ERROR, "Clickatell SMS Gateway: %s", sResult.c_str());
	}
	return bRet;
}

bool CNotificationSMS::IsConfigured()
{
	return (_clickatellApi != "" && _clickatellTo != "");
}
