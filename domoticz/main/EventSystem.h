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
        int EventStatus;
        
	};
    
public:
    
    struct _tDeviceStatus
	{
		unsigned long long ID;
        std::string deviceName;
        unsigned long long nValue;
        std::string sValue;
        unsigned char devType;
        unsigned char subType;
        std::string nValueWording;
        std::string lastUpdate;
        
	};
    std::map<unsigned long long,_tDeviceStatus> m_devicestates;
    
	CEventSystem(void);
	~CEventSystem(void);

	void StartEventSystem(MainWorker *pMainWorker);
	void StopEventSystem();

	void LoadEvents();
	bool ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string devname);
    void RemoveSingleState(int ulDevID);
    
private:
	//lua_State	*m_pLUA;
	MainWorker *m_pMain;
	boost::mutex eventMutex;
    boost::mutex deviceStateMutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	unsigned char m_secondcounter;
   
    
    //our thread
	void Do_Work();
	void ProcessMinute();
    void GetCurrentStates();
    void GetCurrentMeasurementStates();
    std::string UpdateSingleState(unsigned long long ulDevID, std::string devname, const int nValue, const char* sValue,const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, std::string lastUpdate);
    void EvaluateEvent(const std::string reason);
	void EvaluateEvent(const std::string reason, const unsigned long long DeviceID, const std::string devname, const int nValue, const char* sValue, std::string nValueWording);
    void EvaluateBlockly(const std::string reason, const unsigned long long DeviceID, const std::string devname, const int nValue, const char* sValue, std::string nValueWording);
    void EvaluateLua(const std::string reason, const std::string filename);
    void EvaluateLua(const std::string reason, const std::string filename, const unsigned long long DeviceID, const std::string devname, const int nValue, const char* sValue, std::string nValueWording);
    std::string nValueToWording (const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue,const std::string sValue);
    static int l_domoticz_print(lua_State* lua_state);
    void SendEventNotification(const std::string Subject, const std::string Body);
    void ScheduleEvent(int deviceID, std::string Action);
    void ScheduleEvent(std::string ID, std::string Action);
    std::string reciprokalAction (std::string Action);
    std::vector<_tEventItem> m_events;
    std::map<unsigned long long,std::string> tempValues;
    std::map<unsigned long long,std::string> humValues;
    std::map<unsigned long long,std::string> baroValues;
    void reportMissingDevice (int deviceID, std::string EventName, unsigned long long eventID);
    std::string describeError(int resultcode);
    
};

