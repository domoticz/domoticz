/************************************************************************
*
Legrand MyHome / OpenWebNet Interface board driver for Domoticz (with LAN interface)
Date: 24-01-2016
Written by: Stéphane Lebrasseur

Date: 04-11-2016
Update by: Matteo Facchetti

Date: 13-09-2017
Update by: Marco Olivieri - Olix81 -

License: Public domain


 ************************************************************************/
#include "stdafx.h"
#include "OpenWebNetTCP.h"
#include "openwebnet/bt_openwebnet.h"

#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "csocket.h"

#include <string.h>
#include "hardwaretypes.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"

#define OPENWEBNET_HEARTBEAT_DELAY      1
#define OPENWEBNET_STATUS_NB_HEARTBEAT  600
#define OPENWEBNET_RETRY_DELAY          30
#define OPENWEBNET_POLL_INTERVAL        1000
#define OPENWEBNET_BUFFER_SIZE          1024
#define OPENWEBNET_SOCKET_SUCCESS       0

#define OPENWEBNET_GROUP_ID				0x00008000

#define OPENWEBNET_AUTOMATION					"AUTOMATION"
#define OPENWEBNET_LIGHT						"LIGHT"
#define OPENWEBNET_TEMPERATURE					"TEMPERATURE"
#define OPENWEBNET_BURGLAR_ALARM_SENSOR			"ALARM SENSOR ZONE"
#define OPENWEBNET_BURGLAR_ALARM_BATTERY		"ALARM BATTERY"
#define OPENWEBNET_BURGLAR_ALARM_NETWORK		"ALARM NETWORK"
#define OPENWEBNET_BURGLAR_ALARM_SYS_STATUS		"ALARM SYSTEM STATUS"
#define OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT "ALARM SYSTEM ENGAGEMENT"
#define OPENWEBNET_CENPLUS            			"CEN PLUS"
#define OPENWEBNET_AUXILIARY					"AUXILIARY"
#define OPENWEBNET_DRY_CONTACT					"DRYCONTACT"


/**
    Create new hardware OpenWebNet instance
**/
COpenWebNetTCP::COpenWebNetTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &ownPassword) : m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_ownPassword = ownPassword;
	m_heartbeatcntr = OPENWEBNET_HEARTBEAT_DELAY;
	m_pStatusSocket = NULL;
}

/**
    destroys hardware OpenWebNet instance
**/
COpenWebNetTCP::~COpenWebNetTCP(void)
{
}

/**
    Start Hardware OpneWebNet Monitor/Worker Service
**/
bool COpenWebNetTCP::StartHardware()
{
	m_stoprequested = false;
	m_bIsStarted = true;
	mask_request_status = 0x1; // Set scan all devices

	//Start monitor thread
	m_monitorThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWebNetTCP::MonitorFrames, this)));

	//Start worker thread
	if (m_monitorThread != NULL) {
		m_heartbeatThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWebNetTCP::Do_Work, this)));
	}

	return (m_monitorThread!=NULL && m_heartbeatThread!=NULL);
}

