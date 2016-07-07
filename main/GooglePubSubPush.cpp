#include "stdafx.h"
#include "GooglePubSubPush.h"
#include "Helper.h"
#include "Logger.h"
#include "../hardware/hardwaretypes.h"
#include "RFXtrx.h"
#include "SQLHelper.h"
#include <boost/date_time/c_local_time_adjustor.hpp>
#include "../webserver/Base64.h"
#include "../main/localtime_r.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../main/mainworker.h"
#include "../json/json.h"

#ifndef WIN32
	#include <unistd.h> //gethostbyname
#endif

#ifdef ENABLE_PYTHON
extern "C" {
#include <Python.h>
}
#include <boost/python.hpp>
using namespace boost::python;
#endif

extern std::string szUserDataFolder;

typedef struct _STR_TABLE_ID1_ID2 {
	unsigned long    id1;
	unsigned long    id2;
	const char   *str1;
} STR_TABLE_ID1_ID2;

static boost::posix_time::time_duration get_utc_offset() {
	using namespace boost::posix_time;

	// boost::date_time::c_local_adjustor uses the C-API to adjust a
	// moment given in utc to the same moment in the local time zone.
	typedef boost::date_time::c_local_adjustor<ptime> local_adj;

	const ptime utc_now = second_clock::universal_time();
	const ptime now = local_adj::utc_to_local(utc_now);

	return now - utc_now;
}

CGooglePubSubPush::CGooglePubSubPush()
{
	m_bLinkActive = false;
}

void CGooglePubSubPush::Start()
{
	UpdateActive();
	m_sConnection = m_mainworker.sOnDeviceReceived.connect(boost::bind(&CGooglePubSubPush::OnDeviceReceived, this, _1, _2, _3, _4));
}

void CGooglePubSubPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}


