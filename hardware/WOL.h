#pragma once

#include "DomoticzHardware.h"

class CWOL : public CDomoticzHardwareBase
{
public:
	CWOL(const int ID, const std::string &BoradcastAddress, const unsigned short Port);
	~CWOL(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &MACAddress);
	bool UpdateNode(const int ID, const std::string &Name, const std::string &MACAddress);
	void RemoveNode(const int ID);
	void RemoveAllNodes();
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool SendWOLPacket(const unsigned char *pPacket);
private:
	std::string m_broadcast_address;
	unsigned short m_wol_port;
};

