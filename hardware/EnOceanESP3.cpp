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

//#define _DEBUG
#ifdef _DEBUG
// DEBUG: Enable logging of ESP3 packets management
//#define ENABLE_ESP3_PROTOCOL_DEBUG
// DEBUG: Enable logging of ESP3 devices management
#define ENABLE_ESP3_DEVICE_DEBUG
#endif

// Enable running ReadCallback test
// Enable running SP3 protocol tests emulating nodes with different EEP
// These tests are launched when ESP3 worker is started
//#define ENABLE_ESP3_TESTS
#ifdef ENABLE_ESP3_TESTS
//#define READCALLBACK_TESTS
//#define ESP3_TESTS_UNKNOWN_NODE
//#define ESP3_TESTS_1BS_D5_00_01
//#define ESP3_TESTS_4BS_A5_02_XX
//#define ESP3_TESTS_4BS_A5_04_01
//#define ESP3_TESTS_4BS_A5_06_01
//#define ESP3_TESTS_4BS_A5_07_01
//#define ESP3_TESTS_4BS_A5_07_02
//#define ESP3_TESTS_4BS_A5_12_00
//#define ESP3_TESTS_4BS_A5_12_01
//#define ESP3_TESTS_4BS_A5_12_02
//#define ESP3_TESTS_4BS_A5_12_03
//#define ESP3_TESTS_4BS_A5_20_01
//#define ESP3_TESTS_RPS_F6_01_01
//#define ESP3_TESTS_RPS_F6_02_01
//#define ESP3_TESTS_UTE_D2_01_12
//#define ESP3_TESTS_VLD_D2_01_12
#endif

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

// Lines from EO300I API header file

/**
 * @defgroup bitmasks Bitmasks for various fields.
 * There are two definitions for every bit mask. First, the bit mask itself
 * and also the number of necessary shifts.
 * @{
 */
/**
 * @defgroup status_rps Status of telegram (for RPS telegrams)
 * Bitmasks for the status-field, if ORG = RPS.
 * @{
 */
#define S_RPS_T21 0x20
#define S_RPS_T21_SHIFT 5
#define S_RPS_NU  0x10
#define S_RPS_NU_SHIFT 4
#define S_RPS_RPC 0x0F
#define S_RPS_RPC_SHIFT 0
/*@}*/

#define F60201_R1_MASK 0xE0
#define F60201_R1_SHIFT 5
#define F60201_EB_MASK 0x10
#define F60201_EB_SHIFT 4
#define F60201_R2_MASK 0x0E
#define F60201_R2_SHIFT 1
#define F60201_SA_MASK 0x01
#define F60201_SA_SHIFT 0

#define F60201_BUTTON_A1 0
#define F60201_BUTTON_A0 1
#define F60201_BUTTON_B1 2
#define F60201_BUTTON_B0 3

/**
 * @defgroup status_rpc Status of telegram (for 1BS, 4BS, HRC or 6DT telegrams)
 * Bitmasks for the status-field, if ORG = 1BS, 4BS, HRC or 6DT.
 * @{
 */
#define S_RPC 0x0F
#define S_RPC_SHIFT 0
/*@}*/

/**
 * @defgroup data3 Meaning of data_byte 3 (for RPS telegrams, NU = 1)
 * Bitmasks for the data_byte3-field, if ORG = RPS and NU = 1.
 * Specification can be found at:
 *      https://www.enocean.com/fileadmin/redaktion/enocean_alliance/pdf/EnOcean_Equipment_Profiles_EEP_V2.6.3_public.pdf
 * @{
 */

	//!	Rocker ID Mask
#define DB3_RPS_NU_RID 0xC0
#define DB3_RPS_NU_RID_SHIFT 6

//!	Button ID Mask
#define DB3_RPS_NU_BID 0xE0
#define DB3_RPS_NU_BID_SHIFT 5

//!	Up Down Mask
#define DB3_RPS_NU_UD  0x20
#define DB3_RPS_NU_UD_SHIFT  5

//!	Pressed Mask
#define DB3_RPS_NU_PR  0x10
#define DB3_RPS_NU_PR_SHIFT 4

//!	Second Rocker ID Mask
#define DB3_RPS_NU_SRID 0x0C
#define DB3_RPS_NU_SRID_SHIFT 2

//!	Second Button ID Mask
#define DB3_RPS_NU_SBID 0x0E
#define DB3_RPS_NU_SBID_SHIFT 1

//!	Second UpDown Mask
#define DB3_RPS_NU_SUD 0x02
#define DB3_RPS_NU_SUD_SHIFT 1

//!	Second Action Mask
#define DB3_RPS_NU_SA 0x01
#define DB3_RPS_NU_SA_SHIFT 0

/*@}*/

/**
 * @defgroup data3_1 Meaning of data_byte 3 (for RPS telegrams, NU = 0)
 * Bitmasks for the data_byte3-field, if ORG = RPS and NU = 0.
 * @{
 */
#define DB3_RPS_BUTTONS 0xE0
#define DB3_RPS_BUTTONS_SHIFT 5
#define DB3_RPS_PR 0x10
#define DB3_RPS_PR_SHIFT 4
/*@}*/

/**
 * @defgroup data0 Meaning of data_byte 0 (for 4BS telegrams)
 * Bitmasks for the data_byte0-field, if ORG = 4BS.
 * @{
 */
#define DB0_4BS_DI_3 0x08
#define DB0_4BS_DI_3_SHIFT 3
#define DB0_4BS_DI_2 0x04
#define DB0_4BS_DI_2_SHIFT 2
#define DB0_4BS_DI_1 0x02
#define DB0_4BS_DI_1_SHIFT 1
#define DB0_4BS_DI_0 0x01
#define DB0_4BS_DI_0_SHIFT 0
/*@}*/

/**
 * @defgroup data3_hrc Meaning of data_byte 3 (for HRC telegrams)
 * Bitmasks for the data_byte3-field, if ORG = HRC.
 * @{
 */
#define DB3_HRC_RID 0xC0
#define DB3_HRC_RID_SHIFT 6
#define DB3_HRC_UD  0x20
#define DB3_HRC_UD_SHIFT 5
#define DB3_HRC_PR  0x10
#define DB3_HRC_PR_SHIFT 4
#define DB3_HRC_SR  0x08
#define DB3_HRC_SR_SHIFT 3

CEnOceanESP3::CEnOceanESP3(const int ID, const std::string &devname, const int type)
{
	m_HwdID = ID;
	m_szSerialPort = devname;
	m_Type = type;
	m_id_base = 0;
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

		node.idx = atoi(sd[0].c_str());
		node.nodeID = sd[1];
		node.manufacturerID = atoi(sd[2].c_str());
		node.RORG = 0x00;
		node.func = (uint8_t)atoi(sd[3].c_str());
		node.type = (uint8_t)atoi(sd[4].c_str());
		node.generic = true;
/*
		Log(LOG_NORM, "LoadNodesFromDatabase: Idx %u Node %s %sEEP %02X-%02X-%02X (%s) Manufacturer %03X (%s)",
			node.idx, node.nodeID.c_str(),
			node.generic ? "Generic " : "", node.RORG, node.func, node.type, GetEEPLabel(node.RORG, node.func, node.type),
			node.manufacturerID, GetManufacturerName(node.manufacturerID));
*/
		m_nodes[GetINodeID(node.nodeID)] = node;
	}
}

CEnOceanESP3::NodeInfo* CEnOceanESP3::GetNodeInfo(const uint32_t iNodeID)
{
	auto node = m_nodes.find(iNodeID);

	if (node == m_nodes.end())
		return nullptr;

	return &(node->second);
}

CEnOceanESP3::NodeInfo* CEnOceanESP3::GetNodeInfo(const std::string &nodeID)
{
	return GetNodeInfo(GetINodeID(nodeID));
}

