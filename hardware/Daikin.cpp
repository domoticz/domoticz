#include "stdafx.h"
#include "Daikin.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sstream>

// NodeId
// 1 : Marche Arret
// 2 : LedSwitch
// 5 : Mode : represents the operating mode (m_mode)
//		value	desc AUTO,DEHUMDIFICATOR,COLD,HOT,FAN
//		2	DEHUMDIFICATOR
//		3	COLD
//		4	HOT
//		6	FAN
//		0 - 1 - 7	AUTO
// 6 : f_rate : represents the fan rate mode
//		value	desc
//		A	auto     10
//		B	silence  20
//		3	lvl_1    30
//		4	lvl_2    40
//		5	lvl_3    50
//		6	lvl_4    60
//		7	lvl_5    70
// 7  : f_dir : represents the fan direction
//		value	desc
//		0	all wings stopped		10
//		1	vertical wings motion	20
//		2	horizontal wings motion	30
//		3	vertical and horizontal wings motion	40
// 10 : temperature exterieure
// 11 : temperature interieure
// 12 : shum : description: represents the target humidity
// 20 : consigne thermostat


#define Daikin_POLL_INTERVAL 300

#ifdef _DEBUG
	//#define DEBUG_DaikinR
	//#define DEBUG_DaikinR_File "C:\\TEMP\\Daikin_R_control_info.txt"
	//#define DEBUG_DaikinW
	//#define DEBUG_DaikinW_File "C:\\TEMP\\Daikin_W_control_info.txt"
#endif

#ifdef DEBUG_DaikinW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE *fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif
#ifdef DEBUG_DaikinR
std::string ReadFile(std::string filename)
{
	std::ifstream file;
	std::string sResult = "";
	file.open(filename.c_str());
	if (!file.is_open())
		return "";
	std::string sLine;
	while (!file.eof())
	{
		getline(file, sLine);
		sResult += sLine;
	}
	file.close();
	return sResult;
}
#endif

CDaikin::CDaikin(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &username, const std::string &password) :
m_szIPAddress(IPAddress),
m_Username(CURLEncode::URLEncode(username)),
m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID=ID;
	m_usIPPort=usIPPort;
	m_stoprequested=false;
	m_bOutputLog = false;
	Init();
}

CDaikin::~CDaikin(void)
{
}

void CDaikin::Init()
{
}

bool CDaikin::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CDaikin::Do_Work, this);
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_STATUS, "Daikin: Started hardware %s ...", m_szIPAddress.c_str());
	return (m_thread != nullptr);
}

bool CDaikin::StopHardware()
{
	if (m_thread)
	{
		m_stoprequested = true;
		m_thread->join();
		m_thread.reset();
	}
	_log.Log(LOG_STATUS, "Daikin: Stopped hardware %s ...", m_szIPAddress.c_str());
	m_bIsStarted=false;
	return true;
}

void CDaikin::Do_Work()
{
	int sec_counter = Daikin_POLL_INTERVAL - 2;

	while (!m_stoprequested)
	{
		sleep_seconds(1);
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter % Daikin_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}
	}
	_log.Log(LOG_STATUS, "Daikin: Worker stopped %s ...", m_szIPAddress.c_str());
}

// pData = _tThermostat

