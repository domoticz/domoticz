#include "stdafx.h"
#include "appversion.h"

std::string GetVersionStr()
{
	char szTmp[100];
	sprintf(szTmp,"%s%d",VERSION_STRING,SVNVERSION);
	std::string retstr=szTmp;
	return retstr;
}
