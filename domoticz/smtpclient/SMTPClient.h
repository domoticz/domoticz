#pragma once
#include <string>
#include <vector>

class SMTPClient
{
public:
	static bool SendEmail(
		const std::string MailFrom,
		const std::string MailTo,
		const std::string MailServer,
		const int MailPort,
		const std::string MailUsername,
		const std::string MailPassword,
		const std::string MailSubject,
		const std::string MailBody,
		const bool bIsHTML);
};

