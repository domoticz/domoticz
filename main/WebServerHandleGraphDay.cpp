/*
 * WebServerHandleGraphDay.cpp
 *
 * Phase 2 / F05 – handler for srange == "day".
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
// Helper: compute dbasetable for the "day" range
// ---------------------------------------------------------------------------
static std::string CalcDbasetableDay(const GraphContext& ctx)
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

// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter_P1Power(
	const GraphContext& ctx, const request& /*req*/,
	Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;
	const struct tm& tm1      = ctx.tmNow;

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	int P1DisplayType = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
	sql.GetPreferencesVar("P1DisplayType", P1DisplayType);
	root["P1DisplayType"] = P1DisplayType;

	result = sql.safe_query("SELECT Value1, Value2, Value3, Value4, Value5, Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC",
		dbasetable.c_str(), idx);
	if (!result.empty())
	{
		int ii = 0;
		bool bHaveDeliverd = false;
		bool bHaveFirstValue = false;
		int64_t lastUsage1, lastUsage2, lastDeliv1, lastDeliv2;
		time_t lastTime = 0;

		int64_t firstUsage1 = 0;
		int64_t firstUsage2 = 0;
		int64_t firstDeliv1 = 0;
		int64_t firstDeliv2 = 0;

		int lastDay = 0;

		for (const auto& sd : result)
		{
			int64_t actUsage1 = std::stoll(sd[0]);
			int64_t actUsage2 = std::stoll(sd[4]);
			int64_t actDeliv1 = std::stoll(sd[1]);
			int64_t actDeliv2 = std::stoll(sd[5]);
			actDeliv1 = (actDeliv1 < 10) ? 0 : actDeliv1;
			actDeliv2 = (actDeliv2 < 10) ? 0 : actDeliv2;

			std::string stime = sd[6];
			struct tm ntime;
			time_t atime;
			ParseSQLdatetime(atime, ntime, stime, -1);
			if (lastDay != ntime.tm_mday)
			{
				lastDay = ntime.tm_mday;
				firstUsage1 = actUsage1;
				firstUsage2 = actUsage2;
				firstDeliv1 = actDeliv1;
				firstDeliv2 = actDeliv2;
			}

			if (bHaveFirstValue)
			{
				if (
					(actUsage1 < lastUsage1)
					|| (actUsage2 < lastUsage2)
					|| (actDeliv1 < lastDeliv1)
					|| (actDeliv2 < lastDeliv2)
					|| (atime <= lastTime)
					)
				{
					//daylight change happened, meter changed?, ignoring  for now
					lastUsage1 = actUsage1;
					lastUsage2 = actUsage2;
					lastDeliv1 = actDeliv1;
					lastDeliv2 = actDeliv2;
					lastTime = atime;
					continue;
				}

				long curUsage1 = (long)(actUsage1 - lastUsage1);
				long curUsage2 = (long)(actUsage2 - lastUsage2);
				long curDeliv1 = (long)(actDeliv1 - lastDeliv1);
				long curDeliv2 = (long)(actDeliv2 - lastDeliv2);

				float tdiff = static_cast<float>(difftime(atime, lastTime));
				if (tdiff == 0)
					tdiff = 1;
				float tlaps = 3600.0F / tdiff;
				curUsage1 *= int(tlaps);
				curUsage2 *= int(tlaps);
				curDeliv1 *= int(tlaps);
				curDeliv2 *= int(tlaps);

				if ((curUsage1 < 0) || (curUsage1 > 100000))
					curUsage1 = 0;
				if ((curUsage2 < 0) || (curUsage2 > 100000))
					curUsage2 = 0;
				if ((curDeliv1 < 0) || (curDeliv1 > 100000))
					curDeliv1 = 0;
				if ((curDeliv2 < 0) || (curDeliv2 > 100000))
					curDeliv2 = 0;

				root["result"][ii]["d"] = sd[6].substr(0, 16);

				if ((curDeliv1 != 0) || (curDeliv2 != 0))
					bHaveDeliverd = true;

				if (P1DisplayType == 0)
				{
					//Low/High Tarrif
					snprintf(szTmp, sizeof(szTmp), "%ld", curUsage1);
					root["result"][ii]["v1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%ld", curUsage2);
					root["result"][ii]["v2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%ld", -curDeliv1);
					root["result"][ii]["r1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%ld", -curDeliv2);
					root["result"][ii]["r2"] = szTmp;
				}
				else
				{
					//Simple
					snprintf(szTmp, sizeof(szTmp), "%ld", curUsage1 + curUsage2);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%ld", -(curDeliv1 + curDeliv2));
					root["result"][ii]["r"] = szTmp;
				}
				long pUsage1 = (long)(actUsage1 - firstUsage1);
				long pUsage2 = (long)(actUsage2 - firstUsage2);

				snprintf(szTmp, sizeof(szTmp), "%ld", pUsage1 + pUsage2);
				root["result"][ii]["eu"] = szTmp;
				if (bHaveDeliverd)
				{
					long pDeliv1 = (long)(actDeliv1 - firstDeliv1);
					long pDeliv2 = (long)(actDeliv2 - firstDeliv2);
					snprintf(szTmp, sizeof(szTmp), "%ld", -(pDeliv1 + pDeliv2));
					root["result"][ii]["eg"] = szTmp;
				}

				ii++;
			}
			else
			{
				bHaveFirstValue = true;
				if ((ntime.tm_hour != 0) && (ntime.tm_min != 0))
				{
					struct tm ltime;
					struct tm tm1copy = tm1;
					localtime_r(&atime, &tm1copy);
					getNoon(atime, ltime, ntime.tm_year + 1900, ntime.tm_mon + 1,
						ntime.tm_mday - 1); // We're only interested in finding the date
					std::string szDate = FormatDate(ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
					std::vector<std::vector<std::string>> result2;
					result2 = sql.safe_query(
						"SELECT Counter1, Counter2, Counter3, Counter4 FROM Multimeter_Calendar WHERE (DeviceRowID==%" PRIu64
						") AND (Date=='%q')",
						idx, szDate.c_str());
					if (!result2.empty())
					{
						std::vector<std::string> sd = result2[0];
						firstUsage1 = std::stoll(sd[0]);
						firstDeliv1 = std::stoll(sd[1]);
						firstUsage2 = std::stoll(sd[2]);
						firstDeliv2 = std::stoll(sd[3]);
						lastDay = ntime.tm_mday;
					}
				}
			}
			lastUsage1 = actUsage1;
			lastUsage2 = actUsage2;
			lastDeliv1 = actDeliv1;
			lastDeliv2 = actDeliv2;
			lastTime = atime;
		}
		if (bHaveDeliverd)
		{
			root["delivered"] = true;
		}
	}
}

// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter_Current(
	const GraphContext& ctx, const request& /*req*/,
	Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	// CM113
	int displaytype = 0;
	int voltage = 230;
	sql.GetPreferencesVar("CM113DisplayType", displaytype);
	sql.GetPreferencesVar("ElectricVoltage", voltage);

	root["displaytype"] = displaytype;

	result = sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
	if (!result.empty())
	{
		int ii = 0;
		bool bHaveL1 = false;
		bool bHaveL2 = false;
		bool bHaveL3 = false;
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[3].substr(0, 16);

			float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0F);
			float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0F);
			float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0F);

			if (fval1 != 0)
				bHaveL1 = true;
			if (fval2 != 0)
				bHaveL2 = true;
			if (fval3 != 0)
				bHaveL3 = true;

			if (displaytype == 0)
			{
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval1);
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval2);
				root["result"][ii]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval3);
				root["result"][ii]["v3"] = szTmp;
			}
			else
			{
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval1 * voltage));
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval2 * voltage));
				root["result"][ii]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval3 * voltage));
				root["result"][ii]["v3"] = szTmp;
			}
			ii++;
		}
		if ((!bHaveL1) && (!bHaveL2) && (!bHaveL3))
		{
			root["haveL1"] = true; // show at least something
		}
		else
		{
			if (bHaveL1)
				root["haveL1"] = true;
			if (bHaveL2)
				root["haveL2"] = true;
			if (bHaveL3)
				root["haveL3"] = true;
		}
	}
}

// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter_CurrentEnergy(
	const GraphContext& ctx, const request& /*req*/,
	Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable)
{
	const uint64_t idx        = ctx.idx;
	const std::string& sensor = ctx.sensor;
	const std::string& srange = ctx.srange;

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	root["status"] = "OK";
	root["title"] = "Graph " + sensor + " " + srange;

	// CM113
	int displaytype = 0;
	int voltage = 230;
	sql.GetPreferencesVar("CM113DisplayType", displaytype);
	sql.GetPreferencesVar("ElectricVoltage", voltage);

	root["displaytype"] = displaytype;

	result = sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
	if (!result.empty())
	{
		int ii = 0;
		bool bHaveL1 = false;
		bool bHaveL2 = false;
		bool bHaveL3 = false;
		for (const auto& sd : result)
		{
			root["result"][ii]["d"] = sd[3].substr(0, 16);

			float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0F);
			float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0F);
			float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0F);

			if (fval1 != 0)
				bHaveL1 = true;
			if (fval2 != 0)
				bHaveL2 = true;
			if (fval3 != 0)
				bHaveL3 = true;

			if (displaytype == 0)
			{
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval1);
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval2);
				root["result"][ii]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.1f", fval3);
				root["result"][ii]["v3"] = szTmp;
			}
			else
			{
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval1 * voltage));
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval2 * voltage));
				root["result"][ii]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%d", int(fval3 * voltage));
				root["result"][ii]["v3"] = szTmp;
			}
			ii++;
		}
		if ((!bHaveL1) && (!bHaveL2) && (!bHaveL3))
		{
			root["haveL1"] = true; // show at least something
		}
		else
		{
			if (bHaveL1)
				root["haveL1"] = true;
			if (bHaveL2)
				root["haveL2"] = true;
			if (bHaveL3)
				root["haveL3"] = true;
		}
	}
}

