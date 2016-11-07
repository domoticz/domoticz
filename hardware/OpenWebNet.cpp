/************************************************************************
*
Legrand MyHome / OpenWebNet Interface board driver for Domoticz
Date: 24-01-2016
Written by: Stéphane Lebrasseur

Date: 04-11-2016
Update by: Matteo Facchetti

License: Public domain


 ************************************************************************/
#include "stdafx.h"
#include "OpenWebNet.h"
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

#define OPENWEBNET_HEARTBEAT_DELAY 1
#define OPENWEBNET_STATUS_NB_HEARTBEAT 600
#define OPENWEBNET_RETRY_DELAY 30
#define OPENWEBNET_POLL_INTERVAL 1000
#define OPENWEBNET_BUFFER_SIZE 1024
#define OPENWEBNET_SOCKET_SUCCESS 0
#define OPENWEBNET_AUTOMATION "AUTOMATION"
#define OPENWEBNET_LIGHT "LIGHT"
#define OPENWEBNET_TEMPERATURE "TEMPERATURE"

#define OPENWEBNET_DEVICE_ID "00000000"

/**
    Create new hardware OpenWebNet instance
**/
COpenWebNet::COpenWebNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort) : m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_heartbeatcntr = OPENWEBNET_HEARTBEAT_DELAY;
	m_pStatusSocket = NULL;
}

/**
    destroys hardware OpenWebNet instance
**/
COpenWebNet::~COpenWebNet(void)
{
}

/**
    Start Hardware OpneWebNet Monitor/Worker Service
**/
bool COpenWebNet::StartHardware()
{
	m_stoprequested = false;

	m_bIsStarted = true;

	//Start monitor thread
	m_monitorThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWebNet::MonitorFrames, this)));

	//Start worker thread
	if (m_monitorThread != NULL) {
		m_heartbeatThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&COpenWebNet::Do_Work, this)));
	}

	return (m_monitorThread!=NULL && m_heartbeatThread != NULL);
}

