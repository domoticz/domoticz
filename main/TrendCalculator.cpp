#include "stdafx.h"
#include "TrendCalculator.h"

_tTrendCalculator::_tTrendCalculator()
{
	Init();
}

void _tTrendCalculator::Init()
{
	m_state = TENDENCY_UNKNOWN;
	m_lastValue = 0;
	m_timeLastAvarage=time(NULL);
	m_totValues=0;
	m_calcValue=0;
}

_tTrendCalculator::_eTendencyType _tTrendCalculator::AddValueAndReturnTendency(const double Value, const _eTrendAverageTimes TendType)
{
	m_calcValue += Value;
	m_totValues++;
	if (m_state == TENDENCY_UNKNOWN)
	{
		m_state = TENDENCY_STABLE;
		m_timeLastAvarage = time(NULL);
		m_lastValue = Value; //got to start somewhere
	}
	else
	{
		time_t atime = time(NULL);
		int AvarageMinutes = (int)TendType;
		if (atime - m_timeLastAvarage >= (AvarageMinutes * 60))
		{
			//Calculate Mean (M)
			double curAvr = (m_totValues == 0) ? Value : (m_calcValue / m_totValues);

			if (curAvr > m_lastValue)
				m_state = TENDENCY_UP;
			else if (curAvr < m_lastValue)
				m_state = TENDENCY_DOWN;
			else
				m_state = TENDENCY_STABLE;

			m_lastValue = curAvr;

			m_calcValue = 0;
			m_totValues = 0;
			m_timeLastAvarage = atime;
		}
	}
	return m_state;
}
