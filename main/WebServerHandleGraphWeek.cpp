/*
 * WebServerHandleGraphWeek.cpp
 *
 * Phase 2 / F06 – handler for srange == "week".
 *
 * The logic here is extracted verbatim from CWebServer::Cmd_HandleGraph in
 * WebServerHandleGraph.cpp.  No behaviour has been changed.
 */

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerHandleGraphInternals.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <json/json.h>

#include "Helper.h"
#include "localtime_r.h"
#include "Logger.h"
#include "SQLHelper.h"

namespace http
{
namespace server
{

// ---------------------------------------------------------------------------
// sensor == "temp" || sensor == "hum"
//
// Mirrors the "Actual Year" calendar query + "today" query from the temp/hum
// branch of HandleGraphMonthYear() (WebServerHandleGraphMonthYear.cpp), but
// restricted to the 7-day trailing week window; the groupby, Previous Year
// and actmonth/actyear logic are intentionally omitted.
// ---------------------------------------------------------------------------
static void HandleGraphWeek_Temp(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx           = ctx.idx;
	const unsigned char dType    = ctx.dType;
	const unsigned char dSubType = ctx.dSubType;
	const std::string& sensor    = ctx.sensor;
	const std::string& srange    = ctx.srange;
	const std::map<std::string, std::string>& options = ctx.options;
	unsigned char tempsign = sql.m_tempsign[0];

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	result = sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
		" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
		" SetPoint_Max, SetPoint_Avg "
		"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
		" AND Date<='%q') ORDER BY Date ASC",
		dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
	int ii = 0;
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[7].substr(0, 16);

			if (
				(dType == pTypeRego6XXTemp)
				|| (dType == pTypeTEMP)
				|| (dType == pTypeTEMP_HUM)
				|| (dType == pTypeTEMP_HUM_BARO)
				|| (dType == pTypeTEMP_BARO)
				|| (dType == pTypeWIND)
				|| (dType == pTypeThermostat1)
				|| (dType == pTypeRadiator1)
				|| ((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp))
				|| ((dType == pTypeUV) && (dSubType == sTypeUV3))
				|| ((dType == pTypeGeneral) && (dSubType == sTypeSystemTemp))
				|| (dType == pTypeEvohomeZone) || (dType == pTypeThermostat6)
				|| (dType == pTypeEvohomeWater)
				|| ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
				)
			{
				bool bOK = true;
				if (dType == pTypeWIND)
				{
					bOK = ((dSubType != sTypeWINDNoTemp) && (dSubType != sTypeWINDNoTempNoChill));
				}
				if (bOK)
				{
					double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
					double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
					double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
					root["result"][ii]["te"] = te;
					root["result"][ii]["tm"] = tm;
					root["result"][ii]["ta"] = ta;
				}
			}
			if (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)))
			{
				double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
				double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
				root["result"][ii]["ch"] = ch;
				root["result"][ii]["cm"] = cm;
			}
			if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
			{
				root["result"][ii]["hu"] = sd[4];
			}
			if ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)))
			{
				if (dType == pTypeTEMP_HUM_BARO)
				{
					if (dSubType == sTypeTHBFloat)
					{
						snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
						root["result"][ii]["ba"] = szTmp;
					}
					else
						root["result"][ii]["ba"] = sd[5];
				}
				else if (dType == pTypeTEMP_BARO)
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
					root["result"][ii]["ba"] = szTmp;
				}
				else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
					root["result"][ii]["ba"] = szTmp;
				}
			}
			if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater) || (dType == pTypeThermostat6))
			{
				double sm = ConvertTemperature(atof(sd[8].c_str()), tempsign);
				double sx = ConvertTemperature(atof(sd[9].c_str()), tempsign);
				double se = ConvertTemperature(atof(sd[10].c_str()), tempsign);
				root["result"][ii]["sm"] = sm;
				root["result"][ii]["se"] = se;
				root["result"][ii]["sx"] = sx;
			}
			if ((dType == pTypeSetpoint) && (dSubType == sTypeSetpoint))
			{
				std::string value_unit = (options.count("ValueUnit") ? options.at("ValueUnit") : std::string{});
				if (
					(value_unit.empty())
					|| (value_unit == "°C")
					|| (value_unit == "°F")
					|| (value_unit == "C")
					|| (value_unit == "F")
					)
				{
					double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
					double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
					double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
					root["result"][ii]["te"] = te;
					root["result"][ii]["tm"] = tm;
					root["result"][ii]["ta"] = ta;
				}
				else
				{
					root["result"][ii]["te"] = atof(sd[1].c_str());
					root["result"][ii]["tm"] = atof(sd[0].c_str());
					root["result"][ii]["ta"] = atof(sd[6].c_str());
				}
			}

			ii++;
		}
	}
	// add today (have to calculate it)
	result = sql.safe_query("SELECT MIN(Temperature), MAX(Temperature),"
		" MIN(Chill), MAX(Chill), AVG(Humidity),"
		" AVG(Barometer), AVG(Temperature), MIN(SetPoint),"
		" MAX(SetPoint), AVG(SetPoint) "
		"FROM Temperature WHERE (DeviceRowID==%" PRIu64 ""
		" AND Date>='%q')",
		idx, szDateEnd.c_str());
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];

		root["result"][ii]["d"] = szDateEnd;
		if (
			(dType == pTypeRego6XXTemp)
			|| (dType == pTypeTEMP)
			|| (dType == pTypeTEMP_HUM)
			|| (dType == pTypeTEMP_HUM_BARO)
			|| (dType == pTypeTEMP_BARO)
			|| (dType == pTypeWIND)
			|| (dType == pTypeThermostat1)
			|| (dType == pTypeRadiator1)
			|| ((dType == pTypeUV) && (dSubType == sTypeUV3))
			|| ((dType == pTypeWIND) && (dSubType == sTypeWIND4))
			|| (dType == pTypeEvohomeZone) || (dType == pTypeThermostat6)
			|| (dType == pTypeEvohomeWater)
			)
		{
			double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
			double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
			double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
			root["result"][ii]["te"] = te;
			root["result"][ii]["tm"] = tm;
			root["result"][ii]["ta"] = ta;
		}
		if (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)))
		{
			double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
			double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
			root["result"][ii]["ch"] = ch;
			root["result"][ii]["cm"] = cm;
		}
		if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
		{
			root["result"][ii]["hu"] = sd[4];
		}
		if ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)))
		{
			if (dType == pTypeTEMP_HUM_BARO)
			{
				if (dSubType == sTypeTHBFloat)
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
					root["result"][ii]["ba"] = szTmp;
				}
				else
					root["result"][ii]["ba"] = sd[5];
			}
			else if (dType == pTypeTEMP_BARO)
			{
				snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
				root["result"][ii]["ba"] = szTmp;
			}
			else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
			{
				snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
				root["result"][ii]["ba"] = szTmp;
			}
		}
		if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater) || (dType == pTypeThermostat6))
		{
			double sx = ConvertTemperature(atof(sd[8].c_str()), tempsign);
			double sm = ConvertTemperature(atof(sd[7].c_str()), tempsign);
			double se = ConvertTemperature(atof(sd[9].c_str()), tempsign);
			root["result"][ii]["se"] = se;
			root["result"][ii]["sm"] = sm;
			root["result"][ii]["sx"] = sx;
		}
		if ((dType == pTypeSetpoint) && (dSubType == sTypeSetpoint))
		{
			std::string value_unit = (options.count("ValueUnit") ? options.at("ValueUnit") : std::string{});
			if (
				(value_unit.empty())
				|| (value_unit == "°C")
				|| (value_unit == "°F")
				|| (value_unit == "C")
				|| (value_unit == "F")
				)
			{
				double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
				double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
				double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
				root["result"][ii]["te"] = te;
				root["result"][ii]["tm"] = tm;
				root["result"][ii]["ta"] = ta;
			}
			else
			{
				root["result"][ii]["te"] = atof(sd[1].c_str());
				root["result"][ii]["tm"] = atof(sd[0].c_str());
				root["result"][ii]["ta"] = atof(sd[6].c_str());
			}
		}
		ii++;
	}
}