/**
    Stop Hardware OpenWebNet Monitor/Worker Service
**/
bool COpenWebNetTCP::StopHardware()
{
	m_stoprequested = true;

    _log.Log(LOG_STATUS, "COpenWebNetTCP: StopHardware");

	try {
		if (m_monitorThread)
		{
		    m_monitorThread->join();
		}
		if (m_heartbeatThread)
		{
		    m_heartbeatThread->join();
		}
	}
	catch (...)
	{
		//Don't throw from a Stop command
	}

	if (isStatusSocketConnected())
	{
		try {
			disconnect();  // disconnet socket if present
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}

	m_bIsStarted = false;
	return true;
}

/**
    Close and delete the socket
**/
void COpenWebNetTCP::disconnect()
{
	if (m_pStatusSocket != NULL)
    {
        _log.Log(LOG_STATUS, "COpenWebNetTCP: disconnect");
        if (m_pStatusSocket->getState() != csocket::CLOSED)
            m_pStatusSocket->close();
		delete m_pStatusSocket;
		m_pStatusSocket = NULL;
	}
}


/**
   Check socket connection
**/
bool COpenWebNetTCP::isStatusSocketConnected()
{
	return m_pStatusSocket!=NULL && m_pStatusSocket->getState() == csocket::CONNECTED;
};


/**
   Calculate 'nonce-hash' authentication
**/
uint32_t COpenWebNetTCP::ownCalcPass(string password, string nonce)
{
    uint32_t msr = 0x7FFFFFFF;
    uint32_t m_1 = (uint32_t)0xFFFFFFFF;
    uint32_t m_8 = (uint32_t)0xFFFFFFF8;
    uint32_t m_16 = (uint32_t)0xFFFFFFF0;
    uint32_t m_128 = (uint32_t)0xFFFFFF80;
    uint32_t m_16777216 = (uint32_t)0xFF000000;
    bool flag = true;
    uint32_t num1 = 0;
    uint32_t num2 = 0;
    uint32_t numx = 0;
    uint32_t length = 0;

    uint32_t idx;

    for(idx = 0; idx < nonce.length(); idx++)
    {
        if ((nonce[idx] >= '1') && (nonce[idx] <= '9'))
        {
            if (flag)
            {
                num2 = (uint32_t)atoi(password.c_str());
                flag = false;
            }
        }

        switch (nonce[idx])
        {
        case '1':
            num1 = num2 & m_128;
            num1 = num1 >> 1;
            num1 = num1 & msr;
            num1 = num1 >> 6;
            num2 = num2 << 25;
            num1 = num1 + num2;
            break;
        case '2':
            num1 = num2 & m_16;
            num1 = num1 >> 1;
            num1 = num1 & msr;
            num1 = num1 >> 3;
            num2 = num2 << 28;
            num1 = num1 + num2;
            break;
        case '3':
            num1 = num2 & m_8;
            num1 = num1 >> 1;
            num1 = num1 & msr;
            num1 = num1 >> 2;
            num2 = num2 << 29;
            num1 = num1 + num2;
            break;
        case '4':
            num1 = num2 << 1;
            num2 = num2 >> 1;
            num2 = num2 & msr;
            num2 = num2 >> 30;
            num1 = num1 + num2;
            break;
        case '5':
            num1 = num2 << 5;
            num2 = num2 >> 1;
            num2 = num2 & msr;
            num2 = num2 >> 26;
            num1 = num1 + num2;
            break;
        case '6':
            num1 = num2 << 12;
            num2 = num2 >> 1;
            num2 = num2 & msr;
            num2 = num2 >> 19;
            num1 = num1 + num2;
            break;
        case '7':
            num1 = num2 & 0xFF00;
            num1 = num1 + (( num2 & 0xFF ) << 24 );
            num1 = num1 + (( num2 & 0xFF0000 ) >> 16 );
            num2 = num2 & m_16777216;
            num2 = num2 >> 1;
            num2 = num2 & msr;
            num2 = num2 >> 7;
            num1 = num1 + num2;
            break;
        case '8':
            num1 = num2 & 0xFFFF;
            num1 = num1 << 16;
            numx = num2 >> 1;
            numx = numx & msr;
            numx = numx >> 23;
            num1 = num1 + numx;
            num2 = num2 & 0xFF0000;
            num2 = num2 >> 1;
            num2 = num2 & msr;
            num2 = num2 >> 7;
            num1 = num1 + num2;
            break;
        case '9':
            num1 = ~num2;
            break;
        default:
            num1 = num2;
            break;
        }
        num2 = num1;
    }

    return (num1 & m_1);
}

/**
    Perform nonce-hash authentication
**/

bool COpenWebNetTCP:: nonceHashAuthentication(csocket *connectionSocket)
{
    char databuffer[OPENWEBNET_BUFFER_SIZE];
    memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
    int read = connectionSocket->read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseNonce(string(databuffer, read));
    if (responseNonce.IsPwdFrame())
    {
        stringstream frame;
        uint32_t ownHash;

        if (!m_ownPassword.length())
        {
            _log.Log(LOG_STATUS, "COpenWebNetTCP: no password set for a unofficial bticino gateway");
            return false;
        }

        /** calculate nonce-hash **/
        ownHash = ownCalcPass(m_ownPassword, responseNonce.Extract_who());
        /** write frame with nonce-hash **/
        frame << "*#";
        frame << ownHash;
        frame << "##";

        int bytesWritten = connectionSocket->write(frame.str().c_str(), frame.str().length());
        if (bytesWritten != frame.str().length()) {
            _log.Log(LOG_ERROR, "COpenWebNetTCP: partial write");
        }

        /** Open password for test **/
        memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	    read = connectionSocket->read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	    bt_openwebnet responseNonce2(string(databuffer, read));
        if (responseNonce2.IsOKFrame()) return true;
        _log.Log(LOG_ERROR, "COpenWebNetTCP: authentication ERROR!");
        return false;
    }
    else if (responseNonce.IsOKFrame())
    {
        return true;
    }
    _log.Log(LOG_STATUS, "COpenWebNetTCP: ERROR_FRAME? %d", responseNonce.frame_type);
    return false;
}

/**
   Connection to the gateway OpenWebNet
**/
csocket* COpenWebNetTCP::connectGwOwn(const char *connectionMode)
{
	if (m_szIPAddress.size() == 0 || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		_log.Log(LOG_ERROR, "COpenWebNetTCP: Cannot connect to gateway, empty  IP Address or Port");
		return NULL;
	}

    /* new socket for command and session connection */
    csocket *connectionSocket = new csocket();

	connectionSocket->connect(m_szIPAddress.c_str(), m_usIPPort);
	if (connectionSocket->getState() != csocket::CONNECTED)
	{
		_log.Log(LOG_ERROR, "COpenWebNetTCP: Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		disconnect();  // disconnet socket if present
		return NULL;
	}

	char databuffer[OPENWEBNET_BUFFER_SIZE];
	memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	int read = connectionSocket->read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseSession(string(databuffer, read));
	if (!responseSession.IsOKFrame())
    {
		_log.Log(LOG_STATUS, "COpenWebNetTCP: failed to begin session, NACK received (%s:%d)-> %s", m_szIPAddress.c_str(), m_usIPPort, databuffer);
        disconnect();  // disconnet socket if present
		return NULL;
	}

    int bytesWritten = connectionSocket->write(connectionMode, strlen(connectionMode));
	if (bytesWritten != strlen(connectionMode)) {
		_log.Log(LOG_ERROR, "COpenWebNetTCP: partial write");
	}

    if (!nonceHashAuthentication(connectionSocket)) return NULL;

    return connectionSocket;
}

/**
    Thread Monitor: get update from the OpenWebNet gateway and add new devices if necessary
**/
void COpenWebNetTCP::MonitorFrames()
{
	while (!m_stoprequested)
	{
	    if (!isStatusSocketConnected())
        {
            if (m_stoprequested) break;
            disconnect();  // disconnet socket if present
            time_t atime=time(NULL);
			if ((atime%OPENWEBNET_RETRY_DELAY)==0)
			{
			    if ((m_pStatusSocket = connectGwOwn(OPENWEBNET_EVENT_SESSION)))
                {
                    // Monitor session correctly open
                    _log.Log(LOG_STATUS, "COpenWebNetTCP: Monitor session connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
                    sOnConnected(this);
                }
                else
                {
                    _log.Log(LOG_STATUS, "COpenWebNetTCP: TCP/IP monitor not connected, retrying in %d seconds...", OPENWEBNET_RETRY_DELAY);
                    sleep_seconds(1);
                }
			}
        }
        else
		{
		    // Connected
		    bool bIsDataReadable = true;
            m_pStatusSocket->canRead(&bIsDataReadable, 3.0f);
            if (bIsDataReadable)
            {
                char data[OPENWEBNET_BUFFER_SIZE];
                memset(data, 0, OPENWEBNET_BUFFER_SIZE);
                int bread = m_pStatusSocket->read(data, OPENWEBNET_BUFFER_SIZE, false);

                if (m_stoprequested) break;

                if ((bread == 0) || (bread<0)) {
                    _log.Log(LOG_ERROR, "COpenWebNetTCP: TCP/IP monitor connection closed!");
                    disconnect();  // disconnet socket if present
                }
                else
                {
                    boost::lock_guard<boost::mutex> l(readQueueMutex);
                    vector<bt_openwebnet> responses;
                    ParseData(data, bread, responses);

                    for (vector<bt_openwebnet>::iterator iter = responses.begin(); iter != responses.end(); iter++) {
                        if (iter->IsNormalFrame() || iter->IsMeasureFrame())
                        {
                            _log.Log(LOG_STATUS, "COpenWebNetTCP: received=%s", bt_openwebnet::frameToString(*iter).c_str());
                            UpdateDeviceValue(iter);
                        }
                        //else
                        //    _log.Log(LOG_ERROR, "COpenWebNetTCP: SKIPPED FRAME=%s", frameToString(*iter).c_str());
                    }
                }
			}
			if (m_stoprequested) break;
		}
	}
	_log.Log(LOG_STATUS, "COpenWebNetTCP: TCP/IP monitor worker stopped...");
}

/**
    Insert/Update temperature device
**/
void COpenWebNetTCP::UpdateTemp(const int who, const int where, float fval, const int BatteryLevel, const char *devname)
{
	//zone are max 99,, every zone can have 8 slave sensor. Slave sensor address. YZZ: y as slave address (1-8) ,zz zone number (1-99)
    int cnode =  ((who << 12) & 0xF000) | (where & 0xFFF); 
    SendTempSensor(cnode, BatteryLevel, fval, devname);
}


void COpenWebNetTCP::UpdateSetPoint(const int who, const int where, float fval, const char *devname)
{
	int cnode = ((who << 12) & 0xF000) | (where & 0xFF); //setpoint zone (1 - 99)
	SendSetPointSensor((who & 0xFF), 0, (cnode & 0xFF), fval, devname); 
}

/**
    Insert/Update blinds device
**/
void COpenWebNetTCP::UpdateBlinds(const int who, const int where, const int Command, int iInterface, const int BatteryLevel, const char *devname)
{
    //make device ID
    unsigned char ID1 = (unsigned char)((who & 0xFF00) >> 8);
	unsigned char ID2 = (unsigned char)(who & 0xFF);
	unsigned char ID3 = (unsigned char)((where & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)where & 0xFF;

	//interface id (bus identifier)
	int unit = iInterface;

    char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",
                           m_HwdID, szIdx, unit);
	if (!result.empty())
	{
        //check if we have a change, if not do not update it
        int nvalue = atoi(result[0][0].c_str());
        if (Command == nvalue) return;
	}
	else
    {
        // Special insert to set SwitchType = STYPE_VenetianBlindsEU
        // so we have stop button!
        m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Name, Used) "
                         "VALUES (%d,'%s', %d,%d,%d,%d,'%q',0)",
                         m_HwdID, szIdx, unit, pTypeGeneralSwitch, sSwitchBlindsT1, STYPE_VenetianBlindsEU, devname);
    }

    _tGeneralSwitch gswitch;
    gswitch.subtype = sSwitchBlindsT1;
    gswitch.id = (((int32_t)who << 16) & 0xFF0000) | (where & 0xFFFF);
    gswitch.unitcode = 0;
    gswitch.cmnd = Command;
    gswitch.level = 100;
    gswitch.battery_level = BatteryLevel;
    gswitch.rssi = 12;
    gswitch.seqnbr = 0;
    sDecodeRXMessage(this, (const unsigned char *)&gswitch, devname, BatteryLevel);
}

/**
	Insert/Update Alarm system and sensor
**/
void COpenWebNetTCP::UpdateAlarm(const int who, const int where, const int Command, const char *sCommand, int iInterface, const int BatteryLevel, const char *devname)
{
	 //make device ID
    unsigned char ID1 = (unsigned char)((who & 0xFF00) >> 8);
	unsigned char ID2 = (unsigned char)(who & 0xFF);
	unsigned char ID3 = (unsigned char)((where & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)where & 0xFF;

	//interface id (bus identifier)
	int unit = iInterface;

    char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
	

    std::vector<std::vector<std::string> > result;
    string strdev;
    //check first Insert
    result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",
     												m_HwdID, szIdx, unit);  
    if (result.empty())
    {
      m_sql.UpdateValue(m_HwdID, szIdx, unit, pTypeGeneral, sTypeAlert, 12, 255, Command,sCommand,strdev);
      m_sql.safe_query("UPDATE DeviceStatus SET Name='%s' WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",devname, m_HwdID,szIdx,unit);//can't update from devname ???    
      return;
    }                       
    	
        //check if we have a change, if not do not update it
        int nvalue = atoi(result[0][0].c_str());
       
        if (Command == -1 || nvalue == Command) return; // update not necessary
   	 m_sql.UpdateValue(m_HwdID, szIdx, unit, pTypeGeneral, sTypeAlert, 12, 255, Command,sCommand,strdev);

}

/**
	Insert/Update  CEN PLUS
**/
void COpenWebNetTCP::UpdateCenPlus(const int who, const int where, const int Command, const int iAppValue, int iInterface, const int BatteryLevel, const char *devname)
{
	 //make device ID
    unsigned char ID1 = (unsigned char)((who & 0xFF00) >> 8);
	unsigned char ID2 = (unsigned char)(who & 0xFF);
	unsigned char ID3 = (unsigned char)((where+(iAppValue * 2) + (iInterface *3) & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)(where+(iAppValue * 2) + (iInterface *3)) & 0xFF;

	//interface id (bus identifier)
	int unit = iInterface;

    char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
	
	std::vector<std::vector<std::string> > result;
    string strdev;
    //check first Insert
    result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",
     												m_HwdID, szIdx, unit);
     	
     												
    if (result.empty())		
    {
      m_sql.UpdateValue(m_HwdID, szIdx, unit, pTypeGeneralSwitch, sSwitchLightT1, 12, 255, 0,strdev);
      m_sql.safe_query("UPDATE DeviceStatus SET Name='%s'  WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",devname, m_HwdID,szIdx,unit); 
      return;
    }
    
    //check if we have a change, if not do not update it
    //int nvalue = atoi(result[0][0].c_str());
    // if (nvalue == Command)return; // update not necessary
    m_sql.UpdateValue(m_HwdID, szIdx, unit, pTypeGeneralSwitch, sSwitchLightT1, 12, 255, Command,strdev);	
}

/**
    Insert/Update  switch device
**/
void COpenWebNetTCP::UpdateSwitch(const int who, const int where, const int what, int iInterface, const int BatteryLevel, const char *devname, const int subtype)
{
    //make device ID
	unsigned char ID1 = (unsigned char)((who & 0xFF00) >> 8);
	unsigned char ID2 = (unsigned char)(who & 0xFF);
	unsigned char ID3 = (unsigned char)((where & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)(where & 0xFF);

	char szIdx[10];
	sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);

	int level = 0;
	int unit = iInterface;

    /* If Dimmer device, set level... */
	if (what > 1) level = what * 10; // what=0 mean 0% OFF, what=2 to 10 mean 20% to 100% ON

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)",
                            m_HwdID, szIdx, unit);
	if (!result.empty())
	{
        //check if we have a change, if not do not update it
	    int nvalue = atoi(result[0][0].c_str());

	    if ((what == 0) && (nvalue == gswitch_sOff)) return; // Already 0% OFF
	    if ((what == 1) && (nvalue == gswitch_sOn)) return; // Already ON
	    int slevel = atoi(result[0][1].c_str());
        if ((what > 1) && (nvalue != gswitch_sOff) && (slevel == level)) return; // Already ON/LEVEL at x%
    }
	else if (who == WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION)
	{
		// Special insert to set SwitchType = STYPE_Contact
		// so we have a correct contact device
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Name, Used) "
			"VALUES (%d,'%s', %d,%d,%d,%d,'%q',0)",
			m_HwdID, szIdx, unit, pTypeGeneralSwitch, subtype, STYPE_Contact, devname);
	}

    _tGeneralSwitch gswitch;
    gswitch.subtype = subtype;
    gswitch.id = (int32_t) (((int32_t)ID1 << 24) & 0xFF000000) | (((int32_t)ID2 << 16) & 0xFF0000) | (((int32_t)ID3 << 8) & 0xFF00) | ((int32_t)ID4 & 0xFF);
    gswitch.unitcode = 0;

    if (what == 0)
        gswitch.cmnd = gswitch_sOff;
    else
    {
        if (what > 1)
            gswitch.cmnd = gswitch_sSetLevel;
        else
            gswitch.cmnd = gswitch_sOn;
    }

    gswitch.level = level;
    gswitch.battery_level = BatteryLevel;
    gswitch.rssi = 12;
    gswitch.seqnbr = 0;
    sDecodeRXMessage(this, (const unsigned char *)&gswitch, devname, BatteryLevel);
}

