/************************************************************************

Merceded implementation of VehicleApi baseclass
Author: KidDigital (github.com/kiddigital)

24/07/2020 1.0 Creation

License: Public domain

************************************************************************/
#pragma once

#include "../../main/json_helper.h"
#include "VehicleApi.h"

class CMercApi: public CVehicleApi
{
public:
  CMercApi(const std::string &username, const std::string &password, const std::string &vin);
  ~CMercApi() override = default;

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
	enum eApiMethod {
		Post,
		Get
	};
	bool GetData(const std::string &datatype, Json::Value &reply);
	bool GetResourceData(const std::string &datatype, Json::Value &reply);
	bool ProcessAvailableResources(Json::Value& jsondata);
	bool SendCommand(const std::string &command, Json::Value &reply, const std::string &parameters = "");
	void GetLocationData(Json::Value& jsondata, tLocationData& data);
	void GetChargeData(Json::Value& jsondata, tChargeData& data);
	void GetClimateData(Json::Value& jsondata, tClimateData& data);
	void GetVehicleData(Json::Value& jsondata, tVehicleData& data);
	bool GetAuthToken(const std::string &username, const std::string &password, bool refreshUsingToken = false);
	bool SendToApi(eApiMethod eMethod, const std::string &sUrl, const std::string &sPostData, std::string &sResponse, const std::vector<std::string> &vExtraHeaders, Json::Value &jsDecodedResponse,
		       bool bSendAuthHeaders = true, int timeout = 0);
	uint16_t ExtractHTTPResultCode(const std::string& sResponseHeaderLine0);

	std::string m_username;
	std::string m_password;
	std::string m_VIN;

	bool m_authenticating;
	std::string m_authtoken;
	std::string m_accesstoken;
	std::string m_refreshtoken;
	std::string m_uservar_refreshtoken;
	uint64_t m_uservar_refreshtoken_idx;

	uint64_t m_carid;
	uint32_t m_crc;
	std::string m_fields;
	int16_t m_fieldcnt;
	uint16_t m_httpresultcode;
};
