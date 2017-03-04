/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.h
Author : Blaise Thauvin
Version : 0.1
Description : This class is used by various Teleinfo hardware decoders to process and display data
              It is used by EcoDevices, TeleinfoSerial and more to come


History :
- 2017-03-03 : Creation
*/

#pragma once

#include "DomoticzHardware.h"

class TeleinfoBase : public CDomoticzHardwareBase
{

public:

        TeleinfoBase();
        ~TeleinfoBase();
private:

};