/**
    Stop Hardware OpneWebNet Monitor/Worker Service
**/
bool COpenWebNet::StopHardware()
{
	m_stoprequested = true;
	if (isStatusSocketConnected())
	{
		try {
			delete m_pStatusSocket;
			m_pStatusSocket = NULL;
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
   Check socket connection
**/
bool COpenWebNet::isStatusSocketConnected()
{
	return m_pStatusSocket!=NULL && m_pStatusSocket->getState() == csocket::CONNECTED;
};

/**
   Connection to the gateway OpenWebNet
**/
bool COpenWebNet::connectStatusSocket()
{
	if (m_pStatusSocket != NULL) {
		delete m_pStatusSocket;
		m_pStatusSocket = NULL;
	}

	if (m_szIPAddress.size() == 0 || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		_log.Log(LOG_ERROR, "COpenWebNet: Cannot connect to gateway, empty  IP Address or Port");
		return false;
	}

	m_pStatusSocket = new csocket();
	m_pStatusSocket->connect(m_szIPAddress.c_str(), m_usIPPort);

	if (m_pStatusSocket->getState() != csocket::CONNECTED)
	{
		_log.Log(LOG_ERROR, "COpenWebNet: Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		if (m_pStatusSocket != NULL) {
			delete m_pStatusSocket;
			m_pStatusSocket = NULL;
		}
		return false;
	}

	_log.Log(LOG_STATUS, "COpenWebNet: connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	int bytesWritten = m_pStatusSocket->write(OPENWEBNET_EVENT_SESSION, strlen(OPENWEBNET_EVENT_SESSION));
	if (bytesWritten != strlen(OPENWEBNET_EVENT_SESSION)) {
		_log.Log(LOG_ERROR, "COpenWebNet: partial write");
	}
	char databuffer[OPENWEBNET_BUFFER_SIZE];
	memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	int read = m_pStatusSocket->read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseSession(string(databuffer, read));
	if (!responseSession.IsOKFrame()) {
		_log.Log(LOG_STATUS, "COpenWebNet: failed to begin session, NACK received (%s)", m_szIPAddress.c_str(), m_usIPPort, databuffer);
		if (m_pStatusSocket != NULL) {
			delete m_pStatusSocket;
			m_pStatusSocket = NULL;
		}
		return false;
	}

	sOnConnected(this);
	return true;
}

/**
    Thread Monitor: get update from the OpenWebNet gateway and add new devices if necessary
**/
void COpenWebNet::MonitorFrames()
{
	//TODO : monitor socket is closed every 1 hour : replace socket before it closes
	while (!m_stoprequested)
	{
		if (!isStatusSocketConnected() && !m_stoprequested)
		{
			while (!connectStatusSocket())
			{
				_log.Log(LOG_STATUS, "COpenWebNet: TCP/IP monitor not connected, retrying in %d seconds...", OPENWEBNET_RETRY_DELAY);
				sleep_seconds(OPENWEBNET_RETRY_DELAY);
			}
		}
		else
		{
			char data[OPENWEBNET_BUFFER_SIZE];
			memset(data, 0, OPENWEBNET_BUFFER_SIZE);
			int bread = m_pStatusSocket->read(data, OPENWEBNET_BUFFER_SIZE, false);

			if (m_stoprequested)
				break;
			m_LastHeartbeat = mytime(NULL);

			if ((bread == 0) || (bread<0)) {
				_log.Log(LOG_ERROR, "COpenWebNet: TCP/IP monitor connection closed!");
				delete m_pStatusSocket;
				m_pStatusSocket = NULL;
			}
			else
			{
				boost::lock_guard<boost::mutex> l(readQueueMutex);
				vector<bt_openwebnet> responses;
				ParseData(data, bread, responses);

				for (vector<bt_openwebnet>::iterator iter = responses.begin(); iter != responses.end(); iter++) {
					if (iter->IsNormalFrame()) {
						AddDeviceIfNotExits(iter->Extract_who(), iter->Extract_where());
					}
                    UpdateDeviceValue(iter->Extract_who(), iter->Extract_where(), iter->Extract_what());
					_log.Log(LOG_STATUS, "COpenWebNet: received=%s", frameToString(*iter).c_str());
				}
			}
		}
	}
	_log.Log(LOG_STATUS, "COpenWebNet: TCP/IP monitor worker stopped...");
}

void COpenWebNet::UpdateBlinds(const int ptype, const int subtype, const int SubUnit, const int bOn, const int BatLevel)
{
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue,ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, OPENWEBNET_DEVICE_ID, SubUnit, int(ptype), int(subtype));
	if (!result.empty())
	{
        //check if we have a change, if not do not update it
        int nvalue = atoi(result[0][1].c_str());
        if (subtype == sSwitchBlindsT1)
        {
            if (bOn == nvalue) return;

            //Send as Lighting 2
            tRBUF lcmd;
            memset(&lcmd, 0, sizeof(RBUF));
            lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
            lcmd.LIGHTING2.packettype = ptype;
            lcmd.LIGHTING2.subtype = subtype;
            lcmd.LIGHTING2.id1 = 0;
            lcmd.LIGHTING2.id2 = 0;
            lcmd.LIGHTING2.id3 = 0;
            lcmd.LIGHTING2.id4 = 0;
            lcmd.LIGHTING2.unitcode = SubUnit;
            lcmd.LIGHTING2.cmnd = bOn;
            lcmd.LIGHTING2.level = 0;
            lcmd.LIGHTING2.filler = 0;
            lcmd.LIGHTING2.rssi = 12;
            sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, result[0][0].c_str(), BatLevel);
        }
	}
}

void COpenWebNet::UpdateSwitch(const int ptype, const int subtype, const int SubUnit, const int bOn, const double Level, const int BatLevel)
{
    double rlevel = (15.0 / 100)*Level;
	int level = int(rlevel);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue,ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, OPENWEBNET_DEVICE_ID, SubUnit, int(ptype), int(subtype));
	if (!result.empty())
	{
        //check if we have a change, if not do not update it
        int nvalue = atoi(result[0][1].c_str());
        if (subtype == sSwitchLightT1)
        {
            if (bOn == nvalue) return;

             //Send as Lighting 2
            tRBUF lcmd;
            memset(&lcmd, 0, sizeof(RBUF));
            lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
            lcmd.LIGHTING2.packettype = ptype;
            lcmd.LIGHTING2.subtype = subtype;
            lcmd.LIGHTING2.id1 = 0;
            lcmd.LIGHTING2.id2 = 0;
            lcmd.LIGHTING2.id3 = 0;
            lcmd.LIGHTING2.id4 = 0;
            lcmd.LIGHTING2.unitcode = SubUnit;
            lcmd.LIGHTING2.cmnd = bOn;
            lcmd.LIGHTING2.level = level;
            lcmd.LIGHTING2.filler = 0;
            lcmd.LIGHTING2.rssi = 12;
            sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, result[0][0].c_str(), BatLevel);
        }
	}
}

