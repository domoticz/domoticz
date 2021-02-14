#pragma once
class CBaroForecastCalculator
{
public:
	CBaroForecastCalculator();
	~CBaroForecastCalculator() = default;
	void Init();
	int CalculateBaroForecast(double pressure);
	int CalculateBaroForecast(float temp, double pressure);

      private:
	int m_baro_minuteCount = 0;
	double m_pressureSamples[9][6];
	double m_pressureAvg[9];
	double m_dP_dt = 0;
	int m_last_forecast = 0x07; //wsbaroforecast_unknown
	time_t m_BaroCalcLastTime = 0;
};

