/************************************************************************

Merceded implementation of VehicleApi baseclass
Author: KidDigital (github.com/kiddigital)

24/07/2020 1.0 Creation

License: Public domain

************************************************************************/
#include "stdafx.h"
#include "MercApi.h"
#include "VehicleApi.h"
#include "../../main/Logger.h"
#include "../../httpclient/UrlEncode.h"
#include "../../httpclient/HTTPClient.h"
#include "../../main/json_helper.h"
#include "../../main/Helper.h"
#include "../../main/SQLHelper.h"
#include "../../webserver/Base64.h"
#include <sstream>
#include <iomanip>

// based on API's as described on https://developer.mercedes-benz.com/products
// Assumes that one has a registered Mercedes ME account and using the MB developer portal
// registers for the following BYOCAR (Bring Your Own CAR) API's:
// - Fuel Status
// - Pay as you drive
// - Vehicle Lock status
// - Vehicle status
// - Electric Vehicle status (even possible for non-electric/hybrid vehicles)
// so use the following 5 scope's: mb:vehicle:mbdata:vehiclestatus mb:vehicle:mbdata:fuelstatus mb:vehicle:mbdata:payasyoudrive mb:vehicle:mbdata:vehiclelock mb:vehicle:mbdata:evstatus
// and we need the additional scope to get a refresh token: offline_access
#define MERC_URL_AUTH "https://id.mercedes-benz.com"
#define MERC_API_TOKEN "/as/token.oauth2"
#define MERC_URL "https://api.mercedes-benz.com"
#define MERC_API "/vehicledata/v2/vehicles"

#define MERC_APITIMEOUT (30)
#define MERC_REFRESHTOKEN_CLEARED "Refreshtoken cleared because it was invalid!"
#define MERC_REFRESHTOKEN_USERVAR "mercedesme_refreshtoken"

CMercApi::CMercApi(const std::string &username, const std::string &password, const std::string &vinnr)
{
	m_username = username;
	m_password = base64_encode(username);
	m_VIN = vinnr;

	m_authtoken = password;
	m_accesstoken = "";
	m_refreshtoken = "";
	m_uservar_refreshtoken = MERC_REFRESHTOKEN_USERVAR;
	m_uservar_refreshtoken_idx = 0;
	m_authenticating = false;

	m_carid = 0;
	m_config.car_name = "";
	m_config.unit_miles = false;
	m_config.distance_unit = "km";

	m_crc = 0;
	m_fields = "";
	m_fieldcnt = -1;

	m_capabilities.has_battery_level = true;
	m_capabilities.has_charge_command = false;
	m_capabilities.has_climate_command = false;
	m_capabilities.has_defrost_command = false;
	m_capabilities.has_inside_temp = false;
	m_capabilities.has_outside_temp = false;
	m_capabilities.has_odo = true;
	m_capabilities.has_lock_status = true;
	m_capabilities.has_charge_limit = false;
	m_capabilities.has_custom_data = true;
	m_capabilities.sleep_interval = 0;

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", m_uservar_refreshtoken.c_str());
	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO UserVariables (Name, ValueType, Value) VALUES ('%q',%d,'%q')", m_uservar_refreshtoken.c_str(), USERVARTYPE_STRING, m_refreshtoken.c_str());
		result = m_sql.safe_query("SELECT ID, Value FROM UserVariables WHERE (Name=='%q')", m_uservar_refreshtoken.c_str());
		if (result.empty())
			return;
	}

	m_uservar_refreshtoken_idx = atoi(result[0][0].c_str());
	m_refreshtoken = result[0][1];
}

