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
        int SequenceNo;
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
        unsigned char lastLevel;
        
	};
    std::map<unsigned long long,_tDeviceStatus> m_devicestates;
    
	CEventSystem(void);
	~CEventSystem(void);

	void StartEventSystem(MainWorker *pMainWorker);
	void StopEventSystem();

	void LoadEvents();
	bool ProcessDevice(const int HardwareID, const unsigned long long ulDevID, const unsigned char unit, const unsigned char devType, const unsigned char subType, const unsigned char signallevel, const unsigned char batterylevel, const int nValue, const char* sValue, const std::string &devname);
    void RemoveSingleState(int ulDevID);
    void WWWUpdateSingleState(const unsigned long long ulDevID, const std::string &devname);
	void WWWGetItemStates(std::vector<_tDeviceStatus> &iStates);
private:
	//lua_State	*m_pLUA;
	MainWorker *m_pMain;
	boost::mutex eventMutex;
	boost::mutex luaMutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	unsigned char m_secondcounter;
   
    
    //our thread
	void Do_Work();
	void ProcessMinute();
    void GetCurrentStates();
    void GetCurrentMeasurementStates();
    std::string UpdateSingleState(const unsigned long long ulDevID, const std::string &devname, const int nValue, const char* sValue,const unsigned char devType, const unsigned char subType, const _eSwitchType switchType, const std::string &lastUpdate, const unsigned char lastLevel);
    void EvaluateEvent(const std::string &reason);
	void EvaluateEvent(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording);
    void EvaluateBlockly(const std::string &reason, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording);
    bool parseBlocklyActions(const std::string &Actions, const std::string &eventName, const unsigned long long eventID);
    void EvaluateLua(const std::string &reason, const std::string &filename);
    void EvaluateLua(const std::string &reason, const std::string &filename, const unsigned long long DeviceID, const std::string &devname, const int nValue, const char* sValue, std::string nValueWording);
    std::string nValueToWording (const unsigned char dType, const unsigned char dSubType, const _eSwitchType switchtype, const unsigned char nValue,const std::string &sValue);
    static int l_domoticz_print(lua_State* lua_state);
    void SendEventNotification(const std::string &Subject, const std::string &Body);
    void OpenURL(const std::string &URL);
    bool ScheduleEvent(int deviceID, std::string Action, bool isScene, const std::string &eventName);
    bool ScheduleEvent(std::string ID, const std::string &Action, const std::string &eventName);
    //std::string reciprocalAction (std::string Action);
    std::vector<_tEventItem> m_events;
    std::map<std::string,float> m_tempValuesByName;
    std::map<std::string,unsigned char> m_humValuesByName;
    std::map<std::string,int> m_baroValuesByName;
    std::map<unsigned long long,float> m_tempValuesByID;
    std::map<unsigned long long,unsigned char> m_humValuesByID;
    std::map<unsigned long long,int> m_baroValuesByID;
    void reportMissingDevice (const int deviceID, const std::string &EventName, const unsigned long long eventID);
    int getSunRiseSunSetMinutes(const std::string &what);
    bool isEventscheduled(const std::string &eventName);
    void report_errors(lua_State *L, int status);
    unsigned char calculateDimLevel(int deviceID , int percentageLevel);
    
};

