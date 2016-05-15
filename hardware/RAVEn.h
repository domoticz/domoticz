#pragma once

#ifndef WIN32
#include "DomoticzHardware.h"
#include "ASyncSerial.h"
#include <iostream>

#define MAX_BUFFER_LEN  10000

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
	void readCallback(const char *indata, size_t inlen);

    char buffer_[MAX_BUFFER_LEN];
    char* wptr_;

    double currUsage_;
    double totalUsage_;
};

#endif //WIN32