// ---------------------------------------------------------------------------
// sensor == "Percentage"
// ---------------------------------------------------------------------------
static void HandleGraphWeek_Percentage(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	std::vector<std::vector<std::string>> result = sql.safe_query("SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM %s WHERE (DeviceRowID==%" PRIu64
		" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
		dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
	int ii = 0;
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[3].substr(0, 16);
			root["result"][ii]["v_min"] = sd[0];
			root["result"][ii]["v_max"] = sd[1];
			root["result"][ii]["v_avg"] = sd[2];
			ii++;
		}
	}
	// add today (have to calculate it)
	result = sql.safe_query("SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx,
		szDateEnd.c_str());
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];
		root["result"][ii]["d"] = szDateEnd;
		root["result"][ii]["v_min"] = sd[0];
		root["result"][ii]["v_max"] = sd[1];
		root["result"][ii]["v_avg"] = sd[2];
		ii++;
	}
}

// ---------------------------------------------------------------------------
// sensor == "fan"
// ---------------------------------------------------------------------------
static void HandleGraphWeek_Fan(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	std::vector<std::vector<std::string>> result = sql.safe_query("SELECT Speed_Min, Speed_Max, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
		dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
	int ii = 0;
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[2].substr(0, 16);
			root["result"][ii]["v_max"] = sd[1];
			root["result"][ii]["v_min"] = sd[0];
			ii++;
		}
	}
	// add today (have to calculate it)
	result = sql.safe_query("SELECT MIN(Speed), MAX(Speed) FROM Fan WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];
		root["result"][ii]["d"] = szDateEnd;
		root["result"][ii]["v_max"] = sd[1];
		root["result"][ii]["v_min"] = sd[0];
		ii++;
	}
}

