#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
} // namespace Json

class CHttpPoller : public CDomoticzHardwareBase
{
      public:
	CHttpPoller(int ID, const std::string &username, const std::string &password, const std::string &url, const std::string &extradata, unsigned short refresh);
	~CHttpPoller() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();
	void GetScript();

      private:
	std::string m_username;
	std::string m_password;
	std::string m_url;
	std::string m_script;
	std::string m_contenttype;
	std::string m_headers;
	std::string m_postdata;
	unsigned short m_method;
	unsigned short m_refresh;
	std::shared_ptr<std::thread> m_thread;
};
