#include "stdafx.h"
#include "SolarEdgeTCP.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <iostream>
#include "TCPProxy/tcpproxy_server.h"

#include "../main/localtime_r.h"
#include "../main/mainworker.h"

#define RETRY_DELAY 30

SolarEdgeTCP::SolarEdgeTCP(const int ID, const unsigned short usListenPort, const std::string &DestHost, const unsigned short usDestPort)
{
	m_HwdID=ID;
	m_pTCPProxy=NULL;
	m_stoprequested=false;
	m_szSourceHost="0.0.0.0";
	m_usSourcePort=usListenPort;
	m_szDestHost=DestHost;
	std::stringstream sstr;
	sstr << usDestPort;
	m_szDestPort=sstr.str();
}

SolarEdgeTCP::~SolarEdgeTCP(void)
{
	if (m_pTCPProxy)
		delete m_pTCPProxy;
	m_pTCPProxy=NULL;
}

bool SolarEdgeTCP::StartHardware()
{
	m_stoprequested=false;

	//force connect the next first time
	m_retrycntr=RETRY_DELAY;
	m_bIsStarted=true;

	m_state=E_WAITING_SYNC;
	m_bufferpos=0;
	m_wantedlength=0;

	StartHeartbeatThread();

	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&SolarEdgeTCP::Do_Work, this)));
	return (m_thread!=NULL);
}

bool SolarEdgeTCP::StopHardware()
{
	m_bIsStarted=false;
	m_stoprequested=true;
	if (m_pTCPProxy==NULL)
		return true;
	m_pTCPProxy->stop();
	delete m_pTCPProxy;
	m_pTCPProxy=NULL;
	StopHeartbeatThread();
	return true;
}

void SolarEdgeTCP::Do_Work()
{
	while (!m_stoprequested)
	{
		try
		{
			if (m_pTCPProxy!=NULL)
				delete m_pTCPProxy;
			m_pTCPProxy=NULL;
			try {
				m_pTCPProxy= new tcp_proxy::acceptor(m_szSourceHost,m_usSourcePort,m_szDestHost,m_szDestPort);
			}
			catch(const std::exception &e) {
				_log.Log(LOG_ERROR,"SolarEdge: Proxy Error: %s", e.what());
				break;
			}
			m_pTCPProxy->sOnDownstreamData.connect( boost::bind( &SolarEdgeTCP::OnDownstreamData, this, _1, _2 ) );
			m_pTCPProxy->sOnUpstreamData.connect( boost::bind( &SolarEdgeTCP::OnUpstreamData, this, _1, _2 ) );
			m_pTCPProxy->start();
			_log.Log(LOG_STATUS,"SolarEdge: Started");
		}
		catch(const std::exception &e) 
		{
			if (m_stoprequested)
				break;
			_log.Log(LOG_ERROR,"SolarEdge: Proxy Error: %s", e.what());
			m_pTCPProxy->stop();
			continue;
		}
		catch(...)
		{
			//_log.Log(LOG_ERROR,"SolarEdge: Proxy server stopped by exception, starting again...");
			m_pTCPProxy->stop();
			continue;
		}
	}
	_log.Log(LOG_STATUS,"SolarEdge: Worker stopped...");
}

void SolarEdgeTCP::write(const char *data, size_t size)
{
}

bool SolarEdgeTCP::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

void SolarEdgeTCP::OnUpstreamData(const unsigned char *pData, const size_t Len)
{
	ParseData(pData,Len);
}
void SolarEdgeTCP::OnDownstreamData(const unsigned char *pData, const size_t Len)
{
	ParseData(pData,Len);
}
