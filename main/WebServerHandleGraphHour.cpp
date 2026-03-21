/*
 * WebServerHandleGraphHour.cpp
 *
 * Phase 2 / F04 – handler for srange == "hour".
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
void HandleGraphHour(const GraphContext& ctx, const request& req,
                     Json::Value& root, CSQLHelper& sql)
{
	if (ctx.sensor == "counter")
	{
		if (ctx.dType == pTypeP1Power)
		{
			std::string dbasetable = CalcDbasetableHour(ctx);
			const struct tm& tm1 = ctx.tmNow;
			root["status"] = "OK";
			root["title"] = "Graph " + ctx.sensor + " " + ctx.srange;

			std::string szDateEnd = FormatDateTime(tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

			// Subtract a day
			time_t daybefore;
			struct tm tm2;
			getNoon(daybefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 1);
			std::string szDateStart = FormatDateTime(tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

			std::string szGroupBy;
			int resolution = sql.m_PriceResolution.load();
			std::string sResolution = request::findValue(&req, "resolution");
			if (!sResolution.empty()) {
				int reqResolution = atoi(sResolution.c_str());
				if (reqResolution == 15 || reqResolution == 30 || reqResolution == 60)
					resolution = reqResolution;
			}
			if (resolution < 60) {
				// Group by sub-hourly slots using the configured resolution (e.g. 15 or 30 minutes)
				// Note: single % here because szGroupBy is passed via %s substitution in safe_query,
				// so its content is not processed by sqlite3_vmprintf's format parser
				szGroupBy = "strftime('%Y-%m-%d %H:', Date) || printf('%02d', (CAST(strftime('%M', Date) AS INTEGER) / " + std::to_string(resolution) + ") * " + std::to_string(resolution) + ") || ':00'";
			} else {
				// Group by hour (original behavior)
				szGroupBy = "strftime('%Y-%m-%d %H:00:00', Date)";
			}

			std::vector<std::vector<std::string>> result;
			result = sql.safe_query("SELECT %s as ymd, MIN(Value1) as u1, MIN(Value5) as u2, MIN(Value2) as d1, MIN(Value6) as d2, MIN(Price) as price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') GROUP BY ymd",
				szGroupBy.c_str(), dbasetable.c_str(), ctx.idx, szDateStart.c_str(), szDateEnd.c_str());
			if (!result.empty())
			{
				int ii = 0;
				bool bHaveDeliverd = false;
				bool bHaveFirstValue = false;
				int64_t lastUsage, lastDeliv;
				time_t lastTime = 0;
				float lastPrice = 0;

				int lastDay = 0;

				for (const auto& sd : result)
				{
					int64_t actUsage1 = std::stoll(sd[1]);
					int64_t actUsage2 = std::stoll(sd[2]);
					int64_t actDeliv1 = std::stoll(sd[3]);
					int64_t actDeliv2 = std::stoll(sd[4]);
					actDeliv1 = (actDeliv1 < 10) ? 0 : actDeliv1;
					actDeliv2 = (actDeliv2 < 10) ? 0 : actDeliv2;

					int64_t actUsage = actUsage1 + actUsage2;
					int64_t actDeliv = actDeliv1 + actDeliv2;
					float actPrice = std::stof(sd[5]);

					std::string stime = sd[0];
					struct tm ntime;
					time_t atime;
					ParseSQLdatetime(atime, ntime, stime, -1);
					if (lastDay != ntime.tm_mday)
					{
						lastDay = ntime.tm_mday;
					}

					if (bHaveFirstValue)
					{
						if (
							(actUsage < lastUsage)
							|| (actDeliv < lastDeliv)
							|| (atime <= lastTime)
							)
						{
							//daylight change happened, meter changed?, ignoring  for now
							lastUsage = actUsage;
							lastDeliv = actDeliv;
							lastTime = atime;
							lastPrice = actPrice;
							continue;
						}

						long curUsage = (long)(actUsage - lastUsage);
						long curDeliv = (long)(actDeliv - lastDeliv);

						std::string stime = sd[0].substr(0, 16);
						root["result"][ii]["d"] = stime;

						if (curDeliv != 0)
							bHaveDeliverd = true;

						root["result"][ii]["v"] = std::to_string(curUsage);
						root["result"][ii]["r"] = std::to_string(curDeliv);

						float total = (curUsage - curDeliv) / 1000.0F;
						float fPrice = lastPrice * total;
						char szPrice[16];
						snprintf(szPrice, sizeof(szPrice), "%.4f", fPrice);
						root["result"][ii]["p"] = szPrice;
						ii++;
					}
					else
					{
						bHaveFirstValue = true;
					}
					lastUsage = actUsage;
					lastDeliv = actDeliv;
					lastTime = atime;
					lastPrice = actPrice;
				}
				if (bHaveDeliverd)
				{
					root["delivered"] = true;
				}
				root["PriceResolution"] = sql.m_PriceResolution.load();
			}
		}
	}
}

} // namespace server
} // namespace http
