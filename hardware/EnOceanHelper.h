#pragma once

typedef enum
{
	UNKNOWN_MANUFACTURER = 0x000,
	PEHA = 0x001,
	THERMOKON = 0x002,
	SERVODAN = 0x003,
	ECHOFLEX_SOLUTIONS = 0x004,
	OMNIO_AG = 0x005,
	AWAG_ELEKTROTECHNIK_AG = 0x005, // Merged
	HARDMEIER_ELECTRONICS = 0x006,
	REGULVAR_INC = 0x007,
	AD_HOC_ELECTRONICS = 0x008,
	DISTECH_CONTROLS = 0x009,
	KIEBACK_AND_PETER = 0x00A,
	ENOCEAN_GMBH = 0x00B,
	PROBARE = 0x00C,
	VICOS_GMBH = 0x00C, // Merged
	ELTAKO = 0x00D,
	LEVITON = 0x00E,
	HONEYWELL = 0x00F,
	SPARTAN_PERIPHERAL_DEVICES = 0x010,
	SIEMENS = 0x011,
	T_MAC = 0x012,
	RELIABLE_CONTROLS_CORPORATION = 0x013,
	ELSNER_ELEKTRONIK_GMBH = 0x014,
	DIEHL_CONTROLS = 0x015,
	BSC_COMPUTER = 0x016,
	S_AND_S_REGELTECHNIK_GMBH = 0x017,
	ZENO_CONTROLS = 0x018,
	MASCO_CORPORATION = 0x018, // Reassigned
	INTESIS_SOFTWARE_SL = 0x019,
	VIESSMANN = 0x01A,
	LUTUO_TECHNOLOGY = 0x01B,
	CAN2GO = 0x01C,
	SAUTER = 0x01D,
	BOOT_UP = 0x01E,
	OSRAM_SYLVANIA = 0x01F,
	UNOTECH = 0x020,
	DELTA_CONTROLS_INC = 0x21,
	UNITRONIC_AG = 0x022,
	NANOSENSE = 0x023,
	THE_S4_GROUP = 0x024,
	MSR_SOLUTIONS = 0x025,
	VEISSMANN_HAUSATOMATION_GMBH = 0x025, // Purchased
	GE = 0x26,
	MAICO = 0x027,
	RUSKIN_COMPANY = 0x28,
	MAGNUM_ENERGY_SOLUTIONS = 0x29,
	KMC_CONTROLS = 0x02A,
	ECOLOGIX_CONTROLS = 0x02B,
	TRIO_2_SYS = 0x2C,
	AFRISO_EURO_INDEX = 0x02D,
	WALDMANN_GMBH = 0x02E,
	NEC_PLATFORMS_LTD = 0x030,
	ITEC_CORPORATION = 0x031,
	SIMICX_CO_LTD = 0x32,
	PERMUNDO_GMBH = 0x33,
	EUROTRONIC_TECHNOLOGY_GMBH = 0x34,
	ART_JAPAN_CO_LTD = 0x35,
	TIANSU_AUTOMATION_CONTROL_SYSTE_CO_LTD = 0x36,
	WEINZIERL_ENGINEERING_GMBH = 0x37,
	GRUPPO_GIORDANO_IDEA_SPA = 0x38,
	ALPHAEOS_AG = 0x39,
	TAG_TECHNOLOGIES = 0x3A,
	WATTSTOPPER = 0x3B,
	PRESSAC_COMMUNICATIONS_LTD = 0x3C,
	GIGA_CONCEPT = 0x3E,
	SENSORTEC = 0x3F,
	JAEGER_DIREKT = 0x40,
	AIR_SYSTEM_COMPONENTS_INC = 0x41,
	ERMINE_CORP = 0x042,
	SODA_GMBH = 0x043,
	EKE_AUTOMATION = 0x044,
	HOLTER_REGELARMUTREN = 0x045,
	NODON = 0x046,
	DEUTA_CONTROLS_GMBH = 0x047,
	EWATTCH = 0x048,
	MICROPELT = 0x049,
	CALEFFI_SPA = 0x04A,
	DIGITAL_CONCEPTS = 0x04B,
	EMERSON_CLIMATE_TECHNOLOGIES = 0x04C,
	ADEE_ELECTRONIC = 0x04D,
	ALTECON = 0x04E,
	NANJING_PUTIAN_TELECOMMUNICATIONS = 0x04F,
	TERRALUX = 0x050,
	MENRED = 0x051,
	IEXERGY_GMBH = 0x052,
	OVENTROP_GMBH = 0x053,
	BUILDING_AUTOMATION_PRODUCTS_INC = 0x054,
	FUNCTIONAL_DEVICES_INC = 0x055,
	OGGA = 0x056,
	ITHO_DAALDEROP = 0x057,
	RESOL = 0x058,
	ADVANCED_DEVICES = 0x059,
	AUTANI_LCC = 0x05A,
	DR_RIEDEL_GMBH = 0x05B,
	HOPPE_HOLDING_AG = 0x05C,
	SIEGENIA_AUBI_KG = 0x05D,
	ADEO_SERVICES = 0x05E,
	EIMSIG_EFP_GMBH = 0x05F,
	VIMAR_SPA = 0x060,
	GLEN_DIMLAX_GMBH = 0x061,
	PMDM_GMBH = 0x062,
	HUBBEL_LIGHTNING = 0x063,
	DEBFLEX = 0x64,
	PERIFACTORY_SENSORSYSTEMS = 0x65,
	WATTY_CORP = 0x66,
	WAGO_KONTAKTTECHNIK = 0x67,
	KESSEL = 0x68,
	AUG_WINKHAUS = 0x69,
	DECELECT = 0x6A,
	MST_INDUSTRIES = 0x6B,
	BECKER_ANTRIEBS_GMBH = 0x06C,
	NEXELEC = 0x06D,
	WIELAND_ELECTRIC_GMBH = 0x06E,
	ADVISEN = 0x06F,
	CWS_BCO_INTERNATIONAL_GMBH = 0x070,
	ROTO_FRANCK_AG = 0x071, 
	ALM_CONTROLS_E_K = 0x072,
	TOMMASO_TECHNOLOGIES_CO_LTD = 0x073,
	REHAUS_AG_AND_CO = 0x074,
	INABA_DENKI_SANGYO_CO_LTD = 0x075,
	HAGER_CONTROL_SAS = 0x076,
	MULTI_USER_MANUFACTURER = 0x7FF
} ENOCEAN_MANUFACTURER;

