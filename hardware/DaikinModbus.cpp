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

namespace
{
	struct TempSensorDef
	{
		uint16_t reg;
		uint8_t childId;
		const char* name;
	};

	const TempSensorDef c_TempSensors[] = {
		{ 40, 140, "Heating Leaving Water" },
		{ 41, 141, "Heating Leaving Water (Backup Heater)" },
		{ 42, 142, "Return Water Temp" },
		{ 43, 143, "Hot Water Temp" },
		{ 44, 144, "Outside Temp" },
		{ 45, 145, "Refrigerant Temp" },
		{ 50, 150, "Room Temp" },
		{ 54, 154, "Heating Min" },
		{ 55, 155, "Heating Max" },
		{ 56, 156, "Cooling Min" },
		{ 57, 157, "Cooling Max" }
	};

	struct SwitchDef
	{
		uint16_t reg;
		uint8_t childId;
		const char* name;
	};

	const SwitchDef c_InputSwitches[] = {
		{ 30, 130, "Circulation Pump" },
		{ 31, 131, "Compressor" },
		{ 32, 132, "Booster Heater" },
		{ 33, 133, "Disinfection" },
		{ 35, 135, "Defrost/Startup" },
		{ 36, 136, "Hot Start" },
		{ 37, 137, "3-way Valve" },
		{ 52, 152, "Hot Water Active" },
		{ 53, 153, "Heating/Cooling Active" }
	};

	const SwitchDef c_HoldingSwitches[] = {
		{ 4, 4, "Space Heating/Cooling" },
		{ 12, 12, "Hot Water Reheat" },
		{ 13, 13, "Hot Water Booster" },
		{ 59, 59, "Thermostat Main Input A" }
	};

	struct SetPointDef
	{
		uint16_t reg;
		uint8_t childId;
		const char* name;
	};

	const SetPointDef c_SetPoints[] = {
		{ 1, 1, "Heating Setpoint" },
		{ 2, 2, "Cooling Setpoint" },
		{ 6, 6, "Room Heating Setpoint" },
		{ 7, 7, "Room Cooling Setpoint" },
		{ 10, 10, "Hot Water Setpoint" },
		{ 54, 54, "Weather Dependent Heating Offset" },
		{ 55, 55, "Weather Dependent Cooling Offset" }
	};

	struct SelectorDef
	{
		uint16_t reg;
		uint8_t childId;
		const char* name;
		const char* levels;
	};

	const SelectorDef c_Selectors[] = {
		{ 3, 3, "Operation Mode", "Auto|Heating|Cooling" },
		{ 9, 9, "Quiet Mode", "Off|On (Auto)|On (Manual)" },
		{ 53, 53, "Weather Dependent Mode", "Fixed|Weather Dependent|Fixed+Scheduled|Weather Dependent+Scheduled" },
		{ 56, 56, "Smart Grid", "Free|Forced off|Recommended on|Forced on" }
	};

	bool IsValidDaikinRegisterValue(const int16_t value)
	{
		// Daikin EKRHH sentinel values:
		// 32767 = register unsupported
		// 32766 = register unavailable
		// 32765 = Wait for value
		return value < 32765;
	}
}