bool CMercApi::Login()
{
	bool bSuccess = false;
	std::string szLastUpdate = TimeToString(nullptr, TF_DateTime);

	if (m_refreshtoken.empty() || m_refreshtoken == MERC_REFRESHTOKEN_CLEARED)
	{
		_log.Log(LOG_NORM, "MercApi: Attempting login (using provided Authorization code).");

		m_authenticating = true;

		if (GetAuthToken(m_username, m_authtoken, false))
		{
			_log.Log(LOG_NORM, "MercApi: Login successful.");
			m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%q' WHERE (ID==%d)", m_refreshtoken.c_str(), szLastUpdate.c_str(), m_uservar_refreshtoken_idx);
			bSuccess = true;
		}
		else
		{
			_log.Log(LOG_ERROR, "MercApi: Login unsuccessful!");
		}

		m_authenticating = false;
	}
	else
	{
		_log.Log(LOG_NORM, "MercApi: Attempting (re)login (using stored refresh token!).");
		bSuccess = RefreshLogin();
	}

	return bSuccess;
}

bool CMercApi::RefreshLogin()
{
	bool bSuccess = false;
	std::string szLastUpdate = TimeToString(nullptr, TF_DateTime);

	_log.Debug(DEBUG_NORM, "MercApi: Refreshing login credentials.");
	m_authenticating = true;

	if (GetAuthToken("", "", true))
	{	
		_log.Log(LOG_NORM, "MercApi: Login credentials Refresh successful.");
		bSuccess = true;
	}
	else
	{
		_log.Log(LOG_ERROR, "MercApi: Failed to refresh login credentials.");
		m_accesstoken = "";
		if (!m_refreshtoken.empty())
		{
			m_refreshtoken = MERC_REFRESHTOKEN_CLEARED;
		}
	}

	m_authenticating = false;
	m_sql.safe_query("UPDATE UserVariables SET Value='%q', LastUpdate='%q' WHERE (ID==%d)", m_refreshtoken.c_str(), szLastUpdate.c_str(), m_uservar_refreshtoken_idx);

	return bSuccess;
}

bool CMercApi::GetAllData(tAllCarData& data)
{
	bool bSucces = false;

	bSucces = GetVehicleData(data.vehicle);
	bSucces = bSucces && GetLocationData(data.location);
	bSucces = bSucces && GetChargeData(data.charge);
	bSucces = bSucces && GetClimateData(data.climate);
	bSucces = bSucces && GetCustomData(data.custom);

	return bSucces;
}

bool CMercApi::GetLocationData(tLocationData& data)
{
	return true; // not implemented for Mercedes
	/*
	Json::Value reply;

	if (GetData("drive_state", reply))
	{
		GetLocationData(reply["response"], data);
		return true;
	}
	return false;
	*/
}

void CMercApi::GetLocationData(Json::Value& jsondata, tLocationData& data)
{
	/*
	std::string CarLatitude = jsondata["latitude"].asString();
	std::string CarLongitude = jsondata["longitude"].asString();
	data.speed = jsondata["speed"].asInt();
	data.is_driving = data.speed > 0;
	data.latitude = std::stod(CarLatitude);
	data.longitude = std::stod(CarLongitude);
	*/
}

bool CMercApi::GetChargeData(CVehicleApi::tChargeData& data)
{
	Json::Value reply;
	bool bData = false;

	if (GetData("electricvehicle", reply))
	{
		data.battery_level = 255.0f;	// Initialize at 'no reading'
		if (reply.empty())
		{
			bData = true;	// This occurs when the API call return a 204 (No Content). So everything is valid/ok, just no data
		}
		else
		{
			if (!reply.isArray())
			{
				_log.Log(LOG_ERROR, "MercApi: Unexpected reply from ElectricVehicle.");
			}
			else
			{
				GetChargeData(reply, data);
				bData = true;
			}
		}
	}
	else
	{
		if (m_httpresultcode == 403)
		{
			_log.Log(LOG_STATUS, "Access has been denied to the ElectricVehicle data!");
			bData = true;	// We should see if we can continue with the rest
		}
	}

	return bData;
}

