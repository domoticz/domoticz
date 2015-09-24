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
	#define DEBUG_SatelIntegra
#endif

#define SATEL_POLL_INTERVAL 4

#define round(a) ( int ) ( a + .5 )

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

	// clear last local state of zones and outputs
	for (unsigned int i = 0; i< 256; ++i)
	{
		m_zonesLastState[i] = false;
		m_outputsLastState[i] = false;
		m_isOutputSwitch[i] = false;
		m_isTemperature[i] = false;
	}

	m_armLast = false;
	m_alarmLast = false;

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

		ReadAlarm(true);
		ReadArmState(true);
		ReadZonesState(true);
		ReadOutputsState(true);

		UpdateAlarmAndArmName();

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
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: fetching data");
#endif

				if (IsNewData())
				{
					if (m_newData[3] & 8)
					{
						ReadAlarm();
					}
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

	unsigned int zonesCount = models[m_modelIndex].zones;
	if ((zonesCount > 128) && (!m_data32))
	{
		zonesCount = 128;
	}

	unsigned char cmd[1];
	cmd[0] = 0x00; // read zones violation
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		bool violate;
		unsigned int byteNumber;
		unsigned int bitNumber;

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
					UpdateZoneName(index + 1, &buffer[4], 1);
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

	// Read temperatures from ATD100
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Read zones temperatures");
#endif
	for (unsigned int index = 0; index < zonesCount; ++index)
	if (m_isTemperature[index])
	{
#ifdef DEBUG_SatelIntegra
				_log.Log(LOG_STATUS, "Satel Integra: Reading zone %d temperature", index + 1);
#endif
		unsigned char cmd[2];
		cmd[0] = 0x00; // read zone temperature
		cmd[1] = (index != 255) ? (index + 1) : 0;
		if (SendCommand(cmd, 2, buffer) > 0)
		{
			unsigned int temp;
			sscanf(reinterpret_cast<const char*>(&buffer[2]), "%d", &temp);
			ReportTemperature(index + 1, temp);
		}
		else
		{
			_log.Log(LOG_ERROR, "Satel Integra: Send 'Read Temperature' failed");
			return false;
		}
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
						UpdateOutputName(index + 1, &buffer[4], m_isOutputSwitch[index]);
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

		if (firstTime || (m_armLast != armed))
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

bool SatelIntegra::ReadAlarm(const bool firstTime)
{
#ifdef DEBUG_SatelIntegra
	_log.Log(LOG_STATUS, "Satel Integra: Read partitions alarms");
#endif
	unsigned char buffer[5];

	unsigned char cmd[1];
	cmd[0] = 0x13; // read partitions alarm
	if (SendCommand(cmd, 1, buffer) > 0)
	{
		bool alarm = false;

		for (unsigned int index = 0; index < 4; ++index)
		{
			if (buffer[index + 1])
			{
				alarm = true;
				break;
			}
		}

		if (firstTime || (m_alarmLast != alarm))
		{
			if (alarm)
			{
				_log.Log(LOG_STATUS, "Satel Integra: ALARM !!");
			}
			else
			{
				_log.Log(LOG_STATUS, "Satel Integra: Alarm not set");
			}

			ReportAlarm(alarm);
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "Satel Integra: Send 'Get Alarm partitions' failed");
		return false;
	}

	return true;
}


void SatelIntegra::ReportZonesViolation(const unsigned long Idx, const bool violation)
{
	m_zonesLastState[Idx - 1] = violation;

//	char szTmp[4];
//	sprintf(szTmp, "%02X", (unsigned int)Idx);
//	std::string devname;

//	m_sql.UpdateValue(m_HwdID, szTmp, 0, pTypeGeneral, sTypeAlert, 12, 255, violation ? 3 : 1, violation ? "Violate" : "Normal", devname);

	_tGeneralDevice zone;
	zone.subtype = sTypeAlert;
	zone.id = (unsigned char)Idx;
	zone.intval1 = violation ? 3 : 1;

  if (m_mainworker.GetVerboseLevel() == EVBL_ALL)
  {
    _log.Log(LOG_STATUS, "Satel Integra: Report Zone %d = %d", zone.id, zone.intval1);
  }
  
	sDecodeRXMessage(this, (const unsigned char *)&zone);}

void SatelIntegra::ReportOutputState(const unsigned long Idx, const bool state)
{
	m_outputsLastState[Idx - 1] = state;

	if (m_isOutputSwitch[Idx - 1])
	{
		_tGeneralSwitch output;
		output.len = sizeof(_tGeneralSwitch) - 1;
		output.type = pTypeGeneralSwitch;
		output.subtype = sSwitchTypeAC;
		output.id = Idx;
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
		char szTmp[10];
		sprintf(szTmp, "%08X", (unsigned int)Idx);
		std::string devname;

		m_sql.UpdateValue(m_HwdID, szTmp, 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, state ? "On" : "Off", devname);
	}
}

void SatelIntegra::ReportArmState(const bool isArm)
{
	m_armLast = isArm;

	_tGeneralSwitch arm;
	arm.len = sizeof(_tGeneralSwitch) - 1;
	arm.type = pTypeGeneralSwitch;
	arm.subtype = sSwitchTypeAC;
	arm.id = 1;
	arm.unitcode = 2;
	arm.cmnd = isArm ? gswitch_sOn : gswitch_sOff;
	arm.level = 0;
	arm.battery_level = 0x0f;
	arm.rssi = 12;
	arm.seqnbr = 0;
	sDecodeRXMessage(this, (const unsigned char *)&arm);
}

void SatelIntegra::ReportAlarm(const bool isAlarm)
{
	m_alarmLast = isAlarm;

	char szTmp[8];
	sprintf(szTmp, "%06X", (unsigned int)2);
	std::string devname;

	m_sql.UpdateValue(m_HwdID, "Alarm", 2, pTypeGeneral, sTypeAlert, 12, 255, isAlarm ? 4 : 1, isAlarm ? "Alarm !" : "Normal", devname);
}


void SatelIntegra::ReportTemperature(const unsigned long Idx, unsigned int temp)
{
	RBUF tsen;
	memset(&tsen,0,sizeof(RBUF));
	tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
	tsen.TEMP.packettype=pTypeTEMP;
	tsen.TEMP.subtype=sTypeTEMP10;
	tsen.TEMP.battery_level=9;
	tsen.TEMP.rssi=12;
	tsen.TEMP.id1=(BYTE)Idx;
	tsen.TEMP.id2=0;

	temp = temp - 0x6E;
	tsen.TEMP.tempsign=(temp>=0)?0:1;
	int at10=round(abs(temp*10.0f));
	tsen.TEMP.temperatureh=(BYTE)(at10/256);
	at10-=(tsen.TEMP.temperatureh*256);
	tsen.TEMP.temperaturel=(BYTE)(at10);

	sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);
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

bool SatelIntegra::WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *output = (tRBUF*)pdata;

	if (output->ICMND.packettype == pTypeGeneralSwitch && output->ICMND.subtype == sSwitchTypeAC)
	{
		_tGeneralSwitch *general = (_tGeneralSwitch*)pdata;
		if (general->unitcode == 2) // arm
		{
			if (general->cmnd == gswitch_sOn)
			{
				return ArmPartitions(partitions);
			}
			else
			{
				return DisarmPartitions(partitions);
			}
		}
		else if (general->unitcode == 1) // outputs
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

			unsigned char byteNumber = (general->id - 1) / 8;
			unsigned char bitNumber = (general->id - 1) % 8;

			cmd[byteNumber + 9] = 0x01 << bitNumber;

			if (SendCommand(cmd, 41, buffer) != -1)
			{
				_log.Log(LOG_STATUS, "Satel Integra: switched output %d to %s", general->id, general->cmnd == gswitch_sOn ? "on" : "off");
				return true;
			}
			else
			{
				_log.Log(LOG_ERROR, "Satel Integra: Switch output %d failed", general->id);
				return false;
			}

		}
	}

	return false;
}

