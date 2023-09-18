#include "stdafx.h"
#include "MitsubishiWF.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../httpclient/HTTPClient.h"
#include "../httpclient/UrlEncode.h"
#include "../main/json_helper.h"
#include "../main/mainworker.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../notifications/NotificationHelper.h"
#include "../tinyxpath/tinyxml.h"
#include "../webserver/Base64.h"
#include "hardwaretypes.h"
#include <iostream>


#ifdef _DEBUG
//#define DEBUG_MitsubishiWF_R
//#define DEBUG_MitsubishiWF_W
#endif

/*
Example
{"command":"getAirconStat","apiVer":"1.0","operatorId":"4529e305-8472-456f-95f5-8cc47dfb3851","deviceId":"1234567890ABCDEF","timestamp":1649703587,"result":1,"contents":{"airconId":"xxxxxx","airconStat":"AACqmK7/AAAIAIASCgAAAAAAAf////8AO4EECBAupwAQiAAAAgAAAAAAAAOAIKf/gBDM/5QQAAC5zQ==","logStat":0,"updatedBy":"4529e305-8472-456f-95f5-8cc47dfb3851","expires":1691650615,"ledStat":1,"autoHeating":0,"highTemp":"AB","lowTemp":"66","firmType":"WF-RAC","wireless":{"firmVer":"010"},"mcu":{"firmVer":"123"},"timezone":"Europe/Amsterdam","remoteList":["4529e305-8472-456f-95f5-8cc47dfb3851","","",""],"numOfAccount":1}}
*/

/*
Based on the work of:
- https://github.com/jeatheak/Mitsubishi-WF-RAC-Integration
- Mitsubishi M-Air application
*/

#ifdef DEBUG_MitsubishiWF_W
void SaveString2Disk(const std::string& str, const std::string& filename)
{
	FILE* fOut = fopen(filename.c_str(), "wb+");
	if (fOut)
	{
		fwrite(str.c_str(), 1, str.size(), fOut);
		fclose(fOut);
	}
}
#endif

#ifdef DEBUG_MitsubishiWF_R
std::string ReadFile(const std::string& filename)
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

MitsubishiWF::MitsubishiWF(const int ID, const std::string& IPAddress, const unsigned short usIPPort, int PollInterval) :
	m_szIPAddress(IPAddress)
{
	m_HwdID = ID;
	m_usIPPort = usIPPort;

	if (PollInterval < 5)
		PollInterval = 60;
	if (PollInterval > 300)
		PollInterval = 300;
	m_poll_interval = PollInterval;
}

bool MitsubishiWF::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);
	return (m_thread != nullptr);
}

bool MitsubishiWF::StopHardware()
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

void MitsubishiWF::Do_Work()
{
	Log(LOG_STATUS, "Worker started...");
	int sec_counter = m_poll_interval - 3;

	bool bHaveRunOnce = false;

	while (!IsStopRequested(1000))
	{
		sec_counter++;

		if (sec_counter % 12 == 0) {
			m_LastHeartbeat = mytime(nullptr);
		}

		if (sec_counter % m_poll_interval == 0)
		{
			try
			{
				GetAircoStatus();
			}
			catch (const std::exception& e)
			{
				Log(LOG_ERROR, "Error getting airco status: %s", e.what());
				return;
			}
		}
	}
	Log(LOG_STATUS, "Worker stopped...");
}

bool MitsubishiWF::WriteToHardware(const char* pdata, const unsigned char length)
{
	const tRBUF* pCmd = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pCmd->ICMND.packettype;
	unsigned char subtype = pCmd->ICMND.subtype;

	bool result = true;

	if (packettype == pTypeLighting2)
	{
		uint8_t Unit = pCmd->LIGHTING2.unitcode;
		uint8_t command = pCmd->LIGHTING2.cmnd;

		if (Unit == 2)
		{
			//Power on/off
			SetPowerActive(command == light2_sOn);
			return true;
		}
	}
	else if ((packettype == pTypeGeneralSwitch) && (subtype == sSwitchGeneralSwitch))
	{
		const _tGeneralSwitch* pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		int unit_id = pSwitch->id;
		switch (unit_id)
		{
		case 2:
			//Operation Mode
			result = SetOperationMode(pSwitch->level);
			break;
		case 3:
			//Fan Mode
			result = SetFanMode(pSwitch->level);
			break;
		case 4:
			//Swing Mode Up/Down
			result = SetSwingUpDownMode(pSwitch->level);
			break;
		case 5:
			//Swing Mode Left/Right
			result = SetSwingLeftRightMode(pSwitch->level);
			break;
		}
	}
	else if ((packettype == pTypeSetpoint) && (subtype == sTypeSetpoint))
	{
		// Set Point
		const _tSetpoint* pMeter = reinterpret_cast<const _tSetpoint*>(pCmd);
		int node_id = pMeter->id2;
		// int child_sensor_id = pMeter->id3;
		result = SetSetpoint(node_id, pMeter->value);
	}

	return result;
}

