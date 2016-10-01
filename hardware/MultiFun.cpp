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
#define MULTIFUN_POLL_INTERVAL 10

#define round(a) ( int ) ( a + .5 )

typedef struct sensorType {
	std::string name;
	float div;
} Model;

#define sensorsCount 12
#define registersCount 34

// ISO2UTF8
// π        Ê       Í       ≥       Ò       Û       ú       ü       ø       •      ∆                £       —       ”       å       è       Ø
// \xC4\x85\xC4\x87\xC4\x99\xC5\x82\xC5\x84\xC3\xB3\xC5\x9B\xC5\xBA\xC5\xBC\xC4\x84\xC4\x86\xC4\x98\xC5\x81\xC5\x83\xC3\x93\xC5\x9A\xC5\xB9\xC5\xBB

typedef std::map<int, std::string> dictionary;

static dictionary alarmsType = boost::assign::map_list_of
(0x0001, "STOP KOTLA ñ NIEUDANE ROZPALANIE")
(0x0004, "PRZEGRZANIE KOTLA")
(0x0010, "ZGASLO W KOTLE")
(0x0080, "USZKODZONY CZUJNIK KOTLA")
(0x0100, "USZKODZONY CZUJNIK PODAJNIKA")
(0x0200, "CZUJNIK SPALIN")
(0x0400, "NIEPOWODZENIE ñ BLOKADA PRACY");


static dictionary warningsType = boost::assign::map_list_of
(0x0001, "Brak czujnika zewnetrznego")
(0x0002, "Brak czujnika pokojowego nr 1")
(0x0004, "Niewlasciwa wersja zasilacza")
(0x0008, "Brak czujnika powrotu")
(0x0010, "Brak czujnika pokojowego nr 2")
(0x0020, "Otwarta klapa")
(0x0040, "Zadzialalo zabezpieczenie termiczne(termik)");

static dictionary devicesType = boost::assign::map_list_of
(0x0001, "Pompa C.O.1")
(0x0002, "Pompa C.O.2")
(0x0004, "Pompa przewalowa")
(0x0008, "Pompa C.W.U.")
(0x0010, "Pompa cyrkulacyjna")
(0x0020, "Pompa bufora")
(0x0040, "Mieszacz C.O.1 Zam.")
(0x0080, "Mieszacz C.O.1 Otw.")
(0x0100, "Mieszacz C.O.2 Zam.")
(0x0200, "Mieszacz C.O.2 Otw.");
//Wentylator moc 6 bitÛw	MSB	WartoúÊ od 0 ñ 100 przesy≥ana na	najstarszych 6 bitach

static dictionary statesType = boost::assign::map_list_of
(0x0001, "Stop")
(0x0002, "Rozpalanie")
(0x0004, "Palenie")
(0x0008, "Podtrzymanie")
(0x0010, "Wygaszanie");
//Poziom paliwa 6 bitÛw	MSB	WartoúÊ od 0 ñ 100 przesy≥ana na	najstarszych 6 bitach

static sensorType sensors[sensorsCount] =
{
	{ "Temperatura zewnetrzna", 10.0 },
	{ "Temperatura pokojowa 1", 10.0 },
	{ "Temperatura pokojowa 2", 10.0 },
	{ "Temperatura powrotu kotla", 10.0 },
	{ "Temperatura obiegu C.O.1", 10.0 },
	{ "Temperatura obiegu C.O.2", 10.0 },
	{ "Temperatura C.W.U.", 10.0 },
	{ "Temperatura zaru", 1.0 },
	{ "Temperatura spalin", 10.0 },
	{ "Temperatura modulu", 10.0 },
	{ "Temperatura zasilania kotla", 10.0 },
	{ "Temperatura podajnika", 10.0 }
};

static dictionary quickAccessType = boost::assign::map_list_of
	(0x0001, "Prysznic")
	(0x0002, "Party")
	(0x0004, "Komfort")
	(0x0008, "Wietrzenie")
	(0x0010, "Antyzamarzanie");

static std::string errors[4] =
{
	"Bledny kod funkcji",
	"Bledny adres rejestru",
	"Bledna ilosc rejestrow",
	"Blad serwera"
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
}

MultiFun::~MultiFun()
{
	_log.Log(LOG_STATUS, "MultiFun: Destroy instance");
}

bool MultiFun::StartHardware()
{
#ifdef DEBUG_MultiFun
	_log.Log(LOG_STATUS, "MultiFuna: Start hardware");
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
				change = m_LastQuickAccess | (1 << (general->unitcode - 1));
			}
			else
			{ 
				change = m_LastQuickAccess & ~(1 << (general->unitcode - 1));
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
		cmd[14] = therm->temp;

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
				if (buffer[i * 2 + 1] != 254)
				{
					float temp = (buffer[i * 2 + 1] * 256 + buffer[i * 2 + 2]) / sensors[i].div;
					SendTempSensor(i, 255, temp, sensors[i].name);
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
								SendTextSensor(1, 0, 255, "Koniec - " + (*it).second, "Alarms");
							}
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
								SendTextSensor(1, 1, 255, "Koniec - " + (*it).second, "Warnings");
							}
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

					float level = (value & 0xFC00) >> 10;
					SendPercentageSensor(2, 1, 255, level, "Fan power");
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
								SendTextSensor(3, 1, 255, "Koniec - " + (*it).second, "State");
							}
					}
					m_LastState = value;

					float level = (value & 0xFC00) >> 10;
					SendPercentageSensor(3, 1, 255, level, "Level");
					break;
				}

				case 0x1C:
				case 0x1D:
				{
					float temp = value;
					if (value & 0x8000 == 0x8000)
					{
						temp = (value & 0x0FFF) * 0.2;
					}
					char name[20];
					sprintf(name, "Temperatura CO %d", i - 0x1C + 1);
					SendSetPointSensor(i, 1, 1, temp, name);
					break;
				}

				case 0x1E:
				{
					SendSetPointSensor(0x1E, 1, 1, value, "Temperatura CWU");
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

int MultiFun::SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, bool write)
{
	if (!ConnectToDevice())
	{
		return -1;
	}

	boost::lock_guard<boost::mutex> lock(m_mutex);

	unsigned char databuffer[BUFFER_LENGHT];
	int ret;

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
		if (memset(databuffer, 0, BUFFER_LENGHT) > 0)
		{
			ret = m_socket->read((char*)databuffer, BUFFER_LENGHT, false);
		}
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
						_log.Log(LOG_ERROR, "MultiFun: Receive error (%s)", errors[databuffer[8]].c_str());
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