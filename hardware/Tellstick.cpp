#include "stdafx.h"
#include "Tellstick.h"

#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include <json/json.h>

CTellstick::CTellstick(const TelldusFunctions& functions, const int ID, int repeats, int repeatInterval)
    : m_td(functions),
      m_deviceEventId(-1),
      m_rawDeviceEventId(-1),
      m_sensorEventId(-1),
      m_numRepeats(repeats),
      m_repeatInterval(repeatInterval)

{
    m_HwdID = ID;
    m_bSkipReceiveCheck = true;
}

void CTellstick::SetSettings(int repeats, int repeatInterval)
{
    m_numRepeats = repeats;
    m_repeatInterval = std::chrono::milliseconds(repeatInterval);
}

bool CTellstick::WriteToHardware(const char *pdata, const unsigned char length)
{
    const _tGeneralSwitch *pSwitch = reinterpret_cast<const _tGeneralSwitch*>(pdata);
    _log.Log(LOG_NORM, "Tellstick: WriteToHardware %d id: %d cmd: %d", pSwitch->type, pSwitch->id, pSwitch->cmnd);

    if (pSwitch->type != pTypeGeneralSwitch)
        return false; //only allowed to control switches

    std::unique_lock<std::mutex> lock(m_mutex);
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
	if (result.empty())
    {
        _log.Log(LOG_NORM, "Tellstick: device %d %s: %s", id, sid ,devname);
		m_sql.InsertDevice(m_HwdID, sid, 3, pTypeGeneralSwitch, sSwitchTypeAC, isDimmer ? STYPE_Dimmer : STYPE_OnOff, 0, " ", devname);

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
        SendTempSensor(deviceId, 255, static_cast<float>(atof(value)), "Temp");
        break;
    case TELLSTICK_HUMIDITY:
        SendHumiditySensor(deviceId, 255, atoi(value), "Humidity");
        break;
    case TELLSTICK_RAINRATE:
        SendRainSensor(deviceId, 255, static_cast<float>(atof(value)), "Rain");
        break;
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
	    sDecodeRXMessage(this, (const unsigned char *)&gswitch, nullptr, 255, m_Name.c_str());
	break;
    case TELLSTICK_TURNOFF:
        gswitch.cmnd = gswitch_sOff;
	    sDecodeRXMessage(this, (const unsigned char *)&gswitch, nullptr, 255, m_Name.c_str());
	break;
    case TELLSTICK_DIM:
        gswitch.cmnd = gswitch_sSetLevel;
        gswitch.level = atoi(data)*99/255;
	    sDecodeRXMessage(this, (const unsigned char *)&gswitch, nullptr, 255, m_Name.c_str());
	break;
    default:
        _log.Log(LOG_NORM, "Unknown event from device %i\n", deviceId);
        break;
    }
}

void CTellstick::rawDeviceEvent(int controllerId, const char *data)
{
    _log.Log(LOG_NORM, "Tellstick: rawDeviceEvent %d: %s", controllerId, data);

    if (!data)
        return;

    std::string deviceId;
    std::string winddirection;
    std::string windaverage;
    std::string windgust;

    size_t prevPos = 0;
    std::string message = data;
    size_t pos = message.find(';');

    while(pos != std::string::npos) {
        std::string param = message.substr(prevPos, pos-prevPos);
        prevPos = pos+1;
	size_t delim = param.find(':');
	if (delim == std::string::npos)
	{
		break;
	}
	if (param.substr(0, delim) == "id")
	{
		deviceId = param.substr(delim + 1, param.length() - delim);
	}
	else if (param.substr(0, delim) == "winddirection")
	{
		winddirection = param.substr(delim + 1, param.length() - delim);
	}
	else if (param.substr(0, delim) == "windaverage")
	{
		windaverage = param.substr(delim + 1, param.length() - delim);
	}
	else if (param.substr(0, delim) == "windgust")
	{
		windgust = param.substr(delim + 1, param.length() - delim);
	}
	pos = message.find(';', pos + 1);
    }
    if (!deviceId.empty() && !winddirection.empty() && ! windaverage.empty() && ! windgust.empty()) {
        SendWind(atoi(deviceId.c_str()), 255, atoi(winddirection.c_str()), static_cast<float>(atof(windaverage.c_str())), static_cast<float>(atof(windgust.c_str())), 0, 0, false, false, "Wind");
    }
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
    m_td.Init();
    m_deviceEventId = m_td.RegisterDeviceEvent(reinterpret_cast<TDDeviceEvent>(&CTellstick::deviceEventCallback), this);
    m_rawDeviceEventId = m_td.RegisterRawDeviceEvent(reinterpret_cast<TDRawDeviceEvent>(&CTellstick::rawDeviceEventCallback), this);
    m_sensorEventId = m_td.RegisterSensorEvent(reinterpret_cast<TDSensorEvent>(&CTellstick::sensorEventCallback), this);

    const int numDevices = m_td.GetNumberOfDevices();
    for (int i = 0; i < numDevices; i++)
    {
        int id = m_td.GetDeviceId(i);
        char *name = m_td.GetName(id);
        _log.Log(LOG_NORM, "Tellstick: %s method %d", name, m_td.Methods(id, TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM) & TELLSTICK_DIM);
        bool isDimmer = m_td.Methods(id, TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_DIM) & TELLSTICK_DIM;
        AddSwitchIfNotExits(id, name, isDimmer);
        m_td.ReleaseString(name);
    }
}

