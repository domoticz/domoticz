#include "stdafx.h"
#ifdef WITH_TELLDUSCORE
#include "Tellstick.h"

#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../json/json.h"
#include "hardwaretypes.h"
#include <telldus-core.h>

CTellstick::CTellstick(const int ID)
{
	m_HwdID=ID;
	m_bSkipReceiveCheck = true;
}

CTellstick::~CTellstick(void)
{
	m_bIsStarted=false;
}

bool CTellstick::AddSwitchIfNotExits(const int id, const char* devname, bool isDimmer)
{
        char sid[16];
        sprintf(sid, "%08X", id);
  
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", m_HwdID, sid, pTypeGeneralSwitch, sSwitchTypeAC);
	if (result.size()<1)
	{
	        _log.Log(LOG_NORM, "Tellstick: device %d %s: %s", id, sid ,devname);
		m_sql.safe_query(
			"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
			"VALUES (%d,'%q',%d,%d,%d,%d,12,255,'%q',0,' ')",
			m_HwdID, sid, 3, pTypeGeneralSwitch, sSwitchTypeAC, isDimmer ? STYPE_Dimmer : STYPE_OnOff, devname);
		return true;
	}
	return false;
}

void CTellstick::deviceEvent(int deviceId, int method, const char *data) {
	_log.Log(LOG_NORM, "Tellstick: deviceEvent %d %d: %s", deviceId, method, data);

        char sid[16];
        sprintf(sid, "%08d", deviceId);  

	_tGeneralSwitch gswitch;
	// TODO pTypeGeneralSwitch
	gswitch.subtype = sSwitchTypeAC;
	gswitch.id = deviceId;
	gswitch.unitcode = 3;
	gswitch.level = 0;
	gswitch.battery_level = 255;
	gswitch.rssi = 12;
	gswitch.seqnbr = 0;
  
	if (method == TELLSTICK_TURNON) {
		gswitch.cmnd = gswitch_sOn;
		sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
	} else if (method == TELLSTICK_TURNOFF) {
		gswitch.cmnd = gswitch_sOff;
		sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
	} else if (method == TELLSTICK_DIM) {
		gswitch.cmnd = gswitch_sSetLevel;
		gswitch.level = atoi(data)*99/255;
		sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
	} else {
		_log.Log(LOG_NORM, "Unknown event from device %i\n", deviceId);
	}
}
void CTellstick::deviceEventCallback(int deviceId, int method, const char *data, int callbackId, void *context) {
  CTellstick *t = reinterpret_cast<CTellstick *>(context);
  if (t) {
    /** Please note!
     * We are here in another thread than the main. Some measures to syncronize
     * this must be taken!
     **/
    t->deviceEvent(deviceId, method, data);
  }
}

void CTellstick::Init()
{
  tdInit();
  tdRegisterDeviceEvent( reinterpret_cast<TDDeviceEvent>(&CTellstick::deviceEventCallback), this );
  int intNumberOfDevices = tdGetNumberOfDevices();
  for (int i = 0; i < intNumberOfDevices; i++) {
    int id = tdGetDeviceId(i);
    char *name = tdGetName(id);
    _log.Log(LOG_NORM, "Tellstick: %s method %d", name, tdMethods(id, TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM) & TELLSTICK_DIM);
    bool isDimmer = tdMethods(id, TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM) & TELLSTICK_DIM;
    AddSwitchIfNotExits(id, name, isDimmer);
    tdReleaseString(name);
  }

}

bool CTellstick::StartHardware()
{
	Init();
	m_bIsStarted=true;
	sOnConnected(this);
	_log.Log(LOG_NORM, "Tellstick: StartHardware");
	return true;
}

bool CTellstick::StopHardware()
{
	tdClose();
 	m_bIsStarted=false;
	return true;
}

bool CTellstick::WriteToHardware(const char *pdata, const unsigned char length)
{
	_tGeneralSwitch *pSwitch = (_tGeneralSwitch*)pdata;
	//tRBUF *pCmd=(tRBUF*) pdata;
	_log.Log(LOG_NORM, "Tellstick: WriteToHardware %d id: %d cmd: %d", pSwitch->type, pSwitch->id, pSwitch->cmnd);

	if (pSwitch->type != pTypeGeneralSwitch)
		return false; //only allowed to control switches

	if (pSwitch->cmnd == gswitch_sOn)
	{
	  _log.Log(LOG_NORM, "Tellstick: Switch ON");
	  tdTurnOn(pSwitch->id);
	}
	else if(pSwitch->cmnd == gswitch_sOff)
	{
	  _log.Log(LOG_NORM, "Tellstick: Switch OFF");
	  tdTurnOff(pSwitch->id);
	}
	else if(pSwitch->cmnd == gswitch_sSetLevel)
	{
	  _log.Log(LOG_NORM, "Tellstick: Dim level %d %d", pSwitch->level, pSwitch->level * 255 / 99);
	  tdDim(pSwitch->id, pSwitch->level * 255 / 99);
	}

	return true;
}
void CTellstick::SendCommand(const int ID, const std::string &command)
{
	_log.Log(LOG_NORM, "Tellstick: SendCommand");
}
#endif //WITH_TELLDUSCORE
