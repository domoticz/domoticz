#include "stdafx.h"
#include "EnOcean.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

#include <ctime>

#define ENOCEAN_RETRY_DELAY 30

#define round(a) ( int ) ( a + .5 )

//Acknowledgments:
//Part of this code it based on the work of Daniel Lechner, Andreas Dielacher
//Note: This is for the ESP 2.0 protocol !!!

struct _tTableLookup2
{
	unsigned long ID;
	const char* Label;
};
struct _tTableLookup3
{
	unsigned long ID1;
	unsigned long ID2;
	const char* Label;
};

const char *LookupTable2(const _tTableLookup2 *pOrgTable, unsigned long ID)
{
	while (pOrgTable->Label) 
	{
		if (pOrgTable->ID == ID)
			return pOrgTable->Label;
		pOrgTable++;
	}

	return ">>Unkown... Please report!<<";
}

const char *LookupTable3(const _tTableLookup3 *pOrgTable, unsigned long ID1, unsigned long ID2)
{
	while (pOrgTable->Label) 
	{
		if ((pOrgTable->ID1 == ID1)&&(pOrgTable->ID2 == ID2))
			return pOrgTable->Label;
		pOrgTable++;
	}

	return ">>Unkown... Please report!<<";
}

const char* Get_EnoceanManufacturer(unsigned long ID)
{
	const _tTableLookup2 TTable[]=
	{
		{ 0x001, "Peha" },
		{ 0x002, "Thermokon" },
		{ 0x003, "Servodan" },
		{ 0x004, "EchoFlex Solutions" },
		{ 0x005, "Omnio AG" },
		{ 0x006, "Hardmeier electronics" },
		{ 0x007, "Regulvar Inc" },
		{ 0x008, "Ad Hoc Electronics" },
		{ 0x009, "Distech Controls" },
		{ 0x00A, "Kieback + Peter" },
		{ 0x00B, "EnOcean GmbH" },
		{ 0x00C, "Probare" },
		{ 0x00D, "Eltako" },
		{ 0x00E, "Leviton" },
		{ 0x00F, "Honeywell" },
		{ 0x010, "Spartan Peripheral Devices" },
		{ 0x011, "Siemens" },
		{ 0x012, "T-Mac" },
		{ 0x013, "Reliable Controls Corporation" },
		{ 0x014, "Elsner Elektronik GmbH" },
		{ 0x015, "Diehl Controls" },
		{ 0x016, "BSC Computer" },
		{ 0x017, "S+S Regeltechnik GmbH" },
		{ 0x018, "Masco Corporation" },
		{ 0x019, "Intesis Software SL" },
		{ 0x01A, "Res." },
		{ 0x01B, "Lutuo Technology" },
		{ 0x01C, "CAN2GO" },
		{ 0x7FF, "Multi user Manufacturer ID" }, 
		{ 0, NULL }
	};
	return LookupTable2(TTable,ID);
}

//EEP 2.5

struct _t4BSLookup
{
	const int Org;
	const int Func;
	const int Type;
	const char *Description;
	const char *Label;
};

