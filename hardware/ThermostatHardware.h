#pragma once

#include <string>
#include <map>

#include "DomoticzHardware.h"

class CThermostatHardware : public CDomoticzHardwareBase
{
private:

	//thermostat available mode in string "OFF,ECO,COMFORT,AUTO," separated by comma, 
	// and in the order from 0 to N and 0 = OFF 1 = ECO, 2 = COMFORT 3 = AUTO
	std::string m_AvailableMode;

public:

	//thermostat function
	//return the thermostat available mode in string separated by comma 
	virtual std::string& GetAvailableMode() { return m_AvailableMode; };
	virtual void SetAvailableMode(const std::string& pAvailableMode) { m_AvailableMode = pAvailableMode; };
	//return the thermostat mode 
	virtual std::string GetCurrentMode(const std::string& devIdx) = 0;
	//return the thermostat room temperature 
	virtual std::string GetRoomTemperature(const std::string& devIdx) = 0;
	//return the thermostat current setpoint 
	virtual std::string GetSetPoint(const std::string& devIdx) = 0;
	//set the thermostat mode : newState = integer thermostat mode , in the order in AvailableMode
	virtual bool SetThermostatState(const std::string& devIdx, const unsigned int newState) = 0;

	//convert thermostat string state to int state : OFF-->0  ECO-->1
	unsigned int  ThermostatModeStringToInt(const std::string& mode)
	{
		std::vector<std::string> ModeStr;
		StringSplit(m_AvailableMode, ",", ModeStr);
		return (std::find(ModeStr.begin(), ModeStr.end(), mode) - ModeStr.begin()) % ModeStr.size();
	}
	//convert integer thermostat state to string state : 0--> OFF 1-->ECO
	std::string   ThermostatModeIntToString(unsigned int Mode)
	{
		std::vector<std::string> ModeStr;
		StringSplit(m_AvailableMode, ",", ModeStr);
		if (Mode < ModeStr.size())
			return ModeStr[Mode];
		else
			return std::string("UNKNOWN");

	}


};

