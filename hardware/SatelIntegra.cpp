#include <list>

#include "stdafx.h"
#include "SatelIntegra.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#ifdef _DEBUG
//#define DEBUG_SatelIntegra
#endif

#define SATEL_POLL_INTERVAL 5

extern CSQLHelper m_sql;

typedef struct tModel {
	unsigned int id;
	const char* name;
	unsigned int zones;
	unsigned int outputs;
} Model;

#define TOT_MODELS 9

static Model models[TOT_MODELS] =
{
	{ 0, "Integra 24", 24, 20 },
	{ 1, "Integra 32", 32, 32 },
	{ 2, "Integra 64", 64, 64 },
	{ 3, "Integra 128", 128, 128 },
	{ 4, "Integra 128 WRL SIM300", 128, 128 },
	{ 132, "Integra 128 WRL LEON", 128, 128 },
	{ 66, "Integra 64 Plus", 64, 64 },
	{ 67, "Integra 128 Plus", 128, 128 },
	{ 72, "Integra 256 Plus", 256, 256 },
};

#define MAX_LENGTH_OF_ANSWER 63 * 2 + 4

const unsigned char partitions[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

SatelIntegra::SatelIntegra(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode) :
	m_modelIndex(-1),
	m_data32(false),
	m_socket(INVALID_SOCKET),
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_stoprequested(false)
{
	_log.Log(LOG_STATUS, "Satel Integra: Create instance");
	m_HwdID = ID;


#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version;

	version = (MAKEWORD(2, 2));
	ret = WSAStartup(version, &data);
	if (ret != 0)
	{
		ret = WSAGetLastError();

		if (ret == WSANOTINITIALISED)
		{
			_log.Log(LOG_ERROR, "Satel Integra: Winsock could not be initialized!");
		}
	}
#endif

	// clear last local state of zones and outputs
	for (unsigned int i = 0; i< 256; ++i)
	{
		m_zonesLastState[i] = false;
		m_outputsLastState[i] = false;
		m_isOutputSwitch[i] = false;
	}

	m_armLast = false;

	errorCodes[1] = "requesting user code not found";
	errorCodes[2] = "no access";
	errorCodes[3] = "selected user does not exist";
	errorCodes[4] = "selected user already exists";
	errorCodes[5] = "wrong code or code already exists";
	errorCodes[6] = "telephone code already exists";
	errorCodes[7] = "changed code is the same";
	errorCodes[8] = "other error";
	errorCodes[17] = "can not arm, but, but can use force arm";
	errorCodes[18] = "can not arm";

	// decode user code from string to BCD
	uint64_t result(0);
	for (unsigned int i = 0; i < 16; ++i)
	{
		result = result << 4;

		if (i < userCode.size())
		{
			result += (userCode[i] - 48);
		}
		else
		{
			result += 0x0F;
		}
	}
	for (unsigned int i = 0; i < 8; ++i)
	{
		unsigned int c = (unsigned int)(result >> ((7 - i) * 8));
		m_userCode[i] = c;
	}

}

SatelIntegra::~SatelIntegra()
{
	_log.Log(LOG_NORM, "Satel Integra: Destroy instance");

#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool SatelIntegra::StartHardware()
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Start hardware");
#endif

	if (!CheckAddress())
	{
		return false;
	}

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&SatelIntegra::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool SatelIntegra::StopHardware()
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Stop hardware");
#endif

	m_stoprequested = true;

	DestroySocket();

	m_bIsStarted = false;
	return true;
}

void SatelIntegra::Do_Work()
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Start work");
#endif

	if (GetInfo())
	{

		ReadArmState(true);
		ReadZonesState(true);
		ReadOutputsState(true);

		int sec_counter = SATEL_POLL_INTERVAL - 2;

		while (!m_stoprequested)
		{
			sleep_seconds(1);
			if (m_stoprequested)
				break;
			sec_counter++;

			if (sec_counter % 12 == 0) {
				m_LastHeartbeat = mytime(NULL);
			}

			if (sec_counter % SATEL_POLL_INTERVAL == 0)
			{
				_log.Log(LOG_STATUS, "Satel Integra: fetching data");

				if (IsNewData())
				{
					if (m_newData[2] & 4)
					{
						ReadArmState();
					}
					if (m_newData[1] & 1)
					{
						ReadZonesState();
					}
					if (m_newData[3] & 128)
					{
						ReadOutputsState();
					}
				}
			}
		}
	}
}

