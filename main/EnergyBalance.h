#pragma once

#include <string>
#include <vector>

// Where the house consumption of one short log interval came from, in kWh
struct _tEnergyBalanceInterval
{
	std::string Date;		// "YYYY-MM-DD HH:MM", the end of the interval
	double Solar = 0;		// solar used directly
	double BatterySolar = 0;	// battery energy that was charged from solar
	double BatteryGrid = 0;		// battery energy that was charged from the grid
	double Grid = 0;		// grid energy used directly
};

// Today's energy flows of the devices selected in the Energy Dashboard settings, in kWh
struct _tEnergyBalance
{
	std::string Method;		// "5min" (short log intervals), "daily" (today's counters) or "none" (no grid meter selected)
	int Intervals = 0;
	std::string Day;		// "YYYY-MM-DD" the flows belong to
	std::string OldestDay;		// first day with short log data of the grid meter

	double Import = 0;
	double Export = 0;
	double Solar = 0;
	double BatteryIn = 0;
	double BatteryOut = 0;
	double House = 0;

	double SolarToHouse = 0;
	double BatteryToHouse = 0;
	double GridToHouse = 0;
	double SolarToBattery = 0;
	double GridToBattery = 0;
	double SolarToGrid = 0;
	double BatteryToGrid = 0;

	double SelfSufficiency = -1;	// percentage of the house consumption that did not come from the grid, -1 when unknown

	std::vector<_tEnergyBalanceInterval> Series;	// per interval, only with Method "5min"
};

// szDay is "YYYY-MM-DD" of a day still in the short log, empty for today
bool CalculateEnergyBalanceToday(_tEnergyBalance &eb, const std::string &szDay = "");
