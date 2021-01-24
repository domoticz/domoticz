#include "stdafx.h"
#include "../main/Helper.h"
#include "NotificationHTTP.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/SQLHelper.h"
#include "../main/Logger.h"

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
	SetupConfigBase64(std::string("HTTPPostHeaders"), _HTTPPostHeaders);
	SetupConfigBase64(std::string("HTTPPostContentType"), _HTTPPostContentType);
}

bool CNotificationHTTP::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	std::string destURL = _HTTPURL;
	bool bSuccess = false;

	size_t uPos = destURL.find("://");
	if (uPos == std::string::npos)
		return false;

	char szPriority[64];
	sprintf(szPriority, "%d", Priority);

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
		stdreplace(destURL, "#PRIORITY", CURLEncode::URLEncode(std::string(szPriority)));

		std::string sResult;
		if (_HTTPPostData.length() > 0)
		{
			std::vector<std::string> ExtraHeaders;
			ExtraHeaders.push_back("Content-type: " + _HTTPPostContentType);
			if (_HTTPPostHeaders.length() > 0)
			{
				std::vector<std::string> ExtraHeaders2;
				StringSplit(_HTTPPostHeaders, "\r\n", ExtraHeaders2);
				for (auto &i : ExtraHeaders2)
				{
					ExtraHeaders.push_back(i);
				}
			}
			std::string httpData = _HTTPPostData;
			stdreplace(httpData, "#FIELD1", _HTTPField1);
			stdreplace(httpData, "#FIELD2", _HTTPField2);
			stdreplace(httpData, "#FIELD3", _HTTPField3);
			stdreplace(httpData, "#FIELD4", _HTTPField4);
			stdreplace(httpData, "#TO", _HTTPTo);
			stdreplace(httpData, "#SUBJECT", Subject);
			stdreplace(httpData, "#MESSAGE", Text);
			stdreplace(httpData, "#PRIORITY", std::string(szPriority));
			bSuccess = HTTPClient::POST(destURL, httpData, ExtraHeaders, sResult);
		}
		else
		{
			bSuccess = HTTPClient::GET(destURL, sResult, true);
		}
		if (!bSuccess)
			_log.Log(LOG_ERROR, "HTTP: %s", sResult.c_str());
	}
	else if (destURL.find("script://") == 0)
	{
		//Script
		stdreplace(destURL, "#FIELD1", _HTTPField1);
		stdreplace(destURL, "#FIELD2", _HTTPField2);
		stdreplace(destURL, "#FIELD3", _HTTPField3);
		stdreplace(destURL, "#FIELD4", _HTTPField4);
		stdreplace(destURL, "#TO", _HTTPTo);

		if (destURL.find("#SUBJECT") != std::string::npos)
		{
			//Make sure the subject is surrounded by quotes "#SUBJECT"
			if (destURL.find("\"#SUBJECT\"") == std::string::npos)
			{
				stdreplace(destURL, "#SUBJECT", "\"#SUBJECT\"");
			}
		}
		if (destURL.find("#MESSAGE") != std::string::npos)
		{
			//Make sure the message is surrounded by quotes "#MESSAGE"
			if (destURL.find("\"#MESSAGE\"") == std::string::npos)
			{
				stdreplace(destURL, "#MESSAGE", "\"#MESSAGE\"");
			}
		}

		stdreplace(destURL, "#SUBJECT", Subject);
		stdreplace(destURL, "#MESSAGE", Text);

		std::string scriptname = destURL.substr(9);
		std::string scriptparams;
#if !defined WIN32
		if (scriptname.find('/') != 0)
			scriptname = szUserDataFolder + "scripts/" + scriptname;
#endif
		//Add parameters
		uPos = scriptname.find(' ');
		if (uPos != std::string::npos)
		{
			scriptparams = scriptname.substr(uPos + 1);
			scriptname = scriptname.substr(0, uPos);
		}
		if (file_exist(scriptname.c_str()))
		{
			m_sql.AddTaskItem(_tTaskItem::ExecuteScript(0.2F, scriptname, scriptparams));
			bSuccess = true;
		}
	}
	return bSuccess;
}

bool CNotificationHTTP::IsConfigured()
{
	return (!_HTTPURL.empty());
}