bool MitsubishiWF::Execute_Command(const std::string& sCommand, std::string& sResult)
{
	Json::Value contents;
	return Execute_Command(sCommand, contents, sResult);
}

bool MitsubishiWF::Execute_Command(const std::string& sCommand, const Json::Value contents, std::string& sResult)
{
#ifdef DEBUG_MitsubishiWF_R
	sResult = ReadFile(std_format("E:\\MitsubishiWF_%s.json", sCommand.c_str()).c_str());
#else
	std::stringstream sURL;
	sURL << "http://" << m_szIPAddress << ":" << m_usIPPort << "/beaver/command/" << sCommand;

	Json::Value post_data;
	post_data["apiVer"] = m_api_version;
	post_data["command"] = sCommand;
	post_data["deviceId"] = m_device_id;
	post_data["operatorId"] = m_operator_id;
	post_data["timestamp"] = (int)time(nullptr);

	if (!contents.empty())
	{
		post_data["contents"] = contents;
	}
	std::string szPostData = post_data.toStyledString();
	stdreplace(szPostData, " : ", ": "); //fix json parsing issue!?!

	std::vector<std::string> ExtraHeaders;
	if (!HTTPClient::POST(sURL.str(), szPostData, ExtraHeaders, sResult))
	{
		Log(LOG_ERROR, "Error executing HTTP command (%s)", sCommand.c_str());
		return false;
	}
#endif
#ifdef DEBUG_MitsubishiWF_W
	SaveString2Disk(sResult, std_format("E:\\MitsubishiWF_%s.json", sCommand.c_str()).c_str());
#endif
	return true;
}

void MitsubishiWF::GetAircoStatus()
{
	std::string sResult;

	if (!Execute_Command("getAirconStat", sResult))
		return;

	HandleAircoStatus(sResult);
}

bool MitsubishiWF::HandleAircoStatus(const std::string& sResult)
{
	Json::Value root;
	std::string szDecoded;
	std::string sAircoStat;

	bool ret = ParseJSon(sResult, root);
	if ((!ret) || (!root.isObject()))
	{
		goto error_exit;
	}

	if (root["contents"].empty())
	{
		goto error_exit;
	}
	if (root["contents"]["airconStat"].empty())
	{
		goto error_exit;
	}

	if (m_airconId.empty())
	{
		//First time
		m_airconId = root["contents"]["airconId"].asString();
		if (!root["contents"]["remoteList"].empty())
		{
			m_operator_id = root["contents"]["remoteList"][0].asString();
		}
	}
	sAircoStat = root["contents"]["airconStat"].asString();
	//Base64 decode
	szDecoded = base64_decode(sAircoStat);

	TranslateAirconStat(szDecoded, m_AircoStatus);
	if (m_AircoStatus.PresetTemp != 0)
	{
		ParseAirconStat(m_AircoStatus);
	}
	return true;
error_exit:
	Log(LOG_ERROR, "Invalid data received! (GetAircoStatus)");
	return false;
}

static int find_match(const int8_t content, int8_t inputMatrix[], const size_t len)
{
	//Simple method for finding a match from inputMatrix
	for (int i = 0; i<int(len); i++)
	{
		if (content == inputMatrix[i])
		{
			return i;
		}
	}
	return -1;
}

