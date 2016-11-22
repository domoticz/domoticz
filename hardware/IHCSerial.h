#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define IHC_READ_BUFFER_SIZE 32

typedef struct _tIHCTemp {
	unsigned char len;
	unsigned char type;
	unsigned char subtype;
	char          ID[25];
	float         temperature;
} IHCTemp;

typedef struct _tIHCStatus {
	unsigned char   len;
	unsigned char   type;
	unsigned char   subtype;
	char            ID[25];
	int             value;
} IHCStatus;

// Packet format
//
// STX + ID + Type + Data + ETB + Checksum
//
// Checksum = (STX + ID + Type + Data + ETB) & 0xFF
//

namespace IHCDefs {
	// Transmission bytes
	const unsigned char PKT_SOH = 0x01;
	const unsigned char PKT_STX = 0x02;
	const unsigned char PKT_ACK = 0x06;
	const unsigned char PKT_ETB = 0x17;

	// Commands
	const unsigned char PKT_DATA_READY = 0x30;
	const unsigned char PKT_SET_OUTPUT = 0x7A;
	const unsigned char PKT_GET_OUTPUTS = 0x82;
	const unsigned char PKT_OUTP_STATE = 0x83;
	const unsigned char PKT_GET_INPUTS = 0x86;
	const unsigned char PKT_INP_STATE = 0x87;
	const unsigned char PKT_ACT_INPUT = 0x88;

	// Receiver IDs
	const unsigned char ID_DISPLAY = 0x09;
	const unsigned char ID_MODEM = 0x0A;
	const unsigned char ID_IHC = 0x12;
	const unsigned char ID_AC = 0x1B;
	const unsigned char ID_PC = 0x1C;
	const unsigned char ID_PC2 = 0x1D;
};

class UART;

class CIHCSerial: public AsyncSerial, public CDomoticzHardwareBase
{
	// ===================================
	// ============= PUBLIC===============
	// ===================================
public:
    /**
    * Opens a serial device.
    * \param devname serial device name, example "/dev/ttyS0" or "COM1"
    * \param baud_rate serial baud rate
    * \param opt_parity serial parity, default even
    * \param opt_csize serial character size, default 7bit
    * \param opt_flow serial flow control, default none
    * \param opt_stop serial stop bits, default 1
    * \throws boost::system::system_error if cannot open the
    * serial device
    */
	CIHCSerial(const int ID, const std::string& devname, unsigned int baudRate = 19200);

    ~CIHCSerial();
	std::string m_szSerialPort;

	IHCTemp	    m_IHCTemp;
	IHCStatus	m_IHCValue;

	char   m_currStatus[2 + 130] = "o ";        // Last output status information read from IHC
	char   m_inpStatus[2 + 130] = "i ";        // Status information for inputs
	char   m_lastStatus[2 + 130] = "o ";        // Last output status information read from IHC
	bool   m_ihcChanged;                      // Has any output port on IHC changed in last poll

	std::list<int> packetQueue;

	bool WriteToHardware(const char *pdata, const unsigned char length);

	// ################## 
	// ihcMsg PUBLIC
	// ##################

	// Allowed comands for BuildMsg
	enum ihcCmd_t_Build { B_UNKNOWN, B_STATUS, B_STATUS_INPUT, B_ON, B_OFF, B_INPUT };

	// Possible message types from  HandleChars
	enum msgStatus_t { EMPTY, INCOMPLETE, PARITY_ERROR, IHC_ERROR, IHC_READY, OUTPUT_STATUS, INPUT_STATUS, ECHO_MSG, OTHER };

	/*****************************************************

	HandleChars

	Parse latest bytes received from IHC.  Handles ALL
	full messages in str.

	Parameters:
	str:  Pointer to char * containing arrived bytes
	n:    Number of bytes in str

	Returns the type of LAST message (if not followed by
	no other bytes.

	*****************************************************/
	msgStatus_t HandleChars(const char * str, int n);

	/*****************************************************

	BuildMsg

	Prepare a message to be sent to IHC

	Parameters:
	cmd:   Command we wish to send to IHC
	data:  Parameters to those commands needing ones.

	Returns number of bytes in the built message

	*****************************************************/
	int BuildMsg(ihcCmd_t_Build cmd, const char * data = 0);

	/*****************************************************

	SetTrace

	Set trace on or off

	Parameters:
	status:  Wished status for tracing

	*****************************************************/
	void SetTrace(bool status);

	/*****************************************************

	OutCmdMsg

	Returns char * pointing to last message built by
	BuildMsg for sending to IHC

	*****************************************************/
	inline const char * OutCmdMsg() const { return outBuff; }

	/*****************************************************

	OutputStatus

	Get pointer to char[16] containing the current status
	of outputs

	*****************************************************/
	inline const char * OutputStatus() const { return outputStatus; }

	/*****************************************************

	InputStatus

	Get pointer to char[16] containing the current status
	of inputs

	*****************************************************/
	inline const char * InputStatus() const { return inputStatus; }

