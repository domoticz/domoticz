#include "stdafx.h"
#include "WOL.h"
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/HTMLSanitizer.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"
#include "../main/WebServer.h"
#include "../main/mainworker.h"
#include "../webserver/cWebem.h"

namespace Json
{
	class Value;
} // namespace Json

CWOL::CWOL(const int ID, const std::string &BroadcastAddress, const unsigned short Port) :
	m_broadcast_address(BroadcastAddress)
{
	m_HwdID = ID;
	m_bSkipReceiveCheck = true;
	m_wol_port = Port;//9;
}

CWOL::~CWOL()
{
	m_bIsStarted = false;
}

void CWOL::Init()
{
}

bool CWOL::StartHardware()
{
	RequestStart();

	Init();
	m_bIsStarted = true;
	sOnConnected(this);

	StartHeartbeatThread();
	Log(LOG_STATUS, "Started");

	return true;
}

bool CWOL::StopHardware()
{
	StopHeartbeatThread();
	m_bIsStarted = false;
	return true;
}

//6 * 255 or(0xff)
//16 * MAC Address of target PC
std::vector<unsigned char> GenerateWOLPacket(const std::string &MACAddress)
{
	std::vector<unsigned char> packet;

	std::vector<unsigned char> mac;

	std::string hex;

	for (char c : MACAddress) {
		if (c == ':' || c == '-' || c == ' ') {
			if (hex.size() > 0) {
				mac.push_back(static_cast<unsigned char>(std::stoul(hex, nullptr, 16)));
				hex.clear();
			}
		}
		else if (isxdigit(c)) {
			hex += c;
			// Process every 2 hex digits
			if (hex.size() == 2) {
				mac.push_back(static_cast<unsigned char>(std::stoul(hex, nullptr, 16)));
				hex.clear();
			}
		}
	}
	if (hex.size() > 0) {
		mac.push_back(static_cast<unsigned char>(std::stoul(hex, nullptr, 16)));
	}

	if (mac.size() != 6)
		return packet; //invalid AMC address

	// Add 6 bytes of 0xFF
	for (int i = 0; i < 6; i++) {
		packet.push_back(0xFF);
	}

	// Add MAC address 16 times
	for (int i = 0; i < 16; i++) {
		packet.insert(packet.end(), mac.begin(), mac.end());
	}

	return packet;
}

// MAC Address could be (AA:BB:CC:DD:EE:FF) or dashed(AA-BB-CC-DD-EE-FF)
// Broadcast/Multicast Support:
//  IPv4: Uses broadcast(e.g., 255.255.255.255 or 192.168.0.255)
//  IPv6: Uses multicast addresses(e.g., ff02::1 for link-local all-nodes)
bool CWOL::SendWOLPacket(const std::vector<unsigned char>& magic_packet)
{
	try
	{
		// Determine target address
		std::string targetAddr = m_broadcast_address.empty() ? "255.255.255.255" : m_broadcast_address;

		// Setup address hints for getaddrinfo
		addrinfo hints = {};
		hints.ai_family = AF_UNSPEC;      // Support both IPv4 and IPv6
		hints.ai_socktype = SOCK_DGRAM;   // UDP
		hints.ai_protocol = IPPROTO_UDP;

		addrinfo* result = nullptr;
		std::string portStr = std::to_string(m_wol_port);

		if (getaddrinfo(targetAddr.c_str(), portStr.c_str(), &hints, &result) != 0) {
			return false;
		}

		bool success = false;

		// Try to send using the resolved address
		for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
			// Create socket
			SOCKET sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (sock == INVALID_SOCKET) {
				continue;
			}

			if (ptr->ai_family == AF_INET) { // For IPv4 multicast
				
				// Enable broadcast
				int broadcastEnable = 1;
				setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
					(const char*)&broadcastEnable, sizeof(broadcastEnable));
			}
			else if (ptr->ai_family == AF_INET6) { // For IPv6 multicast
				// Set multicast hop limit
				int hopLimit = 1;
				setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
					(const char*)&hopLimit, sizeof(hopLimit));
			}

			// Send the packet
			int bytesSent = sendto(sock, (const char*)magic_packet.data(),
				static_cast<int>(magic_packet.size()), 0,
				ptr->ai_addr, static_cast<int>(ptr->ai_addrlen));

			if (bytesSent > 0) {
				success = true;
			}

			closesocket(sock);

			if (success) break;
		}

		freeaddrinfo(result);

		return success;
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "WOL Exception: %s", e.what());
	}
	return false;
}

