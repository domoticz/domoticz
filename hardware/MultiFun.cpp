#include <list>

#include "stdafx.h"
#include "MultiFun.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "csocket.h"
#include <boost/assign.hpp>

#ifdef _DEBUG
	#define DEBUG_MultiFun
#endif

#define BUFFER_LENGHT 100
#define MULTIFUN_POLL_INTERVAL 10 //TODO - to settings on www

#define round(a) ( int ) ( a + .5 )

typedef struct sensorType {
	std::string name;
	float div;
} Model;

#define sensorsCount 16
#define registersCount 34

typedef std::map<int, std::string> dictionary;

static dictionary alarmsType = boost::assign::map_list_of
(0x0001, "BOILER STOP - FIRING FAILURE")
(0x0004, "BOILER OVERHEATING")
(0x0010, "EXTINUISHED BOILER")
(0x0080, "DAMAGED SENSOR BOILER")
(0x0100, "DAMAGED SENSOR FEEDER")
(0x0200, "FLUE GAS SENSOR")
(0x0400, "FAILURE - LOCK WORKS");

static dictionary warningsType = boost::assign::map_list_of
(0x0001, "No external sensor")
(0x0002, "No room sensor 1")
(0x0004, "Wrong version supply module program")
(0x0008, "No return sensor")
(0x0010, "No room sensor 2")
(0x0020, "Open flap")
(0x0040, "Thermal protection has tripped");

static dictionary devicesType = boost::assign::map_list_of
(0x0001, "C.H.1 PUMP")
(0x0002, "C.H.2 PUMP")
(0x0004, "RESERVE PUMP")
(0x0008, "H.W.U.PUMP")
(0x0010, "CIRCULATION PUMP")
(0x0020, "PUFFER PUMP")
(0x0040, "MIXER C.H.1 Close")
(0x0080, "MIXER C.H.1 Open")
(0x0100, "MIXER C.H.2 Close")
(0x0200, "MIXER C.H.2 Open");

static dictionary statesType = boost::assign::map_list_of
(0x0001, "STOP")
(0x0002, "Firing")
(0x0004, "Heating")
(0x0008, "Maintain")
(0x0010, "Blanking");

static sensorType sensors[sensorsCount] =
{
	{ "External", 10.0 },
	{ "Room 1", 10.0 },
	{ "Room 2", 10.0 },
	{ "Return", 10.0 },
	{ "C.H.1", 10.0 },
	{ "C.H.2", 10.0 },
	{ "H.W.U.", 10.0 },
	{ "Heat", 1.0 },
	{ "Flue gas", 10.0 },
	{ "Module", 10.0 },
	{ "Boiler", 10.0 },
	{ "Feeder", 10.0 },
	{ "Calculated Boiler", 10.0 },
	{ "Calculated H.W.U.", 10.0 },
	{ "Calculated C.H.1", 10.0 },
	{ "Calculated C.H.2", 10.0 }
};

static dictionary quickAccessType = boost::assign::map_list_of
	(0x0001, "Shower")
	(0x0002, "Party")
	(0x0004, "Comfort")
	(0x0008, "Airing")
	(0x0010, "Frost protection");

static std::string errors[4] =
{
	"Incorrect function code",
	"Incorrect register address",
	"Incorrect number of registers",
	"Server error"
};

MultiFun::MultiFun(const int ID, const std::string &IPAddress, const unsigned short IPPort) :
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_stoprequested(false),
	m_socket(NULL),
	m_LastAlarms(0),
	m_LastWarnings(0),
	m_LastDevices(0),
	m_LastState(0),
	m_LastQuickAccess(0)
{
	_log.Log(LOG_STATUS, "MultiFun: Create instance");
	m_HwdID = ID;

	m_isSensorExists[0] = false;
	m_isSensorExists[1] = false;
	m_isWeatherWork[0] = false;
	m_isWeatherWork[1] = false;
}

MultiFun::~MultiFun()
{
	_log.Log(LOG_STATUS, "MultiFun: Destroy instance");
}

bool MultiFun::StartHardware()
{
#ifdef DEBUG_MultiFun
	_log.Log(LOG_STATUS, "MultiFun: Start hardware");
#endif

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&MultiFun::Do_Work, this)));
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != NULL);
}

bool MultiFun::StopHardware()
{
#ifdef DEBUG_MultiFun
	_log.Log(LOG_STATUS, "MultiFun: Stop hardware");
#endif

	m_stoprequested = true;
	
	if (m_thread)
	{
		m_thread->join();
	}

	DestroySocket();

	m_bIsStarted = false;
	return true;
}