constexpr std::array <double, 256> outdoorTempList {
	-50.0,
		-50.0,
		-50.0,
		-50.0,
		-50.0,
		-48.9,
		-46.0,
		-44.0,
		-42.0,
		-41.0,
		-39.0,
		-38.0,
		-37.0,
		-36.0,
		-35.0,
		-34.0,
		-33.0,
		-32.0,
		-31.0,
		-30.0,
		-29.0,
		-28.5,
		-28.0,
		-27.0,
		-26.0,
		-25.5,
		-25.0,
		-24.0,
		-23.5,
		-23.0,
		-22.5,
		-22.0,
		-21.5,
		-21.0,
		-20.5,
		-20.0,
		-19.5,
		-19.0,
		-18.5,
		-18.0,
		-17.5,
		-17.0,
		-16.5,
		-16.0,
		-15.5,
		-15.0,
		-14.6,
		-14.3,
		-14.0,
		-13.5,
		-13.0,
		-12.6,
		-12.3,
		-12.0,
		-11.5,
		-11.0,
		-10.6,
		-10.3,
		-10.0,
		-9.6,
		-9.3,
		-9.0,
		-8.6,
		-8.3,
		-8.0,
		-7.6,
		-7.3,
		-7.0,
		-6.6,
		-6.3,
		-6.0,
		-5.6,
		-5.3,
		-5.0,
		-4.6,
		-4.3,
		-4.0,
		-3.7,
		-3.5,
		-3.2,
		-3.0,
		-2.6,
		-2.3,
		-2.0,
		-1.7,
		-1.5,
		-1.2,
		-1.0,
		-0.6,
		-0.3,
		0.0,
		0.2,
		0.5,
		0.7,
		1.0,
		1.3,
		1.6,
		2.0,
		2.2,
		2.5,
		2.7,
		3.0,
		3.2,
		3.5,
		3.7,
		4.0,
		4.2,
		4.5,
		4.7,
		5.0,
		5.2,
		5.5,
		5.7,
		6.0,
		6.2,
		6.5,
		6.7,
		7.0,
		7.2,
		7.5,
		7.7,
		8.0,
		8.2,
		8.5,
		8.7,
		9.0,
		9.2,
		9.5,
		9.7,
		10.0,
		10.2,
		10.5,
		10.7,
		11.0,
		11.2,
		11.5,
		11.7,
		12.0,
		12.2,
		12.5,
		12.7,
		13.0,
		13.2,
		13.5,
		13.7,
		14.0,
		14.2,
		14.4,
		14.6,
		14.8,
		15.0,
		15.2,
		15.5,
		15.7,
		16.0,
		16.2,
		16.5,
		16.7,
		17.0,
		17.2,
		17.5,
		17.7,
		18.0,
		18.2,
		18.5,
		18.7,
		19.0,
		19.2,
		19.4,
		19.6,
		19.8,
		20.0,
		20.2,
		20.5,
		20.7,
		21.0,
		21.2,
		21.5,
		21.7,
		22.0,
		22.2,
		22.5,
		22.7,
		23.0,
		23.2,
		23.5,
		23.7,
		24.0,
		24.2,
		24.5,
		24.7,
		25.0,
		25.2,
		25.5,
		25.7,
		26.0,
		26.2,
		26.5,
		26.7,
		27.0,
		27.2,
		27.5,
		27.7,
		28.0,
		28.2,
		28.5,
		28.7,
		29.0,
		29.2,
		29.5,
		29.7,
		30.0,
		30.2,
		30.5,
		30.7,
		31.0,
		31.3,
		31.6,
		32.0,
		32.2,
		32.5,
		32.7,
		33.0,
		33.2,
		33.5,
		33.7,
		34.0,
		34.3,
		34.6,
		35.0,
		35.2,
		35.5,
		35.7,
		36.0,
		36.3,
		36.6,
		37.0,
		37.2,
		37.5,
		37.7,
		38.0,
		38.3,
		38.6,
		39.0,
		39.3,
		39.6,
		40.0,
		40.3,
		40.6,
		41.0,
		41.3,
		41.6,
		42.0,
		42.3,
		42.6,
		43.0,
};

constexpr std::array <double, 256> indoorTempList {
	-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-30.0,
		-29.0,
		-28.0,
		-27.0,
		-26.0,
		-25.0,
		-24.0,
		-23.0,
		-22.5,
		-22.0,
		-21.0,
		-20.0,
		-19.5,
		-19.0,
		-18.0,
		-17.5,
		-17.0,
		-16.5,
		-16.0,
		-15.0,
		-14.5,
		-14.0,
		-13.5,
		-13.0,
		-12.5,
		-12.0,
		-11.5,
		-11.0,
		-10.5,
		-10.0,
		-9.5,
		-9.0,
		-8.6,
		-8.3,
		-8.0,
		-7.5,
		-7.0,
		-6.5,
		-6.0,
		-5.6,
		-5.3,
		-5.0,
		-4.5,
		-4.0,
		-3.6,
		-3.3,
		-3.0,
		-2.6,
		-2.3,
		-2.0,
		-1.6,
		-1.3,
		-1.0,
		-0.5,
		0.0,
		0.3,
		0.6,
		1.0,
		1.3,
		1.6,
		2.0,
		2.3,
		2.6,
		3.0,
		3.2,
		3.5,
		3.7,
		4.0,
		4.3,
		4.6,
		5.0,
		5.3,
		5.6,
		6.0,
		6.3,
		6.6,
		7.0,
		7.2,
		7.5,
		7.7,
		8.0,
		8.3,
		8.6,
		9.0,
		9.2,
		9.5,
		9.7,
		10.0,
		10.3,
		10.6,
		11.0,
		11.2,
		11.5,
		11.7,
		12.0,
		12.3,
		12.6,
		13.0,
		13.2,
		13.5,
		13.7,
		14.0,
		14.2,
		14.5,
		14.7,
		15.0,
		15.3,
		15.6,
		16.0,
		16.2,
		16.5,
		16.7,
		17.0,
		17.2,
		17.5,
		17.7,
		18.0,
		18.2,
		18.5,
		18.7,
		19.0,
		19.2,
		19.5,
		19.7,
		20.0,
		20.2,
		20.5,
		20.7,
		21.0,
		21.2,
		21.5,
		21.7,
		22.0,
		22.2,
		22.5,
		22.7,
		23.0,
		23.2,
		23.5,
		23.7,
		24.0,
		24.2,
		24.5,
		24.7,
		25.0,
		25.2,
		25.5,
		25.7,
		26.0,
		26.2,
		26.5,
		26.7,
		27.0,
		27.2,
		27.5,
		27.7,
		28.0,
		28.2,
		28.5,
		28.7,
		29.0,
		29.2,
		29.5,
		29.7,
		30.0,
		30.2,
		30.5,
		30.7,
		31.0,
		31.3,
		31.6,
		32.0,
		32.2,
		32.5,
		32.7,
		33.0,
		33.2,
		33.5,
		33.7,
		34.0,
		34.2,
		34.5,
		34.7,
		35.0,
		35.3,
		35.6,
		36.0,
		36.2,
		36.5,
		36.7,
		37.0,
		37.2,
		37.5,
		37.7,
		38.0,
		38.3,
		38.6,
		39.0,
		39.2,
		39.5,
		39.7,
		40.0,
		40.3,
		40.6,
		41.0,
		41.2,
		41.5,
		41.7,
		42.0,
		42.3,
		42.6,
		43.0,
		43.2,
		43.5,
		43.7,
		44.0,
		44.3,
		44.6,
		45.0,
		45.3,
		45.6,
		46.0,
		46.2,
		46.5,
		46.7,
		47.0,
		47.3,
		47.6,
		48.0,
		48.3,
		48.6,
		49.0,
		49.3,
		49.6,
		50.0,
		50.3,
		50.6,
		51.0,
		51.3,
		51.6,
		52.0,
};

