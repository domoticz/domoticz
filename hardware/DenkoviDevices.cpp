#include "stdafx.h"
#include "DenkoviDevices.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

#define MAX_POLL_INTERVAL 30*1000

#define DAE_DI_DEF		"DigitalInput"
#define DAE_AI_DEF		"AnalogInput"
#define DAE_TI_DEF		"TemperatureInput"
#define DAE_AO_DEF		"AnalogOutput"
#define DAE_PWM_DEF		"PWM"
#define DAE_OUT_DEF		"Output"
#define DAE_RELAY_DEF	"Relay"
#define DAE_CH_DEF		"Channel"

#define DAE_VALUE_DEF	"Value"
#define DAE_STATE_DEF	"State"
#define DAE_RSTATE_DEF	"RelayState"
#define DAE_MEASURE_DEF	"Measure"
#define DAE_NAME_DEF	"Name"

enum _eDenkoviIOType
{
	DIOType_DI = 0,		//0
	DIOType_DO,			//1
	DIOType_Relay,		//2
	DIOType_PWM,		//3
	DIOType_AO			//4
};

CDenkoviDevices::CDenkoviDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password, const int pollInterval, const int model) :
	m_szIPAddress(IPAddress),
	m_Password(CURLEncode::URLEncode(password)),
	m_pollInterval(pollInterval)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_stoprequested = false;
	m_bOutputLog = false;
	m_iModel = model;
	if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;
	Init();
}


CDenkoviDevices::~CDenkoviDevices()
{
}

void CDenkoviDevices::Init()
{
}

bool CDenkoviDevices::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CDenkoviDevices::Do_Work, this);
	m_bIsStarted = true;
	sOnConnected(this);
	switch (m_iModel) {
	case DDEV_DAEnet_IP4:
		_log.Log(LOG_STATUS, "DAEnet IP4: Started");
		break;
	case DDEV_SmartDEN_IP_16_Relays:
		_log.Log(LOG_STATUS, "SmartDEN IP 16 Relays: Started");
		break;
	case DDEV_SmartDEN_IP_32_In:
		_log.Log(LOG_STATUS, "SmartDEN IP 32 In: Started");
		break;
	case DDEV_SmartDEN_IP_Maxi:
		_log.Log(LOG_STATUS, "SmartDEN IP Maxi: Started");
		break;
	case DDEV_SmartDEN_IP_Watchdog:
		_log.Log(LOG_STATUS, "SmartDEN IP Watchdog: Started");
		break;
	case DDEV_SmartDEN_Logger:
		_log.Log(LOG_STATUS, "SmartDEN Logger: Started");
		break;
	case DDEV_SmartDEN_Notifier:
		_log.Log(LOG_STATUS, "SmartDEN Notifier: Started");
		break;
	}
	return (m_thread != nullptr);
}

bool CDenkoviDevices::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CDenkoviDevices::Do_Work()
{
	int poll_interval = m_pollInterval / 100;
	int poll_counter = poll_interval - 2;

	while (!m_stoprequested)
	{
		sleep_milliseconds(100);
		poll_counter++;

		if (poll_counter % 12 * 10 == 0) { //10 steps = 1 second (10 * 100)
			m_LastHeartbeat = mytime(NULL);
		}

		if (poll_counter % poll_interval == 0)
		{
			GetMeterDetails();
		}
	}
	switch (m_iModel) {
	case DDEV_DAEnet_IP4:
		_log.Log(LOG_STATUS, "DAEnet IP4: Worker stopped...");
		break;
	case DDEV_SmartDEN_IP_16_Relays:
		_log.Log(LOG_STATUS, "SmartDEN IP 16 Relays: Worker stopped...");
		break;
	case DDEV_SmartDEN_IP_32_In:
		_log.Log(LOG_STATUS, "SmartDEN IP 32 In: Worker stopped...");
		break;
	case DDEV_SmartDEN_IP_Maxi:
		_log.Log(LOG_STATUS, "SmartDEN IP Maxi: Worker stopped...");
		break;
	case DDEV_SmartDEN_IP_Watchdog:
		_log.Log(LOG_STATUS, "SmartDEN IP Watchdog: Worker stopped...");
		break;
	case DDEV_SmartDEN_Logger:
		_log.Log(LOG_STATUS, "SmartDEN Logger: Worker stopped...");
		break;
	case DDEV_SmartDEN_Notifier:
		_log.Log(LOG_STATUS, "SmartDEN Notifier: Worker stopped...");
		break;
	}
}

