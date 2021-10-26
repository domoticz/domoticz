#include "stdafx.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <ctime>

#include <boost/exception/diagnostic_information.hpp>

#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"

#include "hardwaretypes.h"
#include "EnOceanESP3.h"

// Enable running SP3 protocol tests
// They tests are launched when ESP3 worker is started
// Just uncomment the needed ones

//#define ENABLE_ESP3_TESTS
#ifdef ENABLE_ESP3_TESTS
// TESTS: Enable ReadCallback tests emulating various input data
//#define READCALLBACK_TESTS

// TESTS: Enable teach-in tests emulating nodes with different RORG
//#define ESP3_TESTS_TEACHIN_NODE

// TESTS: Enable tests emulating nodes with different EEP
//#define ESP3_TESTS_1BS_D5_00_01
//#define ESP3_TESTS_4BS_A5_02_XX
//#define ESP3_TESTS_4BS_A5_04_XX
//#define ESP3_TESTS_4BS_A5_06_01
//#define ESP3_TESTS_4BS_A5_07_01
//#define ESP3_TESTS_4BS_A5_07_02
//#define ESP3_TESTS_4BS_A5_07_03
//#define ESP3_TESTS_4BS_A5_09_04
//#define ESP3_TESTS_4BS_A5_10_0X
//#define ESP3_TESTS_4BS_A5_12_00
//#define ESP3_TESTS_4BS_A5_12_01
//#define ESP3_TESTS_4BS_A5_12_02
//#define ESP3_TESTS_4BS_A5_12_03
//#define ESP3_TESTS_RPS_F6_01_01
//#define ESP3_TESTS_RPS_F6_02_0X
//#define ESP3_TESTS_RPS_F6_05_00
//#define ESP3_TESTS_RPS_F6_05_01
//#define ESP3_TESTS_RPS_F6_05_02
//#define ESP3_TESTS_VLD_D2_01_12
//#define ESP3_TESTS_VLD_D2_03_0A
//#define ESP3_TESTS_VLD_D2_14_30
#endif // ENABLE_ESP3_TESTS

// ESP3 Packet types
enum ESP3_PACKET_TYPE : uint8_t
{
	PACKET_RADIO_ERP1 = 0x01,		// Radio telegram
	PACKET_RESPONSE = 0x02,			// Response to any packet
	PACKET_RADIO_SUB_TEL = 0x03,	// Radio subtelegram
	PACKET_EVENT = 0x04,			// Event message
	PACKET_COMMON_COMMAND = 0x05,	// Common command
	PACKET_SMART_ACK_COMMAND = 0x06, // Smart Acknowledge command
	PACKET_REMOTE_MAN_COMMAND = 0x07, // Remote management command
	PACKET_RADIO_MESSAGE = 0x09,	// Radio message
	PACKET_RADIO_ERP2 = 0x0A,		// ERP2 protocol radio telegram
	PACKET_CONFIG_COMMAND = 0x0B,	// RESERVED
	PACKET_COMMAND_ACCEPTED = 0x0C,	// For long operations, informs the host the command is accepted
	PACKET_RADIO_802_15_4 = 0x10,	// 802_15_4 Raw Packet
	PACKET_COMMAND_2_4 = 0x11		// 2.4 GHz Command
};

// ESP3 Return codes
enum ESP3_RETURN_CODE : uint8_t
{
	RET_OK = 0x00,					// OK ... command is understood and triggered
	RET_ERROR = 0x01,				// There is an error occured
	RET_NOT_SUPPORTED = 0x02,		// The functionality is not supported by that implementation
	RET_WRONG_PARAM = 0x03,			// There was a wrong parameter in the command
	RET_OPERATION_DENIED = 0x04,	// Example: memory access denied (code-protected)
	RET_LOCK_SET = 0x05,			// Duty cycle lock
	RET_BUFFER_TO_SMALL = 0x06,		// The internal ESP3 buffer of the device is too small, to handle this telegram
	RET_NO_FREE_BUFFER = 0x07,		// Currently all internal buffers are used
	RET_MEMORY_ERROR = 0x82,		// The memory write process failed
	RET_BASEID_OUT_OF_RANGE = 0x90, // BaseID out of range
	RET_BASEID_MAX_REACHED = 0x91	// BaseID has already been changed 10 times, no more changes are allowed
};

// ESP3 Event codes
enum ESP3_EVENT_CODE : uint8_t
{
	SA_RECLAIM_NOT_SUCCESSFUL = 0x01, // Informs the external host about an unsuccessful reclaim by a Smart Ack. client
	SA_CONFIRM_LEARN = 0x02,		// Request to the external host about how to handle a received learn-in / learn-out of a Smart Ack. client
	SA_LEARN_ACK = 0x03,			// Response to the Smart Ack. client about the result of its Smart Acknowledge learn request
	CO_READY = 0x04,				// Inform the external about the readiness for operation
	CO_EVENT_SECUREDEVICES = 0x05,	// Informs the external host about an event in relation to security processing
	CO_DUTYCYCLE_LIMIT = 0x06,		// Informs the external host about reaching the duty cycle limit
	CO_TRANSMIT_FAILED = 0x07,		// Informs the external host about not being able to send a telegram
	CO_TX_DONE = 0x08,				// Informs that all TX operations are done
	CO_LRN_MODE_DISABLED = 0x09		// Informs that the learn mode has time-out
};

// ESP3 Common commands
enum ESP3_COMMON_COMMAND : uint8_t
{
	CO_WR_SLEEP = 0x01,				// Enter energy saving mode
	CO_WR_RESET = 0x02,				// Reset the device
	CO_RD_VERSION = 0x03,			// Read the device version information
	CO_RD_SYS_LOG = 0x04,			// Read system log
	CO_WR_SYS_LOG = 0x05,			// Reset system log
	CO_WR_BIST = 0x06,				// Perform Self Test
	CO_WR_IDBASE = 0x07,			// Set ID range base address
	CO_RD_IDBASE = 0x08,			// Read ID range base address
	CO_WR_REPEATER = 0x09,			// Set Repeater Level
	CO_RD_REPEATER = 0x10,			// Read Repeater Level
	CO_WR_FILTER_ADD = 0x11,		// Add filter to filter list
	CO_WR_FILTER_DEL = 0x12,		// Delete a specific filter from filter list
	CO_WR_FILTER_DEL_ALL = 0x13,	// Delete all filters from filter list
	CO_WR_FILTER_ENABLE = 0x14,		// Enable / disable filter list
	CO_RD_FILTER = 0x15,			// Read filters from filter list
	CO_WR_WAIT_MATURITY = 0x16,		// Wait until the end of telegram maturity time before received radio telegrams will be forwarded to the external host
	CO_WR_SUBTEL = 0x17,			// Enable / Disable transmission of additional subtelegram info to the external host
	CO_WR_MEM = 0x18,				// Write data to device memory
	CO_RD_MEM = 0x19,				// Read data from device memory
	CO_RD_MEM_ADDRESS = 0x20,		// Read address and length of the configuration area and the Smart Ack Table
	CO_RD_SECURITY = 0x21,			// DEPRECATED Read own security information (level, key)
	CO_WR_SECURITY = 0x22,			// DEPRECATED Write own security information (level, key)
	CO_WR_LEARNMODE = 0x23,			// Enable / disable learn mode
	CO_RD_LEARNMODE = 0x24,			// Read learn mode status
	CO_WR_SECUREDEVICE_ADD = 0x25,	// DEPRECATED Add a secure device
	CO_WR_SECUREDEVICE_DEL = 0x26,	// Delete a secure device from the link table
	CO_RD_SECUREDEVICE_BY_INDEX = 0x27,	// DEPRECATED Read secure device by index
	CO_WR_MODE = 0x28,				// Set the gateway transceiver mode
	CO_RD_NUMSECUREDEVICES = 0x29,	// Read number of secure devices in the secure link table
	CO_RD_SECUREDEVICE_BY_ID = 0x30, // Read information about a specific secure device from the secure link table using the device ID
	CO_WR_SECUREDEVICE_ADD_PSK = 0x31, // Add Pre-shared key for inbound secure device
	CO_WR_SECUREDEVICE_ENDTEACHIN = 0x32, // Send Secure teach-In message
	CO_WR_TEMPORARY_RLC_WINDOW = 0x33, // Set a temporary rolling-code window for every taught-in device
	CO_RD_SECUREDEVICE_PSK = 0x34,	// Read PSK
	CO_RD_DUTYCYCLE_LIMIT = 0x35,	// Read the status of the duty cycle limit monitor
	CO_SET_BAUDRATE = 0x36,			// Set the baud rate used to communicate with the external host
	CO_GET_FREQUENCY_INFO = 0x37,	// Read the radio frequency and protocol supported by the device
	CO_38T_STEPCODE = 0x38,			// Read Hardware Step code and Revision of the Device
	CO_40_RESERVED = 0x40,			// Reserved
	CO_41_RESERVED = 0x41,			// Reserved
	CO_42_RESERVED = 0x42,			// Reserved
	CO_43_RESERVED = 0x43,			// Reserved
	CO_44_RESERVED = 0x44,			// Reserved
	CO_45_RESERVED = 0x45,			// Reserved
	CO_WR_REMAN_CODE = 0x46,		// Set the security code to unlock Remote Management functionality via radio
	CO_WR_STARTUP_DELAY = 0x47,		// Set the startup delay (time from power up until start of operation)
	CO_WR_REMAN_REPEATING = 0x48,	// Select if REMAN telegrams originating from this module can be repeated
	CO_RD_REMAN_REPEATING = 0x49,	// Check if REMAN telegrams originating from this module can be repeated
	CO_SET_NOISETHRESHOLD = 0x50,	// Set the RSSI noise threshold level for telegram reception
	CO_GET_NOISETHRESHOLD = 0x51,	// Read the RSSI noise threshold level for telegram reception
	CO_52_RESERVED = 0x52,			// Reserved
	CO_53_RESERVED = 0x53,			// Reserved
	CO_WR_RLC_SAVE_PERIOD = 0x54,	// Set the period in which outgoing RLCs are saved to the EEPROM
	CO_WR_RLC_LEGACY_MODE = 0x55,	// Activate the legacy RLC security mode allowing roll-over and using the RLC acceptance window for 24bit explicit RLC
	CO_WR_SECUREDEVICEV2_ADD = 0x56, // Add secure device to secure link table
	CO_RD_SECUREDEVICEV2_BY_INDEX = 0x57, // Read secure device from secure link table using the table index
	CO_WR_RSSITEST_MODE = 0x58,		// Control the state of the RSSI-Test mode
	CO_RD_RSSITEST_MODE = 0x59,		// Read the state of the RSSI-Test Mode
	CO_WR_SECUREDEVICE_MAINTENANCEKEY = 0x60, // Add the maintenance key information into the secure link table
	CO_RD_SECUREDEVICE_MAINTENANCEKEY = 0x61, // Read by index the maintenance key information from the secure link table
	CO_WR_TRANSPARENT_MODE = 0x62,	// Control the state of the transparent mode
	CO_RD_TRANSPARENT_MODE = 0x63,	// Read the state of the transparent mode
	CO_WR_TX_ONLY_MODE = 0x64,		// Control the state of the TX only mode
	CO_RD_TX_ONLY_MODE = 0x65		// Read the state of the TX only mode} COMMON_COMMAND;
};

// ESP3 Smart Ack codes
enum ESP3_SMART_ACK_CODE : uint8_t
{
	SA_WR_LEARNMODE = 0x01,			// Set/Reset Smart Ack learn mode
	SA_RD_LEARNMODE = 0x02,			// Get Smart Ack learn mode state
	SA_WR_LEARNCONFIRM = 0x03,		// Used for Smart Ack to add or delete a mailbox of a client
	SA_WR_CLIENTLEARNRQ = 0x04,		// Send Smart Ack Learn request (Client)
	SA_WR_RESET = 0x05,				// Send reset command to a Smart Ack client
	SA_RD_LEARNEDCLIENTS = 0x06,	// Get Smart Ack learned sensors / mailboxes
	SA_WR_RECLAIMS = 0x07,			// Set number of reclaim attempts
	SA_WR_POSTMASTER = 0x08			// Activate/Deactivate Post master functionality
};

// ESP3 Function return codes
enum ESP3_FUNC_RETURN_CODE : uint8_t
{
	RC_OK = 0,						// Action performed. No problem detected
	RC_EXIT,						// Action not performed. No problem detected
	RC_KO,							// Action not performed. Problem detected
	RC_TIME_OUT,					// Action couldn't be carried out within a certain time.
	RC_FLASH_HW_ERROR,				// The write/erase/verify process failed, the flash page seems to be corrupted
	RC_NEW_RX_BYTE,					// A new UART/SPI byte received
	RC_NO_RX_BYTE,					// No new UART/SPI byte received
	RC_NEW_RX_TEL,					// New telegram received
	RC_NO_RX_TEL,					// No new telegram received
	RC_NOT_VALID_CHKSUM,			// Checksum not valid
	RC_NOT_VALID_TEL,				// Telegram not valid
	RC_BUFF_FULL,					// Buffer full, no space in Tx or Rx buffer
	RC_ADDR_OUT_OF_MEM,				// Address is out of memory
	RC_NOT_VALID_PARAM,				// Invalid function parameter
	RC_BIST_FAILED,					// Built in self test failed
	RC_ST_TIMEOUT_BEFORE_SLEEP,		// Before entering power down, the short term timer had timed out.
	RC_MAX_FILTER_REACHED,			// Maximum number of filters reached, no more filter possible
	RC_FILTER_NOT_FOUND,			// Filter to delete not found
	RC_BASEID_OUT_OF_RANGE,			// BaseID out of range
	RC_BASEID_MAX_REACHED,			// BaseID was changed 10 times, no more changes are allowed
	RC_XTAL_NOT_STABLE,				// XTAL is not stable
	RC_NO_TX_TEL,					// No telegram for transmission in queue
	RC_ELEGRAM_WAIT,				// Waiting before sending broadcast message
	RC_OUT_OF_RANGE,				// Generic out of range return code
	RC_LOCK_SET,					// Function was not executed due to sending lock
	RC_NEW_TX_TEL					// New telegram transmitted
};

// Nb seconds between attempts to open ESP3 controller serial device
#define ESP3_CONTROLLER_RETRY_DELAY 30

// ESP3 packet sync byte
#define ESP3_SER_SYNC 0x55

// ESP3 packet header length
#define ESP3_HEADER_LENGTH 4

// ERP1 destID used for broadcast transmissions
#define ERP1_BROADCAST_TRANSMISSION 0xFFFFFFFF

// The following lines are taken from EO300I API header file

// Polynomial G(x) = x8 + x2 + x1 + x0 is used to generate the CRC8 table
const uint8_t crc8table[256] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
	0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
	0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
	0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
	0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
	0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
	0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
	0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
	0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
	0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
	0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
	0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
	0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
	0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
	0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
	0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
	0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
	0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
	0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
	0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
	0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
	0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
	0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
	0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
	0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
	0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
	0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
	0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
	0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

#define proc_crc8(crc, data) (crc8table[crc ^ data])

#define bitrange(data, shift, mask) ((data >> shift) & mask)

#define round(a) ((int) (a + 0.5))

// Nb consecutives teach request from a node to allow teach-in
#define TEACH_NB_REQUESTS 3
// Maximum delay to receive TEACH_NB_REQUESTS from a node to allow teach-in
#define TEACH_MAX_DELAY ((time_t) 2000)

// UTE Direction response codes
typedef enum
{
	UTE_UNIDIRECTIONAL = 0,	// Unidirectional
	UTE_BIDIRECTIONAL = 1	// Bidirectional
} UTE_DIRECTION;

// UTE Response codes
typedef enum
{
	GENERAL_REASON = 0,		// Request not accepted because of general reason
	TEACHIN_ACCEPTED = 1, 	// Request accepted, teach-in successful
	TEACHOUT_ACCEPTED = 2, 	// Request accepted, teach-out successful
	EEP_NOT_SUPPORTED = 3 	// Request not accepted, EEP not supported
} UTE_RESPONSE_CODE;

// UTE Direction response codes
typedef enum
{
	UTE_QUERY = 0,		// Teach-in query command
	UTE_RESPONSE = 1	// Teach-in response command
} UTE_CMD;

CEnOceanESP3::CEnOceanESP3(const int ID, const std::string &devname, const int type)
{
	m_HwdID = ID;
	m_szSerialPort = devname;
	m_Type = type;
	m_id_base = 0;
	m_id_chip = 0;
}

bool CEnOceanESP3::StartHardware()
{
	RequestStart();

	LoadNodesFromDatabase();

	// Will force reconnect first thing
	m_retrycntr = ESP3_CONTROLLER_RETRY_DELAY * 5;

	// Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CEnOceanESP3::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CEnOceanESP3::LoadNodesFromDatabase()
{
	m_nodes.clear();

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID, DeviceID, Manufacturer, Profile, Type FROM EnoceanSensors WHERE (HardwareID==%d)", m_HwdID);
	if (result.empty())
		return;

	for (const auto &sd : result)
	{
		NodeInfo node;

		node.idx = static_cast<uint32_t>(std::stoul(sd[0]));
		node.nodeID = static_cast<uint32_t>(std::stoul(sd[1], 0, 16));
		node.manufacturerID = static_cast<uint16_t>(std::stoul(sd[2]));
		node.RORG = 0x00;
		node.func = static_cast<uint8_t>(std::stoul(sd[3]));
		node.type = static_cast<uint8_t>(std::stoul(sd[4]));
		node.generic = true;
/*
		Debug(DEBUG_NORM, "LoadNodesFromDatabase: Idx %u Node %08X %sEEP %02X-%02X-%02X (%s) Manufacturer %03X (%s)",
			node.idx, node.nodeID,
			node.generic ? "Generic " : "", node.RORG, node.func, node.type, GetEEPLabel(node.RORG, node.func, node.type),
			node.manufacturerID, GetManufacturerName(node.manufacturerID));
*/
		m_nodes[node.nodeID] = node;
	}
}

CEnOceanESP3::NodeInfo* CEnOceanESP3::GetNodeInfo(const uint32_t nodeID)
{
	auto node = m_nodes.find(nodeID);

	if (node == m_nodes.end())
		return nullptr;

	return &(node->second);
}

void CEnOceanESP3::TeachInNode(const uint32_t nodeID, const uint16_t manID, const uint8_t RORG, const uint8_t func, const uint8_t type, const bool generic)
{
	Log(LOG_NORM, "Teach-in Node: HwdID %u Node %08X Manufacturer %03X (%s) %sEEP %02X-%02X-%02X (%s)",
		m_HwdID, nodeID, manID, GetManufacturerName(manID),
		generic ? "Generic " : "", RORG, func, type, GetEEPLabel(RORG, func, type));

	m_sql.safe_query("INSERT INTO EnoceanSensors (HardwareID, DeviceID, Manufacturer, Profile, Type) VALUES (%d,'%08X',%u,%u,%u)",
		m_HwdID, nodeID, manID, func, type);

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID FROM EnoceanSensors WHERE (HardwareID==%d) and (DeviceID=='%08X')", m_HwdID, nodeID);
	if (result.empty())
	{ // Should never happend: node was just created in the database!
		Log(LOG_ERROR, "Teach-in Node: Node %08X not created in database?!?!", nodeID);
		LoadNodesFromDatabase();
	}
	else
	{
		NodeInfo node;

		node.idx = static_cast<uint32_t>(std::stoul(result[0][0]));
		node.nodeID = nodeID;
		node.manufacturerID = manID;
		node.RORG = RORG;
		node.func = func;
		node.type = type;
		node.generic = generic;

		m_nodes[node.nodeID] = node;
	}
}

void CEnOceanESP3::CheckAndUpdateNodeRORG(NodeInfo* pNode, const uint8_t RORG)
{
	if (pNode == nullptr)
		return;

	if (pNode->func == 0x00 && pNode->type == 0x00)
		return;

	if (pNode->RORG == RORG)
		return;

	Log(LOG_NORM, "Node %08X, update EEP from %02X-%02X-%02X (%s) to %02X-%02X-%02X (%s)",
		pNode->nodeID,
		pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type),
		RORG, pNode->func, pNode->type, GetEEPLabel(RORG, pNode->func, pNode->type));

	pNode->RORG = RORG;
}