/**
	Insert/Update device
**/
void COpenWebNetTCP::UpdateDeviceValue(vector<bt_openwebnet>::iterator iter)
{
	string who = iter->Extract_who();
	string where = iter->Extract_where();
	string what = iter->Extract_what();
	std::vector<std::string> whereParam = iter->Extract_whereParameters();
	std::vector<std::string> whatParam = iter->Extract_whatParameters();
	string dimension = iter->Extract_dimension();
	string value = iter->Extract_value(0);
	string sInterface = iter->Extract_interface();
	string devname, sCommand;
	int iAppValue, iWhere;

	switch (atoi(who.c_str())) {
		case WHO_LIGHTING:									// 1
			if (!iter->IsNormalFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s not normal frame! -> frame_type=%d", who.c_str(), iter->frame_type);
				return;
			}

			iAppValue = atoi(what.c_str());
			if (iAppValue == 1000) // What = 1000 (Command translation)
				iAppValue = atoi(whatParam[0].c_str());

			iWhere = atoi(where.c_str());

			devname = OPENWEBNET_LIGHT;		
			if ((!whereParam.empty()) && (iWhere == 0))
			{
				/* GROUP light device */
				iWhere = atoi(whereParam[0].c_str()) + OPENWEBNET_GROUP_ID;
				devname += " GROUP " + whereParam[0];
			}
			else if (iWhere < MAX_WHERE_AREA)
			{
				/* AREA light device */
				mask_request_status |= (0x1 << iWhere); // Gen or area, need a refresh devices status
				if (iWhere == 0)
					devname += " GEN " + where;
				else
					devname += " AREA " + where;
			} 
			else
			{
				/* Normal light device */
				devname += " " + where;
			}

			//pTypeGeneralSwitch, sSwitchLightT1
			UpdateSwitch(WHO_LIGHTING, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str(), sSwitchLightT1);
			break;
		case WHO_AUTOMATION:								// 2
			if (!iter->IsNormalFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s frame error!", who.c_str());
				return;
			}

			iAppValue = atoi(what.c_str());
			if (iAppValue == 1000) // What = 1000 (Command translation)
				iAppValue = atoi(whatParam[0].c_str());

			switch (iAppValue)
			{
			case AUTOMATION_WHAT_STOP:  // 0
				iAppValue = gswitch_sStop;
				break;
			case AUTOMATION_WHAT_UP:    // 1
				iAppValue = gswitch_sOff;
				break;
			case AUTOMATION_WHAT_DOWN:  // 2
				iAppValue = gswitch_sOn;
				break;
			default:
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s, What=%s invalid!", who.c_str(), what.c_str());
				return;
			}

			iWhere = atoi(where.c_str());

			devname = OPENWEBNET_AUTOMATION;
			if ((!whereParam.empty()) && (iWhere == 0))
			{
				/* GROUP automation device */
				iWhere = atoi(whereParam[0].c_str()) + OPENWEBNET_GROUP_ID;
				devname += " GROUP " + whereParam[0];
			}
			else if (iWhere < MAX_WHERE_AREA)
			{
				/* AREA automation device */
				mask_request_status |= (0x1 << iWhere); // Gen or area, need a refresh devices status
				if (iWhere == 0)
					devname += " GEN " + where;
				else
					devname += " AREA " + where;
			}
			else
			{
				/* Normal automation device */
				devname += " " + where;
			}

			//pTypeGeneralSwitch, sSwitchBlindsT1
			UpdateBlinds(WHO_AUTOMATION, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str());
			
			break;
		case WHO_TEMPERATURE_CONTROL:
			if (!iter->IsMeasureFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s frame error!", who.c_str());
				return;
			}
             // 4: this is a openwebnet termoregulation update/poll messagge, setup devname
			devname = OPENWEBNET_TEMPERATURE;
			devname += " " + where;
			switch (atoi(dimension.c_str()))
			{
			case TEMPERATURE_CONTROL_DIMENSION_TEMPERATURE:
				UpdateTemp(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), static_cast<float>(atof(value.c_str()) / 10.), 255, devname.c_str());
				break;
			case TEMPERATURE_CONTROL_DIMENSION_COMPLETE_PROBE_STATUS:
				devname += " Setpoint";
				UpdateSetPoint(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), static_cast<float>(atof(value.c_str()) / 10.), devname.c_str());
				break;
			default:
				_log.Log(LOG_STATUS, "COpenWebNetTCP: who=%s, where=%s, dimension=%s not yet supported", who.c_str(), where.c_str(), dimension.c_str());
				break;
			}
			break;

		case WHO_BURGLAR_ALARM:                         // 5
			if (!iter->IsNormalFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s not normal frame! -> frame_type=%d", who.c_str(), iter->frame_type);
				return;
			}

			switch (atoi(what.c_str())) {
			case 0:         //maintenace
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm in Maintenance");
				iWhere = 0xff; // force where to 0xff because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
				sCommand = "Maintenance";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 2, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;
			case 1:         //active
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Active");
				iWhere = 0xff; // force where to 0xff because not exist	
				devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
				sCommand = "Active";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;
			case 2:         //disabled
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Inactive");
				iWhere = 0xff; // force where to 0xff because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
				sCommand = "Inactive";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 3, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 4:         //battery fault
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Battery Fault");
				iWhere = 0xfd; // force where to 0xfd because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
				sCommand = "Battery Fault";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 5:         //battery ok
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Battery OK");
				iWhere = 0xfd; // force where to 0xfd because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
				sCommand = "Battery Ok";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 6:			//no network
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm no network");
				iWhere = 0xfe; // force where to 0xfe because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_NETWORK;
				sCommand = "No network";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 7:			//network ok
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm network ok");
				iWhere = 0xfe; // force where to 0xfe because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_NETWORK;
				sCommand = "Network OK";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 8: 	//engaged
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Engaged");
				iWhere = 0xfc; // force where to 0xfc because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT;
				sCommand = "Engaged";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 9:         //disengaged
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Disengaged");
				iWhere = 0xfc; // force where to 0xfc because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT;
				sCommand = "DisEngaged";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 0, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 10:         //battery Unloads
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Battery Unloads");
				iWhere = 0xfd; // force where to 0xfd because not exist
				devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
				sCommand = "Battery Unloads";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 11:         // zone N Active
				iWhere = atoi(whereParam[0].c_str());
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Zone %d Active",iWhere);
				devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
				devname += " " + whereParam[0];
				sCommand = "Active";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 15:         //zone N INTRUSION ALARM
				iWhere = atoi(whereParam[0].c_str());
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Zone %d INTRUSION ALARM", iWhere);
				devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
				devname += " " + whereParam[0];
				sCommand = "Intrusion";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			case 18:         // zone N Not Active
				iWhere = atoi(whereParam[0].c_str());
				//_log.Log(LOG_STATUS, "COpenWebNetTCP: Alarm Zone %d Not active", iWhere);
				devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
				devname += " " + whereParam[0];
				sCommand = "Inactive";
				UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 0, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
				break;

			default:
				_log.Log(LOG_STATUS, "COpenWebNetTCP: who=%s, where=%s, dimension=%s not yet supported", who.c_str(), where.c_str(), dimension.c_str());
				break;
			}
			break;
		case WHO_AUXILIARY:                             // 9
			/**
				example:

				*9*what*where##

				what:   0 = OFF
						1 = ON
				where:  1 to 9 (AUX channel)
			**/
			if (!iter->IsNormalFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s frame error!", who.c_str());
				return;
			}

			devname = OPENWEBNET_AUXILIARY;
			devname += " " + where;

			//pTypeGeneralSwitch, sSwitchAuxiliaryT1
			UpdateSwitch(WHO_AUXILIARY, atoi(where.c_str()), atoi(what.c_str()), atoi(sInterface.c_str()), 100, devname.c_str(), sSwitchAuxiliaryT1);
			break;
		case WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION:              // 25
			if (!iter->IsNormalFrame())
			{
				_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s not normal frame! -> frame_type=%d", who.c_str(), iter->frame_type);
				return;
			}


			switch (atoi(what.c_str())) { //CEN PLUS / DRY CONTACT / IR DETECTION

			case 21:         //Short pressure
				iWhere = atoi(where.c_str());
				iAppValue = atoi(whatParam[0].c_str());
				_log.Log(LOG_STATUS, "COpenWebNetTCP: CEN PLUS Short pressure %d Button %d", iWhere, iAppValue);
				devname = OPENWEBNET_CENPLUS;
				devname += " " + where + " Short Press Button " + whatParam[0].c_str();
				UpdateCenPlus(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, 1, iAppValue, atoi(what.c_str()), 255, devname.c_str());
				break;

			case 22:         //Start of extended pressure
				_log.Log(LOG_STATUS, "COpenWebNetTCP: CEN Start of extended pressure");
				break;

			case 23:         //Extended pressure
				_log.Log(LOG_STATUS, "COpenWebNetTCP: CEN Extended pressure");
				break;

			case 24:         //End of Extended pressure
				iWhere = atoi(where.c_str());
				iAppValue = atoi(whatParam[0].c_str());

				_log.Log(LOG_STATUS, "COpenWebNetTCP: CEN PLUS Long pressure %d Button %d", iWhere, iAppValue);
				devname = OPENWEBNET_CENPLUS;
				devname += " " + where + " Long Press Button " + whatParam[0].c_str();
				UpdateCenPlus(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, 1, iAppValue, atoi(what.c_str()), 255, devname.c_str());
				break;

			case 31:
			case 32:
				if (where.substr(0, 1) != "3")
				{
					_log.Log(LOG_ERROR, "COpenWebNetTCP: Where=%s is not correct for who=%s", where.c_str()), who.c_str();
					return;
				}

				devname = OPENWEBNET_DRY_CONTACT;
				devname += " " + where.substr(1);
				iWhere = atoi(where.substr(1).c_str());

				iAppValue = atoi(what.c_str());
				if (iAppValue == DRY_CONTACT_IR_DETECTION_WHAT_ON)
					iAppValue = 1;
				else
					iAppValue = 0;

				//pTypeGeneralSwitch, sSwitchContactT1
				UpdateSwitch(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str(), sSwitchContactT1);
				break;
			default:
				_log.Log(LOG_ERROR, "COpenWebNetTCP: What=%s is not correct for who=%s", what.c_str()), who.c_str();
				return;
			}
			break;
		case WHO_SCENARIO:                              // 0
		case WHO_LOAD_CONTROL:                          // 3
		case WHO_DOOR_ENTRY_SYSTEM:                     // 6
		case WHO_MULTIMEDIA:                            // 7
		case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
		case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
		case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
		case WHO_AUDIO:                                 // 16
		case WHO_SCENARIO_PROGRAMMING:                  // 17
		case WHO_ENERGY_MANAGEMENT:                     // 18
		case WHO_LIHGTING_MANAGEMENT:                   // 24
		case WHO_ZIGBEE_DIAGNOSTIC:                     // 1000
		case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
		case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
		case WHO_DEVICE_DIAGNOSTIC:                     // 1013
			_log.Log(LOG_ERROR, "COpenWebNetTCP: Who=%s not yet supported!", who.c_str());
			return;
		default:
			_log.Log(LOG_ERROR, "COpenWebNetTCP: ERROR Who=%s not exist!", who.c_str());
			return;
	}
}


