#pragma once

#include "ASyncTCP.h"
#include "EvohomeRadio.h"

class CEvohomeTCP : public ASyncTCP, public CEvohomeRadio
{
public:
    CEvohomeTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &UserContID);

private:
    bool StopHardware();
    virtual void Do_Work();
    virtual void Do_Send(std::string str);

    virtual void OnConnect();
    void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnError(const std::exception e);
	void OnError(const boost::system::error_code& error);

    std::string m_szIPAddress;
	unsigned short m_usIPPort;
};
