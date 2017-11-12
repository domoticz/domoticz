#pragma once

#ifdef WITH_TELLDUSCORE

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include <iosfwd>
#include <map>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>

class CTellstick : public CDomoticzHardwareBase
{
public:
    explicit CTellstick(const int ID, int repeats, int repeatInterval);
    void SetSettings(int repeats, int repeatInterval);
    bool WriteToHardware(const char *pdata, const unsigned char length);

private:
    struct Command
    {
        Command()
            : repeat(0),
              repeatTimePoint(boost::get_system_time()) {}
        Command(_tGeneralSwitch genSwitch)
            : genSwitch(genSwitch),
              repeat(0),
              repeatTimePoint(boost::get_system_time()) {}

        _tGeneralSwitch genSwitch;
        int repeat;
        boost::system_time repeatTimePoint;
    };

    void deviceEvent(int deviceId, int method, const char *data);
    static void deviceEventCallback(int deviceId, int method, const char *data, int callbackId, void *context);

    void rawDeviceEvent(int controllerId, const char *data);
    static void rawDeviceEventCallback(const char *data, int controllerId, int callbackId, void *context);

    void sensorEvent(int deviceId, const char *protocol, const char *model, int dataType, const char *value);
    static void sensorEventCallback(const char *protocol, const char *model, int id, int dataType, const char *value,
                    int timestamp, int callbackId, void *context);
    bool AddSwitchIfNotExits(const int id, const char* devname, bool isDimmer);
    void Init();
    bool StartHardware();
    bool StopHardware();
    void SendCommand(int devID, const _tGeneralSwitch &cmd);
    
    void ThreadSendCommands();

    int m_deviceEventId;
    int m_rawDeviceEventId;
    int m_sensorEventId;

    boost::thread m_thread;
    boost::mutex m_mutex;
    boost::condition_variable m_cond;
    std::map<int, Command> m_commands;
    int m_numRepeats;
    boost::posix_time::milliseconds m_repeatInterval;
};

#endif //WITH_TELLSTICK
