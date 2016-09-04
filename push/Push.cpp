#include "stdafx.h"
#include "Push.h"
#include "../hardware/hardwaretypes.h"
#include <boost/date_time/c_local_time_adjustor.hpp>
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/Helper.h"

CPush::CPush()
{
	m_bLinkActive = false;
	m_DeviceRowIdx = -1;
}

// STATIC
static boost::posix_time::time_duration get_utc_offset() {
	using namespace boost::posix_time;

	// boost::date_time::c_local_adjustor uses the C-API to adjust a
	// moment given in utc to the same moment in the local time zone.
	typedef boost::date_time::c_local_adjustor<ptime> local_adj;

	const ptime utc_now = second_clock::universal_time();
	const ptime now = local_adj::utc_to_local(utc_now);

	return now - utc_now;
}

unsigned long CPush::get_tzoffset()
{
	// Compute tz
	boost::posix_time::time_duration uoffset = get_utc_offset();
	unsigned long tzoffset = (int)((double)(uoffset.ticks() / 3600000000LL) * 3600);
	return tzoffset;
}

#ifdef WIN32
std::string CPush::get_lastUpdate(unsigned __int64 localTimeUtc)
#else
std::string CPush::get_lastUpdate(unsigned long long int localTimeUtc)
#endif
{
	// RFC3339 time format
	time_t tmpT = localTimeUtc;
	struct tm* timeinfo = gmtime(&tmpT);

	char llastUpdate[255];
#if !defined WIN32
	snprintf(llastUpdate, sizeof(llastUpdate), "%04d-%02d-%02dT%02d:%02d:%02dZ",
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
#else
	sprintf_s(llastUpdate, sizeof(llastUpdate), "%04d-%02d-%02dT%02d:%02d:%02dZ",
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
#endif

	return std::string(llastUpdate);
}

// STATIC
void CPush::replaceAll(std::string& context, const std::string& from, const std::string& to)
{
	size_t lookHere = 0;
	size_t foundHere;
	while ((foundHere = context.find(from, lookHere)) != std::string::npos)
	{
		context.replace(foundHere, from.size(), to);
		lookHere = foundHere + to.size();
	}
}

std::vector<std::string> CPush::DropdownOptions(const unsigned long long DeviceRowIdxIn)
{
	std::vector<std::string> dropdownOptions;

	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID== %llu)", DeviceRowIdxIn);
	if (result.size()>0)
	{
		int dType=atoi(result[0][0].c_str());
		int dSubType=atoi(result[0][1].c_str());

		std::string sOptions = RFX_Type_SubType_Values(dType,dSubType);
		std::vector<std::string> tmpV; 
		StringSplit(sOptions, ",", tmpV); 
		for (int i = 0; i < (int) tmpV.size(); ++i) { 
			dropdownOptions.push_back(tmpV[i]); 
		}
	}
   	else 
	{
		dropdownOptions.push_back("Not implemented");
	}
	return dropdownOptions;
}

std::string CPush::DropdownOptionsValue(const unsigned long long DeviceRowIdxIn, const int pos)
{	
	std::string wording = "???";
	int getpos = pos-1; // 0 pos is always nvalue/status, 1 and higher goes to svalues
	std::vector<std::vector<std::string> > result;
	
	result=m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID== %llu)", DeviceRowIdxIn);
	if (result.size()>0)
	{
		int dType=atoi(result[0][0].c_str());
		int dSubType=atoi(result[0][1].c_str());

		std::string sOptions = RFX_Type_SubType_Values(dType,dSubType);
		std::vector<std::string> tmpV; 
		StringSplit(sOptions, ",", tmpV);
		if (tmpV.size() > 1)
		{
			if ( (int) tmpV.size() >= pos && getpos >= 0) {
				wording = tmpV[getpos];
			}
		}
		else if (tmpV.size() == 1)
		{
			wording = sOptions;
		}
	}
	return wording;
}

