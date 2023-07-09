#include <list>

#include "stdafx.h"
#include "SatelIntegra.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"

#ifdef _DEBUG
#define DEBUG_SatelIntegra
#endif

#define SATEL_TEMP_POLL_INTERVAL_MS 120*1000 // 120 sec
#define HEARTBEAT_INTERVAL_MS 12*1000 // 12 sec

#define round(a) ( int ) ( a + .5 )

using SatelModel = struct
{
	unsigned int id;
	const char* name;
	unsigned int zones;
	unsigned int outputs;
};

#define TOT_MODELS 9

constexpr std::array<SatelModel, TOT_MODELS> models{
	{
		{ 0, "Integra 24", 24, 20 },		   //
		{ 1, "Integra 32", 32, 32 },		   //
		{ 2, "Integra 64", 64, 64 },		   //
		{ 3, "Integra 128", 128, 128 },		   //
		{ 4, "Integra 128 WRL SIM300", 128, 128 }, //
		{ 132, "Integra 128 WRL LEON", 128, 128 }, //
		{ 66, "Integra 64 Plus", 64, 64 },	   //
		{ 67, "Integra 128 Plus", 128, 128 },	   //
		{ 72, "Integra 256 Plus", 256, 256 },	   //
	}						   //
};

#define MAX_LENGTH_OF_ANSWER 63 * 2 + 4 + 1

SatelIntegra::SatelIntegra(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode, const int pollInterval) :
	m_modelIndex(-1),
	m_data32(false),
	m_socket(INVALID_SOCKET),
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_pollInterval(pollInterval)
{
	Log(LOG_STATUS, "Create instance");
	m_HwdID = ID;
	memset(m_newData, 0, sizeof(m_newData));

	// clear last local state of zones and outputs
	m_zonesLastState = {};
	m_outputsLastState = {};
	m_isOutputSwitch = {};
	m_isTemperature = {};
	m_isPartitions = {};
	m_armLastState = {};
	m_alarmLast = false;

	errorCodes[1] = "requesting user code not found";
	errorCodes[2] = "no access";
	errorCodes[3] = "selected user does not exist";
	errorCodes[4] = "selected user already exists";
	errorCodes[5] = "wrong code or code already exists";
	errorCodes[6] = "telephone code already exists";
	errorCodes[7] = "changed code is the same";
	errorCodes[8] = "other error";
	errorCodes[17] = "can not arm, but can use force arm";
	errorCodes[18] = "can not arm";

	// decode user code from string to BCD
	if (userCode.empty())
	{
		Log(LOG_STATUS, "no user code was entered (arm/disarm/switching will not work)");
	}

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

	if (m_pollInterval < 500)
	{
		m_pollInterval = 500;
	}

}

SatelIntegra::~SatelIntegra()
{
	Log(LOG_STATUS, "Destroy instance");
}

