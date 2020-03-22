#include "stdafx.h"
#include "NotificationPushbullet.h"
#include "../httpclient/HTTPClient.h"
#include "../main/Logger.h"
#include "../main/json_helper.h"
#include "../httpclient/UrlEncode.h"

CNotificationPushbullet::CNotificationPushbullet() : CNotificationBase(std::string("pushbullet"), OPTIONS_URL_SUBJECT | OPTIONS_URL_BODY | OPTIONS_URL_PARAMS)
{
	SetupConfig(std::string("PushbulletEnabled"), &m_IsEnabled);
	SetupConfig(std::string("PushbulletAPI"), _apikey);
}

CNotificationPushbullet::~CNotificationPushbullet()
{
}

bool CNotificationPushbullet::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	//send message to Pushbullet
	std::string cSubject = (Subject == Text) ? "Domoticz" : Subject;

	bool bRet;
	std::string sPostData;
	std::stringstream sHeaderKey;
	std::string sResult;
	std::vector<std::string> ExtraHeaders;
	Json::Value json;

	//Build the message in JSON
	json["type"] = "note";
	json["title"] = CURLEncode::URLDecode(cSubject);
	json["body"] = CURLEncode::URLDecode(Text);


	sPostData = JSonToFormatString(json);

	//Add the required Access Token and Content Type
	sHeaderKey << "Access-Token: " << _apikey;
	ExtraHeaders.push_back(sHeaderKey.str());
	ExtraHeaders.push_back("Content-Type: application/json");

#ifndef WIN32
	HTTPClient::SetSecurityOptions(true, true);
#endif
	bRet = HTTPClient::POST("https://api.pushbullet.com/v2/pushes",sPostData,ExtraHeaders,sResult);
#ifndef WIN32
	HTTPClient::SetSecurityOptions(false, false);
#endif

	bool bSuccess = (sResult.find("\"created\":") != std::string::npos);
	if (!bSuccess)
		_log.Log(LOG_ERROR, "Pushbullet: %s", sResult.c_str());
	return (bRet && bSuccess);
}

bool CNotificationPushbullet::IsConfigured()
{
	return _apikey != "";
}
