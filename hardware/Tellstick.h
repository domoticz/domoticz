#pragma once

#ifdef WITH_TELLDUSCORE

#include "DomoticzHardware.h"
#include <condition_variable>

class CTellstick : public CDomoticzHardwareBase
{
public:
    explicit CTellstick(const int ID, int repeats, int repeatInterval);
    void SetSettings(int repeats, int repeatInterval);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
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
    bool StartHardware() override;
    bool StopHardware() override;
    void SendCommand(int devID, const _tGeneralSwitch &cmd);

    void ThreadSendCommands();
private:
    int m_deviceEventId;
    int m_rawDeviceEventId;
    int m_sensorEventId;

	std::shared_ptr<std::thread> m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::map<int, Command> m_commands;
    int m_numRepeats;
    boost::posix_time::milliseconds m_repeatInterval;
};

#endif //WITH_TELLSTICK