const _t4BSLookup T4BSTable[]=
{
	//A5-02: Temperature Sensors
	{ 0xA5, 0x02, 0x01, "Temperature Sensor Range -40C to 0C",																	"Temperature.01" },
	{ 0xA5, 0x02, 0x02, "Temperature Sensor Range -30C to +10C",																"Temperature.02" },
	{ 0xA5, 0x02, 0x03, "Temperature Sensor Range -20C to +20C",																"Temperature.03" },
	{ 0xA5, 0x02, 0x04, "Temperature Sensor Range -10C to +30C",																"Temperature.04" },
	{ 0xA5, 0x02, 0x05, "Temperature Sensor Range 0C to +40C",																	"Temperature.05" },
	{ 0xA5, 0x02, 0x06, "Temperature Sensor Range +10C to +50C",																"Temperature.06" },
	{ 0xA5, 0x02, 0x07, "Temperature Sensor Range +20C to +60C",																"Temperature.07" },
	{ 0xA5, 0x02, 0x08, "Temperature Sensor Range +30C to +70C",																"Temperature.08" },
	{ 0xA5, 0x02, 0x09, "Temperature Sensor Range +40C to +80C",																"Temperature.09" },
	{ 0xA5, 0x02, 0x0A, "Temperature Sensor Range +50C to +90C",																"Temperature.0A" },
	{ 0xA5, 0x02, 0x0B, "Temperature Sensor Range +60C to +100C",																"Temperature.0B" },
	{ 0xA5, 0x02, 0x10, "Temperature Sensor Range -60C to +20C",																"Temperature.10" },
	{ 0xA5, 0x02, 0x11, "Temperature Sensor Range -50C to +30C",																"Temperature.11" },
	{ 0xA5, 0x02, 0x12, "Temperature Sensor Range -40C to +40C",																"Temperature.12" },
	{ 0xA5, 0x02, 0x13, "Temperature Sensor Range -30C to +50C",																"Temperature.13" },
	{ 0xA5, 0x02, 0x14, "Temperature Sensor Range -20C to +60C",																"Temperature.14" },
	{ 0xA5, 0x02, 0x15, "Temperature Sensor Range -10C to +70C",																"Temperature.15" },
	{ 0xA5, 0x02, 0x16, "Temperature Sensor Range 0C to +80C",																	"Temperature.16" },
	{ 0xA5, 0x02, 0x17, "Temperature Sensor Range +10C to +90C",																"Temperature.17" },
	{ 0xA5, 0x02, 0x18, "Temperature Sensor Range +20C to +100C",																"Temperature.18" },
	{ 0xA5, 0x02, 0x19, "Temperature Sensor Range +30C to +110C",																"Temperature.19" },
	{ 0xA5, 0x02, 0x1A, "Temperature Sensor Range +40C to +120C",																"Temperature.1A" },
	{ 0xA5, 0x02, 0x1B, "Temperature Sensor Range +50C to +130C",																"Temperature.1B" },
	{ 0xA5, 0x02, 0x20, "10 Bit Temperature Sensor Range -10C to +41.2C",														"Temperature.20" },
	{ 0xA5, 0x02, 0x30, "10 Bit Temperature Sensor Range -40C to +62.3C",														"Temperature.30" },

	//A5-04: Temperature and Humidity Sensor
	{ 0xA5, 0x04, 0x01, "Range 0C to +40C and 0% to 100%",																		"TempHum.01" },

	//A5-06: Light Sensor
	{ 0xA5, 0x06, 0x01, "Range 300lx to 60.000lx",																				"LightSensor.01" },
	{ 0xA5, 0x06, 0x02, "Range 0lx to 1.020lx",																					"LightSensor.02" },
	{ 0xA5, 0x06, 0x03, "10-bit measurement (1-Lux resolution) with range 0lx to 1000lx",										"LightSensor.03" },

	//A5-07: Occupancy Sensor
	{ 0xA5, 0x07, 0x01, "Occupancy with Supply voltage monitor",																"OccupancySensor.01" },
	{ 0xA5, 0x07, 0x02, "Occupancy with Supply voltage monitor",																"OccupancySensor.02" },
	{ 0xA5, 0x07, 0x03, "Occupancy with Supply voltage monitor and 10-bit illumination measurement",							"OccupancySensor.03" },

	//A5-08: Light, Temperature and Occupancy Sensor
	{ 0xA5, 0x08, 0x01, "Range 0lx to 510lx, 0C to +51C and Occupancy Button",													"TempOccupancySensor.01" },
	{ 0xA5, 0x08, 0x02, "Range 0lx to 1020lx, 0C to +51C and Occupancy Button",													"TempOccupancySensor.02" },
	{ 0xA5, 0x08, 0x03, "Range 0lx to 1530lx, -30C to +50C and Occupancy Button",												"TempOccupancySensor.03" },

	//A5-09: Gas Sensor
	{ 0xA5, 0x09, 0x01, "CO Sensor (not in use)",																				"GasSensor.01" },
	{ 0xA5, 0x09, 0x02, "CO-Sensor 0 ppm to 1020 ppm",																			"GasSensor.02" },
	{ 0xA5, 0x09, 0x04, "CO2 Sensor",																							"GasSensor.04" },
	{ 0xA5, 0x09, 0x05, "VOC Sensor",																							"GasSensor.05" },
	{ 0xA5, 0x09, 0x06, "Radon",																								"GasSensor.06" },
	{ 0xA5, 0x09, 0x07, "Particles",																							"GasSensor.07" },

	//7. A5-10: Room Operating Panel
	{ 0xA5, 0x10, 0x01, "Temperature Sensor, Set Point, Fan Speed and Occupancy Control",										"RoomOperatingPanel.01" },
	{ 0xA5, 0x10, 0x02, "Temperature Sensor, Set Point, Fan Speed and Day/Night Control",										"RoomOperatingPanel.02" },
	{ 0xA5, 0x10, 0x03, "Temperature Sensor, Set Point Control",																"RoomOperatingPanel.03" },
	{ 0xA5, 0x10, 0x04, "Temperature Sensor, Set Point and Fan Speed Control",													"RoomOperatingPanel.04" },
	{ 0xA5, 0x10, 0x05, "Temperature Sensor, Set Point and Occupancy Control",													"RoomOperatingPanel.05" },
	{ 0xA5, 0x10, 0x06, "Temperature Sensor, Set Point and Day/Night Control",													"RoomOperatingPanel.06" },
	{ 0xA5, 0x10, 0x07, "Temperature Sensor, Fan Speed Control",																"RoomOperatingPanel.07" },
	{ 0xA5, 0x10, 0x08, "Temperature Sensor, Fan Speed and Occupancy Control",													"RoomOperatingPanel.08" },
	{ 0xA5, 0x10, 0x09, "Temperature Sensor, Fan Speed and Day/Night Control",													"RoomOperatingPanel.09" },
	{ 0xA5, 0x10, 0x0A, "Temperature Sensor, Set Point Adjust and Single Input Contact",										"RoomOperatingPanel.0A" },
	{ 0xA5, 0x10, 0x0B, "Temperature Sensor and Single Input Contact",															"RoomOperatingPanel.0B" },
	{ 0xA5, 0x10, 0x0C, "Temperature Sensor and Occupancy Control",																"RoomOperatingPanel.0C" },
	{ 0xA5, 0x10, 0x0D, "Temperature Sensor and Day/Night Control",																"RoomOperatingPanel.0D" },
	{ 0xA5, 0x10, 0x10, "Temperature and Humidity Sensor, Set Point and Occupancy Control",										"RoomOperatingPanel.10" },
	{ 0xA5, 0x10, 0x11, "Temperature and Humidity Sensor, Set Point and Day/Night Control",										"RoomOperatingPanel.11" },
	{ 0xA5, 0x10, 0x12, "Temperature and Humidity Sensor and Set Point",														"RoomOperatingPanel.12" },
	{ 0xA5, 0x10, 0x13, "Temperature and Humidity Sensor, Occupancy Control",													"RoomOperatingPanel.13" },
	{ 0xA5, 0x10, 0x14, "Temperature and Humidity Sensor, Day/Night Control",													"RoomOperatingPanel.14" },
	{ 0xA5, 0x10, 0x15, "10 Bit Temperature Sensor, 6 bit Set Point Control",													"RoomOperatingPanel.15" },
	{ 0xA5, 0x10, 0x16, "10 Bit Temperature Sensor, 6 bit Set Point Control;Occupancy Control",									"RoomOperatingPanel.16" },
	{ 0xA5, 0x10, 0x17, "10 Bit Temperature Sensor, Occupancy Control",															"RoomOperatingPanel.17" },
	{ 0xA5, 0x10, 0x18, "Illumination, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control",				"RoomOperatingPanel.18" },
	{ 0xA5, 0x10, 0x19, "Humidity, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control",					"RoomOperatingPanel.19" },
	{ 0xA5, 0x10, 0x1A, "Supply voltage monitor, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control",	"RoomOperatingPanel.1A" },
	{ 0xA5, 0x10, 0x1B, "Supply Voltage Monitor, Illumination, Temperature Sensor, Fan Speed and Occupancy Control",			"RoomOperatingPanel.1B" },
	{ 0xA5, 0x10, 0x1C, "Illumination, Illumination Set Point, Temperature Sensor, Fan Speed and Occupancy Control",			"RoomOperatingPanel.1C" },
	{ 0xA5, 0x10, 0x1D, "Humidity, Humidity Set Point, Temperature Sensor, Fan Speed and Occupancy Control",					"RoomOperatingPanel.1D" },
	{ 0xA5, 0x10, 0x1E, "Supply Voltage Monitor, Illumination, Temperature Sensor, Fan Speed and Occupancy Control",			"RoomOperatingPanel.1B" },//same as 1B
	{ 0xA5, 0x10, 0x1F, "Temperature Sensor, Set Point, Fan Speed, Occupancy and Un-Occupancy Control",							"RoomOperatingPanel.1F" },

	//A5-11: Controller Status
	{ 0xA5, 0x11, 0x01, "Lighting Controller",																					"ControllerStatus.01" },
	{ 0xA5, 0x11, 0x02, "Temperature Controller Output",																		"ControllerStatus.02" },
	{ 0xA5, 0x11, 0x03, "Blind Status",																							"ControllerStatus.03" },
	{ 0xA5, 0x11, 0x04, "Extended Lighting Status",																				"ControllerStatus.04" },

	//A5-12: Automated meter reading (AMR)
	{ 0xA5, 0x12, 0x00, "Counter",																								"AMR.Counter" },
	{ 0xA5, 0x12, 0x01, "Electricity",																							"AMR.Electricity" },
	{ 0xA5, 0x12, 0x02, "Gas",																									"AMR.Gas" },
	{ 0xA5, 0x12, 0x03, "Water",																								"AMR.Water" },

	//A5-13: Environmental Applications
	{ 0xA5, 0x13, 0x01, "Weather Station",																						"EnvironmentalApplications.01" },
	{ 0xA5, 0x13, 0x02, "Sun Intensity",																						"EnvironmentalApplications.02" },
	{ 0xA5, 0x13, 0x03, "Date Exchange",																						"EnvironmentalApplications.03" },
	{ 0xA5, 0x13, 0x04, "Time and Day Exchange",																				"EnvironmentalApplications.04" },
	{ 0xA5, 0x13, 0x05, "Direction Exchange",																					"EnvironmentalApplications.05" },
	{ 0xA5, 0x13, 0x06, "Geographic Position Exchange",																			"EnvironmentalApplications.06" },
	{ 0xA5, 0x13, 0x10, "Sun position and radiation",																			"EnvironmentalApplications.10" },

	//A5-14: Multi-Func Sensor
	{ 0xA5, 0x14, 0x01, "Single Input Contact (Window/Door), Supply voltage monitor",											"MultiFuncSensor.01" },
	{ 0xA5, 0x14, 0x02, "Single Input Contact (Window/Door), Supply voltage monitor and Illumination",							"MultiFuncSensor.02" },
	{ 0xA5, 0x14, 0x03, "Single Input Contact (Window/Door), Supply voltage monitor and Vibration",								"MultiFuncSensor.03" },
	{ 0xA5, 0x14, 0x04, "Single Input Contact (Window/Door), Supply voltage monitor, Vibration and Illumination",				"MultiFuncSensor.04" },
	{ 0xA5, 0x14, 0x05, "Vibration/Tilt, Supply voltage monitor",																"MultiFuncSensor.05" },
	{ 0xA5, 0x14, 0x06, "Vibration/Tilt, Illumination and Supply voltage monitor",												"MultiFuncSensor.06" },

	//A5-20: HVAC Components
	{ 0xA5, 0x20, 0x01, "Battery Powered Actuator (BI-DIR)",																	"HVAC.01" },
	{ 0xA5, 0x20, 0x02, "Basic Actuator (BI-DIR)",																				"HVAC.02" },
	{ 0xA5, 0x20, 0x03, "Line powered Actuator (BI-DIR)",																		"HVAC.03" },
	{ 0xA5, 0x20, 0x10, "Generic HVAC Interface (BI-DIR)",																		"HVAC.10" },
	{ 0xA5, 0x20, 0x11, "Generic HVAC Interface – Error Control (BI-DIR)",														"HVAC.11" },
	{ 0xA5, 0x20, 0x12, "Temperature Controller Input",																			"HVAC.12" },

	//A5-30: Digital Input
	{ 0xA5, 0x30, 0x01, "Single Input Contact, Battery Monitor",																"DigitalInput.01" },
	{ 0xA5, 0x30, 0x02, "Single Input Contact",																					"DigitalInput.02" },

	//A5-37: Energy Management
	{ 0xA5, 0x37, 0x01, "Demand Response",																						"EnergyManagement.01" },

	//A5-38: Central Command
	{ 0xA5, 0x38, 0x08, "Gateway",																								"CentralCommand.01" },
	{ 0xA5, 0x38, 0x09, "Extended Lighting-Control",																			"CentralCommand.02" },

	//A5-3F: Universal
	{ 0xA5, 0x3F, 0x00, "Radio Link Test (BI-DIR)",																				"Universal.01" },

	//End of table
	{ 0, 0, 0, 0, 0 }
};

