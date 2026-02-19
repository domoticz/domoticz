#pragma once

#include "HardwareMonitor.h"
#include "../main/json_helper.h"

class CHardwareMonitorLHM : public CHardwareMonitorBase
{
public:
	CHardwareMonitorLHM(int ID, const std::string& address, uint16_t port, const std::string& username, const std::string& password, bool bExtendedSensors);
	~CHardwareMonitorLHM() override = default;

protected:
	void FetchData() override;

private:
	bool FetchLHMData();
	void ParseLHMNode(const Json::Value& node, const std::string& hwDevice,
	                   int& tempIdx, int& fanIdx, int& cpuLoadIdx, int& memLoadIdx,
	                   int& memDataIdx, int& storageDataIdx,
	                   int& storageTempIdx, int& gpuFanIdx,
	                   int& loadIdx, int& voltIdx, int& powerIdx, int& clockIdx,
	                   int& dataIdx, int& smallDataIdx, int& throughputIdx,
	                   int& levelIdx, int& factorIdx, int depth = 0);

	std::string m_szLHMAddress;
	uint16_t m_usLHMPort;
	std::string m_szUsername;
	std::string m_szPassword;
	bool m_bExtendedSensors;
};