void COpenWebNet::UpdateDeviceValue(string who, string where, string what)
{
    switch (atoi(who.c_str())) {
        case WHO_LIGHTING:                              // 1
            UpdateSwitch(pTypeGeneralSwitch, sSwitchLightT1, atoi(where.c_str()), atoi(what.c_str()) ? gswitch_sOn : gswitch_sOff, 100., 100);
            break;
        case WHO_AUTOMATION:
            int value;
            switch(atoi(what.c_str()))
            {
            case AUTOMATION_WHAT_STOP:  // 0
                value = gswitch_sStop;
                break;
            case AUTOMATION_WHAT_UP:    // 1
                value = gswitch_sOff;
                break;
            case AUTOMATION_WHAT_DOWN:  // 2
                value = gswitch_sOn;
                break;
            default:
                return;
            }
            UpdateBlinds(pTypeGeneralSwitch, sSwitchBlindsT1, atoi(where.c_str()), value, 100);                       // 2
            break;
        case WHO_SCENARIO:                              // 0
        case WHO_LOAD_CONTROL:                          // 3
        case WHO_TEMPERATURE_CONTROL:                   // 4
        case WHO_BURGLAR_ALARM:                         // 5
        case WHO_DOOR_ENTRY_SYSTEM:                     // 6
        case WHO_MULTIMEDIA:                            // 7
        case WHO_AUXILIARY:                             // 9
        case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
        case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
        case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
        case WHO_AUDIO:                                 // 16
        case WHO_SCENARIO_PROGRAMMING:                  // 17
        case WHO_ENERGY_MANAGEMENT:                     // 18
        case WHO_LIHGTING_MANAGEMENT:                   // 24
        case WHO_SCENARIO_SCHEDULER_BUTTONS:            // 25
        case WHO_DIAGNOSTIC:                            // 1000
        case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
        case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
        case WHO_DEVICE_DIAGNOSTIC:                     // 1013
            _log.Log(LOG_ERROR, "COpenWebNet: Who=%s not yet supported!");
            return;
    default:
            _log.Log(LOG_ERROR, "COpenWebNet: ERROR Who=%s not exist!");
        return;
    }

}


/**
   Convert domoticz command in a OpenWebNet command, then send it to device
**/
bool COpenWebNet:: WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pCmd = (_tGeneralSwitch*)pdata;

	unsigned char packetlength = pCmd->len;
	unsigned char packettype = pCmd->type;
	unsigned char subtype = pCmd->subtype;


	int who;
	int what;
	int where;

    // Test packet type
	switch(packettype){
        case pTypeGeneralSwitch:
            // Test general switch subtype
            switch(subtype){
                case sSwitchBlindsT1:
                    //Blinds/Window command
                    who = WHO_AUTOMATION;
                    where = (int)pCmd->unitcode;

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
                    where = (int)pCmd->unitcode;

                    if (pCmd->cmnd == gswitch_sOff)
                    {
                        what = LIGHT_WHAT_OFF;
	}
                    else if (pCmd->cmnd == gswitch_sOn)
                    {
                        what = LIGHT_WHAT_ON;
                    }
                default:
                    break;
            }
            break;
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
		_log.Log(LOG_STATUS, "COpenWebNet unknown command: packettype=%d subtype=%d", packettype, subtype);
		return false;
	}

	int used = 1;
	if (!FindDevice(who, where, &used)) {
		_log.Log(LOG_ERROR, "COpenWebNet: command received for unknown device : %d/%d", who, where);
		return false;
	}

	vector<bt_openwebnet> responses;
	bt_openwebnet request(who, where, what);
	if (sendCommand(request, responses))
	{
		if (responses.size() > 0)
		{
			return responses.at(0).IsOKFrame();
		}
	}

	return true;
}