bool SatelIntegra::CheckAddress()
{
	if (m_IPAddress.size() == 0 || m_IPPort < 1 || m_IPPort > 65535)
	{
		_log.Log(LOG_ERROR, "Satel Integra: Empty IP Address or bad Port");
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_IPPort);

	unsigned long ip;
	ip = inet_addr(m_IPAddress.c_str());

	if (ip != INADDR_NONE)
	{
		m_addr.sin_addr.s_addr = ip;
	}
	else
	{
		hostent *he = gethostbyname(m_IPAddress.c_str());
		if (he == NULL)
		{
			_log.Log(LOG_ERROR, "Satel Integra: cannot resolve host name");
			return false;
		}
		else
		{
			memcpy(&(m_addr.sin_addr), he->h_addr_list[0], 4);
		}
	}
	return true;
}

bool SatelIntegra::ConnectToIntegra()
{
	if (m_socket != INVALID_SOCKET)
		return true;

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR, "Satel Integra: Unable to create socket");
		return false;
	}


	if (connect(m_socket, (const sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR)
	{
		_log.Log(LOG_ERROR, "Satel Integra: Unable to connect to specified IP Address on specified Port (%s:%d)", m_IPAddress.c_str(), m_IPPort);
		DestroySocket();
		return false;
	}
#if defined WIN32
	// If iMode != 0, non - blocking mode is enabled.
	u_long iMode = 1;
	ioctlsocket(m_socket, FIONBIO, &iMode);
#else
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif
	_log.Log(LOG_STATUS, "Satel Integra: connected to %s:%ld", m_IPAddress.c_str(), m_IPPort);

	return true;
}

void SatelIntegra::DestroySocket()
{
	if (m_socket != INVALID_SOCKET)
	{
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: destroy socket");
#endif
		try
		{
			closesocket(m_socket);
		}
		catch (...)
		{
		}

		m_socket = INVALID_SOCKET;
	}
}

bool SatelIntegra::IsNewData()
{
	unsigned char cmd[1];
	cmd[0] = 0x7F; // list of new data
	if (SendCommand(cmd, 1, m_newData) > 0)
	{
		return true;
	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Get info about new data is failed");
	}

	return false;
}

bool SatelIntegra::GetInfo()
{
	unsigned char buffer[15];

	unsigned char cmd[1];
	cmd[0] = 0x7E; // Integra version
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		for (unsigned int i = 0; i < TOT_MODELS; ++i)
		{
			if (models[i].id == buffer[1])
			{
				m_modelIndex = i; // finded Integra type
				break;
			}
		}
		if (m_modelIndex > -1)
		{
			_log.Log(LOG_STATUS, "Satel Integra: Model %s", models[m_modelIndex].name);

			unsigned char cmd[1];
			cmd[0] = 0x1A; // RTC
			if (SendCommand(cmd, 1, buffer) > 0)
			{
				_log.Log(LOG_STATUS, "Satel Integra: RTC %.2x%.2x-%.2x-%.2x %.2x:%.2x:%.2x",
					buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

				unsigned char cmd[1];
				cmd[0] = 0x7C; // INT-RS/ETHM version
				if (SendCommand(cmd, 1, buffer) > 0)
				{
					if (buffer[12] == 1)
					{
						m_data32 = true;
					}

					_log.Log(LOG_STATUS, "Satel Integra: ETHM-1 ver. %c.%c%c %c%c%c%c-%c%c-%c%c",
						buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11]);
				}
				else
				{
					_log.Log(LOG_STATUS, "Satel Integra: unknown version of ETHM-1");
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "Satel Integra: Unknown basic status");
				return false;
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Satel Integra: Unknown model '%02x'", buffer[0]);
			return false;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Get info about Integra is failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadZonesState(const bool firstTime)
{
	if (m_modelIndex == -1)
	{
		return false;
	}

#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Read zones states");
#endif
	unsigned char buffer[33];

	unsigned char cmd[1];
	cmd[0] = 0x00; // read zones violation
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		bool violate;
		unsigned int byteNumber;
		unsigned int bitNumber;

		unsigned int zonesCount = models[m_modelIndex].zones;
		if ((zonesCount > 128) && (!m_data32))
		{
			zonesCount = 128;
		}

		for (unsigned int index = 0; index < zonesCount; ++index)
		{

			byteNumber = index / 8;
			bitNumber = index % 8;
			violate = (buffer[byteNumber + 1] >> bitNumber) & 0x01;

			if (firstTime)
			{
				unsigned char buffer[21];
#ifdef DEBUG_SatelIntegra
				_log.Log(LOG_STATUS, "Satel Integra: Reading zone %d name", index + 1);
#endif
				unsigned char cmd[3];
				cmd[0] = 0xEE;
				cmd[1] = 0x05;
				cmd[2] = (unsigned char)(index + 1);
				if (SendCommand(cmd, 3, buffer) > 0)
				{
					ReportZonesViolation(index + 1, violate);
					//UpdateZoneName(index + 1, &buffer[4], buffer[20]);  	 Security1 does not support units - partition
					UpdateZoneName(index + 1, &buffer[4], 0);
				}
				else
				{
					_log.Log(LOG_ERROR, "Satel Integra: Receive info about zone %d failed", index + 1);
				}
			}
			else if (m_zonesLastState[index] != violate)
			{
				ReportZonesViolation(index + 1, violate);
			}
		}

	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Read Outputs' failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadOutputsState(const bool firstTime)
{
	if (m_modelIndex == -1)
	{
		return false;
	}
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Read outputs states");
#endif
	unsigned char buffer[33];

	unsigned char cmd[1];
	cmd[0] = 0x17; // read outputs state
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		bool outputState;
		unsigned int byteNumber;
		unsigned int bitNumber;

		unsigned int outputsCount = models[m_modelIndex].outputs;
		if ((outputsCount > 128) && (!m_data32))
		{
			outputsCount = 128;
		}

		for (unsigned int index = 0; index < outputsCount; ++index)
		{
			byteNumber = index / 8;
			bitNumber = index % 8;
			outputState = (buffer[byteNumber + 1] >> bitNumber) & 0x01;

			if (firstTime)
			{
#ifdef DEBUG_SatelIntegra
				_log.Log(LOG_STATUS, "Satel Integra: Reading output %d name", index + 1);
#endif
				unsigned char buffer[21];
				unsigned char cmd[3];
				cmd[0] = 0xEE;
				cmd[1] = 0x04;
				cmd[2] = (unsigned char)(index + 1);
				if (SendCommand(cmd, 3, buffer) > 0)
				{
					if (buffer[3] != 0x00)
					{
						if ((buffer[3] == 24) || (buffer[3] == 25) || (buffer[3] == 105) || (buffer[3] == 106) || // switch MONO, switch BI, roller blind up/down
							((buffer[3] >= 64) && (buffer[3] <= 79)))  // DTMF
						{
							m_isOutputSwitch[index] = true;
						}
						ReportOutputState(index + 1, outputState);
						UpdateOutputName(index + 1 + 256, &buffer[4], m_isOutputSwitch[index]);
					}
					else
					{
#ifdef DEBUG_SatelIntegra
						_log.Log(LOG_STATUS, "Satel Integra: output %d is not used", index + 1);
#endif
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "Satel Integra: Receive info about output %d failed", index);
				}
			}
			else if (m_outputsLastState[index] != outputState)
			{
				ReportOutputState(index + 1, outputState);
			}
		}

	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Read outputs' failed");
		return false;
	}

	return true;
}

void SatelIntegra::ReportZonesViolation(const unsigned long Idx, const bool violation)
{
	m_zonesLastState[Idx - 1] = violation;

	RBUF security;
	memset(&security, 0, sizeof(RBUF));

	security.SECURITY1.packetlength = sizeof(security.SECURITY1) - 1;
	security.SECURITY1.packettype = pTypeSecurity1;
	security.SECURITY1.subtype = sTypeSecX10;
	security.SECURITY1.id1 = 0;
	security.SECURITY1.id2 = 0;
	security.SECURITY1.id3 = (unsigned char)Idx;
	security.SECURITY1.rssi = 12;
	security.SECURITY1.battery_level = 0x0f;
	security.SECURITY1.status = violation ? sStatusAlarm : sStatusNormal;

	sDecodeRXMessage(this, (const unsigned char *)&security.SECURITY1);
}

void SatelIntegra::ReportOutputState(const unsigned long Idx, const bool state)
{
	m_outputsLastState[Idx - 1] = state;

	/*
	RBUF output;
	memset(&output,0,sizeof(RBUF));

	output.LIGHTING2.packetlength = sizeof(output.LIGHTING2)-1;
	output.LIGHTING2.packettype = pTypeLighting2;
	output.LIGHTING2.subtype = sTypeAC;
	output.LIGHTING2.seqnbr = 0;
	output.LIGHTING2.id1 = 0;
	output.LIGHTING2.id2 = 0;
	output.LIGHTING2.id3 = 0;
	output.LIGHTING2.id4 = Idx;
	output.LIGHTING2.unitcode = 1;
	output.LIGHTING2.cmnd = state ? light2_sOn : light2_sOff;
	output.LIGHTING2.level = 0;
	output.LIGHTING2.rssi = 12;

	sDecodeRXMessage(this, (const unsigned char *)&output.LIGHTING2);
	*/
	// TODO - add and use own specialized type

	if (m_isOutputSwitch[Idx - 1])
	{
		_tGeneralSwitch output;
		output.len = sizeof(_tGeneralSwitch) - 1;
		output.type = pTypeGeneralSwitch;
		output.subtype = sSwitchTypeAC;
		output.id = Idx + 256;
		output.unitcode = 1;
		output.cmnd = state ? gswitch_sOn : gswitch_sOff;
		output.level = 0;
		output.battery_level = 0;
		output.rssi = 12;
		output.seqnbr = 0;
		sDecodeRXMessage(this, (const unsigned char *)&output);
	}
	else
	{
		_tGeneralDevice output;
		output.len = sizeof(_tGeneralDevice) - 1;
		output.type = pTypeGeneral;
		output.subtype = sTypePercentage;
		output.id = 0;
		output.floatval1 = state ? 100.0f : 0.0f;
		output.floatval2 = 0;
		output.intval1 = Idx + 256;
		output.intval2 = 0;
		sDecodeRXMessage(this, (const unsigned char *)&output);
	}
}

void SatelIntegra::ReportArmState(const bool isArm)
{
	m_armLast = isArm;

	RBUF security;
	memset(&security, 0, sizeof(RBUF));

	security.SECURITY1.packetlength = sizeof(security.SECURITY1) - 1;
	security.SECURITY1.packettype = pTypeSecurity1;
	security.SECURITY1.subtype = sTypeSecX10R;
	security.SECURITY1.id1 = 1;
	security.SECURITY1.id2 = 0;
	security.SECURITY1.id3 = 0;
	security.SECURITY1.rssi = 12;
	security.SECURITY1.battery_level = 0x0f;
	security.SECURITY1.status = isArm ? sStatusArmAway : sStatusDisarm;

	sDecodeRXMessage(this, (const unsigned char *)&security.SECURITY1);
}

bool SatelIntegra::ArmPartitions(const unsigned char* partitions, const unsigned int mode)
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: arming partitions");
#endif
	if (mode > 3)
	{
		_log.Log(LOG_ERROR, "Satel Integra: incorrect mode %d", mode);
		return false;
	}

	unsigned char buffer[2];

	unsigned char cmd[13];
	cmd[0] = (unsigned char)(0x80 + mode); // arm in mode 0
	for (unsigned int i = 0; i < 8; ++i)
	{
		cmd[i + 1] = m_userCode[i];
	}
	for (unsigned int i = 0; i < 4; ++i)
	{
		cmd[i + 9] = partitions[i];
	}

	if (SendCommand(cmd, 13, buffer) == -1) // arm
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Arm' failed");
		return false;
	}

	_log.Log(LOG_STATUS, "Satel Integra: System armed");
	return true;
}

bool SatelIntegra::DisarmPartitions(const unsigned char* partitions)
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: disarming partitions");
#endif

	unsigned char buffer[2];

	unsigned char cmd[13];
	cmd[0] = 0x84; // disarm
	for (unsigned int i = 0; i < 8; ++i)
	{
		cmd[i + 1] = m_userCode[i];
	}
	for (unsigned int i = 0; i < 4; ++i)
	{
		cmd[i + 9] = partitions[i];
	}

	if (SendCommand(cmd, 13, buffer) == -1) // disarm
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Disarm' failed");
		return false;
	}

	_log.Log(LOG_STATUS, "Satel Integra: System disarmed");
	return true;
}

