#pragma once

#include "ASyncSerial.h"
#include "EvohomeRadio.h"

class CEvohomeSerial : public AsyncSerial, public CEvohomeRadio
{
public:
    CEvohomeSerial(const int ID, const std::string &szSerialPort, const int baudrate, const std::string &UserContID);

private:
    bool StopHardware();
    bool OpenSerialDevice();
    virtual void Do_Work();
    virtual void Do_Send(std::string str);

    void ReadCallback(const char *data, size_t len);

    std::string m_szSerialPort;
    unsigned int m_iBaudRate;
};
