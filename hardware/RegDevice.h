#pragma once
class CRegDevice
{
public:
	int m_iTempDevID;
	int m_iSetpointDevID;
	int m_iSwitchDevID;

public:
	CRegDevice();
	CRegDevice(int temp, int setpoint, int switchid);
	~CRegDevice();
};