bool CTellstick::StartHardware()
{
	RequestStart();

    Init();
    m_bIsStarted=true;
    sOnConnected(this);
    _log.Log(LOG_NORM, "Tellstick: StartHardware");
    //Start worker thread
    m_thread = std::make_shared<std::thread>([this] { ThreadSendCommands(); });
    SetThreadNameInt(m_thread->native_handle());
    return true;
}

bool CTellstick::StopHardware()
{
    if (m_deviceEventId != -1)
        m_td.UnregisterCallback(m_deviceEventId);
    if (m_rawDeviceEventId != -1)
        m_td.UnregisterCallback(m_rawDeviceEventId);
    if (m_sensorEventId != -1)
        m_td.UnregisterCallback(m_sensorEventId);

    m_td.Close();
    std::unique_lock<std::mutex> lock(m_mutex);
    m_bIsStarted = false;
    m_cond.notify_all();
    lock.unlock();
    if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}
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
        m_td.TurnOn(genSwitch.id);
        break;
    case gswitch_sOff:
        _log.Log(LOG_NORM, "Tellstick: Switch OFF");
        m_td.TurnOff(genSwitch.id);
        break;
    case gswitch_sSetLevel:
        _log.Log(LOG_NORM, "Tellstick: Dim level %d %d",
                 genSwitch.level, genSwitch.level * 255 / 99);
        m_td.Dim(genSwitch.id, genSwitch.level * 255 / 99);
        break;
    }
}

void CTellstick::ThreadSendCommands()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_bIsStarted)
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto nextTime = now + m_repeatInterval;

        for (std::map<int, Command>::iterator it = m_commands.begin();
             it != m_commands.end();
             /* no increment */)
        {
            if (it->second.repeatTimePoint > now)
            {
                ++it;
                continue;
            }

            SendCommand(it->first, it->second.genSwitch);
            ++it->second.repeat;
            if (it->second.repeat > m_numRepeats)
                it = m_commands.erase(it);
            else
            {
                it->second.repeatTimePoint += m_repeatInterval;
                if (it->second.repeatTimePoint < nextTime)
                    nextTime = it->second.repeatTimePoint;
                ++it;
            }
        }

        if (m_commands.empty())
            m_cond.wait(lock);
        else
            m_cond.wait_until(lock, nextTime);
    }
}

//Webserver helpers
namespace http {
    namespace server {
        void CWebServer::Cmd_TellstickApplySettings(WebEmSession &session, const request &req, Json::Value &root)
        {
            if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; //Only admin user allowed
			}

            std::string hwIdStr = request::findValue(&req, "idx");
            std::string repeatsStr = request::findValue(&req, "repeats");
            std::string repeatIntervalStr = request::findValue(&req, "repeatInterval");

            if (hwIdStr.empty() || repeatsStr.empty() || repeatIntervalStr.empty())
                return;

            int hwID = atoi(hwIdStr.c_str());
            int repeats = atoi(repeatsStr.c_str());
            int repeatInterval = atoi(repeatIntervalStr.c_str());

            m_sql.safe_query("UPDATE Hardware SET Mode1=%d, Mode2=%d WHERE (ID == %d)",
                             repeats, repeatInterval, hwID);

            CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(hwID);
	    if (pBaseHardware == nullptr)
		    return;
	    if (pBaseHardware->HwdType != HTYPE_Tellstick)
		    return;
	    CTellstick *pTellstick = reinterpret_cast<CTellstick *>(pBaseHardware);
	    pTellstick->SetSettings(repeats, repeatInterval);
	}
    } // namespace server
} // namespace http
