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

#include "hardwaretypes.h"
#include "EnOceanESP2.h"

#define ENOCEAN_RETRY_DELAY 30

#define bitrange(data, shift, mask) ((data >> shift) & mask)

#define round(a) ((int) (a + 0.5))

/**
 * \brief The default structure for EnOcean packets
 *
 * Data structure for RPS, 1BS, 4BS and HRC packages
 * Since most of the packages are in this format, this
 * is taken as default. Packages of other structure have
 * to be converted with the appropriate functions.
 **/
typedef struct enocean_data_structure {
	unsigned char SYNC_BYTE1; ///< Synchronization Byte 1
	unsigned char SYNC_BYTE2; ///< Synchronization Byte 2
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
	unsigned char SYNC_BYTE1; ///< Synchronization Byte 1
	unsigned char SYNC_BYTE2; ///< Synchronization Byte 2
	unsigned char H_SEQ_LENGTH; ///< Header identification and number of octets following the header octet
	unsigned char ORG; ///< Type of telegram
	unsigned char DATA_BYTE5; ///< Data Byte 5
	unsigned char DATA_BYTE4; ///< Data Byte 4
	unsigned char DATA_BYTE3; ///< Data Byte 3
	unsigned char DATA_BYTE2; ///< Data Byte 2
	unsigned char DATA_BYTE1; ///< Data Byte 1
	unsigned char DATA_BYTE0; ///< Data Byte 0
	unsigned char ADDRESS1; ///< Address Byte 1
	unsigned char ADDRESS0; ///< Address Byte 0
	unsigned char STATUS; ///< Status field
	unsigned char CHECKSUM; ///< Checksum of the packet
} enocean_data_structure_6DT;

/// MDA Package structure
/** Data structure for MDA packages
 **/
typedef struct enocean_data_structure_MDA {
	unsigned char SYNC_BYTE1; ///< Synchronization Byte 1
	unsigned char SYNC_BYTE2; ///< Synchronization Byte 2
	unsigned char H_SEQ_LENGTH; ///< Header identification and number of octets following the header octet
	unsigned char ORG; ///< Type of telegram
	unsigned char DATA_UNUSED5; ///< Data Byte 5 (unused)
	unsigned char DATA_UNUSED4; ///< Data Byte 4 (unused)
	unsigned char DATA_UNUSED3; ///< Data Byte 3 (unused)
	unsigned char DATA_UNUSED2; ///< Data Byte 2 (unused)
	unsigned char ADDRESS1; ///< Address Byte 1
	unsigned char ADDRESS0; ///< Address Byte 0
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
 * 6byte Modem Telegram (original or repeated message)
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
* There are two definitions for every bit mask.
* First, the bit mask itself and also the number of necessary shifts.
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

CEnOceanESP2::CEnOceanESP2(const int ID, const std::string& devname, const int type)
{
	m_HwdID = ID;
	m_szSerialPort = devname;
	m_Type = type;
	m_bufferpos = 0;
	memset(&m_buffer, 0, sizeof(m_buffer));
	m_id_base = 0;
	m_receivestate = ERS_SYNC1;
}

bool CEnOceanESP2::StartHardware()
{
	RequestStart();

	m_retrycntr = ENOCEAN_RETRY_DELAY * 5; // Will force reconnect first thing

	// Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());

	return (m_thread != nullptr);
}

bool CEnOceanESP2::StopHardware()
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


void CEnOceanESP2::Do_Work()
{
	int msec_counter = 0;
	int sec_counter = 0;

	Log(LOG_STATUS, "Worker started...");

	while (!IsStopRequested(200))
	{
		msec_counter++;
		if (msec_counter == 5)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0)
			{
				m_LastHeartbeat = mytime(nullptr);
			}
		}

		if (!isOpen())
		{
			if (m_retrycntr == 0)
			{
				Log(LOG_STATUS, "Serial retrying in %d seconds...", ENOCEAN_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr / 5 >= ENOCEAN_RETRY_DELAY)
			{
				m_retrycntr = 0;
				m_bufferpos = 0;
				OpenSerialDevice();
			}
		}
		if (!m_sendqueue.empty())
		{
			std::lock_guard<std::mutex> l(m_sendMutex);

			auto itt = m_sendqueue.begin();
			if (itt != m_sendqueue.end())
			{
				std::string sBytes = *itt;
				write(sBytes.c_str(), sBytes.size());
				m_sendqueue.erase(itt);
			}
		}
	}
	terminate();

	Log(LOG_STATUS, "Worker stopped...");
}

void CEnOceanESP2::Add2SendQueue(const char* pData, const size_t length)
{
	std::string sBytes;
	sBytes.insert(0, pData, length);
	std::lock_guard<std::mutex> l(m_sendMutex);
	m_sendqueue.push_back(sBytes);
}

/**
 * returns a clean data structure, filled with 0
 */
enocean_data_structure enocean_clean_data_structure() {
	int i = 0;
	enocean_data_structure ds;
	for (i = 0; i < sizeof(ds); i++) {
		BYTE* b = (BYTE*)&ds + i;
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
	// No conversion necessary - just overlay the other struct
	out = (enocean_data_structure_6DT*)in;
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
	// No conversion necessary - just overlay the other struct
	out = (enocean_data_structure_MDA*)in;
	return out;
}

unsigned char enocean_calc_checksum(const enocean_data_structure* input_data) {
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
	char* hexstr = (char*) malloc((framesize * 2) + 1); // Because every hex-byte needs 2 characters
	if (!hexstr)
		return nullptr;
	char* tempstr = hexstr;

	int i;
	BYTE* bytearray;
	bytearray = in;
	for (i = 0; i < framesize; i++) {
		sprintf(tempstr, "%02x", bytearray[i]);
		tempstr += 2;
	}
	return hexstr;
}


char* enocean_hexToHuman(const enocean_data_structure* pFrame)
{
	const int framesize = sizeof(enocean_data_structure);
	// Every byte of the frame takes 2 characters in the human representation + the length of the text blocks (without trailing '\0');
	const int stringsize = (framesize * 2) + 1 + sizeof(HR_TYPE) - 1 + sizeof(HR_RPS) - 1 + sizeof(HR_DATA) - 1 + sizeof(HR_SENDER) - 1 + sizeof(HR_STATUS) - 1;
	char* humanString = (char*) malloc(stringsize);
	if (!humanString)
		return nullptr;
	char* tempstring = humanString;
	char* temphexstring;
	sprintf(tempstring, HR_TYPE);
	tempstring += sizeof(HR_TYPE) - 1;

	enocean_data_structure_6DT* frame_6DT;
	enocean_data_structure_MDA* frame_MDA;

	// Now it depends on ORG what to do
	switch (pFrame->ORG) {
	case C_ORG_INF_IDBASE:
		sprintf(tempstring, HR_IDBASE);
		tempstring += sizeof(HR_IDBASE) - 1;
		sprintf(tempstring, "0x%02x%02x%02x%02x", pFrame->DATA_BYTE3, pFrame->DATA_BYTE2, pFrame->DATA_BYTE1, pFrame->DATA_BYTE0);
		tempstring += 10;
		break;
	case C_ORG_INF_SW_VER:
		sprintf(tempstring, HR_SOFTWAREVERSION);
		tempstring += sizeof(HR_SOFTWAREVERSION) - 1;
		sprintf(tempstring, "0x%02x%02x%02x%02x", pFrame->ID_BYTE3, pFrame->ID_BYTE2, pFrame->ID_BYTE1, pFrame->ID_BYTE0);
		tempstring += 10;
		break;
	case C_ORG_RPS: // RBS received
	case C_ORG_4BS:
	case C_ORG_1BS:
	case C_ORG_HRC:
		switch (pFrame->ORG) {
		case C_ORG_RPS: // RBS received
			sprintf(tempstring, HR_RPS);
			tempstring += sizeof(HR_RPS) - 1;
			break;
		case C_ORG_4BS:
			sprintf(tempstring, HR_4BS);
			tempstring += sizeof(HR_4BS) - 1;
			break;
		case C_ORG_1BS:
			sprintf(tempstring, HR_1BS);
			tempstring += sizeof(HR_1BS) - 1;
			break;
		case C_ORG_HRC:
			sprintf(tempstring, HR_HRC);
			tempstring += sizeof(HR_HRC) - 1;
			break;
		}
		sprintf(tempstring, HR_SENDER);
		tempstring += sizeof(HR_SENDER) - 1;
		temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->ID_BYTE3), 4);
		if (temphexstring)
		{
			strcpy(tempstring, temphexstring);
			free(temphexstring);
			tempstring += 8; // We converted 4 bytes and each one takes 2 chars
		}
		sprintf(tempstring, HR_DATA);
		tempstring += sizeof(HR_DATA) - 1;
		temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->DATA_BYTE3), 4);
		if (temphexstring)
		{
			strcpy(tempstring, temphexstring);
			free(temphexstring);
			tempstring += 8; // We converted 4 bytes and each one takes 2 chars
		}
		break;
	case C_ORG_6DT:
		sprintf(tempstring, HR_6DT);
		frame_6DT = enocean_convert_to_6DT(pFrame);
		tempstring += sizeof(HR_6DT) - 1;
		sprintf(tempstring, HR_SENDER);
		tempstring += sizeof(HR_SENDER) - 1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_6DT->ADDRESS1), 2);
		if (temphexstring)
		{
			strcpy(tempstring, temphexstring);
			free(temphexstring);
			tempstring += 4;
		}
		sprintf(tempstring, HR_DATA);
		tempstring += sizeof(HR_DATA) - 1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_6DT->DATA_BYTE5), 6);
		if (temphexstring)
		{
			strcpy(tempstring, temphexstring);
			free(temphexstring);
			tempstring += 12;
		}
		break;
	case C_ORG_MDA:
		sprintf(tempstring, HR_MDA);
		frame_MDA = enocean_convert_to_MDA(pFrame);
		tempstring += sizeof(HR_MDA) - 1;
		sprintf(tempstring, HR_SENDER);
		tempstring += sizeof(HR_SENDER) - 1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_MDA->ADDRESS1), 2);
		if (temphexstring)
		{
			strcpy(tempstring, temphexstring);
			free(temphexstring);
			tempstring += 4;
		}
		break;
	default:
		sprintf(tempstring, HR_TYPEUNKN);
		tempstring += sizeof(HR_TYPEUNKN) - 1;
		break;
	}
	sprintf(tempstring, HR_STATUS);
	tempstring += sizeof(HR_STATUS) - 1;
	temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->STATUS), 1);
	if (temphexstring)
	{
		strcpy(tempstring, temphexstring);
		free(temphexstring);
		tempstring += 2;
	}
	return humanString;
}

