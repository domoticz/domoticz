#pragma once

#include <deque>

//We use a resolution of 5 degrees (360/5 = 72)
struct _tWindCalculator
{
	int m_minute_counter[100];//WIND_DEGREE_TABLE_COUNT
	std::deque<unsigned char> m_history_fifo;
	time_t m_FirstMeasureTime;

	double m_last_direction;
	bool m_bHaveLastDirection;

	int m_MaxSpeed;
	int m_MaxGust;

	int m_MinSpeed;
	int m_MinGust;

	//constructor
	_tWindCalculator();

	void Init();
	double AddValueAndReturnAvarage(const double degree);
	void SetSpeedGust(const int Speed, const int Gust);
	void GetMMSpeedGust(int &MinSpeed, int &MaxSpeed, int &MinGust, int &MaxGust);
	double CalculateAvarage();
};