// ---------------------------------------------------------------------------
// Slot accumulation (method == 0) for regular meter/counter devices.
// Operates on a pre-fetched result set.
// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter_Meter_SlotAccum(
	const GraphContext& ctx,
	const request& req,
	const std::vector<std::vector<std::string>>& rows,
	Json::Value& root,
	CSQLHelper& sql,
	const std::string& dbasetable,
	bool bIsManagedCounter)
{
	const uint64_t idx          = ctx.idx;
	const _eMeterType metertype = ctx.metertype;
	const double divider        = ctx.divider;
	const struct tm& tm1        = ctx.tmNow;

	char szTmp[64];

	int ii = 0;

	bool bHaveFirstValue = false;
	bool bHaveFirstRealValue = false;
	int64_t ulFirstValue = 0;
	int64_t ulRealFirstValue = 0;
	int lastDay = 0;
	std::string szLastDateTimeHour;
	std::string szActDateTimeHour;
	std::string szlastDateTime;
	int64_t ulLastValue = 0;

	int lastHour = 0;
	time_t lastTime = 0;

	// Sub-hourly slot grouping for kWh meters
	int resolution = sql.m_PriceResolution.load();
	std::string sResolution = request::findValue(&req, "resolution");
	if (!sResolution.empty()) {
		int reqResolution = atoi(sResolution.c_str());
		if (reqResolution == 15 || reqResolution == 30 || reqResolution == 60)
			resolution = reqResolution;
	}
	bool bUseSubHourSlots = (resolution < 60);
	int lastSlot = -1;  // replaces lastHour for 15-min mode
	int currentSlot = 0;

	double lastUsageValue = 0;

	for (const auto& sd : rows)
	{
		// bars / hour
		int64_t actValue = std::stoll(sd[0]);
		szlastDateTime = sd[1].substr(0, 16);

		struct tm ntime;
		time_t atime;
		ParseSQLdatetime(atime, ntime, sd[1], -1);

		// Format timestamp for 15-min or hourly slots
		if (bUseSubHourSlots)
		{
			int slotMin = (ntime.tm_min / resolution) * resolution;
			char szSlot[32];
			snprintf(szSlot, sizeof(szSlot), "%s%02d", sd[1].substr(0, 14).c_str(), slotMin);
			szActDateTimeHour = szSlot;
		}
		else
		{
			szActDateTimeHour = sd[1].substr(0, 13) + ":00";
		}

		if (actValue < ulFirstValue)
		{
			if (ulRealFirstValue - actValue > 20000)
			{
				//Assume ,eter/counter turnover
				ulFirstValue = ulRealFirstValue = actValue;
				currentSlot = bUseSubHourSlots ? (ntime.tm_hour * (60 / resolution) + ntime.tm_min / resolution) : ntime.tm_hour;
				lastSlot = currentSlot;
				lastHour = ntime.tm_hour;
			}
		}

		currentSlot = bUseSubHourSlots ? (ntime.tm_hour * (60 / resolution) + ntime.tm_min / resolution) : ntime.tm_hour;
		if (lastSlot != currentSlot)
		{
			if (lastDay != ntime.tm_mday)
			{
				lastDay = ntime.tm_mday;
				ulRealFirstValue = actValue;
			}

			if (bHaveFirstValue)
			{
				root["result"][ii]["d"] = szLastDateTimeHour;

				// float TotalValue = float(actValue - ulFirstValue);

				// prevents graph from going crazy if the meter counter resets
				// removed because it breaks  negative increments
				// Use ulLastValue (last reading in previous slot) so consumption is attributed
				// to the slot where the new counter reading arrives, not the prior slot.
				double TotalValue = double(ulLastValue - ulFirstValue);
				//if (actValue < ulFirstValue) TotalValue=actValue;

				// if (TotalValue != 0)
				{
					switch (metertype)
					{
					case MTYPE_ENERGY:
					case MTYPE_ENERGY_GENERATED:
						snprintf(szTmp, sizeof(szTmp), "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
						break;
					case MTYPE_GAS:
						snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / divider);
						break;
					case MTYPE_WATER:
						snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / divider);
						break;
					case MTYPE_COUNTER:
						snprintf(szTmp, sizeof(szTmp), "%.10g", TotalValue / divider);
						break;
					default:
						strcpy(szTmp, "0");
						break;
					}
					root["result"][ii]["v"] = szTmp;

					if (!bIsManagedCounter)
					{
						double usageValue = lastUsageValue;

						switch (metertype)
						{
						case MTYPE_ENERGY:
						case MTYPE_ENERGY_GENERATED:
							snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
							break;
						case MTYPE_GAS:
							snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
							break;
						case MTYPE_WATER:
							snprintf(szTmp, sizeof(szTmp), "%g", usageValue);
							break;
						case MTYPE_COUNTER:
							snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
							break;
						}
						root["result"][ii]["mu"] = szTmp;
					}
					ii++;
				}
			}
			if (!bIsManagedCounter)
			{
				ulFirstValue = ulLastValue;
			}
			lastSlot = currentSlot;
			lastHour = ntime.tm_hour;
		}

		if (!bHaveFirstValue)
		{
			bHaveFirstValue = true;
			currentSlot = bUseSubHourSlots ? (ntime.tm_hour * (60 / resolution) + ntime.tm_min / resolution) : ntime.tm_hour;
			lastSlot = currentSlot;
			lastHour = ntime.tm_hour;
			ulFirstValue = actValue;
			ulRealFirstValue = actValue;
			lastDay = ntime.tm_mday;

			if (!((ntime.tm_hour == 0) && (ntime.tm_min == 0)))
			{
				struct tm ltime;
				struct tm tm1copy = tm1;
				localtime_r(&atime, &tm1copy);
				getNoon(atime, ltime, ntime.tm_year + 1900, ntime.tm_mon + 1,
					ntime.tm_mday - 1); // We're only interested in finding the date
				std::string szDate = FormatDate(ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
				std::vector<std::vector<std::string>> result2;
				result2 = sql.safe_query(
					"SELECT Counter FROM %s_Calendar WHERE (DeviceRowID==%" PRIu64
					") AND (Date=='%q')",
					dbasetable.c_str(), idx, szDate.c_str());
				if (!result2.empty())
				{
					std::vector<std::string> sd = result2[0];
					ulRealFirstValue = std::stoll(sd[0]);
					lastDay = ntime.tm_mday;
				}
			}
		}
		szLastDateTimeHour = szActDateTimeHour;
		lastUsageValue = (double)(actValue - ulRealFirstValue);
		ulLastValue = actValue;
	}

	if ((!bIsManagedCounter) && (bHaveFirstValue))
	{
		// add last value
		root["result"][ii]["d"] = szLastDateTimeHour;

		int64_t ulTotalValue = ulLastValue - ulFirstValue;

		double TotalValue = double(ulTotalValue);

		// if (TotalValue != 0)
		{
			switch (metertype)
			{
			case MTYPE_ENERGY:
			case MTYPE_ENERGY_GENERATED:
				snprintf(szTmp, sizeof(szTmp), "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
				break;
			case MTYPE_GAS:
				snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / divider);
				break;
			case MTYPE_WATER:
				snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / divider);
				break;
			case MTYPE_COUNTER:
				snprintf(szTmp, sizeof(szTmp), "%.10g", TotalValue / divider);
				break;
			default:
				strcpy(szTmp, "0");
				break;
			}
			root["result"][ii]["v"] = szTmp;

			if (!bIsManagedCounter)
			{
				double usageValue = (double)(ulLastValue - ulRealFirstValue);
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue);
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", usageValue / divider);
					break;
				}
				root["result"][ii]["mu"] = szTmp;
			}
			ii++;
		}
	}
}

