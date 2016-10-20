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
#include <telldus-core.h>

using namespace std;

int CTellstick::s_maxRetries(5);
boost::posix_time::milliseconds CTellstick::s_retryDelay(500);

CTellstick::CTellstick(const int ID)
    : m_deviceEventId(-1),
      m_rawDeviceEventId(-1),
      m_sensorEventId(-1)

{
    m_HwdID = ID;
    m_bSkipReceiveCheck = true;
}

bool CTellstick::WriteToHardware(const char *pdata, const unsigned char length)
{
    const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
    _log.Log(LOG_NORM, "Tellstick: WriteToHardware %d id: %d cmd: %d", pSwitch->type, pSwitch->id, pSwitch->cmnd);

    if (pSwitch->type != pTypeGeneralSwitch)
        return false; //only allowed to control switches

    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_commands[pSwitch->id] = Command(*pSwitch);
    m_cond.notify_all();
    return true;
}

bool CTellstick::AddSwitchIfNotExits(const int id, const char* devname, bool isDimmer)
{
    char sid[16];
    sprintf(sid, "%08X", id);
  
    std::vector<std::vector<std::string> > result;
    result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (SubType==%d)", 
                              m_HwdID, sid, pTypeGeneralSwitch, sSwitchTypeAC);
    if (result.size() < 1)
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

void CTellstick::sensorEvent(int deviceId, const char *protocol, const char *model, int dataType, const char *value)
{
    _log.Log(LOG_NORM, "Tellstick: sensorEvent %d,%s,%s,%d,%s", deviceId, protocol, model, dataType, value);
    switch (dataType)
    {
    case TELLSTICK_TEMPERATURE:
        SendTempSensor(deviceId, 255, atof(value), "Temp");
        break;
    case TELLSTICK_HUMIDITY:
        SendHumiditySensor(deviceId, 255, atof(value), "Humid");
        break;
    case TELLSTICK_RAINRATE:
    case TELLSTICK_RAINTOTAL:
    case TELLSTICK_WINDDIRECTION:
    case TELLSTICK_WINDAVERAGE:
    case TELLSTICK_WINDGUST:
        break;
    }
}

void CTellstick::deviceEvent(int deviceId, int method, const char *data)
{
    _log.Log(LOG_NORM, "Tellstick: deviceEvent %d %d: %s", deviceId, method, data);

    char sid[16];
    sprintf(sid, "%08d", deviceId);  

    _tGeneralSwitch gswitch;
    gswitch.id = deviceId;
    gswitch.unitcode = 3;
  
    switch (method)
    {
    case TELLSTICK_TURNON:
        gswitch.cmnd = gswitch_sOn;
        sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
        break;
    case TELLSTICK_TURNOFF:
        gswitch.cmnd = gswitch_sOff;
        sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
        break;
    case TELLSTICK_DIM:
        gswitch.cmnd = gswitch_sSetLevel;
        gswitch.level = atoi(data)*99/255;
        sDecodeRXMessage(this, (const unsigned char *)&gswitch, NULL, 255);
        break;
    default:
        _log.Log(LOG_NORM, "Unknown event from device %i\n", deviceId);
        break;
    }
}

void CTellstick::rawDeviceEvent(int controllerId, const char *data)
{
    _log.Log(LOG_NORM, "Tellstick: rawDeviceEvent %d: %s", controllerId, data);
}

void CTellstick::deviceEventCallback(int deviceId, int method, const char *data, int callbackId, void *context)
{
    CTellstick *t = reinterpret_cast<CTellstick *>(context);
    if (t) 
    {
        /** Please note!
        * We are here in another thread than the main. Some measures to syncronize
        * this must be taken!
        **/
        t->deviceEvent(deviceId, method, data);
    }
}

