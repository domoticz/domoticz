#pragma once

#include "ASyncTCP.h"
#include "EvohomeRadio.h"

class CEvohomeTCP : public ASyncTCP, public CEvohomeRadio
{
public:
    CEvohomeTCP(const int ID, const std::string &IPAddress, const unsigned short usIPPort, const std::string &UserContID);
private:
    bool StopHardware() override;
    virtual void Do_Work() override;
    virtual void Do_Send(std::string str) override;
    
	void OnConnect();
    void OnDisconnect();
	void OnData(const unsigned char *pData, size_t length);
	void OnErrorStd(const std::exception e);
	void OnErrorBoost(const boost::system::error_code& error);
private:
    std::string m_szIPAddress;
	unsigned short m_usIPPort;
};