void CEnOceanESP3::TeachInNode(const std::string &nodeID, const uint16_t manID, const uint8_t RORG, const uint8_t func, const uint8_t type, const bool generic)
{
	Log(LOG_NORM, "Teach-in Node: HwdID %u Node %s Manufacturer %03X (%s) %sEEP %02X-%02X-%02X (%s)",
		m_HwdID, nodeID.c_str(), manID, GetManufacturerName(manID),
		generic ? "Generic " : "", RORG, func, type, GetEEPLabel(RORG, func, type));

	m_sql.safe_query("INSERT INTO EnoceanSensors (HardwareID, DeviceID, Manufacturer, Profile, Type) VALUES (%d,'%q',%d,%d,%d)",
		m_HwdID, nodeID.c_str(), manID, func, type);

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID FROM EnoceanSensors WHERE (HardwareID==%d) and (DeviceID=='%q')", m_HwdID, nodeID.c_str());
	if (result.empty())
		LoadNodesFromDatabase(); // Should never happend, since node was just created in the database!
	else
	{
		NodeInfo node;

		node.idx = (uint32_t)atoi(result[0][0].c_str());
		node.nodeID = nodeID;
		node.manufacturerID = manID;
		node.RORG = RORG;
		node.func = func;
		node.type = type;
		node.generic = generic;

		m_nodes[GetINodeID(node.nodeID)] = node;
	}
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

#ifdef ENABLE_ESP3_PROTOCOL_DEBUG
			Log(LOG_NORM, "Send: %s", DumpESP3Packet(sBytes).c_str());
#endif
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

#ifdef ESP3_TESTS_UNKNOWN_NODE
// Test Case : Unknown node
//  1BS Unknown node Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x08, 0x00, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x62 },
//  4BS Unknown node Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x00, RORG_4BS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x99 },
// Test Case : Unknown node
//  RPS Unknown node Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x00, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xC2 },
// Unknown node Test
	{ ESP3_SER_SYNC, 0x00, 0x09, 0x07, PACKET_RADIO_ERP1, 0x56, RORG_VLD, 0x04, 0x60, 0xE4, 0x00, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x19 },
#endif // ESP3_TESTS_UNKNOWN_NODE

#ifdef ESP3_TESTS_1BS_D5_00_01
// D5-00-01, Single Input Contact
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_1BS, 0x00, 0x01, RORG_1BS, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x83 },
//  Unidirectional Teach-in Test, node already known
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

#ifdef ESP3_TESTS_4BS_A5_04_01
// A5-04-01, Range 0C to +40C and 0% to 100%
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x10, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x5D },
// Test Case : Humidity Tests
//  Min Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x83 },
//  Max Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFA, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x7C },
//  Mid Humidity Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7D, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x7F },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFA, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE4 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7D, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x88 },
// Test Case : T-Sensor Enum Test
//  T-Sensor: not available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x83 },
//  T-Sensor: available
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x0A, 0x01, RORG_4BS, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xAC },
#endif // ESP3_TESTS_4BS_A5_04_01

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

#ifdef ESP3_TESTS_4BS_A5_20_01
// A5-20-01, Battery Powered Actuator (BI-DIR)
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x80, 0x08, 0x00, 0x80, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFE },
// Test Case : Current Value Tests
//  Min Current Value Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Max Current Value Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x64, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCF },
//  Mid Current Value Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x32, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x63 },
// Test Case : Service On Enum Test
//  Service On: on
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x80, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x06 },
// Test Case : Energy input enabled Enum Test
//  Energy input enabled: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x40, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x84 },
// Test Case : Energy Storage Enum Test
//  Energy Storage: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x20, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xC5 },
// Test Case : Battery capacity  Enum Test
//  Battery capacity : true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
// Test Case : Contact, cover open Enum Test
//  Contact, cover open: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x08, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB4 },
// Test Case : Failure temperature sensor, out off range Enum Test
//  Failure temperature sensor, out off range: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x04, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDD },
// Test Case : Detection, window open Enum Test
//  Detection, window open: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x02, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x6A },
// Test Case : Actuator obstructed Enum Test
//  Actuator obstructed: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x01, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB2 },
// Test Case : Temperature Tests
//  Min Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Max Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0xFF, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x37 },
//  Mid Temperature Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x7F, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xEE },
// Test Case : Valve position or Temperature Setpoint Tests
//  Min Valve position or Temperature Setpoint Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Max Valve position or Temperature Setpoint Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Mid Valve position or Temperature Setpoint Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
// Test Case : Temperature  from RCU Tests
//  Min Temperature  from RCU Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0xFF, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x97 },
//  Max Temperature  from RCU Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Mid Temperature  from RCU Test
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x7F, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x96 },
// Test Case : Run init sequence Enum Test
//  Run init sequence: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x80, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xDE },
// Test Case : Lift set Enum Test
//  Lift set: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x40, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE8 },
// Test Case : Valve open / maintenance Enum Test
//  Valve open / maintenance: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x20, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xF3 },
// Test Case : Valve closed Enum Test
//  Valve closed: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x10, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x7D },
// Test Case : Summer bit, Reduction of energy consumption Enum Test
//  Summer bit, Reduction of energy consumption: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x08, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x3A },
// Test Case : Set Point Selection Enum Test
//  Set Point Selection: Valve position (0-100%). Unit respond to controller.
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Set Point Selection: Temperature set point 0...40°C. Unit respond to room sensor and use internal PI loop.
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x04, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x9A },
// Test Case : Set point inverse Enum Test
//  Set point inverse: true
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x02, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xCA },
// Test Case : Select function Enum Test
//  Select function: RCU
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x00, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x07 },
//  Select function: service on
    { ESP3_SER_SYNC, 0x00, 0x0A, 0x07, PACKET_RADIO_ERP1, 0xEB, RORG_4BS, 0x00, 0x00, 0x01, 0x08, 0x01, RORG_4BS, 0x20, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xE2 },
#endif // ESP3_TESTS_4BS_A5_20_01

#ifdef ESP3_TESTS_RPS_F6_01_01
// A5-20-01, Switch Buttons, Push Button
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xE0 },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xE0 },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xE0 },
// Test Case : Button actions
// NB. following is interpreted as generic EEP F6-02-01
// RPS N-msg: Button pressed
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x9F },
// RPS N-msg: Button released
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x01, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0xE0 },
# endif // ESP3_TESTS_RPS_F6_01_01

#ifdef ESP3_TESTS_RPS_F6_02_01
// F6-02-01, Light and Blind Control - Application Style 1
// Test Case : Teach-in Test
//  Unidirectional Teach-in Test
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x53, 0x00, 0x09 },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x53, 0x00, 0x09 },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x50, 0x00, 0x7E },
	{ ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x20, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x53, 0x00, 0x09 },
// Test Case : Rocker 1st action
//  Button AI
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x47 },
//  Button A0
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x20, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xB9 },
//  Button BI
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x40, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xBC },
//  Button B0
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x60, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x42 },
// Test Case : Rocker 2nd action
//  Button AI 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x01, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xD3 },
//  Button A0 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x03, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xFC },
//  Button BI 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x05, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x8D },
//  Button B0 2nd action
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x07, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0xA2 },
// Test Case : Number of buttons pressed simultaneously
//  no button
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x47 },
//  3 or 4 buttons
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x60, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x42 },
// Test Case : Energy Bow Status 30
//  Pressed (status 30)
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
//  Released (status 30)
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x47 },
// Test Case : Energy Bow Status 20
//  Pressed (status 20)
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x10, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x38 },
//  Released (status 20)
    { ESP3_SER_SYNC, 0x00, 0x07, 0x07, PACKET_RADIO_ERP1, 0x7A, RORG_RPS, 0x00, 0x01, RORG_RPS, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x29, 0x00, 0x47 },
#endif // ESP3_TESTS_RPS_F6_02_01

#ifdef ESP3_TESTS_UTE_D2_01_12
// D2-01-12, , Slot-in module, dual channels, with external button control
// Test Case : Teach-in Test
//  Bi-directional teach-in or teach-out request from Node 00D20001, response expected
	{ ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0xA0, 0x02, 0x46, 0x00, 0x12, 0x01, RORG_VLD, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x2F },
	{ ESP3_SER_SYNC, 0x00, 0x0D, 0x07, PACKET_RADIO_ERP1, 0xFD, RORG_UTE, 0xA0, 0x02, 0x46, 0x00, 0x12, 0x01, RORG_VLD, 0x01, RORG_VLD, 0x01, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x2D, 0x00, 0x2F },
