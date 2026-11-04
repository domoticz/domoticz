#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace OpenWebNetEnergy
{
// Prefer a valid legacy reading, including one arriving after a probe timeout.
// Once legacy is confirmed, a failed poll must not switch it back to 72#1.
class Source
{
public:
	bool IsUnknown() const { return m_source == 0; }
	bool UsesFallback() const { return m_source == 2; }
	bool AcceptLegacy()
	{
		m_source = 1;
		return true;
	}
	void EnableFallback()
	{
		if (IsUnknown())
			m_source = 2;
	}
private:
	int m_source = 0;
};

inline bool ParseUnsigned(const std::string &text, uint64_t &value)
{
	if (text.empty())
		return false;
	value = 0;
	for (const char ch : text)
	{
		if (ch < '0' || ch > '9' || value > (std::numeric_limits<uint64_t>::max() - (ch - '0')) / 10)
			return false;
		value = value * 10 + (ch - '0');
	}
	return true;
}

struct Totalizer
{
	uint64_t raw = 0;
	uint64_t total = 0;
	uint64_t reset = 0;
};

// WHO 18, dimension 72#1: Wh, day, month, year, hour, minute.
inline bool ReadTotalizer(const std::vector<std::string> &values, Totalizer &sample)
{
	if (values.size() != 6)
		return false;
	uint64_t fields[6];
	for (size_t i = 0; i < 6; ++i)
		if (!ParseUnsigned(values[i], fields[i]))
			return false;
	if (fields[2] > 12 || fields[3] > 9999 || fields[4] > 23 || fields[5] > 59)
		return false;
	const uint64_t year = fields[3];
	const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
	static const uint64_t monthDays[13] = { 31, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	const uint64_t maxDay = (fields[2] == 2 && leap) ? 29 : monthDays[fields[2]];
	if (fields[1] > maxDay)
		return false;
	sample.raw = sample.total = fields[0];
	sample.reset = ((((fields[3] * 13 + fields[2]) * 32 + fields[1]) * 24 + fields[4]) * 60 + fields[5]);
	return true;
}

inline bool AdvanceTotalizer(const Totalizer &previous, Totalizer &sample)
{
	// Ignore delayed packets from before a reset, and regressing readings
	// with an unchanged reset date. Neither should create consumption.
	if (sample.reset < previous.reset || (sample.reset == previous.reset && sample.raw < previous.raw))
		return false;
	const uint64_t delta = sample.reset == previous.reset ? sample.raw - previous.raw : sample.raw;
	if (previous.total > std::numeric_limits<uint64_t>::max() - delta)
		return false;
	sample.total = previous.total + delta;
	return true;
}
}
