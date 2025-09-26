#include "stdafx.h"
#include "KWHStats.h"
#include "json_helper.h"
#include "Logger.h"
#include "SQLHelper.h"
#include <thread>
#include <atomic>
#include <mutex>

#include <inttypes.h>
#include <map>


std::atomic<bool> g_kwhstats_saver_running{ false };
std::thread g_kwhstats_saver_thread;
StoppableTask g_kwhstats_task;
std::mutex m_task_mutex;

std::map<uint64_t, CKWHStats> g_kwhstats;


void CKWHStats::InitGlobal(const int interval_seconds)
{
	g_kwhstats_task.RequestStart();

	g_kwhstats_saver_running = true;
	g_kwhstats_saver_thread = std::thread([interval_seconds]() { PeriodicSaveKWHStats(interval_seconds); });
}

void CKWHStats::ExitGlobal()
{
	g_kwhstats_saver_running = false;
	if (g_kwhstats_saver_thread.joinable())
	{
		g_kwhstats_task.RequestStop();
		g_kwhstats_saver_thread.join();
	}
}

void CKWHStats::PeriodicSaveKWHStats(const int interval_seconds) // default: every 5 minutes
{
	while (!g_kwhstats_task.IsStopRequested(interval_seconds * 1000))
	{
		std::unique_lock<std::mutex> lock(m_task_mutex);

		for (auto &itt : g_kwhstats)
		{
			itt.second.SaveToDB();
		}
	}
}


CKWHStats::CKWHStats()
{
}

CKWHStats::~CKWHStats()
{
	SaveToDB();
}

void CKWHStats::Init(const uint64_t deviceID)
{
	m_device_id = deviceID;
	LoadFromDB();
}

void CKWHStats::AddHourValue(const int hour, const int Watt)
{
	if (hour < 0 || hour > 23)
		return;
	daily_hour_kwh[hour] = (daily_hour_kwh[hour] != 0) ? (daily_hour_kwh[hour] + Watt) / 2 : Watt;
	weekday_hour_kwh_raw[hour] = Watt;

	const time_t atime = time(nullptr);
	struct tm now;
	localtime_r(&atime, &now);
	int wday = now.tm_wday; // days since Sunday [0-6]

	weekday_hour_kwh[wday][hour] = (weekday_hour_kwh[wday][hour] != 0) ? (weekday_hour_kwh[wday][hour] + Watt) / 2 : Watt;

	m_bDirty = true;
}

void CKWHStats::FinishDay()
{
	const time_t atime = time(nullptr) - (24 * 3600); //subtract a day, because it's about the previous day
	struct tm now;
	localtime_r(&atime, &now);
	int wday = now.tm_wday; // days since Sunday [0-6]

	int total = 0;
	for (int hour = 0; hour < 24; hour++)
		total += weekday_hour_kwh_raw[hour];

	weekday_kwh[wday] = (weekday_kwh[wday] != 0) ? (weekday_kwh[wday] + total) / 2 : total;
	weekday_hour_kwh_raw.fill(0);
	m_bDirty = true;
}

bool CKWHStats::LoadFromDB()
{
	// clean existing values
	daily_hour_kwh.fill(0);
	weekday_hour_kwh_raw.fill(0);
	weekday_kwh.fill(0);
	for (auto& arr : weekday_hour_kwh) arr.fill(0);

	auto result = m_sql.safe_query("SELECT Value FROM KWHStats WHERE  (DeviceRowID==%" PRIu64 ")", m_device_id);
	if (result.empty())
		return false;

	Json::Value root;

	bool ret = ParseJSon(result[0][0], root);
	if ((!ret) || (!root.isObject()))
	{
		return false;
	}

	// daily_hour_kwh
	if (root.isMember("daily_hour_kwh") && root["daily_hour_kwh"].isArray())
	{
		for (int hour = 0; hour < 24; hour++)
			daily_hour_kwh[hour] = root["daily_hour_kwh"][hour].asInt();
	}
	// weekday_hour_kwh_raw
	if (root.isMember("weekday_hour_kwh_raw") && root["weekday_hour_kwh_raw"].isArray())
	{
		for (int hour = 0; hour < 24; hour++)
			weekday_hour_kwh_raw[hour] = root["weekday_hour_kwh_raw"][hour].asInt();
	}
	// weekday_kwh
	if (root.isMember("weekday_kwh") && root["weekday_kwh"].isArray())
	{
		for (int wday = 0; wday < 7; wday++)
			weekday_kwh[wday] = root["weekday_kwh"][wday].asInt();
	}
	// weekday_hour_kwh
	if (root.isMember("weekday_hour_kwh") && root["weekday_hour_kwh"].isArray())
	{
		for (int wday = 0; wday < 7; wday++)
		{
			if (root["weekday_hour_kwh"][wday].isArray())
			{
				for (int hour = 0; hour < 24; hour++)
					weekday_hour_kwh[wday][hour] = root["weekday_hour_kwh"][wday][hour].asInt();
			}
		}
	}

	return true;
}

