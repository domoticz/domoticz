#pragma once

#include <iosfwd>
#include "ASyncTCP.h"
//#include "MochadBase.h"

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

class MochadTCP: public CDomoticzHardwareBase,  ASyncTCP
{
public:
	MochadTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
	~MochadTCP(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);
	unsigned char hex2bin(char h);
	void setSecID(unsigned char *p);
	void Switch(int onOff);
	void AddSelected(const unsigned char *pData);
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
private:
	RBUF m_mochad;
	RBUF m_mochadsec;
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	bool m_bDoRestart;
	int selected[17][17];
	int currentHouse;
	int currentUnit;
	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	unsigned char m_mochadbuffer[1028];
	char s_buffer[1028];
	int m_bufferpos;

};

