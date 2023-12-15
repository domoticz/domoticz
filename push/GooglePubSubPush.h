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

private:
  void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
  void DoGooglePubSubPush(const uint64_t DeviceRowIdx);
};
extern CGooglePubSubPush m_googlepubsubpush;

