#include "stdafx.h"
#include "Dummy.h"

CDummy::CDummy(const int ID)
{
	m_HwdID=ID;
}

CDummy::~CDummy(void)
{
	m_bIsStarted=false;
}

void CDummy::Init()
{
}

bool CDummy::StartHardware()
{
	Init();
	m_bIsStarted=true;
	sOnConnected(this);
	return true;
}

bool CDummy::StopHardware()
{
	m_bIsStarted=false;
    return true;
}

void CDummy::WriteToHardware(const char *pdata, const unsigned char length)
{

}

