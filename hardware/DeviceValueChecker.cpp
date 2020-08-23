//
#include "stdafx.h"
#include "DeviceValueChecker.h"

#include <algorithm>
#include <string>

CValueChecker::CValueChecker()
{
	mValues.resize(mHistorySize);
}

void CValueChecker::ResetToThisValue(const double& value, time_t timestamp)
{
	for (auto& it : mValues)
	{
		it.mTimestamp = timestamp;
		it.mValue = value;
	}
	mHistoryFilled = mHistorySize;
}

bool CValueChecker::ValidateValue(const double& value, const double& maxRate, const std::string& LogName)
{
	if (false == AddValue(value, LogName))
		return false;

	auto& median = mValues[mMedianPos];

	if (mLastEntry.mTimestamp == median.mTimestamp)
		return true;

	double deltaT = static_cast<double>(mLastEntry.mTimestamp - median.mTimestamp);
	double deltaV = mLastEntry.mValue - median.mValue;
	double rate = (3600 * deltaV) / deltaT;

	if (abs(rate) > maxRate)
	{
		_log.Log(LOG_ERROR, "Rejected value. Rate is above maximum.  rate:%f   maxRate:%f (%s)", rate, maxRate, LogName.c_str());
		return false;
	}

	return true;
}


SValueCheckerItem& CValueChecker::FindOldest()
{
	std::vector<SValueCheckerItem>::iterator oldest = mValues.begin();

	for (std::vector<SValueCheckerItem>::iterator it = mValues.begin(); it != mValues.end(); it++)
	{
		if (it->mTimestamp < oldest->mTimestamp)
		{
			oldest = it;
		}
	}

	return *oldest;
}


bool CValueChecker::AddValue(const double& value, const std::string& LogName)
{
	time_t currentTimestamp = mytime(nullptr);

	if (currentTimestamp < mLastEntry.mTimestamp + 5)
	{
		//_log.Log(LOG_ERROR, "Rejected value. Received a value withing 5 seconds of previous.");
		return false;
	}

	auto& replaced = FindOldest();
	bool fullInitilized = true;
	if (mHistoryFilled < mHistorySize)
	{
		mHistoryFilled++;
		if (mHistoryFilled < mHistorySize)
		{
			_log.Log(LOG_ERROR, "Rejected value. Checker not fully initialized.  %d / %d (%s)", mHistoryFilled, mHistorySize, LogName.c_str());
			fullInitilized = false;
		}
	}

	replaced.mTimestamp = currentTimestamp;
	replaced.mValue = value;

	mLastEntry = replaced;

	std::sort(mValues.begin(), mValues.end());
	return fullInitilized;
}