void MitsubishiWF::TranslateAirconStat(const std::string& szStat, _tAircoStatus& aircoStatus)
{
	if (szStat.length() < 21)
	{
		return;
	}

	int8_t content_byte_array[1024];
	memcpy(content_byte_array, szStat.c_str(), szStat.length());

	//Convert to signed integers instead of bytes
	for (int i = 0; i < (int)szStat.length(); i++)
	{
		if (content_byte_array[i] > 127)
		{
			content_byte_array[i] = (256 - content_byte_array[i]) * (-1);
		}
	}

	//get te start of the first bytearray segment we use
	size_t start_length = content_byte_array[18] * 4 + 21;

	if (szStat.length() < start_length + 18)
	{
		return;
	}

	int8_t* content = content_byte_array + start_length;// [start_length:start_length + 18]

	// get the current ac operation(3th value and with byte 3)
	//0=Off, 1=On
	aircoStatus.Operation = (3 & content[2]);

	// get preset temp : 5th byte divided by 2
	aircoStatus.PresetTemp = double(content[4]) / 2.0; //target_temperature
	// get operation mode : check if 3th byte and byte 60 matches 8, 16, 12 or 4 (add 1)
	int8_t operationModeMatrix[] = { 8, 16, 12, 4 };
	aircoStatus.OperationMode = find_match(60 & content[2], operationModeMatrix, sizeof(operationModeMatrix)) + 1;

	int8_t airFlowMatrix[] = { 7, 0, 1, 2, 6 };
	aircoStatus.AirFlow = find_match(15 & content[3], airFlowMatrix, sizeof(airFlowMatrix));

	int8_t windDirectionUDMatrix[] = { 0, 16, 32, 48 };
	aircoStatus.WindDirectionUD = ((content[2] & 192) == 64) ? 0 : find_match(240 & content[3], windDirectionUDMatrix, sizeof(windDirectionUDMatrix)) + 1;

	int8_t windDirectionLRMatrix[] = { 0, 1, 2, 3, 4, 5, 6 };
	aircoStatus.WindDirectionLR = ((content[12] & 3) == 1) ? 0 : find_match(31 & content[11], windDirectionLRMatrix, sizeof(windDirectionLRMatrix)) + 1;

	aircoStatus.Entrust = 4 == (12 & content[12]);
	aircoStatus.CoolHotJudge = (content[8] & 8) <= 0;

	int8_t modelMatrix[] = { 0, 1, 2 };
	aircoStatus.ModelNr = find_match(content[0] & 127, modelMatrix, sizeof(modelMatrix));

	aircoStatus.Vacant = content[10] & 1;

	aircoStatus.code = content[6] & 127;

	if (aircoStatus.code == 0)
		aircoStatus.szErrorCode = "00";
	else if ((content[6] & -128) <= 0)
		aircoStatus.szErrorCode = "M" + std::to_string(aircoStatus.code);
	else
		aircoStatus.szErrorCode = "E" + std::to_string(aircoStatus.code);

	int8_t* vals = content_byte_array + start_length + 19;// len(content_byte_array) - 2
	int len = (int)szStat.size() - 2 - start_length - 19;

	for (int i = 0; i < len; i += 4)
	{
		if (vals[i] == -128 && vals[i + 1] == 16)
		{
			aircoStatus.OutdoorTemp = (float)outdoorTempList[vals[i + 2] & 0xFF];
			aircoStatus.bHaveOutdoorTemp = true;
		}
		else if (vals[i] == -128 && vals[i + 1] == 32)
		{
			aircoStatus.IndoorTemp = (float)indoorTempList[vals[i + 2] & 0xFF]; //current_temperature
			aircoStatus.bHaveIndoorTemp = true;
		}
		else if (vals[i] == -108 && vals[i + 1] == 16)
		{
			uint16_t usVal = (vals[i + 3] << 8) | vals[i + 2];
			aircoStatus.Electric_kWh_Used = (double)(usVal) * 0.25;
			aircoStatus.bHaveElectric = true;
		}
	}
}