std::string CPush::ProcessSendValue(const std::string &rawsendValue, const int delpos, const int nValue, const int includeUnit, const int metertypein)
{
	std::string vType = DropdownOptionsValue(m_DeviceRowIdx,delpos);
	unsigned char tempsign=m_sql.m_tempsign[0];
	_eMeterType metertype = (_eMeterType) metertypein;
	char szData[100]= "";

	std::string unit = getUnit(delpos, metertypein);

	if ((vType=="Temperature") || (vType=="Temperature 1") || (vType=="Temperature 2")|| (vType == "Set point"))
	{
		double tvalue=ConvertTemperature(atof(rawsendValue.c_str()),tempsign);
		sprintf(szData,"%.1f", tvalue);
	}
	else if (vType == "Concentration")
	{
		sprintf(szData,"%d", nValue);
	}
	else if (vType == "Humidity")
	{
		sprintf(szData,"%d", atoi(rawsendValue.c_str()));
	}
	else if (vType == "Humidity Status")
	{
		sprintf(szData,"%s", RFX_Humidity_Status_Desc(atoi(rawsendValue.c_str())));	
	}
	else if (vType == "Barometer")
	{
		sprintf(szData,"%.1f", atof(rawsendValue.c_str()));
	}
	else if (vType == "Forecast")
	{
		int forecast=atoi(rawsendValue.c_str());
		if (forecast!=baroForecastNoInfo)
		{
			sprintf(szData,"%s", RFX_Forecast_Desc(forecast));
		}
		else {
			sprintf(szData,"%d", forecast);
		}
	}
	else if (vType == "Altitude")
	{
		sprintf(szData,"Not supported yet");
	}

	else if (vType == "UV")
	{
		float UVI = static_cast<float>(atof(rawsendValue.c_str()));
		sprintf(szData,"%.1f",UVI);
	}
	else if (vType == "Direction")
	{
		float Direction = static_cast<float>(atof(rawsendValue.c_str()));
		sprintf(szData,"%.1f",Direction); 
	}
	else if (vType == "Direction string")
	{
		sprintf(szData,"%s",rawsendValue.c_str());
	}
	else if (vType == "Speed")
	{
		int intSpeed = atoi(rawsendValue.c_str());
		if (m_sql.m_windunit != WINDUNIT_Beaufort)
		{
			sprintf(szData, "%.1f", float(intSpeed) * m_sql.m_windscale);
		}
		else
		{
			float speedms = float(intSpeed)*0.1f;
			sprintf(szData, "%d", MStoBeaufort(speedms));
		}
	}
	else if (vType == "Gust")
	{
		int intGust=atoi(rawsendValue.c_str());
		if (m_sql.m_windunit != WINDUNIT_Beaufort)
		{
			sprintf(szData, "%.1f", float(intGust) *m_sql.m_windscale);
		}
		else
		{
			float gustms = float(intGust)*0.1f;
			sprintf(szData, "%d", MStoBeaufort(gustms));
		}
	}
	else if (vType == "Chill")
	{
		double tvalue=ConvertTemperature(atof(rawsendValue.c_str()),tempsign);
		sprintf(szData,"%.1f", tvalue);
	}
	else if (vType == "Rain rate")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Total rain")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Counter")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Mode")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Status")
	{
		sprintf(szData, "%d", nValue);
	}	
	else if ((vType == "Current 1") || (vType == "Current 2") || (vType == "Current 3"))
	{
		sprintf(szData,"Not supported yet");
	}	
	else if (vType == "Instant")
	{
		sprintf(szData,"Not supported yet");
	}	
	else if ((vType == "Usage") || (vType == "Usage 1") || (vType == "Usage 2") )
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}	
	else if ((vType == "Delivery") || (vType == "Delivery 1") || (vType == "Delivery 2") )
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Usage current")
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Delivery current")
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Gas usage")
	{
		sprintf(szData, "%.3f", atof(rawsendValue.c_str()) / 1000.0f);
	}
	else if (vType == "Weight")
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}	
	else if (vType == "Voltage")
	{
		sprintf(szData,"%.3f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Value")
	{
		sprintf(szData,"%d", atoi(rawsendValue.c_str())); //??
	}
	else if (vType == "Visibility")
	{
		float vis = static_cast<float>(atof(rawsendValue.c_str()));
		if (metertype==0)
		{
			//km
			sprintf(szData,"%.1f",vis);
		}
		else
		{
			//miles
			sprintf(szData,"%.1f",vis*0.6214f);
		}
	}
	else if (vType == "Solar Radiation")
	{
		float radiation = static_cast<float>(atof(rawsendValue.c_str()));
		sprintf(szData,"%.1f",radiation);
	}
	else if (vType == "Soil Moisture")
	{
		sprintf(szData,"%d",nValue);
	}
	else if (vType == "Leaf Wetness")
	{
		sprintf(szData,"%d",nValue);
	}
	else if (vType == "Percentage")
	{
		sprintf(szData,"%.2f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Fanspeed")
	{
		sprintf(szData,"%d",atoi(rawsendValue.c_str()));
	}
	else if (vType == "Pressure")
	{
		sprintf(szData,"%.1f",atof(rawsendValue.c_str()));
	}
	else if (vType == "Lux")
	{
		sprintf(szData,"%.0f",atof(rawsendValue.c_str()));
	}
	if (szData[0] != '\0') { 
		std::string sendValue(szData);
		if (includeUnit) {
			sendValue+=" ";
			sendValue+=unit;
		}
		return sendValue;
	}
	else {
		_log.Log(LOG_ERROR,"Could not determine data push value");
		return "";
	}
}

std::string CPush::getUnit(const int delpos, const int metertypein)
{
	std::string vType = DropdownOptionsValue(m_DeviceRowIdx,delpos);
	unsigned char tempsign=m_sql.m_tempsign[0];
	_eMeterType metertype = (_eMeterType) metertypein;
	char szData[100]= "";

	if ((vType=="Temperature") || (vType=="Temperature 1") || (vType=="Temperature 2")|| (vType == "Set point"))
	{
		sprintf(szData,"%c", tempsign);
	}
	else if (vType == "Humidity")
	{
		strcpy(szData,"%%");
	}
	else if (vType == "Humidity Status")
	{
		strcpy(szData, "");
	}
	else if (vType == "Barometer")
	{
		strcpy(szData, "hPa");
	}
	else if (vType == "Forecast")
	{
		strcpy(szData, "");
	}
	else if (vType == "Altitude")
	{
		strcpy(szData, "");
	}
	else if (vType == "UV")
	{
		strcpy(szData, "UVI");
	}
	else if (vType == "Direction")
	{
		strcpy(szData, "Degrees");
	}
	else if (vType == "Direction string")
	{
		strcpy(szData, "");
	}
	else if (vType == "Speed")
	{
		strcpy(szData, "");//todo: unit?
	}
	else if (vType == "Gust")
	{
		strcpy(szData, "");//todo: unit?
	}
	else if (vType == "Chill")
	{
		sprintf(szData, "%c", tempsign);
	}
	else if (vType == "Rain rate")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Total rain")
	{
		strcpy(szData, "");
	}	
	else if (vType == "Counter")
	{
		strcpy(szData, "");
	}	
	else if (vType == "Mode")
	{
		strcpy(szData, "");
	}
	else if (vType == "Status")
	{
		strcpy(szData, "");
	}	
	else if ((vType == "Current 1") || (vType == "Current 2") || (vType == "Current 3"))
	{
		strcpy(szData, "");
	}	
	else if (vType == "Instant")
	{
		strcpy(szData, "");
	}	
	else if ((vType == "Usage") || (vType == "Usage 1") || (vType == "Usage 2") )
	{
		strcpy(szData, "Watt");
	}	
	else if ((vType == "Delivery") || (vType == "Delivery 1") || (vType == "Delivery 2") )
	{
		strcpy(szData, "Watt");
	}
	else if (vType == "Usage current")
	{
		strcpy(szData, "Watt");
	}
	else if (vType == "Delivery current")
	{
		strcpy(szData, "Watt");
	}
	else if (vType == "Gas usage")
	{
		strcpy(szData, "");
	}
	else if (vType == "Weight")
	{
		strcpy(szData, "kg");
	}	
	else if (vType == "Voltage")
	{
		strcpy(szData, "V");
	}
	else if (vType == "Value")
	{
		strcpy(szData, "");
	}
	else if (vType == "Visibility")
	{
		if (metertype==0)
		{
			//km
			strcpy(szData, "km");
		}
		else
		{
			//miles
			strcpy(szData, "mi");
		}
	}
	else if (vType == "Solar Radiation")
	{
		strcpy(szData, "Watt/m2");
	}
	else if (vType == "Soil Moisture")
	{
		strcpy(szData, "cb");
	}
	else if (vType == "Leaf Wetness")
	{
		strcpy(szData, "");
	}
	else if (vType == "Percentage")
	{
		strcpy(szData, "%");
	}
	else if (vType == "Fanspeed")
	{
		strcpy(szData, "RPM");
	}
	else if (vType == "Pressure")
	{
		strcpy(szData, "Bar");
	}
	else if (vType == "Lux")
	{
		strcpy(szData, "Lux");
	}
	else if (vType == "Concentration")
	{
		strcpy(szData, "ppm");
	}
	if (szData[0] != '\0') { 
		std::string sendValue(szData);
		return sendValue;
	}
	// No unit
	return "";
}

