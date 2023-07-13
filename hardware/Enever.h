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

	bool GetPriceElectricity(Json::Value& root);
	bool GetPriceGas(Json::Value& root);

	void parseElectricity(const Json::Value& root);
	void parseGas(const Json::Value& root);

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");
private:
	std::string m_szToken;
	std::string m_szProvider;

	std::string m_szCurrentElectricityPrices;
	std::string m_szCurrentGasPrices;

	std::shared_ptr<std::thread> m_thread;
};
