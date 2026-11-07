/*
 * EnergyBalance.cpp
 *
 * The energy flows of a day (today by default) between grid, solar, battery and house for the devices selected in the Energy Dashboard
 * settings, and the self-sufficiency derived from them.
 *
 * The daily totals of the meters cannot tell whether the battery was charged from solar or from the grid, so the
 * flows are split per short log interval (normally 5 minutes): solar goes to the house first, then to the battery,
 * then to the grid; the house takes battery energy before grid energy. The battery keeps track of how much of the
 * energy it holds came from the grid, and energy it later delivers to the house counts as grid energy for that part.
 * Intervals whose balance is negative because one counter lags behind the others are merged with the following ones.
 * Battery charge that no counter explains counts as solar to battery, so the solar flows can add up to a little more
 * than the solar counter on such a day.
 */

#include "stdafx.h"
#include "EnergyBalance.h"

#include "Helper.h"
#include "SQLHelper.h"
#include "WebServerHelper.h"
#include "json_helper.h"
#include "localtime_r.h"
#include "mainworker.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <algorithm>
#include <map>
#include <mutex>

extern http::server::CWebServerHelper m_webservers;

namespace
{
	// A step larger than this within one interval is a counter reset or replacement, not energy
	constexpr double MAX_INTERVAL_KWH = 50.0;
	constexpr int CACHE_SECONDS = 60;

	struct _tCacheSlot
	{
		time_t time = 0;
		std::string day;
		_tEnergyBalance eb;
	};
	std::mutex g_cacheMutex;
	_tCacheSlot g_cache[2];

	// Cumulative counter in kWh per interval key ("YYYY-MM-DD HH:MM", floored to the short log interval)
	typedef std::map<std::string, double> _tCounterRows;

	std::string IntervalKey(const std::string &sDate, const int interval)
	{
		if (sDate.size() < 16)
			return "";
		int minute = atoi(sDate.substr(14, 2).c_str());
		if (interval > 1)
			minute -= minute % interval;
		return sDate.substr(0, 14) + std_format("%02d", minute);
	}

	bool GetDeviceType(const int idx, unsigned char &dType, unsigned char &dSubType, _eMeterType &metertype, float &divider)
	{
		auto result = m_sql.safe_query("SELECT Type, SubType, SwitchType, AddjValue2 FROM DeviceStatus WHERE (ID == %d)", idx);
		if (result.empty())
			return false;
		dType = static_cast<unsigned char>(atoi(result[0][0].c_str()));
		dSubType = static_cast<unsigned char>(atoi(result[0][1].c_str()));
		metertype = static_cast<_eMeterType>(atoi(result[0][2].c_str()));
		if ((dType == pTypeP1Power) || (dType == pTypeENERGY) || (dType == pTypePOWER) || (dType == pTypeCURRENTENERGY)
			|| ((dType == pTypeGeneral) && (dSubType == sTypeKwh)))
			metertype = MTYPE_ENERGY;
		divider = m_sql.GetCounterDivider(int(metertype), int(dType), static_cast<float>(atof(result[0][3].c_str())));
		if (divider == 0)
			divider = 1000.0F;
		return true;
	}

	// Reads the short log of an energy counter since szStart. For a P1 meter bDelivery selects the returned
	// energy instead of the used energy.
	void ReadCounterRows(const int idx, const bool bDelivery, const std::string &szStart, const std::string &szEnd, const int interval, _tCounterRows &rows)
	{
		unsigned char dType, dSubType;
		_eMeterType metertype;
		float divider;
		if (!GetDeviceType(idx, dType, dSubType, metertype, divider))
			return;

		std::vector<std::vector<std::string>> result;
		if (dType == pTypeP1Power)
		{
			if (bDelivery)
				result = m_sql.safe_query("SELECT Value2 + Value6, Date FROM MultiMeter WHERE (DeviceRowID == %d) AND (Date >= '%q') AND (Date < '%q') ORDER BY Date ASC", idx, szStart.c_str(), szEnd.c_str());
			else
				result = m_sql.safe_query("SELECT Value1 + Value5, Date FROM MultiMeter WHERE (DeviceRowID == %d) AND (Date >= '%q') AND (Date < '%q') ORDER BY Date ASC", idx, szStart.c_str(), szEnd.c_str());
			divider = 1000.0F;
		}
		else
			result = m_sql.safe_query("SELECT Value, Date FROM Meter WHERE (DeviceRowID == %d) AND (Date >= '%q') AND (Date < '%q') ORDER BY Date ASC", idx, szStart.c_str(), szEnd.c_str());

		for (const auto &sd : result)
		{
			std::string key = IntervalKey(sd[1], interval);
			if (!key.empty())
				rows[key] = atof(sd[0].c_str()) / divider;
		}
	}

