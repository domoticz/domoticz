#pragma once

#include "ASyncSerial.h"
#include "EvohomeRadio.h"

class CEvohomeSerial : public AsyncSerial, public CEvohomeRadio
{
public:
    CEvohomeSerial(const int ID, const std::string &szSerialPort, const int baudrate, const std::string &UserContID);
private:
  bool StopHardware() override;
  bool OpenSerialDevice();
  void Do_Work() override;
  void Do_Send(std::string str) override;
  void ReadCallback(const char *data, size_t len);

private:
    std::string m_szSerialPort;
    unsigned int m_iBaudRate;
};
