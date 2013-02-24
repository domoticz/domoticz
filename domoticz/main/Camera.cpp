#include "stdafx.h"
#include <iostream>
#include "Camera.h"
#include "mainworker.h"
#include "localtime_r.h"
#include "Logger.h"

CCamScheduler::CCamScheduler(void)
{
	m_pMain=NULL;
	m_stoprequested=false;
}

CCamScheduler::~CCamScheduler(void)
{
}

void CCamScheduler::StartCameraGrabber(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;
	ReloadCameras();
    CheckCameras();
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CCamScheduler::Do_Work, this)));
}

void CCamScheduler::StopCameraGrabber()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
}

std::vector<cameraDevice> CCamScheduler::GetCameraDevices()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::vector<cameraDevice> ret;

	std::vector<cameraDevice>::iterator itt;
	for (itt=m_cameradevices.begin(); itt!=m_cameradevices.end(); ++itt)
	{
		ret.push_back(*itt);
	}
	return ret;
}

void CCamScheduler::ReloadCameras()
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	m_cameradevices.clear();
    _log.Log(LOG_NORM,"Camera settings (re)loaded");
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	szQuery << "SELECT ID, Name, Address, Port, Username, Password FROM Cameras WHERE (Enabled == 1) ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			cameraDevice citem;

			std::stringstream s_str( sd[0] );
			s_str >> citem.DevID;
            citem.Name = sd[1].c_str();
            citem.Address = sd[2].c_str();
			citem.Port= atoi(sd[3].c_str());
		    citem.Username=sd[4].c_str();
            citem.Password =sd[5].c_str();
            m_cameradevices.push_back(citem);
            
		}
	}

}

void CCamScheduler::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::seconds(30));
		CheckCameras();
	}
	_log.Log(LOG_NORM,"Camera fetch stopped...");
}

void CCamScheduler::CheckCameras()
{
	boost::lock_guard<boost::mutex> l(m_mutex);

	time_t atime=time(NULL);
	struct tm ltime;
	localtime_r(&atime,&ltime);
    //_log.Log(LOG_NORM,"Camera tick");

}
