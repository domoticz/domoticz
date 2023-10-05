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

namespace http
{
	namespace server
	{
		void CWebServer::Cmd_HandleGraph(WebEmSession& session, const request& req, Json::Value& root)
		{
			uint64_t idx = 0;
			if (!request::findValue(&req, "idx").empty())
			{
				idx = std::stoull(request::findValue(&req, "idx"));
			}

			std::vector<std::vector<std::string>> result;
			char szTmp[300];

			std::string sensor = request::findValue(&req, "sensor");
			if (sensor.empty())
				return;
			std::string sensorarea = request::findValue(&req, "sensorarea");
			std::string srange = request::findValue(&req, "range");
			std::string sgroupby = request::findValue(&req, "groupby");
			if (srange.empty() && sgroupby.empty())
				return;

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

			double divider = m_sql.GetCounterDivider(int(metertype), int(dType), float(AddjValue2));

			double meteroffset = AddjValue;

			std::string dbasetable;
			if (srange == "day")
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
				{
					Cmd_GetCosts(session, req, root);

					if ((dType == pTypeP1Power) || (dType == pTypeCURRENT) || (dType == pTypeCURRENTENERGY))
					{
						dbasetable = "MultiMeter";
					}
					else
					{
						dbasetable = "Meter";
					}
				}
				else if ((sensor == "wind") || (sensor == "winddir"))
					dbasetable = "Wind";
				else if (sensor == "uv")
					dbasetable = "UV";
				else
					return;
			}
			else
			{
				// week,year,month
				if (sensor == "temp")
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

			if (srange == "day")
			{
				if (sensor == "temp")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Temperature, Chill, Humidity, Barometer, Date, SetPoint FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC",
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
								|| dType == pTypeEvohomeZone
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
							if ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO))
							{
								root["result"][ii]["hu"] = sd[2];
							}
							if ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)))
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[3];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
							{
								double se = ConvertTemperature(atof(sd[5].c_str()), tempsign);
								root["result"][ii]["se"] = se;
							}
							if (dType == pTypeSetpoint && dSubType == sTypeSetpoint)
							{
								std::string value_unit = options["ValueUnit"];
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

					result = m_sql.safe_query("SELECT Percentage, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Speed, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
					if (dType == pTypeP1Power)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Value4, Value5, Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC",
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

							int nMeterType = 0;
							m_sql.GetPreferencesVar("SmartMeterType", nMeterType);

							int lastDay = 0;

							for (const auto& sd : result)
							{
								if (nMeterType == 0)
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

										sprintf(szTmp, "%ld", curUsage1);
										root["result"][ii]["v"] = szTmp;
										sprintf(szTmp, "%ld", curUsage2);
										root["result"][ii]["v2"] = szTmp;
										sprintf(szTmp, "%ld", curDeliv1);
										root["result"][ii]["r1"] = szTmp;
										sprintf(szTmp, "%ld", curDeliv2);
										root["result"][ii]["r2"] = szTmp;

										long pUsage1 = (long)(actUsage1 - firstUsage1);
										long pUsage2 = (long)(actUsage2 - firstUsage2);

										sprintf(szTmp, "%ld", pUsage1 + pUsage2);
										root["result"][ii]["eu"] = szTmp;
										if (bHaveDeliverd)
										{
											long pDeliv1 = (long)(actDeliv1 - firstDeliv1);
											long pDeliv2 = (long)(actDeliv2 - firstDeliv2);
											sprintf(szTmp, "%ld", pDeliv1 + pDeliv2);
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
											localtime_r(&atime, &tm1);
											getNoon(atime, ltime, ntime.tm_year + 1900, ntime.tm_mon + 1,
												ntime.tm_mday - 1); // We're only interested in finding the date
											int year = ltime.tm_year + 1900;
											int mon = ltime.tm_mon + 1;
											int day = ltime.tm_mday;
											sprintf(szTmp, "%04d-%02d-%02d", year, mon, day);
											std::vector<std::vector<std::string>> result2;
											result2 = m_sql.safe_query(
												"SELECT Counter1, Counter2, Counter3, Counter4 FROM Multimeter_Calendar WHERE (DeviceRowID==%" PRIu64
												") AND (Date=='%q')",
												idx, szTmp);
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
								else
								{
									// this meter has no decimals, so return the use peaks
									root["result"][ii]["d"] = sd[6].substr(0, 16);

									if (sd[3] != "0")
										bHaveDeliverd = true;
									root["result"][ii]["v"] = sd[2];
									root["result"][ii]["r1"] = sd[3];
									ii++;
								}
							}
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{ // day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
						if (!result.empty())
						{
							int ii = 0;
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[1].substr(0, 16);
								float fValue = float(atof(sd[0].c_str())) / vdiv;
								if (metertype == 1)
								{
									if ((dType == pTypeGeneral) && (dSubType == sTypeDistance))
										fValue *= 0.3937007874015748F; // inches
									else
										fValue *= 0.6214F; // miles
								}
								if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
									sprintf(szTmp, "%.3f", fValue);
								else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
									sprintf(szTmp, "%.3f", fValue);
								else
									sprintf(szTmp, "%.1f", fValue);
								root["result"][ii]["v"] = szTmp;
								ii++;
							}
						}
					}
					else if ((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt)))
					{ // day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
						if (!result.empty())
						{
							int ii = 0;
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[1].substr(0, 16);
								sprintf(szTmp, "%.1f", m_sql.m_weightscale * atof(sd[0].c_str()) / 10.0F);
								root["result"][ii]["v"] = szTmp;
								ii++;
							}
						}
					}
					else if (dType == pTypeUsage)
					{ // day
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
					else if (dType == pTypeCURRENT)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						// CM113
						int displaytype = 0;
						int voltage = 230;
						m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"] = displaytype;

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1 * voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2 * voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3 * voltage));
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
					else if (dType == pTypeCURRENTENERGY)
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						// CM113
						int displaytype = 0;
						int voltage = 230;
						m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
						m_sql.GetPreferencesVar("ElectricVoltage", voltage);

						root["displaytype"] = displaytype;

						result = m_sql.safe_query("SELECT Value1, Value2, Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1 * voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2 * voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3 * voltage));
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
					else if ((dType == pTypeENERGY) || (dType == pTypePOWER) || (dType == pTypeYouLess) || ((dType == pTypeGeneral) && (dSubType == sTypeKwh)))
					{
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;
						root["ValueQuantity"] = options["ValueQuantity"];
						root["ValueUnits"] = options["ValueUnits"];
						root["Divider"] = divider;

						// First check if we had any usage in the short log, if not, its probably a meter without usage
						bool bHaveUsage = true;
						result = m_sql.safe_query("SELECT MIN([Usage]), MAX([Usage]) FROM %s WHERE (DeviceRowID==%" PRIu64 ")", dbasetable.c_str(), idx);
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
						result = m_sql.safe_query("SELECT Value,[Usage], Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);

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
												sprintf(szTmp, "%.3f", (TotalValue / dividerHere) * 1000.0); // from kWh -> Watt
												break;
											case MTYPE_GAS:
												sprintf(szTmp, "%.3f", TotalValue / dividerHere);
												break;
											case MTYPE_WATER:
												sprintf(szTmp, "%.3f", TotalValue / dividerHere);
												break;
											case MTYPE_COUNTER:
												sprintf(szTmp, "%.10g", TotalValue / dividerHere);
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
										sprintf(szTmp, "%.3f", (TotalValue / dividerForRate) * 1000.0); // from kWh -> Watt
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", TotalValue / dividerForRate);
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.3f", TotalValue / dividerForRate);
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.10g", TotalValue / dividerForRate);
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
						root["ValueQuantity"] = options["ValueQuantity"];
						root["ValueUnits"] = options["ValueUnits"];
						root["Divider"] = divider;

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

						int method = 0;
						std::string sMethod = request::findValue(&req, "method");
						if (!sMethod.empty())
							method = atoi(sMethod.c_str());

						if (bIsManagedCounter)
						{
							result = m_sql.safe_query("SELECT Usage, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
							bHaveFirstValue = true;
							bHaveFirstRealValue = true;
							method = 1;
						}
						else
						{
							result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
						}

						if (!result.empty())
						{
							double lastUsageValue = 0;

							for (const auto& sd : result)
							{
								if (method == 0)
								{
									// bars / hour
									int64_t actValue = std::stoll(sd[0]);
									szlastDateTime = sd[1].substr(0, 16);
									szActDateTimeHour = sd[1].substr(0, 13) + ":00";

									struct tm ntime;
									time_t atime;
									ParseSQLdatetime(atime, ntime, sd[1], -1);

									if (actValue < ulFirstValue)
									{
										if (ulRealFirstValue - actValue > 20000)
										{
											//Assume ,eter/counter turnover
											ulFirstValue = ulRealFirstValue = actValue;
											lastHour = ntime.tm_hour;
										}
									}

									if (lastHour != ntime.tm_hour)
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
											double TotalValue=double(actValue - ulFirstValue);
											//if (actValue < ulFirstValue) TotalValue=actValue;

											// if (TotalValue != 0)
											{
												switch (metertype)
												{
												case MTYPE_ENERGY:
												case MTYPE_ENERGY_GENERATED:
													sprintf(szTmp, "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
													break;
												case MTYPE_GAS:
													sprintf(szTmp, "%.3f", TotalValue / divider);
													break;
												case MTYPE_WATER:
													sprintf(szTmp, "%.3f", TotalValue / divider);
													break;
												case MTYPE_COUNTER:
													sprintf(szTmp, "%.10g", TotalValue / divider);
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
														sprintf(szTmp, "%.3f", usageValue / divider);
														break;
													case MTYPE_GAS:
														sprintf(szTmp, "%.3f", usageValue / divider);
														break;
													case MTYPE_WATER:
														sprintf(szTmp, "%g", usageValue);
														break;
													case MTYPE_COUNTER:
														sprintf(szTmp, "%.3f", usageValue / divider);
														break;
													}
													root["result"][ii]["mu"] = szTmp;
												}
												ii++;
											}
										}
										if (!bIsManagedCounter)
										{
											ulFirstValue = actValue;
										}
										lastHour = ntime.tm_hour;
									}

									if (!bHaveFirstValue)
									{
										bHaveFirstValue = true;
										lastHour = ntime.tm_hour;
										ulFirstValue = actValue;
										ulRealFirstValue = actValue;
										lastDay = ntime.tm_mday;

										if (!((ntime.tm_hour == 0) && (ntime.tm_min == 0)))
										{
											struct tm ltime;
											localtime_r(&atime, &tm1);
											getNoon(atime, ltime, ntime.tm_year + 1900, ntime.tm_mon + 1,
												ntime.tm_mday - 1); // We're only interested in finding the date
											int year = ltime.tm_year + 1900;
											int mon = ltime.tm_mon + 1;
											int day = ltime.tm_mday;
											sprintf(szTmp, "%04d-%02d-%02d", year, mon, day);
											std::vector<std::vector<std::string>> result2;
											result2 = m_sql.safe_query(
												"SELECT Counter FROM %s_Calendar WHERE (DeviceRowID==%" PRIu64
												") AND (Date=='%q')",
												dbasetable.c_str(), idx, szTmp);
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
								else
								{
									// realtime graph
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
												sprintf(szTmp, "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
												break;
											case MTYPE_GAS:
												sprintf(szTmp, "%.2f", TotalValue / divider);
												break;
											case MTYPE_WATER:
												sprintf(szTmp, "%.3f", TotalValue / divider);
												break;
											case MTYPE_COUNTER:
												sprintf(szTmp, "%.10g", TotalValue / divider);
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
						if ((!bIsManagedCounter) && (bHaveFirstValue) && (method == 0))
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
									sprintf(szTmp, "%.3f", (TotalValue / divider) * 1000.0); // from kWh -> Watt
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.3f", TotalValue / divider);
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", TotalValue / divider);
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.10g", TotalValue / divider);
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
										sprintf(szTmp, "%.3f", usageValue / divider);
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.3f", usageValue / divider);
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.3f", usageValue);
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.3f", usageValue / divider);
										break;
									}
									root["result"][ii]["mu"] = szTmp;
								}
								ii++;
							}
						}
					}
				}
				else if (sensor == "uv")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

					result = m_sql.safe_query("SELECT Total, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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
									double mmval = ActTotal - WorkingHourStartValue;
									mmval *= AddjMulti;
									sprintf(szTmp, "%.1f", mmval);
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
						result = m_sql.safe_query("SELECT sValue, LastUpdate FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
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
						double mmval = LastValue - WorkingHourStartValue;
						if (mmval != 0)
						{
							root["result"][ii]["d"] = WorkingHourDate.substr(0, 14) + "00";
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["result"][ii]["mm"] = szTmp;
							ii++;
						}
					}
				}
				else if (sensor == "wind")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Direction, Speed, Gust, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
					if (!result.empty())
					{
						int ii = 0;
						for (const auto& sd : result)
						{
							root["result"][ii]["d"] = sd[3].substr(0, 16);
							root["result"][ii]["di"] = sd[0];

							int intSpeed = atoi(sd[1].c_str());
							int intGust = atoi(sd[2].c_str());

							if (m_sql.m_windunit != WINDUNIT_Beaufort)
							{
								sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
								root["result"][ii]["gu"] = szTmp;
							}
							else
							{
								float windspeedms = float(intSpeed) * 0.1F;
								float windgustms = float(intGust) * 0.1F;
								sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%d", MStoBeaufort(windgustms));
								root["result"][ii]["gu"] = szTmp;
							}
							ii++;
						}
					}
				}
				else if (sensor == "winddir")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Direction, Speed, Gust FROM %s WHERE (DeviceRowID==%" PRIu64 ") ORDER BY Date ASC", dbasetable.c_str(), idx);
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

						if (m_sql.m_windunit == WINDUNIT_MS)
						{
							szLegendLabels[0] = "&lt; 0.5 " + m_sql.m_windsign;
							szLegendLabels[1] = "0.5-2 " + m_sql.m_windsign;
							szLegendLabels[2] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[3] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[4] = "6-8 " + m_sql.m_windsign;
							szLegendLabels[5] = "8-10 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 10" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_KMH)
						{
							szLegendLabels[0] = "&lt; 2 " + m_sql.m_windsign;
							szLegendLabels[1] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[2] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[3] = "6-10 " + m_sql.m_windsign;
							szLegendLabels[4] = "10-20 " + m_sql.m_windsign;
							szLegendLabels[5] = "20-36 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 36" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_MPH)
						{
							szLegendLabels[0] = "&lt; 3 " + m_sql.m_windsign;
							szLegendLabels[1] = "3-7 " + m_sql.m_windsign;
							szLegendLabels[2] = "7-12 " + m_sql.m_windsign;
							szLegendLabels[3] = "12-18 " + m_sql.m_windsign;
							szLegendLabels[4] = "18-24 " + m_sql.m_windsign;
							szLegendLabels[5] = "24-46 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 46" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_Knots)
						{
							szLegendLabels[0] = "&lt; 3 " + m_sql.m_windsign;
							szLegendLabels[1] = "3-7 " + m_sql.m_windsign;
							szLegendLabels[2] = "7-17 " + m_sql.m_windsign;
							szLegendLabels[3] = "17-27 " + m_sql.m_windsign;
							szLegendLabels[4] = "27-34 " + m_sql.m_windsign;
							szLegendLabels[5] = "34-41 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 41" + m_sql.m_windsign;
						}
						else if (m_sql.m_windunit == WINDUNIT_Beaufort)
						{
							szLegendLabels[0] = "&lt; 2 " + m_sql.m_windsign;
							szLegendLabels[1] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[2] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[3] = "6-8 " + m_sql.m_windsign;
							szLegendLabels[4] = "8-10 " + m_sql.m_windsign;
							szLegendLabels[5] = "10-12 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 12" + m_sql.m_windsign;
						}
						else
						{
							// Todo !
							szLegendLabels[0] = "&lt; 0.5 " + m_sql.m_windsign;
							szLegendLabels[1] = "0.5-2 " + m_sql.m_windsign;
							szLegendLabels[2] = "2-4 " + m_sql.m_windsign;
							szLegendLabels[3] = "4-6 " + m_sql.m_windsign;
							szLegendLabels[4] = "6-8 " + m_sql.m_windsign;
							szLegendLabels[5] = "8-10 " + m_sql.m_windsign;
							szLegendLabels[6] = "&gt; 10" + m_sql.m_windsign;
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
							// float speed = speedOrg * m_sql.m_windscale;
							float gust = gustOrg * m_sql.m_windscale;
							int bucket = int(fdirection / 22.5F);

							int speedpos = 0;

							if (m_sql.m_windunit == WINDUNIT_MS)
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
							else if (m_sql.m_windunit == WINDUNIT_KMH)
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
							else if (m_sql.m_windunit == WINDUNIT_MPH)
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
							else if (m_sql.m_windunit == WINDUNIT_Knots)
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
							else if (m_sql.m_windunit == WINDUNIT_Beaufort)
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
								sprintf(szTmp, "%.2f", svalue);
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
								sprintf(szTmp, "%.2f", percentage);
								root["result"][ii]["div"] = szTmp;
								ii++;
							}
						}
					}
				}

			} // day
			else if (srange == "week")
			{
				if (sensor == "rain")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					char szDateStart[40];
					char szDateEnd[40];
					sprintf(szDateEnd, "%04d-%02d-%02d", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);

					// Subtract one week
					time_t weekbefore;
					struct tm tm2;
					getNoon(weekbefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 7); // We only want the date
					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
					int ii = 0;
					if (!result.empty())
					{
						for (const auto& sd : result)
						{
							root["result"][ii]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["result"][ii]["mm"] = szTmp;
							ii++;
						}
					}
					// add today (have to calculate it)
					if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
					{
						result = m_sql.safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1", idx,
							szDateEnd);
					}
					else
					{
						result = m_sql.safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
							total_real = total_max - total_min;
						}
						total_real *= AddjMulti;
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
				}
				else if (sensor == "counter")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;
					root["ValueQuantity"] = options["ValueQuantity"];
					root["ValueUnits"] = options["ValueUnits"];
					root["Divider"] = divider;

					char szDateStart[40];
					char szDateEnd[40];
					sprintf(szDateEnd, "%04d-%02d-%02d", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);

					// Subtract one week
					time_t weekbefore;
					struct tm tm2;
					getNoon(weekbefore, tm2, tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday - 7); // We only want the date
					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);

					int ii = 0;
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT Value1,Value2,Value5,Value6,Date FROM %s WHERE (DeviceRowID==%" PRIu64
							" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							bool bHaveDeliverd = false;
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[4].substr(0, 16);
								std::string szValueUsage1 = sd[0];
								std::string szValueDeliv1 = sd[1];
								std::string szValueUsage2 = sd[2];
								std::string szValueDeliv2 = sd[3];

								float fUsage1 = (float)(atof(szValueUsage1.c_str()));
								float fUsage2 = (float)(atof(szValueUsage2.c_str()));
								float fDeliv1 = (float)(atof(szValueDeliv1.c_str()));
								float fDeliv2 = (float)(atof(szValueDeliv2.c_str()));

								fDeliv1 = (fDeliv1 < 10) ? 0 : fDeliv1;
								fDeliv2 = (fDeliv2 < 10) ? 0 : fDeliv2;

								if ((fDeliv1 != 0) || (fDeliv2 != 0))
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage1 / divider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fUsage2 / divider);
								root["result"][ii]["v2"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv1 / divider);
								root["result"][ii]["r1"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv2 / divider);
								root["result"][ii]["r2"] = szTmp;
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[1].substr(0, 16);
								std::string szValue = sd[0];
								switch (metertype)
								{
								case MTYPE_ENERGY:
								case MTYPE_ENERGY_GENERATED:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								default:
									szValue = "0";
									break;
								}
								root["result"][ii]["v"] = szValue;
								ii++;
							}
						}
					}
					// add today (have to calculate it)
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT MIN(Value1), MAX(Value1), MIN(Value2), MAX(Value2),MIN(Value5), MAX(Value5), MIN(Value6), MAX(Value6) FROM "
							"MultiMeter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szDateEnd);
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							uint64_t total_min_usage_1 = std::stoull(sd[0]);
							uint64_t total_max_usage_1 = std::stoull(sd[1]);
							uint64_t total_min_usage_2 = std::stoull(sd[4]);
							uint64_t total_max_usage_2 = std::stoull(sd[5]);
							uint64_t total_real_usage_1, total_real_usage_2;
							uint64_t total_min_deliv_1 = std::stoull(sd[2]);
							uint64_t total_max_deliv_1 = std::stoull(sd[3]);
							uint64_t total_min_deliv_2 = std::stoull(sd[6]);
							uint64_t total_max_deliv_2 = std::stoull(sd[7]);
							uint64_t total_real_deliv_1, total_real_deliv_2;

							bool bHaveDeliverd = false;

							total_real_usage_1 = total_max_usage_1 - total_min_usage_1;
							total_real_usage_2 = total_max_usage_2 - total_min_usage_2;

							total_real_deliv_1 = total_max_deliv_1 - total_min_deliv_1;
							total_real_deliv_2 = total_max_deliv_2 - total_min_deliv_2;
							if ((total_real_deliv_1 != 0) || (total_real_deliv_2 != 0))
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szDateEnd;

							sprintf(szTmp, "%" PRIu64, total_real_usage_1);
							std::string szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v"] = szTmp;

							sprintf(szTmp, "%" PRIu64, total_real_usage_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v2"] = szTmp;

							sprintf(szTmp, "%" PRIu64, total_real_deliv_1);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["r1"] = szTmp;

							sprintf(szTmp, "%" PRIu64, total_real_deliv_2);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["r2"] = szTmp;

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
						result = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd);
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							int64_t total_min = std::stoll(sd[0]);
							int64_t total_max = total_min;
							int64_t total_real;

							// get the last value of the day
							result = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx, szDateEnd);
							if (!result.empty())
							{
								std::vector<std::string> sd = result[0];
								total_max = std::stoull(sd[0].c_str());
							}

							total_real = total_max - total_min;
							sprintf(szTmp, "%" PRId64, total_real);

							std::string szValue = szTmp;
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							default:
								szValue = "0";
								break;
							}

							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["v"] = szValue;
							ii++;
						}
					}
				}
			} // week
			else if (srange == "month" || srange == "year" || !sgroupby.empty())
			{
				char szDateStart[40];
				char szDateEnd[40];
				char szDateStartPrev[40];
				char szDateEndPrev[40];

				std::string sactmonth = request::findValue(&req, "actmonth");
				std::string sactyear = request::findValue(&req, "actyear");

				int actMonth = atoi(sactmonth.c_str());
				int actYear = atoi(sactyear.c_str());

				if ((!sactmonth.empty()) && (!sactyear.empty()))
				{
					sprintf(szDateStart, "%04d-%02d-%02d", actYear, actMonth, 1);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", actYear - 1, actMonth, 1);
					actMonth++;
					if (actMonth == 13)
					{
						actMonth = 1;
						actYear++;
					}
					sprintf(szDateEnd, "%04d-%02d-%02d", actYear, actMonth, 1);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", actYear - 1, actMonth, 1);
				}
				else if (!sactyear.empty())
				{
					sprintf(szDateStart, "%04d-%02d-%02d", actYear, 1, 1);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", actYear - 1, 1, 1);
					actYear++;
					sprintf(szDateEnd, "%04d-%02d-%02d", actYear, 1, 1);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", actYear - 1, 1, 1);
				}
				else
				{
					sprintf(szDateEnd, "%04d-%02d-%02d", tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday);
					sprintf(szDateEndPrev, "%04d-%02d-%02d", tm1.tm_year + 1900 - 1, tm1.tm_mon + 1, tm1.tm_mday);

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

					sprintf(szDateStart, "%04d-%02d-%02d", tm2.tm_year + 1900, tm2.tm_mon + 1, tm2.tm_mday);
					sprintf(szDateStartPrev, "%04d-%02d-%02d", tm2.tm_year + 1900 - 1, tm2.tm_mon + 1, tm2.tm_mday);
				}

				if (sensor == "temp")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					// Actual Year
					result = m_sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
						" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
						" SetPoint_Max, SetPoint_Avg "
						"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
								|| (dType == pTypeEvohomeZone)
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
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
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
								std::string value_unit = options["ValueUnit"];
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
					result = m_sql.safe_query("SELECT MIN(Temperature), MAX(Temperature),"
						" MIN(Chill), MAX(Chill), AVG(Humidity),"
						" AVG(Barometer), AVG(Temperature), MIN(SetPoint),"
						" MAX(SetPoint), AVG(SetPoint) "
						"FROM Temperature WHERE (DeviceRowID==%" PRIu64 ""
						" AND Date>='%q')",
						idx, szDateEnd);
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
							|| (dType == pTypeEvohomeZone)
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
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
								else
									root["result"][ii]["ba"] = sd[5];
							}
							else if (dType == pTypeTEMP_BARO)
							{
								sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
								root["result"][ii]["ba"] = szTmp;
							}
							else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
							{
								sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
								root["result"][ii]["ba"] = szTmp;
							}
						}
						if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
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
							std::string value_unit = options["ValueUnit"];
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
					result = m_sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
						" Humidity, Barometer, Temp_Avg, Date, SetPoint_Min,"
						" SetPoint_Max, SetPoint_Avg "
						"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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
								|| (dType == pTypeEvohomeZone)
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
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
										root["resultprev"][iPrev]["ba"] = szTmp;
									}
									else
										root["resultprev"][iPrev]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["resultprev"][iPrev]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["resultprev"][iPrev]["ba"] = szTmp;
								}
							}
							if ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))
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
								std::string value_unit = options["ValueUnit"];
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
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM %s WHERE (DeviceRowID==%" PRIu64
						" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query("SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage) FROM Percentage WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx,
						szDateEnd);
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
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Speed_Min, Speed_Max, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query("SELECT MIN(Speed), MAX(Speed) FROM Fan WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(),
						idx, szDateStart, szDateEnd);
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
					result = m_sql.safe_query("SELECT MAX(Level) FROM UV WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
					// Previous Year
					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC", dbasetable.c_str(),
						idx, szDateStartPrev, szDateEndPrev);
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
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
					int ii = 0;
					if (!result.empty())
					{
						for (const auto& sd : result)
						{
							root["result"][ii]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["result"][ii]["mm"] = szTmp;
							ii++;
						}
					}
					// add today (have to calculate it)
					if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
					{
						result = m_sql.safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1", idx,
							szDateEnd);
					}
					else
					{
						result = m_sql.safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
							total_real = total_max - total_min;
						}
						total_real *= AddjMulti;
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
					// Previous Year
					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
					if (!result.empty())
					{
						iPrev = 0;
						for (const auto& sd : result)
						{
							root["resultprev"][iPrev]["d"] = sd[2].substr(0, 16);
							double mmval = atof(sd[0].c_str());
							mmval *= AddjMulti;
							sprintf(szTmp, "%.1f", mmval);
							root["resultprev"][iPrev]["mm"] = szTmp;
							iPrev++;
						}
					}
				}
				else if (sensor == "counter")
				{
					root["status"] = "OK";
					root["title"] = sgroupby.empty() ? "Graph " + sensor + " " + srange : "Comparing " + sensor;
					root["ValueQuantity"] = options["ValueQuantity"];
					root["ValueUnits"] = options["ValueUnits"];
					root["Divider"] = divider;

					// int nValue = 0;
					std::string sValue; //Counter

					result = m_sql.safe_query("SELECT sValue FROM DeviceStatus WHERE (ID==%" PRIu64 ")", idx);
					if (!result.empty())
					{
						sValue = result[0][0];
					}

					std::function<std::string(std::string, std::string)> tableColumn = [](std::string table, std::string expr) {
						return (table.empty() ? "" : table + ".") + expr;
					};

					int ii = 0;
					iPrev = 0;
					if (dType == pTypeP1Power)
					{
						if (!sgroupby.empty()) {
							if (sensorarea.empty())
							{
								_log.Log(LOG_ERROR, "Parameter sensorarea missing with groupby '%s'", sgroupby.c_str());
								return;
							}
							std::function<std::string(const char*, const char*, const char*, const char*, const char*)> sensorareaExpr =
								[sensorarea, this](const char* expr, const char* usageLow, const char* usageNormal, const char* deliveryLow, const char* deliveryNormal) {
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
							GroupBy(
								root, dbasetable, idx, sgroupby, bIsManagedCounter,
								[counterExpr, tableColumn](std::string table) {
									return counterExpr(tableColumn(table, "Counter%s") + "+" + tableColumn(table, "Counter%s"));
								},
								[valueExpr, tableColumn](std::string table) { return valueExpr(tableColumn(table, "Value%s") + "+" + tableColumn(table, "Value%s")); },
									[divider, this](double sum) {
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
							result = m_sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date,"
								" Counter1, Counter2, Counter3, Counter4 "
								"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
								" AND Date<='%q') ORDER BY Date ASC",
								dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
									sprintf(szTmp, "%.3f", fUsage_1 / divider);
									root["result"][ii]["v"] = szTmp;
									sprintf(szTmp, "%.3f", fUsage_2 / divider);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.3f", fDeliv_1 / divider);
									root["result"][ii]["r1"] = szTmp;
									sprintf(szTmp, "%.3f", fDeliv_2 / divider);
									root["result"][ii]["r2"] = szTmp;

									if (counter_1 != 0)
									{
										sprintf(szTmp, "%.3f", (counter_1 - fUsage_1) / divider);
									}
									else
									{
										strcpy(szTmp, "0");
									}
									root["result"][ii]["c1"] = szTmp;

									if (counter_2 != 0)
									{
										sprintf(szTmp, "%.3f", (counter_2 - fDeliv_1) / divider);
									}
									else
									{
										strcpy(szTmp, "0");
									}
									root["result"][ii]["c2"] = szTmp;

									if (counter_3 != 0)
									{
										sprintf(szTmp, "%.3f", (counter_3 - fUsage_2) / divider);
									}
									else
									{
										strcpy(szTmp, "0");
									}
									root["result"][ii]["c3"] = szTmp;

									if (counter_4 != 0)
									{
										sprintf(szTmp, "%.3f", (counter_4 - fDeliv_2) / divider);
									}
									else
									{
										strcpy(szTmp, "0");
									}
									root["result"][ii]["c4"] = szTmp;

									ii++;
								}
								if (bHaveDeliverd)
								{
									root["delivered"] = true;
								}
							}
							// Previous Year
							result = m_sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date "
								"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
								dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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
									sprintf(szTmp, "%.3f", fUsage_1 / divider);
									root["resultprev"][iPrev]["v"] = szTmp;
									sprintf(szTmp, "%.3f", fUsage_2 / divider);
									root["resultprev"][iPrev]["v2"] = szTmp;
									sprintf(szTmp, "%.3f", fDeliv_1 / divider);
									root["resultprev"][iPrev]["r1"] = szTmp;
									sprintf(szTmp, "%.3f", fDeliv_2 / divider);
									root["resultprev"][iPrev]["r2"] = szTmp;
									iPrev++;
								}
								if (bHaveDeliverd)
								{
									root["delivered"] = true;
								}
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
						result = m_sql.safe_query("SELECT Value2,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
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
					}
					else if (((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
						((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt))))
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					}
					else if (((dType == pTypeGeneral) && (dSubType == sTypeVisibility)) || ((dType == pTypeGeneral) && (dSubType == sTypeDistance)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSolarRadiation)) || ((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeCurrent)) || ((dType == pTypeGeneral) && (dSubType == sTypePressure)) ||
						((dType == pTypeGeneral) && (dSubType == sTypeSoundLevel)))
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						float vdiv = 10.0F;
						if (((dType == pTypeGeneral) && (dSubType == sTypeVoltage)) || ((dType == pTypeGeneral) && (dSubType == sTypeCurrent)))
						{
							vdiv = 1000.0F;
						}

						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
									sprintf(szTmp, "%.3f", fValue1);
									root["result"][ii]["v_min"] = szTmp;
									sprintf(szTmp, "%.3f", fValue2);
									root["result"][ii]["v_max"] = szTmp;
									if (fValue3 != 0)
									{
										sprintf(szTmp, "%.3f", fValue3);
										root["result"][ii]["v_avg"] = szTmp;
									}
								}
								else
								{
									sprintf(szTmp, "%.1f", fValue1);
									root["result"][ii]["v_min"] = szTmp;
									sprintf(szTmp, "%.1f", fValue2);
									root["result"][ii]["v_max"] = szTmp;
									if (fValue3 != 0)
									{
										sprintf(szTmp, "%.1f", fValue3);
										root["result"][ii]["v_avg"] = szTmp;
									}
								}
								ii++;
							}
						}
					}
					else if (dType == pTypeLux)
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1,Value2,Value3, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
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
					}
					else if (dType == pTypeWEIGHT)
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[2].substr(0, 16);
								sprintf(szTmp, "%.1f", m_sql.m_weightscale * atof(sd[0].c_str()) / 10.0F);
								root["result"][ii]["v_min"] = szTmp;
								sprintf(szTmp, "%.1f", m_sql.m_weightscale * atof(sd[1].c_str()) / 10.0F);
								root["result"][ii]["v_max"] = szTmp;
								ii++;
							}
						}
					}
					else if (dType == pTypeUsage)
					{ // month/year
						root["status"] = "OK";
						root["title"] = "Graph " + sensor + " " + srange;

						result = m_sql.safe_query("SELECT Value1,Value2, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[2].substr(0, 16);
								root["result"][ii]["u_min"] = atof(sd[0].c_str()) / 10.0F;
								root["result"][ii]["u_max"] = atof(sd[1].c_str()) / 10.0F;
								ii++;
							}
						}
					}
					else if (dType == pTypeCURRENT)
					{
						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64
							" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							// CM113
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

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
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%.1f", fval4);
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%.1f", fval5);
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%.1f", fval6);
									root["result"][ii]["v6"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1 * voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2 * voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3 * voltage));
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%d", int(fval4 * voltage));
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%d", int(fval5 * voltage));
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%d", int(fval6 * voltage));
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
					else if (dType == pTypeCURRENTENERGY)
					{
						result = m_sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6, Date FROM %s WHERE (DeviceRowID==%" PRIu64
							" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart, szDateEnd);
						if (!result.empty())
						{
							// CM180i
							int displaytype = 0;
							int voltage = 230;
							m_sql.GetPreferencesVar("CM113DisplayType", displaytype);
							m_sql.GetPreferencesVar("ElectricVoltage", voltage);

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
									sprintf(szTmp, "%.1f", fval1);
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%.1f", fval2);
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%.1f", fval3);
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%.1f", fval4);
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%.1f", fval5);
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%.1f", fval6);
									root["result"][ii]["v6"] = szTmp;
								}
								else
								{
									sprintf(szTmp, "%d", int(fval1 * voltage));
									root["result"][ii]["v1"] = szTmp;
									sprintf(szTmp, "%d", int(fval2 * voltage));
									root["result"][ii]["v2"] = szTmp;
									sprintf(szTmp, "%d", int(fval3 * voltage));
									root["result"][ii]["v3"] = szTmp;
									sprintf(szTmp, "%d", int(fval4 * voltage));
									root["result"][ii]["v4"] = szTmp;
									sprintf(szTmp, "%d", int(fval5 * voltage));
									root["result"][ii]["v5"] = szTmp;
									sprintf(szTmp, "%d", int(fval6 * voltage));
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
					else
					{
						if (dType == pTypeP1Gas)
						{
							// Add last counter value
							sprintf(szTmp, "%.3f", atof(sValue.c_str()) / 1000.0);
							root["counter"] = szTmp;
						}
						else if (dType == pTypeENERGY)
						{
							size_t spos = sValue.find(';');
							if (spos != std::string::npos)
							{
								float fvalue = static_cast<float>(atof(sValue.substr(spos + 1).c_str()));
								sprintf(szTmp, "%.3f", fvalue / (divider / 100.0F));
								root["counter"] = szTmp;
							}
						}
						else if ((dType == pTypeGeneral) && (dSubType == sTypeKwh))
						{
							size_t spos = sValue.find(';');
							if (spos != std::string::npos)
							{
								float fvalue = static_cast<float>(atof(sValue.substr(spos + 1).c_str()));
								sprintf(szTmp, "%.3f", fvalue / divider);
								root["counter"] = szTmp;
							}
						}
						else if (dType == pTypeRFXMeter)
						{
							// Add last counter value
							double fvalue = atof(sValue.c_str());
							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f", meteroffset + (fvalue / divider));
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", meteroffset + (fvalue / divider));
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", meteroffset + (fvalue / divider));
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", meteroffset + (fvalue / divider));
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
									sprintf(szTmp, "%.3f", fvalue / divider);
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", fvalue / divider);
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", fvalue / divider);
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.10g", fvalue / divider);
									break;
								default:
									strcpy(szTmp, "");
									break;
								}
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
								sprintf(szTmp, "%.3f", meteroffset + (dvalue / divider));
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.3f", meteroffset + (dvalue / divider));
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", meteroffset + (dvalue / divider));
								break;
							case MTYPE_COUNTER:
							default:
								sprintf(szTmp, "%.10g", meteroffset + (dvalue / divider));
								break;
							}
							root["counter"] = szTmp;
						}
						else if (!bIsManagedCounter)
						{
							double dvalue = static_cast<double>(atof(sValue.c_str()));
							sprintf(szTmp, "%.10g", meteroffset + (dvalue / divider));
							root["counter"] = szTmp;
						}
						else
						{
							root["counter"] = "0";
						}
						if (!sgroupby.empty())
						{
							GroupBy(
								root, dbasetable, idx, sgroupby, bIsManagedCounter, [tableColumn](std::string table) { return tableColumn(table, "Counter"); },
								[tableColumn](std::string table) { return tableColumn(table, "Value"); },
								[metertype, AddjValue, divider, this](double sum) {
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
							result =
								m_sql.safe_query("SELECT Value, Date, Counter FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
									dbasetable.c_str(), idx, szDateStart, szDateEnd);
							if (!result.empty())
							{
								for (const auto& sd : result)
								{
									root["result"][ii]["d"] = sd[1].substr(0, 16);

									std::string szValue = sd[0];

									double fcounter = atof(sd[2].c_str());

									switch (metertype)
									{
									case MTYPE_ENERGY:
									case MTYPE_ENERGY_GENERATED:
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										if (fcounter != 0)
											sprintf(szTmp, "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
										else
											strcpy(szTmp, "0");
										root["result"][ii]["c"] = szTmp;
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										if (fcounter != 0)
											sprintf(szTmp, "%.2f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
										else
											strcpy(szTmp, "0");
										root["result"][ii]["c"] = szTmp;
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										if (fcounter != 0)
											sprintf(szTmp, "%.3f", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
										else
											strcpy(szTmp, "0");
										root["result"][ii]["c"] = szTmp;
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										if (fcounter != 0)
											sprintf(szTmp, "%.10g", meteroffset + ((fcounter - atof(szValue.c_str())) / divider));
										else
											strcpy(szTmp, "0");
										root["result"][ii]["c"] = szTmp;
										break;
									}
									ii++;
								}
							}
							// Past Year
							result =
								m_sql.safe_query("SELECT Value, Date, Counter FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
									dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
							if (!result.empty())
							{
								iPrev = 0;
								for (const auto& sd : result)
								{
									root["resultprev"][iPrev]["d"] = sd[1].substr(0, 16);

									std::string szValue = sd[0];
									switch (metertype)
									{
									case MTYPE_ENERGY:
									case MTYPE_ENERGY_GENERATED:
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["resultprev"][iPrev]["v"] = szTmp;
										break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", atof(szValue.c_str()) / divider);
										root["resultprev"][iPrev]["v"] = szTmp;
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["resultprev"][iPrev]["v"] = szTmp;
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
										root["resultprev"][iPrev]["v"] = szTmp;
										break;
									}
									iPrev++;
								}
							}
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
								sprintf(szDateEnd, "%04d-%02d-%02d", loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
							}
						}
						else if (!sactyear.empty())
						{
							bool bIsThisYear = (atoi(sactyear.c_str()) == loctime.tm_year + 1900);
							if (bIsThisYear)
							{
								sprintf(szDateEnd, "%04d-%02d-%02d", loctime.tm_year + 1900, loctime.tm_mon + 1, loctime.tm_mday);
							}
						}
					}

					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT "
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
							idx, szDateEnd);
						bool bHaveDeliverd = false;
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];
							uint64_t total_min_usage_1 = std::stoull(sd[0]);
							uint64_t total_max_usage_1 = std::stoull(sd[1]);
							uint64_t total_min_usage_2 = std::stoull(sd[4]);
							uint64_t total_max_usage_2 = std::stoull(sd[5]);
							uint64_t total_real_usage_1, total_real_usage_2;
							uint64_t total_min_deliv_1 = std::stoull(sd[2]);
							uint64_t total_max_deliv_1 = std::stoull(sd[3]);
							uint64_t total_min_deliv_2 = std::stoull(sd[6]);
							uint64_t total_max_deliv_2 = std::stoull(sd[7]);
							uint64_t total_real_deliv_1, total_real_deliv_2;

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
								AddTodayValueToResult(root, sgroupby, std::string(szDateEnd), todayValue, "%.3f");
							}
							else
							{
								root["result"][ii]["d"] = szDateEnd;

								sprintf(szTmp, "%.3f", (float)(total_real_usage_1 / divider));
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", (float)(total_real_usage_2 / divider));
								root["result"][ii]["v2"] = szTmp;

								sprintf(szTmp, "%.3f", (float)(total_real_deliv_1 / divider));
								root["result"][ii]["r1"] = szTmp;
								sprintf(szTmp, "%.3f", (float)(total_real_deliv_2 / divider));
								root["result"][ii]["r2"] = szTmp;

								sprintf(szTmp, "%.3f", (float)(total_min_usage_1 / divider));
								root["result"][ii]["c1"] = szTmp;
								sprintf(szTmp, "%.3f", (float)(total_min_usage_2 / divider));
								root["result"][ii]["c3"] = szTmp;

								if (total_max_deliv_2 != 0)
								{
									sprintf(szTmp, "%.3f", (float)(total_min_deliv_1 / divider));
									root["result"][ii]["c2"] = szTmp;
									sprintf(szTmp, "%.3f", (float)(total_min_deliv_2 / divider));
									root["result"][ii]["c4"] = szTmp;
								}
								else
								{
									strcpy(szTmp, "0");
									root["result"][ii]["c2"] = szTmp;
									root["result"][ii]["c4"] = szTmp;
								}

								ii++;
							}
						}
						if (bHaveDeliverd)
						{
							root["delivered"] = true;
						}
					}
					else if (dType == pTypeAirQuality)
					{
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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

						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
								sprintf(szTmp, "%.3f", fValue1);
							else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
								sprintf(szTmp, "%.3f", fValue1);
							else
								sprintf(szTmp, "%.1f", fValue1);
							root["result"][ii]["v_min"] = szTmp;
							if ((dType == pTypeGeneral) && (dSubType == sTypeVoltage))
								sprintf(szTmp, "%.3f", fValue2);
							else if ((dType == pTypeGeneral) && (dSubType == sTypeCurrent))
								sprintf(szTmp, "%.3f", fValue2);
							else
								sprintf(szTmp, "%.1f", fValue2);
							root["result"][ii]["v_max"] = szTmp;
							ii++;
						}
					}
					else if (dType == pTypeLux)
					{
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
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
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
						if (!result.empty())
						{
							root["result"][ii]["d"] = szDateEnd;
							sprintf(szTmp, "%.1f", m_sql.m_weightscale * atof(result[0][0].c_str()) / 10.0F);
							root["result"][ii]["v_min"] = szTmp;
							sprintf(szTmp, "%.1f", m_sql.m_weightscale * atof(result[0][1].c_str()) / 10.0F);
							root["result"][ii]["v_max"] = szTmp;
							ii++;
						}
					}
					else if (dType == pTypeUsage)
					{
						result = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')", idx, szDateEnd);
						if (!result.empty())
						{
							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["u_min"] = atof(result[0][0].c_str()) / 10.0F;
							root["result"][ii]["u_max"] = atof(result[0][1].c_str()) / 10.0F;
							ii++;
						}
					}
					else if (!bIsManagedCounter)
					{
						/*if (sgroupby == "year") {

						} else*/
						{
							// get the first value
							result = m_sql.safe_query(
								//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
								"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd);
							if (!result.empty())
							{
								std::vector<std::string> sd = result[0];
								int64_t total_min = std::stoll(sd[0]);
								int64_t total_max = total_min;
								int64_t total_real;

								// Get the last value
								result = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx,
									szDateEnd);
								if (!result.empty())
								{
									std::vector<std::string> sd = result[0];
									total_max = std::stoull(sd[0]);
								}

								total_real = total_max - total_min;
								sprintf(szTmp, "%" PRId64, total_real);

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
									AddTodayValueToResult(root, sgroupby, std::string(szDateEnd), todayValue, formatString);
								}
								else
								{
									root["result"][ii]["d"] = szDateEnd;
									switch (metertype)
									{
									case MTYPE_ENERGY:
									case MTYPE_ENERGY_GENERATED: {
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;

										std::vector<std::string> mresults;
										StringSplit(sValue, ";", mresults);
										if (mresults.size() == 2)
										{
											sValue = mresults[1];
										}
										if (dType == pTypeENERGY)
											sprintf(szTmp, "%.3f", meteroffset + (((atof(sValue.c_str()) * 100.0F) - atof(szValue.c_str())) / divider));
										else
											sprintf(szTmp, "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
										root["result"][ii]["c"] = szTmp;
									}
															   break;
									case MTYPE_GAS:
										sprintf(szTmp, "%.2f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										sprintf(szTmp, "%.2f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
										root["result"][ii]["c"] = szTmp;
										break;
									case MTYPE_WATER:
										sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										sprintf(szTmp, "%.3f", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
										root["result"][ii]["c"] = szTmp;
										break;
									case MTYPE_COUNTER:
										sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
										root["result"][ii]["v"] = szTmp;
										sprintf(szTmp, "%.10g", meteroffset + ((atof(sValue.c_str()) - atof(szValue.c_str())) / divider));
										root["result"][ii]["c"] = szTmp;
										break;
									}
									ii++;
								}
							}
						}
					}
				}
				else if (sensor == "wind")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int ii = 0;

					result = m_sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
						" Gust_Max, Date "
						"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart, szDateEnd);
					if (!result.empty())
					{
						for (const auto& sd : result)
						{
							root["result"][ii]["d"] = sd[5].substr(0, 16);
							root["result"][ii]["di"] = sd[0];

							int intSpeed = atoi(sd[2].c_str());
							int intGust = atoi(sd[4].c_str());
							if (m_sql.m_windunit != WINDUNIT_Beaufort)
							{
								sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
								root["result"][ii]["gu"] = szTmp;
							}
							else
							{
								float windspeedms = float(intSpeed) * 0.1F;
								float windgustms = float(intGust) * 0.1F;
								sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%d", MStoBeaufort(windgustms));
								root["result"][ii]["gu"] = szTmp;
							}
							ii++;
						}
					}
					// add today (have to calculate it)
					result = m_sql.safe_query("SELECT AVG(Direction), MIN(Speed), MAX(Speed),"
						" MIN(Gust), MAX(Gust) "
						"FROM Wind WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC",
						idx, szDateEnd);
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						int intGust = atoi(sd[4].c_str());
						if (m_sql.m_windunit != WINDUNIT_Beaufort)
						{
							sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
							root["result"][ii]["sp"] = szTmp;
							sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
							root["result"][ii]["gu"] = szTmp;
						}
						else
						{
							float windspeedms = float(intSpeed) * 0.1F;
							float windgustms = float(intGust) * 0.1F;
							sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
							root["result"][ii]["sp"] = szTmp;
							sprintf(szTmp, "%d", MStoBeaufort(windgustms));
							root["result"][ii]["gu"] = szTmp;
						}
						ii++;
					}
					// Previous Year
					result = m_sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
						" Gust_Max, Date "
						"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
						" AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStartPrev, szDateEndPrev);
					if (!result.empty())
					{
						iPrev = 0;
						for (const auto& sd : result)
						{
							root["resultprev"][iPrev]["d"] = sd[5].substr(0, 16);
							root["resultprev"][iPrev]["di"] = sd[0];

							int intSpeed = atoi(sd[2].c_str());
							int intGust = atoi(sd[4].c_str());
							if (m_sql.m_windunit != WINDUNIT_Beaufort)
							{
								sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								root["resultprev"][iPrev]["sp"] = szTmp;
								sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
								root["resultprev"][iPrev]["gu"] = szTmp;
							}
							else
							{
								float windspeedms = float(intSpeed) * 0.1F;
								float windgustms = float(intGust) * 0.1F;
								sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
								root["resultprev"][iPrev]["sp"] = szTmp;
								sprintf(szTmp, "%d", MStoBeaufort(windgustms));
								root["resultprev"][iPrev]["gu"] = szTmp;
							}
							iPrev++;
						}
					}
				}
			}													 // month or year
			else if ((srange.substr(0, 1) == "2") && (srange.substr(10, 1) == "T") && (srange.substr(11, 1) == "2")) // custom range 2013-01-01T2013-12-31
			{
				std::string szDateStart = srange.substr(0, 10);
				std::string szDateEnd = srange.substr(11, 10);
				std::string sgraphtype = request::findValue(&req, "graphtype");
				std::string sgraphTemp = request::findValue(&req, "graphTemp");
				std::string sgraphChill = request::findValue(&req, "graphChill");
				std::string sgraphHum = request::findValue(&req, "graphHum");
				std::string sgraphBaro = request::findValue(&req, "graphBaro");
				std::string sgraphDew = request::findValue(&req, "graphDew");
				std::string sgraphSet = request::findValue(&req, "graphSet");

				if (sensor == "temp")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					bool sendTemp = false;
					bool sendChill = false;
					bool sendHum = false;
					bool sendBaro = false;
					bool sendDew = false;
					bool sendSet = false;

					if ((sgraphTemp == "true") &&
						((dType == pTypeRego6XXTemp) || (dType == pTypeTEMP) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) ||
							(dType == pTypeWIND) || (dType == pTypeThermostat1) || (dType == pTypeRadiator1) || ((dType == pTypeUV) && (dSubType == sTypeUV3)) ||
							((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeRFXSensor) && (dSubType == sTypeRFXSensorTemp)) ||
							((dType == pTypeSetpoint) && (dSubType == sTypeSetpoint)) || (dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater)))
					{
						sendTemp = true;
					}
					if ((sgraphSet == "true") && ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater))) // FIXME cheat for water setpoint is just on or off
					{
						sendSet = true;
					}
					if ((sgraphChill == "true") && (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))))
					{
						sendChill = true;
					}
					if ((sgraphHum == "true") && ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO)))
					{
						sendHum = true;
					}
					if ((sgraphBaro == "true") && ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro))))
					{
						sendBaro = true;
					}
					if ((sgraphDew == "true") && ((dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO)))
					{
						sendDew = true;
					}

					if (sgraphtype == "1")
					{
						// Need to get all values of the end date so 23:59:59 is appended to the date string
						result = m_sql.safe_query("SELECT Temperature, Chill, Humidity, Barometer,"
							" Date, DewPoint, SetPoint "
							"FROM Temperature WHERE (DeviceRowID==%" PRIu64 ""
							" AND Date>='%q' AND Date<='%q 23:59:59') ORDER BY Date ASC",
							idx, szDateStart.c_str(), szDateEnd.c_str());
						int ii = 0;
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[4]; //.substr(0,16);
								if (sendTemp)
								{
									double te = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									root["result"][ii]["te"] = te;
									root["result"][ii]["tm"] = tm;
								}
								if (sendChill)
								{
									double ch = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double cm = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									root["result"][ii]["ch"] = ch;
									root["result"][ii]["cm"] = cm;
								}
								if (sendHum)
								{
									root["result"][ii]["hu"] = sd[2];
								}
								if (sendBaro)
								{
									if (dType == pTypeTEMP_HUM_BARO)
									{
										if (dSubType == sTypeTHBFloat)
										{
											sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
											root["result"][ii]["ba"] = szTmp;
										}
										else
											root["result"][ii]["ba"] = sd[3];
									}
									else if (dType == pTypeTEMP_BARO)
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
									else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
									{
										sprintf(szTmp, "%.1f", atof(sd[3].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
								}
								if (sendDew)
								{
									double dp = ConvertTemperature(atof(sd[5].c_str()), tempsign);
									root["result"][ii]["dp"] = dp;
								}
								if (sendSet)
								{
									double se = ConvertTemperature(atof(sd[6].c_str()), tempsign);
									root["result"][ii]["se"] = se;
								}
								ii++;
							}
						}
					}
					else
					{
						result = m_sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
							" Humidity, Barometer, Date, DewPoint, Temp_Avg,"
							" SetPoint_Min, SetPoint_Max, SetPoint_Avg "
							"FROM Temperature_Calendar "
							"WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
							" AND Date<='%q') ORDER BY Date ASC",
							idx, szDateStart.c_str(), szDateEnd.c_str());
						int ii = 0;
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[6].substr(0, 16);
								if (sendTemp)
								{
									double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
									double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
									double ta = ConvertTemperature(atof(sd[8].c_str()), tempsign);

									root["result"][ii]["te"] = te;
									root["result"][ii]["tm"] = tm;
									root["result"][ii]["ta"] = ta;
								}
								if (sendChill)
								{
									double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
									double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);

									root["result"][ii]["ch"] = ch;
									root["result"][ii]["cm"] = cm;
								}
								if (sendHum)
								{
									root["result"][ii]["hu"] = sd[4];
								}
								if (sendBaro)
								{
									if (dType == pTypeTEMP_HUM_BARO)
									{
										if (dSubType == sTypeTHBFloat)
										{
											sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
											root["result"][ii]["ba"] = szTmp;
										}
										else
											root["result"][ii]["ba"] = sd[5];
									}
									else if (dType == pTypeTEMP_BARO)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
									else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
								}
								if (sendDew)
								{
									double dp = ConvertTemperature(atof(sd[7].c_str()), tempsign);
									root["result"][ii]["dp"] = dp;
								}
								if (sendSet)
								{
									double sm = ConvertTemperature(atof(sd[9].c_str()), tempsign);
									double sx = ConvertTemperature(atof(sd[10].c_str()), tempsign);
									double se = ConvertTemperature(atof(sd[11].c_str()), tempsign);
									root["result"][ii]["sm"] = sm;
									root["result"][ii]["se"] = se;
									root["result"][ii]["sx"] = sx;
									char szTmp[1024];
									sprintf(szTmp, "%.1f %.1f %.1f", sm, se, sx);
									_log.Log(LOG_STATUS, "%s", szTmp);
								}
								ii++;
							}
						}

						// add today (have to calculate it)
						result = m_sql.safe_query("SELECT MIN(Temperature), MAX(Temperature),"
							" MIN(Chill), MAX(Chill), AVG(Humidity),"
							" AVG(Barometer), MIN(DewPoint), AVG(Temperature),"
							" MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) "
							"FROM Temperature WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szDateEnd.c_str());
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							root["result"][ii]["d"] = szDateEnd;
							if (sendTemp)
							{
								double te = ConvertTemperature(atof(sd[1].c_str()), tempsign);
								double tm = ConvertTemperature(atof(sd[0].c_str()), tempsign);
								double ta = ConvertTemperature(atof(sd[7].c_str()), tempsign);

								root["result"][ii]["te"] = te;
								root["result"][ii]["tm"] = tm;
								root["result"][ii]["ta"] = ta;
							}
							if (sendChill)
							{
								double ch = ConvertTemperature(atof(sd[3].c_str()), tempsign);
								double cm = ConvertTemperature(atof(sd[2].c_str()), tempsign);
								root["result"][ii]["ch"] = ch;
								root["result"][ii]["cm"] = cm;
							}
							if (sendHum)
							{
								root["result"][ii]["hu"] = sd[4];
							}
							if (sendBaro)
							{
								if (dType == pTypeTEMP_HUM_BARO)
								{
									if (dSubType == sTypeTHBFloat)
									{
										sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
										root["result"][ii]["ba"] = szTmp;
									}
									else
										root["result"][ii]["ba"] = sd[5];
								}
								else if (dType == pTypeTEMP_BARO)
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
								else if ((dType == pTypeGeneral) && (dSubType == sTypeBaro))
								{
									sprintf(szTmp, "%.1f", atof(sd[5].c_str()) / 10.0F);
									root["result"][ii]["ba"] = szTmp;
								}
							}
							if (sendDew)
							{
								double dp = ConvertTemperature(atof(sd[6].c_str()), tempsign);
								root["result"][ii]["dp"] = dp;
							}
							if (sendSet)
							{
								double sm = ConvertTemperature(atof(sd[8].c_str()), tempsign);
								double sx = ConvertTemperature(atof(sd[9].c_str()), tempsign);
								double se = ConvertTemperature(atof(sd[10].c_str()), tempsign);

								root["result"][ii]["sm"] = sm;
								root["result"][ii]["se"] = se;
								root["result"][ii]["sx"] = sx;
							}
							ii++;
						}
					}
				}
				else if (sensor == "uv")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ""
						" AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
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
					result = m_sql.safe_query("SELECT MAX(Level) FROM UV WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
				}
				else if (sensor == "rain")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					result = m_sql.safe_query("SELECT Total, Rate, Date FROM %s "
						"WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
					int ii = 0;
					if (!result.empty())
					{
						for (const auto& sd : result)
						{
							root["result"][ii]["d"] = sd[2].substr(0, 16);
							root["result"][ii]["mm"] = sd[0];
							ii++;
						}
					}
					// add today (have to calculate it)
					if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
					{
						result = m_sql.safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1", idx,
							szDateEnd.c_str());
					}
					else
					{
						result = m_sql.safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szDateEnd.c_str());
					}
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						float total_min = static_cast<float>(atof(sd[0].c_str()));
						float total_max = static_cast<float>(atof(sd[1].c_str()));
						// int rate = atoi(sd[2].c_str());

						float total_real = 0;
						if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
						{
							total_real = total_max;
						}
						else
						{
							total_real = total_max - total_min;
						}
						sprintf(szTmp, "%.1f", total_real);
						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
				}
				else if (sensor == "counter")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;
					root["ValueQuantity"] = options["ValueQuantity"];
					root["ValueUnits"] = options["ValueUnits"];
					root["Divider"] = divider;

					int ii = 0;
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date "
							"FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q'"
							" AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
						if (!result.empty())
						{
							bool bHaveDeliverd = false;
							for (const auto& sd : result)
							{
								root["result"][ii]["d"] = sd[4].substr(0, 16);

								std::string szUsage1 = sd[0];
								std::string szDeliv1 = sd[1];
								std::string szUsage2 = sd[2];
								std::string szDeliv2 = sd[3];

								float fUsage = (float)(atof(szUsage1.c_str()) + atof(szUsage2.c_str()));
								float fDeliv = (float)(atof(szDeliv1.c_str()) + atof(szDeliv2.c_str()));

								if (fDeliv != 0)
									bHaveDeliverd = true;
								sprintf(szTmp, "%.3f", fUsage / divider);
								root["result"][ii]["v"] = szTmp;
								sprintf(szTmp, "%.3f", fDeliv / divider);
								root["result"][ii]["v2"] = szTmp;
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
						result = m_sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
							dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
						if (!result.empty())
						{
							for (const auto& sd : result)
							{
								std::string szValue = sd[0];
								switch (metertype)
								{
								case MTYPE_ENERGY:
								case MTYPE_ENERGY_GENERATED:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_GAS:
									sprintf(szTmp, "%.2f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_WATER:
									sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;
								case MTYPE_COUNTER:
									sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
									szValue = szTmp;
									break;

								}
								root["result"][ii]["d"] = sd[1].substr(0, 16);
								root["result"][ii]["v"] = szValue;
								ii++;
							}
						}
					}
					// add today (have to calculate it)
					if (dType == pTypeP1Power)
					{
						result = m_sql.safe_query("SELECT MIN(Value1), MAX(Value1), MIN(Value2),"
							" MAX(Value2),MIN(Value5), MAX(Value5),"
							" MIN(Value6), MAX(Value6) "
							"FROM MultiMeter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szDateEnd.c_str());
						bool bHaveDeliverd = false;
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							uint64_t total_min_usage_1 = std::stoull(sd[0]);
							uint64_t total_max_usage_1 = std::stoull(sd[1]);
							uint64_t total_min_usage_2 = std::stoull(sd[4]);
							uint64_t total_max_usage_2 = std::stoull(sd[5]);
							uint64_t total_real_usage;

							uint64_t total_min_deliv_1 = std::stoull(sd[2]);
							uint64_t total_max_deliv_1 = std::stoull(sd[3]);
							uint64_t total_min_deliv_2 = std::stoull(sd[6]);
							uint64_t total_max_deliv_2 = std::stoull(sd[7]);
							uint64_t total_real_deliv;

							total_real_usage = (total_max_usage_1 + total_max_usage_2) - (total_min_usage_1 + total_min_usage_2);
							total_real_deliv = (total_max_deliv_1 + total_max_deliv_2) - (total_min_deliv_1 + total_min_deliv_2);

							if (total_real_deliv != 0)
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szDateEnd;

							sprintf(szTmp, "%" PRIu64, total_real_usage);
							std::string szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v"] = szTmp;

							sprintf(szTmp, "%" PRIu64, total_real_deliv);
							szValue = szTmp;
							sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v2"] = szTmp;
							
							ii++;
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else if (!bIsManagedCounter)
					{ // get the first value of the day
						result = m_sql.safe_query(
							//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szDateEnd.c_str());
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];
							int64_t total_min = std::stoll(sd[0]);
							int64_t total_max = total_min;
							int64_t total_real;

							// get the last value of the day
							result = m_sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx,
								szDateEnd.c_str());
							if (!result.empty())
							{
								std::vector<std::string> sd = result[0];
								total_max = std::stoull(sd[0]);
							}

							total_real = total_max - total_min;
							sprintf(szTmp, "%" PRId64, total_real);							std::string szValue = szTmp;

							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								sprintf(szTmp, "%.2f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								sprintf(szTmp, "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_COUNTER:
								sprintf(szTmp, "%.10g", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							}

							root["result"][ii]["d"] = szDateEnd;
							root["result"][ii]["v"] = szValue;
							ii++;
						}
					}
				}
				else if (sensor == "wind")
				{
					root["status"] = "OK";
					root["title"] = "Graph " + sensor + " " + srange;

					int ii = 0;

					result = m_sql.safe_query("SELECT Direction, Speed_Min, Speed_Max, Gust_Min,"
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
							if (m_sql.m_windunit != WINDUNIT_Beaufort)
							{
								sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
								root["result"][ii]["gu"] = szTmp;
							}
							else
							{
								float windspeedms = float(intSpeed) * 0.1F;
								float windgustms = float(intGust) * 0.1F;
								sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
								root["result"][ii]["sp"] = szTmp;
								sprintf(szTmp, "%d", MStoBeaufort(windgustms));
								root["result"][ii]["gu"] = szTmp;
							}
							ii++;
						}
					}
					// add today (have to calculate it)
					result = m_sql.safe_query("SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==%" PRIu64
						" AND Date>='%q') ORDER BY Date ASC",
						idx, szDateEnd.c_str());
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szDateEnd;
						root["result"][ii]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						int intGust = atoi(sd[4].c_str());
						if (m_sql.m_windunit != WINDUNIT_Beaufort)
						{
							sprintf(szTmp, "%.1f", float(intSpeed) * m_sql.m_windscale);
							root["result"][ii]["sp"] = szTmp;
							sprintf(szTmp, "%.1f", float(intGust) * m_sql.m_windscale);
							root["result"][ii]["gu"] = szTmp;
						}
						else
						{
							float windspeedms = float(intSpeed) * 0.1F;
							float windgustms = float(intGust) * 0.1F;
							sprintf(szTmp, "%d", MStoBeaufort(windspeedms));
							root["result"][ii]["sp"] = szTmp;
							sprintf(szTmp, "%d", MStoBeaufort(windgustms));
							root["result"][ii]["gu"] = szTmp;
						}
						ii++;
					}
				}
			} // custom range
		}

	} // namespace server
} // namespace http
