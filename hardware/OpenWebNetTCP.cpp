/************************************************************************

Legrand MyHome / OpenWebNet Interface board driver for Domoticz (with LAN interface)
Date: 24-01-2016
Written by: Stéphane Lebrasseur

Date: 04-11-2016
Update by: Matteo Facchetti

Date: 13-09-2017
Update by: Marco Olivieri - Olix81 -

License: Public domain

************************************************************************/
#include "stdafx.h"
#include "OpenWebNetTCP.h"
#include "openwebnet/bt_openwebnet.h"

#include "../main/Logger.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"
#include "csocket.h"

#include <string.h>
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"

#include "../notifications/NotificationHelper.h"

#define OPENWEBNET_HEARTBEAT_DELAY      1
#define OPENWEBNET_STATUS_NB_HEARTBEAT  600
#define OPENWEBNET_RETRY_DELAY          30
#define OPENWEBNET_POLL_INTERVAL        1000
#define OPENWEBNET_BUFFER_SIZE          1024
#define OPENWEBNET_SOCKET_SUCCESS       0

#define	SCAN_TIME_REQ_AUTO_UPDATE_POWER	(14400) // 4hour = 240min = 14400sec
#define SCAN_TIME_REQ_ENERGY_TOTALIZER	(900)	// 15min = 900sec

/* 2 bits function.. */
#define OPENWEBNET_DEFAULT				0x00000000	// 0
#define OPENWEBNET_2CHARS				0x00001000	// 1
#define OPENWEBNET_4CHARS				0x00002000	// 2
#define OPENWEBNET_AREA_ID				0x00004000	// 4
#define OPENWEBNET_GROUP_ID				0x00008000	// 8
#define OPENWEBNET_MASK_WHERE_FUNCT		0x0000f000

#define OPENWEBNET_AUTOMATION					"AUTOMATION"
#define OPENWEBNET_LIGHT						"LIGHT"
#define OPENWEBNET_TEMPERATURE					"TEMPERATURE Z"
#define OPENWEBNET_TEMPERATURE_STATUS			"TEMPERATURE STATUS Z"
#define OPENWEBNET_BURGLAR_ALARM_SENSOR			"ALARM SENSOR ZONE"
#define OPENWEBNET_BURGLAR_ALARM_BATTERY		"ALARM BATTERY"
#define OPENWEBNET_BURGLAR_ALARM_NETWORK		"ALARM NETWORK"
#define OPENWEBNET_BURGLAR_ALARM_SYS_STATUS		"ALARM SYSTEM STATUS"
#define OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT "ALARM SYSTEM ENGAGEMENT"
#define OPENWEBNET_CENPLUS            			"CEN PLUS"
#define OPENWEBNET_AUXILIARY					"AUXILIARY"
#define OPENWEBNET_DRY_CONTACT					"DRYCONTACT"
#define OPENWEBNET_ENERGY_MANAGEMENT			"ENERGY MANAGEMENT"
#define OPENWEBNET_SOUND_DIFFUSION				"SOUND DIFFUSION"

enum
{
	ID_DEV_BURGLAR_SYS_STATUS		= 17,	// ex 0xff
	ID_DEV_BURGLAR_NETWORK			= 18,	// ex 0xfe
	ID_DEV_BURGLAR_BATTERY			= 19,	// ex 0xfd
	ID_DEV_BURGLAR_SYS_ENGAGEMENT	= 20	// ex 0xfc
};


 /**
	 Create new hardware OpenWebNet instance
 **/
COpenWebNetTCP::COpenWebNetTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &ownPassword, const int ownScanTime, const int ownSynch) : m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_ownPassword = ownPassword;

	Log(LOG_STATUS, "scan devices %s(%d)!", ownScanTime ? "ENABLED" : "DISABLED", ownScanTime);
	Log(LOG_STATUS, "sync time %s(%d)!", ownSynch ? "ENABLED" : "DISABLED", ownSynch);

	m_ownScanTime = ownScanTime;
	m_ownSynch = ownSynch * 60;	// minutes to seconds..
	
	m_heartbeatcntr = OPENWEBNET_HEARTBEAT_DELAY;
	m_pStatusSocket = nullptr;
}

/**
	Start Hardware OpneWebNet Monitor/Worker Service
**/
bool COpenWebNetTCP::StartHardware()
{
	RequestStart();

	m_bIsStarted = true;
	mask_request_status = 0x1; // Set scan all devices
	LastScanTimeEnergy = LastScanTimeEnergyTot = LastScanTime = LastScanSync = 0;	// Force first request command

	//Start monitor thread
	m_monitorThread = std::make_shared<std::thread>([this] { MonitorFrames(); });
	SetThreadName(m_monitorThread->native_handle(), "OpenWebNetTCPMF");

	//Start worker thread
	if (m_monitorThread != nullptr)
	{
		m_heartbeatThread = std::make_shared<std::thread>([this] { Do_Work(); });
		SetThreadName(m_heartbeatThread->native_handle(), "OpenWebNetTCPW");
	}

	return (m_monitorThread != nullptr && m_heartbeatThread != nullptr);
}

/**
	Stop Hardware OpenWebNet Monitor/Worker Service
**/
bool COpenWebNetTCP::StopHardware()
{
	RequestStop();

	//Force socket close to stop the blocking thread?
	if (m_pStatusSocket != nullptr)
	{
		m_pStatusSocket->close();
	}

	try
	{
		if (m_monitorThread)
		{
			m_monitorThread->join();
			m_monitorThread.reset();
		}
	}
	catch (...)
	{
	}

	try
	{
		if (m_heartbeatThread)
		{
			m_heartbeatThread->join();
			m_heartbeatThread.reset();
		}
	}
	catch (...)
	{
	}

	m_bIsStarted = false;
	return true;
}

/**
	Close and delete the socket
**/
void COpenWebNetTCP::disconnect()
{
	if (m_pStatusSocket != nullptr)
	{
		Log(LOG_STATUS, "Disconnect");
		m_pStatusSocket->close();
		delete m_pStatusSocket;
		m_pStatusSocket = nullptr;
	}
}


/**
   Check socket connection
**/
bool COpenWebNetTCP::isStatusSocketConnected()
{
	return m_pStatusSocket != nullptr && m_pStatusSocket->getState() == csocket::CONNECTED;
};

/**
   write ...
**/
bool COpenWebNetTCP::ownWrite(csocket *connectionSocket, const char* pdata, size_t size)
{
	int bytesWritten = connectionSocket->write(pdata, static_cast<unsigned int>(size));
	if (bytesWritten != size) 
	{
		Log(LOG_ERROR, "partial write: %u/%u", bytesWritten, (unsigned int)size);
		return (false);
	}
	return (true);
}

/**
   read ...
**/
int COpenWebNetTCP::ownRead(csocket *connectionSocket, char* pdata, size_t size)
{
	memset(pdata, 0, size);
	int read = connectionSocket->read(pdata, static_cast<unsigned int>(size), false);
	return (read);
}

/**
   Calculate 'nonce-hash' authentication
**/
uint32_t COpenWebNetTCP::ownCalcPass(const std::string &password, const std::string &nonce)
{
	uint32_t msr = 0x7FFFFFFF;
	uint32_t m_1 = (uint32_t)0xFFFFFFFF;
	uint32_t m_8 = (uint32_t)0xFFFFFFF8;
	uint32_t m_16 = (uint32_t)0xFFFFFFF0;
	uint32_t m_128 = (uint32_t)0xFFFFFF80;
	uint32_t m_16777216 = (uint32_t)0xFF000000;
	bool flag = true;
	uint32_t num1 = 0;
	uint32_t num2 = 0;
	uint32_t numx = 0;
	uint32_t length = 0;

	uint32_t idx;

	for (idx = 0; idx < nonce.length(); idx++)
	{
		if ((nonce[idx] >= '1') && (nonce[idx] <= '9'))
		{
			if (flag)
			{
				num2 = (uint32_t)atoi(password.c_str());
				flag = false;
			}
		}

		switch (nonce[idx])
		{
		case '1':
			num1 = num2 & m_128;
			num1 = num1 >> 1;
			num1 = num1 & msr;
			num1 = num1 >> 6;
			num2 = num2 << 25;
			num1 = num1 + num2;
			break;
		case '2':
			num1 = num2 & m_16;
			num1 = num1 >> 1;
			num1 = num1 & msr;
			num1 = num1 >> 3;
			num2 = num2 << 28;
			num1 = num1 + num2;
			break;
		case '3':
			num1 = num2 & m_8;
			num1 = num1 >> 1;
			num1 = num1 & msr;
			num1 = num1 >> 2;
			num2 = num2 << 29;
			num1 = num1 + num2;
			break;
		case '4':
			num1 = num2 << 1;
			num2 = num2 >> 1;
			num2 = num2 & msr;
			num2 = num2 >> 30;
			num1 = num1 + num2;
			break;
		case '5':
			num1 = num2 << 5;
			num2 = num2 >> 1;
			num2 = num2 & msr;
			num2 = num2 >> 26;
			num1 = num1 + num2;
			break;
		case '6':
			num1 = num2 << 12;
			num2 = num2 >> 1;
			num2 = num2 & msr;
			num2 = num2 >> 19;
			num1 = num1 + num2;
			break;
		case '7':
			num1 = num2 & 0xFF00;
			num1 = num1 + ((num2 & 0xFF) << 24);
			num1 = num1 + ((num2 & 0xFF0000) >> 16);
			num2 = num2 & m_16777216;
			num2 = num2 >> 1;
			num2 = num2 & msr;
			num2 = num2 >> 7;
			num1 = num1 + num2;
			break;
		case '8':
			num1 = num2 & 0xFFFF;
			num1 = num1 << 16;
			numx = num2 >> 1;
			numx = numx & msr;
			numx = numx >> 23;
			num1 = num1 + numx;
			num2 = num2 & 0xFF0000;
			num2 = num2 >> 1;
			num2 = num2 & msr;
			num2 = num2 >> 7;
			num1 = num1 + num2;
			break;
		case '9':
			num1 = ~num2;
			break;
		default:
			num1 = num2;
			break;
		}
		num2 = num1;
	}

	return (num1 & m_1);
}

/**
	Perform conversion 80/128 DEC-chars to 40/64 HEX-chars
**/
std::string COpenWebNetTCP::decToHexStrConvert(const std::string &paramString)
{
	char retStr[256];
	size_t idxb, idxh;
	for (idxb = 0, idxh = 0; idxb < paramString.length(); idxb += 2, idxh++)
	{
		std::string str = paramString.substr(idxb, 2);
		sprintf(&retStr[idxh], "%x", atoi(str.c_str()));
	}
	return std::string(retStr);
}