bool SatelIntegra::StartHardware()
{
	RequestStart();

#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Start hardware");
#endif

	if (!CheckAddress())
	{
		return false;
	}

	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool SatelIntegra::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void SatelIntegra::Do_Work()
{
	if (!GetInfo())
		return;

	Log(LOG_STATUS, "worker started");

	ReadZonesState(true);
	ReadAlarm(true);
	ReadArmState(true);
	ReadTemperatures(true);
	ReadOutputsState(true);

	UpdateAlarmAndArmName();

	int heartbeatInterval = 0;
	long msec_poll_counter = 0;
	long msec_temp_counter = 0;
	long interval = m_pollInterval;
	if (interval > HEARTBEAT_INTERVAL_MS)
		interval = HEARTBEAT_INTERVAL_MS;

	while (!IsStopRequested(interval))
	{
		msec_poll_counter += interval;
		msec_temp_counter += interval;
		heartbeatInterval += interval;

		if (heartbeatInterval >= HEARTBEAT_INTERVAL_MS) {
			m_LastHeartbeat = mytime(nullptr);
			heartbeatInterval = 0;
		}

		if (msec_poll_counter >= m_pollInterval)
		{
			msec_poll_counter = 0;
#ifdef DEBUG_SatelIntegra
			Log(LOG_STATUS, "fetching changed data");
#endif

			if (ReadNewData())
			{
				SetHeartbeatReceived();

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
			//	ReadEvents();
		}

		if (msec_temp_counter >= SATEL_TEMP_POLL_INTERVAL_MS)
		{
			msec_temp_counter = 0;
#ifdef DEBUG_SatelIntegra
			Log(LOG_STATUS, "fetching temperature");
#endif
			ReadTemperatures();
		}
	}//while (!IsStopRequested())
	DestroySocket();
	Log(LOG_STATUS, "worker stopped");
}

bool SatelIntegra::CheckAddress()
{
	if (m_IPAddress.empty() || m_IPPort < 1 || m_IPPort > 65535)
	{
		Log(LOG_ERROR, "Empty IP Address or bad Port");
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
		if (he == nullptr)
		{
			Log(LOG_ERROR, "cannot resolve host name");
			return false;
		}
		memcpy(&(m_addr.sin_addr), he->h_addr_list[0], 4);
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
		Log(LOG_ERROR, "Unable to create socket");
		return false;
	}

	if (connect(m_socket, (const sockaddr*)&m_addr, sizeof(m_addr)) == SOCKET_ERROR)
	{
		Log(LOG_ERROR, "Unable to connect to specified IP Address on specified Port (%s:%d)", m_IPAddress.c_str(), m_IPPort);
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
	Log(LOG_STATUS, "connected to %s:%d", m_IPAddress.c_str(), m_IPPort);

	return true;
}

void SatelIntegra::DestroySocket()
{
	if (m_socket != INVALID_SOCKET)
	{
#ifdef DEBUG_SatelIntegra
		Log(LOG_STATUS, "destroy socket");
#endif
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

bool SatelIntegra::ReadNewData()
{
	unsigned char cmd[2];
	cmd[0] = 0x7F; // list of new data
	if (SendCommand(cmd, 1, m_newData, 6) > 0)
	{
		return true;
	}

	Log(LOG_ERROR, "Get info about new data is failed");
	return false;
}

bool SatelIntegra::GetInfo()
{
	unsigned char buffer[15];

	unsigned char cmd[1];
	cmd[0] = 0x7E; // Integra version
	if (SendCommand(cmd, 1, buffer, 15) > 0)
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
			Log(LOG_STATUS, "Model %s", models[m_modelIndex].name);

			unsigned char cmd[1];
			cmd[0] = 0x1A; // RTC
			if (SendCommand(cmd, 1, buffer, 10) > 0)
			{
				Log(LOG_STATUS, "RTC %.2x%.2x-%.2x-%.2x %.2x:%.2x:%.2x",
					buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

				unsigned char cmd[1];
				cmd[0] = 0x7C; // INT-RS/ETHM version
				if (SendCommand(cmd, 1, buffer, 13) > 0)
				{
					 m_data32 = ((buffer[12] & 1) == 1) && (m_modelIndex == 8); // supported and required 256 PLUS INTEGRA, m_modelIndex tabel explained @ line 34-42

					Log(LOG_STATUS, "ETHM-1 ver. %c.%c%c %c%c%c%c-%c%c-%c%c (32 bytes mode = %s)",
						buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11], m_data32 ? "true" : "false");
					return true;
				}
				Log(LOG_ERROR, "unknown version of ETHM-1");
			}
			else
			{
				Log(LOG_ERROR, "Unknown basic status");
			}
		}
		else
		{
			Log(LOG_ERROR, "Unknown model '%02x'", buffer[0]);
		}
	}
	else
	{
		Log(LOG_ERROR, "Get info about Integra is failed");
	}

	return false;
}

bool SatelIntegra::ReadZonesState(const bool firstTime)
{
	if (m_modelIndex == -1)
	{
		return false;
	}

#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Read zones states");
#endif
	unsigned char buffer[33];

	unsigned int zonesCount = models[m_modelIndex].zones;
	if ((zonesCount > 256) && (!m_data32))
	{
		zonesCount = 256;
	}

	unsigned char cmd[2];
	cmd[0] = 0x00; // read zones violation
	if (SendCommand(cmd, 1 + m_data32, buffer, 17 + m_data32 * 16) > 0)
	{
		bool violate;
		unsigned int byteNumber;
		unsigned int bitNumber;

		for (unsigned int index = 0; index < zonesCount; ++index)
		{
			if (!IsStopRequested(0))
			{
				byteNumber = index / 8;
				bitNumber = index % 8;
				violate = (buffer[byteNumber + 1] >> bitNumber) & 0x01;

				if (firstTime)
				{
					m_LastHeartbeat = mytime(nullptr);

					unsigned char buffer[21];
#ifdef DEBUG_SatelIntegra
					Log(LOG_STATUS, "Reading zone %d name", index + 1);
#endif
					unsigned char cmd[3];
					cmd[0] = 0xEE;
					cmd[1] = 0x05;
					cmd[2] = (index != 255) ? (index + 1) : 0; // send 0 instead of 256 in INTEGRA 256 PLUS
					if (SendCommand(cmd, 3, buffer, 21) > 0)
					{
						m_isPartitions[buffer[20] - 1] = true;
						ReportZonesViolation(index + 1, violate);
						UpdateZoneName(index + 1, &buffer[4], 1);
					}
					else
					{
						Log(LOG_ERROR, "Receive info about zone %d failed (probably zone is not used)", index + 1);
					}
				}
				else if (m_zonesLastState[index] != violate)
				{
					ReportZonesViolation(index + 1, violate);
				}
			}
		}

	}
	else
	{
		Log(LOG_ERROR, "Send 'Read Outputs' failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadTemperatures(const bool firstTime)
{
	if (m_modelIndex == -1)
	{
		return false;
	}

	// Read temperatures from ATD100
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Read zones temperatures");
#endif

	unsigned char buffer[33];

	unsigned int zonesCount = models[m_modelIndex].zones;
	if ((zonesCount > 256) && (!m_data32))
	{
		zonesCount = 256;
	}

	for (unsigned int index = 0; index < zonesCount; ++index)
		if ((m_isTemperature[index]) && (!IsStopRequested(0)))
		{
#ifdef DEBUG_SatelIntegra
			Log(LOG_STATUS, "Reading zone %d temperature", index + 1);
#endif
			unsigned char cmd[2];
			cmd[0] = 0x7D; // read zone temperature
			cmd[1] = (index != 255) ? (index + 1) : 0;
			if (SendCommand(cmd, 2, buffer, 4) > 0)
			{
				uint16_t temp = buffer[2] * 256 + buffer[3];

				if (temp != 0 && temp != 65535) // -55oC and undetermined
				{
					if (firstTime)
					{
						m_LastHeartbeat = mytime(nullptr);

						unsigned char buffer[21];
#ifdef DEBUG_SatelIntegra
						Log(LOG_STATUS, "Reading temperature zone %d name", index + 1);
#endif
						unsigned char cmd[3];
						cmd[0] = 0xEE;
						cmd[1] = 0x05;
						cmd[2] = (index != 255) ? (index + 1) : 0; // send 0 instead of 256 in INTEGRA 256 PLUS
						if (SendCommand(cmd, 3, buffer, 21) > 0)
						{
							ReportTemperature(index + 1, temp);
							UpdateTempName(index + 1, &buffer[4], 0);
						}
						else
						{
							Log(LOG_ERROR, "Receive info about zone %d failed", index + 1);
						}
					}
					else
					{
						ReportTemperature(index + 1, temp);
					}
				}
			}

			else
			{
				Log(LOG_ERROR, "Send 'Read Temperature' failed");
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
	Log(LOG_STATUS, "Read outputs states");
#endif
	unsigned char buffer[33];

	unsigned char cmd[2];
	cmd[0] = 0x17; // read outputs state
	if (SendCommand(cmd, 1 + m_data32, buffer, 17 + m_data32 * 16) > 0)
	{
		bool findBlindOutput = false;
		bool outputState;
		unsigned int byteNumber;
		unsigned int bitNumber;

		unsigned int outputsCount = models[m_modelIndex].outputs;
		if ((outputsCount > 256) && (!m_data32))
		{
			outputsCount = 256;
		}

		for (unsigned int index = 0; index < outputsCount; ++index)
		{
			if (!IsStopRequested(0))
			{
				byteNumber = index / 8;
				bitNumber = index % 8;
				outputState = (buffer[byteNumber + 1] >> bitNumber) & 0x01;

				if (firstTime)
				{
#ifdef DEBUG_SatelIntegra
					Log(LOG_STATUS, "Reading output %d name", index + 1);
#endif
					unsigned char buffer[21];
					unsigned char cmd[3];
					cmd[0] = 0xEE;
					cmd[1] = 0x04;
					cmd[2] = (index != 255) ? (index + 1) : 0; // send 0 instead of 256 in INTEGRA 256 PLUS
					if (SendCommand(cmd, 3, buffer, 20) > 0)
					{
						if (buffer[3] != 0x00)
						{
							if ((buffer[3] == 24) || (buffer[3] == 25) || (buffer[3] == 105) || (buffer[3] == 106) || // switch MONO, switch BI, roller blind up/down
								((buffer[3] >= 64) && (buffer[3] <= 79)))  // DTMF
							{
								m_isOutputSwitch[index] = true;
							}

							if ((buffer[3] == 105) || (buffer[3] == 106)) // roller blind up and down
							{
								if (findBlindOutput == false)
								{
									ReportOutputState(index + 1 + 1024, outputState);
									UpdateOutputName(index + 1 + 1024, &buffer[4], STYPE_Blinds);
								}
								findBlindOutput = !findBlindOutput;
							}

							ReportOutputState(index + 1, outputState);
							UpdateOutputName(index + 1, &buffer[4], m_isOutputSwitch[index] ? STYPE_OnOff : STYPE_Contact);
						}
						else
						{
#ifdef DEBUG_SatelIntegra
							Log(LOG_STATUS, "output %d is not used", index + 1);
#endif
						}
					}
					else
					{
						Log(LOG_ERROR, "Receive info about output %d failed", index);
					}
				}
				else if (m_outputsLastState[index] != outputState)
				{
					ReportOutputState(index + 1, outputState);
				}
			}
		}
	}
	else
	{
		Log(LOG_ERROR, "Send 'Read outputs' failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadArmState(const bool firstTime)
{
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Read arm state");
#endif
	unsigned char buffer[5];

	unsigned char cmd[1];
	cmd[0] = 0x0A; // read armed partition
	if (SendCommand(cmd, 1, buffer, 5) > 0)
	{
		for (unsigned int index = 0; index < 32; ++index)
		{
			unsigned int byteNumber = index / 8;
			unsigned int bitNumber = index % 8;
			bool armed = (buffer[byteNumber + 1] >> bitNumber) & 0x01;

			if ((firstTime || (m_armLastState[index] != armed)) && (m_isPartitions[index]))
			{
				if (armed)
				{
					Log(LOG_STATUS, "partition %d arm", index + 1);
				}
				else
				{
					Log(LOG_STATUS, "partition %d not arm", index + 1);
				}

				ReportArmState(index + 1, armed);
			}
		}
	}
	else
	{
		Log(LOG_ERROR, "Send 'Get Armed partitions' failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadAlarm(const bool firstTime)
{
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Read partitions alarms");
#endif
	unsigned char buffer[5];

	unsigned char cmd[1];
	cmd[0] = 0x13; // read partitions alarm
	if (SendCommand(cmd, 1, buffer, 5) > 0)
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
				Log(LOG_STATUS, "ALARM !!");
			}
			else
			{
				Log(LOG_STATUS, "Alarm not set");
			}

			ReportAlarm(alarm);
		}
	}
	else
	{
		Log(LOG_ERROR, "Send 'Get Alarm partitions' failed");
		return false;
	}

	return true;
}

bool SatelIntegra::ReadEvents()
{
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "Read events");
#endif
	unsigned char buffer[15];

	unsigned char cmd[4];
	cmd[0] = 0x8C; // read events
	cmd[1] = 0xFF;
	cmd[2] = 0xFF;
	cmd[3] = 0xFF;
	if (SendCommand(cmd, 4, buffer, 15) > 0)
	{
		while (buffer[1] & 32)
		{
			cmd[1] = buffer[9];
			cmd[2] = buffer[10];
			cmd[3] = buffer[11];
			int ret = SendCommand(cmd, 4, buffer, 15);
			if (ret > 0)
			{
				std::string val(1, buffer[6]);
				SendTextSensor(1, 1, 255, val, "Events");
			}
			else
				return false;
		}
		return true;
	}
	return false;
}

void SatelIntegra::ReportZonesViolation(const int Idx, const bool violation)
{
	Debug(DEBUG_HARDWARE, "Report Zone %d = %d", Idx, violation ? 3 : 1);

	m_zonesLastState[Idx - 1] = violation;

	SendAlertSensor(Idx, 255, violation ? 3 : 1, "", "");
}

void SatelIntegra::ReportOutputState(const int Idx, const bool state)
{
	m_outputsLastState[Idx - 1] = state;

	if ((Idx > 1024) || m_isOutputSwitch[Idx - 1])
	{
		SendGeneralSwitch(Idx, 1, 255, state ? gswitch_sOn : gswitch_sOff, 0, "", m_Name);
	}
	else
	{
		std::string sTmp = std_format("%08X", Idx);
		std::string devname;

		m_sql.UpdateValue(m_HwdID, sTmp.c_str(), 1, pTypeGeneral, sTypeTextStatus, 12, 255, 0, state ? "On" : "Off", devname, true, m_Name.c_str());
	}
}

void SatelIntegra::ReportArmState(const int Idx, const bool isArm)
{
	m_armLastState[Idx - 1] = isArm;

	SendGeneralSwitch(Idx, 2, 255, isArm ? gswitch_sOn : gswitch_sOff, 0, "", m_Name);
}

void SatelIntegra::ReportAlarm(const bool isAlarm)
{
	m_alarmLast = isAlarm;

	std::string devname;

	m_sql.UpdateValue(m_HwdID, "Alarm", 2, pTypeGeneral, sTypeAlert, 12, 255, isAlarm ? 4 : 1, isAlarm ? "Alarm !" : "Normal", devname, true, m_Name.c_str());
}

void SatelIntegra::ReportTemperature(const int Idx, const int temp)
{
	float ftemp = static_cast<float>(temp - 0x6E);
	ftemp /= 2;
	SendTempSensor(Idx, 255, ftemp, "Temperature");
}

bool SatelIntegra::ArmPartitions(const int partition, const int mode)
{
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "arming partition %d", partition);
#endif
	if (mode > 3)
	{
		Log(LOG_ERROR, "incorrect arm mode %d", mode);
		return false;
	}

	unsigned char buffer[2];

	unsigned char cmd[13] = { 0 };
	cmd[0] = (unsigned char)(0x80 + mode); // arm in mode 0
	for (unsigned int i = 0; i < 8; ++i)
	{
		cmd[i + 1] = m_userCode[i];
	}

	unsigned char byteNumber = (partition - 1) / 8;
	unsigned char bitNumber = (partition - 1) % 8;

	cmd[byteNumber + 9] = 0x01 << bitNumber;

	if (SendCommand(cmd, 13, buffer, 1) == -1) // arm
	{
		Log(LOG_ERROR, "Send 'Arm partition %d' failed", partition);
		return false;
	}

	m_armLastState[partition - 1] = true;
	Log(LOG_STATUS, "Partition %d armed", partition);
	return true;
}

bool SatelIntegra::DisarmPartitions(const int partition)
{
#ifdef DEBUG_SatelIntegra
	Log(LOG_STATUS, "disarming partition %d", partition);
#endif

	unsigned char buffer[2];

	unsigned char cmd[13];
	cmd[0] = 0x84; // disarm
	for (unsigned int i = 0; i < 8; ++i)
	{
		cmd[i + 1] = m_userCode[i];
	}

	unsigned char byteNumber = (partition - 1) / 8;
	unsigned char bitNumber = (partition - 1) % 8;

	cmd[byteNumber + 9] = 0x01 << bitNumber;

	if (SendCommand(cmd, 13, buffer, 1) == -1) // disarm
	{
		Log(LOG_ERROR, "Send 'Disarm partition %d' failed", partition);
		return false;
	}

	m_armLastState[partition - 1] = false;
	Log(LOG_STATUS, "Partition %d disarmed", partition);
	return true;
}

bool SatelIntegra::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeGeneralSwitch && output->ICMND.subtype == sSwitchTypeAC)
	{
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		if (general->unitcode == 2) // arm
		{
			if (general->cmnd == gswitch_sOn)
			{
				return ArmPartitions(general->id);
			}
			return DisarmPartitions(general->id);
		}
		if (general->unitcode == 1) // outputs
		{
			unsigned char buffer[2];
			unsigned char cmd[41] = { 0 };

			for (unsigned int i = 0; i < sizeof(m_userCode); ++i)
			{
				cmd[i + 1] = m_userCode[i];
			}

			int id = general->id;
			unsigned char cmnd = general->cmnd;

			if (id > 1024) // blind roller
			{
				id = id - 1024;
				if (cmnd == gswitch_sOn)
				{
					id++;
				}
				cmnd = gswitch_sOn;
			}

			if (cmnd == gswitch_sOn)
			{
				cmd[0] = 0x88;
			}
			else
			{
				cmd[0] = 0x89;
			}

			unsigned char byteNumber = (id - 1) / 8;
			unsigned char bitNumber = (id - 1) % 8;

			cmd[byteNumber + 9] = 0x01 << bitNumber;

			if (SendCommand(cmd, 41, buffer, 1) != -1)
			{
				if (general->id <= 1024)
				{
					m_outputsLastState[id - 1] = cmnd == gswitch_sOn ? true : false;
				}
				Log(LOG_STATUS, "switched output %d to %s", id, cmnd == gswitch_sOn ? "on" : "off");
				return true;
			}
			Log(LOG_ERROR, "Switch output %d failed", id);
			return false;
		}
	}

	return false;
}

std::string SatelIntegra::ISO2UTF8(const std::string &name)
{
	char cp1250[] = "\xB9\xE6\xEA\xB3\xF1\xF3\x9C\x9F\xBF\xA5\xC6\xCA\xA3\xD1\xD3\x8C\x8F\xAF";
	char utf8[] = "\xC4\x85\xC4\x87\xC4\x99\xC5\x82\xC5\x84\xC3\xB3\xC5\x9B\xC5\xBA\xC5\xBC\xC4\x84\xC4\x86\xC4\x98\xC5\x81\xC5\x83\xC3\x93\xC5\x9A\xC5\xB9\xC5\xBB";

	std::string UTF8Name;
	for (char i : name)
	{
		bool changed = false;
		for (int j = 0; j < sizeof(cp1250); ++j)
		{
			if (i == cp1250[j])
			{
				UTF8Name += utf8[j * 2];
				UTF8Name += utf8[j * 2 + 1];
				changed = true;
				break;
			}
		}
		if (!changed)
		{
			UTF8Name += i;
		}
	}
	return UTF8Name;
}

void SatelIntegra::UpdateZoneName(const int Idx, const unsigned char* name, const int partition)
{
	std::vector<std::vector<std::string> > result;

	std::string sTmp = std_format("%d", Idx);

	std::string shortName((char*)name, 16);
	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);
	shortName = ISO2UTF8(shortName);

	std::string namePrefix = "Zone";
	if (shortName.find("ATD100") != std::string::npos)
	{
		m_isTemperature[Idx - 1] = true;
		namePrefix = "Temp";
	}

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name!='Unknown') AND (Unit=1)", m_HwdID, sTmp.c_str());
	if (result.empty())
	{
		//Assign zone name from Integra
#ifdef DEBUG_SatelIntegra
		Log(LOG_STATUS, "update name for %d to '%s:%s'", Idx, namePrefix.c_str(), shortName.c_str());
#endif
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q:%q', SwitchType=%d, Unit=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=1)", namePrefix.c_str(), shortName.c_str(),
				 STYPE_Contact, partition, m_HwdID, sTmp.c_str());
	}
}

void SatelIntegra::UpdateTempName(const int Idx, const unsigned char* name, const int partition)
{
	std::vector<std::vector<std::string> > result;

	std::string sTmp = std_format("%d", Idx);

	std::string shortName((char*)name, 16);
	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);
	shortName = ISO2UTF8(shortName);

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name!='Unknown') AND (Unit=0)", m_HwdID, sTmp.c_str());
	if (result.empty())
	{
		//Assign zone name from Integra
#ifdef DEBUG_SatelIntegra
		Log(LOG_STATUS, "update name for %d to 'Temp:%s'", Idx, shortName.c_str());
#endif
		m_sql.safe_query("UPDATE DeviceStatus SET Name='Temp:%q', SwitchType=%d, Unit=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=0)", shortName.c_str(), STYPE_Contact, partition,
				 m_HwdID, sTmp.c_str());
	}
}

void SatelIntegra::UpdateOutputName(const int Idx, const unsigned char* name, const _eSwitchType switchType)
{
	std::vector<std::vector<std::string> > result;

	std::string sTmp = std_format("%08X", Idx);

	std::string shortName((char*)name, 16);

	if (Idx > 1024)
	{
		shortName = "Common " + shortName; // for common roller blind
	}

	std::string::size_type pos = shortName.find_last_not_of(' ');
	shortName.erase(pos + 1);
	shortName = ISO2UTF8(shortName);

	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name!='Unknown') AND (Unit=1)", m_HwdID, sTmp.c_str());
	if (result.empty())
	{
		//Assign output name from Integra
#ifdef DEBUG_SatelIntegra
		Log(LOG_STATUS, "update name for %d to '%s'", Idx, shortName.c_str());
#endif

		m_sql.safe_query("UPDATE DeviceStatus SET Name='Output:%q', SwitchType=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=1)", shortName.c_str(), switchType, m_HwdID,
				 sTmp.c_str());
	}
}

void SatelIntegra::UpdateAlarmAndArmName()
{
	std::vector<std::vector<std::string> > result;

	// Alarm
	result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='Alarm') AND (Name!='Unknown') AND (Unit=2)", m_HwdID);
	if (result.empty())
	{
		//Assign name for Alarm
#ifdef DEBUG_SatelIntegra
		Log(LOG_STATUS, "update Alarm name to 'Alarm'");
#endif
		m_sql.safe_query("UPDATE DeviceStatus SET Name='Alarm' WHERE (HardwareID==%d) AND (DeviceID=='Alarm') AND (Unit=2)", m_HwdID);
	}

	//Arm
	for (const auto &itt : m_isPartitions)
	{
		if (itt == 0)
			continue;

		std::string sTmp = std_format("%08X", itt + 1);
		result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Name!='Unknown') AND (Unit=2)", m_HwdID, sTmp.c_str(), itt + 1);
		if (result.empty())
		{
			// Assign name for Arm
#ifdef DEBUG_SatelIntegra
			Log(LOG_STATUS, "update Arm name to 'Arm %d partition'", itt + 1);
#endif
			m_sql.safe_query("UPDATE DeviceStatus SET Name='Arm %d partition' WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit=2)", itt + 1, m_HwdID, sTmp.c_str());
		}
	}
}

