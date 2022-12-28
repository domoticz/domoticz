#pragma once

#include <map>

#include "EnOceanEEP.h"
#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#include "EnOceanRawValue.h"

// 550 bytes buffer is enough for one ESP3 packet, EnOceanLink 1.8.1.0, eoPacket.h
#define ESP3_PACKET_BUFFER_SIZE 550

class CEnOceanESP3 : public enocean::CEnOceanEEP, public AsyncSerial, public CDomoticzHardwareBase
{
public:
	enum TeachinMode : uint8_t
	{
		GENERIC_NODE = 0, // WARNING : Do not change this value !!
		TEACHEDIN_NODE = 1,
		VIRTUAL_NODE = 2
	};

	struct NodeInfo
	{
		uint32_t idx;
		uint32_t nodeID;
		std::string name;
		uint16_t manufacturerID;
		uint8_t RORG;
		uint8_t func;
		uint8_t type;
		std::string description;
		TeachinMode teachin_mode;
	};

	CEnOceanESP3(int ID, const std::string &devname, int type);
	~CEnOceanESP3() override = default;

	bool WriteToHardware(const char *pdata, unsigned char length) override;

	void ResetHardware();

	void GetNodesJSON(Json::Value &root);

	void EnableLearnMode(const uint32_t minutes);
	void DisableLearnMode();
	bool IsLearnModeEnabled();
	int IsNodeTeachedInJSON(Json::Value &root);

	NodeInfo *GetNodeInfo(const uint32_t nodeID);

	void TeachInNode(const uint32_t nodeID, const uint16_t manID,
					 const uint8_t RORG, const uint8_t func, const uint8_t type,
					 const TeachinMode teachin_mode);
	void TeachInVirtualNode(const uint32_t nodeID, const uint8_t RORG, const uint8_t func, const uint8_t type);
	void CheckAndUpdateNodeRORG(NodeInfo *pNode, const uint8_t RORG);
	void UpdateNode(const uint32_t nodeID, const std::string &name, const uint16_t manID, const uint8_t RORG, const uint8_t func, const uint8_t type, const std::string &description);
	void DeleteNode(const uint32_t nodeID);
	std::string GetNodeState(const uint32_t nodeID);

	uint32_t m_id_base;
	uint32_t m_id_chip;

private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();

	void LoadNodesFromDatabase();

	std::string DumpESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	std::string DumpESP3Packet(std::string esp3packet);

	std::string FormatESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void SendESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void SendESP3PacketQueued(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);

	void ReadCallback(const char *data, size_t len);
	void ParseESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);
	void ParseERP1Packet(uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen);

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

	enum ReceiveState
	{
		ERS_SYNCBYTE = 0,
		ERS_HEADER,
		ERS_CRC8H,
		ERS_DATA,
		ERS_CRC8D
	};

	ReceiveState m_receivestate;
	uint32_t m_wantedlen;
	uint8_t m_crc;

	uint8_t m_packettype;
	uint16_t m_datalen;
	uint8_t m_optionallen;

	std::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;

	bool m_learn_mode_enabled;
	uint32_t m_last_teachedin_nodeID;
	uint32_t m_RPS_teachin_nodeID;
	uint8_t m_RPS_teachin_DATA;
	uint8_t m_RPS_teachin_STATUS;
	time_t m_RPS_teachin_timer;
	uint8_t m_RPS_teachin_count;

	// Keep track of requested blind position.
	// If the same position request is sent to the same node twice, it is translated to a "Stop" command.
	// For example:
	//	- press the button to close the blind, it starts to close
	//	- press the button again, it stops where it is

	uint32_t m_last_blind_nodeID = 0;
	uint8_t m_last_blind_position = 0xFF;
	std::string GetDbValue(const char *tableName, const char *fieldName, const char *whereFieldName, const char *whereFielValue);
	void sendVld(unsigned int sID, unsigned int destID, int channel, int value);
	void sendVld(unsigned int sID, unsigned int destID, unsigned char *data, int DataLen);
	uint32_t sendVld(unsigned int unitBaseAddr, unsigned int destID, enocean::T_DATAFIELD *OffsetDes, ...);
};
