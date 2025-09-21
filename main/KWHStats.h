#pragma once
#include <array>
#include <cstdint>

class CKWHStats
{
public:
    static constexpr int HOURS_PER_DAY = 24;
    static constexpr int DAYS_PER_WEEK = 7;

    CKWHStats();
    ~CKWHStats();
    void Init(uint64_t deviceID);
    void AddHourValue(int hour, double kwh);
    void FinishDay();

    static void HandleKWHStatsHour();

private:
    bool LoadFromDB();
    bool SaveToDB();

    uint64_t m_device_id = 0;
    std::array<double, HOURS_PER_DAY> daily_hour_kwh{};
    std::array<double, HOURS_PER_DAY> weekday_hour_kwh_raw{};
    std::array<double, DAYS_PER_WEEK> weekday_kwh{};
    std::array<std::array<double, HOURS_PER_DAY>, DAYS_PER_WEEK> weekday_hour_kwh{};
};

