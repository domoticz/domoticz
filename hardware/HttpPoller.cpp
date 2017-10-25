#include "stdafx.h"
#include "HttpPoller.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "hardwaretypes.h"
#include "../httpclient/HTTPClient.h"
#include "../json/json.h"
#include "../webserver/Base64.h"
#include "../main/WebServer.h"
#include "../main/LuaHandler.h"

#define round(a) ( int ) ( a + .5 )

CHttpPoller::CHttpPoller(const int ID, const std::string& username, const std::string& password, const std::string& url, const std::string& extradata, const unsigned short refresh) :
m_username(CURLEncode::URLEncode(username)),
m_password(CURLEncode::URLEncode(password)),
m_url(url),
m_refresh(refresh)
{
	// extract the data
	std::vector<std::string> strextra;
	StringSplit(extradata, "|", strextra);
	if (strextra.size() == 3 || strextra.size() == 4 || strextra.size() == 5)
	{
		m_script = base64_decode(strextra[0]);
		m_method = atoi(base64_decode(strextra[1]).c_str());
		m_contenttype = base64_decode(strextra[2]);
		if (strextra.size() >= 4)
		{
			m_headers = base64_decode(strextra[3]);
			if (strextra.size() == 5)
			{
				m_postdata = base64_decode(strextra[4]);
			}
		}
	}

	m_HwdID=ID;

	m_stoprequested=false;
	Init();
}

CHttpPoller::~CHttpPoller(void)
{
}

void CHttpPoller::Init()
{
}

bool CHttpPoller::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

bool CHttpPoller::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CHttpPoller::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	return (m_thread!=NULL);
}

bool CHttpPoller::StopHardware()
{
	if (m_thread!=NULL)
	{
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CHttpPoller::Do_Work()
{
	int sec_counter = 300 - 5;
	_log.Log(LOG_STATUS, "Http: Worker started...");
	while (!m_stoprequested)
	{
		sleep_seconds(1);
		if (m_stoprequested)
			break;
		sec_counter++;
		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(NULL);
		}
		if (sec_counter % m_refresh == 0) {
			GetScript();
		}
	}
	_log.Log(LOG_STATUS,"Http: Worker stopped...");
}

void CHttpPoller::GetScript()
{
	std::string sURL(m_url);
	std::vector<std::string> ExtraHeaders;
	std::string sResult;

	if (m_contenttype.length() > 0) {
		ExtraHeaders.push_back("Content-type: " + m_contenttype);
	}

	if (m_headers.length() > 0)
	{
		std::vector<std::string> ExtraHeaders2;
		StringSplit(m_headers, "\n", ExtraHeaders2);
		for (size_t i = 0; i < ExtraHeaders2.size(); i++)
		{
			ExtraHeaders.push_back(ExtraHeaders2[i]);
		}
	}

	std::string auth;
	if (m_username.length() > 0 || m_password.length() > 0)
	{
		if (m_username.length() > 0)
		{
			auth += m_username;
		}
		auth += ":";
		if (m_password.length() > 0)
		{
			auth += m_password;
		}
		std::string encodedAuth = base64_encode((const unsigned char *)auth.c_str(), auth.length());
		ExtraHeaders.push_back("Authorization:Basic " + encodedAuth);
	}

	if (m_method == 0) {
		if (!HTTPClient::GET(sURL, ExtraHeaders, sResult))
		{
			std::string err = "Http: Error getting data from url \"" + sURL + "\"";
			_log.Log(LOG_ERROR, err.c_str());
			return;
		}
	}
	if (m_method == 1) {
		if (!HTTPClient::POST(sURL, m_postdata, ExtraHeaders, sResult)) {
			std::string err = "Http: Error getting data from url \"" + sURL + "\"";
			_log.Log(LOG_ERROR, err.c_str());
			return;
		}
	}

	// Got some data, send them to the lua parsers for processing
	CLuaHandler luaScript(m_HwdID);
	luaScript.executeLuaScript(m_script, sResult);
}