/**
   Send OpenWebNet command to device
**/
bool COpenWebNet::sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, int waitForResponse, bool silent)
{
	csocket commandSocket;

	if (m_szIPAddress.size() == 0 || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet: Cannot connect to gateway, empty IP Address or Port");
		}
		return false;
	}

	int connectResult = commandSocket.connect(m_szIPAddress.c_str(), m_usIPPort);
	if (connectResult != OPENWEBNET_SOCKET_SUCCESS) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet: Cannot connect to gateway : %d", connectResult);
		}
		return false;
	}

	if (commandSocket.getState() != csocket::CONNECTED)
	{
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet: Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		}
		return false;
	}

	int bytesWritten = commandSocket.write(OPENWEBNET_COMMAND_SESSION, strlen(OPENWEBNET_COMMAND_SESSION));
	if (bytesWritten != strlen(OPENWEBNET_COMMAND_SESSION)) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet sendCommand: partial write");
		}
	}

	char databuffer[OPENWEBNET_BUFFER_SIZE];
	memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	int read = commandSocket.read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseSession(string(databuffer, read));
	_log.Log(LOG_STATUS, "COpenWebNet : sent=%s received=%s", OPENWEBNET_COMMAND_SESSION, databuffer);
	if (!responseSession.IsOKFrame()) {
		if (!silent) {
			_log.Log(LOG_STATUS, "COpenWebNet: failed to begin session, NACK received (%s)", m_szIPAddress.c_str(), m_usIPPort, databuffer);
		}
		return false;
	}

	bytesWritten = commandSocket.write(command.frame_open.c_str(), command.frame_open.length());
	if (bytesWritten != command.frame_open.length()) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet sendCommand: partial write");
		}
	}

	if (waitForResponse > 0) {
		sleep_seconds(waitForResponse);
	}

	char responseBuffer[OPENWEBNET_BUFFER_SIZE];
	memset(responseBuffer, 0, OPENWEBNET_BUFFER_SIZE);
	read = commandSocket.read(responseBuffer, OPENWEBNET_BUFFER_SIZE, false);

	if (!silent) {
		_log.Log(LOG_STATUS, "COpenWebNet: sent=%s received=%s", command.frame_open.c_str(), responseBuffer);
	}
	return ParseData(responseBuffer, read, response);
}


bool COpenWebNet::ParseData(char* data, int length, vector<bt_openwebnet>& messages)
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

void COpenWebNet::Do_Work()
{
	while (!m_stoprequested)
	{
		sleep_seconds(OPENWEBNET_HEARTBEAT_DELAY);
		m_LastHeartbeat = mytime(NULL);
	}
	_log.Log(LOG_STATUS, "COpenWebNet: Heartbeat worker stopped...");
}


/**
   Add a new device if not exist
**/
bool COpenWebNet::AddDeviceIfNotExits(string who, string where)
{
	if (!m_sql.m_bAcceptNewHardware)
	{
		return false; //We do not allow new devices
	}

    if (!FindDevice(atoi(who.c_str()), atoi(where.c_str()), NULL))
	{
        int devType = -1;
        int subType = -1;
        int switchType = -1;
        string devname;

        switch (atoi(who.c_str())) {
            case WHO_LIGHTING:                              // 1
                devType = pTypeGeneralSwitch;
                subType = sSwitchLightT1;
                switchType = STYPE_OnOff;
                devname = OPENWEBNET_LIGHT;
                devname += " " + where;
                break;
            case WHO_AUTOMATION:                            // 2
                devType = pTypeGeneralSwitch;
                subType = sSwitchBlindsT1;
                switchType = STYPE_Blinds;
                devname = OPENWEBNET_AUTOMATION;
                devname += " " + where;
                break;

            case WHO_SCENARIO:                              // 0
            case WHO_LOAD_CONTROL:                          // 3
            case WHO_TEMPERATURE_CONTROL:                   // 4
            case WHO_BURGLAR_ALARM:                         // 5
            case WHO_DOOR_ENTRY_SYSTEM:                     // 6
            case WHO_MULTIMEDIA:                            // 7
            case WHO_AUXILIARY:                             // 9
            case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
            case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
            case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
            case WHO_AUDIO:                                 // 16
            case WHO_SCENARIO_PROGRAMMING:                  // 17
            case WHO_ENERGY_MANAGEMENT:                     // 18
            case WHO_LIHGTING_MANAGEMENT:                   // 24
            case WHO_SCENARIO_SCHEDULER_BUTTONS:            // 25
            case WHO_DIAGNOSTIC:                            // 1000
            case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
            case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
            case WHO_DEVICE_DIAGNOSTIC:                     // 1013
                _log.Log(LOG_ERROR, "COpenWebNet: Who=%s not yet supported!");
                return false;
        default:
                _log.Log(LOG_ERROR, "COpenWebNet: ERROR Who=%s not exist!");
            return false;
        }

		//Insert
		m_sql.safe_query(
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Name, Used) "
			"VALUES (%d,'%q','%q',%d,%d,%d,'%q',0)",
			m_HwdID, OPENWEBNET_DEVICE_ID, where.c_str(), devType, subType, switchType, devname.c_str());

		//Get new ID
		vector<vector<string> > result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit='%q' AND Type=%d AND SubType=%d)",
			m_HwdID, OPENWEBNET_DEVICE_ID, where.c_str(), devType, subType);
		if (result.size() == 0)
		{
			_log.Log(LOG_ERROR, "Serious database error, problem getting ID from DeviceStatus!");
			return false;
		}

		std::stringstream s_str(result[0][0]);
		m_sql.m_LastSwitchID = "OpenWebNet";
		s_str >> m_sql.m_LastSwitchRowID;

        // why want disable m_bAcceptNewHardware? Only first is taken... :/
		//m_sql.m_bAcceptNewHardware = false;

		return true;
	}

	return false;
}


