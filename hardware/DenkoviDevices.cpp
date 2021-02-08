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

#define MAX_POLL_INTERVAL 3600*1000

#define DAE_DI_DEF		"DigitalInput"
#define DAE_AI_DEF		"AnalogInput"
#define DAE_TI_DEF		"TemperatureInput"
#define DAE_AO_DEF		"AnalogOutput"
#define DAE_PWM_DEF		"PWM"
#define DAE_OUT_DEF		"Output"
#define DAE_RELAY_DEF	"Relay"
#define DAE_CH_DEF		"Channel"


#define SDO_AI_DEF		"AI"
#define SDO_DI_DEF		"DI"
#define SDO_TI_DEF		"TI"
#define SDO_RELAY_DEF	"Relay"
#define SDO_MAIN_DEF	"Main"

#define DAENETIP3_PORTA_MDO_DEF			"ASG"//portA multiple output state
#define DAENETIP3_PORTA_SDO_DEF			"AS"//portA single output state
#define DAENETIP3_PORTA_SNAME_DEF		"AC"//portA signle output name

#define DAENETIP3_PORTB_MDI_DEF			"BVG"//portB multiple input state
#define DAENETIP3_PORTB_SDI_DEF			"BV"//portB single output state
#define DAENETIP3_PORTB_SNAME_DEF		"BC"//portB signle input name

#define DAENETIP3_PORTC_SNAME_DEF		"CC"//portC signle analog input name
#define DAENETIP3_PORTC_SDIM_DEF		"CS"//portC signle analog input dimension
#define DAENETIP3_PORTC_SVAL_DEF		"CD"//portC signle analog input value + dimension
 

#define MIN_DAENETIP3_RESPOND_LENGTH	105

#define DAE_VALUE_DEF		"Value"
#define DAE_STATE_DEF		"State"
#define DAE_RSTATE_DEF		"RelayState"
#define DAE_MEASURE_DEF		"Measure"
#define DAE_SCAL_VALUE_DEF	"Scaled_Value"
#define DAE_NAME_DEF		"Name"
#define DAE_LASTO_DEF		"LastOpened"
#define DAE_LASTC_DEF		"LastClosed"
#define DAE_COUNT_DEF		"Count"

#define DAENETIP3_AI_VALUE				0
#define DAENETIP3_AI_DIMENSION			1
#define DAENETIP2_PORT_3_VAL			0
#define DAENETIP2_PORT_5_VAL			1
#define DAENETIP2_AI_VOLTAGE			0
#define DAENETIP2_AI_TEMPERATURE		1

enum _eDenkoviIOType
{
	DIOType_DI = 0,		//0
	DIOType_DO,			//1
	DIOType_Relay,		//2
	DIOType_PWM,		//3
	DIOType_AO,			//4
	DIOType_AI,			//5
	DIOType_DIO,		//6
	DIOType_MCD,		//7
	DIOType_TXT			//8
};

namespace
{
	constexpr std::array<const char *, 14> szDenkoviHardwareNames{
		"DAEnetIP4",					    //
		"smartDEN IP-16R",				    //
		"smartDEN IP-32IN",				    //
		"smartDEN IP-Maxi",				    //
		"smartDEN IP-Watchdog",				    //
		"smartDEN Logger",				    //
		"smartDEN Notifier",				    //
		"DAEnetIP3",					    //
		"DAEnetIP2 (DAEnetIP2 v2)",			    //
		"DAEnetIP2 (DAEnetIP2 v2) 8 Relay Module - LM35DZ", //
		"smartDEN Opener",				    //
		"smartDEN IP-PLC",				    //
		"smartDEN IP-16R-MT",				    //
		"smartDEN IP-16R-MQ",				    //
	};
} // namespace

CDenkoviDevices::CDenkoviDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &password, const int pollInterval, const int model) :
	m_szIPAddress(IPAddress),
	m_Password(CURLEncode::URLEncode(password)),
	m_pollInterval(pollInterval)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_bOutputLog = false;
	m_iModel = model;
	if (m_pollInterval < 500)
		m_pollInterval = 500;
	else if (m_pollInterval > MAX_POLL_INTERVAL)
		m_pollInterval = MAX_POLL_INTERVAL;
	Init();
}

void CDenkoviDevices::Init()
{
}

bool CDenkoviDevices::StartHardware()
{
	RequestStart();

	Init();

	// Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	Log(LOG_STATUS, "%s: Started.",szDenkoviHardwareNames[m_iModel]);
	return (m_thread != nullptr);
}

