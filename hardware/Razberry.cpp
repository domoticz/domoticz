#include "stdafx.h"
#include "Razberry.h"
#include "../main/Logger.h"

CRazberry::CRazberry(const int ID, const std::string &ipaddress, const int port, const std::string &username, const std::string &password)
{
	
}


CRazberry::~CRazberry(void)
{
}

bool CRazberry::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

bool CRazberry::StartHardware()
{
	return false;
}

bool CRazberry::StopHardware()
{
	return false;
}
