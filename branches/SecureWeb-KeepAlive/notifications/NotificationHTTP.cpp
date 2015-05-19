#include "stdafx.h"
#include "../main/Helper.h"
#include "NotificationHTTP.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/SQLHelper.h"

CNotificationHTTP::CNotificationHTTP() : CNotificationBase(std::string("http"), OPTIONS_NONE)
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
	bool bRet = false;

	size_t uPos = destURL.find("://");
	if (uPos == std::string::npos)
		return false;

	if (destURL.find("http") == 0)
	{
		//HTTP/HTTPS
		destURL = stdreplace(destURL, "#FIELD1", CURLEncode::URLEncode(_HTTPField1));
		destURL = stdreplace(destURL, "#FIELD2", CURLEncode::URLEncode(_HTTPField2));
		destURL = stdreplace(destURL, "#FIELD3", CURLEncode::URLEncode(_HTTPField3));
		destURL = stdreplace(destURL, "#FIELD4", CURLEncode::URLEncode(_HTTPField4));
		destURL = stdreplace(destURL, "#TO", CURLEncode::URLEncode(_HTTPTo));
		destURL = stdreplace(destURL, "#SUBJECT", CURLEncode::URLEncode(Subject));
		destURL = stdreplace(destURL, "#MESSAGE", CURLEncode::URLEncode(Text));
		std::string sResult;
		bRet = HTTPClient::GET(destURL, sResult);
	}
	else if (destURL.find("script://") == 0)
	{
		//Script

		destURL = stdreplace(destURL, "#FIELD1", _HTTPField1);
		destURL = stdreplace(destURL, "#FIELD2", _HTTPField2);
		destURL = stdreplace(destURL, "#FIELD3", _HTTPField3);
		destURL = stdreplace(destURL, "#FIELD4", _HTTPField4);
		destURL = stdreplace(destURL, "#TO", _HTTPTo);
		destURL = stdreplace(destURL, "#SUBJECT", CURLEncode::URLDecode(Subject));
		destURL = stdreplace(destURL, "#MESSAGE", CURLEncode::URLDecode(Text));

		std::string scriptname = "";
		std::string scriptparams = "";
#if !defined WIN32
		if (destURL.find("script:///") != std::string::npos)
			scriptname = destURL.substr(9);
		else
			scriptname = destURL.substr(8);
#else
		scriptname = destURL.substr(9);
#endif
		//Add parameters
		uPos = scriptname.find(" ");
		if (uPos != std::string::npos)
		{
			scriptparams = scriptname.substr(uPos + 1);
			scriptname = scriptname.substr(0, uPos);
		}
		if (file_exist(scriptname.c_str()))
		{
			m_sql.AddTaskItem(_tTaskItem::ExecuteScript(1, scriptname, scriptparams));
			bRet = true;
		}
	}
	return bRet;
}

bool CNotificationHTTP::IsConfigured()
{
	return (!_HTTPURL.empty());
}