bool CDenkoviDevices::StopHardware()
{
	if (m_thread != nullptr)
	{
		RequestStop();
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

	while (!IsStopRequested(100))
	{
		poll_counter++;

		if (poll_counter % 12 * 10 == 0) { //10 steps = 1 second (10 * 100)
			m_LastHeartbeat = mytime(nullptr);
		}

		if (poll_counter % poll_interval == 0)
		{
			GetMeterDetails();
		}
	}
	Log(LOG_STATUS, "%s: Stopped.",szDenkoviHardwareNames[m_iModel]);	 
}

bool CDenkoviDevices::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	const _tGeneralSwitch *pSen1 = reinterpret_cast<const _tGeneralSwitch*>(pdata);
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);

	int ioType = pSen->LIGHTING2.id4;
	int io;
	int Relay;
	uint8_t command;
	uint8_t level;

	if (ioType!=DIOType_DO && ioType!=DIOType_Relay && ioType!= DIOType_DIO && ioType!= DIOType_MCD)
	{
		ioType = pSen1->id;
		io = pSen1->unitcode;
		Relay = pSen1->unitcode;
		command = pSen1->cmnd;
		level = pSen1->level;
	}
	else
	{
		ioType = pSen->LIGHTING2.id4;
		io = pSen->LIGHTING2.unitcode;
		Relay = pSen->LIGHTING2.unitcode;
		command = pSen->LIGHTING2.cmnd;
		level = pSen->LIGHTING2.level;
	}

	if (m_Password.empty())
	{
		Log(LOG_STATUS, "%s: Please enter a password!",szDenkoviHardwareNames[m_iModel]); 
		return false;
	}

	switch (m_iModel) {
	case DDEV_SmartDEN_Opener: {
		
		if ((ioType != DIOType_MCD) && (ioType != DIOType_Relay))
		{
			Log(LOG_ERROR, "%s: Not a valid Relay or Main Controlled Device!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//Relay1, Relay2 and MCD
		if (io > 3)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password << "&";
		else
			szURL << "?";

		if (ioType == DIOType_Relay)
			szURL << "R" << io << "=";
		else //DIOType_PWM:
			szURL << "MS=";

		if (ioType == DIOType_Relay) {
			if (command== light2_sOff)
				szURL << "0";
			else if (command== light2_sOn)
				szURL << "1";
			else {
				Log(LOG_ERROR, "%s: Not a valid command for Relay!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}
		else if (ioType == DIOType_MCD) {
			if (command== blinds_sOpen)
				szURL << "1";
			else if (command== blinds_sClose)
				szURL << "0";
			else if (command== 0x11) //stop button
				szURL << "2";
			else {
				Log(LOG_ERROR, "%s: Not a valid command for MCD!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}

		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("Message") != std::string::npos) {
			Log(LOG_ERROR, "%s: Error sending command to: %s",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	case DDEV_DAEnet_IP2_8_RELAYS: {
		std::string sPass = m_Password;
		sPass.replace(sPass.find("%3A"), 3, ":");
		//int ioType = pSen->id;
		if (ioType != DIOType_DIO && ioType != DIOType_Relay)
		{
			Log(LOG_ERROR, "%s: Not a valid Digital Input/Output or Relay!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//DIO1 to DIO16
		if (io > 16)
			return false;

		std::stringstream szURL;
		std::string sResult;

		szURL << "http://" << sPass << "@" << m_szIPAddress << ":" << m_usIPPort << "/ioreg.js";
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s", m_szIPAddress.c_str(),szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		uint8_t port3 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_3_VAL);
		uint8_t port5 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_5_VAL);
		if (io < 9) { //DIO1 to DIO8 are from Port 3
			if (command== light2_sOff)
				port3 = port3 & (~(0x01 << (io - 1)));
			else if (command== light2_sOn)
				port3 = port3 | (0x01 << (io - 1));
			else {
				Log(LOG_ERROR, "%s: Not a valid command. Digital Input/Output could be On or Off!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}
		else { //DIO9 to DIO16 are from Port 5
			if (command== light2_sOff)
				port5 = port5 & (~(0x01 << (io - 8 - 1)));
			else if (command== light2_sOn)
				port5 = port5 | (0x01 << (io - 8 - 1));
			else {
				Log(LOG_ERROR, "%s: Not a valid command. Digital Input/Output could be On or Off!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}
		szURL.str("");
		char port3Val[3], port5Val[3];
		sprintf(port3Val, "%02X", port3);
		sprintf(port5Val, "%02X", port5);
		szURL << "http://" << sPass << "@" << m_szIPAddress << ":" << m_usIPPort << "/iochange.cgi?ref=re-io&01=" << port3Val << "&02=" << port5Val;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}		
		return true;
	}
	case DDEV_DAEnet_IP2: {
		std::string sPass = m_Password;
		sPass.replace(sPass.find("%3A"), 3, ":");
		//int ioType = pSen->id;
		if (ioType != DIOType_DIO)
		{
			Log(LOG_ERROR, "%s: Not a valid Digital Input/Output!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//DIO1 to DIO16
		if (io > 16)
			return false;

		std::stringstream szURL;
		std::string sResult;

		szURL << "http://" << sPass << "@" << m_szIPAddress << ":" << m_usIPPort << "/ioreg.js";
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		uint8_t port3 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_3_VAL);
		uint8_t port5 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_5_VAL);
		if (io < 9) { //DIO1 to DIO8 are from Port 3
			if (command== light2_sOff)
				port3 = port3 & (~(0x01 << (io - 1)));
			else if (command== light2_sOn)
				port3 = port3 | (0x01 << (io - 1));
			else {
				Log(LOG_ERROR, "%s: Not a valid command. Digital Input/Output must be On or Off!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}
		else { //DIO9 to DIO16 are from Port 5
			if (command== light2_sOff)
				port5 = port5 & (~(0x01 << (io - 8 - 1)));
			else if (command== light2_sOn)
				port5 = port5 | (0x01 << (io - 8 - 1));
			else {
				Log(LOG_ERROR, "%s: Not a valid command. Digital Input/Output must be On or Off!",szDenkoviHardwareNames[m_iModel]);
				return false;
			}
		}
		szURL.str("");
		char port3Val[3], port5Val[3];
		sprintf(port3Val, "%02X", port3);
		sprintf(port5Val, "%02X", port5);
		szURL << "http://" << sPass << "@" << m_szIPAddress << ":" << m_usIPPort << "/iochange.cgi?ref=re-io&01=" << port3Val << "&02=" << port5Val;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}		
		return true;
	}
	case DDEV_DAEnet_IP3: {
		//int ioType = pSen->id;
		if (ioType != DIOType_DO)
		{
			Log(LOG_ERROR, "%s: Not a valid Digital Output!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//DO1 to DO16
		if (io > 16)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/command.html";
		if (!m_Password.empty())
			szURL << "?P=" << m_Password << "&";
		else
			szURL << "?";

		szURL << DAENETIP3_PORTA_SDO_DEF << std::uppercase << std::hex << io - 1 << "=";
		if (command== light2_sOff)
			szURL << "0&";
		else if (command== light2_sOn) {
			szURL << "1&";
		}
		else {
			Log(LOG_ERROR, "%s: Not a valid command. Digital Output could be On or Off!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		szURL.str("");
		szURL << DAENETIP3_PORTA_SDO_DEF << std::uppercase << std::hex << io - 1;
		if (sResult.find(szURL.str()) == std::string::npos) {
			Log(LOG_ERROR, "%s: Error setting Digital Output %u!",szDenkoviHardwareNames[m_iModel], io);
			return false;
		}
		return true;

	}
	case DDEV_SmartDEN_Notifier:
	case DDEV_SmartDEN_Logger:	 
	case DDEV_SmartDEN_IP_32_In: {
		Log(LOG_ERROR, "%s: This module does not have outputs!",szDenkoviHardwareNames[m_iModel]);
		return false;
	}
	case DDEV_SmartDEN_PLC:
	case DDEV_SmartDEN_IP_Maxi: {
		//int ioType = pSen1->LIGHTING2.unitcode;//pSen->id;
		if ((ioType != DIOType_AO) && (ioType != DIOType_Relay))
		{
			Log(LOG_ERROR, "%s: Not a valid Relay or Analog Output!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen1->LIGHTING2.id4;//Relay1 to Relay8 and AO1 to AO2
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

		if (command == light2_sOff)//pSen->cmnd
			szURL << "0";
		else if (command== light2_sSetLevel) {
			double lvl = level*10.34;
			int iLvl = (int)lvl;
			szURL << std::to_string(iLvl);
		}
		else
			szURL << "1";
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s Error sending command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos) {
			Log(LOG_ERROR, "%s: Error sending command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	case DDEV_DAEnet_IP4: {
		//int ioType = pSen->id;
		if ((ioType != DIOType_DO) && (ioType != DIOType_PWM))
		{
			Log(LOG_ERROR, "%s: Not a valid Digital or PWM output!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		//int io = pSen->unitcode;//Output1 to Output16 and PWM1 to PWM2
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

		if (command== light2_sOff)
			szURL << "0";
		else if (command== light2_sSetLevel)
			szURL << std::to_string(level);
		else
			szURL << "1";
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult)) {
			Log(LOG_ERROR, "%s: Error sending command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos) {
			Log(LOG_ERROR, "%s: Error sending command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		return true;  
	}
	case DDEV_SmartDEN_IP_16_R_MQ:
	case DDEV_SmartDEN_IP_16_R_MT:	
	case DDEV_SmartDEN_IP_Watchdog:
	case DDEV_SmartDEN_IP_16_Relays: {
  
		if (ioType != DIOType_Relay)
		{
			Log(LOG_ERROR, "%s: Not a valid Relay switch!",szDenkoviHardwareNames[m_iModel]);			 
			return false;
		}
		//int Relay = pSen->unitcode;
		if (Relay > 16)
			return false;

		std::stringstream szURL;

		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password << "&Relay" << Relay << "=";
		else
			szURL << "?Relay" << Relay << "=";

		if (command== light2_sOff)
			szURL << "0";
		else if (command== light2_sOn) {
			szURL << "1";
		}
		else {
			Log(LOG_ERROR, "%s: Not a valid command. Relay must be On or Off!",szDenkoviHardwareNames[m_iModel]);
			return false;
		}
		std::string sResult;
		if (!HTTPClient::GET(szURL.str(), sResult))
		{
			Log(LOG_ERROR, "%s: Error sending relay command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());			 
			return false;
		}
		if (sResult.find("CurrentState") == std::string::npos)
		{
			Log(LOG_ERROR, "%s: Error sending relay command to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str());
			return false;
		}
		return true;
	}
	}
	Log(LOG_ERROR, "Unknown Device!");
	return false;
}

int CDenkoviDevices::DenkoviCheckForIO(std::string tmpstr, const std::string &tmpIoType) {
	int Idx = -1;
	std::string ioType = "<" + tmpIoType;
	size_t pos1 = tmpstr.find(ioType);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(ioType.c_str()));
		pos1 = tmpstr.find('>');
		if (pos1 != std::string::npos)
			Idx = atoi(tmpstr.substr(0, pos1).c_str());
	}
	return Idx;
}

int CDenkoviDevices::DenkoviGetIntParameter(std::string tmpstr, const std::string &tmpParameter) {
	int lValue = -1;
	std::string parameter = "<" + tmpParameter + ">";
	size_t pos1 = tmpstr.find(parameter);
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
	std::string sMeasure;
	std::string parameter = "<" + tmpParameter + ">";

	size_t pos1 = tmpstr.find(parameter);
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
	float value = NAN;
	std::string parameter = "<" + tmpParameter + ">";

	size_t pos1 = tmpstr.find(parameter);
	if (pos1 != std::string::npos)
	{
		tmpstr = tmpstr.substr(pos1 + strlen(parameter.c_str()));
		pos1 = tmpstr.find('<');
		if (pos1 != std::string::npos)
			value = static_cast<float>(atof(tmpstr.substr(0, pos1).c_str()));
	}
	return value;
}

std::string CDenkoviDevices::DAEnetIP3GetIo(const std::string &tmpstr, const std::string &tmpParameter)
{
	std::string parameter = tmpParameter + "=";
	size_t pos1 = tmpstr.find(parameter);
	size_t pos2 = tmpstr.find(';', pos1);
	return tmpstr.substr(pos1 + strlen(parameter.c_str()), pos2 - (pos1 + strlen(parameter.c_str())));
}

std::string CDenkoviDevices::DAEnetIP3GetAi(const std::string &tmpstr, const std::string &tmpParameter, const int &ciType)
{
	std::string parameter = tmpParameter + "=";
	size_t pos1 = tmpstr.find(parameter);
	size_t pos2;
	if (ciType == DAENETIP3_AI_VALUE) {
		pos2 = tmpstr.find('[', pos1);
		return tmpstr.substr(pos1 + strlen(parameter.c_str()), pos2 - (pos1 + strlen(parameter.c_str())));
	}
	if (ciType == DAENETIP3_AI_DIMENSION)
	{
		pos1 = tmpstr.find('[', pos1);
		pos2 = tmpstr.find(']', pos1);
		return tmpstr.substr(pos1 + 1, pos2 - (pos1 + 1));
	}
	return "";// tmpstr.substr(pos1 + strlen(parameter.c_str()), pos2 - (pos1 + strlen(parameter.c_str()))).c_str();
}

uint8_t CDenkoviDevices::DAEnetIP2GetIoPort(const std::string &tmpstr, const int &port)
{
	std::stringstream ss;
	size_t pos1, pos2;
	int b;
	if (port == DAENETIP2_PORT_3_VAL) {
		pos1 = tmpstr.find("(0x");
		pos2 = tmpstr.find(',', pos1);
		ss << std::hex << tmpstr.substr(pos1 + 3, pos2 - (pos1 + 3)).c_str();
		ss >> b;
		return (uint8_t)b;
	}
	if (port == DAENETIP2_PORT_5_VAL)
	{
		pos1 = tmpstr.find(",0x");
		pos2 = tmpstr.find(',', pos1 + 3);
		ss << std::hex << tmpstr.substr(pos1 + 3, pos2 - (pos1 + 3)).c_str();
		ss >> b;
		return (uint8_t)b;
	}
	return 0;
}

std::string CDenkoviDevices::DAEnetIP2GetName(const std::string &tmpstr, const int &nmr)
{ // nmr should be from 1 to 24
	size_t pos1 = 0, pos2 = 0;
	for (int ii = 0; ii < (((nmr - 1) * 2) + 1); ii++)
	{
		pos1 = tmpstr.find('"', pos1 + 1);
	}
	pos2 = tmpstr.find('"', pos1 + 1);
	return tmpstr.substr(pos1 + 1, pos2 - (pos1 + 1));
}

uint16_t CDenkoviDevices::DAEnetIP2GetAiValue(const std::string &tmpstr, const int &aiNmr)
{
	size_t pos1 = 0, pos2 = 0;
	std::stringstream ss;
	int b = 0;
	for (uint8_t ii = 0; ii < aiNmr + 3; ii++) {
		pos1 = tmpstr.find("0x", pos1 + 2);
	}
	pos2 = tmpstr.find(',', pos1 + 2);
	ss << std::hex << tmpstr.substr(pos1 + 2, pos2 - (pos1 + 2)).c_str();
	ss >> b;
	return (uint16_t)b;
}

float CDenkoviDevices::DAEnetIP2CalculateAi(int adc, const int &valType) {
	if (valType == DAENETIP2_AI_TEMPERATURE) {
		return static_cast<float>(10000 * ((1.2*204.8)*adc / (120 * 1024) + 0) / 100);
	}
	if (valType == DAENETIP2_AI_VOLTAGE)
	{
		return static_cast<float>(10000 * ((1.2*0.377)*adc / (4.7 * 1024) + 0) / 100);
	}
	return 0.0F;
}

void CDenkoviDevices::SendDenkoviTextSensor(const int NodeID, const int ChildID, const int BatteryLevel, const std::string &textMessage, const std::string &defaultname)
{
	//check if we have a change, if not do not update it
	bool bExists;
	std::string oldvalue = GetTextSensorText(NodeID, ChildID, bExists);
	if (oldvalue != textMessage)
		SendTextSensor(NodeID, ChildID, BatteryLevel, textMessage, defaultname);
}



void CDenkoviDevices::GetMeterDetails()
{
	std::string sResult, sResult2;
	std::stringstream szURL, szURL2;

	if (m_Password.empty())
	{
		Log(LOG_STATUS, "%s: Please enter a password!",szDenkoviHardwareNames[m_iModel]);		 
		return;
	}

	if (m_iModel == DDEV_DAEnet_IP2 || m_iModel == DDEV_DAEnet_IP2_8_RELAYS) {
		std::string sPass = m_Password;
		if (sPass.find("%3A") == std::string::npos) {
			if (m_iModel == DDEV_DAEnet_IP2)
				Log(LOG_ERROR, "%s: Please enter username and password in format username:password. Example admin:admin!",szDenkoviHardwareNames[m_iModel]);
			else if (m_iModel == DDEV_DAEnet_IP2_8_RELAYS)
				Log(LOG_ERROR, "%s: Please enter username and password in format username:password. Example admin:admin!",szDenkoviHardwareNames[m_iModel]);
			return;
		}
		sPass.replace(sPass.find("%3A"), 3, ":");
		szURL << "http://" << sPass << "@" << m_szIPAddress << ":" << m_usIPPort << "/ioreg.js";
	}
	else if (m_iModel == DDEV_DAEnet_IP3) {
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/command.html";
		szURL2 << "http://" << m_szIPAddress << ":" << m_usIPPort << "/command.html";
		if (!m_Password.empty()) {
			szURL << "?P=" << m_Password;
			szURL2 << "?P=" << m_Password;
		}
		szURL << "&ASG=?&BVG=?&CD0=?&CD1=?&CD2=?&CD3=?&CD4=?&CD5=?&CD6=?&CD7=?&AC0=?&AC1=?&AC2=?&AC3=?&AC4=?&AC5=?&AC6=?&AC7=?&AC8=?&AC9=?&ACA=?&ACB=?&ACC=?&ACD=?&ACE=?&ACF=?&";
		szURL2 << "&BC0=?&BC1=?&BC2=?&BC3=?&BC4=?&BC5=?&BC6=?&BC7=?&CC0=?&CC1=?&CC2=?&CC3=?&CC4=?&CC5=?&CC6=?&CC7=?&";
	}
	else {
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/current_state.xml";
		if (!m_Password.empty())
			szURL << "?pw=" << m_Password;
	}

	if (!HTTPClient::GET(szURL.str(), sResult) || (m_iModel == DDEV_DAEnet_IP3 && !HTTPClient::GET(szURL2.str(), sResult2)))
	{
		Log(LOG_ERROR, "%s: Error connecting to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}
	std::vector<std::string> results;
	if (m_iModel == DDEV_SmartDEN_Opener)
		StringSplit(sResult, "\n", results);
	else
		StringSplit(sResult, "\r\n", results);
	//int z = strlen(sResult.c_str());
	if (m_iModel != DDEV_DAEnet_IP3 && m_iModel != DDEV_DAEnet_IP2 && m_iModel != DDEV_DAEnet_IP2_8_RELAYS && results.size() < 8)
	{
		Log(LOG_ERROR, "%s: Error connecting to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 		 
		return;
	}
	if (m_iModel == DDEV_DAEnet_IP3 && (strlen(sResult.c_str()) < MIN_DAENETIP3_RESPOND_LENGTH))
	{
		Log(LOG_ERROR, "%s: Error connecting to: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}
	if (m_iModel == DDEV_DAEnet_IP2 && sResult.find("var IO=new Array") == std::string::npos)
	{
		Log(LOG_ERROR, "%s: Error getting status from: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}
	if (m_iModel == DDEV_DAEnet_IP2_8_RELAYS && sResult.find("var IO=new Array") == std::string::npos)
	{
		Log(LOG_ERROR, "%s: Error getting status from: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}

	if (m_iModel != DDEV_DAEnet_IP3 && m_iModel != DDEV_DAEnet_IP2 && m_iModel != DDEV_DAEnet_IP2_8_RELAYS && results[0] != "<CurrentState>")
	{
		Log(LOG_ERROR, "%s: Error getting status from: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}
	if (m_iModel == DDEV_DAEnet_IP3 && ((sResult.find(DAENETIP3_PORTA_MDO_DEF) == std::string::npos) || (sResult2.find(DAENETIP3_PORTB_SNAME_DEF) == std::string::npos)))
	{
		Log(LOG_ERROR, "%s: Error getting status from: %s!",szDenkoviHardwareNames[m_iModel], m_szIPAddress.c_str()); 
		return;
	}

	int ii;
	std::string tmpstr;
	std::string tmp_counter;
	std::string tmp_adc_raw_value; 
	int tmpState = 0;
	int tmpValue = 0;
	float tmpTiValue = NAN;
	std::string tmpMeasure;
 
	std::string tmpName;
	std::string name;
	int Idx = -1;

	switch (m_iModel) {
	case DDEV_SmartDEN_Opener: {//has DI, AI, TI, Relays, MCD
		bool bHaveDigitalInput = false;
		bool bHaveMCD = false;
		bool bHaveAnalogInput = false;
		bool bHaveTemperatureInput = false;
		bool bHaveRelay = false;
		for (ii = 1; ii < (int)results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (Idx == -1) {
				if (!bHaveDigitalInput && ((Idx = DenkoviCheckForIO(tmpstr, SDO_DI_DEF)) != -1))
				{
					bHaveDigitalInput = true;
					continue;
				}

				if (!bHaveAnalogInput && ((Idx = DenkoviCheckForIO(tmpstr, SDO_AI_DEF)) != -1))
				{
					bHaveAnalogInput = true;
					continue;
				}

				if (!bHaveMCD && ((Idx = DenkoviCheckForIO(tmpstr, SDO_MAIN_DEF)) != -1))
				{
					bHaveMCD = true;
					continue;
				}

				if (!bHaveTemperatureInput && ((Idx = DenkoviCheckForIO(tmpstr, SDO_TI_DEF)) != -1))
				{
					bHaveTemperatureInput = true;
					continue;
				}

				if (!bHaveRelay && ((Idx = DenkoviCheckForIO(tmpstr, SDO_RELAY_DEF)) != -1))
				{
					bHaveRelay = true;
					continue;
				}
			}

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_STATE_DEF)) != -1))
			{
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")"; 
				SendSwitch(DIOType_DI, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, name, m_Name);
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_SCAL_VALUE_DEF)).empty()))
			{
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure); 
				int len = tmpMeasure.length() - tmpMeasure.find_first_of('.', 0) - 2;
				std::string units = tmpMeasure.substr(tmpMeasure.find_first_of('.',0)+2, len);
				SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), "Analog Input Scaled (" + name + ")", units);

				std::vector<std::string> vRaw;
				tmpstr = results[ii - 1];
				std::string tmpRawValue = DenkoviGetStrParameter(tmpstr, DAE_VALUE_DEF);
				StringSplit(tmpRawValue, " ", vRaw);
				SendCustomSensor(Idx+1, 1, 255, static_cast<float>(atof(vRaw[0].c_str())), "Analog Input (" + name + ")", "");

				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveTemperatureInput && (Idx != -1) && (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_VALUE_DEF)).empty()))
			{
				name = "Temperature Input (" + name + ")";
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				tmpTiValue = static_cast<float>(atof(tmpMeasure.c_str()));
				if (tmpMeasure.find('F') != std::string::npos)
					tmpTiValue = (float)ConvertToCelsius((double)tmpTiValue);
				SendTempSensor(Idx, 255, tmpTiValue, name);

				Idx = -1;
				bHaveTemperatureInput = false;
				continue;
			}

			if (bHaveRelay && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_STATE_DEF)) != -1))
			{
				name = "Relay " + std::to_string(Idx) + " (" + name + ")";
				SendSwitch(DIOType_Relay, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, name, m_Name);
				Idx = -1;
				bHaveRelay = false;
				continue;
			}

			if (bHaveMCD && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_STATE_DEF)) != -1))
			{
				std::string tmpN;
				tmpN = "MCD (" + name + ")";
				if (tmpValue == 1 || tmpValue == 3)
					tmpValue = blinds_sOpen;
				else if (tmpValue == 0 || tmpValue == 4)
					tmpValue = blinds_sClose;
				else
					tmpValue = 0x11;
				SendSwitch(DIOType_MCD, (uint8_t)Idx, 255, tmpValue, 0, tmpN, m_Name);
				ii = ii + 4;
				tmpstr = stdstring_trim(results[ii]);
				tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_LASTO_DEF);
				SendDenkoviTextSensor(DIOType_TXT, ++Idx, 255, tmpMeasure, name + " Last Opened");
				ii = ii + 1;
				tmpstr = stdstring_trim(results[ii]);
				tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_LASTC_DEF);
				SendDenkoviTextSensor(DIOType_TXT, ++Idx, 255, tmpMeasure, name + " Last Closed");
				Idx = -1;
				bHaveMCD = false;
				continue;
			}
		}
		break;
	}
	case DDEV_DAEnet_IP2_8_RELAYS: {
		uint8_t port3 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_3_VAL);
		uint8_t port5 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_5_VAL);
		for (ii = 0; ii < 8; ii++)//8 digital inputs/outputs from port3
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 1);
			name = "Digital IO " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_DIO, (uint8_t)(ii + 1), 255, ((port3 & (0x01 << ii)) != 0) ? true : false, 0, name, m_Name);
		}
		for (ii = 0; ii < 8; ii++)//8 digital inputs/outputs from port5
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 8 + 1);
			name = "Relay " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_Relay, (uint8_t)(ii + 8 + 1), 255, ((port5 & (0x01 << ii)) != 0) ? true : false, 0, name, m_Name);
		}
		for (ii = 0; ii < 8; ii++)//8 analog inputs
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 16 + 1);
			tmpValue = DAEnetIP2GetAiValue(sResult, ii + 1);
			name = "Analog Input " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendCustomSensor(DIOType_AI, (uint8_t)(ii + 16 + 1), 255, static_cast<float>(tmpValue), name, "");
			//float voltageValue = (float)(10.44*(tmpValue / 1024));
			float voltageValue = DAEnetIP2CalculateAi(tmpValue, DAENETIP2_AI_VOLTAGE);
			SendVoltageSensor(DIOType_AI, ii + 24 + 1, 255, voltageValue, name);
			//float temperatureValue = (float)(2.05*(tmpValue / 1024));
			float temperatureValue = DAEnetIP2CalculateAi(tmpValue, DAENETIP2_AI_TEMPERATURE);
			SendTempSensor(ii + 1, 255, temperatureValue, name);
		}
		break;
	}
	case DDEV_DAEnet_IP2: {
		uint8_t port3 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_3_VAL);
		uint8_t port5 = DAEnetIP2GetIoPort(sResult, DAENETIP2_PORT_5_VAL);
		for (ii = 0; ii < 8; ii++)//8 digital inputs/outputs from port3
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 1);
			name = "Digital IO " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_DIO, (uint8_t)(ii + 1), 255, ((port3 & (0x01 << ii)) != 0) ? true : false, 0, name, m_Name);
		}
		for (ii = 0; ii < 8; ii++)//8 digital inputs/outputs from port5
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 8 + 1);
			name = "Digital IO " + std::to_string(ii + 8 + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_DIO, (uint8_t)(ii + 8 + 1), 255, ((port5 & (0x01 << ii)) != 0) ? true : false, 0, name, m_Name);
		}
		for (ii = 0; ii < 8; ii++)//8 analog inputs
		{
			tmpName = DAEnetIP2GetName(sResult, ii + 16 + 1);
			tmpValue = DAEnetIP2GetAiValue(sResult, ii + 1);
			name = "Analog Input " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendCustomSensor(DIOType_AI, (uint8_t)(ii + 16 + 1), 255, static_cast<float>(tmpValue), name, "");
		}
		break;
	}
	case DDEV_DAEnet_IP3: {
		unsigned int pins = std::stoul(DAEnetIP3GetIo(sResult, DAENETIP3_PORTA_MDO_DEF), nullptr, 16);
		for (ii = 0; ii < 16; ii++)//16 digital outputs
		{
			std::stringstream io;
			io << std::uppercase << std::hex << ii;
			tmpName = DAEnetIP3GetIo(sResult, DAENETIP3_PORTA_SNAME_DEF + io.str());
			name = "Digital Output " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_DO, (uint8_t)(ii + 1), 255, ((pins & (0x0001 << ii)) != 0) ? true : false, 0, name, m_Name);
		}

		pins = std::stoul(DAEnetIP3GetIo(sResult, DAENETIP3_PORTB_MDI_DEF), nullptr, 16);
		for (ii = 0; ii < 8; ii++)//8 digital inputs
		{
			std::stringstream io;
			io << std::uppercase << std::hex << ii;
			tmpName = DAEnetIP3GetIo(sResult2, DAENETIP3_PORTB_SNAME_DEF + io.str());
			name = "Digital Input " + std::to_string(ii + 1) + " (" + tmpName + ")";
			SendSwitch(DIOType_DI, (uint8_t)(ii + 1), 255, ((pins & (0x01 << ii)) != 0) ? true : false, 0, name, m_Name);
		} 

		for (ii = 0; ii < 8; ii++)//8 analog inputs
		{
			tmpMeasure = DAEnetIP3GetAi(sResult, (DAENETIP3_PORTC_SVAL_DEF + std::to_string(ii)), DAENETIP3_AI_VALUE);
			tmpstr = DAEnetIP3GetAi(sResult, (DAENETIP3_PORTC_SVAL_DEF + std::to_string(ii)), DAENETIP3_AI_DIMENSION);
			tmpName = DAEnetIP3GetIo(sResult2, DAENETIP3_PORTC_SNAME_DEF + std::to_string(ii)); 
			SendCustomSensor(DIOType_AI, (uint8_t)(ii + 1), 255, static_cast<float>(atoi(tmpMeasure.c_str())), "Analog Input Scaled " + std::to_string(ii + 1) + " (" + tmpName + ")", tmpstr);
		}
	}
	case DDEV_SmartDEN_IP_16_R_MT://has only relays
	case DDEV_SmartDEN_IP_16_R_MQ:  
	case DDEV_SmartDEN_IP_16_Relays: {//has only relays
		bool bHaveRelays = false;
		for (ii = 1; ii < (int)results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (!bHaveRelays && ((Idx = DenkoviCheckForIO(tmpstr, DAE_RELAY_DEF)) != -1))
			{
				bHaveRelays = true;
				continue;
			}

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveRelays && (Idx != -1) && ((tmpState = DenkoviGetIntParameter(tmpstr, DAE_STATE_DEF)) != -1))
			{
				std::stringstream sstr;
				sstr << "Relay " << Idx << " (" << name << ")";
				SendSwitch(DIOType_Relay, (uint8_t)Idx, 255, (tmpState == 1) ? true : false, 0, sstr.str(), m_Name);
				Idx = -1;
				bHaveRelays = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_IP_Watchdog: {//has only relays
		bool bHaveRelays = false;
		for (ii = 1; ii < (int)results.size(); ii++)
		{
			tmpstr = stdstring_trim(results[ii]);

			if (!bHaveRelays && ((Idx = DenkoviCheckForIO(tmpstr, DAE_CH_DEF)) != -1))
			{
				bHaveRelays = true;
				continue;
			}

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveRelays && (Idx != -1) && ((tmpState = DenkoviGetIntParameter(tmpstr, DAE_RSTATE_DEF)) != -1))
			{
				name = "Relay " + std::to_string(Idx) + "(" + name + ")";
				SendSwitch(DIOType_Relay, (uint8_t)Idx, 255, (tmpState == 1) ? true : false, 0, name, m_Name);
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
		for (ii = 1; ii < (int)results.size(); ii++)
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

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{ 
				SendSwitch(DIOType_DI, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, "Digital Input " + std::to_string(Idx) + " (" + name + ")", m_Name);

				//Check if there is counter
				tmpstr = stdstring_trim(results[ii + 1]);
				if ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_COUNT_DEF)) != -1)
				{
					SendDenkoviTextSensor(DIOType_TXT, Idx + 8, 255, DenkoviGetStrParameter(tmpstr, DAE_COUNT_DEF), "Digital Input Counter " + std::to_string(Idx) + " (" + name + ")");
				}
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				SendCustomSensor(Idx + 8, 1, 255, static_cast<float>(tmpValue), "Analog Input " + std::to_string(Idx) + " (" + name + ")", "");

				//Check if there is sclaed value
				tmpstr = stdstring_trim(results[ii + 1]);
				if (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)).empty())
				{
					std::vector<std::string> vMeasure;
					StringSplit(tmpMeasure, " ", vMeasure);
					if (vMeasure.size() == 2)
					{
						SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), "Analog Input Scaled " + std::to_string(Idx) + " (" + name + ")", vMeasure[1]);
					}
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveTemperatureInput && (Idx != -1) && (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_VALUE_DEF)).empty()))
			{ 
				name = "Temperature Input " + std::to_string(Idx) + " (" + name + ")";				 
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				
				if ((tmpMeasure.find("---") == std::string::npos))
				{
					tmpTiValue = static_cast<float>(atof(tmpMeasure.c_str()));

					if (tmpMeasure[tmpMeasure.size() - 1] == -119)//Check if F is found
						tmpTiValue = (float)ConvertToCelsius((double)tmpTiValue);
				}
				else
					tmpTiValue = 0; 

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
		for (ii = 1; ii < (int)results.size(); ii++)
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

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				SendSwitch(DIOType_DI, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, "Digital Input " + std::to_string(Idx) + " (" + name + ")", m_Name);

				//Check if there is counter
				tmpstr = stdstring_trim(results[ii + 1]);
				if ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_COUNT_DEF)) != -1)
				{
					SendDenkoviTextSensor(DIOType_TXT, Idx + 8, 255, DenkoviGetStrParameter(tmpstr, DAE_COUNT_DEF), "Digital Input Counter " + std::to_string(Idx) + " (" + name + ")");
				}
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}
						
			if (bHaveAnalogInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				SendCustomSensor(Idx+8, 1, 255, static_cast<float>(tmpValue), "Analog Input " + std::to_string(Idx) + " (" + name + ")", "");

				//Check if there is sclaed value
				tmpstr = stdstring_trim(results[ii + 1]);
				if (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)).empty())
				{
					std::vector<std::string> vMeasure;
					StringSplit(tmpMeasure, " ", vMeasure);
					if (vMeasure.size() == 2)
					{
						SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), "Analog Input Scaled " + std::to_string(Idx) + " (" + name + ")", vMeasure[1]);
					}
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}
			if (bHaveTemperatureInput && (Idx != -1) && (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_VALUE_DEF)).empty()))
			{
				name = "Temperature Input " + std::to_string(Idx) + " (" + name + ")";
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);

				if ((tmpMeasure.find("---") == std::string::npos))
				{
					tmpTiValue = static_cast<float>(atof(tmpMeasure.c_str()));

					if (tmpMeasure[tmpMeasure.size() - 1] == -119)//Check if F is found
						tmpTiValue = (float)ConvertToCelsius((double)tmpTiValue);
				}
				else
					tmpTiValue = 0;

				SendTempSensor(Idx, 255, tmpTiValue, name);
				Idx = -1;
				bHaveTemperatureInput = false;
				continue;
			}
		}
		break;
	}
	case DDEV_SmartDEN_PLC:
	case DDEV_SmartDEN_IP_Maxi: {//has DI, AO, AI, Relays
		bool bHaveDigitalInput = false; 
		bool bHaveAnalogOutput = false;
		bool bHaveAnalogInput = false;
		bool bHaveRelay = false;
		for (ii = 1; ii < (int)results.size(); ii++)
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

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{	//Value
				name = "Digital Input " + std::to_string(Idx) + " (" + name + ")";
				SendSwitch(DIOType_DI, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, name, m_Name);
				//Counter
				tmp_counter = stdstring_trim(results[ii+1]);
				SendDenkoviTextSensor(DIOType_TXT, Idx + 8, 255, DenkoviGetStrParameter(tmp_counter, DAE_COUNT_DEF), name + " Counter");
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}

			if (bHaveAnalogInput && (Idx != -1) && (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)).empty()))
			{
				std::vector<std::string> vMeasure;
				StringSplit(tmpMeasure, " ", vMeasure);
				if (Idx <= 4)
				{
					//scaled value				 
					SendCustomSensor(Idx, DIOType_AI, 255, static_cast<float>(atof(vMeasure[0].c_str())), "Analog Input Scaled " + std::to_string(Idx) + " (" + name + ")", vMeasure[1]);
					//raw value					
					tmp_adc_raw_value = stdstring_trim(results[ii-1]);
					SendCustomSensor(Idx+8, DIOType_AI, 255, DenkoviGetFloatParameter(tmp_adc_raw_value, DAE_VALUE_DEF), "Analog Input Raw " + std::to_string(Idx) + " (" + name + ")","");
					
				}
				else {
					name = "Analog Input " + std::to_string(Idx) + " (" + name + ")";
					if (vMeasure.size() == 2) {
						tmpTiValue = static_cast<float>(atof(vMeasure[0].c_str()));
						if (vMeasure[1] == "degF")
							tmpTiValue = (float)ConvertToCelsius((double)tmpTiValue);
						SendTempSensor(Idx, 255, tmpTiValue, name);
					}
					else
						SendTempSensor(Idx, 255, NAN, name);
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveRelay && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Relay " + std::to_string(Idx) + " (" + name + ")";
				SendSwitch(DIOType_Relay, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, name, m_Name);
				Idx = -1;
				bHaveRelay = false;
				continue;
			}

			if (bHaveAnalogOutput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Analog Output " + std::to_string(Idx) + " (" + name + ")";
				double val = (100 * tmpValue) / 1023;
				SendGeneralSwitch(DIOType_AO, Idx, 255, (tmpValue > 0) ? true : false, (uint8_t)val, name, m_Name);
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
		for (ii = 1; ii < (int)results.size(); ii++)
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

			if (!(tmpName = DenkoviGetStrParameter(tmpstr, DAE_NAME_DEF)).empty())
			{
				name = tmpName;
				continue;
			}

			if (bHaveDigitalInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				SendSwitch(DIOType_DI, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, "Digital Input " + std::to_string(Idx) + " (" + name + ")", m_Name);

				//Check if there is counter
				tmpstr = stdstring_trim(results[ii + 1]);
				if ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_COUNT_DEF)) != -1)
				{
					SendDenkoviTextSensor(DIOType_TXT, Idx + 8, 255, DenkoviGetStrParameter(tmpstr, DAE_COUNT_DEF), "Digital Input Counter " + std::to_string(Idx) + " (" + name + ")");
				}
				Idx = -1;
				bHaveDigitalInput = false;
				continue;
			}
						
			if (bHaveAnalogInput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				SendCustomSensor(Idx + 8, 1, 255, static_cast<float>(tmpValue), "Analog Input " + std::to_string(Idx) + " (" + name + ")", "");

				//Check if there is sclaed value
				tmpstr = stdstring_trim(results[ii + 1]);
				if (!(tmpMeasure = DenkoviGetStrParameter(tmpstr, DAE_MEASURE_DEF)).empty())
				{
					std::vector<std::string> vMeasure;
					StringSplit(tmpMeasure, " ", vMeasure);
					if (vMeasure.size() == 2)
					{
						SendCustomSensor(Idx, 1, 255, static_cast<float>(atof(vMeasure[0].c_str())), "Analog Input Scaled " + std::to_string(Idx) + " (" + name + ")", vMeasure[1]);
					}
				}
				Idx = -1;
				bHaveAnalogInput = false;
				continue;
			}

			if (bHaveDigitalOutput && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "Digital Output " + std::to_string(Idx) + " (" + name + ")";
				SendSwitch(DIOType_DO, (uint8_t)Idx, 255, (tmpValue == 1) ? true : false, 0, name, m_Name);
				Idx = -1;
				bHaveDigitalOutput = false;
				continue;
			}

			if (bHavePWM && (Idx != -1) && ((tmpValue = DenkoviGetIntParameter(tmpstr, DAE_VALUE_DEF)) != -1))
			{
				name = "PWM " + std::to_string(Idx) + " (" + name + ")";
				SendGeneralSwitch(DIOType_PWM, Idx, 255, (tmpValue > 0) ? true : false, (uint8_t)tmpValue, name, m_Name);
				Idx = -1;
				bHavePWM = false;
				continue;
			}
		}
		break;
	}
	}
}