void CTellstick::rawDeviceEventCallback(const char *data, int controllerId, int callbackId, void *context)
{
    CTellstick *t = reinterpret_cast<CTellstick *>(context);
    if (t) 
    {
        /** Please note!
        * We are here in another thread than the main. Some measures to syncronize
        * this must be taken!
        **/
        t->rawDeviceEvent(controllerId, data);
    }
}

void CTellstick::sensorEventCallback(const char *protocol, const char *model, int deviceId, int dataType, 
                                     const char *value, int timestamp, int callbackId, void *context) 
 {
    CTellstick *t = reinterpret_cast<CTellstick *>(context);
    if (t) 
    {
        /** Please note!
        * We are here in another thread than the main. Some measures to syncronize
        * this must be taken!
        **/
        t->sensorEvent(deviceId, protocol, model, dataType, value);
    }
}

void CTellstick::Init()
{
    tdInit();
    m_deviceEventId = tdRegisterDeviceEvent(reinterpret_cast<TDDeviceEvent>(&CTellstick::deviceEventCallback), this);
    m_rawDeviceEventId = tdRegisterRawDeviceEvent(reinterpret_cast<TDRawDeviceEvent>(&CTellstick::rawDeviceEventCallback), this);
    m_sensorEventId = tdRegisterSensorEvent(reinterpret_cast<TDSensorEvent>(&CTellstick::sensorEventCallback), this);

    for (int i = 0; i < tdGetNumberOfDevices(); i++)
    {
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
    //Start worker thread
    m_thread = boost::thread(boost::bind(&CTellstick::ThreadSendCommands, this));
    return true;
}

bool CTellstick::StopHardware()
{
    if (m_deviceEventId != -1)
        tdUnregisterCallback(m_deviceEventId);
    if (m_rawDeviceEventId != -1)
        tdUnregisterCallback(m_rawDeviceEventId);
    if (m_sensorEventId != -1)
        tdUnregisterCallback(m_sensorEventId);
  
    tdClose();
    boost::unique_lock<boost::mutex> lock(m_mutex);
    m_bIsStarted = false;
    m_cond.notify_all();
    m_thread.join();
    return true;
}

void CTellstick::SendCommand(int devID, const _tGeneralSwitch &genSwitch)
{
    _log.Log(LOG_NORM, "Tellstick: SendCommand %d id: %d cmd: %d",
             genSwitch.type, genSwitch.id, genSwitch.cmnd);
    switch (genSwitch.cmnd)
    {
    case gswitch_sOn:
        _log.Log(LOG_NORM, "Tellstick: Switch ON");
        tdTurnOn(genSwitch.id);
        break;
    case gswitch_sOff:
        _log.Log(LOG_NORM, "Tellstick: Switch OFF");
        tdTurnOff(genSwitch.id);
        break;
    case gswitch_sSetLevel:
        _log.Log(LOG_NORM, "Tellstick: Dim level %d %d",
                 genSwitch.level, genSwitch.level * 255 / 99);
        tdDim(genSwitch.id, genSwitch.level * 255 / 99);
        break;
    }
}

void CTellstick::ThreadSendCommands()
{
    boost::unique_lock<boost::mutex> lock(m_mutex);
    while (m_bIsStarted)
    {
        boost::system_time now = boost::get_system_time();
        boost::system_time nextTime = now + s_retryDelay;
        for (map<int, Command>::iterator it = m_commands.begin();
             it != m_commands.end();
             /* no increment */)
        {
            if (it->second.retryTimePoint > now)
            {
                ++it;
                continue;
            }

            SendCommand(it->first, it->second.genSwitch);
            ++it->second.retry;
            if (it->second.retry >= s_maxRetries)
                m_commands.erase(it++);
            else
            {
                it->second.retryTimePoint += s_retryDelay;
                if (it->second.retryTimePoint < nextTime)
                    nextTime = it->second.retryTimePoint;
                ++it;
            }
        }

        if (m_commands.empty())
            m_cond.wait(lock);
        else
            m_cond.timed_wait(lock, nextTime);
    }
}
#endif //WITH_TELLDUSCORE
