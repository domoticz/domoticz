/*
 Domoticz, Open Source Home Automation System

 Copyright (C) 2012,2026 Rob Peters (GizMoCuz)

 Domoticz is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Domoticz is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Domoticz.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "DaikinModbus.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/mainworker.h"

CDaikinModbus::CDaikinModbus(int ID, const std::string& IPAddress, unsigned short usIPPort, int iPollInterval, bool bIsAirToAir, int iUnitID)
	: ASyncTCP(false) // MODBUS TCP is NOT secure (no SSL)
	, m_szIPAddress(IPAddress)
	, m_iPollInterval(iPollInterval)
	, m_bIsAirToAir(bIsAirToAir)
	, m_iUnitID(iUnitID)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	if (m_iPollInterval < 5)
		m_iPollInterval = 30;
	m_TransactionID = 0;
}

CDaikinModbus::~CDaikinModbus()
{
	StopHardware();
}

bool CDaikinModbus::StartHardware()
{
	Log(LOG_STATUS, "Initializing (ID: %d, IP: %s, Port: %d, Poll: %d, Unit: %d, Model: %s)",
		m_HwdID, m_szIPAddress.c_str(), m_usIPPort, m_iPollInterval, m_iUnitID, m_bIsAirToAir ? "Air-to-Air" : "Altherma 3");

	m_bIsStarted = true;
	this->CDomoticzHardwareBase::RequestStart();

	// Set a reasonable reconnect delay (10 seconds)
	SetReconnectDelay(10);

	// Initiate connection
	connect(m_szIPAddress, m_usIPPort);

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	sOnConnected(this);
	return true;
}

bool CDaikinModbus::StopHardware()
{
	m_bIsStarted = false;
	this->CDomoticzHardwareBase::RequestStop();
	terminate();
	if (m_thread)
	{
		if (m_thread->joinable())
			m_thread->join();
		m_thread.reset();
	}
	return true;
}

void CDaikinModbus::Do_Work()
{
	Log(LOG_STATUS, "Worker thread started...");

	int iSecCounter = 0;
	int iPollCounter = m_iPollInterval; // Trigger poll immediately once connected

	while (!this->CDomoticzHardwareBase::IsStopRequested(1000))
	{
		// Always update heartbeat to satisfy watchdog
		mytime(&m_LastHeartbeat);

		if (!m_bIsStarted)
			continue;

		if (!isConnected())
		{
			iSecCounter++;
			if (iSecCounter % 30 == 0)
				Log(LOG_STATUS, "Waiting for connection...");

			// We reset the poll counter so we poll immediately upon reconnection
			iPollCounter = m_iPollInterval;
			continue;
		}

		iSecCounter = 0;
		iPollCounter++;
		if (iPollCounter >= m_iPollInterval)
		{
			iPollCounter = 0;
			try
			{
				Debug(DEBUG_HARDWARE, "Polling registers...");
				if (m_bIsAirToAir)
				{
					// Homehub / Air-to-Air: Holding Registers 1000, count 2
					SendReadRegisters(0x03, 1000, 2);
				}
				else
				{
					// Altherma 3: Holding Registers 1-60
					SendReadRegisters(0x03, 1, 60);

					// Slight wait between blocks, but check for stop
					if (this->CDomoticzHardwareBase::IsStopRequested(200))
						break;

					// Altherma 3: Input Registers 21-61 (Start 21, count 41)
					SendReadRegisters(0x04, 21, 41);
				}
			}
			catch (const std::exception& e)
			{
				Log(LOG_ERROR, "Exception in worker thread: %s", e.what());
			}
		}
	}

	Log(LOG_STATUS, "Worker thread stopped.");
}

void CDaikinModbus::SendReadRegisters(uint8_t function_code, uint16_t start_reg, uint16_t count)
{
	uint16_t tid = ++m_TransactionID;

	Debug(DEBUG_HARDWARE, "Sending Read Request (FC: 0x%02X, Start: %u, Count: %u, TID: %u, Unit: %d)", function_code, start_reg, count, tid, m_iUnitID);

	uint8_t req[12];
	req[0] = (tid >> 8) & 0xFF;
	req[1] = tid & 0xFF;
	req[2] = 0;
	req[3] = 0;
	req[4] = 0;
	req[5] = 6;
	req[6] = (uint8_t)m_iUnitID;
	req[7] = function_code;
	req[8] = ((start_reg - 1) >> 8) & 0xFF;
	req[9] = (start_reg - 1) & 0xFF;
	req[10] = (count >> 8) & 0xFF;
	req[11] = count & 0xFF;

	write(req, sizeof(req));
}

void CDaikinModbus::SendWriteRegister(uint16_t reg, uint16_t value)
{
	uint16_t tid = ++m_TransactionID;

	Debug(DEBUG_HARDWARE, "Sending Write Request (Reg: %u, Value: %u, TID: %u, Unit: %d)", reg, value, tid, m_iUnitID);

	uint8_t req[12];
	req[0] = (tid >> 8) & 0xFF;
	req[1] = tid & 0xFF;
	req[2] = 0;
	req[3] = 0;
	req[4] = 0;
	req[5] = 6;
	req[6] = (uint8_t)m_iUnitID;
	req[7] = 0x06;
	req[8] = ((reg - 1) >> 8) & 0xFF;
	req[9] = (reg - 1) & 0xFF;
	req[10] = (value >> 8) & 0xFF;
	req[11] = value & 0xFF;

	write(req, sizeof(req));
}

bool CDaikinModbus::WriteToHardware(const char* pdata, unsigned char length)
{
	const tRBUF* prb = (const tRBUF*)pdata;

	if (prb->ICMND.packettype == pTypeSetpoint)
	{
		const _tSetpoint* pSetpoint = reinterpret_cast<const _tSetpoint*>(prb);
		if (pSetpoint->id1 != (uint8_t)m_iUnitID) return false;

		uint16_t childid = pSetpoint->id4;
		float setpoint = pSetpoint->value;

		uint16_t modbus_reg = 0;
		uint16_t modbus_val = 0;

		if (childid == 1) { modbus_reg = 1; modbus_val = (uint16_t)setpoint; }
		else if (childid == 2) { modbus_reg = 2; modbus_val = (uint16_t)setpoint; }
		else if (childid == 6) { modbus_reg = 6; modbus_val = (uint16_t)setpoint; }
		else if (childid == 7) { modbus_reg = 7; modbus_val = (uint16_t)setpoint; }
		else if (childid == 10) { modbus_reg = 10; modbus_val = (uint16_t)setpoint; }
		else if (childid == 54) { modbus_reg = 54; modbus_val = (uint16_t)setpoint; }
		else if (childid == 55) { modbus_reg = 55; modbus_val = (uint16_t)setpoint; }

		if (modbus_reg > 0)
		{
			SendWriteRegister(modbus_reg, modbus_val);
			return true;
		}
	}
	else if (prb->ICMND.packettype == pTypeGeneralSwitch)
	{
		if (prb->LIGHTING5.id != (int32_t)m_iUnitID) return false;

		uint16_t childid = prb->LIGHTING5.unitcode;
		uint16_t value = prb->LIGHTING5.level;

		uint16_t modbus_reg = 0;
		uint16_t modbus_val = 0;

		if (childid == 3) { modbus_reg = 3; modbus_val = value / 10; }
		else if (childid == 4) { modbus_reg = 4; modbus_val = (prb->LIGHTING5.cmnd == light5_sOn) ? 1 : 0; }
		else if (childid == 9) { modbus_reg = 9; modbus_val = value / 10; }
		else if (childid == 12) { modbus_reg = 12; modbus_val = (prb->LIGHTING5.cmnd == light5_sOn) ? 1 : 0; }
		else if (childid == 13) { modbus_reg = 13; modbus_val = (prb->LIGHTING5.cmnd == light5_sOn) ? 1 : 0; }
		else if (childid == 53) { modbus_reg = 53; modbus_val = value / 10; }
		else if (childid == 56) { modbus_reg = 56; modbus_val = value / 10; }
		else if (childid == 59) { modbus_reg = 59; modbus_val = (prb->LIGHTING5.cmnd == light5_sOn) ? 1 : 0; }
		else if (m_bIsAirToAir && childid == 100) { modbus_reg = 1000; modbus_val = value / 10; }

		if (modbus_reg > 0)
		{
			SendWriteRegister(modbus_reg, modbus_val);
			return true;
		}
	}

	return false;
}

void CDaikinModbus::OnConnect()
{
	Log(LOG_STATUS, "Connected to %s:%d", m_szIPAddress.c_str(), m_usIPPort);
	std::lock_guard<std::mutex> lock(m_rbufferMutex);
	m_vRBuffer.clear();
}

void CDaikinModbus::OnDisconnect()
{
	Log(LOG_STATUS, "Disconnected from %s:%d", m_szIPAddress.c_str(), m_usIPPort);
}

void CDaikinModbus::OnError(const boost::system::error_code& error)
{
	Log(LOG_ERROR, "TCP Error: %s", error.message().c_str());
}

void CDaikinModbus::OnData(const uint8_t* pData, size_t length)
{
	std::lock_guard<std::mutex> lock(m_rbufferMutex);
	m_vRBuffer.insert(m_vRBuffer.end(), pData, pData + length);

	while (m_vRBuffer.size() >= 7)
	{
		uint16_t payload_len = (m_vRBuffer[4] << 8) | m_vRBuffer[5];
		size_t total_expected = 6 + payload_len;

		if (payload_len < 2)
		{
			m_vRBuffer.erase(m_vRBuffer.begin());
			continue;
		}

		if (m_vRBuffer.size() < total_expected)
			break;

		uint8_t fc = m_vRBuffer[7];

		SetHeartbeatReceived();

		if (fc == 0x03 && payload_len >= 3)
		{
			uint8_t byte_count = m_vRBuffer[8];
			if (payload_len >= (size_t)(3 + byte_count))
				ProcessHoldingRegisters(m_vRBuffer.data() + 9, byte_count);
		}
		else if (fc == 0x04 && payload_len >= 3)
		{
			uint8_t byte_count = m_vRBuffer[8];
			if (payload_len >= (size_t)(3 + byte_count))
				ProcessInputRegisters(m_vRBuffer.data() + 9, byte_count);
		}

		m_vRBuffer.erase(m_vRBuffer.begin(), m_vRBuffer.begin() + total_expected);
	}
}

void CDaikinModbus::ProcessInputRegisters(const uint8_t* pData, size_t length)
{
	auto getReg = [&](int reg) -> int16_t {
		int idx = (reg - 21) * 2;
		if (idx < 0 || idx + 1 >= (int)length) return 0x7FFF;
		return (int16_t)((pData[idx] << 8) | pData[idx + 1]);
	};

	if (m_bIsAirToAir) return;

	int16_t val;

	// Combined Alert for Unit Error (Reg 21 and 23)
	int16_t err = getReg(21);
	int16_t suberr = getReg(23);
	if (err != 0x7FFF)
	{
		int alertLevel = (err == 0) ? 1 : 4; // 1=Green (OK), 4=Red (Error)
		std::string msg = (err == 0) ? "Unit Status: OK" : "Unit Error: " + std::to_string(err) + " (Sub: " + std::to_string(suberr) + ")";
		SendAlertSensor((m_iUnitID << 8) | 121, 255, alertLevel, msg, "Unit Status");
	}

	// Binary States (30-37, 52, 53)
	if ((val = getReg(30)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 130, 255, val != 0 ? 1 : 0, 0, "Circulation Pump", m_Name);
	if ((val = getReg(31)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 131, 255, val != 0 ? 1 : 0, 0, "Compressor", m_Name);
	if ((val = getReg(32)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 132, 255, val != 0 ? 1 : 0, 0, "Booster Heater", m_Name);
	if ((val = getReg(33)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 133, 255, val != 0 ? 1 : 0, 0, "Disinfection", m_Name);
	if ((val = getReg(35)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 135, 255, val != 0 ? 1 : 0, 0, "Defrost/Startup", m_Name);
	if ((val = getReg(36)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 136, 255, val != 0 ? 1 : 0, 0, "Hot Start", m_Name);
	if ((val = getReg(37)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 137, 255, val != 0 ? 1 : 0, 0, "3-way Valve", m_Name);
	if ((val = getReg(52)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 152, 255, val != 0 ? 1 : 0, 0, "DHW Normal Op", m_Name);
	if ((val = getReg(53)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 153, 255, val != 0 ? 1 : 0, 0, "Heating/Cooling Normal Op", m_Name);

	// Temperatures (40-45, 50) - Scale 0.01
	if ((val = getReg(40)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 140, 255, val / 100.0f, "LWT PHE");
	if ((val = getReg(41)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 141, 255, val / 100.0f, "LWT BUH");
	if ((val = getReg(42)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 142, 255, val / 100.0f, "Return Water Temp");
	if ((val = getReg(43)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 143, 255, val / 100.0f, "DHW Temp");
	if ((val = getReg(44)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 144, 255, val / 100.0f, "Outside Temp");
	if ((val = getReg(45)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 145, 255, val / 100.0f, "Refrigerant Temp");
	if ((val = getReg(50)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 150, 255, val / 100.0f, "Room Temp");

	// Limits (54-57) - Scale 0.01
	if ((val = getReg(54)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 154, 255, val / 100.0f, "LWT Heating Lower Limit");
	if ((val = getReg(55)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 155, 255, val / 100.0f, "LWT Heating Upper Limit");
	if ((val = getReg(56)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 156, 255, val / 100.0f, "LWT Cooling Lower Limit");
	if ((val = getReg(57)) != 0x7FFF && val != 32766) SendTempSensor((m_iUnitID << 8) | 157, 255, val / 100.0f, "LWT Cooling Upper Limit");

	// Sensors (49, 51)
	if ((val = getReg(49)) != 0x7FFF && val != 32766) SendWaterflowSensor(m_iUnitID, 149, 255, val / 100.0f, "Flow Rate");
	if ((val = getReg(51)) != 0x7FFF && val != 32766) SendWattMeter(m_iUnitID, 151, 255, (val / 100.0f) * 1000.0f, "Power Consumption");
}

void CDaikinModbus::ProcessHoldingRegisters(const uint8_t* pData, size_t length)
{
	if (m_bIsAirToAir)
	{
		auto getReg = [&](int reg) -> int16_t {
			int idx = (reg - 1000) * 2;
			if (idx < 0 || idx + 1 >= (int)length) return 0x7FFF;
			return (int16_t)((pData[idx] << 8) | pData[idx + 1]);
		};

		int16_t val;
		if ((val = getReg(1000)) != 0x7FFF && val != 32766) SendSelectorSwitch(m_iUnitID, 100, std::to_string(val * 10), "Smart Grid Mode", 0, false, "Free|Forced off|Recommended on|Forced on", "", false, m_Name);
		if ((val = getReg(1001)) != 0x7FFF && val != 32766) SendWattMeter(m_iUnitID, 101, 255, (val / 100.0f) * 1000.0f, "Power Limit");
		return;
	}

	auto getReg = [&](int reg) -> int16_t {
		int idx = (reg - 1) * 2;
		if (idx < 0 || idx + 1 >= (int)length) return 0x7FFF;
		return (int16_t)((pData[idx] << 8) | pData[idx + 1]);
	};

	int16_t val;
	// Setpoints (1, 2, 6, 7, 10)
	if ((val = getReg(1)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 1, 1, 255, (float)val, "LWT Heating Setpoint");
	if ((val = getReg(2)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 2, 1, 255, (float)val, "LWT Cooling Setpoint");
	if ((val = getReg(6)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 6, 1, 255, (float)val, "Room Heating Setpoint");
	if ((val = getReg(7)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 7, 1, 255, (float)val, "Room Cooling Setpoint");
	if ((val = getReg(10)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 10, 1, 255, (float)val, "DHW Reheat Setpoint");

	// Modes and Other (3, 4, 9, 12, 13, 53, 54, 55, 56, 58, 59)
	if ((val = getReg(3)) != 0x7FFF && val != 32766) SendSelectorSwitch(m_iUnitID, 3, std::to_string(val * 10), "Operation Mode", 0, false, "Auto|Heating|Cooling", "", false, m_Name);
	if ((val = getReg(4)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 4, 255, val != 0 ? 1 : 0, 0, "Space Heating/Cooling", m_Name);
	if ((val = getReg(9)) != 0x7FFF && val != 32766) SendSelectorSwitch(m_iUnitID, 9, std::to_string(val * 10), "Quiet Mode", 0, false, "Off|On (Auto)|On (Manual)", "", false, m_Name);
	if ((val = getReg(12)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 12, 255, val != 0 ? 1 : 0, 0, "DHW Reheat", m_Name);
	if ((val = getReg(13)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 13, 255, val != 0 ? 1 : 0, 0, "DHW Booster", m_Name);
	if ((val = getReg(53)) != 0x7FFF && val != 32766) SendSelectorSwitch(m_iUnitID, 53, std::to_string(val * 10), "Weather Dependent Mode", 0, false, "Fixed|Weather Dependent|Fixed+Scheduled|Weather Dependent+Scheduled", "", false, m_Name);
	if ((val = getReg(54)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 54, 1, 255, (float)val, "Weather Dependent Heating Offset");
	if ((val = getReg(55)) != 0x7FFF && val != 32766) SendSetPointSensor(m_iUnitID, 0, 0, 55, 1, 255, (float)val, "Weather Dependent Cooling Offset");
	if ((val = getReg(56)) != 0x7FFF && val != 32766) SendSelectorSwitch(m_iUnitID, 56, std::to_string(val * 10), "Smart Grid", 0, false, "Free|Forced off|Recommended on|Forced on", "", false, m_Name);
	if ((val = getReg(58)) != 0x7FFF && val != 32766) SendWattMeter(m_iUnitID, 58, 255, (val / 100.0f) * 1000.0f, "General Power Limit");
	if ((val = getReg(59)) != 0x7FFF && val != 32766) SendGeneralSwitch(m_iUnitID, 59, 255, val != 0 ? 1 : 0, 0, "Thermostat Main Input A", m_Name);
}
