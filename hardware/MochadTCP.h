#pragma once

#include <deque>
#include <iostream>
#include "ASyncTCP.h"
//#include "MochadBase.h"

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"

class MochadTCP: public CDomoticzHardwareBase,  ASyncTCP
{
public:
	MochadTCP(const int ID, const std::string IPAddress, const unsigned short usIPPort);
	~MochadTCP(void);
	void WriteToHardware(const char *pdata, const unsigned char length);
	RBUF m_mochad;

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

	void Do_Work();
	bool ConnectInternal();
	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	sockaddr_in m_addr;
	int m_socket;

	static const int readBufferSize=1028;
	int selected[17][17];
	int currentHouse;
	int currentUnit;
	int statusPart;
	void Switch(int onOff);
	void AddSelected(const unsigned char *pData);
	unsigned char m_linecount;
	unsigned char m_exclmarkfound;
	void MatchLine();
	void ParseData(const unsigned char *pData, int Len);
	unsigned char m_buffer[readBufferSize];
	char s_buffer[readBufferSize];
	int m_bufferpos;

};

