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

	void OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoFibaroPush();
};
extern CFibaroPush m_fibaropush;
