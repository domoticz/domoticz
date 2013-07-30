#include "stdafx.h"
#include "Dummy.h"

CDummy::CDummy(const int ID)
{
	m_HwdID=ID;
}

CDummy::~CDummy(void)
{
}

void CDummy::Init()
{
}

bool CDummy::StartHardware()
{
	Init();
	sOnConnected(this);
	return true;
}

bool CDummy::StopHardware()
{
    return true;
}

void CDummy::WriteToHardware(const char *pdata, const unsigned char length)
{

}