void CEnOceanESP3::Do_Work()
{
	uint32_t msec_counter = 0;
	uint32_t sec_counter = 0;

	Log(LOG_STATUS, "ESP3 worker started...");

	while (!IsStopRequested(200))
	{ // Loop each 200 ms, until task stop has been requested
		msec_counter++;
		if (msec_counter == 5)
		{ //  5 * 200 ms = 1 second ellapsed
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0) // Each 12 seconds, m_LastHeartbeat is updated
				m_LastHeartbeat = mytime(nullptr);
		}
		if (!isOpen())
		{ // ESP3 controller is not open
			if (m_retrycntr == 0)
				Log(LOG_STATUS, "Retrying to open in %d seconds...", ESP3_CONTROLLER_RETRY_DELAY);

			m_retrycntr++;
			if (m_retrycntr / 5 >= ESP3_CONTROLLER_RETRY_DELAY)
			{ // Open controller at first loop iteration, and then each ESP3_CONTROLLER_RETRY_DELAY seconds
				m_retrycntr = 0;
				OpenSerialDevice();
			}
			continue;
		}
		if (!m_sendqueue.empty())
		{ // Send first queued telegram
			std::vector<std::string>::iterator it = m_sendqueue.begin();
			std::string sBytes = *it;

			Debug(DEBUG_HARDWARE, "Send: %s", DumpESP3Packet(sBytes).c_str());

			// Write telegram to ESP3 hardware
			write(sBytes.c_str(), sBytes.size());

			std::lock_guard<std::mutex> l(m_sendMutex);
			m_sendqueue.erase(it);
		}
	}
	// Close ESP3 hardware
	terminate();
	Log(LOG_STATUS, "ESP3 worker stopped...");
}

#ifdef ENABLE_ESP3_TESTS
static const std::vector<uint8_t> ESP3TestsCases[] =
{	
#ifdef READCALLBACK_TESTS
// Junk data
	{ 0x00, 0x01 },

// Bad CRC8H packet + start of new packet
	{ ESP3_SER_SYNC, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, ESP3_SER_SYNC, 0x00 },
// Continued bad CRC8H packet + start of new packet
	{ 0x01, 0x02, 0x03, 0x04, ESP3_SER_SYNC, 0x00, 0x01, 0x02, 0x03, ESP3_SER_SYNC },
// Continued bad CRC8H packet + start of valid 1BS packet
	{ 0x00, 0x01, 0x02, ESP3_SER_SYNC, 0x00, 0x01, ESP3_SER_SYNC, 0x00, ESP3_SER_SYNC, 0x00, 0x07, 0x00 },
// Continued valid 1BS packet
	{ PACKET_RADIO_ERP1, 0x11, RORG_1BS, 0x08, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0x0D },

// Packet with valid header, but no data nor optdata
	{ ESP3_SER_SYNC, 0x00, 0x00, 0x00, PACKET_RADIO_ERP1, 0x07 },

// Packet with valid header, but overzised data + optdata length
	{ ESP3_SER_SYNC, 0xFF, 0xFF, 0xFF, PACKET_RADIO_ERP1, 0x07 },

// Bad CRC8D packets
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x00, PACKET_RADIO_ERP1, 0x11, RORG_1BS, 0x00, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0xFF },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x00, PACKET_RADIO_ERP1, 0x11, RORG_1BS, ESP3_SER_SYNC, 0x00, RORG_1BS, 0x00, 0x21, 0x00, 0xFF },
#endif // READCALLBACK_TESTS

#ifdef ESP3_TESTS_TEACHIN_NODE
// Test Case : 1BS Teach-in Test
//  1BS Unknown node Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x08, RORG_1BS, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE8 },
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x00, RORG_1BS, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x54 },
//  Unidirectional Teach-in Test, Node already known
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x00, RORG_1BS, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x54 },
//  4BS Unknown node Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0xFF, 0x08, RORG_4BS, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFC },
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x08, 0x00, 0x80, RORG_4BS, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB2 },
//  Unidirectional Teach-in Test, Node already known
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x08, 0x00, 0x80, RORG_4BS, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB2 },
// Test Case : Unknown node
//  RPS Unknown node Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, RORG_RPS, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x3E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, RORG_RPS, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x3E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, RORG_RPS, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x3E },
// Test Case : UTE/VLD Teach-in Test
//  1BS Unknown node Test
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x60, 0xE4, RORG_VLD, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  UTE Unidirectional Teach-in Test
//  Universal Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x40, 0x01, 0x46, 0x00, 0x0A, 0x03, 0xD2, RORG_VLD, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x56 },
//  UTE Unidirectional Teach-in Test, Node already known
    { ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x40, 0x01, 0x46, 0x00, 0x0A, 0x03, 0xD2, RORG_VLD, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x56 },
//  UTE Unidirectional Teach-out Test, Node already known
    { ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x50, 0x01, 0x46, 0x00, 0x0A, 0x03, 0xD2, RORG_VLD, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x15 },
//  UTE Unidirectional Teach-in or Teach-out Test, Node already known
    { ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x60, 0x01, 0x46, 0x00, 0x0A, 0x03, 0xD2, RORG_VLD, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD0 },
#endif // ESP3_TESTS_TEACHIN_NODE

#ifdef ESP3_TESTS_1BS_D5_00_01
// D5-00-01, Single Input Contact
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x00, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x83 },
// Test Case : Contact Tests
//  Open Contact Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x08, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3F },
//  Close Contact Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x09, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAB },
#endif // ESP3_TESTS_1BS_D5_00_01

#ifdef ESP3_TESTS_4BS_A5_02_XX
// A5-02-01, Temperature Sensor Range -40C to 0C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test, Variation 2
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1A },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC4 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF4 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7F, 0x08, 0x01, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1D },
// A5-02-02, Temperature Sensor Range -30C to +10C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x10, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x43 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4F },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x7F },
// A5-02-03, Temperature Sensor Range -20C to +20C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x18, 0x88, 0x80, 0x01, RORG_4BS, 0x02, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x6D },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x36 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x06 },
// A5-02-04, Temperature Sensor Range -10C to +30C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x20, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF1 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x6E },
// A5-02-05, Temperature Sensor Range 0C to +40C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x28, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3B },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x27 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x05, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x17 },
// A5-02-06, Temperature Sensor Range +10C to +50C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x30, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x62 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x06, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9C },
// A5-02-07, Temperature Sensor Range +20C to +60C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x38, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA8 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD5 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x07, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE5 },
// A5-02-08, Temperature Sensor Range +30C to +70C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x40, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x92 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x7C },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4C },
// A5-02-09, Temperature Sensor Range +40C to +80C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x48, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x09, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x58 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x09, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x05 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x09, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x35 },
// A5-02-0A, Temperature Sensor Range +50C to +90C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x50, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x8E },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xBE },
// A5-02-0B, Temperature Sensor Range +60C to +100C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x58, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCB },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF7 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC7 },
// A5-02-10, Temperature Sensor Range -60C to +20C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x80, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x54 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x08 },
// A5-02-11, Temperature Sensor Range -50C to +30C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x88, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x11, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9E },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x11, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x41 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x11, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x71 },
// A5-02-12, Temperature Sensor Range -40C to +40C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x90, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC7 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCA },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFA },
// A5-02-13, Temperature Sensor Range -30C to +50C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0x98, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x13, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x0D },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x13, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB3 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x13, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x83 },
// A5-02-14, Temperature Sensor Range -20C to +60C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xA0, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x14, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x75 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x14, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDB },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x14, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xEB },
// A5-02-15, Temperature Sensor Range -10C to +70C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xA8, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x15, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xBF },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x15, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA2 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x15, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x92 },
// A5-02-16, Temperature Sensor Range 0C to +80C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xB0, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x16, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE6 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x16, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x29 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x16, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x19 },
// A5-02-17, Temperature Sensor Range +10C to +90C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xB8, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x17, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x2C },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x17, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x50 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x17, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x60 },
// A5-02-18, Temperature Sensor Range +20C to +100C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xC0, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x18, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x16 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x18, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF9 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x18, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC9 },
// A5-02-19, Temperature Sensor Range +30C to +110C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xC8, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x19, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDC },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x19, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x80 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x19, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB0 },
// A5-02-1A, Temperature Sensor Range +40C to +120C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xD0, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x1A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x85 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x1A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x0B },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x1A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3B },
// A5-02-1B, Temperature Sensor Range +50C to +130C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x08, 0xD8, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x1B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4F },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x1B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x72 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x1B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x42 },
// A5-02-20, 10 Bit Temperature Sensor Range -10C to +41.2C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x09, 0x00, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDF },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x03, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x68 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x80 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x01, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x20, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x05 },
// A5-02-30, 10 Bit Temperature Sensor Range -40C to +62.3C
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x09, 0x80, 0x00, 0x80, 0x01, RORG_4BS, 0x02, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5B },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x03, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x02, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x05 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x01, 0xFF, 0x08, 0x01, RORG_4BS, 0x02, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x80 },
#endif // ESP3_TESTS_4BS_A5_02_XX

#ifdef ESP3_TESTS_4BS_A5_04_XX
// A5-04-01, TMP 0C to +40C, HUM 0% to 100%
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x10, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5D },
// Test Case : Temperature/Humidity Tests
//  Min Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
//  Max Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFA, 0xFA, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1B },
//  Mid Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7D, 0x7D, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x74 },
// Test Case : T-Sensor Enum Test
//  T-Sensor: not available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xBB, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4A },
// A5-04-02, TMP -20C to +60C, HUM 0% to 100%
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x10, 0x10, 0x00, 0x80, 0x01, RORG_4BS, 0x04, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x04 },
// Test Case : Temperature/Humidity Tests
//  Min Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x04, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x27 },
//  Max Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFA, 0xFA, 0x0A, 0x01, RORG_4BS, 0x04, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x90 },
//  Mid Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7D, 0x7D, 0x0A, 0x01, RORG_4BS, 0x04, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFF },
// Test Case : T-Sensor Enum Test
//  T-Sensor: not available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xBB, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC1 },
// A5-04-03, TMP -20C to +60C 10bit, HUM 0% to 100%
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x10, 0x18, 0x00, 0x80, 0x01, RORG_4BS, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCE },
// Test Case : Temperature/Humidity Tests
//  Min Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x71 },
//  Max Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0x03, 0xFF, 0x08, 0x01, RORG_4BS, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x60 },
//  Mid Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0x01, 0xFF, 0x08, 0x01, RORG_4BS, 0x04, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x0A },
// A5-04-04, TMP -40°C to +120°C 12bit, HUM 0% to 100%
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x10, 0x20, 0x00, 0x80, 0x01, RORG_4BS, 0x04, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB6 },
// Test Case : Temperature/Humidity Tests
//  Min Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x19 },
//  Max Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xC7, 0x06, 0x3F, 0x08, 0x01, RORG_4BS, 0x04, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x21 },
//  Mid Temperature/Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x64, 0x03, 0x20, 0x08, 0x01, RORG_4BS, 0x04, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFD },
#endif // ESP3_TESTS_4BS_A5_04_XX

#ifdef ESP3_TESTS_4BS_A5_06_01
// A5-06-01, Range 300lx to 60.000lx
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x18, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9D },
// Test Case : Supply voltage Tests
//  Min Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x53 },
//  Max Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAA },
//  Mid Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAD },
// Test Case : Illumination Tests ILL1
//  Min Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x53 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x63 },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7F, 0x08, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xBA },
// Test Case : Illumination Tests ILL2
//  Min Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC7 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0x00, 0x09, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x57 },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7F, 0x00, 0x09, 0x01, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x56 },
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test (ELTAKO)
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x18, 0x08 | (ELTAKO & 0x700), (ELTAKO & 0x0FF), 0x80, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3F },
// Test Case : Illumination Tests ILL1 (ELTAKO)
	// DATA_BYTE2 is Range select where 0 = ILL1 else = ILL2
	// DATA_BYTE3 is the low illuminance (ILL1) where min 0 = 0 lx, max 255 = 100 lx
	// DATA_BYTE2 is the illuminance (ILL2) where min 0 = 300 lx, max 255 = 30000 lx
//  Min Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB4 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0x00, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4D },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0x00, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4A },
// Test Case : Illumination Tests ILL2 (ELTAKO)
//  Min Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x01, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x24 },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7F, 0x00, 0x08, 0x02, RORG_4BS, 0x06, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x25 },
#endif // ESP3_TESTS_4BS_A5_06_01

#ifdef ESP3_TESTS_4BS_A5_07_01
// A5-07-01, Occupancy with optional Supply voltage monitor
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x1C, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFD },
// Test Case : Supply voltage availability Enum Test
//  Supply voltage availability: Supply voltage is not supported
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3B },
//  Supply voltage availability: Supply voltage is supported
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAF },
// Test Case : Supply voltage Tests
//  Min Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAF },
//  Max Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFA, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5C },
//  Mid Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7D, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x55 },
// Test Case : PIR Status Enum Test
//  PIR Status: PIR off
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7F, 0x08, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD2 },
//  PIR Status: PIR on
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x80, 0x08, 0x01, RORG_4BS, 0x07, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE2 },
#endif // ESP3_TESTS_4BS_A5_07_01

#ifdef ESP3_TESTS_4BS_A5_07_02
// A5-07-02, Occupancy sensor with Supply voltage monitor
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x1C, 0x10, 0x00, 0x80, 0x01, RORG_4BS, 0x07, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA4 },
// Test Case : Supply voltage Tests
//  Min Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB0 },
//  Max Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFA, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x43 },
//  Mid Supply voltage Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7D, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4A },
// Test Case : PIR Status Enum Test
//  PIR Status: Motion detected
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x88, 0x01, RORG_4BS, 0x07, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x41 },
#endif // ESP3_TESTS_4BS_A5_07_02

#ifdef ESP3_TESTS_4BS_A5_07_03
// A5-07-03, Occupancy with Supply voltage monitor and 10-bit illumination measurement
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x1C, 0x18, 0x00, 0x80, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x6E },
// Test Case : Supply voltage (REQUIRED) Tests
//  Min Supply voltage  Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC9 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFA, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x36 },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7D, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x35 },
// Test Case : Illumination Tests
//  Min Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC9 },
//  Max Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFD, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x34 },
//  Mid Illumination Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7D, 0x00, 0x08, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x35 },
// Test Case : PIR Status Enum Test
//  PIR Status: Motion detected
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x88, 0x01, RORG_4BS, 0x07, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
#endif // ESP3_TESTS_4BS_A5_07_03

#ifdef ESP3_TESTS_4BS_A5_09_04
// A5-09-04, CO2 Gas Sensor with Temp and Humidity
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x24, 0x20, 0x00, 0x80, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x58 },
// Test Case : Temperature/Humidity/CONC Tests
//  Min Temperature/Humidity/CONC Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0E, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xEE },
//  Max Temperature/Humidity/CONC Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xC8, 0xFF, 0xFF, 0x0E, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD9 },
//  Mid Temperature/Humidity/CONC Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x64, 0x7F, 0x7F, 0x0E, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
// Test Case : H-Sensor Enum Test
//  H-Sensor: Humidity Sensor not available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xC8, 0xFF, 0x00, 0x08, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x98 },
// Test Case : T-Sensor Enum Test
//  T-Sensor: Temperature Sensor not available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x09, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3F },
#endif // ESP3_TESTS_4BS_A5_09_04

#ifdef ESP3_TESTS_4BS_A5_10_0X
// A5-10-01, Temperature Sensor, Set Point, Fan Speed and Occupancy Control
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x40, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC8 },
// Test Case : Turn-switch for fan speed Enum Test
//  Turn-switch for fan speed: Stage Auto
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x4F },
//  Turn-switch for fan speed: Stage 0
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xD1, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x13 },
//  Turn-switch for fan speed: Stage 1
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xBD, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCB },
//  Turn-switch for fan speed: Stage 2
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xA4, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF9 },
//  Turn-switch for fan speed: Stage 3
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x90, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x91 },
// Test Case : Set point Tests
//  Min Set point Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB6 },
//  Max Set point Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x26 },
//  Mid Set point Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7F, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x27 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x86 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB6 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7F, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5F },
// Test Case : Occupancy button
//  Button released
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x22 },
//  Button pressed
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB6 },
// A5-10-0B, Temperature Sensor and Single Input Contact
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x40, 0x58, 0x00, 0x80, 0x01, RORG_4BS, 0x10, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x19 },
// Test Case : Contact State Enum Test
//  Contact State: closed
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x85 },
//  Contact State: open
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x10, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x11 },
// A5-10-0D, Temperature Sensor and Day/Night Control
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x40, 0x68, 0x00, 0x80, 0x01, RORG_4BS, 0x10, 0x0D, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAB },
// Test Case : Slide switch Enum Test
//  Slide switch: Position I / Night / Off
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x10, 0x0D, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Slide switch: Position O / Day / On
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x10, 0x0D, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x00 },
#endif // ESP3_TESTS_4BS_A5_10_0X

#ifdef ESP3_TESTS_4BS_A5_12_00
// A5-12-00, Counter
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x48, 0x00, 0x00, 0x80, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC2 },
// Test Case : Meter reading (MR) Tests
//  Min Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1F },
//  Max Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x46 },
//  Mid Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x41 },
// Test Case : Measurement channel (CH) Tests
//  Min Measurement channel Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1F },
//  Max Measurement channel Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0xF8, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Mid Measurement channel Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x78, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x65 },
// Test Case : Data type (DT) Enum Test
//  Data type: Cumulative value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1F },
//  Data type: Current value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0C, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x41 },
// Test Case : Divisor (DIV) Enum Test
//  Divisor: x/1
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1F },
//  Divisor: x/10
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x8B },
//  Divisor: x/100
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x30 },
//  Divisor: x/1000
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0B, 0x01, RORG_4BS, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA4 },
#endif // ESP3_TESTS_4BS_A5_12_00