void CMercApi::GetChargeData(Json::Value& jsondata, CVehicleApi::tChargeData& data)
{
	uint8_t cnt = 0;
	do
	{
		Json::Value iter;

		iter = jsondata[cnt];
		for (auto const& id : iter.getMemberNames())
		{
			if(!(iter[id].isNull()))
			{
				_log.Debug(DEBUG_NORM, "MercApi: Found non empty field %s", id.c_str());

				if (id == "soc")
				{
					Json::Value iter2;
					iter2 = iter[id];
					if(!iter2["value"].empty())
					{
						_log.Debug(DEBUG_NORM, "MercApi: SoC has value %s", iter2["value"].asString().c_str());
						data.battery_level = static_cast<float>(atof(iter2["value"].asString().c_str()));
					}
				}
				if (id == "rangeelectric")
				{
					Json::Value iter2;
					iter2 = iter[id];
					if(!iter2["value"].empty())
					{
						_log.Debug(DEBUG_NORM, "MercApi: RangeElectric has value %s", iter2["value"].asString().c_str());
						data.status_string = "Approximate range " + iter2["value"].asString() + " " + m_config.distance_unit;
					}
				}
			}
		}
		cnt++;
	} while (!jsondata[cnt].empty());
}

bool CMercApi::GetClimateData(tClimateData& data)
{
	return true; // not implemented for Mercedes
	/*
	Json::Value reply;

	if (GetData("climate_state", reply))
	{
		GetClimateData(reply["response"], data);
		return true;
	}
	return false;
	*/
}

void CMercApi::GetClimateData(Json::Value& jsondata, tClimateData& data)
{
	/*
	data.inside_temp = jsondata["inside_temp"].asFloat();
	data.outside_temp = jsondata["outside_temp"].asFloat();
	data.is_climate_on = jsondata["is_climate_on"].asBool();
	data.is_defrost_on = (jsondata["defrost_mode"].asInt() != 0);
	*/
}

bool CMercApi::GetVehicleData(tVehicleData& data)
{
	Json::Value reply;
	bool bData = false;

	if (GetData("vehiclelockstatus", reply))
	{
		if (reply.empty())
		{
			bData = true;	// This occurs when the API call return a 204 (No Content). So everything is valid/ok, just no data
		}
		else
		{
			if (!reply.isArray())
			{
				_log.Log(LOG_ERROR, "MercApi: Unexpected reply from VehicleLockStatus.");
			}
			else
			{
				GetVehicleData(reply, data);
				bData = true;
			}
		}
	}
	else
	{
		if (m_httpresultcode == 403)
		{
			_log.Log(LOG_STATUS, "Access has been denied to the VehicleLockStatus data!");
			bData = true;	// We should see if we can continue with the rest
		}
	}

	reply.clear();

	if (GetData("payasyoudrive", reply))
	{
		if (reply.empty())
		{
			bData = true;	// This occurs when the API call return a 204 (No Content). So everything is valid/ok, just no data
		}
		else
		{
			if (!reply.isArray())
			{
				_log.Log(LOG_ERROR, "MercApi: Unexpected reply from PayasyouDrive.");
			}
			else
			{
				GetVehicleData(reply, data);
				bData = true;
			}
		}
	}
	else
	{
		if (m_httpresultcode == 403)
		{
			_log.Log(LOG_STATUS, "Access has been denied to the PayAsYouDrive data!");
			bData = true;	// We should see if we can continue with the rest
		}
	}

	return bData;
}

bool CMercApi::GetCustomData(tCustomData& data)
{
	Json::Value reply;
	std::vector<std::string> strarray;

	if (m_capabilities.has_custom_data)
	{
		m_fieldcnt--;
		StringSplit(m_fields, ",", strarray);

		if(m_fieldcnt < 0)
		{
			m_fieldcnt = static_cast<int16_t>(strarray.size());
			_log.Debug(DEBUG_NORM, "MercApi: Reset Customfield count to %d", m_fieldcnt);
		}
		else
		{
			if (GetResourceData(strarray[m_fieldcnt], reply))
			{
				if (reply.empty())
				{
					_log.Debug(DEBUG_NORM, "MercApi: Got empty data for resource %s", strarray[m_fieldcnt].c_str());
				}
				else
				{
					_log.Debug(DEBUG_RECEIVED, "MercApi: Got data for resource %s :\n%s", strarray[m_fieldcnt].c_str(),reply.toStyledString().c_str());

					if (!reply[strarray[m_fieldcnt]].empty())
					{
						if (reply[strarray[m_fieldcnt]].isMember("value"))
						{
							std::string resourceValue = reply[strarray[m_fieldcnt]]["value"].asString();

							Json::Value customItem;
							customItem["id"] = m_fieldcnt;
							customItem["value"] = resourceValue;
							customItem["label"] = strarray[m_fieldcnt];

							data.customdata.append(customItem);

							_log.Debug(DEBUG_NORM, "MercApi: Got data for resource (%d) %s : %s", m_fieldcnt, strarray[m_fieldcnt].c_str(), resourceValue.c_str());
						}
					}
				}
			}
			else
			{
				_log.Debug(DEBUG_NORM,"MercApi: Failed to retrieve data for resource %s!", strarray[m_fieldcnt].c_str());
			}
		}
	}

	return true;
}

