#include "stdafx.h"
#include "WOL.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/RFXtrx.h"

CWOL::CWOL(const int ID, const std::string &BroadcastAddress, const unsigned short Port):
m_broadcast_address(BroadcastAddress)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
#ifdef WIN32
	// Initialize Winsock
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		_log.Log(LOG_ERROR,"WOL: Error initializing Winsock!");
	}
#endif
	m_wol_port=Port;//9;
}

CWOL::~CWOL(void)
{
	m_bIsStarted=false;
#ifdef WIN32
	WSACleanup();
#endif
}

void CWOL::Init()
{
}

bool CWOL::StartHardware()
{
	Init();
	m_bIsStarted=true;
	sOnConnected(this);

	StartHeartbeatThread();
	_log.Log(LOG_STATUS,"WOL: Started");

	return true;
}

bool CWOL::StopHardware()
{
	StopHeartbeatThread();
	m_bIsStarted=false;
    return true;
}

//6 * 255 or(0xff)
//16 * MAC Address of target PC
bool GenerateWOLPacket(unsigned char *pPacket, const std::string &MACAddress)
{
	std::vector<std::string> results;
	StringSplit(MACAddress, "-", results);
	if (results.size() != 6)
	{
		StringSplit(MACAddress, ":", results);
		if (results.size() != 6)
		{
			return false;
		}
	}

	unsigned char mac[6];
	int ii;

	for (ii = 0; ii < 6; ii++)
	{
		std::stringstream SS(results[ii]);
		unsigned int c;
		SS >> std::hex >> c;
		mac[ii] = (unsigned char)c;
	}

	/** first 6 bytes of 255 **/
	for (ii = 0; ii < 6; ii++) {
		pPacket[ii] = 0xFF;
	}
	/** append it 16 times to packet **/
	for (ii = 1; ii <= 16; ii++) {
		memcpy(&pPacket[ii * 6], &mac, 6 * sizeof(unsigned char));
	}
	return true;
}

bool CWOL::SendWOLPacket(const unsigned char *pPacket)
{
	int udpSocket;
	struct sockaddr_in udpClient, udpServer;

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	// this call is what allows broadcast packets to be sent:
	int broadcast = 1;
	if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1) 
	{
		return false;
	}
	udpClient.sin_family = AF_INET;
	udpClient.sin_addr.s_addr = INADDR_ANY;
	udpClient.sin_port = 0;

	bind(udpSocket, (struct sockaddr*)&udpClient, sizeof(udpClient));

	/** …make the packet as shown above **/

	/** set server end point (the broadcast addres)**/
	udpServer.sin_family = AF_INET;
	udpServer.sin_addr.s_addr = inet_addr(m_broadcast_address.c_str());
	udpServer.sin_port = htons(m_wol_port);

	/** send the packet **/
	sendto(udpSocket, (const char*)pPacket, 102, 0, (struct sockaddr*)&udpServer, sizeof(udpServer));

	closesocket(udpSocket);

	return true;
}

bool CWOL::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pSen=(tRBUF*)pdata;

	unsigned char packettype=pSen->ICMND.packettype;
	//unsigned char subtype=pSen->ICMND.subtype;

	if (packettype!=pTypeLighting2)
		return false;

	if (pSen->LIGHTING2.cmnd != light2_sOn) // only send WOL with ON command
		return true;

	int nodeID=(pSen->LIGHTING2.id3<<8)|pSen->LIGHTING2.id4;

	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Find our Node
	szQuery << "SELECT MacAddress FROM WOLNodes WHERE (ID==" << nodeID << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false; //Not Found

	unsigned char tosend[102];
	std::string mac_address=result[0][0];
	if (!GenerateWOLPacket(tosend, mac_address))
	{
		_log.Log(LOG_ERROR,"WOL: Error creating magic packet");
		return false;
	}

	if (SendWOLPacket(tosend))
	{
		_log.Log(LOG_STATUS,"WOL: Wake-up send to: %s",mac_address.c_str());
	}
	else
	{
		_log.Log(LOG_ERROR,"WOL: Error sending notification to: %s",mac_address.c_str());
		return false;
	}
	return true;
}

void CWOL::AddNode(const std::string &Name, const std::string &MACAddress)
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << MACAddress << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()>0)
		return; //Already exists
	szQuery.clear();
	szQuery.str("");
	szQuery << "INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (" << m_HwdID << ",'" << Name << "','" << MACAddress << "')";
	m_sql.query(szQuery.str());

	szQuery.clear();
	szQuery.str("");
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (Name=='" << Name << "') AND (MacAddress=='" << MACAddress << "')";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return;

	int ID=atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	//Also add a light (push) device
	szQuery.clear();
	szQuery.str("");
	szQuery <<
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (" << m_HwdID << ",'" << szID << "'," << int(1) << "," << pTypeLighting2 << "," <<sTypeAC << "," << int(STYPE_PushOn) << ",1, 12,255,'" << Name << "',1,' ')";
	m_sql.query(szQuery.str());
}

bool CWOL::UpdateNode(const int ID, const std::string &Name, const std::string &MACAddress)
{
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	//Check if exists
	szQuery << "SELECT ID FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	result=m_sql.query(szQuery.str());
	if (result.size()<1)
		return false; //Not Found!?

	szQuery.clear();
	szQuery.str("");

	szQuery << "UPDATE WOLNodes SET Name='" << Name << "', MacAddress='" << MACAddress << "' WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	//Also update Light/Switch
	szQuery.clear();
	szQuery.str("");
	szQuery <<
		"UPDATE DeviceStatus SET Name='" << Name << "' WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());

	return true;
}

void CWOL::RemoveNode(const int ID)
{
	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ") AND (ID==" << ID << ")";
	m_sql.query(szQuery.str());

	//Also delete the switch
	szQuery.clear();
	szQuery.str("");

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szID << "')";
	m_sql.query(szQuery.str());
}

void CWOL::RemoveAllNodes()
{
	std::stringstream szQuery;
	szQuery << "DELETE FROM WOLNodes WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());

	//Also delete the all switches
	szQuery.clear();
	szQuery.str("");
	szQuery << "DELETE FROM DeviceStatus WHERE (HardwareID==" << m_HwdID << ")";
	m_sql.query(szQuery.str());
}