#ifdef ESP3_TESTS_4BS_A5_12_01
// A5-12-01, Electricity
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x48, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x08 },
// Test Case : Meter reading Tests
//  Min Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x66 },
//  Max Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3F },
//  Mid Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
// Test Case : Tariff info Tests
//  Min Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x66 },
//  Max Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0xF8, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  Mid Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x78, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1C },
// Test Case : Data type (unit) Enum Test
//  Data type (unit): Cumulative value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x66 },
//  Data type (unit): Current value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0C, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
// Test Case : Divisor (scale) Enum Test
//  Divisor (scale): x/1
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x66 },
//  Divisor (scale): x/10
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF2 },
//  Divisor (scale): x/100
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x49 },
//  Divisor (scale): x/1000
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0B, 0x01, RORG_4BS, 0x12, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDD },
#endif // ESP3_TESTS_4BS_A5_12_01

#ifdef ESP3_TESTS_4BS_A5_12_02
// A5-12-02, Gas
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x48, 0x10, 0x00, 0x80, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x51 },
// Test Case : meter reading Tests
//  Min meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  Max meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB4 },
//  Mid meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB3 },
// Test Case : Tariff info Tests
//  Min Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  Max Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0xF8, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x66 },
//  Mid Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x78, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x97 },
// Test Case : data type (unit) Enum Test
//  data type (unit): Cumulative value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  data type (unit): Current value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0C, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB3 },
// Test Case : divisor (scale) Enum Test
//  divisor (scale): x/1
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xED },
//  divisor (scale): x/10
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x79 },
//  divisor (scale): x/100
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC2 },
//  divisor (scale): x/1000
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0B, 0x01, RORG_4BS, 0x12, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x56 },
#endif // ESP3_TESTS_4BS_A5_12_02

#ifdef ESP3_TESTS_4BS_A5_12_03
// A5-12-03, Water
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x48, 0x18, 0x00, 0x80, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9B },
// Test Case : Meter reading Tests
//  Min Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Max Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0xFF, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCD },
//  Mid Meter reading Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x7F, 0xFF, 0xFF, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCA },
// Test Case : Tariff info Tests
//  Min Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Max Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0xF8, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x1F },
//  Mid Tariff info Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x78, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xEE },
// Test Case : Data type (unit) Enum Test
//  Data type (unit): Cumulative value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Data type (unit): Current value
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0C, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCA },
// Test Case : Divisor (scale) Enum Test
//  Divisor (scale): x/1
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
//  Divisor (scale): x/10
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x09, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x00 },
//  Divisor (scale): x/100
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xBB },
//  Divisor (scale): x/1000
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0B, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x2F },
// Test Case : unusedFieldTest
//  unused field test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x12, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x94 },
#endif // ESP3_TESTS_4BS_A5_12_03

#ifdef ESP3_TESTS_RPS_F6_01_01
// F6-01-01, Switch Buttons, Push Button
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
// NB. following is wrongly interpreted as generic EEP F6-02-01 as it is created by default...
// Test Case : Button actions
// RPS N-msg: Button pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
// RPS N-msg: Button released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xE0 },
# endif // ESP3_TESTS_RPS_F6_01_01

#ifdef ESP3_TESTS_RPS_F6_02_0X
// F6-02-01, Light and Blind Control - Application Style 1
// F6-02-02, Light and Blind Control - Application Style 2
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
// Test Case : Rocker 1st action
//  Button AI
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x61 },
//  Button A0
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x30, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9F },
//  Button BI
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x50, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9A },
//  Button B0
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x70, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x64 },
// Test Case : Rocker 2nd action
//  Button A0 + AI 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x21, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x74 },
//  Button A0 + BI 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x25, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x2A },
//  Button B0 + BI 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x65, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD1 },
//  Button AI + B0 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x07, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFB },
// Test Case : Number of buttons pressed simultaneously
//  no button
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x56 },
//  3 or 4 buttons
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x70, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x53 },
// Test Case : Energy Bow Status
//  Released
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x29 },
#endif // ESP3_TESTS_RPS_F6_02_0X

#ifdef ESP3_TESTS_RPS_F6_05_00
// F6-05-00, Wind Speed Threshold Detector
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
// NB. following is wrongly interpreted as generic EEP F6-02-01 as it is created by default...
// Test Case : Status of detection and battery 
//  Wind speed exceeds threshold
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5E },
//  Wind speed below threshold
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x21 },
//  Energy LOW
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x30, 0x01, RORG_RPS, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA0 },
#endif // ESP3_TESTS_RPS_F6_05_00

#ifdef ESP3_TESTS_RPS_F6_05_01
// F6-05-01, Liquid Leakage Sensor (mechanic harvester)
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
// NB. following is wrongly interpreted as generic EEP F6-02-01 as it is created by default...
// Test Case : Water detection
//  Liquid leakage alarm: On
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x11, 0x01, RORG_RPS, 0x05, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xEA },
//  Liquid leakage alarm: Off
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x01 },
#endif // ESP3_TESTS_RPS_F6_05_01

#ifdef ESP3_TESTS_RPS_F6_05_02
// F6-05-02, Smoke Detector
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
// NB. following is wrongly interpreted as generic EEP F6-02-01 as it is created by default...
// Test Case : Status of detection and battery 
//  Smoke Alarm ON
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
//  Smoke Alarm OFF
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD3 },
//  Energy LOW
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x30, 0x01, RORG_RPS, 0x05, 0x02, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x52 },
#endif // ESP3_TESTS_RPS_F6_05_02

#ifdef ESP3_TESTS_VLD_D2_01_12
// D2-01-12, Slot-in module, dual channels, with external button control
// Test Case : Teach-in Test
//  Bi-directional teach-in or teach-out request from Node 00D20001, response expected
	{ ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0xA0, 0x02, 0x46, 0x00, 0x12, 0x01, RORG_VLD, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x2F },
// Test Case : Actuator Status Response + Buttons usage Test
// VLD msg: Actuator Status Response, Channel 0 status On
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x60, 0xE4, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x44 },
// VLD msg: Actuator Status Response, Channel 1 status Off
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x61, 0x80, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xAE },
// RPS msg: Button AO pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x30, 0x01, RORG_VLD, 0x01, 0x12, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xE8 },
// RPS msg: Energy bow released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_VLD, 0x01, 0x12, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x5E },
// VLD msg: Actuator Status Response, Channel 0 status On
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x60, 0xE4, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x10 },
// RPS msg: Button AI pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_VLD, 0x01, 0x12, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x16 },
// RPS msg: Energy bow released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_VLD, 0x01, 0x12, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x5E },
// VLD msg: Actuator Status Response, Channel 0 status Off
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x60, 0x80, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x4B },
// RPS msg: Button BO pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x70, 0x01, RORG_VLD, 0x01, 0x12, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x13 },
// RPS msg: Energy bow released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_VLD, 0x01, 0x12, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x5E },
// VLD msg: Actuator Status Response, Channel 1 status On
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x61, 0xE4, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xF5 },
// RPS msg: Button BI pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x50, 0x01, RORG_VLD, 0x01, 0x12, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xED },
// RPS msg: Energy bow released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_VLD, 0x01, 0x12, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x5E },
// VLD msg: Actuator Status Response, Channel 1 status Off
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x61, 0x80, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xAE },
#endif // ESP3_TESTS_VLD_D2_01_12

#ifdef ESP3_TESTS_VLD_D2_03_0A
// D2-03-0A, Push Button – Single Button
// Test Case : Teach-in Test
//  Universal Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x60, 0x01, 0x46, 0x00, 0x0A, 0x03, 0xD2, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x93 },
// Test Case : Battery Autonomy
//   Min Battery Autonomy
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x01, 0x00, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x73 },
//   Max Battery Autonomy
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x64, 0x00, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x10 },
//   Mid Battery Autonomy
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x32, 0x00, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD5 },
// Test Case : Button action
//   Simple Press
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x00, 0x01, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x02 },
//   Double press
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x00, 0x02, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB9 },
//   Long press
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x00, 0x03, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x2D },
//   Long press release
    { ESP3_SER_SYNC, 0x00, 0x08, 0x07, PACKET_RADIO_ERP1, 0x3D, RORG_VLD, 0x00, 0x04, 0x01, RORG_VLD, 0x03, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC8 },
#endif // ESP3_TESTS_VLD_D2_03_0A

#ifdef ESP3_TESTS_VLD_D2_14_30
// D2-14-30, Sensor for Smoke, Air quality, Hygrothermal comfort, Temperature and Humidity
// Universal Teach-in Test
	{ ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0x40, 0x01, 0x6D, 0x00, 0x30, 0x14, RORG_VLD, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x60 },
// VLD msg: Smoke alarm non activated
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Smoke alarm activated
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xE4 },
// VLD msg: Sensor fault mode non activated
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Sensor fault mode activated
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x39 },
// VLD msg: Smoke alarm maintenance OK
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Smoke alarm maintenance not done
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xD4 },
// VLD msg: Smoke alarm: Humidity range OK
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Smoke alarm: Humidity range not OK
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x21 },
// VLD msg: Smoke alarm: Temperature range OK
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Smoke alarm: Temperature range not OK
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xD8 },
// VLD msg: Smoke alarm: Less than one week since the last maintenace(MIN)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Smoke alarm: 250 weeks since the last maintenace(MAX)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x07, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x25 },
// VLD msg: Smoke alarm: 125 weeks since the last maintenace(MID)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x03, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xDA },
// VLD msg: Smoke alarm: Time error since the last maintenace(MAX)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x07, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x92 },
// VLD msg: Energy Storage Status: High
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Energy Storage Status: Medium
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x93 },
// VLD msg: Energy Storage Status: Low
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xB7 },
// VLD msg: Energy Storage Status: Critical
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xAB },
// VLD msg: Remaing life less than one mounth (MIN)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Remaing life 120 months (MAX)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x68 },
// VLD msg: Remaing life 60 months (MID)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x7F },
// VLD msg: Remaing life Error
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x01, 0xFE, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x7A },
// VLD msg: Temperature: 0C (MIN)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Temperature: 250C (MAX)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x01, 0xF4, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x76 },
// VLD msg: Temperature: 125C (MID)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x70 },
// VLD msg: Temperature: Error
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x01, 0xFE, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xA8 },
// VLD msg: Humidity: 0RH (MIN)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: Humidity: 100RH (MAX)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x01, 0x90, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x99 },
// VLD msg: Humidity: 50RH (MID)
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x84 },
// VLD msg: Humidity: Error
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x01, 0xFE, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xEF },
// VLD msg: HCI: Good
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: HCI: Medium
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x7E },
// VLD msg: HCI: Bad
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x6A },
// VLD msg: HCI: Error
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x9B },
// VLD msg: IAQTH: Optimal air range
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x8F },
// VLD msg: IAQTH: Dry Air range
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xF0 },
// VLD msg: IAQTH: High humidity range
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x71 },
// VLD msg: IAQTH: High temperature and humidity range
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x0E },
// VLD msg: IAQTH: Temperature or Humidity out of analysis range
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x74 },
// VLD msg: IAQTH: Error
	{ ESP3_SER_SYNC, 0x00, 0x0C, 0x07, PACKET_RADIO_ERP1, 0x96, RORG_VLD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x01, RORG_VLD, 0x14, 0x30, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0xF5 },
#endif // ESP3_TESTS_VLD_D2_14_30
};
#endif // ENABLE_ESP3_TESTS

bool CEnOceanESP3::OpenSerialDevice()
{
	// Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 57600); // ECP3 open with 57600
		Log(LOG_STATUS, "Using serial port %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		Log(LOG_ERROR, "Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch ( ... )
	{
		Log(LOG_ERROR, "Error opening serial port!");
		return false;
	}
	m_bIsStarted = true;

	m_RPS_teachin_nodeID = 0;

	m_receivestate = ERS_SYNCBYTE;
	setReadCallback([this](auto d, auto l) { ReadCallback(d, l); });

	sOnConnected(this);

#ifdef ENABLE_ESP3_TESTS
	Debug(DEBUG_NORM, "------------ ESP3 tests begin ---------------------------");
	m_sql.AllowNewHardwareTimer(1);

	for (const auto &itt : ESP3TestsCases)
		ReadCallback((const char *)itt.data(), itt.size());

	m_sql.AllowNewHardwareTimer(0);
	Debug(DEBUG_NORM, "------------ ESP3 tests end -----------------------------");
#endif

	uint8_t cmd;

	// Request BASE_ID
	m_id_base = 0;
	cmd = CO_RD_IDBASE;
	Debug(DEBUG_HARDWARE, "Request base ID");
	SendESP3PacketQueued(PACKET_COMMON_COMMAND, &cmd, 1, nullptr, 0);

	// Request base version
	m_id_chip = 0;
	cmd = CO_RD_VERSION;
	Debug(DEBUG_HARDWARE, "Request base version");
	SendESP3PacketQueued(PACKET_COMMON_COMMAND, &cmd, 1, nullptr, 0);

	return true;
}

std::string CEnOceanESP3::DumpESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	std::stringstream sstr;

	sstr << GetPacketTypeLabel(packettype);

	sstr << " DATA (" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t) datalen << ")";
	for (int i = 0; i < datalen; i++)
		if (i == 0 && packettype == PACKET_RADIO_ERP1)
			sstr << " " << GetRORGLabel((uint32_t) data[i]);
		else
			sstr << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t) data[i];

	if (optdatalen > 0)
	{
		sstr << " OPTDATA (" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t) optdatalen << ")";

		for (int i = 0; i < optdatalen; i++)
			sstr << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t) optdata[i];
	}
	return sstr.str();
}

std::string CEnOceanESP3::DumpESP3Packet(std::string esp3packet)
{
	uint8_t packettype = esp3packet[4];
	uint8_t *data = (uint8_t *) &esp3packet[ESP3_HEADER_LENGTH + 2];
	uint16_t datalen = esp3packet[1] << 8 | esp3packet[2];
	uint8_t *optdata = data + datalen;
	uint8_t optdatalen = esp3packet[3];

	return DumpESP3Packet(packettype, data, datalen, optdata, optdatalen);
}

std::string CEnOceanESP3::FormatESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	uint8_t buf[ESP3_PACKET_BUFFER_SIZE];
	uint32_t len = 0;

	uint8_t defaulERP1optdata[] =
	{	0x03, // SubTelNum : Send = 0x03
		0xFF, 0xFF, 0xFF, 0xFF, // Dest ID : Broadcast = 0xFFFFFFFF
		0xFF, // RSSI : Send = 0xFF
		0x00 // Seurity Level : Send = will be ignored
	};
	if (optdatalen == 0 && packettype == PACKET_RADIO_ERP1)
	{ // If not provided, add default optional data for PACKET_RADIO_ERP1
		optdata = defaulERP1optdata;
		optdatalen = 7;
	}
	buf[len++] = ESP3_SER_SYNC;
	buf[len++] = (uint8_t) bitrange(datalen, 8, 0x00FF);
	buf[len++] = (uint8_t) bitrange(datalen, 0, 0x00FF);
	buf[len++] = optdatalen;
	buf[len++] = packettype;

	uint8_t crc = 0;

	crc = proc_crc8(crc, buf[1]);
	crc = proc_crc8(crc, buf[2]);
	crc = proc_crc8(crc, buf[3]);
	crc = proc_crc8(crc, buf[4]);
	buf[len++] = crc;

	crc = 0;
	for (uint32_t i = 0; i < datalen; i++)
	{
		buf[len] = data[i];
		crc = proc_crc8(crc, buf[len]);
		len++;
	}
	for (uint32_t i = 0; i < optdatalen; i++)
	{
		buf[len] = optdata[i];
		crc = proc_crc8(crc, buf[len]);
		len++;
	}
	buf[len++] = crc;

	std::string sBytes;
	sBytes.assign((const char *)buf, len);
	return sBytes;
}

void CEnOceanESP3::SendESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	std::string sBytes = FormatESP3Packet(packettype, data, datalen, optdata, optdatalen);
	std::lock_guard<std::mutex> l(m_sendMutex);
	m_sendqueue.insert(m_sendqueue.begin(), sBytes);
}

void CEnOceanESP3::SendESP3PacketQueued(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	std::string sBytes = FormatESP3Packet(packettype, data, datalen, optdata, optdatalen);
	std::lock_guard<std::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}