/**
	Perform conversion HEX-40/64 chars to 80/128 DEC-chars
**/
std::string COpenWebNetTCP::hexToDecStrConvert(const std::string &paramString)
{
	uint32_t bval;
	size_t idxb, idxh;
	char retStr[256];
	for (idxb = 0, idxh = 0; idxb < paramString.length(); idxb++, idxh += 2)
	{
		std::stringstream s_strid;
		s_strid << std::hex << paramString.substr(idxb, 1);
		s_strid >> bval;
		sprintf(&retStr[idxh], "%02u", bval & 0xf);
	}
	
	return std::string(retStr);
}

/**
	Perform conversion byte to HEX-chars
**/
std::string COpenWebNetTCP::byteToHexStrConvert(uint8_t *digest, size_t digestLen, char *pArray)
{
	size_t idxb, idxh;
	char arrayOfChar1[] = "0123456789abcdef";
	for (idxb = 0, idxh = 0; idxb < digestLen; idxb++, idxh += 2)
	{
		uint8_t bval = digest[idxb] & 0xFF;
		pArray[idxh] = arrayOfChar1[(bval >> 4) & 0xf];
		pArray[idxh + 1] = arrayOfChar1[bval & 0xF];
	}
	pArray[idxh] = 0;
	return(std::string(pArray));
}

/**
	Perform SHA1/SHA256 and convert into HEX-chars
**/
std::string COpenWebNetTCP::shaCalc(const std::string &paramString, int auth_type)
{
	uint8_t *digest;
	uint8_t strArray[OPENWEBNET_BUFFER_SIZE];
	memset(strArray, 0, sizeof(strArray));
	memcpy(strArray, paramString.c_str(), paramString.length());

	if (auth_type == 0)
	{
		// Perform SHA1
		digest = SHA1(strArray, paramString.length(), nullptr);
		char arrayOfChar2[(SHA_DIGEST_LENGTH * 2) + 1];
		return (byteToHexStrConvert(digest, SHA_DIGEST_LENGTH, arrayOfChar2));
	}
	// Perform SHA256
	digest = SHA256(strArray, paramString.length(), nullptr);
	char arrayOfChar2[(SHA256_DIGEST_LENGTH * 2) + 1];
	return (byteToHexStrConvert(digest, SHA256_DIGEST_LENGTH, arrayOfChar2));
}

/**
	Perform HMAC authentication
**/
bool COpenWebNetTCP::hmacAuthentication(csocket *connectionSocket, int auth_type)
{
	// Write ACK
	ownWrite(connectionSocket, OPENWEBNET_MSG_OPEN_OK, strlen(OPENWEBNET_MSG_OPEN_OK));

	// Read server Response
	char databuffer[OPENWEBNET_BUFFER_SIZE];
	int read = ownRead(connectionSocket, databuffer, sizeof(databuffer));
	bt_openwebnet responseSrv(std::string(databuffer, read));
	if (responseSrv.IsPwdFrame())
	{
		struct timeval tv;
		gettimeofday(&tv, nullptr);

		// receive Ra
		const std::string strRcvSrv = responseSrv.Extract_who();							  // Ra from server in DEC-chars
		const std::string strRa = decToHexStrConvert(strRcvSrv);							  // convert Ra in HEX-chars
		std::stringstream strRb_rand;
		strRb_rand << "Time" << tv.tv_sec << tv.tv_usec;									  // get random Rb from time..
		const std::string strRb = shaCalc(strRb_rand.str(), auth_type);						  // sha of Rb and convert into HEX-chars	
		const std::string strA = "736F70653E";												  // is the client identity (HEX-chars)
		const std::string strB = "636F70653E";												  // is the server identity (HEX-chars)
		const std::string strKab = shaCalc(m_ownPassword, auth_type);						  // perform SHA of password
		const std::string strHMAC = shaCalc(strRa + strRb + strA + strB + strKab, auth_type); // HMAC

#if 0
		// print some logs for debug..
		Log(LOG_STATUS, "HMAC Ra digits received: '%s'", strRcvSrv.c_str());
		Log(LOG_STATUS, "Ra = '%s'", strRa.c_str());
		Log(LOG_STATUS, "Rb = '%s'", strRb.c_str());
		Log(LOG_STATUS, "A = '%s', B = '%s'", strA.c_str(), strB.c_str());
		Log(LOG_STATUS, "pwd = '%s', Kab = '%s'", m_ownPassword.c_str(), strKab.c_str());
		Log(LOG_STATUS, "HMAC(Ra,Rb,A,B,Kab) = '%s'", strHMAC.c_str());
#endif
		// Write HMAC
		const std::string strSend = "*#" + hexToDecStrConvert(strRb) + "*" + hexToDecStrConvert(strHMAC) + "##";
		ownWrite(connectionSocket, strSend.c_str(), strSend.length());

		// Server response....
		read = ownRead(connectionSocket, databuffer, sizeof(databuffer));
		bt_openwebnet responseSrv2(std::string(databuffer, read));
		if (responseSrv2.IsPwdFrame())
		{
			const std::string strRcvSrv2 = decToHexStrConvert(responseSrv2.Extract_who());
			const std::string strHMAC2 = shaCalc(strRa + strRb + strKab, auth_type);

			if (strHMAC2 == strRcvSrv2)
			{
				ownWrite(connectionSocket, OPENWEBNET_MSG_OPEN_OK, strlen(OPENWEBNET_MSG_OPEN_OK)); // Write ACK
				return (true); // HMAC authentication OK
			}
			Log(LOG_ERROR, "HMAC(Ra,Rb,Kab) received: '%s'", strRcvSrv2.c_str());
			Log(LOG_ERROR, "not match with: '%s'", strHMAC2.c_str());
		}
	}
	Log(LOG_ERROR, "HMAC authentication ERROR!");
	return false; // error!
}

/**
	Perform nonce-hash authentication
**/
bool COpenWebNetTCP::nonceHashAuthentication(csocket *connectionSocket, const std::string &nonce)
{
	std::stringstream frame;
	/** calculate nonce-hash **/
	uint32_t ownHash = ownCalcPass(m_ownPassword, nonce);
	/** write frame with nonce-hash **/
	frame << "*#";
	frame << ownHash;
	frame << "##";
	ownWrite(connectionSocket, frame.str().c_str(), frame.str().length());

	char databuffer[OPENWEBNET_BUFFER_SIZE];
	int read = ownRead(connectionSocket, databuffer, sizeof(databuffer));
	bt_openwebnet responseNonce2(std::string(databuffer, read));

	if (responseNonce2.IsOKFrame()) return true; // hash authentication OK

	Log(LOG_ERROR, "hash authentication ERROR!");
	return false;
}

/**
	Perform nonce-hash authentication
**/

bool COpenWebNetTCP::ownAuthentication(csocket *connectionSocket)
{
	char databuffer[OPENWEBNET_BUFFER_SIZE];
	int read = ownRead(connectionSocket, databuffer, sizeof(databuffer));
	bt_openwebnet responseNonce(std::string(databuffer, read));

	//Log(LOG_STATUS, "authentication rcv: '%s'", responseNonce.Extract_frame().c_str());

	if (responseNonce.IsPwdFrame())
	{
		if (!m_ownPassword.length())
		{
			Log(LOG_ERROR, "no password set for a unofficial bticino gateway");
			return false;
		}

		// Hash authentication for unofficial gateway
		return(nonceHashAuthentication(connectionSocket, responseNonce.Extract_who()));
	}
	if (responseNonce.IsNormalFrame())
	{
		if (!m_ownPassword.length())
		{
			Log(LOG_ERROR, "bticino gateway requires the password");
			return false;
		}

		// TODO: only alphanumeric password....

		const std::string strFrame = responseNonce.Extract_frame();
		if (strFrame == OPENWEBNET_AUTH_REQ_SHA1) // *98*1##
		{
			// HMAC authentication with SHA-1
			return (hmacAuthentication(connectionSocket, 0));
		}
		if (strFrame == OPENWEBNET_AUTH_REQ_SHA2) // *98*2##
		{
			// HMAC authentication with SHA-256
			return (hmacAuthentication(connectionSocket, 1));
		}
		Log(LOG_ERROR, "frame request error:'%s'", strFrame.c_str());
		return false;
	}
	if (responseNonce.IsOKFrame())
	{
		// no authentication required..ok!
		//Log(LOG_STATUS, "authentication OK, no password!");
		return true;
	}
	Log(LOG_ERROR, "ERROR_FRAME? %d", responseNonce.m_frameType);
	return false;
}

/**
   Connection to the gateway OpenWebNet
**/
csocket* COpenWebNetTCP::connectGwOwn(const char *connectionMode)
{
	if (m_szIPAddress.empty() || m_usIPPort == 0 || m_usIPPort > 65535)
	{
		Log(LOG_ERROR, "Cannot connect to gateway, empty  IP Address or Port");
		return nullptr;
	}

	/* new socket for command and session connection */
	csocket *connectionSocket = new csocket();

	connectionSocket->connect(m_szIPAddress.c_str(), m_usIPPort);
	if (connectionSocket->getState() != csocket::CONNECTED)
	{
		Log(LOG_ERROR, "Cannot connect to gateway, Unable to connect to specified IP Address on specified Port");
		disconnect();  // disconnet socket if present
		return nullptr;
	}

	char databuffer[OPENWEBNET_BUFFER_SIZE];
	int read = ownRead(connectionSocket, databuffer, OPENWEBNET_BUFFER_SIZE);
	bt_openwebnet responseSession(std::string(databuffer, read));
	if (!responseSession.IsOKFrame())
	{
		Log(LOG_ERROR, "failed to begin session, (%s:%d)-> '%s'", m_szIPAddress.c_str(), m_usIPPort, databuffer);
		disconnect();  // disconnet socket if present
		return nullptr;
	}

	ownWrite(connectionSocket, connectionMode, strlen(connectionMode));

	if (!ownAuthentication(connectionSocket))
		return nullptr;

	return connectionSocket;
}

