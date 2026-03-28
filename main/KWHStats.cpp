#include "stdafx.h"
#include "KWHStats.h"
#include "json_helper.h"
#include "Logger.h"
#include "SQLHelper.h"
#include <thread>
#include <atomic>
#include <mutex>

#include <climits>
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
	g_kwhstats.clear();
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

bool CKWHStats::GetJSONStats(const uint64_t device_id, Json::Value &root)
{
	std::unique_lock<std::mutex> lock(m_task_mutex);
	if (g_kwhstats.find(device_id) == g_kwhstats.end())
	{
		//First time we see this device, create the object
		CKWHStats kwhs;
		kwhs.Init(device_id);
		g_kwhstats[device_id] = kwhs;
	}
	g_kwhstats[device_id].MakeJSONStats(root);
	return true;
}

bool CKWHStats::ResetJSONStats(const uint64_t device_id)
{
	std::unique_lock<std::mutex> lock(m_task_mutex);

	m_sql.safe_query("DELETE FROM KWHStats WHERE (DeviceRowID==%" PRIu64 ")", device_id);

	if (g_kwhstats.find(device_id) == g_kwhstats.end())
	{
		//First time we see this device, create the object
		CKWHStats kwhs;
		kwhs.Init(device_id);
		g_kwhstats[device_id] = kwhs;
	}
	g_kwhstats[device_id].Init(device_id);// re-init
	return true;
}

bool CKWHStats::RemoveSpikeStats(const uint64_t device_id)
{
	std::unique_lock<std::mutex> lock(m_task_mutex);

	if (g_kwhstats.find(device_id) == g_kwhstats.end())
	{
		CKWHStats kwhs;
		kwhs.Init(device_id);
		g_kwhstats[device_id] = kwhs;
	}

	CKWHStats& kwhs = g_kwhstats[device_id];

	// Derive hourly spike threshold from the distribution of all non-zero weekday_hour_kwh values.
	// Uses Q3 + 3*IQR — the same method as the frontend chart spike detector.
	std::vector<int> all_hourly;
	all_hourly.reserve(DAYS_PER_WEEK * HOURS_PER_DAY);
	for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
		for (int hour = 0; hour < HOURS_PER_DAY; hour++)
			if (kwhs.weekday_hour_kwh[wday][hour] > 0)
				all_hourly.push_back(kwhs.weekday_hour_kwh[wday][hour]);

	if (all_hourly.empty())
		return false;

	std::sort(all_hourly.begin(), all_hourly.end());
	const size_t n = all_hourly.size();
	const int64_t q1 = all_hourly[n / 4];
	const int64_t q3 = all_hourly[n * 3 / 4];
	const int64_t iqr = q3 - q1;
	// If IQR is 0 (all values the same), use 10x the median as fallback to avoid flagging everything
	const int64_t median = all_hourly[n / 2];
	const int64_t hourly_fence = (iqr > 0) ? (q3 + 3 * iqr) : (median * 10);
	const int hourly_threshold = static_cast<int>(std::min(hourly_fence, (int64_t)INT_MAX));
	// Daily threshold: 24 peak-hours at the hourly fence
	const int daily_threshold = static_cast<int>(std::min((int64_t)hourly_threshold * 24, (int64_t)INT_MAX));

	bool changed = false;

	// Fix weekday_hour_kwh: replace each spike with the average of non-spike values
	// for the same hour across all other weekdays (computed before any modification).
	// Falls back to 0 only when every weekday cell for that hour is also a spike.
	for (int hour = 0; hour < HOURS_PER_DAY; hour++)
	{
		int64_t sum = 0;
		int count = 0;
		for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
		{
			int v = kwhs.weekday_hour_kwh[wday][hour];
			if (v > 0 && v <= hourly_threshold)
			{
				sum += v;
				count++;
			}
		}
		if (count == 0)
		{
			bool any_spike = false;
			for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
				if (kwhs.weekday_hour_kwh[wday][hour] > hourly_threshold) { any_spike = true; break; }
			if (any_spike)
				_log.Log(LOG_STATUS, "KWHStats: device %" PRIu64 " hour %d: no valid baseline across weekdays, zeroing spike(s)", device_id, hour);
		}
		const int replacement = (count > 0) ? static_cast<int>(sum / count) : 0;
		for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
		{
			if (kwhs.weekday_hour_kwh[wday][hour] > hourly_threshold)
			{
				kwhs.weekday_hour_kwh[wday][hour] = replacement;
				changed = true;
			}
		}
	}

	// Fix daily_hour_kwh: replace spikes with average of (now-repaired) weekday values for same hour.
	// Only marks changed when a valid baseline is found; leaves the value intact otherwise so
	// a subsequent normal sample can correct it through the running average.
	for (int hour = 0; hour < HOURS_PER_DAY; hour++)
	{
		if (kwhs.daily_hour_kwh[hour] > hourly_threshold)
		{
			int64_t sum = 0;
			int count = 0;
			for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
			{
				int v = kwhs.weekday_hour_kwh[wday][hour];
				if (v > 0 && v <= hourly_threshold)
				{
					sum += v;
					count++;
				}
			}
			if (count > 0)
			{
				kwhs.daily_hour_kwh[hour] = static_cast<int>(sum / count);
				changed = true;
			}
			else
			{
				_log.Log(LOG_STATUS, "KWHStats: device %" PRIu64 " daily hour %d: no valid baseline found, leaving spike intact", device_id, hour);
			}
		}
	}

	// Fix weekday_kwh: replace spikes with the average of non-spike daily totals.
	// Falls back to 0 only when every weekday total is also a spike.
	{
		int64_t sum = 0;
		int count = 0;
		for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
		{
			int v = kwhs.weekday_kwh[wday];
			if (v > 0 && v <= daily_threshold)
			{
				sum += v;
				count++;
			}
		}
		if (count == 0)
		{
			bool any_spike = false;
			for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
				if (kwhs.weekday_kwh[wday] > daily_threshold) { any_spike = true; break; }
			if (any_spike)
				_log.Log(LOG_STATUS, "KWHStats: device %" PRIu64 " weekday totals: no valid baseline, zeroing spike(s)", device_id);
		}
		const int replacement = (count > 0) ? static_cast<int>(sum / count) : 0;
		for (int wday = 0; wday < DAYS_PER_WEEK; wday++)
		{
			if (kwhs.weekday_kwh[wday] > daily_threshold)
			{
				kwhs.weekday_kwh[wday] = replacement;
				changed = true;
			}
		}
	}

	if (changed)
	{
		kwhs.m_bDirty = true;
		kwhs.SaveToDB();
	}

	return changed;
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

	// clean existing values
	daily_hour_kwh.fill(0);
	weekday_hour_kwh_raw.fill(0);
	weekday_kwh.fill(0);
	for (auto& arr : weekday_hour_kwh) arr.fill(0);

	LoadFromDB();
}

