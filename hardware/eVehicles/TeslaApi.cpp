/************************************************************************

Tesla implementation of VehicleApi baseclass
Author: MrHobbes74 (github.com/MrHobbes74)

21/02/2020 1.0 Creation
13/03/2020 1.1 Added keep asleep support
28/04/2020 1.2 Added new devices (odometer, lock alert, max charge switch)
09/02/2021 1.4 Added Testcar Class for easier testing of eVehicle framework

License: Public domain

************************************************************************/
#include "stdafx.h"
#include "TeslaApi.h"
#include "VehicleApi.h"
#include "../../main/Logger.h"
#include "../../main/Helper.h"
#include "../../httpclient/UrlEncode.h"
#include "../../httpclient/HTTPClient.h"
#include "../../webserver/Base64.h"
#include "../../main/json_helper.h"
#include <sstream>
#include <iomanip>

// based on TESLA API as described on https://tesla-api.timdorr.com/
#define TESLA_URL "https://owner-api.teslamotors.com"
#define TESLA_API "/api/1/vehicles"
#define TESLA_API_COMMAND "/command/"
#define TESLA_API_REQUEST "/data_request/"
#define TESLA_API_AUTH "/oauth/token"

#define TESLA_CLIENT_ID "81527cff06843c8634fdc09e8ac0abefb46ac849f38fe1e431c2ef2106796384"
#define TESLA_CLIENT_SECRET "c7257eb71a564034f9419ee651c7d0e5f7aa6bfbd18bafb5c5c033b093bb2fa3"

#define TLAPITIMEOUT (25)

CTeslaApi::CTeslaApi(const std::string &username, const std::string &password, const std::string &extra)
{
	m_username = username;
	m_password = password;

	m_apikey = "";
	m_VIN = "";

	std::vector<std::string> strextra;
	StringSplit(extra, "|", strextra);
	if (strextra.size() == 1 || strextra.size() == 2)
	{
		m_VIN = strextra[0];
		if (strextra.size() >= 2)
		{
			m_apikey = base64_decode(strextra[1]);
		}
	}

	m_authtoken = "";
	m_refreshtoken = "";
	m_carid = 0;
	m_config.car_name = "";
	m_config.unit_miles = false;
	m_config.distance_unit = "km";
	m_config.home_latitude = 0;
	m_config.home_longitude = 0;

	m_capabilities.has_battery_level = true;
	m_capabilities.has_charge_command = true;
	m_capabilities.has_climate_command = true;
	m_capabilities.has_defrost_command = true;
	m_capabilities.has_inside_temp = true;
	m_capabilities.has_outside_temp = true;
	m_capabilities.has_odo = true;
	m_capabilities.has_lock_status = true;
	m_capabilities.has_charge_limit = true;
	m_capabilities.has_custom_data = false;
	m_capabilities.seconds_to_sleep = 1200;
	m_capabilities.minimum_poll_interval = 60;
}

bool CTeslaApi::Login()
{
	_log.Log(LOG_NORM, "TeslaApi: Attempting login.");
	if (GetAuthToken(m_username, m_password, false))
	{
		if(FindCarInAccount())
		{
			_log.Log(LOG_NORM, "TeslaApi: Login successful.");
			return true;
		}
	}

	_log.Log(LOG_NORM, "TeslaApi: Failed to login.");
	m_authtoken = "";
	m_refreshtoken = "";
	return false;
}

bool CTeslaApi::RefreshLogin()
{
	_log.Log(LOG_NORM, "TeslaApi: Refreshing login credentials.");
	if (GetAuthToken(m_username, m_password, true))
	{	
		_log.Log(LOG_NORM, "TeslaApi: Refresh successful.");
		return true;
	}

	_log.Log(LOG_ERROR, "TeslaApi: Failed to refresh login credentials.");
	m_authtoken = "";
	m_refreshtoken = "";
	return false;
}

