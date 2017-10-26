#pragma once

// implememtation for weather with free api key : http://openweathermap.org/
// by Fantom (szczukot@poczta.onet.pl)

#include "DomoticzHardware.h"
#include <iosfwd>

class COpenWeatherMap : public CDomoticzHardwareBase
{
public:
	COpenWeatherMap(const int ID, const std::string &APIKey, const std::string &Location);
	~COpenWeatherMap(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	std::string GetForecastURL();
private:
	std::string m_APIKey;
	std::string m_Location;
	std::string m_ForecastURL;
	std::string m_Language;
	bool m_bHaveGPSCoordinated;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	bool StartHardware();
	bool StopHardware();
	void Do_Work();
	void GetMeterDetails();
};