bool CDaikin::WriteToHardware(const char *pdata, const unsigned char /*length*/)
{
	_log.Debug(DEBUG_HARDWARE, "Daikin: Worker %s, Write to Hardware...", m_szIPAddress.c_str());
	const tRBUF *pCmd = reinterpret_cast<const tRBUF *>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;

	// hardware\MySensorsBase.cpp(1309)

	if (packettype == pTypeLighting2)
	{
		//Light command
		int node_id = pCmd->LIGHTING2.id4;
		//int child_sensor_id = pCmd->LIGHTING2.unitcode;
		bool command = pCmd->LIGHTING2.cmnd;
		if (node_id == 1)
		{
			SetGroupOnOFF(command);
		}
		else if (node_id==2)
		{
			SetLedOnOFF(command);
		}

	}
	else if (packettype == pTypeGeneralSwitch)
	{
		//Light command
		const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int node_id = pSwitch->id;
		//int child_sensor_id = pSwitch->unitcode;
		bool command = pSwitch->cmnd;

		_log.Debug(DEBUG_HARDWARE, "Daikin: Worker %s, Set General Switch ID %d, command : %d, value : %d", m_szIPAddress.c_str(), node_id, command, pSwitch->level);


		if (node_id == 1)
		{
			SetGroupOnOFF(command);
		}
		else if (node_id == 2)
		{
			SetLedOnOFF(command);
		}
		else if (node_id == 5)
		{

			SetModeLevel(pSwitch->level);
		}
		else if (node_id == 6)
		{
			SetF_RateLevel(pSwitch->level);
		}
		else if (node_id == 7)
		{
			SetF_DirLevel(pSwitch->level);
		}
	}
	else if (packettype == pTypeColorSwitch)
	{
		//RGW/RGBW command
	}
	else if (packettype == pTypeBlinds)
	{
		//Blinds/Window command
		//int node_id = pCmd->BLINDS1.id3;
		//int child_sensor_id = pCmd->BLINDS1.unitcode;
	}
	else if ((packettype == pTypeThermostat) && (subtype == sTypeThermSetpoint))
	{
		//Set Point
		const _tThermostat *pMeter = reinterpret_cast<const _tThermostat *>(pCmd);
		int node_id = pMeter->id2;
		//int child_sensor_id = pMeter->id3;

		_log.Debug(DEBUG_HARDWARE, "Daikin: Worker %s, Thermostat %.1f", m_szIPAddress.c_str(), pMeter->temp);

		char szTmp[10];
		sprintf(szTmp, "%.1f", pMeter->temp);
		SetSetpoint(node_id, pMeter->temp);
	}
	else
	{
		_log.Log(LOG_ERROR, "Daikin : Unknown action received");
		return false;
	}

	return true;
}

// https://github.com/ael-code/daikin-control
// sample aircon/set_control_info?pow=1&mode=4&f_rate=A&f_dir=0&stemp=18.0&shum=0

void CDaikin::SetSetpoint(const int /*idx*/, const float temp)
{
	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Set Point...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/aircon/set_control_info?";
	szURL << "pow=" << m_pow;
	szURL << "&mode=" << m_mode;

	// cibre température
	char szTmp[100];
	sprintf(szTmp, "%.1f", temp);
	szURL << "&stemp=" << szTmp;
	szURL << "&shum=" << m_shum;
	szURL << "&f_rate=" << m_f_rate;
	szURL << "&f_dir=" << m_f_dir;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}


	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}

}


// https://github.com/ael-code/daikin-control
// sample aircon/set_control_info?pow=1&mode=4&f_rate=A&f_dir=0&stemp=18.0&shum=0


void CDaikin::UpdateSwitchNew(const unsigned char Idx, const int /*SubUnit*/, const bool bOn, const double Level, const std::string &defaultname)
{
	_tGeneralSwitch gswitch;
	gswitch.subtype = sSwitchGeneralSwitch;
	gswitch.type = pTypeGeneralSwitch;
	gswitch.battery_level = 255;
	gswitch.id = Idx;

	// Get device level to set
	//int level = static_cast<int>(Level);

	// Now check the values
	if (bOn)
		gswitch.cmnd = gswitch_sOff;
	else
		gswitch.cmnd = gswitch_sOn;
	gswitch.level = static_cast<uint8_t>(Level);
	gswitch.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, defaultname.c_str(), 255);
}


void CDaikin::GetMeterDetails()
{
	GetBasicInfo();
	GetControlInfo();
	GetSensorInfo();
}

void CDaikin::GetBasicInfo()
{
	std::string sResult;
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/common/basic_info";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	/*
	ret=OK,
	type=aircon,
	reg=eu,
	dst=1,
	ver=3_3_1,
	pow=1,
	err=0,location=0,name=%53%61%6c%6f%6e,icon=9,
	method=home only,port=30050,id=,pw=,lpw_flag=0,adp_kind=2,pv=0,cpv=0,cpv_minor=00,
	led=0,
	en_setzone=1,mac=A408EAD5D394,adp_mode=run,en_hol=0,
	grp_name=%72%65%7a%20%64%65%20%63%68%61%75%73%73%c3%a9,en_grp=1
	*/
	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 8)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid data received");
		return;
	}
	for (const auto & itt : results)
	{
		std::string sVar = itt;
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "led")
		{
			m_led = results2[1];
			UpdateSwitchNew(2, -1, (m_led[0] == '0') ? true : false, 100, "Led indicator");

		}
	}

}