const char* Get_Enocean4BSType(const int Org, const int Func, const int Type)
{
	const _t4BSLookup *pOrgTable=(const _t4BSLookup *)&T4BSTable;
	while (pOrgTable->Label) 
	{
		if (
			(pOrgTable->Org == Org)&&
			(pOrgTable->Func == Func)&&
			(pOrgTable->Type == Type)
			)
			return pOrgTable->Label;
		pOrgTable++;
	}

	return ">>Unkown... Please report!<<";
}

const char* Get_Enocean4BSDesc(const int Org, const int Func, const int Type)
{
	const _t4BSLookup *pOrgTable=(const _t4BSLookup *)&T4BSTable;
	while (pOrgTable->Label) 
	{
		if (
			(pOrgTable->Org == Org)&&
			(pOrgTable->Func == Func)&&
			(pOrgTable->Type == Type)
			)
			return pOrgTable->Description;
		pOrgTable++;
	}

	return ">>Unkown... Please report!<<";
}


/**
 * \brief The default structure for EnOcean packets
 *
 * Data structure for RPS, 1BS, 4BS and HRC packages
 * Since most of the packages are in this format, this
 * is taken as default. Packages of other structure have
 * to be converted with the appropirate functions.
 **/
typedef struct enocean_data_structure {
  unsigned char SYNC_BYTE1; ///< Synchronisation Byte 1
  unsigned char SYNC_BYTE2; ///< Synchronisation Byte 2
  unsigned char H_SEQ_LENGTH; ///< Header identification and number of octets following the header octet
  unsigned char ORG; ///< Type of telegram
  unsigned char DATA_BYTE3; ///< Data Byte 3
  unsigned char DATA_BYTE2; ///< Data Byte 2
  unsigned char DATA_BYTE1; ///< Data Byte 1
  unsigned char DATA_BYTE0; ///< Data Byte 0
  unsigned char ID_BYTE3; ///< Transmitter ID Byte 3
  unsigned char ID_BYTE2; ///< Transmitter ID Byte 2
  unsigned char ID_BYTE1; ///< Transmitter ID Byte 1
  unsigned char ID_BYTE0; ///< Transmitter ID Byte 0
  unsigned char STATUS; ///< Status field
  unsigned char CHECKSUM; ///< Checksum of the packet
} enocean_data_structure;

/// 6DT Package structure
/** Data structure for 6DT packages
 **/
typedef struct enocean_data_structure_6DT {
  unsigned char SYNC_BYTE1; ///< Synchronisation Byte 1
  unsigned char SYNC_BYTE2; ///< Synchronisation Byte 2
  unsigned char H_SEQ_LENGTH; ///< Header identification and number of octets following the header octet
  unsigned char ORG; ///< Type of telegram
  unsigned char DATA_BYTE5; ///< Data Byte 5
  unsigned char DATA_BYTE4; ///< Data Byte 4
  unsigned char DATA_BYTE3; ///< Data Byte 3
  unsigned char DATA_BYTE2; ///< Data Byte 2
  unsigned char DATA_BYTE1; ///< Data Byte 1
  unsigned char DATA_BYTE0; ///< Data Byte 0
  unsigned char ADDRESS1; ///< Adress Byte 1
  unsigned char ADDRESS0; ///< Adress Byte 0
  unsigned char STATUS; ///< Status field
  unsigned char CHECKSUM; ///< Checksum of the packet
} enocean_data_structure_6DT;

/// MDA Package structure
/** Data structure for MDA packages
 **/
typedef struct enocean_data_structure_MDA {
  unsigned char SYNC_BYTE1; ///< Synchronisation Byte 1
  unsigned char SYNC_BYTE2; ///< Synchronisation Byte 2
  unsigned char H_SEQ_LENGTH; ///< Header identification and number of octets following the header octet
  unsigned char ORG; ///< Type of telegram
  unsigned char DATA_UNUSED5; ///< Data Byte 5 (unused)
  unsigned char DATA_UNUSED4; ///< Data Byte 4 (unused)
  unsigned char DATA_UNUSED3; ///< Data Byte 3 (unused)
  unsigned char DATA_UNUSED2; ///< Data Byte 2 (unused)
  unsigned char ADDRESS1; ///< Adress Byte 1
  unsigned char ADDRESS0; ///< Adress Byte 0
  unsigned char DATA_UNUSED1; ///< Data Byte 1 (unused)
  unsigned char DATA_UNUSED0; ///< Data Byte 0 (unused)
  unsigned char STATUS; ///< Status field
  unsigned char CHECKSUM; ///< Checksum of the packet
} enocean_data_structure_MDA;

#define C_S_BYTE1 0xA5
#define C_S_BYTE2 0x5A

/**
 * @defgroup h_seq Header identification
 * The definitions for the header identification. This field is contained
 * contains the highest 3 bits of the H_SEQ_LENGTH byte.
 * @{
 */
/**
 * \brief RRT
 *
 * Header identification says receive radio telegram (RRT)
 */
#define C_H_SEQ_RRT 0x00
/**
 * \brief TRT
 *
 * Header identification says transmit radio telegram (TRT)
 */
#define C_H_SEQ_TRT 0x60
/**
 * \brief RMT
 *
 * Header identification says receive message telegram (RMT)
 */
#define C_H_SEQ_RMT 0x80
/**
 * \brief TCT
 *
 * Header identification says transmit command telegram (TCT)
 */
#define C_H_SEQ_TCT 0xA0
/**
 * \brief OK
 *
 * Standard message to confirm that an action was performed correctly by the TCM
 */
#define H_SEQ_OK 0x80
/**
 * \brief ERR
 *
 * Standard error message response if an action was not performed correctly by the TCM
 */
#define H_SEQ_ERR 0x80
/*@}*/

/**
 * @defgroup length Length byte
 * Number of octets following the header octed.
 * Is contained in the last 5 bits of the H_SEQ_LENGTH byte.
 * @{
 */
/**
 * \brief Fixed length
 *
 * Every packet has the same length: 0x0B
 */
#define C_LENGTH 0x0B
/*@}*/

/**
 * @defgroup org Type of telegram
 * Type definition of the telegram.
 * @{
 */
/**
 * \brief PTM telegram
 *
 * Telegram from a PTM switch module received (original or repeated message)
 */
#define C_ORG_RPS 0x05
/**
 * \brief 1 byte data telegram
 *
 * Detailed 1 byte data telegram from a STM sensor module received (original or repeated message)
 */
#define C_ORG_1BS 0x06
/**
 * \brief 4 byte data telegram
 *
 * Detailed 4 byte data telegram from a STM sensor module received (original or repeated message)
 */
#define C_ORG_4BS 0x07
/**
 * \brief CTM telegram
 *
 * Telegram from a CTM module received (original or repeated message)
 */
#define C_ORG_HRC 0x08
/**
 * \brief Modem telegram
 *
 * 6byte Modem Telegram (original or repeated)
 */
#define C_ORG_6DT 0x0A
/**
 * \brief Modem ack
 *
 * Modem Acknowledge Telegram
 */
