//
#pragma once

#include <vector>

struct SValueCheckerItem
{
	time_t mTimestamp = 0;
	double mValue = 0.00;

	bool operator<(const SValueCheckerItem& other)
	{
		return mValue < other.mValue;
	};
};


class CValueChecker
{
public:
	CValueChecker();
	
	bool ValidateValue(const double& value, const double& maxRate, const std::string& LogName);
	void ResetToThisValue(const double& value, time_t timestamp);

protected:
	SValueCheckerItem& FindOldest();
	bool AddValue(const double& value, const std::string& LogName);

private:
	std::vector<SValueCheckerItem> mValues;
	const int32_t mMedianPos = 3;
	const int32_t mHistorySize = (mMedianPos*2)-1;
	int32_t mHistoryFilled = 0;
	SValueCheckerItem mLastEntry;
};
