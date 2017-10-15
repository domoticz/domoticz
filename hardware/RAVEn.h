#pragma once

/* Handles reading data from Rainforest Automation RAVEn:
 
   http://rainforestautomation.com/wp-content/uploads/2014/02/raven_xml_api_r127.pdf

   Sample xml output:

    <InstantaneousDemand>
        <DeviceMacId>0xFFFFFFFFFFFFFFFF</DeviceMacId>
        <MeterMacId>0xFFFFFFFFFFFFFFFF</MeterMacId>
        <TimeStamp>0xFFFFFFFF</TimeStamp>
        <Demand>0x000320</Demand>
        <Multiplier>0x00000001</Multiplier>
        <Divisor>0x000003e8</Divisor>
        <DigitsRight>0x03</DigitsRight>
        <DigitsLeft>0x07</DigitsLeft>
        <SuppressLeadingZero>Y</SuppressLeadingZero>
    </InstantaneousDemand>

*/

#include "DomoticzHardware.h"
#include "ASyncSerial.h"
#include <iosfwd>

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

    char m_buffer[MAX_BUFFER_LEN];
    char* m_wptr;

    double m_currUsage;
    double m_totalUsage;
};

