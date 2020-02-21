#pragma once

#include "../main/json_helper.h"

class CTeslaApi
{
public:
	enum eCommandType {
		Climate_Off,
		Climate_Comfort,
		Climate_Defrost,
		Climate_Defrost_Off,
		Charge_Start,
		Charge_Stop,
		Wake_Up
	};

	enum eDataType {
		Climate_State,
		Charge_State,
		Drive_State
	};

	CTeslaApi();
	~CTeslaApi();

	bool Login(const std::string username, const std::string password, const std::string vin);
	bool RefreshLogin();
	bool SendCommand(eCommandType command);
	bool GetData(eDataType datatype, Json::Value& reply);

	std::string m_carname;
private:
	enum eApiMethod {
		Post,
		Get
	};
	bool GetData(std::string datatype, Json::Value& reply);
	bool SendCommand(std::string command, Json::Value& reply, std::string parameters = "");
	bool GetCarInfo(const std::string vin);
	bool GetAuthToken(const std::string username, const std::string password, const bool refreshUsingToken = false);
	bool SendToApi(const eApiMethod eMethod, const std::string& sUrl, const std::string& sPostData, std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse, const bool bSendAuthHeaders = true);

	std::string m_authtoken;
	std::string m_refreshtoken;
	int64_t m_carid;
};

