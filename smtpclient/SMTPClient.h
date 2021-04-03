#pragma once
#include <string>

class SMTPClient
{
      public:
	SMTPClient();
	~SMTPClient() = default;

	void SetFrom(const std::string &From);
	void SetTo(const std::string &To);
	void SetSubject(const std::string &Subject);
	void SetServer(const std::string &Server, int Port);
	void SetCredentials(const std::string &Username, const std::string &Password);
	void AddAttachment(const std::string &Base64EncodedData, const std::string &FileName); // should already be base64 encoded
	void SetPlainBody(const std::string &body);
	void SetHTMLBody(const std::string &body);

	bool SendEmail();

      private:
	std::string MakeMessage();
	std::vector<std::string> m_Recipients;
	std::string m_From;
	std::string m_Server;
	int m_Port;
	std::string m_Username;
	std::string m_Password;
	std::string m_Subject;
	std::string m_HTMLBody;
	std::string m_PlainBody;
	std::vector<std::pair<std::string, std::string>> m_Attachments; // data(base64 encoded), filename(type)
};