	/*****************************************************

	OutputChanged

	Has any output status changed compared to previous
	information

	*****************************************************/
	inline bool OutputChanged() const { return outputChanged; }

	/*****************************************************

	InputChanged

	Has any input status changed compared to previous
	information

	*****************************************************/
	inline bool InputChanged() const { return inputChanged; }

	/*****************************************************

	OutputRcvd

	Has output status just been received

	*****************************************************/
	inline bool OutputRcvd() const { return rcvdOutput; }

	/*****************************************************

	InputRcvd

	Has input status just been received

	*****************************************************/
	inline bool InputRcvd() const { return rcvdInput; }


	/*****************************************************

	LastReadyTime

	Return seconds since last "READY" read from IHC

	*****************************************************/
	inline time_t LastReadyTime() const { return (time(NULL) - timeStamp); }

	/*****************************************************

	Statistic retrieval function

	To retrieve varous statistical information about
	communication to IHC

	*****************************************************/

	inline unsigned long ParityErrors() const { return nrParErr; }
	inline unsigned long TotParityErrors() const { return (nrParErr + totParErr); }
	inline unsigned long NrStatusRcvd() const { return nrStatusRcvd; }
	inline unsigned long NrCmdBuilt() const { return nrCmdBuilt; }
	inline unsigned long NrReadyRcvd() const { return nrReadyRcvd; }
	inline unsigned long NrStatReq() const { return nrStatReq; }

	// Command to passed in SendCommand
#define bit(n) (1 << (n-1))
	enum ihcCmd_t	 {
		STATUS = 0,
		ON = bit(1),
		OFF = bit(2),
		INPUT = bit(3),
		STATUS_INPUT = bit(4),
		SEND_AGAIN = bit(8)
	};
#undef bit

	/*****************************************************

	Function SendCommand

	Sends command to ihc

	Parameters:
	command:  Command to be sent
	port:     Port the command applies to

	*******************************************************/
	bool SendCommand(ihcCmd_t sendCmd, int cmdPort = 0);

	/******************************************************

	Function IsOn

	Tells if given port is switched on

	Parameters:
	port:     The port number to be checked

	*******************************************************/
	bool IsOn(int port) const;


	// ===================================
	// ============= PRIVATE =============
	// ===================================
private:
	void Init();
	bool StartHardware();
	bool StopHardware();
	bool OpenSerialDevice();
	void Do_Work();	
	
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	int m_pollDelaycntr;
	unsigned int m_pollcntr;
    int m_regoType;
    unsigned int m_errorcntr;

	// Create a circular buffer.
    char m_readBuffer[IHC_READ_BUFFER_SIZE];
	volatile unsigned char m_readBufferHead;
	volatile unsigned char m_readBufferTail;

	/**
     * Read callback, stores data in the buffer
     */
	void readCallback(const char *data, size_t len);

	void UpdateSwitch();

	//IHCRS485Packet getPacket(UART& uart, int ID = IHCDefs::ID_PC, bool useTimeout = true) throw (bool);

	// HANDLER
	//static const ihcCmd_t_Build LAST_COMMAND = B_STATUS_INPUT;
	static const ihcCmd_t LAST_COMMAND = STATUS_INPUT;
	static const int MAX_OUTPUT_DEVS = 16;

	char portsOFF[MAX_OUTPUT_DEVS];   // What ports shall be switched off
	char portsON[MAX_OUTPUT_DEVS];    // What ports shall be turned on
	int  inputPort;                   // Port to which input command implies.
	unsigned int Queue;               // Contains a bit for all commands whish are in queue //bitset
	unsigned int packetCount;         // Used to make sure InputStates are read most often and OutputStates not as often
	unsigned int lastCmd;
	ihcCmd_t command;                 // Comand to be sent next to ihc
	ihcCmd_t command2;                 // Comand to be sent next to ihc

	// Private methods
	void AddQueue(ihcCmd_t cmd);                               // Add command to queue
	void DelQueue(ihcCmd_t cmd);                               // Remove command from queue
	bool IsQueued(ihcCmd_t cmd);                               // Is command queued
	unsigned int Schedule();                                   // Return next command to be handled
	unsigned int SendNextPacket();                                   // Return next command to be handled
	void OutStatus(const char * pollStat, char * oStat) const; // Generates status in chars
	void ToglePort(int port, char * portArr);

	// ################## 
	// ihcMsg PRIVATE
	// ##################

	enum ihcCmdChar_t {
		CHAR_READY = '\x30',
		CHAR_OUTPUT_STATUS = '\x83',
		CHAR_INPUT_STATUS = '\x87',
		CHAR_REQUEST_STATUS = '\x82',
		CHAR_SWITCH_ON = '\x84',
		CHAR_SWITCH_OFF = '\x85',
		CHAR_REQUEST_INPUT = '\x86',
		CHAR_INPUT_ON = '\x88'
	};

	enum ihcMsgType_t {
		CHAR_MSG_FROM_IHC = '\x1c',
		CHAR_MSG_TO_IHC = '\x12'
	};

	static const char   CHAR_START = '\x02';
	static const char   CHAR_END = '\x17';

