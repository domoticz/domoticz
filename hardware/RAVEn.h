#pragma once

#ifndef WIN32
#include "DomoticzHardware.h"
#include "ASyncSerial.h"
#include <iostream>

class RAVEn : public CDomoticzHardwareBase, 
              public AsyncSerial
{
public:
	explicit RAVEn(const int ID, const std::string& devname);
	~RAVEn(void);

	bool WriteToHardware(const char *pdata, const unsigned char length);
private:
    const std::string device_;
	boost::shared_ptr<boost::thread> m_thread;

	bool StartHardware();
	bool StopHardware();
	void readCallback(const char *data, size_t len);
};

#endif //WIN32
