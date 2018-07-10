#pragma once
#ifdef WIN32
#include "1WireSystem.h"

namespace Json
{
	class Value;
};

class C1WireForWindows : public I_1WireSystem
{
public:
   C1WireForWindows();
   virtual ~C1WireForWindows();

   // I_1WireSystem implementation
   virtual void GetDevices(/*out*/std::vector<_t1WireDevice>& devices) const;
   virtual void SetLightState(const std::string& sId,int unit,bool value, const unsigned int level);
   virtual float GetTemperature(const _t1WireDevice& device) const;
   virtual float GetHumidity(const _t1WireDevice& device) const;
   virtual float GetPressure(const _t1WireDevice& device) const;
   virtual bool GetLightState(const _t1WireDevice& device,int unit) const;
   virtual unsigned int GetNbChannels(const _t1WireDevice& device) const;
   virtual unsigned long GetCounter(const _t1WireDevice& device,int unit) const;
   virtual int GetVoltage(const _t1WireDevice& device,int unit) const;
   virtual float GetIlluminance(const _t1WireDevice& device) const;
   virtual int GetWiper(const _t1WireDevice& device) const;
   virtual void StartSimultaneousTemperatureRead();
   virtual void PrepareDevices();
   // END : I_1WireSystem implementation

   static bool IsAvailable();

protected:
   void GetDevice(const std::string& deviceName, /*out*/_t1WireDevice& device) const;
   Json::Value readData(const _t1WireDevice& device,int unit=0) const;
   unsigned int readChanelsNb(const _t1WireDevice& device) const;
   static bool StartService(){return false;}

   // Socket
   std::string SendAndReceive(const std::string &requestToSend) const;

   SOCKET m_Socket;
   std::mutex m_SocketMutex;
};

class C1WireForWindowsReadException : public std::exception
{
public:
   explicit C1WireForWindowsReadException(const std::string& message) : m_Message("1-Wire system : error reading value, ") {m_Message.append(message);}
   virtual ~C1WireForWindowsReadException() throw() {}
   virtual const char* what() const throw() {return m_Message.c_str();}
protected:
   std::string m_Message;
};
#endif // WIN32
