#include "stdafx.h"
#include "WindCalculation.h"
#include <time.h>
#include "../main/localtime_r.h"
#include <string.h>
#include <math.h>

#define WIND_HISTORY_MINUTES 10
#define WIND_DEGREE_RESOLUTION 5
#define WIND_DEGREE_TABLE_COUNT 360/WIND_DEGREE_RESOLUTION

#define myround(a) ( int ) ( a + .5 )

#define RAD 57.2957795

_tWindCalculationStruct::_tWindCalculationStruct()
{
	Init();
}

void _tWindCalculationStruct::Init()
{
	//clear buffer
	memset(&m_minute_counter,0,sizeof(m_minute_counter));
	m_FirstMeasureTime=mytime(NULL);
	m_history_fifo.clear();
	m_bHaveLastDirection=false;
	m_last_direction = 0;

	m_MaxSpeed = -1;
	m_MaxGust = -1;
	m_MinSpeed = -1;
	m_MinGust = -1;
}

double _tWindCalculationStruct::AddValueAndReturnAvarage(double degree)
{
	double dirresult=degree;
	if (m_bHaveLastDirection)
	{
		double x, y, cosx, cosy, sinx, siny;
		x = m_last_direction;
		y = degree;
		cosx = cos(x/RAD);
		cosy = cos(y/RAD);
		sinx = sin(x/RAD);
		siny = sin(y/RAD);
		dirresult = (atan2 ((sinx + siny),(cosx + cosy)))*RAD;
	}
	m_last_direction=degree;
	m_bHaveLastDirection=true;
	if (dirresult<0)
		dirresult+=360;
	return dirresult;
/*
	time_t atime=mytime(NULL);
	long tot_minutes=(long)((atime-m_FirstMeasureTime)/60);
	if (tot_minutes<0)
	{
		Init();
	}
	if (tot_minutes>WIND_HISTORY_MINUTES)
		tot_minutes=WIND_HISTORY_MINUTES;
	if (tot_minutes==WIND_HISTORY_MINUTES)
	{
		//we are in our last minute tab, remove first item from the fifo
		if (m_history_fifo.size()>0)
		{
			unsigned char degree_pos=m_history_fifo.front();
			m_history_fifo.pop_front();
			//sanity check
			if (m_minute_counter[degree_pos]>0)
				m_minute_counter[degree_pos]--;
		}
	}
	int iDegree=myround(degree);
	if (iDegree==360)
		iDegree=0;
	int degree_pos = iDegree/WIND_DEGREE_RESOLUTION;
	m_minute_counter[degree_pos]++;

	m_history_fifo.push_back(degree_pos);

	return CalculateAvarage();
*/
}

double _tWindCalculationStruct::CalculateAvarage()
{
	int highpos=0;
	int highestval=0;
	int ii;
	for (ii=0; ii<WIND_DEGREE_TABLE_COUNT; ii++)
	{
		if (m_minute_counter[ii]>highestval)
		{
			highestval=m_minute_counter[ii];
			highpos=ii;
		}
	}
	return highpos*WIND_DEGREE_RESOLUTION;
}

void _tWindCalculationStruct::SetSpeedGust(const int Speed, const int Gust)
{
	if ((Speed > m_MaxSpeed) || (m_MaxSpeed == -1))
	{
		m_MaxSpeed = Speed;
	}
	if ((Gust > m_MaxGust) || (m_MaxGust == -1))
	{
		m_MaxGust = Gust;
	}
	if ((Speed < m_MinSpeed) || (m_MinSpeed == -1))
	{
		m_MinSpeed = Speed;
	}
	if ((Gust < m_MinGust) || (m_MinGust == -1))
	{
		m_MinGust = Gust;
	}
}

//Get min/max and reset
void _tWindCalculationStruct::GetMMSpeedGust(int &MinSpeed, int &MaxSpeed, int &MinGust, int &MaxGust)
{
	MinSpeed = m_MinSpeed;
	MaxSpeed = m_MaxSpeed;
	MinGust = m_MinGust;
	MaxGust = m_MaxGust;
	m_MaxSpeed = -1;
	m_MaxGust = -1;
	m_MinSpeed = -1;
	m_MinGust = -1;
}