	// Today's counter of a device as the device list shows it (CounterToday, or CounterDelivToday for the returned energy)
	double GetCounterToday(const int idx, const bool bDelivery)
	{
		Json::Value devices;
		m_webservers.GetJSonDevices(devices, "", "", "", std::to_string(idx), "", "", false, true, false, 0, "", "");
		if (!devices.isMember("result") || devices["result"].empty())
			return 0;
		const Json::Value &dev = devices["result"][0];
		const char *szField = bDelivery ? "CounterDelivToday" : "CounterToday";
		if (!dev.isMember(szField))
			return 0;
		return std::max(0.0, atof(dev[szField].asString().c_str()));
	}

	void CalculateDaily(_tEnergyBalance &eb, const int idP1, const int idSolar, const int idBatIn, const int idBatOut)
	{
		eb.Method = "daily";
		eb.Import = GetCounterToday(idP1, false);
		eb.Export = GetCounterToday(idP1, true);
		eb.Solar = (idSolar != -1) ? GetCounterToday(idSolar, false) : 0;
		eb.BatteryIn = (idBatIn != -1) ? GetCounterToday(idBatIn, false) : 0;
		eb.BatteryOut = (idBatOut != -1) ? GetCounterToday(idBatOut, false) : 0;
		eb.House = std::max(0.0, eb.Import + eb.Solar - eb.Export - eb.BatteryIn + eb.BatteryOut);
		if (eb.House > 0)
			eb.SelfSufficiency = std::min(100.0, std::max(0.0, (1.0 - eb.Import / eb.House) * 100.0));
	}

