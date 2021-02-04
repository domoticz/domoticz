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
		float DHW_flow_rate;
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
	OTGWBase();
	~OTGWBase() override = default;
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SetSetpoint(int idx, float temp);
	virtual bool WriteInt(const unsigned char *pData, unsigned char Len) = 0;
	std::string m_Version;

      protected:
	void SetModes(int Mode1, int Mode2, int Mode3, int Mode4, int Mode5, int Mode6);
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void UpdateSetPointSensor(unsigned char Idx, float Temp, const std::string &defaultname);
	void UpdateSwitch(unsigned char Idx, bool bOn, const std::string &defaultname);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	bool SwitchLight(int idx, const std::string &LCmd, int svalue);
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