void MultiFun::Do_Work()
{
#ifdef DEBUG_MultiFun
	_log.Log(LOG_STATUS, "MultiFun: Start work");
#endif

	int sec_counter = MULTIFUN_POLL_INTERVAL;

	bool firstTime = true;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}

		if (sec_counter % MULTIFUN_POLL_INTERVAL == 0)
		{
			GetTemperatures();
			GetRegisters(firstTime);
			firstTime = false;
#ifdef DEBUG_MultiFun
			_log.Log(LOG_STATUS, "MultiFun: fetching changed data");
#endif
		}
	}
}

bool MultiFun::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);

	if (output->ICMND.packettype == pTypeGeneralSwitch && output->LIGHTING2.subtype == sSwitchTypeAC)
	{
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		if (general->id == 0x21)
		{
			int change;
			if (general->cmnd == gswitch_sOn)
			{ 
				change = m_LastQuickAccess | (general->unitcode);
			}
			else
			{ 
				change = m_LastQuickAccess & ~(general->unitcode);
			}

			unsigned char buffer[100];
			unsigned char cmd[20];
			cmd[0] = 0x01; // transaction id (2 bytes)
			cmd[1] = 0x02;
			cmd[2] = 0x00; // protocol id (2 bytes)
			cmd[3] = 0x00;
			cmd[4] = 0x00; // length (2 bytes)
			cmd[5] = 0x09;
			cmd[6] = 0xFF; // unit id
			cmd[7] = 0x10; // function code 
			cmd[8] = 0x00; // start address (2 bytes)
			cmd[9] = 0x21;
			cmd[10] = 0x00; // number of sensor (2 bytes)
			cmd[11] = 0x01;
			cmd[12] = 0x02; // number of bytes
			cmd[13] = 0x00;
			cmd[14] = change;

			int ret = SendCommand(cmd, 15, buffer, true);
			if (ret == 4)
				return true;
		}
	}

	if (output->ICMND.packettype == pTypeThermostat && output->LIGHTING2.subtype == sTypeThermSetpoint)
	{
		const _tThermostat *therm = reinterpret_cast<const _tThermostat*>(pdata);

		float temp = therm->temp;
		int calculatedTemp = (int)temp;

		if ((therm->id2 == 0x1F || therm->id2 == 0x20) ||
			((therm->id2 == 0x1C || therm->id2 == 0x1D) && m_isWeatherWork[therm->id2 - 0x1C]))
		{
			calculatedTemp = (int)(temp * 5);
			calculatedTemp = calculatedTemp | 0x8000;
		}

		unsigned char buffer[100];
		unsigned char cmd[20];
		cmd[0] = 0x01; // transaction id (2 bytes)
		cmd[1] = 0x02;
		cmd[2] = 0x00; // protocol id (2 bytes)
		cmd[3] = 0x00;
		cmd[4] = 0x00; // length (2 bytes)
		cmd[5] = 0x09;
		cmd[6] = 0xFF; // unit id
		cmd[7] = 0x10; // function code 
		cmd[8] = 0x00; // start address (2 bytes)
		cmd[9] = therm->id2;
		cmd[10] = 0x00; // number of sensor (2 bytes)
		cmd[11] = 0x01; 
		cmd[12] = 0x02; // number of bytes
		cmd[13] = 0x00;
		cmd[14] = calculatedTemp;

		int ret = SendCommand(cmd, 15, buffer, true);
		if (ret == 4)
			return true;
	}

	return false;
}

bool MultiFun::ConnectToDevice()
{
	if (m_socket != NULL)
		return true;

	m_socket = new csocket();

	m_socket->connect(m_IPAddress.c_str(), m_IPPort);

	if (m_socket->getState() != csocket::CONNECTED)
	{
		_log.Log(LOG_ERROR, "MultiFun: Unable to connect to specified IP Address on specified Port (%s:%d)", m_IPAddress.c_str(), m_IPPort);
		DestroySocket();
		return false;
	}

	_log.Log(LOG_STATUS, "MultiFun: connected to %s:%ld", m_IPAddress.c_str(), m_IPPort);

	return true;
}

void MultiFun::DestroySocket()
{
	if (m_socket != NULL)
	{
#ifdef DEBUG_MultiFun
		_log.Log(LOG_STATUS, "MultiFun: destroy socket");
#endif
		try
		{
			delete m_socket;
		}
		catch (...)
		{
		}

		m_socket = NULL;
	}
}

