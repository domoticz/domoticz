#pragma once

#include "RFXNames.h"
#include "../hardware/hardwaretypes.h"
#include <string>

struct tScheduleItem
{
	bool bEnabled = false;
	bool bIsScene = false;
	bool bIsThermostat = false;
	std::string DeviceName;
	uint64_t RowID = 0;
	uint64_t TimerID = 0;
	unsigned char startDay = 0;
	unsigned char startMonth = 0;
	unsigned short startYear = 0;
	unsigned char startHour = 0;
	unsigned char startMin = 0;
	_eTimerType	timerType = TTYPE_ONTIME;
	_eTimerCommand timerCmd = TCMD_ON;
	int Level = 0;
	_tColor Color;
	float Temperature = 0.F;
	bool bUseRandomness = false;
	int Days = 0;
	int MDay = 0;
	int Month = 0;
	int Occurence = 0;
	//internal
	time_t startTime = 0;

	tScheduleItem() {
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
  CScheduler();
  ~CScheduler() = default;

  void StartScheduler();
  void StopScheduler();

  void ReloadSchedules();

  void SetSunRiseSetTimes(const std::string &sSunRise, const std::string &sSunSet, const std::string &sSunAtSouth, const std::string &sCivTwStart, const std::string &sCivTwEnd,
			   const std::string &sNautTwStart, const std::string &sNauTtwEnd, const std::string &sAstTwStart, const std::string &sAstTwEnd);

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
	void AdjustSunRiseSetSchedules();
	//will check if anything needs to be scheduled
	void CheckSchedules();
	void DeleteExpiredTimers();
};