bool CTeslaApi::GetAllData(tAllCarData& data)
{
	Json::Value reply;

	if (GetData("vehicle_data", reply))
	{
		GetUnitData(reply["response"]["gui_settings"], m_config);
		GetLocationData(reply["response"]["drive_state"], data.location);
		GetChargeData(reply["response"]["charge_state"], data.charge);
		GetClimateData(reply["response"]["climate_state"], data.climate);
		GetVehicleData(reply["response"]["vehicle_state"], data.vehicle);
		return true;
	}
	return false;
}

bool CTeslaApi::GetLocationData(tLocationData& data)
{
	Json::Value reply;

	if (GetData("drive_state", reply))
	{
		GetLocationData(reply["response"], data);
		return true;
	}
	return false;
}

void CTeslaApi::GetLocationData(Json::Value &jsondata, tLocationData &data)
{
	std::string CarLatitude = jsondata["latitude"].asString();
	std::string CarLongitude = jsondata["longitude"].asString();
	data.speed = jsondata["speed"].asInt();
	data.is_driving = data.speed > 0;
	data.latitude = std::stod(CarLatitude);
	data.longitude = std::stod(CarLongitude);
	if (m_config.home_latitude != 0 && m_config.home_latitude != 0)
	{
		if ((std::fabs(m_config.home_latitude - data.latitude) < 2E-4) && (std::fabs(m_config.home_longitude - data.longitude) < 2E-3) && !data.is_driving)
			data.is_home = true;
		else
			data.is_home = false;
	}
	else
	{
		data.is_home = false;
	}
}

bool CTeslaApi::GetChargeData(CVehicleApi::tChargeData& data)
{
	Json::Value reply;

	if (GetData("charge_state", reply))
	{
		GetChargeData(reply["response"], data);
		return true;
	}
	return false;
}

void CTeslaApi::GetChargeData(Json::Value& jsondata, CVehicleApi::tChargeData& data)
{
	data.battery_level = jsondata["battery_level"].asFloat();
	data.status_string = jsondata["charging_state"].asString();
	data.is_connected = (data.status_string != "Disconnected");
	data.is_charging = (data.status_string == "Charging") || (data.status_string == "Starting");
	data.charge_limit = jsondata["charge_limit_soc"].asInt();

	if(data.status_string == "Disconnected")
		data.status_string = "Charge Cable Disconnected";

	if (data.is_charging)
	{
		data.status_string.append(" (until ");
		data.status_string.append(std::to_string(data.charge_limit));
		data.status_string.append("%)");
	}
}

bool CTeslaApi::GetClimateData(tClimateData& data)
{
	Json::Value reply;

	if (GetData("climate_state", reply))
	{
		GetClimateData(reply["response"], data);
		return true;
	}
	return false;
}

void CTeslaApi::GetClimateData(Json::Value& jsondata, tClimateData& data)
{
	data.inside_temp = jsondata["inside_temp"].asFloat();
	data.outside_temp = jsondata["outside_temp"].asFloat();
	data.is_climate_on = jsondata["is_climate_on"].asBool();
	data.is_defrost_on = (jsondata["defrost_mode"].asInt() != 0);
}

bool CTeslaApi::GetVehicleData(tVehicleData& data)
{
	Json::Value reply;

	if (GetData("vehicle_state", reply))
	{
		GetVehicleData(reply["response"], data);
		return true;
	}
	return false;
}

void CTeslaApi::GetVehicleData(Json::Value& jsondata, tVehicleData& data)
{
	data.odo = (jsondata["odometer"].asFloat());
	if (!m_config.unit_miles)
		data.odo = data.odo * (float)1.60934;

	if (jsondata["df"].asBool() ||
		jsondata["pf"].asBool() ||
		jsondata["pr"].asBool() ||
		jsondata["dr"].asBool())
	{
		data.car_open = true;
		data.car_open_message = "Door open";
	}
	else if (
		jsondata["ft"].asBool() ||
		jsondata["rt"].asBool())
	{
		data.car_open = true;
		data.car_open_message = "Trunk open";
	}
	else if (
		jsondata["fd_window"].asBool() ||
		jsondata["fp_window"].asBool() ||
		jsondata["rp_window"].asBool() ||
		jsondata["rd_window"].asBool())
	{
		data.car_open = true;
		data.car_open_message = "Window open";
	}
	else if (
		!jsondata["locked"].asBool())
	{
		data.car_open = true;
		data.car_open_message = "Unlocked";
	}
	else
	{
		data.car_open = false;
		data.car_open_message = "Locked";
	}
}

