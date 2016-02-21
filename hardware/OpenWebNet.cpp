/************************************************************************
*
Legrand MyHome / OpenWebNet Interface board driver for Domoticz
Date: 24-01-2016
Written by: Stéphane Lebrasseur
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

COpenWebNet::COpenWebNet(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_stoprequested = false;
	m_usIPPort = usIPPort;
	m_heartbeatcntr = OPENWEBNET_HEARTBEAT_DELAY;
	m_pStatusSocket = NULL;
}

COpenWebNet::~COpenWebNet(void)
{
}

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

bool COpenWebNet::isStatusSocketConnected()
{
	return m_pStatusSocket!=NULL && m_pStatusSocket->getState() == csocket::CONNECTED;
};

bool COpenWebNet::connectStatusSocket()
{
	if (m_pStatusSocket != NULL) {
		delete m_pStatusSocket;
		m_pStatusSocket = NULL;
	}
	
	if (m_szIPAddress.size() == 0 || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		_log.Log(LOG_ERROR, "COpenWebNet : Cannot connect to gateway, empty  IP Address or Port");
		return false;
	}

	m_pStatusSocket = new csocket();
	m_pStatusSocket->connect(m_szIPAddress.c_str(), m_usIPPort);
	
	if (m_pStatusSocket->getState() != csocket::CONNECTED)
	{
		_log.Log(LOG_ERROR, "COpenWebNet : Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		if (m_pStatusSocket != NULL) {
			delete m_pStatusSocket;
			m_pStatusSocket = NULL;
		}
		return false;
	}

	_log.Log(LOG_STATUS, "COpenWebNet : connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);

	int bytesWritten = m_pStatusSocket->write(OPENWEBNET_EVENT_SESSION, strlen(OPENWEBNET_EVENT_SESSION));
	if (bytesWritten != strlen(OPENWEBNET_EVENT_SESSION)) {
		_log.Log(LOG_ERROR, "COpenWebNet : partial write");
	}
	char databuffer[OPENWEBNET_BUFFER_SIZE];
	memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	int read = m_pStatusSocket->read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseSession(string(databuffer, read));
	if (!responseSession.IsOKFrame()) {
		_log.Log(LOG_STATUS, "COpenWebNet : failed to begin session, NACK received (%s)", m_szIPAddress.c_str(), m_usIPPort, databuffer);
		if (m_pStatusSocket != NULL) {
			delete m_pStatusSocket;
			m_pStatusSocket = NULL;
		}
		return false;
	}

	sOnConnected(this);
	return true;
}

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
						AddDeviceIfNotExits(iter->Extract_chi(), iter->Extract_dove());
					}
					_log.Log(LOG_STATUS, "COpenWebNet : received=%s", frameToString(*iter).c_str());
				}

			}
		}
	}
	_log.Log(LOG_STATUS, "COpenWebNet: TCP/IP monitor worker stopped...");
}

bool COpenWebNet:: WriteToHardware(const char *pdata, const unsigned char length)
{
	tRBUF *pCmd = (tRBUF*)pdata;

	unsigned char packetlength = pCmd->ICMND.packetlength;
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;
	unsigned char seqnbr = pCmd->ICMND.seqnbr;

	int who;
	int what;
	int where;
	
	if (packettype == pTypeBlinds && subtype == sTypeBlindsT12){
		//Blinds/Window command
		int blindId = pCmd->BLINDS1.id1;

		who = WHO_AUTOMATION;
		where = blindId;

		if (pCmd->BLINDS1.cmnd == blinds_sOpen)
		{
			what = AUTOMATION_WHAT_UP;
		}
		else if (pCmd->BLINDS1.cmnd == blinds_sClose)
		{
			what = AUTOMATION_WHAT_DOWN;
		}
		else if (pCmd->BLINDS1.cmnd == blinds_sStop)
		{
			what = AUTOMATION_WHAT_STOP;
		}
	}
	else {
		_log.Log(LOG_STATUS, "COpenWebNet unknown command : packettype=%d subtype=%d", packettype, subtype);
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

bool COpenWebNet::sendCommand(bt_openwebnet& command, vector<bt_openwebnet>& response, int waitForResponse, bool silent)
{
	csocket commandSocket;
	
	if (m_szIPAddress.size() == 0 || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet : Cannot connect to gateway, empty  IP Address or Port");
		}
		return false;
	}

	int connectResult = commandSocket.connect(m_szIPAddress.c_str(), m_usIPPort);
	if (connectResult != OPENWEBNET_SOCKET_SUCCESS) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet : Cannot connect to gateway : %d", connectResult);
		}
		return false;
	}

	if (commandSocket.getState() != csocket::CONNECTED)
	{
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet : Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		}
		return false;
	}

	int bytesWritten = commandSocket.write(OPENWEBNET_COMMAND_SESSION, strlen(OPENWEBNET_COMMAND_SESSION));
	if (bytesWritten != strlen(OPENWEBNET_COMMAND_SESSION)) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet sendCommand : partial write");
		}
	}

	char databuffer[OPENWEBNET_BUFFER_SIZE];
	memset(databuffer, 0, OPENWEBNET_BUFFER_SIZE);
	int read = commandSocket.read(databuffer, OPENWEBNET_BUFFER_SIZE, false);
	bt_openwebnet responseSession(string(databuffer, read));
	_log.Log(LOG_STATUS, "COpenWebNet : sent=%s received=%s", OPENWEBNET_COMMAND_SESSION, databuffer);
	if (!responseSession.IsOKFrame()) {
		if (!silent) {
			_log.Log(LOG_STATUS, "COpenWebNet : failed to begin session, NACK received (%s)", m_szIPAddress.c_str(), m_usIPPort, databuffer);
		}
		return false;
	}

	bytesWritten = commandSocket.write(command.frame_open.c_str(), command.frame_open.length());
	if (bytesWritten != command.frame_open.length()) {
		if (!silent) {
			_log.Log(LOG_ERROR, "COpenWebNet sendCommand : partial write");
		}
	}

	if (waitForResponse > 0) {
		sleep_seconds(waitForResponse);
	}

	char responseBuffer[OPENWEBNET_BUFFER_SIZE]; 
	memset(responseBuffer, 0, OPENWEBNET_BUFFER_SIZE);
	read = commandSocket.read(responseBuffer, OPENWEBNET_BUFFER_SIZE, false);

	if (!silent) {
		_log.Log(LOG_STATUS, "COpenWebNet : sent=%s received=%s", command.frame_open.c_str(), responseBuffer);
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
		case WHO_AUTOMATION:
			devType = pTypeBlinds;
			subType = sTypeBlindsT12; 
			switchType = STYPE_Blinds;
			devname = OPENWEBNET_AUTOMATION;
			devname += " " + where;
			break;
		default:
			return false;
		}

		//Insert
		m_sql.safe_query(
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Name, Used) "
			"VALUES (%d,'OpenWebNet',%q,%d,%d,%d,'%q',0)",
			m_HwdID, where.c_str(), devType, subType, switchType, devname.c_str());

		//Get new ID
		vector<vector<string> > result = m_sql.safe_query(
			"SELECT ID FROM DeviceStatus WHERE (HardwareID=%d AND DeviceID='%q' AND Unit='%q' AND Type=%d AND SubType=%d)",
			m_HwdID, "OpenWebNet", where.c_str(), devType, subType);
		if (result.size() == 0)
		{
			_log.Log(LOG_ERROR, "Serious database error, problem getting ID from DeviceStatus!");
			return false;
		}
		std::stringstream s_str(result[0][0]);

		m_sql.m_LastSwitchID = "OpenWebNet";
		s_str >> m_sql.m_LastSwitchRowID;

		m_sql.m_bAcceptNewHardware = false;

		return true;
	}
	return false;
}


bool COpenWebNet::FindDevice(int who, int where, int* used)
{
	vector<vector<string> > result;
	int devType = -1;
	int subType = -1;

	switch (who) {
	case WHO_AUTOMATION : 
		devType = pTypeBlinds;
		subType = sTypeBlindsT12;
		break;
	default:
		return "";
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

		if (frame.estesa) {
			frameStr << " - EXTENDED";
		}

		frameStr << " - who=" << frame.Extract_chi();
		frameStr << " - what=" << frame.Extract_cosa();
		frameStr << " - where=" << frame.Extract_dove();
		if (!frame.Extract_quando().empty()) {
			frameStr << " - when=" << frame.Extract_quando();
		}
		if (!frame.Extract_livello().empty()) {
			frameStr << " - level=" << frame.Extract_livello();
		}
		if (!frame.Extract_interfaccia().empty()) {
			frameStr << " - interface=" << frame.Extract_interfaccia();
		}
		if (!frame.Extract_grandezza().empty()) {
			frameStr << " - dimension=" << frame.Extract_grandezza();
		}

		string indirizzo = frame.Extract_indirizzo(0);
		if (!indirizzo.empty()) {
			int i = 1;
			frameStr << " - address=";
			while (!indirizzo.empty()) {
				frameStr << indirizzo;
				indirizzo = frame.Extract_indirizzo(i++);
				if (!indirizzo.empty()) {
					frameStr << ", ";
				}
			}
		}

		string valori = frame.Extract_valori(0);
		if (!valori.empty()) {
			int i = 1;
			frameStr << " - value=";
			while (!valori.empty()) {
				frameStr << valori;
				indirizzo = frame.Extract_valori(i++);
				if (!valori.empty()) {
					frameStr << ", ";
				}
			}
		}
	}

	return frameStr.str();
}