void CMercApi::GetVehicleData(Json::Value& jsondata, tVehicleData& data)
{
	uint8_t cnt = 0;
	do
	{
		Json::Value iter;

		iter = jsondata[cnt];
		for (auto const& id : iter.getMemberNames())
		{
			if(!(iter[id].isNull()))
			{
				_log.Debug(DEBUG_NORM, "MercApi: Found non empty field %s", id.c_str());

				if (id == "doorlockstatusvehicle")
				{
					Json::Value iter2;
					iter2 = iter[id];
					if(!iter2["value"].empty())
					{
						_log.Debug(DEBUG_NORM, "MercApi: DoorLockStatusVehicle has value %s", iter2["value"].asString().c_str());
						data.car_open = (iter2["value"].asString() == "1" || iter2["value"].asString() == "2" ? false : true);
						if(iter2["value"].asString() == "3")
						{
							data.car_open_message = "Your Mercedes is partly unlocked";
						}
						else
						{
							data.car_open_message = (data.car_open ? "Your Mercedes is open" : "Your Mercedes is locked");
						}
					}
				}
				if (id == "odo")
				{
					Json::Value iter2;
					iter2 = iter[id];
					if(!iter2["value"].empty())
					{
						_log.Debug(DEBUG_NORM, "MercApi: Odo has value %s", iter2["value"].asString().c_str());
						data.odo = static_cast<float>(atof(iter2["value"].asString().c_str()));
					}
				}
			}
		}
		cnt++;
	} while (!jsondata[cnt].empty());
}

bool CMercApi::GetData(const std::string &datatype, Json::Value &reply)
{
	std::stringstream ss;
	ss << MERC_URL << MERC_API << "/" << m_VIN << "/containers/" << datatype;
	std::string _sUrl = ss.str();
	std::string _sResponse;

	if (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), reply, true))
	{
		_log.Log(LOG_ERROR, "MercApi: Failed to get data %s.", datatype.c_str());
		return false;
	}

	_log.Debug(DEBUG_NORM, "MercApi: Get data %s received reply: %s", datatype.c_str(), _sResponse.c_str());

	return true;
}

bool CMercApi::GetResourceData(const std::string &datatype, Json::Value &reply)
{
	std::stringstream ss;
	ss << MERC_URL << MERC_API << "/" << m_VIN << "/resources/" << datatype;
	std::string _sUrl = ss.str();
	std::string _sResponse;

	if (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), reply, true, (MERC_APITIMEOUT / 2)))
	{
		_log.Log(LOG_ERROR, "MercApi: Failed to get resource data %s.", datatype.c_str());
		return false;
	}

	_log.Debug(DEBUG_NORM, "MercApi: Get resource data %s received reply: %s", datatype.c_str(), _sResponse.c_str());

	return true;
}

bool CMercApi::IsAwake()
{
	// Current Mercedes Me (API) does not have an 'Awake' state
	// So we fake one, we just request the list of all available resources that are available for the current (BYO)CAR

	std::stringstream ss;
	ss << MERC_URL << MERC_API << "/" << m_VIN << "/resources";
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
			_log.Log(LOG_ERROR, "Failed to get awake state (available resources)!");
			return false;
		}
	}

	if (!ProcessAvailableResources(_jsRoot))
	{
		_log.Log(LOG_ERROR, "Unable to process list of available resources!");
	}
	else
	{
		is_awake = true;
		_log.Debug(DEBUG_NORM, "MercApi: Awake state checked. We are awake.");
	}

	return(is_awake);
}

