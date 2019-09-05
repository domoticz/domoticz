#pragma once

#include "DomoticzHardware.h"
#include "csocket.h"

class bt_openwebnet;

class COpenWebNetTCP : public CDomoticzHardwareBase
{
	enum _eArea {
		WHERE_CEN_0 = 0,
		WHERE_AREA_1 = 1,
		WHERE_AREA_2 = 2,
		WHERE_AREA_3 = 3,
		WHERE_AREA_4 = 4,
		WHERE_AREA_5 = 5,
		WHERE_AREA_6 = 6,
		WHERE_AREA_7 = 7,
		WHERE_AREA_8 = 8,
		WHERE_AREA_9 = 9,
		MAX_WHERE_AREA = 10
		/*
		TODO: with virtual configuration are present PL [10 - 15]
		but in this case need to develop all this sending rules:

		- A = 00;			PL [01 - 15];
		- A [1 -9];		PL [1 - 9];
		- A = 10;			PL [01 - 15];
		- A [01 - 09];	PL [10 - 15];

		*/
	};

	enum _eWhereEnergy {
		WHERE_ENERGY_1 = 51,
		WHERE_ENERGY_2 = 52,
		WHERE_ENERGY_3 = 53,
		WHERE_ENERGY_4 = 54,
		WHERE_ENERGY_5 = 55,
		WHERE_ENERGY_6 = 56,
		MAX_WHERE_ENERGY = 57
	};
public:
	COpenWebNetTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &ownPassword, const int ownScanTime);
	~COpenWebNetTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	bool SetSetpoint(const int idx, const float temp);
	boost::signals2::signal<void()>	sDisconnected;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool isStatusSocketConnected();
	void Do_Work();
	void MonitorFrames();
	uint32_t ownCalcPass(const std::string &password, const std::string &nonce);
	bool ownAuthentication(csocket* connectionSocket);
	bool nonceHashAuthentication(csocket *connectionSocket, std::string nonce);
	const std::string decToHexStrConvert(std::string paramString);
	const std::string hexToDecStrConvert(std::string paramString);
	const std::string byteToHexStrConvert(uint8_t* digest, size_t digestLen, char* pArray);
	const std::string shaCalc(std::string paramString, int auth_type);
	bool hmacAuthentication(csocket* connectionSocket, int auth_type);
	csocket* connectGwOwn(const char *connectionMode);
	void disconnect();
	bool ownWrite(csocket* connectionSocket, const char* pdata, size_t size);
	int ownRead(csocket* connectionSocket, char* pdata, size_t size);
	bool sendCommand(bt_openwebnet& command, std::vector<bt_openwebnet>& response, int waitForResponse = 0, bool silent = false);
	bool ParseData(char* data, int length, std::vector<bt_openwebnet>& messages);
	void UpdateSwitch(const int who, const int where, const int Level, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateBlinds(const int who, const int where, const int Command, const int iInterface, const int iLevel, const int BatteryLevel, const char *devname);
	void UpdateAlarm(const int who, const int where, const int Command, const char *sCommand, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateCenPlus(const int who, const int where, const int Command, const int iAppValue, const int what, const int iInterface, const int BatteryLevel, const char* devname);
	void UpdateTemp(const int who, const int where, float fval, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateSetPoint(const int who, const int where, float fval, const int iInterface, const char *devname);
	void UpdatePower(const int who, const int where, double fval, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateEnergy(const int who, const int where, double fval, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateSoundDiffusion(const int who, const int where, const int what, const int iInterface, const int BatteryLevel, const char* devname);
	bool GetValueMeter(const int NodeID, const int ChildID, double *usage, double *energy);
	void UpdateDeviceValue(std::vector<bt_openwebnet>::iterator iter);
	void scan_automation_lighting(const int cen_area);
	void scan_sound_diffusion();
	void scan_temperature_control();
	void scan_device();
	void requestTime();
	void setTime();
	void requestBurglarAlarmStatus();
	void requestDryContactIRDetectionStatus();
	void requestEnergyTotalizer();
	void requestAutomaticUpdatePower(int time);
private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_ownPassword;
	unsigned short m_ownScanTime;

	time_t LastScanTime, LastScanTimeEnergy, LastScanTimeEnergyTot;

	std::shared_ptr<std::thread> m_monitorThread;
	std::shared_ptr<std::thread> m_heartbeatThread;
	volatile uint32_t mask_request_status;
	int m_heartbeatcntr;
	csocket* m_pStatusSocket;
	std::mutex readQueueMutex;
};
