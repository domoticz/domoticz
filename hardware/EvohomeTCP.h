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
    virtual void OnConnect() override;
    void OnDisconnect() override;
	void OnData(const unsigned char *pData, size_t length) override;
	void OnError(const std::exception e) override;
	void OnError(const boost::system::error_code& error) override;
private:
    std::string m_szIPAddress;
	unsigned short m_usIPPort;
};