bool CMercApi::ProcessAvailableResources(Json::Value& jsondata)
{
	bool bProcessed = false;
	uint8_t cnt = 0;
	uint32_t crc = 0;
	std::stringstream ss;

	crc = jsondata.size();	// hm.. not easy to create something of a real crc32 of JSON content.. so for now we just compare the amount of 'keys'
	if (crc == m_crc)
	{
		_log.Debug(DEBUG_NORM, "CRC32 of content is the same.. skipping processing");
		return true;
	}
	_log.Debug(DEBUG_NORM, "CRC32 of content is the not the same (%d).. start processing", crc);

	try
	{
		do
		{
			Json::Value iter;

			iter = jsondata[cnt];
			for (auto const& id : iter.getMemberNames())
			{
				if(!(iter[id].isNull()))
				{
					Json::Value iter2;
					iter2 = iter[id];
					//_log.Debug(DEBUG_NORM, "MercApi: Field (%d) %s has value %s",cnt, id.c_str(), iter2.asString().c_str());
					if (id == "name")
					{
						if (cnt > 0)
						{
							ss << ",";
						}
						ss << iter2.asString();
					}
					if (id == "version" && iter2.asString() != "1.0")
					{
						_log.Log(LOG_STATUS, "Found resources with another version (%s) than expected 1.0! Continueing but results may be wrong!", iter2.asString().c_str());
					}
				}
			}
			cnt++;
		} while (!jsondata[cnt].empty());

		if (ss.str().length() > 0)
		{
			std::vector<std::string> strarray;

			m_fields = ss.str();
			StringSplit(m_fields, ",", strarray);
			m_fieldcnt = static_cast<int16_t>(strarray.size());

			_log.Log(LOG_STATUS, "Found %d resource fields: %s", m_fieldcnt, m_fields.c_str());

			m_crc = crc;

			bProcessed = true;
		}
		else
		{
			_log.Debug(DEBUG_NORM, "MercApi: Found %d resource fields but none called name!",cnt);
		}
	}
	catch(const std::exception& e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "MercApi: Crashed during processing of resources: %s", what.c_str());
	}

	return bProcessed;
}

bool CMercApi::SendCommand(eCommandType command, std::string parameter)
{
	return true; // Not implemented
	/*
	std::string command_string;
	Json::Value reply;
	std::string parameters = "";

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
	*/
}

bool CMercApi::SendCommand(const std::string &command, Json::Value &reply, const std::string &parameters)
{
	/*
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
		_log.Log(LOG_ERROR, "MercApi: Failed to send command %s.", command.c_str());
		return false;
	}

	//_log.Log(LOG_NORM, "MercApi: Command %s received reply: %s", command.c_str(), _sResponse.c_str());
	_log.Debug(DEBUG_NORM, "MercApi: Command %s received reply: %s", command.c_str(), _sResponse.c_str());
	*/
	return true;
}

