#pragma once

#include "RFXNames.h"
#include "../hardware/hardwaretypes.h"
#include <string>
#include "StoppableTask.h"

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

	tScheduleItem() {
		bEnabled = false;
		bIsScene = false;
		bIsThermostat = false;
		RowID = 0;
		TimerID = 0;
		startDay = 0;
		startMonth = 0;
		startYear = 0;
		startHour = 0;
		startMin = 0;
		timerType = TTYPE_ONTIME;
		timerCmd = TCMD_ON;
		Level = 0;
		Temperature = 0.0f;
		bUseRandomness = false;
		Days = 0;
		MDay = 0;
		Month = 0;
		Occurence = 0;
		//internal
		startTime = 0;
	}

	bool operator==(const tScheduleItem &comp) const {
		return (this->TimerID == comp.TimerID)
			&& (this->bIsScene == comp.bIsScene)
			&& (this->bIsThermostat == comp.bIsThermostat);
	}
};

class CScheduler : public StoppableTask
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
	std::mutex m_mutex;
	std::shared_ptr<std::thread> m_thread;
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

