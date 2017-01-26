#pragma once

#include "DomoticzHardware.h"

namespace Json
{
	class Value;
};

class CHttpPoller : public CDomoticzHardwareBase
{
public:
	CHttpPoller(const int ID, const std::string& username, const std::string& password, const std::string& url, const std::string& extradata, const unsigned short refresh);
	~CHttpPoller(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);

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

	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetScript();
};