bool CKWHStats::SaveToDB()
{
	if (!m_bDirty)
		return false; // nothing changed, no need to save

	m_bDirty = false;

	Json::Value root;

	root["daily_hour_kwh"] = Json::Value(Json::arrayValue);
	for (int hour = 0; hour < 24; hour++)
		root["daily_hour_kwh"].append(daily_hour_kwh[hour]);
	root["weekday_hour_kwh_raw"] = Json::Value(Json::arrayValue);
	for (int hour = 0; hour < 24; hour++)
		root["weekday_hour_kwh_raw"].append(weekday_hour_kwh_raw[hour]);
	root["weekday_kwh"] = Json::Value(Json::arrayValue);
	for (int wday = 0; wday < 7; wday++)
		root["weekday_kwh"].append(weekday_kwh[wday]);
	root["weekday_hour_kwh"] = Json::Value(Json::arrayValue);
	for (int wday = 0; wday < 7; wday++)
	{
		root["weekday_hour_kwh"].append(Json::Value(Json::arrayValue));
		for (int hour = 0; hour < 24; hour++)
			root["weekday_hour_kwh"][wday].append(weekday_hour_kwh[wday][hour]);
	}

	std::string out = JSonToRawString(root);

	auto result = m_sql.safe_query("SELECT ID FROM KWHStats WHERE  (DeviceRowID==%" PRIu64 ")", m_device_id);
	// either insert or update
	if (result.empty())
	{
		// insert
		m_sql.safe_query("INSERT INTO KWHStats (DeviceRowID, Value) VALUES(%" PRIu64 ", '%q')", m_device_id, out.c_str());
		return true;
	}
	else
	{
		// update
		std::string id = result[0][0];
		m_sql.safe_query("UPDATE KWHStats SET Value='%q', LastUpdate='%s' WHERE (ID=%s)", out.c_str(), TimeToString(nullptr, TF_DateTime).c_str(), id.c_str());
	}
	return true;
}

void CKWHStats::HandleKWHStatsHour()
{
	time_t atime = time(nullptr);
	struct tm now;
	localtime_r(&atime, &now);
	now.tm_min = 0;
	now.tm_sec = 0;

	const int actHour = now.tm_hour;

	atime = mktime(&now);
	atime -= 3600; // last hour
	struct tm last_hour;
	localtime_r(&atime, &last_hour);
	const int hour = last_hour.tm_hour;

	char szStartTime[20];
	snprintf(szStartTime, sizeof(szStartTime), "%04d-%02d-%02d %02d:%02d:%02d", last_hour.tm_year + 1900, last_hour.tm_mon + 1, last_hour.tm_mday, last_hour.tm_hour, 0, 0);

	//First handle all P1 meters
	auto result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (Type=%d)", pTypeP1Power);
	for (const auto& itt : result)
	{
		const uint64_t device_id = std::stoull(itt[0]);

		//Get the total kWh usage for the last hour
		auto result2 = m_sql.safe_query("SELECT MIN(Value1), MIN(Value5), MIN(Value2), MIN(Value6), MAX(Value1), MAX(Value5), MAX(Value2), MAX(Value6) FROM Multimeter WHERE (DeviceRowID==%" PRIu64 ") AND ([Date] > '%q')", device_id, szStartTime);
		if (!result2.empty())
		{
			std::unique_lock<std::mutex> lock(m_task_mutex);

			if (g_kwhstats.find(device_id) == g_kwhstats.end())
			{
				//First time we see this device, create the object
				CKWHStats kwhs;
				kwhs.Init(device_id);
				g_kwhstats[device_id] = kwhs;
			}

			const int64_t minUsage1 = std::stoll(result2[0][0]);
			const int64_t minUsage2 = std::stoll(result2[0][1]);
			const int64_t minDeliv1 = std::stoll(result2[0][2]);
			const int64_t minDeliv2 = std::stoll(result2[0][3]);

			const int64_t maxUsage1 = std::stoll(result2[0][4]);
			const int64_t maxUsage2 = std::stoll(result2[0][5]);
			const int64_t maxDeliv1 = std::stoll(result2[0][6]);
			const int64_t maxDeliv2 = std::stoll(result2[0][7]);

			const int64_t minUsage = minUsage1 + minUsage2;
			const int64_t minDeliv = minDeliv1 + minDeliv2;
			const int64_t maxUsage = maxUsage1 + maxUsage2;
			const int64_t maxDeliv = maxDeliv1 + maxDeliv2;

			const int64_t actUsage = (maxUsage - minUsage);
			const int64_t actDeliv = (maxDeliv - minDeliv);

			const int Wh = static_cast<int>(actUsage - actDeliv);

			g_kwhstats[device_id].AddHourValue(hour, Wh);

			if (actHour == 0)
			{
				// we just passed midnight, finish the day
				g_kwhstats[device_id].FinishDay();
			}
		}
	}

	// Next, handle all kWh sensors
	result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (Type=%d) AND (SubType=%d)", pTypeGeneral, sTypeKwh);
	for (const auto& itt : result)
	{
		const uint64_t device_id = std::stoull(itt[0]);

		//Get the total kWh usage for the last hour
		auto result2 = m_sql.safe_query("SELECT MIN(Value), MAX(Value) FROM Meter WHERE (DeviceRowID==%" PRIu64 ") AND ([Date] > '%q')", device_id, szStartTime);
		if (!result2.empty())
		{
			std::unique_lock<std::mutex> lock(m_task_mutex);

			if (g_kwhstats.find(device_id) == g_kwhstats.end())
			{
				//First time we see this device, create the object
				CKWHStats kwhs;
				kwhs.Init(device_id);
				g_kwhstats[device_id] = kwhs;
			}

			const int64_t minUsage = std::stoll(result2[0][0]);
			const int64_t maxUsage = std::stoll(result2[0][1]);

			const int64_t actUsage = (maxUsage - minUsage);

			const int Wh = static_cast<int>(actUsage);

			g_kwhstats[device_id].AddHourValue(hour, Wh);

			if (actHour == 0)
			{
				// we just passed midnight, finish the day
				g_kwhstats[device_id].FinishDay();
			}
		}
	}

}