constexpr std::array<const char*, 5> szOperationMode {
	"Auto",
		"Cool",
		"Heat",
		"Fan Only",
		"Dry"
};

constexpr std::array<const char*, 5> szFanMode {
	"Auto",
		"Lowest",
		"Low",
		"High",
		"Highest"
};
constexpr std::array<const char*, 6> szSwingModeVerticle {
	"Auto Up/Down",
		"Highest",
		"Middle",
		"Normal",
		"Lowest",
		"3D Auto"
};

constexpr std::array<const char*, 8> szSwingModeHorizontal {
	"Auto Left/Right",
		"Left-Left",
		"Left-Center",
		"Center-Center",
		"Center-Right",
		"Right-Right",
		"Left-Right",
		"Right-Left"
};

void MitsubishiWF::ParseAirconStat(const _tAircoStatus& aircoStatus)
{
	SendSwitch(1, 2, 255, aircoStatus.Operation, 0, "Airco Power", "MitsubishiWF");

	SendSetPointSensor(20, 1, 1, static_cast<float>(aircoStatus.PresetTemp), "Target Setpoint");

	std::string szDeviceName = "Operation Mode";
	ParseModeSwitch(2, (const char**)&szOperationMode, szOperationMode.size(), aircoStatus.OperationMode, true, szDeviceName);

	szDeviceName = "Fan Mode";
	ParseModeSwitch(3, (const char**)&szFanMode, szFanMode.size(), aircoStatus.AirFlow, true, szDeviceName);

	szDeviceName = "Swing Mode Up/Down";
	ParseModeSwitch(4, (const char**)&szSwingModeVerticle, szSwingModeVerticle.size(), aircoStatus.WindDirectionUD, false, szDeviceName);

	szDeviceName = "Swing Mode Left/Right";
	ParseModeSwitch(5, (const char**)&szSwingModeHorizontal, szSwingModeHorizontal.size(), aircoStatus.WindDirectionLR, false, szDeviceName);
	/*
		if (aircoStatus.Entrust)
		{
			std::cout << "Entrust mode is on, Auto Up/Down swing mode\n";
		}

		std::cout << std_format("CoolHotJudge: %s\n", (aircoStatus.CoolHotJudge == true) ? "True" : "False");

		//aircoStatus.ModelNr;

		std::cout << std_format("Airco Vacant status: %s\n", aircoStatus.Vacant ? "Vacant" : "Occupied");

		if (aircoStatus.code != 0)
			std::cout << std_format("Error code: %s\n", aircoStatus.szErrorCode.c_str());
	*/
	if (aircoStatus.bHaveOutdoorTemp)
	{
		SendTempSensor(1, 255, static_cast<float>(aircoStatus.OutdoorTemp), "Outside temperature");
	}

	if (aircoStatus.bHaveIndoorTemp)
	{
		SendTempSensor(2, 255, static_cast<float>(aircoStatus.IndoorTemp), "Inside temperature");
	}
	
	if (aircoStatus.bHaveElectric)
	{
		SendKwhMeter(1, 1, 255, 0, aircoStatus.Electric_kWh_Used, "Electricity used");
	}
}

uint64_t MitsubishiWF::UpdateValueInt(const char* ID, unsigned char unit, unsigned char devType, unsigned char subType, unsigned char signallevel, unsigned char batterylevel, int nValue,
	const char* sValue, std::string& devname, bool bUseOnOffAction, const std::string& user)
{
	uint64_t DeviceRowIdx = m_sql.UpdateValue(m_HwdID, ID, unit, devType, subType, signallevel, batterylevel, nValue, sValue, devname, bUseOnOffAction, (!user.empty()) ? user.c_str() : m_Name.c_str());
	if (DeviceRowIdx == (uint64_t)-1)
		return -1;
	if (m_bOutputLog)
	{
		std::string szLogString = RFX_Type_Desc(devType, 1) + std::string("/") + std::string(RFX_Type_SubType_Desc(devType, subType)) + " (" + devname + ")";
		Log(LOG_NORM, szLogString);
	}
	m_mainworker.sOnDeviceReceived(m_HwdID, DeviceRowIdx, devname, nullptr);
	m_notifications.CheckAndHandleNotification(DeviceRowIdx, m_HwdID, ID, devname, unit, devType, subType, nValue, sValue);
	return DeviceRowIdx;
}

