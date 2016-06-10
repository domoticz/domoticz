#include "stdafx.h"
#include "../main/Helper.h"
#include "NotificationHTTP.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/SQLHelper.h"

extern std::string szUserDataFolder;

CNotificationHTTP::CNotificationHTTP() : CNotificationBase(std::string("http"), OPTIONS_NONE)
{
	SetupConfig(std::string("HTTPEnabled"), &m_IsEnabled);
	SetupConfigBase64(std::string("HTTPField1"), _HTTPField1);
	SetupConfigBase64(std::string("HTTPField2"), _HTTPField2);
	SetupConfigBase64(std::string("HTTPField3"), _HTTPField3);
	SetupConfigBase64(std::string("HTTPField4"), _HTTPField4);
	SetupConfigBase64(std::string("HTTPTo"), _HTTPTo);
	SetupConfigBase64(std::string("HTTPURL"), _HTTPURL);
	SetupConfigBase64(std::string("HTTPPostData"), _HTTPPostData);
	SetupConfigBase64(std::string("HTTPPostContentType"), _HTTPPostContentType);
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
		stdreplace(destURL, "#FIELD1", CURLEncode::URLEncode(_HTTPField1));
		stdreplace(destURL, "#FIELD2", CURLEncode::URLEncode(_HTTPField2));
		stdreplace(destURL, "#FIELD3", CURLEncode::URLEncode(_HTTPField3));
		stdreplace(destURL, "#FIELD4", CURLEncode::URLEncode(_HTTPField4));
		stdreplace(destURL, "#TO", CURLEncode::URLEncode(_HTTPTo));
		stdreplace(destURL, "#SUBJECT", CURLEncode::URLEncode(Subject));
		stdreplace(destURL, "#MESSAGE", CURLEncode::URLEncode(Text));

		std::string sResult;
		if (_HTTPPostData.length() > 0)
		{
			std::vector<std::string> ExtraHeaders;
			ExtraHeaders.push_back("Content-type: " + _HTTPPostContentType);
			std::string httpData = _HTTPPostData;
			stdreplace(httpData, "#MESSAGE", Text);
			bRet = HTTPClient::POST(destURL, httpData, ExtraHeaders, sResult);
		}
		else
		{
			bRet = HTTPClient::GET(destURL, sResult, true);
		}
	}
	else if (destURL.find("script://") == 0)
	{
		//Script
		stdreplace(destURL, "#FIELD1", _HTTPField1);
		stdreplace(destURL, "#FIELD2", _HTTPField2);
		stdreplace(destURL, "#FIELD3", _HTTPField3);
		stdreplace(destURL, "#FIELD4", _HTTPField4);
		stdreplace(destURL, "#TO", _HTTPTo);
		stdreplace(destURL, "#SUBJECT", CURLEncode::URLDecode(Subject));
		stdreplace(destURL, "#MESSAGE", CURLEncode::URLDecode(Text));

		std::string scriptname = destURL.substr(9);
		std::string scriptparams = "";
#if !defined WIN32
		if (scriptname.find("/") != 0)
			scriptname = szUserDataFolder + "scripts/" + scriptname;
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
