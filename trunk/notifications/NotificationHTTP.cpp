#include "stdafx.h"
#include "../main/Helper.h"
#include "NotificationHTTP.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/SQLHelper.h"

CNotificationHTTP::CNotificationHTTP() : CNotificationBase(std::string("http"), OPTIONS_HTML_BODY)
{
	SetupConfig(std::string("HTTPEnabled"), &m_IsEnabled);
	SetupConfigBase64(std::string("HTTPField1"), _HTTPField1);
	SetupConfigBase64(std::string("HTTPField2"), _HTTPField2);
	SetupConfigBase64(std::string("HTTPField3"), _HTTPField3);
	SetupConfigBase64(std::string("HTTPField4"), _HTTPField4);
	SetupConfigBase64(std::string("HTTPTo"), _HTTPTo);
	SetupConfigBase64(std::string("HTTPURL"), _HTTPURL);
}

CNotificationHTTP::~CNotificationHTTP()
{
}

bool CNotificationHTTP::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	std::string destURL = _HTTPURL;
	std::string sResult;
	bool bRet;

	destURL = stdreplace(destURL, "#FIELD1", _HTTPField1);
	destURL = stdreplace(destURL, "#FIELD2", _HTTPField2);
	destURL = stdreplace(destURL, "#FIELD3", _HTTPField3);
	destURL = stdreplace(destURL, "#FIELD4", _HTTPField4);
	destURL = stdreplace(destURL, "#TO", _HTTPTo);

	destURL = CURLEncode::URLEncode(destURL);

	bRet = HTTPClient::GET(destURL, sResult);
	return bRet;
}

bool CNotificationHTTP::IsConfigured()
{
	return (!_HTTPURL.empty());
}
