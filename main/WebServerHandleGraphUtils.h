#pragma once

/*
 * WebServerHandleGraphUtils.h
 *
 * Utility types and free functions shared across graph-handler translation units.
 * All declarations here are in the http::server namespace to match the rest of
 * the WebServer subsystem.
 */

#include <map>
#include <string>
#include <ctime>

#include "../main/RFXNames.h"
#include <libwebem/request.h>

// Forward declaration
class CSQLHelper;

namespace http
{
namespace server
{

// ---------------------------------------------------------------------------

/**
 * All device-level context that is constant for the lifetime of a single
 * graph request.  Built once by BuildGraphContext() at the top of
 * Cmd_HandleGraph and then passed by const-ref to every sub-handler.
 */
struct GraphContext
{
    uint64_t                           idx;
    unsigned char                      dType;
    unsigned char                      dSubType;
    _eMeterType                        metertype;
    bool                               bIsManagedCounter;
    double                             divider;
    double                             meteroffset;   // == AddjValue
    double                             AddjValue;
    double                             AddjMulti;
    double                             AddjValue2;
    std::string                        sensor;
    std::string                        srange;
    std::string                        sgroupby;
    std::map<std::string, std::string> options;
    struct tm                          tmNow;         // localtime at request
};

/**
 * Populate @p ctx from @p req and the device row in @p sql.
 *
 * @return false if the device cannot be found or required parameters are
 *         missing (caller should return early without emitting JSON).
 */
bool BuildGraphContext(const request& req, CSQLHelper& sql, GraphContext& ctx);

// ---------------------------------------------------------------------------

/**
 * A pair of [start, end] date strings (format "YYYY-MM-DD") plus the
 * matching one-year-earlier interval used for the "previous year" series.
 */
struct DateRange
{
    std::string start;      // "YYYY-MM-DD"
    std::string end;        // "YYYY-MM-DD"
    std::string startPrev;  // same, one year earlier
    std::string endPrev;
};

/**
 * Compute the date range for srange == "month" or srange == "year".
 *
 * If the request contains actmonth + actyear the explicit month window is
 * used.  If only actyear is present the whole calendar year is used.
 * Otherwise the trailing month/year up to today is used.
 *
 * Produces identical date strings to the inline code in Cmd_HandleGraph.
 */
DateRange CalcMonthYearRange(const request& req, const struct tm& tmNow, const std::string& srange);

/**
 * Compute the 7-day trailing window ending today (srange == "week").
 *
 * start  = today-7 days  (date only, "YYYY-MM-DD")
 * end    = today          (date only, "YYYY-MM-DD")
 * startPrev / endPrev are not meaningful for the week range and are left
 * empty.
 */
DateRange CalcWeekRange(const struct tm& tmNow);

/**
 * Parse a custom range string of the form "2013-01-01T2013-12-31" into
 * its two date components.
 *
 * @return true on success, false if the format does not match.
 */
bool ParseCustomRange(const std::string& srange, std::string& start, std::string& end);

// ---------------------------------------------------------------------------

/**
 * Determine the database table name for srange == "hour" (or "day").
 *
 * Returns an empty string if the sensor type is not handled by this range.
 */
std::string CalcDbasetableHour(const GraphContext& ctx);

/**
 * Determine the database table name for srange == "week".
 *
 * Returns an empty string if the sensor type is not handled by this range.
 */
std::string CalcDbasetableWeek(const GraphContext& ctx);

// ---------------------------------------------------------------------------

/**
 * Format a cumulative counter/meter value according to its meter type.
 *
 * The returned string uses the same precision as the inline sprintf()
 * calls scattered through Cmd_HandleGraph:
 *   ENERGY / ENERGY_GENERATED  ->  "%.3f"  (value already in kWh units)
 *   GAS                        ->  "%.3f"
 *   WATER                      ->  "%.3f"
 *   COUNTER                    ->  "%.10g"
 *   default                    ->  "0"
 */
std::string FormatMeterValue(double value, _eMeterType metertype);

/**
 * Format an instantaneous rate value (Watt for energy, m³/h for gas, etc.).
 *
 * Precision rules (matching the inline code):
 *   ENERGY / ENERGY_GENERATED  ->  "%.3f"   (value * 1000, i.e. kWh -> Wh)
 *   GAS                        ->  "%.2f"
 *   WATER                      ->  "%.3f"
 *   COUNTER                    ->  "%.10g"
 *   default                    ->  "0"
 *
 * The caller is responsible for any kWh->Watt conversion; this function
 * just applies the format string.
 */
std::string FormatMeterRate(double value, _eMeterType metertype);

/**
 * Format a date as "YYYY-MM-DD".
 */
std::string FormatDate(int year, int month, int day);

/**
 * Format a date+time as "YYYY-MM-DD HH:MM:SS".
 */
std::string FormatDateTime(int year, int month, int day, int h, int m, int s);

// ---------------------------------------------------------------------------

/**
 * Format a wind speed or gust value for JSON output.
 *
 * When @p windunit is WINDUNIT_Beaufort the raw 0.1 m/s integer is converted
 * to Beaufort and the result is an integer string (e.g. "5").
 * Otherwise the value is scaled by @p windscale and formatted as "%.1f".
 *
 * @param rawTenthsMs  Raw speed value in units of 0.1 m/s (as stored in DB).
 * @param windunit     Value of sql.m_windunit.
 * @param windscale    Value of sql.m_windscale.
 */
std::string FormatWindSpeed(int rawTenthsMs, int windunit, float windscale);

} // namespace server
} // namespace http