void CGooglePubSubPush::UpdateActive()
{
	int fActive;
	m_sql.GetPreferencesVar("GooglePubSubActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CGooglePubSubPush::OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	m_DeviceRowIdx = DeviceRowIdx;
	if (m_bLinkActive)
	{
		DoGooglePubSubPush();
	}
}

void CGooglePubSubPush::DoGooglePubSubPush()
{			
	std::string googlePubSubData = "";

	int googlePubSubDebugActiveInt;
	bool googlePubSubDebugActive = false;
	m_sql.GetPreferencesVar("googlePubSubDebug", googlePubSubDebugActiveInt);
	if (googlePubSubDebugActiveInt == 1) {
		googlePubSubDebugActive = true;
	}
	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query(
		"SELECT A.DeviceID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, A.IncludeUnit, B.SwitchType, strftime('%%s', B.LastUpdate), B.Name FROM GooglePubSubLink as A, DeviceStatus as B "
		"WHERE (A.DeviceID == '%llu' AND A.Enabled = '1' AND A.DeviceID==B.ID)",
		m_DeviceRowIdx);
	if (result.size()>0)
	{
		std::string sendValue;
		std::vector<std::vector<std::string> >::const_iterator itt;
		for (itt=result.begin(); itt!=result.end(); ++itt)
		{
			m_sql.GetPreferencesVar("GooglePubSubData", googlePubSubData);
			if (googlePubSubData == "")
				return;

			std::vector<std::string> sd=*itt;
			unsigned int deviceId = atoi(sd[0].c_str());
			std::string ldelpos = sd[1].c_str();
			int delpos = atoi(sd[1].c_str());
			int dType = atoi(sd[3].c_str());
			int dSubType = atoi(sd[4].c_str());
			std::string lType = sd[3].c_str();
			std::string lSubType = sd[4].c_str();
			int nValue = atoi(sd[5].c_str());
			std::string sValue = sd[6].c_str();
			int targetType = atoi(sd[7].c_str());
			std::string targetVariable = sd[8].c_str();
			int targetDeviceID = atoi(sd[9].c_str());
			std::string targetProperty = sd[10].c_str();
			int includeUnit = atoi(sd[11].c_str());
			int metertype = atoi(sd[12].c_str());
			int lastUpdate = atoi(sd[13].c_str());
			std::string lstatus="";
			std::string lunit = "";
			std::string ltargetVariable = sd[8].c_str();
			std::string ltargetDeviceId = sd[9].c_str();
			std::string lname = sd[14].c_str();
			sendValue = sValue;

			// Compute tz
			boost::posix_time::time_duration uoffset = get_utc_offset();
			unsigned long tzoffset = (int)((double)(uoffset.ticks() / 3600000000LL) * 3600);

#ifdef WIN32
			unsigned __int64 localTime = lastUpdate;
			unsigned __int64 localTimeUtc = lastUpdate - tzoffset;
#else
			unsigned long long int localTime = lastUpdate;
			unsigned long long int localTimeUtc = lastUpdate - tzoffset;
#endif

			char szLocalTime[16];
			sprintf(szLocalTime, "%llu", localTime);
			char szLocalTimeUtc[16];
			sprintf(szLocalTimeUtc, "%llu", localTimeUtc);
			char szLocalTimeMs[16];
			sprintf(szLocalTimeMs, "%llu", localTime*1000);
			char szLocalTimeUtcMs[16];
			sprintf(szLocalTimeUtcMs, "%llu", localTimeUtc * 1000);

			// RFC3339 time format
			time_t tmpT = localTimeUtc;
			struct tm* timeinfo = gmtime(&tmpT);

			char llastUpdate[255];
#if !defined WIN32
			snprintf(llastUpdate, sizeof(llastUpdate), "%04d-%02d-%02dT%02d:%02d:%02dZ",
				timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
#else
			sprintf_s(llastUpdate, sizeof(llastUpdate), "%04d-%02d-%02dT%02d:%02d:%02dZ",
				timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
#endif

			// Replace keywords
			/*
			%v : Value
			%t0 : Timestamp (epoc time localtime)
			%t1 : Timestamp (epoc ms localtime)
			%t2 : Timestamp (epoc time UTC)
			%t3 : Timestamp (epoc ms UTC)
			%t4 : Timestamp : "2015-01-29T21:50:44Z"
			%D : Target Device id
			%V : Target Variable
			%u : Unit
			%n : Name
			%T0 : Type
			%T1 : SubType
			%h : hostname
			*/

			lunit = getUnit(delpos, metertype);			
			lType = RFX_Type_Desc(dType,1);
			lSubType = RFX_Type_SubType_Desc(dType,dSubType);
			
			char hostname[256];
			gethostname(hostname, sizeof(hostname));

			std::vector<std::string> strarray;
			if (sendValue.find(";")!=std::string::npos)
			{
				StringSplit(sendValue, ";", strarray);
				if (int(strarray.size())>=delpos)
				{
					std::string rawsendValue = strarray[delpos-1].c_str();
					sendValue = ProcessSendValue(rawsendValue,delpos,nValue,false,metertype);
				}
			}
			else
			{
				sendValue = ProcessSendValue(sendValue,delpos,nValue,false,metertype);
			}
			ltargetDeviceId+="_";
			ltargetDeviceId+=ldelpos;

			replaceAll(googlePubSubData, "%v", sendValue);
			replaceAll(googlePubSubData, "%u", includeUnit ? lunit : "");
			replaceAll(googlePubSubData, "%D", ltargetDeviceId);
			replaceAll(googlePubSubData, "%V", ltargetVariable);
			replaceAll(googlePubSubData, "%t0", std::string(szLocalTime));
			replaceAll(googlePubSubData, "%t1", std::string(szLocalTimeMs));
			replaceAll(googlePubSubData, "%t2", std::string(szLocalTimeUtc));
			replaceAll(googlePubSubData, "%t3", std::string(szLocalTimeUtcMs));
			replaceAll(googlePubSubData, "%t4", std::string(llastUpdate));
			replaceAll(googlePubSubData, "%n", std::string(lname));
			replaceAll(googlePubSubData, "%T0", lType);
			replaceAll(googlePubSubData, "%T1", lSubType);
			replaceAll(googlePubSubData, "%h", std::string(hostname));

			if (sendValue != "") {
				std::stringstream python_DirT;

#ifdef ENABLE_PYTHON
#ifdef WIN32
				python_DirT << szUserDataFolder << "scripts\\python\\";
#else
				python_DirT << szUserDataFolder << "scripts/python/";
#endif
				std::string python_Dir = python_DirT.str();
				if (!Py_IsInitialized()) {
					Py_SetProgramName((char*)Python_exe); // will this cast lead to problems ?
					Py_Initialize();
					Py_InitModule("domoticz_", DomoticzMethods);

					// TODO: may have a small memleak, remove references in destructor
					PyObject* sys = PyImport_ImportModule("sys");
					PyObject *path = PyObject_GetAttrString(sys, "path");
					PyList_Append(path, PyString_FromString(python_Dir.c_str()));

					FILE* PythonScriptFile = fopen(filename.c_str(), "r");
					object main_module = import("__main__");
					object main_namespace = dict(main_module.attr("__dict__")).copy();
				}

				try {
					object domoticz_module = import("domoticz");
					object reloader = import("reloader");
					reloader.attr("_check_reload")();

					domoticz_namespace["data"] = googlePubSubData;

					// debug
					if (googlePubSubDebugActive) {
						_log.Log(LOG_NORM, "GooglePubSubLink: data to send : ", googlePubSubData.c_str());
					}

					if (PyString.length() > 0)
						exec(str(PyString), main_namespace);
					else
						object ignored = exec_file(str(filename), main_namespace);
				}
				catch (...) {
					PyObject *exc, *val, *tb;
					PyErr_Fetch(&exc, &val, &tb);
					boost::python::handle<> hexc(exc), hval(boost::python::allow_null(val)), htb(boost::python::allow_null(tb));
					boost::python::object traceback(boost::python::import("traceback"));

					boost::python::object format_exception(traceback.attr("format_exception"));
					boost::python::object formatted_list = format_exception(hexc, hval, htb);
					boost::python::object formatted = boost::python::str("\n").join(formatted_list);

					object traceback_module = import("traceback");
					std::string formatted_str = extract<std::string>(formatted);
					//PyErr_Print();
					PyErr_Clear();
					if (PyString.length() > 0)
						_log.Log(LOG_ERROR, "in event %s: %s", filename.c_str(), formatted_str.c_str());
					else
						_log.Log(LOG_ERROR, "%s", formatted_str.c_str());
				}
#else
				_log.Log(LOG_ERROR, "Error sending data to GooglePubSub : Python not available!");
#endif
			}
		}
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_SaveGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string url = request::findValue(&req, "url");
			std::string data = request::findValue(&req, "data");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string debugenabled = request::findValue(&req, "debugenabled");
			if (
				(url == "") ||
				(data == "") ||
				(linkactive == "") ||
				(debugenabled == "")
				)
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("GooglePubSubUrl", url.c_str());
			m_sql.UpdatePreferencesVar("GooglePubSubData", data.c_str());
			m_sql.UpdatePreferencesVar("GooglePubSubActive", ilinkactive);
			m_sql.UpdatePreferencesVar("GooglePubSubDebug", idebugenabled);

			m_mainworker.m_googlepubsubpush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveGooglePubSubLinkConfig";
		}

		void CWebServer::Cmd_GetGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
				return;//Only admin user allowed
			std::string sValue;
			int nValue;
			if (m_sql.GetPreferencesVar("GooglePubSubActive", nValue)) {
				root["GooglePubSubActive"] = nValue;
			}
			else {
				root["GooglePubSubActive"] = 0;
			}
			if (m_sql.GetPreferencesVar("GooglePubSubDebug", nValue)) {
				root["GooglePubSubDebug"] = nValue;
			}
			else {
				root["GooglePubSubDebug"] = 0;
			}
			if (m_sql.GetPreferencesVar("GooglePubSubData", sValue))
			{
				root["GooglePubSubData"] = sValue;
			}
			root["status"] = "OK";
			root["title"] = "GetGooglePubSubLinkConfig";
		}

		void CWebServer::Cmd_GetGooglePubSubLinks(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
				return;//Only admin user allowed
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM GooglePubSubLink as A, DeviceStatus as B WHERE (A.DeviceID==B.ID)");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["DeviceID"] = sd[1];
					root["result"][ii]["Delimitedvalue"] = sd[2];
					root["result"][ii]["TargetType"] = sd[3];
					root["result"][ii]["TargetVariable"] = sd[4];
					root["result"][ii]["TargetDevice"] = sd[5];
					root["result"][ii]["TargetProperty"] = sd[6];
					root["result"][ii]["Enabled"] = sd[7];
					root["result"][ii]["Name"] = sd[8];
					root["result"][ii]["IncludeUnit"] = sd[9];
					ii++;
				}
			}
			root["status"] = "OK";
			root["title"] = "GetGooglePubSubLinks";
		}

		void CWebServer::Cmd_SaveGooglePubSubLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
				return;//Only admin user allowed
			std::string idx = request::findValue(&req, "idx");
			std::string deviceid = request::findValue(&req, "deviceid");
			int deviceidi = atoi(deviceid.c_str());
			std::string valuetosend = request::findValue(&req, "valuetosend");
			std::string targettype = request::findValue(&req, "targettype");
			int targettypei = atoi(targettype.c_str());
			std::string targetvariable = request::findValue(&req, "targetvariable");
			std::string targetdeviceid = request::findValue(&req, "targetdeviceid");
			std::string targetproperty = request::findValue(&req, "targetproperty");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string includeunit = request::findValue(&req, "includeunit");
			if ((targettypei == 0) && (targetvariable == ""))
				return;
			if ((targettypei == 1) && ((targetdeviceid == "") || (targetproperty == "")))
				return;
			if ((targettypei == 2) && (targetdeviceid == ""))
				return;
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO GooglePubSubLink (DeviceID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%q','%d','%q','%d','%d')",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str())
					);
			}
			else {
				m_sql.safe_query(
					"UPDATE GooglePubSubLink SET DeviceID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%q', TargetDeviceID=%d, TargetProperty='%q', IncludeUnit='%d', Enabled='%d' WHERE (ID == '%q')",
					deviceidi,
					atoi(valuetosend.c_str()),
					targettypei,
					targetvariable.c_str(),
					atoi(targetdeviceid.c_str()),
					targetproperty.c_str(),
					atoi(includeunit.c_str()),
					atoi(linkactive.c_str()),
					idx.c_str()
					);
			}
			root["status"] = "OK";
			root["title"] = "SaveGooglePubSubLink";
		}

		void CWebServer::Cmd_DeleteGooglePubSubLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				//No admin user, and not allowed to be here
				return;
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx == "")
				return;
			m_sql.safe_query("DELETE FROM GooglePubSubLink WHERE (ID=='%q')", idx.c_str());
			root["status"] = "OK";
			root["title"] = "DeleteGooglePubSubLink";
		}
	}
}