void CDaikin::GetControlInfo()
{
	std::string sResult;
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/aircon/get_control_info";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
	/*	ret = OK,
		pow = 1,
		mode = 4, adv=,
		stemp = 20.0, shum = 0,
		dt1 = 20.0, dt2 = M, dt3 = 25.0, dt4 = 20.0, dt5 = 20.0, dt7 = 20.0,
		dh1 = 0, dh2 = 0, dh3 = 0, dh4 = 0, dh5 = 0, dh7 = 0,
		dhh = 50,
		b_mode = 4,
		b_stemp = 20.0,
		b_shum = 0,
		alert = 255,
		f_rate = A,
		f_dir = 0,
		b_f_rate = A,
		b_f_dir = 0,
		dfr1 = A, dfr2 = A, dfr3 = 5, dfr4 = A, dfr5 = A, dfr6 = 5, dfr7 = A, dfrh = 5,
		dfd1 = 0, dfd2 = 0, dfd3 = 0, dfd4 = 0, dfd5 = 0, dfd6 = 0, dfd7 = 0, dfdh = 0
	*/
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, DEBUG_DaikinW_File);
#endif
	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 8)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid data received");
		return;
	}
	for (const auto & itt : results)
	{
		std::string sVar = itt;
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "mode")
		{
			if (m_mode != results2[1])
			{
				//		value	desc
				//		2	DEHUMDIFICATOR
				//		3	COLD
				//		4	HOT
				//		6	FAN
				//		0 - 1 - 7	AUTO
				m_mode = results2[1];
				if (m_mode == "2") // DEHUMDIFICATOR
					InsertUpdateSwitchSelector(5,true,20,"Mode");
				else if (m_mode == "3") // COLD
					InsertUpdateSwitchSelector(5, true, 30, "Mode");
				else if (m_mode == "4") // HOT
					InsertUpdateSwitchSelector(5, true, 40, "Mode");
				else if (m_mode == "6")
					InsertUpdateSwitchSelector(5, true, 50, "Mode");
				else if ((m_mode == "0") || (m_mode == "1" ) || ( m_mode == "7") )
					InsertUpdateSwitchSelector(5, true, 10, "Mode");
			}
		}
		else if (results2[0] == "pow")
		{
			if (m_pow!=results2[1])
			{
				m_pow = results2[1];
				UpdateSwitchNew(1, -1, (results2[1][0] == '1') ? false : true, 100, "Power");
			}
		}
		else if (results2[0] == "otemp")
		{
			if (m_otemp!=results2[1])
			{
				m_otemp = results2[1];
				SendTempSensor(10, -1, static_cast<float>(atof(results2[1].c_str())), "Outside Temperature");
			}
		}
		else if (results2[0] == "stemp")
		{
			if (m_stemp != results2[1])
			{
				m_stemp = results2[1];
				SendSetPointSensor(20, 1, 1, static_cast<float>(atof(results2[1].c_str())), "Target Temperature");
			}
		}
		else if (results2[0] == "f_rate")
		{

			m_f_rate=results2[1];
			if (m_f_rate == "A") // DEHUMDIFICATOR
				InsertUpdateSwitchSelector(6, true, 10, "Ventillation");
			else if (m_f_rate == "B") // COLD
				InsertUpdateSwitchSelector(6, true, 20, "Ventillation");
			else if (m_f_rate == "3") // HOT
				InsertUpdateSwitchSelector(6, true, 30, "Ventillation");
			else if (m_f_rate == "4")
				InsertUpdateSwitchSelector(6, true, 40, "Ventillation");
			else if (m_f_rate == "5")
				InsertUpdateSwitchSelector(6, true, 50, "Ventillation");
			else if (m_f_rate == "6")
				InsertUpdateSwitchSelector(6, true, 60, "Ventillation");
			else if (m_f_rate == "7")
				InsertUpdateSwitchSelector(6, true, 70, "Ventillation");
		}
		else if (results2[0] == "f_dir")
		{
			m_f_dir = results2[1];
			if (m_f_dir == "0") //
				InsertUpdateSwitchSelector(7, true, 10, "Winds");
			else if (m_f_dir == "1") // COLD
				InsertUpdateSwitchSelector(7, true, 20, "Winds");
			else if (m_f_dir == "2") // COLD
				InsertUpdateSwitchSelector(7, true, 30, "Winds");
			else if (m_f_dir == "3") // COLD
				InsertUpdateSwitchSelector(7, true, 40, "Winds");

		}
		else if (results2[0] == "shum")
		{
			m_shum = results2[1];
		}

	}

}