/**
	Thread Monitor: get update from the OpenWebNet gateway and add new devices if necessary
**/
void COpenWebNetTCP::MonitorFrames()
{
	while (!IsStopRequested(0))
	{
		if (!isStatusSocketConnected())
		{
			disconnect();  // disconnet socket if present
			time_t atime = time(nullptr);
			if ((atime%OPENWEBNET_RETRY_DELAY) == 0)
			{
				if ((m_pStatusSocket = connectGwOwn(OPENWEBNET_EVENT_SESSION)))
				{
					// Monitor session correctly open
					Log(LOG_STATUS, "Monitor session connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);
					sOnConnected(this);
				}
				else
				{
					Log(LOG_STATUS, "Monitor not connected, retrying in %d seconds...", OPENWEBNET_RETRY_DELAY);
					sleep_seconds(1);
				}
			}
		}
		else
		{
			// Connected
			bool bIsDataReadable = true;
			m_pStatusSocket->canRead(&bIsDataReadable, 3.0F);
			if (bIsDataReadable)
			{
				char data[OPENWEBNET_BUFFER_SIZE];
				int bread = ownRead(m_pStatusSocket, data, OPENWEBNET_BUFFER_SIZE);

				if (IsStopRequested(0))
					break;

				if ((bread == 0) || (bread < 0)) {
					Log(LOG_ERROR, "monitor connection closed!");
					disconnect();  // disconnet socket if present
				}
				else
				{
					std::lock_guard<std::mutex> l(readQueueMutex);
					std::vector<bt_openwebnet> responses;
					ParseData(data, bread, responses);

					for (auto iter = responses.begin(); iter != responses.end(); iter++)
					{
						if (iter->IsNormalFrame() || iter->IsMeasureFrame())
						{
							Log(LOG_STATUS, "received=%s", bt_openwebnet::frameToString(*iter).c_str());
							UpdateDeviceValue(iter);
						}
						//else
						//    Log(LOG_ERROR, "SKIPPED FRAME=%s", frameToString(*iter).c_str());
					}
				}
			}
			else
				sleep_milliseconds(100);
		}
	}
	if (isStatusSocketConnected())
	{
		try {
			disconnect();  // disconnet socket if present
		}
		catch (...)
		{
			//Don't throw from a Stop command
		}
	}

	Log(LOG_STATUS, "monitor worker stopped...");
}

/**
	Insert/Update temperature device
**/
void COpenWebNetTCP::UpdateTemp(const int who, const int where, float fval, const int iInterface, const int BatteryLevel, const char *devname)
{
	/**
		NOTE:	interface id (bus identifier) go in 'ID' field
				ID is: ((who << 12) & 0xC000) | (iInterface & 0x3c00) (where & 0x3FF)
	**/

	//zone are max 99, every zone can have 8 slave sensor. Slave sensor address. YZZ: y as slave address (1-8) ,zz zone number (1-99)
	// so who is always 4, iInterface is 0-9, where 0x001-0x383 (1 to 899)
	int cnode = ((who << 12) & 0xC000) | (iInterface & 0x3c00) | (where & 0x3FF);
	SendTempSensor(cnode, BatteryLevel, fval, devname);
}

/**
	Update temperature setpoint
**/
void COpenWebNetTCP::UpdateSetPoint(const int who, const int where, float fval, const int iInterface, const char *devname)
{
	/**
		NOTE:	interface id (bus identifier) go in 'ID' field
				ID is: ((who << 16) | (iInterface << 8) | where)

				where is setpoint zone (1 - 99)
	**/
	SendSetPointSensor(0, (who & 0xFF), (iInterface & 0xff), (where & 0xFF), 1, 255, fval, devname);
}

/**
Get last Meter Usage
**/
bool COpenWebNetTCP::GetValueMeter(const int NodeID, const int ChildID, double *usage, double *energy)
{
	int dID = (NodeID << 8) | ChildID;
	char szTmp[30];
	sprintf(szTmp, "%08X", dID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szTmp, int(pTypeGeneral), int(sTypeKwh));
	if (!result.empty())
	{
		std::string sup, sValue = result[0][0];

		if (usage)
			*usage = (float)atof(sValue.c_str());

		if (energy)
		{
			size_t pos = sValue.find(';', 0);
			if (pos >= 0)
			{
				sup = sValue.substr(pos + 1);
				*energy = (float)atof(sup.c_str());
			}
		}

		return true;
	}
	return false; // not found
}

/**
	Update Active power usage
**/
void COpenWebNetTCP::UpdatePower(const int who, const int where, double fval, const int iInterface, const int BatteryLevel, const char *devname)
{
	/**
		NOTE:	interface id (bus identifier) go in 'ID' field
				ID is: ((iInterface << 16) | (who << 8) | where)
	**/
	double energy = 0.;

	int NodeId = (iInterface << 8) | who;
	GetValueMeter(NodeId, where, nullptr, &energy);
	SendKwhMeter(NodeId, where, BatteryLevel, fval, (energy / 1000.), devname);
}


/**
	Update total energy
**/
void COpenWebNetTCP::UpdateEnergy(const int who, const int where, double fval, const int iInterface, const int BatteryLevel, const char *devname)
{
	/**
		NOTE:	interface id (bus identifier) go in 'ID' field
				ID is: ((iInterface << 16) | (who << 8) | where)
	**/

	double usage = 0.;
	int NodeId = (iInterface << 8) | who;
	GetValueMeter(NodeId, where, &usage, nullptr);
	SendKwhMeter(NodeId, where, BatteryLevel, usage, fval, devname);
}

/**
	Update Temperature probe status
**/
void COpenWebNetTCP::UpdateTempProbe(const int who, const int where, const int child, const int idx_str, std::string sStatus, const int iInterface, const int BatteryLevel, const char *devname)
{
	bool bExists;
	std::string strvalue, str1, str2;
	int NodeID = ((who << 12) & 0xC000) | (iInterface & 0x3c00) | (where & 0x3FF);
	int tChild = (child < 0) ? where : child;

	strvalue = GetTextSensorText(NodeID, tChild, bExists);
	if (bExists == false) 
	{
		str1 = "";
		str2 = ""; 
	} 
	else 
	{
		std::stringstream ss(strvalue);
		std::string token;

		if (std::getline(ss, token, ',')) str1 = token;
		if (std::getline(ss, token, ',')) str2 = token;
	}

	if ((idx_str == 0) && (str1 != sStatus) ||
	    (idx_str == 1) && (str2 != sStatus))
	{
			if (idx_str == 0)
				strvalue = sStatus + "," + str2;
			else if (idx_str == 1)
				strvalue = str1 + "," + sStatus;
			SendTextSensor(NodeID, tChild, BatteryLevel, strvalue, devname);
	}		
}

/**
	Insert/Update Alarm system and sensor
**/
void COpenWebNetTCP::UpdateAlarm(const int who, const int where, const int Command, const char* sCommand, const int iInterface, const int BatteryLevel, const char* devname)
{
	/**
		NOTE:	interface id (bus identifier) go in 'ID' field

		each interface can occupy 1 to 20 IDs.
		so you can calculate the ID:
		ID + interface * 20 so for example:

		iInterface = 0, Id from 1 to 20
		iInterface = 1, Id from 21 to 40
		iInterface = 2, Id from 41 to 60
		and so on..
	**/

	//make device ID
	//int NodeID = (((int)who << 16) & 0xFFFF0000) | (((int)where) & 0x0000FFFF);
	int NodeID = (where & 0xff) + iInterface * 20;

	char szIdx[10];
	sprintf(szIdx, "%u", NodeID);

	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s')", m_HwdID, szIdx);
	if (!result.empty())
	{
		//check if we have a change, if not do not update it
		int nvalue = atoi(result[0][0].c_str());
		if (Command == -1 || nvalue == Command) return; // update not necessary
	}

	SendAlertSensor(NodeID, BatteryLevel, Command, sCommand, devname);
}

void COpenWebNetTCP::SendGeneralSwitch(const int NodeID, const uint8_t ChildID, const int BatteryLevel, const int cmd, const int level, const std::string& defaultname, const int RssiLevel /* =12 */)
{
	_tGeneralSwitch gSwitch;
	gSwitch.id = NodeID;
	gSwitch.len = sizeof(_tGeneralSwitch) - 1;
	gSwitch.type = pTypeGeneralSwitch;
	gSwitch.subtype = sSwitchTypeAC;
	gSwitch.unitcode = ChildID;
	gSwitch.cmnd = cmd;
	gSwitch.level = level;
	gSwitch.rssi = (uint8_t)RssiLevel;
	sDecodeRXMessage(this, (const unsigned char*)& gSwitch, defaultname.c_str(), BatteryLevel, m_Name.c_str());
}

/**
	Insert/Update blinds device
**/
void COpenWebNetTCP::UpdateBlinds(const int who, const int where, const int Command, const int iInterface, const int iLevel, const int BatteryLevel, const char *devname)
{
	//NOTE: interface id (bus identifier) go in 'Unit' field
	//make device ID
	int NodeID = (((int)who << 16) & 0xFFFF0000) | (((int)where) & 0x0000FFFF);

	/* insert switch type */
	char szIdx[10];
	sprintf(szIdx, "%08X", NodeID);

	int nvalue, slevel, switch_type;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,nValue,sValue,SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", m_HwdID, szIdx, iInterface);
	if (result.empty())
	{
		nvalue = 0;
		slevel = 0;
		switch_type = (iLevel < 0) ? STYPE_VenetianBlindsEU : STYPE_BlindsPercentage;
		m_sql.InsertDevice(m_HwdID, 0, szIdx, iInterface, pTypeGeneralSwitch, sSwitchTypeAC, switch_type, 0, "", devname);
	}
	else
	{
		nvalue = atoi(result[0][1].c_str());
		slevel = atoi(result[0][2].c_str());
		switch_type = atoi(result[0][3].c_str());
	}

	if (
		(switch_type == STYPE_BlindsPercentage)
		&& (iLevel < 0)
		)
		return;

	int cmd = -1;
	switch (Command)
	{
	case AUTOMATION_WHAT_STOP:			// 0
	case AUTOMATION_WHAT_STOP_ADVANCED: // 10
		cmd = gswitch_sStop;
		break;
	case AUTOMATION_WHAT_UP:			// 1
	case AUTOMATION_WHAT_DOWN_ADVANCED: // 12 INVERTED
		cmd = gswitch_sOff;
		break;
	case AUTOMATION_WHAT_DOWN:			// 2
	case AUTOMATION_WHAT_UP_ADVANCED:	// 11 INVERTED
		cmd = gswitch_sOn;
		break;
	default:
		Log(LOG_ERROR, "Command %d invalid!", Command);
		return;
	}
	
	// if (iLevel < 0)  is a Normal Frame and device is standard
	// if (iLevel >= 0) is a Meseaure Frame (percentual) and device is Advanced
	int level = (iLevel < 0) ? 0 : iLevel;
		
	//check if we have a change, if not do not update it
	if (cmd == nvalue)
	{
		if (switch_type == STYPE_VenetianBlindsEU) 
			return; // normal blind without change

		// Check Level percentage blind
		if (slevel == level)
			return;
	}

	// verify command for advanced type
	if (switch_type == STYPE_BlindsPercentage)
	{
		cmd = (iLevel == 0) ? gswitch_sOff : gswitch_sSetLevel;
	}

	SendGeneralSwitch(NodeID, iInterface, BatteryLevel, cmd, level, devname);
}


/**
	Insert/Update  CEN PLUS
**/
void COpenWebNetTCP::UpdateCenPlus(const int who, const int where, const int Command, const int iAppValue, const int what, const int iInterface, const int BatteryLevel, const char *devname)
{
	//NOTE: interface id (bus identifier) go in 'Unit' field
	//make device ID
	int NodeID = (int)((((int)who << 16) & 0xFFFF0000) | (((int)(where + (iAppValue * 2) + (what * 3))) & 0x0000FFFF));
	char szIdx[10];
	sprintf(szIdx, "%08X", NodeID);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,nValue,SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", m_HwdID, szIdx, iInterface);
	
	int nvalue;
	if (result.empty())
	{
		// First insert, set SwitchType = STYPE_Contact, so we have a correct contact device
		nvalue = 0;
		m_sql.InsertDevice(m_HwdID, 0, szIdx, iInterface, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_Contact, 0, "Unavailable", devname);
	}
	else
	{
		nvalue = atoi(result[0][1].c_str());
	}

	//check if we have a change, if not do not update it
	bool bOn = (bool)Command;
	if ((!bOn) && (nvalue == gswitch_sOff)) return;
	if ((bOn && (nvalue == gswitch_sOn))) return;

	int cmnd = (bOn) ? gswitch_sOn : gswitch_sOff;

	SendGeneralSwitch(NodeID, iInterface, BatteryLevel, cmnd, 0, devname);
}

/**
	Insert/Update sound diffusion device
**/
void COpenWebNetTCP::UpdateSoundDiffusion(const int who, const int where, const int what, const int iInterface, const int BatteryLevel, const char* devname)
{
	//NOTE: interface id (bus identifier) go in 'Unit' field
	//make device ID
	int NodeID = (((int)who << 16) & 0xFFFF0000) | (((int)where) & 0x0000FFFF);

	/* insert switch type */
	char szIdx[10];
	sprintf(szIdx, "%08X", NodeID);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", m_HwdID, szIdx, iInterface);
	if (result.empty())
	{
		//m_sql.InsertDevice(m_HwdID, 0, szIdx, iInterface, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_Media, 0, "Unavailable", "OpenWebNet Media", 12, 255, 1);
	}

	//TODO: manage SoundDiffusion device like dimmer (on, off and set volume) or like media device (check how to do it)
}

/**
	Insert/Update  switch device
**/
void COpenWebNetTCP::UpdateSwitch(const int who, const int where, const int what, const int iInterface, const int BatteryLevel, const char* devname)
{
	//NOTE: interface id (bus identifier) go in 'Unit' field
	//make device ID
	int NodeID = (((int)who << 16) & 0xFFFF0000) | (((int)where) & 0x0000FFFF);

	/* insert switch type */
	char szIdx[10];
	sprintf(szIdx, "%08X", NodeID);

	int nvalue, slevel, switch_type;
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID,nValue,sValue,SwitchType FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", m_HwdID, szIdx, iInterface);

	if (result.empty())
	{
		nvalue = 0;
		slevel = 0;
		if ((who == WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION) || (who == (WHO_TEMPERATURE_CONTROL + 500)) || (who == (WHO_TEMPERATURE_CONTROL + 600)))
		{
			// First insert, set SwitchType = STYPE_Contact, so we have a correct contact device
			switch_type = STYPE_Contact;
		}
		else
			switch_type = STYPE_OnOff;
		m_sql.InsertDevice(m_HwdID, 0, szIdx, iInterface, pTypeGeneralSwitch, sSwitchTypeAC, switch_type, 0, "Unavailable", devname);
	}
	else
	{
		nvalue = atoi(result[0][1].c_str());
		slevel = atoi(result[0][2].c_str());
		switch_type = atoi(result[0][3].c_str());
	}

	int level = (what > 1) ? (what * 10) : 0;

	//check if we have a change, if not do not update it
	bool bOn = (bool)what;
	if ((!bOn) && (nvalue == gswitch_sOff)) return;
	if ((bOn && (nvalue != gswitch_sOff)))
	{
		if (slevel == level) return; //Check Level
	}

	int cmnd = gswitch_sOff;
	if (bOn) cmnd = (level != 0) ? gswitch_sSetLevel : gswitch_sOn;

	SendGeneralSwitch(NodeID, iInterface, BatteryLevel, cmnd, level, devname);
}

/**
	Decode where and get tipe of command with details..
**/
void COpenWebNetTCP::decodeWhereAndFill(const int who, const std::string &where, std::vector<std::string> whereParam, std::string *devname, int *iWhere)
{
	size_t wlen = where.length();	
	int iArea = -1;
	*iWhere = -1;

	if (wlen == 0)
	{
		/* GROUP light device */
		if (!whereParam.empty())
		{
			*iWhere = atoi(whereParam[0].c_str()) + OPENWEBNET_GROUP_ID;
			*devname += " GROUP " + whereParam[0];
		}
	}
	else if ((who == WHO_LIGHTING) || (who == WHO_AUTOMATION))
	{
		/*
			Decode where...
				A = 00;			PL [01 - 15];
				A [1 -9];		PL [1 - 9];
				A = 10;			PL [01 - 15];
				A [01 - 09];	PL [10 - 15];
		*/
		if (wlen == 1)
		{
			// General o Area 1-9 command
			int tmp = atoi(where.c_str());
			if (tmp > 0) iArea = tmp;
		}
		else if ((wlen == 2) && (where == "00"))	// Area 0 (00) command
		{
			iArea = 0;
		}
		else if ((wlen == 3) && (where == "100"))	// area 10 (100) command
		{
			iArea = 10;
		}
		else									// generic Area+PL command
		{
			*iWhere = atoi(where.c_str());
			*devname += " " + where;
			if (wlen == 4)	// device with 4 chars command
				*iWhere += OPENWEBNET_4CHARS;
			else if ((wlen == 2) && (*iWhere < 10))
				*iWhere += OPENWEBNET_2CHARS;
		}

		/*
			General o area command:

			iWhere = -1, iArea = -1		-> General command
			iWhere = -1, iArea = 0-10	-> Area command
			iWhere = xxxx iArea = -1	-> A-PL command
		*/
		if (*iWhere < 0)
		{
			if (iArea < 0)	// General command
			{
				*devname += " GEN " + where;
				*iWhere = 0;
			}
			else			// Area command
			{
				mask_request_status |= (0x1 << (iArea + 1)); // Gen or area, need a refresh devices status
				*devname += " AREA " + where;
				*iWhere = OPENWEBNET_AREA_ID + iArea;
			}
		}
	}
	else
	{
		// others who..
		*iWhere = atoi(where.c_str());
		*devname += " " + where;
	}
}

/**
	Insert/Update device
**/
void COpenWebNetTCP::UpdateDeviceValue(std::vector<bt_openwebnet>::iterator iter)
{
	std::string who = iter->Extract_who();
	std::string where = iter->Extract_where();
	std::string what = iter->Extract_what();
	std::vector<std::string> whereParam = iter->Extract_whereParameters();
	std::vector<std::string> whatParam = iter->Extract_whatParameters();
	std::string dimension = iter->Extract_dimension();
	std::string value = iter->Extract_value(0);
	std::vector<std::string> valueParam = iter->Extract_values();
	std::string sInterface = iter->Extract_interface();
	std::string devname, sCommand;
	int iAppValue, iWhere, iLevel;

	switch (atoi(who.c_str())) {
	case WHO_LIGHTING:	// 1

		if (!iter->IsNormalFrame())
		{
			if (iter->IsMeasureFrame())
			{
				switch (atoi(value.c_str()))
				{
				case LIGHTING_DIMENSION_SET_UP_LEVEL_WITH_GIVEN_SPEED:				// 1
				case LIGHTING_DIMENSION_TEMPORISATION:								// 2
				case LIGHTING_DIMENSION_REQUIRED_ONLY_ON_LIGHT:						// 3
				case LIGHTING_DIMENSION_STATUS_DIMMER_100_LEVELS_WITH_GIVEN_SPEED:	// 4
				case LIGHTING_DIMENSION_WORKING_TIME_LAMP:							// 8
				case LIGHTING_DIMENSION_MAX_WORKING_TIME_LAMP:						// 9
					// TODO: manage lighting parameter..
					return;
				default:
					Log(LOG_ERROR, "Who=%s measure error -> param=%u", who.c_str(), atoi(value.c_str()));
					return;
				}
			}
			else
			{
				Log(LOG_ERROR, "Who=%s error frame! -> frame_type=%d", who.c_str(), iter->m_frameType);
				return;
			}
		}
		else
		{
			// NORMALE FRAME..
			iAppValue = atoi(what.c_str());
			if (iAppValue == 1000) // What = 1000 (Command translation)
				iAppValue = atoi(whatParam[0].c_str());
		}		

		devname = OPENWEBNET_LIGHT;
		decodeWhereAndFill(WHO_LIGHTING, where, whereParam, &devname, &iWhere);
		UpdateSwitch(WHO_LIGHTING, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str());
		break;
	case WHO_AUTOMATION:								// 2
		if (!iter->IsNormalFrame() && !iter->IsMeasureFrame())
		{
			Log(LOG_ERROR, "Who=%s frame error!", who.c_str());
			return;
		}
		if (iter->IsMeasureFrame()) // Advanced motor actuator (percentual) *#2*19*10*10*65*000*0##
		{
			std::string level = iter->Extract_value(1);
			iLevel = atoi(level.c_str());
			iAppValue = atoi(value.c_str());

			if (iAppValue == 1000) // What = 1000 (Command translation)
				iAppValue = atoi(whatParam[0].c_str());

			devname = OPENWEBNET_AUTOMATION;
			decodeWhereAndFill(WHO_AUTOMATION, where, whereParam, &devname, &iWhere);
			UpdateBlinds(WHO_AUTOMATION, iWhere, iAppValue, atoi(sInterface.c_str()), iLevel, 255, devname.c_str());
		}
		if (iter->IsNormalFrame())
		{
			iAppValue = atoi(what.c_str());
			if (iAppValue == 1000) // What = 1000 (Command translation)
				iAppValue = atoi(whatParam[0].c_str());

			decodeWhereAndFill(WHO_AUTOMATION, where, whereParam, &devname, &iWhere);
			UpdateBlinds(WHO_AUTOMATION, iWhere, iAppValue, atoi(sInterface.c_str()), -1, 255, devname.c_str());
		}
		break;
	case WHO_TEMPERATURE_CONTROL:
		if (iter->IsMeasureFrame())
		{

			// 4: this is a openwebnet termoregulation update/poll messagge, setup devname
			devname = OPENWEBNET_TEMPERATURE;
			devname += where;
			switch (atoi(dimension.c_str()))
			{
				case TEMPERATURE_CONTROL_DIMENSION_TEMPERATURE:				// 0
					UpdateTemp(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), static_cast<float>(atof(value.c_str()) / 10.), atoi(sInterface.c_str()), 255, devname.c_str());
					break;
				case TEMPERATURE_CONTROL_DIMENSION_VALVES_STATUS:				// 19
					devname += " Valves";
					UpdateSwitch(WHO_TEMPERATURE_CONTROL + 600, atoi(where.c_str()), atoi(value.c_str()), atoi(sInterface.c_str()), 255, devname.c_str());
					break;
				case TEMPERATURE_CONTROL_DIMENSION_ACTUATOR_STATUS:				// 20
					devname += " Actuator";
					UpdateSwitch(WHO_TEMPERATURE_CONTROL + 500, atoi(where.c_str()), atoi(value.c_str()), atoi(sInterface.c_str()), 255, devname.c_str());
					break;
				case TEMPERATURE_CONTROL_DIMENSION_COMPLETE_PROBE_STATUS:		// 12
					devname += " Setpoint";
					UpdateSetPoint(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), static_cast<float>(atof(value.c_str()) / 10.), atoi(sInterface.c_str()), devname.c_str());
					break;
				case TEMPERATURE_CONTROL_DIMENSION_LOCAL_SET_OFFSET:			// 13
				{
					devname = OPENWEBNET_TEMPERATURE_STATUS;
					devname += where;
					//std::string sStatus = "";
					std::stringstream sStatus;
					switch (atoi(value.c_str()))
					{
						case TEMPERATURE_SELECTOR_0_NORMAL: // 0
							sStatus << "No offset";
							break;
						case TEMPERATURE_SELECTOR_01:	    // 1
						case TEMPERATURE_SELECTOR_02:	    // 2
						case TEMPERATURE_SELECTOR_03:	    // 3
							sStatus << "Offset +";
							sStatus << atoi(value.c_str());
							break;
						case TEMPERATURE_SELECTOR_11: // 11 -> -1
						case TEMPERATURE_SELECTOR_12: // 12 -> -2
						case TEMPERATURE_SELECTOR_13: // 13 -> -3
							sStatus << "Offset -";
							sStatus << (atoi(value.c_str()) - 10);
							break;
						case TEMPERATURE_SELECTOR_4: // 4
							sStatus << "Local OFF";
							break;
						case TEMPERATURE_SELECTOR_5: // 5
							sStatus << "Local protection";
							break;
						default:
							break;
					}

					if (sStatus.str() != "")
						UpdateTempProbe(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), -1, 1, sStatus.str(), atoi(sInterface.c_str()), 255, devname.c_str());
				}
				break;
				case TEMPERATURE_CONTROL_DIMENSION_SET_POINT_TEMPERATURE:		// 14
					Log(LOG_STATUS, "Setpoint without offset for Zone%s is %s", where.c_str(), value.c_str());
					break;
				case TEMPERATURE_CONTROL_DIMENSION_FAN_COIL_SPEED:				// 11				
				case TEMPERATURE_CONTROL_DIMENSION_SPLIT_CONTROL:				// 22
				case TEMPERATURE_CONTROL_DIMENSION_END_DATE_HOLIDAY_SCENARIO:	// 30
				default:
					Log(LOG_STATUS, "who=%s, where=%s, dimension=%s not yet supported", who.c_str(), where.c_str(), dimension.c_str());
					break;
			}
		}
		else if (iter->IsNormalFrame())
		{
			devname = OPENWEBNET_TEMPERATURE_STATUS;			
			devname += " " + where;
			std::string sStatus = "";
			switch (atoi(what.c_str()))
			{
				case TEMPERATURE_WHAT_MODE_CONDITIONING:		// 0
					sStatus = "Conditioning";
					break;
				case TEMPERATURE_WHAT_MODE_HEATING:				// 1
					sStatus = "Heating";
					break;
				case TEMPERATURE_WHAT_MODE_ANTIFREEZE:			// 102
					sStatus = "Antifreeze"; 
					break;
				case TEMPERATURE_WHAT_MODE_THERMAL_PROTECTION:	// 202
					sStatus = "Thermal protection";
					break;
				case TEMPERATURE_WHAT_MODE_GENERIC_OFF:			// 303
					sStatus = "Off";
					break;
				default:
					Log(LOG_STATUS, "who=%s, what:%s, where=%s not yet supported", who.c_str(), what.c_str(), where.c_str());
					break;
			}

			if (sStatus != "")
			{
				//Log(LOG_STATUS, "Temperature Zone%s in %s", where.c_str(), sStatus.c_str());
				UpdateTempProbe(WHO_TEMPERATURE_CONTROL, atoi(where.c_str()), -1, 0, sStatus, atoi(sInterface.c_str()), 255, devname.c_str());
			}
		}
		else
			Log(LOG_ERROR, "Who=%s frame error!", who.c_str());

		break;

	case WHO_BURGLAR_ALARM:                         // 5
		if (!iter->IsNormalFrame())
		{
			Log(LOG_ERROR, "Who=%s not normal frame! -> frame_type=%d", who.c_str(), iter->m_frameType);
			return;
		}
		
		switch (atoi(what.c_str())) {
		case 0:         //maintenace
			//Log(LOG_STATUS, "Alarm in Maintenance");
			iWhere = ID_DEV_BURGLAR_SYS_STATUS; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
			sCommand = "Maintenance";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 2, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;
		case 1:         //active
			//Log(LOG_STATUS, "Alarm Active");
			iWhere = ID_DEV_BURGLAR_SYS_STATUS; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
			sCommand = "Active";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;
		case 2:         //disabled
			//Log(LOG_STATUS, "Alarm Inactive");
			iWhere = ID_DEV_BURGLAR_SYS_STATUS; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_SYS_STATUS;
			sCommand = "Inactive";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 3, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 4:         //battery fault
			//Log(LOG_STATUS, "Alarm Battery Fault");
			iWhere = ID_DEV_BURGLAR_BATTERY; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
			sCommand = "Battery Fault";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 5:         //battery ok
			//Log(LOG_STATUS, "Alarm Battery OK");
			iWhere = ID_DEV_BURGLAR_BATTERY; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
			sCommand = "Battery Ok";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 6:			//no network
			//Log(LOG_STATUS, "Alarm no network");
			iWhere = ID_DEV_BURGLAR_NETWORK; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_NETWORK;
			sCommand = "No network";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 7:			//network ok
			//Log(LOG_STATUS, "Alarm network ok");
			iWhere = ID_DEV_BURGLAR_NETWORK; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_NETWORK;
			sCommand = "Network OK";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 8: 	//engaged
			//Log(LOG_STATUS, "Alarm Engaged");
			iWhere = ID_DEV_BURGLAR_SYS_ENGAGEMENT; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT;
			sCommand = "Engaged";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 9:         //disengaged
			//Log(LOG_STATUS, "Alarm Disengaged");
			iWhere = ID_DEV_BURGLAR_SYS_ENGAGEMENT; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_SYS_ENGAGEMENT;
			sCommand = "DisEngaged";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 0, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 10:         //battery Unloads
			//Log(LOG_STATUS, "Alarm Battery Unloads");
			iWhere = ID_DEV_BURGLAR_BATTERY; // force where because not exist
			devname = OPENWEBNET_BURGLAR_ALARM_BATTERY;
			sCommand = "Battery Unloads";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 11:         // zone N Active
			iWhere = atoi(whereParam[0].c_str());
			//Log(LOG_STATUS, "Alarm Zone %d Active",iWhere);
			devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
			devname += " " + whereParam[0];
			sCommand = "Active";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 1, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 15:         //zone N INTRUSION ALARM
			iWhere = atoi(whereParam[0].c_str());
			//Log(LOG_STATUS, "Alarm Zone %d INTRUSION ALARM", iWhere);
			devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
			devname += " " + whereParam[0];
			sCommand = "Intrusion";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 4, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 18:         // zone N Not Active
			iWhere = atoi(whereParam[0].c_str());
			//Log(LOG_STATUS, "Alarm Zone %d Not active", iWhere);
			devname = OPENWEBNET_BURGLAR_ALARM_SENSOR;
			devname += " " + whereParam[0];
			sCommand = "Inactive";
			UpdateAlarm(WHO_BURGLAR_ALARM, iWhere, 0, sCommand.c_str(), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		default:
			Log(LOG_STATUS, "who=%s, where=%s, dimension=%s not yet supported", who.c_str(), where.c_str(), dimension.c_str());
			break;
		}
		break;
	case WHO_AUXILIARY:                             // 9
		/**
			example:

			*9*what*where##

			what:   0 = OFF
					1 = ON
			where:  1 to 9 (AUX channel)
		**/
		if (!iter->IsNormalFrame())
		{
			Log(LOG_ERROR, "Who=%s frame error!", who.c_str());
			return;
		}

		devname = OPENWEBNET_AUXILIARY;
		devname += " " + where;

		UpdateSwitch(WHO_AUXILIARY, atoi(where.c_str()), atoi(what.c_str()), atoi(sInterface.c_str()), 255, devname.c_str());
		break;
	case WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION:              // 25
		if (!iter->IsNormalFrame())
		{
			Log(LOG_ERROR, "Who=%s not normal frame! -> frame_type=%d", who.c_str(), iter->m_frameType);
			return;
		}


		switch (atoi(what.c_str())) { //CEN PLUS / DRY CONTACT / IR DETECTION

		case 21:         //Short pressure
			iWhere = atoi(where.c_str());
			iAppValue = atoi(whatParam[0].c_str());
			Log(LOG_STATUS, "CEN PLUS Short pressure %d Button %d", iWhere, iAppValue);
			devname = OPENWEBNET_CENPLUS;
			devname += " " + where + " Short Press Button " + whatParam[0];
			UpdateCenPlus(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, 1, iAppValue, atoi(what.c_str()), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 22:         //Start of extended pressure
			Log(LOG_STATUS, "CEN Start of extended pressure");
			break;

		case 23:         //Extended pressure
			Log(LOG_STATUS, "CEN Extended pressure");
			break;

		case 24:         //End of Extended pressure
			iWhere = atoi(where.c_str());
			iAppValue = atoi(whatParam[0].c_str());

			Log(LOG_STATUS, "CEN PLUS Long pressure %d Button %d", iWhere, iAppValue);
			devname = OPENWEBNET_CENPLUS;
			devname += " " + where + " Long Press Button " + whatParam[0];
			UpdateCenPlus(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, 1, iAppValue, atoi(what.c_str()), atoi(sInterface.c_str()), 255, devname.c_str());
			break;

		case 31:
		case 32:
			if (where.substr(0, 1) != "3")
			{
				Log(LOG_ERROR, "Where=%s is not correct for who=%s", where.c_str(), who.c_str());
				return;
			}

			devname = OPENWEBNET_DRY_CONTACT;
			devname += " " + where.substr(1);
			iWhere = atoi(where.substr(1).c_str());

			iAppValue = atoi(what.c_str());
			if (iAppValue == DRY_CONTACT_IR_DETECTION_WHAT_ON)
				iAppValue = 1;
			else
				iAppValue = 0;

			UpdateSwitch(WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str());
			break;
		default:
			Log(LOG_ERROR, "What=%s is not correct for who=%s", what.c_str(), who.c_str());
			return;
		}
		break;
	case WHO_ENERGY_MANAGEMENT:                     // 18
		if (!iter->IsMeasureFrame())
		{
			if (iter->IsNormalFrame())
				Log(LOG_STATUS, "who=%s, what:%s, where=%s not yet supported", who.c_str(), what.c_str(), where.c_str());
			else
				Log(LOG_ERROR, "Who=%s frame error!", who.c_str());
			return;
		}
		devname = OPENWEBNET_ENERGY_MANAGEMENT;
		devname += " " + where;
		switch (atoi(dimension.c_str()))
		{
		case ENERGY_MANAGEMENT_DIMENSION_ACTIVE_POWER:
			UpdatePower(WHO_ENERGY_MANAGEMENT, atoi(where.c_str()), static_cast<float>(atof(value.c_str())), atoi(sInterface.c_str()), 255, devname.c_str());
			break;
		case ENERGY_MANAGEMENT_DIMENSION_ENERGY_TOTALIZER:
			UpdateEnergy(WHO_ENERGY_MANAGEMENT, atoi(where.c_str()), static_cast<float>(atof(value.c_str()) / 1000.), atoi(sInterface.c_str()), 255, devname.c_str());
			break;
		case ENERGY_MANAGEMENT_DIMENSION_END_AUTOMATIC_UPDATE:			
			if (atoi(value.c_str()))
				Log(LOG_STATUS, "Start sending instantaneous consumption %s for %s minutes", where.c_str(), value.c_str());
			else
				Log(LOG_STATUS, "Stop sending instantaneous consumption %s", where.c_str());
			break;
		default:
			Log(LOG_STATUS, "who=%s, where=%s, dimension=%s not yet supported", who.c_str(), where.c_str(), dimension.c_str());
			break;
		}
		break;

	case WHO_GATEWAY_INTERFACES_MANAGEMENT:         // 13
		int timezone, model;
		switch (atoi(dimension.c_str()))
		{
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_TIME:					// 0,		/* Read/Write */
			if (valueParam.size() >= 4)
			{
				timezone = atoi(valueParam[3].c_str());
				Log(LOG_STATUS, "Gateway Time %s:%s:%s GMT%c%u", valueParam[0].c_str(), valueParam[1].c_str(), valueParam[2].c_str(), 
																					  (timezone > 99) ? '-' : '+', (timezone % 100));
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DATE:					// 1,		/* Read/Write */
			if (valueParam.size() >= 4)
			{
				Log(LOG_STATUS, "Gateway Date %s %s/%s/%s", valueParam[0].c_str(), valueParam[1].c_str(), 
																				 valueParam[2].c_str(), valueParam[3].c_str());
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_IP_ADDRESS:			// 10,		/* Read		  */
			if (valueParam.size() >= 4)
			{
				Log(LOG_STATUS, "Gateway IP Address %u.%u.%u.%u", atoi(valueParam[0].c_str()), atoi(valueParam[1].c_str()),
																					   atoi(valueParam[2].c_str()), atoi(valueParam[3].c_str()));
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_NET_MASK:				// 11,		/* Read		  */
			if (valueParam.size() >= 4)
			{
				Log(LOG_STATUS, "Gateway Net Mask %u.%u.%u.%u", atoi(valueParam[0].c_str()), atoi(valueParam[1].c_str()),
																					 atoi(valueParam[2].c_str()), atoi(valueParam[3].c_str()));
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_MAC_ADDRESS:			// 12,		/* Read		  */
			if (valueParam.size() >= 6)
			{
				Log(LOG_STATUS, "Gateway MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
														atoi(valueParam[0].c_str()), atoi(valueParam[1].c_str()), atoi(valueParam[2].c_str()),
														atoi(valueParam[3].c_str()), atoi(valueParam[4].c_str()), atoi(valueParam[5].c_str()));
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DEVICE_TYPE:			// 15,		/* Read		  */
			if (!valueParam.empty())
			{
				model = atoi(valueParam[0].c_str());
				switch (model)
				{
				case GATEWAY_MODEL_MHSERVER:	// 2
					Log(LOG_STATUS, "COpenWebNetTCP:Model #%u MHServer", model);
					break;
				case GATEWAY_MODEL_MH200:		// 4
					Log(LOG_STATUS, "Model #%u MH200", model);
					break;
				case GATEWAY_MODEL_F452:		// 6
					Log(LOG_STATUS, "Model #%u F452", model);
					break;
				case GATEWAY_MODEL_F452V:		// 7
					Log(LOG_STATUS, "Model #%u F452V", model);
					break;
				case GATEWAY_MODEL_MHSERVER2:	// 11
					Log(LOG_STATUS, "Model #%u MHServer2", model);
					break;
				case GATEWAY_MODEL_H4684:		// 13
					Log(LOG_STATUS, "Model #%u H4684", model);
					break;
				default:
					Log(LOG_STATUS, "Model #%u", model);
					break;
				}
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_FIRMWARE_VERSION:		// 16,		/* Read		  */
			if (valueParam.size() >= 3)
			{
				Log(LOG_STATUS, "Gateway Firmware Version %s.%s.%s",
														valueParam[0].c_str(), valueParam[1].c_str(), valueParam[2].c_str());
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_UPTIME:				// 19,		/* Read		  */
			if (valueParam.size() >= 4)
			{
				Log(LOG_STATUS, "Gateway UpTime %sdays %sh %sm %ss",
														valueParam[0].c_str(), valueParam[1].c_str(), valueParam[2].c_str(), valueParam[3].c_str());
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DATE_AND_TIME:			// 22,		/* Read/Write */
			if (valueParam.size() >= 8)
			{
				time_t now, rcv_tm;
				int delta;
				struct tm ltime, lnowtime;

				timezone = atoi(valueParam[3].c_str());
				timezone = ((timezone > 99) ? -(timezone % 100) : (timezone % 100)); // 

				memset(&ltime, 0, sizeof(ltime));
				ltime.tm_hour = atoi(valueParam[0].c_str());  // hours since midnight - [0, 23]
				ltime.tm_min = atoi(valueParam[1].c_str());   // minutes after the hour - [0, 59]
				ltime.tm_sec = atoi(valueParam[2].c_str());   // seconds after the minute - [0, 60] including leap second
				ltime.tm_wday = atoi(valueParam[4].c_str());  // days since Sunday - [0, 6]
				ltime.tm_mday = atoi(valueParam[5].c_str());  // day of the month - [1, 31]
				ltime.tm_mon = atoi(valueParam[6].c_str());   // months since January - [0, 11]
				ltime.tm_year = atoi(valueParam[7].c_str());  // years since 1900	

				Log(LOG_STATUS, "Gateway DateTime %02u/%02u/%04u - %02u:%02u:%02u GMT%c%u",
														ltime.tm_mday, ltime.tm_mon, ltime.tm_year,
														ltime.tm_hour, ltime.tm_min, ltime.tm_sec, (timezone > 0) ? '+' : ' ', timezone);

				if (m_ownSynch)
				{
					now = mytime(nullptr);
					localtime_r(&now, &lnowtime);

					ltime.tm_mon -= 1;		// months since January - [0, 11]
					ltime.tm_year -= 1900;  // years since 1900	
					ltime.tm_isdst = lnowtime.tm_isdst;
					rcv_tm = mktime(&ltime);

					delta = static_cast<int>(now - rcv_tm);

					if ((delta < -60) || (delta > 60))	// delta +-1min
					{
						setDateTime(valueParam[3]);	// SET DATE TIME						
						Log(LOG_STATUS, "Date/Time Change - now:%ld, rcv:%ld, delta: %d", now, rcv_tm, delta);

						/************* notification ************/
						std::stringstream body;
						char frame_dt[50];
						strftime(frame_dt, sizeof(frame_dt) - 1, "%d/%m/%Y %H:%M:%S", &ltime); //set date time 
						body << frame_dt;
						body << " - Delta:";
						body << delta;
						m_notifications.SendMessageEx(0, std::string(""), NOTIFYALL, std::string(""), std::string("OWN Date/Time Change"), body.str(), std::string(""), 0, std::string(""), true);
						/************* notification ************/						
					}
					else
					{
						Log(LOG_STATUS, "Date/Time is OK - now:%ld, rcv:%ld, delta: %d", now, rcv_tm, delta);
					}
				}
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_KERNEL_VERSION:		// 23,		/* Read		  */
			if (valueParam.size() >= 3)
			{
				Log(LOG_STATUS, "Gateway Kernel Version %s.%s.%s",
											valueParam[0].c_str(), valueParam[1].c_str(), valueParam[2].c_str());
			}
			break;
		case GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DISTRIBUTION_VERSION:	// 24		/* Read		  */
			if (valueParam.size() >= 3)
			{
				Log(LOG_STATUS, "Gateway Distribution Version %s.%s.%s",
											valueParam[0].c_str(), valueParam[1].c_str(), valueParam[2].c_str());
			}
			break;
		default:
			Log(LOG_STATUS, "who=%s, dimension=%s", who.c_str(), dimension.c_str());
			break;
		}
		break;

	case WHO_SOUND_DIFFUSION:						// 22
		//iAppValue = atoi(what.c_str());
		//iWhere = atoi(where.c_str());
		//devname = OPENWEBNET_SOUND_DIFFUSION;
		//devname += " " + where;
		//UpdateSoundDiffusion(WHO_SOUND_DIFFUSION, iWhere, iAppValue, atoi(sInterface.c_str()), 255, devname.c_str());
		//break;

	case WHO_SCENARIO:                              // 0
	case WHO_LOAD_CONTROL:                          // 3
	case WHO_DOOR_ENTRY_SYSTEM:                     // 6
	case WHO_MULTIMEDIA:                            // 7
	case WHO_LIGHT_SHUTTER_ACTUATOR_LOCK:           // 14
	case WHO_SCENARIO_SCHEDULER_SWITCH:             // 15
	case WHO_AUDIO:                                 // 16
	case WHO_SCENARIO_PROGRAMMING:                  // 17
	case WHO_LIHGTING_MANAGEMENT:                   // 24
	case WHO_ZIGBEE_DIAGNOSTIC:                     // 1000
	case WHO_AUTOMATIC_DIAGNOSTIC:                  // 1001
	case WHO_THERMOREGULATION_DIAGNOSTIC_FAILURES:  // 1004
	case WHO_DEVICE_DIAGNOSTIC:                     // 1013
	case WHO_ENERGY_MANAGEMENT_DIAGNOSTIC:			// 1018
		Log(LOG_ERROR, "Who=%s not yet supported!", who.c_str());
		return;
	default:
		Log(LOG_ERROR, "ERROR Who=%s not exist!", who.c_str());
		return;
	}
}

/**
   Convert device id into correct where
**/
std::string COpenWebNetTCP::getWhereForWrite(int where)
{
	std::stringstream whereStr;
	uint32_t iArea;

	switch (where & OPENWEBNET_MASK_WHERE_FUNCT)
	{
	case OPENWEBNET_GROUP_ID:
		whereStr << "#";
		whereStr << (where - OPENWEBNET_GROUP_ID);
		break;
	case OPENWEBNET_AREA_ID:
		iArea = where - OPENWEBNET_AREA_ID;
		if (iArea == 0)
			whereStr << "00";	// Area 0
		else if (iArea == 10)
			whereStr << "100";	// Area 10
		else
			whereStr << iArea;	// Area 1-9
		break;
	case OPENWEBNET_2CHARS:
		where -= OPENWEBNET_2CHARS;
		if (where < 10)  whereStr << "0";
		whereStr << where;
		break;
	case OPENWEBNET_4CHARS:
		where -= OPENWEBNET_4CHARS;
		if (where < 100) whereStr << "00";						// APL Command : A[00]; PL[01 - 15]
		if ((where > 99) && (where < 1000)) whereStr << "0";	// APL Command : A[01 - 09]; PL[10 - 15]
		whereStr << where;
		break;
	default:
		whereStr << where;
		break;
	}
			
	return whereStr.str();
}

/**
   Convert domoticz command in a OpenWebNet command, then send it to device
**/
bool COpenWebNetTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pCmd = (_tGeneralSwitch*)pdata;
	//const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	
	unsigned char packetlength = pCmd->len;
	unsigned char packettype = pCmd->type;
	unsigned char subtype = pCmd->subtype;

	int who = 0;
	int what = 0;
	int where = 0;
	int iInterface = 0;
	int Level = -1;
	std::vector<std::vector<std::string> > result;


	char szIdx[10];

	// Test packet type
	switch (packettype) {
	case pTypeGeneralSwitch:
		// Test general switch subtype
		iInterface = pCmd->unitcode;

		who = (pCmd->id >> 16) & 0xFFFF;
		where = pCmd->id & 0xFFFF;

		switch (who) {
		case WHO_AUTOMATION:
			//Blinds/Window command
			sprintf(szIdx, "%08X", ((who << 16) & 0xffff0000) | (where & 0x0000ffff));
			result = m_sql.safe_query(
				"SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (SwitchType==%d)",
				m_HwdID, szIdx, STYPE_BlindsPercentage);

			if (result.empty())// from a normal button
			{
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
			}
			else // advanced button
			{
				if (pCmd->cmnd == gswitch_sOn)
				{
					what = AUTOMATION_WHAT_UP;
				}
				else if (pCmd->cmnd == gswitch_sOff)
				{
					what = AUTOMATION_WHAT_DOWN;
				}
				else if (pCmd->cmnd == gswitch_sSetLevel)
				{
					// setting level of Blinds
					Level = pCmd->level;
				}
				else if (pCmd->cmnd == gswitch_sStop)
				{
					what = AUTOMATION_WHAT_STOP;
				}
			}

			break;
		case WHO_LIGHTING:
			//Light/Switch command
			if (pCmd->cmnd == gswitch_sOff)
			{
				what = LIGHTING_WHAT_OFF;
			}
			else if (pCmd->cmnd == gswitch_sOn)
			{
				what = LIGHTING_WHAT_ON;
			}
			else if (pCmd->cmnd == gswitch_sSetLevel)
			{
				// setting level of dimmer
				if (pCmd->level != 0)
				{
					BYTE level = pCmd->level;
					if (level < 20) level = 20; // minimum value after 0
					what = (level + 5) / 10;
				}
				else
				{
					what = LIGHTING_WHAT_OFF;
				}
			}
			break;
		case WHO_AUXILIARY:
			//Auxiliary command
			if (pCmd->cmnd == gswitch_sOff)
			{
				what = AUXILIARY_WHAT_OFF;
			}
			else if (pCmd->cmnd == gswitch_sOn)
			{
				what = AUXILIARY_WHAT_ON;
			}
			break;
		
		case 0xF00: // Custom command, fake who..
			sprintf(szIdx, "%08X", ((who << 16) & 0xffff0000) | (where & 0x0000ffff));
			result = m_sql.safe_query("SELECT StrParam1 FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s')", m_HwdID, szIdx);
			if (!result.empty())
			{
				// Custom command fount, send it!
				Log(LOG_STATUS, "send custom command: '%s'", result[0][0].c_str());
				std::vector<bt_openwebnet> responses;
				bt_openwebnet request(result[0][0]);
				if (sendCommand(request, responses))
				{
					//if (responses.size() > 0) return responses.at(0).IsOKFrame();
					return true; // if send ok, return always ok without check the response..
				}
			}
			Log(LOG_ERROR, "custom command error: '%s'", result[0][0].c_str());
			return false; // error
		default:
			break;
		}
		break;
	default:
		Log(LOG_STATUS, "COpenWebNetTCP unknown command: packettype=%d subtype=%d", packettype, subtype);
		return false;
	}


	if (iInterface == 0) {
		//local bus
		if (Level >= 0) { //Advanced Blind
			std::vector<bt_openwebnet> responses;
			bt_openwebnet request;
			std::stringstream dimensionStr;
			dimensionStr << AUTOMATION_DIMENSION_GOTO_LEVEL;
			std::string sDimension = dimensionStr.str();

			std::stringstream whoStr;
			whoStr << who;
			std::string sWho = whoStr.str();

			std::string sWhere = getWhereForWrite(where);

			std::stringstream levelStr;
			levelStr << Level;

			std::vector<std::string> value;
			value.clear();
			value.push_back("001");
			value.push_back(levelStr.str());
			request.CreateWrDimensionMsgOpen2(sWho, sWhere, sDimension, value);
			if (sendCommand(request, responses))
			{
				if (!responses.empty())
					return responses.at(0).IsOKFrame();
			}
		}
		else {
			std::vector<bt_openwebnet> responses;
			
			std::stringstream whoStr;
			whoStr << who;
			std::string sWho = whoStr.str();

			std::stringstream whatStr;
			whatStr << what;
			std::string sWhat = whatStr.str();
			
			std::string sWhere = getWhereForWrite(where);

			std::string when;
			bt_openwebnet request(sWho, sWhat, sWhere, when);
			if (sendCommand(request, responses))
			{
				if (!responses.empty())
					return responses.at(0).IsOKFrame();
			}
		}
	}
	else
	{
		std::vector<bt_openwebnet> responses;
		bt_openwebnet request;

		std::stringstream whoStr;
		whoStr << who;
		std::string sWho = whoStr.str();

		std::stringstream whatStr;
		whatStr << what;
		std::string sWhat = whatStr.str();

		std::string sWhere = getWhereForWrite(where);

		std::stringstream interfaceStr;
		interfaceStr << iInterface;
		std::string sInterface = interfaceStr.str();

		std::string lev;
		std::string when;
		request.CreateMsgOpen(sWho, sWhat, sWhere, lev, sInterface, when);
		if (sendCommand(request, responses))
		{
			if (!responses.empty())
				return responses.at(0).IsOKFrame();
		}
	}

	return true;
}


bool COpenWebNetTCP::SetSetpoint(const int idx, const float temp)
{
	//int who = (idx >> 16) & 0xff;  fixed to 'WHO_TEMPERATURE_CONTROL'
	int iInterface = (idx >> 8) & 0xff;
	int where = idx & 0xff;
	int _temp = (int)(temp * 10);

	std::vector<bt_openwebnet> responses;
	bt_openwebnet request;

	std::stringstream whoStr;
	whoStr << WHO_TEMPERATURE_CONTROL;
	std::string sWho = whoStr.str();

	// add # to set value permanent
	std::string sWhere = "#" + getWhereForWrite(where);	

	std::stringstream dimensionStr;
	dimensionStr << TEMPERATURE_CONTROL_DIMENSION_SET_POINT_TEMPERATURE;
	std::string sDimension = dimensionStr.str();

	std::stringstream valueStr;

	valueStr << 0;
	valueStr << _temp;
	std::vector<std::string> sValue;
	sValue.push_back(valueStr.str());
	sValue.push_back("3");					//send generic mode. We don't need to know in witch state the bt3550 or BTI-L4695 is (cooling or heating).

	if (iInterface)
	{
		// if Bus Id interface is present...
		std::stringstream interfaceStr;
		interfaceStr << iInterface;
		std::string sInterface = interfaceStr.str();

		std::string sLev;
		request.CreateWrDimensionMsgOpen(sWho, sWhere, sLev, sInterface, sDimension, sValue);
		if (sendCommand(request, responses))
		{
			if (!responses.empty())
				return responses.at(0).IsOKFrame();
		}
	}
	else
	{
		// Local Bus..
		request.CreateWrDimensionMsgOpen(sWho, sWhere, sDimension, sValue); // (const std::string& who, const std::string& where, const std::string& dimension, const std::vector<std::string>& value)
		if (sendCommand(request, responses, 1, false))
		{
			if (!responses.empty())
			{
				return responses.at(0).IsOKFrame();
			}
		}
	}

	return false;
}



/**
   Send OpenWebNet command to device
**/
bool COpenWebNetTCP::sendCommand(bt_openwebnet& command, std::vector<bt_openwebnet>& response, int waitForResponse, bool silent)
{
	csocket *commandSocket;
	bool ret;
	if (!(commandSocket = connectGwOwn(OPENWEBNET_COMMAND_SESSION)))
	{
		Log(LOG_ERROR, "Command session ERROR");
		return false;
	}
	// Command session correctly open
	//Log(LOG_STATUS, "Command session connected to: %s:%d", m_szIPAddress.c_str(), m_usIPPort);

	// Command session correctly open -> write command
	if (!silent) {
		Log(LOG_STATUS, "Command %s", command.m_frameOpen.c_str());
	}
	ownWrite(commandSocket, command.m_frameOpen.c_str(), command.m_frameOpen.length());

	if (waitForResponse > 0) {
		sleep_seconds(waitForResponse);

		char responseBuffer[OPENWEBNET_BUFFER_SIZE];
		int read = ownRead(commandSocket, responseBuffer, OPENWEBNET_BUFFER_SIZE);
		if (!silent) {
			Log(LOG_STATUS, "sent=%s received=%s", command.m_frameOpen.c_str(), responseBuffer);
		}

		std::lock_guard<std::mutex> l(readQueueMutex);
		ret = ParseData(responseBuffer, read, response);
	}
	else
		ret = true;

	if (commandSocket->getState() != csocket::CLOSED)
		commandSocket->close();
	return (ret);
}

/**
	automatic scan of automation/lighting device
**/
void COpenWebNetTCP::scan_automation_lighting(const int cen_area)
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream whereStr;
	whoStr << WHO_LIGHTING;
	if (cen_area < 0)
		whereStr << "0";		// general scan
	else if (cen_area == 0)
		whereStr << "00";		// area 0 scan
	else
		whereStr << cen_area;	// area 1-9 scan
	request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
	sendCommand(request, responses, 0, false);
}

/**
	automatic scan of sound diffusion device
**/
void COpenWebNetTCP::scan_sound_diffusion()
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream whereStr;
	whoStr << WHO_SOUND_DIFFUSION;
	whereStr << 0;
	request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
	sendCommand(request, responses, 0, false);
}

/**
	automatic scan of temperature control device
**/
void COpenWebNetTCP::scan_temperature_control()
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream dimensionStr;
	whoStr << WHO_TEMPERATURE_CONTROL;
	dimensionStr << ""; //0;

	for (int where = 1; where < 10; where++)
	{
		std::stringstream whereStr;
		whereStr << where;
		request.CreateDimensionMsgOpen(whoStr.str(), whereStr.str(), dimensionStr.str());
		sendCommand(request, responses, 0, true);
	}
}

/**
	request general burglar alarm status
**/
void COpenWebNetTCP::requestBurglarAlarmStatus()
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream whereStr;
	whoStr << WHO_BURGLAR_ALARM;
	whereStr << 0;
	request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
	sendCommand(request, responses, 0, false);
}

/**
	request Dry contact/IR Detection alarm status
**/
void COpenWebNetTCP::requestDryContactIRDetectionStatus()
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream whereStr;
	whoStr << WHO_CEN_PLUS_DRY_CONTACT_IR_DETECTION;
	whereStr << 30;
	request.CreateStateMsgOpen(whoStr.str(), whereStr.str());
	sendCommand(request, responses, 0, false);
}

/**
	request energy totalizer
**/
void COpenWebNetTCP::requestEnergyTotalizer()
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr;
	std::stringstream dimensionStr;
	whoStr << WHO_ENERGY_MANAGEMENT;
	dimensionStr << ENERGY_MANAGEMENT_DIMENSION_ENERGY_TOTALIZER;

	/** request for the energy totalizer of the central units **/
	for (int where = WHERE_ENERGY_CU_1; where < MAX_WHERE_ENERGY_CU; where++)
	{
		std::stringstream whereStr;
		whereStr << where;
		request.CreateDimensionMsgOpen(whoStr.str(), whereStr.str(), dimensionStr.str());
		sendCommand(request, responses, 0, true);
	}
	
	/** request for the energy totalizer of the actuators **/
	for (int where = WHERE_ENERGY_A_1; where < MAX_WHERE_ENERGY_A; where++)
	{
		std::stringstream whereStr;
		whereStr << where;
		whereStr << "#0";
		request.CreateDimensionMsgOpen(whoStr.str(), whereStr.str(), dimensionStr.str());
		sendCommand(request, responses, 0, true);
	}
}

/**
	request automatic update power
**/
void COpenWebNetTCP::requestAutomaticUpdatePower(int time)
{
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	std::stringstream whoStr, dimensionStr, appStr;
	std::vector<std::string> value;
	whoStr << WHO_ENERGY_MANAGEMENT;
	dimensionStr << ENERGY_MANAGEMENT_DIMENSION_END_AUTOMATIC_UPDATE;

	if (time > 255) time = 255; // Time in minutes

	value.clear();
	value.push_back("1");
	appStr << time;
	value.push_back(appStr.str());

	/** request for the automatic update power of the central units **/
	for (int where = WHERE_ENERGY_CU_1; where < MAX_WHERE_ENERGY_CU; where++)
	{
		std::stringstream whereStr;
		whereStr << where;
		request.CreateWrDimensionMsgOpen2(whoStr.str(), whereStr.str(), dimensionStr.str(), value);
		sendCommand(request, responses, 0, true);
	}

	/** request for the automatic update power of the actuators **/
	for (int where = WHERE_ENERGY_A_1; where < MAX_WHERE_ENERGY_A; where++)
	{
		std::stringstream whereStr;
		whereStr << where;
		whereStr << "#0";
		request.CreateWrDimensionMsgOpen2(whoStr.str(), whereStr.str(), dimensionStr.str(), value);
		sendCommand(request, responses, 0, true);
	}
}

/**
	Request dimensions to gateway
**/
void COpenWebNetTCP::requestGatewayInfo()
{
	Log(LOG_STATUS, "request gateway information...");
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;

	for (int idim = GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_TIME; idim <= GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DISTRIBUTION_VERSION; idim++)
	{
		std::stringstream dimensionStr;

		switch (idim)
		{
		//case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_TIME:					// 0,			/* Read/Write */
		//case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DATE:					// 1,			/* Read/Write */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_IP_ADDRESS:				// 10,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_NET_MASK:				// 11,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_MAC_ADDRESS:			// 12,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DEVICE_TYPE:			// 15,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_FIRMWARE_VERSION:		// 16,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_UPTIME:					// 19,			/* Read		  */
		//case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DATE_AND_TIME:			// 22,			/* Read/Write */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_KERNEL_VERSION:			// 23,			/* Read		  */
		case	GATEWAY_INTERFACES_MANAGEMENT_DIMENSION_DISTRIBUTION_VERSION:	// 24			/* Read		  */
			dimensionStr << idim;
			request.CreateGatewayReqMsgOpen(dimensionStr.str());
			sendCommand(request, responses, 0, true);
			sleep_milliseconds(200);
			break;
		default:
			break; // skip
		}
	}
}

/**
	Request time to gateway
**/
void COpenWebNetTCP::requestDateTime()
{
	Log(LOG_STATUS, "request DateTime...");
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	request.CreateDateTimeReqMsgOpen();
	sendCommand(request, responses, 0, true);
}

void COpenWebNetTCP::setDateTime(const std::string &tzString)
{
	Log(LOG_STATUS, "set DateTime...");
	bt_openwebnet request;
	std::vector<bt_openwebnet> responses;
	request.CreateSetDateTimeMsgOpen(tzString);
	sendCommand(request, responses, 0, false);
}

/**
	scan devices
**/
void COpenWebNetTCP::scan_device()
{
	int iWhere;

	if (mask_request_status & 0x1)
	{
		
		requestGatewayInfo();
		sleep_seconds(5L);

		Log(LOG_STATUS, "scanning automation/lighting...");
		// Scan of all devices
		scan_automation_lighting(WHERE_CEN_0);

		requestDryContactIRDetectionStatus();

		Log(LOG_STATUS, "request burglar alarm status...");
		requestBurglarAlarmStatus();

		/** Scanning of temperature sensor is not necessary simply wait an update **/
		//Log(LOG_STATUS, "scanning temperature control...");
		scan_temperature_control();

		// Scan of sound diffusion device 
		//scan_sound_diffusion();

		Log(LOG_STATUS, "scan device complete, wait all the update data..");

		/* Update complete scan time*/
		LastScanTime = mytime(nullptr);
	}
	else
	{
		// Scan only the set areas
		for (iWhere = WHERE_AREA_0; iWhere < MAX_WHERE_AREA; iWhere++)
		{
			if (mask_request_status & ((0x1 << (iWhere + 1))))
			{
				Log(LOG_STATUS, "scanning AREA %u...", iWhere);
				scan_automation_lighting(iWhere);
			}
		}
	}
}

bool COpenWebNetTCP::ParseData(char* data, int length, std::vector<bt_openwebnet>& messages)
{
	std::string buffer = std::string(data, length);
	size_t begin = 0;
	size_t end = std::string::npos;
	do {
		end = buffer.find(OPENWEBNET_END_FRAME, begin);
		if (end != std::string::npos) {
			bt_openwebnet message(buffer.substr(begin, end - begin + 2));
			messages.push_back(message);
			begin = end + 2;
		}
	} while (end != std::string::npos);

	return true;
}

void COpenWebNetTCP::Do_Work()
{
	while (!IsStopRequested(OPENWEBNET_HEARTBEAT_DELAY*1000))
	{
		if (isStatusSocketConnected())
		{
			time_t tnow = mytime(nullptr);
			if ((tnow - LastScanTimeEnergy) > SCAN_TIME_REQ_AUTO_UPDATE_POWER)
			{
				requestAutomaticUpdatePower(255); // automatic update for 255 minutes
				LastScanTimeEnergy = tnow;
			}

			if ((tnow - LastScanTimeEnergyTot) > SCAN_TIME_REQ_ENERGY_TOTALIZER)
			{
				requestEnergyTotalizer();
				LastScanTimeEnergyTot = tnow;
			}

			if (mask_request_status)
			{
				scan_device();
				mask_request_status = 0x0; // scan devices complete
			}

			if (m_ownSynch && ((tnow - LastScanSync) >= m_ownSynch))
			{
				/* request date/time to check synchronization */
				requestDateTime();				
				LastScanSync = tnow;
			}
		}

		// every 5 minuts force scan ALL devices for refresh status
		if (m_ownScanTime && ((mytime(nullptr) - LastScanTime) > m_ownScanTime))
		{
			if ((mask_request_status & 0x1) == 0)
				Log(LOG_STATUS, "HEARTBEAT set scan devices ...");
			mask_request_status = 0x1; // force scan devices
		}

		m_LastHeartbeat = mytime(nullptr);
	}
	Log(LOG_STATUS, "Heartbeat worker stopped...");
}
