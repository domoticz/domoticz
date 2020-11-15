#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"
#include <map>

#define ENOCEAN3_READ_BUFFER_SIZE 65 * 1024

class CEnOceanESP3 : public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eEnOcean_Receive_State
	{
		ERS_SYNCBYTE = 0,
		ERS_HEADER,
		ERS_DATA,
		ERS_CHECKSUM
	};
	struct _tVLDNode
	{
		int idx;
		int manufacturer;
		uint8_t profile;
		uint8_t type;
	};

      public:
	CEnOceanESP3(int ID, const std::string &devname, int type);
	~CEnOceanESP3() override = default;
	bool WriteToHardware(const char *pdata, unsigned char length) override;
	void SendDimmerTeachIn(const char *pdata, unsigned char length);
	unsigned long m_id_base;

      private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
	void Add2SendQueue(const char *pData, size_t length);
	float GetValueRange(float InValue, float ScaleMax, float ScaleMin = 0, float RangeMax = 255, float RangeMin = 0);

	bool sendFrame(unsigned char frametype, unsigned char *databuf, unsigned short datalen, unsigned char *optdata, unsigned char optdatalen);
	bool sendFrameQueue(unsigned char frametype, unsigned char *databuf, unsigned short datalen, unsigned char *optdata, unsigned char optdatalen);

	void ParseRadioDatagram();
	void readCallback(const char *data, size_t len);

	void ReloadVLDNodes();

      private:
	_eEnOcean_Receive_State m_receivestate;
	int m_wantedlength;

	std::shared_ptr<std::thread> m_thread;
	int m_Type;
	std::string m_szSerialPort;

	bool m_bBaseIDRequested;

	// Create a circular buffer.
	unsigned char m_ReceivedPacketType;
	int m_DataSize;
	int m_OptionalDataSize;
	unsigned char m_buffer[ENOCEAN3_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_retrycntr;

	std::map<uint32_t, _tVLDNode> m_VLDNodes;

	std::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;
};