#define C_ORG_MDA 0x0B
/*@}*/


/**
 * \brief ID-range telegram
 *
 * When this command is sent to the TCM, the base ID range number is retrieved though an INF_IDBASE telegram.
 */
#define C_ORG_RD_IDBASE 0x58

/**
 * \brief Reset the TCM 120 module
 *
 * Performs a reset of the TCM microcontroller. When the TCM is
 * ready to operate again, it sends an ASCII message (INF_INIT)
 * containing the current settings.
 */
#define C_ORG_RESET 0x0A

/**
 * \brief Actual ID range base
 *
 * This message informs the user about the ID range base number.
 * IDBaseByte3 is the most significant byte.
 */
#define C_ORG_INF_IDBASE 0x98

#define C_ORG_RD_SW_VER 0x4B
#define C_ORG_INF_SW_VER 0x8C

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
 * @{
 */
#define DB3_RPS_NU_RID 0xC0
#define DB3_RPS_NU_RID_SHIFT 6
#define DB3_RPS_NU_UD  0x20
#define DB3_RPS_NU_UD_SHIFT 5
#define DB3_RPS_NU_PR  0x10
#define DB3_RPS_NU_PR_SHIFT 4
#define DB3_RPS_NU_SRID 0x0C
#define DB3_RPS_NU_SRID_SHIFT 2
#define DB3_RPS_NU_SUD 0x02
#define DB3_RPS_NU_SUD_SHIFT 1
#define DB3_RPS_NU_SA 0x01
#define DB3_RPS_NU_SA_SHIFT 0
/*@}*/

/**
 * @defgroup data3_1 Meaning of data_byte 3 (for RPS telegrams, NU = 0)
 * Bitmasks for the data_byte3-field, if ORG = RPS and NU = 0.
 * @{
 */
#define DB3_RPS_BUTTONS 0xE0
#define DB3_RPS_BUTTONS_SHIFT 4
#define DB3_RPS_PR 0x10
#define DB3_RPS_PR_SHIFT 3
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

/**
 * @defgroup Definitions for the string representation
 * The definitions for the human-readable string representation
 * @{
 */
#define HR_TYPE "Type: "
#define HR_RPS  "RPS "
#define HR_1BS  "1BS "
#define HR_4BS  "4BS "
#define HR_HRC  "HRC "
#define HR_6DT  "6DT "
#define HR_MDA  "MDA "
#define HR_DATA " Data: "
#define HR_SENDER "Sender: "
#define HR_STATUS " Status: "
#define HR_IDBASE "ID_Base: "
#define HR_SOFTWAREVERSION "Software: "
#define HR_TYPEUNKN "unknown "
/**
 * @}
 */

CEnOcean::CEnOcean(const int ID, const std::string& devname, const int type)
{
	m_HwdID=ID;
	m_szSerialPort=devname;
    m_Type = type;
	m_bufferpos=0;
	memset(&m_buffer,0,sizeof(m_buffer));
	m_id_base=0;
	m_receivestate=ERS_SYNC1;
	m_stoprequested=false;
}

CEnOcean::~CEnOcean()
{
	clearReadCallback();
}

bool CEnOcean::StartHardware()
{
	m_retrycntr=ENOCEAN_RETRY_DELAY*5; //will force reconnect first thing

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CEnOcean::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CEnOcean::StopHardware()
{
	m_stoprequested=true;
	m_thread->join();
    // Wait a while. The read thread might be reading. Adding this prevents a pointer error in the async serial class.
    sleep_milliseconds(10);
	if (isOpen())
	{
		try {
			clearReadCallback();
			close();
			doClose();
			setErrorStatus(true);
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted=false;
	return true;
}


void CEnOcean::Do_Work()
{
	while (!m_stoprequested)
	{
		sleep_milliseconds(200);
		if (m_stoprequested)
			break;
		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"EnOcean: serial retrying in %d seconds...", ENOCEAN_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr/5>=ENOCEAN_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_bufferpos=0;
				OpenSerialDevice();
			}
		}
		if (m_sendqueue.size()>0)
		{
			boost::lock_guard<boost::mutex> l(m_sendMutex);

			std::vector<std::string>::iterator itt=m_sendqueue.begin();
			if (itt!=m_sendqueue.end())
			{
				std::string sBytes=*itt;
				write(sBytes.c_str(),sBytes.size());
				m_sendqueue.erase(itt);
			}
		}
	}
	_log.Log(LOG_NORM,"EnOcean: Serial Worker stopped...");
}

void CEnOcean::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0,pData,length);
	boost::lock_guard<boost::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}

/**
 * returns a clean data structure, filled with 0
 */
enocean_data_structure enocean_clean_data_structure() {
	int i = 0;
	enocean_data_structure ds;
	BYTE* b;
	for (i=0;i < sizeof(ds);i++) {
		b = (BYTE*) &ds + i;
		*b = 0x00;
	}
	return ds;
}

/**
 * Convert a data_structure into a data_structure_6DT
 * Note: There will be no copy of the passed data_structure.
 *   So if you change data in the returned new structure, also
 *   data in the original struct will be changed (pointers!)
 */
enocean_data_structure_6DT* enocean_convert_to_6DT(const enocean_data_structure* in) {
	enocean_data_structure_6DT* out;
	// no conversion necessary - just overlay the other struct
	out = (enocean_data_structure_6DT*) in;
	return out;
}

/**
 * Convert a data_structure into a data_structure_MDA
 * Note: There will be no copy of the passed data_structure.
 *   So if you change data in the returned new structure, also
 *   data in the original struct will be changed (pointers!)
 */
enocean_data_structure_MDA* enocean_convert_to_MDA(const enocean_data_structure* in) {
	enocean_data_structure_MDA* out;
	// no conversion necessary - just overlay the other struct
	out = (enocean_data_structure_MDA*) in;
	return out;
}

unsigned char enocean_calc_checksum(const enocean_data_structure *input_data) {
	unsigned char checksum = 0;
	checksum += input_data->H_SEQ_LENGTH;
	checksum += input_data->ORG;
	checksum += input_data->DATA_BYTE3;
	checksum += input_data->DATA_BYTE2;
	checksum += input_data->DATA_BYTE1;
	checksum += input_data->DATA_BYTE0;
	checksum += input_data->ID_BYTE3;
	checksum += input_data->ID_BYTE2;
	checksum += input_data->ID_BYTE1;
	checksum += input_data->ID_BYTE0;
	checksum += input_data->STATUS;
	return checksum;
}

char* enocean_gethex_internal(BYTE* in, const int framesize) {
	char* hexstr = (char*) malloc ((framesize*2)+1);  // because every hex-byte needs 2 characters
	char* tempstr = hexstr;

	int i;
	BYTE* bytearray;
	bytearray = in;
	for (i=0;i<framesize;i++) {
		sprintf(tempstr,"%02x",bytearray[i]);
		tempstr += 2;
	}
	return hexstr;
}