bool CTeslaApi::GetCustomData(tCustomData& data)
{
	return true;
}

void CTeslaApi::GetUnitData(Json::Value& jsondata, tConfigData &config)
{
	std::string sUnit = jsondata["gui_distance_units"].asString();
	config.unit_miles = (sUnit == "mi/hr");
	if (config.unit_miles)
		config.distance_unit = "mi";
	else
		config.distance_unit = "km";

	_log.Debug(DEBUG_NORM, "TeslaApi: unit found %s, report in miles = %s.", sUnit.c_str(), config.unit_miles ? "true":"false");
}

bool CTeslaApi::GetData(const std::string &datatype, Json::Value &reply)
{
	std::stringstream ss;
	if(datatype == "vehicle_data")
		ss << TESLA_URL << TESLA_API << "/" << m_carid << "/" << datatype;
	else
		ss << TESLA_URL << TESLA_API << "/" << m_carid << TESLA_API_REQUEST << datatype;
	std::string _sUrl = ss.str();
	std::string _sResponse;

	if (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), reply, true))
	{
		_log.Log(LOG_ERROR, "TeslaApi: Failed to get data %s.", datatype.c_str());
		return false;
	}

	//_log.Log(LOG_NORM, "TeslaApi: Get data %s received reply: %s", datatype.c_str(), _sResponse.c_str());
	_log.Debug(DEBUG_NORM, "TeslaApi: Get data %s received reply: %s", datatype.c_str(), _sResponse.c_str());

	return true;
}

bool CTeslaApi::FindCarInAccount()
{
	std::stringstream ss;
	ss << TESLA_URL << TESLA_API;
	std::string _sUrl = ss.str();
	std::string _sResponse;
	Json::Value _jsRoot;
	bool car_found = false;

	if (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot, true))
	{
		_log.Log(LOG_ERROR, "TeslaApi: Failed to get car from account.");
		return false;
	}

	_log.Debug(DEBUG_NORM, "TeslaApi: Received %d vehicles from API: %s", _jsRoot["count"].asInt(), _sResponse.c_str());
	for (int i = 0; i < _jsRoot["count"].asInt(); i++)
	{
		if (_jsRoot["response"][i]["vin"].asString() == m_VIN)
		{
			m_carid = _jsRoot["response"][i]["id"].asInt64();
			m_config.car_name = _jsRoot["response"][i]["display_name"].asString();
			car_found = true;
			_log.Log(LOG_NORM, "TeslaApi: Car found in account: VIN %s NAME %s", m_VIN.c_str(), m_config.car_name.c_str());
			return true;
		}
	}

	_log.Log(LOG_ERROR, "TeslaApi: Car with VIN number %s NOT found in account.", m_VIN.c_str());
	return car_found;
}

bool CTeslaApi::IsAwake()
{
	std::stringstream ss;
	ss << TESLA_URL << TESLA_API << "/" << m_carid;
	std::string _sUrl = ss.str();
	std::string _sResponse;
	Json::Value _jsRoot;
	bool is_awake = false;
	int nr_retry = 0;

	while (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), _jsRoot, true, 10))
	{
		nr_retry++;
		if (nr_retry == 4)
		{
			_log.Log(LOG_ERROR, "TeslaApi: Failed to get awake state.");
			return false;
		}
	}

	_log.Debug(DEBUG_NORM, "Awake state: %s", _jsRoot["response"]["state"].asString().c_str());
	is_awake = (_jsRoot["response"]["state"] == "online");
	return(is_awake);
}

