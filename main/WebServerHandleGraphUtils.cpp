/*
 * WebServerHandleGraphUtils.cpp
 *
 * Implementation of the utility types declared in WebServerHandleGraphUtils.h.
 */

#include "stdafx.h"
#include "WebServerHandleGraphUtils.h"

#include <libwebem/request.h>
#include "Helper.h"
#include "SQLHelper.h"
#include "localtime_r.h"
#include "mainworker.h"   // pType* / sType* constants

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

namespace http
{
namespace server
{

// ===========================================================================

bool BuildGraphContext(const request& req, CSQLHelper& sql, GraphContext& ctx)
{
    // --- idx ----------------------------------------------------------------
    ctx.idx = 0;
    const std::string sIdx = request::findValue(&req, "idx");
    if (!sIdx.empty())
        ctx.idx = std::stoull(sIdx);

    // --- sensor / srange / sgroupby -----------------------------------------
    ctx.sensor   = request::findValue(&req, "sensor");
    ctx.srange   = request::findValue(&req, "range");
    ctx.sgroupby = request::findValue(&req, "groupby");

    // sensor is mandatory; srange or sgroupby must be present
    if (ctx.sensor.empty())
        return false;
    if (ctx.srange.empty() && ctx.sgroupby.empty())
        return false;

    // --- current local time -------------------------------------------------
    time_t now = mytime(nullptr);
    localtime_r(&now, &ctx.tmNow);

    // --- device row ---------------------------------------------------------
    std::vector<std::vector<std::string>> result =
        sql.safe_query(
            "SELECT Type, SubType, SwitchType, AddjValue, AddjMulti, AddjValue2, Options"
            " FROM DeviceStatus WHERE (ID == %" PRIu64 ")",
            ctx.idx);
    if (result.empty())
        return false;

    ctx.dType    = static_cast<unsigned char>(atoi(result[0][0].c_str()));
    ctx.dSubType = static_cast<unsigned char>(atoi(result[0][1].c_str()));
    ctx.metertype = static_cast<_eMeterType>(atoi(result[0][2].c_str()));

    // Override metertype for well-known energy/gas/counter device types
    // (exact logic from Cmd_HandleGraph lines 73-80)
    if (   ctx.dType == pTypeP1Power
        || ctx.dType == pTypeENERGY
        || ctx.dType == pTypePOWER
        || ctx.dType == pTypeCURRENTENERGY
        || (ctx.dType == pTypeGeneral && ctx.dSubType == sTypeKwh))
    {
        ctx.metertype = MTYPE_ENERGY;
    }
    else if (ctx.dType == pTypeP1Gas)
    {
        ctx.metertype = MTYPE_GAS;
    }
    else if (ctx.dType == pTypeRego6XXValue && ctx.dSubType == sTypeRego6XXCounter)
    {
        ctx.metertype = MTYPE_COUNTER;
    }

    // --- managed counter flag -----------------------------------------------
    // Special case: Usage instead of Value in the Meter table; no last-value
    // calculation needed.
    ctx.bIsManagedCounter =
        (ctx.dType == pTypeGeneral) && (ctx.dSubType == sTypeManagedCounter);

    // --- adjustment values --------------------------------------------------
    ctx.AddjValue  = atof(result[0][3].c_str());
    ctx.AddjMulti  = atof(result[0][4].c_str());
    ctx.AddjValue2 = atof(result[0][5].c_str());

    // --- device options -----------------------------------------------------
    const std::string sOptions = result[0][6];
    ctx.options = sql.BuildDeviceOptions(sOptions);

    // AddDBLogEntry=true also implies managed-counter semantics
    if (ctx.options["AddDBLogEntry"] == "true")
        ctx.bIsManagedCounter = true;

    // --- divider and meter offset -------------------------------------------
    ctx.divider     = sql.GetCounterDivider(int(ctx.metertype), int(ctx.dType), float(ctx.AddjValue2));
    ctx.meteroffset = ctx.AddjValue;

    return true;
}

// ===========================================================================

DateRange CalcMonthYearRange(const request& req,
                              const struct tm& tmNow,
                              const std::string& srange)
{
    DateRange dr;

    const std::string sactmonth = request::findValue(&req, "actmonth");
    const std::string sactyear  = request::findValue(&req, "actyear");

    int actMonth = atoi(sactmonth.c_str());
    int actYear  = atoi(sactyear.c_str());

    if (!sactmonth.empty() && !sactyear.empty())
    {
        // Explicit month + year supplied
        dr.start     = FormatDate(actYear,     actMonth, 1);
        dr.startPrev = FormatDate(actYear - 1, actMonth, 1);
        actMonth++;
        if (actMonth == 13)
        {
            actMonth = 1;
            actYear++;
        }
        dr.end     = FormatDate(actYear,     actMonth, 1);
        dr.endPrev = FormatDate(actYear - 1, actMonth, 1);
    }
    else if (!sactyear.empty())
    {
        // Whole calendar year
        dr.start     = FormatDate(actYear,     1, 1);
        dr.startPrev = FormatDate(actYear - 1, 1, 1);
        actYear++;
        dr.end     = FormatDate(actYear,     1, 1);
        dr.endPrev = FormatDate(actYear - 1, 1, 1);
    }
    else
    {
        // Trailing window up to today
        dr.end     = FormatDate(tmNow.tm_year + 1900,     tmNow.tm_mon + 1, tmNow.tm_mday);
        dr.endPrev = FormatDate(tmNow.tm_year + 1900 - 1, tmNow.tm_mon + 1, tmNow.tm_mday);

        struct tm tm2;
        if (srange == "month")
        {
            // Subtract one month (getNoon normalises the date)
            time_t monthbefore;
            getNoon(monthbefore, tm2,
                    tmNow.tm_year + 1900,
                    tmNow.tm_mon,       // tm_mon is 0-based, so this is "one month back"
                    tmNow.tm_mday);
        }
        else
        {
            // Subtract one year
            time_t yearbefore;
            getNoon(yearbefore, tm2,
                    tmNow.tm_year + 1900 - 1,
                    tmNow.tm_mon + 1,
                    tmNow.tm_mday);
        }

        dr.start     = FormatDate(tm2.tm_year + 1900,     tm2.tm_mon + 1, tm2.tm_mday);
        dr.startPrev = FormatDate(tm2.tm_year + 1900 - 1, tm2.tm_mon + 1, tm2.tm_mday);
    }

    return dr;
}

DateRange CalcWeekRange(const struct tm& tmNow)
{
    DateRange dr;

    dr.end = FormatDate(tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday);

    // Subtract seven days (getNoon normalises across month/year boundaries)
    time_t weekbefore;
    struct tm tm2;
    getNoon(weekbefore, tm2,
            tmNow.tm_year + 1900, tmNow.tm_mon + 1, tmNow.tm_mday - 7);

    dr.start = FormatDate(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);
    // startPrev / endPrev are not used for week ranges
    return dr;
}

bool ParseCustomRange(const std::string& srange,
                      std::string& start,
                      std::string& end)
{
    // Expected format: "2013-01-01T2013-12-31"
    // Validity check mirrors the condition in Cmd_HandleGraph:
    //   srange.substr(0,1)  == "2"
    //   srange.substr(10,1) == "T"
    //   srange.substr(11,1) == "2"
    if (srange.size() < 21)
        return false;
    if (srange[0] != '2' || srange[10] != 'T' || srange[11] != '2')
        return false;

    start = srange.substr(0, 10);
    end   = srange.substr(11, 10);
    return true;
}

// ===========================================================================

std::string CalcDbasetableHour(const GraphContext& ctx)
{
    if (ctx.sensor == "temp")
        return "Temperature";
    if (ctx.sensor == "rain")
        return "Rain";
    if (ctx.sensor == "Percentage")
        return "Percentage";
    if (ctx.sensor == "fan")
        return "Fan";
    if (ctx.sensor == "counter")
    {
        if ((ctx.dType == pTypeP1Power) || (ctx.dType == pTypeCURRENT) || (ctx.dType == pTypeCURRENTENERGY))
            return "MultiMeter";
        return "Meter";
    }
    if ((ctx.sensor == "wind") || (ctx.sensor == "winddir"))
        return "Wind";
    if (ctx.sensor == "uv")
        return "UV";
    return "";
}

std::string CalcDbasetableWeek(const GraphContext& ctx)
{
    if (ctx.sensor == "rain")
        return "Rain_Calendar";
    if (ctx.sensor == "counter")
    {
        if (ctx.dType == pTypeP1Power
            || ctx.dType == pTypeCURRENT
            || ctx.dType == pTypeCURRENTENERGY
            || ctx.dType == pTypeAirQuality
            || ctx.dType == pTypeLux
            || ctx.dType == pTypeWEIGHT
            || ctx.dType == pTypeUsage
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeVisibility
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeDistance
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeSolarRadiation
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeSoilMoisture
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeLeafWetness
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeVoltage
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeCurrent
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypePressure
            || ctx.dType == pTypeGeneral && ctx.dSubType == sTypeSoundLevel
            || ctx.dType == pTypeRFXSensor && ctx.dSubType == sTypeRFXSensorAD
            || ctx.dType == pTypeRFXSensor && ctx.dSubType == sTypeRFXSensorVolt
            )
            return "MultiMeter_Calendar";
        return "Meter_Calendar";
    }
    return "";
}

// ===========================================================================

std::string FormatMeterValue(double value, _eMeterType metertype)
{
    char szTmp[64];
    switch (metertype)
    {
    case MTYPE_ENERGY:
    case MTYPE_ENERGY_GENERATED:
        snprintf(szTmp, sizeof(szTmp), "%.3f", value);
        break;
    case MTYPE_GAS:
        snprintf(szTmp, sizeof(szTmp), "%.3f", value);
        break;
    case MTYPE_WATER:
        snprintf(szTmp, sizeof(szTmp), "%.3f", value);
        break;
    case MTYPE_COUNTER:
        snprintf(szTmp, sizeof(szTmp), "%.10g", value);
        break;
    default:
        return "0";
    }
    return szTmp;
}

std::string FormatMeterRate(double value, _eMeterType metertype)
{
    char szTmp[64];
    switch (metertype)
    {
    case MTYPE_ENERGY:
    case MTYPE_ENERGY_GENERATED:
        // Rate is in W (caller has already multiplied kWh delta by 1000 if
        // needed, or passes the raw Watt value here)
        snprintf(szTmp, sizeof(szTmp), "%.3f", value);
        break;
    case MTYPE_GAS:
        // Gas flow rates are shown to 2 decimal places (matching the original
        // inline code), unlike the 3 decimal places used for energy/water.
        snprintf(szTmp, sizeof(szTmp), "%.2f", value);
        break;
    case MTYPE_WATER:
        snprintf(szTmp, sizeof(szTmp), "%.3f", value);
        break;
    case MTYPE_COUNTER:
        snprintf(szTmp, sizeof(szTmp), "%.10g", value);
        break;
    default:
        return "0";
    }
    return szTmp;
}

// ===========================================================================

std::string FormatDate(int year, int month, int day)
{
    if (year < 0)
        year = 0;
    else if (year > 9999)
        year = 9999;

    if (month < 1)
        month = 1;
    else if (month > 12)
        month = 12;

    if (day < 1)
        day = 1;
    else if (day > 31)
        day = 31;

    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
    return buf;
}

std::string FormatDateTime(int year, int month, int day, int h, int m, int s)
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, h, m, s);
    return buf;
}

std::string FormatWindSpeed(int rawTenthsMs, int windunit, float windscale)
{
    char buf[16];
    if (windunit == WINDUNIT_Beaufort)
    {
        float ms = float(rawTenthsMs) * 0.1F;
        snprintf(buf, sizeof(buf), "%d", MStoBeaufort(ms));
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.1f", float(rawTenthsMs) * windscale);
    }
    return buf;
}

} // namespace server
} // namespace http
