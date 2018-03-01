#include "stdafx.h"
#include "NotificationEmail.h"
#include "../smtpclient/SMTPClient.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"

const static char *szHTMLMail =
"<html>\n"
"<head>\n"
"<style>\n"
"table{\n"
"border: 1px solid #cccccc;\n"
"border-collapse: collapse;\n"
"width: 100%;\n"
"}\n"
"thead{\n"
"background: #6099B7;\n"
"font-weight: bold;\n"
"text-align: left;\n"
"color: #ffffff;\n"
"}\n"
"td, th{\n"
"padding: 8px;\n"
"}\n"
"body{\n"
"font: 12px Arial, Helvetica, sans-serif;\n"
"}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<table>\n"
"<thead>\n"
"<tr>\n"
"<th>$DEVNAME (idx: $DEVIDX)</th>\n"
"<th style=\"text-align:right\">$DATETIME</th>\n"
"</tr>\n"
"</thead>\n"
"<tbody>\n"
"<tr>\n"
"<td colspan=\"2\">$MESSAGE</td>\n"
"</tr>\n"
"</tbody>\n"
"</table>\n"
"</body>\n"
"</html>\n";


CNotificationEmail::CNotificationEmail() : CNotificationBase(std::string("email"), OPTIONS_HTML_BODY)
{
	SetupConfig(std::string("EmailEnabled"), &m_IsEnabled);
	SetupConfig(std::string("EmailFrom"), _EmailFrom);
	SetupConfig(std::string("EmailTo"), _EmailTo);
	SetupConfig(std::string("EmailServer"), _EmailServer);
	SetupConfig(std::string("EmailPort"), &_EmailPort);
	SetupConfigBase64(std::string("EmailUsername"), _EmailUsername);
	SetupConfigBase64(std::string("EmailPassword"), _EmailPassword);
	SetupConfig(std::string("UseEmailInNotifications"), &_UseEmailInNotifications);
	SetupConfig(std::string("EmailAsAttachment"), &_EmailAsAttachment);
}

CNotificationEmail::~CNotificationEmail()
{
}

bool CNotificationEmail::SendMessageImplementation(
	const uint64_t Idx,
	const std::string &Name,
	const std::string &Subject,
	const std::string &Text,
	const std::string &ExtraData,
	const int Priority,
	const std::string &Sound,
	const bool bFromNotification)
{
	if (bFromNotification)
	{
		if (_UseEmailInNotifications != 1) {
			return true; //we are not using email for sending notification messages
		}
	}

	SMTPClient sclient;

	std::string MessageText = Text;
	stdreplace(MessageText, "&lt;br&gt;", "<br>");

	std::string HtmlBody;

	if (Idx != 0)
	{
		HtmlBody = szHTMLMail;

		std::stringstream sstr;
		sstr << Idx;
		stdreplace(HtmlBody, "$DEVIDX", sstr.str());
		stdreplace(HtmlBody, "$DEVNAME", Name);
		stdreplace(HtmlBody, "$MESSAGE", MessageText);

		std::string szDate = TimeToString(NULL, TF_DateTimeMs);
		stdreplace(HtmlBody, "$DATETIME", szDate);
	}
	else
	{
		HtmlBody = std::string("<html>\n<body>\n<b>") + MessageText + std::string("</body>\n</html>\n");
	}

	sclient.SetFrom(_EmailFrom.c_str());
	sclient.SetTo(_EmailTo.c_str());
	if (_EmailUsername != "" && _EmailPassword != "") {
		sclient.SetCredentials(_EmailUsername.c_str(), _EmailPassword.c_str());
	}
	sclient.SetServer(_EmailServer.c_str(), _EmailPort);
	sclient.SetSubject(Subject.c_str());
	sclient.SetHTMLBody(HtmlBody.c_str());
	bool bRet=sclient.SendEmail();
	if (!bRet) {
		_log.Log(LOG_ERROR, "Failed to send Email notification!");
	}
	return bRet;
}

bool CNotificationEmail::IsConfigured()
{
	return (_EmailFrom != "" && _EmailTo != "" && _EmailServer != "" && _EmailPort != 0);
}
