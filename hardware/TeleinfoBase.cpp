/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 0.1
Description : This class is used by various Teleinfo hardware decoders to process and display data
			  It is used by EcoDevices, TeleinfoSerial and more to come

History :
0.1 2017-03-03 : Creation
*/

#include "stdafx.h"
#include "TeleinfoBase.h"

CTeleinfoBase::CTeleinfoBase()
{
	ID=1;
	OPTARIF="";
	PTEC="";
	DEMAIN="";
	PAPP=0;
	BASE=0;
	HCHC=0;
	HCHP=0;
	PEJP=0;
	EJPHN=0;
	EJPHPM=0;
	BBRHPJB=0;
	BBRHCJB=0;
	BBRHPJW=0;
	BBRHCJW=0;
	BBRHPJR=0;
	BBRHCJR=0;
	previous=0;
	rate="";
	tariff="";
	color="";
	last = 0;
}


CTeleinfoBase::~CTeleinfoBase(void) {}

bool CTeleinfoBase::UpdateDevices()
{
	return true;
}