bool SatelIntegra::ReadArmState(const bool firstTime)
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Read arm state");
#endif
	unsigned char buffer[5];

	unsigned char cmd[1];
	cmd[0] = 0x0A; // read armed partition
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		bool armed = false;

		for (unsigned int index = 0; index < 4; ++index)
		{
			if (buffer[index + 1])
			{
				armed = true;
				break;
			}
		}

		if (firstTime)
		{
			ReportArmState(armed);
		}
		else if (m_armLast != armed)
		{
			if (armed)
			{
				_log.Log(LOG_STATUS, "Satel Integra: System arm");
			}
			else
			{
				_log.Log(LOG_STATUS, "Satel Integra: System not arm");
			}

			ReportArmState(armed);
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Get Armed partitions' failed");
		return false;
	}

	return true;
}


bool SatelIntegra::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *output = (tRBUF*)pdata;

	if (output->ICMND.packettype == pTypeSecurity1 && output->ICMND.subtype == sTypeSecX10R)
	{
		if (output->ICMND.cmnd == gswitch_sOn)
		{
			return ArmPartitions(partitions);
		}
		else
		{
			return DisarmPartitions(partitions);
		}
	}

	if (output->ICMND.packettype == pTypeGeneralSwitch && output->ICMND.subtype == sSwitchTypeAC)
	{
		_tGeneralSwitch *general = (_tGeneralSwitch*)pdata;
		unsigned char buffer[2];
		unsigned char cmd[41] = { 0 };

		if (general->cmnd == gswitch_sOn)
		{
			cmd[0] = 0x88;
		}
		else
		{
			cmd[0] = 0x89;
		}

		for (unsigned int i = 0; i < sizeof(m_userCode); ++i)
		{
			cmd[i + 1] = m_userCode[i];
		}

		unsigned char byteNumber = (general->id - 256) / 8;
		unsigned char bitNumber = (general->id - 256) % 8;

		cmd[byteNumber + 9] = 0x01 << bitNumber;

		if (SendCommand(cmd, 41, buffer) != -1)
		{
			_log.Log(LOG_STATUS, "Satel Integra: switched output %d", general->id - 256);
			return true;
		}
		else
		{
			_log.Log(LOG_ERROR, "Satel Integra: Switch output %d failed", general->id - 256);
			return false;
		}
	}
	return false;
}