/**
   Convert domoticz command in a OpenWebNet command, then send it to device
**/
bool COpenWebNetTCP:: WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pCmd = (_tGeneralSwitch*)pdata;

	unsigned char packetlength = pCmd->len;
	unsigned char packettype = pCmd->type;
	unsigned char subtype = pCmd->subtype;

    int who = 0;
	int what = 0;
	int where = 0;
	int iInterface = pCmd->unitcode;

	// Test packet type
	switch(packettype){
        case pTypeGeneralSwitch:
            // Test general switch subtype
            switch(subtype){
                case sSwitchBlindsT1:
                    //Blinds/Window command
                    who = WHO_AUTOMATION;
                	where = (int)(pCmd->id & 0xFFFF);

                    if (pCmd->cmnd == gswitch_sOff)
                    {
                        what = AUTOMATION_WHAT_UP;
                    }
                    else if (pCmd->cmnd == gswitch_sOn)
                    {
                        what = AUTOMATION_WHAT_DOWN;
                    }
                    else if (pCmd->cmnd == gswitch_sStop)
                    {
                        what = AUTOMATION_WHAT_STOP;
                    }
                    break;
                case sSwitchLightT1:
                    //Light/Switch command
                    who = WHO_LIGHTING;
                	where = (int)(pCmd->id & 0xFFFF);

                    if (pCmd->cmnd == gswitch_sOff)
                    {
                        what = LIGHTING_WHAT_OFF;
                    }
                    else if (pCmd->cmnd == gswitch_sOn)
                    {
                        what = LIGHTING_WHAT_ON;
                    }
                    else if (pCmd->cmnd == gswitch_sSetLevel)
                    {
                        // setting level of dimmer
                        if (pCmd->level != 0)
                        {
                            if (pCmd->level < 20) pCmd->level = 20; // minimum value after 0
                            what = int((pCmd->level + 5)/10);
                        }
                        else
                        {
                            what = LIGHTING_WHAT_OFF;
                        }
                    }
                    break;
                case sSwitchAuxiliaryT1:
                    //Auxiliary command
                    who = WHO_AUXILIARY;
                	where = (int)(pCmd->id & 0xFFFF);

                    if (pCmd->cmnd == gswitch_sOff)
                    {
                        what = AUXILIARY_WHAT_OFF;
                    }
                    else if (pCmd->cmnd == gswitch_sOn)
                    {
                        what = AUXILIARY_WHAT_ON;
                    }
                    break;
                default:
                    break;
            }
            break;
		case sSwitchContactT1:
			// Dry Contact / IR Detection
			return(false); // can't write a contact
        case pTypeThermostat:
            // Test Thermostat subtype
            switch(subtype){
                case sTypeThermSetpoint:
                case sTypeThermTemperature:
                    break;
                default:
                    break;
            }
            break;

	default:
		_log.Log(LOG_STATUS, "COpenWebNetTCP unknown command: packettype=%d subtype=%d", packettype, subtype);
		return false;
	}

	//int used = 1;
	if (!FindDevice(who, where, iInterface, NULL)) {
		_log.Log(LOG_ERROR, "COpenWebNetTCP: command received for unknown device : %d/%d", who, where);
		return false;
	}

	if (iInterface==0) {
		//local bus
		vector<bt_openwebnet> responses;
		bt_openwebnet request(who, what, (where & ~OPENWEBNET_GROUP_ID), (where & OPENWEBNET_GROUP_ID));
		if (sendCommand(request, responses))
		{
			if (responses.size() > 0)
			{
				return responses.at(0).IsOKFrame();
			}
		}
	}
	else {
		vector<bt_openwebnet> responses;
		bt_openwebnet request;

		std::stringstream whoStr;
		whoStr << who;
		std::string sWho = whoStr.str();

		std::stringstream whatStr;
		whatStr << what;
		std::string sWhat = whatStr.str();

		std::stringstream whereStr;
		whereStr << (where & ~OPENWEBNET_GROUP_ID);
		std::string sWhere = "";
		if ((where & OPENWEBNET_GROUP_ID))
			sWhere += "#";
		sWhere += whereStr.str();

		std::stringstream interfaceStr;
		interfaceStr << iInterface;
		std::string sInterface = interfaceStr.str();

		std::string lev = "";
		std::string when = "";

		request.CreateMsgOpen(sWho, sWhat, sWhere, lev, sInterface, when);
		if (sendCommand(request, responses))
		{
			if (responses.size() > 0)
			{
				return responses.at(0).IsOKFrame();
			}
		}
	}

		return true;
}