#define ENOCEAN_MANUFACTURER_FIRST PEHA
#define ENOCEAN_MANUFACTURER_LAST HAGER_CONTROL_SAS

const char* EnOceanGetManufacturerLabel(const uint32_t ID);
const char* EnOceanGetManufacturerName(const uint32_t ID);

//ESP 3

// ESP3 Packet types
typedef enum
{
	PACKET_RADIO_ERP1 = 0x01, // Radio telegram
	PACKET_RESPONSE = 0x02, // Response to any packet
	PACKET_RADIO_SUB_TEL = 0x03, // Radio subtelegram
	PACKET_EVENT = 0x04, // Event message
	PACKET_COMMON_COMMAND = 0x05, // Common command"
	PACKET_SMART_ACK_COMMAND = 0x06, // Smart Acknowledge command
	PACKET_REMOTE_MAN_COMMAND = 0x07, // Remote management command
	PACKET_RADIO_MESSAGE = 0x09, // Radio message
	PACKET_RADIO_ERP2 = 0x0A, // ERP2 protocol radio telegram
	PACKET_CONFIG_COMMAND = 0x0B, // RESERVED
	PACKET_COMMAND_ACCEPTED = 0x0C, // For long operations, informs the host the command is accepted
	PACKET_RADIO_802_15_4 = 0x10, // 802_15_4 Raw Packet
	PACKET_COMMAND_2_4 = 0x11, // 2.4 GHz Command
} ESP3_PACKET_TYPE;

const char *ESP3GetPacketTypeLabel(const uint8_t PT);
const char *ESP3GetPacketTypeDescription(const uint8_t PT);

