#include "stdafx.h"
#include "IHCSerial.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../main/SQLHelper.h"
#include "../webserver/cWebem.h"

// This code is for the first version of the IHC Controller, using the RS485 protocol (serial)
// It has been tested with Simon VIS, a spanish version of the IHC controller
//
// Allmost all the code is based in the development made by Jouni Viikari
// But I want also to thank to Thomas Bøjstrup Johansen for his help,
// and also to Michael Odälv and Martin Hejnfelt for their code 
// that allowed me to understand the IHC serial protocol
//
// https://en.wikipedia.org/wiki/Intelligent_Home_Control
//
// Packet format
//
// STX + ID + Type + Data + ETB + Checksum
//
// Checksum = (STX + ID + Type + Data + ETB) & 0xFF
//

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "hardwaretypes.h"

#include <ctime>

#define IHC_RETRY_DELAY 30
#define IHC_COMMAND_DELAY 5
#define IHC_READ_BUFFER_MASK (IHC_READ_BUFFER_SIZE - 1)
#define IHC_MAX_ERRORS_UNITL_RESTART 20


//
//Class IHCSerial
//
CIHCSerial::CIHCSerial(const int ID, const std::string& devname, unsigned int baudRate /*= 115200*/) :
	m_szSerialPort(devname)
{
	m_HwdID=ID;
    m_errorcntr = 0;

	m_stoprequested=false;
	m_retrycntr = 0;
	m_pollcntr = 0;
	m_pollDelaycntr = 0;

	// Init part from ihcHsg
	buffLen = 0;
	outBuffLen = 0;
	nrParErr = 0;
	totParErr = 0;
	nrStatusRcvd = 0;
	nrStatReq = 0;
	nrCmdBuilt = 0;
	nrReadyRcvd = 0;
	inMsg = false;
	inDataSection = false;
	memset(outputStatus, 0, sizeof(outputStatus));
	memset(inputStatus, 0, sizeof(inputStatus));
	timeStamp = time(NULL);


	// Init part from ihcHandler
	nrParErr = 0;
	command = STATUS;
	memset(portsOFF, 0, sizeof(portsOFF));
	memset(portsON, 0, sizeof(portsON));
	inputPort = 0;
	Queue = 0;
	packetCount = 0;
	lastCmd = 0;
}

CIHCSerial::~CIHCSerial()
{
	StopHardware();
}

bool CIHCSerial::StartHardware()
{
	m_retrycntr=IHC_RETRY_DELAY; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CIHCSerial::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CIHCSerial::StopHardware()
{
	if (m_thread != NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    sleep_milliseconds(10);
    terminate();
	m_bIsStarted=false;
	return true;
}


void CIHCSerial::Do_Work()
{
	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;

		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_STATUS,"IHC: serial retrying in %d seconds...", IHC_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=IHC_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_pollcntr = 0;
				m_pollDelaycntr = 0;
				OpenSerialDevice();
			}
		}
		else if(m_errorcntr > IHC_MAX_ERRORS_UNITL_RESTART)
        {
            // Reopen the port and clear the error counter.
			terminate();

		    _log.Log(LOG_ERROR,"IHC: Reopening serial port");
		    sleep_seconds(2);

			m_retrycntr=0;
			m_pollcntr = 0;
			m_pollDelaycntr = 0;
            m_errorcntr = 0;

		    OpenSerialDevice();
        }
        else
		{
			m_pollDelaycntr++;

			if (m_pollDelaycntr>=IHC_COMMAND_DELAY)
			{
                // Before issueing a new command, the recieve buffer must be empty.
                // It seems that there are errors sometimes and this will take care
                // of any problems even if it bypasses the circular buffer concept.
                // There should not be any data left to recieve anyway since commands
                // are sent with 5 seconds in between.
   				m_pollDelaycntr = 0;
			}
		}
	}
	_log.Log(LOG_STATUS,"IHC: Serial Worker stopped...");
} 


bool CIHCSerial::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 19200);
		_log.Log(LOG_STATUS,"IHC: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"IHC: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"IHC: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	setReadCallback(boost::bind(&CIHCSerial::readCallback, this, _1, _2));
	sOnConnected(this);
	return true;
}

