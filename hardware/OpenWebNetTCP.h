#pragma once

#include "DomoticzHardware.h"
#include "csocket.h"

class bt_openwebnet;

class COpenWebNetTCP : public CDomoticzHardwareBase
{
	enum _eArea
	{
		WHERE_CEN_0 = -1,
		WHERE_AREA_0 = 0,
		WHERE_AREA_1 = 1,
		WHERE_AREA_2 = 2,
		WHERE_AREA_3 = 3,
		WHERE_AREA_4 = 4,
		WHERE_AREA_5 = 5,
		WHERE_AREA_6 = 6,
		WHERE_AREA_7 = 7,
		WHERE_AREA_8 = 8,
		WHERE_AREA_9 = 9,
		WHERE_AREA_10 = 10,
		MAX_WHERE_AREA = 11
	};

	enum _eWhereEnergyCentralUnit
	{
		WHERE_ENERGY_CU_1 = 51,
		WHERE_ENERGY_CU_2 = 52,
		WHERE_ENERGY_CU_3 = 53,
		WHERE_ENERGY_CU_4 = 54,
		WHERE_ENERGY_CU_5 = 55,
		WHERE_ENERGY_CU_6 = 56,
		MAX_WHERE_ENERGY_CU = 57
	};

	enum _eWhereEnergyActuators
	{
		WHERE_ENERGY_A_1 = 71,
		WHERE_ENERGY_A_2 = 72,
		WHERE_ENERGY_A_3 = 73,
		WHERE_ENERGY_A_4 = 74,
		WHERE_ENERGY_A_5 = 75,
		WHERE_ENERGY_A_6 = 76,
		MAX_WHERE_ENERGY_A = 77
	};

      public:
	COpenWebNetTCP(int ID, const std::string &IPAddress, unsigned short usIPPort, const std::string &ownPassword, int ownScanTime, int ownEnSync);
	~COpenWebNetTCP() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	bool SetSetpoint(int idx, float temp);
	boost::signals2::signal<void()> sDisconnected;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	bool isStatusSocketConnected();
	void Do_Work();
	void MonitorFrames();
	uint32_t ownCalcPass(const std::string &password, const std::string &nonce);
	bool ownAuthentication(csocket *connectionSocket);
	bool nonceHashAuthentication(csocket *connectionSocket, const std::string &nonce);
	std::string decToHexStrConvert(const std::string &paramString);
	std::string hexToDecStrConvert(const std::string &paramString);
	std::string byteToHexStrConvert(uint8_t *digest, size_t digestLen, char *pArray);
	std::string shaCalc(const std::string &paramString, int auth_type);
	bool hmacAuthentication(csocket *connectionSocket, int auth_type);
	csocket *connectGwOwn(const char *connectionMode);
	void disconnect();
	bool ownWrite(csocket *connectionSocket, const char *pdata, size_t size);
	int ownRead(csocket *connectionSocket, char *pdata, size_t size);
	bool sendCommand(bt_openwebnet &command, std::vector<bt_openwebnet> &response, int waitForResponse = 0, bool silent = false);
	bool ParseData(char *data, int length, std::vector<bt_openwebnet> &messages);
	void SendGeneralSwitch(int NodeID, uint8_t ChildID, int BatteryLevel, int cmd, int level, const std::string &defaultname, int RssiLevel = 12);
	void UpdateSwitch(int who, int where, int Level, int iInterface, int BatteryLevel, const char *devname);
	void UpdateBlinds(int who, int where, int Command, int iInterface, int iLevel, int BatteryLevel, const char *devname);
	void UpdateAlarm(int who, int where, int Command, const char *sCommand, int iInterface, int BatteryLevel, const char *devname);
	void UpdateCenPlus(int who, int where, int Command, int iAppValue, int what, int iInterface, int BatteryLevel, const char *devname);
	void UpdateTemp(int who, int where, float fval, int iInterface, int BatteryLevel, const char *devname);
	void UpdateTempProbe(const int who, const int where, const int child, const int idx_str, std::string sStatus, const int iInterface, const int BatteryLevel, const char *devname);
	void UpdateSetPoint(int who, int where, float fval, int iInterface, const char *devname);
	void UpdatePower(int who, int where, double fval, int iInterface, int BatteryLevel, const char *devname);
	void UpdateEnergy(int who, int where, double fval, int iInterface, int BatteryLevel, const char *devname);
	void UpdateSoundDiffusion(int who, int where, int what, int iInterface, int BatteryLevel, const char *devname);
	bool GetValueMeter(int NodeID, int ChildID, double *usage, double *energy);
	void decodeWhereAndFill(int who, const std::string &where, std::vector<std::string> whereParam, std::string *devname, int *iWhere);
	void UpdateDeviceValue(std::vector<bt_openwebnet>::iterator iter);
	void scan_automation_lighting(int cen_area);
	void scan_sound_diffusion();
	void scan_temperature_control();
	void scan_device();
	void requestGatewayInfo();
	void requestDateTime();
	void setDateTime(const std::string &tzString);
	void requestBurglarAlarmStatus();
	void requestDryContactIRDetectionStatus();
	void requestEnergyTotalizer();
	void requestAutomaticUpdatePower(int time);
	std::string getWhereForWrite(int where);

      private:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;
	std::string m_ownPassword;
	unsigned short m_ownScanTime;
	unsigned short m_ownSynch;

	time_t LastScanTime, LastScanTimeEnergy, LastScanTimeEnergyTot, LastScanSync;

	std::shared_ptr<std::thread> m_monitorThread;
	std::shared_ptr<std::thread> m_heartbeatThread;
	volatile uint32_t mask_request_status;
	int m_heartbeatcntr;
	csocket *m_pStatusSocket;
	std::mutex readQueueMutex;
};
