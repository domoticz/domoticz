#include "stdafx.h"
#include "EvohomeSerial.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"

CEvohomeSerial::CEvohomeSerial(const int ID, const std::string &szSerialPort, const int baudrate, const std::string &UserContID) :
CEvohomeRadio(ID, UserContID)
{
    if(baudrate!=0)
	{
	  m_iBaudRate=baudrate;
	}
	else
	{
	  // allow migration of hardware created before baud rate was configurable
	  m_iBaudRate=115200;
	}
	m_szSerialPort=szSerialPort;
}

bool CEvohomeSerial::StopHardware()
{
	m_stoprequested=true;
	if (m_thread != NULL)
		m_thread->join();
	// Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
	sleep_milliseconds(10);
	terminate();
	m_bIsStarted=false;
	if(m_bDebug && m_pEvoLog)
	{
		delete m_pEvoLog;
		m_pEvoLog=NULL;
	}
	return true;
}

bool CEvohomeSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		_log.Log(LOG_STATUS,"evohome serial: Opening serial port: %s@%d", m_szSerialPort.c_str(), m_iBaudRate);
		open(m_szSerialPort,m_iBaudRate);
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"evohome serial: Error opening serial port: %s", m_szSerialPort.c_str());
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"evohome serial: Error opening serial port!!!");
		return false;
	}
	m_nBufPtr=0;
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CEvohomeSerial::ReadCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CEvohomeSerial::ReadCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	try
	{
		//_log.Log(LOG_NORM,"evohome: received %ld bytes",len);
		if (!HandleLoopData(data,len))
		{
			//error in data, try again...
			terminate();
		}
	}
	catch (...)
	{

	}
}

void CEvohomeSerial::Do_Work()
{
	boost::system_time stLastRelayCheck(boost::posix_time::min_date_time);
	int nStartup=0;
	int nStarts = 0;
	int sec_counter = 0;
	bool startup = true;
	std::vector<std::vector<std::string> > result;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;
		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}
		if (m_stoprequested)
			break;

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"evohome serial: serial setup retry in %d seconds...", EVOHOME_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=EVOHOME_RETRY_DELAY)
			{
				m_retrycntr=0;
				OpenSerialDevice();
				if (isOpen())//do some startup stuff
				{
					if(GetControllerID()!=0)//can't proceed without it
						RequestCurrentState();
				}
			}
		}
		else
		{
			Send();//FIXME afaik if there is a collision then we should increase the back off period exponentially if it happens again
			if(nStartup<300)//if we haven't got zone names in 300s we auto init them (zone creation blocked until we have something)...
			{
				nStartup++;
				if(nStartup==300)
				{
					if (startup)
					{
						InitControllerName();
						InitZoneNames();
						RequestZoneNames();

						if (GetControllerID() == 0xFFFFFF)  //Check whether multiple controllers have been detected
						{
							uint8_t MultiControllerCount = 0;
							for (uint8_t i = 0; i < 5; i++)
								if (MultiControllerID[i] != 0)
									MultiControllerCount++;
							if (MultiControllerCount > 1) // If multiple controllers detected then stop and user required to set controller ID on hardware settings page
							{
								_log.Log(LOG_ERROR, "evohome serial: multiple controllers detected!  Please set controller ID in hardware settings.");
								StopHardware();
							}
							else if (MultiControllerCount == 1) // If only 1 controller detected then proceed, otherwise continue searching for controller
							{
								Log(true, LOG_STATUS, "evohome serial: Multi-controller check passed. Controller ID: 0x%x", MultiControllerID[0]);
								SetControllerID(MultiControllerID[0]);
								startup = false;
							}
							RequestCurrentState(); //and also get startup info as this should only happen during initial setup
						}
						else
							startup = false;
					}
					else//Request each individual zone temperature every 300s as the controller omits multi-room zones
					{
						uint8_t nZoneCount = GetZoneCount();
						for (uint8_t i = 0; i < nZoneCount; i++)
							RequestZoneTemp(i);
						RequestDHWTemp();  // Request DHW temp from controller as workaround for being unable to identify DeviceID
					}
					if (AllSensors == false) // Check whether individual zone sensors has been activated
					{
						result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Type==%d) AND (Unit >= 13) AND (Unit <= 24)", m_HwdID, (int)pTypeEvohomeZone);
						if (!result.empty())
							AllSensors = true;
						// Check if the dummy sensor exists and delete
						result = m_sql.safe_query("SELECT HardwareID, DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID == 'FFFFFF') AND (Type==%d) AND (Unit == 13)", m_HwdID, (int)pTypeEvohomeZone);
						if (!result.empty())
							m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='FFFFFF' AND (Type==%d) AND (Unit == 13))", m_HwdID, (int)pTypeEvohomeZone);
					}
					if (nStarts < 20)
						nStarts++;
					else if (nStarts == 20) // After 1h all devices should have been detected so re-request zone names and trigger device naming
					{
						RequestZoneNames();
						nStarts++;
					}
					nStartup = 0;
				}
			}
			boost::lock_guard<boost::mutex> l(m_mtxRelayCheck);
			if(!m_RelayCheck.empty() && GetGatewayID()!=0)
			{
				CheckRelayHeatDemand();
				if((boost::get_system_time()-stLastRelayCheck)>boost::posix_time::seconds(604)) //not sure if it makes a difference but avg time is about 604-605 seconds but not clear how reference point derived - seems steady once started
				{
					SendRelayKeepAlive();
					stLastRelayCheck=boost::get_system_time();
				}
			}
		}
	}
	_log.Log(LOG_STATUS,"evohome serial: Serial Worker stopped...");
}

void CEvohomeSerial::Do_Send(std::string str)
{
    write(str.c_str(), str.length());
}
