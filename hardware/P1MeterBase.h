#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class P1MeterBase : public CDomoticzHardwareBase
{
	friend class P1MeterSerial;
	friend class P1MeterTCP;
	friend class CRFXBase;
public:
	P1MeterBase(void);
	~P1MeterBase(void);

	P1Power	m_power;
	P1Gas	m_gas;
private:
	void Init();
	bool MatchLine();
	void ParseP1Data(const uint8_t* pDataIn, const int LenIn, const bool disable_crc, int ratelimit);

	bool CheckCRC();

	bool m_bDisableCRC;
	int m_ratelimit;

	unsigned char m_p1version;

	unsigned char m_buffer[1400];
	int m_bufferpos;
	unsigned char m_exclmarkfound;
	unsigned char m_linecount;
	unsigned char m_CRfound;

	unsigned char l_buffer[128];
	int l_bufferpos;
	unsigned char l_exclmarkfound;

	unsigned long m_lastgasusage;
	time_t m_lastSharedSendGas;
	time_t m_lastUpdateTime;

	float m_voltagel1;
	float m_voltagel2;
	float m_voltagel3;

	bool m_bReceivedAmperage;
	float m_amperagel1;
	float m_amperagel2;
	float m_amperagel3;
	
	float m_powerdell1;
	float m_powerdell2;
	float m_powerdell3;

	float m_powerusel1;
	float m_powerusel2;
	float m_powerusel3;

	unsigned char m_gasmbuschannel;
	std::string m_gasprefix;
	std::string m_gastimestamp;
	double m_gasclockskew;
	time_t m_gasoktime;

	//Encryption
	bool m_bIsEncrypted = false;
	std::vector<char> m_szHexKey;
	enum class P1EcryptionState
	{
		waitingForStartByte = 0,
		readSystemTitleLength,
		readSystemTitle,
		readSeparator82,
		readPayloadLength,
		readSeparator30,
		readFrameCounter,
		readPayload,
		readGcmTag,
		doneReadingTelegram
	};

	P1EcryptionState m_p1_encryption_state = P1EcryptionState::waitingForStartByte;
	int m_currentBytePosition = 0;
	int m_changeToNextStateAt = 0;
	int m_dataLength = 0;
	std::string m_systemTitle;
	uint32_t m_frameCounter = 0;
	std::string m_dataPayload;
	std::string m_gcmTag;
	uint8_t* m_pDecryptBuffer = nullptr;
	size_t m_DecryptBufferSize = 0;
	void InitP1EncryptionState();
	bool ParseP1EncryptedData(const uint8_t p1_byte);
};
