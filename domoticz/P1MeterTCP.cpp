#include "stdafx.h"
#include "P1MeterTCP.h"
#include <iostream>

#define RETRY_DELAY 30

P1MeterTCP::P1MeterTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort)
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
			std::cout << "Winsock could not be initialized!" << std::endl;
		}
	}
	m_socket=INVALID_SOCKET;
#endif
	m_stoprequested=false;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
}

P1MeterTCP::~P1MeterTCP(void)
{
#if defined WIN32
	//
	// Release WinSock
	//
	WSACleanup();
#endif
}

bool P1MeterTCP::StartHardware()
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

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&P1MeterTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool P1MeterTCP::StopHardware()
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
	return true;
}

bool P1MeterTCP::ConnectInternal()
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		std::cerr << "could not create a TCP/IP socket!" << std::endl;
		return false;
	}

	// connect to the server
	int nRet;
	nRet = connect(m_socket,(const sockaddr*)&m_addr, sizeof(m_addr));
	if (nRet == SOCKET_ERROR)
	{
		closesocket(m_socket);
		m_socket=INVALID_SOCKET;
		std::cerr << "P1 Smart Meter could not connect to: " << m_szIPAddress << ":" << std::dec << m_usIPPort << std::endl;
		return false;
	}

	std::cout << "P1 Smart Meter connected to: " << m_szIPAddress << ":" << std::dec << m_usIPPort << std::endl;

	Init();

	sOnConnected(this);
	return true;
}

void P1MeterTCP::disconnect()
{
	m_stoprequested=true;
	if (m_socket==INVALID_SOCKET)
		return;
	closesocket(m_socket);	//will terminate the thread
	m_socket=INVALID_SOCKET;
}


void P1MeterTCP::Do_Work()
{
	while (!m_stoprequested)
	{
		if (
			(m_socket == INVALID_SOCKET)&&
			(!m_stoprequested)
			)
		{
			boost::this_thread::sleep(boost::posix_time::seconds(1));
			m_retrycntr++;
			if (m_retrycntr>=RETRY_DELAY)
			{
				m_retrycntr=0;
				if (!ConnectInternal())
				{
					std::cout << "retrying in " << std::dec << RETRY_DELAY << " seconds..." << std::endl;
					continue;
				}
			}
		}
		else
		{
			unsigned char data[1028];
			int bread=recv(m_socket,(char*)&data,sizeof(data),0);
			if ((bread==0)||(bread<0)) {
				std::cout << "P1 TCP/IP connection closed!" << std::endl;
				closesocket(m_socket);
				m_socket=INVALID_SOCKET;
				if (!m_stoprequested)
				{
					std::cout << "retrying in " << std::dec << RETRY_DELAY << " seconds..." << std::endl;
					m_retrycntr=0;
					continue;
				}
			}
			else
			{
				boost::lock_guard<boost::mutex> l(readQueueMutex);
				ParseData((const unsigned char*)&data,bread);
			}
		}
	}
	std::cout << "P1 TCP/IP Worker stopped...\n";
} 

void P1MeterTCP::write(const char *data, size_t size)
{
}

void P1MeterTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	if (isConnected())
		write(pdata,length);
}