bool COpenWebNetTCP::SetSetpoint(const int idx, const float temp)
{
int where = idx;
int _temp = (int)(temp * 10);



	vector<bt_openwebnet> responses;
	bt_openwebnet request;

	std::stringstream whoStr;
	whoStr << WHO_TEMPERATURE_CONTROL;
	std::string sWho = whoStr.str();
	
	std::stringstream whereStr;
	whereStr << (where & ~OPENWEBNET_GROUP_ID);
	std::string sWhere = "";
	// add # to set value permanent
	sWhere += "#" + whereStr.str();

	std::stringstream dimensionStr;
	dimensionStr << TEMPERATURE_CONTROL_DIMENSION_SET_POINT_TEMPERATURE;
	std::string sDimension = dimensionStr.str();

	std::stringstream valueStr;


	valueStr << 0;
	valueStr << _temp;
	vector<std::string> sValue;
	sValue.push_back(valueStr.str());
	sValue.push_back("3");					//send generic mode. We don't need to know in witch state the bt3550 or BTI-L4695 is (cooling or heating).

	request.CreateWrDimensionMsgOpen(sWho, sWhere, sDimension, sValue); // (const std::string& who, const std::string& where, const std::string& dimension, const std::vector<std::string>& value)
	if (sendCommand(request, responses, 1, false))
	{
		if (responses.size() > 0)
		{
			return responses.at(0).IsOKFrame();
		}
	}

	return false;
}