void SatelIntegra::UpdateZoneName(const unsigned int Idx, const unsigned char* name, const unsigned int partition)
{
	std::vector<std::vector<std::string> > result;

	char szTmp[4];
	sprintf(szTmp, "%d", (unsigned int)Idx);

	std::string shortName((char*)name, 16);
	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);

	std::string namePrefix = "Zone";
	if (shortName.find("ATD100") != std::string::npos)
	{
		m_isTemperature[Idx - 1] = true;
		namePrefix = "Temp";
	}

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name=='%q:%q') AND (Unit=1)", m_HwdID, szTmp, namePrefix.c_str(), shortName.c_str());
	if (result.size() < 1)
	{
		//Assign zone name from Integra
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update name for %d to '%s:%s'", Idx, namePrefix.c_str(), shortName.c_str());
#endif
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='%q:%q', SwitchType=%d, Unit=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=1)", namePrefix.c_str(), shortName.c_str(), STYPE_Contact, partition, m_HwdID, szTmp);
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

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name=='Output:%q') AND (Unit=1)", m_HwdID, szTmp, shortName.c_str());
	if (result.size() < 1)
	{
		//Assign output name from Integra
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update name for %d to '%s'", Idx, shortName.c_str());
#endif

		_eSwitchType switchType = STYPE_Contact;
		if (switchable)
		{
			switchType = STYPE_OnOff;
		}
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='Output:%q', SwitchType=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=1)", shortName.c_str(), switchType, m_HwdID, szTmp);
	}
}

void SatelIntegra::UpdateAlarmAndArmName()
{
	std::vector<std::vector<std::string> > result;

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='Alarm') AND (Name=='Alarm') AND (Unit=2)", m_HwdID);
	if (result.size() < 1)
	{
		//Assign name for Alarm
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update Alarm name to 'Alarm'");
#endif
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='Alarm' WHERE (HardwareID==%d) AND (DeviceID=='Alarm') AND (Unit=2)", m_HwdID);
	}

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='00000001') AND (Name=='Arm') AND (Unit=2)", m_HwdID);
	if (result.size() < 1)
	{
		//Assign name for Arm
#ifdef DEBUG_SatelIntegra
		_log.Log(LOG_STATUS, "Satel Integra: update Arm name for to 'Arm'");
#endif
		result = m_sql.safe_query("UPDATE DeviceStatus SET Name='Arm' WHERE (HardwareID==%d) AND (DeviceID=='00000001') AND (Unit=2)", m_HwdID);
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

int SatelIntegra::SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer)
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
		_log.Log(LOG_ERROR, "Satel Integra: connection lost.");
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

std::pair<unsigned char*, unsigned int> SatelIntegra::getFullFrame(const unsigned char* pCmd, const unsigned int cmdLength)
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
