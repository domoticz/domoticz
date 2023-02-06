#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class EnphaseAPI : public CDomoticzHardwareBase
{
public:
	EnphaseAPI(int ID, const std::string& IPAddress, unsigned short usIPPort, int PollInterval, const bool bPollInverters, const std::string& szUsername, const std::string& szPassword);
	~EnphaseAPI() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;

private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool GetSerialSoftwareVersion();
	bool GetAccessToken();
	bool getProductionDetails(Json::Value& result);
	bool getInverterDetails();
	std::string V5_emupwGetMobilePasswd(const std::string &serialNumber, const std::string &userName, const std::string &realm);

	void parseProduction(const Json::Value& root);
	void parseConsumption(const Json::Value& root);
	void parseStorage(const Json::Value& root);

	bool IsItSunny();
	int getSunRiseSunSetMinutes(bool bGetSunRise);

	bool NeedToken();

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");
private:
	int m_poll_interval = 30;

	std::string m_szSerial;
	std::string m_szSoftwareVersion;
	std::string m_szToken;
	std::string m_szIPAddress;
	std::string m_szInstallerPassword; // derived from serial number

	std::string m_szUsername;
	std::string m_szPassword;

	bool m_bGetInverterDetails;

	bool m_bHaveConsumption = false;
	bool m_bHaveeNetConsumption = false;
	bool m_bHaveStorage = false;

	std::shared_ptr<std::thread> m_thread;
};