void expandForSpecialValue(std::list<unsigned char> &result)
{
	const unsigned char specialValue = 0xFE;

	for (auto it = result.begin(); it != result.end(); it++)
	{
		if (*it == specialValue)
		{
			it = result.insert(++it, 0xF0);
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

int SatelIntegra::SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, const int expectedLength)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!ConnectToIntegra())
	{
		return -1;
	}

	std::pair<unsigned char*, unsigned int> cmdPayload;
	cmdPayload = getFullFrame(cmd, cmdLength);

	//Send cmd
	if (send(m_socket, (const char*)cmdPayload.first, cmdPayload.second, 0) < 0)
	{
		Log(LOG_ERROR, "Send command '%02X' failed", cmdPayload.first[2]);
		DestroySocket();
		delete[] cmdPayload.first;
		return -1;
	}

	delete[] cmdPayload.first;

	unsigned char buffer[MAX_LENGTH_OF_ANSWER];
	// Receive answer
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(m_socket, &rfds);

	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	if (select(m_socket + 1, &rfds, nullptr, nullptr, &tv) < 0)
	{
		Log(LOG_ERROR, "connection lost.");
		DestroySocket();
		return -1;
	}

	int totalRet = 0;
	do
	{
		int ret = recv(m_socket, (char*)&buffer[totalRet], MAX_LENGTH_OF_ANSWER - totalRet, 0);
		totalRet += ret;
		if ((ret <= 0) || (totalRet >= MAX_LENGTH_OF_ANSWER))
		{
			Log(LOG_ERROR, "bad data length received (totalRet %d)", totalRet);
			DestroySocket();
			return -1;
		}
	} while (totalRet < expectedLength);

	// remove special chars
	int offset = 0;
	for (int i = 0; i < totalRet; i++)
	{
		buffer[i] = buffer[i + offset];
		if (buffer[i] == 0xFE && buffer[i + 1] == 0xF0)
		{
			++offset;
			totalRet--;
		}
	}
	buffer[totalRet] = 0x00; // not needed but look nice :)

	if (totalRet > 6)
	{
		if (buffer[0] == 0xFE && buffer[1] == 0xFE && buffer[totalRet - 1] == 0x0D && buffer[totalRet - 2] == 0xFE) // check prefix and sufix
		{
			if ((buffer[2] != 0xEF) && ((totalRet - 6) != expectedLength))
			{
				Log(LOG_ERROR, "bad data length received (expectedLength %d, totalRet %d, cmd %X, cmdLength %d)", expectedLength, totalRet - 6, cmd[0], cmdLength);
				DestroySocket();
				return -1;
			}

			unsigned int answerLength = 0;
			for (int i = 0; i < totalRet - 6; i++) // skip prefix, suffix and crc
			{
				answer[i] = buffer[i + 2];
			}
			answerLength = totalRet - 6; // answer = frame - prefix - suffix - crc

			unsigned short crc;
			calculateCRC(answer, answerLength, crc);
			if ((crc & 0xFF) == buffer[totalRet - 3] && (crc >> 8) == buffer[totalRet - 4]) // check crc
			{
				if (buffer[2] == 0xEF)
				{
					if (buffer[3] == 0x00 || buffer[3] == 0xFF)
					{
						return 0;
					}
					const char *error = "other errors";

					auto it = errorCodes.find(buffer[3]);
					if (it != errorCodes.end())
					{
						error = it->second;
					}
					Log(LOG_ERROR, "receive error: %s", error);
					return -1;
				}
				return answerLength;
			}
			Log(LOG_ERROR, "receive bad CRC");
			return -1;
		}
		if (buffer[0] == 16)
		{
			Log(LOG_ERROR, "busy");
			return -1;
		}
		Log(LOG_ERROR, "received bad frame (prefix or sufix)");
		return -1;
	}
	Log(LOG_ERROR, "received frame is too short.");
	DestroySocket();
	return -1;
}

std::pair<unsigned char*, unsigned int> SatelIntegra::getFullFrame(const unsigned char* pCmd, const unsigned int cmdLength)
{
	std::list<unsigned char> result;

	for (unsigned int i = 0; i < cmdLength; ++i)
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

	return { pResult, resultSize };
}
