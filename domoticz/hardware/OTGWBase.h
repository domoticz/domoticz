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
	friend class OTGWSerial;
	friend class OTGWTCP;
public:
	OTGWBase(void);
	~OTGWBase(void);
	std::string m_szSerialPort;
	unsigned int m_iBaudRate;
private:
	void SetModes( const int Mode1, const int Mode2, const int Mode3, const int Mode4, const int Mode5);
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();
	void UpdateTempSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSetPointSensor(const unsigned char Idx, const float Temp, const std::string &defaultname);
	void UpdateSwitch(const unsigned char Idx, const bool bOn, const std::string &defaultname);
	bool GetOutsideTemperatureFromDomoticz(float &tvalue);
	static const int readBufferSize=1028;
	unsigned char m_buffer[readBufferSize];
	int m_bufferpos;
	int m_OutsideTemperatureIdx;
};

