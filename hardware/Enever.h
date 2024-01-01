#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class Enever : public CDomoticzHardwareBase
{
public:
	Enever(int ID, const std::string& szToken, const std::string& szProvider);
	~Enever() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
	std::string m_szSoftwareVersion;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	std::string MakeURL(const std::string& sURL);

	bool GetPriceElectricity();
	bool GetPriceElectricity_Tomorrow();
	bool GetPriceGas(const bool bForce);

	void parseElectricity(const std::string &szElectricityData, const bool bIsToday);
	void parseGas();

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "", const bool bUseEventSystem = true);
private:
	std::string m_szToken;
	std::string m_szProviderElectricity;
	std::string m_szProviderGas;

	std::string m_szCurrentElectricityPrices;
	std::string m_szCurrentElectricityPrices_Tomorrow;
	std::string m_szCurrentGasPrices;

	std::shared_ptr<std::thread> m_thread;
};
