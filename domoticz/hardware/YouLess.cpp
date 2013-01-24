#include "stdafx.h"
#include "YouLess.h"
#include "../main/Helper.h"
#include "../httpclient/mynetwork.h"
#include "hardwaretypes.h"

#define YOULESS_POLL_INTERVAL 10

CYouLess::CYouLess(const int ID, const std::string IPAddress, const unsigned short usIPPort)
{
	m_HwdID=ID;
	m_szIPAddress=IPAddress;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	Init();
}

CYouLess::~CYouLess(void)
{
}

void CYouLess::Init()
{
	m_PollCounter=YOULESS_POLL_INTERVAL-2;
	m_meter.len=sizeof(YouLessMeter)-1;
	m_meter.type=pTypeYouLess;
	m_meter.subtype=sTypeYouLess;
	m_meter.powerusage=0;
	m_meter.usagecurrent=0;

	m_meter.ID=boost::to_string(m_usIPPort);
}

bool CYouLess::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CYouLess::Do_Work, this)));

	return (m_thread!=NULL);
}

bool CYouLess::StopHardware()
{
	m_stoprequested=true;
	if (m_thread)
		m_thread->join();
	return true;
}

void CYouLess::Do_Work()
{
	while (!m_stoprequested)
	{
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		m_PollCounter++;
		if (m_PollCounter>=YOULESS_POLL_INTERVAL)
		{
			GetMeterDetails();
			m_PollCounter=0;
		}
	}
	std::cout << "YouLess Worker stopped...\n";
}

void CYouLess::WriteToHardware(const char *pdata, const unsigned char length)
{

}

void CYouLess::GetMeterDetails()
{
	unsigned char *pData=NULL;
	unsigned long ulLength=0;

	char szURL[200];
	sprintf(szURL,"http://%s:%d/a",m_szIPAddress.c_str(), m_usIPPort);
	I_HTTPRequest * r = NewHTTPRequest( szURL );
	if (r!=NULL)
	{
		if (r->readDataInVecBuffer())
		{
			r->getBuffer(pData, ulLength);
		}
		r->dispose();
	}
	if ((pData==NULL)||(ulLength<5))
	{
		std::cout << "YouLess error connecting to: " << m_szIPAddress << std::endl;
		return;
	}
	std::string response=(char*)pData;
	delete [] pData;
	std::vector<std::string> results;
	StringSplit(response, "\n", results);
	if (results.size()<2)
	{
		std::cout << "YouLess error connecting to: " << m_szIPAddress << std::endl;
		return;
	}
	int fpos;
	std::string pusage=results[0];
	fpos=pusage.find_first_of(" ");
	if (fpos!=std::string::npos)
		pusage=pusage.substr(0,fpos);
	pusage=stdreplace(pusage,",","");

	std::string pcurrent=results[1];
	fpos=pcurrent.find_first_of(" ");
	if (fpos!=std::string::npos)
		pcurrent=pcurrent.substr(0,fpos);
	pcurrent=stdreplace(pcurrent,",","");

	m_meter.powerusage=atol(pusage.c_str());
	m_meter.usagecurrent=atol(pcurrent.c_str());
	sDecodeRXMessage(this, (const unsigned char *)&m_meter);//decode message
	m_sharedserver.SendToAll((const char*)&m_meter,sizeof(m_meter));
}