/**
   Send OpenWebNet command to device
**/
bool COpenWebNetTCP::sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, int waitForResponse, bool silent)
{
    csocket *commandSocket;
	bool ret;

    if(!(commandSocket = connectGwOwn(OPENWEBNET_COMMAND_SESSION)))
    {
        _log.Log(LOG_ERROR, "COpenWebNetTCP: Command session ERROR");
        return false;
    }
    // Command session correctly open
    _log.Log(LOG_STATUS, "COpenWebNetTCP: Command session connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

    // Command session correctly open -> write command
	int bytesWritten = commandSocket->write(command.frame_open.c_str(), command.frame_open.length());
	if (bytesWritten != command.frame_open.length()) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNetTCP sendCommand: partial write");
		}
	}

	if (waitForResponse > 0) {
		sleep_seconds(waitForResponse);

		char responseBuffer[OPENWEBNET_BUFFER_SIZE];
		memset(responseBuffer, 0, OPENWEBNET_BUFFER_SIZE);
		int read = commandSocket->read(responseBuffer, OPENWEBNET_BUFFER_SIZE, false);

		if (!silent) {
			_log.Log(LOG_STATUS, "COpenWebNetTCP: sent=%s received=%s", command.frame_open.c_str(), responseBuffer);
		}

		boost::lock_guard<boost::mutex> l(readQueueMutex);
		ret = ParseData(responseBuffer, read, response);
	}
	else
		ret = true;

	if (commandSocket->getState() != csocket::CLOSED)
		commandSocket->close();
	return (ret);
}