void MitsubishiWF::ParseModeSwitch(const uint8_t id, const char** vModes, const size_t cbModes, int currentMode, const bool bButtons, std::string& sName)
{
	int devType = pTypeGeneralSwitch;
	int subType = sSwitchGeneralSwitch;
	int switchType = STYPE_Selector;

	bool bIsNewDevice = false;

	uint8_t unit = id;
	std::string DeviceID = std::to_string(id);

	bool bValid = true;

	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, DeviceID.c_str(), devType, subType, unit);
	if (result.empty())
	{
		// New switch, add it to the system
		bIsNewDevice = true;
		int iUsed = 1;
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, switchType, SignalLevel, BatteryLevel, Name, Used, nValue, sValue, Options) "
			"VALUES (%d, '%q', %d, %d, %d, %d, %d, %d, '%q', %d, %d, '0', null)",
			m_HwdID, DeviceID.c_str(), unit, devType, subType, switchType, 12, 255, sName.c_str(), iUsed, 0);
		result = m_sql.safe_query("SELECT ID,Name,nValue,sValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d) AND (Unit==%d)", m_HwdID, DeviceID.c_str(), devType, subType, unit);
		if (result.empty())
			return; // should not happen!
	}

	std::string szIdx = result[0][0];
	uint64_t DevRowIdx = std::stoull(szIdx);
	std::string szDeviceName = result[0][1];
	int nValue = atoi(result[0][2].c_str());
	std::string sValue = result[0][3];
	std::string sOptions = result[0][4];

	int iActualIndex = -1;

	// Build switch options
	int iValueIndex = 0;
	std::string tmpOptionString;
	for (size_t i = 0; i < cbModes; i++)
	{
		if (i == currentMode)
			iActualIndex = iValueIndex;
		if (!tmpOptionString.empty())
			tmpOptionString += "|";
		tmpOptionString += vModes[i];
		iValueIndex += 10;
	}

	if (iActualIndex == -1)
	{
		bValid = false;
	}

	int new_nValue = 0;
	std::string new_sValue("");

	if (bValid)
	{
		std::map<std::string, std::string> optionsMap;
		optionsMap["SelectorStyle"] = (bButtons) ? "0" : "1";
		optionsMap["LevelOffHidden"] = "false";
		optionsMap["LevelNames"] = tmpOptionString;

		std::string newOptions = m_sql.FormatDeviceOptions(optionsMap);
		if (newOptions != sOptions)
			m_sql.SetDeviceOptions(DevRowIdx, optionsMap);

		new_nValue = (iActualIndex == 0) ? 0 : 2;
		new_sValue = std_format("%d", iActualIndex);

		if ((new_nValue != nValue) || (new_sValue != sValue))
		{
			UpdateValueInt(DeviceID.c_str(), unit, devType, subType, 12, 255, new_nValue, new_sValue.c_str(), sName);
		}
	}
}

uint16_t crc16ccitt(const char* ptr, int count)
{
	int i = 65535;
	for (int ll = 0; ll < count; ll++)
	{
		const char b = ptr[ll];
		for (int i2 = 0; i2 < 8; i2++)
		{
			bool z = true;
			bool z2 = ((b >> (7 - i2)) & 1) == 1;
			if (((i >> 15) & 1) != 1)
			{
				z = false;
			}
			i <<= 1;
			if (z2 ^ z) {
				i ^= 4129;
			}
		}
	}
	return i & 65535;
}


