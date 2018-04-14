#include "stdafx.h"
#include "CurrentCostMeterBase.h"
#include "hardwaretypes.h"

CurrentCostMeterBase::CurrentCostMeterBase(void) :
	m_tempuratureCounter(0)
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
//				    sensor Type, "2" = electric impulse
//    <ch1>                         sensor channel
//       <watts>00345</watts>       data and units
//    </ch1>
//    <ch2>
//       <watts>02151</watts>
//    </ch2>
//    <ch3>
//       <watts>00000</watts>
//    </ch3>
//    <type>2</type> 
//    <imp>0000089466</imp>	    Meter Impulse Count
//    <ipu>1000</ipu>		    Meter Impulses Per Unit : nbr impulse to have 1kWH
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
	float sensor(0.0);
	float type;
	float reading;
	
	// if we have the sensor tag then read it
	// earlier versions don't have this
	if(m_buffer.find("<sensor>") != std::string::npos)
	{
		if(!ExtractNumberBetweenStrings("<sensor>", "</sensor>", &sensor) || sensor > 9.0)
		{
			// no sensor end tag found or too high a sensor number 
			// indicating data must be corrupt
			return;
		}
	}

	if(!ExtractNumberBetweenStrings("<type>", "</type>", &type) || ((type != 1.0) && (type != 2.0)))
	{
		// not a power sensor, ignore
		return;
	}

	// get the temp and only process if it is from the whole house sensor
	if(sensor == 0.0 && ExtractNumberBetweenStrings("<tmpr>", "</tmpr>", &temp))
	{
		// only send the temp once a minute to avoid filling up the db
		if((m_tempuratureCounter++ % 10) == 0)
		{
			SendTempSensor(1, 255, temp, "Temp");
		}
	}
	if (type == 1.0)
	{
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
		if(sensor == 0.0)
		{
			SendWattMeter(2, 1, 255, totalPower, "CC Whole House Power");
		}
		else
		{
			// create a suitable default name
			// there can only be 8 sensors so this
			// method is OK and should be faster
			char sensorInt(static_cast<char>(sensor));
			std::string sensorName("CC Sensor 0 Power");
			sensorName[10] += sensorInt;
			SendWattMeter(2 + sensorInt, 1, 255, totalPower, sensorName);
		}
	}
	else if (type == 2.0)
	{
		float consumption(0.0);
		float ipu(1000.0);
		if (ExtractNumberBetweenStrings("<imp>", "</imp>", &reading))
		{
			consumption = reading;	
		}
		if (ExtractNumberBetweenStrings("<ipu>", "</ipu>", &reading) && (reading != 0.0))
		{
			ipu = reading;	
		}
		if (consumption != 0.0)
		{
			consumption /= ipu;
			char sensorInt(static_cast<char>(sensor));
			std::string sensorName("CC Sensor 0 kWh consumption");
			sensorName[10] += sensorInt;
			SendKwhMeter(2 + sensorInt, 2, 255, 0, consumption, sensorName);
		}
	}
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