// Requests an access token from the MB OAuth Api.
bool CMercApi::GetAuthToken(const std::string &username, const std::string &password, const bool refreshUsingToken)
{
	if (!refreshUsingToken && username.empty())
	{
		_log.Log(LOG_ERROR, "MercApi: No username specified.");
		return false;
	}
	if (!refreshUsingToken && username.empty())
	{
		_log.Log(LOG_ERROR, "MercApi: No password specified.");
		return false;
	}

	if (refreshUsingToken && (m_refreshtoken.empty() || m_refreshtoken == MERC_REFRESHTOKEN_CLEARED))
	{
		_log.Log(LOG_ERROR, "MercApi: No refresh token to perform refresh!");
		return false;
	}

	std::stringstream ss;
	ss << MERC_URL_AUTH << MERC_API_TOKEN;
	std::string _sUrl = ss.str();
	std::ostringstream s;
	std::string _sGrantType = (refreshUsingToken ? "refresh_token" : "authorization_code");

	s << "grant_type=" << _sGrantType;

	if (refreshUsingToken)
	{
		s << "&refresh_token=" << m_refreshtoken;
	}
	else
	{
		s << "&code=" << password;
		s << "&redirect_uri=https://localhost";
	}

	std::string sPostData = s.str();

	std::vector<std::string> _vExtraHeaders;
	_vExtraHeaders.push_back("Content-Type: application/x-www-form-urlencoded");
	_vExtraHeaders.push_back("Authorization: Basic " + m_password);

	std::string _sResponse;
	Json::Value _jsRoot;

	if (!SendToApi(Post, _sUrl, sPostData, _sResponse, _vExtraHeaders, _jsRoot, false))
	{
		_log.Log(LOG_ERROR, "MercApi: Failed to get token.");
		return false;
	}

	if(!_jsRoot["error"].empty()) 
	{
		_log.Log(LOG_ERROR, "MercApi: Received error response (%s).", _jsRoot["error"].asString().c_str());
		return false;
	}

	m_accesstoken = _jsRoot["access_token"].asString();
	if (m_accesstoken.empty())
	{
		_log.Log(LOG_ERROR, "MercApi: Received access token is zero length.");
		return false;
	}

	m_refreshtoken = _jsRoot["refresh_token"].asString();
	if (m_refreshtoken.empty())
	{
		_log.Log(LOG_ERROR, "MercApi: Received refresh token is zero length.");
		return false;
	}
	_log.Log(LOG_STATUS, "MercApi: Received new refresh token %s .", m_refreshtoken.c_str());
	_log.Debug(DEBUG_NORM, "MercApi: Received access token from API.");

	return true;
}