	static const int dataSectionBegin = 3;
	//static const int BUFFER_SIZE = 255;
#define IHC_BUFFER_SIZE 255 

	char buff[IHC_BUFFER_SIZE];      // Buffer to hold incoming chars from IHC
	char outBuff[IHC_BUFFER_SIZE];   // Buffer to hold outgoing messages to be sent to IHC
	int buffLen, outBuffLen;     // Current lengths of above buffers
	char outputStatus[16];       // Status of IHC outputs
	char inputStatus[16];        // Status of IHC inputs
	unsigned long nrParErr, totParErr, nrStatusRcvd, nrStatReq, nrCmdBuilt, nrReadyRcvd;  // Statistic
	bool inMsg, inDataSection;   // State information about the parsing of incoming data from IHC
	int dataSectionEnd;          // State information about the IHC data currently being parsed
	bool inputChanged, outputChanged, rcvdInput, rcvdOutput;  // Info about parsed data during last HandleChars call
	bool trace;                  // Shall trace be printed
	time_t timeStamp;            // Time when last IHC_READY received

								 // Checks what is the received message in buffer.
	msgStatus_t HandleMessage();

	//  Is parity bit correct in received message
	bool ParityOk();

	// Does the started arriving message contain data section 
	// which can contain ANY bytes (including magic \x02 and \x17)
	bool HasDataSection();
};


// Class IHCRS485Packet 
//
//
class IHCRS485Packet {
public:
	IHCRS485Packet(unsigned char id,
		unsigned char dataType,
		const std::vector<unsigned char>* data = NULL) :
		m_id(id),
		m_dataType(dataType),
		m_isComplete(false)
	{
		m_packet.push_back(IHCDefs::PKT_STX);
		m_packet.push_back(id);
		m_packet.push_back(dataType);
		if (data != NULL) {
			for (unsigned int j = 0; j < data->size(); j++) {
				m_packet.push_back((*data)[j]);
				m_data.push_back((*data)[j]);
			}
		}
		m_packet.push_back(IHCDefs::PKT_ETB);
		unsigned int crc = 0;
		for (unsigned int j = 0; j < m_packet.size(); j++) {
			crc += m_packet[j];
		}
		m_packet.push_back((unsigned char)(crc & 0xFF));
		m_isComplete = true;
	};

	IHCRS485Packet(const std::vector<unsigned char>& data) :
		m_isComplete(false)
	{
		unsigned char crc = 0;
		if (data.size() > 2 && data[0] == IHCDefs::PKT_STX) {
			m_id = data[1];
			m_dataType = data[2];
			crc += data[0];
			crc += data[1];
			crc += data[2];
			m_packet.push_back(data[0]);
			m_packet.push_back(data[1]);
			m_packet.push_back(data[2]);
			for (unsigned int j = 3; j < data.size(); j++) {
				m_packet.push_back(data[j]);
				crc += data[j];
				if (data[j] != IHCDefs::PKT_ETB) {
					m_data.push_back(data[j]);
				}
				else if (data[j] == IHCDefs::PKT_ETB) {
					if ((j + 1) == (data.size() - 1)) {
						if (data[j + 1] == (unsigned char)(crc & 0xFF)) {
							m_packet.push_back(data[j + 1]);
							m_isComplete = true;
							break;
						}
					}
				}
			}
		}
	};

	void PrintVector(const std::vector<unsigned char>& data) {
		for (unsigned int j = 0; j < data.size(); j++) {
			printf("%.2X ", data[j]);
		}
		printf("\n");
	};

	void printTimeStamp(time_t t = 0) {
		time_t now = (t == 0 ? time(NULL) : t);
		struct tm* timeinfo;
		timeinfo = localtime(&now);
		printf("%.2i:%.2i:%.2i ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		return;
	};

	void print() {
		printTimeStamp();
		std::string receiver;
		switch (m_id) {
		case IHCDefs::ID_MODEM:
			receiver = "MODEM";
			break;
		case IHCDefs::ID_DISPLAY:
			receiver = "DISPLAY";
			break;
		case IHCDefs::ID_IHC:
			receiver = "IHC";
			break;
		case IHCDefs::ID_AC:
			receiver = "AC";
			break;
		case IHCDefs::ID_PC:
			receiver = "PC";
			break;
		case IHCDefs::ID_PC2:
			receiver = "PC2";
			break;
		default:
			receiver = "UNKNOWN";
			break;
		}
		printf("(%s) ", receiver.c_str());
		PrintVector(m_packet);
	};

	~IHCRS485Packet() {
	};

	bool isComplete() { return m_isComplete; };

	std::vector<unsigned char> getPacket() {
		return m_packet;
	};

	std::vector<unsigned char> getData() {
		return m_data;
	};

	unsigned char getID() {
		return m_id;
	};

	unsigned char getDataType() {
		return m_dataType;
	};

private:
	unsigned char m_id;
	unsigned char m_dataType;
	bool m_isComplete;
	std::vector<unsigned char> m_packet;
	std::vector<unsigned char> m_data;
};
