#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "TelldusFunctions.h"

class CTellstick : public CDomoticzHardwareBase
{
public:
    // If possible, creates a CTellstick object and return true, else false
    static bool Create(CTellstick** tellstick, const int ID, int repeats, int repeatInterval);

    void SetSettings(int repeats, int repeatInterval);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
private:
    explicit CTellstick(const TelldusFunctions& functions, const int ID, int repeats, int repeatInterval);

    struct Command
    {
        Command()
            : repeat(0),
              repeatTimePoint(std::chrono::system_clock::now()) {}
        Command(_tGeneralSwitch genSwitch)
            : genSwitch(genSwitch),
              repeat(0),
              repeatTimePoint(std::chrono::system_clock::now()) {}

        _tGeneralSwitch genSwitch;
        int repeat;
        std::chrono::system_clock::time_point repeatTimePoint;
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
    const TelldusFunctions m_td;
    int m_deviceEventId;
    int m_rawDeviceEventId;
    int m_sensorEventId;

	std::shared_ptr<std::thread> m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::map<int, Command> m_commands;
    int m_numRepeats;
    std::chrono::milliseconds m_repeatInterval;
};
