#pragma once

// implememtation for regulators : https://www.recalart.com/
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"

class csocket;

class MultiFun : public CDomoticzHardwareBase
{
public:
	MultiFun(const int ID, const std::string &IPAddress, const unsigned short IPPort);
	virtual ~MultiFun();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool ConnectToDevice();
	void DestroySocket();

	int SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, bool write);

	void GetTemperatures();
	void GetRegisters(bool FirstTime);
private:
	const unsigned short m_IPPort;
	const std::string m_IPAddress;
	std::shared_ptr<std::thread> m_thread;
	csocket *m_socket;
	sockaddr_in m_addr;
	std::mutex m_mutex;

	int m_LastAlarms;
	int m_LastWarnings;
	int m_LastDevices;
	int m_LastState;
	int m_LastQuickAccess;
	bool m_isSensorExists[2];
	bool m_isWeatherWork[2];
};