void CIHCSerial::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);

	if (!m_bEnableReceive)
		return; //receiving not enabled

	msgStatus_t retVal;
	static int msgLen = 0;
	m_ihcChanged = 0;

	retVal = HandleChars(data, len);

	if (retVal == IHC_READY) {
		command = (CIHCSerial::ihcCmd_t)SendNextPacket();

		switch (command) {
		case STATUS:
			msgLen = BuildMsg(B_STATUS);
			break;
		case STATUS_INPUT:
			msgLen = BuildMsg(B_STATUS_INPUT);
			break;
		case ON:
			msgLen = BuildMsg(B_ON, portsON);
			//bzero(portsON, sizeof(portsON));
			memset(portsON, 0, sizeof(portsON));
			break;
		case OFF:
			msgLen = BuildMsg(B_OFF, portsOFF);
			//bzero(portsOFF, sizeof(portsOFF));
			memset(portsOFF, 0, sizeof(portsOFF));
			break;
		case INPUT:
			msgLen = BuildMsg(B_INPUT, (char *)&inputPort);
			inputPort = 0;
			break;
		case SEND_AGAIN:
			// Just send last message again
			break;
		default:
			msgLen = 0;
			break;
		}
		DelQueue(command);
		//if (packetQueue.size() > 0) {
		//	packetQueue.pop_front();
		//}

		if (msgLen > 0) WriteToHardware(OutCmdMsg(), msgLen);
	}

	//changed = OutputChanged();
	m_ihcChanged = OutputChanged();
	if (m_ihcChanged) {
		//OutStatus(OutputStatus(), curStat);
		memcpy(m_lastStatus, m_currStatus, sizeof(m_currStatus));
		OutStatus(OutputStatus(), m_currStatus);
		UpdateSwitch();
		_log.Log(LOG_STATUS, "After Update Switch");
	}
}

void CIHCSerial::UpdateSwitch()
{
	int module;
	int output;

	if (m_ihcChanged) {
		for (module = 0; module < 16; module++)
		{
			for (output = 0; output < 8; output++)
			{
				if (m_currStatus[module * 8 + output] != m_lastStatus[module * 8 + output]) {
					
					char szModule[3];
					sprintf(szModule, "%d", (int)(module + 65));
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)", m_HwdID, szModule, output + 1);
					if (!result.empty())
					{
						//check if we have a change, if not do not update it
						int nvalue = atoi(result[0][1].c_str());
						char szValue2[2];
						szValue2[0] = m_currStatus[module * 8 + output];
						szValue2[1] = '\0';
						int nvalue2 = atoi( szValue2);

						if (nvalue != nvalue2 && std::strcmp(result[0][0].c_str(),"Unknown") != 0 ) {
							//Send as Lighting 1
							tRBUF lcmd;
							memset(&lcmd, 0, sizeof(RBUF));
							lcmd.LIGHTING1.packetlength = sizeof(lcmd.LIGHTING2) - 1;
							lcmd.LIGHTING1.packettype = pTypeLighting1;
							lcmd.LIGHTING1.subtype = sTypeX10;
							lcmd.LIGHTING1.seqnbr = 0;
							lcmd.LIGHTING1.housecode = module + 65;
							lcmd.LIGHTING1.unitcode = output + 1;
							lcmd.LIGHTING1.filler = 0;
							lcmd.LIGHTING1.rssi = 12;

							if (m_currStatus[module * 8 + output] == '0')
							{
								lcmd.LIGHTING1.cmnd = light1_sOff;
							}
							else
							{
								lcmd.LIGHTING1.cmnd = light1_sOn;
							}

							sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING1, NULL, 255);
						}
					}
				}
			}
		}
	}
}

bool CIHCSerial::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (!isOpen())
		return false;
	write(pdata,length);
	return true;
}

//Webserver helpers
namespace http {
	namespace server {
	}
}


/*********************************************************

Function AddQueue

Add command to the queue of commands waiting to be
handled

**********************************************************/
inline void CIHCSerial::AddQueue(ihcCmd_t cmd)
{
	//Queue |= cmd;
	packetQueue.push_back(cmd);
}


/*********************************************************

Function DelQueue

Remove command from the queue of commands waiting to be
handled

**********************************************************/
inline void CIHCSerial::DelQueue(ihcCmd_t cmd)
{
	//Queue &= ~cmd;
	if (packetQueue.size() > 0) {
		packetQueue.pop_front();
	}
}


/*********************************************************

Function IsQueued

Is command in the queue of commands waiting to be
handled

**********************************************************/
inline bool CIHCSerial::IsQueued(ihcCmd_t cmd)
{
	//return Queue & cmd;
	return std::find(packetQueue.begin(), packetQueue.end(), cmd) != packetQueue.end();
}

