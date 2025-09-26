#pragma once
#include <array>
#include <cstdint>

class CKWHStats
{
public:
    static constexpr int HOURS_PER_DAY = 24;
    static constexpr int QUARTERS_PER_DAY = HOURS_PER_DAY * 4;
    static constexpr int DAYS_PER_WEEK = 7;

    CKWHStats();
    ~CKWHStats();
    void Init(uint64_t deviceID);
    void AddHourValue(const int hour, const int Watt);
    void FinishDay();


    static void InitGlobal(const int interval_seconds = 300);
    static void ExitGlobal();
    static void HandleKWHStatsHour();
    static void PeriodicSaveKWHStats(const int interval_seconds);

private:
    bool LoadFromDB();
    bool SaveToDB();

    uint64_t m_device_id = 0;
    bool m_bDirty = false;
    std::array<int, HOURS_PER_DAY> daily_hour_kwh{};
    std::array<int, HOURS_PER_DAY> weekday_hour_kwh_raw{};
    std::array<int, DAYS_PER_WEEK> weekday_kwh{};
    std::array<std::array<int, HOURS_PER_DAY>, DAYS_PER_WEEK> weekday_hour_kwh{};
};