void CDaikin::GetSensorInfo()
{
	std::string sResult;
#ifdef DEBUG_DaikinR
	sResult = ReadFile("E:\\Daikin_get_sensor_info.txt");
#else
	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/aircon/get_sensor_info";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, DEBUG_DaikinW_File);
#endif
#endif

	 // ret=OK,htemp=21.0,hhum=-,otemp=9.0,err=0,cmpfreq=35
	 // grp.update('compressorfreq', tonumber(data.cmpfreq))


	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 6)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid data received");
		return;
	}
	float htemp = -1;
	int hhum = -1;
	for (const auto & itt : results)
	{
		std::string sVar = itt;
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "htemp")
		{
			htemp = static_cast<float>(atof(results2[1].c_str()));
		}
		else if (results2[0] == "hhum")
		{
			if (results2[1]!="-")
				hhum = static_cast<int>(atoi(results2[1].c_str()));
		}
		else if (results2[0] == "otemp")
		{
			SendTempSensor(10, -1, static_cast<float>(atof(results2[1].c_str())), "Outside Temperature");

		}
	}
	if (htemp != -1)
	{
		if (hhum != -1) {
			SendTempHumSensor(m_HwdID, -1, htemp, hhum, "Home Temp+Hum");
		}
		else
			SendTempSensor(11, -1, htemp, "Home Temperature");
	}
}

void CDaikin::SetLedOnOFF(const bool OnOFF)
{
	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Set Led ...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}

	szURL << "/common/set_led?led=";

	if (OnOFF)
		szURL << "1";
	else
		szURL << "0";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}

	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}
}

void CDaikin::SetGroupOnOFF(const bool OnOFF)
{
	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Group On/Off...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}
	szURL << "/aircon/set_control_info?";

	if (OnOFF)
		szURL << "pow=1";
	else
		szURL << "pow=0";

	szURL << "&mode=" << m_mode;

	szURL << "&stemp=" << m_stemp;

	szURL << "&shum=" << m_shum;

	szURL << "&f_rate=" << m_f_rate;

	szURL << "&f_dir=" << m_f_dir;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}


	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}

}


void CDaikin::InsertUpdateSwitchSelector(const unsigned char Idx,  const bool bIsOn, const int level, const std::string &defaultname)
{

	unsigned int sID = Idx;

	char szTmp[300];
	std::string ID;
	sprintf (szTmp, "%c", Idx);
	ID = szTmp;

	_tGeneralSwitch xcmd;
	xcmd.len = sizeof(_tGeneralSwitch) - 1;
	xcmd.id = sID;
	xcmd.type = pTypeGeneralSwitch;
	xcmd.subtype = sSwitchGeneralSwitch;
	xcmd.unitcode = 1;
	int customimage = 0;

//	if ((xcmd.unitcode > 2) && (xcmd.unitcode < 8)) {
//		customimage = 8; //speaker
//	}

	if (bIsOn) {
		xcmd.cmnd = gswitch_sOn;
	}
	else {
		xcmd.cmnd = gswitch_sOff;
	}
	_eSwitchType switchtype;
	switchtype = STYPE_Selector;

	xcmd.subtype = sSwitchTypeSelector;
	if (level > 0) {
		xcmd.level = (uint8_t)level;
	}

	//check if this switch is already in the database
	std::vector<std::vector<std::string> > result;

	// block this device if it is already added for another gateway hardware id
	result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID!=%d) AND (DeviceID=='%d') AND (Type==%d) AND (Unit == '%d')", m_HwdID, sID, xcmd.type, xcmd.unitcode);

	if (!result.empty()) {
		return;
	}

	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&xcmd, defaultname.c_str(), 255);

	result = m_sql.safe_query("SELECT nValue, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='0000000%d') AND (Type==%d) AND (Unit == '%d')", m_HwdID, sID, xcmd.type, xcmd.unitcode);
	if (result.empty())
	{
		// New Hardware -> Create the corresponding devices Selector
		if (customimage == 0) {
			if (sID == 5) {
				customimage = 16; //wall socket
			}
			else if ((sID == 6) || (sID == 7)) {
				customimage = 7;
			}
		}
		m_sql.safe_query("UPDATE DeviceStatus SET Name='%q', SwitchType=%d, CustomImage=%i WHERE(HardwareID == %d) AND (DeviceID == '0000000%d') AND (Unit == '%d')", defaultname.c_str(), (switchtype), customimage, m_HwdID, sID, xcmd.unitcode);

		result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='0000000%d') AND (Type==%d) AND (Unit == '%d')", m_HwdID, sID, xcmd.type, xcmd.unitcode);
		// SelectorStyle:0;LevelNames:Off|Main|Sub|Main+Sub;LevelOffHidden:true;LevelActions:00|01|02|03"
		if (!result.empty()) {
			std::string sIdx = result[0][0];
			if (defaultname == "Mode") {
				m_sql.SetDeviceOptions(atoi(sIdx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|AUTO|DEHUMDIFICATOR|COLD|HOT|FAN;LevelOffHidden:true;LevelActions:00|01|02|03|04|06", false));
			}
			else if (defaultname == "Ventillation") {
				//for the Fans
				m_sql.SetDeviceOptions(atoi(sIdx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|AUTO|Silence|Lev 1|Lev 2|Lev 3|Lev 4|Lev 5;LevelOffHidden:true;LevelActions:00|10|20|30|40|50|60|70", false));
			}
			else if (defaultname == "Winds") {
				//for the Wings
				m_sql.SetDeviceOptions(atoi(sIdx.c_str()), m_sql.BuildDeviceOptions("SelectorStyle:0;LevelNames:Off|Stopped|Vert|Horiz|Both;LevelOffHidden:true", false));
			}
		}
	}
}

void CDaikin::SetModeLevel(const int NewLevel)
{

	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Mode Level...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}
	szURL << "/aircon/set_control_info?";

	szURL << "pow=" << m_pow;

        if (NewLevel == 0)
                szURL << "&mode=0";
        else if (NewLevel == 10)
                szURL << "&mode=1"; /* 10-AUTO  1 */
        else if (NewLevel == 20)
                szURL << "&mode=2"; /* 20-DEHUM 2 */
        else if (NewLevel == 30)
                szURL << "&mode=3"; /* 30-COLD  3 */
        else if (NewLevel == 40)
                szURL << "&mode=4"; /* 40-HOT   4 */
        else if (NewLevel == 50)
                szURL << "&mode=6"; /* 50-FAN   6 */

	szURL << "&stemp=" << m_stemp;

	szURL << "&shum=" << m_shum;

	szURL << "&f_rate=" << m_f_rate;

	szURL << "&f_dir=" << m_f_dir;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}


	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}



}

