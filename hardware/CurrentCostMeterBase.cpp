#include "stdafx.h"
#include "CurrentCostMeterBase.h"

CurrentCostMeterBase::CurrentCostMeterBase(void)
{
	Init();
}


CurrentCostMeterBase::~CurrentCostMeterBase(void)
{
}

void CurrentCostMeterBase::Init()
{
	m_buffer = "";
}

// Extract readings from the xml messages posted by the
// CurrentCost meter as documented here
// http://www.currentcost.com/cc128/xml.htm
// a sample (expanded for readibility) message looks like
// <msg>                            start of message
//    <src>CC128-v0.11</src>        source and software version
//    <dsb>00089</dsb>              days since birth, ie days run
//    <time>13:02:39</time>         24 hour clock time as displayed
//    <tmpr>18.7</tmpr>             temperature as displayed
//    <sensor>1</sensor>            Appliance Number as displayed
//    <id>01234</id>                radio ID received from the sensor
//    <type>1</type>                sensor Type, "1" = electricity
//    <ch1>                         sensor channel
//       <watts>00345</watts>       data and units
//    </ch1>
//    <ch2>
//       <watts>02151</watts>
//    </ch2>
//    <ch3>
//       <watts>00000</watts>
//    </ch3>
// </msg>                           end of message
// there are also periodic messages with history 
// these all have the <hist> tag in them.
void CurrentCostMeterBase::ExtractReadings()
{
	// isn't a sensible line of data so ignore
	if(m_buffer.compare(0, 5, "<msg>") != 0)
	{
		return;
	}
	
	// for now we are not interested in history data
	if(m_buffer.find("<hist>") != std::string::npos)
	{
		return;
	}
	
	float temp;
	float sensor;
	float type;
	float reading;
	
	// if we have the sensor tag then only do the whle house one
	// earlier versions don't have this
	if(m_buffer.find("<sensor>") != std::string::npos)
	{
		if(!ExtractNumberBetweenStrings("<sensor>", "</sensor>", &sensor) || sensor != 0.0)
		{
			// not the whole house sensor, ignore
			return;
		}
	}

	if(!ExtractNumberBetweenStrings("<type>", "</type>", &type) || type != 1.0)
	{
		// not a power sensor, ignore
		return;
	}

	if(ExtractNumberBetweenStrings("<tmpr>", "</tmpr>", &temp))
	{
		SendTempSensor(1, 255, temp, "Temp");
	}

	float totalPower(0.0);
	if(ExtractNumberBetweenStrings("<ch1><watts>", "</watts></ch1>", &reading))
	{
		totalPower += reading;
	}
	if(ExtractNumberBetweenStrings("<ch2><watts>", "</watts></ch2>", &reading))
	{
		totalPower += reading;
	}
	if(ExtractNumberBetweenStrings("<ch3><watts>", "</watts></ch3>", &reading))
	{
		totalPower += reading;
	}
	SendWattMeter(2, 1, 255, totalPower, "CC Power");
}

bool CurrentCostMeterBase::ExtractNumberBetweenStrings(const char *startString, const char *endString, float *pResult)
{
	size_t startOfStart = m_buffer.find(startString);
	size_t startOfEnd = m_buffer.find(endString);
	size_t startLen = strlen(startString);
	
	// check that we can continue
	if(startOfStart == std::string::npos || startOfEnd ==std::string::npos || startOfEnd < startOfStart)
	{
		*pResult = -1.0;
		return false;
	}
	
	// extract the bit between the strings
	std::string substring(m_buffer.substr(startOfStart + startLen, startOfEnd - (startOfStart + startLen)));

	// convert to double and make sure there is no other crud
	// as the data is sometimes corrupted
	char* endPtr;
	*pResult = strtof(substring.c_str(), &endPtr);
	if(*endPtr == 0)
	{
		return true;
	}
	else
	{
		*pResult = -1.0;
		return false;
	}
}

void CurrentCostMeterBase::ParseData(const char *pData, int Len)
{
	int ii=0;
	while (ii<Len)
	{
		const char c = pData[ii];
		if(c == 0x0d)
		{
			ii++;
			continue;
		}

		m_buffer += c;
		if(c == 0x0a)
		{
			// discard newline, close string, parse line and clear it.
			if (!m_buffer.empty()) 
			{
				ExtractReadings();
			}
			m_buffer = "";
		}
		ii++;
	}
}
