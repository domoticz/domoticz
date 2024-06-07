#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define ZiBlue_READ_BUFFER_SIZE 5 * 1024

class CZiBlueBase : public CDomoticzHardwareBase
{
	friend class CZiBlueSerial;
	friend class CZiBlueTCP;
	enum _eZiBlueState
	{
		ZIBLUE_STATE_ZI_1 = 0,
		ZIBLUE_STATE_ZI_2,
		ZIBLUE_STATE_SDQ,
		ZIBLUE_STATE_QL_1,
		ZIBLUE_STATE_QL_2,
		ZIBLUE_STATE_DATA,
		ZIBLUE_STATE_DATA_ASCII
	};

	public:
	CZiBlueBase();
	~CZiBlueBase() override = default;

	virtual bool HasManualSwitchesSupport() override;
	virtual std::string GetManualSwitchesJsonConfiguration() override;
	virtual bool AddManualSwitch(const std::string Name, _eSwitchType SwitchType, int Type, const std::multimap<std::string, std::string>& Parameters) override;
	virtual bool TestManualSwitch(_eSwitchType SwitchType, int Type, const std::multimap<std::string, std::string>& Parameters) override;

	bool WriteToHardware(const char *pdata, unsigned char length) override;
	private:
	void Init();
	void OnConnected();
	void OnDisconnected();
	void ParseData(const char *data, size_t len);
	bool ParseBinary(uint8_t SDQ, const uint8_t *data, size_t len);
	bool SendSwitchInt(int ID, int switchunit, int BatteryLevel, const std::string &switchType, const std::string &switchcmd, int level, const std::string &defaultName);

	virtual bool WriteInt(const std::string &sendString) = 0;
	virtual bool WriteInt(const uint8_t *pData, size_t length) = 0;

      private:
	void AdaptManualSwitchParameters(const std::multimap<std::string, std::string>& Parameters, _eSwitchType& SwitchTypeInOut, int& LightTypeInOut,
		int& dTypeOut, int& dSubTypeOut, std::string& devIDOut, std::string& sUnitOut) const;

	bool m_bTXokay;
	std::string m_Version;

	unsigned char m_rfbuffer[ZiBlue_READ_BUFFER_SIZE];
	int m_rfbufferpos;
	int m_retrycntr;
	time_t m_LastReceivedTime;
	std::map<int, time_t> m_LastReceivedKWhMeterTime;
	std::map<int, int> m_LastReceivedKWhMeterValue;

	//
	_eZiBlueState m_State;
	uint8_t m_SDQ;
	uint8_t m_QL_1;
	uint8_t m_QL_2;
	uint16_t m_Length;
	bool m_bRFLinkOn;
};