/**
   Find OpenWebNetDevice in DB
**/
bool COpenWebNet::FindDevice(int who, int where, int* used)
{
	vector<vector<string> > result;
	int devType = -1;
	int subType = -1;

	switch (who) {
        case WHO_LIGHTING:                              // 1
		    devType = pTypeGeneralSwitch;
			subType = sSwitchLightT1;
            break;
		case WHO_AUTOMATION:                            // 2
            devType = pTypeGeneralSwitch;
            subType = sSwitchBlindsT1;
            break;
        case WHO_SCENARIO:                              // 0
		case WHO_LOAD_CONTROL:                          // 3
        case WHO_TEMPERATURE_CONTROL:                   // 4
		case WHO_BURGLAR_ALARM:                         // 5
		case WHO_DOOR_ENTRY_SYSTEM:                     // 6
		case WHO_MULTIMEDIA:                            // 7
		case WHO_AUXILIARY:                             // 9
		case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
		case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
		case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
		case WHO_AUDIO:                                 // 16
		case WHO_SCENARIO_PROGRAMMING:                  // 17
		case WHO_ENERGY_MANAGEMENT:                     // 18
		case WHO_LIHGTING_MANAGEMENT:                   // 24
		case WHO_SCENARIO_SCHEDULER_BUTTONS:            // 25
		case WHO_DIAGNOSTIC:                            // 1000
		case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
		case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
		case WHO_DEVICE_DIAGNOSTIC:                     // 1013
	default:
			return false;
	}

	if (used != NULL) {
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE HardwareID==%d AND Unit == %d AND Type=%d AND SubType=%d and Used == %d",
			m_HwdID, where, devType, subType, *used);
	}
	else {
		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE HardwareID==%d AND Unit == %d AND Type=%d AND SubType=%d",
			m_HwdID, where, devType, subType);
	}

	if (result.size() > 0)
	{
		return true;
	}

	return false;
}

