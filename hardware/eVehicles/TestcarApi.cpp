/************************************************************************

Testcar implementation of VehicleApi baseclass
Author: MrHobbes74 (github.com/MrHobbes74)

09/02/2021 1.4 Added Testcar Class for easier testing of eVehicle framework

License: Public domain

************************************************************************/
#include "stdafx.h"
#include "TestcarApi.h"
#include "VehicleApi.h"
#include "../../main/Logger.h"
#include "../../main/Helper.h"
#include "../../httpclient/UrlEncode.h"
#include "../../httpclient/HTTPClient.h"
#include "../../webserver/Base64.h"
#include "../../main/json_helper.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

CTestcarApi::CTestcarApi(const std::string &username, const std::string &password, const std::string &extra)
{
	m_capabilities.has_battery_level = false;
	m_capabilities.has_charge_command = true;
	m_capabilities.has_climate_command = false;
	m_capabilities.has_defrost_command = false;
	m_capabilities.has_inside_temp = false;
	m_capabilities.has_outside_temp = false;
	m_capabilities.has_odo = false;
	m_capabilities.has_lock_status = false;
	m_capabilities.has_charge_limit = false;
	m_capabilities.has_custom_data = false;
	m_capabilities.seconds_to_sleep = 10;
	m_capabilities.minimum_poll_interval = 5;

	m_wakeup_counter = 0;

	m_car.charge_cable_connected = false;
	m_car.charging = false;
	m_car.is_awake = false;
	m_car.is_home = false;
	m_car.is_reachable = true;
	m_car.speed = 0;
}

bool CTestcarApi::Login()
{
	return WriteCar();
}

bool CTestcarApi::RefreshLogin()
{
	return true;
}

bool CTestcarApi::GetAllData(tAllCarData &data)
{
	if (GetLocationData(data.location) &&
		GetChargeData(data.charge) &&
		GetVehicleData(data.vehicle) &&
		GetClimateData(data.climate) &&
		GetCustomData(data.custom))
		return true;

	return false;
}

bool CTestcarApi::GetLocationData(tLocationData &data)
{
	if (ReadCar(false))
	{
		data.is_driving = (m_car.speed > 0);
		data.is_home = m_car.is_home;
		data.speed = m_car.speed;
		return true;
	}
	return false;
}

bool CTestcarApi::GetChargeData(CVehicleApi::tChargeData &data)
{
	if (ReadCar(false))
	{
		data.is_charging = m_car.charging;
		data.is_connected = m_car.charge_cable_connected;
		if (!m_car.charge_cable_connected)
			data.status_string = "Disconnected";
		else if (!m_car.charging)
			data.status_string = "Connected";
		else
			data.status_string = "Charging";

		return true;
	}
	return false;
}

bool CTestcarApi::GetClimateData(tClimateData &data)
{
	if (ReadCar(false))
	{
		return true;
	}
	return false;
}

bool CTestcarApi::GetVehicleData(tVehicleData &data)
{
	if(ReadCar(false))
	{
		return true;
	}
	return false;
}

bool CTestcarApi::GetCustomData(tCustomData &data)
{
	if (ReadCar(false))
	{
		return true;
	}
	return false;
}

bool CTestcarApi::IsAwake()
{
	if (ReadCar(true))
	{
		return (m_car.is_awake);
	}
	return false;
}

bool CTestcarApi::SendCommand(eCommandType command, std::string parameter)
{
	switch (command)
	{
		case Charge_Start:
		case Charge_Stop:
		case Climate_Off:
		case Climate_On:
		case Climate_Defrost:
		case Climate_Defrost_Off:
		case Set_Charge_Limit:
			return true;
		case Wake_Up:
			if (m_wakeup_counter == 3)
			{
				m_wakeup_counter = 0;
				m_car.is_awake = true;
				_log.Log(LOG_NORM, "TestcarApi: 'Wake_Up = %d", m_car.is_awake);
				return WriteCar();
			}
			else
			{
				m_wakeup_counter++;
				_log.Log(LOG_NORM, "TestcarApi: 'Wake_Up = %d", m_car.is_awake);
				return false;
			}
			break;
	}

	return false;

}
bool CTestcarApi::ReadCar(bool is_awake_check)
{
	const std::ifstream input_stream("testcar.txt", std::ios_base::binary);
	std::stringstream carstring;
	Json::Value car;

	carstring << input_stream.rdbuf();
	ParseJSon(carstring.str(), car);

	if (car["is_reachable"].asBool() || is_awake_check)
	{
		m_car.charge_cable_connected = car["charge_cable_connected"].asBool();
		m_car.charging = car["charging"].asBool();
		m_car.is_awake = car["is_awake"].asBool();
		m_car.is_home = car["is_home"].asBool();
		m_car.is_reachable = car["is_reachable"].asBool();
		m_car.speed = car["speed"].asInt();

		if (m_car.charging && !m_car.charge_cable_connected)
		{
			// impossible input state, correct
			m_car.charge_cable_connected = true;
			WriteCar();
		}

		if ((m_car.speed > 0) && (m_car.is_home || !m_car.is_awake || m_car.charge_cable_connected))
		{
			// impossible input state, correct
			m_car.speed = 0;
			WriteCar();
		}

		return true;
	}

	_log.Log(LOG_ERROR, "TestcarApi: Car unreachable.");

	return false;
}

bool CTestcarApi::WriteCar()
{
	std::ofstream output_stream("testcar.txt");
	std::string carstring;
	Json::Value car;

	car["charge_cable_connected"] = Json::Value(m_car.charge_cable_connected);
	car["charging"] = Json::Value(m_car.charging);
	car["is_awake"] = Json::Value(m_car.is_awake);
	car["is_home"] = Json::Value(m_car.is_home);
	car["is_reachable"] = Json::Value(m_car.is_reachable);
	car["speed"] = Json::Value(m_car.speed);

	carstring = car.toStyledString();

	output_stream << carstring;
	output_stream.close();

	return true;
}