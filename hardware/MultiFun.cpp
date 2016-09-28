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

#ifdef _DEBUG
	#define DEBUG_MultiFun
#endif

#define BUFFER_LENGHT 100
#define MULTIFUN_POLL_INTERVAL 30

#define round(a) ( int ) ( a + .5 )

typedef struct sensorType {
	std::string name;
	float div;
} Model;

#define sensorsCount 12

// ISO2UTF8
// π        Ê       Í       ≥       Ò       Û       ú       ü       ø       •      ∆                £       —       ”       å       è       Ø
// \xC4\x85\xC4\x87\xC4\x99\xC5\x82\xC5\x84\xC3\xB3\xC5\x9B\xC5\xBA\xC5\xBC\xC4\x84\xC4\x86\xC4\x98\xC5\x81\xC5\x83\xC3\x93\xC5\x9A\xC5\xB9\xC5\xBB

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

static std::string errors[4] =
{
	{"Bledny kod funkcji"},
	{"Bledny adres rejestru"},
	{"Bledna ilosc rejestrow"},
	{"Blad serwera"}
};

MultiFun::MultiFun(const int ID, const std::string &IPAddress, const unsigned short IPPort) :
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_stoprequested(false),
	m_socket(NULL)
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
#ifdef DEBUG_MultiFun
			_log.Log(LOG_STATUS, "MultiFun: fetching changed data");
#endif
		}
	}
}

bool MultiFun::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *output = reinterpret_cast<const tRBUF*>(pdata);

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
	char cmd[12];
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

	int ret = SendCommand(cmd, 12, buffer);
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
				float temp = (buffer[i * 2 + 1] * 256 + buffer[i * 2 + 2]) / sensors[i].div;
				SendTempSensor(i, 255, temp, sensors[i].name);
			}
		}

	}
	else
	{
		_log.Log(LOG_ERROR, "MultiFun: Receive info about output %d failed", 1);
	}
}

int MultiFun::SendCommand(const char* cmd, const unsigned int cmdLength, unsigned char *answer)
{
	if (!ConnectToDevice())
	{
		return -1;
	}

	boost::lock_guard<boost::mutex> lock(m_mutex);

	char databuffer[BUFFER_LENGHT];
	int ret;

	if (m_socket->write(cmd, cmdLength) != cmdLength)
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
			ret = m_socket->read(databuffer, BUFFER_LENGHT, false);
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
					return answerLength;
				}
				else
				{
					_log.Log(LOG_ERROR, "MultiFun: bad size of frame");
					return -1;
				}
			}
			else
				if (cmd[0] + 0x80 == databuffer[7])
				{
					if (databuffer[8] >= 1 && databuffer[8] <= 4)
					{
						_log.Log(LOG_ERROR, "MultiFun: Receive error (%s)", errors[databuffer[8]]);
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
				return -1;
		}
	}
	else
	{
		_log.Log(LOG_ERROR, "MultiFun: received frame is too short.");
		DestroySocket();
		return -1;
	}

}