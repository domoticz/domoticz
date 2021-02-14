#include "stdafx.h"
#include "GooglePubSubPush.h"
#include "../hardware/hardwaretypes.h"
#include <json/json.h>
#include "../main/Helper.h"
#include "../main/localtime_r.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/Base64.h"
#include "../webserver/cWebem.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#ifdef ENABLE_PYTHON_DECAP
extern "C" {
#include <Python.h>
}
#include <boost/python.hpp>
#endif

extern std::string szUserDataFolder;
extern CGooglePubSubPush m_googlepubsubpush;

// this should be filled in by the preprocessor
extern const char * Python_exe;

#ifdef ENABLE_PYTHON_DECAP
static struct PyModuleDef eventModuledef;
#endif //ENABLE_PYTHON_DECAP

CGooglePubSubPush::CGooglePubSubPush()
{
	m_PushType = PushType::PUSHTYPE_GOOGLE_PUB_SUB;
	m_bLinkActive = false;
}

void CGooglePubSubPush::Start()
{
	UpdateActive();
	ReloadPushLinks(m_PushType);
	m_sConnection = m_mainworker.sOnDeviceReceived.connect([this](auto id, auto idx, const auto &name, auto rx) { OnDeviceReceived(id, idx, name, rx); });
}

void CGooglePubSubPush::Stop()
{
	if (m_sConnection.connected())
		m_sConnection.disconnect();
}


void CGooglePubSubPush::UpdateActive()
{
	int fActive = 0;
	m_sql.GetPreferencesVar("GooglePubSubActive", fActive);
	m_bLinkActive = (fActive == 1);
}

void CGooglePubSubPush::OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand)
{
	if (m_bLinkActive)
	{
		DoGooglePubSubPush(DeviceRowIdx);
	}
}


#ifdef ENABLE_PYTHON_DECAP
static int numargs = 0;

/* Return the number of arguments of the application command line */
static PyObject*
PyDomoticz_log(PyObject *self, PyObject *args)
{
	char* msg;
	int type;
	if (!PyArg_ParseTuple(args, "is", &type, &msg))
		return NULL;
	_log.Log((_eLogLevel)type, msg);
	Py_INCREF(Py_None);
	return Py_None;

}

static PyMethodDef DomoticzMethods[] = {
	{ "log", PyDomoticz_log, METH_VARARGS,  "log to Domoticz." },
	{ NULL, NULL, 0, NULL }
};


// from https://gist.github.com/octavifs/5362297

template <class K, class V>
boost::python::dict toPythonDict(std::map<K, V> map) {
	boost::python::dict dictionary;
	for (const auto &m : map)
		dictionary[m.first] = m.second;
	return dictionary;
}
#endif

void CGooglePubSubPush::DoGooglePubSubPush(const uint64_t DeviceRowIdx)
{
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT A.DeviceRowID, A.DelimitedValue, B.ID, B.Type, B.SubType, B.nValue, B.sValue, A.TargetType, A.TargetVariable, A.TargetDeviceID, A.TargetProperty, "
				  "A.IncludeUnit, B.SwitchType, strftime('%%s', B.LastUpdate), B.Name FROM PushLink as A, DeviceStatus as B "
				  "WHERE (A.PushType==%d AND A.DeviceRowID == '%" PRIu64 "' AND A.Enabled = '1' AND A.DeviceRowID==B.ID)",
				  PushType::PUSHTYPE_GOOGLE_PUB_SUB, DeviceRowIdx);
	if (result.empty())
		return;

	std::string googlePubSubData;
#ifdef ENABLE_PYTHON_DECAP
	bool googlePubSubDebugActive = false;

	int googlePubSubDebugActiveInt = 0;
	m_sql.GetPreferencesVar("GooglePubSubDebug", googlePubSubDebugActiveInt);
	if (googlePubSubDebugActiveInt == 1) {
		googlePubSubDebugActive = true;
	}
