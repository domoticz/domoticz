/*
 * WebServerHandleGraphMonthYear.cpp
 *
 * Handler for srange == "month", srange == "year", or !sgroupby.empty().
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
// Helper: compute dbasetable for the month/year range
// ---------------------------------------------------------------------------
static std::string CalcDbasetableMonthYear(const GraphContext& ctx)
{
	if ((ctx.sensor == "temp") || (ctx.sensor == "hum"))
		return "Temperature_Calendar";
	if (ctx.sensor == "rain")
		return "Rain_Calendar";
	if (ctx.sensor == "Percentage")
		return "Percentage_Calendar";
	if (ctx.sensor == "fan")
		return "Fan_Calendar";
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
	if ((ctx.sensor == "wind") || (ctx.sensor == "winddir"))
		return "Wind_Calendar";
	if (ctx.sensor == "uv")
		return "UV_Calendar";
	return "";
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_P1MultiMeter
//   Handles dType == pTypeP1Power: MultiMeter_Calendar usage/delivery data
//   (sgroupby path + Actual Year + Previous Year calendar queries) plus the
//   "today" partial-period query against the raw MultiMeter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_P1MultiMeter(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	double divider,
	bool bIsManagedCounter,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sensorarea,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	std::string szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& ii,
	int& iPrev,
	std::function<std::string(std::string, std::string)> tableColumn)
{
	char szTmp[64];
	int P1DisplayType = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
	sql.GetPreferencesVar("P1DisplayType", P1DisplayType);
	root["P1DisplayType"] = P1DisplayType;
	std::string sT1Name, sT2Name;
	sql.GetPreferencesVar("T1Name", sT1Name);
	sql.GetPreferencesVar("T2Name", sT2Name);
	root["T1Name"] = sT1Name;
	root["T2Name"] = sT2Name;
	std::string sR1Name, sR2Name;
	sql.GetPreferencesVar("R1Name", sR1Name);
	sql.GetPreferencesVar("R2Name", sR2Name);
	root["R1Name"] = sR1Name;
	root["R2Name"] = sR2Name;

	if (!sgroupby.empty()) {
		if (sensorarea.empty())
		{
			_log.Log(LOG_ERROR, "Parameter sensorarea missing with groupby '%s'", sgroupby.c_str());
			return;
		}
		std::function<std::string(const char*, const char*, const char*, const char*, const char*)> sensorareaExpr =
			[sensorarea](const char* expr, const char* usageLow, const char* usageNormal, const char* deliveryLow, const char* deliveryNormal) {
			if (sensorarea == "usage")
			{
				return std_format(expr, usageLow, usageNormal);
			}
			if (sensorarea == "delivery")
			{
				return std_format(expr, deliveryLow, deliveryNormal);
			}
			return std::string(expr);
			};
		std::function<std::string(std::string)> counterExpr = [sensorareaExpr](std::string expr) {
			return sensorareaExpr(expr.c_str(), "1", "3", "2", "4");
			};
		std::function<std::string(std::string)> valueExpr = [sensorareaExpr](std::string expr) {
			return sensorareaExpr(expr.c_str(), "1", "5", "2", "6");
			};
		webserver.GroupBy(
			root, dbasetable, idx, sgroupby, bIsManagedCounter,
			[counterExpr, tableColumn](std::string table) {
				return counterExpr(tableColumn(table, "Counter%s") + "+" + tableColumn(table, "Counter%s"));
			},
			[valueExpr, tableColumn](std::string table) { return valueExpr(tableColumn(table, "Value%s") + "+" + tableColumn(table, "Value%s")); },
			[divider](double sum) {
				if (sum == 0)
				{
					return std::string("0");
				}
				return std_format("%.3f", sum / divider);
			});
		ii = root["result"].size();
	}
	else
	{
		// Actual Year
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date,"
				" Counter1, Counter2, Counter3, Counter4, Price "
				"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
				" AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			bool bHaveDeliverd = false;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[4].substr(0, 16);

				double counter_1 = std::stod(sd[5]);
				double counter_2 = std::stod(sd[6]);
				double counter_3 = std::stod(sd[7]);
				double counter_4 = std::stod(sd[8]);

				float fUsage_1 = std::stof(sd[0]);
				float fUsage_2 = std::stof(sd[2]);
				float fDeliv_1 = std::stof(sd[1]);
				float fDeliv_2 = std::stof(sd[3]);

				fDeliv_1 = (fDeliv_1 < 10) ? 0 : fDeliv_1;
				fDeliv_2 = (fDeliv_2 < 10) ? 0 : fDeliv_2;

				if ((fDeliv_1 != 0) || (fDeliv_2 != 0))
				{
					bHaveDeliverd = true;
				}

				//P1DisplayType
				snprintf(szTmp, sizeof(szTmp), "%.3f", fUsage_1 / divider);
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fUsage_2 / divider);
				root["result"][ii]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fDeliv_1 / divider);
				root["result"][ii]["r1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fDeliv_2 / divider);
				root["result"][ii]["r2"] = szTmp;

				float fPrice = std::stof(sd[9]);
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp;

				if (counter_1 != 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", (counter_1 - fUsage_1) / divider);
					root["result"][ii]["c1"] = szTmp;
				}
				else
				{
					root["result"][ii]["c1"] = "0";
				}

				if (counter_2 != 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", (counter_2 - fDeliv_1) / divider);
					root["result"][ii]["c2"] = szTmp;
				}
				else
				{
					root["result"][ii]["c2"] = "0";
				}

				if (counter_3 != 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", (counter_3 - fUsage_2) / divider);
					root["result"][ii]["c3"] = szTmp;
				}
				else
				{
					root["result"][ii]["c3"] = "0";
				}

				if (counter_4 != 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", (counter_4 - fDeliv_2) / divider);
					root["result"][ii]["c4"] = szTmp;
				}
				else
				{
					root["result"][ii]["c4"] = "0";
				}

				ii++;
			}
			if (bHaveDeliverd)
			{
				root["delivered"] = true;
			}
		}
		// Previous Year
		result = sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date "
			"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			bool bHaveDeliverd = false;
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[4].substr(0, 16);

				float fUsage_1 = std::stof(sd[0]);
				float fUsage_2 = std::stof(sd[2]);
				float fDeliv_1 = std::stof(sd[1]);
				float fDeliv_2 = std::stof(sd[3]);

				if ((fDeliv_1 != 0) || (fDeliv_2 != 0))
				{
					bHaveDeliverd = true;
				}
				snprintf(szTmp, sizeof(szTmp), "%.3f", fUsage_1 / divider);
				root["resultprev"][iPrev]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fUsage_2 / divider);
				root["resultprev"][iPrev]["v2"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fDeliv_1 / divider);
				root["resultprev"][iPrev]["r1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", fDeliv_2 / divider);
				root["resultprev"][iPrev]["r2"] = szTmp;
				iPrev++;
			}
			if (bHaveDeliverd)
			{
				root["delivered"] = true;
			}
		}
	}

	// add today (have to calculate it) – raw MultiMeter table, not Meter_Calendar
	if ((!sactmonth.empty()) || (!sactyear.empty()))
	{
		struct tm loctime;
		time_t now = mytime(nullptr);
		localtime_r(&now, &loctime);
		if ((!sactmonth.empty()) && (!sactyear.empty()))
		{
			bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
			if (bIsThisMonth)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
		else if (!sactyear.empty())
		{
			bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
			if (bIsThisYear)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
	}

	{
		int P1DisplayType2 = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
		sql.GetPreferencesVar("P1DisplayType", P1DisplayType2);
		root["P1DisplayType"] = P1DisplayType2;
		std::string sT1Name, sT2Name;
		sql.GetPreferencesVar("T1Name", sT1Name);
		sql.GetPreferencesVar("T2Name", sT2Name);
		root["T1Name"] = sT1Name;
		root["T2Name"] = sT2Name;
		std::string sR1Name, sR2Name;
		sql.GetPreferencesVar("R1Name", sR1Name);
		sql.GetPreferencesVar("R2Name", sR2Name);
		root["R1Name"] = sR1Name;
		root["R2Name"] = sR2Name;

		std::vector<std::vector<std::string>> result2 = sql.safe_query("SELECT "
			" MIN(Value1) as levering_laag_min,"
			" MAX(Value1) as levering_laag_max,"
			" MIN(Value2) as teruglevering_laag_min,"
			" MAX(Value2) as teruglevering_laag_max,"
			" MIN(Value5) as levering_normaal_min,"
			" MAX(Value5) as levering_normaal_max,"
			" MIN(Value6) as teruglevering_normaal_min,"
			" MAX(Value6) as teruglevering_normaal_max"
			" FROM MultiMeter WHERE (DeviceRowID=%" PRIu64 ""
			" AND Date>='%q')",
			idx, szDateEnd.c_str());
		bool bHaveDeliverd = false;
		if (!result2.empty())
		{
			std::vector<std::string> sd = result2[0];
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

			total_real_usage_1 = total_max_usage_1 - total_min_usage_1;
			total_real_usage_2 = total_max_usage_2 - total_min_usage_2;

			total_real_deliv_1 = total_max_deliv_1 - total_min_deliv_1;
			total_real_deliv_2 = total_max_deliv_2 - total_min_deliv_2;

			if (total_max_deliv_1 != 0 || total_max_deliv_2 != 0)
				bHaveDeliverd = true;

			if (!sgroupby.empty())
			{
				const double todayValue = (sensorarea == "usage" ? (total_real_usage_1 + total_real_usage_2)
					: sensorarea == "delivery" ? (total_real_deliv_1 + total_real_deliv_2)
					: 0) /
					divider;
				webserver.AddTodayValueToResult(root, sgroupby, szDateEnd, todayValue, "%.3f");
			}
			else
			{
				root["result"][ii]["d"] = szDateEnd;

				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_real_usage_1 / divider));
				root["result"][ii]["v1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_real_usage_2 / divider));
				root["result"][ii]["v2"] = szTmp;

				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_real_deliv_1 / divider));
				root["result"][ii]["r1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_real_deliv_2 / divider));
				root["result"][ii]["r2"] = szTmp;

				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_min_usage_1 / divider));
				root["result"][ii]["c1"] = szTmp;
				snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_min_usage_2 / divider));
				root["result"][ii]["c3"] = szTmp;

				if (total_max_deliv_2 != 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_min_deliv_1 / divider));
					root["result"][ii]["c2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.3f", (float)(total_min_deliv_2 / divider));
					root["result"][ii]["c4"] = szTmp;
				}
				else
				{
					root["result"][ii]["c2"] = "0";
					root["result"][ii]["c4"] = "0";
				}

				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp;

				ii++;
			}
		}
		if (bHaveDeliverd)
		{
			root["delivered"] = true;
		}
	}
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_P1Energy
//   Handles pTypeENERGY, sTypeKwh, and sTypeCounterIncremental energy types:
//   sets root["counter"] from sValue, then queries Meter_Calendar (Actual
//   Year + Past Year) or GroupBy, and appends the current partial period
//   using the raw Meter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_P1Energy(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	unsigned char dType,
	unsigned char dSubType,
	_eMeterType metertype,
	bool bIsManagedCounter,
	double divider,
	double AddjValue,
	double meteroffset,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sValue,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	std::string szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& ii,
	int& iPrev,
	std::function<std::string(std::string, std::string)> tableColumn)
{
	char szTmp[64];
	// Set root["counter"] based on energy device type
	if (dType == pTypeENERGY)
	{
		size_t spos = sValue.find(';');
		if (spos != std::string::npos)
		{
			float fvalue = static_cast<float>(atof(sValue.substr(spos + 1).c_str()));
			snprintf(szTmp, sizeof(szTmp), "%.3f", fvalue / (divider / 100.0F));
			root["counter"] = szTmp;
		}
	}
	else if ((dType == pTypeGeneral) && (dSubType == sTypeKwh))
	{
		size_t spos = sValue.find(';');
		if (spos != std::string::npos)
		{
			float fvalue = static_cast<float>(atof(sValue.substr(spos + 1).c_str()));
			snprintf(szTmp, sizeof(szTmp), "%.3f", fvalue / divider);
			root["counter"] = szTmp;
		}
	}
	else if ((dType == pTypeGeneral) && (dSubType == sTypeCounterIncremental))
	{
		double dvalue = static_cast<double>(atof(sValue.c_str()));

		switch (metertype)
		{
		case MTYPE_ENERGY:
		case MTYPE_ENERGY_GENERATED:
			snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (dvalue / divider));
			break;
		case MTYPE_GAS:
			snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (dvalue / divider));
			break;
		case MTYPE_WATER:
			snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (dvalue / divider));
			break;
		case MTYPE_COUNTER:
		default:
			snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + (dvalue / divider));
			break;
		}
		root["counter"] = szTmp;
	}

	// GroupBy or Meter_Calendar + today (shared with Regular path)
	if (!sgroupby.empty())
	{
		webserver.GroupBy(
			root, dbasetable, idx, sgroupby, bIsManagedCounter, [tableColumn](std::string table) { return tableColumn(table, "Counter"); },
			[tableColumn](std::string table) { return tableColumn(table, "Value"); },
			[metertype, AddjValue, divider](double sum) {
				if (sum == 0)
				{
					return std::string("0");
				}
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					return std_format("%.3f", sum / divider);
				case MTYPE_GAS:
					return std_format("%.2f", sum / divider);
				case MTYPE_WATER:
					return std_format("%.3f", sum / divider);
				case MTYPE_COUNTER:
					return std_format("%.10g", sum / divider);
				}
				return std::string("");
			});
		ii = root["result"].size();
	}
	else
	{

		// Actual Year
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];

				double fcounter = atof(sd[2].c_str());
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp; // Json::Value::null;
				ii++;
			}
		}
		// Past Year
		result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["resultprev"][iPrev]["p"] = szTmp;
				iPrev++;
			}
		}
	}

	// add today (have to calculate it) – raw Meter table, not Meter_Calendar
	if ((!sactmonth.empty()) || (!sactyear.empty()))
	{
		struct tm loctime;
		time_t now = mytime(nullptr);
		localtime_r(&now, &loctime);
		if ((!sactmonth.empty()) && (!sactyear.empty()))
		{
			bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
			if (bIsThisMonth)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
		else if (!sactyear.empty())
		{
			bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
			if (bIsThisYear)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
	}

	/*if (sgroupby == "year") {

	} else*/
	{
		// get the first value
		std::vector<std::vector<std::string>> result2 = sql.safe_query(
			//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
			"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd.c_str());
		if (!result2.empty())
		{
			std::vector<std::string> sd = result2[0];
			int64_t total_min = std::stoll(sd[0]);
			int64_t total_max = total_min;
			int64_t total_real;

			// Get the last value
			result2 = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx,
				szDateEnd.c_str());
			if (!result2.empty())
			{
				std::vector<std::string> sd = result2[0];
				total_max = std::stoull(sd[0]);
			}

			total_real = total_max - total_min;
			snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real);

			std::string szValue = szTmp;

			if (!sgroupby.empty())
			{
				double todayValue = double(total_real) / divider;
				std::string formatString;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					formatString = "%.3f";
					break;
				case MTYPE_GAS:
					formatString = "%.2f";
					break;
				case MTYPE_WATER:
					formatString = "%.3f";
					break;
				case MTYPE_COUNTER:
					formatString = "%.10g";
					break;
				}
				webserver.AddTodayValueToResult(root, sgroupby, szDateEnd, todayValue, formatString);
			}
			else
			{
				root["result"][ii]["d"] = szDateEnd;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED: {
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;

					std::vector<std::string> mresults;
					StringSplit(sValue, ";", mresults);
					if (mresults.size() == 2)
					{
						const_cast<std::string&>(sValue) = mresults[1];
					}
					if (dType == pTypeENERGY)
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (((atof(sValue.c_str()) * 100.0F) - atof(szValue.c_str())) / divider));
					else
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
				}
										   break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				}

				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp;

				ii++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_P1Gas
