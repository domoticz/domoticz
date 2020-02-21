#include "stdafx.h"
#include "TeslaApi.h"
#include "../main/Logger.h"
#include "../httpclient/UrlEncode.h"
#include "../httpclient/HTTPClient.h"
#include "../main/json_helper.h"
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

#define TLAPITIMEOUT (30)

CTeslaApi::CTeslaApi()
{
	m_authtoken = "";
	m_refreshtoken = "";
	m_carid = 0;
	m_carname = "";
}

CTeslaApi::~CTeslaApi()
{
}

bool CTeslaApi::Login(const std::string username, const std::string password, const std::string vinnr)
{
	_log.Log(LOG_NORM, "TeslaApi: Attempting login.");
	if (GetAuthToken(username, password, false))
	{
		if(GetCarInfo(vinnr))
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
	if (GetAuthToken("", "", true))
	{	
		_log.Log(LOG_NORM, "TeslaApi: Refresh successful.");
		return true;
	}

	_log.Log(LOG_ERROR, "TeslaApi: Failed to refresh login credentials.");
	m_authtoken = "";
	m_refreshtoken = "";
	return false;
}

bool CTeslaApi::GetData(eDataType datatype, Json::Value& reply)
{
	std::string data;
	switch (datatype)
	{
	case Climate_State:
		data = "climate_state";
		break;
	case Charge_State:
		data = "charge_state";
		break;
	case Drive_State:
		data = "drive_state";
		break;
	}

	return(GetData(data, reply));
}

bool CTeslaApi::GetData(std::string datatype, Json::Value& reply)
{
	std::stringstream ss;
	ss << TESLA_URL << TESLA_API << "/" << m_carid << TESLA_API_REQUEST << datatype;
	std::string _sUrl = ss.str();
	std::string _sResponse;

	if (!SendToApi(Get, _sUrl, "", _sResponse, *(new std::vector<std::string>()), reply, true))
	{
		_log.Log(LOG_ERROR, "TeslaApi: Failed to get data %s.", datatype.c_str());
		return false;
	}

	//		_log.Log(LOG_NORM, "TeslaApi: Get data %s received reply: %s", datatype.c_str(), _sResponse.c_str());
	_log.Debug(DEBUG_NORM, "TeslaApi: Get data %s received reply: %s", datatype.c_str(), _sResponse.c_str());

	return true;
}

bool CTeslaApi::GetCarInfo(const std::string vinnr)
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
		if (_jsRoot["response"][i]["vin"].asString() == vinnr)
		{
			m_carid = _jsRoot["response"][i]["id"].asInt64();
			m_carname = _jsRoot["response"][i]["display_name"].asString();
			car_found = true;
			_log.Log(LOG_NORM, "TeslaApi: Car found in account: ID %lld VIN %s NAME %s", m_carid, vinnr.c_str(), m_carname.c_str());
			return true;
		}
	}

	_log.Log(LOG_ERROR, "TeslaApi: Car with VIN number %s NOT found in account.", vinnr.c_str());
	return car_found;
}

bool CTeslaApi::SendCommand(eCommandType command)
{
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
	case Climate_Comfort:
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


bool CTeslaApi::SendCommand(std::string command, Json::Value& reply, std::string parameters)
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

	//	_log.Log(LOG_NORM, "TeslaApi: Command %s received reply: %s", GetCommandString(command).c_str(), _sResponse.c_str());
	_log.Debug(DEBUG_NORM, "TeslaApi: Command %s received reply: %s", command.c_str(), _sResponse.c_str());

	return true;
}

// Requests an authentication token from the Tesla OAuth Api.
bool CTeslaApi::GetAuthToken(const std::string username, const std::string password, const bool refreshUsingToken)
{
	if (username.size() == 0 && !refreshUsingToken)
	{
		_log.Log(LOG_ERROR, "TeslaApi: No username specified.");
		return false;
	}
	if (username.size() == 0 && !refreshUsingToken)
	{
		_log.Log(LOG_ERROR, "TeslaApi: No password specified.");
		return false;
	}

	std::stringstream ss;
	ss << TESLA_URL << TESLA_API_AUTH;
	std::string _sUrl = ss.str();
	std::ostringstream s;
	std::string _sGrantType = (refreshUsingToken ? "refresh_token" : "password");

	s << "client_id=" << TESLA_CLIENT_ID << "&grant_type=";
	s << _sGrantType << "&client_secret=";
	s << TESLA_CLIENT_SECRET;

	if (refreshUsingToken)
	{
		s << "&refresh_token=" << m_refreshtoken;
	}
	else
	{
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
	if (m_authtoken.size() == 0)
	{
		_log.Log(LOG_ERROR, "TeslaApi: Received token is zero length.");
		return false;
	}

	m_refreshtoken = _jsRoot["refresh_token"].asString();
	if (m_refreshtoken.size() == 0)
	{
		_log.Log(LOG_ERROR, "TeslaApi: Received refresh token is zero length.");
		return false;
	}

	_log.Debug(DEBUG_NORM, "TeslaApi: Received access token from API.");

	return true;
}

// Sends a request to the Tesla API.
bool CTeslaApi::SendToApi(const eApiMethod eMethod, const std::string& sUrl, const std::string& sPostData,
	std::string& sResponse, const std::vector<std::string>& vExtraHeaders, Json::Value& jsDecodedResponse, const bool bSendAuthHeaders)
{
	try 
	{
		// If there is no token stored then there is no point in doing a request. Unless we specifically
		// decide not to do authentication.
		if (m_authtoken.size() == 0 && bSendAuthHeaders) 
		{
			_log.Log(LOG_ERROR, "TeslaApi: No access token available.");
			return false;
		}

		// Prepare the headers. Copy supplied vector.
		std::vector<std::string> _vExtraHeaders = vExtraHeaders;

		// If the supplied postdata validates as json, add an appropriate content type header
		if (sPostData.size() > 0)
			if (ParseJSon(sPostData, *(new Json::Value))) 
				_vExtraHeaders.push_back("Content-Type: application/json");

		// Prepare the authentication headers if requested.
		if (bSendAuthHeaders) 
			_vExtraHeaders.push_back("Authorization: Bearer " + m_authtoken);

		// Increase default timeout, tesla is slow
		HTTPClient::SetConnectionTimeout(TLAPITIMEOUT);
		HTTPClient::SetTimeout(TLAPITIMEOUT);

		std::vector<std::string> _vResponseHeaders;
		std::stringstream _ssResponseHeaderString;

		switch (eMethod)
		{
		case Post:
			if (!HTTPClient::POST(sUrl, sPostData, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				for (unsigned int i = 0; i < _vResponseHeaders.size(); i++) 
					_ssResponseHeaderString << _vResponseHeaders[i];
				_log.Debug(DEBUG_NORM, "TeslaApi: Failed to perform POST request to Api: %s; Response headers: %s", sResponse.c_str(), _ssResponseHeaderString.str().c_str());
				return false;
			}
			break;

		case Get:
			if (!HTTPClient::GET(sUrl, _vExtraHeaders, sResponse, _vResponseHeaders))
			{
				for (unsigned int i = 0; i < _vResponseHeaders.size(); i++) 
					_ssResponseHeaderString << _vResponseHeaders[i];
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

		if (sResponse.size() == 0)
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