/********************************************************

Function SendCommand

Sends command to ihc

Parameters:
command:  Command to be sent
port:     Port the command applies to

********************************************************/
bool CIHCSerial::SendCommand(ihcCmd_t sendCmd, int cmdPort)
{
	_log.Log(LOG_STATUS, "SEND-COMMAND ");
	switch (sendCmd) {
	case ON:
		ToglePort(cmdPort, portsON);
		AddQueue(ON);
		break;

	case OFF:
		ToglePort(cmdPort, portsOFF);
		AddQueue(OFF);
		break;

	case INPUT:
		if (IsQueued(INPUT))  // input comands are not queued at this version.
			return false;
		inputPort = cmdPort;
		AddQueue(INPUT);
		break;

	case STATUS_INPUT:
		AddQueue(STATUS_INPUT);
		break;

	default:
		// No other commands not supported by ihc_c class
		break;
	}
	return true;
}


/*********************************************************

Function Schedule

Return next command from the queue of commands which'
turn is to be handled

**********************************************************/
unsigned int CIHCSerial::Schedule()
{
	static unsigned int prevCmd = STATUS;

	if (!Queue) {
		prevCmd = STATUS;
		return STATUS;
	}

	prevCmd = prevCmd ? (prevCmd << 1) : 1;

	while (!IsQueued((ihcCmd_t)prevCmd)) {
		prevCmd <<= 1;
		if (prevCmd > LAST_COMMAND) {
			prevCmd = STATUS;
			return STATUS;
		}
	}

	return prevCmd;
}

unsigned int CIHCSerial::SendNextPacket()
{
	static unsigned int nextCmd = 0;

	if (packetCount++ > 2) {
		if (packetQueue.size() > 0) {
			nextCmd = packetQueue.front();
		}
		else {
			nextCmd = STATUS;
		}
		packetCount = 0;
	}
	else {
		nextCmd = STATUS_INPUT;
		packetCount++;
	}

	return nextCmd;

	if (packetQueue.size() > 0) {
		nextCmd = packetQueue.front();
		//DelQueue((ihcCmd_t)nextCmd);
	}
	else {
		if (lastCmd == STATUS) {
			lastCmd = nextCmd = STATUS_INPUT;
		}
		else {
			lastCmd = nextCmd = STATUS;
		}
	}

	return nextCmd;
}

/**********************************************************

Function toglePort

Sets the correspondent bit for port up in port array

Parameters:
port:     Port nr on which the bit should be turned on
portArr:  Array to hold bits for all ihc outputs.

*********************************************************/
void CIHCSerial::ToglePort(int port, char * portArr)
{
	int dev = port / 10;
	int devLine = port % 10;
	char line = '\x01' << (devLine - 1);

	if ((devLine == 0) || (devLine == 9))
		return;

	if ((dev >= MAX_OUTPUT_DEVS) || (dev < 0))
		return;

	portArr[dev] |= line;
}


/********************************************************************

Function outStatus

Generates array of '1':s and '0':s meaning the output statuses
of various ihc ports.

Parameters:
pollStat:  The status data from IHC (lines separate bits)
oStat:     The status data in chars ('1':s or '0':s)

********************************************************************/
void CIHCSerial::OutStatus(const char * pollStat, char * oStat) const
{
	char mask;

	for (int i = 0; i < 16; i++) {
		mask = '\x1';
		for (int j = 0; j < 8; j++) {
			if (pollStat[i] & mask) {
				oStat[(i * 8) + j] = '1';
			}
			else {
				oStat[(i * 8) + j] = '0';
			}
			mask <<= 1;
		}
	}
}

/*****************************************************************

Function isOn

Tells if given port is switched on

Parameters:
port:     The port number to be checked

*****************************************************************/
bool CIHCSerial::IsOn(int port) const
{
	int dev = port / 10;
	int devLine = port % 10;

	if ((devLine == 0) || (devLine == 9))
		return false;

	char line = '\x01' << (devLine - 1);
	return (OutputStatus()[dev] & line);
}



/**************************************

HasdataSection()

Does the started arriving message contain data section
which can contain ANY bytes (including magic \x02 and \x17)

***************************************/
bool CIHCSerial::HasDataSection()
{
	char msgTypeChar = buff[1];
	char cmdChar = buff[2];

	switch (msgTypeChar) {

	case CHAR_MSG_FROM_IHC:
		if ((cmdChar == CHAR_OUTPUT_STATUS) || (cmdChar == CHAR_INPUT_STATUS)) {
			dataSectionEnd = 21;  // Two past, so *prev* read is not data.. and to be sure to read the checksum...
			return true;
		}

	case CHAR_MSG_TO_IHC:
		if ((cmdChar == CHAR_SWITCH_ON) || (cmdChar == CHAR_SWITCH_OFF)) {
			dataSectionEnd = 21;
			return true;
		}
		if (cmdChar == CHAR_INPUT_ON) {
			dataSectionEnd = 7;
			return true;
		}

	}
	return false;
}

