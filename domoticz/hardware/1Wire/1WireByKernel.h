#pragma once
#include "1WireSystem.h"

class C1WireByKernel : public I_1WireSystem
{
public:
   C1WireByKernel();
   virtual ~C1WireByKernel();

   // I_1WireSystem implementation
   virtual void GetDevices(/*out*/std::vector<_t1WireDevice>& devices) const;
   virtual void SetLightState(const std::string& sId,int unit,bool value);
   virtual float GetTemperature(const _t1WireDevice& device) const;
   virtual float GetHumidity(const _t1WireDevice& device) const;
   virtual bool GetLightState(const _t1WireDevice& device,int unit) const;
   virtual unsigned int GetNbChannels(const _t1WireDevice& device) const;
   virtual unsigned long GetCounter(const _t1WireDevice& device,int unit) const;
   virtual int GetVoltage(const _t1WireDevice& device,int unit) const;
   // END : I_1WireSystem implementation

   static bool IsAvailable();

protected:
   void GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const;

   bool sendAndReceiveByRwFile(std::string path,const unsigned char * const cmd,size_t cmdSize,unsigned char * const answer,size_t answerSize) const;


   // Thread management
   boost::thread* m_Thread;
   void ThreadFunction();
   void ThreadProcessPendingChanges();
   void ThreadBuildDevicesList();
   float ThreadReadRawDataHighPrecisionDigitalThermometer(const std::string& deviceFileName) const;
   unsigned char ThreadReadRawDataDualChannelAddressableSwitch(const std::string& deviceFileName) const;
   unsigned char ThreadReadRawData8ChannelAddressableSwitch(const std::string& deviceFileName) const;

   void ThreadWriteRawDataDualChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const;
   void ThreadWriteRawData8ChannelAddressableSwitch(const std::string& deviceFileName,int unit,bool value) const;

   // Devices
   #define MAX_DIGITAL_IO  8
   class DeviceState
   {
   public:
      DeviceState(_t1WireDevice device) : m_Device(device) {}
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

   bool m_AllDevicesInitialized;

   // Thread-shared data and lock methods
   boost::mutex m_Mutex;
   class Locker:boost::lock_guard<boost::mutex>
   {
   public:
      Locker(const boost::mutex& mutex):boost::lock_guard<boost::mutex>(*(const_cast<boost::mutex*>(&mutex))){}
      virtual ~Locker(){}
   };
   typedef std::map<std::string,DeviceState*> DeviceCollection;
   DeviceCollection m_Devices;

   // Pending changes queue
   std::list<DeviceState> m_PendingChanges;
   const DeviceState* GetDevicePendingState(const std::string& deviceId) const;
   boost::mutex m_PendingChangesMutex;
   boost::condition_variable m_PendingChangesCondition;
   class IsPendingChanges
   {
   private:
      std::list<DeviceState>& m_List;
   public:
      IsPendingChanges(std::list<DeviceState>& list):m_List(list){}
      bool operator()() const {return !m_List.empty();}
   };
};

class OneWireReadErrorException : public std::exception
{
public:
   OneWireReadErrorException(const std::string& deviceFileName) : m_Message("1-Wire system : error reading value from ") {m_Message.append(deviceFileName);}
   virtual ~OneWireReadErrorException() throw() {}
   virtual const char* what() const throw() {return m_Message.c_str();}
protected:
   std::string m_Message;
};

class OneWireWriteErrorException : public std::exception
{
public:
   OneWireWriteErrorException(const std::string& deviceFileName) : m_Message("1-Wire system : error writing value from ") {m_Message.append(deviceFileName);}
   virtual ~OneWireWriteErrorException() throw() {}
   virtual const char* what() const throw() {return m_Message.c_str();}
protected:
   std::string m_Message;
};
