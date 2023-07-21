#pragma once
#include "1WireSystem.h"

#define HUB_MAIN_SUB_PATH     "/main"
#ifdef WIN32
#define HUB_AUX_SUB_PATH      "/_aux"
#else
#define HUB_AUX_SUB_PATH      "/aux"
#endif

class C1WireByOWFS : public I_1WireSystem
{
private:
	std::string m_path; // OWFS mountpoint
	std::string m_simultaneousTemperaturePath; // OWFS mountpoint + "/simultaneous/temperature"

public:
   explicit C1WireByOWFS(const std::string& path, C1Wire *C1WareBase);
   ~C1WireByOWFS() override = default;

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

protected:
  static bool IsValidDir(const struct dirent *de);
  virtual bool FindDevice(const std::string &sID, /*out*/ _t1WireDevice &device) const;
  virtual bool FindDevice(const std::string &inDir, const std::string &sID, /*out*/ _t1WireDevice &device) const;
  void GetDevice(const std::string &inDir, const std::string &dirname, /*out*/ _t1WireDevice &device) const;
  void GetDevices(const std::string &inDir, /*out*/ std::vector<_t1WireDevice> &devices) const;
  std::string readRawData(const std::string &filename) const;
  void writeData(const _t1WireDevice &device, const std::string &propertyName, const std::string &value) const;
  std::string nameHelper(const std::string &dirname, _e1WireFamilyType family) const;
};
