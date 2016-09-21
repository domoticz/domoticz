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

#ifdef _DEBUG
	#define DEBUG_MultiFun
#endif

#define MULTIFUN_POLL_INTERVAL 1

#define round(a) ( int ) ( a + .5 )

MultiFun::MultiFun(const int ID, const std::string &IPAddress, const unsigned short IPPort) :
	m_IPPort(IPPort),
	m_IPAddress(IPAddress),
	m_stoprequested(false)
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

