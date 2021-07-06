#pragma once

#include <map>

#include "EnOceanEEP.h"
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

// 550 bytes buffer is enough for one ESP3 packet, EnOceanLink 1.8.1.0, eoPacket.h
#define ESP3_PACKET_BUFFER_SIZE 550

class CEnOceanESP3 : public CEnOceanEEP, public AsyncSerial, public CDomoticzHardwareBase
{
 	enum ReceiveState
	{
		ERS_SYNCBYTE = 0,
		ERS_HEADER,
		ERS_CRC8H,
		ERS_DATA,
		ERS_CRC8D
	};
	
	struct NodeInfo
	{
		int idx;
		int manufacturer;
		uint8_t profile;
		uint8_t type;
	};

     public:
	CEnOceanESP3(int ID, const std::string &devname, int type);
	~CEnOceanESP3() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SendDimmerTeachIn(const char *pdata, unsigned char length);

	uint32_t m_id_base;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();

	std::string FormatESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void SendESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void SendESP3PacketQueued(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);

	void ReadCallback(const char *data, size_t len);
	void ParseESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void ParseERP1Packet(uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);

	void LoadNodesFromDatabase();

 	const char *GetPacketTypeLabel(const uint8_t PT);
	const char *GetPacketTypeDescription(const uint8_t PT);

	const char *GetReturnCodeLabel(const uint8_t RC);
	const char *GetReturnCodeDescription(const uint8_t RC);

	const char *GetFunctionReturnCodeLabel(const uint8_t RC);
	const char *GetFunctionReturnCodeDescription(const uint8_t RC);

	const char *GetEventCodeLabel(const uint8_t EC);
	const char *GetEventCodeDescription(const uint8_t EC);

	const char *GetCommonCommandLabel(const uint8_t CC);
	const char *GetCommonCommandDescription(const uint8_t CC);

	const char *GetSmarkAckCodeLabel(const uint8_t SA);
	const char *GetSmartAckCodeDescription(const uint8_t SA);

	std::string m_szSerialPort;
	int m_Type;

	uint32_t m_retrycntr;
	std::shared_ptr<std::thread> m_thread;
	
	std::map<uint32_t, NodeInfo> m_nodes;

 	uint8_t m_buffer[ESP3_PACKET_BUFFER_SIZE];
	uint32_t m_bufferpos;

	ReceiveState m_receivestate;
	uint32_t m_wantedlen;
	uint8_t m_crc;

	uint8_t m_packettype;
	uint16_t m_datalen;
	uint8_t m_optionallen;

	bool m_wait_version_base;

	std::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;
};
