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
	bool WriteToHardware(const char *pdata, const unsigned char length);
	RBUF m_mochad;
	RBUF m_mochadsec;
private:
	bool StartHardware();
	bool StopHardware();
protected:
	std::string m_szIPAddress;
	unsigned short m_usIPPort;

	void OnConnect();
	void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

	unsigned char hex2bin(char h);
	void setSecID(unsigned char *p);

	void Do_Work();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	bool m_bDoRestart;
	int selected[17][17];
	int currentHouse;
	int currentUnit;
	void Switch(int onOff);
	void AddSelected(const unsigned char *pData);
	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
	unsigned char m_mochadbuffer[1028];
	char s_buffer[1028];
	int m_bufferpos;

};