bool CTeslaApi::SendCommand(eCommandType command, std::string parameter)
{
	std::string command_string;
	Json::Value reply;
	std::string parameters;

	switch (command)
	{
	case Charge_Start:
		command_string = "charge_start";
		break;
	case Charge_Stop:
		command_string = "charge_stop";
		break;
	case Climate_Off:
		command_string = "auto_conditioning_stop";
		break;
	case Climate_On:
		command_string = "auto_conditioning_start";
		break;
	case Climate_Defrost:
		command_string = "set_preconditioning_max";
		parameters = "on=true";
		break;
	case Climate_Defrost_Off:
		command_string = "set_preconditioning_max";
		parameters = "on=false";
		break;
	case Wake_Up:
		command_string = "wake_up";
		break;
	case Set_Charge_Limit:
		if (parameter == "0")
			command_string = "charge_standard";
		else if(parameter == "100")
			command_string = "charge_max_range";
		else
		{
			command_string = "set_charge_limit";
			parameters = "percent=" + parameter;
		}
		break;
	}

	if (SendCommand(command_string, reply, parameters))
	{
		if (command == Wake_Up)
		{
			if (reply["response"]["state"].asString() == "online")
				return true;
		}
		else
		{
			if (reply["response"]["result"].asString() == "true")
				return true;
		}
	}

	return false;
}

bool CTeslaApi::SendCommand(const std::string &command, Json::Value &reply, const std::string &parameters)
{
	std::stringstream ss;
	if (command == "wake_up")
		ss << TESLA_URL << TESLA_API << "/" << m_carid << "/" << command;
	else
		ss << TESLA_URL << TESLA_API << "/" << m_carid << TESLA_API_COMMAND << command;

	std::string _sUrl = ss.str();
	std::string _sResponse;

	std::stringstream parss;
	parss << parameters;
	std::string sPostData = parss.str();

	if (!SendToApi(Post, _sUrl, sPostData, _sResponse, *(new std::vector<std::string>()), reply, true))
	{
		_log.Log(LOG_ERROR, "TeslaApi: Failed to send command %s.", command.c_str());
		return false;
	}

	//_log.Log(LOG_NORM, "TeslaApi: Command %s received reply: %s", command.c_str(), _sResponse.c_str());
	_log.Debug(DEBUG_NORM, "TeslaApi: Command %s received reply: %s", command.c_str(), _sResponse.c_str());

	return true;
}

// Requests an authentication token from the Tesla OAuth Api.
bool CTeslaApi::GetAuthToken(const std::string &username, const std::string &password, const bool refreshUsingToken)
{
	if (username.empty())
	{
		_log.Log(LOG_ERROR, "TeslaApi: No username specified.");
		return false;
	}
	if (username.empty())
	{
		_log.Log(LOG_ERROR, "TeslaApi: No password specified.");
		return false;
	}

	std::stringstream ss;
	ss << TESLA_URL << TESLA_API_AUTH;
	std::string _sUrl = ss.str();
	std::ostringstream s;

	s << "client_id=" << TESLA_CLIENT_ID;
	s << "&client_secret=" << TESLA_CLIENT_SECRET;

	if ((refreshUsingToken) && (!m_refreshtoken.empty()))
	{
		s << "&grant_type=" << "refresh_token";
		s << "&refresh_token=" << m_refreshtoken;
	}
	else
	{
		s << "&grant_type=" << "password";
		s << "&password=" << CURLEncode::URLEncode(password);
		s << "&email=" << CURLEncode::URLEncode(username);
	}

	std::string sPostData = s.str();

	std::string _sResponse;
	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");

	Json::Value _jsRoot;

	if (!SendToApi(Post, _sUrl, sPostData, _sResponse, _vExtraHeaders, _jsRoot, false))
	{
		_log.Log(LOG_ERROR, "TeslaApi: Failed to get token.");
		return false;
	}

	m_authtoken = _jsRoot["access_token"].asString();
	if (m_authtoken.empty())
	{
		if (m_apikey.empty())
		{
			_log.Log(LOG_ERROR, "TeslaApi: Received token is zero length.");
			return false;
		}
		_log.Log(LOG_STATUS, "TeslaApi: Cannot retrieve token from account. Using manual API key");
		m_authtoken = m_apikey;
		return true;
	}

	m_refreshtoken = _jsRoot["refresh_token"].asString();
	if (m_refreshtoken.empty())
	{
		_log.Log(LOG_ERROR, "TeslaApi: Received refresh token is zero length.");
		return false;
	}

	_log.Debug(DEBUG_NORM, "TeslaApi: Received access token from API.");

	return true;
}

