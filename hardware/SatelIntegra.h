#pragma once

// implememtation for Security System : https://www.satel.pl/en/cat/2#cat15
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"

class SatelIntegra : public CDomoticzHardwareBase
{
public:
	SatelIntegra(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode, const int pollInterval);
	virtual ~SatelIntegra();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void Do_Work();

	bool CheckAddress();
	// Closes socket
	void DestroySocket();
	// Connects socket
	bool ConnectToIntegra();
	// new data is collected in Integra for selected command
	bool ReadNewData();
	// Gets info from hardware about PCB, ETHM1 etc
	bool GetInfo();
	// Reads and reports zones violation
	bool ReadZonesState(const bool firstTime = false);
	// Reads and reports temperatures
	bool ReadTemperatures(const bool firstTime = false);
	// Reads and reports state of outputs
	bool ReadOutputsState(const bool firstTime = false);
	// Read state of arming
	bool ReadArmState(const bool firstTime = false);
	// Read alarm
	bool ReadAlarm(const bool firstTime = false);
	// Read events
	bool ReadEvents();
	// Updates temperature name and type in database
	void UpdateTempName(const int Idx, const unsigned char* name, const int partition);
	// Updates zone name and type in database
	void UpdateZoneName(const int Idx, const unsigned char* name, const int partition);
	// Updates output name and type in database
	void UpdateOutputName(const int Idx, const unsigned char* name, const _eSwitchType switchType);
	// Updates output name for virtual in/out (arming ald alarm)
	void UpdateAlarmAndArmName();
	// Reports zones states to domoticz
	void ReportZonesViolation(const int Idx, const bool violation);
	// Reports output states to domoticz
	void ReportOutputState(const int Idx, const bool state);
	// Reports arm state to domoticz
	void ReportArmState(const int Idx, const bool isArm);
	// Reports alarms to domoticz
	void ReportAlarm(const bool isAlarm);
	// Reports temperatures to domoticz
	void ReportTemperature(const int Idx, const int temp);
	// arms given partitions
	bool ArmPartitions(const int  partition, const int mode = 0);
	// disarms given partitions
	bool DisarmPartitions(const int partition);

	// convert string from iso to utf8
	std::string ISO2UTF8(const std::string &name);

	std::pair<unsigned char*, unsigned int> getFullFrame(const unsigned char* pCmd, const unsigned int cmdLength);
	int SendCommand(const unsigned char* cmd, const unsigned int cmdLength, unsigned char *answer, const unsigned int expectedLength1, const unsigned int expectedLength2 = -1);
private:
	int m_modelIndex;
	bool m_data32;
	sockaddr_in m_addr;
	int m_socket;
	const unsigned short m_IPPort;
	const std::string m_IPAddress;
	int m_pollInterval;
	volatile bool m_stoprequested;
	std::shared_ptr<std::thread> m_thread;
	std::map<unsigned int, const char*> errorCodes;
	// filled by 0x7F command
	unsigned char m_newData[7];

	// password to Integra
	unsigned char m_userCode[8];

	//TODO maybe create dynamic array ?
	bool m_zonesLastState[256];
	bool m_outputsLastState[256];
	bool m_isOutputSwitch[256];
	bool m_isTemperature[256];
	bool m_isPartitions[32];
	bool m_armLastState[32];

	// thread-safe for read and write
	std::mutex m_mutex;

	bool m_alarmLast;
};