bool CWOL::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype=pSen->ICMND.subtype;

	if (packettype != pTypeLighting2)
		return false;

	if (pSen->LIGHTING2.cmnd != light2_sOn) // only send WOL with ON command
		return true;

	int nodeID = (pSen->LIGHTING2.id3 << 8) | pSen->LIGHTING2.id4;

	std::vector<std::vector<std::string> > result;

	//Find our Node
	result = m_sql.safe_query("SELECT MacAddress FROM WOLNodes WHERE (ID==%d)", nodeID);
	if (result.empty())
		return false; //Not Found

	std::string mac_address = result[0][0];

	std::vector<unsigned char> magic_packet = GenerateWOLPacket(mac_address);

	if (magic_packet.empty())
	{
		Log(LOG_ERROR, "Error creating magic packet (Check for valid mac-address)");
		return false;
	}

	if (SendWOLPacket(magic_packet))
	{
		Log(LOG_STATUS, "Wake-up send to: %s", mac_address.c_str());
	}
	else
	{
		Log(LOG_ERROR, "Error sending notification to: %s", mac_address.c_str());
		return false;
	}
	return true;
}

void CWOL::AddNode(const std::string &Name, const std::string &MACAddress)
{
	m_sql.AllowNewHardwareTimer(5);

	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')",
		m_HwdID, Name.c_str(), MACAddress.c_str());
	if (!result.empty())
		return; //Already exists
	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (%d,'%q','%q')",
		m_HwdID, Name.c_str(), MACAddress.c_str());

	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')",
		m_HwdID, Name.c_str(), MACAddress.c_str());
	if (result.empty())
		return;

	int ID = atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also add a light (push) device
	m_sql.InsertDevice(m_HwdID, 0, szID, 1, pTypeLighting2, sTypeAC, STYPE_PushOn, 1, " ", Name, 12, 255, 1);
}

bool CWOL::UpdateNode(const int ID, const std::string &Name, const std::string &MACAddress)
{
	std::vector<std::vector<std::string> > result;

	//Check if exists
	result = m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)",
		m_HwdID, ID);
	if (result.empty())
		return false; //Not Found!?

	m_sql.safe_query("UPDATE WOLNodes SET Name='%q', MacAddress='%q' WHERE (HardwareID==%d) AND (ID==%d)",
		Name.c_str(), MACAddress.c_str(), m_HwdID, ID);

	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	//Also update Light/Switch
	m_sql.safe_query(
		"UPDATE DeviceStatus SET Name='%q' WHERE (HardwareID==%d) AND (DeviceID=='%q')",
		Name.c_str(), m_HwdID, szID);

	return true;
}

void CWOL::RemoveNode(const int ID)
{
	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d) AND (ID==%d)",
		m_HwdID, ID);

	//Also delete the switch
	char szID[40];
	sprintf(szID, "%X%02X%02X%02X", 0, 0, (ID & 0xFF00) >> 8, ID & 0xFF);

	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')",
		m_HwdID, szID);
}

void CWOL::RemoveAllNodes()
{
	m_sql.safe_query("DELETE FROM WOLNodes WHERE (HardwareID==%d)", m_HwdID);

	//Also delete the all switches
	m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d)",
		m_HwdID);
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_WOLGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == nullptr)
				return;
			if (pHardware->HwdType != HTYPE_WOL)
				return;

			root["status"] = "OK";
			root["title"] = "WOLGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID,Name,MacAddress FROM WOLNodes WHERE (HardwareID==%d)",
				iHardwareID);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Mac"] = sd[2];
					ii++;
				}
			}
		}

		void CWebServer::Cmd_WOLAddNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string mac = HTMLSanitizer::Sanitize(request::findValue(&req, "mac"));
			if ((hwid.empty()) || (name.empty()) || (mac.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = dynamic_cast<CWOL*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "WOLAddNode";
			pHardware->AddNode(name, mac);
		}

		void CWebServer::Cmd_WOLUpdateNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			std::string name = HTMLSanitizer::Sanitize(request::findValue(&req, "name"));
			std::string mac = HTMLSanitizer::Sanitize(request::findValue(&req, "mac"));
			if ((hwid.empty()) || (nodeid.empty()) || (name.empty()) || (mac.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = dynamic_cast<CWOL*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "WOLUpdateNode";
			pHardware->UpdateNode(NodeID, name, mac);
		}

		void CWebServer::Cmd_WOLRemoveNode(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			std::string nodeid = request::findValue(&req, "nodeid");
			if ((hwid.empty()) || (nodeid.empty()))
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = dynamic_cast<CWOL*>(pBaseHardware);

			int NodeID = atoi(nodeid.c_str());
			root["status"] = "OK";
			root["title"] = "WOLRemoveNode";
			pHardware->RemoveNode(NodeID);
		}

		void CWebServer::Cmd_WOLClearNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string hwid = request::findValue(&req, "idx");
			if (hwid.empty())
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
			if (pBaseHardware == nullptr)
				return;
			if (pBaseHardware->HwdType != HTYPE_WOL)
				return;
			CWOL *pHardware = dynamic_cast<CWOL*>(pBaseHardware);

			root["status"] = "OK";
			root["title"] = "WOLClearNodes";
			pHardware->RemoveAllNodes();
		}
	} // namespace server
} // namespace http
