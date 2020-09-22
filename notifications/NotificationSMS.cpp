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

	thisTo = stdstring_trim(thisTo);
	if (thisTo.empty())
		return false;

	std::string thisFrom = CURLEncode::URLDecode(_clickatellFrom);

	stdreplace(thisFrom, "+", "");
	stdreplace(thisFrom, " ", "");
	thisFrom = stdstring_trim(thisFrom);

	if (thisTo.find(";") != std::string::npos) {
		std::vector<std::string> recipients;
		StringSplit(thisTo, ";", recipients);

		thisTo = "";
		for (size_t i = 0; i < recipients.size(); i++) {
			thisTo = thisTo + "\"" + recipients.at(i) + "\"" + ",";
		}

		if (!thisTo.empty()) {
			thisTo.pop_back();
		}
	}
	else {
		thisTo = "\"" + thisTo + "\"";
	}
	
	std::string sResult;
	std::stringstream sJsonPostData;

	std::string message = CURLEncode::URLDecode(Text);
	sJsonPostData
		<< "{"
		<< "\"content\":" << "\"" << message << "\","
		<< "\"to\":" << "[" << thisTo << "],";

	if (!thisFrom.empty()) {
		sJsonPostData << "\"from\":" << "\"" << thisFrom << "\",";
	}

	sJsonPostData
		<< "\"binary\": false,"
		<< "\"charset\": \"UTF-8\""
		<< "}";

	_log.Log(LOG_NORM, "Clickatell SMS notification json: " + sJsonPostData.str());

	std::string apiKey = CURLEncode::URLDecode(_clickatellApi);
	std::vector<std::string> ExtraHeaders;
	ExtraHeaders.push_back("Authorization: " + apiKey);
	ExtraHeaders.push_back("Content-Type: application/json");
	ExtraHeaders.push_back("Accept: application/json");
	bRet |= HTTPClient::POST("https://platform.clickatell.com/messages", sJsonPostData.str(), ExtraHeaders, sResult);
	_log.Log(LOG_NORM, "Clickatell SMS Gateway: %s", sResult.c_str());
	if (sResult.find("ERR:") != std::string::npos)
	{
		//We have an error
		bRet = false;
	}
	return bRet;
}

bool CNotificationSMS::IsConfigured()
{
	return (_clickatellApi != "" && _clickatellTo != "");
}
