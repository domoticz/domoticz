/*
 * WebServerHandleGraph.cpp
 *
 *  Created on: 7 August 2023
 *
 * This file is NOT a separate class but is part of 'main/WebServer.cpp'
 * It contains the 'HandleGraph' Cmd that is part of the WebServer class, but for sourcecode management
 * reasons separated out into its own file. The definitions are still in 'main/Webserver.h'
*/

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerHelper.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <iostream>
#include <fstream>
#include <json/json.h>

#include "mainworker.h"
#include "Helper.h"
#include "EventSystem.h"
#include "HTMLSanitizer.h"
#include "json_helper.h"
#include "Logger.h"
#include "SQLHelper.h"
#include "WebServerHandleGraphInternals.h"

namespace http
{
	namespace server
	{
		void CWebServer::Cmd_HandleGraph(WebEmSession& session, const request& req, Json::Value& root)
		{
			// "stats" is a special pseudo-range that redirects to kWh stats; handle
			// it before the device lookup performed by BuildGraphContext.
			if (request::findValue(&req, "range") == "stats")
			{
				Cmd_GetkWhStats(session, req, root);
				return;
			}

			GraphContext ctx;
			if (!BuildGraphContext(req, m_sql, ctx))
				return;

<<<<<<< HEAD
			_log.Debug(DEBUG_WEBSERVER, "CWebServer::Cmd_HandleGraph() : dType:%02X  dSubType:%02X  metertype:%d",
				ctx.dType, ctx.dSubType, int(ctx.metertype));

			if (ctx.sensor == "counter")
				Cmd_GetCosts(session, req, root);

			if (ctx.srange == "hour")
			{
				HandleGraphHour(ctx, req, root, m_sql);
				return;
			}
			else if (ctx.srange == "day")
			{
				HandleGraphDay(ctx, req, root, m_sql, *this);
				return;
			}
			else if (ctx.srange == "week")
			{
				HandleGraphWeek(ctx, req, root, m_sql);
				return;
			}
			else if (ctx.srange == "month" || ctx.srange == "year" || !ctx.sgroupby.empty())
			{
				HandleGraphMonthYear(ctx, req, root, m_sql, *this);
				return;
			}
			else
			{
				std::string dateStart, dateEnd;
				if (ParseCustomRange(ctx.srange, dateStart, dateEnd))
=======
			time_t now = mytime(nullptr);
			struct tm tm1;
			localtime_r(&now, &tm1);

			result = m_sql.safe_query("SELECT Type, SubType, SwitchType, AddjValue, AddjMulti, AddjValue2, Options FROM DeviceStatus WHERE (ID == %" PRIu64 ")", idx);
			if (result.empty())
				return;

			unsigned char dType = atoi(result[0][0].c_str());
			unsigned char dSubType = atoi(result[0][1].c_str());
			_eMeterType metertype = (_eMeterType)atoi(result[0][2].c_str());
			_log.Debug(DEBUG_WEBSERVER, "CWebServer::Cmd_HandleGraph() : dType:%02X  dSubType:%02X  metertype:%d", dType, dSubType, int(metertype));
			if ((dType == pTypeP1Power) || (dType == pTypeENERGY) || (dType == pTypePOWER) || (dType == pTypeCURRENTENERGY) || ((dType == pTypeGeneral) && (dSubType == sTypeKwh)))
			{
				metertype = MTYPE_ENERGY;
			}
			else if (dType == pTypeP1Gas)
				metertype = MTYPE_GAS;
			else if ((dType == pTypeRego6XXValue) && (dSubType == sTypeRego6XXCounter))
				metertype = MTYPE_COUNTER;

			// Special case of managed counter: Usage instead of Value in Meter table, and we don't want to calculate last value
			bool bIsManagedCounter = (dType == pTypeGeneral) && (dSubType == sTypeManagedCounter);

			double AddjValue = atof(result[0][3].c_str());
			double AddjMulti = atof(result[0][4].c_str());
			double AddjValue2 = atof(result[0][5].c_str());
			std::string sOptions = result[0][6];
			std::map<std::string, std::string> options = m_sql.BuildDeviceOptions(sOptions);

			if (options["AddDBLogEntry"] == "true")
			{
				bIsManagedCounter = true;
			}

			double divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

			double meteroffset = AddjValue;

			std::string dbasetable;
			if ((srange == "day") || (srange == "hour"))
			{
				if (sensor == "temp")
					dbasetable = "Temperature";
				else if (sensor == "rain")
					dbasetable = "Rain";
				else if (sensor == "Percentage")
					dbasetable = "Percentage";
				else if (sensor == "fan")
					dbasetable = "Fan";
				else if (sensor == "counter")
>>>>>>> 516df26bf (Roll back in time if no data available for pTypeP1Power hour view)
				{
					HandleGraphCustomRange(ctx, req, root, m_sql, *this);
					return;
<<<<<<< HEAD
=======
			}
			else
			{
				// week,year,month
				if (
					(sensor == "temp")
					|| (sensor == "hum")
					)
					dbasetable = "Temperature_Calendar";
				else if (sensor == "rain")
					dbasetable = "Rain_Calendar";
				else if (sensor == "Percentage")
					dbasetable = "Percentage_Calendar";
				else if (sensor == "fan")
					dbasetable = "Fan_Calendar";
				else if (sensor == "counter")
				{
					Cmd_GetCosts(session, req, root);

					if (dType == pTypeP1Power
						|| dType == pTypeCURRENT
						|| dType == pTypeCURRENTENERGY
						|| dType == pTypeAirQuality
						|| dType == pTypeLux
						|| dType == pTypeWEIGHT
						|| dType == pTypeUsage
						|| dType == pTypeGeneral && dSubType == sTypeVisibility
						|| dType == pTypeGeneral && dSubType == sTypeDistance
						|| dType == pTypeGeneral && dSubType == sTypeSolarRadiation
						|| dType == pTypeGeneral && dSubType == sTypeSoilMoisture
						|| dType == pTypeGeneral && dSubType == sTypeLeafWetness
						|| dType == pTypeGeneral && dSubType == sTypeVoltage
						|| dType == pTypeGeneral && dSubType == sTypeCurrent
						|| dType == pTypeGeneral && dSubType == sTypePressure
						|| dType == pTypeGeneral && dSubType == sTypeSoundLevel
						|| dType == pTypeRFXSensor && dSubType == sTypeRFXSensorAD
						|| dType == pTypeRFXSensor && dSubType == sTypeRFXSensorVolt
						) {
						dbasetable = "MultiMeter_Calendar";
					}
					else {
						dbasetable = "Meter_Calendar";
					}
				}
				else if ((sensor == "wind") || (sensor == "winddir"))
					dbasetable = "Wind_Calendar";
				else if (sensor == "uv")
					dbasetable = "UV_Calendar";
				else
					return;
			}
			unsigned char tempsign = m_sql.m_tempsign[0];

			int iPrev;

			if (srange == "hour")
			{
				if (sensor == "counter")
				{
					if (dType == pTypeP1Power)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						// We extract the most recent data, ideally from current time, otherwise we go back to yesterday at midnight
						// get date of last data
						result = m_sql.safe_query("SELECT MAX(Date) FROM %s WHERE DeviceRowID==%" PRIu64, dbasetable.c_str(), idx);

						time_t sqlTime = now;

						if (!result.empty() && !result[0][0].empty())
						{
							ParseSQLdatetime(sqlTime, tm1, result[0][0], -1);
						}
						else
						{
							sqlTime = now;
						}

						// calculate date of yesterday midnight
						struct tm tm_limit;
						localtime_r(&now, &tm_limit);
						tm_limit.tm_hour = 0;
						tm_limit.tm_min  = 0;
						tm_limit.tm_sec  = 0;
						tm_limit.tm_mday -= 1;

						time_t limit = mktime(&tm_limit);

						// limit date to yesterday midnight
						if (sqlTime < limit)
						{
							sqlTime = limit;
						}

						localtime_r(&sqlTime, &tm1);

						// check if data are from yesterday or before
						if (tm1.tm_mday != (tm_limit.tm_mday+1))
						{
							root["warningmessage"] = "Warning: data are from yesterday or older";
						}

						char szDateStart[40];
						char szDateEnd[40];
						sprintf(szDateEnd, "%04d-%02d-%02d %02d:%02d:%02d", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

						// Subtract a day
						time_t daybefore;
						struct tm tm2;
						getNoon(daybefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 1);
						sprintf(szDateStart, "%04d-%02d-%02d %02d:%02d:%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

						std::string szGroupBy;
						int resolution = m_sql.m_PriceResolution.load();
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

						result = m_sql.safe_query("SELECT %s as ymd, MIN(Value1) as u1, MIN(Value5) as u2, MIN(Value2) as d1, MIN(Value6) as d2, MIN(Price) as price FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') GROUP BY ymd",
							szGroupBy.c_str(), dbasetable.c_str(), idx, szDateStart, szDateEnd);
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

									sprintf(szTmp, "%ld", curUsage);
									root["result"][ii]["v"] = szTmp;
									sprintf(szTmp, "%ld", curDeliv);
									root["result"][ii]["r"] = szTmp;

									float total = (curUsage - curDeliv) / 1000.0F;
									float fPrice = lastPrice * total;
									sprintf(szTmp, "%.4f", fPrice);
									root["result"][ii]["p"] = szTmp;
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
							root["PriceResolution"] = m_sql.m_PriceResolution.load();
						}
					}
>>>>>>> 516df26bf (Roll back in time if no data available for pTypeP1Power hour view)
				}
			}
		}

	} // namespace server
} // namespace http