/**
   Convert the frame in string a string
**/
string COpenWebNet::frameToString(bt_openwebnet& frame)
{
	stringstream frameStr;

	frameStr << frame.frame_open;
	frameStr << " : ";

	if (frame.IsErrorFrame())
	{
		frameStr << "ERROR FRAME";
	}
	else if(frame.IsNullFrame())
	{
		frameStr << "NULL FRAME";
	}
	else if (frame.IsMeasureFrame())
	{
		frameStr << "MEASURE FRAME";
	}
	else if (frame.IsStateFrame())
	{
		frameStr << "STATE FRAME";
	}
	else if (frame.IsWriteFrame())
	{
		frameStr << "WRITE FRAME";
	}
	else if (frame.IsPwdFrame())
	{
		frameStr << "PASSWORD FRAME";
	}
	else if (frame.IsOKFrame())
	{
		frameStr << "ACK FRAME";
	}
	else if (frame.IsKOFrame())
	{
		frameStr << "NACK FRAME";
	}
	else if (frame.IsNormalFrame())
	{
		frameStr << "NORMAL FRAME";

		if (frame.extended) {
			frameStr << " - EXTENDED";
		}

		frameStr << " - who=" << getWhoDescription(frame.Extract_who());
		frameStr << " - what=" << getWhatDescription(frame.Extract_who(), frame.Extract_what());
		frameStr << " - where=" << frame.Extract_where();
		if (!frame.Extract_when().empty()) {
			frameStr << " - when=" << frame.Extract_when();
		}
		if (!frame.Extract_level().empty()) {
			frameStr << " - level=" << frame.Extract_level();
		}
		if (!frame.Extract_interface().empty()) {
			frameStr << " - interface=" << frame.Extract_interface();
		}
		if (!frame.Extract_dimension().empty()) {
			frameStr << " - dimension=" << frame.Extract_dimension();
		}

		string indirizzo = frame.Extract_address(0);
		if (!indirizzo.empty()) {
			int i = 1;
			frameStr << " - address=";
			while (!indirizzo.empty()) {
				frameStr << indirizzo;
				indirizzo = frame.Extract_address(i++);
				if (!indirizzo.empty()) {
					frameStr << ", ";
				}
			}
		}

		string valori = frame.Extract_value(0);
		if (!valori.empty()) {
			int i = 1;
			frameStr << " - value=";
			while (!valori.empty()) {
				frameStr << valori;
				indirizzo = frame.Extract_value(i++);
				if (!valori.empty()) {
					frameStr << ", ";
				}
			}
		}
	}

	return frameStr.str();
}

/**
    Get a string Description of WHO
**/
string COpenWebNet::getWhoDescription(string who)
{
	if (who == "0") {
		return "Scenario";
	}
	if (who == "1") {
		return "Lighting";
	}
	if (who == "2") {
		return "Automation";
	}
	if (who == "3") {
		return "Load control";
	}
	if (who == "4") {
		return "Temperature control";
	}
	if (who == "5") {
		return "Burglar alarm";
	}
	if (who == "6") {
		return "Door entry system";
	}
	if (who == "7") {
		return "Multimedia";
	}
	if (who == "9") {
		return "Auxiliary";
	}
	if (who == "13") {
		return "Gateway interfaces management";
	}
	if (who == "14") {
		return "Light shutter actuator lock";
	}
	if (who == "15") {
		return "Scenario Scheduler Switch";
	}
	if (who == "16") {
		return "Audio";
	}
	if (who == "17") {
		return "Scenario programming";
	}
	if (who == "18") {
		return "Energy management";
	}
	if (who == "24") {
		return "Lighting management";
	}
	if (who == "25") {
		return "Scenario scheduler buttons";
	}
	if (who == "1000") {
		return "Diagnostic";
	}
	if (who == "1001") {
		return "Automation diagnostic";
	}
	if (who == "1004") {
		return "Thermoregulation diagnostic failure";
	}
	if (who == "1013") {
		return "Device diagnostic";
	}

	return who;
}

/**
    Get a string Description of WHAT
**/
string COpenWebNet::getWhatDescription(string who, string what)
{
	if (who == "0") {
		// "Scenario";
	}
	if (who == "1") {
		// "Lighting";
	}
	if (who == "2") {
		// "Automation";
		if (what == "0") {
			return "Stop";
		}
		if (what == "1") {
			return "Up";
		}
		if (what == "2") {
			return "Down";
		}
	}
	if (who == "3") {
		// "Load control";
	}
	if (who == "4") {
		// "Temperature control";
	}
	if (who == "5") {
		// "Burglar alarm";
	}
	if (who == "6") {
		// "Door entry system";
	}
	if (who == "7") {
		// "Multimedia";
	}
	if (who == "9") {
		// "Auxiliary";
	}
	if (who == "13") {
		// "Gateway interfaces management";
	}
	if (who == "14") {
		// "Light shutter actuator lock";
	}
	if (who == "15") {
		// "Scenario Scheduler Switch";
	}
	if (who == "16") {
		// "Audio";
	}
	if (who == "17") {
		// "Scenario programming";
	}
	if (who == "18") {
		// "Energy management";
	}
	if (who == "24") {
		// "Lighting management";
	}
	if (who == "25") {
		// "Scenario scheduler buttons";
	}
	if (who == "1000") {
		// "Diagnostic";
	}
	if (who == "1001") {
		// "Automation diagnostic";
	}
	if (who == "1004") {
		// "Thermoregulation diagnostic failure";
	}
	if (who == "1013") {
		// "Device diagnostic";
	}

	return what;
}