// ESP3 RORGs
typedef enum {
	RORG_ST = 0x30, // Secure telegram
	RORG_ST_WE = 0x31, // Secure telegram with RORG encapsulation
	RORG_STT_FW = 0x35, // Secure teach-in telegram for switch
	RORG_4BS = 0xA5, // 4 Bytes Communication
	RORG_ADT = 0xA6, // Adressing Destination Telegram
	RORG_SM_REC = 0xA7, // Smart Ack Reclaim
	RORG_GP_SD = 0xB3, // Generic Profiles selective data
	RORG_SM_LRN_REQ = 0xC6, // Smart Ack Learn Request
	RORG_SM_LRN_ANS = 0xC7, // Smart Ack Learn Answer
	RORG_SM_ACK_SGNL = 0xD0, // Smart Acknowledge Signal telegram
	RORG_MSC = 0xD1, // Manufacturer Specific Communication
	RORG_VLD = 0xD2, // Variable length data telegram 
	RORG_UTE = 0xD4, // Universal teach-in EEP based 
	RORG_1BS = 0xD5, // 1 Byte Communication
	RORG_RPS = 0xF6, // Repeated Switch Communication
	RORG_SYS_EX = 0xC5, // Remote Management
} ESP3_RORG_TYPE;

const char *ESP3GetRORGLabel(const uint8_t RORG);
const char *ESP3GetRORGDescription(const uint8_t RORG);

// ESP3 Return codes
typedef enum
{
	RET_OK = 0x00, // OK ... command is understood and triggered
	RET_ERROR = 0x01, // There is an error occured
	RET_NOT_SUPPORTED = 0x02, // The functionality is not supported by that implementation
	RET_WRONG_PARAM = 0x03, // There was a wrong parameter in the command
	RET_OPERATION_DENIED = 0x04, // Example: memory access denied (code-protected)
	RET_LOCK_SET = 0x05, // Duty cycle lock
	RET_BUFFER_TO_SMALL = 0x06, // The internal ESP3 buffer of the device is too small, to handle this telegram
	RET_NO_FREE_BUFFER = 0x07, // Currently all internal buffers are used
	RET_MEMORY_ERROR = 0x82, // The memory write process failed
	RET_BASEID_OUT_OF_RANGE = 0x90, // BaseID out of range
	RET_BASEID_MAX_REACHED = 0x91, // BaseID has already been changed 10 times, no more changes are allowed
} ESP3_RETURN_CODE;

const char *ESP3GetReturnCodeLabel(const uint8_t RC);
const char *ESP3GetReturnCodeDescription(const uint8_t RC);

// Event codes
typedef enum
{
	SA_RECLAIM_NOT_SUCCESSFUL = 0x01, // Informs the external host about an unsuccessful reclaim by a Smart Ack. client
	SA_CONFIRM_LEARN = 0x02, // Request to the external host about how to handle a received learn-in / learn-out of a Smart Ack. client
	SA_LEARN_ACK = 0x03, // Response to the Smart Ack. client about the result of its Smart Acknowledge learn request
	CO_READY = 0x04, // Inform the external about the readiness for operation
	CO_EVENT_SECUREDEVICES = 0x05, // Informs the external host about an event in relation to security processing
	CO_DUTYCYCLE_LIMIT = 0x06, // Informs the external host about reaching the duty cycle limit
	CO_TRANSMIT_FAILED = 0x07, // Informs the external host about not being able to send a telegram
	CO_TX_DONE = 0x08, // Informs that all TX operations are done
	CO_LRN_MODE_DISABLED = 0x09 // Informs that the learn mode has time-out
} ESP3_EVENT_CODE;

const char *ESP3GetEventCodeLabel(const uint8_t EC);
const char *ESP3GetEventCodeDescription(const uint8_t EC);

