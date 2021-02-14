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
// 10 : Outside temperature
// 11 : Inside temperature
// 12 : shum : description: represents the target humidity
// 20 : SetPoint


#define Daikin_POLL_INTERVAL 300

#ifdef _DEBUG
	//#define DEBUG_DaikinR
	//#define DEBUG_DaikinW
#endif

#ifdef DEBUG_DaikinW
void SaveString2Disk(std::string str, std::string filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fwrite("\n", 1, 1, fOut);
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

CDaikin::CDaikin(const int ID, const std::string& IPAddress, const unsigned short usIPPort, const std::string& username, const std::string& password) :
	m_szIPAddress(IPAddress),
	m_Username(CURLEncode::URLEncode(username)),
	m_Password(CURLEncode::URLEncode(password))
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;
	m_bOutputLog = false;
	Init();
}

void CDaikin::Init()
{
}

bool CDaikin::StartHardware()
{
	RequestStart();

	Init();
	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool CDaikin::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted = false;
	return true;
}

void CDaikin::Do_Work()
{
	time_t last_sci_update = 0;
	time_t current_time = 0;
	m_sec_counter = Daikin_POLL_INTERVAL - 2; // Trigger immediatly (in 2s) a POLL after startup.
	m_last_setcontrolinfo = 0; // no set request occured at this point
	m_force_sci = false;

	Log(LOG_STATUS, "Worker started %s ...", m_szIPAddress.c_str());
	while (!IsStopRequested(1000))
	{
		m_sec_counter++;

		if (m_sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (m_sec_counter % Daikin_POLL_INTERVAL == 0)
		{
			GetMeterDetails();
		}

		current_time = mytime(nullptr);

		if (m_force_sci || ((current_time - m_last_setcontrolinfo > 2) && (last_sci_update < m_last_setcontrolinfo))) {
			HTTPSetControlInfo(); // Fire HTTP request
			m_force_sci = false;
			m_last_setcontrolinfo = 0;
			last_sci_update = current_time;
		}
	}
	Log(LOG_STATUS, "Worker stopped %s ...", m_szIPAddress.c_str());
}

bool CDaikin::WriteToHardware(const char* pdata, const unsigned char /*length*/)
{
	Debug(DEBUG_HARDWARE, "Worker %s, Write to Hardware...", m_szIPAddress.c_str());
	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;
	bool result = true;

	if (packettype == pTypeLighting2)
	{
		//Light command
		int node_id = pCmd->LIGHTING2.id4;
		//int child_sensor_id = pCmd->LIGHTING2.unitcode;
		bool command = pCmd->LIGHTING2.cmnd;
		if (node_id == 1)
		{
			result = SetGroupOnOFF(command);
		}
		else if (node_id == 2)
		{
			result = SetLedOnOFF(command);
		}
	}
	else if (packettype == pTypeGeneralSwitch)
	{
		//Light command
		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int node_id = pSwitch->id;
		//int child_sensor_id = pSwitch->unitcode;
		bool command = pSwitch->cmnd;

		Debug(DEBUG_HARDWARE, "Worker %s, Set General Switch ID %d, command : %d, value : %d", m_szIPAddress.c_str(), node_id, command, pSwitch->level);

		if (node_id == 1)
		{
			result = SetGroupOnOFF(command);
		}
		else if (node_id == 2)
		{
			result = SetLedOnOFF(command);
		}
		else if (node_id == 5)
		{
			result = SetModeLevel(pSwitch->level);
		}
		else if (node_id == 6)
		{
			result = SetF_RateLevel(pSwitch->level);
		}
		else if (node_id == 7)
		{
			result = SetF_DirLevel(pSwitch->level);
		}
	}
	else if ((packettype == pTypeThermostat) && (subtype == sTypeThermSetpoint))
	{
		//Set Point
		const _tThermostat* pMeter = reinterpret_cast<const _tThermostat*>(pCmd);
		int node_id = pMeter->id2;
		//int child_sensor_id = pMeter->id3;

		Debug(DEBUG_HARDWARE, "Worker %s, Thermostat %.1f", m_szIPAddress.c_str(), pMeter->temp);

		char szTmp[10];
		sprintf(szTmp, "%.1f", pMeter->temp);
		result = SetSetpoint(node_id, pMeter->temp);
	}
	else
	{
		Log(LOG_ERROR, "Unknown action received");
		return false;
	}

	return result;
}

void CDaikin::UpdateSwitchNew(const unsigned char Idx, const int /*SubUnit*/, const bool bOn, const double Level, const std::string& defaultname)
{
	_tGeneralSwitch gswitch;
	gswitch.subtype = sSwitchGeneralSwitch;
	gswitch.type = pTypeGeneralSwitch;
	gswitch.battery_level = 255;
	gswitch.id = Idx;

	// Now check the values
	if (bOn)
		gswitch.cmnd = gswitch_sOff;
	else
		gswitch.cmnd = gswitch_sOn;
	gswitch.level = static_cast<uint8_t>(Level);
	gswitch.rssi = 12;
	sDecodeRXMessage(this, (const unsigned char *)&gswitch, defaultname.c_str(), 255, m_Name.c_str());
}

void CDaikin::GetMeterDetails()
{
	GetBasicInfo();
	GetControlInfo();
	GetSensorInfo();
	GetYearPower();
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
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\daikin_basic_info.txt");
#endif

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
		Log(LOG_ERROR, "Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 8)
	{
		Log(LOG_ERROR, "Invalid data received");
		return;
	}

	for (const auto &sVar : results)
	{
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "led")
		{
			if (m_led != results2[1]) { // update only if device led state was changed from other source than domoticz : direct http command for example
				m_led = results2[1];
				UpdateSwitchNew(2, -1, (m_led[0] == '0') ? true : false, 100, "Led indicator");
			}

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
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
		SaveString2Disk(sResult, "E:\\daikin_get_control_info.txt");
#endif

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
	if (sResult.find("ret=OK") == std::string::npos)
	{
		Log(LOG_ERROR, "Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 8)
	{
		Log(LOG_ERROR, "Invalid data received");
		return;
	}
	for (const auto &sVar : results)
	{
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
					InsertUpdateSwitchSelector(5, true, 20, "Mode");
				else if (m_mode == "3") // COLD
					InsertUpdateSwitchSelector(5, true, 30, "Mode");
				else if (m_mode == "4") // HOT
					InsertUpdateSwitchSelector(5, true, 40, "Mode");
				else if (m_mode == "6") // FAN
					InsertUpdateSwitchSelector(5, true, 50, "Mode");
				else if ((m_mode == "0") || (m_mode == "1") || (m_mode == "7")) // AUTO
					InsertUpdateSwitchSelector(5, true, 10, "Mode");
			}
		}
		else if (results2[0] == "pow")
		{
			if (m_pow != results2[1])// update only if device led state was changed from other source than domoticz : direct http command or daikin remote IR/wifi controller
			{
				m_pow = results2[1];
				UpdateSwitchNew(1, -1, (results2[1][0] == '1') ? false : true, 100, "Power");
			}
		}
		else if (results2[0] == "otemp")
		{
			if (m_otemp != results2[1])
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
			if (m_f_rate != results2[1])
			{
				m_f_rate = results2[1];
				if (m_f_rate == "A") // AUTOMATIC
					InsertUpdateSwitchSelector(6, true, 10, "Ventillation");
				else if (m_f_rate == "B") // SILENCE
					InsertUpdateSwitchSelector(6, true, 20, "Ventillation");
				else if (m_f_rate == "3") // Level 1
					InsertUpdateSwitchSelector(6, true, 30, "Ventillation");
				else if (m_f_rate == "4") // Level 2
					InsertUpdateSwitchSelector(6, true, 40, "Ventillation");
				else if (m_f_rate == "5") // Level 3
					InsertUpdateSwitchSelector(6, true, 50, "Ventillation");
				else if (m_f_rate == "6") // Level 4
					InsertUpdateSwitchSelector(6, true, 60, "Ventillation");
				else if (m_f_rate == "7") // Level 5
					InsertUpdateSwitchSelector(6, true, 70, "Ventillation");
			}
		}
		else if (results2[0] == "f_dir")
		{
			if (m_f_dir != results2[1])
			{
				m_f_dir = results2[1];
				if (m_f_dir == "0") // All wings off
					InsertUpdateSwitchSelector(7, true, 10, "Winds");
				else if (m_f_dir == "1") // Vertical wings in motion
					InsertUpdateSwitchSelector(7, true, 20, "Winds");
				else if (m_f_dir == "2") // Horizontal wings in motion
					InsertUpdateSwitchSelector(7, true, 30, "Winds");
				else if (m_f_dir == "3") // Vertical and Horizontal wings in motion
					InsertUpdateSwitchSelector(7, true, 40, "Winds");
			}
		}
		else if (results2[0] == "shum")
			m_shum = results2[1];
		else if (results2[0] == "dt1")
			m_dt[1] = results2[1];
		else if (results2[0] == "dt2")
			m_dt[2] = results2[1];
		else if (results2[0] == "dt3")
			m_dt[3] = results2[1];
		else if (results2[0] == "dt4")
			m_dt[4] = results2[1];
		else if (results2[0] == "dt5")
			m_dt[5] = results2[1];
		else if (results2[0] == "dt7")
			m_dt[7] = results2[1];
		else if (results2[0] == "dh1")
			m_dh[1] = results2[1];
		else if (results2[0] == "dh2")
			m_dh[2] = results2[1];
		else if (results2[0] == "dh3")
			m_dh[3] = results2[1];
		else if (results2[0] == "dh4")
			m_dh[4] = results2[1];
		else if (results2[0] == "dh5")
			m_dh[5] = results2[1];
		else if (results2[0] == "dh7")
			m_dh[7] = results2[1];
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
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#endif
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\daikin_get_sensor_info.txt");
#endif
	// ret=OK,htemp=21.0,hhum=-,otemp=9.0,err=0,cmpfreq=35
	// grp.update('compressorfreq', tonumber(data.cmpfreq))

	if (sResult.find("ret=OK") == std::string::npos)
	{
		Log(LOG_ERROR, "Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 6)
	{
		Log(LOG_ERROR, "Invalid data received");
		return;
	}
	float htemp = -1;
	int hhum = -1;
	for (const auto &sVar : results)
	{
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
			if (results2[1] != "-")
				hhum = static_cast<int>(atoi(results2[1].c_str()));
		}
		else if (results2[0] == "otemp")
		{
			SendTempSensor(10, -1, static_cast<float>(atof(results2[1].c_str())), "Outside Temperature");

		}
	}
	if (htemp != -1)
	{
		if (hhum != -1)
			SendTempHumSensor(m_HwdID, -1, htemp, hhum, "Home Temp+Hum");
		else
			SendTempSensor(11, -1, htemp, "Home Temperature");
	}
}

void CDaikin::GetYearPower()
{
	std::string sResult;
#ifdef DEBUG_DaikinR
	sResult = ReadFile("E:\\Daikin_get_year_power.txt");
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

	szURL << "/aircon/get_year_power_ex";

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#endif
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\daikin_get_year_power.txt");
#endif
	// ret=OK,htemp=21.0,hhum=-,otemp=9.0,err=0,cmpfreq=35
	// grp.update('compressorfreq', tonumber(data.cmpfreq))

	if (sResult.find("ret=OK") == std::string::npos)
	{
		Log(LOG_ERROR, "Error getting data (check IP/Port)");
		return;
	}
	std::vector<std::string> results;
	StringSplit(sResult, ",", results);
	if (results.size() < 4)
	{
		Log(LOG_ERROR, "Invalid data received");
		return;
	}

	float cooltotalkWh = 0;
	float heattotalkWh = 0;
	for (const auto &sVar : results)
	{
		std::vector<std::string> results2;
		StringSplit(sVar, "=", results2);
		if (results2.size() != 2)
			continue;
		if (results2[0] == "curr_year_cool")
		{
			std::vector<std::string> months;
			StringSplit(results2[1], "/", months);
			for (const auto &month : months)
			{
				cooltotalkWh += std::stof(month) / 10; // energy saved as 0.1 kWh units
			}
			SendKwhMeter(30, 30, 255, 0, cooltotalkWh, "Cool kWh");
		}
		else if (results2[0] == "curr_year_heat")
		{
			std::vector<std::string> months;
			StringSplit(results2[1], "/", months);
			for (const auto &month : months)
			{
				heattotalkWh += std::stof(month) / 10; // energy saved as 0.1 kWh units
			}
			SendKwhMeter(31, 31, 255, 0, heattotalkWh, "Heat kWh");
		}
	}
}

bool CDaikin::SetLedOnOFF(const bool OnOFF)
{
	std::string sResult;
	std::string sLed;

	Debug(DEBUG_HARDWARE, "Set Led ...");

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
		sLed = "1";
	else
		sLed = "0";

	szURL << sLed;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return false;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\daikin_set_led.txt");
#endif

	if (sResult.find("ret=OK") == std::string::npos)
	{
		Log(LOG_ERROR, "Invalid response");
		return false;
	}
	m_led = sLed;
	return true;
}

void CDaikin::InsertUpdateSwitchSelector(const unsigned char Idx, const bool bIsOn, const int level, const std::string& defaultname)
{
	unsigned int sID = Idx;

	char szTmp[300];
	std::string ID;
	sprintf(szTmp, "%c", Idx);
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

	result = m_sql.safe_query("SELECT nValue, BatteryLevel FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='0000000%d') AND (Type==%d) AND (Unit == '%d')", m_HwdID, sID, xcmd.type, xcmd.unitcode);
	m_mainworker.PushAndWaitRxMessage(this, (const unsigned char*)&xcmd, defaultname.c_str(), 255, m_Name.c_str());
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

// https://github.com/ael-code/daikin-control
// sample aircon/set_control_info?pow=1&mode=4&f_rate=A&f_dir=0&stemp=18.0&shum=0
bool CDaikin::SetSetpoint(const int /*idx*/, const float temp)
{
	Debug(DEBUG_HARDWARE, "Set Point...");

	// cible température
	char szTmp[100];
	sprintf(szTmp, "%.1f", temp);
	AggregateSetControlInfo(szTmp, nullptr, nullptr, nullptr, nullptr, nullptr);

	SendSetPointSensor(20, 1, 1, temp, "Target Temperature"); // Suppose request succeed to keep reactive web interface
	return true;
}

bool CDaikin::SetGroupOnOFF(const bool OnOFF)
{
	std::string pow;
	Debug(DEBUG_HARDWARE, "Group On/Off...");

	if (OnOFF)
		pow = "1";
	else
		pow = "0";
	AggregateSetControlInfo(nullptr, pow.c_str(), nullptr, nullptr, nullptr, nullptr);
	return true;
}

bool CDaikin::SetModeLevel(const int NewLevel)
{
	std::string mode;
	std::string stemp;
	std::string shum;
	Debug(DEBUG_HARDWARE, "Mode Level...");

	if (NewLevel == 0)
		mode = "0"; //szURL << "&mode=0";
	else if (NewLevel == 10)
		mode = "1"; //szURL << "&mode=1"; /* 10-AUTO  1 */
	else if (NewLevel == 20)
		mode = "2"; //szURL << "&mode=2"; /* 20-DEHUM 2 */
	else if (NewLevel == 30)
		mode = "3"; //szURL << "&mode=3"; /* 30-COLD  3 */
	else if (NewLevel == 40)
		mode = "4"; //szURL << "&mode=4"; /* 40-HOT   4 */
	else if (NewLevel == 50)
		mode = "6"; //szURL << "&mode=6"; /* 50-FAN   6 */
	else
		return false;

	if (NewLevel == 0) // 0 is AUTO, but there is no dt0! So in case lets force to dt1 and dh1
	{
		stemp = m_dt[1]; //szURL << "&stemp=" << m_dt[1];
		shum = m_dh[1]; //szURL << "&shum=" << m_dh[1];
	}
	else if (NewLevel == 6) // FAN mode, there is no temp/hum memorized.
	{
		stemp = "20"; //szURL << "&stemp=" << "20";
		shum = "0"; //szURL << "&shum=" << "0";
	}
	else // DEHUMDIFICATOR, COLD, HOT ( 2,3,6 )
	{
		stemp = m_dt[(NewLevel / 10)]; //szURL << "&stemp=" << m_dt[(NewLevel/10)];
		shum = m_dh[(NewLevel / 10)]; //szURL << "&shum=" << m_dh[(NewLevel/10)];
	}
	AggregateSetControlInfo(
		stemp.c_str(), nullptr, mode.c_str(), nullptr, nullptr,
		shum.c_str()); // temp & hum are memorized value from the device itself, they can be modified by another command safely
			       // (does not have to fire 2 different http request in case of instantly modification on temp or hum
	return true;
}

bool CDaikin::SetF_RateLevel(const int NewLevel)
{
	std::string f_rate;
	Debug(DEBUG_HARDWARE, "Rate ...");

	if (NewLevel == 10)
		f_rate = "A"; //szURL << "&f_rate=A";
	else if (NewLevel == 20)
		f_rate = "B"; //szURL << "&f_rate=B";
	else if (NewLevel == 30)
		f_rate = "3"; //szURL << "&f_rate=3";
	else if (NewLevel == 40)
		f_rate = "4"; //szURL << "&f_rate=4";
	else if (NewLevel == 50)
		f_rate = "5"; //szURL << "&f_rate=5";
	else if (NewLevel == 60)
		f_rate = "6"; //szURL << "&f_rate=6";
	else if (NewLevel == 70)
		f_rate = "7"; //szURL << "&f_rate=7";
	else
		return false;
	AggregateSetControlInfo(nullptr, nullptr, nullptr, f_rate.c_str(), nullptr, nullptr);
	return true;
}

bool CDaikin::SetF_DirLevel(const int NewLevel)
{
	std::string f_dir;
	Debug(DEBUG_HARDWARE, "Dir Level ...");

	if (NewLevel == 10)
		f_dir = "0"; //szURL << "&f_dir=0"; // All wings motion
	else if (NewLevel == 20)
		f_dir = "1"; //szURL << "&f_dir=1"; // Vertical wings motion
	else if (NewLevel == 30)
		f_dir = "2"; //szURL << "&f_dir=2"; // Horizontal wings motion
	else if (NewLevel == 40)
		f_dir = "3"; //szURL << "&f_dir=3"; // vertical + horizontal wings motion
	else
		return false;
	AggregateSetControlInfo(nullptr, nullptr, nullptr, nullptr, f_dir.c_str(), nullptr);
	return true;
}

void CDaikin::AggregateSetControlInfo(const char* Temp/* setpoint */, const char* OnOFF /* group on/off */, const char* ModeLevel, const char* FRateLevel, const char* FDirLevel, const char* Hum)
{
	time_t cmd_time = mytime(nullptr);
	if (cmd_time - m_last_setcontrolinfo < 2) {
		// another set request so keep old values, and update only the settled ones
		if (Temp != nullptr)
			m_sci_Temp = Temp;
		if (OnOFF != nullptr)
			m_sci_OnOFF = OnOFF;
		if (ModeLevel != nullptr)
			m_sci_ModeLevel = ModeLevel;
		if (FRateLevel != nullptr)
			m_sci_FRateLevel = FRateLevel;
		if (FDirLevel != nullptr)
			m_sci_FDirLevel = FDirLevel;
		if (Hum != nullptr)
			m_sci_Hum = Hum;

		m_last_setcontrolinfo = cmd_time;
		if (cmd_time - m_first_setcontrolinfo > 5) { // force http set request every 5 seconds in case of continious set resquest (which could be a bug, a external entry request (json) or  a misprogrammed lua script)
			m_force_sci = true;
			m_first_setcontrolinfo = cmd_time;
			Log(LOG_STATUS, "Daikin worker HTTPSetControlInfo : HTTP resquest forced, may be a bug ?"); // [JCJ] remark to github : where is the LOG_WARNING ?
		}
	}
	else {
		if (Temp != nullptr)
			m_sci_Temp = Temp;
		else
			m_sci_Temp = m_stemp;
		if (OnOFF != nullptr)
			m_sci_OnOFF = OnOFF;
		else
			m_sci_OnOFF = m_pow;
		if (ModeLevel != nullptr)
			m_sci_ModeLevel = ModeLevel;
		else
			m_sci_ModeLevel = m_mode;
		if (FRateLevel != nullptr)
			m_sci_FRateLevel = FRateLevel;
		else
			m_sci_FRateLevel = m_f_rate;
		if (FDirLevel != nullptr)
			m_sci_FDirLevel = FDirLevel;
		else
			m_sci_FDirLevel = m_f_dir;
		if (Hum != nullptr)
			m_sci_Hum = Hum;
		else
			m_sci_Hum = m_shum;
		m_last_setcontrolinfo = cmd_time;
		m_first_setcontrolinfo = cmd_time; // new set request, so mark the beginning
	}
}

void CDaikin::HTTPSetControlInfo()
{
	std::string sResult;
	Debug(DEBUG_HARDWARE, "HTTPSetControlInfo ...");
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
	szURL << "pow=" << m_sci_OnOFF;
	szURL << "&mode=" << m_sci_ModeLevel;
	szURL << "&stemp=" << m_sci_Temp;
	szURL << "&shum=" << m_sci_Hum;
	szURL << "&f_rate=" << m_sci_FRateLevel;
	szURL << "&f_dir=" << m_sci_FDirLevel;

	if (!HTTPClient::GET(szURL.str(), sResult))
	{
		Log(LOG_ERROR, "Error connecting to: %s", m_szIPAddress.c_str());
		return;
	}
#ifdef DEBUG_DaikinW
	SaveString2Disk(sResult, "E:\\daikin_get_control_info.txt");
#endif

	if (sResult.find("ret=OK") == std::string::npos)
	{
		Log(LOG_ERROR, "Invalid response");
		return;
	}
	// once http sci request is done and OK, update internal values
	m_pow = m_sci_OnOFF;
	m_mode = m_sci_ModeLevel;
	m_stemp = m_sci_Temp;
	m_shum = m_sci_Hum;
	m_f_rate = m_sci_FRateLevel;
	m_f_dir = m_sci_FDirLevel;
}
