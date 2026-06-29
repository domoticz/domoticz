#pragma once

#include "BasePush.h"

#include "../main/StoppableTask.h"

class CGooglePubSubPush : public CBasePush, public StoppableTask
{
public:
	CGooglePubSubPush();
	void Start();
	void Stop();
	void UpdateActive();
	void DoGooglePubSubPush(const uint64_t DeviceRowIdx);

private:
  void OnDeviceReceived(int HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
};
extern CGooglePubSubPush m_googlepubsubpush;