/**
    automatic scan of automation/lighting device
**/
void COpenWebNetTCP::scan_automation_lighting(const int cen_area)
{
    bt_openwebnet request;
    vector<bt_openwebnet> responses;
    stringstream whoStr;
    stringstream whereStr;
	whoStr << WHO_LIGHTING;
    whereStr << cen_area;
    request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
    sendCommand(request, responses, 0, false);
}

/**
    automatic scan of temperature control device
**/
void COpenWebNetTCP::scan_temperature_control()
{
    bt_openwebnet request;
    vector<bt_openwebnet> responses;
	stringstream whoStr;
	stringstream dimensionStr;
	whoStr << WHO_TEMPERATURE_CONTROL;
    dimensionStr << 0;

    for (int where = 1; where < 100; where++)
    {
        stringstream whereStr;
        whereStr << where;
        request.CreateDimensionMsgOpen(whoStr.str(), whereStr.str(), dimensionStr.str());
        sendCommand(request, responses, 0, true);
    }
}

/**
    request general burglar alarm status
**/
void COpenWebNetTCP::requestBurglarAlarmStatus()
{
    bt_openwebnet request;
    vector<bt_openwebnet> responses;
    stringstream whoStr;
    stringstream whereStr;
	whoStr << WHO_BURGLAR_ALARM;
    whereStr << 0;
    request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
    sendCommand(request, responses, 0, false);
}

/**
	request Dry contact/IR Detection alarm status
**/
void COpenWebNetTCP::requestDryContactIRDetectionStatus()
{
	bt_openwebnet request;
	vector<bt_openwebnet> responses;
	stringstream whoStr;
	stringstream whereStr;
	whoStr << WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION;
	whereStr << 30;
	request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
	sendCommand(request, responses, 0, false);
}

/**
    Request time to gateway
**/
void COpenWebNetTCP::requestTime()
{
    _log.Log(LOG_STATUS, "COpenWebNetTCP: request time...");
    bt_openwebnet request;
    vector<bt_openwebnet> responses;
    request.CreateTimeReqMsgOpen();
    sendCommand(request, responses, 0, true);
}