// ---------------------------------------------------------------------------
// sensor == "uv"
// ---------------------------------------------------------------------------
static void HandleGraphWeek_UV(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	std::vector<std::vector<std::string>> result = sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(),
		idx, szDateStart.c_str(), szDateEnd.c_str());
	int ii = 0;
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[1].substr(0, 16);
			root["result"][ii]["uvi"] = sd[0];
			ii++;
		}
	}
	// add today (have to calculate it)
	result = sql.safe_query("SELECT MAX(Level) FROM UV WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];

		root["result"][ii]["d"] = szDateEnd;
		root["result"][ii]["uvi"] = sd[0];
		ii++;
	}
}

// ---------------------------------------------------------------------------
// sensor == "wind"
//
// Only "wind" is handled here, matching HandleGraphMonthYear() which also has
// no dedicated "winddir" branch (winddir devices share the "wind" dbasetable
// lookup but fall through without a result).
// ---------------------------------------------------------------------------
static void HandleGraphWeek_Wind(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	int ii = 0;

	std::vector<std::vector<std::string>> result = sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
		" Gust_Max, Date "
		"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
		" AND Date<='%q') ORDER BY Date ASC",
		dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
	if (!result.empty())
	{
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[5].substr(0, 16);
			root["result"][ii]["di"] = sd[0];

			int intSpeed = atoi(sd[2].c_str());
			int intGust = atoi(sd[4].c_str());
			root["result"][ii]["sp"] = FormatWindSpeed(intSpeed, sql.m_windunit, sql.m_windscale);
			root["result"][ii]["gu"] = FormatWindSpeed(intGust, sql.m_windunit, sql.m_windscale);
			ii++;
		}
	}
	// add today (have to calculate it)
	result = sql.safe_query("SELECT AVG(Direction), MIN(Speed), MAX(Speed),"
		" MIN(Gust), MAX(Gust) "
		"FROM Wind WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC",
		idx, szDateEnd.c_str());
	if (!result.empty())
	{
		std::vector<std::string> sd = result[0];

		root["result"][ii]["d"] = szDateEnd;
		root["result"][ii]["di"] = sd[0];

		int intSpeed = atoi(sd[2].c_str());
		int intGust = atoi(sd[4].c_str());
		root["result"][ii]["sp"] = FormatWindSpeed(intSpeed, sql.m_windunit, sql.m_windscale);
		root["result"][ii]["gu"] = FormatWindSpeed(intGust, sql.m_windunit, sql.m_windscale);
		ii++;
	}
}