void CDaikin::SetF_RateLevel(const int NewLevel)
{

	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Rate ...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}
	szURL << "/aircon/set_control_info?";

	szURL << "pow=" << m_pow;

	szURL << "&mode=" << m_mode;

	szURL << "&stemp=" << m_stemp;

	szURL << "&shum=" << m_shum;

	if (NewLevel == 10)
		szURL << "&f_rate=A";
	else if (NewLevel == 20)
		szURL << "&f_rate=B";
	else if (NewLevel == 30)
		szURL << "&f_rate=3";
	else if (NewLevel == 40)
		szURL << "&f_rate=4";
	else if (NewLevel == 50)
		szURL << "&f_rate=5";
	else if (NewLevel == 60)
		szURL << "&f_rate=6";
	else if (NewLevel == 70)
		szURL << "&f_rate=7";

	szURL << "&f_dir=" << m_f_dir;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}

	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}
}
void CDaikin::SetF_DirLevel(const int NewLevel)
{
	std::string sResult;

	_log.Debug(DEBUG_HARDWARE, "Daikin: Dir Level ...");

	std::stringstream szURL;

	if (m_Password.empty())
	{
		szURL << "http://" << m_szIPAddress << ":" << m_usIPPort;
	}
	else
	{
		szURL << "http://" << m_Username << ":" << m_Password << "@" << m_szIPAddress << ":" << m_usIPPort;
	}
	szURL << "/aircon/set_control_info?";

	szURL << "pow=" << m_pow;

	szURL << "&mode=" << m_mode;

	szURL << "&stemp=" << m_stemp;

	szURL << "&shum=" << m_shum;

	szURL << "&f_rate=" << m_f_rate;

	if (NewLevel == 10)
		szURL << "&f_dir=0"; // All wings motion
	else if (NewLevel == 20)
		szURL << "&f_dir=1"; // Vertical wings motion
	else if (NewLevel == 30)
		szURL << "&f_dir=2"; // Horizontal wings motion
	else if (NewLevel == 40)
		szURL << "&f_dir=3"; // vertical + horizontal wings motion

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		_log.Log(LOG_ERROR, "Daikin: Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}

	if (sResult.find("ret=OK") == std::string::npos)
	{
		_log.Log(LOG_ERROR, "Daikin: Invalid response");
		return;
	}
}
