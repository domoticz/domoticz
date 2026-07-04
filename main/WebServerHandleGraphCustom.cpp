/*
 * WebServerHandleGraphCustom.cpp
 *
 * Phase 2 / F08 – handler for custom date range ("2013-01-01T2013-12-31").
 *
 * The logic here is extracted verbatim from CWebServer::Cmd_HandleGraph in
 * WebServerHandleGraph.cpp.  No behaviour has been changed.
 */

#include "stdafx.h"
#include "WebServer.h"
#include "WebServerHandleGraphInternals.h"

#define __STDC_FORMAT_MACROS
#include <ctime>
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
// Helper: compute dbasetable for the custom range (same as month/year)
// ---------------------------------------------------------------------------
static std::string CalcDbasetableCustom(const GraphContext& ctx)
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
void HandleGraphCustomRange(const GraphContext& ctx, const request& req,
                             Json::Value& root, CSQLHelper& sql,
                             CWebServer& /*webserver*/)
{
	const uint64_t idx             = ctx.idx;
	const unsigned char dType      = ctx.dType;
	const unsigned char dSubType   = ctx.dSubType;
	const _eMeterType metertype    = ctx.metertype;
	const bool bIsManagedCounter   = ctx.bIsManagedCounter;
	const double divider           = ctx.divider;
	const std::string& sensor      = ctx.sensor;
	const std::string& srange      = ctx.srange;
	const std::map<std::string, std::string>& options = ctx.options;

	std::string dbasetable = CalcDbasetableCustom(ctx);
	unsigned char tempsign = sql.m_tempsign[0];

	char szTmp[64];
	std::vector<std::vector<std::string>> result;

			std::string szDateStart = srange.substr(0, 10);
			std::string szDateEnd = srange.substr(11, 10);
			std::string sgraphtype = request::findValue(&req, "graphtype");
			std::string sgraphTemp = request::findValue(&req, "graphTemp");
			std::string sgraphChill = request::findValue(&req, "graphChill");
			std::string sgraphHum = request::findValue(&req, "graphHum");
			std::string sgraphBaro = request::findValue(&req, "graphBaro");
			std::string sgraphDew = request::findValue(&req, "graphDew");
			std::string sgraphSet = request::findValue(&req, "graphSet");

			std::string szToday = "";
			bool bAddToday = false;

			// get today and check if within date range
			const time_t now = mytime(nullptr);
			struct tm tmNow;
			localtime_r(&now, &tmNow);
			char buf[16];
			strftime(buf, sizeof(buf), "%Y-%m-%d", &tmNow);
			szToday = buf;
			if (szToday >= szDateStart && szToday <= szDateEnd)
			{
				bAddToday = true;
			}

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
						((dType == pTypeSetpoint) && (dSubType == sTypeSetpoint)) || (dType == pTypeEvohomeZone) || (dType == pTypeThermostat6) || (dType == pTypeEvohomeWater)))
				{
					sendTemp = true;
				}
				if ((sgraphSet == "true") && ((dType == pTypeEvohomeZone) || (dType == pTypeEvohomeWater) || (dType == pTypeThermostat6))) // FIXME cheat for water setpoint is just on or off
				{
					sendSet = true;
				}
				if ((sgraphChill == "true") && (((dType == pTypeWIND) && (dSubType == sTypeWIND4)) || ((dType == pTypeWIND) && (dSubType == sTypeWINDNoTemp))))
				{
					sendChill = true;
				}
				if ((sgraphHum == "true") && ((dType == pTypeHUM) || (dType == pTypeTEMP_HUM) || (dType == pTypeTEMP_HUM_BARO) || ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempHum) || (dSubType == sTypeThermostat6TempHumBaro)))))
				{
					sendHum = true;
				}
				if ((sgraphBaro == "true") && ((dType == pTypeTEMP_HUM_BARO) || (dType == pTypeTEMP_BARO) || ((dType == pTypeGeneral) && (dSubType == sTypeBaro)) || ((dType == pTypeThermostat6) && ((dSubType == sTypeThermostat6TempBaro) || (dSubType == sTypeThermostat6TempHumBaro)))))
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
					result = sql.safe_query("SELECT Temperature, Chill, Humidity, Barometer,"
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
					result = sql.safe_query("SELECT Temp_Min, Temp_Max, Chill_Min, Chill_Max,"
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
								snprintf(szTmp, sizeof(szTmp), "%.1f %.1f %.1f", sm, se, sx);
								_log.Log(LOG_STATUS, "%s", szTmp);
							}
							ii++;
						}
					}

					// add today (have to calculate it)
					if (bAddToday)
					{
						result = sql.safe_query("SELECT MIN(Temperature), MAX(Temperature),"
							" MIN(Chill), MAX(Chill), AVG(Humidity),"
							" AVG(Barometer), MIN(DewPoint), AVG(Temperature),"
							" MIN(SetPoint), MAX(SetPoint), AVG(SetPoint) "
							"FROM Temperature WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							root["result"][ii]["d"] = szToday;
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
			}
			else if (sensor == "uv")
			{
				root["status"] = "OK";
				root["title"] = "Graph " + sensor + " " + srange;

				result = sql.safe_query("SELECT Level, Date FROM %s WHERE (DeviceRowID==%" PRIu64 ""
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
				if (bAddToday)
				{
					result = sql.safe_query("SELECT MAX(Level) FROM UV WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szToday.c_str());
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szToday;
						root["result"][ii]["uvi"] = sd[0];
						ii++;
					}
				}
			}
			else if (sensor == "rain")
			{
				root["status"] = "OK";
				root["title"] = "Graph " + sensor + " " + srange;

				result = sql.safe_query("SELECT Total, Rate, Date FROM %s "
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
				if (bAddToday)
				{
					if (dSubType == sTypeRAINWU || dSubType == sTypeRAINByRate)
					{
						result = sql.safe_query("SELECT Total, Total, Rate FROM Rain WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY ROWID DESC LIMIT 1", idx,
							szToday.c_str());
					}
					else
					{
						result = sql.safe_query("SELECT MIN(Total), MAX(Total), MAX(Rate) FROM Rain WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')", idx, szToday.c_str());
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
							total_real = std::max(0.0F, total_max - total_min);
						}
						snprintf(szTmp, sizeof(szTmp), "%.1f", total_real);
						root["result"][ii]["d"] = szToday;
						root["result"][ii]["mm"] = szTmp;
						ii++;
					}
				}
			}
			else if (sensor == "counter")
			{
				root["status"] = "OK";
				root["title"] = "Graph " + sensor + " " + srange;
				root["ValueQuantity"] = (options.count("ValueQuantity") ? options.at("ValueQuantity") : std::string{});
				root["ValueUnits"] = (options.count("ValueUnits") ? options.at("ValueUnits") : std::string{});
				root["Divider"] = divider;

				// Built-in counter families carry no ValueUnits option, but their unit is
				// fixed by device type. Populate it once here (the single place that owns
				// the custom-range series) so consumers such as the MCP server can label
				// values without re-deriving units. Gap-fill only: never overrides an
				// option-provided unit, and does not touch the emitted values. This is a
				// single per-response unit, correct because every family here uses one unit
				// for all of its series. Current/CM113 is set in its own branch below (which
				// already reads the A-vs-Watt display preference, so we do not re-read it
				// here); LeafWetness intentionally has no unit.
				// metertype: 0 = metric, 1 = imperial (matches the value conversion below).
				if (root["ValueUnits"].asString().empty())
				{
					std::string vu;
					if (dType == pTypeAirQuality)
						vu = "ppm";
					else if (dType == pTypeLux)
						vu = "Lux";
					else if (dType == pTypeUsage)
						vu = "Watt";
					else if (dType == pTypeWEIGHT)
						vu = sql.m_weightsign; // verified "kg" or "lb" (SQLHelper.cpp:10542/10547)
					else if (dType == pTypeRFXSensor && (dSubType == sTypeRFXSensorAD || dSubType == sTypeRFXSensorVolt))
						vu = "mV";
					else if (dType == pTypeGeneral)
					{
						if (dSubType == sTypeVoltage)
							vu = "V";
						else if (dSubType == sTypeCurrent)
							vu = "A";
						else if (dSubType == sTypePressure)
							vu = "Bar";
						else if (dSubType == sTypeSoundLevel)
							vu = "dB";
						else if (dSubType == sTypeSolarRadiation)
							vu = "Watt/m2";
						else if (dSubType == sTypeSoilMoisture)
							vu = "cb";
						else if (dSubType == sTypeVisibility)
							vu = (metertype == 0) ? "km" : "mi";
						else if (dSubType == sTypeDistance)
							vu = (metertype == 0) ? "cm" : "in";
					}
					if (!vu.empty())
						root["ValueUnits"] = vu;
				}

				int ii = 0;
				if (dType == pTypeP1Power)
				{
					int P1DisplayType = 0; //0=Low/High tariff, 1=simple (for dynamic contracts)
					sql.GetPreferencesVar("P1DisplayType", P1DisplayType);
					root["P1DisplayType"] = P1DisplayType;

					result = sql.safe_query("SELECT Value1,Value2,Value5,Value6, Date "
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
							snprintf(szTmp, sizeof(szTmp), "%.3f", fUsage / divider);
							root["result"][ii]["v1"] = szTmp;
							snprintf(szTmp, sizeof(szTmp), "%.3f", fDeliv / divider);
							root["result"][ii]["v2"] = szTmp;
							ii++;
						}
						if (bHaveDeliverd)
						{
							root["delivered"] = true;
						}
					}
				}
				else if (dType == pTypeAirQuality)
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
				}
				else if (dType == pTypeLux)
				{
					result = sql.safe_query("SELECT Value1,Value2,Value3,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
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
				}
				else if (dType == pTypeWEIGHT)
				{
					result = sql.safe_query("SELECT Value1,Value2,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
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
				}
				else if (dType == pTypeCURRENT || dType == pTypeCURRENTENERGY)
				{
					result = sql.safe_query("SELECT Value1,Value2,Value3,Value4,Value5,Value6,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
						dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
					if (!result.empty())
					{
						int displaytype = 0;
						int voltage = 230;
						sql.GetPreferencesVar("CM113DisplayType", displaytype);
						sql.GetPreferencesVar("ElectricVoltage", voltage);
						root["displaytype"] = displaytype;
						if (root["ValueUnits"].asString().empty())
							root["ValueUnits"] = (displaytype == 1) ? "Watt" : "A";
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
							root["haveL1"] = true;
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
				else if (((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
					((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt))))
				{
					result = sql.safe_query("SELECT Value1,Value2,Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
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
				}
				else if ((dType == pTypeGeneral) && ((dSubType == sTypeVisibility) || (dSubType == sTypeDistance) ||
					(dSubType == sTypeSolarRadiation) || (dSubType == sTypeVoltage) ||
					(dSubType == sTypeCurrent) || (dSubType == sTypePressure) || (dSubType == sTypeSoundLevel)))
				{
					float vdiv = 10.0F;
					if ((dSubType == sTypeVoltage) || (dSubType == sTypeCurrent))
						vdiv = 1000.0F;
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
								if (dSubType == sTypeDistance)
								{
									fValue1 *= 0.3937007874015748F;
									fValue2 *= 0.3937007874015748F;
								}
								else
								{
									fValue1 *= 0.6214F;
									fValue2 *= 0.6214F;
								}
							}
							if ((dSubType == sTypeVoltage) || (dSubType == sTypeCurrent))
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
				}
				else
				{
					result = sql.safe_query("SELECT Value, Date FROM %s WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
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
								snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_COUNTER:
								snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
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
				if (bAddToday)
				{
					if (dType == pTypeP1Power)
					{
						result = sql.safe_query("SELECT MIN(Value1), MAX(Value1), MIN(Value2),"
							" MAX(Value2),MIN(Value5), MAX(Value5),"
							" MIN(Value6), MAX(Value6) "
							"FROM MultiMeter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						bool bHaveDeliverd = false;
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];

							int64_t total_min_usage_1 = std::stoll(sd[0]);
							int64_t total_max_usage_1 = std::stoll(sd[1]);
							int64_t total_min_usage_2 = std::stoll(sd[4]);
							int64_t total_max_usage_2 = std::stoll(sd[5]);
							int64_t total_real_usage;

							int64_t total_min_deliv_1 = std::stoll(sd[2]);
							int64_t total_max_deliv_1 = std::stoll(sd[3]);
							int64_t total_min_deliv_2 = std::stoll(sd[6]);
							int64_t total_max_deliv_2 = std::stoll(sd[7]);
							int64_t total_real_deliv;

							total_real_usage = (total_max_usage_1 + total_max_usage_2) - (total_min_usage_1 + total_min_usage_2);
							total_real_deliv = (total_max_deliv_1 + total_max_deliv_2) - (total_min_deliv_1 + total_min_deliv_2);

							if (total_real_deliv != 0)
								bHaveDeliverd = true;

							root["result"][ii]["d"] = szToday;

							snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real_usage);
							std::string szValue = szTmp;
							snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v1"] = szTmp;

							snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real_deliv);
							szValue = szTmp;
							snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
							root["result"][ii]["v2"] = szTmp;

							ii++;
							if (bHaveDeliverd)
							{
								root["delivered"] = true;
							}
						}
					}
					else if (dType == pTypeAirQuality)
					{
						result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							root["result"][ii]["co2_min"] = result[0][0];
							root["result"][ii]["co2_max"] = result[0][1];
							root["result"][ii]["co2_avg"] = result[0][2];
							ii++;
						}
					}
					else if (dType == pTypeLux)
					{
						result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							root["result"][ii]["lux_min"] = result[0][0];
							root["result"][ii]["lux_max"] = result[0][1];
							root["result"][ii]["lux_avg"] = result[0][2];
							ii++;
						}
					}
					else if (dType == pTypeWEIGHT)
					{
						result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(result[0][0].c_str()) / 10.0F);
							root["result"][ii]["v_min"] = szTmp;
							snprintf(szTmp, sizeof(szTmp), "%.1f", sql.m_weightscale * atof(result[0][1].c_str()) / 10.0F);
							root["result"][ii]["v_max"] = szTmp;
							ii++;
						}
					}
					else if (dType == pTypeUsage)
					{
						result = sql.safe_query("SELECT MIN(Value), MAX(Value), AVG(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							root["result"][ii]["u_min"] = atof(result[0][0].c_str()) / 10.0F;
							root["result"][ii]["u_max"] = atof(result[0][1].c_str()) / 10.0F;
							root["result"][ii]["u_avg"] = static_cast<int>((atof(result[0][2].c_str()) / 10.0F) + 0.5F);
							ii++;
						}
					}
					else if (dType == pTypeCURRENT || dType == pTypeCURRENTENERGY)
					{
						// today data for multi-channel current meters resides in MultiMeter, not Meter; skip
					}
					else if (((dType == pTypeGeneral) && ((dSubType == sTypeSoilMoisture) || (dSubType == sTypeLeafWetness))) ||
						((dType == pTypeRFXSensor) && ((dSubType == sTypeRFXSensorAD) || (dSubType == sTypeRFXSensorVolt))))
					{
						result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							root["result"][ii]["v_min"] = result[0][0];
							root["result"][ii]["v_max"] = result[0][1];
							ii++;
						}
					}
					else if ((dType == pTypeGeneral) && ((dSubType == sTypeVisibility) || (dSubType == sTypeDistance) ||
						(dSubType == sTypeSolarRadiation) || (dSubType == sTypeVoltage) ||
						(dSubType == sTypeCurrent) || (dSubType == sTypePressure) || (dSubType == sTypeSoundLevel)))
					{
						float vdiv = 10.0F;
						if ((dSubType == sTypeVoltage) || (dSubType == sTypeCurrent))
							vdiv = 1000.0F;
						result = sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							idx, szToday.c_str());
						if (!result.empty() && !result[0][0].empty())
						{
							root["result"][ii]["d"] = szToday;
							float fValue1 = float(atof(result[0][0].c_str())) / vdiv;
							float fValue2 = float(atof(result[0][1].c_str())) / vdiv;
							if (metertype == 1)
							{
								if (dSubType == sTypeDistance)
								{
									fValue1 *= 0.3937007874015748F;
									fValue2 *= 0.3937007874015748F;
								}
								else
								{
									fValue1 *= 0.6214F;
									fValue2 *= 0.6214F;
								}
							}
							if ((dSubType == sTypeVoltage) || (dSubType == sTypeCurrent))
							{
								snprintf(szTmp, sizeof(szTmp), "%.3f", fValue1);
								root["result"][ii]["v_min"] = szTmp;
								snprintf(szTmp, sizeof(szTmp), "%.3f", fValue2);
								root["result"][ii]["v_max"] = szTmp;
							}
							else
							{
								snprintf(szTmp, sizeof(szTmp), "%.1f", fValue1);
								root["result"][ii]["v_min"] = szTmp;
								snprintf(szTmp, sizeof(szTmp), "%.1f", fValue2);
								root["result"][ii]["v_max"] = szTmp;
							}
							ii++;
						}
					}
					else if (!bIsManagedCounter)
					{ // get the first value of the day
						result = sql.safe_query(
							//"SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q')",
							"SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date ASC LIMIT 1", idx, szToday.c_str());
						if (!result.empty())
						{
							std::vector<std::string> sd = result[0];
							int64_t total_min = std::stoll(sd[0]);
							int64_t total_max = total_min;
							int64_t total_real;

							// get the last value of the day
							result = sql.safe_query("SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q') ORDER BY Date DESC LIMIT 1", idx, szToday.c_str());
							if (!result.empty())
							{
								std::vector<std::string> sd = result[0];
								total_max = std::stoull(sd[0]);
							}

							total_real = total_max - total_min;
							snprintf(szTmp, sizeof(szTmp), "%" PRId64, total_real);							
							std::string szValue = szTmp;

							switch (metertype)
							{
							case MTYPE_ENERGY:
							case MTYPE_ENERGY_GENERATED:
								snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_GAS:
								snprintf(szTmp, sizeof(szTmp), "%.2f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_WATER:
								snprintf(szTmp, sizeof(szTmp), "%.3f", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							case MTYPE_COUNTER:
								snprintf(szTmp, sizeof(szTmp), "%.10g", atof(szValue.c_str()) / divider);
								szValue = szTmp;
								break;
							}

							root["result"][ii]["d"] = szToday;
							root["result"][ii]["v"] = szValue;
							ii++;
						}
					}
				}
			}
			else if (sensor == "wind")
			{
				root["status"] = "OK";
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
				if (bAddToday)
				{
					// add today (have to calculate it)
					result = sql.safe_query("SELECT AVG(Direction), MIN(Speed), MAX(Speed), MIN(Gust), MAX(Gust) FROM Wind WHERE (DeviceRowID==%" PRIu64
						" AND Date>='%q') ORDER BY Date ASC",
						idx, szToday.c_str());
					if (!result.empty())
					{
						std::vector<std::string> sd = result[0];

						root["result"][ii]["d"] = szToday;
						root["result"][ii]["di"] = sd[0];

						int intSpeed = atoi(sd[2].c_str());
						int intGust = atoi(sd[4].c_str());
						root["result"][ii]["sp"] = FormatWindSpeed(intSpeed, sql.m_windunit, sql.m_windscale);
						root["result"][ii]["gu"] = FormatWindSpeed(intGust, sql.m_windunit, sql.m_windscale);
						ii++;
					}
				}
			}
		else if (sensor == "Percentage")
		{
			root["status"] = "OK";
			root["title"] = "Graph " + sensor + " " + srange;

			int ii = 0;
			result = sql.safe_query("SELECT Percentage_Min, Percentage_Max, Percentage_Avg, Date FROM %s"
				" WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					root["result"][ii]["d"]     = sd[3].substr(0, 16);
					root["result"][ii]["v_min"] = sd[0];
					root["result"][ii]["v_max"] = sd[1];
					root["result"][ii]["v_avg"] = sd[2];
					ii++;
				}
			}
			// add today (have to calculate it)
			if (bAddToday)
			{
				result = sql.safe_query("SELECT MIN(Percentage), MAX(Percentage), AVG(Percentage)"
					" FROM Percentage WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
					idx, szToday.c_str());
				if (!result.empty() && !result[0][0].empty())
				{
					const auto& sd = result[0];
					root["result"][ii]["d"] = szToday;
					root["result"][ii]["v_min"] = sd[0];
					root["result"][ii]["v_max"] = sd[1];
					root["result"][ii]["v_avg"] = sd[2];
					ii++;
				}
			}
		}
		else if (sensor == "fan")
		{
			root["status"] = "OK";
			root["title"] = "Graph " + sensor + " " + srange;

			int ii = 0;
			result = sql.safe_query("SELECT Speed_Min, Speed_Max, Date FROM %s"
				" WHERE (DeviceRowID==%" PRIu64 " AND Date>='%q' AND Date<='%q') ORDER BY Date ASC",
				dbasetable.c_str(), idx, szDateStart.c_str(), szDateEnd.c_str());
			if (!result.empty())
			{
				for (const auto& sd : result)
				{
					root["result"][ii]["d"]     = sd[2].substr(0, 16);
					root["result"][ii]["v_min"] = sd[0];
					root["result"][ii]["v_max"] = sd[1];
					ii++;
				}
			}
			// add today (have to calculate it)
			if (bAddToday)
			{
				result = sql.safe_query("SELECT MIN(Speed), MAX(Speed) FROM Fan"
					" WHERE (DeviceRowID=%" PRIu64 " AND Date>='%q')",
					idx, szToday.c_str());
				if (!result.empty() && !result[0][0].empty())
				{
					const auto& sd = result[0];
					root["result"][ii]["d"] = szToday;
					root["result"][ii]["v_min"] = sd[0];
					root["result"][ii]["v_max"] = sd[1];
					ii++;
				}
			}
		}

} // HandleGraphCustomRange

} // namespace server
} // namespace http
