#include "stdafx.h"
#include <iostream>
#include "Camera.h"
#include "mainworker.h"
#include "localtime_r.h"
#include "Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../smtpclient/SMTPClient.h"
#include "../webserver/Base64.h"

#define CAMERA_POLL_INTERVAL 30

CCamScheduler::CCamScheduler(void)
{
	m_pMain=NULL;
	m_stoprequested=false;
	m_seconds_counter=0;
}

CCamScheduler::~CCamScheduler(void)
{
}

void CCamScheduler::StartCameraGrabber(MainWorker *pMainWorker)
{
	m_pMain=pMainWorker;
	ReloadCameras();
	m_seconds_counter=CAMERA_POLL_INTERVAL;
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
	std::stringstream szQuery;
	std::vector<std::vector<std::string> > result;

	szQuery << "SELECT ID, Name, Address, Port, Username, Password, VideoURL, ImageURL FROM Cameras WHERE (Enabled == 1) ORDER BY ID";
	result=m_pMain->m_sql.query(szQuery.str());
	if (result.size()>0)
	{
		_log.Log(LOG_NORM,"Camera settings (re)loaded");
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			std::vector<std::string> sd=*itt;

			cameraDevice citem;

			std::stringstream s_str( sd[0] );
			s_str >> citem.ID;
            citem.Name		= sd[1].c_str();
            citem.Address	= sd[2].c_str();
			citem.Port		= atoi(sd[3].c_str());
		    citem.Username	= sd[4].c_str();
            citem.Password	= sd[5].c_str();
			citem.VideoURL	= sd[6].c_str();
			citem.ImageURL	= sd[7].c_str();
            m_cameradevices.push_back(citem);
		}
	}
}

void CCamScheduler::Do_Work()
{
	while (!m_stoprequested)
	{
		//sleep 1 second
		boost::this_thread::sleep(boost::posix_time::seconds(1));
		if (m_stoprequested)
			break;
		m_seconds_counter++;
		if (m_seconds_counter>=CAMERA_POLL_INTERVAL)
		{
			m_seconds_counter=0;
			CheckCameras();
		}
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

std::string CCamScheduler::GetCameraURL(cameraDevice *pCamera)
{
	std::stringstream s_str;
	if (pCamera->Username!="")
		s_str << "http://" << pCamera->Username << ":" << pCamera->Password << "@" << pCamera->Address << ":" << pCamera->Port;
	else
		s_str << "http://" << pCamera->Username << ":" << pCamera->Password << "@" << pCamera->Address << ":" << pCamera->Port;
	return s_str.str();
}

cameraDevice* CCamScheduler::GetCamera(const std::string CamID)
{
	unsigned long long ulID;
	std::stringstream s_str( CamID );
	s_str >> ulID;
	return GetCamera(ulID);
}

cameraDevice* CCamScheduler::GetCamera(const unsigned long long CamID)
{
	boost::lock_guard<boost::mutex> l(m_mutex);
	std::vector<cameraDevice>::iterator itt;
	for (itt=m_cameradevices.begin(); itt!=m_cameradevices.end(); ++itt)
	{
		if (itt->ID==CamID)
			return &(*itt);
	}
	return NULL;
}

bool CCamScheduler::TakeSnapshot(const std::string CamID, std::vector<unsigned char> &camimage)
{
	unsigned long long ulID;
	std::stringstream s_str( CamID );
	s_str >> ulID;
	return TakeSnapshot(ulID,camimage);
}

bool CCamScheduler::TakeSnapshot(const unsigned long long CamID, std::vector<unsigned char> &camimage)
{
	cameraDevice *pCamera=GetCamera(CamID);
	if (pCamera==NULL)
		return false;

	std::string szURL=GetCameraURL(pCamera);
	szURL+="/" + pCamera->ImageURL;

	return HTTPClient::GETBinary(szURL,camimage);
}

bool CCamScheduler::EmailCameraSnapshot(const std::string CamIdx, const std::string subject)
{
	int nValue;
	std::string sValue;
	if (!m_pMain->m_sql.GetPreferencesVar("EmailServer",nValue,sValue))
	{
		return false;//no email setup
	}
	if (sValue=="")
	{
		return false;//no email setup
	}
	std::vector<unsigned char> camimage;
	if (CamIdx=="")
		return false;

	if (!TakeSnapshot(CamIdx, camimage))
		return false;

	std::string EmailFrom;
	std::string EmailTo;
	std::string EmailServer=sValue;
	int EmailPort=25;
	std::string EmailUsername;
	std::string EmailPassword;
	m_pMain->m_sql.GetPreferencesVar("EmailFrom",nValue,EmailFrom);
	m_pMain->m_sql.GetPreferencesVar("EmailTo",nValue,EmailTo);
	m_pMain->m_sql.GetPreferencesVar("EmailUsername",nValue,EmailUsername);
	m_pMain->m_sql.GetPreferencesVar("EmailPassword",nValue,EmailPassword);

	std::string htmlMsg=
		"<html>\r\n"
		"<body>\r\n"
		"<img src=\"data:image/jpeg;base64,";
	std::vector<char> filedata;
	filedata.insert(filedata.begin(),camimage.begin(),camimage.end());
	std::string imgstring;
	imgstring.insert(imgstring.end(),filedata.begin(),filedata.end());
	imgstring=base64_encode((const unsigned char*)imgstring.c_str(),filedata.size());
	htmlMsg+=
		imgstring +
		"\">\r\n"
		"</body>\r\n"
		"</html>\r\n";

	bool bRet=SMTPClient::SendEmail(
		CURLEncode::URLDecode(EmailFrom.c_str()),
		CURLEncode::URLDecode(EmailTo.c_str()),
		CURLEncode::URLDecode(EmailServer.c_str()),
		EmailPort,
		base64_decode(EmailUsername),
		base64_decode(EmailPassword),
		CURLEncode::URLDecode(subject),
		htmlMsg,
		true
		);
	return bRet;
}
