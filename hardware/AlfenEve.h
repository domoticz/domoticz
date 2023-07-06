#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

typedef void CURL;

class AlfenEve : public CDomoticzHardwareBase
{
public:
	AlfenEve(const int ID, const std::string& IPAddress, const uint16_t usIPPort, int PollInterval, const std::string& szUsername, const std::string& szPassword);
	~AlfenEve() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	std::string m_szSoftwareVersion;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool DoLogin();
	bool GetProperties(Json::Value& result);
	void DoLogout();

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");
private:
	std::string BuildURL(const std::string& path);
	void parseProperties(const Json::Value& root);
	bool SendCommand(const std::string& command);
	int m_poll_interval = 30;

	std::string m_szIPAddress;
	uint16_t m_IPPort;

	std::string m_szInstallerPassword; // derived from serial number

	std::string m_szUsername;
	std::string m_szPassword;

	std::shared_ptr<std::thread> m_thread;

	std::mutex m_mutex;

	int m_LastBootcount = 0;

	CURL* m_curl = nullptr;
	std::vector<unsigned char> m_vHTTPResponse;
};