bool CDenkoviDevices::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const _tGeneralSwitch *pSen = reinterpret_cast<const _tGeneralSwitch*>(pdata);


	switch (m_iModel) {
	case DDEV_SmartDEN_Notifier: {
		_log.Log(LOG_ERROR, "SmartDEN Notifier: This board does not have outputs! ");
		return false;
	}
	case DDEV_SmartDEN_Logger: {
		_log.Log(LOG_ERROR, "SmartDEN Logger: This board does not have outputs! ");
		return false;
	}
	case DDEV_SmartDEN_IP_32_In: {
		_log.Log(LOG_ERROR, "SmartDEN IP 32 In: This board does not have outputs! ");
		return false;
	}
	case DDEV_SmartDEN_IP_Maxi: {
		int ioType = pSen->id;
		if ((ioType != DIOType_AO) && (ioType != DIOType_Relay))
		{
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Not a valid Relay or Analog Output!");
			return false;
		}
		int io = pSen->unitcode;//Relay1 to Relay8 and AO1 to AO2
		if (io > 8)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password << "&";
		else
			szURL << "?";

		if (ioType == DIOType_Relay)
			szURL << "Relay" << io << "=";
		else //DIOType_PWM:
			szURL << "AnalogOutput" << io << "=";

		if (pSen->cmnd == light2_sOff)
			szURL << "0";
		else if (pSen->cmnd == light2_sSetLevel) {
			double lvl = pSen->level*10.23;
			int iLvl = (int)lvl;
			szURL << std::to_string(iLvl);
		}
		else
			szURL << "1";
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Error sending command to: %s", m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos) {
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Error sending command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	case DDEV_DAEnet_IP4: {
		int ioType = pSen->id;
		if ((ioType != DIOType_DO) && (ioType != DIOType_PWM))
		{
			_log.Log(LOG_ERROR, "DAEnet IP4: Not a valid Digital or PWM output");
			return false;
		}
		int io = pSen->unitcode;//Output1 to Output16 and PWM1 to PWM2
		if (io > 16)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password << "&";
		else
			szURL << "?";

		if (ioType == DIOType_DO)
			szURL << "Output" << io << "=";
		else //DIOType_PWM:
			szURL << "PWM" << io << "=";

		if (pSen->cmnd == light2_sOff)
			szURL << "0";
		else if (pSen->cmnd == light2_sSetLevel)
			szURL << std::to_string(pSen->level);
		else
			szURL << "1";
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			_log.Log(LOG_ERROR, "DAEnet IP4: Error sending command to: %s", m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos) {
			_log.Log(LOG_ERROR, "DAEnet IP4: Error sending command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	case DDEV_SmartDEN_IP_Watchdog:
	case DDEV_SmartDEN_IP_16_Relays: {
		int ioType = pSen->id;
		if (ioType != DIOType_Relay)
		{
			if (m_iModel == DDEV_SmartDEN_IP_Watchdog)
				_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Not a valid Relay switch.");
			else
				_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Not a valid Relay switch.");
			return false;
		}
		int Relay = pSen->unitcode;
		if (Relay > 16)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password << "&Relay" << Relay << "=";
		else
			szURL << "?Relay" << Relay << "=";

		if (pSen->cmnd == light2_sOff)
			szURL << "0";
		else if (pSen->cmnd == light2_sOn) {
			szURL << "1";
		}
		else {
			if (m_iModel == DDEV_SmartDEN_IP_Watchdog)
				_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Not a valid command. Relay could be On or Off.");
			else
				_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Not a valid command. Relay could be On or Off.");
			return false;
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			if (m_iModel == DDEV_SmartDEN_IP_Watchdog)
				_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Error sending relay command to: %s", m_szIPAddress.c_str());
			else
				_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos)
		{
			if (m_iModel == DDEV_SmartDEN_IP_Watchdog)
				_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Error sending relay command to: %s", m_szIPAddress.c_str());
			else
				_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Error sending relay command to: %s", m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	}
	_log.Log(LOG_ERROR, "Denkovi: Unknown Device!");
	return false;
}

int CDenkoviDevices::DenkoviCheckForIO(std::string tmpstr, const std::string &tmpIoType) {
	int pos1;
	int Idx = -1;
	std::string ioType = "<" + tmpIoType;
	pos1 = tmpstr.find(ioType);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(ioType.c_str()));
		pos1 = tmpstr.find(">");
		if (pos1 != std::string::npos)
			Idx = atoi(tmpstr.substr(0, pos1).c_str());
	}
	return Idx;
}

int CDenkoviDevices::DenkoviGetIntParameter(std::string tmpstr, const std::string &tmpParameter) {
	int pos1;
	int lValue = -1;
	std::string parameter = "<" + tmpParameter + ">";
	pos1 = tmpstr.find(parameter);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(parameter.c_str()));
		pos1 = tmpstr.find('<');
		if (pos1 != std::string::npos)
			lValue = atoi(tmpstr.substr(0, pos1).c_str());
	}
	return lValue;
}

std::string CDenkoviDevices::DenkoviGetStrParameter(std::string tmpstr, const std::string &tmpParameter) {
	int pos1;
	std::string sMeasure = "";
	std::string parameter = "<" + tmpParameter + ">";

	pos1 = tmpstr.find(parameter);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(parameter.c_str()));
		pos1 = tmpstr.find('<');
		if (pos1 != std::string::npos)
			sMeasure = tmpstr.substr(0, pos1);
	}
	return sMeasure;
}