// ---------------------------------------------------------------------------
// Handles all meter/counter device types not covered by the specialised
// sub-handlers above (pTypeAirQuality, pTypeGeneral, pTypeRFXSensor,
// pTypeLux, pTypeWEIGHT, pTypeUsage, pTypeENERGY, pTypePOWER,
// pTypeYouLess, sTypeKwh, pTypeP1Gas, and generic Meter devices).
// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter_Meter(
	const GraphContext& ctx, const request& req,
	Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable)
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

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	if (dType == pTypeAirQuality)
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["co2"] = sd[0];
				ii++;
			}
		}
	}
	else if ((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness)))
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["v"] = sd[0];
				ii++;
			}
		}
	}
	else if (((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) || ((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) || ((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) || ((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
		((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel)))
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;
		float vdiv = 10.0F;
		if (((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) || ((dType == pTypeGeneral) && (dSubType == sTypeCurrent)))
		{
			vdiv = 1000.0F;
		}
		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				float fValue = float(atof(sd[0].c_str())) / vdiv;
				if (ctx.metertype == 1)
				{
					if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
						fValue *= 0.3937007874015748F; // inches
					else
						fValue *= 0.6214F; // miles
				}
				if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
					snprintf(szTmp, sizeof(szTmp), "%.3f", fValue);
				else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
					snprintf(szTmp, sizeof(szTmp), "%.3f", fValue);
				else
					snprintf(szTmp, sizeof(szTmp), "%.1f", fValue);
				root["result"][ii]["v"] = szTmp;
				ii++;
			}
		}
	}
	else if ((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["v"] = sd[0];
				ii++;
			}
		}
	}
	else if (dType == pTypeLux)
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["lux"] = sd[0];
				ii++;
			}
		}
	}
	else if (dType == pTypeWEIGHT)
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(sd[0].c_str()) / 10.0F);
				root["result"][ii]["v"] = szTmp;
				ii++;
			}
		}
	}
	else if (dType == pTypeUsage)
	{ // day
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["u"] = atof(sd[0].c_str()) / 10.0F;
				ii++;
			}
		}
	}
	else if ((dType == pTypeENERGY) || (dType == pTypePOWER) || (dType == pTypeYouLess) || ((dType == pTypeGeneral) && (dSubType == sTypeKwh)))
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;
		root["ValueQuantity"] = (options.count("ValueQuantity") ? options.at("ValueQuantity") : std::string{});
		root["ValueUnits"] = (options.count("ValueUnits") ? options.at("ValueUnits") : std::string{});
		root["Divider"] = divider;

		// First check if we had any usage in the short log, if not, its probably a meter without usage
		bool bHaveUsage = true;
		result = sql.safe_query("SELECT MIN([Usage]), MAX([Usage]) FROM %s WHERE (DeviceRowID==%" PRIu64 ")", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int64_t minValue = std::stoll(result[0][0]);
			int64_t maxValue = std::stoll(result[0][1]);

			if ((minValue == 0) && (maxValue == 0))
			{
				bHaveUsage = false;
			}
		}

		int ii = 0;
		result = sql.safe_query("SELECT Value,[Usage], Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);

		int method = 0;
		std::string sMethod = request::findValue(&req, "method");
		if (!sMethod.empty())
			method = atoi(sMethod.c_str());
		if (bHaveUsage == false)
			method = 0;

		if ((dType == pTypeYouLess) && ((metertype == MTYPE_ENERGY) || (metertype == MTYPE_ENERGY_GENERATED)))
			method = 1;

		double dividerForQuantity = divider; // kWh, m3, l
		double dividerForRate = divider; // Watt, m3/hour, l/hour
		if (method != 0)
		{
			// realtime graph
			if ((dType == pTypeENERGY) || (dType == pTypePOWER))
			{
				dividerForRate /= 100.0F;
			}
		}

		root["method"] = method;
		bool bHaveFirstValue = false;
		bool bHaveFirstRealValue = false;
		int64_t ulFirstRealValue = 0;
		int64_t ulFirstValue = 0;
		int64_t ulLastValue = 0;
		std::string LastDateTime;

		if (!result.empty())
		{
			for (auto itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;

				// If method == 1, provide BOTH hourly and instant usage for combined graph
				{
					// bars / hour
					std::string actDateTimeHour = sd[2].substr(0, 13);
					int64_t actValue = std::stoll(sd[0]); // actual energy value

					ulLastValue = actValue;

					if (ulLastValue < ulFirstValue)
					{
						if (ulFirstValue - ulLastValue > 20000)
						{
							//probably a meter/counter turnover
							ulFirstValue = ulFirstRealValue = ulLastValue;
							LastDateTime = actDateTimeHour;
						}
					}

					if (actDateTimeHour != LastDateTime || ((method == 1) && (itt + 1 == result.end())))
					{
						if (bHaveFirstValue)
						{
							// root["result"][ii]["d"] = LastDateTime + (method == 1 ? ":30" : ":00");
							//^^ not necessarily bad, but is currently inconsistent with all other day graphs
							root["result"][ii]["d"] = LastDateTime + ":00";

							int64_t ulTotalValue = ulLastValue - ulFirstValue;
							if (ulTotalValue == 0)
							{
								// Could be the P1 Gas Meter, only transmits one every 1 a 2 hours
								ulTotalValue = ulLastValue - ulFirstRealValue;
							}
							ulFirstRealValue = ulLastValue;
							double TotalValue = double(ulTotalValue);
							double dividerHere = method == 1 ? dividerForQuantity : dividerForRate;
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								snprintf(szTmp, sizeof(szTmp), "%.3f", (TotalValue / dividerHere) * 1000.0); // from kWh -> Watt
								break;
							case MTYPE_GAS:
								snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / dividerHere);
								break;
							case MTYPE_WATER:
								snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / dividerHere);
								break;
							case MTYPE_COUNTER:
								snprintf(szTmp, sizeof(szTmp), "%.10g", TotalValue / dividerHere);
								break;
							default:
								strcpy(szTmp, "0");
								break;
							}
							root["result"][ii][method == 1 ? "eu" : "v"] = szTmp;
							ii++;
						}
						LastDateTime = actDateTimeHour;
						bHaveFirstValue = false;
					}
					if (!bHaveFirstValue)
					{
						ulFirstValue = ulLastValue;
						bHaveFirstValue = true;
					}
					if (!bHaveFirstRealValue)
					{
						bHaveFirstRealValue = true;
						ulFirstRealValue = ulLastValue;
					}
				}

				if (method == 1)
				{
					int64_t actValue = std::stoll(sd[1]);

					root["result"][ii]["d"] = sd[2].substr(0, 16);

					double TotalValue = double(actValue);
					if ((dType == pTypeGeneral) && (dSubType == sTypeKwh))
						TotalValue /= 10.0F;
					switch (metertype)
					{
					case MTYPE_ENERGY:
					case MTYPE_ENERGY_GENERATED:
						snprintf(szTmp, sizeof(szTmp), "%.3f", (TotalValue / dividerForRate) * 1000.0); // from kWh -> Watt
						break;
					case MTYPE_GAS:
						snprintf(szTmp, sizeof(szTmp), "%.2f", TotalValue / dividerForRate);
						break;
					case MTYPE_WATER:
						snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / dividerForRate);
						break;
					case MTYPE_COUNTER:
						snprintf(szTmp, sizeof(szTmp), "%.10g", TotalValue / dividerForRate);
						break;
					default:
						strcpy(szTmp, "0");
						break;
					}
					root["result"][ii]["v"] = szTmp;
					ii++;
				}
			}
		}
	}
	else
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;
		root["ValueQuantity"] = (options.count("ValueQuantity") ? options.at("ValueQuantity") : std::string{});
		root["ValueUnits"] = (options.count("ValueUnits") ? options.at("ValueUnits") : std::string{});
		root["Divider"] = divider;
		root["PriceResolution"] = sql.m_PriceResolution.load();

		int method = 0;
		std::string sMethod = request::findValue(&req, "method");
		if (!sMethod.empty())
			method = atoi(sMethod.c_str());

		if (bIsManagedCounter)
		{
			result = sql.safe_query("SELECT Usage, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
			method = 1;
		}
		else
		{
			result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		}

		if (!result.empty())
		{
			if (method == 0)
			{
				HandleGraphDay_Counter_Meter_SlotAccum(ctx, req, result, root, sql, dbasetable, bIsManagedCounter);
			}
			else
			{
				// realtime graph (method == 1)
				int ii = 0;
				bool bHaveFirstRealValue = false;
				int64_t ulLastValue = 0;
				time_t lastTime = 0;

				for (const auto& sd : result)
				{
					int64_t actValue = std::stoll(sd[0]);

					std::string stime = sd[1];
					struct tm ntime;
					time_t atime;
					ParseSQLdatetime(atime, ntime, stime, -1);
					if (bHaveFirstRealValue)
					{
						int64_t curValue;
						float tlaps = 1;

						if (!bIsManagedCounter)
						{
							curValue = actValue - ulLastValue;
							float tdiff;
							tdiff = static_cast<float>(difftime(atime, lastTime));
							if (tdiff == 0)
								tdiff = 1;
							tlaps = 3600.0F / tdiff;
						}
						else
						{
							curValue = actValue;
						}

						curValue *= int(tlaps);

						root["result"][ii]["d"] = sd[1].substr(0, 16);

						double TotalValue = double(curValue);
						// if (TotalValue != 0)
						{
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								snprintf(szTmp, sizeof(szTmp), "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
								break;
							case MTYPE_GAS:
								snprintf(szTmp, sizeof(szTmp), "%.2f", TotalValue / divider);
								break;
							case MTYPE_WATER:
								snprintf(szTmp, sizeof(szTmp), "%.3f", TotalValue / divider);
								break;
							case MTYPE_COUNTER:
								snprintf(szTmp, sizeof(szTmp), "%.10g", TotalValue / divider);
								break;
							default:
								strcpy(szTmp, "0");
								break;
							}
							root["result"][ii]["v"] = szTmp;
							ii++;
						}
					}
					else
						bHaveFirstRealValue = true;
					if (!bIsManagedCounter)
					{
						ulLastValue = actValue;
					}
					lastTime = atime;
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// HandleGraphDay_Counter – dispatches to per-type sub-handlers.
// Cmd_GetCosts is already called by the caller (WebServerHandleGraph.cpp)
// before HandleGraphDay is invoked.
// ---------------------------------------------------------------------------

static void HandleGraphDay_Counter(
	const GraphContext& ctx, const request& req,
	Json::Value& root, CSQLHelper& sql,
	const std::string& dbasetable)
{
	const unsigned char dType = ctx.dType;

	if (dType == pTypeP1Power)
		HandleGraphDay_Counter_P1Power(ctx, req, root, sql, dbasetable);
	else if (dType == pTypeCURRENT)
		HandleGraphDay_Counter_Current(ctx, req, root, sql, dbasetable);
	else if (dType == pTypeCURRENTENERGY)
		HandleGraphDay_Counter_CurrentEnergy(ctx, req, root, sql, dbasetable);
	else
		// pTypeP1Gas uses the Meter table and falls through to the generic handler,
		// as do all other counter-class device types not listed above.
		HandleGraphDay_Counter_Meter(ctx, req, root, sql, dbasetable);
}

// ---------------------------------------------------------------------------
void HandleGraphDay(const GraphContext& ctx, const request& req,
                    Json::Value& root, CSQLHelper& sql,
                    CWebServer& /*webserver*/)
{
	const uint64_t idx         = ctx.idx;
	const unsigned char dType  = ctx.dType;
	const unsigned char dSubType = ctx.dSubType;
	const _eMeterType metertype  = ctx.metertype;
	const bool bIsManagedCounter = ctx.bIsManagedCounter;
	const double divider         = ctx.divider;
	const double AddjMulti       = ctx.AddjMulti;
	const std::string& sensor    = ctx.sensor;
	const std::string& srange    = ctx.srange;
	const std::map<std::string, std::string>& options = ctx.options;
	const struct tm& tm1         = ctx.tmNow;

	std::string dbasetable = CalcDbasetableDay(ctx);
	unsigned char tempsign = sql.m_tempsign[0];

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

	if (sensor == "temp")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Temperature, Chill, Humidity, Barometer, Date, SetPoint FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC",
			dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[4].substr(0, 16);
				if (dType == pTypeRego6XXTemp
					|| dType == pTypeTEMP
					|| dType == pTypeTEMP_HUM
					|| dType == pTypeTEMP_HUM_BARO
					|| dType == pTypeTEMP_BARO
					|| dType == pTypeWIND && dSubType == sTypeWIND4
					|| dType == pTypeUV && dSubType == sTypeUV3
					|| dType == pTypeThermostat1
					|| dType == pTypeRadiator1
					|| dType == pTypeRFXSensor && dSubType == sTypeRFXSensorTemp
					|| dType == pTypeGeneral && dSubType == sTypeSystemTemp
					|| dType == pTypeGeneral && dSubType == sTypeBaro
					|| dType == pTypeEvohomeZone || dType == pTypeThermostat6
					|| dType == pTypeEvohomeWater
					)
				{
					double tvalue = ConvertTemperature(atof(sd[0].c_str()), tempsign);
					root["result"][ii]["te"] = tvalue;
				}
				if (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)))
				{
					double tvalue = ConvertTemperature(atof(sd[1].c_str()), tempsign);
					root["result"][ii]["ch"] = tvalue;
				}
				if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempHum) || (dSubType == sTypeThermostat6TempHumBaro))))
				{
					root["result"][ii]["hu"] = sd[2];
				}
				if ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)) || ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempBaro) || (dSubType == sTypeThermostat6TempHumBaro))))
				{
					if (dType == pTypeTEMP_HUM_BARO)
					{
						if (dSubType == sTypeTHBFloat)
						{
							snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[3].c_str()) / 10.0F);
							root["result"][ii]["ba"] = szTmp;
						}
						else
							root["result"][ii]["ba"] = sd[3];
					}
					else if (dType == pTypeTEMP_BARO)
					{
						snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[3].c_str()) / 10.0F);
						root["result"][ii]["ba"] = szTmp;
					}
					else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
					{
						snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[3].c_str()) / 10.0F);
						root["result"][ii]["ba"] = szTmp;
					}
					else if ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempBaro) || (dSubType == sTypeThermostat6TempHumBaro)))
					{
						snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[3].c_str()) / 10.0F);
						root["result"][ii]["ba"] = szTmp;
					}
				}
				if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater) || (dType == pTypeThermostat6))
				{
					double se = ConvertTemperature(atof(sd[5].c_str()), tempsign);
					root["result"][ii]["se"] = se;
				}
				if (dType == pTypeSetpoint && dSubType == sTypeSetpoint)
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
						double se = ConvertTemperature(atof(sd[0].c_str()), tempsign);
						root["result"][ii]["te"] = se;
					}
					else
						root["result"][ii]["te"] = atof(sd[0].c_str());
				}
				ii++;
			}
		}
	}
	else if (sensor == "Percentage")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Percentage, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["v"] = sd[0];
				ii++;
			}
		}
	}
	else if (sensor == "fan")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Speed, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["v"] = sd[0];
				ii++;
			}
		}
	}
	else if (sensor == "counter")
	{
		HandleGraphDay_Counter(ctx, req, root, sql, dbasetable);
	}
	else if (sensor == "uv")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);
				root["result"][ii]["uvi"] = sd[0];
				ii++;
			}
		}
	}
	else if (sensor == "rain")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		int WorkingHour = -1;
		std::string WorkingHourDate;
		float WorkingHourStartValue = -1;

		float LastValue = -1;
		std::string LastDate;

		result = sql.safe_query("SELECT Total, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				float ActTotal = static_cast<float>(atof(sd[0].c_str()));
				int Hour = atoi(sd[1].substr(11, 2).c_str());
				if (Hour != WorkingHour)
				{
					if (WorkingHour != -1)
					{
						//Finish current hour
						root["result"][ii]["d"] = WorkingHourDate.substr(0, 14) + "00";
						double mmval = std::max(0.0, static_cast<double>(ActTotal - WorkingHourStartValue));
						mmval *= AddjMulti;
						snprintf(szTmp, sizeof(szTmp), "%.1f", mmval);
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
					WorkingHour = Hour;
					WorkingHourStartValue = ActTotal;
					WorkingHourDate = sd[1];
				}
				LastValue = ActTotal;
				LastDate = sd[1];
			}
			//Add last value
			result = sql.safe_query("SELECT sValue, LastUpdate FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
			if (!result.empty())
			{
				std::string sValue = result[0][0];
				std::vector<std::string> results;
				StringSplit(sValue, ";", results);
				if (results.size() == 2)
				{
					float ActTotal = static_cast<float>(atof(results[1].c_str()));
					if (ActTotal > LastValue)
						LastValue = ActTotal;
				}
			}
			double mmval = std::max(0.0, static_cast<double>(LastValue - WorkingHourStartValue));
			if (mmval != 0)
			{
				root["result"][ii]["d"] = WorkingHourDate.substr(0, 14) + "00";
				mmval *= AddjMulti;
				snprintf(szTmp, sizeof(szTmp), "%.1f", mmval);
				root["result"][ii]["mm"] = szTmp;
				ii++;
			}
		}
	}
	else if (sensor == "wind")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Direction, Speed, Gust, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			int ii = 0;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[3].substr(0, 16);
				root["result"][ii]["di"] = sd[0];

				int intSpeed = atoi(sd[1].c_str());
				int intGust = atoi(sd[2].c_str());

				root["result"][ii]["sp"] = FormatWindSpeed(intSpeed, sql.m_windunit, sql.m_windscale);
				root["result"][ii]["gu"] = FormatWindSpeed(intGust, sql.m_windunit, sql.m_windscale);
				ii++;
			}
		}
	}
	else if (sensor == "winddir")
	{
		root["status"] = "OK";
		root["title"] = "Graph " + sensor + " " + srange;

		result = sql.safe_query("SELECT Direction, Speed, Gust FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
		if (!result.empty())
		{
			std::map<int, int> _directions;
			std::array<std::array<int, 8>, 17> wdirtabletemp = {};
			std::string szLegendLabels[7];
			int ii = 0;

			int totalvalues = 0;
			// init dir list
			int idir;
			for (idir = 0; idir < 360 + 1; idir++)
				_directions[idir] = 0;

			if (sql.m_windunit == WINDUNIT_MS)
			{
				szLegendLabels[0] = "&lt; 0.5 " + sql.m_windsign;
				szLegendLabels[1] = "0.5-2 " + sql.m_windsign;
				szLegendLabels[2] = "2-4 " + sql.m_windsign;
				szLegendLabels[3] = "4-6 " + sql.m_windsign;
				szLegendLabels[4] = "6-8 " + sql.m_windsign;
				szLegendLabels[5] = "8-10 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 10" + sql.m_windsign;
			}
			else if (sql.m_windunit == WINDUNIT_KMH)
			{
				szLegendLabels[0] = "&lt; 2 " + sql.m_windsign;
				szLegendLabels[1] = "2-4 " + sql.m_windsign;
				szLegendLabels[2] = "4-6 " + sql.m_windsign;
				szLegendLabels[3] = "6-10 " + sql.m_windsign;
				szLegendLabels[4] = "10-20 " + sql.m_windsign;
				szLegendLabels[5] = "20-36 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 36" + sql.m_windsign;
			}
			else if (sql.m_windunit == WINDUNIT_MPH)
			{
				szLegendLabels[0] = "&lt; 3 " + sql.m_windsign;
				szLegendLabels[1] = "3-7 " + sql.m_windsign;
				szLegendLabels[2] = "7-12 " + sql.m_windsign;
				szLegendLabels[3] = "12-18 " + sql.m_windsign;
				szLegendLabels[4] = "18-24 " + sql.m_windsign;
				szLegendLabels[5] = "24-46 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 46" + sql.m_windsign;
			}
			else if (sql.m_windunit == WINDUNIT_Knots)
			{
				szLegendLabels[0] = "&lt; 3 " + sql.m_windsign;
				szLegendLabels[1] = "3-7 " + sql.m_windsign;
				szLegendLabels[2] = "7-17 " + sql.m_windsign;
				szLegendLabels[3] = "17-27 " + sql.m_windsign;
				szLegendLabels[4] = "27-34 " + sql.m_windsign;
				szLegendLabels[5] = "34-41 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 41" + sql.m_windsign;
			}
			else if (sql.m_windunit == WINDUNIT_Beaufort)
			{
				szLegendLabels[0] = "&lt; 2 " + sql.m_windsign;
				szLegendLabels[1] = "2-4 " + sql.m_windsign;
				szLegendLabels[2] = "4-6 " + sql.m_windsign;
				szLegendLabels[3] = "6-8 " + sql.m_windsign;
				szLegendLabels[4] = "8-10 " + sql.m_windsign;
				szLegendLabels[5] = "10-12 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 12" + sql.m_windsign;
			}
			else
			{
				// Todo !
				szLegendLabels[0] = "&lt; 0.5 " + sql.m_windsign;
				szLegendLabels[1] = "0.5-2 " + sql.m_windsign;
				szLegendLabels[2] = "2-4 " + sql.m_windsign;
				szLegendLabels[3] = "4-6 " + sql.m_windsign;
				szLegendLabels[4] = "6-8 " + sql.m_windsign;
				szLegendLabels[5] = "8-10 " + sql.m_windsign;
				szLegendLabels[6] = "&gt; 10" + sql.m_windsign;
			}

			for (const auto& sd : result)
			{
				float fdirection = static_cast<float>(atof(sd[0].c_str()));
				if (fdirection >= 360)
					fdirection = 0;
				int direction = int(fdirection);
				float speedOrg = static_cast<float>(atof(sd[1].c_str()));
				float gustOrg = static_cast<float>(atof(sd[2].c_str()));
				if ((gustOrg == 0) && (speedOrg != 0))
					gustOrg = speedOrg;
				if (gustOrg == 0)
					continue; // no direction if wind is still
				// float speed = speedOrg * sql.m_windscale;
				float gust = gustOrg * sql.m_windscale;
				int bucket = int(fdirection / 22.5F);

				int speedpos = 0;

				if (sql.m_windunit == WINDUNIT_MS)
				{
					if (gust < 0.5F)
						speedpos = 0;
					else if (gust < 2.0F)
						speedpos = 1;
					else if (gust < 4.0F)
						speedpos = 2;
					else if (gust < 6.0F)
						speedpos = 3;
					else if (gust < 8.0F)
						speedpos = 4;
					else if (gust < 10.0F)
						speedpos = 5;
					else
						speedpos = 6;
				}
				else if (sql.m_windunit == WINDUNIT_KMH)
				{
					if (gust < 2.0F)
						speedpos = 0;
					else if (gust < 4.0F)
						speedpos = 1;
					else if (gust < 6.0F)
						speedpos = 2;
					else if (gust < 10.0F)
						speedpos = 3;
					else if (gust < 20.0F)
						speedpos = 4;
					else if (gust < 36.0F)
						speedpos = 5;
					else
						speedpos = 6;
				}
				else if (sql.m_windunit == WINDUNIT_MPH)
				{
					if (gust < 3.0F)
						speedpos = 0;
					else if (gust < 7.0F)
						speedpos = 1;
					else if (gust < 12.0F)
						speedpos = 2;
					else if (gust < 18.0F)
						speedpos = 3;
					else if (gust < 24.0F)
						speedpos = 4;
					else if (gust < 46.0F)
						speedpos = 5;
					else
						speedpos = 6;
				}
				else if (sql.m_windunit == WINDUNIT_Knots)
				{
					if (gust < 3.0F)
						speedpos = 0;
					else if (gust < 7.0F)
						speedpos = 1;
					else if (gust < 17.0F)
						speedpos = 2;
					else if (gust < 27.0F)
						speedpos = 3;
					else if (gust < 34.0F)
						speedpos = 4;
					else if (gust < 41.0F)
						speedpos = 5;
					else
						speedpos = 6;
				}
				else if (sql.m_windunit == WINDUNIT_Beaufort)
				{
					float gustms = gustOrg * 0.1F;
					int iBeaufort = MStoBeaufort(gustms);
					if (iBeaufort < 2)
						speedpos = 0;
					else if (iBeaufort < 4)
						speedpos = 1;
					else if (iBeaufort < 6)
						speedpos = 2;
					else if (iBeaufort < 8)
						speedpos = 3;
					else if (iBeaufort < 10)
						speedpos = 4;
					else if (iBeaufort < 12)
						speedpos = 5;
					else
						speedpos = 6;
				}
				else
				{
					// Still todo !
					if (gust < 0.5F)
						speedpos = 0;
					else if (gust < 2.0F)
						speedpos = 1;
					else if (gust < 4.0F)
						speedpos = 2;
					else if (gust < 6.0F)
						speedpos = 3;
					else if (gust < 8.0F)
						speedpos = 4;
					else if (gust < 10.0F)
						speedpos = 5;
					else
						speedpos = 6;
				}
				wdirtabletemp[bucket][speedpos]++;
				_directions[direction]++;
				totalvalues++;
			}

			for (int jj = 0; jj < 7; jj++)
			{
				root["result_speed"][jj]["label"] = szLegendLabels[jj];

				for (ii = 0; ii < 16; ii++)
				{
					float svalue = 0;
					if (totalvalues > 0)
					{
						svalue = (100.0F / totalvalues) * wdirtabletemp[ii][jj];
					}
					snprintf(szTmp, sizeof(szTmp), "%.2f", svalue);
					root["result_speed"][jj]["sp"][ii] = szTmp;
				}
			}
			ii = 0;
			for (idir = 0; idir < 360 + 1; idir++)
			{
				if (_directions[idir] != 0)
				{
					root["result"][ii]["dig"] = idir;
					float percentage = 0;
					if (totalvalues > 0)
					{
						percentage = (float(100.0 / float(totalvalues)) * float(_directions[idir]));
					}
					snprintf(szTmp, sizeof(szTmp), "%.2f", percentage);
					root["result"][ii]["div"] = szTmp;
					ii++;
				}
			}
		}
	}
}

} // namespace server
} // namespace http