void CKWHStats::AddHourValue(const int hour, const int wday, const int Watt)
{
	if (hour < 0 || hour > 23)
		return;

	// Skip extreme outliers to prevent corrupting the running averages
	// (e.g. meter counter resets or bad sensor readings)
	const bool daily_spike = (daily_hour_kwh[hour] > 0) && (Watt > daily_hour_kwh[hour] * 100);
	const bool weekly_spike = (weekday_hour_kwh[wday][hour] > 0) && (Watt > weekday_hour_kwh[wday][hour] * 100);
	// Absolute cap: >100 kWh in a single hour is physically unrealistic for typical home/building devices.
	// This guards against corrupt first readings when no prior average is available yet.
	const bool absolute_spike = (Watt > 100000);
	if (daily_spike || weekly_spike || absolute_spike)
		return;

	daily_hour_kwh[hour] = (daily_hour_kwh[hour] != 0) ? (daily_hour_kwh[hour] + Watt) / 2 : Watt;
	weekday_hour_kwh_raw[hour] = Watt;

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
	auto result = m_sql.safe_query("SELECT Value FROM KWHStats WHERE (DeviceRowID==%" PRIu64 ")", m_device_id);
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

void CKWHStats::MakeJSONStats(Json::Value &root)
{
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
}

bool CKWHStats::SaveToDB()
{
	if (!m_bDirty)
		return false; // nothing changed, no need to save

	m_bDirty = false;

	Json::Value root;
	MakeJSONStats(root);

	std::string out = JSonToRawString(root);

	auto result = m_sql.safe_query("SELECT ID FROM KWHStats WHERE (DeviceRowID==%" PRIu64 ")", m_device_id);
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
	const int wday = last_hour.tm_wday;

	char szStartTime[32];
	if (strftime(szStartTime, sizeof(szStartTime), "%Y-%m-%d %H:%M:%S", &last_hour) == 0) {
    	// fallback (shouldn't normally happen, but keep a sensible default)
    	strncpy(szStartTime, "1970-01-01 00:00:00", sizeof(szStartTime));
    	szStartTime[sizeof(szStartTime) - 1] = '\0';
	}

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

			g_kwhstats[device_id].AddHourValue(hour, wday, Wh);

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
		// Use FIRST/LAST instead of MIN/MAX to correctly handle counter resets:
		// if a meter was replaced mid-hour, MIN/MAX would produce a huge spurious difference.
		// With FIRST/LAST, a reset (last < first) is detected and treated as zero consumption.
		auto result2 = m_sql.safe_query(
			"SELECT "
			"(SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 ") AND ([Date] > '%q') ORDER BY [Date] ASC, rowid ASC LIMIT 1),"
			"(SELECT Value FROM Meter WHERE (DeviceRowID==%" PRIu64 ") AND ([Date] > '%q') ORDER BY [Date] DESC, rowid DESC LIMIT 1)",
			device_id, szStartTime, device_id, szStartTime);
		if (!result2.empty() && !result2[0][0].empty() && !result2[0][1].empty())
		{
			std::unique_lock<std::mutex> lock(m_task_mutex);

			if (g_kwhstats.find(device_id) == g_kwhstats.end())
			{
				//First time we see this device, create the object
				CKWHStats kwhs;
				kwhs.Init(device_id);
				g_kwhstats[device_id] = kwhs;
			}

			const int64_t firstUsage = std::stoll(result2[0][0]);
			const int64_t lastUsage = std::stoll(result2[0][1]);

			// If last < first, a counter reset occurred this hour — treat as zero consumption
			const int64_t actUsage = (lastUsage >= firstUsage) ? (lastUsage - firstUsage) : 0;

			const int Wh = static_cast<int>(actUsage);

			g_kwhstats[device_id].AddHourValue(hour, wday, Wh);

			if (actHour == 0)
			{
				// we just passed midnight, finish the day
				g_kwhstats[device_id].FinishDay();
			}
		}
	}

}
