#pragma once
#include "1WireSystem.h"
#include <condition_variable>
#include <list>
#include "../../main/StoppableTask.h"

class C1WireByKernel : public I_1WireSystem, StoppableTask
{
public:
   C1WireByKernel();
   ~C1WireByKernel() override;

   // I_1WireSystem implementation
   void GetDevices(/*out*/ std::vector<_t1WireDevice> &devices) const override;
   void SetLightState(const std::string &sId, int unit, bool value, unsigned int level) override;
   float GetTemperature(const _t1WireDevice &device) const override;
   float GetHumidity(const _t1WireDevice &device) const override;
   float GetPressure(const _t1WireDevice &device) const override;
   bool GetLightState(const _t1WireDevice &device, int unit) const override;
   unsigned int GetNbChannels(const _t1WireDevice &device) const override;
   unsigned long GetCounter(const _t1WireDevice &device, int unit) const override;
   int GetVoltage(const _t1WireDevice &device, int unit) const override;
   float GetIlluminance(const _t1WireDevice &device) const override;
   int GetWiper(const _t1WireDevice &device) const override;
   void StartSimultaneousTemperatureRead() override;
   void PrepareDevices() override;
   // END : I_1WireSystem implementation

   static bool IsAvailable();

protected:
   void GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const;

   bool sendAndReceiveByRwFile(std::string path, const unsigned char *cmd, size_t cmdSize, unsigned char *answer, size_t answerSize) const;
   void ReadStates();

   // Thread management
   std::shared_ptr<std::thread> m_thread;
   void ThreadFunction();
   void ThreadProcessPendingChanges();
   void ThreadBuildDevicesList();
   float ThreadReadRawDataHighPrecisionDigitalThermometer(const std::string& deviceFileName) const;
   unsigned char ThreadReadRawDataDualChannelAddressableSwitch(const std::string& deviceFileName) const;
   unsigned char ThreadReadRawData8ChannelAddressableSwitch(const std::string& deviceFileName) const;

   void ThreadWriteRawDataDualChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const;
   void ThreadWriteRawData8ChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const;
private:
   // Devices
   #define MAX_DIGITAL_IO  8
   class DeviceState
   {
   public:
	   explicit DeviceState(const _t1WireDevice &device) : m_Device(device) {}
      _t1WireDevice GetDevice() const {return m_Device;}
      union
      {
         float m_Temperature;      // °C
         bool m_DigitalIo[MAX_DIGITAL_IO];
      };
      int m_Unit;
   protected:
      _t1WireDevice m_Device;
   };

   // Thread-shared data and lock methods
   std::mutex m_Mutex;
   class Locker:std::lock_guard<std::mutex>
   {
   public:
	   explicit Locker(const std::mutex& mutex):std::lock_guard<std::mutex>(*(const_cast<std::mutex*>(&mutex))){}
	   virtual ~Locker() = default;
   };
   typedef std::map<std::string,DeviceState*> DeviceCollection;
   DeviceCollection m_Devices;

   // Pending changes queue
   std::list<DeviceState> m_PendingChanges;
   const DeviceState* GetDevicePendingState(const std::string& deviceId) const;
   std::mutex m_PendingChangesMutex;
   std::condition_variable m_PendingChangesCondition;
   class IsPendingChanges
   {
   private:
      std::list<DeviceState>& m_List;
   public:
	   explicit  IsPendingChanges(std::list<DeviceState>& list):m_List(list){}
      bool operator()() const {return !m_List.empty();}
   };
};

class OneWireReadErrorException : public std::exception
{
public:
	explicit OneWireReadErrorException(const std::string& deviceFileName) : m_Message("1-Wire system : error reading value from ") {m_Message.append(deviceFileName);}
	~OneWireReadErrorException() noexcept override = default;
	const char *what() const noexcept override
	{
		return m_Message.c_str();
	}

protected:
   std::string m_Message;
};

class OneWireWriteErrorException : public std::exception
{
public:
	explicit  OneWireWriteErrorException(const std::string& deviceFileName) : m_Message("1-Wire system : error writing value from ") {m_Message.append(deviceFileName);}
	~OneWireWriteErrorException() noexcept override = default;
	const char *what() const noexcept override
	{
		return m_Message.c_str();
	}

protected:
   std::string m_Message;
};
