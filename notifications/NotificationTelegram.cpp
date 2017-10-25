#include "stdafx.h"
#include "NotificationTelegram.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../json/json.h"
#include "../httpclient/UrlEncode.h"

CNotificationTelegram::CNotificationTelegram() : CNotificationBase(std::string("telegram"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("TelegramEnabled"), &m_IsEnabled);
	SetupConfig(std::string("TelegramAPI"), _apikey);
	SetupConfig(std::string("TelegramChat"), _chatid);
}

CNotificationTelegram::~CNotificationTelegram()
{
}

bool CNotificationTelegram::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message to Telegram
	//std::string cSubject = (Subject == Text) ? "Domoticz" : Subject;

	bool bRet;
	std::string sPostData;
	std::string sUrl;
	std::stringstream sBuildUrl;
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	Json::Value json;
	Json::StyledWriter jsonWriter;

	//Build url
	sBuildUrl << "https://api.telegram.org/bot" << _apikey << "/sendMessage";
	sUrl = sBuildUrl.str();

	//Build the message in JSON
	json["chat_id"] = _chatid;
	json["text"] = CURLEncode::URLDecode(Text);
	//json["body"] = CURLEncode::URLDecode(Text);
	sPostData = jsonWriter.write(json);

	//Add the required Content Type
	ExtraHeaders.push_back("Content-Type: application/json");

#ifndef WIN32
	HTTPClient::SetSecurityOptions(true, true);
#endif
	bRet = HTTPClient::POST(sUrl,sPostData,ExtraHeaders,sResult);
#ifndef WIN32
	HTTPClient::SetSecurityOptions(false, false);
#endif
//"ok":true
	bool bSuccess = (sResult.find("\"ok\":true") != std::string::npos);
	if (!bSuccess)
		_log.Log(LOG_ERROR, "Telegram: %s", sResult.c_str());
	return (bRet && bSuccess);
}

bool CNotificationTelegram::IsConfigured()
{
	return (_apikey != "" && _chatid != "");
}
