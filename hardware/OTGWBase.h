#pragma once

#include "DomoticzHardware.h"

class OTGWBase : public CDomoticzHardwareBase
{
	struct _tOTGWStatus
	{
		std::string MsgID;
		float Control_setpoint;
		std::string Remote_parameter_flags;
		float Maximum_relative_modulation_level;
		std::string Boiler_capacity_and_modulation_limits;
		float Room_Setpoint;
		float Relative_modulation_level;
		float CH_water_pressure;
		float Room_temperature;
		float Boiler_water_temperature;
		float DHW_temperature;
		float Outside_temperature;
		float Return_water_temperature;
		std::string DHW_setpoint_boundaries;
		std::string Max_CH_setpoint_boundaries;
		float DHW_setpoint;
		float Max_CH_water_setpoint;
		long Burner_starts;
		long CH_pump_starts;
		long DHW_pump_valve_starts;
		long DHW_burner_starts;
		long Burner_operation_hours;
		long CH_pump_operation_hours;
		long DHW_pump_valve_operation_hours;
		long DHW_burner_operation_hours;
	};
	struct _tOTGWGPIO
	{
		int A;
		int B;
	};
public:
	OTGWBase(void);
	~OTGWBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SetSetpoint(const int idx, const float temp);
	virtual bool WriteInt(const unsigned char *pData, const unsigned char Len) = 0;
	std::string m_Version;
protected:
	void SetModes(const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5, const int Mode6);
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void UpdateSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	bool SwitchLight(const int idx, const std::string &LCmd, const int svalue);
	void GetGatewayDetails();
	void GetVersion();
	void SendTime();
	void SendOutsideTemperature();
protected:
	unsigned char m_buffer[1028];
	int m_bufferpos;
	bool m_bRequestVersion;
	int m_OutsideTemperatureIdx;
	float m_OverrideTemperature;
};