void SatelIntegra::UpdateZoneName(const unsigned int Idx, const unsigned char* name, const unsigned int partition)
{
	std::vector<std::vector<std::string> > result;

	char szTmp[10];
	sprintf(szTmp, "%06X", (unsigned int)Idx);

	std::string shortName((char*)name, 16);
	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name=='%q')", m_HwdID, szTmp, shortName.c_str());
	if (result.size() < 1)
	{
		//Assign name from Integra
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update name for %d to '%s'", Idx, shortName.c_str());
#endif
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, Unit=%d WHERE (HardwareID==%d) AND (DeviceID=='%q')", shortName.c_str(), STYPE_Contact, partition, m_HwdID, szTmp);
	}
}

void SatelIntegra::UpdateOutputName(const unsigned int Idx, const unsigned char* name, const bool switchable)
{
	std::vector<std::vector<std::string> > result;

	char szTmp[10];
	sprintf(szTmp, "%08X", (unsigned int)Idx);

	std::string shortName((char*)name, 16);
	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name=='%q')", m_HwdID, szTmp, shortName.c_str());
	if (result.size() < 1)
	{
		//Assign name from Integra
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update name for %d to '%s'", Idx, shortName.c_str());
#endif

		_eSwitchType switchType = STYPE_Contact;
		if (switchable)
		{
			switchType = STYPE_OnOff;
		}
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d WHERE (HardwareID==%d) AND (DeviceID=='%q')", shortName.c_str(), switchType, m_HwdID, szTmp);
	}
}