#endif
	for (const auto &sd : result)
	{
		std::string sendValue;

		m_sql.GetPreferencesVar("GooglePubSubData", googlePubSubData);
		if (googlePubSubData.empty())
			return;


		std::string sdeviceId = sd[0];
		std::string ldelpos = sd[1];
		int delpos = atoi(sd[1].c_str());
		int dType = atoi(sd[3].c_str());
		int dSubType = atoi(sd[4].c_str());
		int nValue = atoi(sd[5].c_str());
		std::string sValue = sd[6];
		//int targetType = atoi(sd[7].c_str());
		std::string targetVariable = sd[8];
		//int targetDeviceID = atoi(sd[9].c_str());
		std::string targetProperty = sd[10];
		int includeUnit = atoi(sd[11].c_str());
		int metertype = atoi(sd[12].c_str());
		int lastUpdate = atoi(sd[13].c_str());
		std::string ltargetVariable = sd[8];
		std::string ltargetDeviceId = sd[9];
		std::string lname = sd[14];
		sendValue = sValue;

		unsigned long tzoffset = get_tzoffset();

#ifdef WIN32
		unsigned __int64 localTime = lastUpdate;
		unsigned __int64 localTimeUtc = lastUpdate - tzoffset;
#else
		unsigned long long int localTime = lastUpdate;
		unsigned long long int localTimeUtc = lastUpdate - tzoffset;
#endif

		char szLocalTime[21];
		sprintf(szLocalTime, "%llu", localTime);
		char szLocalTimeUtc[21];
		sprintf(szLocalTimeUtc, "%llu", localTimeUtc);
		char szLocalTimeMs[21];
		sprintf(szLocalTimeMs, "%llu", localTime * 1000);
		char szLocalTimeUtcMs[21];
		sprintf(szLocalTimeUtcMs, "%llu", localTimeUtc * 1000);

		std::string llastUpdate = get_lastUpdate(localTimeUtc);

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
		%n : Device name
		%T0 : Type
		%T1 : SubType
		%h : hostname
		%idx : 'Original device' id (idx)
		*/

		std::string lunit = getUnit(dType, dSubType, delpos, metertype);
		std::string lType = RFX_Type_Desc(dType, 1);
		std::string lSubType = RFX_Type_SubType_Desc(dType, dSubType);

		char hostname[256];
		gethostname(hostname, sizeof(hostname));

		std::vector<std::string> strarray;
		if (sendValue.find(';') != std::string::npos)
		{
			StringSplit(sendValue, ";", strarray);
			if (int(strarray.size()) >= delpos)
			{
				std::string rawsendValue = strarray[delpos - 1];
				sendValue = ProcessSendValue(DeviceRowIdx, rawsendValue, delpos, nValue, false, dType, dSubType, metertype);
			}
		}
		else
		{
			sendValue = ProcessSendValue(DeviceRowIdx, sendValue, delpos, nValue, false, dType, dSubType, metertype);
		}
		if (sendValue.empty())
			continue;

		ltargetDeviceId += "_";
		ltargetDeviceId += ldelpos;

		replaceAll(googlePubSubData, "%v", sendValue);
		replaceAll(googlePubSubData, "%u", includeUnit ? lunit : "");
		replaceAll(googlePubSubData, "%D", ltargetDeviceId);
		replaceAll(googlePubSubData, "%V", ltargetVariable);
		replaceAll(googlePubSubData, "%t0", std::string(szLocalTime));
		replaceAll(googlePubSubData, "%t1", std::string(szLocalTimeMs));
		replaceAll(googlePubSubData, "%t2", std::string(szLocalTimeUtc));
		replaceAll(googlePubSubData, "%t3", std::string(szLocalTimeUtcMs));
		replaceAll(googlePubSubData, "%t4", llastUpdate);
		replaceAll(googlePubSubData, "%n", lname);
		replaceAll(googlePubSubData, "%T0", lType);
		replaceAll(googlePubSubData, "%T1", lSubType);
		replaceAll(googlePubSubData, "%h", std::string(hostname));
		replaceAll(googlePubSubData, "%idx", sdeviceId);

		std::stringstream python_DirT;

#ifdef ENABLE_PYTHON_DECAP
#ifdef WIN32
		python_DirT << szUserDataFolder << "scripts\\python\\";
		std::string filename = szUserDataFolder + "scripts\\python\\" + "googlepubsub.py";
#else
		python_DirT << szUserDataFolder << "scripts/python/";
		std::string filename = szUserDataFolder + "scripts/python/" + "googlepubsub.py";
#endif

		wchar_t * argv[1];
		argv[0] = (wchar_t *)filename.c_str();
		PySys_SetArgv(1, argv);

		std::string python_Dir = python_DirT.str();
		if (!Py_IsInitialized()) {
			Py_SetProgramName(Py_GetProgramFullPath());
			ialize();
			PyModule_Create(&eventModuledef);

			// TODO: may have a small memleak, remove references in destructor
			PyObject* sys = PyImport_ImportModule("sys");
			PyObject *path = PyObject_GetAttrString(sys, "path");
		}

		FILE* PythonScriptFile = fopen(filename.c_str(), "r");
		object main_module = import("__main__");
		object main_namespace = dict(main_module.attr("__dict__")).copy();

		try {
			object domoticz_module = import("domoticz");
			object reloader = import("reloader");
			reloader.attr("_check_reload")();

			object domoticz_namespace = domoticz_module.attr("__dict__");
			main_namespace["data"] = googlePubSubData;
			domoticz_namespace["data"] = googlePubSubData;

			// debug
			if (googlePubSubDebugActive) {
				_log.Log(LOG_NORM, "GooglePubSubLink: data to send : %s", googlePubSubData.c_str());
			}

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
			_log.Log(LOG_ERROR, "%s", formatted_str.c_str());
		}
#else
		_log.Log(LOG_ERROR, "Error sending data to GooglePubSub : Python not available!");
#endif
	}
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_SaveGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string data = request::findValue(&req, "data");
			std::string linkactive = request::findValue(&req, "linkactive");
			std::string debugenabled = request::findValue(&req, "debugenabled");
			if ((data.empty()) || (linkactive.empty()) || (debugenabled.empty()))
				return;
			int ilinkactive = atoi(linkactive.c_str());
			int idebugenabled = atoi(debugenabled.c_str());
			m_sql.UpdatePreferencesVar("GooglePubSubData", data);
			m_sql.UpdatePreferencesVar("GooglePubSubActive", ilinkactive);
			m_sql.UpdatePreferencesVar("GooglePubSubDebug", idebugenabled);

			m_googlepubsubpush.UpdateActive();
			root["status"] = "OK";
			root["title"] = "SaveGooglePubSubLinkConfig";
		}

		void CWebServer::Cmd_GetGooglePubSubLinkConfig(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
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
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT A.ID,A.DeviceRowID,A.Delimitedvalue,A.TargetType,A.TargetVariable,A.TargetDeviceID,A.TargetProperty,A.Enabled, B.Name, A.IncludeUnit FROM PushLink as A, DeviceStatus as B WHERE (A.PushType==%d AND A.DeviceRowID==B.ID)", CBasePush::PushType::PUSHTYPE_GOOGLE_PUB_SUB);
			if (!result.empty())
			{
				int ii = 0;
				for (const auto &sd : result)
				{
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
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}
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
			if ((targettypei == 0) && (targetvariable.empty()))
				return;
			if ((targettypei == 1) && ((targetdeviceid.empty()) || (targetproperty.empty())))
				return;
			if ((targettypei == 2) && (targetdeviceid.empty()))
				return;
			if (idx == "0") {
				m_sql.safe_query(
					"INSERT INTO PushLink (PushType, DeviceRowID,DelimitedValue,TargetType,TargetVariable,TargetDeviceID,TargetProperty,IncludeUnit,Enabled) VALUES ('%d','%d','%d','%d','%q','%d','%q','%d','%d')",
					CBasePush::PushType::PUSHTYPE_GOOGLE_PUB_SUB,
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
					"UPDATE PushLink SET DeviceRowID='%d', DelimitedValue=%d, TargetType=%d, TargetVariable='%q', TargetDeviceID=%d, TargetProperty='%q', IncludeUnit='%d', Enabled='%d' WHERE (ID == '%q')",
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
			m_googlepubsubpush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_GOOGLE_PUB_SUB);
			root["status"] = "OK";
			root["title"] = "SaveGooglePubSubLink";
		}

		void CWebServer::Cmd_DeleteGooglePubSubLink(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

			std::string idx = request::findValue(&req, "idx");
			if (idx.empty())
				return;
			m_sql.safe_query("DELETE FROM PushLink WHERE (ID=='%q')", idx.c_str());
			m_googlepubsubpush.ReloadPushLinks(CBasePush::PushType::PUSHTYPE_GOOGLE_PUB_SUB);
			root["status"] = "OK";
			root["title"] = "DeleteGooglePubSubLink";
		}
	} // namespace server
} // namespace http