char* enocean_hexToHuman(const enocean_data_structure *pFrame)
{
	const int framesize = sizeof(enocean_data_structure);
	// every byte of the frame takes 2 characters in the human representation + the length of the text blocks (without trailing '\0');
	const int stringsize = (framesize*2)+1+sizeof(HR_TYPE)-1+sizeof(HR_RPS)-1+sizeof(HR_DATA)-1+sizeof(HR_SENDER)-1+sizeof(HR_STATUS)-1;
	char *humanString = (char*) malloc (stringsize);
	char *tempstring = humanString;
	char *temphexstring;
	sprintf(tempstring,HR_TYPE);
	tempstring += sizeof(HR_TYPE)-1;

	enocean_data_structure_6DT* frame_6DT;
	enocean_data_structure_MDA* frame_MDA;

	// Now it depends on ORG what to do
	switch (pFrame->ORG) {
	case C_ORG_INF_IDBASE:
		sprintf(tempstring,HR_IDBASE);
		tempstring += sizeof(HR_IDBASE)-1;
		sprintf(tempstring,"0x%02x%02x%02x%02x",pFrame->DATA_BYTE3,pFrame->DATA_BYTE2,pFrame->DATA_BYTE1,pFrame->DATA_BYTE0);
		tempstring += 10;
		//printf("TCM120 Module initialized");
		break;
	case C_ORG_INF_SW_VER:
		sprintf(tempstring,HR_SOFTWAREVERSION);
		tempstring += sizeof(HR_SOFTWAREVERSION)-1;
		sprintf(tempstring,"0x%02x%02x%02x%02x",pFrame->ID_BYTE3,pFrame->ID_BYTE2,pFrame->ID_BYTE1,pFrame->ID_BYTE0);
		tempstring += 10;
		break;
	case C_ORG_RPS: // RBS received
	case C_ORG_4BS:
	case C_ORG_1BS:
	case C_ORG_HRC:
		switch (pFrame->ORG) {
		case C_ORG_RPS: // RBS received
			sprintf(tempstring,HR_RPS);
			tempstring += sizeof(HR_RPS)-1;
			break;
		case C_ORG_4BS:
			sprintf(tempstring,HR_4BS);
			tempstring += sizeof(HR_4BS)-1;
			break;
		case C_ORG_1BS:
			sprintf(tempstring,HR_1BS);
			tempstring += sizeof(HR_1BS)-1;
			break;
		case C_ORG_HRC:
			sprintf(tempstring,HR_HRC);
			tempstring += sizeof(HR_HRC)-1;
			break;
		}
		sprintf(tempstring,HR_SENDER);
		tempstring += sizeof(HR_SENDER)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->ID_BYTE3), 4);
		strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 8;  // we converted 4 bytes and each one takes 2 chars
		sprintf(tempstring,HR_DATA);
		tempstring += sizeof(HR_DATA)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->DATA_BYTE3), 4);
		strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 8;  // we converted 4 bytes and each one takes 2 chars
		break;
	case C_ORG_6DT:
		sprintf(tempstring,HR_6DT);
		frame_6DT = enocean_convert_to_6DT(pFrame);
		tempstring += sizeof(HR_6DT)-1;
		sprintf(tempstring,HR_SENDER);
		tempstring += sizeof(HR_SENDER)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_6DT->ADDRESS1), 2);
		strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 4;
		sprintf(tempstring,HR_DATA);
		tempstring += sizeof(HR_DATA)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_6DT->DATA_BYTE5), 6);
		strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 12;
		break;
	case C_ORG_MDA:
		sprintf(tempstring,HR_MDA);
		frame_MDA = enocean_convert_to_MDA(pFrame);
		tempstring += sizeof(HR_MDA)-1;
		sprintf(tempstring,HR_SENDER);
		tempstring += sizeof(HR_SENDER)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_MDA->ADDRESS1), 2);
		strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 4;
		break;
	default:
		sprintf(tempstring,HR_TYPEUNKN);
		tempstring += sizeof(HR_TYPEUNKN)-1;
		break;
	}
	sprintf(tempstring,HR_STATUS);
	tempstring += sizeof(HR_STATUS)-1;
	temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->STATUS), 1);
	strcpy(tempstring,temphexstring);
	free(temphexstring);
	tempstring += 2;
	return humanString;
}

enocean_data_structure create_base_frame() {
	enocean_data_structure returnvalue = enocean_clean_data_structure();
	returnvalue.SYNC_BYTE1 = C_S_BYTE1;
	returnvalue.SYNC_BYTE2 = C_S_BYTE2;
	returnvalue.H_SEQ_LENGTH = C_H_SEQ_TCT | C_LENGTH;
	return returnvalue;
}