// Sends a request to the MB API.
bool CMercApi::SendToApi(const eApiMethod eMethod, const std::string& sUrl, const std::string& sPostData,
	std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse, const bool bSendAuthHeaders, const int timeout)
{
	m_httpresultcode = 0;	// Reset to 0

	// If there is no token stored then there is no point in doing a request. Unless we specifically
	// decide not to do authentication.
	if (m_accesstoken.empty() && bSendAuthHeaders)
	{
		_log.Log(LOG_ERROR, "MercApi: No access token available.");
		return false;
	}

	try
	{
		// Prepare the headers. Copy supplied vector.
		std::vector<std::string> _vExtraHeaders = vExtraHeaders;

		// If the supplied postdata validates as json, add an appropriate content type header
		if (!sPostData.empty())
			if (ParseJSon(sPostData, *(new Json::Value))) 
				_vExtraHeaders.push_back("Content-Type: application/json");

		// Prepare the authentication headers if requested.
		if (bSendAuthHeaders) 
			_vExtraHeaders.push_back("Authorization: Bearer " + m_accesstoken);

		// In/Decrease default timeout
		if(timeout == 0)
		{
			HTTPClient::SetConnectionTimeout(MERC_APITIMEOUT);
			HTTPClient::SetTimeout(MERC_APITIMEOUT);
		}
		else
		{
			HTTPClient::SetConnectionTimeout(timeout);
			HTTPClient::SetTimeout(timeout);
		}

		std::vector<std::string> _vResponseHeaders;
		std::stringstream _ssResponseHeaderString;
		uint16_t _iHttpCode;

		_log.Debug(DEBUG_RECEIVED, "MercApi: Performing request to Api: %s", sUrl.c_str());

		switch (eMethod)
		{
		case Post:
			if (!HTTPClient::POST(sUrl, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				_iHttpCode = ExtractHTTPResultCode(_vResponseHeaders[0]);
				_log.Log(LOG_ERROR, "Failed to perform POST request (%d)!", _iHttpCode);
			}
			break;

		case Get:
			if (!HTTPClient::GET(sUrl, _vExtraHeaders, sResponse, _vResponseHeaders, true))
			{
				_iHttpCode = ExtractHTTPResultCode(_vResponseHeaders[0]);
				_log.Log(LOG_ERROR, "Failed to perform GET request (%d)!", _iHttpCode);
			}
			break;

		default:
			{
				_log.Log(LOG_ERROR, "MercApi: Unknown method specified.");
				return false;
			}
		}

		m_httpresultcode = ExtractHTTPResultCode(_vResponseHeaders[0]);

		// Debug response
		for (auto &_vResponseHeader : _vResponseHeaders)
			_ssResponseHeaderString << _vResponseHeader;
		_log.Debug(DEBUG_RECEIVED, "MercApi: Performed request to Api: (%d)\n%s\nResponse headers: %s", m_httpresultcode, sResponse.c_str(), _ssResponseHeaderString.str().c_str());

		switch(m_httpresultcode)
		{
		case 28:
			_log.Log(LOG_STATUS, "Received (Curl) returncode 28.. API request response took too long, timed-out!");
			return false;
			break;
		case 200:
			break; // Ok, continue to process content
		case 204:
			_log.Log(LOG_STATUS, "Received (204) No Content.. likely because of no activity/updates in the last 12 hours!");
			return true; // OK and directly return as there is no content to actually process
			break;
		case 400:
		case 401:
			if(!m_authenticating) 
			{
				_log.Log(LOG_STATUS, "Received 400/401.. Let's try to (re)authorize again!");
				RefreshLogin();
			}
			else
			{
				_log.Log(LOG_STATUS, "Received 400/401.. During authorisation proces. Aborting!");
			}			
			return false;
			break;
		case 403:
			if(!m_authenticating)
			{
				_log.Log(LOG_STATUS, "Received 403.. Access has been denied!");
			}
			else
			{
				_log.Log(LOG_STATUS, "Received 403.. Access denied during authorisation proces. Aborting!");
			}
			return false;
			break;
		case 429:
			_log.Log(LOG_STATUS, "Received 429.. Too many request... we need to back off!");
			return false;
			break;
		case 500:
		case 503:
			_log.Log(LOG_STATUS, "Received 500/503.. Service is not available!");
			return false;
			break;
		default:
			_log.Log(LOG_STATUS, "Received unhandled HTTP returncode %d !", m_httpresultcode);
			return false;
		}

		if (sResponse.empty())
		{
			_log.Log(LOG_ERROR, "MercApi: Received an empty response from Api (HTTP %d).", m_httpresultcode);
			return false;
		}

		if (!ParseJSon(sResponse, jsDecodedResponse))
		{
			_log.Log(LOG_ERROR, "MercApi: Failed to decode Json response from Api (HTTP %d).", m_httpresultcode);
			return false;
		}
	}
	catch (std::exception & e)
	{
		std::string what = e.what();
		_log.Log(LOG_ERROR, "MercApi: Error sending information to Api: %s", what.c_str());
		return false;
	}
	return true;
}

// Interpret the return message headers to extract the HTTP resultcode
uint16_t CMercApi::ExtractHTTPResultCode(const std::string& sResponseHeaderLine0)
{
	uint16_t iHttpCode = 9999;
	uint8_t iHttpCodeStartPos = 0;

	if(!sResponseHeaderLine0.empty())
	{
		if (sResponseHeaderLine0.find("HTTP") == 0)		// Ok, so this header indeeds starts with HTTP
		{
			if (sResponseHeaderLine0.find_first_of(' ') != std::string::npos)
			{
				iHttpCodeStartPos = (uint8_t)sResponseHeaderLine0.find_first_of(' ') + 1;	// So look for a SPACE as the seperator (RFC2616)

				iHttpCode = (uint16_t)std::stoi(sResponseHeaderLine0.substr(iHttpCodeStartPos, 3));
				if (iHttpCode < 100 || iHttpCode > 599)		// Check valid resultcode range
				{
					_log.Log(LOG_STATUS, "Found non-standard resultcode (%d) in HTTP response statusline: %s", iHttpCode, sResponseHeaderLine0.c_str());
				}
			}
		}
	}

	return iHttpCode;
}