float CDenkoviDevices::DenkoviGetFloatParameter(std::string tmpstr, const std::string &tmpParameter) {
	int pos1;
	float value = NAN;
	std::string parameter = "<" + tmpParameter + ">";

	pos1 = tmpstr.find(parameter);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(parameter.c_str()));
		pos1 = tmpstr.find('<');
		if (pos1 != std::string::npos)
			value = static_cast<float>(atof(tmpstr.substr(0, pos1).c_str()));
	}
	return value;
}


void CDenkoviDevices::GetMeterDetails()
{
	std::string sResult;
#ifdef DEBUG_DenkoviInR
	sResult = ReadFile("E:\\DenkoviIPIn.xml");
#else
	std::stringstream szURL;

	szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
	if (!m_Password.empty())
	{
		szURL << "?pw=" << m_Password;
	}

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		switch (m_iModel) {
		case DDEV_DAEnet_IP4:
			_log.Log(LOG_ERROR, "DAEnet IP4: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_16_Relays:
			_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_32_In:
			_log.Log(LOG_ERROR, "SmartDEN IP 32 In: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_Maxi:
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_Watchdog:
			_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_Logger:
			_log.Log(LOG_ERROR, "SmartDEN Logger: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_Notifier:
			_log.Log(LOG_ERROR, "SmartDEN Notifier: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		}
		return;
	}
#endif
	std::vector<std::string> results;
	StringSplit(sResult, "\r\n", results);
	if (results.size() < 8)
	{
		switch (m_iModel) {
		case DDEV_DAEnet_IP4:
			_log.Log(LOG_ERROR, "DAEnet IP4: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_16_Relays:
			_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_32_In:
			_log.Log(LOG_ERROR, "SmartDEN IP 32 In: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_Maxi:
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_IP_Watchdog:
			_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_Logger:
			_log.Log(LOG_ERROR, "SmartDEN Logger: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		case DDEV_SmartDEN_Notifier:
			_log.Log(LOG_ERROR, "SmartDEN Notifier: Error connecting to: %s", m_szIPAddress.c_str());
			break;
		}
		return;
	}
	if (results[0] != "<CurrentState>")
	{
		switch (m_iModel) {
		case DDEV_DAEnet_IP4:
			_log.Log(LOG_ERROR, "DAEnet IP4: Error getting status");
			break;
		case DDEV_SmartDEN_IP_16_Relays:
			_log.Log(LOG_ERROR, "SmartDEN IP 16 Relays: Error getting status");
			break;
		case DDEV_SmartDEN_IP_32_In:
			_log.Log(LOG_ERROR, "SmartDEN IP 32 In: Error getting status");
		case DDEV_SmartDEN_IP_Maxi:
			_log.Log(LOG_ERROR, "SmartDEN IP Maxi: Error getting status");
			break;
		case DDEV_SmartDEN_IP_Watchdog:
			_log.Log(LOG_ERROR, "SmartDEN IP Watchdog: Error getting status");
		case DDEV_SmartDEN_Logger:
			_log.Log(LOG_ERROR, "SmartDEN Logger: Error getting status");
			break;
		case DDEV_SmartDEN_Notifier:
			_log.Log(LOG_ERROR, "SmartDEN Notifier: Error getting status");
			break;
		}
		return;
	}
	size_t ii;
	std::string tmpstr;
	int tmpState = 0;
	int tmpValue = 0;
	float tmpTiValue = NAN;
	std::string tmpMeasure;
	std::string tmpName;
	std::string name;
	int Idx = -1;

	switch (m_iModel) {
	case DDEV_SmartDEN_IP_16_Relays: {//has only relays
		bool bHaveRelays = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (!bHaveRelays && ((Idx = DenkoviCheckForIO(tmpstr, DAE_RELAY_DEF)) != -1))
			{
				bHaveRelays = true;
				continue;
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveRelays && (Idx != -1) && ((tmpState = DenkoviGetIntParameter(tmpstr, DAE_STATE_DEF)) != -1))
			{
				std::stringstream sstr;
				sstr << "Relay " << Idx << " (" << name << ")";
				SendGeneralSwitch(DIOType_Relay, Idx, 255, (tmpState == 1) ? true : false, 100, sstr.str());
				Idx = -1;
				bHaveRelays = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_IP_Watchdog: {//has only relays
		bool bHaveRelays = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (!bHaveRelays && ((Idx = DenkoviCheckForIO(tmpstr, DAE_CH_DEF)) != -1))
			{
				bHaveRelays = true;
				continue;
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveRelays && (Idx != -1) && ((tmpState = DenkoviGetIntParameter(tmpstr, DAE_RSTATE_DEF)) != -1))
			{
				name = "Relay " + std::to_string(Idx) + "(" + name + ")";
				SendGeneralSwitch(DIOType_Relay, Idx, 255, (tmpState == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveRelays = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_Notifier:
	case DDEV_SmartDEN_Logger: {
		bool bHaveDigitalInput = false;
		bool bHaveAnalogInput = false;
		bool bHaveTemperatureInput = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (Idx == -1) {
				if (!bHaveDigitalInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_DI_DEF)) != -1))
				{
					bHaveDigitalInput = true;
					continue;
				}

				if (!bHaveAnalogInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_AI_DEF)) != -1))
				{
					bHaveAnalogInput = true;
					continue;
				}

				if (!bHaveTemperatureInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_TI_DEF)) != -1))
				{
					bHaveTemperatureInput = true;
					continue;
				}
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_DI, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
				SendCustomSensor(Idx, 1, 255, static_cast<float>(tmpValue), name, "");
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveTemperatureInput && (Idx != -1) && ((tmpTiValue = DenkoviGetFloatParameter(tmpstr, DAE_VALUE_DEF)) != NAN))
			{
				name = "Temperature Input " + std::to_string(Idx) + " (" + name + ")";
				SendTempSensor(Idx, 255, tmpTiValue, name);
				Idx = -1;
				bHaveTemperatureInput = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_IP_32_In: {
		bool bHaveDigitalInput = false;
		bool bHaveAnalogInput = false;
		bool bHaveTemperatureInput = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (Idx == -1) {
				if (!bHaveDigitalInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_DI_DEF)) != -1))
				{
					bHaveDigitalInput = true;
					continue;
				}

				if (!bHaveAnalogInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_AI_DEF)) != -1))
				{
					bHaveAnalogInput = true;
					continue;
				}

				if (!bHaveTemperatureInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_TI_DEF)) != -1))
				{
					bHaveTemperatureInput = true;
					continue;
				}
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_DI, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && ((tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)) != ""))
			{
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				if (vMeasure.size() == 2)
				{
					name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
					SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), name, vMeasure[1]);
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveTemperatureInput && (Idx != -1) && ((tmpTiValue = DenkoviGetFloatParameter(tmpstr, DAE_VALUE_DEF)) != NAN))
			{
				name = "Temperature Input " + std::to_string(Idx) + " (" + name + ")";
				SendTempSensor(Idx, 255, tmpTiValue, name);
				Idx = -1;
				bHaveTemperatureInput = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_IP_Maxi: {//has DI, AO, AI, Relays
		bool bHaveDigitalInput = false;
		bool bHaveAnalogOutput = false;
		bool bHaveAnalogInput = false;
		bool bHaveRelay = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (Idx == -1) {
				if (!bHaveDigitalInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_DI_DEF)) != -1))
				{
					bHaveDigitalInput = true;
					continue;
				}

				if (!bHaveAnalogInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_AI_DEF)) != -1))
				{
					bHaveAnalogInput = true;
					continue;
				}

				if (!bHaveAnalogOutput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_AO_DEF)) != -1))
				{
					bHaveAnalogOutput = true;
					continue;
				}

				if (!bHaveRelay && ((Idx = DenkoviCheckForIO(tmpstr, DAE_RELAY_DEF)) != -1))
				{
					bHaveRelay = true;
					continue;
				}
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_DI, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && ((tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)) != ""))
			{
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				if (vMeasure.size() == 2)
				{
					name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
					SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), name, vMeasure[1]);
				}
				else {
					name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
					SendTempSensor(Idx, 255, tmpTiValue, name);
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveRelay && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Relay " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_Relay, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveRelay = false;
				continue;
			}

			if (bHaveAnalogOutput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Analog Output " + std::to_string(Idx) + " (" + name + ")";
				double val = (100 / 1023) * tmpValue;
				SendGeneralSwitch(DIOType_AO, Idx, 255, (tmpValue > 0) ? true : false, (uint8_t)val, name);
				Idx = -1;
				bHaveAnalogOutput = false;
				continue;
			}
		}
		break;
	}
	case DDEV_DAEnet_IP4: {//has DI, DO, AI, PWM
		bool bHaveDigitalInput = false;
		bool bHaveDigitalOutput = false;
		bool bHaveAnalogInput = false;
		bool bHavePWM = false;
		for (ii = 1; ii < results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (Idx == -1) {
				if (!bHaveDigitalInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_DI_DEF)) != -1))
				{
					bHaveDigitalInput = true;
					continue;
				}

				if (!bHaveAnalogInput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_AI_DEF)) != -1))
				{
					bHaveAnalogInput = true;
					continue;
				}

				if (!bHaveDigitalOutput && ((Idx = DenkoviCheckForIO(tmpstr, DAE_OUT_DEF)) != -1))
				{
					bHaveDigitalOutput = true;
					continue;
				}

				if (!bHavePWM && ((Idx = DenkoviCheckForIO(tmpstr, DAE_PWM_DEF)) != -1))
				{
					bHavePWM = true;
					continue;
				}
			}

			if ((tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)) != "") {
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_DI, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && ((tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)) != ""))
			{
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				if (vMeasure.size() == 2)
				{
					name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
					SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), name, vMeasure[1]);
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveDigitalOutput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Output " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_DO, Idx, 255, (tmpValue == 1) ? true : false, 100, name);
				Idx = -1;
				bHaveDigitalOutput = false;
				continue;
			}

			if (bHavePWM && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "PWM " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_PWM, Idx, 255, (tmpValue > 0) ? true : false, (uint8_t)tmpValue, name);
				Idx = -1;
				bHavePWM = false;
				continue;
			}
		}
		break;
	}
	}
}
