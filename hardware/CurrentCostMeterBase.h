#pragma once

#include "DomoticzHardware.h"

class CurrentCostMeterBase : public CDomoticzHardwareBase
{
public:
	CurrentCostMeterBase(void);
	virtual ~CurrentCostMeterBase(void);

protected:
	void ParseData(const char *pData, int Len);
	void Init();

private:
	void ExtractReadings();
	bool ExtractNumberBetweenStrings(const char *startString, const char *endString, float *pResult);
	std::string m_buffer;
	unsigned int m_tempuratureCounter;
};

