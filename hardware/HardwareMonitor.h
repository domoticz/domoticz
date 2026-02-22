#pragma once

#include "DomoticzHardware.h"

class CHardwareMonitorBase : public CDomoticzHardwareBase
{
public:
	enum nOSType
	{
		OStype_Unknown = 0,
		OStype_Linux = 1,
		OStype_Rpi = 2,
		OStype_WSL = 3,
		OStype_CYGWIN = 4,
		OStype_FreeBSD = 8,
		OStype_OpenBSD = 9,
		OStype_Windows = 14,
		OStype_Apple = 15
	};

	explicit CHardwareMonitorBase(int ID);
	~CHardwareMonitorBase() override;
	bool WriteToHardware(const char* /*pdata*/, const unsigned char /*length*/) override { return false; }

	bool GetOSType(nOSType& OStype);
	std::string TranslateOSTypeToString(nOSType OSType);

protected:
	bool StartHardware() override;
	bool StopHardware() override;

	// Platform-specific hooks (override in derived classes)
	virtual void OnStartPlatform() {}
	virtual void OnStopPlatform() {}
	virtual void FetchData() = 0;
	virtual void FetchCPU() {}
	virtual void FetchMemory() {}
	virtual void FetchDisk() {}
	virtual void CheckForOnboardSensors() {}
	virtual bool IsWSL() { return false; }

	void UpdateSystemSensor(const std::string& qType, int dindex, const std::string& devName, const std::string& devValue);
	void SendCurrent(unsigned long Idx, float Curr, const std::string& defaultname);

	struct _tDUsageStruct
	{
		std::string MountPoint;
		int64_t TotalBlocks;
		int64_t UsedBlocks;
		int64_t AvailBlocks;
	};

	double m_lastquerytime;
	int64_t m_lastloadcpu;
	int m_totcpu;
	nOSType m_OStype;

private:
	void Do_Work();
	std::shared_ptr<std::thread> m_thread;
};