#endif // ESP3_TESTS_UTE_D2_01_12

#ifdef ESP3_TESTS_VLD_D2_01_12
// D2-01-12, , Slot-in module, dual channels, with external button control
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
};
#endif

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

	m_RPS_teachin_nodeID = "";

	m_receivestate = ERS_SYNCBYTE;
	setReadCallback([this](auto d, auto l) { ReadCallback(d, l); });

	sOnConnected(this);

#ifdef ENABLE_ESP3_TESTS
	Log(LOG_STATUS, "------------ ESP3 tests begin ---------------------------");
	m_sql.AllowNewHardwareTimer(1);

	for (const auto &itt : ESP3TestsCases)
		ReadCallback((const char *)itt.data(), itt.size());

	m_sql.AllowNewHardwareTimer(0);
	Log(LOG_STATUS, "------------ ESP3 tests end -----------------------------");
#endif

	uint8_t cmd;

	// Request BASE_ID
	m_id_base = 0;
	cmd = CO_RD_IDBASE;
	SendESP3PacketQueued(PACKET_COMMON_COMMAND, &cmd, 1, nullptr, 0);
	Log(LOG_NORM, "Request base ID");

	// Request base version
	m_wait_version_base = true;
	cmd = CO_RD_VERSION;
	SendESP3PacketQueued(PACKET_COMMON_COMMAND, &cmd, 1, nullptr, 0);
	Log(LOG_NORM, "Request base version");

	return true;
}

std::string CEnOceanESP3::DumpESP3Packet(uint8_t packettype, uint8_t *data, uint16_t datalen, uint8_t *optdata, uint8_t optdatalen)
{
	std::stringstream sstr;

	sstr << GetPacketTypeLabel(packettype);

	sstr << " DATA (" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t)datalen << ")";
	for (int i = 0; i < datalen; i++)
		if (i == 0 && packettype == PACKET_RADIO_ERP1)
			sstr << " " << GetRORGLabel((uint32_t)data[i]);
		else
			sstr << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t)data[i];

	if (optdatalen > 0)
	{
		sstr << " OPTDATA (" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t)optdatalen << ")";

		for (int i = 0; i < optdatalen; i++)
			sstr << " " << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (uint32_t)optdata[i];
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

	uint8_t defaulERP1optdata[] = { 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
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
	if (m_id_base == 0)
		return false;

	if (!isOpen())
		return false;

	RBUF *tsen = (RBUF *) pdata;

	if (tsen->LIGHTING2.packettype != pTypeLighting2)
		return false; // Only allowed to control switches

	uint32_t iNodeID = GetINodeID(tsen->LIGHTING2.id1, tsen->LIGHTING2.id2, tsen->LIGHTING2.id3, tsen->LIGHTING2.id4);
	std::string nodeID = GetNodeID(iNodeID);

	if (iNodeID <= m_id_base || iNodeID > (m_id_base + 128))
	{
		Log(LOG_ERROR, "Node %s can not be used as a switch", nodeID.c_str());
		Log(LOG_ERROR, "Create a virtual switch associated with HwdID %u", m_HwdID);
		return false;
	}
	if (tsen->LIGHTING2.unitcode >= 10)
	{
		Log(LOG_ERROR, "Node %s, double press not supported", nodeID.c_str());
		return false;
	}

	uint8_t RockerID = tsen->LIGHTING2.unitcode - 1;
	uint8_t Pressed = 1;
	bool bIsDimmer = false;
	uint8_t LastLevel = 0;

	// Find out if this is a Dimmer switch, because they are threaded differently

	std::string deviceID = (nodeID[0] == '0') ? nodeID.substr(1, nodeID.length() - 1) : nodeID;
	std::vector<std::vector<std::string>> result;

	result = m_sql.safe_query("SELECT SwitchType, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
		m_HwdID, deviceID.c_str(), (int) tsen->LIGHTING2.unitcode);
	if (!result.empty())
	{
		_eSwitchType switchtype = (_eSwitchType)atoi(result[0][0].c_str());
		if (switchtype == STYPE_Dimmer)
			bIsDimmer = true;
		LastLevel = (uint8_t)atoi(result[0][1].c_str());
	}

	uint8_t iLevel = tsen->LIGHTING2.level;
	int cmnd = tsen->LIGHTING2.cmnd;
	int orgcmd = cmnd;
	if (tsen->LIGHTING2.level == 0 && !bIsDimmer)
		cmnd = light2_sOff;
	else
	{
		if (cmnd == light2_sOn)
			iLevel = LastLevel;
		else
		{ // Scale to 0 - 100 %
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

	if (bIsDimmer)
	{ // A5-38-02, On/Off switch with dimming capability
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
			uint8_t UpDown = (cmnd != light2_sOff) && (cmnd != light2_sGroupOff);

			buf[1] = (RockerID << DB3_RPS_NU_RID_SHIFT) | (UpDown << DB3_RPS_NU_UD_SHIFT) | (Pressed << DB3_RPS_NU_PR_SHIFT);
			buf[9] = 0x30;

			SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);

			// Next command is send a bit later (button release)
			buf[1] = 0x00;
			buf[9] = 0x20;

			SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
		}
		else
		{ // Send dim value
			buf[1] = 0x02;
			buf[2] = iLevel;
			buf[3] = 0x01; // Very fast dimming

			if ((iLevel == 0) || (orgcmd == light2_sOff))
				buf[4] = 0x08; // Dim Off
			else
				buf[4] = 0x09; // Dim On

			SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
		}
	}
	else
	{ // F6-02-01, On/Off switch without dimming capability
		uint8_t UpDown = (orgcmd != light2_sOff) && (orgcmd != light2_sGroupOff);

		buf[0] = RORG_RPS;

		switch (RockerID)
		{
			case 0: // Button A
				if (UpDown)
					buf[1] = F60201_BUTTON_A1 << F60201_R1_SHIFT;
				else
					buf[1] = F60201_BUTTON_A0 << F60201_R1_SHIFT;
				break;

			case 1: // Button B
				if (UpDown)
					buf[1] = F60201_BUTTON_B1 << F60201_R1_SHIFT;
				else
					buf[1] = F60201_BUTTON_B0 << F60201_R1_SHIFT;
				break;

			default:
				return false; // Not supported
		}

		buf[1] |= F60201_EB_MASK; // Button is pressed

		buf[2] = tsen->LIGHTING2.id1; // Sender ID
		buf[3] = tsen->LIGHTING2.id2;
		buf[4] = tsen->LIGHTING2.id3;
		buf[5] = tsen->LIGHTING2.id4;

		buf[6] = S_RPS_T21 | S_RPS_NU; // Press button

		SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 7, nullptr, 0);

		// Next command is send a bit later (button release)
		buf[1] = 0x00; // No button press
		buf[6] = S_RPS_T21; // Release button

		SendESP3PacketQueued(PACKET_RADIO_ERP1, buf, 7, nullptr, 0);
	}

	return true;
}

