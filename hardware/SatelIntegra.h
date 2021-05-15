#pragma once

// implememtation for Security System : https://www.satel.pl/en/cat/2#cat15
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"

class SatelIntegra : public CDomoticzHardwareBase
{
      public:
	SatelIntegra(int ID, const std::string &IPAddress, unsigned short IPPort, const std::string &userCode, int pollInterval);
	~SatelIntegra() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

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
	bool ReadZonesState(bool firstTime = false);
	// Reads and reports temperatures
	bool ReadTemperatures(bool firstTime = false);
	// Reads and reports state of outputs
	bool ReadOutputsState(bool firstTime = false);
	// Read state of arming
	bool ReadArmState(bool firstTime = false);
	// Read alarm
	bool ReadAlarm(bool firstTime = false);
	// Read events
	bool ReadEvents();
	// Updates temperature name and type in database
	void UpdateTempName(int Idx, const unsigned char *name, int partition);
	// Updates zone name and type in database
	void UpdateZoneName(int Idx, const unsigned char *name, int partition);
	// Updates output name and type in database
	void UpdateOutputName(int Idx, const unsigned char *name, _eSwitchType switchType);
	// Updates output name for virtual in/out (arming ald alarm)
	void UpdateAlarmAndArmName();
	// Reports zones states to domoticz
	void ReportZonesViolation(int Idx, bool violation);
	// Reports output states to domoticz
	void ReportOutputState(int Idx, bool state);
	// Reports arm state to domoticz
	void ReportArmState(int Idx, bool isArm);
	// Reports alarms to domoticz
	void ReportAlarm(bool isAlarm);
	// Reports temperatures to domoticz
	void ReportTemperature(int Idx, int temp);
	// arms given partitions
	bool ArmPartitions(int partition, int mode = 0);
	// disarms given partitions
	bool DisarmPartitions(int partition);

	// convert string from iso to utf8
	std::string ISO2UTF8(const std::string &name);

	std::pair<unsigned char *, unsigned int> getFullFrame(const unsigned char *pCmd, unsigned int cmdLength);
	int SendCommand(const unsigned char *cmd, unsigned int cmdLength, unsigned char *answer, int expectedLength);

      private:
	int m_modelIndex;
	bool m_data32;
	sockaddr_in m_addr;
	int m_socket;
	const unsigned short m_IPPort;
	const std::string m_IPAddress;
	int m_pollInterval;
	std::shared_ptr<std::thread> m_thread;
	std::map<unsigned int, const char *> errorCodes;
	// filled by 0x7F command
	unsigned char m_newData[7];

	// password to Integra
	unsigned char m_userCode[8];

	// TODO maybe create dynamic array ?
	std::array<bool, 256> m_zonesLastState;
	std::array<bool, 256> m_outputsLastState;
	std::array<bool, 256> m_isOutputSwitch;
	std::array<bool, 256> m_isTemperature;
	std::array<bool, 32> m_isPartitions;
	std::array<bool, 32> m_armLastState;

	// thread-safe for read and write
	std::mutex m_mutex;

	bool m_alarmLast;
};
