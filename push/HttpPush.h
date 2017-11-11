#pragma once

#include "BasePush.h"

class CHttpPush : public CBasePush
{
public:
	CHttpPush();
	void Start();
	void Stop();
	void UpdateActive();

private:

	void OnDeviceReceived(const int m_HwdID, const uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoHttpPush();
};
extern CHttpPush m_httppush;