bool MitsubishiWF::command_to_byte(const _tAircoStatus& aircon_stat, std::string& sResult)
{
	//uint8_t stat_byte[18 + 2] = { 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
	uint8_t stat_byte[18 + 5 + 2] = { 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 255, 255, 255, 255, 0xFF, 0xFF };

	// On/Off
	if (aircon_stat.Operation)
		stat_byte[2] |= 3;
	else
		stat_byte[2] |= 2;

	// Operating Mode
	if (aircon_stat.OperationMode == 0)
		stat_byte[2] |= 32;
	else {
		if (aircon_stat.OperationMode == 1)
			stat_byte[2] |= 40;
		else if (aircon_stat.OperationMode == 2)
			stat_byte[2] |= 48;
		else if (aircon_stat.OperationMode == 3)
			stat_byte[2] |= 44;
		else if (aircon_stat.OperationMode == 4)
			stat_byte[2] |= 36;
	}

	// airflow
	if (aircon_stat.AirFlow == 0)
		stat_byte[3] |= 15;
	else if (aircon_stat.AirFlow == 1)
		stat_byte[3] |= 8;
	else if (aircon_stat.AirFlow == 2)
		stat_byte[3] |= 9;
	else if (aircon_stat.AirFlow == 3)
		stat_byte[3] |= 10;
	else if (aircon_stat.AirFlow == 4)
		stat_byte[3] |= 14;

	// Vertical wind direction
	if (aircon_stat.WindDirectionUD == 0)
	{
		stat_byte[2] |= 192;
		stat_byte[3] |= 128;
	}
	else if (aircon_stat.WindDirectionUD == 1)
	{
		stat_byte[2] |= 128;
		stat_byte[3] |= 128;
	}
	else if (aircon_stat.WindDirectionUD == 2)
	{
		stat_byte[2] |= 128;
		stat_byte[3] |= 144;
	}
	else if (aircon_stat.WindDirectionUD == 3)
	{
		stat_byte[2] |= 128;
		stat_byte[3] |= 160;
	}
	else if (aircon_stat.WindDirectionUD == 4)
	{
		stat_byte[2] |= 128;
		stat_byte[3] |= 176;
	}

	// Horizontal wind direction
	if (aircon_stat.WindDirectionLR == 0)
	{
		stat_byte[12] |= 3;
		stat_byte[11] |= 16;
	}
	else if (aircon_stat.WindDirectionLR == 1)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 16;
	}
	else if (aircon_stat.WindDirectionLR == 2)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 17;
	}
	else if (aircon_stat.WindDirectionLR == 3)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 18;
	}
	else if (aircon_stat.WindDirectionLR == 4)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 19;
	}
	else if (aircon_stat.WindDirectionLR == 5)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 20;
	}
	else if (aircon_stat.WindDirectionLR == 6)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 21;
	}
	else if (aircon_stat.WindDirectionLR == 7)
	{
		stat_byte[12] |= 2;
		stat_byte[11] |= 22;
	}

	// preset temp
	double preset_temp = (aircon_stat.OperationMode == 3) ? 25.0 : aircon_stat.PresetTemp;
	stat_byte[4] |= int(preset_temp / 0.5) + 128;

	// entrust
	if (aircon_stat.Entrust == 0)
		stat_byte[12] |= 8;
	else
		stat_byte[12] |= 12;

	if (aircon_stat.CoolHotJudge == 0)
		stat_byte[8] |= 8;

	if (aircon_stat.ModelNr == 1)
	{
		if (aircon_stat.Vacant)
			stat_byte[10] |= 1;
	}

	if (aircon_stat.ModelNr != 1 && aircon_stat.ModelNr != 2)
	{
	}
	else
	{
		if (aircon_stat.IsSelfCleanReset)
			stat_byte[10] |= 4;

		if (aircon_stat.IsSelfCleanOperation)
			stat_byte[10] |= 144;
		else
			stat_byte[10] |= 128;
	}

	uint16_t crc = crc16ccitt((char*)stat_byte, sizeof(stat_byte) - 2);
	stat_byte[sizeof(stat_byte) - 2] = crc & 0xFF;
	stat_byte[sizeof(stat_byte) - 1] = (crc >> 8) & 0xFF;

	sResult.clear();
	sResult.insert(sResult.end(), stat_byte, stat_byte + sizeof(stat_byte));

	return true;
}

