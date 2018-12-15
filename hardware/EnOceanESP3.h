#pragma once

#include "ASyncSerial.h"
#include "DomoticzHardware.h"
#include <map>

#define ENOCEAN3_READ_BUFFER_SIZE 65*1024

class CEnOceanESP3: public AsyncSerial, public CDomoticzHardwareBase
{
	enum _eEnOcean_Receive_State
	{
		ERS_SYNCBYTE=0,
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
	CEnOceanESP3(const int ID, const std::string& devname, const int type);
    ~CEnOceanESP3();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	void SendDimmerTeachIn(const char *pdata, const unsigned char length);
	unsigned long m_id_base;
private:
	void Init();
	bool StartHardware() override;
	bool StopHardware() override;
	bool OpenSerialDevice();
	void Do_Work();
	bool ParseData();
	void Add2SendQueue(const char* pData, const size_t length);
	float GetValueRange(const float InValue, const float ScaleMax, const float ScaleMin=0, const float RangeMax=255, const float RangeMin=0);

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
	int				m_DataSize;
	int				m_OptionalDataSize;
    unsigned char m_buffer[ENOCEAN3_READ_BUFFER_SIZE];
	int m_bufferpos;
	int m_retrycntr;

	std::map<uint32_t, _tVLDNode> m_VLDNodes;

	std::mutex m_sendMutex;
	std::vector<std::string> m_sendqueue;
};