// Common commands
typedef enum
{
	CO_WR_SLEEP = 0x01, // Enter energy saving mode
	CO_WR_RESET = 0x02, // Reset the device
	CO_RD_VERSION = 0x03, // Read the device version information
	CO_RD_SYS_LOG = 0x04, // Read system log
	CO_WR_SYS_LOG = 0x05, // Reset system log
	CO_WR_BIST = 0x06, // Perform Self Test
	CO_WR_IDBASE = 0x07, // Set ID range base address
	CO_RD_IDBASE = 0x08, // Read ID range base address
	CO_WR_REPEATER = 0x09, // Set Repeater Level
	CO_RD_REPEATER = 0x10, // Read Repeater Level
	CO_WR_FILTER_ADD = 0x11, // Add filter to filter list
	CO_WR_FILTER_DEL = 0x12, // Delete a specific filter from filter list
	CO_WR_FILTER_DEL_ALL = 0x13, // Delete all filters from filter list
	CO_WR_FILTER_ENABLE = 0x14, // Enable / disable filter list
	CO_RD_FILTER = 0x15, // Read filters from filter list
	CO_WR_WAIT_MATURITY = 0x16, // Wait until the end of telegram maturity time before received radio telegrams will be forwarded to the external host
	CO_WR_SUBTEL = 0x17, // Enable / Disable transmission of additional subtelegram info to the external host
	CO_WR_MEM = 0x18, // Write data to device memory
	CO_RD_MEM = 0x19, // Read data from device memory
	CO_RD_MEM_ADDRESS = 0x20, // Read address and length of the configuration area and the Smart Ack Table
	CO_RD_SECURITY = 0x21, // DEPRECATED Read own security information (level, // key)
	CO_WR_SECURITY = 0x22, // DEPRECATED Write own security information (level, // key)
	CO_WR_LEARNMODE = 0x23, // Enable / disable learn mode
	CO_RD_LEARNMODE = 0x24, // ead learn mode status
	CO_WR_SECUREDEVICE_ADD = 0x25, // DEPRECATED Add a secure device
	CO_WR_SECUREDEVICE_DEL = 0x26, // Delete a secure device from the link table
	CO_RD_SECUREDEVICE_BY_INDEX = 0x27, // DEPRECATED Read secure device by index
	CO_WR_MODE = 0x28, // Set the gateway transceiver mode
	CO_RD_NUMSECUREDEVICES = 0x29, // Read number of secure devices in the secure link table
	CO_RD_SECUREDEVICE_BY_ID = 0x30, // Read information about a specific secure device from the secure link table using the device ID
	CO_WR_SECUREDEVICE_ADD_PSK = 0x31, // Add Pre-shared key for inbound secure device
	CO_WR_SECUREDEVICE_ENDTEACHIN = 0x32, // Send Secure teach-In message
	CO_WR_TEMPORARY_RLC_WINDOW = 0x33, // Set a temporary rolling-code window for every taught-in device
	CO_RD_SECUREDEVICE_PSK = 0x34, // Read PSK
	CO_RD_DUTYCYCLE_LIMIT = 0x35, // Read the status of the duty cycle limit monitor
	CO_SET_BAUDRATE = 0x36, // Set the baud rate used to communicate with the external host
	CO_GET_FREQUENCY_INFO = 0x37, // Read the radio frequency and protocol supported by the device
	CO_38T_STEPCODE = 0x38, // Read Hardware Step code and Revision of the Device
	CO_40_RESERVED = 0x40, // Reserved
	CO_41_RESERVED = 0x41, // Reserved
	CO_42_RESERVED = 0x42, // Reserved
	CO_43_RESERVED = 0x43, // Reserved
	CO_44_RESERVED = 0x44, // Reserved
	CO_45_RESERVED = 0x45, // Reserved
	CO_WR_REMAN_CODE = 0x46, // Set the security code to unlock Remote Management functionality via radio
	CO_WR_STARTUP_DELAY = 0x47, // Set the startup delay (time from power up until start of operation)
	CO_WR_REMAN_REPEATING = 0x48, // Select if REMAN telegrams originating from this module can be repeated
	CO_RD_REMAN_REPEATING = 0x49, // Check if REMAN telegrams originating from this module can be repeated
	CO_SET_NOISETHRESHOLD = 0x50, // Set the RSSI noise threshold level for telegram reception
	CO_GET_NOISETHRESHOLD = 0x51, // Read the RSSI noise threshold level for telegram reception
	CO_52_RESERVED = 0x52, // Reserved
	CO_53_RESERVED = 0x53, // Reserved
	CO_WR_RLC_SAVE_PERIOD = 0x54, // Set the period in which outgoing RLCs are saved to the EEPROM
	CO_WR_RLC_LEGACY_MODE = 0x55, // Activate the legacy RLC security mode allowing roll-over and using the RLC acceptance window for 24bit explicit RLC
	CO_WR_SECUREDEVICEV2_ADD = 0x56, // Add secure device to secure link table
	CO_RD_SECUREDEVICEV2_BY_INDEX = 0x57, // Read secure device from secure link table using the table index
	CO_WR_RSSITEST_MODE = 0x58, // Control the state of the RSSI-Test mode
	CO_RD_RSSITEST_MODE = 0x59, // Read the state of the RSSI-Test Mode
	CO_WR_SECUREDEVICE_MAINTENANCEKEY = 0x60, // Add the maintenance key information into the secure link table
	CO_RD_SECUREDEVICE_MAINTENANCEKEY = 0x61, // Read by index the maintenance key information from the secure link table
	CO_WR_TRANSPARENT_MODE = 0x62, // Control the state of the transparent mode
	CO_RD_TRANSPARENT_MODE = 0x63, // Read the state of the transparent mode
	CO_WR_TX_ONLY_MODE = 0x64, // Control the state of the TX only mode
	CO_RD_TX_ONLY_MODE = 0x65 // Read the state of the TX only mode} COMMON_COMMAND;
} ESP3_COMMON_COMMAND;