bool MitsubishiWF::recieve_to_bytes(const _tAircoStatus& aircon_stat, std::string& sResult)
{
	//uint8_t stat_byte[18 + 2] = { 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
	uint8_t stat_byte[18 + 5 + 2] = { 0, 0, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 255, 255, 255, 255, 0xFF, 0xFF };

	// On/Off
	if (aircon_stat.Operation)
		stat_byte[2] |= 1;

	// Operating Mode
	if (aircon_stat.OperationMode == 1)
		stat_byte[2] |= 8;
	else if (aircon_stat.OperationMode == 2)
		stat_byte[2] |= 16;
	else if (aircon_stat.OperationMode == 3)
		stat_byte[2] |= 12;
	else if (aircon_stat.OperationMode == 4)
		stat_byte[2] |= 4;

	// airflow
	if (aircon_stat.AirFlow == 0)
		stat_byte[3] |= 7;
	else if (aircon_stat.AirFlow == 2)
		stat_byte[3] |= 1;
	else if (aircon_stat.AirFlow == 3)
		stat_byte[3] |= 2;
	else if (aircon_stat.AirFlow == 4)
		stat_byte[3] |= 6;

	//Vertical wind direction
	if (aircon_stat.WindDirectionUD == 0)
		stat_byte[2] |= 64;
	else if (aircon_stat.WindDirectionUD == 2)
		stat_byte[3] |= 16;
	else if (aircon_stat.WindDirectionUD == 3)
		stat_byte[3] |= 32;
	else if (aircon_stat.WindDirectionUD == 4)
		stat_byte[3] |= 48;

	// Horizontal wind direction
	if (aircon_stat.WindDirectionLR == 0)
		stat_byte[12] |= 1;
	else if (aircon_stat.WindDirectionLR == 1)
		stat_byte[11] |= 0;
	else if (aircon_stat.WindDirectionLR == 2)
		stat_byte[11] |= 1;
	else if (aircon_stat.WindDirectionLR == 3)
		stat_byte[11] |= 2;
	else if (aircon_stat.WindDirectionLR == 4)
		stat_byte[11] |= 3;
	else if (aircon_stat.WindDirectionLR == 5)
		stat_byte[11] |= 4;
	else if (aircon_stat.WindDirectionLR == 6)
		stat_byte[11] |= 5;
	else if (aircon_stat.WindDirectionLR == 7)
		stat_byte[11] |= 6;

	// preset temp
	double preset_temp = (aircon_stat.OperationMode == 3) ? 25.0 : aircon_stat.PresetTemp;
	stat_byte[4] |= int(preset_temp / 0.5);

	// entrust
	if (aircon_stat.Entrust)
		stat_byte[12] |= 4;

	if (!aircon_stat.CoolHotJudge)
		stat_byte[8] |= 8;

	if (aircon_stat.ModelNr == 1)
		stat_byte[0] |= 1;
	else if (aircon_stat.ModelNr == 2)
		stat_byte[0] |= 2;

	if (aircon_stat.ModelNr == 1)
	{
		if (aircon_stat.Vacant)
			stat_byte[10] |= 1;
	}

	if (aircon_stat.ModelNr != 1 && aircon_stat.ModelNr != 2)
	{
	}
	else
	{
		if (aircon_stat.IsSelfCleanOperation)
			stat_byte[15] |= 1;
	}

	uint16_t crc = crc16ccitt((char*)stat_byte, sizeof(stat_byte) - 2);
	stat_byte[sizeof(stat_byte) - 2] = crc & 0xFF;
	stat_byte[sizeof(stat_byte) - 1] = (crc >> 8) & 0xFF;

	sResult.clear();
	sResult.insert(sResult.end(), stat_byte, stat_byte + sizeof(stat_byte));

	return true;
}

bool MitsubishiWF::SendAircoStatus(const _tAircoStatus& aircoStatus)
{
	try
	{
		if (m_airconId.empty())
			return false;

		std::string sCommand2Byte;
		command_to_byte(aircoStatus, sCommand2Byte);

		std::string sReceive2Byte;
		recieve_to_bytes(aircoStatus, sReceive2Byte);

		std::string szStatString = base64_encode(sCommand2Byte + sReceive2Byte);

		Json::Value contents;
		contents["airconId"] = m_airconId;
		contents["airconStat"] = szStatString;

		std::string sResult;
		if (!Execute_Command("setAirconStat", contents, sResult))
			return false;

		//New status received in sResult, parse it
		return HandleAircoStatus(sResult);
	}
	catch (const std::exception& e)
	{
		Log(LOG_ERROR, "Error setting airco status: %s", e.what());
		return false;
	}
}

bool MitsubishiWF::SetSetpoint(const int /*idx*/, const float temp)
{
	m_AircoStatus.PresetTemp = temp;

	//If we are powered off, we need to power as well
	m_AircoStatus.Operation = true;

	SendSetPointSensor(20, 1, 1, temp, "Target Temperature"); // Suppose request succeed to keep reactive web interface

	return SendAircoStatus(m_AircoStatus);
}

bool MitsubishiWF::SetPowerActive(const bool bActive)
{
	m_AircoStatus.Operation = bActive;

	return SendAircoStatus(m_AircoStatus);
}

bool MitsubishiWF::SetOperationMode(const int level)
{
	int iOption = level / 10;
	m_AircoStatus.OperationMode = iOption;
	return SendAircoStatus(m_AircoStatus);
}

bool MitsubishiWF::SetFanMode(const int level)
{
	int iOption = level / 10;
	m_AircoStatus.AirFlow = iOption;
	return SendAircoStatus(m_AircoStatus);
}

bool MitsubishiWF::SetSwingUpDownMode(const int level)
{
	int iOption = level / 10;
	m_AircoStatus.WindDirectionUD = iOption;
	return SendAircoStatus(m_AircoStatus);
}

bool MitsubishiWF::SetSwingLeftRightMode(const int level)
{
	int iOption = level / 10;
	m_AircoStatus.WindDirectionLR = iOption;
	return SendAircoStatus(m_AircoStatus);
}
