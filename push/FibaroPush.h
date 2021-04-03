#pragma once

#include "BasePush.h"

class CFibaroPush : public CBasePush
{
public:
	CFibaroPush();
	void Start();
	void Stop();
	void UpdateActive();

private:
  void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
  void DoFibaroPush(const uint64_t DeviceRowIdx);
};
extern CFibaroPush m_fibaropush;