//   Handles pTypeP1Gas: sets root["counter"] as gas m3 value, then queries
//   Meter_Calendar (Actual Year + Past Year) or GroupBy, and appends the
//   current partial period using the raw Meter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_P1Gas(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	unsigned char dType,
	_eMeterType metertype,
	bool bIsManagedCounter,
	double divider,
	double AddjValue,
	double meteroffset,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sValue,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	std::string szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& ii,
	int& iPrev,
	std::function<std::string(std::string, std::string)> tableColumn)
{
	char szTmp[64];
	// Add last counter value (gas m3)
	snprintf(szTmp, sizeof(szTmp), "%.3f", atof(sValue.c_str()) / 1000.0);
	root["counter"] = szTmp;

	// GroupBy or Meter_Calendar + today (shared with Regular path)
	if (!sgroupby.empty())
	{
		webserver.GroupBy(
			root, dbasetable, idx, sgroupby, bIsManagedCounter, [tableColumn](std::string table) { return tableColumn(table, "Counter"); },
			[tableColumn](std::string table) { return tableColumn(table, "Value"); },
			[metertype, AddjValue, divider](double sum) {
				if (sum == 0)
				{
					return std::string("0");
				}
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					return std_format("%.3f", sum / divider);
				case MTYPE_GAS:
					return std_format("%.2f", sum / divider);
				case MTYPE_WATER:
					return std_format("%.3f", sum / divider);
				case MTYPE_COUNTER:
					return std_format("%.10g", sum / divider);
				}
				return std::string("");
			});
		ii = root["result"].size();
	}
	else
	{

		// Actual Year
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];

				double fcounter = atof(sd[2].c_str());
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp; // Json::Value::null;
				ii++;
			}
		}
		// Past Year
		result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["resultprev"][iPrev]["p"] = szTmp;
				iPrev++;
			}
		}
	}

	// add today (have to calculate it) – raw Meter table, not Meter_Calendar
	if ((!sactmonth.empty()) || (!sactyear.empty()))
	{
		struct tm loctime;
		time_t now = mytime(nullptr);
		localtime_r(&now, &loctime);
		if ((!sactmonth.empty()) && (!sactyear.empty()))
		{
			bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
			if (bIsThisMonth)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
		else if (!sactyear.empty())
		{
			bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
			if (bIsThisYear)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
	}

	/*if (sgroupby == "year") {

	} else*/
	{
		// get the first value
		std::vector<std::vector<std::string>> result2 = sql.safe_query(
			//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
			"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd.c_str());
		if (!result2.empty())
		{
			std::vector<std::string> sd = result2[0];
			int64_t total_min = std::stoll(sd[0]);
			int64_t total_max = total_min;
			int64_t total_real;

			// Get the last value
			result2 = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx,
				szDateEnd.c_str());
			if (!result2.empty())
			{
				std::vector<std::string> sd = result2[0];
				total_max = std::stoull(sd[0]);
			}

			total_real = total_max - total_min;
			snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real);

			std::string szValue = szTmp;

			if (!sgroupby.empty())
			{
				double todayValue = double(total_real) / divider;
				std::string formatString;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					formatString = "%.3f";
					break;
				case MTYPE_GAS:
					formatString = "%.2f";
					break;
				case MTYPE_WATER:
					formatString = "%.3f";
					break;
				case MTYPE_COUNTER:
					formatString = "%.10g";
					break;
				}
				webserver.AddTodayValueToResult(root, sgroupby, szDateEnd, todayValue, formatString);
			}
			else
			{
				root["result"][ii]["d"] = szDateEnd;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED: {
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;

					std::vector<std::string> mresults;
					StringSplit(sValue, ";", mresults);
					if (mresults.size() == 2)
					{
						const_cast<std::string&>(sValue) = mresults[1];
					}
					if (dType == pTypeENERGY)
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (((atof(sValue.c_str()) * 100.0F) - atof(szValue.c_str())) / divider));
					else
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
				}
										   break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				}

				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp;

				ii++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_CurrentMulti
//   Handles dType == pTypeCURRENT and dType == pTypeCURRENTENERGY:
//   multi-channel current meter (CM113 / CM180i) from MultiMeter_Calendar.
//   Includes the today partial-period query against the raw Meter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_CurrentMulti(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	unsigned char dType,
	_eMeterType metertype,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	const std::string& szDateEnd,
	int& ii)
{
	char szTmp[64];
	if (dType == pTypeCURRENT)
	{
		if (!sgroupby.empty())
		{
			root["status"] = "OK";
			root["title"] = "Comparing ";
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "(Value2+Value4+Value6)/3", 10.0);
			return;
		}
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64
				" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			// CM113
			int displaytype = 0;
			int voltage = 230;
			sql.GetPreferencesVar("CM113DisplayType", displaytype);
			sql.GetPreferencesVar("ElectricVoltage", voltage);

			root["displaytype"] = displaytype;

			bool bHaveL1 = false;
			bool bHaveL2 = false;
			bool bHaveL3 = false;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[6].substr(0, 16);

				float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0F);
				float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0F);
				float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0F);
				float fval4 = static_cast<float>(atof(sd[3].c_str()) / 10.0F);
				float fval5 = static_cast<float>(atof(sd[4].c_str()) / 10.0F);
				float fval6 = static_cast<float>(atof(sd[5].c_str()) / 10.0F);

				if ((fval1 != 0) || (fval2 != 0))
					bHaveL1 = true;
				if ((fval3 != 0) || (fval4 != 0))
					bHaveL2 = true;
				if ((fval5 != 0) || (fval6 != 0))
					bHaveL3 = true;

				if (displaytype == 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval1);
					root["result"][ii]["v1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval2);
					root["result"][ii]["v2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval3);
					root["result"][ii]["v3"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval4);
					root["result"][ii]["v4"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval5);
					root["result"][ii]["v5"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval6);
					root["result"][ii]["v6"] = szTmp;
				}
				else
				{
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval1 * voltage));
					root["result"][ii]["v1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval2 * voltage));
					root["result"][ii]["v2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval3 * voltage));
					root["result"][ii]["v3"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval4 * voltage));
					root["result"][ii]["v4"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval5 * voltage));
					root["result"][ii]["v5"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval6 * voltage));
					root["result"][ii]["v6"] = szTmp;
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
	else // pTypeCURRENTENERGY
	{
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64
				" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			// CM180i
			int displaytype = 0;
			int voltage = 230;
			sql.GetPreferencesVar("CM113DisplayType", displaytype);
			sql.GetPreferencesVar("ElectricVoltage", voltage);

			root["displaytype"] = displaytype;

			bool bHaveL1 = false;
			bool bHaveL2 = false;
			bool bHaveL3 = false;
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[6].substr(0, 16);

				float fval1 = static_cast<float>(atof(sd[0].c_str()) / 10.0F);
				float fval2 = static_cast<float>(atof(sd[1].c_str()) / 10.0F);
				float fval3 = static_cast<float>(atof(sd[2].c_str()) / 10.0F);
				float fval4 = static_cast<float>(atof(sd[3].c_str()) / 10.0F);
				float fval5 = static_cast<float>(atof(sd[4].c_str()) / 10.0F);
				float fval6 = static_cast<float>(atof(sd[5].c_str()) / 10.0F);

				if ((fval1 != 0) || (fval2 != 0))
					bHaveL1 = true;
				if ((fval3 != 0) || (fval4 != 0))
					bHaveL2 = true;
				if ((fval5 != 0) || (fval6 != 0))
					bHaveL3 = true;

				if (displaytype == 0)
				{
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval1);
					root["result"][ii]["v1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval2);
					root["result"][ii]["v2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval3);
					root["result"][ii]["v3"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval4);
					root["result"][ii]["v4"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval5);
					root["result"][ii]["v5"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.1f", fval6);
					root["result"][ii]["v6"] = szTmp;
				}
				else
				{
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval1 * voltage));
					root["result"][ii]["v1"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval2 * voltage));
					root["result"][ii]["v2"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval3 * voltage));
					root["result"][ii]["v3"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval4 * voltage));
					root["result"][ii]["v4"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval5 * voltage));
					root["result"][ii]["v5"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%d", int(fval6 * voltage));
					root["result"][ii]["v6"] = szTmp;
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
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_Managed
//   Handles bIsManagedCounter == true: counter value is always "0", then
//   queries Meter_Calendar (Actual Year + Past Year) or GroupBy, and appends
//   the current partial period using the raw Meter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_Managed(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	_eMeterType metertype,
	bool bIsManagedCounter,
	double divider,
	double AddjValue,
	double meteroffset,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sValue,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	const std::string& szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& ii,
	int& iPrev,
	std::function<std::string(std::string, std::string)> tableColumn)
{
	char szTmp[64];
	root["counter"] = "0";

	if (!sgroupby.empty())
	{
		webserver.GroupBy(
			root, dbasetable, idx, sgroupby, bIsManagedCounter, [tableColumn](std::string table) { return tableColumn(table, "Counter"); },
			[tableColumn](std::string table) { return tableColumn(table, "Value"); },
			[metertype, AddjValue, divider](double sum) {
				if (sum == 0)
				{
					return std::string("0");
				}
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					return std_format("%.3f", sum / divider);
				case MTYPE_GAS:
					return std_format("%.2f", sum / divider);
				case MTYPE_WATER:
					return std_format("%.3f", sum / divider);
				case MTYPE_COUNTER:
					return std_format("%.10g", sum / divider);
				}
				return std::string("");
			});
		ii = root["result"].size();
	}
	else
	{

		// Actual Year
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];

				double fcounter = atof(sd[2].c_str());
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp; // Json::Value::null;
				ii++;
			}
		}
		// Past Year
		result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["resultprev"][iPrev]["p"] = szTmp;
				iPrev++;
			}
		}
	}
	// Managed counters have no "today" raw Meter row — nothing to append here
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter_Regular
//   Handles the general Meter_Calendar path: pTypeRFXMeter, pTypeYouLess,
//   and all other non-managed counter types that are not P1Gas/P1Energy.
//   Sets root["counter"] then queries Meter_Calendar (Actual Year + Past
//   Year) or GroupBy, and appends the current partial period using the raw
//   Meter table.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter_Regular(
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	uint64_t idx,
	unsigned char dType,
	_eMeterType metertype,
	bool bIsManagedCounter,
	double divider,
	double AddjValue,
	double meteroffset,
	const std::string& dbasetable,
	const std::string& sgroupby,
	const std::string& sValue,
	const std::string& sactmonth,
	const std::string& sactyear,
	const std::string& szDateStart,
	std::string szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& ii,
	int& iPrev,
	std::function<std::string(std::string, std::string)> tableColumn)
{
	char szTmp[64];
	// Set root["counter"] based on regular meter device type
	if (dType == pTypeRFXMeter)
	{
		// Add last counter value
		double fvalue = atof(sValue.c_str());
		switch (metertype)
		{
		case MTYPE_ENERGY:
		case MTYPE_ENERGY_GENERATED:
			snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (fvalue / divider));
			break;
		case MTYPE_GAS:
			snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + (fvalue / divider));
			break;
		case MTYPE_WATER:
			snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (fvalue / divider));
			break;
		case MTYPE_COUNTER:
			snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + (fvalue / divider));
			break;
		default:
			strcpy(szTmp, "");
			break;
		}
		root["counter"] = szTmp;
	}
	else if (dType == pTypeYouLess)
	{
		std::vector<std::string> results;
		StringSplit(sValue, ";", results);
		if (results.size() == 2)
		{
			// Add last counter value
			double fvalue = atof(results[0].c_str());
			switch (metertype)
			{
			case MTYPE_ENERGY:
			case MTYPE_ENERGY_GENERATED:
				snprintf(szTmp, sizeof(szTmp), "%.3f", fvalue / divider);
				break;
			case MTYPE_GAS:
				snprintf(szTmp, sizeof(szTmp), "%.2f", fvalue / divider);
				break;
			case MTYPE_WATER:
				snprintf(szTmp, sizeof(szTmp), "%.3f", fvalue / divider);
				break;
			case MTYPE_COUNTER:
				snprintf(szTmp, sizeof(szTmp), "%.10g", fvalue / divider);
				break;
			default:
				strcpy(szTmp, "");
				break;
			}
			root["counter"] = szTmp;
		}
	}
	else
	{
		double dvalue = static_cast<double>(atof(sValue.c_str()));
		snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + (dvalue / divider));
		root["counter"] = szTmp;
	}

	if (!sgroupby.empty())
	{
		webserver.GroupBy(
			root, dbasetable, idx, sgroupby, bIsManagedCounter, [tableColumn](std::string table) { return tableColumn(table, "Counter"); },
			[tableColumn](std::string table) { return tableColumn(table, "Value"); },
			[metertype, AddjValue, divider](double sum) {
				if (sum == 0)
				{
					return std::string("0");
				}
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					return std_format("%.3f", sum / divider);
				case MTYPE_GAS:
					return std_format("%.2f", sum / divider);
				case MTYPE_WATER:
					return std_format("%.3f", sum / divider);
				case MTYPE_COUNTER:
					return std_format("%.10g", sum / divider);
				}
				return std::string("");
			});
		ii = root["result"].size();
	}
	else
	{

		// Actual Year
		std::vector<std::vector<std::string>> result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
		if (!result.empty())
		{
			for (const auto& sd : result)
			{
				root["result"][ii]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];

				double fcounter = atof(sd[2].c_str());
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					if (fcounter != 0)
					{
						snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
						root["result"][ii]["c"] = szTmp;
					}
					else
						root["result"][ii]["c"] = "0";
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp; // Json::Value::null;
				ii++;
			}
		}
		// Past Year
		result =
			sql.safe_query("SELECT Value, Date, Counter, Price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

				std::string szValue = sd[0];
				float fPrice = std::stof(sd[3]);

				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["resultprev"][iPrev]["v"] = szTmp;
					break;
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["resultprev"][iPrev]["p"] = szTmp;
				iPrev++;
			}
		}
	}

	// add today (have to calculate it) – raw Meter table, not Meter_Calendar
	if ((!sactmonth.empty()) || (!sactyear.empty()))
	{
		struct tm loctime;
		time_t now = mytime(nullptr);
		localtime_r(&now, &loctime);
		if ((!sactmonth.empty()) && (!sactyear.empty()))
		{
			bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
			if (bIsThisMonth)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
		else if (!sactyear.empty())
		{
			bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
			if (bIsThisYear)
			{
				szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
			}
		}
	}

	/*if (sgroupby == "year") {

	} else*/
	{
		// get the first value
		std::vector<std::vector<std::string>> result2 = sql.safe_query(
			//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
			"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd.c_str());
		if (!result2.empty())
		{
			std::vector<std::string> sd = result2[0];
			int64_t total_min = std::stoll(sd[0]);
			int64_t total_max = total_min;
			int64_t total_real;

			// Get the last value
			result2 = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx,
				szDateEnd.c_str());
			if (!result2.empty())
			{
				std::vector<std::string> sd = result2[0];
				total_max = std::stoull(sd[0]);
			}

			total_real = total_max - total_min;
			snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real);

			std::string szValue = szTmp;

			if (!sgroupby.empty())
			{
				double todayValue = double(total_real) / divider;
				std::string formatString;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED:
					formatString = "%.3f";
					break;
				case MTYPE_GAS:
					formatString = "%.2f";
					break;
				case MTYPE_WATER:
					formatString = "%.3f";
					break;
				case MTYPE_COUNTER:
					formatString = "%.10g";
					break;
				}
				webserver.AddTodayValueToResult(root, sgroupby, szDateEnd, todayValue, formatString);
			}
			else
			{
				root["result"][ii]["d"] = szDateEnd;
				switch (metertype)
				{
				case MTYPE_ENERGY:
				case MTYPE_ENERGY_GENERATED: {
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;

					std::vector<std::string> mresults;
					StringSplit(sValue, ";", mresults);
					if (mresults.size() == 2)
					{
						const_cast<std::string&>(sValue) = mresults[1];
					}
					if (dType == pTypeENERGY)
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + (((atof(sValue.c_str()) * 100.0F) - atof(szValue.c_str())) / divider));
					else
						snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
				}
									   break;
				case MTYPE_GAS:
					snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.2f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_WATER:
					snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				case MTYPE_COUNTER:
					snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
					root["result"][ii]["v"] = szTmp;
					snprintf(szTmp, sizeof(szTmp), "%.10g", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
					root["result"][ii]["c"] = szTmp;
					break;
				}

				float fPrice = 0;
				if (sql.m_actual_prices.find(idx) != sql.m_actual_prices.end())
				{
					fPrice = sql.m_actual_prices[idx];
				}
				snprintf(szTmp, sizeof(szTmp), "%.4f", fPrice);
				root["result"][ii]["p"] = szTmp;

				ii++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// HandleGraphMonthYear_Counter
//   Top-level counter dispatcher: dispatches to one of the six named
//   sub-functions above based on dType / bIsManagedCounter.
// ---------------------------------------------------------------------------
static void HandleGraphMonthYear_Counter(
	const GraphContext& ctx,
	const request& req,
	Json::Value& root,
	CSQLHelper& sql,
	CWebServer& webserver,
	const std::string& sensor,
	const std::string& srange,
	const std::string& dbasetable,
	const std::string& sensorarea,
	const std::string& szDateStart,
	std::string szDateEnd,
	const std::string& szDateStartPrev,
	const std::string& szDateEndPrev,
	int& iPrev)
{
	const uint64_t idx             = ctx.idx;
	const unsigned char dType      = ctx.dType;
	const unsigned char dSubType   = ctx.dSubType;
	const _eMeterType metertype    = ctx.metertype;
	const bool bIsManagedCounter   = ctx.bIsManagedCounter;
	const double divider           = ctx.divider;
	const double AddjValue         = ctx.AddjValue;
	const double meteroffset       = ctx.AddjValue;
	const std::string& sgroupby    = ctx.sgroupby;
	const std::map<std::string, std::string>& options = ctx.options;

	std::string sactmonth = request::findValue(&req, "actmonth");
	std::string sactyear  = request::findValue(&req, "actyear");

	root["status"] = "OK";
	root["title"] = sgroupby.empty() ? "Graph " + sensor + " " + srange : "Comparing " + sensor;
	root["ValueQuantity"] = (options.count("ValueQuantity") ? options.at("ValueQuantity") : std::string{});
	root["ValueUnits"] = (options.count("ValueUnits") ? options.at("ValueUnits") : std::string{});
	root["Divider"] = divider;

	// int nValue = 0;
	std::string sValue; //Counter

	std::vector<std::vector<std::string>> result = sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
	if (!result.empty())
	{
		sValue = result[0][0];
	}

	std::function<std::string(std::string, std::string)> tableColumn = [](std::string table, std::string expr) {
		return (table.empty() ? "" : table + ".") + expr;
		};

	char szTmp[64];
	int ii = 0;
	iPrev = 0;

	if (dType == pTypeP1Power)
	{
		HandleGraphMonthYear_Counter_P1MultiMeter(
			root, sql, webserver, idx, divider, bIsManagedCounter,
			dbasetable, sgroupby, sensorarea, sactmonth, sactyear,
			szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev,
			ii, iPrev, tableColumn);
	}
	else if (dType == pTypeAirQuality)
	{ // month/year
		root["status"] = "OK";

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value3", 10.0);
			if (sql.m_weightscale != 1.0)
			{
				for (auto& itt : root["result"])
				{
					itt["s"] = itt["s"].asDouble() * sql.m_weightscale;
				}
			}
			return;
		}

		root["title"] = "Graph " + sensor + " " + srange;

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
		result = sql.safe_query("SELECT Value2,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
			dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
		if (!result.empty())
		{
			iPrev = 0;
			for (const auto& sd : result)
			{
				root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);
				root["resultprev"][iPrev]["co2_max"] = sd[0];
				iPrev++;
			}
		}

		// add today (have to calculate it)
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	{ // month/year
		root["status"] = "OK";

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			double divider = 1.0;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value2", divider);
			return;
		}

		root["title"] = "Graph " + sensor + " " + srange;

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
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	{ // month/year
		root["status"] = "OK";

		float vdiv = 10.0F;
		if (((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) || ((dType == pTypeGeneral) && (dSubType == sTypeCurrent)))
		{
			vdiv = 1000.0F;
		}

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			double divider = (double)vdiv;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value3", divider);
			return;
		}
		root["title"] = "Graph " + sensor + " " + srange;

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
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	{ // month/year
		root["status"] = "OK";

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value3");
			return;
		}

		root["title"] = "Graph " + sensor + " " + srange;

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
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	{ // month/year
		root["status"] = "OK";

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value3", 10.0);
			if (sql.m_weightscale != 1.0)
			{
				for (auto& itt : root["result"])
				{
					itt["s"] = itt["s"].asDouble() * sql.m_weightscale;
				}
			}
			return;
		}


		root["title"] = "Graph " + sensor + " " + srange;

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
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	{ // month/year
		root["status"] = "OK";

		if (!sgroupby.empty())
		{
			root["title"] = "Comparing " + sensor;
			webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Value2", 10.0);
			return;
		}
		root["title"] = "Graph " + sensor + " " + srange;

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
		if ((!sactmonth.empty()) || (!sactyear.empty()))
		{
			struct tm loctime;
			time_t now = mytime(nullptr);
			localtime_r(&now, &loctime);
			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				bool bIsThisMonth = (atoi(sactyear.c_str()) == loctime.tm_year + 1900) && (atoi(sactmonth.c_str()) == loctime.tm_mon + 1);
				if (bIsThisMonth)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
			else if (!sactyear.empty())
			{
				bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
				if (bIsThisYear)
				{
					szDateEnd = FormatDate(loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
				}
			}
		}

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
	else if (dType == pTypeCURRENT || dType == pTypeCURRENTENERGY)
	{
		HandleGraphMonthYear_Counter_CurrentMulti(
			root, sql, webserver, idx, dType, metertype,
			dbasetable, sgroupby, sactmonth, sactyear,
			szDateStart, szDateEnd, ii);
	}
	else if (dType == pTypeP1Gas)
	{
		HandleGraphMonthYear_Counter_P1Gas(
			root, sql, webserver, idx, dType, metertype, bIsManagedCounter,
			divider, AddjValue, meteroffset, dbasetable, sgroupby,
			sValue, sactmonth, sactyear,
			szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev,
			ii, iPrev, tableColumn);
	}
	else if (dType == pTypeENERGY
		|| ((dType == pTypeGeneral) && (dSubType == sTypeKwh))
		|| ((dType == pTypeGeneral) && (dSubType == sTypeCounterIncremental)))
	{
		HandleGraphMonthYear_Counter_P1Energy(
			root, sql, webserver, idx, dType, dSubType, metertype, bIsManagedCounter,
			divider, AddjValue, meteroffset, dbasetable, sgroupby,
			sValue, sactmonth, sactyear,
			szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev,
			ii, iPrev, tableColumn);
	}
	else if (bIsManagedCounter)
	{
		HandleGraphMonthYear_Counter_Managed(
			root, sql, webserver, idx, metertype, bIsManagedCounter,
			divider, AddjValue, meteroffset, dbasetable, sgroupby,
			sValue, sactmonth, sactyear,
			szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev,
			ii, iPrev, tableColumn);
	}
	else
	{
		HandleGraphMonthYear_Counter_Regular(
			root, sql, webserver, idx, dType, metertype, bIsManagedCounter,
			divider, AddjValue, meteroffset, dbasetable, sgroupby,
			sValue, sactmonth, sactyear,
			szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev,
			ii, iPrev, tableColumn);
	}
}

// ---------------------------------------------------------------------------
void HandleGraphMonthYear(const GraphContext& ctx, const request& req,
                           Json::Value& root, CSQLHelper& sql,
                           CWebServer& webserver)
{
	const uint64_t idx             = ctx.idx;
	const unsigned char dType      = ctx.dType;
	const unsigned char dSubType   = ctx.dSubType;
	const _eMeterType metertype    = ctx.metertype;
	const bool bIsManagedCounter   = ctx.bIsManagedCounter;
	const double divider           = ctx.divider;
	const double AddjValue         = ctx.AddjValue;
	const double meteroffset       = ctx.meteroffset;
	const double AddjMulti         = ctx.AddjMulti;
	const std::string& sensor      = ctx.sensor;
	const std::string& srange      = ctx.srange;
	const std::string& sgroupby    = ctx.sgroupby;
	const std::map<std::string, std::string>& options = ctx.options;
	const struct tm& tm1           = ctx.tmNow;

	std::string dbasetable  = CalcDbasetableMonthYear(ctx);
	unsigned char tempsign  = sql.m_tempsign[0];
	std::string sensorarea  = request::findValue(&req, "sensorarea");

	char szTmp[64];
	std::vector<std::vector<std::string>> result;
	int iPrev = 0;

	std::string szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev;

			std::string sactmonth = request::findValue(&req, "actmonth");
			std::string sactyear = request::findValue(&req, "actyear");

			int actMonth = atoi(sactmonth.c_str());
			int actYear = atoi(sactyear.c_str());

			if ((!sactmonth.empty()) && (!sactyear.empty()))
			{
				szDateStart = FormatDate(actYear, actMonth, 1);
				szDateStartPrev = FormatDate(actYear - 1, actMonth, 1);
				actMonth++;
				if (actMonth == 13)
				{
					actMonth = 1;
					actYear++;
				}
				szDateEnd = FormatDate(actYear, actMonth, 1);
				szDateEndPrev = FormatDate(actYear - 1, actMonth, 1);
			}
			else if (!sactyear.empty())
			{
				szDateStart = FormatDate(actYear, 1, 1);
				szDateStartPrev = FormatDate(actYear - 1, 1, 1);
				actYear++;
				szDateEnd = FormatDate(actYear, 1, 1);
				szDateEndPrev = FormatDate(actYear - 1, 1, 1);
			}
			else
			{
				szDateEnd = FormatDate(tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
				szDateEndPrev = FormatDate(tm1.tm_year + 1900 - 1, tm1.tm_mon + 1, tm1.tm_mday);

				struct tm tm2;
				if (srange == "month")
				{
					// Subtract one month
					time_t monthbefore;
					getNoon(monthbefore, tm2, tm1.tm_year + 1900, tm1.tm_mon, tm1.tm_mday);
				}
				else
				{
					// Subtract one year
					time_t yearbefore;
					getNoon(yearbefore, tm2, tm1.tm_year + 1900 - 1, tm1.tm_mon + 1, tm1.tm_mday);
				}

				szDateStart = FormatDate(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);
				szDateStartPrev = FormatDate(tm2.tm_year + 1900 - 1, tm2.tm_mon + 1, tm2.tm_mday);
			}

			if (
				(sensor == "temp")
				|| (sensor == "hum")
				)
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					std::string var_name = request::findValue(&req, "var_name");
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, var_name);

					if (sensor == "temp")
					{
						if (var_name == "Barometer")
						{
							// Barometer values are stored *10 for certain device types,
							// apply the same /10.0 correction as the regular graph does
							bool bDivideBarometer =
								(dType == pTypeTEMP_BARO)
								|| ((dType == pTypeTEMP_HUM_BARO) && (dSubType == sTypeTHBFloat))
								|| ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								|| ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempBaro) || (dSubType == sTypeThermostat6TempHumBaro)));
							if (bDivideBarometer)
							{
								for (auto& itt : root["result"])
								{
									itt["s"] = itt["s"].asDouble() / 10.0;
								}
							}
						}
						else if (tempsign == 'F')
						{
							for (auto& itt : root["result"])
							{
								itt["s"] = ConvertTemperature(itt["s"].asDouble(), tempsign);
							}
						}
					}
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

				// Actual Year
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
				// Previous Year
				result = sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
					" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
					" SetPoint_Max, SetPoint_Avg "
					"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
					" AND Date<='%q') ORDER BY Date ASC",
					dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
				if (!result.empty())
				{
					iPrev = 0;
					for (const auto& sd : result)
					{
						root["resultprev"][iPrev]["d"] = sd[7].substr(0, 16);

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
							)
						{
							bool bOK = true;
							if (dType == pTypeWIND)
							{
								bOK = ((dSubType == sTypeWIND4) || (dSubType == sTypeWINDNoTemp));
							}
							if (bOK)
							{
								double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
								double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
								double ta = ConvertTemperature(atof(sd[6].c_str()), tempsign);
								root["resultprev"][iPrev]["te"] = te;
								root["resultprev"][iPrev]["tm"] = tm;
								root["resultprev"][iPrev]["ta"] = ta;
							}
						}
						if (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp)))
						{
							double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
							double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
							root["resultprev"][iPrev]["ch"] = ch;
							root["resultprev"][iPrev]["cm"] = cm;
						}
						if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
						{
							root["resultprev"][iPrev]["hu"] = sd[4];
						}
						if ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)))
						{
							if (dType == pTypeTEMP_HUM_BARO)
							{
								if (dSubType == sTypeTHBFloat)
								{
									snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["resultprev"][iPrev]["ba"] = szTmp;
								}
								else
									root["resultprev"][iPrev]["ba"] = sd[5];
							}
							else if (dType == pTypeTEMP_BARO)
							{
								snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
								root["resultprev"][iPrev]["ba"] = szTmp;
							}
							else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
							{
								snprintf(szTmp, sizeof(szTmp), "%.1f", atof(sd[5].c_str()) / 10.0F);
								root["resultprev"][iPrev]["ba"] = szTmp;
							}
						}
						if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater) || (dType == pTypeThermostat6))
						{
							double sx = ConvertTemperature(atof(sd[8].c_str()), tempsign);
							double sm = ConvertTemperature(atof(sd[7].c_str()), tempsign);
							double se = ConvertTemperature(atof(sd[9].c_str()), tempsign);
							root["resultprev"][iPrev]["se"] = se;
							root["resultprev"][iPrev]["sm"] = sm;
							root["resultprev"][iPrev]["sx"] = sx;
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
								root["resultprev"][iPrev]["te"] = te;
								root["resultprev"][iPrev]["tm"] = tm;
								root["resultprev"][iPrev]["ta"] = ta;
							}
							else
							{
								root["resultprev"][iPrev]["te"] = atof(sd[1].c_str());
								root["resultprev"][iPrev]["tm"] = atof(sd[0].c_str());
								root["resultprev"][iPrev]["ta"] = atof(sd[6].c_str());
							}
						}

						iPrev++;
					}
				}
			}
			else if (sensor == "Percentage")
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Percentage_Avg");
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

				result = sql.safe_query("SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM %s WHERE (DeviceRowID==%" PRIu64
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
			else if (sensor == "fan")
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Speed_Avg");
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

				result = sql.safe_query("SELECT Speed_Min, Speed_Max, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
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
			else if (sensor == "uv")
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Level");
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

				result = sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(),
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
				// Previous Year
				result = sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(),
					idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
				if (!result.empty())
				{
					iPrev = 0;
					for (const auto& sd : result)
					{
						root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);
						root["resultprev"][iPrev]["uvi"] = sd[0];
						iPrev++;
					}
				}
			}
			else if (sensor == "rain")
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Total", 1, true);
					if (AddjMulti != 1.0)
					{
						for (auto& itt : root["result"])
							itt["s"] = itt["s"].asDouble() * AddjMulti;
					}
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

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
						snprintf(szTmp, sizeof(szTmp), "%.1f", mmval);
						root["result"][ii]["mm"] = szTmp;
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
					snprintf(szTmp, sizeof(szTmp), "%.1f", total_real);
					root["result"][ii]["d"] = szDateEnd;
					root["result"][ii]["mm"] = szTmp;
					ii++;
				}
				// Previous Year
				result = sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
					dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
				if (!result.empty())
				{
					iPrev = 0;
					for (const auto& sd : result)
					{
						root["resultprev"][iPrev]["d"] = sd[2].substr(0, 16);
						double mmval = atof(sd[0].c_str());
						mmval *= AddjMulti;
						snprintf(szTmp, sizeof(szTmp), "%.1f", mmval);
						root["resultprev"][iPrev]["mm"] = szTmp;
						iPrev++;
					}
				}
			}
			else if (sensor == "counter")
			{
				HandleGraphMonthYear_Counter(ctx, req, root, sql, webserver,
					sensor, srange, dbasetable, sensorarea,
					szDateStart, szDateEnd, szDateStartPrev, szDateEndPrev, iPrev);
			}
			else if (sensor == "wind")
			{
				root["status"] = "OK";

				if (!sgroupby.empty())
				{
					root["title"] = "Comparing " + sensor;
					webserver.MakeCompareDataSensor(root, sgroupby, dbasetable, idx, "Speed_Max");
					for (auto& itt : root["result"])
					{
						if (sql.m_windunit != WINDUNIT_Beaufort)
							itt["s"] = itt["s"].asDouble() * sql.m_windscale;
						else
							itt["s"] = MStoBeaufort(static_cast<float>(itt["s"].asDouble() * 0.1F));
					}
					return;
				}

				root["title"] = "Graph " + sensor + " " + srange;

				int ii = 0;

				result = sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
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
				// Previous Year
				result = sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
					" Gust_Max, Date "
					"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
					" AND Date<='%q') ORDER BY Date ASC",
					dbasetable.c_str(), idx, szDateStartPrev.c_str(), szDateEndPrev.c_str());
				if (!result.empty())
				{
					iPrev = 0;
					for (const auto& sd : result)
					{
						root["resultprev"][iPrev]["d"] = sd[5].substr(0, 16);
						root["resultprev"][iPrev]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						int intGust = atoi(sd[4].c_str());
						root["resultprev"][iPrev]["sp"] = FormatWindSpeed(intSpeed, sql.m_windunit, sql.m_windscale);
						root["resultprev"][iPrev]["gu"] = FormatWindSpeed(intGust, sql.m_windunit, sql.m_windscale);
						iPrev++;
					}
				}
			}

} // HandleGraphMonthYear

} // namespace server
} // namespace http
