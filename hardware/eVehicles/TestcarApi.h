/************************************************************************

Test implementation of VehicleApi baseclass
Author: MrHobbes74 (github.com/MrHobbes74)

09/02/2021 1.4 Added Testcar Class for easier testing of eVehicle framework

License: Public domain

************************************************************************/
#pragma once

#include "VehicleApi.h"

class CTestcarApi : public CVehicleApi
{
      public:
	CTestcarApi(const std::string &username, const std::string &password, const std::string &vin);
	~CTestcarApi() override = default;

	bool Login() override;
	bool RefreshLogin() override;
	bool SendCommand(eCommandType command, std::string parameter) override;
	bool GetAllData(tAllCarData &data) override;
	bool GetLocationData(tLocationData &data) override;
	bool GetChargeData(tChargeData &data) override;
	bool GetClimateData(tClimateData &data) override;
	bool GetVehicleData(tVehicleData &data) override;
	bool GetCustomData(tCustomData &data) override;
	bool IsAwake() override;

      private:

	struct tCar
	{
		bool charge_cable_connected;
		bool charging;
		bool is_home;
		int  speed;
		bool is_awake;
		bool is_reachable;
	};

	bool ReadCar(bool is_awake_check);
	bool WriteCar();

	int m_wakeup_counter;
	tCar m_car;
};