void MultiFun::GetTemperatures()
{
	unsigned char buffer[50];
	unsigned char cmd[12];
	cmd[0] = 0x01; // transaction id (2 bytes)
	cmd[1] = 0x02;
	cmd[2] = 0x00; // protocol id (2 bytes)
	cmd[3] = 0x00;
	cmd[4] = 0x00; // length (2 bytes)
	cmd[5] = 0x06;
	cmd[6] = 0xFF; // unit id
	cmd[7] = 0x04; // function code 
	cmd[8] = 0x00; // start address (2 bytes)
	cmd[9] = 0x00;
	cmd[10] = 0x00; // number of sensor (2 bytes)
	cmd[11] = sensorsCount;

	int ret = SendCommand(cmd, 12, buffer, false);
	if (ret > 0)
	{
		if ((ret != 1 + sensorsCount * 2) || (buffer[0] != sensorsCount * 2))
		{
			_log.Log(LOG_ERROR, "MultiFun: Receive wrong number of bytes");
		}
		else
		{
			for (int i = 0; i < sensorsCount; i++)
			{
				unsigned int val = (buffer[i * 2 + 1] & 127) * 256 + buffer[i * 2 + 2];
				int signedVal = (((buffer[i * 2 + 1] & 128) >> 7) * -32768) + val;
				float temp = signedVal / sensors[i].div;

				if ((temp > -39) && (temp < 1000))
				{			
					SendTempSensor(i, 255, temp, sensors[i].name);
				}
				if ((i == 1) || (i == 2))
				{
					m_isSensorExists[i - 1] = ((temp > -39) && (temp < 1000));
				}
			}
		}

	}
	else
	{
		_log.Log(LOG_ERROR, "MultiFun: Receive info about temperatures failed");
	}
}

void MultiFun::GetRegisters(bool firstTime)
{
	unsigned char buffer[100];
	unsigned char cmd[12];
	cmd[0] = 0x01; // transaction id (2 bytes)
	cmd[1] = 0x02;
	cmd[2] = 0x00; // protocol id (2 bytes)
	cmd[3] = 0x00;
	cmd[4] = 0x00; // length (2 bytes)
	cmd[5] = 0x06;
	cmd[6] = 0xFF; // unit id
	cmd[7] = 0x03; // function code 
	cmd[8] = 0x00; // start address (2 bytes)
	cmd[9] = 0x00;
	cmd[10] = 0x00; // number of sensor (2 bytes)
	cmd[11] = registersCount;

	int ret = SendCommand(cmd, 12, buffer, false);
	if (ret > 0)
	{
		if ((ret != 1 + registersCount * 2) || (buffer[0] != registersCount * 2))
		{
			_log.Log(LOG_ERROR, "MultiFun: Receive wrong number of bytes");
		}
		else
		{
			for (int i = 0; i < registersCount; i++)
			{
				int value = buffer[2 * i + 1] * 256 + buffer[2 * i + 2];
				switch (i)
				{
				case 0x00:
				{
					dictionary::iterator it = alarmsType.begin();
					for (; it != alarmsType.end(); it++)
					{
						if (((*it).first & value) && !((*it).first & m_LastAlarms))
						{
							SendTextSensor(1, 0, 255, (*it).second, "Alarms");
						}
						else
							if (!((*it).first & value) && ((*it).first & m_LastAlarms))
							{
								SendTextSensor(1, 0, 255, "End - " + (*it).second, "Alarms");
							}
					}
					if (((m_LastAlarms != 0) != (value != 0)) || firstTime)
					{
						SendAlertSensor(0, 255, value ? 4 : 1, "", "Alarm");
					}
					m_LastAlarms = value;
					break;
				}
				case 0x01: 
				{
					dictionary::iterator it = warningsType.begin();
					for (; it != warningsType.end(); it++)
					{
						if (((*it).first & value) && !((*it).first & m_LastWarnings))
						{
							SendTextSensor(1, 1, 255, (*it).second, "Warnings");
						}
						else
							if (!((*it).first & value) && ((*it).first & m_LastWarnings))
							{
								SendTextSensor(1, 1, 255, "End - " + (*it).second, "Warnings");
							}
					}
					if (((m_LastWarnings != 0) != (value != 0)) || firstTime)
					{
						SendAlertSensor(1, 255, value ? 3 : 1, "", "Warning");
					}
					m_LastWarnings = value;
					break;
				}
				case 0x02:
				{
					dictionary::iterator it = devicesType.begin();
					for (; it != devicesType.end(); it++)
					{
						if (((*it).first & value) && !((*it).first & m_LastDevices))
						{
							SendGeneralSwitchSensor(2, 255, true, (*it).second.c_str(), (*it).first);
						}
						else
							if (!((*it).first & value) && ((*it).first & m_LastDevices))
							{
								SendGeneralSwitchSensor(2, 255, false, (*it).second.c_str(), (*it).first);
							}
					}
					m_LastDevices = value;

					float level = (float)((value & 0xFC00) >> 10);
					SendPercentageSensor(2, 1, 255, level, "BLOWER POWER");
					break;
				}
				case 0x03:
				{ 
					dictionary::iterator it = statesType.begin();
					for (; it != statesType.end(); it++)
					{
						if (((*it).first & value) && !((*it).first & m_LastState))
						{
							SendTextSensor(3, 1, 255, (*it).second, "State");
						}
						else
							if (!((*it).first & value) && ((*it).first & m_LastState))
							{
								SendTextSensor(3, 1, 255, "End - " + (*it).second, "State");
							}
					}
					m_LastState = value;

					float level = (float)((value & 0xFC00) >> 10);
					SendPercentageSensor(3, 1, 255, level, "Fuel Level");
					break;
				}

				case 0x1C:
				case 0x1D:
				{
					char name[20];
					sprintf(name, "C.H. %d Temperature", i - 0x1C + 1);

					float temp = (float)value;
					if ((value & 0x8000) == 0x8000)
					{
						temp = (float)((value & 0x0FFF) * 0.2);
					}
					m_isWeatherWork[i - 0x1C] = (value & 0x8000) == 0x8000;
					SendSetPointSensor(i, 1, 1, temp, name);
					break;
				}

				case 0x1E:
				{
					SendSetPointSensor(0x1E, 1, 1, (float)value, "H.W.U. Temperature");
					break;
				}

				case 0x1F:
				case 0x20:
				{
					char name[20];
					sprintf(name, "Lowering C.H. %d", i - 0x1F + 1);

					if (m_isSensorExists[i - 0x1F])
					{
						float temp = (float)((value & 0x0FFF) * 0.2);
						SendSetPointSensor(i, 1, 1, temp, name);
					}
					else
					{
						//SendGeneralSwitchSensor(i, 255, value, name, 1); // TODO - send level (dimmer)
					}					
					break;
				}

				case 0x21:
				{
					dictionary::iterator it = quickAccessType.begin();
					for (; it != quickAccessType.end(); it++)
					{
						if (((*it).first & value) && !((*it).first & m_LastQuickAccess))
						{
							SendGeneralSwitchSensor(0x21, 255, true, (*it).second.c_str(), (*it).first);
						}
						else
							if ((!((*it).first & value) && ((*it).first & m_LastQuickAccess)) || firstTime)
							{
								SendGeneralSwitchSensor(0x21, 255, false, (*it).second.c_str(), (*it).first);
							}
					}
					m_LastQuickAccess = value;
					break;
				}
				default: break;
				}
			}
		}

	}
	else
	{
		_log.Log(LOG_ERROR, "MultiFun: Receive info about registers failed");
	}
}

