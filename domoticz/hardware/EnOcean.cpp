#include "stdafx.h"
#include "EnOcean.h"
#include "../main/Logger.h"
#include "../main/Helper.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>
#include "hardwaretypes.h"
#include "../main/localtime_r.h"

#include <ctime>

#define ENOCEAN_RETRY_DELAY 30

/**
Class EnOcean

Part of this code it based on the work of Daniel Lechner, Andreas Dielacher

Note: This is for the ESP 2.0 protocol !!!

**/

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
#define HR_IDBASE " ID_Base: "
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

	m_stoprequested=false;
}

CEnOcean::~CEnOcean()
{
	clearReadCallback();
}

bool CEnOcean::StartHardware()
{
	m_retrycntr=ENOCEAN_RETRY_DELAY; //will force reconnect first thing

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
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		if (!isOpen())
		{
			if (m_retrycntr==0)
			{
				_log.Log(LOG_NORM,"EnOcean: serial retrying in %d seconds...", ENOCEAN_RETRY_DELAY);
			}
			m_retrycntr++;
			if (m_retrycntr>=ENOCEAN_RETRY_DELAY)
			{
				m_retrycntr=0;
				m_bufferpos=0;
				OpenSerialDevice();
			}
		}
	}
	_log.Log(LOG_NORM,"EnOcean: Serial Worker stopped...");
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
		sprintf(tempstring,"0x%02x%02x%02x%02x",pFrame->ID_BYTE3,pFrame->ID_BYTE2,pFrame->ID_BYTE1,pFrame->ID_BYTE0);
		tempstring += 10;
		//printf("TCM120 Module initialized");
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
		(void*)strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 8;  // we converted 4 bytes and each one takes 2 chars
		sprintf(tempstring,HR_DATA);
		tempstring += sizeof(HR_DATA)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(pFrame->DATA_BYTE3), 4);
		(void*)strcpy(tempstring,temphexstring);
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
		(void*)strcpy(tempstring,temphexstring);
		free(temphexstring);
		tempstring += 4;
		sprintf(tempstring,HR_DATA);
		tempstring += sizeof(HR_DATA)-1;
		temphexstring = enocean_gethex_internal((BYTE*)&(frame_6DT->DATA_BYTE5), 6);
		(void*)strcpy(tempstring,temphexstring);
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
		(void*)strcpy(tempstring,temphexstring);
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
	(void*)strcpy(tempstring,temphexstring);
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

	//Send Initial Reset
	enocean_data_structure iframe;
	iframe = tcm120_reset();
	write((const char*)&iframe,sizeof(enocean_data_structure));
	sleep_seconds(1);

	setReadCallback(boost::bind(&CEnOcean::readCallback, this, _1, _2));
	sOnConnected(this);

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
		m_buffer[m_bufferpos] = c;
		if(m_bufferpos == sizeof(m_buffer) - 1)
		{
			//overflow
			m_bufferpos = 0;
			_log.Log(LOG_ERROR,"EnOcean: Buffer_Overflow, Restarting...");
			m_retrycntr=ENOCEAN_RETRY_DELAY-2; //will force reconnect after 2 seconds
			m_bufferpos=0;
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
		else
		{
			m_bufferpos++;
			if (m_bufferpos==2)
			{
				//Check for valid sync bytes
				if (
					(m_buffer[0]!=C_S_BYTE1)||
					(m_buffer[1]!=C_S_BYTE2)
					)
				{
					//Invalid Sync, restart
					_log.Log(LOG_ERROR,"EnOcean: Sync_Error, Restarting...");
					m_retrycntr=ENOCEAN_RETRY_DELAY-2; //will force reconnect after 2 seconds
					m_bufferpos=0;
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
			}
			else if (m_bufferpos-3==C_LENGTH)
			{
				ParseData();
				m_bufferpos=0;
			}
		}
		ii++;
	}
}

void CEnOcean::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isOpen()) {
		write(pdata,length);
	}
}


bool CEnOcean::ParseData()
{
	enocean_data_structure *pFrame=(enocean_data_structure*)&m_buffer;
	unsigned char Checksum=enocean_calc_checksum(pFrame);
	if (Checksum!=pFrame->CHECKSUM)
		return false; //checksum Mismatch!

	long id = (pFrame->ID_BYTE3 << 24) + (pFrame->ID_BYTE2 << 16) + (pFrame->ID_BYTE1 << 8) + pFrame->ID_BYTE0;

	switch (pFrame->ORG)
	{
	case C_ORG_RPS:
		if (pFrame->STATUS & S_RPS_NU) {
			// NU == 1, N-Message
			_log.Log(LOG_NORM,"Received RPS N-Message Node 0x%08x Rocker ID: %i UD: %i Pressed: %i Second Rocker ID: %i SUD: %i Second Action: %i", id,
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_RID) >> DB3_RPS_NU_RID_SHIFT, 
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_UD) >> DB3_RPS_NU_UD_SHIFT, 
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_PR)>>DB3_RPS_NU_PR_SHIFT,
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_SRID)>>DB3_RPS_NU_SRID_SHIFT, 
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_SUD)>>DB3_RPS_NU_SUD_SHIFT,
				(pFrame->DATA_BYTE3 & DB3_RPS_NU_SA)>>DB3_RPS_NU_SA_SHIFT);
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