bool CEnOceanESP3::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_id_base == 0 || m_id_chip == 0)
		return false;

	if (!isOpen())
		return false;

	RBUF *tsen = (RBUF *) pdata;

	uint32_t nodeID;

	if (tsen->RAW.packettype == pTypeLighting2)
		nodeID = GetNodeID(tsen->LIGHTING2.id1, tsen->LIGHTING2.id2, tsen->LIGHTING2.id3, tsen->LIGHTING2.id4);
	else
		return false; // Only allowed to control switches

	NodeInfo* pNode = GetNodeInfo(nodeID);

	if (pNode == nullptr)
	{ // Virtual switch created from m_id_base
		if (nodeID <= m_id_base || nodeID > (m_id_base + 128))
		{
			Log(LOG_ERROR, "Node %08X, invalid virtual switch", nodeID);
			return false;
		}
		if (tsen->LIGHTING2.unitcode >= 10)
		{
			Log(LOG_ERROR, "Node %08X, double press not supported", nodeID);
			return false;
		}

		uint8_t RockerID = tsen->LIGHTING2.unitcode - 1;
		uint8_t EB = 1;
		_eSwitchType switchtype = STYPE_OnOff;
		uint8_t LastLevel = 0;

		// Find out if this is a virtual switch or dimmer, because they are threaded differently
		// Virtual On/Off emulate RPS EEP: F6-02-01/02, Rocker switch, 2 Rocker
		// Virtual dimmers emulate 4BS EEP: A5-38-08, Central Command, Gateway

		std::string sDeviceID = GetDeviceID(nodeID);
		std::vector<std::vector<std::string>> result;

		result = m_sql.safe_query("SELECT SwitchType, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
			m_HwdID, sDeviceID.c_str(), (int) tsen->LIGHTING2.unitcode);
		if (!result.empty())
		{ // Device found in the database
			switchtype = static_cast<_eSwitchType>(std::stoul(result[0][0]));
			LastLevel = static_cast<uint32_t>(std::stoul(result[0][1]));
		}

		uint8_t iLevel = tsen->LIGHTING2.level;
		int cmnd = tsen->LIGHTING2.cmnd;
		int orgcmd = cmnd;
		if (tsen->LIGHTING2.level == 0 && switchtype != STYPE_Dimmer)
			cmnd = light2_sOff;
		else
		{
			if (cmnd == light2_sOn)
				iLevel = LastLevel;
			else
			{ // Scale to 0 - 100%
				iLevel = tsen->LIGHTING2.level;
				if (iLevel > 15)
					iLevel = 15;
				float fLevel = (100.0F / 15.0F) * float(iLevel);
				if (fLevel > 99.0F)
					fLevel = 100.0F;
				iLevel = (uint8_t) fLevel;
			}
			cmnd = light2_sSetLevel;
		}

		uint8_t buf[20];

		if (switchtype == STYPE_OnOff)
		{ // ESP3 virtual switch : F6-02-01/02, emulation
			// F6-02-01, Rocker switch, 2 Rocker (Light and blind control, Application style 1)
			// F6-02-02, Rocker switch, 2 Rocker (Light and blind control, Application style 2)

			uint8_t CO = (orgcmd != light2_sOff) && (orgcmd != light2_sGroupOff);

			buf[0] = RORG_RPS;

			switch (RockerID)
			{
				case 0: // Button A
					buf[1] = CO ? 0x00 : 0x20;
					break;

				case 1: // Button B
					buf[1] = CO ? 0x40 : 0x60;
					break;

				default:
					return false; // Not supported
			}
			buf[1] |= 0x10; // Press energy bow
			buf[2] = tsen->LIGHTING2.id1; // Sender ID
			buf[3] = tsen->LIGHTING2.id2;
			buf[4] = tsen->LIGHTING2.id3;
			buf[5] = tsen->LIGHTING2.id4;
			buf[6] = 0x30; // Press button

			Debug(DEBUG_NORM, "Node %08X, virtual switch, set to %s",
				nodeID, CO ? "On" : "Off");

			SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 7, nullptr, 0);

			// Button release is send a bit later

			buf[1] = 0x00; // No button press
			buf[6] = 0x20; // Release button

			SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 7, nullptr, 0);
			return true;
		}
		if (switchtype == STYPE_Dimmer)
		{ // ESP3 virtual dimmer: F6-02-01 emulation
			// F6-02-01, Rocker switch, 2 Rocker (Light and blind control, Application style 1)
			// F6-02-02, Rocker switch, 2 Rocker (Light and blind control, Application style 2)
			buf[0] = RORG_RPS;
			buf[1] = 0x02;
			buf[2] = 0x64; // Level : 100
			buf[3] = 0x01; // Speed : 1
			buf[4] = 0x09; // Dim Off
			buf[5] = tsen->LIGHTING2.id1; // Sender ID
			buf[6] = tsen->LIGHTING2.id2;
			buf[7] = tsen->LIGHTING2.id3;
			buf[8] = tsen->LIGHTING2.id4;
			buf[9] = 0x30; // Status

			if (cmnd != light2_sSetLevel)
			{ // On/Off
				uint8_t CO = (cmnd != light2_sOff) && (cmnd != light2_sGroupOff);

				buf[1] = (RockerID << 6) | (CO << 5) | (EB << 4);
				buf[9] = 0x30;

				Debug(DEBUG_NORM, "Node %08X, virtual dimmer, set to %s",
					nodeID, CO ? "On" : "Off");

				SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);

				// Button release is send a bit later

				buf[1] = 0x00;
				buf[9] = 0x20;

				SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
			}
			else
			{ // Dimmer value
				buf[1] = 0x02;
				buf[2] = iLevel;
				buf[3] = 0x01; // Very fast dimming
				if (iLevel == 0 || orgcmd == light2_sOff)
					buf[4] = 0x08; // Dim Off
				else
					buf[4] = 0x09; // Dim On

				Debug(DEBUG_NORM, "Node %08X, virtual dimmer, dimm %s, level %d%%",
					nodeID, (iLevel == 0 || orgcmd == light2_sOff) ? "Off" : "On", iLevel);

				SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
			}
			return true;
		}
		Log(LOG_ERROR, "Node %08X (virtual), switch type not supported (%d)", nodeID, switchtype);
		return false;
	}
	if ((pNode->RORG == RORG_VLD || pNode->RORG == 0x00)
		&& pNode->func == 0x01
		&& (pNode->type == 0x0D || pNode->type == 0x0E || pNode->type == 0x0F || pNode->type == 0x12))
	{ // D2-01-XX, Electronic Switches and Dimmers with Local Control
		// D2-01-0D, Micro smart plug, single channel, with external button control
		// D2-01-0E, Micro smart plug, single channel, with external button control
		// D2-01-0F, Slot-in module, single channel, with external button control
		// D2-01-12, Slot-in module, dual channels, with external button control

		CheckAndUpdateNodeRORG(pNode, RORG_VLD);

		if (tsen->LIGHTING2.unitcode > 0x1D)
		{
			Log(LOG_ERROR, "Node %08X, channel %d not supported", nodeID, (int) tsen->LIGHTING2.unitcode);
			return false;
		}

		uint8_t buf[9];
		uint8_t optbuf[7];

		buf[0] = RORG_VLD;
		buf[1] = 0x01; // CMD 0x1, Actuator set output
		buf[2] = tsen->LIGHTING2.unitcode - 1; // I/O Channel
		buf[3] = (tsen->LIGHTING2.cmnd == light2_sOn) ? 0x64 : 0x00; // Output Value
		buf[4] = bitrange(m_id_chip, 24, 0xFF); // Sender ID
		buf[5] = bitrange(m_id_chip, 16, 0xFF);
		buf[6] = bitrange(m_id_chip, 8, 0xFF);
		buf[7] = bitrange(m_id_chip, 0, 0xFF);
		buf[8] = 0x00; // Status

		optbuf[0] = 0x03; // SubTelNum : Send = 0x03
		optbuf[1] = tsen->LIGHTING2.id1; // Dest ID
		optbuf[2] = tsen->LIGHTING2.id2;
		optbuf[3] = tsen->LIGHTING2.id3;
		optbuf[4] = tsen->LIGHTING2.id4;
		optbuf[5] = 0xFF; // RSSI : Send = 0xFF
		optbuf[6] = 0x00; // Seurity Level : Send = ignored

		Debug(DEBUG_NORM, "Send %s switch command to Node %08X",
			(tsen->LIGHTING2.cmnd == light2_sOn) ? "On" : "Off", nodeID);

		SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 9, optbuf, 7);
		return true;
	}
	Log(LOG_ERROR, "Node %08X can not be used as a switch", nodeID);
	Log(LOG_ERROR, "Create a virtual switch associated with HwdID %u", m_HwdID);
	return false;
}

void CEnOceanESP3::ReadCallback(const char *data, size_t len)
{
	size_t nbyte = 0;
	uint8_t db;
	uint8_t *rbuf = nullptr;
	size_t rbuflen = 0;
	size_t rbufpos;

	while (nbyte < len || rbuf != nullptr)
	{
		if (rbuf == nullptr)
			db = data[nbyte++];
		else
		{
			db = rbuf[rbufpos++];
			if (rbufpos == rbuflen) {
				free(rbuf);
				rbuf = nullptr;
			}
		}
		switch (m_receivestate)
		{
			case ERS_SYNCBYTE: // Waiting for ESP3_SER_SYNC
				if (db != ESP3_SER_SYNC)
				{
					Log(LOG_ERROR, "Read: Skip unexpected byte (0x%02X)", db);
					continue;
				}
				// Serial synchronization ESP3_SER_SYNC received
				m_bufferpos = 0;
				m_wantedlen = ESP3_HEADER_LENGTH;
				m_crc = 0;
				m_receivestate = ERS_HEADER;
				continue;

			case ERS_HEADER: // Waiting for 4 byte header
				m_buffer[m_bufferpos++] = db;
				m_crc = proc_crc8(m_crc, db);
				if (m_bufferpos < m_wantedlen)
					continue;

				// Header received

				m_datalen = (m_buffer[0] << 8) | m_buffer[1];
				m_optionallen = m_buffer[2];
				m_packettype = m_buffer[3];

				if ((m_datalen + m_optionallen) == 0)
				{
					Log(LOG_ERROR, "Read: Invalid packet size (no data)");
					break;
				}
				if ((m_datalen + m_optionallen + 7) >= ESP3_PACKET_BUFFER_SIZE)
				{
					Log(LOG_ERROR, "Read: Invalid packet size (oversized)");
					break;
				}
				m_receivestate = ERS_CRC8H;
				continue;

			case ERS_CRC8H: // Waiting for header CRC
				m_buffer[m_bufferpos++] = db;
				if (db != m_crc)
				{
					Log(LOG_ERROR, "Read: CRC8H error (expected 0x%02X got 0x%02X)", m_crc, db);
					break;
				}
				m_crc = 0;
				m_wantedlen += m_datalen + m_optionallen + 1;
				m_receivestate = ERS_DATA;
				continue;

			case ERS_DATA: // Waiting for data CRC
				m_buffer[m_bufferpos++] = db;
				m_crc = proc_crc8(m_crc, db);
				if (m_bufferpos < m_wantedlen)
					continue;

				// Data + Optional data received

				m_receivestate = ERS_CRC8D;
				continue;

			case ERS_CRC8D:
				m_buffer[m_bufferpos++] = db;
				if (db != m_crc)
				{
					Log(LOG_ERROR, "Read: CRC8D error (expected 0x%02X got 0x%02X)", m_crc, db);
					break;
				}
				// Parse ESP3 packet : type + data + optional data
				uint8_t *data = m_buffer + ESP3_HEADER_LENGTH + 1;
				uint8_t *optdata = data + m_datalen;
				ParseESP3Packet(m_packettype, data, m_datalen, optdata, m_optionallen);

				m_receivestate = ERS_SYNCBYTE;
				continue;
		}
		// Rolling back (m_bufferpos) bytes
		Log(LOG_ERROR, "Read: Rolling back %d bytes", m_bufferpos);
		if (rbuf != nullptr)
			rbufpos -= m_bufferpos;
		else
		{
			rbuflen = m_bufferpos;
			rbuf = (uint8_t *) calloc(rbuflen, sizeof(uint8_t));
			memcpy(rbuf, m_buffer, rbuflen);
			rbufpos = 0;
		}
		m_receivestate = ERS_SYNCBYTE;
	}
}

void CEnOceanESP3::ParseESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	Debug(DEBUG_HARDWARE, "Read: %s", DumpESP3Packet(packettype, data, datalen, optdata, optdatalen).c_str());

	switch (packettype)
	{
		case PACKET_RESPONSE: // Response
		{
			uint8_t return_code = data[0];

			if (return_code != RET_OK)
			{
				Log(LOG_ERROR, "HwdID %d, received error response %s", m_HwdID, GetReturnCodeLabel(return_code));
				return;
			}
			// Response OK

			if (m_id_base == 0 && datalen == 5)
			{ // Base ID Information
				m_id_base = GetNodeID(data[1], data[2], data[3], data[4]);
				Log(LOG_STATUS, "HwdID %d ID_Base %08X", m_HwdID, m_id_base);
				return;
			}
			if (m_id_chip == 0 && datalen == 33)
			{ // Base version Information
				m_id_chip = GetNodeID(data[9], data[10], data[11], data[12]);
				Log(LOG_STATUS, "HwdID %d ChipID %08X ChipVersion %02X.%02X.%02X.%02X App %02X.%02X.%02X.%02X API %02X.%02X.%02X.%02X Description '%s'",
					 m_HwdID, m_id_chip, data[13], data[14], data[15], data[16], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], (const char *)data + 17);
				return;
			}
			Debug(DEBUG_NORM, "HwdID %d, received response (%s)", m_HwdID, GetReturnCodeLabel(return_code));
		}
		return;

		case PACKET_RADIO_ERP1:
			ParseERP1Packet(data, datalen, optdata, optdatalen);
			return;

		default:
			Log(LOG_ERROR, "HwdID %d, ESP3 Packet Type not supported (%s)", m_HwdID, GetPacketTypeLabel(packettype));
	}
}