void CEnOceanESP3::SendDimmerTeachIn(const char *pdata, const unsigned char length)
{
	if (m_id_base == 0)
		return;

	if (!isOpen())
		return;

	RBUF *tsen = (RBUF *) pdata;

	if (tsen->LIGHTING2.packettype != pTypeLighting2)
		return; // Only allowed to control switches

	uint32_t iNodeID = GetINodeID(tsen->LIGHTING2.id1, tsen->LIGHTING2.id2, tsen->LIGHTING2.id3, tsen->LIGHTING2.id4);
	std::string nodeID = GetNodeID(iNodeID);

	if (iNodeID <= m_id_base || iNodeID > (m_id_base + 128))
	{
		Log(LOG_ERROR, "Node %s can not be used as a switch", nodeID.c_str());
		Log(LOG_ERROR, "Create a virtual switch associated with HwdID %u", m_HwdID);
		return;
	}
	if (tsen->LIGHTING2.unitcode >= 10)
	{
		Log(LOG_ERROR, "Node %s, double not supported", nodeID.c_str());
		return;
	}
	Log(LOG_NORM, "4BS teach-in request from Node %s (variation 3 : bi-directional)", nodeID.c_str());

	uint8_t buf[10];

	// TODO : recheck following values

	buf[0] = RORG_4BS;
	buf[1] = 0x02;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = 0x00; // DB0.3 = 0 -> teach in
	buf[5] = tsen->LIGHTING2.id1; // Sender ID
	buf[6] = tsen->LIGHTING2.id2;
	buf[7] = tsen->LIGHTING2.id3;
	buf[8] = tsen->LIGHTING2.id4;
	buf[9] = 0x30; // Status

	SendESP3Packet(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
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
#ifdef ENABLE_ESP3_PROTOCOL_DEBUG
	Log(LOG_NORM, "Read: %s", DumpESP3Packet(packettype, data, datalen, optdata, optdatalen).c_str());
#endif

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
				m_id_base = GetINodeID(data[1], data[2], data[3], data[4]);
				Log(LOG_NORM, "HwdID %d ID_Base %08X", m_HwdID, m_id_base);
				return;
			}
			if (m_wait_version_base && datalen == 33)
			{ // Base version Information
				m_wait_version_base = false;
				Log(LOG_NORM,
					 "HwdID %d Version Info: App %02X.%02X.%02X.%02X API %02X.%02X.%02X.%02X ChipID %02X.%02X.%02X.%02X ChipVersion %02X.%02X.%02X.%02X Description '%s'",
					 m_HwdID, data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15], data[16], (const char *)data + 17);
				return;
			}
			Log(LOG_NORM, "HwdID %d, received response (%s)", m_HwdID, GetReturnCodeLabel(return_code));
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

			uint32_t dstID = GetINodeID(optdata[1], optdata[2], optdata[3], optdata[4]);

			// Ignore telegrams addressed to another device
			if (dstID != ERP1_BROADCAST_TRANSMISSION && dstID != m_id_base)
			{
#ifdef ENABLE_ESP3_PROTOCOL_DEBUG
				Log(LOG_NORM, "HwdID %d, ignore addressed telegram sent to %08X", m_id_base, dstID);
#endif
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
				rssi = static_cast<int>((dBm + 100) / 5);

#ifdef ENABLE_ESP3_PROTOCOL_DEBUG
			if (dstID == ERP1_BROADCAST_TRANSMISSION)
				Log(LOG_NORM, "Broadcast RSSI %idBm (%u/11)", dBm, rssi);
			else
				Log(LOG_NORM, "Dst %08X RSSI %idBm (%u/11)", dstID, dBm, rssi);
#endif
		}
	}
	// Parse data

	uint8_t RORG = data[0];

	uint8_t ID_BYTE3 = data[datalen - 5];
	uint8_t ID_BYTE2 = data[datalen - 4];
	uint8_t ID_BYTE1 = data[datalen - 3];
	uint8_t ID_BYTE0 = data[datalen - 2];
	uint32_t iSenderID = GetINodeID(ID_BYTE3, ID_BYTE2, ID_BYTE1, ID_BYTE0);
	std::string senderID = GetNodeID(iSenderID);
	NodeInfo* pNode = GetNodeInfo(iSenderID);

	uint8_t STATUS = data[datalen - 1];

	switch (RORG)
	{
		case RORG_1BS:
			{ // 1BS telegram, D5-xx-xx, 1 Byte Communication
				uint8_t DATA_BYTE0 = data[1];
				uint8_t LRN = bitrange(DATA_BYTE0, 3, 0x01);

				if (LRN == 0)
				{ 	// 1BS teach-in
					Log(LOG_NORM, "1BS teach-in request from Node %s", senderID.c_str());

					if (pNode != nullptr)
					{
						Log(LOG_NORM, "Node %s already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
							senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{
						Log(LOG_NORM, "Unknown Node %s, please allow accepting new hardware and proceed to teach-in", senderID.c_str());
						return;
					}
					// Node not found and learn mode enabled : add it to the database
					// Generic node : Unknown manufacturer, Switch Buttons, Push Button

					uint8_t node_func = 0x00;
					uint8_t node_type = 0x01;
					
					Log(LOG_NORM, "Creating Node %s with generic EEP %02X-%02X-%02X (%s)",
						senderID.c_str(), RORG_1BS, node_func, node_type, GetEEPLabel(RORG_1BS, node_func, node_type));
					Log(LOG_NORM, "Please adjust by hand if need be");

					TeachInNode(senderID, UNKNOWN_MANUFACTURER, RORG_1BS, node_func, node_type, true);
					return;
				}
				// 1BS data

				if (pNode == nullptr)
				{
					Log(LOG_NORM, "1BS msg: Unknown Node %s, please proceed to teach-in", senderID.c_str());
					return;
				}
				if (pNode->func == 0x00 && pNode->type == 0x01)
				{ // D5-00-01, Contacts and Switches, Single Input Contact
					uint8_t CO = bitrange(DATA_BYTE0, 0, 0x01);

					Log(LOG_NORM, "1BS msg: Node %s Contact %s", senderID.c_str(), CO ? "Off" : "On");

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
					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
					return;
				}
				// Should never happend, since D5-00-01 is the only 1BS EEP
				Log(LOG_ERROR, "1BS msg: Node %s, EEP %02X-%02X-%02X (%s) not supported",
					senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_4BS:
			{ // 4BS telegram, A5-xx-xx, 4 bytes communication
				uint8_t DATA_BYTE3 = data[1];
				uint8_t DATA_BYTE2 = data[2];
				uint8_t DATA_BYTE1 = data[3];
				uint8_t DATA_BYTE0 = data[4];

				uint8_t LRNB = bitrange(DATA_BYTE0, 3, 0x01);
				if (LRNB == 0)
				{ // 4BS teach-in
					uint8_t LRN_TYPE = bitrange(DATA_BYTE0, 7, 0x01);

					Log(LOG_NORM, "4BS teach-in request from Node %s (variation %s EEP)",
						senderID.c_str(), (LRN_TYPE == 0) ? "1: without" : "2 : with");

					if (pNode != nullptr)
					{
						Log(LOG_NORM, "Node %s already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
							senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{
						Log(LOG_NORM, "Unknown Node %s, please allow accepting new hardware and proceed to teach-in", senderID.c_str());
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

						Log(LOG_NORM, "Creating Node %s with generic EEP %02X-%02X-%02X (%s)",
							senderID.c_str(), RORG_4BS, node_func, node_type, GetEEPLabel(RORG_4BS, node_func, node_type));
						Log(LOG_NORM, "Please adjust by hand if need be");
					}
					else
					{ // 4BS teach-in variation 2, with Manufacturer ID and EEP
						node_manID = (bitrange(DATA_BYTE2, 0, 0x07) << 8) | DATA_BYTE1;
						node_func = bitrange(DATA_BYTE3, 2, 0x3F);
						node_type = (bitrange(DATA_BYTE3, 0, 0x03) << 5) | bitrange(DATA_BYTE2, 3, 0x1F);

						Log(LOG_NORM, "Creating Node %s Manufacturer 0x%03X (%s) EEP %02X-%02X-%02X (%s)",
							senderID.c_str(), node_manID, GetManufacturerName(node_manID),
							RORG_4BS, node_func, node_type, GetEEPLabel(RORG_4BS, node_func, node_type));
					}

					TeachInNode(senderID, node_manID, RORG_4BS, node_func, node_type, (LRN_TYPE == 0));

					// EEP requiring 4BS teach-in variation 3 response
					// A5-20-XX, HVAC Components
					// A5-38-08, Central Command Gateway
					// A5-3F-00, Radio Link Test
					if (node_func == 0x20
						|| (node_func == 0x38 && node_type == 0x08)
						|| (node_func == 0x3F && node_type == 0x00))
					{
						Log(LOG_NORM, "4BS teach-in request from Node %s (variation 3 : bi-directional)", senderID.c_str());

						uint8_t buf[10];

						buf[0] = RORG_4BS;
						buf[1] = 0x02;		// DB0.0
						buf[2] = 0x00;		// DB0.1
						buf[3] = 0x00;		// DB0.2
						buf[4] = 0xF0;		// DB0.3 -> teach-in response
						buf[5] = ID_BYTE3; 	// Send to teached-in node id
						buf[6] = ID_BYTE2;
						buf[7] = ID_BYTE1;
						buf[8] = ID_BYTE0;
						buf[9] = 0x00;		// Status
						SendESP3Packet(PACKET_RADIO_ERP1, buf, 10, nullptr, 0);
					}
					return;
				}
				// 4BS data

				if (pNode == nullptr)
				{
					Log(LOG_NORM, "4BS msg: Unknown Node %s, please proceed to teach-in", senderID.c_str());
					return;
				}
				Log(LOG_NORM, "4BS msg: Node %s EEP %02X-%02X-%02X (%s) Data %02X %02X %02X %02X",
					senderID.c_str(),
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
						tsen.TEMP.subtype = sTypeTEMP10;
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, TMP %.1F°C", senderID.c_str(), TMP);
#endif

						sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, nullptr, -1, nullptr);
						return;
					}
				}
				if (pNode->func == 0x04 && pNode->type <= 0x03)
				{ // A5-04-01..03, Temperature and Humidity Sensor
					// TOTO : implement A5-04-04

					float ScaleMax = 0;
					float ScaleMin = 0;
					if (pNode->type == 0x01) { ScaleMin = 0; ScaleMax = 40; }
					else if (pNode->type == 0x02) { ScaleMin = -20; ScaleMax = 60; }
					else if (pNode->type == 0x03) { ScaleMin = -20; ScaleMax = 60; } //10bit?

					float temp = GetDeviceValue(DATA_BYTE1, 0, 250, ScaleMin, ScaleMax);
					float hum = GetDeviceValue(DATA_BYTE2, 0, 255, 0, 100);
					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
					tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
					tsen.TEMP_HUM.subtype = sTypeTH5;
					tsen.TEMP_HUM.seqnbr = 0;
					tsen.TEMP_HUM.id1 = ID_BYTE2;
					tsen.TEMP_HUM.id2 = ID_BYTE1;
					tsen.TEMP_HUM.tempsign = (temp >= 0) ? 0 : 1;
					int at10 = round(std::abs(temp * 10.0F));
					tsen.TEMP_HUM.temperatureh = (BYTE) (at10 / 256);
					at10 -= (tsen.TEMP_HUM.temperatureh * 256);
					tsen.TEMP_HUM.temperaturel = (BYTE) at10;
					tsen.TEMP_HUM.humidity = (BYTE) hum;
					tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);
					tsen.TEMP_HUM.battery_level = 9; // OK, TODO: Should be 255 (unknown battery level) ?
					tsen.TEMP_HUM.rssi = rssi;
					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP_HUM, nullptr, -1, nullptr);
					return;
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, SVC %.1FmV", senderID.c_str(), SVC);
#endif

						sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, nullptr, 255, nullptr);
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, RS %s ILL %.1Flx", senderID.c_str(), (RS == 0) ? "ILL1" : "ILL2", ILL);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &lmeter, nullptr, 255, nullptr);
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, SVC %.1FmV", senderID.c_str(), SVC);
#endif

						sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, nullptr, 255, nullptr);
					}
#ifdef ENABLE_ESP3_DEVICE_DEBUG
					else
						Log(LOG_NORM,"4BS msg: Node %s, SVC not supported", senderID.c_str());
#endif

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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, PIRS %u (%s)",
							senderID.c_str(), PIRS, (PIRS >= 128) ? "On" : "Off");
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, SVC %.1FmV", senderID.c_str(), SVC);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, nullptr, 255, nullptr);

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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM,"4BS msg: Node %s, PIRS %u (%s)",
							senderID.c_str(), PIRS, (PIRS == 1) ? "Motion detected" : "Uncertain of occupancy status");
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x07 && pNode->type == 0x03)
				{ // A5-07-03, Occupancy sensor with Supply voltage monitor and 10-bit illumination measurement
					RBUF tsen;

					float voltage = GetDeviceValue(DATA_BYTE3, 0, 250, 0, 5000.0F); // Convert V to mV

					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
					tsen.RFXSENSOR.packettype = pTypeRFXSensor;
					tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
					tsen.RFXSENSOR.seqnbr = 0;
					tsen.RFXSENSOR.id = ID_BYTE1;
					// WARNING
					// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
					// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
					tsen.RFXSENSOR.filler = ID_BYTE0 & 0x0F;
					tsen.RFXSENSOR.rssi = (ID_BYTE0 & 0xF0) >> 4;
					tsen.RFXSENSOR.msg1 = (BYTE) (voltage / 256);
					tsen.RFXSENSOR.msg2 = (BYTE) (voltage - (tsen.RFXSENSOR.msg1 * 256));
					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, nullptr, 255, nullptr);

					uint16_t lux = (DATA_BYTE2 << 2) | (DATA_BYTE1>>6);
					if (lux > 1000)
						lux = 1000;

					_tLightMeter lmeter;
					lmeter.id1 = (BYTE) ID_BYTE3;
					lmeter.id2 = (BYTE) ID_BYTE2;
					lmeter.id3 = (BYTE) ID_BYTE1;
					lmeter.id4 = (BYTE) ID_BYTE0;
					lmeter.dunit = 1;
					lmeter.fLux = (float)lux;
					sDecodeRXMessage(this, (const unsigned char *) &lmeter, nullptr, 255, nullptr);

					bool bPIROn = (DATA_BYTE0 & 0x80)!=0;

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
					tsen.LIGHTING2.cmnd = (bPIROn) ? light2_sOn : light2_sOff;
					tsen.LIGHTING2.rssi = rssi;
					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x09 && pNode->type == 0x04)
				{ // A5-09-04, CO2 Gas Sensor with Temp and Humidity
					// DB3 = Humidity in 0.5% steps, 0...200 -> 0...100% RH (0x51 = 40%)
					// DB2 = CO2 concentration in 10 ppm steps, 0...255 -> 0...2550 ppm (0x39 = 570 ppm)
					// DB1 = Temperature in 0.2C steps, 0...255 -> 0...51 C (0x7B = 24.6 C)
					// DB0 = flags (DB0.3: 1=data, 0=teach-in; DB0.2: 1=Hum Sensor available, 0=no Hum; DB0.1: 1=Temp sensor available, 0=No Temp; DB0.0 not used)
					// mBuffer[15] is RSSI as -dBm (ie value of 100 means "-100 dBm"), but RssiLevel is in the range 0...11 (or reported as 12 if not known)
					// Battery level is not reported by node, so use fixed value of 9 as per other sensor functions

					// TODO: Check sensor availability flags and only report humidity and/or temp if available.
					// TODO: Report battery level as 255 (unknown battery level) ?

					float temp = GetDeviceValue(DATA_BYTE1, 0, 255, 0, 51);
					float hum = GetDeviceValue(DATA_BYTE3, 0, 200, 0, 100);
					int co2 = (int) GetDeviceValue(DATA_BYTE2, 0, 255, 0, 2550);
					uint32_t shortID = (ID_BYTE2 << 8) + ID_BYTE1;

					// Report battery level as 9
					SendTempHumSensor(shortID, 9, temp, round(hum), "GasSensor.04", rssi);
					SendAirQualitySensor((shortID & 0xFF00) >> 8, shortID & 0xFF, 9, co2, "GasSensor.04");
					return;
				}
				if (pNode->func == 0x10 && pNode->type <= 0x0D)
				{ // A5-10-01..OD, RoomOperatingPanel
					// Eltako FTR55D, FTR55H, Thermokon SR04 *, Thanos SR *, [untested]
					if (pNode->manufacturerID != ELTAKO)
					{ // General case for A5-10-01..OD
						// DATA_BYTE3 is the fan speed
						// DATA_BYTE2 is the setpoint where 0x00 = min ... 0xFF = max
						// DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
						// DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch

						uint8_t fspeed = 3;
						if (DATA_BYTE3 >= 145)
							fspeed = 2;
						else if (DATA_BYTE3 >= 165)
							fspeed = 1;
						else if (DATA_BYTE3 >= 190)
							fspeed = 0;
						else if (DATA_BYTE3 >= 210)
							fspeed = -1; // Auto

						// uint8_t iswitch = DATA_BYTE0 & 1;
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

						// float setpointTemp = GetDeviceValue(DATA_BYTE2, 0, 255, 0, 40);
					}
					float temp = GetDeviceValue(DATA_BYTE1, 0, 255, 40, 0);

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
					tsen.TEMP.packettype = pTypeTEMP;
					tsen.TEMP.subtype = sTypeTEMP10;
					tsen.TEMP.seqnbr = 0;
					tsen.TEMP.id1 = ID_BYTE2;
					tsen.TEMP.id2 = ID_BYTE1;
					// WARNING
					// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
					// decode_Temp assumes battery_level = 255 (Unknown) & rssi = 12 (Not available)
					tsen.TEMP.battery_level = ID_BYTE0 & 0x0F;
					tsen.TEMP.rssi = (ID_BYTE0 & 0xF0) >> 4;
					tsen.TEMP.tempsign = (temp >= 0) ? 0 : 1;
					int at10 = round(std::abs(temp * 10.0F));
					tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
					at10 -= tsen.TEMP.temperatureh * 256;
					tsen.TEMP.temperaturel = (BYTE) at10;
					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, nullptr, -1, nullptr);
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x00)
				{ // A5-12-00, Automated Meter Reading, Counter
					uint8_t CH = bitrange(DATA_BYTE0, 4, 0x0F); // Channel number
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 : cumulative count, 1: current value / s
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM,"4BS msg: Node %s, CH %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID.c_str(), CH, DT, DIV, scaleMax, MR);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, nullptr, 255, nullptr);
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x01)
				{ // A5-12-01, Automated Meter Reading, Electricity
					uint32_t MR = (DATA_BYTE3 << 16) | (DATA_BYTE2 << 8) | DATA_BYTE1;
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 : cumulative count (kWh), 1: current value (W)
					uint8_t DIV = bitrange(DATA_BYTE0, 0, 0x03);
					float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));

					_tUsageMeter umeter;
					umeter.id1 = (BYTE) ID_BYTE3;
					umeter.id2 = (BYTE) ID_BYTE2;
					umeter.id3 = (BYTE) ID_BYTE1;
					umeter.id4 = (BYTE) ID_BYTE0;
					umeter.dunit = 1;
					umeter.fusage = GetDeviceValue(MR, 0, 16777215, 0.0F, scaleMax);