/**************************************

ParityOk()

Is parity bit correct in received message

***************************************/
bool CIHCSerial::ParityOk()
{
	char checkSum = 0;
	int i = 0;
	int lastByte = buffLen - 1;

	do {
		checkSum += buff[i++];
	} while (i < lastByte);

	if (buff[i] == checkSum) {
		totParErr += nrParErr;
		nrParErr = 0;
		return true;
	}
	else {
		nrParErr++;
		_log.Log(LOG_STATUS, "Parity ERROR: ");
		return false;
	}
}

/**************************************

BuildMsg

Prepare a message to be sent to IHC

Parameters:
cmd:   Command we wish to send to IHC
data:  Parameters to those commands needing ones.

Returns number of bytes in the built message

***************************************/
int CIHCSerial::BuildMsg(ihcCmd_t_Build cmd, const char * data)
{
	char checkSum = 0;

	outBuffLen = 0;
	outBuff[outBuffLen++] = CHAR_START;
	outBuff[outBuffLen++] = CHAR_MSG_TO_IHC;

	switch (cmd) {

	case B_STATUS:
		// Do we want 'fast path' for this (you do this continuously)
		// Save permanently this char[5]?
		outBuff[outBuffLen++] = CHAR_REQUEST_STATUS;
		nrStatReq++;
		break;

	case B_STATUS_INPUT:
		outBuff[outBuffLen++] = CHAR_REQUEST_INPUT;
		nrStatReq++;
		break;

	case B_ON:
		outBuff[outBuffLen++] = CHAR_SWITCH_ON;
		_log.Log(LOG_STATUS, "ON ");
		if (data) {
			memcpy(&outBuff[outBuffLen], data, 16);
			outBuffLen += 16;
			nrCmdBuilt++;
		}
		else {
			outBuffLen = 0;
			return outBuffLen;
		}
		break;

	case B_OFF:
		outBuff[outBuffLen++] = CHAR_SWITCH_OFF;
		_log.Log(LOG_STATUS, "OFF ");
		if (data) {
			memcpy(&outBuff[outBuffLen], data, 16);
			//memset(&outBuff[outBuffLen], 255, 16); test purposes, switch off all outputs
			outBuffLen += 16;
			nrCmdBuilt++;
		}
		else {
			outBuffLen = 0;
			return outBuffLen;
		}
		break;

	case B_INPUT:
		outBuff[outBuffLen++] = CHAR_INPUT_ON;
		if (data) {
			outBuff[outBuffLen++] = data[0];
			nrCmdBuilt++;
		}
		else {
			outBuffLen = 0;
			return outBuffLen;
		}
		outBuff[outBuffLen++] = '\x01';
		break;

	default:
		outBuffLen = 0;
		return outBuffLen;
		break;
	}

	outBuff[outBuffLen++] = CHAR_END;

	// Add checkSum
	for (int i = 0; i < outBuffLen; i++) {
		checkSum += outBuff[i];
	}
	outBuff[outBuffLen++] = checkSum;

	_log.Log(LOG_STATUS, "BuildMsg: ");

	return outBuffLen;
}

