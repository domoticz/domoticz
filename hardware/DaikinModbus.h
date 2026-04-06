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

#pragma once

#include "DomoticzHardware.h"
#include "ASyncTCP.h"
#include <atomic>

class CDaikinModbus : public CDomoticzHardwareBase, ASyncTCP
{
public:
	CDaikinModbus(int ID, const std::string& IPAddress, unsigned short usIPPort, int iPollInterval, bool bIsAirToAir, int iUnitID);
	~CDaikinModbus() override;

	bool StartHardware() override;
	bool StopHardware() override;
	bool WriteToHardware(const char* pdata, unsigned char length) override;

private:
	void Do_Work();

	void SendReadRegisters(uint8_t function_code, uint16_t start_reg, uint16_t count);
	void SendWriteRegister(uint16_t reg, uint16_t value);
	void ProcessInputRegisters(const uint8_t* pData, size_t length);
	void ProcessHoldingRegisters(const uint8_t* pData, size_t length);

	// ASyncTCP callbacks
	void OnConnect() override;
	void OnDisconnect() override;
	void OnData(const uint8_t* pData, size_t length) override;
	void OnError(const boost::system::error_code& error) override;

	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	int m_iPollInterval;
	bool m_bIsAirToAir;
	int m_iUnitID;
	std::atomic<uint16_t> m_TransactionID;
	std::vector<uint8_t> m_vRBuffer;
	std::mutex m_rbufferMutex;
	std::shared_ptr<std::thread> m_thread;
	double m_dTotalEnergyWh;
	std::chrono::time_point<std::chrono::steady_clock> m_tLastEnergyUpdate;
};
