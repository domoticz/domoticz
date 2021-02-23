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
	std::array<std::array<double, 6>, 9> m_pressureSamples;
	std::array<double, 9> m_pressureAvg;
	double m_dP_dt = 0;
	int m_last_forecast = 0x07; //wsbaroforecast_unknown
	time_t m_BaroCalcLastTime = 0;
};