/**************************************

HandleMessage

Checks what is the received message in buffer.

***************************************/
CIHCSerial::msgStatus_t CIHCSerial::HandleMessage()
{
	msgStatus_t msgStatus = PARITY_ERROR;
	int msgLen = 0;
	int i;
	char strmsg[33];

	char msgTypeChar = buff[1];
	char cmdChar = buff[2];

	_log.Log(LOG_STATUS, "HandleMessage: ");

	if (msgTypeChar == CHAR_MSG_FROM_IHC) {
		// Message from IHC to us
		switch (cmdChar) {

		case CHAR_READY:
			_log.Log(LOG_STATUS, "IHC_READY rcvd ");
			if (ParityOk()) {
				msgStatus = IHC_READY;
				msgLen = 5;
				if (msgLen == buffLen) {
					nrReadyRcvd++;
					timeStamp = time(NULL);
				}
			}
			break;

		case CHAR_OUTPUT_STATUS:
			for (i = 0; i < 16; i++) {
				sprintf(&strmsg[i*2], "%02X", (unsigned char)buff[i+3]);
			}
			_log.Log(LOG_STATUS, "NEW output status rcvd: ");
			_log.Log(LOG_STATUS, strmsg);
			if (ParityOk()) {
				msgStatus = OUTPUT_STATUS;
				msgLen = 21;
				if (msgLen == buffLen) {
					rcvdOutput = true;
					nrStatusRcvd++;
					if ((memcmp(outputStatus, &buff[3], 16) != 0)) {
						memcpy(outputStatus, &buff[3], 16);
						_log.Log(LOG_STATUS, "Output CHANGED ");
						outputChanged = true;
					}
				}
			}
			break;

		case CHAR_INPUT_STATUS:
			_log.Log(LOG_STATUS, "NEW input status rcvd: ");
			if (ParityOk()) {
				msgStatus = INPUT_STATUS;
				msgLen = 21;
				if (msgLen == buffLen) {
					rcvdInput = true;
					nrStatusRcvd++;
					if ((memcmp(inputStatus, &buff[3], 16) != 0)) {
						memcpy(inputStatus, &buff[3], 16);
						_log.Log(LOG_STATUS, "Input changed. ");
						inputChanged = true;
					}
				}
			}
			break;

		default:
			msgStatus = OTHER;
			_log.Log(LOG_STATUS, "Unknown message rcvd: %02X", cmdChar);
			// We do not waste CPU on calculating parity here
			//if (!ParityOk()) msgStatus = PARITY_ERROR;
			break;
		}

	}
	else if (msgTypeChar == CHAR_MSG_TO_IHC) {
		// Messagege originally from us to IHC -> local ECHO.
		msgStatus = ECHO_MSG;
		_log.Log(LOG_STATUS, "Echo rcvd");
	}
	else {
		_log.Log(LOG_STATUS, "Unknown message type rcvd: %02X", msgTypeChar);
		msgStatus = OTHER;
		// We do not waste CPU on calculating parity here
		//if (!ParityOk()) msgStatus = PARITY_ERROR;
	}

	if ((msgLen) && (buffLen != msgLen)) {
		_log.Log(LOG_STATUS, " - Msg length did NOT match! ");
		nrParErr++;
		msgStatus = IHC_ERROR;
	}

	_log.Log(LOG_STATUS, "\n");
	buffLen = 0;
	return msgStatus;
}

/**************************************

HandleChars

Parse latest bytes received from IHC.  Handles ALL
full messages in str.

Parameters:
str:  Pointer to char * containing arrived bytes
n:    Number of bytes in str

Returns the type of LAST message (if not followed by
no other bytes.

***************************************/
CIHCSerial::msgStatus_t CIHCSerial::HandleChars(const char * str, int n)
// Checksum can be 02 or 17... - should not happen in normal commands w/o data section?
{
	int i = 0;
	char ch;
	static msgStatus_t rcvdMsg = EMPTY;

	_log.Log(LOG_STATUS, "\n");
	outputChanged = inputChanged = rcvdOutput = rcvdInput = false;
	while (i < n) {
		ch = str[i++];

		// Beginning of new message?
		if ((ch == CHAR_START) && !inDataSection) {
			inMsg = true;
			buff[0] = CHAR_START;
			buffLen = 1;   // Previous (even partial) msg can be thrown away
			rcvdMsg = INCOMPLETE;
			_log.Log(LOG_STATUS, "\nStart 02 ");
			continue;
		}

		// Inside message?
		if (inMsg) {

			if (buffLen >= IHC_BUFFER_SIZE) {
				// We should maybe even abort...
				_log.Log(LOG_STATUS, "Full buffer ERROR!\n");
				inMsg = false;
				buffLen = 0;
				nrParErr++;
				rcvdMsg = EMPTY;
			}

			buff[buffLen++] = ch;
			//_log.Log(LOG_ERROR, "ch %d",ch);

			// Let's check if we are in datasection which can contain all chars
			if (buffLen == dataSectionBegin) {
				inDataSection = HasDataSection();
#ifdef PRINT_TRACE
				if (trace && inDataSection) printf("DataB ");
#endif
			}
			// Have we passed allready the datasection?
			// When inside data section only thing which stops it is getting enough bytes.
			if (buffLen == dataSectionEnd) {
				inDataSection = false;
				dataSectionEnd = 128;
				_log.Log(LOG_STATUS, "DataE ");
			}

			// Was previous char the end char?
			// If so, current one is parity bit.  Let's process the message
			if ((buff[buffLen - 2] == CHAR_END) && (!inDataSection)) {
				inMsg = false;
				_log.Log(LOG_STATUS, "\n");
				rcvdMsg = HandleMessage();
			}
		}
		else {
			// Not in Msg or beginning of new message
			rcvdMsg = EMPTY;
		}
	}

	// Note that return value is IHC_READY only if this message was the last characters received.
	// This is exactly what we want, otherwise the message can be too old and not valid any more.
	return rcvdMsg;
}



