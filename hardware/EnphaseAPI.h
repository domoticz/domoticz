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
	EnphaseAPI(int ID, const std::string& IPAddress, unsigned short usIPPort, int PollInterval, const bool bPollInverters, const std::string& szUsername, const std::string& szPassword, const std::string &szSiteID);
	~EnphaseAPI() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	std::string m_szSoftwareVersion;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool GetSerialSoftwareVersion();
	bool GetOwnerToken();
	bool GetInstallerToken();
	bool getProductionDetails(Json::Value& result);
	bool getGridStatus();
	bool getPowerStatus();
	bool getInverterDetails();
	bool getInventoryDetails(Json::Value& result);

	void parseProduction(const Json::Value& root);
	void parseConsumption(const Json::Value& root);
	void parseStorage(const Json::Value& root);
	void parseInventory(const Json::Value& root);
	bool SetPowerActive(const bool bActive);

	bool CheckAuthJWT(const std::string& szToken, const bool bDisplayErrors);

	bool IsItSunny();
	int getSunRiseSunSetMinutes(bool bGetSunRise);

	bool NeedToken();

	std::string MakeURL(const char* szPath);

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");
private:
	int m_poll_interval = 30;

	std::string m_szSerial;
	std::string m_szToken;
	std::string m_szTokenInstaller;
	std::string m_szIPAddress;
	std::string m_szInstallerPassword; // derived from serial number

	std::string m_szUsername;
	std::string m_szPassword;
	std::string m_szSiteID;

	bool m_bGetInverterDetails = false;

	bool m_bHaveConsumption = false;
	bool m_bHaveNetConsumption = false;
	bool m_bHaveStorage = false;

	bool m_bOldFirmware = false;

	bool m_bCheckedInventory = false;
	bool m_bHaveInventory = false;

	uint64_t m_nLastProductionCounterValue = 0;
	uint64_t m_nProductionCounterOffset = 0;

	std::shared_ptr<std::thread> m_thread;
};
