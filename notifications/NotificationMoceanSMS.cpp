#include "stdafx.h"
#include "NotificationMoceanSMS.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

CNotificationMoceanSMS::CNotificationMoceanSMS() : CNotificationBase(std::string("moceansms"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("MoceanEnabled"), &m_IsEnabled);
	SetupConfigBase64(std::string("MoceanAPIKey"), _moceanApiKey);
	SetupConfigBase64(std::string("MoceanAPISecret"), _moceanApiSecret);
	SetupConfigBase64(std::string("MoceanTo"), _moceanTo);
	SetupConfigBase64(std::string("MoceanFrom"), _moceanFrom);
}

CNotificationMoceanSMS::~CNotificationMoceanSMS()
{
}

bool CNotificationMoceanSMS::SendMessageImplementation(
	const uint64_t Idx,
	const std::string& Name,
	const std::string& Subject,
	const std::string& Text,
	const std::string& ExtraData,
	const int Priority,
	const std::string& Sound,
	const bool bFromNotification)
{
	/* send message by Mocean SMS */
	bool bRet = false;

	std::string sTo = CURLEncode::URLDecode(_moceanTo);
	std::string sFrom = CURLEncode::URLDecode(_moceanFrom);

	stdreplace(sTo, "+", "");
	stdreplace(sTo, " ", "");

	/* Multiple destination is supported, seperated by ',' */
	stdreplace(sTo, ";", ",");

	/**
	 * mocean-from is required. Sender ID is subjected to carrier availability.
	 * Using "Dominic Notification" if not specified.
	 */
	sFrom = stdstring_trim(sFrom);
	if (sFrom.empty())
	{
		sFrom = "Domonitz Notification";
	}

	sTo = stdstring_trim(sTo);
	if (sTo.empty())
		return false;

	std::string sResult;
	std::stringstream sPostData;

	sPostData
		<< "mocean-api-key=" << _moceanApiKey
		<< "&mocean-api-secret=" << _moceanApiSecret
		<< "&mocean-from=" << sFrom
		<< "&mocean-to=" << sTo
		<< "&mocean-medium=domoticz"
		<< "&mocean-resp-format=json"
		<< "&mocean-text=" << Text;

	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	bRet |= HTTPClient::POST("https://rest.moceanapi.com/rest/2/sms", sPostData.str(), ExtraHeaders, sResult);
	if (sResult.find("err_msg") != std::string::npos)
	{
		/* We have an error */
		bRet = false;
		_log.Log(LOG_ERROR, "Mocean SMS Gateway: %s", sResult.c_str());
	}
	return bRet;
}

bool CNotificationMoceanSMS::IsConfigured()
{
	return ((_moceanApiKey != "") && (_moceanApiSecret != "") && (_moceanTo != ""));
}