// Sends a request to the Tesla API.
bool CTeslaApi::SendToApi(const eApiMethod eMethod, const std::string& sUrl, const std::string& sPostData,
	std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse, const bool bSendAuthHeaders, const int timeout)
{
	try 
	{
		// If there is no token stored then there is no point in doing a request. Unless we specifically
		// decide not to do authentication.
		if (m_authtoken.empty() && bSendAuthHeaders)
		{
			_log.Log(LOG_ERROR, "TeslaApi: No access token available.");
			return false;
		}

		// Prepare the headers. Copy supplied vector.
		std::vector<std::string> _vExtraHeaders = vExtraHeaders;

		// If the supplied postdata validates as json, add an appropriate content type header
		if (!sPostData.empty())
			if (ParseJSon(sPostData, *(new Json::Value))) 
				_vExtraHeaders.push_back("Content-Type: application/json");

		// Prepare the authentication headers if requested.
		if (bSendAuthHeaders) 
			_vExtraHeaders.push_back("Authorization: Bearer " + m_authtoken);

		// Increase default timeout, tesla is slow
		if(timeout == 0)
		{
			HTTPClient::SetConnectionTimeout(TLAPITIMEOUT);
			HTTPClient::SetTimeout(TLAPITIMEOUT);
		}
		else
		{
			HTTPClient::SetConnectionTimeout(timeout);
			HTTPClient::SetTimeout(timeout);
		}

		std::vector<std::string> _vResponseHeaders;
		std::stringstream _ssResponseHeaderString;

		switch (eMethod)
		{
		case Post:
			if (!HTTPClient::POST(sUrl, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				for (auto &_vResponseHeader : _vResponseHeaders)
				{
					if ((_vResponseHeader.find("HTTP") != std::string::npos) && (_vResponseHeader.find("401") != std::string::npos))
					{
						_log.Log(LOG_ERROR, "TeslaApi: Access token no longer valid. Clearing token.");
						m_authtoken = "";
					}
					_ssResponseHeaderString << _vResponseHeader;
				}
				_log.Debug(DEBUG_NORM, "TeslaApi: Failed to perform POST request to Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str().c_str());
				return false;
			}
			break;

		case Get:
			if (!HTTPClient::GET(sUrl, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				for (auto &_vResponseHeader : _vResponseHeaders)
				{
					if ((_vResponseHeader.find("HTTP") != std::string::npos) && (_vResponseHeader.find("401") != std::string::npos))
					{
						_log.Log(LOG_ERROR, "TeslaApi: Access token no longer valid. Clearing token.");
						m_authtoken = "";
					}
					_ssResponseHeaderString << _vResponseHeader;
				}
				_log.Debug(DEBUG_NORM, "TeslaApi: Failed to perform GET request to Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str().c_str());

				return false;
			}
			break;

		default:
		{
			_log.Log(LOG_ERROR, "TeslaApi: Unknown method specified.");
			return false;
		}
		}

		if (sResponse.empty())
		{
			_log.Log(LOG_ERROR, "TeslaApi: Received an empty response from Api.");
			return false;
		}

		if (!ParseJSon(sResponse, jsDecodedResponse))
		{
			_log.Log(LOG_ERROR, "TeslaApi: Failed to decode Json response from Api.");
			return false;
		}
	}
	catch (std::exception & e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "TeslaApi: Error sending information to Api: %s", what.c_str());
		return false;
	}
	return true;
}
