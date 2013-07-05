#pragma once

#include <string>
#include <vector>

class MainWorker;

class CEventSystem
{
	typedef struct lua_State lua_State;

	struct _tEventItem
	{
		unsigned long long ID;
        std::string Name;
        std::string Conditions;
        std::string Actions;
        
	};

	struct _tDeviceStatus
	{
		unsigned long long ID;
        unsigned long long nValue;
        std::string sValue;
        
	};


public:
	CEventSystem(void);
	~CEventSystem(void);

	void StartEventSystem(MainWorker *pMainWorker);
	void StopEventSystem();

	void LoadEvents();
	bool ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string devname);
private:
	lua_State	*m_pLUA;
	MainWorker *m_pMain;
	boost::mutex eventMutex;
    boost::mutex deviceStateMutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	unsigned char m_secondcounter;
    struct tm *aTime;
	
    //our thread
	void Do_Work();
	void ProcessMinute();
    void GetCurrentStates();
    void EvaluateEvent(const std::string reason, const unsigned long long DeviceID);
	std::vector<_tEventItem> m_events;
    std::vector<_tDeviceStatus> m_devicestates;
};

