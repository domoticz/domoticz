#pragma once

#include "BasePush.h"

#include "../main/StoppableTask.h"

class CHttpPush : public CBasePush, public StoppableTask
{
public:
	CHttpPush();
	void Start();
	void Stop();
	void UpdateActive();

private:
  void OnDeviceReceived(int m_HwdID, uint64_t DeviceRowIdx, const std::string &DeviceName, const unsigned char *pRXCommand);
  void DoHttpPush(const uint64_t DeviceRowIdx);
};
extern CHttpPush m_httppush;
