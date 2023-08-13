#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

namespace Json
{
	class Value;
} // namespace Json

class MitsubishiWF : public CDomoticzHardwareBase
{
	struct _tAircoStatus
	{
		bool Operation = false;
		double PresetTemp = 0;
		uint8_t OperationMode = 1; //cool
		uint8_t AirFlow = 1; //fan mode
		uint8_t WindDirectionUD = 2; //middle
		uint8_t WindDirectionLR = 3; //center-center
		uint8_t Entrust = 0;
		bool CoolHotJudge = false;
		uint8_t ModelNr = 1;
		bool Vacant = false;
		uint8_t code = 0;

		std::string szErrorCode;

		bool bHaveOutdoorTemp = false;
		bool bHaveIndoorTemp = false;
		bool bHaveElectric = false;

		double OutdoorTemp = 0.F;
		double IndoorTemp = 0.F;
		double Electric_kWh_Used = 0.0F;

		bool IsSelfCleanReset = false;
		bool IsSelfCleanOperation = false;
	};

public:
	MitsubishiWF(int ID, const std::string& IPAddress, unsigned short usIPPort = 51443, int PollInterval = 60);
	~MitsubishiWF() override = default;
	bool WriteToHardware(const char* pdata, unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool Execute_Command(const std::string& sCommand, std::string& sResult);
	bool Execute_Command(const std::string& sCommand, const Json::Value contents, std::string& sResult);

	void GetAircoStatus();
	bool HandleAircoStatus(const std::string& sResult);
	void TranslateAirconStat(const std::string& szStat, _tAircoStatus& aircoStatus);
	void ParseAirconStat(const _tAircoStatus& aircoStatus);

	bool SetPowerActive(const bool bActive);
	bool SetSetpoint(int idx, float temp);

	bool command_to_byte(const _tAircoStatus& aircon_stat, std::string& sResult);
	bool recieve_to_bytes(const _tAircoStatus& aircon_stat, std::string& sResult);
	bool SendAircoStatus(const _tAircoStatus& aircoStatus);

	void ParseModeSwitch(const uint8_t id, const char** vModes, const size_t cbModes, const int currentMode, const bool bButtons, std::string& sName);

	bool SetOperationMode(const int level);
	bool SetFanMode(const int level);
	bool SetSwingUpDownMode(const int level);
	bool SetSwingLeftRightMode(const int level);

	uint64_t UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
		const char* sValue, std::string& devname, bool bUseOnOffAction = true, const std::string& user = "");
private:
	int m_poll_interval = 30;

	std::string m_api_version = "1.0";
	std::string m_timezone = "Europe/Amsterdam";
	std::string m_operator_id = "4529e305-8472-456f-95f5-8cc47dfb3851";
	std::string m_device_id = "1234567890ABCDEF";
	std::string m_airconId;

	std::string m_szIPAddress;
	uint16_t m_usIPPort;

	_tAircoStatus m_AircoStatus;

	std::shared_ptr<std::thread> m_thread;
};
