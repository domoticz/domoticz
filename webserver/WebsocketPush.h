#pragma once
#include <boost/signals2.hpp>
#include "../main/Push.h"

class CWebSocketPush : public CPush
{
public:
	CWebSocketPush();
	void Start();
	void Stop();
	void onRoomplanChanged();
	void onDeviceTableChanged(); // device added, or deleted
	// etc, we need a notification of all changes that need to be reflected in the UI
private:
	void OnDeviceReceived(const int m_HwdID, const unsigned long long DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
};

