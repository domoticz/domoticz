
#include "stdafx.h"
#ifndef WIN32
#include "RAVEn.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../tinyxpath/tinyxml.h"
#include <usb.h>
#include <stdint.h>

RAVEn::RAVEn(const int ID, const std::string& devname)
: device_(devname)
{
    m_HwdID = ID;
}

RAVEn::~RAVEn(void)
{
}

bool RAVEn::StartHardware()
{
    StartHeartbeatThread();
    //Try to open the Serial Port
    try
    {
        _log.Log(LOG_STATUS, "RAVEn: Using serial port: %s", device_.c_str());
        open(device_.c_str(), 115200);
    }
    catch (boost::exception & e)
    {
        _log.Log(LOG_ERROR, "RAVEn: Error opening serial port!");
#ifdef _DEBUG
        _log.Log(LOG_ERROR, "-----------------\n%s\n-----------------", boost::diagnostic_information(e).c_str());
#else
        (void)e;
#endif
        return false;
    }
    catch (...)
    {
        _log.Log(LOG_ERROR, "RAVEn: Error opening serial port!!!");
        return false;
    }
    setReadCallback(boost::bind(&RAVEn::readCallback, this, _1, _2));
    m_bIsStarted = true;
    sOnConnected(this);

    return true;
}

bool RAVEn::StopHardware()
{
    terminate();
    StopHeartbeatThread();
    m_bIsStarted = false;
    return true;
}

void RAVEn::readCallback(const char *data, size_t len)
{
    boost::lock_guard<boost::mutex> l(readQueueMutex);
    if (!m_bEnableReceive)
        return; //receiving not enabled

#ifdef _DEBUG
        _log.Log(LOG_INFO, "RAVEn::readCallback got %d: %s", len, data)
#endif
    TiXmlDocument doc;
//FIXME: Null terminated?
    if (doc.Parse(data))
    {
        _log.Log(LOG_ERROR, "RAVEn: Not enough data received: ", data);
        return;
    }

    TiXmlElement *pRoot;

    pRoot = doc.FirstChildElement("InstantaneousDemand");
    if (!pRoot)
    {
        _log.Log(LOG_ERROR, "RAVEn: Unknown node");
        return;
    }
    double currUsage = double(strtoul(pRoot->FirstChildElement("Demand")->GetText(), NULL, 16))/strtoul(pRoot->FirstChildElement("Divisor")->GetText(), NULL, 16);
    SendKwhMeter(CDomoticzHardwareBase::m_HwdID, 1, 255, currUsage, 5, "Power Meter");

}

bool RAVEn::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}


#endif //WIN32
