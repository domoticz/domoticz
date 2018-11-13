#pragma once

#include <deque>

struct _tTrendCalculator
{
public:
	enum _eTendencyType
	{
		TENDENCY_UNKNOWN = 0,
		TENDENCY_STABLE,
		TENDENCY_UP,
		TENDENCY_DOWN
	};
	enum _eTrendAverageTimes
	{
		TAVERAGE_TEMP = 30,
		TAVERAGE_HUM = 30,
		TAVERAGE_BARO = 30,
		TAVERAGE_USAGE = 15,
		TAVERAGE_RAIN = 10,
	};

	_tTrendCalculator();

	void Init();
	_eTendencyType AddValueAndReturnTendency(const double Value, const _eTrendAverageTimes TendType);
	_eTendencyType m_state;
private:
	double m_lastValue;
	time_t m_timeLastAvarage;
	int m_totValues;
	double m_calcValue;
};