#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM,"4BS msg: Node %s, TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID.c_str(), TI, DT, DIV, scaleMax, MR);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &umeter, nullptr, 255, nullptr);
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x02)
				{ // A5-12-02, Automated Meter Reading, Gas
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 : cumulative count (kWh), 1: current value (W)
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM,"4BS msg: Node %s, TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID.c_str(), TI, DT, DIV, scaleMax, MR);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, nullptr, 255, nullptr);
					return;
				}
				if (pNode->func == 0x12 && pNode->type == 0x03)
				{ // A5-12-03, Automated Meter Reading, Water
					uint8_t TI = bitrange(DATA_BYTE0, 4, 0x0F); // Tariff info
					uint8_t DT = bitrange(DATA_BYTE0, 2, 0x01); // 0 : cumulative count (kWh), 1: current value (W)
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

#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM,"4BS msg: Node %s, TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
						senderID.c_str(), TI, DT, DIV, scaleMax, MR);
#endif

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, nullptr, 255, nullptr);
					return;
				}
				Log(LOG_ERROR, "4BS msg: Node %s, EEP %02X-%02X-%02X (%s) not supported",
					senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_RPS:
			{ // RPS telegram, F6-xx-xx, Repeated Switch Communication
				uint8_t DATA = data[1];

				uint8_t T21 = bitrange(STATUS, 5, 0x01); // 0=PTM switch module of type 1 (PTM1xx), 1=PTM switch module of type 2 (PTM2xx)
				uint8_t NU = bitrange(STATUS, 4, 0x01); // 0=U-message (Unassigned), 1=N-message (Normal)

				if (pNode == nullptr)
				{ // Node not found
					if (!m_sql.m_bAcceptNewHardware)
					{ // Node not found, but learn mode disabled
						Log(LOG_NORM, "RPS teach-in request from Node %s", senderID.c_str());
						Log(LOG_NORM, "Unknown Node %s, please allow accepting new hardware and proceed to teach-in", senderID.c_str());
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
						Log(LOG_NORM, "RPS teach-in request #%u from Node %s", m_RPS_teachin_count, senderID.c_str());
						Log(LOG_NORM, "RPS teach-in requires %u actions in less than %ldms to complete", TEACH_NB_REQUESTS, TEACH_MAX_DELAY);
						return;
					}
					if (DATA != m_RPS_teachin_DATA || STATUS != m_RPS_teachin_STATUS)
					{ // Same node ID, but data or status differ => ignore telegram
						Log(LOG_NORM, "RPS teach-in request from Node %s (ignored telegram)", senderID.c_str());
						return;
					}
					time_t now = GetClockTicks();
					if (m_RPS_teachin_timer != 0 && now > (m_RPS_teachin_timer + TEACH_MAX_DELAY))
					{ // Max delay expired => teach-in request ignored
						m_RPS_teachin_nodeID = "";
						Log(LOG_NORM, "RPS teach-in request from Node %s (timed out)", senderID.c_str());
						return;
					}
					if (++m_RPS_teachin_count < TEACH_NB_REQUESTS)
					{ // TEACH_NB_REQUESTS not reached => continue to wait
						Log(LOG_NORM, "RPS teach-in request #%u from Node %s", m_RPS_teachin_count, senderID.c_str());
						return;
					}
					// Received 3 identical telegrams from the node within delay
					// RPS teachin

					m_RPS_teachin_nodeID = "";

					Log(LOG_NORM, "RPS teach-in request #%u from Node %s accepted", m_RPS_teachin_count, senderID.c_str());

					// EEP F6-01-01, Rocker Switch, 2 Rocker
					uint8_t node_func = 0x02;
					uint8_t node_type = 0x01;

					Log(LOG_NORM, "Creating Node %s with generic EEP %02X-%02X-%02X (%s)",
						senderID.c_str(), RORG_RPS, node_func, node_type, GetEEPLabel(RORG_RPS, node_func, node_type));
					Log(LOG_NORM, "Please adjust by hand if need be");

					TeachInNode(senderID, UNKNOWN_MANUFACTURER, RORG_RPS, node_func, node_type, true);
					return;
				}
				// RPS data

				Log(LOG_NORM, "RPS %s: Node %s EEP %02X-%02X_%02X (%s) DATA %02X (%s)",
					(NU == 0) ? "U-msg" : "N-msg",
					senderID.c_str(),
					pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type),
					DATA, (T21 == 0) ? "PTM1xx" : "PTM2xx");

				// EEP D2-01-XX, Electronic Switches and Dimmers with Local Control
				// D2-01-0D, Micro smart plug, single channel, with external button control
				// D2-01-0E, Micro smart plug, single channel, with external button control
				// D2-01-0F, Slot-in module, single channel, with external button control
				// D2-01-12, Slot-in module, dual channels, with external button control
				// D2-01-15, D2-01-16, D2-01-17
				// These nodes send RPS telegrams whenever the external button control is used
				// Ignore these RPS telegrams : device status will be reported using VLD datagram

				if (pNode->RORG == RORG_VLD
					|| (pNode->RORG == 0x00 && pNode->func == 0x01
						&& (pNode->type == 0x0D || pNode->type == 0x0E
							|| pNode->type == 0x0F || pNode->type == 0x12
							|| pNode->type == 0x15 || pNode->type == 0x16
							|| pNode->type == 0x17)))
				{
#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM,"Node %s, VLD device button press ignored", senderID.c_str());
#endif
					return;
				}
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
					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
					return;
				}
				if (pNode->func == 0x02 && (pNode->type == 0x01 ||pNode->type == 0x02))
				{ // F6-02-01, F6-02-02
					// F6-02-01, Rocker switch, 2 Rocker (Light and blind control, Application style 1)
					// F6-02-02, Rocker switch, 2 Rocker (Light and blind control, Application style 2)

					bool useButtonIDs = true; // Whether we use the ButtonID reporting with ON/OFF

					if (STATUS & S_RPS_NU)
					{
						uint8_t DATA_BYTE3 = data[1];

						// NU == 1, N-Message
						uint8_t ButtonID = (DATA_BYTE3 & DB3_RPS_NU_BID) >> DB3_RPS_NU_BID_SHIFT;
						uint8_t RockerID = (DATA_BYTE3 & DB3_RPS_NU_RID) >> DB3_RPS_NU_RID_SHIFT;
						uint8_t UpDown = (DATA_BYTE3 & DB3_RPS_NU_UD)  >> DB3_RPS_NU_UD_SHIFT;
						uint8_t Pressed = (DATA_BYTE3 & DB3_RPS_NU_PR) >> DB3_RPS_NU_PR_SHIFT;

						uint8_t SecondButtonID = (DATA_BYTE3 & DB3_RPS_NU_SBID) >> DB3_RPS_NU_SBID_SHIFT;
						uint8_t SecondRockerID = (DATA_BYTE3 & DB3_RPS_NU_SRID) >> DB3_RPS_NU_SRID_SHIFT;
						uint8_t SecondUpDown = (DATA_BYTE3 & DB3_RPS_NU_SUD)>>DB3_RPS_NU_SUD_SHIFT;
						uint8_t SecondAction = (DATA_BYTE3 & DB3_RPS_NU_SA)>>DB3_RPS_NU_SA_SHIFT;

#ifdef ENABLE_ESP3_DEVICE_DEBUG
						Log(LOG_NORM, "RPS N-Message: Node %s RockerID: %i ButtonID: %i Pressed: %i UD: %i Second Rocker ID: %i SecondButtonID: %i SUD: %i Second Action: %i",
							senderID.c_str(),
							RockerID, ButtonID, UpDown, Pressed,
							SecondRockerID, SecondButtonID, SecondUpDown, SecondAction);
#endif

						if (Pressed == 1)
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
							if (SecondAction==0)
							{
								if (useButtonIDs)
								{ // Left/Right Pressed
									tsen.LIGHTING2.unitcode = ButtonID + 1;
									tsen.LIGHTING2.cmnd     = light2_sOn; // Button is pressed, so we don't get an OFF message here
								}
								else
								{ // Left/Right Up/Down
									tsen.LIGHTING2.unitcode = RockerID + 1;
									tsen.LIGHTING2.cmnd     = (UpDown == 1) ? light2_sOn : light2_sOff;
								}
							}
							else
							{
								if (useButtonIDs)
								{ // Left+Right Pressed
									tsen.LIGHTING2.unitcode = ButtonID + 10;
									tsen.LIGHTING2.cmnd     = light2_sOn;  // Button is pressed, so we don't get an OFF message here
								}
								else
								{ // Left+Right Up/Down
									tsen.LIGHTING2.unitcode = SecondRockerID + 10;
									tsen.LIGHTING2.cmnd     = (SecondUpDown == 1) ? light2_sOn : light2_sOff;
								}
							}
							tsen.LIGHTING2.rssi = rssi;

#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s UnitID: %02X cmd: %02X ",
								senderID.c_str(), tsen.LIGHTING2.unitcode, tsen.LIGHTING2.cmnd);
#endif

							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
						}
					}
					else
					{ // RPS U-Message
						// Release message of any button pressed before

						// TODO : check if following if is necessary or not

						if (T21 == 1 && NU == 0)
						{
							uint8_t DATA_BYTE3 = data[1];

							uint8_t ButtonID = (DATA_BYTE3 & DB3_RPS_BUTTONS) >> DB3_RPS_BUTTONS_SHIFT;
							uint8_t Pressed = (DATA_BYTE3 & DB3_RPS_PR) >> DB3_RPS_PR_SHIFT;
							uint8_t UpDown = !((DATA_BYTE3 == 0xD0) || (DATA_BYTE3 == 0xF0));

#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS T21-msg: Node %s ButtonID: %i Pressed: %i UD: %i",
								senderID.c_str(), ButtonID, Pressed, UpDown);
#endif

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

							if (useButtonIDs)
							{
								// It's the release message of any button pressed before
								tsen.LIGHTING2.unitcode = 0; // Does not matter, since we are using a group command
								tsen.LIGHTING2.cmnd = (Pressed == 1) ? light2_sGroupOn : light2_sGroupOff;
							}
							else
							{
								tsen.LIGHTING2.unitcode = 1;
								tsen.LIGHTING2.cmnd = (UpDown == 1) ? light2_sOn : light2_sOff;
							}
							tsen.LIGHTING2.rssi = rssi;
							
#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s UnitID: %02X cmd: %02X ",
								senderID.c_str(), tsen.LIGHTING2.unitcode, tsen.LIGHTING2.cmnd);