void COpenWebNetTCP::setTime()
{
    _log.Log(LOG_STATUS, "COpenWebNetTCP: set DateTime...");
    bt_openwebnet request;
    vector<bt_openwebnet> responses;
    request.CreateSetTimeMsgOpen();
    sendCommand(request, responses, 0, true);
}

/**
	scan devices
**/
void COpenWebNetTCP::scan_device()
{
	int iWhere;

	/* uncomment the line below to enable the time request to the gateway.
	Note that this is only for debugging, the answer to who = 13 is not yet supported */
	//requestTime();

	/* uncomment the line below to enable the set time commenad to the gateway.*/
	//setTime();
    
	if (mask_request_status & 0x1)
	{
		_log.Log(LOG_STATUS, "COpenWebNetTCP: scanning automation/lighting...");
		// Scan of all devices
		scan_automation_lighting(WHERE_CEN_0);

		requestDryContactIRDetectionStatus();

		_log.Log(LOG_STATUS, "COpenWebNetTCP: request burglar alarm status...");
		requestBurglarAlarmStatus();

		/** Scanning of temperature sensor is not necessary simply wait an update **/
		//_log.Log(LOG_STATUS, "COpenWebNetTCP: scanning temperature control...");
		//scan_temperature_control();

		_log.Log(LOG_STATUS, "COpenWebNetTCP: scan device complete, wait all the update data..");

		/* Update complete scan time*/
		LastScanTime = mytime(NULL);
	}
	else
	{
		// Scan only the set areas
		for (iWhere = WHERE_AREA_1; iWhere < MAX_WHERE_AREA; iWhere++)
		{
			if (mask_request_status & (0x1 << iWhere))
			{
				_log.Log(LOG_STATUS, "COpenWebNetTCP: scanning AREA %u...", iWhere);
				scan_automation_lighting(iWhere);
			}				
		}
	}   
}

bool COpenWebNetTCP::ParseData(char* data, int length, vector<bt_openwebnet>& messages)
{
	string buffer = string(data, length);
	size_t begin = 0;
	size_t end = string::npos;
	do {
		end = buffer.find(OPENWEBNET_END_FRAME, begin);
		if (end != string::npos) {
			bt_openwebnet message(buffer.substr(begin, end - begin + 2));
			messages.push_back(message);
			begin = end + 2;
		}
	} while (end != string::npos);

	return true;
}

void COpenWebNetTCP::Do_Work()
{
	while (!m_stoprequested)
	{
	    if (isStatusSocketConnected() && mask_request_status)
        {
            scan_device();
			mask_request_status = 0x0; // scan devices complete
        }

		// every 5 minuts force scan ALL devices for refresh status
		if ((mytime(NULL) - LastScanTime) > 300)
		{
			if ((mask_request_status & 0x1) == 0)
				_log.Log(LOG_STATUS, "COpenWebNetTCP: HEARTBEAT set scan devices ...");
			mask_request_status = 0x1; // force scan devices
		}

		sleep_seconds(OPENWEBNET_HEARTBEAT_DELAY);
		m_LastHeartbeat = mytime(NULL);
	}
	_log.Log(LOG_STATUS, "COpenWebNetTCP: Heartbeat worker stopped...");
}

/**
   Find OpenWebNetDevice in DB
**/
bool COpenWebNetTCP::FindDevice(int who, int where, int iInterface, int* used)
{
	vector<vector<string> > result;
	int devType = -1;
	int subType = -1;
	int subUnit = iInterface;

    		//make device ID
    unsigned char ID1 = (unsigned char)((who & 0xFF00) >> 8);
	unsigned char ID2 = (unsigned char)(who & 0xFF);
	unsigned char ID3 = (unsigned char)((where & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)(where & 0xFF);

	char szIdx[10];
	switch (who) {
        case WHO_LIGHTING:                              // 1
			devType = pTypeGeneralSwitch;
			subType = sSwitchLightT1;
			sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
            break;
		case WHO_AUTOMATION:                            // 2
			devType = pTypeGeneralSwitch;
            subType = sSwitchBlindsT1;
            sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
            break;
        case WHO_TEMPERATURE_CONTROL:                   // 4
            //devType = pTypeGeneral;
            //subType = sTypeTemperature;
            //subUnit = where;
            //printf(szIdx, "%02X%02X", who, where);
            //break;
			return true; // device always present
        case WHO_AUXILIARY:                             // 9
			devType = pTypeGeneralSwitch;
			subType = sSwitchAuxiliaryT1;
			sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
            break;
		case WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION:     // 25
			devType = pTypeGeneralSwitch;
			subType = sSwitchContactT1;
			sprintf(szIdx, "%02X%02X%02X%02X", ID1, ID2, ID3, ID4);
			break;
        case WHO_SCENARIO:                              // 0
		case WHO_LOAD_CONTROL:                          // 3
		case WHO_BURGLAR_ALARM:                         // 5
		case WHO_DOOR_ENTRY_SYSTEM:                     // 6
		case WHO_MULTIMEDIA:                            // 7
		case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
		case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
		case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
		case WHO_AUDIO:                                 // 16
		case WHO_SCENARIO_PROGRAMMING:                  // 17
		case WHO_ENERGY_MANAGEMENT:                     // 18
		case WHO_LIHGTING_MANAGEMENT:                   // 24
		case WHO_ZIGBEE_DIAGNOSTIC:                     // 1000
		case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
		case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
		case WHO_DEVICE_DIAGNOSTIC:                     // 1013
	default:
			return false;
	}

    if ((who == WHO_LIGHTING) || (who == WHO_AUTOMATION) || (who == WHO_TEMPERATURE_CONTROL) || (who == WHO_AUXILIARY) || (who == WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION))
    {
        if (used != NULL)
        {
            result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d) and Used == %d",
                    m_HwdID, szIdx, subUnit, devType, subType, *used);
        }
        else
        {
            result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
                m_HwdID, szIdx, subUnit, devType, subType);
        }
    }
    else
        return false;


	if (result.size() > 0)
	{
		return true;
	}

	return false;
}
