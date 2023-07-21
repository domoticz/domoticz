#pragma once

#include "1WireCommon.h"
#include "../1Wire.h"

// 1Wire system interface. Represent the mean used to access to the 1Wire bus.
// Implementation can be C1WireByKernel, C1WireByOWFS or C1WireForWindows
class I_1WireSystem
{
public:
  virtual ~I_1WireSystem() = default;

  virtual void GetDevices(/*out*/ std::vector<_t1WireDevice> &devices) const = 0;

  // Read/write on 1-wire network
  virtual void SetLightState(const std::string &sId, int unit, bool value, unsigned int level) = 0;
  virtual float GetTemperature(const _t1WireDevice &device) const = 0;
  virtual float GetHumidity(const _t1WireDevice &device) const = 0;
  virtual float GetPressure(const _t1WireDevice &device) const = 0;
  virtual bool GetLightState(const _t1WireDevice &device, int unit) const = 0;
  virtual unsigned int GetNbChannels(const _t1WireDevice &device) const = 0;
  virtual unsigned long GetCounter(const _t1WireDevice &device, int unit) const = 0;
  virtual int GetVoltage(const _t1WireDevice &device, int unit) const = 0;
  virtual float GetIlluminance(const _t1WireDevice &device) const = 0;
  virtual int GetWiper(const _t1WireDevice &device) const = 0;
  virtual void StartSimultaneousTemperatureRead() = 0;
  virtual void PrepareDevices() = 0;

	C1Wire *m_p1WireBase = nullptr;
};