void CEnOceanESP3::ParseERP1Packet(uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	// Parse optional data

	uint8_t rssi = 12; // RSSI for Domoticz, normal value is between 0 (very weak) and 11 (strong), 12 = no RSSI value

	if (optdatalen > 0)
	{
		if (optdatalen != 7)
			Log(LOG_ERROR, "Invalid ERP1 optional data size (%d)", optdatalen);
		else
		{
			// RSSI reported by Enocean Dongle in dBm

			uint32_t dstID = GetNodeID(optdata[1], optdata[2], optdata[3], optdata[4]);

			// Ignore telegrams addressed to another device
			if (dstID != ERP1_BROADCAST_TRANSMISSION && dstID != m_id_base && dstID != m_id_chip)
			{
				Debug(DEBUG_HARDWARE, "HwdID %d, ignore addressed telegram sent to %08X", m_HwdID, dstID);
				return;
			}

			int dBm = optdata[5] * -1;

			// Convert RSSI dBm to RSSI Domoticz
			// This is not the best conversion algo, but, according to my tests, it's a good start

			if (dBm > -50)
				rssi = 11;
			else if (dBm < -100)
				rssi = 0;
			else
				rssi = static_cast<uint8_t>((dBm + 100) / 5);

			if (dstID == ERP1_BROADCAST_TRANSMISSION)
				Debug(DEBUG_HARDWARE, "Broadcast RSSI %idBm (%u/11)", dBm, rssi);
			else
				Debug(DEBUG_HARDWARE, "Dst %08X RSSI %idBm (%u/11)", dstID, dBm, rssi);
		}
	}
	// Parse data

	uint8_t RORG = data[0];

	uint8_t ID_BYTE3 = data[datalen - 5];
	uint8_t ID_BYTE2 = data[datalen - 4];
	uint8_t ID_BYTE1 = data[datalen - 3];
	uint8_t ID_BYTE0 = data[datalen - 2];
	uint32_t senderID = GetNodeID(ID_BYTE3, ID_BYTE2, ID_BYTE1, ID_BYTE0);
	NodeInfo* pNode = GetNodeInfo(senderID);

	uint8_t STATUS = data[datalen - 1];

	switch (RORG)
	{
		case RORG_1BS:
			{ // 1BS telegram, D5-XX-XX, 1 Byte Communication
				uint8_t DATA = data[1];
				uint8_t LRN = bitrange(DATA, 3, 0x01);

				if (LRN == 0)
				{ 	// 1BS teach-in
					Log(LOG_NORM, "1BS teach-in request from Node %08X", senderID);

					if (pNode != nullptr)
					{
						Log(LOG_NORM, "Node %08X already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
							senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{
						Log(LOG_NORM, "Unknown Node %08X, please allow accepting new hardware and proceed to teach-in", senderID);
						return;
					}
					// Node not found and learn mode enabled : add it to the database
					// Generic node : Unknown manufacturer, Switch Buttons, Push Button

					uint8_t node_func = 0x00;
					uint8_t node_type = 0x01;
					
					Log(LOG_NORM, "Creating Node %08X with generic EEP %02X-%02X-%02X (%s)",
						senderID, RORG_1BS, node_func, node_type, GetEEPLabel(RORG_1BS, node_func, node_type));
					Log(LOG_NORM, "Please adjust by hand if need be");

					TeachInNode(senderID, UNKNOWN_MANUFACTURER, RORG_1BS, node_func, node_type, true);
					return;
				}
				// 1BS data

				if (pNode == nullptr)
				{
					Log(LOG_NORM, "1BS msg: Unknown Node %08X, please proceed to teach-in", senderID);
					return;
				}
				CheckAndUpdateNodeRORG(pNode, RORG_1BS);

				Debug(DEBUG_NORM, "1BS msg: Node %08X EEP %02X-%02X-%02X (%s) Data %02X",
					senderID,
					pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), DATA);

				if (pNode->func == 0x00 && pNode->type == 0x01)
				{ // D5-00-01, Contacts and Switches, Single Input Contact
					uint8_t CO = bitrange(DATA, 0, 0x01);

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
					tsen.LIGHTING2.packettype = pTypeLighting2;
					tsen.LIGHTING2.subtype = sTypeAC;
					tsen.LIGHTING2.seqnbr = 0;
					tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
					tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
					tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
					tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
					tsen.LIGHTING2.level = 0;
					tsen.LIGHTING2.unitcode = 1;
					tsen.LIGHTING2.cmnd = CO ? light2_sOff : light2_sOn;
					tsen.LIGHTING2.rssi = rssi;

					Debug(DEBUG_NORM, "1BS msg: Node %08X Contact %s", senderID, CO ? "Off" : "On");

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				// Should never happend, since D5-00-01 is the only 1BS EEP
				Log(LOG_ERROR, "1BS msg: Node %08X EEP %02X-%02X-%02X (%s) not supported",
					senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_4BS:
			{ // 4BS telegram, A5-XX-XX, 4 bytes communication
				uint8_t DATA_BYTE3 = data[1];
				uint8_t DATA_BYTE2 = data[2];
				uint8_t DATA_BYTE1 = data[3];
				uint8_t DATA_BYTE0 = data[4];

				uint8_t LRNB = bitrange(DATA_BYTE0, 3, 0x01);
				if (LRNB == 0)
				{ // 4BS teach-in
					uint8_t LRN_TYPE = bitrange(DATA_BYTE0, 7, 0x01);

					Log(LOG_NORM, "4BS teach-in request from Node %08X (variation %s EEP)",
						senderID, (LRN_TYPE == 0) ? "1 : without" : "2 : with");

					if (pNode != nullptr)
					{
						Log(LOG_NORM, "Node %08X already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
							senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{
						Log(LOG_NORM, "Unknown Node %08X, please allow accepting new hardware and proceed to teach-in", senderID);
						return;
					}
					// Node not found and learn mode enabled : add it to the database

					uint16_t node_manID;
					uint8_t node_func;
					uint8_t node_type;

					if (LRN_TYPE == 0)
					{ // 4BS teach-in, variation 1 : without EEP
						// EnOcean EEP 2.6.8 specification §3.3 p.321 : an EEP must be manually allocated

						node_manID = UNKNOWN_MANUFACTURER;
						node_func = 0x02; // Generic Temperature Sensors, Temperature Sensor ranging from 0°C to +40°C
						node_type = 0x05;

						Log(LOG_NORM, "Creating Node %08X with generic EEP %02X-%02X-%02X (%s)",
							senderID, RORG_4BS, node_func, node_type, GetEEPLabel(RORG_4BS, node_func, node_type));
						Log(LOG_NORM, "Please adjust by hand if need be");
					}
					else
					{ // 4BS teach-in variation 2, with Manufacturer ID and EEP
						node_manID = (bitrange(DATA_BYTE2, 0, 0x07) << 8) | DATA_BYTE1;
						node_func = bitrange(DATA_BYTE3, 2, 0x3F);
						node_type = (bitrange(DATA_BYTE3, 0, 0x03) << 5) | bitrange(DATA_BYTE2, 3, 0x1F);

						Log(LOG_NORM, "Creating Node %08X Manufacturer %03X (%s) EEP %02X-%02X-%02X (%s)",
							senderID, node_manID, GetManufacturerName(node_manID),
							RORG_4BS, node_func, node_type, GetEEPLabel(RORG_4BS, node_func, node_type));
					}

					TeachInNode(senderID, node_manID, RORG_4BS, node_func, node_type, (LRN_TYPE == 0));

					// 4BS EEP requiring teach-in variation 3 response
					// A5-20-XX, HVAC Components
					// A5-38-08, Central Command Gateway
					// A5-3F-00, Radio Link Test
					if (node_func == 0x20
						|| (node_func == 0x38 && node_type == 0x08)
						|| (node_func == 0x3F && node_type == 0x00))
					{
						Log(LOG_NORM, "4BS teach-in request from Node %08X (variation 3 : bi-directional)", senderID);

						uint8_t buf[10];

						buf[0] = RORG_4BS;
						buf[1] = DATA_BYTE3; // Func, Type and Manufacturer ID
						buf[2] = DATA_BYTE2;
						buf[3] = DATA_BYTE1;
						buf[1] = 0xF0; // Successful teach-in
						buf[5] = bitrange(m_id_chip, 24, 0xFF); // Sender ID
						buf[6] = bitrange(m_id_chip, 16, 0xFF);
						buf[7] = bitrange(m_id_chip, 8, 0xFF);
						buf[8] = bitrange(m_id_chip, 0, 0xFF);
						buf[9] = 0x00; // Status

						Debug(DEBUG_NORM, "Send 4BS teach-in accepted response");

						SendESP3Packet(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
					}
					return;
				}
				// 4BS data

				if (pNode == nullptr)
				{
					Log(LOG_NORM, "4BS msg: Unknown Node %08X, please proceed to teach-in", senderID);
					return;
				}
				CheckAndUpdateNodeRORG(pNode, RORG_4BS);

				Debug(DEBUG_NORM, "4BS msg: Node %08X EEP %02X-%02X-%02X (%s) Data %02X %02X %02X %02X",
					senderID,
					pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 
					DATA_BYTE3, DATA_BYTE2, DATA_BYTE1, DATA_BYTE0);

				if (pNode->func == 0x02)
				{ // A5-02-01..30, Temperature sensor
					float TMP = -275.0F; // Initialize to an arbitrary out of range value
					if (pNode->type == 0x01)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -40.0F, 0.0F);
					else if (pNode->type == 0x02)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -30.0F, 10.0F);
					else if (pNode->type == 0x03)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -20.0F, 20.0F);
					else if (pNode->type == 0x04)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -10.0F, 30.0F);
					else if (pNode->type == 0x05)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 0.0F, 40.0F);
					else if (pNode->type == 0x06)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 10.0F, 50.0F);
					else if (pNode->type == 0x07)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 20.0F, 60.0F);
					else if (pNode->type == 0x08)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 30.0F, 70.0F);
					else if (pNode->type == 0x09)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 40.0F, 80.0F);
					else if (pNode->type == 0x0A)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 50.0F, 90.0F);
					else if (pNode->type == 0x0B)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 60.0F, 100.0F);
					else if (pNode->type == 0x10)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -60.0F, 20.0F);
					else if (pNode->type == 0x11)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -50.0F, 30.0F);
					else if (pNode->type == 0x12)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -40.0F, 40.0F);
					else if (pNode->type == 0x13)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -30.0F, 50.0F);
					else if (pNode->type == 0x14)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -20.0F, 60.0F);
					else if (pNode->type == 0x15)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, -10.0F, 70.0F);
					else if (pNode->type == 0x16)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 0.0F, 80.0F);
					else if (pNode->type == 0x17)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 10.0F, 90.0F);
					else if (pNode->type == 0x18)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 20.0F, 100.0F);
					else if (pNode->type == 0x19)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 30.0F, 110.0F);
					else if (pNode->type == 0x1A)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 40.0F, 120.0F);
					else if (pNode->type == 0x1B)
						TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 50.0F, 130.0F);
					else if (pNode->type == 0x20)
						TMP = GetDeviceValue(((DATA_BYTE2 & 0x03) << 8) | DATA_BYTE1, 1023, 0, -10.0F, 41.2F); // 10bit
					else if (pNode->type == 0x30)
						TMP = GetDeviceValue(((DATA_BYTE2 & 0x03) << 8) | DATA_BYTE1, 1023, 0, -40.0F, 62.3F); // 10bit

					if (TMP > -274.0F)
					{ // TMP value has been changed => EEP is managed => update TMP
						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
						tsen.TEMP.packettype = pTypeTEMP;
						tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
						tsen.TEMP.seqnbr = 0;
						tsen.TEMP.id1 = ID_BYTE2;
						tsen.TEMP.id2 = ID_BYTE1;
						// WARNING
						// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
						// decode_Temp assumes battery_level = 255 (Unknown) & rssi = 12 (Not available)
						tsen.TEMP.battery_level = ID_BYTE0 & 0x0F;
						tsen.TEMP.rssi = (ID_BYTE0 & 0xF0) >> 4;
						tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
						int at10 = round(std::abs(TMP * 10.0F));
						tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
						at10 -= (tsen.TEMP.temperatureh * 256);
						tsen.TEMP.temperaturel = (BYTE) at10;

						Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", senderID, TMP);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
						return;
					}
				}
				if (pNode->func == 0x04)
				{ // A5-04-01..04, Temperature and Humidity Sensor
					float HUM = -2.0F;   // Initialize to an arbitrary out of range value
					float TMP = -275.0F; // Initialize to an arbitrary out of range value
					if (pNode->type == 0x01)
					{
						HUM = GetDeviceValue(DATA_BYTE2, 0, 250, 0.0F, 100.0F);

						uint8_t TSN = (DATA_BYTE0 & 0x02) >> 1;
						if (TSN == 1) // Temperature sensor available
							TMP = GetDeviceValue(DATA_BYTE1, 0, 250, 0.0F, 40.0F);
					}
					else if (pNode->type == 0x02)
					{
						HUM = GetDeviceValue(DATA_BYTE2, 0, 250, 0.0F, 100.0F);

						uint8_t TSN = (DATA_BYTE0 & 0x02) >> 1;
						if (TSN == 1) // Temperature sensor available
							TMP = GetDeviceValue(DATA_BYTE1, 0, 250, -20.0F, 60.0F);
					}
					else if (pNode->type == 0x03)
					{
						HUM = GetDeviceValue(DATA_BYTE3, 0, 255, 0.0F, 100.0F);
						TMP = GetDeviceValue(bitrange(DATA_BYTE2, 0, 0x03) << 8 | DATA_BYTE1, 0, 1023, -20.0F, 60.0F); // 10bit
						// uint8_t TTP = DATA_BYTE0 & 0x01;
					}
					else if (pNode->type == 0x04)
					{
						HUM = GetDeviceValue(DATA_BYTE3, 0, 199, 0.0F, 100.0F);
						TMP = GetDeviceValue(bitrange(DATA_BYTE2, 0, 0x0F) << 8 | DATA_BYTE1, 0, 1599, -40.0F, +120.0F); // 12bit
					}
					if (TMP > -274.0F && HUM  > -1.0F)
					{ // TMP + HUM values have been changed => EEP is managed => update TEMP_HUM
						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
						tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
						tsen.TEMP_HUM.subtype = sTypeTH5; // WTGR800
						tsen.TEMP_HUM.seqnbr = 0;
						tsen.TEMP_HUM.id1 = ID_BYTE2;
						tsen.TEMP_HUM.id2 = ID_BYTE1;
						tsen.TEMP_HUM.tempsign = (TMP >= 0) ? 0 : 1;
						int at10 = round(std::abs(TMP * 10.0F));
						tsen.TEMP_HUM.temperatureh = (BYTE) (at10 / 256);
						at10 -= (tsen.TEMP_HUM.temperatureh * 256);
						tsen.TEMP_HUM.temperaturel = (BYTE) at10;
						tsen.TEMP_HUM.humidity = (BYTE) round(HUM);
						tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);
						tsen.TEMP_HUM.battery_level = 9; // OK, TODO: Should be 255 (unknown battery level) ?
						tsen.TEMP_HUM.rssi = rssi;

						Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C HUM %d%%", senderID, TMP, tsen.TEMP_HUM.humidity);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP_HUM, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
						return;
					}
					if (HUM > -1.0F)
					{ // HUM value has been changed => EEP is managed => update HUM (TMP unavailable)
						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
						tsen.HUM.packettype = pTypeHUM;
						tsen.HUM.subtype = sTypeHUM1; // LaCrosse TX3
						tsen.HUM.seqnbr = 0;
						tsen.HUM.id1 = ID_BYTE2;
						tsen.HUM.id2 = ID_BYTE1;
						tsen.HUM.humidity = (BYTE) round(HUM);
						tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
						tsen.HUM.battery_level = 9; // OK, TODO: Should be 255 (unknown battery level) ?
						tsen.HUM.rssi = rssi;

						Debug(DEBUG_NORM, "4BS msg: Node %08X HUM %d%%", senderID, tsen.HUM.humidity);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.HUM, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
						return;
					}
				}
				if (pNode->func == 0x06 && pNode->type == 0x01)
				{ // A5-06-01, Light Sensor
					uint8_t RS;
					float ILL = 0.0F;

					if (pNode->manufacturerID != ELTAKO)
					{ // General case for A5-06-01
						// DATA_BYTE3 is the voltage where 0x00 = 0 V ... 0xFF = 5.1 V

						float SVC = GetDeviceValue(DATA_BYTE3, 0, 255, 0.0F, 5100.0F); // Convert V to mV

						// DATA_BYTE0_bit_0 is Range select where 0 = ILL1, 1 = ILL2
						// DATA_BYTE1 is the illuminance (ILL1) where min 0 = 600 lx, max 255 = 60000 lx
						// DATA_BYTE2 is the illuminance (ILL2) where min 0 = 300 lx, max 255 = 30000 lx

						RS = bitrange(DATA_BYTE0, 0, 0x01);
						if (RS == 0)
							ILL = GetDeviceValue(DATA_BYTE1, 0, 255, 600.0F, 60000.0F);
						else
							ILL = GetDeviceValue(DATA_BYTE2, 0, 255, 300.0F, 30000.0F);

						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
						tsen.RFXSENSOR.packettype = pTypeRFXSensor;
						tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
						tsen.RFXSENSOR.seqnbr = 0;
						tsen.RFXSENSOR.id = ID_BYTE1;
						// WARNING
						// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
						// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
						tsen.RFXSENSOR.filler = bitrange(ID_BYTE0, 0, 0x0F);
						tsen.RFXSENSOR.rssi = bitrange(ID_BYTE0, 4, 0x0F);
						tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
						tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

						Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", senderID, SVC);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					}
					else
					{ // WARNING : ELTAKO specific implementation
						// Eltako FAH60, FAH60B, FAH65S, FIH65S, FAH63, FIH63
						// DATA_BYTE2 is Range select where 0 = ILL1  else = ILL2
						// DATA_BYTE3 is the low illuminance (ILL1) where min 0 = 0 lx, max 255 = 100 lx
						// DATA_BYTE2 is the illuminance (ILL2) where min 0 = 300 lx, max 255 = 30000 lx

						RS = DATA_BYTE2;
						if (RS == 0)
							ILL = GetDeviceValue(DATA_BYTE3, 0, 255, 0.0F, 100.0F);
						else
							ILL = GetDeviceValue(DATA_BYTE2, 0, 255, 300.0F, 30000.0F);
					}
					_tLightMeter lmeter;
					lmeter.id1 = (BYTE) ID_BYTE3; // Sender ID
					lmeter.id2 = (BYTE) ID_BYTE2;
					lmeter.id3 = (BYTE) ID_BYTE1;
					lmeter.id4 = (BYTE) ID_BYTE0;
					lmeter.dunit = 1;
					lmeter.fLux = ILL;

					Debug(DEBUG_NORM, "4BS msg: Node %08X RS %s ILL %.1Flx", senderID, (RS == 0) ? "ILL1" : "ILL2", ILL);

					sDecodeRXMessage(this, (const unsigned char *) &lmeter, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x07 && pNode->type == 0x01)
				{ // A5-07-01, Occupancy sensor with optional Supply voltage monitor
					RBUF tsen;

					uint8_t SVA = bitrange(DATA_BYTE0, 0, 0x01);
					if (SVA == 1)
					{ // Supply voltage supported
						float SVC = GetDeviceValue(DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

						memset(&tsen, 0, sizeof(RBUF));
						tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
						tsen.RFXSENSOR.packettype = pTypeRFXSensor;
						tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
						tsen.RFXSENSOR.seqnbr = 0;
						tsen.RFXSENSOR.id = ID_BYTE1;
						// WARNING
						// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
						// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
						tsen.RFXSENSOR.filler = bitrange(ID_BYTE0, 0, 0x0F);
						tsen.RFXSENSOR.rssi = bitrange(ID_BYTE0, 4, 0x0F);
						tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
						tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

						Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", senderID, SVC);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					}
					uint8_t PIRS = DATA_BYTE1;

					memset(&tsen, 0, sizeof(RBUF));
					tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
					tsen.LIGHTING2.packettype = pTypeLighting2;
					tsen.LIGHTING2.subtype = sTypeAC;
					tsen.LIGHTING2.seqnbr = 0;
					tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
					tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
					tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
					tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
					tsen.LIGHTING2.level = 0;
					tsen.LIGHTING2.unitcode = 1;
					tsen.LIGHTING2.cmnd = (PIRS >= 128) ? light2_sOn : light2_sOff;
					tsen.LIGHTING2.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)", senderID, PIRS, (PIRS >= 128) ? "On" : "Off");

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x07 && pNode->type == 0x02)
				{ // A5-07-02, Occupancy sensor with Supply voltage monitor
					RBUF tsen;

					float SVC = GetDeviceValue(DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
					tsen.RFXSENSOR.packettype = pTypeRFXSensor;
					tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
					tsen.RFXSENSOR.seqnbr = 0;
					tsen.RFXSENSOR.id = ID_BYTE1;
					// WARNING
					// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
					// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
					tsen.RFXSENSOR.filler = bitrange(ID_BYTE0, 0, 0x0F);
					tsen.RFXSENSOR.rssi = bitrange(ID_BYTE0, 4, 0x0F);
					tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
					tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

					Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", senderID, SVC);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());

					uint8_t PIRS = bitrange(DATA_BYTE0, 7, 0x01);

					memset(&tsen, 0, sizeof(RBUF));
					tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
					tsen.LIGHTING2.packettype = pTypeLighting2;
					tsen.LIGHTING2.subtype = sTypeAC;
					tsen.LIGHTING2.seqnbr = 0;
					tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
					tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
					tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
					tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
					tsen.LIGHTING2.level = 0;
					tsen.LIGHTING2.unitcode = 1;
					tsen.LIGHTING2.cmnd = (PIRS == 1) ? light2_sOn : light2_sOff;
					tsen.LIGHTING2.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)",
						senderID, PIRS, (PIRS == 1) ? "Motion detected" : "Uncertain of occupancy status");

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x07 && pNode->type == 0x03)
				{ // A5-07-03, Occupancy sensor with Supply voltage monitor and 10-bit illumination measurement
					RBUF tsen;

					float SVC = GetDeviceValue(DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
					tsen.RFXSENSOR.packettype = pTypeRFXSensor;
					tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
					tsen.RFXSENSOR.seqnbr = 0;
					tsen.RFXSENSOR.id = ID_BYTE1;
					// WARNING
					// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
					// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
					tsen.RFXSENSOR.filler = bitrange(ID_BYTE0, 0, 0x0F);
					tsen.RFXSENSOR.rssi = bitrange(ID_BYTE0, 4, 0x0F);
					tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
					tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

					Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", senderID, SVC);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());

					float ILL = GetDeviceValue((DATA_BYTE2 << 2) | bitrange(DATA_BYTE1, 6, 0x03), 0, 1000, 0.0F, 1000.0F);

					_tLightMeter lmeter;
					lmeter.id1 = (BYTE) ID_BYTE3;
					lmeter.id2 = (BYTE) ID_BYTE2;
					lmeter.id3 = (BYTE) ID_BYTE1;
					lmeter.id4 = (BYTE) ID_BYTE0;
					lmeter.dunit = 1;
					lmeter.fLux = ILL;

					Debug(DEBUG_NORM, "4BS msg: Node %08X ILL %.1Flx", senderID, ILL);

					sDecodeRXMessage(this, (const unsigned char *) &lmeter, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());

					uint8_t PIRS = bitrange(DATA_BYTE0, 7, 0x01);

					memset(&tsen, 0, sizeof(RBUF));
					tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
					tsen.LIGHTING2.packettype = pTypeLighting2;
					tsen.LIGHTING2.subtype = sTypeAC;
					tsen.LIGHTING2.seqnbr = 0;
					tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
					tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
					tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
					tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
					tsen.LIGHTING2.level = 0;
					tsen.LIGHTING2.unitcode = 1;
					tsen.LIGHTING2.cmnd = (PIRS == 1) ? light2_sOn : light2_sOff;
					tsen.LIGHTING2.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)",
						senderID, PIRS, (PIRS == 1) ? "Motion detected" : "Uncertain of occupancy status");

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x09 && pNode->type == 0x04)
				{ // A5-09-04, CO2 Gas Sensor with Temp and Humidity
					// Battery level is not reported, set value 9 (OK)
					// TODO: Report battery level as 255 (unknown battery level) ?

					RBUF tsen;

					uint8_t HSN = bitrange(DATA_BYTE0, 2, 0x01);
					if (HSN == 1)
					{
						float HUM = GetDeviceValue(DATA_BYTE3, 0, 200, 0.0F, 100.0F);

						memset(&tsen, 0, sizeof(RBUF));
						tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
						tsen.HUM.packettype = pTypeHUM;
						tsen.HUM.subtype = sTypeHUM1; // LaCrosse TX3
						tsen.HUM.seqnbr = 0;
						tsen.HUM.id1 = ID_BYTE2;
						tsen.HUM.id2 = ID_BYTE1;
						tsen.HUM.humidity = (BYTE) round(HUM);
						tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
						tsen.HUM.battery_level = 9; // OK
						tsen.HUM.rssi = rssi;

						Debug(DEBUG_NORM, "4BS msg: Node %08X HUM %d%%", senderID, tsen.HUM.humidity);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.HUM, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
					}

					float CONC = GetDeviceValue(DATA_BYTE2, 0, 255, 0.0F, 2550.0F);

					Debug(DEBUG_NORM, "4BS msg: Node %08X CO2 %.1Fppm", senderID, CONC);

					SendAirQualitySensor(ID_BYTE2, ID_BYTE1, 9, round(CONC), GetEEPLabel(pNode->RORG, pNode->func, pNode->type));

					uint8_t TSN = bitrange(DATA_BYTE0, 1, 0x01);
					if (TSN == 1)
					{
						float TMP = GetDeviceValue(DATA_BYTE1, 0, 255, 0.0F, 51.0F);

						memset(&tsen, 0, sizeof(RBUF));
						tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
						tsen.TEMP.packettype = pTypeTEMP;
						tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
						tsen.TEMP.id1 = ID_BYTE2;
						tsen.TEMP.id2 = ID_BYTE1;
						// WARNING
						// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
						// decode_Temp assumes BatteryLevel to Unknown and SignalLevel to Not available
						tsen.TEMP.battery_level = ID_BYTE0 & 0x0F;
						tsen.TEMP.rssi = (ID_BYTE0 & 0xF0) >> 4;
						tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
						int at10 = round(std::abs(TMP * 10.0F));
						tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
						at10 -= (tsen.TEMP.temperatureh * 256);
						tsen.TEMP.temperaturel = (BYTE) at10;

						Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", senderID, TMP);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
					}
					return;
				}
				if (pNode->func == 0x10 && pNode->type <= 0x0D)
				{ // A5-10-01..OD, RoomOperatingPanel
					RBUF tsen;

					if (pNode->manufacturerID != ELTAKO)
					{ // General case for A5-10-01..OD
						// DATA_BYTE3 is the fan speed
						// DATA_BYTE2 is the setpoint where 0x00 = min ... 0xFF = max
						// DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
						// DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch

						// A5-10-01, A5-10-02, A5-10-04, A5-10-07, A5-10-08, A5-10-09 have FAN information
						if (pNode->type == 0x01 || pNode->type == 0x02 || pNode->type == 0x04 || pNode->type == 0x07 || pNode->type == 0x08 || pNode->type == 0x09)
						{
							uint8_t FAN;
							if (DATA_BYTE3 >= 210)
								FAN = fan_NovyLearn; // Auto
							else if (DATA_BYTE3 >= 190)
								FAN = fan_NovyLight;
							else if (DATA_BYTE3 >= 165)
								FAN = fan_NovyMin;
							else if (DATA_BYTE3 >= 145)
								FAN = fan_NovyPlus;
							else
								FAN = fan_NovyPower;

							memset(&tsen, 0, sizeof(RBUF));
							tsen.FAN.packetlength = sizeof(tsen.FAN) - 1;
							tsen.FAN.packettype = pTypeFan;
							tsen.FAN.subtype = sTypeNovy;
							tsen.FAN.seqnbr = 0;
							tsen.FAN.id1 = ID_BYTE3;
							tsen.FAN.id2 = ID_BYTE2;
							tsen.FAN.id3 = ID_BYTE1;
							tsen.FAN.cmnd = FAN;
							tsen.FAN.rssi = rssi;

							Debug(DEBUG_NORM, "4BS msg: Node %08X FAN %u", senderID, FAN);

							sDecodeRXMessage(this, (const unsigned char *) &tsen.FAN, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
						}

						// A5-10-01, A5-10-02, A5-10-03, A5-10-04, A5-10-05, A5-10-06, A5-10-0A have SP information
						if (pNode->type == 0x01 || pNode->type == 0x02 || pNode->type == 0x03 || pNode->type == 0x04 || pNode->type == 0x05 || pNode->type == 0x06 || pNode->type == 0x0A)
						{
							float SP = GetDeviceValue(DATA_BYTE2, 0, 255, 0.0F, 255.0F);

							Debug(DEBUG_NORM, "4BS msg: Node %08X SP %.0F not supported", senderID, SP);

							// TODO: implement SP
						}

						// A5-10-01, A5-10-05, A5-10-08, A5-10-0C have OCC information
						// A5-10-02, A5-10-06, A5-10-09, A5-10-0D have SLSW information
						// A5-10-0A, A5-10-0B have CTST information
						if (pNode->type == 0x01 || pNode->type == 0x05 || pNode->type == 0x08 || pNode->type == 0x0C
							|| pNode->type == 0x02 || pNode->type == 0x06 || pNode->type == 0x09 || pNode->type == 0x0D
							|| pNode->type == 0x0A || pNode->type == 0x0B)
						{
							uint8_t SW = bitrange(DATA_BYTE0, 0, 0x01);

							memset(&tsen, 0, sizeof(RBUF));
							tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
							tsen.LIGHTING2.packettype = pTypeLighting2;
							tsen.LIGHTING2.subtype = sTypeAC;
							tsen.LIGHTING2.seqnbr = 0;
							tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
							tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
							tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
							tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
							tsen.LIGHTING2.level = 0;
							tsen.LIGHTING2.unitcode = 1;
							tsen.LIGHTING2.cmnd = (SW == 0) ? light2_sOff : light2_sOn;
							tsen.LIGHTING2.rssi = rssi;

							const char *sSW;
							const char *sSW0;
							const char *sSW1;

							if (pNode->type == 0x01 || pNode->type == 0x05 || pNode->type == 0x08 || pNode->type == 0x0C)
								sSW = "OCC", sSW0 = "Button pressed", sSW1 = "Button released";
							else if (pNode->type == 0x02 || pNode->type == 0x06 || pNode->type == 0x09 || pNode->type == 0x0D)
								sSW = "SLSW", sSW0 = "Position I/Night/Off", sSW1 = "Position O/Day/On";
							else // if (pNode->type == 0x0A || pNode->type == 0x0B)
								sSW = "CTST", sSW0 = "Closed", sSW1 = "Open";

							Debug(DEBUG_NORM, "4BS msg: Node %08X %s %u (%s)", senderID, sSW, SW, (SW == 0) ? sSW0 : sSW1);

							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
						}
					}
					else
					{ // WARNING : ELTAKO specific implementation
						// DATA_BYTE3 is the night reduction
						// DATA_BYTE2 is the reference temperature where 0x00 = 0°C ... 0xFF = 40°C
						// DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
						// DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch

						uint8_t nightReduction = 0;
						if (DATA_BYTE3 == 0x06)
							nightReduction = 1;
						else if (DATA_BYTE3 == 0x0C)
							nightReduction = 2;
						else if (DATA_BYTE3 == 0x13)
							nightReduction = 3;
						else if (DATA_BYTE3 == 0x19)
							nightReduction = 4;
						else if (DATA_BYTE3 == 0x1F)
							nightReduction = 5;

						// float SPTMP = GetDeviceValue(DATA_BYTE2, 0, 255, 0.0F, 40.0F);
					}
					// All A5-10-01 to A5-10-0D have TMP information

					float TMP = GetDeviceValue(DATA_BYTE1, 255, 0, 0.0F, 40.0F);

					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
					tsen.TEMP.packettype = pTypeTEMP;
					tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
					tsen.TEMP.seqnbr = 0;
					tsen.TEMP.id1 = ID_BYTE2;
					tsen.TEMP.id2 = ID_BYTE1;
					// WARNING
					// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
					// decode_Temp assumes battery_level = 255 (Unknown) & rssi = 12 (Not available)
					tsen.TEMP.battery_level = bitrange(ID_BYTE0, 0, 0x0F);
					tsen.TEMP.rssi = bitrange(ID_BYTE0, 4, 0x0F);
					tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
					int at10 = round(std::abs(TMP * 10.0F));
					tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
					at10 -= tsen.TEMP.temperatureh * 256;
					tsen.TEMP.temperaturel = (BYTE) at10;

					Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", senderID, TMP);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x00)
				{ // A5-12-00, Automated Meter Reading, Counter
					uint8_t CH = bitrange(DATA_BYTE0, 4, 0x0F); // Channel number
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 = cumulative count, 1 = current value / s
					uint8_t DIV = bitrange(DATA_BYTE0, 0, 0x03);
					float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
					uint32_t MR = round(GetDeviceValue((DATA_BYTE3 << 16) | (DATA_BYTE2 << 8) | DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
					tsen.RFXMETER.packettype = pTypeRFXMeter;
					tsen.RFXMETER.subtype = sTypeRFXMeterCount;
					tsen.RFXMETER.seqnbr = 0;
					tsen.RFXMETER.id1 = ID_BYTE2;
					tsen.RFXMETER.id2 = ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);
					tsen.RFXMETER.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X CH %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID, CH, DT, DIV, scaleMax, MR);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x01)
				{ // A5-12-01, Automated Meter Reading, Electricity
					uint32_t MR = (DATA_BYTE3 << 16) | (DATA_BYTE2 << 8) | DATA_BYTE1;
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
					uint8_t DIV = bitrange(DATA_BYTE0, 0, 0x03);
					float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));

					_tUsageMeter umeter;
					umeter.id1 = (BYTE) ID_BYTE3;
					umeter.id2 = (BYTE) ID_BYTE2;
					umeter.id3 = (BYTE) ID_BYTE1;
					umeter.id4 = (BYTE) ID_BYTE0;
					umeter.dunit = 1;
					umeter.fusage = GetDeviceValue(MR, 0, 16777215, 0.0F, scaleMax);

					Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID, TI, DT, DIV, scaleMax, MR);

					sDecodeRXMessage(this, (const unsigned char *) &umeter, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x02)
				{ // A5-12-02, Automated Meter Reading, Gas
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
					uint8_t DIV = bitrange(DATA_BYTE0, 0, 0x03);
					float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
					uint32_t MR = round(GetDeviceValue((DATA_BYTE3 << 16) | (DATA_BYTE2 << 8) | DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
					tsen.RFXMETER.packettype = pTypeRFXMeter;
					tsen.RFXMETER.subtype = sTypeRFXMeterCount;
					tsen.RFXMETER.seqnbr = 0;
					tsen.RFXMETER.id1 = ID_BYTE2;
					tsen.RFXMETER.id2 = ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);
					tsen.RFXMETER.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID, TI, DT, DIV, scaleMax, MR);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x03)
				{ // A5-12-03, Automated Meter Reading, Water
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
					uint8_t DIV = bitrange(DATA_BYTE0, 0, 0x03);
					float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
					uint32_t MR = round(GetDeviceValue((DATA_BYTE3 << 16) | (DATA_BYTE2 << 8) | DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
					tsen.RFXMETER.packettype = pTypeRFXMeter;
					tsen.RFXMETER.subtype = sTypeRFXMeterCount;
					tsen.RFXMETER.seqnbr = 0;
					tsen.RFXMETER.id1 = ID_BYTE2;
					tsen.RFXMETER.id2 = ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);
					tsen.RFXMETER.rssi = rssi;

					Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID, TI, DT, DIV, scaleMax, MR);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				Log(LOG_ERROR, "4BS msg: Node %08X EEP %02X-%02X-%02X (%s) not supported",
					senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_RPS:
			{ // RPS telegram, F6-XX-XX, Repeated Switch Communication
				uint8_t DATA = data[1];

				uint8_t T21 = bitrange(STATUS, 5, 0x01); // 0 = PTM switch module of type 1 (PTM1xx), 1 = PTM switch module of type 2 (PTM2xx)
				uint8_t NU = bitrange(STATUS, 4, 0x01); // 0 = U-message (Unassigned), 1 = N-message (Normal)

				if (pNode == nullptr)
				{ // Node not found
					if (!m_sql.m_bAcceptNewHardware)
					{ // Node not found, but learn mode disabled
						Log(LOG_NORM, "RPS teach-in request from Node %08X", senderID);
						Log(LOG_NORM, "Unknown Node %08X, please allow accepting new hardware and proceed to teach-in", senderID);
						return;
					}
					// Node not found and learn mode enabled

					// EnOcean EEP 2.6.8 specification, §1.7 p.13
					// To avoid inadvertent learning, RPS telegrams have to be triggered 3 times within 2 seconds to allow teach-in

					if (senderID != m_RPS_teachin_nodeID)
					{ // First occurence of RPS teach-in request for senderID
						m_RPS_teachin_nodeID = senderID;
						m_RPS_teachin_DATA = DATA;
						m_RPS_teachin_STATUS = STATUS;
						m_RPS_teachin_timer = GetClockTicks();
						m_RPS_teachin_count = 1;
						Log(LOG_NORM, "RPS teach-in request #%u from Node %08X", m_RPS_teachin_count, senderID);
						Log(LOG_NORM, "RPS teach-in requires %u actions in less than %ldms to complete", TEACH_NB_REQUESTS, TEACH_MAX_DELAY);
						return;
					}
					if (DATA != m_RPS_teachin_DATA || STATUS != m_RPS_teachin_STATUS)
					{ // Same node ID, but data or status differ => ignore telegram
						Log(LOG_NORM, "RPS teach-in request from Node %08X (ignored telegram)", senderID);
						return;
					}
					time_t now = GetClockTicks();
					if (m_RPS_teachin_timer != 0 && now > (m_RPS_teachin_timer + TEACH_MAX_DELAY))
					{ // Max delay expired => teach-in request ignored
						m_RPS_teachin_nodeID = 0;
						Log(LOG_NORM, "RPS teach-in request from Node %08X (timed out)", senderID);
						return;
					}
					if (++m_RPS_teachin_count < TEACH_NB_REQUESTS)
					{ // TEACH_NB_REQUESTS not reached => continue to wait
						Log(LOG_NORM, "RPS teach-in request #%u from Node %08X", m_RPS_teachin_count, senderID);
						return;
					}
					// Received 3 identical telegrams from the node within delay
					// RPS teachin

					m_RPS_teachin_nodeID = 0;

					Log(LOG_NORM, "RPS teach-in request #%u from Node %08X accepted", m_RPS_teachin_count, senderID);

					// F6-02-01, Rocker Switch, 2 Rocker
					uint8_t node_func = 0x02;
					uint8_t node_type = 0x01;

					Log(LOG_NORM, "Creating Node %08X with generic EEP %02X-%02X-%02X (%s)",
						senderID, RORG_RPS, node_func, node_type, GetEEPLabel(RORG_RPS, node_func, node_type));
					Log(LOG_NORM, "Please adjust by hand if need be");

					TeachInNode(senderID, UNKNOWN_MANUFACTURER, RORG_RPS, node_func, node_type, true);
					return;
				}
				// RPS data

				// WARNING : several VLD nodes, with external switch/button control, also send RPS telegrams
				// Examples : D2-01-0F, D2-01-12, D2-01-15, D2-01-16, D2-01-17, D2-05-00...
				// Ignore RPS data for these nodes, because status will be reported by VLD datagram
				// nb. RPS RORG shall later be updated to VLD, either manually or whenever node sends VLD status

				// TODO: Remove that, as user may want to use external switch/button control as a sensor

				// If RORG = VLD
				// Or RORG unknown & RPS-func-type EEP doesn't exist & VLD-func-type EEP exists
				// => assume VLD and ignore RPS data

				if (pNode->RORG == RORG_VLD
					|| (pNode->RORG == 0x00
						&& GetEEP(RORG_RPS, pNode->func, pNode->type) == nullptr
						&& GetEEP(RORG_VLD, pNode->func, pNode->type) != nullptr))
				{
					Debug(DEBUG_NORM, "RPS %c-msg: Node %08X, button press from VLD device (ignored)",
						(NU == 0) ? 'U' : 'N', senderID);
					CheckAndUpdateNodeRORG(pNode, RORG_VLD);
					return;
				}
				CheckAndUpdateNodeRORG(pNode, RORG_RPS);

				Debug(DEBUG_NORM, "RPS %c-msg: Node %08X EEP %02X-%02X-%02X (%s) Data %02X (%s) Status %02X",
					(NU == 0) ? 'U' : 'N',
					senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type),
					DATA, (T21 == 0) ? "PTM1xx" : "PTM2xx", STATUS);

				if (pNode->func == 0x01 && pNode->type == 0x01)
				{ // F6-01-01, Switch Buttons, Push button
					uint8_t PB = bitrange(DATA, 4, 0x01);

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
					tsen.LIGHTING2.packettype = pTypeLighting2;
					tsen.LIGHTING2.subtype = sTypeAC;
					tsen.LIGHTING2.seqnbr = 0;
					tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
					tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
					tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
					tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
					tsen.LIGHTING2.level = 0;
					tsen.LIGHTING2.unitcode = 1;
					tsen.LIGHTING2.cmnd = PB ? light2_sOn : light2_sOff;
					tsen.LIGHTING2.rssi = rssi;

					Debug(DEBUG_NORM, "RPS msg: Node %08X PB %s", senderID, PB ? "Pressed" : "Released");

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x02 && (pNode->type == 0x01 ||pNode->type == 0x02))
				{ // F6-02-01, F6-02-02
					// F6-02-01, Rocker switch, 2 Rocker (Light and blind control, Application style 1)
					// F6-02-02, Rocker switch, 2 Rocker (Light and blind control, Application style 2)

					if (NU == 1)
					{ // RPS N-Message
						uint8_t R1 = bitrange(DATA, 5, 0x07);
						uint8_t R1_AB = bitrange(DATA, 6, 0x01);
						uint8_t R1_IO = bitrange(DATA, 5, 0x01);

						uint8_t EB = bitrange(DATA, 4, 0x01);

						uint8_t R2 = bitrange(DATA, 1, 0x07);
						uint8_t R2_AB = bitrange(DATA, 2, 0x01);
						uint8_t R2_IO = bitrange(DATA, 1, 0x01);

						uint8_t SA = bitrange(DATA, 0, 0x01);

						if (SA == 0)
							Debug(DEBUG_NORM, "RPS N-msg: Node %08X Button %c%c %s",
								senderID,
								(R1_AB == 0) ? 'A' : 'B', (R1_IO == 0) ? 'I' : 'O',
								(EB == 0) ? "released" : "pressed");
						else
							Debug(DEBUG_NORM, "RPS N-msg: Node %08X Buttons %c%c and %c%c simultaneously %s",
								senderID,
								(R1_AB == 0) ? 'A' : 'B', (R1_IO == 0) ? 'I' : 'O', (R2_AB == 0) ? 'A' : 'B', (R2_IO == 0) ? 'I' : 'O',
								(EB == 0) ? "released" : "pressed");

						if (EB == 1)
						{ // Energy bow pressed
							RBUF tsen;
							memset(&tsen, 0, sizeof(RBUF));
							tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
							tsen.LIGHTING2.packettype = pTypeLighting2;
							tsen.LIGHTING2.subtype = sTypeAC;
							tsen.LIGHTING2.seqnbr = 0;
							tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
							tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
							tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
							tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
							tsen.LIGHTING2.level = 0;

							// 3 types of buttons presses from a switch: A / B / A & B
							if (SA == 0)
								tsen.LIGHTING2.unitcode = R1 + 1; // A / B Pressed
							else
								tsen.LIGHTING2.unitcode = R1 + 10; // A & B Pressed

							tsen.LIGHTING2.cmnd = light2_sOn; // Button is pressed, so we don't get an OFF message here
							tsen.LIGHTING2.rssi = rssi;

							Debug(DEBUG_NORM, "RPS N-msg: Node %08X UnitID %02X Cmd %02X",
								senderID, tsen.LIGHTING2.unitcode, tsen.LIGHTING2.cmnd);

							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
						}
					}
					else
					{ // RPS U-Message
						// Release message of any button pressed before

						uint8_t R1 = bitrange(DATA, 5, 0x07);
						uint8_t EB = bitrange(DATA, 4, 0x01);

						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
						tsen.LIGHTING2.packettype = pTypeLighting2;
						tsen.LIGHTING2.subtype = sTypeAC;
						tsen.LIGHTING2.seqnbr = 0;
						tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
						tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
						tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
						tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
						tsen.LIGHTING2.level = 0;
						tsen.LIGHTING2.unitcode = 0; // Does not matter, since we are using a group command
						tsen.LIGHTING2.cmnd = (EB == 1) ? light2_sGroupOn : light2_sGroupOff;
						tsen.LIGHTING2.rssi = rssi;

						Debug(DEBUG_NORM, "RPS U-msg: Node %08X Energy Bow %s (%s pressed) UnitID %02X Cmd %02X",
							senderID,
							(EB == 0) ? "released" : "pressed",
							(R1 == 0) ? "no button" : "3 or 4 buttons",
							tsen.LIGHTING2.unitcode, tsen.LIGHTING2.cmnd);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
					}
					return;
				}
				if (pNode->func == 0x05 && pNode->type == 0x00)
				{ // F6-05-00, Wind speed threshold detector
					bool WIND = (DATA == 0x10);
					int batterylevel = (DATA == 0x30) ? 5 : 100;

					Debug(DEBUG_NORM, "RPS msg: Node %08X Wind speed alarm %s Energy level %s",
						senderID, WIND ? "ON" : "OFF", (batterylevel > 5) ? "OK" : "LOW");

					SendSwitch(senderID, 1, batterylevel, WIND, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);
					return;
				}
				if (pNode->func == 0x05 && pNode->type == 0x01)
				{ // F6-05-01, Liquid Leakage Sensor (mechanic harvester)
					bool WAS = (DATA == 0x11);
					int batterylevel = 255;

					Debug(DEBUG_NORM, "RPS msg: Node %08X Liquid Leakage alarm %s", senderID, WAS ? "ON" : "OFF");

					SendSwitch(senderID, 1, batterylevel, WAS, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);
					return;
				}
				if (pNode->func == 0x05 && pNode->type == 0x02)
				{ // F6-05-02, Smoke detector
					bool SMO = (DATA == 0x10);
					int batterylevel = (DATA == 0x30) ? 5 : 100;

					Debug(DEBUG_NORM, "RPS msg: Node %08X Smoke alarm %s Energy level %s",
						senderID, SMO ? "ON" : "OFF", (batterylevel > 5) ? "OK" : "LOW");

					SendSwitch(senderID, 1, batterylevel, SMO, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);
					return;
				}
				Log(LOG_ERROR, "RPS msg: Node %08X EEP %02X-%02X-%02X (%s) not supported",
					senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_UTE:
			{ // UTE telegram, D4-XX-XX, Universal Teach-in
				uint8_t CMD = bitrange(data[1], 0, 0x0F);				// 0 = teach-in query, 1 = teach-In response
				if (CMD != 0)
				{
					Log(LOG_ERROR, "UTE teach request: Node %08X, CMD 0x%1X not supported", senderID, CMD);
					return;
				}
				// UTE teach-in or teach-out Query (UTE Telegram / CMD 0x0)

				uint8_t ute_direction = bitrange(data[1], 7, 0x01);	// 0 = uni-directional, 1 = bi-directional
				uint8_t ute_response = bitrange(data[1], 6, 0x01);	// 0 = yes, 1 = no
				uint8_t ute_request = bitrange(data[1], 4, 0x03); // 0 = teach-in, 1 = teach-out, 2 = teach-in or teach-out

				// TODO: is num_channel information reliable ?
				// EEP 2.6.8 specifies : 0x00..0xFE = individual channel number, 0xFF = all supported channels
				// Nodon SIN-2-2-01 Slot-in module (D2-01-0D) always sends 2, which corresponds to the number of supported channels
				// Nodon MSP-2-1-01 Micro Smart Plug (D2-01-0E) always sends 1, which corresponds to the number of supported channels
				uint8_t num_channel = data[2]; // 0x00..0xFE = individual channel number, 0xFF = all supported channels

				uint16_t node_manID = (bitrange(data[4], 0, 0x07) << 8) | data[3];

				uint8_t node_type = data[5];
				uint8_t node_func = data[6];
				uint8_t node_RORG = data[7];

				Log(LOG_NORM, "UTE %s-directional %s request from Node %08X, nb_channels %u, %sresponse expected",
					(ute_direction == 0) ? "uni" : "bi",
					(ute_request == 0) ? "teach-in" : ((ute_request == 1) ? "teach-out" : "teach-in or teach-out"),
					senderID, num_channel,
					(ute_response == 0) ? "" : "no ");

				uint8_t buf[13];
				uint8_t optbuf[7];

				if (ute_response == 0)
				{ // Prepare response buffer
					// The device intended to be taught-in broadcasts a query message
					// and gets back an addresses response message, containing its own ID as the transmission target address
					buf[0] = RORG_UTE;
					buf[1] = ((UTE_BIDIRECTIONAL & 0x01) << 7) | (UTE_RESPONSE & 0x0F); // UTE data
					buf[2] = num_channel;
					buf[3] = data[3]; // Manufacturer ID
					buf[4] = data[4];
					buf[5] = node_type;
					buf[6] = node_func;
					buf[7] = node_RORG;
					buf[8] = bitrange(m_id_chip, 24, 0xFF); // Sender ID
					buf[9] = bitrange(m_id_chip, 16, 0xFF);
					buf[10] = bitrange(m_id_chip, 8, 0xFF);
					buf[11] = bitrange(m_id_chip, 0, 0xFF);
					buf[12] = 0x00; // Status

					optbuf[0] = 0x03; // SubTelNum : Send = 0x03
					optbuf[1] = ID_BYTE3; // Dest ID
					optbuf[2] = ID_BYTE2;
					optbuf[3] = ID_BYTE1;
					optbuf[4] = ID_BYTE0;
					optbuf[5] = 0xFF; // RSSI : Send = 0xFF
					optbuf[6] = 0x00; // Seurity Level : Send = ignored
				}
				if (pNode == nullptr)
				{ // Node not found
					if (ute_request == 1)
					{ // Node not found and teach-out request => ignore
						Log(LOG_NORM, "Unknown Node %08X, teach-out request ignored", senderID);

						if (ute_response == 0)
						{ // Build and send response
							buf[1] |= (GENERAL_REASON & 0x03) << 4;

							Debug(DEBUG_NORM, "Send UTE teach-out refused response");

							SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, optbuf, 7);
						}
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{ // Node not found and learn mode disabled => error
						Log(LOG_NORM, "Unknown Node %08X, please allow accepting new hardware and proceed to teach-in", senderID);
						if (ute_response == 0)
						{ // Build and send response
							buf[1] |= (GENERAL_REASON & 0x03) << 4;

							Debug(DEBUG_NORM, "Send UTE teach-in refused response");

							SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, optbuf, 7);
						}
						return;
					}
					// Node not found and learn mode enabled and teach-in request : add it to the database

					Log(LOG_NORM, "Creating Node %08X Manufacturer %03X (%s) EEP %02X-%02X-%02X (%s), %u channel%s",
						senderID, node_manID, GetManufacturerName(node_manID),
						node_RORG, node_func, node_type, GetEEPLabel(node_RORG, node_func, node_type),
						num_channel, (num_channel > 1) ? "s" : "");

					TeachInNode(senderID, node_manID, node_RORG, node_func, node_type, false);

					if (ute_response == 0)
					{ // Build and send response
						buf[1] |= (TEACHIN_ACCEPTED & 0x03) << 4;

						Debug(DEBUG_NORM, "Send UTE teach-in accepted response");

						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, optbuf, 7);
					}
					// TODO : instead of creating node channels, ask all node channels to report their status

					if (node_RORG == RORG_VLD && node_func == 0x01
						&& (node_type == 0x0D || node_type == 0x0E
							|| node_type == 0x0F || node_type == 0x12
							|| node_type == 0x15 || node_type == 0x16
							|| node_type == 0x17))
					{ // Create devices for :
						// D2-01-0D, Micro smart plug, single channel, with external button control
						// D2-01-0E, Micro smart plug, single channel, with external button control
						// D2-01-0F, Slot-in module, single channel, with external button control
						// D2-01-12, Slot-in module, dual channels, with external button control
						// D2-01-15, D2-01-16, D2-01-17

						// TODO : num_channel is only a valid channel number when between 0x00 & 0x1D
						// 0x1E means all supported output channels
						// 0x1F means input channel (for mains supply)
						for (uint8_t nbc = 1; nbc <= num_channel; nbc++)
						{
							RBUF tsen;

							memset(&tsen, 0, sizeof(RBUF));
							tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
							tsen.LIGHTING2.packettype = pTypeLighting2;
							tsen.LIGHTING2.subtype = sTypeAC;
							tsen.LIGHTING2.seqnbr = 0;
							tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
							tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
							tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
							tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
							tsen.LIGHTING2.level = 0;
							tsen.LIGHTING2.unitcode = nbc;
							tsen.LIGHTING2.cmnd = light2_sOff;
							tsen.LIGHTING2.rssi = rssi;

							Debug(DEBUG_NORM, "UTE msg: Node %08X, creating switch/dimmer channel %d", senderID, nbc);

							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(node_RORG, node_func, node_type), 255, m_Name.c_str());
						}
					}
					return;
				}
				// Node found

				CheckAndUpdateNodeRORG(pNode, node_RORG);

				if (ute_request == 0)
				{ // Node found and teach-in request => ignore
					Log(LOG_NORM, "Node %08X already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
						senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));

					if (ute_response == 0)
					{ // Build and send response
						buf[1] |= (TEACHIN_ACCEPTED & 0x03) << 4;

						Debug(DEBUG_NORM, "Send UTE teach-in accepted response");

						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, optbuf, 7);
					}
				}
				else if (ute_request == 1 || ute_request == 2)
				{ // Node found and teach-out request => teach-out
					// Ignore teach-out request to avoid teach-in/out loop
					Debug(DEBUG_NORM, "UTE msg: Node %08X, teach-out request not supported", senderID);

					if (ute_response == 0)
					{ // Build and send response
						buf[1] |= (GENERAL_REASON & 0x03) << 4;

						Debug(DEBUG_NORM, "Send UTE teach-out refused response");

						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, optbuf, 7);
					}
				}
			}
			return;

		case RORG_VLD:
			{ // VLD telegram, D2-XX-XX, Variable Length Data
				if (pNode == nullptr)
				{
					Log(LOG_NORM, "VLD msg: Unknown Node %08X, please proceed to teach-in", senderID);
					return;
				}
				CheckAndUpdateNodeRORG(pNode, RORG_VLD);

				Debug(DEBUG_NORM, "VLD msg: Node %08X EEP %02X-%02X-%02X", senderID, pNode->RORG, pNode->func, pNode->type);

				if (pNode->func == 0x01)
				{ // D2-01-XX, Electronic Switches and Dimmers with Local Control
					uint8_t CMD = bitrange(data[1], 0, 0x0F); // Command ID

					// TODO: manage remote configuration and querying/polling

					if (CMD == 0x04)
					{ // CMD 0x4 - Actuator Status Response
						uint8_t IO = bitrange(data[2], 0, 0x1F); // I/O Channel

						uint8_t OV = bitrange(data[3], 0, 0x7F); // Output Value : 0x00 = Off, 0x01...0x64: On or 1% to 100%

						uint8_t PF = bitrange(data[1], 7, 0x01); // Power failure
						uint8_t PFD = bitrange(data[1], 6, 0x01); // Power failure detection

						uint8_t OC = bitrange(data[2], 7, 0x01); // Over current switch off
						uint8_t EL = bitrange(data[2], 5, 0x03); // Error level

						uint8_t LC = bitrange(data[3], 7, 0x01); // Local control

						RBUF tsen;
						memset(&tsen, 0, sizeof(RBUF));
						tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
						tsen.LIGHTING2.packettype = pTypeLighting2;
						tsen.LIGHTING2.subtype = sTypeAC;
						tsen.LIGHTING2.seqnbr = 0;
						tsen.LIGHTING2.id1 = (BYTE) ID_BYTE3;
						tsen.LIGHTING2.id2 = (BYTE) ID_BYTE2;
						tsen.LIGHTING2.id3 = (BYTE) ID_BYTE1;
						tsen.LIGHTING2.id4 = (BYTE) ID_BYTE0;
						tsen.LIGHTING2.level = OV;
						tsen.LIGHTING2.unitcode = IO + 1;
						tsen.LIGHTING2.cmnd = (OV > 0) ? light2_sOn : light2_sOff;
						tsen.LIGHTING2.rssi = rssi;

						Debug(DEBUG_NORM, "VLD msg: Node %08X status, IO %02X (UnitID %d) OV %02X (Cmnd %s Level %d)",
							senderID, IO, tsen.LIGHTING2.unitcode, OV, tsen.LIGHTING2.cmnd ? "On" : "Off", tsen.LIGHTING2.level);
						Debug(DEBUG_NORM, "VLD msg: Node %08X status, PF %d PFD %d OC %d EL %d LC %d",
							senderID, PF, PFD, OC, EL, LC);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), 255, m_Name.c_str());
						return;
					}
					// TODO: handle other CMD returning status data
					// TODO: manage CMD 0x7 - Actuator Measurement Response
					// TODO: manage CMD 0xA - Actuator Pilot Wire Mode Response

					Log(LOG_ERROR, "VLD msg: Node %08X, command 0x%01X not supported", senderID, CMD);
					return;
				}
				if (pNode->func == 0x03 && pNode->type == 0x0A)
				{ // D2-03-0A, Push Button, Single Button
					uint8_t BATT = round(GetDeviceValue(data[1], 1, 100, 1.0F, 100.0F));
					uint8_t BA = data[2]; // 1 = Simple press, 2 = Double press, 3 = Long press, 4 = Long press released

					Debug(DEBUG_NORM, "VLD msg: Node %08X status, BATT %d%% BA %02X (%s)", senderID,
						BATT, BA, (BA == 1) ? "Simple press" : ((BA == 2) ? "Double press" : ((BA == 3) ? "Long press" : ((BA == 4) ? "Long press released" : "Invalid value"))));

					SendGeneralSwitch(senderID, BA, BATT, 1, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);
					return;
				}
				if (pNode->func == 0x14 && pNode->type == 0x30)
				{ // D2-14-30, Sensor for Smoke, Air quality, Hygrothermal comfort, Temperature and Humidity
					// Smoke Alarm Status, 0 = Non activated, 1 = Activated
					uint8_t SMA_SAS = bitrange(data[1], 7, 0x01);
					// Sensor Fault Mode Status, 0 = Non activated, 1 = Activated
					uint8_t SMA_SFMS = bitrange(data[1], 6, 0x01);
					// Smoke Alarm Condition Analysis Maintenance, 0 = Not done, 1 = Maintenance OK
					uint8_t SMA_SACA_MNT = bitrange(data[1], 5, 0x01);
					// Smoke Alarm Condition Analysis Humidity, 0 = Range OK, 1 = Range NOK
					uint8_t SMA_SACA_HUM = bitrange(data[1], 4, 0x01);
					// Smoke Alarm Condition Analysis Temperature, 0 = Range OK, 1 = Range NOK
					uint8_t SMA_SACA_TMP = bitrange(data[1], 3, 0x01);
					// Time Since Last Maintenance, in weeks
					uint8_t SMA_TSLM = (bitrange(data[1], 0, 0x07) << 5) | bitrange(data[2], 3, 0x1F);
					// Energy Storage, 0 = High, 1 = Medium, 2 = Low, 3 = Critical
					uint8_t ES = bitrange(data[2], 1, 0x03);
					// Remaining Product Life Time, in months
					uint8_t RPLT = (bitrange(data[2], 0, 0x01) << 7) | bitrange(data[3], 1, 0x7F);
					// Temperature (linear), 0..250=>0..50°C, 255 = Error
					uint8_t TMP8 = (bitrange(data[3], 0, 0x01) << 7) | bitrange(data[4], 1, 0x7F);
					// Relative Humidity (linear), 0..200=>0..100%RH, 255 = Error
					uint8_t HUM = (bitrange(data[4], 0, 0x01) << 7) | bitrange(data[5], 1, 0x7F);
					// Hygrothermal Comfort Index, 0 = Good, 1 = Medium, 2 = Bad, 3 = Error
					uint8_t HCI = (bitrange(data[5], 0, 0x01) << 1) | bitrange(data[6], 7, 0x01);
					// Indoor Air Quality Analysis, 0 = Optimal, 1 = Dry, 2 = High hum rng, 3 = High temp & hum rng, 4 = Temp & hum out of rng, 7 = Error
					uint8_t IAQTH = bitrange(data[6], 4, 0x07);

					Debug(DEBUG_NORM, "VLD msg: Node %08X status, SMA_SAS %u SMA_SFMS %u SMA_SACA_MNT %u SMA_SACA_HUM %u SMA_SACA_TMP %u SMA_TSLM %u ES %u RPLT %u TMP8 %u HUM %u HCI %u IAQTH %u",
						senderID, SMA_SAS, SMA_SFMS, SMA_SACA_MNT, SMA_SACA_HUM, SMA_SACA_TMP, SMA_TSLM, ES, RPLT, TMP8, HUM, HCI, IAQTH);

					bool alarm = (SMA_SAS == 1);
					int batterylevel = (ES == 0) ? 100 : ((ES == 0) ? 100 : ((ES == 0) ? 50 : ((ES == 0) ? 25 : 10)));
					SendSwitch(senderID, 1, batterylevel, alarm, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);

					RBUF tsen;

					if (TMP8 <= 250)
					{
						float FTMP8 = GetDeviceValue(TMP8, 0, 250, 0.0F, 50.0F);

						memset(&tsen, 0, sizeof(RBUF));
						tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
						tsen.TEMP.packettype = pTypeTEMP;
						tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
						tsen.TEMP.id1 = ID_BYTE2;
						tsen.TEMP.id2 = ID_BYTE1;
						// WARNING
						// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
						// decode_Temp assumes BatteryLevel to Unknown and SignalLevel to Not available
						tsen.TEMP.battery_level = bitrange(ID_BYTE0, 0, 0x0F);
						tsen.TEMP.rssi = bitrange(ID_BYTE0, 4, 0x0F);
						tsen.TEMP.tempsign = (FTMP8 >= 0) ? 0 : 1;
						int at10 = round(std::abs(FTMP8 * 10.0F));
						tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
						at10 -= (tsen.TEMP.temperatureh * 256);
						tsen.TEMP.temperaturel = (BYTE) at10;

						Debug(DEBUG_NORM, "VLD msg: Node %08X TMP8 %.1F°C", senderID, FTMP8);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
					}
					if (HUM <= 200)
					{
						float FHUM = GetDeviceValue(HUM, 0, 200, 0.0F, 100.0F);

						memset(&tsen, 0, sizeof(RBUF));
						tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
						tsen.HUM.packettype = pTypeHUM;
						tsen.HUM.subtype = sTypeHUM1; // LaCrosse TX3
						tsen.HUM.seqnbr = 0;
						tsen.HUM.id1 = ID_BYTE2;
						tsen.HUM.id2 = ID_BYTE1;
						tsen.HUM.humidity = (BYTE) round(FHUM);
						tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
						tsen.HUM.battery_level = (BYTE) batterylevel;
						tsen.HUM.rssi = rssi;

						Debug(DEBUG_NORM, "VLD msg: Node %08X HUM %d%%", senderID, tsen.HUM.humidity);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.HUM, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), -1, m_Name.c_str());
					}
					return;
				}
				Log(LOG_ERROR, "VLD msg: Node %08X EEP %02X-%02X-%02X (%s) not supported",
					senderID, pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		default:
			Log(LOG_ERROR, "ERP1: Node %08X RORG %s (%s) not supported", senderID, GetRORGLabel(RORG), GetRORGDescription(RORG));
	}
}

