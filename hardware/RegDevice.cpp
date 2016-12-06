#include "stdafx.h"
#include "RegDevice.h"


CRegDevice::CRegDevice()
{
	m_iSetpointDevID = 0;
	m_iSwitchDevID = 0;
	m_iTempDevID = 0;
}


CRegDevice::~CRegDevice()
{
}

CRegDevice::CRegDevice(int temp, int setpoint, int switchid)
{
	m_iSetpointDevID = temp;
	m_iSwitchDevID = setpoint;
	m_iTempDevID = switchid;
}

