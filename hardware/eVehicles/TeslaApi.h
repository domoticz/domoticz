/************************************************************************

Tesla implementation of VehicleApi baseclass
Author: MrHobbes74 (github.com/MrHobbes74)

21/02/2020 1.0 Creation
13/03/2020 1.1 Added keep asleep support
28/04/2020 1.2 Added new devices (odometer, lock alert, max charge switch)

License: Public domain

************************************************************************/
#pragma once

#include "../../main/json_helper.h"
#include "VehicleApi.h"

class CTeslaApi: public CVehicleApi
{
public:
	CTeslaApi(const std::string username, const std::string password, const std::string vin);
	~CTeslaApi();

	bool Login() override;
	bool RefreshLogin() override;
	bool SendCommand(eCommandType command, std::string parameter) override;
	bool GetAllData(tAllCarData& data) override;
	bool GetLocationData(tLocationData& data) override;
	bool GetChargeData(tChargeData& data) override;
	bool GetClimateData(tClimateData& data) override;
	bool GetVehicleData(tVehicleData& data) override;
	bool GetCustomData(tCustomData& data) override;
	bool IsAwake() override;
private:
	enum eApiMethod {
		Post,
		Get
	};
	bool GetData(std::string datatype, Json::Value& reply);
	bool SendCommand(std::string command, Json::Value& reply, std::string parameters = "");
	bool FindCarInAccount();
	void GetLocationData(Json::Value& jsondata, tLocationData& data);
	void GetChargeData(Json::Value& jsondata, tChargeData& data);
	void GetClimateData(Json::Value& jsondata, tClimateData& data);
	void GetVehicleData(Json::Value& jsondata, tVehicleData& data);
	void GetUnitData(Json::Value& jsondata, tConfigData& data);
	bool GetAuthToken(const std::string username, const std::string password, const bool refreshUsingToken = false);
	bool SendToApi(const eApiMethod eMethod, const std::string& sUrl, const std::string& sPostData, std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse, const bool bSendAuthHeaders = true, const int timeout = 0);

	std::string m_username;
	std::string m_password;
	std::string m_VIN;

	std::string m_authtoken;
	std::string m_refreshtoken;
	uint64_t m_carid;
};