// return length of answer (-1 = error)
int MultiFun::SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, bool write)
{
	if (!ConnectToDevice())
	{
		return -1;
	}

	boost::lock_guard<boost::mutex> lock(m_mutex);

	unsigned char databuffer[BUFFER_LENGHT];
	int ret = -1;

	if (m_socket->write((char*)cmd, cmdLength) != cmdLength)
	{
		_log.Log(LOG_ERROR, "MultiFun: Send command failed");
		DestroySocket();
		return -1;
	}

	bool bIsDataReadable = true;
	m_socket->canRead(&bIsDataReadable, 3.0f);
	if (bIsDataReadable)
	{
		memset(databuffer, 0, BUFFER_LENGHT);
		ret = m_socket->read((char*)databuffer, BUFFER_LENGHT, false);
	}

	if ((ret <= 0) || (ret >= BUFFER_LENGHT))
	{
		_log.Log(LOG_ERROR, "MultiFun: no data received");
		return -1;
	}

	if (ret > 8)
	{
		if (cmd[0] == databuffer[0] && cmd[1] == databuffer[1] && cmd[2] == databuffer[2] && cmd[3] == databuffer[3] && cmd[6] == databuffer[6])
		{
			if (cmd[7] == databuffer[7])
			{
				unsigned int answerLength = 0;
				for (int i = 0; i < ret - 8; i++) // skip prefix
				{
					answer[i] = databuffer[i + 8];
				}
				answerLength = ret - 8; // answer = frame - prefix

				if ((int)databuffer[4] * 256 + (int)databuffer[5] == answerLength + 2)
				{
					if (write)
					{
						if (cmd[8] == databuffer[8] && cmd[9] == databuffer[9] && cmd[10] == databuffer[10] && cmd[11] == databuffer[11])
						{
							return answerLength;
						}
						else
						{
							_log.Log(LOG_ERROR, "MultiFun: bad response after write");
						}
					}
					else
					{
						return answerLength;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "MultiFun: bad size of frame");
				}
			}
			else
				if (cmd[7] + 0x80 == databuffer[7])
				{
					if (databuffer[8] >= 1 && databuffer[8] <= 4)
					{
						_log.Log(LOG_ERROR, "MultiFun: Receive error (%s)", errors[databuffer[8] - 1].c_str());
					}
					else
					{
						_log.Log(LOG_ERROR, "MultiFun: Receive unknown error");
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "MultiFun: Receive error (unknown function code)");
				}
		}
		else
		{
				_log.Log(LOG_ERROR, "MultiFun: received bad frame prefix");
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "MultiFun: received frame is too short.");
		DestroySocket();
	}

	return -1;
}
