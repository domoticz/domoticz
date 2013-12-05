#pragma once


#include "DomoticzHardware.h"

class I_1WireSystem;
class C1Wire : public CDomoticzHardwareBase
{
public:
	C1Wire(const int ID);
	virtual ~C1Wire();

	static bool Have1WireSystem();
	void WriteToHardware(const char *pdata, const unsigned char length);

private:
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
   I_1WireSystem* m_system;

   static void LogSystem();
   void DetectSystem();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetDeviceDetails();

   // Messages to Domoticz
   void ReportLightState(const std::string& deviceId,int unit,bool state);
   void ReportTemperature(const std::string& deviceId,float temperature);
   void ReportTemperatureHumidity(const std::string& deviceId,float temperature,float humidity);
   void ReportHumidity(const std::string& deviceId,float humidity);
   void ReportCounter(const std::string& deviceId,unsigned long counter);
   void ReportVoltage(int unit,int voltage);
};
