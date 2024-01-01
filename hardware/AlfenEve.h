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
	bool SetChargeCurrent(const int current);
	void SetSetpoint(const int idx, const float value);
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool DoLogin();
	bool GetProperties(Json::Value& result);
	bool GetInfo();
	void DoLogout();

	void SetGETHeaders();
	void SetPOSTHeaders();
private:
	std::string BuildURL(const std::string& path);
	void parseProperties(const Json::Value& root);
	bool SendCommand(const std::string& command);
	bool SetProperty(const std::string& szName, const int Value);
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