enocean_data_structure tcm120_reset() {
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RESET;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_rd_idbase() {
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RD_IDBASE;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_rd_sw_ver() {
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RD_SW_VER;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_create_inf_packet() {
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.H_SEQ_LENGTH = 0x8B;
	returnvalue.ORG = 0x89;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

bool CEnOcean::OpenSerialDevice()
{
	//Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 9600); //ECP2 protocol, for ECP3 open with 57600
		_log.Log(LOG_NORM,"EnOcean: Using serial port: %s", m_szSerialPort.c_str());
	}
	catch (boost::exception & e)
	{
		_log.Log(LOG_ERROR,"EnOcean: Error opening serial port!");
#ifdef _DEBUG
		_log.Log(LOG_ERROR,"-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#endif
		return false;
	}
	catch ( ... )
	{
		_log.Log(LOG_ERROR,"EnOcean: Error opening serial port!!!");
		return false;
	}
	m_bIsStarted=true;
	m_receivestate=ERS_SYNC1;
	setReadCallback(boost::bind(&CEnOcean::readCallback, this, _1, _2));
	sOnConnected(this);

	enocean_data_structure iframe;
/*
	//Send Initial Reset
	iframe = tcm120_reset();
	write((const char*)&iframe,sizeof(enocean_data_structure));
	sleep_seconds(1);
*/

	iframe = tcm120_rd_idbase();
	write((const char*)&iframe,sizeof(enocean_data_structure));

	return true;
}

void CEnOcean::readCallback(const char *data, size_t len)
{
	boost::lock_guard<boost::mutex> l(readQueueMutex);
	size_t ii=0;
	while (ii<len)
	{
		const unsigned char c = data[ii];

		switch (m_receivestate)
		{
		case ERS_SYNC1:
			if (c!=C_S_BYTE1)
				return;
			m_receivestate = ERS_SYNC2;
			break;
		case ERS_SYNC2:
			if (c!=C_S_BYTE2)
			{
				m_receivestate = ERS_SYNC1;
				return;
			}
			m_receivestate = ERS_LENGTH;
			break;
		case ERS_LENGTH:
			m_buffer[0] = C_S_BYTE1;
			m_buffer[1] = C_S_BYTE2;
			m_buffer[2] = c;
			m_wantedlength=(c&0x0F)+3;
			m_bufferpos=3;
			m_receivestate = ERS_DATA;
			break;
		case ERS_DATA:
			m_buffer[m_bufferpos++] = c;
			if (m_bufferpos>=m_wantedlength-1)
			{
				m_receivestate = ERS_CHECKSUM;
			}
			break;
		case ERS_CHECKSUM:
			m_buffer[m_bufferpos++] = c;
			if (m_buffer[m_bufferpos-1]==enocean_calc_checksum((const enocean_data_structure*)&m_buffer))
			{
				ParseData();
			}
			else
			{
				_log.Log(LOG_ERROR,"EnOcean: Frame Checksum Error!...");
			}
			m_receivestate = ERS_SYNC1;
			break;
		}
		ii++;
	}
}

void CEnOcean::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (m_id_base==0)
		return;
	if (isOpen()) {

		RBUF *tsen=(RBUF*)pdata;
		if (tsen->LIGHTING2.packettype!=pTypeLighting2)
			return; //only allowed to control switches

		enocean_data_structure iframe = create_base_frame();
		iframe.H_SEQ_LENGTH=0x6B;//TX+Length
		iframe.ORG = 0x05;

		unsigned long sID=(tsen->LIGHTING2.id1<<24)|(tsen->LIGHTING2.id2<<16)|(tsen->LIGHTING2.id3<<8)|tsen->LIGHTING2.id4;
		if ((sID<m_id_base)||(sID>m_id_base+129))
		{
			_log.Log(LOG_ERROR,"EnOcean: Can not switch with this DeviceID, use a switch created with our id_base!...");
			return;
		}

		iframe.ID_BYTE3=(unsigned char)((sID&0xFF000000)>>24);//tsen->LIGHTING2.id1;
		iframe.ID_BYTE2=(unsigned char)((sID&0x00FF0000)>>16);//tsen->LIGHTING2.id2;
		iframe.ID_BYTE1=(unsigned char)((sID&0x0000FF00)>>8);//tsen->LIGHTING2.id3;
		iframe.ID_BYTE0=(unsigned char)(sID&0x0000FF);//tsen->LIGHTING2.id4;

		unsigned char RockerID=0;
		unsigned char UpDown=1;
		unsigned char Pressed=1;

		if (tsen->LIGHTING2.unitcode<10)
			RockerID=tsen->LIGHTING2.unitcode-1;
		else
			return;//double not supported yet!
		UpDown=((tsen->LIGHTING2.cmnd!=light2_sOff)&&(tsen->LIGHTING2.cmnd!=light2_sGroupOff));
			

		iframe.DATA_BYTE3 = (RockerID<<DB3_RPS_NU_RID_SHIFT) | (UpDown<<DB3_RPS_NU_UD_SHIFT) | (Pressed<<DB3_RPS_NU_PR_SHIFT);//0x30;
		iframe.STATUS = 0x30;

		iframe.CHECKSUM = enocean_calc_checksum(&iframe);

		Add2SendQueue((const char*)&iframe,sizeof(enocean_data_structure));

		//Next command is send a bit later (button release)
		iframe.DATA_BYTE3 = 0;
		iframe.STATUS = 0x20;
		iframe.CHECKSUM = enocean_calc_checksum(&iframe);
		Add2SendQueue((const char*)&iframe,sizeof(enocean_data_structure));
	}
}

float CEnOcean::GetValueRange(const float InValue, const float ScaleMax, const float ScaleMin, const float RangeMax, const float RangeMin)
{
	float vscale=ScaleMax-ScaleMin;
	if (vscale==0)
		return 0.0f;
	float vrange=RangeMax-RangeMin;
	if (vrange==0)
		return 0.0f;
	float multiplyer=vscale/vrange;
	return multiplyer*(InValue-RangeMin)+ScaleMin;
}

bool CEnOcean::ParseData()
{
	enocean_data_structure *pFrame=(enocean_data_structure*)&m_buffer;
	unsigned char Checksum=enocean_calc_checksum(pFrame);
	if (Checksum!=pFrame->CHECKSUM)
		return false; //checksum Mismatch!

	long id = (pFrame->ID_BYTE3 << 24) + (pFrame->ID_BYTE2 << 16) + (pFrame->ID_BYTE1 << 8) + pFrame->ID_BYTE0;
	char szDeviceID[20];
	sprintf(szDeviceID,"%08X",id);

	//Handle possible OK/Errors
	bool bStopProcessing=false;
	if (pFrame->H_SEQ_LENGTH==0x8B)
	{
		switch (pFrame->ORG)
		{
		case 0x58:
			//OK
#ifdef _DEBUG
			_log.Log(LOG_NORM,"EnOcean: OK");
#endif
			bStopProcessing=true;
			break;
		case 0x28:
			_log.Log(LOG_ERROR,"EnOcean: ERR_MODEM_NOTWANTEDACK");
			bStopProcessing=true;
			break;
		case 0x29:
			_log.Log(LOG_ERROR,"EnOcean: ERR_MODEM_NOTACK");
			bStopProcessing=true;
			break;
		case 0x0C:
			_log.Log(LOG_ERROR,"EnOcean: ERR_MODEM_DUP_ID");
			bStopProcessing=true;
			break;
		case 0x08:
			_log.Log(LOG_ERROR,"EnOcean: Error in H_SEQ");
			bStopProcessing=true;
			break;
		case 0x09:
			_log.Log(LOG_ERROR,"EnOcean: Error in LENGTH");
			bStopProcessing=true;
			break;
		case 0x0A:
			_log.Log(LOG_ERROR,"EnOcean: Error in CHECKSUM");
			bStopProcessing=true;
			break;
		case 0x0B:
			_log.Log(LOG_ERROR,"EnOcean: Error in ORG");
			bStopProcessing=true;
			break;
		case 0x22:
			_log.Log(LOG_ERROR,"EnOcean: ERR_TX_IDRANGE");
			bStopProcessing=true;
			break;
		case 0x1A:
			_log.Log(LOG_ERROR,"EnOcean: ERR_ IDRANGE");
			bStopProcessing=true;
			break;
		}
	}
	if (bStopProcessing)
		return true;

	switch (pFrame->ORG)
	{
	case C_ORG_INF_IDBASE:
		m_id_base = (pFrame->DATA_BYTE3 << 24) + (pFrame->DATA_BYTE2 << 16) + (pFrame->DATA_BYTE1 << 8) + pFrame->DATA_BYTE0;
		_log.Log(LOG_NORM,"EnOcean: Transceiver ID_Base: 0x%08x",m_id_base);
		break;
	case C_ORG_RPS:
		if (pFrame->STATUS & S_RPS_NU) {
			//Rocker
			// NU == 1, N-Message
			unsigned char RockerID=(pFrame->DATA_BYTE3 & DB3_RPS_NU_RID) >> DB3_RPS_NU_RID_SHIFT;
			unsigned char UpDown=(pFrame->DATA_BYTE3 & DB3_RPS_NU_UD) >> DB3_RPS_NU_UD_SHIFT;
			unsigned char Pressed=(pFrame->DATA_BYTE3 & DB3_RPS_NU_PR)>>DB3_RPS_NU_PR_SHIFT;
			unsigned char SecondRockerID=(pFrame->DATA_BYTE3 & DB3_RPS_NU_SRID)>>DB3_RPS_NU_SRID_SHIFT;
			unsigned char SecondUpDown=(pFrame->DATA_BYTE3 & DB3_RPS_NU_SUD)>>DB3_RPS_NU_SUD_SHIFT;
			unsigned char SecondAction=(pFrame->DATA_BYTE3 & DB3_RPS_NU_SA)>>DB3_RPS_NU_SA_SHIFT;
#ifdef _DEBUG
			_log.Log(LOG_NORM,"Received RPS N-Message Node 0x%08x Rocker ID: %i UD: %i Pressed: %i Second Rocker ID: %i SUD: %i Second Action: %i",
				id,
				RockerID, 
				UpDown, 
				Pressed,
				SecondRockerID, 
				SecondUpDown,
				SecondAction);
#endif
			//We distinguish 3 types of buttons from a switch: Left/Right/Left+Right
			if (Pressed==1)
			{
				RBUF tsen;
				memset(&tsen,0,sizeof(RBUF));
				tsen.LIGHTING2.packetlength=sizeof(tsen.LIGHTING2)-1;
				tsen.LIGHTING2.packettype=pTypeLighting2;
				tsen.LIGHTING2.subtype=sTypeAC;
				tsen.LIGHTING2.seqnbr=0;
				tsen.LIGHTING2.id1=(BYTE)pFrame->ID_BYTE3;
				tsen.LIGHTING2.id2=(BYTE)pFrame->ID_BYTE2;
				tsen.LIGHTING2.id3=(BYTE)pFrame->ID_BYTE1;
				tsen.LIGHTING2.id4=(BYTE)pFrame->ID_BYTE0;
				tsen.LIGHTING2.level=0;
				tsen.LIGHTING2.rssi=12;

				if (SecondAction==0)
				{
					//Left/Right Up/Down
					tsen.LIGHTING2.unitcode=RockerID+1;
					tsen.LIGHTING2.cmnd=(UpDown==1)?light2_sOn:light2_sOff;
				}
				else
				{
					//Left+Right Up/Down
					tsen.LIGHTING2.unitcode=SecondRockerID+10;
					tsen.LIGHTING2.cmnd=(SecondUpDown==1)?light2_sOn:light2_sOff;
				}
				sDecodeRXMessage(this, (const unsigned char *)&tsen.LIGHTING2);
			}
		}
		break;
	case C_ORG_4BS:
		{
			if ((pFrame->DATA_BYTE0 & 0x08) == 0)
			{
				if (pFrame->DATA_BYTE0 & 0x80)
				{
					//Tech in datagram

					//DB3		DB3/2	DB2/1			DB0
					//Profile	Type	Manufacturer-ID	LRN Type	RE2		RE1
					//6 Bit		7 Bit	11 Bit			1Bit		1Bit	1Bit	1Bit	1Bit	1Bit	1Bit	1Bit

					int manufacturer = ((pFrame->DATA_BYTE2 & 7) << 8) | pFrame->DATA_BYTE1;
					int profile = pFrame->DATA_BYTE3 >> 2;
					int ttype = ((pFrame->DATA_BYTE3 & 3) << 5) | (pFrame->DATA_BYTE2 >> 3);
					_log.Log(LOG_NORM,"EnOcean: 4BS, Teach-in diagram: Sender_ID: 0x%08X\nManufacturer: 0x%02x (%s)\nProfile: 0x%02X\nType: 0x%02X (%s)", 
						id, manufacturer,Get_EnoceanManufacturer(manufacturer),
						profile,ttype,Get_Enocean4BSType(0xA5,profile,ttype));


					std::stringstream szQuery;
					std::vector<std::vector<std::string> > result;
					szQuery << "SELECT ID FROM EnoceanSensors WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szDeviceID << "')";
					result=m_pMainWorker->m_sql.query(szQuery.str());
					if (result.size()<1)
					{
						//Add it to the database
						szQuery.clear();
						szQuery.str("");
						szQuery << "INSERT INTO EnoceanSensors (HardwareID, DeviceID, Manufacturer, Profile, [Type]) VALUES (" << m_HwdID << ",'" << szDeviceID << "'," << manufacturer << "," << profile << "," << ttype << ")";
						result=m_pMainWorker->m_sql.query(szQuery.str());
					}

				}
			}
			else
			{
				//Following sensors need to have had a teach-in
				std::stringstream szQuery;
				std::vector<std::vector<std::string> > result;
				szQuery << "SELECT ID, Manufacturer, Profile, [Type] FROM EnoceanSensors WHERE (HardwareID==" << m_HwdID << ") AND (DeviceID=='" << szDeviceID << "')";
				result=m_pMainWorker->m_sql.query(szQuery.str());
				if (result.size()<1)
				{
					char *pszHumenTxt=enocean_hexToHuman(pFrame);
					_log.Log(LOG_NORM,"EnOcean: Need Teach-In for %s", pszHumenTxt);
					free(pszHumenTxt);
					return true;
				}
				int Manufacturer=atoi(result[0][1].c_str());
				int Profile=atoi(result[0][2].c_str());
				int iType=atoi(result[0][3].c_str());

				const std::string szST=Get_Enocean4BSType(0xA5,Profile,iType);

				if (szST=="AMR.Counter")
				{
					//0xA5, 0x12, 0x00, "Counter"
					unsigned long cvalue=(pFrame->DATA_BYTE3<<16)|(pFrame->DATA_BYTE2<<8)|(pFrame->DATA_BYTE1);
					RBUF tsen;
					memset(&tsen,0,sizeof(RBUF));
					tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
					tsen.RFXMETER.packettype=pTypeRFXMeter;
					tsen.RFXMETER.subtype=sTypeRFXMeterCount;
					tsen.RFXMETER.rssi=12;
					tsen.RFXMETER.id1=pFrame->ID_BYTE2;
					tsen.RFXMETER.id2=pFrame->ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE)((cvalue & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE)((cvalue & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE)((cvalue & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE)(cvalue & 0x000000FF);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);
				}
				else if (szST=="AMR.Electricity")
				{
					//0xA5, 0x12, 0x01, "Electricity"
					int cvalue=(pFrame->DATA_BYTE3<<16)|(pFrame->DATA_BYTE2<<8)|(pFrame->DATA_BYTE1);
					_tUsageMeter umeter;
					umeter.id1=(BYTE)pFrame->ID_BYTE3;
					umeter.id2=(BYTE)pFrame->ID_BYTE2;
					umeter.id3=(BYTE)pFrame->ID_BYTE1;
					umeter.id4=(BYTE)pFrame->ID_BYTE0;
					umeter.dunit=1;
					umeter.fusage=(float)cvalue;
					sDecodeRXMessage(this, (const unsigned char *)&umeter);
				}
				else if (szST=="AMR.Gas")
				{
					//0xA5, 0x12, 0x02, "Gas"
					unsigned long cvalue=(pFrame->DATA_BYTE3<<16)|(pFrame->DATA_BYTE2<<8)|(pFrame->DATA_BYTE1);
					RBUF tsen;
					memset(&tsen,0,sizeof(RBUF));
					tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
					tsen.RFXMETER.packettype=pTypeRFXMeter;
					tsen.RFXMETER.subtype=sTypeRFXMeterCount;
					tsen.RFXMETER.rssi=12;
					tsen.RFXMETER.id1=pFrame->ID_BYTE2;
					tsen.RFXMETER.id2=pFrame->ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE)((cvalue & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE)((cvalue & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE)((cvalue & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE)(cvalue & 0x000000FF);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);
				}
				else if (szST=="AMR.Water")
				{
					//0xA5, 0x12, 0x03, "Water"
					unsigned long cvalue=(pFrame->DATA_BYTE3<<16)|(pFrame->DATA_BYTE2<<8)|(pFrame->DATA_BYTE1);
					RBUF tsen;
					memset(&tsen,0,sizeof(RBUF));
					tsen.RFXMETER.packetlength=sizeof(tsen.RFXMETER)-1;
					tsen.RFXMETER.packettype=pTypeRFXMeter;
					tsen.RFXMETER.subtype=sTypeRFXMeterCount;
					tsen.RFXMETER.rssi=12;
					tsen.RFXMETER.id1=pFrame->ID_BYTE2;
					tsen.RFXMETER.id2=pFrame->ID_BYTE1;
					tsen.RFXMETER.count1 = (BYTE)((cvalue & 0xFF000000) >> 24);
					tsen.RFXMETER.count2 = (BYTE)((cvalue & 0x00FF0000) >> 16);
					tsen.RFXMETER.count3 = (BYTE)((cvalue & 0x0000FF00) >> 8);
					tsen.RFXMETER.count4 = (BYTE)(cvalue & 0x000000FF);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXMETER);
				}
				else if (szST.find("RoomOperatingPanel") == 0)
				{
					if (iType<0x0E)
					{
						// Room Sensor and Control Unit (EEP A5-10-01 ... A5-10-0D)
						// [Eltako FTR55D, FTR55H, Thermokon SR04 *, Thanos SR *, untested]
						// pFrame->DATA_BYTE3 is the fan speed or night reduction for Eltako
						// pFrame->DATA_BYTE2 is the setpoint where 0x00 = min ... 0xFF = max or
						// reference temperature for Eltako where 0x00 = 0°C ... 0xFF = 40°C
						// pFrame->DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
						// pFrame->DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch
						float temp=GetValueRange(pFrame->DATA_BYTE1,0,40);
						if (Manufacturer == 0x0D) 
						{
							//Eltako
							int nightReduction = 0;
							if (pFrame->DATA_BYTE3 == 0x06)
								nightReduction = 1;
							else if (pFrame->DATA_BYTE3 == 0x0C)
								nightReduction = 2;
							else if (pFrame->DATA_BYTE3 == 0x13)
								nightReduction = 3;
							else if (pFrame->DATA_BYTE3 == 0x19)
								nightReduction = 4;
							else if (pFrame->DATA_BYTE3 == 0x1F)
								nightReduction = 5;
							float setpointTemp=GetValueRange(pFrame->DATA_BYTE2,40);
						}
						else 
						{
							int fspeed = 3;
							if (pFrame->DATA_BYTE3 >= 145)
								fspeed = 2;
							else if (pFrame->DATA_BYTE3 >= 165)
								fspeed = 1;
							else if (pFrame->DATA_BYTE3 >= 190)
								fspeed = 0;
							else if (pFrame->DATA_BYTE3 >= 210)
								fspeed = -1; //auto
							int iswitch = pFrame->DATA_BYTE0 & 1;
						}
						RBUF tsen;
						memset(&tsen,0,sizeof(RBUF));
						tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
						tsen.TEMP.packettype=pTypeTEMP;
						tsen.TEMP.subtype=sTypeTEMP10;
						tsen.TEMP.id1=pFrame->ID_BYTE2;
						tsen.TEMP.id2=pFrame->ID_BYTE1;
						tsen.TEMP.battery_level=pFrame->ID_BYTE0&0x0F;
						tsen.TEMP.rssi=(pFrame->ID_BYTE0&0xF0)>>4;

						tsen.TEMP.tempsign=(temp>=0)?0:1;
						int at10=round(abs(temp*10.0f));
						tsen.TEMP.temperatureh=(BYTE)(at10/256);
						at10-=(tsen.TEMP.temperatureh*256);
						tsen.TEMP.temperaturel=(BYTE)(at10);
						sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);
					}
				}
				else if (szST == "LightSensor.01")
				{
					// Light Sensor (EEP A5-06-01)
					// [Eltako FAH60, FAH63, FIH63, Thermokon SR65 LI, untested]
					// pFrame->DATA_BYTE3 is the voltage where 0x00 = 0 V ... 0xFF = 5.1 V
					// pFrame->DATA_BYTE3 is the low illuminance for Eltako devices where
					// min 0x00 = 0 lx, max 0xFF = 100 lx, if pFrame->DATA_BYTE2 = 0
					// pFrame->DATA_BYTE2 is the illuminance (ILL2) where min 0x00 = 300 lx, max 0xFF = 30000 lx
					// pFrame->DATA_BYTE1 is the illuminance (ILL1) where min 0x00 = 600 lx, max 0xFF = 60000 lx
					// pFrame->DATA_BYTE0_bit_0 is Range select where 0 = ILL1, 1 = ILL2
					float lux =0;
					if (Manufacturer == 0x0D)
					{
						if(pFrame->DATA_BYTE2 == 0) {
							lux=GetValueRange(pFrame->DATA_BYTE3,100);
						} else {
							lux=GetValueRange(pFrame->DATA_BYTE2,30000,300);
						}
					} else {
						float voltage=GetValueRange(pFrame->DATA_BYTE3,5100); //mV
						if(pFrame->DATA_BYTE0 & 1) {
							lux=GetValueRange(pFrame->DATA_BYTE2,30000,300);
						} else {
							lux=GetValueRange(pFrame->DATA_BYTE1,60000,600);
						}
						RBUF tsen;
						memset(&tsen,0,sizeof(RBUF));
						tsen.RFXSENSOR.packetlength=sizeof(tsen.RFXSENSOR)-1;
						tsen.RFXSENSOR.packettype=pTypeRFXSensor;
						tsen.RFXSENSOR.subtype=sTypeRFXSensorVolt;
						tsen.RFXSENSOR.id=pFrame->ID_BYTE1;
						tsen.RFXSENSOR.filler=pFrame->ID_BYTE0&0x0F;
						tsen.RFXSENSOR.rssi=(pFrame->ID_BYTE0&0xF0)>>4;
						tsen.RFXSENSOR.msg1 = (BYTE)(voltage/256);
						tsen.RFXSENSOR.msg2 = (BYTE)(voltage-(tsen.RFXSENSOR.msg1*256));
						sDecodeRXMessage(this, (const unsigned char *)&tsen.RFXSENSOR);
					}
					_tLightMeter lmeter;
					lmeter.id1=(BYTE)pFrame->ID_BYTE3;
					lmeter.id2=(BYTE)pFrame->ID_BYTE2;
					lmeter.id3=(BYTE)pFrame->ID_BYTE1;
					lmeter.id4=(BYTE)pFrame->ID_BYTE0;
					lmeter.dunit=1;
					lmeter.fLux=lux;
					sDecodeRXMessage(this, (const unsigned char *)&lmeter);
				}
				else if (szST.find("Temperature")==0)
				{
					//(EPP A5-02 01/30)
					float ScaleMax=0;
					float ScaleMin=0;
					if (iType==0x01) { ScaleMax=-40; ScaleMin=0; }
					else if (iType==0x02) { ScaleMax=-30; ScaleMin=10; }
					else if (iType==0x03) { ScaleMax=-20; ScaleMin=20; }
					else if (iType==0x04) { ScaleMax=-10; ScaleMin=30; }
					else if (iType==0x05) { ScaleMax=0; ScaleMin=40; }
					else if (iType==0x06) { ScaleMax=10; ScaleMin=50; }
					else if (iType==0x07) { ScaleMax=20; ScaleMin=60; }
					else if (iType==0x08) { ScaleMax=30; ScaleMin=70; }
					else if (iType==0x09) { ScaleMax=40; ScaleMin=80; }
					else if (iType==0x0A) { ScaleMax=50; ScaleMin=90; }
					else if (iType==0x0B) { ScaleMax=60; ScaleMin=100; }
					else if (iType==0x10) { ScaleMax=-60; ScaleMin=20; }
					else if (iType==0x11) { ScaleMax=-50; ScaleMin=30; }
					else if (iType==0x12) { ScaleMax=-40; ScaleMin=40; }
					else if (iType==0x13) { ScaleMax=-30; ScaleMin=50; }
					else if (iType==0x14) { ScaleMax=-20; ScaleMin=60; }
					else if (iType==0x15) { ScaleMax=-10; ScaleMin=70; }
					else if (iType==0x16) { ScaleMax=0; ScaleMin=80; }
					else if (iType==0x17) { ScaleMax=10; ScaleMin=90; }
					else if (iType==0x18) { ScaleMax=20; ScaleMin=100; }
					else if (iType==0x19) { ScaleMax=30; ScaleMin=110; }
					else if (iType==0x1A) { ScaleMax=40; ScaleMin=120; }
					else if (iType==0x1B) { ScaleMax=50; ScaleMin=130; }
					else if (iType==0x20) { ScaleMax=-10; ScaleMin=41.2f; }
					else if (iType==0x30) { ScaleMax=-40; ScaleMin=62.3f; }

					float temp;
					if (iType<0x20)
						temp=GetValueRange(pFrame->DATA_BYTE1,ScaleMax,ScaleMin);
					else
						temp=GetValueRange(float(((pFrame->DATA_BYTE2&3)<<8)|pFrame->DATA_BYTE1),ScaleMax,ScaleMin); //10bit
					RBUF tsen;
					memset(&tsen,0,sizeof(RBUF));
					tsen.TEMP.packetlength=sizeof(tsen.TEMP)-1;
					tsen.TEMP.packettype=pTypeTEMP;
					tsen.TEMP.subtype=sTypeTEMP10;
					tsen.TEMP.id1=pFrame->ID_BYTE2;
					tsen.TEMP.id2=pFrame->ID_BYTE1;
					tsen.TEMP.battery_level=pFrame->ID_BYTE0&0x0F;
					tsen.TEMP.rssi=(pFrame->ID_BYTE0&0xF0)>>4;

					tsen.TEMP.tempsign=(temp>=0)?0:1;
					int at10=round(abs(temp*10.0f));
					tsen.TEMP.temperatureh=(BYTE)(at10/256);
					at10-=(tsen.TEMP.temperatureh*256);
					tsen.TEMP.temperaturel=(BYTE)(at10);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP);
				}
				else if (szST=="TempHum.01")
				{
					//(EPP A5-04-01)
					float temp=GetValueRange(pFrame->DATA_BYTE1,40);
					float hum=GetValueRange(pFrame->DATA_BYTE2,100);
					RBUF tsen;
					memset(&tsen,0,sizeof(RBUF));
					tsen.TEMP_HUM.packetlength=sizeof(tsen.TEMP_HUM)-1;
					tsen.TEMP_HUM.packettype=pTypeTEMP_HUM;
					tsen.TEMP_HUM.subtype=sTypeTH5;
					tsen.TEMP_HUM.rssi=12;
					tsen.TEMP_HUM.id1=pFrame->ID_BYTE2;
					tsen.TEMP_HUM.id2=pFrame->ID_BYTE1;
					tsen.TEMP_HUM.battery_level=9;
					tsen.TEMP_HUM.tempsign=(temp>=0)?0:1;
					int at10=round(abs(temp*10.0f));
					tsen.TEMP_HUM.temperatureh=(BYTE)(at10/256);
					at10-=(tsen.TEMP_HUM.temperatureh*256);
					tsen.TEMP_HUM.temperaturel=(BYTE)(at10);
					tsen.TEMP_HUM.humidity=(BYTE)hum;
					tsen.TEMP_HUM.humidity_status=Get_Humidity_Level(tsen.TEMP_HUM.humidity);
					sDecodeRXMessage(this, (const unsigned char *)&tsen.TEMP_HUM);
				}
			}
		}
		break;
	default:
		{
			char *pszHumenTxt=enocean_hexToHuman(pFrame);
			_log.Log(LOG_NORM,"EnOcean: %s", pszHumenTxt);
			free(pszHumenTxt);
		}
		break;
	}

    return true;
}
