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
	SetupConfigBase64(std::string("ClickatellFrom"), _clickatellFrom);
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

	std::string thisFrom = CURLEncode::URLDecode(_clickatellFrom);
	_log.Log(LOG_NORM, "From: " + thisFrom);

	stdreplace(thisFrom, "+", "");
	stdreplace(thisFrom, " ", "");
	thisFrom = stdstring_trim(thisFrom);

	std::string sResult;
	std::stringstream sGetData;

	sGetData
		<< "apiKey=" << _clickatellApi
		<< "&to=" << thisTo
		<< "&content=" << Text;

	if (!thisFrom.empty()) {
		sGetData << "&from=" << thisFrom;
	}
		
	_log.Log(LOG_NORM, "Get params: " + sGetData.str());

	std::vector<std::string> ExtraHeaders;
	bRet |= HTTPClient::GET("https://platform.clickatell.com/messages/http/send?" + sGetData.str(), ExtraHeaders, sResult);
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