// ---------------------------------------------------------------------------
// sensor == "counter" for the dTypes whose CalcDbasetableWeek() resolves to
// MultiMeter_Calendar (Value1..Value6 columns, no plain "Value" column) but
// that are not P1Power / CURRENT / CURRENTENERGY, which keep their existing
// handling.  Mirrors the corresponding dType branches of
// HandleGraphMonthYear_Counter() (WebServerHandleGraphMonthYear.cpp) without
// the groupby / actmonth / actyear logic, since week always ends today.
// ---------------------------------------------------------------------------
static bool IsWeekCounterAggregateType(unsigned char dType, unsigned char dSubType)
{
	return (dType == pTypeAirQuality)
		|| ((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness)))
		|| ((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
		|| ((dType == pTypeGeneral) && ((dSubType == sTypeVisibility) || (dSubType == sTypeDistance) || (dSubType == sTypeSolarRadiation)
			|| (dSubType == sTypeVoltage) || (dSubType == sTypeCurrent) || (dSubType == sTypePressure) || (dSubType == sTypeSoundLevel)))
		|| (dType == pTypeLux)
		|| (dType == pTypeWEIGHT)
		|| (dType == pTypeUsage);
}

static void HandleGraphWeek_CounterAggregate(const GraphContext& ctx, Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable, const std::string& szDateStart, const std::string& szDateEnd)
{
	const uint64_t idx           = ctx.idx;
	const unsigned char dType    = ctx.dType;
	const unsigned char dSubType = ctx.dSubType;
	const _eMeterType metertype  = ctx.metertype;

	char szTmp[64];
	int ii = 0;
	std::vector<std::vector<std::string>> result;

	if (dType == pTypeAirQuality)
	{
		result = sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[3].substr(0, 16);
				root["result"][ii]["co2_min"] = sd[0];
				root["result"][ii]["co2_max"] = sd[1];
				root["result"][ii]["co2_avg"] = sd[2];
				ii++;
			}
		}
		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			root["result"][ii]["co2_min"] = result[0][0];
			root["result"][ii]["co2_max"] = result[0][1];
			root["result"][ii]["co2_avg"] = result[0][2];
			ii++;
		}
	}
	else if (((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
		((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt))))
	{
		result = sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[2].substr(0, 16);
				root["result"][ii]["v_min"] = sd[0];
				root["result"][ii]["v_max"] = sd[1];
				ii++;
			}
		}
		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			root["result"][ii]["v_min"] = result[0][0];
			root["result"][ii]["v_max"] = result[0][1];
			ii++;
		}
	}
	else if (((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) || ((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) || ((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) || ((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel)))
	{
		float vdiv = 10.0F;
		if (((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) || ((dType == pTypeGeneral) && (dSubType == sTypeCurrent)))
		{
			vdiv = 1000.0F;
		}

		result = sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				float fValue1 = float(atof(sd[0].c_str())) / vdiv;
				float fValue2 = float(atof(sd[1].c_str())) / vdiv;
				float fValue3 = float(atof(sd[2].c_str())) / vdiv;
				root["result"][ii]["d"] = sd[3].substr(0, 16);

				if (metertype == 1)
				{
					if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
					{
						// Inches
						fValue1 *= 0.3937007874015748F;
						fValue2 *= 0.3937007874015748F;
					}
					else
					{
						// Miles
						fValue1 *= 0.6214F;
						fValue2 *= 0.6214F;
					}
				}
				if (((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) || ((dType == pTypeGeneral) && (dSubType == sTypeCurrent)))
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", fValue1);
					root["result"][ii]["v_min"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.3f", fValue2);
					root["result"][ii]["v_max"] = szTmp;
					if (fValue3 != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", fValue3);
						root["result"][ii]["v_avg"] = szTmp;
					}
				}
				else
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", fValue1);
					root["result"][ii]["v_min"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fValue2);
					root["result"][ii]["v_max"] = szTmp;
					if (fValue3 != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.1f", fValue3);
						root["result"][ii]["v_avg"] = szTmp;
					}
				}
				ii++;
			}
		}

		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			float fValue1 = float(atof(result[0][0].c_str())) / vdiv;
			float fValue2 = float(atof(result[0][1].c_str())) / vdiv;
			if (metertype == 1)
			{
				if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
				{
					// Inches
					fValue1 *= 0.3937007874015748F;
					fValue2 *= 0.3937007874015748F;
				}
				else
				{
					// Miles
					fValue1 *= 0.6214F;
					fValue2 *= 0.6214F;
				}
			}

			if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
				snprintf(szTmp, sizeof(szTmp), "%.3f", fValue1);
			else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
				snprintf(szTmp, sizeof(szTmp), "%.3f", fValue1);
			else
				snprintf(szTmp, sizeof(szTmp), "%.1f", fValue1);
			root["result"][ii]["v_min"] = szTmp;
			if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
				snprintf(szTmp, sizeof(szTmp), "%.3f", fValue2);
			else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
				snprintf(szTmp, sizeof(szTmp), "%.3f", fValue2);
			else
				snprintf(szTmp, sizeof(szTmp), "%.1f", fValue2);
			root["result"][ii]["v_max"] = szTmp;
			ii++;
		}
	}
	else if (dType == pTypeLux)
	{
		result = sql.safe_query("SELECT Value1,Value2,Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[3].substr(0, 16);
				root["result"][ii]["lux_min"] = sd[0];
				root["result"][ii]["lux_max"] = sd[1];
				root["result"][ii]["lux_avg"] = sd[2];
				ii++;
			}
		}
		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			root["result"][ii]["lux_min"] = result[0][0];
			root["result"][ii]["lux_max"] = result[0][1];
			root["result"][ii]["lux_avg"] = result[0][2];
			ii++;
		}
	}
	else if (dType == pTypeWEIGHT)
	{
		result = sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[2].substr(0, 16);
				snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(sd[0].c_str()) / 10.0F);
				root["result"][ii]["v_min"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(sd[1].c_str()) / 10.0F);
				root["result"][ii]["v_max"] = szTmp;
				ii++;
			}
		}
		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(result[0][0].c_str()) / 10.0F);
			root["result"][ii]["v_min"] = szTmp;
			snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(result[0][1].c_str()) / 10.0F);
			root["result"][ii]["v_max"] = szTmp;
			ii++;
		}
	}
	else if (dType == pTypeUsage)
	{
		result = sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[3].substr(0, 16);
				root["result"][ii]["u_min"] = atof(sd[0].c_str()) / 10.0F;
				root["result"][ii]["u_max"] = atof(sd[1].c_str()) / 10.0F;
				root["result"][ii]["u_avg"] = static_cast<int>((atof(sd[2].c_str()) / 10.0F) + 0.5F);
				ii++;
			}
		}
		// add today (have to calculate it)
		result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		if (!result.empty())
		{
			root["result"][ii]["d"] = szDateEnd;
			root["result"][ii]["u_min"] = atof(result[0][0].c_str()) / 10.0F;
			root["result"][ii]["u_max"] = atof(result[0][1].c_str()) / 10.0F;
			root["result"][ii]["u_avg"] = static_cast<int>((atof(result[0][2].c_str()) / 10.0F) + 0.5F);
			ii++;
		}
	}
}

