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
}

} // namespace server
} // namespace http