void expandForSpecialValue(std::list<unsigned char> &result)
{
	std::list<unsigned char>::iterator it = result.begin();

	const unsigned char specialValue = 0xFE;

	for (; it != result.end(); it++)
	{
		if (*it == specialValue)
		{
			result.insert(++it, 0xF0);
			it--;
		}
	}
}

void calculateCRC(const unsigned char* pCmd, unsigned int length, unsigned short &result)
{
	unsigned short crc = 0x147A;

	for (unsigned int i = 0; i < length; ++i)
	{
		crc = (crc << 1) | (crc >> 15);
		crc = crc ^ 0xFFFF;
		crc = crc + (crc >> 8) + pCmd[i];
	}

	result = crc;
}

int SatelIntegra::SendCommand(const unsigned char* cmd, unsigned int cmdLength, unsigned char *answer)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);

	if (!ConnectToIntegra())
	{
		return -1;
	}

	std::pair<unsigned char*, unsigned int> cmdPayload;
	cmdPayload = getFullFrame(cmd, cmdLength);

	//Send cmd
	if (send(m_socket, (const char*)cmdPayload.first, cmdPayload.second, 0) < 0)
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send command '%02X' failed", cmdPayload.first[2]);
		delete cmdPayload.first;
		return -1;
	}

	delete cmdPayload.first;

	unsigned char buffer[MAX_LENGTH_OF_ANSWER];
	// Receive answer
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(m_socket, &rfds);

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (select(m_socket + 1, &rfds, NULL, NULL, &tv) < 0)
	{
		_log.Log(LOG_ERROR, "Satel Integra: no connection.");
		DestroySocket();
		return -1;
	}

	int ret = recv(m_socket, (char*)&buffer, MAX_LENGTH_OF_ANSWER, 0);

	if (ret > 6)
	{
		if (buffer[0] == 0xFE && buffer[1] == 0xFE && buffer[ret - 1] == 0x0D && buffer[ret - 2] == 0xFE)
		{
			unsigned int answerLength = 0;
			for (int i = 0; i < ret - 6; i++)
				if (buffer[i + 2] != 0xF0 || buffer[i + 1] != 0xFE) // skip special value
				{
					answer[answerLength] = buffer[i + 2];
					answerLength++;
				}
			unsigned short crc;
			calculateCRC(answer, answerLength, crc);
			if ((crc & 0xFF) == buffer[ret - 3] && (crc >> 8) == buffer[ret - 4])
			{
				if (buffer[2] == 0xEF)
				{
					if (buffer[3] == 0x00 || buffer[3] == 0xFF)
					{
						return 0;
					}
					else
					{
						const char* error = "other errors";

						std::map<unsigned int, const char*>::iterator it;
						it = errorCodes.find(buffer[3]);
						if (it != errorCodes.end())
						{
							error = it->second;
						}
						_log.Log(LOG_ERROR, "Satel Integra: receive error: %s", error);
						return -1;
					}
				}
				else
				{
					return answerLength;
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "Satel Integra: receive bad CRC");
				return -1;
			}
		}
		else
		{
			if (buffer[0] == 16)
			{
				_log.Log(LOG_STATUS, "Satel Integra: busy");
				return -1;
			}
			else
			{
				_log.Log(LOG_ERROR, "Satel Integra: received bad frame (prefix or sufix)");
				return -1;
			}
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: received frame is too short.");
		DestroySocket();
		return -1;
	}

}

std::pair<unsigned char*, unsigned int> SatelIntegra::getFullFrame(const unsigned char* pCmd, unsigned int cmdLength)
{
	std::list<unsigned char> result;

	for (unsigned int i = 0; i< cmdLength; ++i)
	{
		result.push_back(pCmd[i]);
	}

	// add CRC
	unsigned short crc;
	calculateCRC(pCmd, cmdLength, crc);
	result.push_back(crc >> 8);
	result.push_back(crc & 0xFF);
	// check special value
	expandForSpecialValue(result);
	// add prefix
	result.push_front(0xFE);
	result.push_front(0xFE);
	// add sufix
	result.push_back(0xFE);
	result.push_back(0x0D);

	unsigned int resultSize = result.size();
	unsigned char* pResult = new unsigned char[resultSize];
	memset(pResult, 0, resultSize);
	std::list<unsigned char>::iterator it = result.begin();
	for (unsigned int index = 0; it != result.end(); ++it, ++index)
	{
		pResult[index] = *it;
	}

	return std::pair<unsigned char*, unsigned int>(pResult, resultSize);
}
