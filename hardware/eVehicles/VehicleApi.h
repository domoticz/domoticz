#pragma once
#include <string>

class CVehicleApi
{
public:
	enum eCommandType {
		Climate_Off,
		Climate_On,
		Climate_Defrost,
		Climate_Defrost_Off,
		Charge_Start,
		Charge_Stop,
		Wake_Up
	};

	struct tLocationData {
		double latitude;
		double longitude;
		int speed;
		bool is_driving;
	};

	struct tChargeData {
		float battery_level;
		bool is_connected;
		bool is_charging;
		std::string status_string;
	};

	struct tClimateData {
		float inside_temp;
		float outside_temp;
		bool is_climate_on;
		bool is_defrost_on;
	};

	struct tAllCarData {
		tLocationData location;
		tChargeData charge;
		tClimateData climate;
	};

	virtual bool Login() = 0;
	virtual bool RefreshLogin() = 0;
	virtual bool SendCommand(eCommandType command) = 0;
	virtual bool IsAwake() = 0;
	virtual bool GetAllData(tAllCarData& data) = 0;
	virtual bool GetLocationData(tLocationData& data) = 0;
	virtual bool GetChargeData(tChargeData& data) = 0;
	virtual bool GetClimateData(tClimateData& data) = 0;
	
	virtual int GetSleepInterval() = 0;

	std::string m_carname;
};
