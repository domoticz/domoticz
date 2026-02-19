#include "stdafx.h"
#include "HardwareMonitor.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <wchar.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define POLL_INTERVAL_CPU	30
#define POLL_INTERVAL_TEMP	70
#define POLL_INTERVAL_MEM	80
#define POLL_INTERVAL_DISK	170

CHardwareMonitorBase::CHardwareMonitorBase(const int ID)
{
	m_HwdID = ID;
	m_lastquerytime = 0;
	m_totcpu = 0;
	m_lastloadcpu = 0;
}

CHardwareMonitorBase::~CHardwareMonitorBase()
{
	StopHardware();
}

bool CHardwareMonitorBase::StartHardware()
{
	StopHardware();

	if (!GetOSType(m_OStype))
	{
		Log(LOG_STATUS, "Hardware Monitor was not able to detect an (known) OS type!");
	}
	else if (m_OStype == OStype_Apple)
	{
		Log(LOG_ERROR, "Hardware Monitor does not (yet) support Apple hardware!");
		return false;
	}

	CheckForOnboardSensors();

	RequestStart();

	OnStartPlatform();

	m_lastquerytime = 0;
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted = true;
	sOnConnected(this);

	return true;
}

bool CHardwareMonitorBase::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	OnStopPlatform();
	m_bIsStarted = false;
	return true;
}

void CHardwareMonitorBase::Do_Work()
{
	Log(LOG_STATUS, "Hardware Monitor: Started (OStype %s)", TranslateOSTypeToString(m_OStype).c_str());

	int msec_counter = 0;
	int64_t sec_counter = 140 - 2;	// Start at a moment that is close to most devicecheck intervals
	while (!IsStopRequested(500))
	{
		msec_counter++;
		if (msec_counter == 2)
		{
			msec_counter = 0;
			sec_counter++;
			if (sec_counter % 12 == 0)
				m_LastHeartbeat = mytime(nullptr);

			if (sec_counter % POLL_INTERVAL_TEMP == 0)
			{
				try
				{
					FetchData();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching motherboard sensors!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_CPU == 0)
			{
				try
				{
					FetchCPU();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching CPU data!...");
				}
			}
			if (sec_counter % POLL_INTERVAL_MEM == 0)
			{
				try
				{
					FetchMemory();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching memory data!...");
				}
			}

			if (sec_counter % POLL_INTERVAL_DISK == 0)
			{
				try
				{
					FetchDisk();
				}
				catch (...)
				{
					Log(LOG_ERROR, "Hardware Monitor: Error occurred while Fetching disk data!...");
				}
			}
		}
	}
	Log(LOG_STATUS, "Hardware Monitor: Stopped...");
}

void CHardwareMonitorBase::SendCurrent(const unsigned long Idx, const float Curr, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = 1;
	gDevice.floatval1 = Curr;
	gDevice.intval1 = static_cast<int>(Idx);
	sDecodeRXMessage(this, (const unsigned char*)&gDevice, defaultname.c_str(), 255, nullptr);
}

void CHardwareMonitorBase::UpdateSystemSensor(const std::string& qType, const int dindex, const std::string& devName, const std::string& devValue)
{
	if (!m_HwdID) {
		Debug(DEBUG_NORM, "Hardware Monitor: Id not found!");
		return;
	}
	int doffset = 0;
	if (qType == "Temperature")
	{
		doffset = 1000;
		float temp = static_cast<float>(atof(devValue.c_str()));
		SendTempSensor(doffset + dindex, 255, temp, devName);
	}
	else if (qType == "Load")
	{
		doffset = 1100;
		float perc = static_cast<float>(atof(devValue.c_str()));
		SendPercentageSensor(doffset + dindex, 0, 255, perc, devName);
	}
	else if (qType == "Fan")
	{
		doffset = 1200;
		int fanspeed = atoi(devValue.c_str());
		SendFanSensor(doffset + dindex, 255, fanspeed, devName);
	}
	else if (qType == "Voltage")
	{
		doffset = 1300;
		float volt = static_cast<float>(atof(devValue.c_str()));
		SendVoltageSensor(0, (uint32_t)(doffset + dindex), 255, volt, devName);
	}
	else if (qType == "Current")
	{
		doffset = 1400;
		float curr = static_cast<float>(atof(devValue.c_str()));
		SendCurrent(doffset + dindex, curr, devName);
	}
	else if (qType == "Process")
	{
		doffset = 1500;
		float usage = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, usage, devName, "MB");
	}
	else if (qType == "Power")
	{
		doffset = 1600;
		float watts = static_cast<float>(atof(devValue.c_str()));
		SendWattMeter(0, doffset + dindex, 255, watts, devName);
	}
	else if (qType == "Clock")
	{
		doffset = 1700;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "MHz");
	}
	else if (qType == "Data")
	{
		doffset = 1800;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "GB");
	}
	else if (qType == "SmallData")
	{
		doffset = 1900;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "MB");
	}
	else if (qType == "Throughput")
	{
		doffset = 2000;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "mbps");
	}
	else if (qType == "Level")
	{
		doffset = 2100;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendPercentageSensor(doffset + dindex, 0, 255, value, devName);
	}
	else if (qType == "Factor")
	{
		doffset = 2200;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "");
	}
	else if (qType == "StorageTemperature")
	{
		doffset = 2300;
		float temp = static_cast<float>(atof(devValue.c_str()));
		SendTempSensor(doffset + dindex, 255, temp, devName);
	}
	else if (qType == "GPUFan")
	{
		doffset = 2400;
		int fanspeed = atoi(devValue.c_str());
		SendFanSensor(doffset + dindex, 255, fanspeed, devName);
	}
	else if (qType == "CPULoad")
	{
		doffset = 2500;
		float perc = static_cast<float>(atof(devValue.c_str()));
		SendPercentageSensor(doffset + dindex, 0, 255, perc, devName);
	}
	else if (qType == "MemoryLoad")
	{
		doffset = 2600;
		float perc = static_cast<float>(atof(devValue.c_str()));
		SendPercentageSensor(doffset + dindex, 0, 255, perc, devName);
	}
	else if (qType == "MemoryData")
	{
		doffset = 2700;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "GB");
	}
	else if (qType == "StorageData")
	{
		doffset = 2800;
		float value = static_cast<float>(atof(devValue.c_str()));
		SendCustomSensor(0, doffset + dindex, 255, value, devName, "GB");
	}
}