#endif

							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
						}
					}
					return;
				}
				if (pNode->func == 0x05 && pNode->type <= 0x02)
				{ // F6-05-xx, Detectors
					// F6-05-00 : Wind speed threshold detector
					// F6-05-01 : Liquid leakage sensor
					// F6-05-02 : Smoke detector

					bool alarm = false;
					// Only F6-05-00 and F6-05-02 report Energy LOW warning
					int batterylevel = (pNode->type == 0x00 || pNode->type == 0x02) ? 255 : 100;

					switch (DATA)
					{
						case 0x00: // F6-05-00 and F6-05-02
						{
#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s Alarm OFF", senderID.c_str());
#endif
							break;
						}
						case 0x10: // F6-05-00 and F6-05-02
						{
#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s Alarm ON", senderID.c_str());
#endif
							alarm = true;
							break;
						}
						case 0x11: // F6-05-01
						{
#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s Alarm ON water detected", senderID.c_str());
#endif
							alarm = true;
							break;
						}
						case 0x30: // F6-05-00 and F6-05-02
						{
#ifdef ENABLE_ESP3_DEVICE_DEBUG
							Log(LOG_NORM, "RPS msg: Node %s Energy LOW warning", senderID.c_str());
#endif
							batterylevel = 5;
							break;
						}
					}
					SendSwitch(iSenderID, 1, batterylevel, alarm, 0, GetEEPLabel(pNode->RORG, pNode->func, pNode->type), m_Name, rssi);
					return;
				}
				Log(LOG_ERROR, "RPS msg: Node %s, EEP %02X-%02X-%02X (%s) not supported",
					senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		case RORG_UTE:
			{ // UTE telegram, D4-xx-xx, Universal Teach-in
				uint8_t CMD = bitrange(data[1], 0, 0x0F);				// 0=teach-in query, 1=teach-In response
				if (CMD != 0)
				{
					Log(LOG_ERROR, "UTE teach request: Node %s, command 0x%1X not supported", senderID.c_str(), CMD);
					return;
				}
				// UTE teach-in or teach-out Query (UTE Telegram / CMD 0x0)

				uint8_t ute_direction = bitrange(data[1], 7, 0x01);	// 0=uni-directional, 1= bi-directional
				uint8_t need_response = bitrange(data[1], 6, 0x01);	// 0=yes, 1=no
				uint8_t ute_request = bitrange(data[1], 4, 0x03);		// 0=teach-in, 1=teach-out, 2=teach-in or teach-out

				uint8_t node_nb_channels = data[2];

				uint16_t node_manID = (bitrange(data[4], 0, 0x07) << 8) | data[3];

				uint8_t node_type = data[5];
				uint8_t node_func = data[6];
				uint8_t node_RORG = data[7];

				Log(LOG_NORM, "UTE %s-directional %s request from Node %s, %sresponse expected",
					(ute_direction == 0) ? "uni" : "bi",
					(ute_request == 0) ? "teach-in" : ((ute_request == 1) ? "teach-out" : "teach-in or teach-out"),
					senderID.c_str(),
					(need_response == 0) ? "" : "no "
					);

				uint8_t buf[15];

				if (need_response == 0)
				{ // Prepare response buffer
					// the device intended to be taught-in broadcasts a query message
					// and gets back an addresses response message, containing its own ID as the transmission target address
					buf[0] = RORG_UTE;
					buf[2] = data[2]; // Nb channels
					buf[3] = data[3]; // Manufacturer ID
					buf[4] = data[4];
					buf[5] = data[5]; // Type
					buf[6] = data[6]; // Func
					buf[7] = data[7]; // RORG
					buf[8] = data[8]; // Dest ID
					buf[9] = data[9];
					buf[10] = data[10];
					buf[11] = data[11];
					buf[12] = 0x00; // Status
				}
				if (pNode == nullptr)
				{ // Node not found
					if (ute_request == 1)
					{ // Node not found and teach-out request => ignore
						Log(LOG_NORM, "Unknown Node %s, teach-out request ignored", senderID.c_str());

						if (need_response == 0)
						{ // Build and send response
							buf[1] = (UTE_BIDIRECTIONAL & 0x01) << 7 | (TEACHOUT_ACCEPTED & 0x03) << 4 | UTE_RESPONSE;
							SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, nullptr, 0);
						}
						return;
					}
					if (!m_sql.m_bAcceptNewHardware)
					{ // Node not found and learn mode disabled => error
						Log(LOG_NORM, "Unknown Node %s, please allow accepting new hardware and proceed to teach-in", senderID.c_str());
						if (need_response == 0)
						{ // Build and send response
							buf[1] = (UTE_BIDIRECTIONAL & 0x01) << 7 | (GENERAL_REASON & 0x03) << 4 | UTE_RESPONSE;
							SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, nullptr, 0);
						}
						return;
					}
					// Node not found and learn mode enabled and teach-in request : add it to the database

					Log(LOG_NORM, "Creating Node %s Manufacturer 0x%03X (%s) EEP %02X-%02X-%02X (%s), %u channel%s",
						senderID.c_str(), node_manID, GetManufacturerName(node_manID),
						node_RORG, node_func, node_type, GetEEPLabel(node_RORG, node_func, node_type),
						node_nb_channels, (node_nb_channels > 1) ? "s" : "");

					TeachInNode(senderID, node_manID, node_RORG, node_func, node_type, false);

					if (need_response == 0)
					{ // Build and send response
						buf[1] = (UTE_BIDIRECTIONAL & 0x01) << 7 | (TEACHIN_ACCEPTED & 0x03) << 4 | UTE_RESPONSE;
						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, nullptr, 0);
					}
					if (node_RORG == RORG_VLD && node_func == 0x01
						&& (node_type == 0x0D || node_type == 0x0E
							|| node_type == 0x0F || node_type == 0x12
							|| node_type == 0x15 || node_type == 0x16
							|| node_type == 0x17))
					{
						// Create devices for :
						// D2-01-0D, Micro smart plug, single channel, with external button control
						// D2-01-0E, Micro smart plug, single channel, with external button control
						// D2-01-0F, Slot-in module, single channel, with external button control
						// D2-01-12, Slot-in module, dual channels, with external button control
						// D2-01-15, D2-01-16, D2-01-17

						for (uint8_t nbc = 0; nbc < node_nb_channels; nbc++)
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
							tsen.LIGHTING2.unitcode = nbc + 1;
							tsen.LIGHTING2.cmnd = light2_sOff;
							tsen.LIGHTING2.rssi = rssi;
							sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());
						}
					}
					return;
				}
				// Node found
				if (ute_request == 0)
				{ // Node found and teach-in request => ignore
					Log(LOG_NORM, "Node %s already known with EEP %02X-%02X-%02X (%s), teach-in request ignored",
						senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));

					if (need_response == 0)
					{ // Build and send response
						buf[1] = (UTE_BIDIRECTIONAL & 0x01) << 7 | (TEACHIN_ACCEPTED & 0x03) << 4 | UTE_RESPONSE;
						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, nullptr, 0);
					}
				}
				else if (ute_request == 1 || ute_request == 2)
				{ // Node found and teach-out request => teach-out
					// Ignore teach-out request to avoid teach-in/out loop
					Log(LOG_NORM, "UTE msg: Node %s, teach-out request not supported", senderID.c_str());

					if (need_response == 0)
					{ // Build and send response
						buf[1] = (UTE_BIDIRECTIONAL & 0x01) << 7 | (GENERAL_REASON & 0x03) << 4 | UTE_RESPONSE;
						SendESP3Packet(PACKET_RADIO_ERP1, buf, 13, nullptr, 0);
					}
				}
			}
			return;

		case RORG_VLD:
			{ // VLD telegram, D2-xx-xx, Variable Length Data
				if (pNode == nullptr)
				{
					Log(LOG_NORM, "VLD msg: Unknown Node %s, please proceed to teach-in", senderID.c_str());
					return;
				}
				Log(LOG_NORM, "VLD msg: Node %s EEP: %02X-%02X-%02X", senderID.c_str(), pNode->RORG, pNode->func, pNode->type);

				if (pNode->func == 0x01)
				{ // D2-01-XX, Electronic Switches and Dimmers with Local Control
					uint8_t CMD = bitrange(data[1], 0, 0x0F); // Command ID
					if (CMD != 0x04)
					{
						Log(LOG_ERROR, "VLD msg: Node %s, command 0x%01X not supported", senderID.c_str(), CMD);
						return;
					}
					// CMD 0x4 - Actuator Status Response

					uint8_t IO = bitrange(data[2], 0, 0x1F); // I/O Channel

					uint8_t OV = bitrange(data[3], 0, 0x7F); // Output Value % : 0x00 = Off, 0x01...0x64: On or 1% to 100%

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
					sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, nullptr, 255, m_Name.c_str());

#ifdef ENABLE_ESP3_DEVICE_DEBUG
					Log(LOG_NORM, "VLD msg: Node %s CMD: 0x%X IO: %02X (UnitID: %d) OV: %02X (Cmnd: %d Level: %d)",
						senderID.c_str(), CMD, IO, tsen.LIGHTING2.unitcode, OV, tsen.LIGHTING2.cmnd, tsen.LIGHTING2.level);
#endif

					// Note: if a device uses simultaneously RPS and VLD (ex: nodon inwall module), it can be partially initialized.
					// Domoticz will show device status but some functions may not work because EnoceanSensors table has no info on this device (until teach-in is performed)
					// If a device has local control (ex: nodon inwall module with physically attached switched), domoticz will record the local control as unit 0.
					// Ex: nodon inwall 2 channels will show 3 entries. Unit 0 is the local switch, 1 is the first channel, 2 is the second channel.
					return;
				}
				if (pNode->func == 0x03 && pNode->type == 0x0A)
				{ // D2-03-0A, Push Button, Single Button
					uint8_t BATT = round(GetDeviceValue(data[1], 1, 100, 1, 100));
					uint8_t BA = data[2]; // 1 = simple press, 2 = double press, 3 = long press, 4 = long press released
					SendGeneralSwitch(iSenderID, BA, BATT, 1, 0, "Switch", m_Name, 12);
					return;
				}
				Log(LOG_ERROR, "VLD msg: Node %s, EEP %02X-%02X-%02X (%s) not supported",
					senderID.c_str(), pNode->RORG, pNode->func, pNode->type, GetEEPLabel(pNode->RORG, pNode->func, pNode->type));
			}
			return;

		default:
			Log(LOG_NORM, "ERP1: Node %s, RORG %s (%s) not supported", senderID.c_str(), GetRORGLabel(RORG), GetRORGDescription(RORG));
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