// ---------------------------------------------------------------------------
void HandleGraphWeek(const GraphContext& ctx, const request& req,
                     Json::Value& root, CSQLHelper& sql)
{
	const uint64_t idx           = ctx.idx;
	const unsigned char dType    = ctx.dType;
	const unsigned char dSubType = ctx.dSubType;
	const _eMeterType metertype  = ctx.metertype;
	const bool bIsManagedCounter = ctx.bIsManagedCounter;
	const double divider         = ctx.divider;
	const double AddjMulti       = ctx.AddjMulti;
	const std::string& sensor    = ctx.sensor;
	const std::string& srange    = ctx.srange;
	const std::map<std::string, std::string>& options = ctx.options;
	const struct tm& tm1         = ctx.tmNow;

	std::string dbasetable = CalcDbasetableWeek(ctx);

	std::vector<std::vector<std::string>> result;

	if (sensor == "rain")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		std::string szDateEnd = FormatDate(tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);

		// Subtract one week
		time_t weekbefore;
		struct tm tm2;
		getNoon(weekbefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 7); // We only want the date
		std::string szDateStart = FormatDate(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

		result = sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		int ii = 0;
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[2].substr(0, 16);
				double mmval = atof(sd[0].c_str());
				mmval *= AddjMulti;
				char szMm[16];
				snprintf(szMm, sizeof(szMm), "%.1f", mmval);
				root["result"][ii]["mm"] = szMm;
				ii++;
			}
		}
		// add today (have to calculate it)
		if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
		{
			result = sql.safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1", idx,
				szDateEnd.c_str());
		}
		else
		{
			result = sql.safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
		}
		if (!result.empty())
		{
			std::vector<std::string> sd = result[0];

			float total_min = static_cast<float>(atof(sd[0].c_str()));
			float total_max = static_cast<float>(atof(sd[1].c_str()));
			// int rate = atoi(sd[2].c_str());

			double total_real = 0;
			if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
			{
				total_real = total_max;
			}
			else
			{
				total_real = std::max(0.0, static_cast<double>(total_max - total_min));
			}
			total_real *= AddjMulti;
			char szMm[16];
			snprintf(szMm, sizeof(szMm), "%.1f", total_real);
			root["result"][ii]["d"] = szDateEnd;
			root["result"][ii]["mm"] = szMm;
			ii++;
		}
	}
	else if (sensor == "counter")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;
		root["ValueQuantity"] = (options.count("ValueQuantity") ? options.at("ValueQuantity") : std::string{});
		root["ValueUnits"] = (options.count("ValueUnits") ? options.at("ValueUnits") : std::string{});
		root["Divider"] = divider;

		std::string szDateEnd = FormatDate(tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);

		// Subtract one week
		time_t weekbefore;
		struct tm tm2;
		getNoon(weekbefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 7); // We only want the date
		std::string szDateStart = FormatDate(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

		// MultiMeter_Calendar devices that don't have a plain "Value" column
		// (Air Quality, Lux, Weight, Usage, General/Voltage etc.) can't use the
		// generic query below; handle them with their own per-dType aggregates
		// and skip the rest of this branch.
		if (IsWeekCounterAggregateType(dType, dSubType))
		{
			HandleGraphWeek_CounterAggregate(ctx, root, sql, dbasetable, szDateStart, szDateEnd);
			return;
		}

		int ii = 0;
		if (dType == pTypeP1Power)
		{
			int P1DisplayType = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
			sql.GetPreferencesVar("P1DisplayType", P1DisplayType);
			root["P1DisplayType"] = P1DisplayType;

			result = sql.safe_query("SELECT Value1,Value2,Value5,Value6,Price,Date FROM %s WHERE (DeviceRowID==%" PRIu64
				" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
			if (!result.empty())
			{
				bool bHaveDeliverd = false;
				for (const auto& sd : result)
				{
					root["result"][ii]["d"] = sd[5].substr(0, 16);
					std::string szValueUsage1 = sd[0];
					std::string szValueDeliv1 = sd[1];
					std::string szValueUsage2 = sd[2];
					std::string szValueDeliv2 = sd[3];

					float fPrice = std::stof(sd[4]);

					float fUsage1 = (float)(atof(szValueUsage1.c_str()));
					float fUsage2 = (float)(atof(szValueUsage2.c_str()));
					float fDeliv1 = (float)(atof(szValueDeliv1.c_str()));
					float fDeliv2 = (float)(atof(szValueDeliv2.c_str()));

					fDeliv1 = (fDeliv1 < 10) ? 0 : fDeliv1;
					fDeliv2 = (fDeliv2 < 10) ? 0 : fDeliv2;

					if ((fDeliv1 != 0) || (fDeliv2 != 0))
						bHaveDeliverd = true;
					char szV[32];
					snprintf(szV, sizeof(szV), "%.3f", fUsage1 / divider);
					root["result"][ii]["v1"] = szV;
					snprintf(szV, sizeof(szV), "%.3f", fUsage2 / divider);
					root["result"][ii]["v2"] = szV;
					snprintf(szV, sizeof(szV), "%.3f", fDeliv1 / divider);
					root["result"][ii]["r1"] = szV;
					snprintf(szV, sizeof(szV), "%.3f", fDeliv2 / divider);
					root["result"][ii]["r2"] = szV;

					char szP[16];
					snprintf(szP, sizeof(szP), "%.4f", fPrice);
					root["result"][ii]["p"] = szP;
					ii++;
				}
				if (bHaveDeliverd)
				{
					root["delivered"] = true;
				}
			}
		}
		else
		{
			result = sql.safe_query("SELECT Value, Price, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					root["result"][ii]["d"] = sd[2].substr(0, 16);
					std::string szValue = sd[0];
					float fPrice = std::stof(sd[1]);

					char szV[32];
					switch (metertype)
					{
					case MTYPE_ENERGY:
					case MTYPE_ENERGY_GENERATED:
						snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
						szValue = szV;
						break;
					case MTYPE_GAS:
						snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
						szValue = szV;
						break;
					case MTYPE_WATER:
						snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
						szValue = szV;
						break;
					case MTYPE_COUNTER:
						snprintf(szV, sizeof(szV), "%.10g", atof(szValue.c_str()) / divider);
						szValue = szV;
						break;
					default:
						szValue = "0";
						break;
					}
					root["result"][ii]["v"] = szValue;

					char szP[16];
					snprintf(szP, sizeof(szP), "%.4f", fPrice);
					root["result"][ii]["p"] = szP;
					ii++;
				}
			}
		}
		// add today (have to calculate it)
		if (dType == pTypeP1Power)
		{
			int P1DisplayType = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
			sql.GetPreferencesVar("P1DisplayType", P1DisplayType);
			root["P1DisplayType"] = P1DisplayType;

			result = sql.safe_query("SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM "
				"MultiMeter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
				idx, szDateEnd.c_str());
			if (!result.empty())
			{
				std::vector<std::string> sd = result[0];

				int64_t total_min_usage_1 = std::stoll(sd[0]);
				int64_t total_max_usage_1 = std::stoll(sd[1]);
				int64_t total_min_usage_2 = std::stoll(sd[4]);
				int64_t total_max_usage_2 = std::stoll(sd[5]);
				int64_t total_real_usage_1, total_real_usage_2;
				int64_t total_min_deliv_1 = std::stoll(sd[2]);
				int64_t total_max_deliv_1 = std::stoll(sd[3]);
				int64_t total_min_deliv_2 = std::stoll(sd[6]);
				int64_t total_max_deliv_2 = std::stoll(sd[7]);
				int64_t total_real_deliv_1, total_real_deliv_2;

				bool bHaveDeliverd = false;

				total_real_usage_1 = total_max_usage_1 - total_min_usage_1;
				total_real_usage_2 = total_max_usage_2 - total_min_usage_2;

				total_real_deliv_1 = total_max_deliv_1 - total_min_deliv_1;
				total_real_deliv_2 = total_max_deliv_2 - total_min_deliv_2;
				if ((total_real_deliv_1 != 0) || (total_real_deliv_2 != 0))
					bHaveDeliverd = true;

				root["result"][ii]["d"] = szDateEnd;

				char szV[32];
				snprintf(szV, sizeof(szV), "%.3f", double(total_real_usage_1) / divider);
				root["result"][ii]["v1"] = szV;

				snprintf(szV, sizeof(szV), "%.3f", double(total_real_usage_2) / divider);
				root["result"][ii]["v2"] = szV;

				snprintf(szV, sizeof(szV), "%.3f", double(total_real_deliv_1) / divider);
				root["result"][ii]["r1"] = szV;

				snprintf(szV, sizeof(szV), "%.3f", double(total_real_deliv_2) / divider);
				root["result"][ii]["r2"] = szV;

				//Calculate price
				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				char szP[16];
				snprintf(szP, sizeof(szP), "%.4f", fPrice);
				root["result"][ii]["p"] = szP;

				ii++;
				if (bHaveDeliverd)
				{
					root["delivered"] = true;
				}
			}
		}
		else if (!bIsManagedCounter)
		{
			// get the first value of the day
			result = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd.c_str());
			if (!result.empty())
			{
				std::vector<std::string> sd = result[0];

				int64_t total_min = std::stoll(sd[0]);
				int64_t total_max = total_min;
				int64_t total_real;

				// get the last value of the day
				result = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx, szDateEnd.c_str());
				if (!result.empty())
				{
					std::vector<std::string> sd = result[0];
					total_max = std::stoull(sd[0].c_str());
				}

				total_real = total_max - total_min;
				std::string szValue = std::to_string(total_real);
				char szV[32];
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
					szValue = szV;
					break;
				case MTYPE_GAS:
					snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
					szValue = szV;
					break;
				case MTYPE_WATER:
					snprintf(szV, sizeof(szV), "%.3f", atof(szValue.c_str()) / divider);
					szValue = szV;
					break;
				case MTYPE_COUNTER:
					snprintf(szV, sizeof(szV), "%.10g", atof(szValue.c_str()) / divider);
					szValue = szV;
					break;
				default:
					szValue = "0";
					break;
				}

				root["result"][ii]["d"] = szDateEnd;
				root["result"][ii]["v"] = szValue;

				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				char szP[16];
				snprintf(szP, sizeof(szP), "%.4f", fPrice);
				root["result"][ii]["p"] = szP;

				ii++;
			}
		}
	}
	else if ((sensor == "temp") || (sensor == "hum"))
	{
		const DateRange dr = CalcWeekRange(tm1);
		HandleGraphWeek_Temp(ctx, root, sql, dbasetable, dr.start, dr.end);
	}
	else if (sensor == "Percentage")
	{
		const DateRange dr = CalcWeekRange(tm1);
		HandleGraphWeek_Percentage(ctx, root, sql, dbasetable, dr.start, dr.end);
	}
	else if (sensor == "fan")
	{
		const DateRange dr = CalcWeekRange(tm1);
		HandleGraphWeek_Fan(ctx, root, sql, dbasetable, dr.start, dr.end);
	}
	else if (sensor == "uv")
	{
		const DateRange dr = CalcWeekRange(tm1);
		HandleGraphWeek_UV(ctx, root, sql, dbasetable, dr.start, dr.end);
	}
	else if (sensor == "wind")
	{
		const DateRange dr = CalcWeekRange(tm1);
		HandleGraphWeek_Wind(ctx, root, sql, dbasetable, dr.start, dr.end);
	}
}

} // namespace server
} // namespace http
