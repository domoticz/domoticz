#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define MAX_IDS 16

class Meteostick : public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eMState {
		MSTATE_INIT = 0,
		MSTATE_FILTERS,
		MSTATE_VALUES,
		MSTATE_FREQUENCY,
		MSTATE_DATA
	};
public:
	Meteostick(const int ID, const std::string& devname, const unsigned int baud_rate);
	~Meteostick();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	void StartPollerThread();
	void StopPollerThread();
	bool OpenSerialDevice();
	void Do_PollWork();

	void readCallback(const char *data, size_t len);

	void SendTempBaroSensorInt(const unsigned char Idx, const float Temp, const float Baro, const std::string &defaultname);
	void SendWindSensor(const unsigned char Idx, const float Temp, const float Speed, const int Direction, const std::string &defaultname);
	void SendLeafWetnessRainSensor(const unsigned char Idx, const unsigned char Channel, const int Wetness, const std::string &defaultname);
	void SendSoilMoistureSensor(const unsigned char Idx, const unsigned char Channel, const int Moisture, const std::string &defaultname);
	void SendSolarRadiationSensor(const unsigned char Idx, const float Radiation, const std::string &defaultname);
private:
	float m_LastOutsideTemp[MAX_IDS];
	int m_LastOutsideHum[MAX_IDS];
	float m_ActRainCounter[MAX_IDS];
	int m_LastRainValue[MAX_IDS];

	std::string m_szSerialPort;
	unsigned int m_iBaudRate;

	_eMState m_state;
	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	bool m_stoprequested;
	unsigned char m_buffer[1028];
	int m_bufferpos;
};

