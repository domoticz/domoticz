#pragma once

#include "RFXNames.h"
#include "../hardware/hardwaretypes.h"
#include <string>
#include <vector>

struct tScheduleItem
{
	bool bEnabled;
	bool bIsScene;
	bool bIsThermostat;
	std::string DeviceName;
	uint64_t RowID;
	uint64_t TimerID;
	unsigned char startDay;
	unsigned char startMonth;
	unsigned short startYear;
	unsigned char startHour;
	unsigned char startMin;
	_eTimerType	timerType; 
	_eTimerCommand timerCmd;
	int Level;
	_tColor Color;
	float Temperature;
	bool bUseRandomness;
	int Days;
	int MDay;
	int Month;
	int Occurence;
	//internal
	time_t startTime;

	bool operator==(const tScheduleItem &comp) const {
		return (this->TimerID == comp.TimerID)
			&& (this->bIsScene == comp.bIsScene)
			&& (this->bIsThermostat == comp.bIsThermostat);
	}
};

class CScheduler
{
public:
	CScheduler(void);
	~CScheduler(void);

	void StartScheduler();
	void StopScheduler();

	void ReloadSchedules();

	void SetSunRiseSetTimers(const std::string &sSunRise, const std::string &sSunSet, const std::string &sSunAtSouth, const std::string &sCivTwStart, const std::string &sCivTwEnd, const std::string &sNautTwStart, const std::string &sNauTtwEnd, const std::string &sAstTwStart, const std::string &sAstTwEnd);

	std::vector<tScheduleItem> GetScheduleItems();

private:
	time_t m_tSunRise;
	time_t m_tSunSet;
	time_t m_tSunAtSouth;
	time_t m_tCivTwStart;
	time_t m_tCivTwEnd;
	time_t m_tNautTwStart;
	time_t m_tNautTwEnd;
	time_t m_tAstTwStart;
	time_t m_tAstTwEnd;
	boost::mutex m_mutex;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;
	std::vector<tScheduleItem> m_scheduleitems;

	//our thread
	void Do_Work();

	//will set the new/next startTime
	//returns false if timer is invalid (like no sunset/sunrise known yet)
	bool AdjustScheduleItem(tScheduleItem *pItem, bool bForceAddDay);
	//will check if anything needs to be scheduled
	void CheckSchedules();
	void DeleteExpiredTimers();
};

