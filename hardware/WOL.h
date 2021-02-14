#pragma once

#include "DomoticzHardware.h"

class CWOL : public CDomoticzHardwareBase
{
      public:
	CWOL(int ID, const std::string &BoradcastAddress, unsigned short Port);
	~CWOL() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void AddNode(const std::string &Name, const std::string &MACAddress);
	bool UpdateNode(int ID, const std::string &Name, const std::string &MACAddress);
	void RemoveNode(int ID);
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
