#include "stdafx.h"
#include "KMTronicUDP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"

#define KMTRONIC_POLL_INTERVAL 10

KMTronicUDP::KMTronicUDP(const int ID, const std::string &IPAddress, const unsigned short usIPPort) :
m_szIPAddress(IPAddress)
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
}

void KMTronicUDP::Init()
{
}

bool KMTronicUDP::StartHardware()
{
	RequestStart();

	Init();
 	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool KMTronicUDP::StopHardware()
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

void KMTronicUDP::Do_Work()
{
	int sec_counter = KMTRONIC_POLL_INTERVAL - 2;

	_log.Log(LOG_STATUS, "KMTronic: UDP Worker started...");

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % KMTRONIC_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "KMTronic: UDP Worker stopped...");
}

bool KMTronicUDP::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if (packettype == pTypeLighting2)
	{
		//light command

	        int udpSocket, n;
	        struct sockaddr_in udpClient;
		char buf[7]="FF0000";

		int Relay = pSen->LIGHTING2.id4;
		if (Relay > Max_KMTronic_Relais)
			return false;

        	struct hostent *he;
		if ((he = gethostbyname(m_szIPAddress.c_str())) == nullptr)
		{ // get the host info
			_log.Log(LOG_ERROR, "KMTronic: Error with IP address!...");
			return false;
		}

		udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

		memset(&udpClient,0,sizeof(udpClient));
	        udpClient.sin_family = AF_INET;
	        udpClient.sin_port = htons(m_usIPPort); // short, network byte order
		udpClient.sin_addr = *((struct in_addr *)he->h_addr);

		/** build the packet **/
		buf[3]=(char)(Relay+'0');

		if (pSen->LIGHTING2.cmnd == light2_sOn)
		{
			buf[5]='1';
		}

	        /** send the packet **/
	        n=sendto(udpSocket, buf, 6, 0, (struct sockaddr*)&udpClient, sizeof(udpClient));
		closesocket(udpSocket);
    		if (n < 0) {
			_log.Log(LOG_ERROR, "KMTronic: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	return false;
}

bool KMTronicUDP::WriteInt(const unsigned char* /*data*/, const size_t /*len*/, const bool /*bWaitForReturn*/)
{
	return true;
}

void KMTronicUDP::GetMeterDetails()
{
	//status command

	int udpSocket, n;
	struct sockaddr_in udpClient;
	char buf[8];
	socklen_t serverlen;

        struct hostent *he;
	if ((he = gethostbyname(m_szIPAddress.c_str())) == nullptr)
	{ // get the host info
		_log.Log(LOG_ERROR, "KMTronic: Error with IP address!...");
		return;
	}

	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&udpClient,0,sizeof(udpClient));
	udpClient.sin_family = AF_INET;
	udpClient.sin_port = htons(m_usIPPort); // short, network byte order
	udpClient.sin_addr = *((struct in_addr *)he->h_addr);

	/** set timeout to 1 second**/
#if !defined WIN32
        struct timeval tv;
        tv.tv_sec = 1;
        setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
#else
        unsigned long nTimeout = 1*1000;
        setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(DWORD));
#endif


	/** send the packet **/
	serverlen = sizeof(udpClient);
	n=sendto(udpSocket, "FF0000", 6, 0, (struct sockaddr*)&udpClient, serverlen);
    	if (n < 0) {
		closesocket(udpSocket);
		_log.Log(LOG_ERROR, "KMTronic: Error sending relay command to: %s", m_szIPAddress.c_str());
		return;
	}

	/** get reply from socket **/
	n = recvfrom(udpSocket, buf, 8, 0, (struct sockaddr*)&udpClient, &serverlen);
	closesocket(udpSocket);
    	if (n < 0) {
		_log.Log(LOG_ERROR, "KMTronic: Error reading relay status from: %s", m_szIPAddress.c_str());
		return;
	}

//	_log.Debug(DEBUG_HARDWARE, "KMTronic: response %s",buf);

	m_TotRelais=n;
	int jj;

	for (jj = 0; jj < m_TotRelais; jj++)
	{
        	bool bIsOn = (buf[jj] != '0');
                std::stringstream sstr;
                int iRelay = (jj + 1);
                sstr << "Relay " << iRelay;
		SendSwitch(iRelay, 1, 255, bIsOn, 0, sstr.str(), m_Name);
        }
}
