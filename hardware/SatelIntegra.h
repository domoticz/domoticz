#pragma once

// implememtation for Security System : https://www.satel.pl/en/cat/2#cat15
// by Fantom (szczukot@poczta.onet.pl)

#include <map>
#include "DomoticzHardware.h"

class SatelIntegra : public CDomoticzHardwareBase
{
public:
	SatelIntegra(const int ID, const std::string &IPAddress, const unsigned short IPPort, const std::string& userCode);
	virtual ~SatelIntegra();

	bool WriteToHardware(const char *pdata, const unsigned char length);

private:

	int m_modelIndex;
	bool m_data32;
	sockaddr_in m_addr;
	int m_socket;
	const unsigned short m_IPPort;
	const std::string m_IPAddress;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
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

	// thread-safe for read and write
	boost::mutex m_mutex;

	bool m_armLast;
	bool m_alarmLast;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();

	bool CheckAddress();
	// Closes socket
	void DestroySocket();
	// Connects socket
	bool ConnectToIntegra();
	// new data is collected in Integra for selected command
	bool IsNewData();
	// Gets info from hardware about PCB, ETHM1 etc
	bool GetInfo();
	// Reads and reports zones violation
	bool ReadZonesState(const bool firstTime = false);
	// Reads and reports state of outputs
	bool ReadOutputsState(const bool firstTime = false);
	// Read state of arming
	bool ReadArmState(const bool firstTime = false);
	// Read alarm
	bool ReadAlarm(const bool firstTime = false);
	// Updates zone name and type in database
	void UpdateZoneName(const unsigned int Idx, const unsigned char* name, const unsigned int partition);
	// Updates output name and type in database
	void UpdateOutputName(const unsigned int Idx, const unsigned char* name, const bool switchable);
	// Updates output name for virtual in/out (arming ald alarm)
	void UpdateAlarmAndArmName();
	// Reports zones states to domoticz
	void ReportZonesViolation(const unsigned long Idx, const bool violation);
	// Reports output states to domoticz
	void ReportOutputState(const unsigned long Idx, const bool state);
	// Reports arm state to domoticz
	void ReportArmState(const bool isArm);
	// Reports alarms to domoticz
	void ReportAlarm(const bool isAlarm);
	// Reports temperatures to domoticz
	void ReportTemperature(const unsigned long Idx, unsigned int temp);
	// arms given partitions
	bool ArmPartitions(const unsigned char* partitions, const unsigned int mode = 0);
	// disarms given partitions
	bool DisarmPartitions(const unsigned char* partitions);

	std::pair<unsigned char*, unsigned int> getFullFrame(const unsigned char* pCmd, unsigned int cmdLength);
	int SendCommand(const unsigned char* cmd, unsigned int cmdLength, unsigned char *answer);
};