const char *ESP3GetCommonCommandLabel(const uint8_t CC);
const char *ESP3GetCommonCommandDescription(const uint8_t CC);

// Smart Ack codes
typedef enum
{
	SA_WR_LEARNMODE = 0x01, // Set/Reset Smart Ack learn mode
	SA_RD_LEARNMODE = 0x02, // Get Smart Ack learn mode state
	SA_WR_LEARNCONFIRM = 0x03, // Used for Smart Ack to add or delete a mailbox of a client
	SA_WR_CLIENTLEARNRQ = 0x04, // Send Smart Ack Learn request (Client)
	SA_WR_RESET = 0x05, // Send reset command to a Smart Ack client
	SA_RD_LEARNEDCLIENTS = 0x06, // Get Smart Ack learned sensors / mailboxes
	SA_WR_RECLAIMS = 0x07, // Set number of reclaim attempts
	SA_WR_POSTMASTER = 0x08 // Activate/Deactivate Post master functionality
} ESP3_SMART_ACK_CODE;

const char *ESP3GetSmarkAckCodeLabel(const uint8_t SA);
const char *ESP3GetSmartAckCodeDescription(const uint8_t SA);

// Function return codes
typedef enum
{
	RC_OK = 0, // Action performed. No problem detected
	RC_EXIT, // Action not performed. No problem detected
	RC_KO, // Action not performed. Problem detected
	RC_TIME_OUT, // Action couldn't be carried out within a certain time.
	RC_FLASH_HW_ERROR, // The write/erase/verify process failed, the flash page seems to be corrupted
	RC_NEW_RX_BYTE, // A new UART/SPI byte received
	RC_NO_RX_BYTE, // No new UART/SPI byte received
	RC_NEW_RX_TEL, // New telegram received
	RC_NO_RX_TEL, // No new telegram received
	RC_NOT_VALID_CHKSUM, // Checksum not valid
	RC_NOT_VALID_TEL, // Telegram not valid
	RC_BUFF_FULL, // Buffer full, no space in Tx or Rx buffer
	RC_ADDR_OUT_OF_MEM, // Address is out of memory
	RC_NOT_VALID_PARAM, // Invalid function parameter
	RC_BIST_FAILED, // Built in self test failed
	RC_ST_TIMEOUT_BEFORE_SLEEP, // Before entering power down, the short term timer had timed out.
	RC_MAX_FILTER_REACHED, // Maximum number of filters reached, no more filter possible
	RC_FILTER_NOT_FOUND, // Filter to delete not found
	RC_BASEID_OUT_OF_RANGE, // BaseID out of range
	RC_BASEID_MAX_REACHED, // BaseID was changed 10 times, no more changes are allowed
	RC_XTAL_NOT_STABLE, // XTAL is not stable
	RC_NO_TX_TEL, // No telegram for transmission in queue
	RC_ELEGRAM_WAIT, //	Waiting before sending broadcast message
	RC_OUT_OF_RANGE, //	Generic out of range return code
	RC_LOCK_SET, //	Function was not executed due to sending lock
	RC_NEW_TX_TEL // New telegram transmitted
} ESP3_FUNC_RETURN_CODE;

const char *ESP3GetFunctionReturnCodeLabel(const uint8_t RC);
const char *ESP3GetFunctionReturnCodeDescription(const uint8_t RC);

const char* EnOceanGetProfile(const int RORG, const int func, const int type);
const char* EnOceanGetProfileTypeLabel(const int RORG, const int func, const int type);
const char* EnOceanGetProfileTypeDescription(const int RORG, const int func, const int type);

uint32_t EnOceanGetINodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0);

std::string EnOceanGetNodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0);
std::string EnOceanGetNodeID(const uint32_t iNodeID);

float EnOceanGetDeviceValue(const float rawValue, const float scaleMin, const float scaleMax, const float rangeMin, const float rangeMax);
