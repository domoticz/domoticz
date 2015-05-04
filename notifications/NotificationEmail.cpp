#include "stdafx.h"
#include "NotificationEmail.h"
#include "../smtpclient/SMTPClient.h"

CNotificationEmail::CNotificationEmail() : CNotificationBase(std::string("email"), OPTIONS_HTML_BODY)
{
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

bool CNotificationEmail::SendMessageImplementation(const std::string &Subject, const std::string &Text, const std::string &ExtraData, const int Priority, const std::string &Sound, const bool bFromNotification)
{
	if (bFromNotification)
	{
		if (_UseEmailInNotifications != 1) {
			return true; //we are not using email for sending notification messages
		}
	}

	SMTPClient sclient;

	std::string HtmlBody = std::string("<html>\n<body>\n<b>") + Text + std::string("</body>\n</html>\n");
	sclient.SetFrom(_EmailFrom.c_str());
	sclient.SetTo(_EmailTo.c_str());
	if (_EmailUsername != "" && _EmailPassword != "") {
		sclient.SetCredentials(_EmailUsername.c_str(), _EmailPassword.c_str());
	}
	sclient.SetServer(_EmailServer.c_str(), _EmailPort);
	sclient.SetSubject(Subject.c_str());
	sclient.SetHTMLBody(HtmlBody.c_str());
	bool bRet=sclient.SendEmail();
	return bRet;
}

bool CNotificationEmail::IsConfigured()
{
	return (_EmailFrom != "" && _EmailTo != "" && _EmailServer != "" && _EmailPort != 0);
}