struct _tPacketTypeTable
{
	uint8_t PT;
	const char *label;
	const char *description;
};

static const _tPacketTypeTable _packetTypeTable[] = {
	{ PACKET_RADIO_ERP1, "RADIO_ERP1", "ERP1 radio telegram" },
	{ PACKET_RESPONSE, "RESPONSE", "Response to any packet" },
	{ PACKET_RADIO_SUB_TEL, "RADIO_SUB_TEL", "Radio subtelegram" },
	{ PACKET_EVENT, "EVENT", "Event message" },
	{ PACKET_COMMON_COMMAND, "COMMON_COMMAND", "Common command" },
	{ PACKET_SMART_ACK_COMMAND, "SMART_ACK_COMMAND", "Smart Acknowledge command" },
	{ PACKET_REMOTE_MAN_COMMAND, "REMOTE_MAN_COMMAND", "Remote management command" },
	{ PACKET_RADIO_MESSAGE, "RADIO_MESSAGE", "Radio message" },
	{ PACKET_RADIO_ERP2, "RADIO_ERP2", "ERP2 radio telegram" },
	{ PACKET_CONFIG_COMMAND, "CONFIG_COMMAND", "RESERVED" },
	{ PACKET_COMMAND_ACCEPTED, "COMMAND_ACCEPTED", "For long operations, informs the host the command is accepted" },
	{ PACKET_RADIO_802_15_4, "RADIO_802_15_4", "802_15_4 Raw Packet" },
	{ PACKET_COMMAND_2_4, "COMMAND_2_4", "2.4 GHz Command" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetPacketTypeLabel(uint8_t PT)
{
	for (const _tPacketTypeTable *pTable = _packetTypeTable; pTable->PT; pTable++)
		if (pTable->PT == PT)
			return pTable->label;

	return "RESERVED";
}

const char *CEnOceanESP3::GetPacketTypeDescription(uint8_t PT)
{
	for (const _tPacketTypeTable *pTable = _packetTypeTable; pTable->PT; pTable++)
		if (pTable->PT == PT)
			return pTable->description;

	return "Reserved ESP3 packet type";
}

struct _tReturnCodeTable
{
	uint8_t RC;
	const char *label;
	const char *description;
};

static const _tReturnCodeTable _returnCodeTable[] = {
	{ RET_OK, "OK", "No error" },
	{ RET_ERROR, "ERROR", "There is an error occurred" },
	{ RET_NOT_SUPPORTED, "NOT_SUPPORTED", "The functionality is not supported by that implementation" },
	{ RET_WRONG_PARAM, "WRONG_PARAM", "There was a wrong parameter in the command" },
	{ RET_OPERATION_DENIED, "OPERATION_DENIED", "The operation cannot be performed" },
	{ RET_LOCK_SET, "LOCK_SET", "Duty cycle lock" },
	{ RET_BUFFER_TO_SMALL, "BUFFER_TO_SMALL", "The internal ESP3 buffer of the device is too small, to handle this telegram" },
	{ RET_NO_FREE_BUFFER, "NO_FREE_BUFFER", "Currently all internal buffers are used" },
	{ RET_MEMORY_ERROR, "MEMORY_ERROR", "The memory write process failed" },
	{ RET_BASEID_OUT_OF_RANGE, "BASEID_OUT_OF_RANGE", "Invalid BaseID" },
	{ RET_BASEID_MAX_REACHED, "BASEID_MAX_REACHED", "BaseID has already been changed 10 times, no more changes are allowed" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetReturnCodeLabel(uint8_t RC)
{
	for (const _tReturnCodeTable *pTable = _returnCodeTable; pTable->label; pTable++)
		if (pTable->RC == RC)
			return pTable->label;

	if (RC > 0x80)
		return "RC>0x80";

	return "UNKNOWN";
}

const char *CEnOceanESP3::GetReturnCodeDescription(uint8_t RC)
{
	for (const _tReturnCodeTable *pTable = _returnCodeTable; pTable->description; pTable++)
		if (pTable->RC == RC)
			return pTable->description;

	if (RC > 0x80)
		return "Return codes greater than 0x80 used for commands with special return information, not commonly useable";

	return "<<Unknown return code... Please report!<<";
}

struct _tEventCodeTable
{
	uint8_t EC;
	const char *label;
	const char *description;
};
static const _tEventCodeTable _eventCodeTable[] = {
	{ SA_RECLAIM_NOT_SUCCESSFUL, "RECLAIM_NOT_SUCCESSFUL", "Informs the external host about an unsuccessful reclaim by a Smart Ack client" },
	{ SA_CONFIRM_LEARN, "CONFIRM_LEARN", "Request to the external host about how to handle a received learn-in / learn-out of a Smart Ack. client" },
	{ SA_LEARN_ACK, "LEARN_ACK", "Response to the Smart Ack. client about the result of its Smart Acknowledge learn request" },
	{ CO_READY, "READY", "Inform the external about the readiness for operation" },
	{ CO_EVENT_SECUREDEVICES, "EVENT_SECUREDEVICES", "Informs the external host about an event in relation to security processing" },
	{ CO_DUTYCYCLE_LIMIT, "DUTYCYCLE_LIMIT", "Informs the external host about reaching the duty cycle limit" },
	{ CO_TRANSMIT_FAILED, "TRANSMIT_FAILED", "Informs the external host about not being able to send a telegram" },
	{ CO_TX_DONE, "TX_DONE", "Informs that all TX operations are done" },
	{ CO_LRN_MODE_DISABLED, "LRN_MODE_DISABLED", "Informs that the learn mode has time-out" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetEventCodeLabel(const uint8_t EC)
{
	for (const _tEventCodeTable *pTable = _eventCodeTable; pTable->EC; pTable++)
		if (pTable->EC == EC)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanESP3::GetEventCodeDescription(const uint8_t EC)
{
	for (const _tEventCodeTable *pTable = _eventCodeTable; pTable->EC; pTable++)
		if (pTable->EC == EC)
			return pTable->description;

	return ">>Unkown function event code... Please report!<<";
}

struct _tCommonCommandTable
{
	uint8_t CC;
	const char *label;
	const char *description;
};

static const _tCommonCommandTable _commonCommandTable[] = {
	{ CO_WR_SLEEP, "WR_SLEEP", "Enter energy saving mode" },
	{ CO_WR_RESET, "WR_RESET", "Reset the device" },
	{ CO_RD_VERSION, "RD_VERSION", "Read the device version information" },
	{ CO_RD_SYS_LOG, "RD_SYS_LOG", "Read system log" },
	{ CO_WR_SYS_LOG, "WR_SYS_LOG", "Reset system log" },
	{ CO_WR_BIST, "WR_BIST", "Perform Self Test" },
	{ CO_WR_IDBASE, "WR_IDBASE", "Set ID range base address" },
	{ CO_RD_IDBASE, "RD_IDBASE", "Read ID range base address" },
	{ CO_WR_REPEATER, "WR_REPEATER", "Set Repeater Level" },
	{ CO_RD_REPEATER, "RD_REPEATER", "Read Repeater Level" },
	{ CO_WR_FILTER_ADD, "WR_FILTER_ADD", "Add filter to filter list" },
	{ CO_WR_FILTER_DEL, "WR_FILTER_DEL", "Delete a specific filter from filter list" },
	{ CO_WR_FILTER_DEL_ALL, "WR_FILTER_DEL_ALL", "Delete all filters from filter list" },
	{ CO_WR_FILTER_ENABLE, "WR_FILTER_ENABLE", "Enable / disable filter list" },
	{ CO_RD_FILTER, "RD_FILTER", "Read filters from filter list" },
	{ CO_WR_WAIT_MATURITY, "WR_WAIT_MATURITY", "Wait until the end of telegram maturity time before received radio telegrams will be forwarded to the external host" },
	{ CO_WR_SUBTEL, "WR_SUBTEL", "Enable / Disable transmission of additional subtelegram info to the external host" },
	{ CO_WR_MEM, "WR_MEM", "Write data to device memory" },
	{ CO_RD_MEM, "RD_MEM", "Read data from device memory" },
	{ CO_RD_MEM_ADDRESS, "RD_MEM_ADDRESS", "Read address and length of the configuration area and the Smart Ack Table" },
	{ CO_RD_SECURITY, "RD_SECURITY", "key)" },
	{ CO_WR_SECURITY, "WR_SECURITY", "key)" },
	{ CO_WR_LEARNMODE, "WR_LEARNMODE", "Enable / disable learn mode" },
	{ CO_RD_LEARNMODE, "RD_LEARNMODE", "ead learn mode status" },
	{ CO_WR_SECUREDEVICE_ADD, "WR_SECUREDEVICE_ADD", "DEPRECATED Add a secure device" },
	{ CO_WR_SECUREDEVICE_DEL, "WR_SECUREDEVICE_DEL", "Delete a secure device from the link table" },
	{ CO_RD_SECUREDEVICE_BY_INDEX, "RD_SECUREDEVICE_BY_INDEX", "DEPRECATED Read secure device by index" },
	{ CO_WR_MODE, "WR_MODE", "Set the gateway transceiver mode" },
	{ CO_RD_NUMSECUREDEVICES, "RD_NUMSECUREDEVICES", "Read number of secure devices in the secure link table" },
	{ CO_RD_SECUREDEVICE_BY_ID, "RD_SECUREDEVICE_BY_ID", "Read information about a specific secure device from the secure link table using the device ID" },
	{ CO_WR_SECUREDEVICE_ADD_PSK, "WR_SECUREDEVICE_ADD_PSK", "Add Pre-shared key for inbound secure device" },
	{ CO_WR_SECUREDEVICE_ENDTEACHIN, "WR_SECUREDEVICE_ENDTEACHIN", "Send Secure teach-In message" },
	{ CO_WR_TEMPORARY_RLC_WINDOW, "WR_TEMPORARY_RLC_WINDOW", "Set a temporary rolling-code window for every taught-in device" },
	{ CO_RD_SECUREDEVICE_PSK, "RD_SECUREDEVICE_PSK", "Read PSK" },
	{ CO_RD_DUTYCYCLE_LIMIT, "RD_DUTYCYCLE_LIMIT", "Read the status of the duty cycle limit monitor" },
	{ CO_SET_BAUDRATE, "SET_BAUDRATE", "Set the baud rate used to communicate with the external host" },
	{ CO_GET_FREQUENCY_INFO, "GET_FREQUENCY_INFO", "Read the radio frequency and protocol supported by the device" },
	{ CO_38T_STEPCODE, "38T_STEPCODE", "Read Hardware Step code and Revision of the Device" },
	{ CO_40_RESERVED, "40_RESERVED", "Reserved" },
	{ CO_41_RESERVED, "41_RESERVED", "Reserved" },
	{ CO_42_RESERVED, "42_RESERVED", "Reserved" },
	{ CO_43_RESERVED, "43_RESERVED", "Reserved" },
	{ CO_44_RESERVED, "44_RESERVED", "Reserved" },
	{ CO_45_RESERVED, "45_RESERVED", "Reserved" },
	{ CO_WR_REMAN_CODE, "WR_REMAN_CODE", "Set the security code to unlock Remote Management functionality via radio" },
	{ CO_WR_STARTUP_DELAY, "WR_STARTUP_DELAY", "Set the startup delay (time from power up until start of operation)" },
	{ CO_WR_REMAN_REPEATING, "WR_REMAN_REPEATING", "Select if REMAN telegrams originating from this module can be repeated" },
	{ CO_RD_REMAN_REPEATING, "RD_REMAN_REPEATING", "Check if REMAN telegrams originating from this module can be repeated" },
	{ CO_SET_NOISETHRESHOLD, "SET_NOISETHRESHOLD", "Set the RSSI noise threshold level for telegram reception" },
	{ CO_GET_NOISETHRESHOLD, "GET_NOISETHRESHOLD", "Read the RSSI noise threshold level for telegram reception" },
	{ CO_52_RESERVED, "52_RESERVED", "Reserved" },
	{ CO_53_RESERVED, "53_RESERVED", "Reserved" },
	{ CO_WR_RLC_SAVE_PERIOD, "WR_RLC_SAVE_PERIOD", "Set the period in which outgoing RLCs are saved to the EEPROM" },
	{ CO_WR_RLC_LEGACY_MODE, "WR_RLC_LEGACY_MODE", "Activate the legacy RLC security mode allowing roll-over and using the RLC acceptance window for 24bit explicit RLC" },
	{ CO_WR_SECUREDEVICEV2_ADD, "WR_SECUREDEVICEV2_ADD", "Add secure device to secure link table" },
	{ CO_RD_SECUREDEVICEV2_BY_INDEX, "RD_SECUREDEVICEV2_BY_INDEX", "Read secure device from secure link table using the table index" },
	{ CO_WR_RSSITEST_MODE, "WR_RSSITEST_MODE", "Control the state of the RSSI-Test mode" },
	{ CO_RD_RSSITEST_MODE, "RD_RSSITEST_MODE", "Read the state of the RSSI-Test Mode" },
	{ CO_WR_SECUREDEVICE_MAINTENANCEKEY, "WR_SECUREDEVICE_MAINTENANCEKEY", "Add the maintenance key information into the secure link table" },
	{ CO_RD_SECUREDEVICE_MAINTENANCEKEY, "RD_SECUREDEVICE_MAINTENANCEKEY", "Read by index the maintenance key information from the secure link table" },
	{ CO_WR_TRANSPARENT_MODE, "WR_TRANSPARENT_MODE", "Control the state of the transparent mode" },
	{ CO_RD_TRANSPARENT_MODE, "RD_TRANSPARENT_MODE", "Read the state of the transparent mode" },
	{ CO_WR_TX_ONLY_MODE, "WR_TX_ONLY_MODE", "Control the state of the TX only mode" },
	{ CO_RD_TX_ONLY_MODE, "RD_TX_ONLY_MODE", "Read the state of the TX only mode" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetCommonCommandLabel(const uint8_t CC)
{
	for (const _tCommonCommandTable *pTable = _commonCommandTable; pTable->CC; pTable++)
		if (pTable->CC == CC)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanESP3::GetCommonCommandDescription(const uint8_t CC)
{
	for (const _tCommonCommandTable *pTable = _commonCommandTable; pTable->CC; pTable++)
		if (pTable->CC == CC)
			return pTable->description;

	return ">>Unkown Common Command... Please report!<<";
}

struct _tSmarkAckCodeTable
{
	uint8_t SA;
	const char *label;
	const char *description;
};

static const _tSmarkAckCodeTable _smarkAckCodeTable[] = {
	{ SA_WR_LEARNMODE, "WR_LEARNMODE", "Set/Reset Smart Ack learn mode" },
	{ SA_RD_LEARNMODE, "RD_LEARNMODE", "Get Smart Ack learn mode state" },
	{ SA_WR_LEARNCONFIRM, "WR_LEARNCONFIRM", "Used for Smart Ack to add or delete a mailbox of a client" },
	{ SA_WR_CLIENTLEARNRQ, "WR_CLIENTLEARNRQ", "Send Smart Ack Learn request (Client)" },
	{ SA_WR_RESET, "WR_RESET", "Send reset command to a Smart Ack client" },
	{ SA_RD_LEARNEDCLIENTS, "RD_LEARNEDCLIENTS", "Get Smart Ack learned sensors / mailboxes" },
	{ SA_WR_RECLAIMS, "WR_RECLAIMS", "Set number of reclaim attempts" },
	{ SA_WR_POSTMASTER, "WR_POSTMASTER", "Activate/Deactivate Post master functionality" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetSmarkAckCodeLabel(const uint8_t SA)
{
	for (const _tSmarkAckCodeTable *pTable = _smarkAckCodeTable; pTable->SA; pTable++)
		if (pTable->SA == SA)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanESP3::GetSmartAckCodeDescription(const uint8_t SA)
{
	for (const _tSmarkAckCodeTable *pTable = _smarkAckCodeTable; pTable->SA; pTable++)
		if (pTable->SA == SA)
			return pTable->description;

	return ">>Unkown smark ack code... Please report!<<";
}

struct _tFunctionReturnCodeTable
{
	uint8_t RC;
	const char *label;
	const char *description;
};

static const _tFunctionReturnCodeTable _functionReturnCodeTable[] = {
	{ RC_OK, "RC_OK", "Action performed. No problem detected" },
	{ RC_EXIT, "RC_EXIT", "Action not performed. No problem detected" },
	{ RC_KO, "RC_KO", "Action not performed. Problem detected" },
	{ RC_TIME_OUT, "RC_TIME_OUT", "Action couldn't be carried out within a certain time." },
	{ RC_FLASH_HW_ERROR, "RC_FLASH_HW_ERROR", "The write/erase/verify process failed, the flash page seems to be corrupted" },
	{ RC_NEW_RX_BYTE, "RC_NEW_RX_BYTE", "A new UART/SPI byte received" },
	{ RC_NO_RX_BYTE, "RC_NO_RX_BYTE", "No new UART/SPI byte received" },
	{ RC_NEW_RX_TEL, "RC_NEW_RX_TEL", "New telegram received" },
	{ RC_NO_RX_TEL, "RC_NO_RX_TEL", "No new telegram received" },
	{ RC_NOT_VALID_CHKSUM, "RC_NOT_VALID_CHKSUM", "Checksum not valid" },
	{ RC_NOT_VALID_TEL, "RC_NOT_VALID_TEL", "Telegram not valid" },
	{ RC_BUFF_FULL, "RC_BUFF_FULL", "Buffer full, no space in Tx or Rx buffer" },
	{ RC_ADDR_OUT_OF_MEM, "RC_ADDR_OUT_OF_MEM", "Address is out of memory" },
	{ RC_NOT_VALID_PARAM, "RC_NOT_VALID_PARAM", "Invalid function parameter" },
	{ RC_BIST_FAILED, "RC_BIST_FAILED", "Built in self test failed" },
	{ RC_ST_TIMEOUT_BEFORE_SLEEP, "RC_ST_TIMEOUT_BEFORE_SLEEP", "Before entering power down, the short term timer had timed out." },
	{ RC_MAX_FILTER_REACHED, "RC_MAX_FILTER_REACHED", "Maximum number of filters reached, no more filter possible" },
	{ RC_FILTER_NOT_FOUND, "RC_FILTER_NOT_FOUND", "Filter to delete not found" },
	{ RC_BASEID_OUT_OF_RANGE, "RC_BASEID_OUT_OF_RANGE", "BaseID out of range" },
	{ RC_BASEID_MAX_REACHED, "RC_BASEID_MAX_REACHED", "BaseID was changed 10 times, no more changes are allowed" },
	{ RC_XTAL_NOT_STABLE, "RC_XTAL_NOT_STABLE", "XTAL is not stable" },
	{ RC_NO_TX_TEL, "RC_NO_TX_TEL", "No telegram for transmission in queue" },
	{ RC_ELEGRAM_WAIT, "RC_ELEGRAM_WAIT", "Waiting before sending broadcast message" },
	{ RC_OUT_OF_RANGE, "RC_OUT_OF_RANGE", "Generic out of range return code" },
	{ RC_LOCK_SET, "RC_LOCK_SET", "Function was not executed due to sending lock" },
	{ RC_NEW_TX_TEL, "RC_NEW_TX_TEL", "New telegram transmitted" },
	{ 0, nullptr, nullptr }
};

const char *CEnOceanESP3::GetFunctionReturnCodeLabel(const uint8_t RC)
{
	for (const _tFunctionReturnCodeTable *pTable = _functionReturnCodeTable; pTable->RC; pTable++)
		if (pTable->RC == RC)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanESP3::GetFunctionReturnCodeDescription(const uint8_t RC)
{
	for (const _tFunctionReturnCodeTable *pTable = _functionReturnCodeTable; pTable->RC; pTable++)
		if (pTable->RC == RC)
			return pTable->description;

	return ">>Unkown function return code... Please report!<<";
}