enocean_data_structure create_base_frame()
{
	enocean_data_structure returnvalue = enocean_clean_data_structure();
	returnvalue.SYNC_BYTE1 = C_S_BYTE1;
	returnvalue.SYNC_BYTE2 = C_S_BYTE2;
	returnvalue.H_SEQ_LENGTH = C_H_SEQ_TCT | C_LENGTH;
	return returnvalue;
}

enocean_data_structure tcm120_reset()
{
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RESET;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_rd_idbase()
{
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RD_IDBASE;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_rd_sw_ver()
{
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.ORG = C_ORG_RD_SW_VER;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

enocean_data_structure tcm120_create_inf_packet()
{
	enocean_data_structure returnvalue = create_base_frame();
	returnvalue.H_SEQ_LENGTH = 0x8B;
	returnvalue.ORG = 0x89;
	returnvalue.CHECKSUM = enocean_calc_checksum(&returnvalue);
	return returnvalue;
}

bool CEnOceanESP2::OpenSerialDevice()
{
	// Try to open the Serial Port
	try
	{
		open(m_szSerialPort, 9600); // ECP2 open with 9600
		Log(LOG_STATUS, "Using serial port %s", m_szSerialPort.c_str());
	}
	catch (boost::exception& e)
	{
		Log(LOG_ERROR, "Error opening serial port!");
#ifdef _DEBUG
		Log(LOG_ERROR, "-----------------\n%s\n----------------", boost::diagnostic_information(e).c_str());
#else
		(void)e;
#endif
		return false;
	}
	catch (...)
	{
		Log(LOG_ERROR, "Error opening serial port!!!");
		return false;
	}
	m_bIsStarted = true;
	m_receivestate = ERS_SYNC1;
	setReadCallback([this](auto d, auto l) { readCallback(d, l); });
	sOnConnected(this);

	enocean_data_structure iframe;
	/*
		// Send Initial Reset
		iframe = tcm120_reset();
		write((const char*)&iframe,sizeof(enocean_data_structure));
		sleep_seconds(1);
	*/

	iframe = tcm120_rd_idbase();
	write((const char*)&iframe, sizeof(enocean_data_structure));

	return true;
}

void CEnOceanESP2::readCallback(const char* data, size_t len)
{
	size_t ii = 0;
	while (ii < len)
	{
		const unsigned char c = data[ii];

		switch (m_receivestate)
		{
		case ERS_SYNC1:
			if (c != C_S_BYTE1)
				return;
			m_receivestate = ERS_SYNC2;
			break;
		case ERS_SYNC2:
			if (c != C_S_BYTE2)
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
			m_wantedlength = (c & 0x0F) + 3;
			m_bufferpos = 3;
			m_receivestate = ERS_DATA;
			break;
		case ERS_DATA:
			m_buffer[m_bufferpos++] = c;
			if (m_bufferpos >= m_wantedlength - 1)
			{
				m_receivestate = ERS_CHECKSUM;
			}
			break;
		case ERS_CHECKSUM:
			m_buffer[m_bufferpos++] = c;
			if (m_buffer[m_bufferpos - 1] == enocean_calc_checksum((const enocean_data_structure*)&m_buffer))
			{
				ParseData();
			}
			else
			{
				Log(LOG_ERROR, "Frame Checksum Error!...");
			}
			m_receivestate = ERS_SYNC1;
			break;
		}
		ii++;
	}
}

bool CEnOceanESP2::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	if (m_id_base == 0)
		return false;

	if (!isOpen())
		return false;

	RBUF* tsen = (RBUF*) pdata;

	if (tsen->LIGHTING2.packettype != pTypeLighting2)
		return false; // Only allowed to control switches

	uint32_t nodeID = GetNodeID(tsen->LIGHTING2.id1, tsen->LIGHTING2.id2, tsen->LIGHTING2.id3, tsen->LIGHTING2.id4);

	if (nodeID <= m_id_base || nodeID > (m_id_base + 128))
	{
		Log(LOG_ERROR, "Node %08X can not be used as a switch", nodeID);
		Log(LOG_ERROR, "Create a virtual switch associated with HwdID %u", m_HwdID);
		return false;
	}
	if (tsen->LIGHTING2.unitcode >= 10)
	{
		Log(LOG_ERROR, "Node %08X, double press not supported!", nodeID);
		return false;
	}
	uint8_t RockerID = tsen->LIGHTING2.unitcode - 1;
	uint8_t EB = 1;
	bool bIsDimmer = false;
	uint8_t LastLevel = 0;

	// Find out if this is a virtual switch or dimmer, because they are threaded differently
	// ESP2 virtual switches emulate RPS EEP: F6-02-01/02, Rocker switch, 2 Rocker
	// ESP2 virtual dimmers emulate 4BS EEP: A5-38-08, Central Command, Gateway

	std::string sDeviceID = GetDeviceID(nodeID);
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT SwitchType, LastLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
		m_HwdID, sDeviceID.c_str(), (int) tsen->LIGHTING2.unitcode);
	if (!result.empty())
	{
		_eSwitchType switchtype = static_cast<_eSwitchType>(std::stoul(result[0][0]));
		if (switchtype == STYPE_Dimmer)
			bIsDimmer = true;

		LastLevel = static_cast<uint8_t>(std::stoul(result[0][1]));
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

	enocean_data_structure iframe = create_base_frame();

	iframe.H_SEQ_LENGTH = 0x6B; // TX + Length
	iframe.ORG = C_ORG_RPS;
	iframe.ID_BYTE3 = (unsigned char) tsen->LIGHTING2.id1;
	iframe.ID_BYTE2 = (unsigned char) tsen->LIGHTING2.id2;
	iframe.ID_BYTE1 = (unsigned char) tsen->LIGHTING2.id3;
	iframe.ID_BYTE0 = (unsigned char) tsen->LIGHTING2.id4;

	// TODO: ESP2 virtual dimmers, emulate 4BS EEP: A5-38-08, Central Command, Gateway
	// They should ALWAYS send 4BS telegrams, for On/Off and dimming

	if (cmnd != light2_sSetLevel)
	{ // On/Off
		uint8_t CO = (cmnd != light2_sOff) && (cmnd != light2_sGroupOff);

		iframe.DATA_BYTE3 = (RockerID << 6) | (CO << 5) | (EB << 4);
		iframe.STATUS = 0x30;
		iframe.CHECKSUM = enocean_calc_checksum(&iframe);

		Add2SendQueue((const char*)&iframe, sizeof(enocean_data_structure));

		// Button release is send a bit later

		iframe.DATA_BYTE3 = 0x00;
		iframe.STATUS = 0x20;
		iframe.CHECKSUM = enocean_calc_checksum(&iframe);

		Add2SendQueue((const char*)&iframe, sizeof(enocean_data_structure));
	}
	else
	{ // Send dim value
		iframe.ORG = C_ORG_4BS;
		iframe.DATA_BYTE3 = 0x02;
		iframe.DATA_BYTE2 = iLevel;
		iframe.DATA_BYTE1 = 0x01; // Very fast dimming
		if ((iLevel == 0) || (orgcmd == light2_sOff))
			iframe.DATA_BYTE0 = 0x08; // Dim Off
		else
			iframe.DATA_BYTE0 = 0x09; // Dim On
		iframe.CHECKSUM = enocean_calc_checksum(&iframe);

		Add2SendQueue((const char*)&iframe, sizeof(enocean_data_structure));
	}
	return true;
}

// Called when testing a virtual dimmer, from manual switches creation dialog
// ESP2 virtual dimmers emulate 4BS EEP: A5-38-08, Central Command, Gateway
// They need to broadcast a 4BS teach-in request
void CEnOceanESP2::SendDimmerTeachIn(const char* pdata, const unsigned char /*length*/)
{
	if (m_id_base == 0)
		return;

	if (!isOpen())
		return;

	RBUF* tsen = (RBUF*) pdata;

	if (tsen->LIGHTING2.packettype != pTypeLighting2)
		return; // Only allowed to control switches

	uint32_t nodeID = GetNodeID(tsen->LIGHTING2.id1, tsen->LIGHTING2.id2, tsen->LIGHTING2.id3, tsen->LIGHTING2.id4);

	if (nodeID <= m_id_base || nodeID > (m_id_base + 128))
	{
		Log(LOG_ERROR, "Node %08X can not be used as a switch", nodeID);
		Log(LOG_ERROR, "Create a virtual switch associated with HwdID %u", m_HwdID);
		return;
	}
	if (tsen->LIGHTING2.unitcode >= 10)
	{
		Log(LOG_ERROR, "Node %08X, double press not supported!", nodeID);
		return;
	}
	Log(LOG_NORM, "4BS teach-in request from Node %08X (variation 3 : bi-directional)", nodeID);

	enocean_data_structure iframe = create_base_frame();

	iframe.H_SEQ_LENGTH = 0x6B; // TX + Length
	iframe.ORG = C_ORG_4BS;
	iframe.DATA_BYTE3 = 0x02;
	iframe.DATA_BYTE2 = 0x00;
	iframe.DATA_BYTE1 = 0x00;
	iframe.DATA_BYTE0 = 0x00;
	iframe.CHECKSUM = enocean_calc_checksum(&iframe);

	Add2SendQueue((const char*) &iframe, sizeof(enocean_data_structure));
}

bool CEnOceanESP2::ParseData()
{
	enocean_data_structure* pFrame = (enocean_data_structure*)&m_buffer;
	unsigned char Checksum = enocean_calc_checksum(pFrame);
	if (Checksum != pFrame->CHECKSUM)
		return false; // Checksum mismatch!

	uint32_t nodeID = GetNodeID(pFrame->ID_BYTE3, pFrame->ID_BYTE2, pFrame->ID_BYTE1, pFrame->ID_BYTE0);

	// Handle possible OK/Errors
	bool bStopProcessing = false;
	if (pFrame->H_SEQ_LENGTH == 0x8B)
	{
		switch (pFrame->ORG)
		{
		case 0x58:
			Debug(DEBUG_NORM, "OK");
			bStopProcessing = true;
			break;
		case 0x28:
			Log(LOG_ERROR, "ERR_MODEM_NOTWANTEDACK");
			bStopProcessing = true;
			break;
		case 0x29:
			Log(LOG_ERROR, "ERR_MODEM_NOTACK");
			bStopProcessing = true;
			break;
		case 0x0C:
			Log(LOG_ERROR, "ERR_MODEM_DUP_ID");
			bStopProcessing = true;
			break;
		case 0x08:
			Log(LOG_ERROR, "Error in H_SEQ");
			bStopProcessing = true;
			break;
		case 0x09:
			Log(LOG_ERROR, "Error in LENGTH");
			bStopProcessing = true;
			break;
		case 0x0A:
			Log(LOG_ERROR, "Error in CHECKSUM");
			bStopProcessing = true;
			break;
		case 0x0B:
			Log(LOG_ERROR, "Error in ORG");
			bStopProcessing = true;
			break;
		case 0x22:
			Log(LOG_ERROR, "ERR_TX_IDRANGE");
			bStopProcessing = true;
			break;
		case 0x1A:
			Log(LOG_ERROR, "ERR_ IDRANGE");
			bStopProcessing = true;
			break;
		}
	}
	if (bStopProcessing)
		return true;

	switch (pFrame->ORG)
	{
	case C_ORG_INF_IDBASE:
		m_id_base = GetNodeID(pFrame->DATA_BYTE3, pFrame->DATA_BYTE2, pFrame->DATA_BYTE1, pFrame->DATA_BYTE0);
		Log(LOG_STATUS, "Transceiver ID_Base %08X", m_id_base);
		break;
	case C_ORG_RPS:
		if (pFrame->STATUS & S_RPS_NU)
		{
			// Rocker
			// NU == 1, N-Message
			unsigned char RockerID = (pFrame->DATA_BYTE3 & DB3_RPS_NU_RID) >> DB3_RPS_NU_RID_SHIFT;
			unsigned char UpDown = (pFrame->DATA_BYTE3 & DB3_RPS_NU_UD) >> DB3_RPS_NU_UD_SHIFT;
			unsigned char Pressed = (pFrame->DATA_BYTE3 & DB3_RPS_NU_PR) >> DB3_RPS_NU_PR_SHIFT;
			unsigned char SecondRockerID = (pFrame->DATA_BYTE3 & DB3_RPS_NU_SRID) >> DB3_RPS_NU_SRID_SHIFT;
			unsigned char SecondUpDown = (pFrame->DATA_BYTE3 & DB3_RPS_NU_SUD) >> DB3_RPS_NU_SUD_SHIFT;
			unsigned char SecondAction = (pFrame->DATA_BYTE3 & DB3_RPS_NU_SA) >> DB3_RPS_NU_SA_SHIFT;

			Debug(DEBUG_NORM, "RPS N-msg: Node %08X Rocker ID %i UD %i Pressed %i Second Rocker ID %i SUD %i Second Action %i",
				nodeID, RockerID, UpDown, Pressed, SecondRockerID, SecondUpDown, SecondAction);

			// 3 types of buttons from a switch: Left/Right/Left+Right
			if (Pressed == 1)
			{
				RBUF tsen;
				memset(&tsen, 0, sizeof(RBUF));
				tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
				tsen.LIGHTING2.packettype = pTypeLighting2;
				tsen.LIGHTING2.subtype = sTypeAC;
				tsen.LIGHTING2.seqnbr = 0;
				tsen.LIGHTING2.id1 = (BYTE) pFrame->ID_BYTE3;
				tsen.LIGHTING2.id2 = (BYTE) pFrame->ID_BYTE2;
				tsen.LIGHTING2.id3 = (BYTE) pFrame->ID_BYTE1;
				tsen.LIGHTING2.id4 = (BYTE) pFrame->ID_BYTE0;
				tsen.LIGHTING2.level = 0;
				tsen.LIGHTING2.rssi = 12;

				if (SecondAction == 0)
				{ // Left/Right Up/Down
					tsen.LIGHTING2.unitcode = RockerID + 1;
					tsen.LIGHTING2.cmnd = (UpDown == 1) ? light2_sOn : light2_sOff;
				}
				else
				{ // Left+Right Up/Down
					tsen.LIGHTING2.unitcode = SecondRockerID + 10;
					tsen.LIGHTING2.cmnd = (SecondUpDown == 1) ? light2_sOn : light2_sOff;
				}
				sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, "Rocker", 255, m_Name.c_str());
			}
		}
		break;
	case C_ORG_4BS:
	{
		if ((pFrame->DATA_BYTE0 & 0x08) == 0)
		{
			if (pFrame->DATA_BYTE0 & 0x80)
			{ // 4BS Teach-in
				// DB3		DB3/2	DB2/1			DB0
				// Func  	Type	Manufacturer-ID	LRN Type	RE2		RE1
				// 6 Bit	7 Bit	11 Bit			1Bit		1Bit	1Bit	1Bit	1Bit	1Bit	1Bit	1Bit

				uint16_t manID = ((pFrame->DATA_BYTE2 & 7) << 8) | pFrame->DATA_BYTE1;
				uint8_t func = pFrame->DATA_BYTE3 >> 2;
				uint8_t type = ((pFrame->DATA_BYTE3 & 3) << 5) | (pFrame->DATA_BYTE2 >> 3);

				Log(LOG_NORM, "4BS Teach-in diagram: Node %08X Manufacturer %02X (%s) Profile %02X Type %02X (%s)",
					nodeID, manID, GetManufacturerName(manID),
					func, type, GetEEPLabel(enocean::RORG_4BS, func, type));

				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT ID FROM EnOceanNodes WHERE (HardwareID==%d) AND (NodeID==%u)", m_HwdID, nodeID);
				if (result.empty())
				{ // Add it to the database
					m_sql.safe_query(
						"INSERT INTO EnOceanNodes (HardwareID, NodeID, ManufacturerID, RORG, Func, Type) VALUES (%d,%u,%u,%u,%u,%u)",
						m_HwdID, nodeID, manID, enocean::RORG_4BS, func, type);
				}
			}
		}
		else
		{ // Following sensors need to have had a teach-in
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT ID, ManufacturerID, Func, Type FROM EnOceanNodes WHERE (HardwareID==%d) AND (NodeID==%u)", m_HwdID, nodeID);
			if (result.empty())
			{
				char* pszHumenTxt = enocean_hexToHuman(pFrame);
				if (pszHumenTxt)
				{
					Log(LOG_NORM, "Need Teach-In for %s", pszHumenTxt);
					free(pszHumenTxt);
				}
				return true;
			}
			uint16_t Manufacturer = static_cast<uint16_t>(std::stoul(result[0][1]));
			uint8_t Profile = static_cast<uint8_t>(std::stoul(result[0][2]));
			uint8_t iType = static_cast<uint8_t>(std::stoul(result[0][3]));

			if (Profile == 0x12 && iType == 0x00)
			{ // A5-12-00, Automated Meter Reading, Counter
				uint8_t CH = bitrange(pFrame->DATA_BYTE0, 4, 0x0F); // Channel number
				uint8_t DT = bitrange(pFrame->DATA_BYTE0, 2, 0x01); // 0 = cumulative count, 1 = current value / s
				uint8_t DIV = bitrange(pFrame->DATA_BYTE0, 0, 0x03);
				float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
				uint32_t MR = round(GetDeviceValue((pFrame->DATA_BYTE3 << 16) | (pFrame->DATA_BYTE2 << 8) | pFrame->DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

				RBUF tsen;
				memset(&tsen, 0, sizeof(RBUF));
				tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
				tsen.RFXMETER.packettype = pTypeRFXMeter;
				tsen.RFXMETER.subtype = sTypeRFXMeterCount;
				tsen.RFXMETER.rssi = 12;
				tsen.RFXMETER.id1 = pFrame->ID_BYTE2;
				tsen.RFXMETER.id2 = pFrame->ID_BYTE1;
				tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
				tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
				tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
				tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);

				Debug(DEBUG_NORM, "4BS msg: Node %08X CH %u DT %u DIV %u (scaleMax %.3F) MR %u",
					nodeID, CH, DT, DIV, scaleMax, MR);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x12 && iType == 0x01)
			{ // A5-12-01, Automated Meter Reading, Electricity
				uint32_t MR =(pFrame->DATA_BYTE3 << 16) | (pFrame->DATA_BYTE2 << 8) | pFrame->DATA_BYTE1;
				uint8_t TI = bitrange(pFrame->DATA_BYTE0, 4, 0x0F); // Tariff info
				uint8_t DT = bitrange(pFrame->DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
				uint8_t DIV = bitrange(pFrame->DATA_BYTE0, 0, 0x03);
				float scaleMax = (DIV == 0) ? 16777215.0F : ((DIV == 1) ? 1677721.5F : ((DIV == 2) ? 167772.15F : 16777.215F));

				_tUsageMeter umeter;
				umeter.id1 = (BYTE) pFrame->ID_BYTE3;
				umeter.id2 = (BYTE) pFrame->ID_BYTE2;
				umeter.id3 = (BYTE) pFrame->ID_BYTE1;
				umeter.id4 = (BYTE) pFrame->ID_BYTE0;
				umeter.dunit = 1;
				umeter.fusage = GetDeviceValue(MR, 0, 16777215, 0.0F, scaleMax);

				Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
					nodeID, TI, DT, DIV, scaleMax, MR);

				sDecodeRXMessage(this, (const unsigned char *) &umeter, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x12 && iType == 0x02)
			{ // A5-12-02, Automated Meter Reading, Gas
				uint8_t TI = bitrange(pFrame->DATA_BYTE0, 4, 0x0F); // Tariff info
				uint8_t DT = bitrange(pFrame->DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
				uint8_t DIV = bitrange(pFrame->DATA_BYTE0, 0, 0x03);
				float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
				uint32_t MR = round(GetDeviceValue((pFrame->DATA_BYTE3 << 16) | (pFrame->DATA_BYTE2 << 8) | pFrame->DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

				RBUF tsen;
				memset(&tsen, 0, sizeof(RBUF));
				tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
				tsen.RFXMETER.packettype = pTypeRFXMeter;
				tsen.RFXMETER.subtype = sTypeRFXMeterCount;
				tsen.RFXMETER.id1 = pFrame->ID_BYTE2;
				tsen.RFXMETER.id2 = pFrame->ID_BYTE1;
				tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
				tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
				tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
				tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);
				tsen.RFXMETER.rssi = 12;

				Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
					nodeID, TI, DT, DIV, scaleMax, MR);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x12 && iType == 0x03)
			{ // A5-12-03, Automated Meter Reading, Water
				uint8_t TI = bitrange(pFrame->DATA_BYTE0, 4, 0x0F); // Tariff info
				uint8_t DT = bitrange(pFrame->DATA_BYTE0, 2, 0x01); // 0 = cumulative count (kWh), 1 = current value (W)
				uint8_t DIV = bitrange(pFrame->DATA_BYTE0, 0, 0x03);
				float scaleMax = (DIV == 0) ? 16777215.000F : ((DIV == 1) ? 1677721.500F : ((DIV == 2) ? 167772.150F : 16777.215F));
				uint32_t MR = round(GetDeviceValue((pFrame->DATA_BYTE3 << 16) | (pFrame->DATA_BYTE2 << 8) | pFrame->DATA_BYTE1, 0, 16777215, 0.0F, scaleMax));

				RBUF tsen;
				memset(&tsen, 0, sizeof(RBUF));
				tsen.RFXMETER.packetlength = sizeof(tsen.RFXMETER) - 1;
				tsen.RFXMETER.packettype = pTypeRFXMeter;
				tsen.RFXMETER.subtype = sTypeRFXMeterCount;
				tsen.RFXMETER.id1 = pFrame->ID_BYTE2;
				tsen.RFXMETER.id2 = pFrame->ID_BYTE1;
				tsen.RFXMETER.count1 = (BYTE) ((MR & 0xFF000000) >> 24);
				tsen.RFXMETER.count2 = (BYTE) ((MR & 0x00FF0000) >> 16);
				tsen.RFXMETER.count3 = (BYTE) ((MR & 0x0000FF00) >> 8);
				tsen.RFXMETER.count4 = (BYTE) (MR & 0x000000FF);
				tsen.RFXMETER.rssi = 12;

				Debug(DEBUG_NORM, "4BS msg: Node %08X TI %u DT %u DIV %u (scaleMax %.3F) MR %u",
					nodeID, TI, DT, DIV, scaleMax, MR);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXMETER, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x10 && iType <= 0x0D)
			{ // A5-10-01..0D, Room Operating Panel
				RBUF tsen;

				if (Manufacturer != enocean::ELTAKO)
				{ // General case for A5-10-01..0D
					// pFrame->DATA_BYTE3 is the fan speed
					// pFrame->DATA_BYTE2 is the setpoint where 0x00 = min ... 0xFF = max
					// pFrame->DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
					// pFrame->DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch

					// A5-10-01, A5-10-02, A5-10-04, A5-10-07, A5-10-08, A5-10-09 have FAN information
					if (iType == 0x01 || iType == 0x02 || iType == 0x04 || iType == 0x07 || iType == 0x08 || iType == 0x09)
					{
						uint8_t FAN;
						if (pFrame->DATA_BYTE3 >= 210)
							FAN = fan_NovyLearn; // Auto
						else if (pFrame->DATA_BYTE3 >= 190)
							FAN = fan_NovyLight;
						else if (pFrame->DATA_BYTE3 >= 165)
							FAN = fan_NovyMin;
						else if (pFrame->DATA_BYTE3 >= 145)
							FAN = fan_NovyPlus;
						else
							FAN = fan_NovyPower;

						memset(&tsen, 0, sizeof(RBUF));
						tsen.FAN.packetlength = sizeof(tsen.FAN) - 1;
						tsen.FAN.packettype = pTypeFan;
						tsen.FAN.subtype = sTypeNovy;
						tsen.FAN.seqnbr = 0;
						tsen.FAN.id1 = pFrame->ID_BYTE3;
						tsen.FAN.id2 = pFrame->ID_BYTE2;
						tsen.FAN.id3 = pFrame->ID_BYTE1;
						tsen.FAN.cmnd = FAN;
						tsen.FAN.rssi = 12;

						Debug(DEBUG_NORM, "4BS msg: Node %08X FAN %u", nodeID, FAN);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.FAN, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
					}

					// A5-10-01, A5-10-02, A5-10-03, A5-10-04, A5-10-05, A5-10-06, A5-10-0A have SP information
					if (iType == 0x01 || iType == 0x02 || iType == 0x03 || iType == 0x04 || iType == 0x05 || iType == 0x06 || iType == 0x0A)
					{
						float SP = GetDeviceValue(pFrame->DATA_BYTE2, 0, 255, 0.0F, 255.0F);

						Debug(DEBUG_NORM, "4BS msg: Node %08X SP %.0F", nodeID, SP);

						// TODO: implement SP
					}

					// A5-10-01, A5-10-05, A5-10-08, A5-10-0C have OCC information
					// A5-10-02, A5-10-06, A5-10-09, A5-10-0D have SLSW information
					// A5-10-0A, A5-10-0B have CTST information
					if (iType == 0x01 || iType == 0x05 || iType == 0x08 || iType == 0x0C
						|| iType == 0x02 || iType == 0x06 || iType == 0x09 || iType == 0x0D
						|| iType == 0x0A || iType == 0x0B)
					{
						uint8_t SW = bitrange(pFrame->DATA_BYTE0, 0, 0x01);

						memset(&tsen, 0, sizeof(RBUF));
						tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
						tsen.LIGHTING2.packettype = pTypeLighting2;
						tsen.LIGHTING2.subtype = sTypeAC;
						tsen.LIGHTING2.seqnbr = 0;
						tsen.LIGHTING2.id1 = (BYTE) pFrame->ID_BYTE3;
						tsen.LIGHTING2.id2 = (BYTE) pFrame->ID_BYTE2;
						tsen.LIGHTING2.id3 = (BYTE) pFrame->ID_BYTE1;
						tsen.LIGHTING2.id4 = (BYTE) pFrame->ID_BYTE0;
						tsen.LIGHTING2.level = 0;
						tsen.LIGHTING2.unitcode = 1;
						tsen.LIGHTING2.cmnd = (SW == 0) ? light2_sOff : light2_sOn;
						tsen.LIGHTING2.rssi = 12;

						const char *sSW;
						const char *sSW0;
						const char *sSW1;

						if (iType == 0x01 || iType == 0x05 || iType == 0x08 || iType == 0x0C)
							sSW = "OCC", sSW0 = "Button pressed", sSW1 = "Button released";
						else if (iType == 0x02 || iType == 0x06 || iType == 0x09 || iType == 0x0D)
							sSW = "SLSW", sSW0 = "Position I/Night/Off", sSW1 = "Position O/Day/On";
						else // if (iType == 0x0A || iType == 0x0B)
							sSW = "CTST", sSW0 = "Closed", sSW1 = "Open";

						Debug(DEBUG_NORM, "4BS msg: Node %08X %s %u (%s)", nodeID, sSW, SW, (SW == 0) ? sSW0 : sSW1);

						sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
					}
				}
				else
				{ // WARNING : ELTAKO specific implementation
					// Eltako FTR55D, FTR55H
					// pFrame->DATA_BYTE3 is the night reduction
					// pFrame->DATA_BYTE2 is reference temperature where 0x00 = 0°C ... 0xFF = 40°C
					// pFrame->DATA_BYTE1 is the temperature where 0x00 = +40°C ... 0xFF = 0°C
					// pFrame->DATA_BYTE0_bit_0 is the occupy button, pushbutton or slide switch

					uint8_t nightReduction = 0;
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
					// float SPTMP = GetDeviceValue(pFrame->DATA_BYTE2, 0, 255, 0.0F, 40.0F);
				}
				// All A5-10-01 to A5-10-0D have TMP information

				float TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 0.0F, 40.0F);

				memset(&tsen, 0, sizeof(RBUF));
				tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
				tsen.TEMP.packettype = pTypeTEMP;
				tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
				tsen.TEMP.seqnbr = 0;
				tsen.TEMP.id1 = pFrame->ID_BYTE2;
				tsen.TEMP.id2 = pFrame->ID_BYTE1;
				// WARNING
				// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
				// decode_Temp sets battery_level = 255 (Unknown) & rssi = 12 (Not available)
				tsen.TEMP.battery_level = bitrange(pFrame->ID_BYTE0, 0, 0x0F);
				tsen.TEMP.rssi = bitrange(pFrame->ID_BYTE0, 4, 0x0F);
				tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
				int at10 = round(std::abs(TMP * 10.0F));
				tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
				at10 -= tsen.TEMP.temperatureh * 256;
				tsen.TEMP.temperaturel = (BYTE) at10;

				Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", nodeID, TMP);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
			}
			else if (Profile == 0x06 && iType == 0x01)
			{ // A5-06-01, Light Sensor
				uint8_t RS;
				float ILL = 0.0F;

				if (Manufacturer != enocean::ELTAKO)
				{ // General case for A5-06-01
					// pFrame->DATA_BYTE3 is the voltage where 0x00 = 0 V ... 0xFF = 5.1 V

					float SVC = GetDeviceValue(pFrame->DATA_BYTE3, 0, 255, 0.0F, 5100.0F); // Convert V to mV

					// DATA_BYTE0_bit_0 is Range select where 0 = ILL1, 1 = ILL2
					// DATA_BYTE1 is the illuminance (ILL1) where min 0 = 600 lx, max 255 = 60000 lx
					// DATA_BYTE2 is the illuminance (ILL2) where min 0 = 300 lx, max 255 = 30000 lx

					RS = bitrange(pFrame->DATA_BYTE0, 0, 0x01);
					if (RS == 0)
						ILL = GetDeviceValue(pFrame->DATA_BYTE1, 0, 255, 600.0F, 60000.0F);
					else
						ILL = GetDeviceValue(pFrame->DATA_BYTE2, 0, 255, 300.0F, 30000.0F);

					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
					tsen.RFXSENSOR.packettype = pTypeRFXSensor;
					tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
					tsen.RFXSENSOR.seqnbr = 0;
					tsen.RFXSENSOR.id = pFrame->ID_BYTE1;
					// WARNING
					// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
					// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
					tsen.RFXSENSOR.filler = bitrange(pFrame->ID_BYTE0, 0, 0x0F);
					tsen.RFXSENSOR.rssi = bitrange(pFrame->ID_BYTE0, 4, 0x0F);
					tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
					tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

					Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", nodeID, SVC);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
				}
				else
				{ // WARNING : ELTAKO specific implementation
					// Eltako FAH60, FAH60B, FAH65S, FIH65S, FAH63, FIH63
					// DATA_BYTE2 is Range select where 0 = ILL1, 1 = ILL2
					// DATA_BYTE3 is the low illuminance (ILL1) where min 0 = 0 lx, max 255 = 100 lx
					// DATA_BYTE2 is the illuminance (ILL2) where min 0 = 300 lx, max 255 = 30000 lx

					RS = pFrame->DATA_BYTE2;
					if (RS == 0)
						ILL = GetDeviceValue(pFrame->DATA_BYTE3, 0, 255, 0.0F, 100.0F);
					else
						ILL = GetDeviceValue(pFrame->DATA_BYTE2, 0, 255, 300.0F, 30000.0F);
				}
				_tLightMeter lmeter;
				lmeter.id1 = (BYTE) pFrame->ID_BYTE3; // Sender ID
				lmeter.id2 = (BYTE) pFrame->ID_BYTE2;
				lmeter.id3 = (BYTE) pFrame->ID_BYTE1;
				lmeter.id4 = (BYTE) pFrame->ID_BYTE0;
				lmeter.dunit = 1;
				lmeter.fLux = ILL;

				Debug(DEBUG_NORM, "4BS msg: Node %08X RS %s ILL %.1Flx", nodeID, (RS == 0) ? "ILL1" : "ILL2", ILL);

				sDecodeRXMessage(this, (const unsigned char *) &lmeter, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x02 && (iType <= 0x1B || iType == 0x20 || iType == 0x30))
			{	// A5-02-01..30, Temperature sensor
				float TMP = -275.0F; // Initialize to an arbitrary out of range value
				if (iType == 0x01)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -40.0F, 0.0F);
				else if (iType == 0x02)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -30.0F, 10.0F);
				else if (iType == 0x03)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -20.0F, 20.0F);
				else if (iType == 0x04)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -10.0F, 30.0F);
				else if (iType == 0x05)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 0.0F, 40.0F);
				else if (iType == 0x06)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 10.0F, 50.0F);
				else if (iType == 0x07)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 20.0F, 60.0F);
				else if (iType == 0x08)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 30.0F, 70.0F);
				else if (iType == 0x09)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 40.0F, 80.0F);
				else if (iType == 0x0A)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 50.0F, 90.0F);
				else if (iType == 0x0B)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 60.0F, 100.0F);
				else if (iType == 0x10)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -60.0F, 20.0F);
				else if (iType == 0x11)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -50.0F, 30.0F);
				else if (iType == 0x12)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -40.0F, 40.0F);
				else if (iType == 0x13)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -30.0F, 50.0F);
				else if (iType == 0x14)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -20.0F, 60.0F);
				else if (iType == 0x15)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, -10.0F, 70.0F);
				else if (iType == 0x16)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 0.0F, 80.0F);
				else if (iType == 0x17)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 10.0F, 90.0F);
				else if (iType == 0x18)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 20.0F, 100.0F);
				else if (iType == 0x19)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 30.0F, 110.0F);
				else if (iType == 0x1A)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 40.0F, 120.0F);
				else if (iType == 0x1B)
					TMP = GetDeviceValue(pFrame->DATA_BYTE1, 255, 0, 50.0F, 130.0F);
				else if (iType == 0x20)
					TMP = GetDeviceValue(((pFrame->DATA_BYTE2 & 0x03) << 8) | pFrame->DATA_BYTE1, 1023, 0, -10.0F, 41.2F); // 10bit
				else if (iType == 0x30)
					TMP = GetDeviceValue(((pFrame->DATA_BYTE2 & 0x03) << 8) | pFrame->DATA_BYTE1, 1023, 0, -40.0F, 62.3F); // 10bit

				if (TMP > -274.0F)
				{ // TMP value has been changed => EEP is managed => update TMP
					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
					tsen.TEMP.packettype = pTypeTEMP;
					tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
					tsen.TEMP.id1 = pFrame->ID_BYTE2;
					tsen.TEMP.id2 = pFrame->ID_BYTE1;
					// WARNING
					// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
					// decode_Temp sets battery_level = 255 (Unknown) & rssi = 12 (Not available)
					tsen.TEMP.battery_level = pFrame->ID_BYTE0 & 0x0F;
					tsen.TEMP.rssi = (pFrame->ID_BYTE0 & 0xF0) >> 4;
					tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
					int at10 = round(std::abs(TMP * 10.0F));
					tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
					at10 -= (tsen.TEMP.temperatureh * 256);
					tsen.TEMP.temperaturel = (BYTE) at10;

					Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", nodeID, TMP);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
				}
			}
			else if (Profile == 0x04 && iType <= 0x04)
			{ // A5-04-01..04, Temperature and Humidity Sensor
				float HUM = -2.0F;   // Initialize to an arbitrary out of range value
				float TMP = -275.0F; // Initialize to an arbitrary out of range value
				if (iType == 0x01)
				{
					HUM = GetDeviceValue(pFrame->DATA_BYTE2, 0, 250, 0.0F, 100.0F);

					uint8_t TSN = (pFrame->DATA_BYTE0 & 0x02) >> 1;
					if (TSN == 1) // Temperature sensor available
						TMP = GetDeviceValue(pFrame->DATA_BYTE1, 0, 250, 0.0F, 40.0F);
				}
				else if (iType == 0x02)
				{
					HUM = GetDeviceValue(pFrame->DATA_BYTE2, 0, 250, 0.0F, 100.0F);

					uint8_t TSN = (pFrame->DATA_BYTE0 & 0x02) >> 1;
					if (TSN == 1) // Temperature sensor available
						TMP = GetDeviceValue(pFrame->DATA_BYTE1, 0, 250, -20.0F, 60.0F);
				}
				else if (iType == 0x03)
				{
					HUM = GetDeviceValue(pFrame->DATA_BYTE3, 0, 255, 0.0F, 100.0F);
					TMP = GetDeviceValue(bitrange(pFrame->DATA_BYTE2, 0, 0x03) << 8 | pFrame->DATA_BYTE1, 0, 1023, -20.0F, 60.0F); // 10bit
					// uint8_t TTP = bitrange(pFrame->DATA_BYTE0, 0, 0x01);
				}
				else if (iType == 0x04)
				{
					HUM = GetDeviceValue(pFrame->DATA_BYTE3, 0, 199, 0.0F, 100.0F);
					TMP = GetDeviceValue(bitrange(pFrame->DATA_BYTE2, 0, 0x0F) << 8 | pFrame->DATA_BYTE1, 0, 1599, -40.0F, +120.0F); // 12bit
				}
				if (TMP > -274.0F && HUM  > -1.0F)
				{ // TMP + HUM values have been changed => EEP is managed => update TEMP_HUM
					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP_HUM.packetlength = sizeof(tsen.TEMP_HUM) - 1;
					tsen.TEMP_HUM.packettype = pTypeTEMP_HUM;
					tsen.TEMP_HUM.subtype = sTypeTH5; // WTGR800
					tsen.TEMP_HUM.id1 = pFrame->ID_BYTE2;
					tsen.TEMP_HUM.id2 = pFrame->ID_BYTE1;
					tsen.TEMP_HUM.tempsign = (TMP >= 0) ? 0 : 1;
					int at10 = round(std::abs(TMP * 10.0F));
					tsen.TEMP_HUM.temperatureh = (BYTE) (at10 / 256);
					at10 -= (tsen.TEMP_HUM.temperatureh * 256);
					tsen.TEMP_HUM.temperaturel = (BYTE) at10;
					tsen.TEMP_HUM.humidity = (BYTE) round(HUM);
					tsen.TEMP_HUM.humidity_status = Get_Humidity_Level(tsen.TEMP_HUM.humidity);
					tsen.TEMP_HUM.battery_level = 9; // OK
					tsen.TEMP_HUM.rssi = 12; // Not available

					Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C HUM %d%%", nodeID, TMP, tsen.TEMP_HUM.humidity);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP_HUM, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
				}
				else if (HUM > -1.0F)
				{ // HUM value has been changed => EEP is managed => update HUM (TMP unavailable)
					RBUF tsen;
					memset(&tsen, 0, sizeof(RBUF));
					tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
					tsen.HUM.packettype = pTypeHUM;
					tsen.HUM.subtype = sTypeHUM1; // LaCrosse TX3
					tsen.HUM.seqnbr = 0;
					tsen.HUM.id1 = pFrame->ID_BYTE2;
					tsen.HUM.id2 = pFrame->ID_BYTE1;
					tsen.HUM.humidity = (BYTE) round(HUM);
					tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
					tsen.HUM.battery_level = 9; // OK, TODO: Should be 255 (unknown battery level) ?
					tsen.HUM.rssi = 12; // Not available

					Debug(DEBUG_NORM, "4BS msg: Node %08X HUM %d%%", nodeID, tsen.HUM.humidity);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.HUM, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
				}
			}
			else if (Profile == 0x07 && iType == 0x01)
			{ // A5-07-01, Occupancy sensor with optional Supply voltage monitor
				RBUF tsen;

				uint8_t SVA = bitrange(pFrame->DATA_BYTE0, 0, 0x01);
				if (SVA == 1)
				{ // Supply voltage supported
					float SVC = GetDeviceValue(pFrame->DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

					memset(&tsen, 0, sizeof(RBUF));
					tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
					tsen.RFXSENSOR.packettype = pTypeRFXSensor;
					tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
					tsen.RFXSENSOR.seqnbr = 0;
					tsen.RFXSENSOR.id = pFrame->ID_BYTE1;
					// WARNING
					// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
					// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
					tsen.RFXSENSOR.filler = bitrange(pFrame->ID_BYTE0, 0, 0x0F);
					tsen.RFXSENSOR.rssi = bitrange(pFrame->ID_BYTE0, 4, 0x0F);
					tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
					tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

					Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", nodeID, SVC);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
				}
				uint8_t PIRS = pFrame->DATA_BYTE1;

				memset(&tsen, 0, sizeof(RBUF));
				tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
				tsen.LIGHTING2.packettype = pTypeLighting2;
				tsen.LIGHTING2.subtype = sTypeAC;
				tsen.LIGHTING2.seqnbr = 0;
				tsen.LIGHTING2.id1 = (BYTE) pFrame->ID_BYTE3;
				tsen.LIGHTING2.id2 = (BYTE) pFrame->ID_BYTE2;
				tsen.LIGHTING2.id3 = (BYTE) pFrame->ID_BYTE1;
				tsen.LIGHTING2.id4 = (BYTE) pFrame->ID_BYTE0;
				tsen.LIGHTING2.level = 0;
				tsen.LIGHTING2.unitcode = 1;
				tsen.LIGHTING2.cmnd = (PIRS >= 128) ? light2_sOn : light2_sOff;
				tsen.LIGHTING2.rssi = 12;

				Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)", nodeID, PIRS, (PIRS >= 128) ? "On" : "Off");

				sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x07 && iType == 0x02)
			{ // A5-07-02, Occupancy sensor with Supply voltage monitor
				RBUF tsen;

				float SVC = GetDeviceValue(pFrame->DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

				memset(&tsen, 0, sizeof(RBUF));
				tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
				tsen.RFXSENSOR.packettype = pTypeRFXSensor;
				tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
				tsen.RFXSENSOR.seqnbr = 0;
				tsen.RFXSENSOR.id = pFrame->ID_BYTE1;
				// WARNING
				// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
				// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
				tsen.RFXSENSOR.filler = bitrange(pFrame->ID_BYTE0, 0, 0x0F);
				tsen.RFXSENSOR.rssi = bitrange(pFrame->ID_BYTE0, 4, 0x0F);
				tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
				tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

				Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", nodeID, SVC);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());

				uint8_t PIRS = bitrange(pFrame->DATA_BYTE0, 7, 0x01);

				memset(&tsen, 0, sizeof(RBUF));
				tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
				tsen.LIGHTING2.packettype = pTypeLighting2;
				tsen.LIGHTING2.subtype = sTypeAC;
				tsen.LIGHTING2.seqnbr = 0;
				tsen.LIGHTING2.id1 = (BYTE) pFrame->ID_BYTE3;
				tsen.LIGHTING2.id2 = (BYTE) pFrame->ID_BYTE2;
				tsen.LIGHTING2.id3 = (BYTE) pFrame->ID_BYTE1;
				tsen.LIGHTING2.id4 = (BYTE) pFrame->ID_BYTE0;
				tsen.LIGHTING2.level = 0;
				tsen.LIGHTING2.unitcode = 1;
				tsen.LIGHTING2.cmnd = (PIRS == 1) ? light2_sOn : light2_sOff;
				tsen.LIGHTING2.rssi = 12;

				Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)",
					nodeID, PIRS, (PIRS == 1) ? "Motion detected" : "Uncertain of occupancy status");

				sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x07 && iType == 0x03)
			{ // A5-07-03, Occupancy sensor with Supply voltage monitor and 10-bit illumination measurement
				RBUF tsen;

				float SVC = GetDeviceValue(pFrame->DATA_BYTE3, 0, 250, 0.0F, 5000.0F); // Convert V to mV

				memset(&tsen, 0, sizeof(RBUF));
				tsen.RFXSENSOR.packetlength = sizeof(tsen.RFXSENSOR) - 1;
				tsen.RFXSENSOR.packettype = pTypeRFXSensor;
				tsen.RFXSENSOR.subtype = sTypeRFXSensorVolt;
				tsen.RFXSENSOR.seqnbr = 0;
				tsen.RFXSENSOR.id = pFrame->ID_BYTE1;
				// WARNING
				// filler & rssi fields are used here to transmit ID_BYTE0 value to decode_RFXSensor in mainworker.cpp
				// decode_RFXSensor sets BatteryLevel to 255 (Unknown) and rssi to 12 (Not available)
				tsen.RFXSENSOR.filler = bitrange(pFrame->ID_BYTE0, 0, 0x0F);
				tsen.RFXSENSOR.rssi = bitrange(pFrame->ID_BYTE0, 4, 0x0F);
				tsen.RFXSENSOR.msg1 = (BYTE) (SVC / 256);
				tsen.RFXSENSOR.msg2 = (BYTE) (SVC - (tsen.RFXSENSOR.msg1 * 256));

				Debug(DEBUG_NORM, "4BS msg: Node %08X SVC %.1FmV", nodeID, SVC);

				sDecodeRXMessage(this, (const unsigned char *) &tsen.RFXSENSOR, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());

				float ILL = GetDeviceValue((pFrame->DATA_BYTE2 << 2) | bitrange(pFrame->DATA_BYTE1, 6, 0x03), 0, 1000, 0.0F, 1000.0F);

				_tLightMeter lmeter;
				lmeter.id1 = (BYTE) pFrame->ID_BYTE3;
				lmeter.id2 = (BYTE) pFrame->ID_BYTE2;
				lmeter.id3 = (BYTE) pFrame->ID_BYTE1;
				lmeter.id4 = (BYTE) pFrame->ID_BYTE0;
				lmeter.dunit = 1;
				lmeter.fLux = ILL;

				Debug(DEBUG_NORM, "4BS msg: Node %08X ILL %.1Flx", nodeID, ILL);

				sDecodeRXMessage(this, (const unsigned char *) &lmeter, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());

				uint8_t PIRS = bitrange(pFrame->DATA_BYTE0, 7, 0x01);

				memset(&tsen, 0, sizeof(RBUF));
				tsen.LIGHTING2.packetlength = sizeof(tsen.LIGHTING2) - 1;
				tsen.LIGHTING2.packettype = pTypeLighting2;
				tsen.LIGHTING2.subtype = sTypeAC;
				tsen.LIGHTING2.seqnbr = 0;
				tsen.LIGHTING2.id1 = (BYTE) pFrame->ID_BYTE3;
				tsen.LIGHTING2.id2 = (BYTE) pFrame->ID_BYTE2;
				tsen.LIGHTING2.id3 = (BYTE) pFrame->ID_BYTE1;
				tsen.LIGHTING2.id4 = (BYTE) pFrame->ID_BYTE0;
				tsen.LIGHTING2.level = 0;
				tsen.LIGHTING2.unitcode = 1;
				tsen.LIGHTING2.cmnd = (PIRS == 1) ? light2_sOn : light2_sOff;
				tsen.LIGHTING2.rssi = 12;

				Debug(DEBUG_NORM, "4BS msg: Node %08X PIRS %u (%s)",
					nodeID, PIRS, (PIRS == 1) ? "Motion detected" : "Uncertain of occupancy status");

				sDecodeRXMessage(this, (const unsigned char *) &tsen.LIGHTING2, GetEEPLabel(enocean::RORG_4BS, Profile, iType), 255, m_Name.c_str());
			}
			else if (Profile == 0x09 && iType == 0x04)
			{ // A5-09-04, CO2 Gas Sensor with Temp and Humidity
				// Battery level is not reported, so set value 9 (OK)
				// TODO: Report battery level as 255 (unknown battery level) ?

				RBUF tsen;

				uint8_t HSN = bitrange(pFrame->DATA_BYTE0, 2, 0x01);
				if (HSN == 1)
				{
					float HUM = GetDeviceValue(pFrame->DATA_BYTE3, 0, 200, 0.0F, 100.0F);

					memset(&tsen, 0, sizeof(RBUF));
					tsen.HUM.packetlength = sizeof(tsen.HUM) - 1;
					tsen.HUM.packettype = pTypeHUM;
					tsen.HUM.subtype = sTypeHUM1; // LaCrosse TX3
					tsen.HUM.seqnbr = 0;
					tsen.HUM.id1 = pFrame->ID_BYTE2;
					tsen.HUM.id2 = pFrame->ID_BYTE1;
					tsen.HUM.humidity = (BYTE) round(HUM);
					tsen.HUM.humidity_status = Get_Humidity_Level(tsen.HUM.humidity);
					tsen.HUM.battery_level = 9; // OK
					tsen.HUM.rssi = 12;

					Debug(DEBUG_NORM, "4BS msg: Node %08X HUM %d%%", nodeID, tsen.HUM.humidity);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.HUM, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
				}

				float CONC = GetDeviceValue(pFrame->DATA_BYTE2, 0, 255, 0.0F, 2550.0F);

				Debug(DEBUG_NORM, "4BS msg: Node %08X CO2 %.1Fppm", nodeID, CONC);

				SendAirQualitySensor(pFrame->ID_BYTE2, pFrame->ID_BYTE1, 9, round(CONC), GetEEPLabel(enocean::RORG_4BS, Profile, iType));

				uint8_t TSN = bitrange(pFrame->DATA_BYTE0, 1, 0x01);
				if (TSN == 1)
				{
					float TMP = GetDeviceValue(pFrame->DATA_BYTE1, 0, 255, 0.0F, 51.0F);

					memset(&tsen, 0, sizeof(RBUF));
					tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
					tsen.TEMP.packettype = pTypeTEMP;
					tsen.TEMP.subtype = sTypeTEMP10; // TFA 30.3133
					tsen.TEMP.id1 = pFrame->ID_BYTE2;
					tsen.TEMP.id2 = pFrame->ID_BYTE1;
					// WARNING
					// battery_level & rssi fields are used here to transmit ID_BYTE0 value to decode_Temp in mainworker.cpp
					// decode_Temp assumes BatteryLevel to Unknown and SignalLevel to Not available
					tsen.TEMP.battery_level = pFrame->ID_BYTE0 & 0x0F;
					tsen.TEMP.rssi = (pFrame->ID_BYTE0 & 0xF0) >> 4;
					tsen.TEMP.tempsign = (TMP >= 0) ? 0 : 1;
					int at10 = round(std::abs(TMP * 10.0F));
					tsen.TEMP.temperatureh = (BYTE) (at10 / 256);
					at10 -= (tsen.TEMP.temperatureh * 256);
					tsen.TEMP.temperaturel = (BYTE) at10;

					Debug(DEBUG_NORM, "4BS msg: Node %08X TMP %.1F°C", nodeID, TMP);

					sDecodeRXMessage(this, (const unsigned char *) &tsen.TEMP, GetEEPLabel(enocean::RORG_4BS, Profile, iType), -1, m_Name.c_str());
				}
			}
		}
	}
	break;

	default:
	{
		char* pszHumenTxt = enocean_hexToHuman(pFrame);
		if (pszHumenTxt)
		{
			Log(LOG_NORM, "%s", pszHumenTxt);
			free(pszHumenTxt);
		}
	}
	break;
	}

	return true;
}
