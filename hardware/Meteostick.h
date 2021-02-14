#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"

#define MAX_IDS 16

class Meteostick : public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eMState
	{
		MSTATE_INIT = 0,
		MSTATE_FILTERS,
		MSTATE_VALUES,
		MSTATE_FREQUENCY,
		MSTATE_DATA
	};

      public:
	Meteostick(int ID, const std::string &devname, unsigned int baud_rate);
	~Meteostick() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

      private:
	bool StartHardware() override;
	bool StopHardware() override;
	void ParseData(const unsigned char *pData, int Len);
	void ParseLine();

	bool OpenSerialDevice();
	void Do_Work();

	void readCallback(const char *data, size_t len);

	void SendTempBaroSensorInt(unsigned char Idx, float Temp, float Baro, const std::string &defaultname);
	void SendWindSensor(unsigned char Idx, float Temp, float Speed, int Direction, const std::string &defaultname);
	void SendLeafWetnessRainSensor(unsigned char Idx, unsigned char Channel, int Wetness, const std::string &defaultname);
	void SendSoilMoistureSensor(unsigned char Idx, unsigned char Channel, int Moisture, const std::string &defaultname);
	void SendSolarRadiationSensor(unsigned char Idx, float Radiation, const std::string &defaultname);

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
	unsigned char m_buffer[1028];
	int m_bufferpos;
};