	std::string FormatDateTime(const time_t t)
	{
		struct tm lt;
		localtime_r(&t, &lt);
		return std_format("%04d-%02d-%02d %02d:%02d:%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
	}

	// Local midnight of the day that is iDays away from the day of tBase
	time_t MidnightOffset(const time_t tBase, const int iDays)
	{
		struct tm lt;
		localtime_r(&tBase, &lt);
		lt.tm_hour = 0;
		lt.tm_min = 0;
		lt.tm_sec = 0;
		lt.tm_mday += iDays;
		lt.tm_isdst = -1;
		return mktime(&lt);
	}

	// szDay is "YYYY-MM-DD", or empty for today
	void Calculate(_tEnergyBalance &eb, const std::string &szDay)
	{
		std::string szESettings;
		Json::Value esettings;
		if (!m_sql.GetPreferencesVar("ESettings", szESettings) || !ParseJSon(szESettings, esettings))
		{
			eb.Method = "none";
			return;
		}
		auto getIdx = [&esettings](const char *szName) {
			return (esettings.isMember(szName) && esettings[szName].isInt() && esettings[szName].asInt() > 0) ? esettings[szName].asInt() : -1;
		};
		const int idP1 = getIdx("idP1");
		const int idSolar = getIdx("idSolar");
		const int idBatIn = getIdx("idBatteryEnergyIn");
		const int idBatOut = getIdx("idBatteryEnergyOut");
		if (idP1 == -1)
		{
			eb.Method = "none";
			return;
		}

		const int interval = std::max(1, m_sql.m_ShortLogInterval);
		time_t now = mytime(nullptr);
		time_t tDay = MidnightOffset(now, 0);
		bool bToday = true;
		if (!szDay.empty())
		{
			// strictly "YYYY-MM-DD"
			bool bValid = (szDay.size() == 10) && (szDay[4] == '-') && (szDay[7] == '-');
			for (size_t i = 0; bValid && (i < szDay.size()); i++)
				if ((i != 4) && (i != 7) && !isdigit(static_cast<unsigned char>(szDay[i])))
					bValid = false;
			if (!bValid)
			{
				eb.Method = "none";
				return;
			}
			const int y = atoi(szDay.substr(0, 4).c_str());
			const int m = atoi(szDay.substr(5, 2).c_str());
			const int d = atoi(szDay.substr(8, 2).c_str());
			struct tm lt;
			localtime_r(&now, &lt);
			lt.tm_year = y - 1900;
			lt.tm_mon = m - 1;
			lt.tm_mday = d;
			lt.tm_hour = lt.tm_min = lt.tm_sec = 0;
			lt.tm_isdst = -1;
			const time_t tRequested = mktime(&lt);
			// mktime normalizes a date that does not exist (2026-02-31 becomes March 3), refuse it instead
			if ((tRequested == static_cast<time_t>(-1)) || (lt.tm_year != y - 1900) || (lt.tm_mon != m - 1) || (lt.tm_mday != d) || (tRequested > tDay))
			{
				eb.Method = "none";
				return;
			}
			bToday = (tRequested == tDay);
			tDay = tRequested;
		}
		const std::string szToday = FormatDateTime(tDay).substr(0, 16);
		eb.Day = szToday.substr(0, 10);
		// The day before warms up the share of grid energy in the battery
		const std::string szStart = FormatDateTime(MidnightOffset(tDay, -1));
		// The interval that ends at midnight belongs to the day that just ended
		const time_t tNext = MidnightOffset(tDay, 1);
		const std::string szNext = FormatDateTime(tNext).substr(0, 16);
		const std::string szEnd = FormatDateTime(tNext + interval * 60);

		// A kWh meter can be the grid meter too (import only, like the Energy Dashboard shows it); only a P1 meter
		// has a separate return counter. Reading the kWh counter as export as well cancels every import.
		bool bGridIsP1 = false;
		{
			unsigned char dType, dSubType;
			_eMeterType metertype;
			float divider;
			bGridIsP1 = GetDeviceType(idP1, dType, dSubType, metertype, divider) && (dType == pTypeP1Power);
		}

		_tCounterRows rowsImport, rowsExport, rowsSolar, rowsBatIn, rowsBatOut;
		ReadCounterRows(idP1, false, szStart, szEnd, interval, rowsImport);
		if (bGridIsP1)
			ReadCounterRows(idP1, true, szStart, szEnd, interval, rowsExport);
		if (idSolar != -1)
			ReadCounterRows(idSolar, true, szStart, szEnd, interval, rowsSolar);
		if (idBatIn != -1)
			ReadCounterRows(idBatIn, false, szStart, szEnd, interval, rowsBatIn);
		if (idBatOut != -1)
			ReadCounterRows(idBatOut, false, szStart, szEnd, interval, rowsBatOut);

		// The first day with short log data of the grid meter, so the user can step back to it
		{
			unsigned char dType, dSubType;
			_eMeterType metertype;
			float divider;
			if (GetDeviceType(idP1, dType, dSubType, metertype, divider))
			{
				std::vector<std::vector<std::string>> oldest;
				if (dType == pTypeP1Power)
					oldest = m_sql.safe_query("SELECT MIN(Date) FROM MultiMeter WHERE (DeviceRowID == %d)", idP1);
				else
					oldest = m_sql.safe_query("SELECT MIN(Date) FROM Meter WHERE (DeviceRowID == %d)", idP1);
				if (!oldest.empty() && (oldest[0][0].size() >= 10))
					eb.OldestDay = oldest[0][0].substr(0, 10);
			}
		}

		if (rowsImport.empty() || (bToday && (rowsImport.rbegin()->first < szToday)))
		{
			if (bToday)
				CalculateDaily(eb, idP1, idSolar, idBatIn, idBatOut);
			else
				eb.Method = "5min";	// a past day without short log data
			return;
		}

		std::vector<std::string> keys;
		for (const auto *rows : { &rowsImport, &rowsExport, &rowsSolar, &rowsBatIn, &rowsBatOut })
			for (const auto &itt : *rows)
				keys.push_back(itt.first);
		std::sort(keys.begin(), keys.end());
		keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

		// Last known cumulative value per counter, the step since then is the energy of the interval
		struct _tCounterState
		{
			const _tCounterRows *rows;
			bool bHave = false;
			double last = 0;

			double Step(const std::string &key)
			{
				auto itt = rows->find(key);
				if (itt == rows->end())
					return 0;
				double step = 0;
				if (bHave)
				{
					step = itt->second - last;
					if ((step < 0) || (step > MAX_INTERVAL_KWH))
						step = 0;
				}
				bHave = true;
				last = itt->second;
				return step;
			}
		};
		_tCounterState cImport{ &rowsImport }, cExport{ &rowsExport }, cSolar{ &rowsSolar }, cBatIn{ &rowsBatIn }, cBatOut{ &rowsBatOut };

		double poolEnergy = 0;		// energy in the battery
		double poolGrid = 0;		// the part of it that came from the grid
		double gridToHouseTotal = 0;	// grid energy used by the house, directly or through the battery

		// The meters do not update at the same moment: a counter that stalls for a while and then catches up makes
		// the balance of an interval negative (the battery charges from energy no counter has reported yet) and the
		// catch-up interval too high. Such intervals are merged with the following ones until their sum is no longer
		// negative, and the merged energy is spread evenly over them. At most two hours are merged, so a counter that
		// stopped for good does not swallow the rest of the day, and never across midnight.
		const size_t maxMerge = static_cast<size_t>(std::max(1, 120 / interval));
		std::vector<std::string> pendingKeys;
		double pI = 0, pE = 0, pS = 0, pC = 0, pD = 0;

		for (size_t iKey = 0; iKey < keys.size(); iKey++)
		{
			const std::string &key = keys[iKey];
			pI += cImport.Step(key);
			pE += cExport.Step(key);
			pS += cSolar.Step(key);
			pC += cBatIn.Step(key);
			pD += cBatOut.Step(key);
			pendingKeys.push_back(key);

			const bool bLast = (iKey + 1 == keys.size());
			// the next interval belongs to another day (the interval that ends at midnight still belongs to the day before)
			const bool bDayEnd = !bLast && (((key <= szToday) && (keys[iKey + 1] > szToday)) || ((key <= szNext) && (keys[iKey + 1] > szNext)));
			if ((pI + pS + pD - pE - pC < 0) && (pendingKeys.size() < maxMerge) && !bLast && !bDayEnd)
				continue;

			const double I = pI, E = pE, S = pS, C = pC, D = pD;
			std::vector<std::string> mergedKeys;
			mergedKeys.swap(pendingKeys);
			pI = pE = pS = pC = pD = 0;

			const double used = std::max(0.0, I + S + D - E - C);
			const double s2h = std::min(S, used);
			const double b2h = std::min(D, used - s2h);
			const double g2h = used - s2h - b2h;
			const double sRest = S - s2h;
			const double s2b = std::min(sRest, C);
			// The grid cannot charge the battery with more than it delivered; charge that no counter explains (a
			// window that is still negative) is most likely unreported solar, so it does not count as grid energy
			const double g2b = std::min(C - s2b, std::max(0.0, I - g2h));
			const double own2b = C - s2b - g2b;
			const double s2g = sRest - s2b;
			const double b2g = D - b2h;

			// Only the part of the discharge the pool can explain carries its grid share; the rest was in the
			// battery before the warm-up started and counts as own energy
			const double share = (poolEnergy > 0) ? std::min(1.0, poolGrid / poolEnergy) : 0.0;
			const double fromPool = std::min(D, poolEnergy);
			const double b2hGrid = (D > 0) ? b2h * (fromPool / D) * share : 0.0;
			poolGrid = std::max(0.0, poolGrid - fromPool * share) + g2b;
			poolEnergy = std::max(0.0, poolEnergy - D) + s2b + g2b + own2b;

			// The interval that ends at midnight still belongs to yesterday
			if (key <= szToday)
				continue;
			if (key > szNext)
				break;

			eb.Intervals += static_cast<int>(mergedKeys.size());
			eb.Import += I;
			eb.Export += E;
			eb.Solar += S;
			eb.BatteryIn += C;
			eb.BatteryOut += D;
			eb.House += used;
			eb.SolarToHouse += s2h;
			eb.BatteryToHouse += b2h;
			eb.GridToHouse += g2h;
			eb.SolarToBattery += s2b + own2b;
			eb.GridToBattery += g2b;
			eb.SolarToGrid += s2g;
			eb.BatteryToGrid += b2g;
			gridToHouseTotal += g2h + b2hGrid;

			const double parts = static_cast<double>(mergedKeys.size());
			for (const auto &mkey : mergedKeys)
			{
				_tEnergyBalanceInterval point;
				point.Date = mkey;
				point.Solar = s2h / parts;
				point.BatterySolar = (b2h - b2hGrid) / parts;
				point.BatteryGrid = b2hGrid / parts;
				point.Grid = g2h / parts;
				eb.Series.push_back(point);
			}
		}

		eb.Method = "5min";
		if (eb.House > 0)
			eb.SelfSufficiency = std::min(100.0, std::max(0.0, (1.0 - gridToHouseTotal / eb.House) * 100.0));
	}
} // namespace

bool CalculateEnergyBalanceToday(_tEnergyBalance &eb, const std::string &szDay)
{
	std::lock_guard<std::mutex> l(g_cacheMutex);
	// Today (refreshed by the widgets) and the last other day looked at have their own slot, so browsing the
	// past days does not make every widget refresh recalculate today
	_tCacheSlot &slot = g_cache[szDay.empty() ? 0 : 1];
	time_t now = mytime(nullptr);
	if ((slot.time != 0) && (slot.day == szDay) && (now - slot.time < CACHE_SECONDS) && (now >= slot.time))
	{
		eb = slot.eb;
		return eb.Method != "none";
	}
	slot.eb = _tEnergyBalance();
	Calculate(slot.eb, szDay);
	slot.time = now;
	slot.day = szDay;
	eb = slot.eb;
	return eb.Method != "none";
}
