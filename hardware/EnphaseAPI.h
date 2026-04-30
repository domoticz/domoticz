#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

// Tracks a monotonically-increasing kWh lifetime counter and guards against
// two failure modes seen on the Enphase Envoy:
//
//  1. Hard reset (firmware update / reboot): whLifetime drops to 0 and stays
//     there, then counts up from 0.  After CONFIRM_COUNT consecutive readings
//     below the expected value the offset is locked in and the true cumulative
//     total is preserved.
//
//  2. Communications glitch (token refresh): whLifetime briefly returns 0 or a
//     very small value for 1-4 poll cycles, then jumps back to the real value.
//     The pending-reset confirmation window absorbs the brief dip.  If a reset
//     is incorrectly confirmed and the very next reading recovers to > 1 % of
//     the pre-reset lifetime, the offset is reverted automatically.
//
// Net-consumption is intentionally excluded: its whLifetime can legitimately
// decrease when the house is a net exporter, so rollover logic does not apply.
struct EnphaseCounterTracker
{
	bool   initialized     = false;
	double lastGoodTotal   = 0;  // kWh: last value submitted to the DB
	double offset          = 0;  // kWh: accumulated from all confirmed resets
	int    lowReadingCount = 0;  // consecutive readings below expected
	double preResetTotal   = 0;  // kWh: lastGoodTotal when the drop was first seen
	bool   justConfirmed   = false; // true for one cycle after a reset is confirmed
};

class EnphaseAPI : public CDomoticzHardwareBase
{
public:
	EnphaseAPI(
		int ID,
		const std::string& IPAddress,
		unsigned short usIPPort,
		int PollInterval,
		const bool bPollInverters,
		const bool iInverterDetails,
		const bool bDontGetMeteredValues,
		const std::string& szUsername,
		const std::string& szPassword,
		const std::string &szSiteID);
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
	bool getLivedataDetails(Json::Value& result);
	bool getDevStatusDetails(Json::Value& result);
	bool getEnsemblePowerDetails();
	bool getTariffDetails();

	void parseProduction(const Json::Value& root);
	void parseConsumption(const Json::Value& root);
	void parseInventory(const Json::Value& root);
	void parseLivedata(const Json::Value& root);
	void parseDevStatus(const Json::Value& root);

	bool SetPowerActive(const bool bActive);
	bool SetChargeFromGrid(const bool bEnable);
	bool SetPowerExportLimit(bool bEnable, float fLimitW = -1.0F);

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
	bool m_bDontGetMeteredValues = false;
	int iInverterDetailsLevel = 0;

	bool m_bHaveConsumption = false;
	bool m_bHaveNetConsumption = false;
	bool m_bHaveStorage = false;

	bool m_bOldFirmware = false;

	bool m_bCheckedInventory = false;
	bool m_bHaveInventory = false;

	bool m_bHaveDevStatus = false;

	bool m_bHaveLiveData = true;

	bool m_bCheckedEnsemblePower = false;
	bool m_bHaveEnsemblePower = false;

	bool m_bCheckedTariff = false;
	bool m_bHaveTariff = false;

	std::string m_szLastTariffData;

	bool m_bPELEnabled = false;
	float m_fPELLimitW = 3500.0F;
	float m_fPELSlewRate = 900.0F;

	EnphaseCounterTracker m_productionTracker;
	EnphaseCounterTracker m_totalConsumptionTracker;

	std::shared_ptr<std::thread> m_thread;
};
