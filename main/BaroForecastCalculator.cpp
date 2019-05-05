#include "stdafx.h"
#include "BaroForecastCalculator.h"
#include "../hardware/hardwaretypes.h"
#include "localtime_r.h"

//http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf

CBaroForecastCalculator::CBaroForecastCalculator()
{
	Init();
}

CBaroForecastCalculator::~CBaroForecastCalculator()
{
}

void CBaroForecastCalculator::Init()
{
	m_baro_minuteCount = 0;
	m_dP_dt = 0; // Pressure delta over time
	m_last_forecast = wsbaroforcast_unknown;
	mytime(&m_BaroCalcLastTime);
	for (int ii = 0; ii < 9; ii++)
	{
		for (int jj = 0; jj < 6; jj++)
		{
			m_pressureSamples[ii][jj] = 0;
		}
		m_pressureAvg[ii] = 0;
	}
}

//Should be called every minute
int CBaroForecastCalculator::CalculateBaroForecast(const double pressure)
{
	//From 0 to 5 min.
	if (m_baro_minuteCount <= 5) {
		m_pressureSamples[0][m_baro_minuteCount] = pressure;
	}
	//From 30 to 35 min.
	else if ((m_baro_minuteCount >= 30) && (m_baro_minuteCount <= 35)) {
		m_pressureSamples[1][m_baro_minuteCount - 30] = pressure;
	}
	//From 60 to 65 min.
	else if ((m_baro_minuteCount >= 60) && (m_baro_minuteCount <= 65)) {
		m_pressureSamples[2][m_baro_minuteCount - 60] = pressure;
	}
	//From 90 to 95 min.
	else if ((m_baro_minuteCount >= 90) && (m_baro_minuteCount <= 95)) {
		m_pressureSamples[3][m_baro_minuteCount - 90] = pressure;
	}
	//From 120 to 125 min.
	else if ((m_baro_minuteCount >= 120) && (m_baro_minuteCount <= 125)) {
		m_pressureSamples[4][m_baro_minuteCount - 120] = pressure;
	}
	//From 150 to 155 min.
	else if ((m_baro_minuteCount >= 150) && (m_baro_minuteCount <= 155)) {
		m_pressureSamples[5][m_baro_minuteCount - 150] = pressure;
	}
	//From 180 to 185 min.
	else if ((m_baro_minuteCount >= 180) && (m_baro_minuteCount <= 185)) {
		m_pressureSamples[6][m_baro_minuteCount - 180] = pressure;
	}
	//From 210 to 215 min.
	else if ((m_baro_minuteCount >= 210) && (m_baro_minuteCount <= 215)) {
		m_pressureSamples[7][m_baro_minuteCount - 210] = pressure;
	}
	//From 240 to 245 min.
	else if ((m_baro_minuteCount >= 240) && (m_baro_minuteCount <= 245)) {
		m_pressureSamples[8][m_baro_minuteCount - 240] = pressure;
	}


	if (m_baro_minuteCount == 5) {
		// Avg pressure in first 5 min, value averaged from 0 to 5 min.
		m_pressureAvg[0] = ((m_pressureSamples[0][0] + m_pressureSamples[0][1]
			+ m_pressureSamples[0][2] + m_pressureSamples[0][3]
			+ m_pressureSamples[0][4] + m_pressureSamples[0][5]) / 6);
	}
	else if (m_baro_minuteCount == 35) {
		// Avg pressure in 30 min, value averaged from 0 to 5 min.
		m_pressureAvg[1] = ((m_pressureSamples[1][0] + m_pressureSamples[1][1]
			+ m_pressureSamples[1][2] + m_pressureSamples[1][3]
			+ m_pressureSamples[1][4] + m_pressureSamples[1][5]) / 6);
		double change = (m_pressureAvg[1] - m_pressureAvg[0]);
		m_dP_dt = change / 5;
	}
	else if (m_baro_minuteCount == 65) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[2] = ((m_pressureSamples[2][0] + m_pressureSamples[2][1]
			+ m_pressureSamples[2][2] + m_pressureSamples[2][3]
			+ m_pressureSamples[2][4] + m_pressureSamples[2][5]) / 6);
		double change = (m_pressureAvg[2] - m_pressureAvg[0]);
		m_dP_dt = change / 10;
	}
	else if (m_baro_minuteCount == 95) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[3] = ((m_pressureSamples[3][0] + m_pressureSamples[3][1]
			+ m_pressureSamples[3][2] + m_pressureSamples[3][3]
			+ m_pressureSamples[3][4] + m_pressureSamples[3][5]) / 6);
		double change = (m_pressureAvg[3] - m_pressureAvg[0]);
		m_dP_dt = change / 15;
	}
	else if (m_baro_minuteCount == 125) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[4] = ((m_pressureSamples[4][0] + m_pressureSamples[4][1]
			+ m_pressureSamples[4][2] + m_pressureSamples[4][3]
			+ m_pressureSamples[4][4] + m_pressureSamples[4][5]) / 6);
		double change = (m_pressureAvg[4] - m_pressureAvg[0]);
		m_dP_dt = change / 20;
	}
	else if (m_baro_minuteCount == 155) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[5] = ((m_pressureSamples[5][0] + m_pressureSamples[5][1]
			+ m_pressureSamples[5][2] + m_pressureSamples[5][3]
			+ m_pressureSamples[5][4] + m_pressureSamples[5][5]) / 6);
		double change = (m_pressureAvg[5] - m_pressureAvg[0]);
		m_dP_dt = change / 25;
	}
	else if (m_baro_minuteCount == 185) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[6] = ((m_pressureSamples[6][0] + m_pressureSamples[6][1]
			+ m_pressureSamples[6][2] + m_pressureSamples[6][3]
			+ m_pressureSamples[6][4] + m_pressureSamples[6][5]) / 6);
		double change = (m_pressureAvg[6] - m_pressureAvg[0]);
		m_dP_dt = change / 30;
	}
	else if (m_baro_minuteCount == 215) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[7] = ((m_pressureSamples[7][0] + m_pressureSamples[7][1]
			+ m_pressureSamples[7][2] + m_pressureSamples[7][3]
			+ m_pressureSamples[7][4] + m_pressureSamples[7][5]) / 6);
		double change = (m_pressureAvg[7] - m_pressureAvg[0]);
		m_dP_dt = change / 35;
	}
	else if (m_baro_minuteCount == 245) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		m_pressureAvg[8] = ((m_pressureSamples[8][0] + m_pressureSamples[8][1]
			+ m_pressureSamples[8][2] + m_pressureSamples[8][3]
			+ m_pressureSamples[8][4] + m_pressureSamples[8][5]) / 6);
		double change = (m_pressureAvg[8] - m_pressureAvg[0]);
		m_dP_dt = change / 40; // note this is for t = 4 hour

		m_baro_minuteCount -= 30;
		m_pressureAvg[0] = m_pressureAvg[1];
		m_pressureAvg[1] = m_pressureAvg[2];
		m_pressureAvg[2] = m_pressureAvg[3];
		m_pressureAvg[3] = m_pressureAvg[4];
		m_pressureAvg[4] = m_pressureAvg[5];
		m_pressureAvg[5] = m_pressureAvg[6];
		m_pressureAvg[6] = m_pressureAvg[7];
		m_pressureAvg[7] = m_pressureAvg[8];
	}

	m_baro_minuteCount++;

	if (m_baro_minuteCount < 36) //if time is less than 35 min
		return wsbaroforcast_unknown; // Unknown, more time needed
	else if (m_dP_dt < (-0.25))
		return wsbaroforcast_heavy_rain; // Quickly falling LP, Thunderstorm, not stable
	else if (m_dP_dt > 0.25)
		return wsbaroforcast_unstable; // Quickly rising HP, not stable weather
	else if ((m_dP_dt > (-0.25)) && (m_dP_dt < (-0.05)))
		return wsbaroforcast_rain; // Slowly falling Low Pressure System, stable rainy weather
	else if ((m_dP_dt > 0.05) && (m_dP_dt < 0.25))
		return wsbaroforcast_sunny; // Slowly rising HP stable good weather
	else if ((m_dP_dt > (-0.05)) && (m_dP_dt < 0.05))
		return wsbaroforcast_stable; // Stable weather
	else
	{
		if (pressure <= 980)
			return  wsbaroforcast_heavy_rain;
		else if (pressure <= 995)
			return wsbaroforcast_rain;
		else if (pressure >= 1029)
			return wsbaroforcast_sunny;
	}
	return wsbaroforcast_unknown;
}

int CBaroForecastCalculator::CalculateBaroForecast(const float temp, const double pressure)
{
	int forecast = CalculateBaroForecast(pressure);
	if (temp < 0)
	{
		if (
			(forecast == wsbaroforcast_rain) ||
			(forecast == wsbaroforcast_heavy_rain)
			)
		{
			forecast = wsbaroforcast_snow;
		}
	}
	return forecast;
}