CDaikinModbus::CDaikinModbus(int ID, const std::string& IPAddress, unsigned short usIPPort, int iPollInterval, bool bIsAirToAir, int iUnitID)
	: ASyncTCP(false) // MODBUS TCP is NOT secure (no SSL)
	, m_szIPAddress(IPAddress)
	, m_iPollInterval(iPollInterval)
	, m_bIsAirToAir(bIsAirToAir)
	, m_iUnitID(iUnitID)
	, m_dTotalEnergyWh(0.0)
	, m_tLastEnergyUpdate()
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

	// Initialize CounterHelper to load the previous state from DB
	// Note: Unit is typically 1 for these auto-created Kwh meters.
	m_kWhCounter.CheckTotalCounter(this, m_iUnitID, 152, 1, 0.0);

	// Recover the raw session energy (in Wh) by subtracting the offset from the last known total
	m_dTotalEnergyWh = (m_kWhCounter.GetLastCounterValue() - m_kWhCounter.GetCounterOffset()) * 1000.0;
	if (m_dTotalEnergyWh < 0.0)
		m_dTotalEnergyWh = 0.0;
	m_tLastEnergyUpdate = std::chrono::steady_clock::now();

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
	auto lastPollTime = std::chrono::steady_clock::now() - std::chrono::seconds(m_iPollInterval); // Trigger poll immediately once connected

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

			// We reset the poll time so we poll immediately upon reconnection
			lastPollTime = std::chrono::steady_clock::now() - std::chrono::seconds(m_iPollInterval);
			continue;
		}

		iSecCounter = 0;
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPollTime).count() >= m_iPollInterval)
		{
			lastPollTime = now;
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
		uint16_t modbus_val = (uint16_t)setpoint;

		switch (childid)
		{
		case 1: modbus_reg = 1; break;
		case 2: modbus_reg = 2; break;
		case 6: modbus_reg = 6; break;
		case 7: modbus_reg = 7; break;
		case 10: modbus_reg = 10; break;
		case 54: modbus_reg = 54; break;
		case 55: modbus_reg = 55; break;
		}

		if (modbus_reg > 0)
		{
			SendWriteRegister(modbus_reg, modbus_val);
			return true;
		}
	}
	else if (prb->ICMND.packettype == pTypeGeneralSwitch)
	{
		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(prb);
		if (pSwitch->id != (int32_t)m_iUnitID) return false;

		uint16_t childid = (uint16_t)pSwitch->unitcode;
		uint16_t value = pSwitch->level;

		uint16_t modbus_reg = 0;
		uint16_t modbus_val = 0;

		switch (childid)
		{
		case 3: modbus_reg = 3; modbus_val = value / 10; break;
		case 4: modbus_reg = 4; modbus_val = (pSwitch->cmnd == gswitch_sOn) ? 1 : 0; break;
		case 9: modbus_reg = 9; modbus_val = value / 10; break;
		case 12: modbus_reg = 12; modbus_val = (pSwitch->cmnd == gswitch_sOn) ? 1 : 0; break;
		case 13: modbus_reg = 13; modbus_val = (pSwitch->cmnd == gswitch_sOn) ? 1 : 0; break;
		case 53: modbus_reg = 53; modbus_val = value / 10; break;
		case 56: modbus_reg = 56; modbus_val = value / 10; break;
		case 59: modbus_reg = 59; modbus_val = (pSwitch->cmnd == gswitch_sOn) ? 1 : 0; break;
		case 100:
			if (m_bIsAirToAir) { modbus_reg = 1000; modbus_val = value / 10; }
			break;
		}

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
	if (IsValidDaikinRegisterValue(err))
	{
		int alertLevel = (err == 0) ? 1 : 4; // 1=Green (OK), 4=Red (Error)
		std::string msg = (err == 0) ? "Unit Status: OK" : "Unit Error: " + std::to_string(err) + " (Sub: " + std::to_string(suberr) + ")";
		SendAlertSensor((m_iUnitID << 8) | 121, 255, alertLevel, msg, "Unit Status");
	}

	for (const auto& sw : c_InputSwitches)
	{
		if (IsValidDaikinRegisterValue(val = getReg(sw.reg)))
			SendSwitch(m_iUnitID, sw.childId, 255, val != 0, 0, sw.name, m_Name);
	}

	for (const auto& ts : c_TempSensors)
	{
		if (IsValidDaikinRegisterValue(val = getReg(ts.reg)))
			SendTempSensor((m_iUnitID << 8) | ts.childId, 255, val / 100.0f, ts.name);
	}

	// Sensors (49, 51)
	if (IsValidDaikinRegisterValue(val = getReg(49))) SendWaterflowSensor(m_iUnitID, 149, 255, val / 100.0f, "Flow Rate");
	if (IsValidDaikinRegisterValue(val = getReg(51)))
	{
		double dPowerWatt = (val / 100.0) * 1000.0;
		auto tNow = std::chrono::steady_clock::now();
		if (m_tLastEnergyUpdate.time_since_epoch().count() != 0)
		{
			std::chrono::duration<double> diff = tNow - m_tLastEnergyUpdate;
			double dDeltaHours = diff.count() / 3600.0;

			// Only update if time advanced and limit max interpolation time
			// to avoid massive spikes after disconnections or being powered off.
			// Using 3 times the poll interval as a safe boundary.
			double dMaxHours = (m_iPollInterval * 3.0) / 3600.0;

			if (dDeltaHours > 0)
			{
				if (dDeltaHours > dMaxHours)
					dDeltaHours = dMaxHours; // Cap the interval to prevent spikes

				m_dTotalEnergyWh += (dPowerWatt * dDeltaHours);
			}
		}
		m_tLastEnergyUpdate = tNow;

		// Note: Unit is typically 1 for these auto-created Kwh meters.
		double dTotalEnergyKWh = m_kWhCounter.CheckTotalCounter(this, m_iUnitID, 152, 1, m_dTotalEnergyWh / 1000.0);
		SendWattMeter(m_iUnitID, 151, 255, static_cast<float>(dPowerWatt), "Power Consumption");
		SendKwhMeter(m_iUnitID, 152, 255, dPowerWatt, dTotalEnergyKWh, "Energy Usage");
	}
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
		if (IsValidDaikinRegisterValue(val = getReg(1000))) SendSelectorSwitch(m_iUnitID, 100, std::to_string(val * 10), "Smart Grid Mode", 0, false, "Free|Forced off|Recommended on|Forced on", "", false, m_Name);
		if (IsValidDaikinRegisterValue(val = getReg(1001))) SendWattMeter(m_iUnitID, 101, 255, (val / 100.0f) * 1000.0f, "Power Limit");
		return;
	}

	auto getReg = [&](int reg) -> int16_t {
		int idx = (reg - 1) * 2;
		if (idx < 0 || idx + 1 >= (int)length) return 0x7FFF;
		return (int16_t)((pData[idx] << 8) | pData[idx + 1]);
	};

	int16_t val;

	for (const auto& sp : c_SetPoints)
	{
		if (IsValidDaikinRegisterValue(val = getReg(sp.reg)))
			SendSetPointSensor(m_iUnitID, 0, 0, sp.childId, 1, 255, (float)val, sp.name);
	}

	for (const auto& sw : c_HoldingSwitches)
	{
		if (IsValidDaikinRegisterValue(val = getReg(sw.reg)))
			SendSwitch(m_iUnitID, sw.childId, 255, val != 0, 0, sw.name, m_Name);
	}

	for (const auto& sel : c_Selectors)
	{
		if (IsValidDaikinRegisterValue(val = getReg(sel.reg)))
			SendSelectorSwitch(m_iUnitID, sel.childId, std::to_string(val * 10), sel.name, 0, false, sel.levels, "", false, m_Name);
	}

	if (IsValidDaikinRegisterValue(val = getReg(58))) SendWattMeter(m_iUnitID, 58, 255, (val / 100.0f) * 1000.0f, "General Power Limit");
}
