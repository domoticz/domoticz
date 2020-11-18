#pragma once

#include "BasePush.h"

class CGooglePubSubPush : public CBasePush
{
public:
	CGooglePubSubPush();
	void Start();
	void Stop();
	void UpdateActive();

private:
  void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
  void DoGooglePubSubPush();
};
extern CGooglePubSubPush m_googlepubsubpush;

