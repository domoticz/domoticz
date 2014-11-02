#pragma once

#include <deque>

//We use a resolution of 5 degrees (360/5 = 72)
struct _tWindCalculationStruct
{
	int m_minute_counter[100];//WIND_DEGREE_TABLE_COUNT
	std::deque<unsigned char> m_history_fifo;
	time_t m_FirstMeasureTime;

	double m_last_direction;
	bool m_bHaveLastDirection;

	//constructor
	_tWindCalculationStruct();

	void Init();
	double AddValueAndReturnAvarage(double degree);
	double CalculateAvarage();
};
