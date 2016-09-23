#pragma once

#include <boost/signals2.hpp>

#include "Push.h"

class CHttpPush : public CPush
{
public:
	CHttpPush();
	void Start();
	void Stop();
	void UpdateActive();

private:

	void OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
	void DoHttpPush();
};