bool CHardwareMonitorBase::GetOSType(nOSType& OStype)
{
	OStype = OStype_Unknown;

#if defined (__linux__)
	OStype = OStype_Linux;

	if (IsWSL())
		OStype = OStype_WSL;
#endif

#if defined (__FreeBSD__)
	OStype = OStype_FreeBSD;
#endif

#if defined (__OpenBSD__)
	OStype = OStype_OpenBSD;
#endif

#if defined (__CYGWIN32__)
	OStype = OStype_CYGWIN;
#endif

#ifdef WIN32
	OStype = OStype_Windows;
#endif

#ifdef __APPLE__
	OStype = OStype_Apple;
#endif

	if (OStype == OStype_Unknown)
		return false;
	return true;
}

std::string CHardwareMonitorBase::TranslateOSTypeToString(nOSType OSType)
{
	std::string sOSType;

	switch (OSType)
	{
	case OStype_Linux:
		sOSType = "Linux";
		break;
	case OStype_Rpi:
		sOSType = "Raspberry Pi Linux";
		break;
	case OStype_WSL:
		sOSType = "WSL Linux";
		break;
	case OStype_CYGWIN:
		sOSType = "CYGWIN Linux";
		break;
	case OStype_FreeBSD:
		sOSType = "FreeBSD";
		break;
	case OStype_OpenBSD:
		sOSType = "OpenBSD";
		break;
	case OStype_Windows:
		sOSType = "Windows";
		break;
	case OStype_Apple:
		sOSType = "Apple";
		break;
	default:
		sOSType = "Unknown";
		break;
	}
	return sOSType;
}
