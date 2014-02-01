#include "stdafx.h"
#include "RFXComTCP.h"
#include "../main/Logger.h"
//#include <boost/bind.hpp>
//#include <boost/asio.hpp>
#include "../main/Helper.h"

#define RETRY_DELAY 30

RFXComTCP::RFXComTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
#if defined WIN32
	int ret;
	//Init winsock
	WSADATA data;
	WORD version; 

	version = (MAKEWORD(2, 2)); 
	ret = WSAStartup(version, &data); 
	if (ret != 0) 
	{  
		ret = WSAGetLastError(); 

		if (ret == WSANOTINITIALISED) 
		{  
			_log.Log(LOG_ERROR,"Winsock could not be initialized!");
		}
	}
#endif
	m_socket=INVALID_SOCKET;
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
}

RFXComTCP::~RFXComTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool RFXComTCP::StartHardware()
{
	m_stoprequested=false;

	memset(&m_addr,0,sizeof(sockaddr_in));
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(m_usIPPort);

	unsigned long ip;
	ip=inet_addr(m_szIPAddress.c_str());

	// if we have a error in the ip, it means we have entered a string
	if(ip!=INADDR_NONE)
	{
		m_addr.sin_addr.s_addr=ip;
	}
	else
	{
		// change Hostname in serveraddr
		hostent *he=gethostbyname(m_szIPAddress.c_str());
		if(he==NULL)
		{
			return false;
		}
		else
		{
			memcpy(&(m_addr.sin_addr),he->h_addr_list[0],4);
		}
	}

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;
	m_rxbufferpos=0;
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&RFXComTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool RFXComTCP::StopHardware()
{
	if (isConnected())
	{
		try {
			disconnect();
		} catch(...)
		{
			//Don't throw from a Stop command
		}
	}
	m_bIsStarted=false;
	return true;
}

bool RFXComTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		_log.Log(LOG_ERROR,"RFXCOM could not create a TCP/IP socket!");
		return false;
	}

	// connect to the server
	int nRet;
	nRet = connect(m_socket,(const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket=INVALID_SOCKET;
		_log.Log(LOG_ERROR,"RFXCOM could not connect to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
		return false;
	}

	_log.Log(LOG_NORM,"RFXCOM connected to: %s:%ld", m_szIPAddress.c_str(), m_usIPPort);
	sOnConnected(this);
	return true;
}

void RFXComTCP::disconnect()
{
	m_stoprequested=true;
	if (m_socket==INVALID_SOCKET)
		return;
	closesocket(m_socket);	//will terminate the thread
	m_socket=INVALID_SOCKET;
}

void RFXComTCP::Do_Work()
{
	while (!m_stoprequested)
	{
		if (
			(m_socket == INVALID_SOCKET)&&
			(!m_stoprequested)
			)
		{
			sleep_seconds(1);
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (!ConnectInternal())
				{
					_log.Log(LOG_NORM,"retrying in %d seconds...", RETRY_DELAY);
					continue;
				}
			}
		}
		else
		{
			char buf;
			int bread=recv(m_socket,&buf,1,0);
			if (m_stoprequested)
				break;
			if ((bread==0)||(bread<0)) {
				_log.Log(LOG_ERROR,"TCP/IP connection closed!");
				closesocket(m_socket);
				m_socket=INVALID_SOCKET;
				if (!m_stoprequested)
				{
					_log.Log(LOG_NORM,"retrying in %d seconds...", RETRY_DELAY);
					m_retrycntr=0;
					continue;
				}
			}
			else
			{
				boost::lock_guard<boost::mutex> l(readQueueMutex);
				onRFXMessage((const unsigned char *)&buf,1);
			}
		}
		
	}
	_log.Log(LOG_NORM,"TCP/IP Worker stopped...");
} 

void RFXComTCP::write(const char *data, size_t size)
{
	if (m_socket==INVALID_SOCKET)
		return; //not connected!
	send(m_socket,data,size,0);
}

void RFXComTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isConnected())
		write(pdata,length);
}
