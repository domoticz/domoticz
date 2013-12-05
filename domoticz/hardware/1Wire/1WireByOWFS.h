#pragma once
#include "1WireSystem.h"

class C1WireByOWFS : public I_1WireSystem
{
public:
   C1WireByOWFS() {}
   virtual ~C1WireByOWFS() {}

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
   static bool IsValidDir(const struct dirent*const de);
   virtual bool FindDevice(const std::string sID, /*out*/_t1WireDevice& device) const;
   virtual bool FindDevice(const std::string inDir, const std::string sID, /*out*/_t1WireDevice& device) const;
   void GetDevice(const std::string inDir, const std::string dirname, /*out*/_t1WireDevice& device) const;
   void GetDevices(const std::string inDir, /*out*/std::vector<_t1WireDevice>& devices) const;
   std::string readRawData(const std::string& filename) const;
   void writeData(const _t1WireDevice& device,std::string propertyName,const std::string value) const;
};
