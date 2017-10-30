#include "stdafx.h"
#include "BasePush.h"
#include "../hardware/hardwaretypes.h"
#include "../json/json.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/SQLHelper.h"
#include "../main/WebServer.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <boost/date_time/c_local_time_adjustor.hpp>

typedef struct _STR_TABLE_ID1_ID2 {
	unsigned long    id1;
	unsigned long    id2;
	const char   *str1;
} STR_TABLE_ID1_ID2;

extern const char *findTableID1ID2(const _STR_TABLE_ID1_ID2 *t, const unsigned long id1, const unsigned long id2);

const char *RFX_Type_SubType_Values(const unsigned char dType, const unsigned char sType)
{
	static const STR_TABLE_ID1_ID2	Table[] =
	{
		{ pTypeTEMP, sTypeTEMP1, "Temperature" },
		{ pTypeTEMP, sTypeTEMP2, "Temperature" },
		{ pTypeTEMP, sTypeTEMP3, "Temperature" },
		{ pTypeTEMP, sTypeTEMP4, "Temperature" },
		{ pTypeTEMP, sTypeTEMP5, "Temperature" },
		{ pTypeTEMP, sTypeTEMP6, "Temperature" },
		{ pTypeTEMP, sTypeTEMP7, "Temperature" },
		{ pTypeTEMP, sTypeTEMP8, "Temperature" },
		{ pTypeTEMP, sTypeTEMP9, "Temperature" },
		{ pTypeTEMP, sTypeTEMP10, "Temperature" },
		{ pTypeTEMP, sTypeTEMP11, "Temperature" },
		{ pTypeTEMP, sTypeTEMP_SYSTEM, "Temperature" },

		{ pTypeHUM, sTypeHUM1, "Humidity,Humidity Status" },
		{ pTypeHUM, sTypeHUM2, "Humidity,Humidity Status" },

		{ pTypeTEMP_HUM, sTypeTH1, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH2, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH3, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH4, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH5, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH6, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH7, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH8, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH9, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH10, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH11, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH12, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH13, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH14, "Temperature,Humidity,Humidity Status" },
		{ pTypeTEMP_HUM, sTypeTH_LC_TC, "Temperature,Humidity,Humidity Status" },


		{ pTypeTEMP_HUM_BARO, sTypeTHB1, "Temperature,Humidity,Humidity Status,Barometer,Forecast" },
		{ pTypeTEMP_HUM_BARO, sTypeTHB2, "Temperature,Humidity,Humidity Status,Barometer,Forecast" },
		{ pTypeTEMP_HUM_BARO, sTypeTHBFloat, "Temperature,Humidity,Humidity Status,Barometer,Forecast" },

		{ pTypeRAIN, sTypeRAIN1, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAIN2, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAIN3, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAIN4, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAIN5, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAIN6, "Rain rate,Total rain" },
		{ pTypeRAIN, sTypeRAINWU, "Rain rate,Total rain" },

		{ pTypeWIND, sTypeWIND1, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND2, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND3, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND4, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND5, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND6, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWIND7, "Direction,Direction string,Speed,Gust,Temperature,Chill" },
		{ pTypeWIND, sTypeWINDNoTemp, "Direction,Direction string,Speed,Gust,Temperature,Chill" },

		{ pTypeUV, sTypeUV1, "UV,Temperature" },
		{ pTypeUV, sTypeUV2, "UV,Temperature" },
		{ pTypeUV, sTypeUV3, "UV,Temperature" },

		{ pTypeLighting1, sTypeX10, "Status" },
		{ pTypeLighting1, sTypeARC, "Status" },
		{ pTypeLighting1, sTypeAB400D, "Status" },
		{ pTypeLighting1, sTypeWaveman, "Status" },
		{ pTypeLighting1, sTypeEMW200, "Status" },
		{ pTypeLighting1, sTypeIMPULS, "Status" },
		{ pTypeLighting1, sTypeRisingSun, "Status" },
		{ pTypeLighting1, sTypePhilips, "Status" },
		{ pTypeLighting1, sTypeEnergenie, "Status" },
		{ pTypeLighting1, sTypeEnergenie5, "Status" },
		{ pTypeLighting1, sTypeGDR2, "Status" },
		{ pTypeLighting1, sTypeHQ, "Status" },

		{ pTypeLighting2, sTypeAC, "Status" },
		{ pTypeLighting2, sTypeHEU, "Status" },
		{ pTypeLighting2, sTypeANSLUT, "Status" },
		{ pTypeLighting2, sTypeKambrook, "Status" },

		{ pTypeLighting3, sTypeKoppla, "Status" },

		{ pTypeLighting4, sTypePT2262, "Status" },

		{ pTypeLighting5, sTypeLightwaveRF, "Status" },
		{ pTypeLighting5, sTypeEMW100, "Status" },
		{ pTypeLighting5, sTypeBBSB, "Status" },
		{ pTypeLighting5, sTypeMDREMOTE, "Status" },
		{ pTypeLighting5, sTypeRSL, "Status" },
		{ pTypeLighting5, sTypeLivolo, "Status" },
		{ pTypeLighting5, sTypeTRC02, "Status" },
		{ pTypeLighting5, sTypeTRC02_2, "Status" },
		{ pTypeLighting5, sTypeAoke, "Status" },
		{ pTypeLighting5, sTypeEurodomest, "Status" },
		{ pTypeLighting5, sTypeLivoloAppliance, "Status" },
		{ pTypeLighting5, sTypeRGB432W, "Status" },
		{ pTypeLighting5, sTypeMDREMOTE107, "Status" },
		{ pTypeLighting5, sTypeLegrandCAD, "Status" },
		{ pTypeLighting5, sTypeAvantek, "Status" },
		{ pTypeLighting5, sTypeIT, "Status" },
		{ pTypeLighting5, sTypeMDREMOTE108, "Status" },
		{ pTypeLighting5, sTypeKangtai, "Status" },

		{ pTypeLighting6, sTypeBlyss, "Status" },

		{ pTypeHomeConfort, sTypeHomeConfortTEL010, "Status" },

		{ pTypeCurtain, sTypeHarrison, "Status" },

		{ pTypeBlinds, sTypeBlindsT0, "Status" },
		{ pTypeBlinds, sTypeBlindsT1, "Status" },
		{ pTypeBlinds, sTypeBlindsT2, "Status" },
		{ pTypeBlinds, sTypeBlindsT3, "Status" },
		{ pTypeBlinds, sTypeBlindsT4, "Status" },
		{ pTypeBlinds, sTypeBlindsT5, "Status" },
		{ pTypeBlinds, sTypeBlindsT6, "Status" },
		{ pTypeBlinds, sTypeBlindsT7, "Status" },
		{ pTypeBlinds, sTypeBlindsT8, "Status" },
		{ pTypeBlinds, sTypeBlindsT9, "Status" },
		{ pTypeBlinds, sTypeBlindsT10, "Status" },
		{ pTypeBlinds, sTypeBlindsT11, "Status" },
		{ pTypeBlinds, sTypeBlindsT12, "Status" },
		{ pTypeBlinds, sTypeBlindsT13, "Status" },

		{ pTypeSecurity1, sTypeSecX10, "Status" },
		{ pTypeSecurity1, sTypeSecX10M, "Status" },
		{ pTypeSecurity1, sTypeSecX10R, "Status" },
		{ pTypeSecurity1, sTypeKD101, "Status" },
		{ pTypeSecurity1, sTypePowercodeSensor, "Status" },
		{ pTypeSecurity1, sTypePowercodeMotion, "Status" },
		{ pTypeSecurity1, sTypeCodesecure, "Status" },
		{ pTypeSecurity1, sTypePowercodeAux, "Status" },
		{ pTypeSecurity1, sTypeMeiantech, "Status" },
		{ pTypeSecurity1, sTypeSA30, "Status" },
		{ pTypeSecurity1, sTypeDomoticzSecurity, "Status" },

		{ pTypeSecurity2, sTypeSec2Classic, "Status" },

		{ pTypeCamera, sTypeNinja, "Not implemented" },

		{ pTypeRemote, sTypeATI, "Status" },
		{ pTypeRemote, sTypeATIplus, "Status" },
		{ pTypeRemote, sTypeMedion, "Status" },
		{ pTypeRemote, sTypePCremote, "Status" },
		{ pTypeRemote, sTypeATIrw2, "Status" },

		{ pTypeThermostat1, sTypeDigimax, "Temperature,Set point,Mode,Status" },
		{ pTypeThermostat1, sTypeDigimaxShort, "Temperature,Set point,Mode,Status" },

		{ pTypeThermostat2, sTypeHE105, "Status" },
		{ pTypeThermostat2, sTypeRTS10, "Status" },

		{ pTypeThermostat3, sTypeMertikG6RH4T1, "Status" },
		{ pTypeThermostat3, sTypeMertikG6RH4TB, "Status" },
		{ pTypeThermostat3, sTypeMertikG6RH4TD, "Status" },

		{ pTypeThermostat4, sTypeMCZ1, "Status,Mode" },
		{ pTypeThermostat4, sTypeMCZ2, "Status,Mode" },
		{ pTypeThermostat4, sTypeMCZ3, "Status,Mode" },

		{ pTypeRadiator1, sTypeSmartwares, "Status" },
		{ pTypeRadiator1, sTypeSmartwaresSwitchRadiator, "Status" },

		{ pTypeDT, sTypeDT1, "?????" },

		{ pTypeCURRENT, sTypeELEC1, "Current 1,Current 2,Current 3" },

		{ pTypeENERGY, sTypeELEC2, "Instant,Usage" },
		{ pTypeENERGY, sTypeELEC3, "Instant,Usage" },

		{ pTypeCURRENTENERGY, sTypeELEC4, "Current 1,Current 2,Current 3,Usage" },

		{ pTypeWEIGHT, sTypeWEIGHT1, "Weight" },
		{ pTypeWEIGHT, sTypeWEIGHT2, "Weight" },

		{ pTypeRFXSensor, sTypeRFXSensorTemp, "Temperature" },
		{ pTypeRFXSensor, sTypeRFXSensorAD, "Voltage" },
		{ pTypeRFXSensor, sTypeRFXSensorVolt, "Voltage" },

		{ pTypeRFXMeter, sTypeRFXMeterCount, "Counter" },

		{ pTypeP1Power, sTypeP1Power, "Usage 1,Usage 2,Delivery 1,Delivery 2,Usage current,Delivery current" },
		{ pTypeP1Gas, sTypeP1Gas, "Gas usage" },

		{ pTypeYouLess, sTypeYouLess, "Usage,Usage current" },

		{ pTypeRego6XXTemp, sTypeRego6XXTemp, "Temperature" },
		{ pTypeRego6XXValue, sTypeRego6XXStatus, "Value" },
		{ pTypeRego6XXValue, sTypeRego6XXCounter, "Counter" },

		{ pTypeAirQuality, sTypeVoltcraft, "Concentration" },

		{ pTypeUsage, sTypeElectric, "Usage" },

		{ pTypeTEMP_BARO, sTypeBMP085, "Temperature,Barometer,Forecast,Altitude" },

		{ pTypeLux, sTypeLux, "Lux" },

		{ pTypeGeneral, sTypeVisibility, "Visibility" },
		{ pTypeGeneral, sTypeSolarRadiation, "Solar Radiation" },
		{ pTypeGeneral, sTypeSoilMoisture, "Soil Moisture" },
		{ pTypeGeneral, sTypeLeafWetness, "Leaf Wetness" },
		{ pTypeGeneral, sTypeSystemTemp, "Temperature" },
		{ pTypeGeneral, sTypePercentage, "Percentage" },
		{ pTypeGeneral, sTypeFan, "Fanspeed" },
		{ pTypeGeneral, sTypeVoltage, "Voltage" },
		{ pTypeGeneral, sTypeCurrent, "Current" },
		{ pTypeGeneral, sTypePressure, "Pressure" },
		{ pTypeGeneral, sTypeBaro, "Barometer" },
		{ pTypeGeneral, sTypeSetPoint, "Temperature" },
		{ pTypeGeneral, sTypeTemperature, "Temperature" },
		{ pTypeGeneral, sTypeZWaveClock, "Thermostat Clock" },
		{ pTypeGeneral, sTypeTextStatus, "Text" },
		{ pTypeGeneral, sTypeZWaveThermostatMode, "Thermostat Mode" },
		{ pTypeGeneral, sTypeZWaveThermostatFanMode, "Thermostat Fan Mode" },
		{ pTypeGeneral, sTypeAlert, "Alert" },
		{ pTypeGeneral, sTypeSoundLevel, "Sound Level" },
		{ pTypeGeneral, sTypeUV, "UV,Temperature" },
		{ pTypeGeneral, sTypeDistance, "Distance" },
		{ pTypeGeneral, sTypeCounterIncremental, "Counter Incremental" },
		{ pTypeGeneral, sTypeKwh, "Instant,Usage" },
		{ pTypeGeneral, sTypeWaterflow, "Percentage" },
		{ pTypeGeneral, sTypeCustom, "Percentage" },
		{ pTypeGeneral, sTypeZWaveAlarm, "Status" },

		{ pTypeThermostat, sTypeThermSetpoint, "Temperature" },
		{ pTypeThermostat, sTypeThermTemperature, "Temperature" },

		{ pTypeChime, sTypeByronSX, "Status" },
		{ pTypeChime, sTypeByronMP001, "Status" },
		{ pTypeChime, sTypeSelectPlus, "Status" },
		{ pTypeChime, sTypeSelectPlus3, "Status" },
		{ pTypeChime, sTypeEnvivo, "Status" },

		{ pTypeTEMP_RAIN, sTypeTR1, "Temperature,Total rain" },

		{ pTypeBBQ, sTypeBBQ1, "Temperature 1,Temperature 2" },

		{ pTypePOWER, sTypeELEC5, "Instant,Usage" },

		{ pTypeLimitlessLights, sTypeLimitlessRGBW, "Status" },
		{ pTypeLimitlessLights, sTypeLimitlessRGB, "Status" },
		{ pTypeLimitlessLights, sTypeLimitlessWhite, "Status" },
		{ pTypeLimitlessLights, sTypeLimitlessRGBWW, "Status" },

		{ pTypeRFY, sTypeRFY, "Status" },
		{ pTypeRFY, sTypeRFY2, "Status" },
		{ pTypeRFY, sTypeRFYext, "Status" },
		{ pTypeRFY, sTypeASA, "Status" },
		{ pTypeEvohome, sTypeEvohome, "Status" },
		{ pTypeEvohomeZone, sTypeEvohomeZone, "Temperature,Set point,Status" },
		{ pTypeEvohomeWater, sTypeEvohomeWater, "Temperature,State,Status" },
		{ pTypeEvohomeRelay, sTypeEvohomeRelay, "Status" },

		{ pTypeGeneralSwitch, sSwitchTypeX10, "Status" },

		{ pTypeGeneralSwitch, sSwitchTypeX10, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeARC, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAB400D, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeWaveman, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEMW200, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeIMPULS, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeRisingSun, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypePhilips, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEnergenie, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEnergenie5, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeGDR2, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAC, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeHEU, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeANSLUT, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeKambrook, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeKoppla, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypePT2262, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeLightwaveRF, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEMW100, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeBBSB, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeMDREMOTE, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeRSL, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeLivolo, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeTRC02, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAoke, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeTRC02_2, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEurodomest, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeLivoloAppliance, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeBlyss, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeByronSX, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeByronMP001, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSelectPlus, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSelectPlus3, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeFA20, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeChuango, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypePlieger, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSilvercrest, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeMertik, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeHomeConfort, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypePowerfix, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeTriState, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeDeltronic, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeFA500, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeHT12E, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEV1527, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeElmes, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAster, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSartano, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeEurope, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAvidsen, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeBofu, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeBrel, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeRTS, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeElroDB, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeDooya, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeUnitec, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSelector, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeMaclean, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeR546, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeDiya, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeX10secu, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeAtlantic, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeSilvercrestDB, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeMedionDB, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeVMC, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeKeeloq, "Status" },
		{ pTypeGeneralSwitch, sSwitchCustomSwitch, "Status" },
		{ pTypeGeneralSwitch, sSwitchGeneralSwitch, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeKoch, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeKingpin, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeFunkbus, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeNice, "Status" },
		{ pTypeGeneralSwitch, sSwitchTypeForest, "Status" },

		{ 0,0,NULL }
	};
	return findTableID1ID2(Table, dType, sType);
}

CBasePush::CBasePush()
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

unsigned long CBasePush::get_tzoffset()
{
	// Compute tz
	boost::posix_time::time_duration uoffset = get_utc_offset();
	unsigned long tzoffset = (int)((double)(uoffset.ticks() / 3600000000LL) * 3600);
	return tzoffset;
}

#ifdef WIN32
std::string CBasePush::get_lastUpdate(unsigned __int64 localTimeUtc)
#else
std::string CBasePush::get_lastUpdate(unsigned long long int localTimeUtc)
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
void CBasePush::replaceAll(std::string& context, const std::string& from, const std::string& to)
{
	size_t lookHere = 0;
	size_t foundHere;
	while ((foundHere = context.find(from, lookHere)) != std::string::npos)
	{
		context.replace(foundHere, from.size(), to);
		lookHere = foundHere + to.size();
	}
}

std::vector<std::string> CBasePush::DropdownOptions(const uint64_t DeviceRowIdxIn)
{
	std::vector<std::string> dropdownOptions;

	std::vector<std::vector<std::string> > result;
	result=m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID== %" PRIu64 ")", DeviceRowIdxIn);
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

std::string CBasePush::DropdownOptionsValue(const uint64_t DeviceRowIdxIn, const int pos)
{	
	std::string wording = "???";
	int getpos = pos-1; // 0 pos is always nvalue/status, 1 and higher goes to svalues
	std::vector<std::vector<std::string> > result;
	
	result=m_sql.safe_query("SELECT Type, SubType FROM DeviceStatus WHERE (ID== %" PRIu64 ")", DeviceRowIdxIn);
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

std::string CBasePush::ProcessSendValue(const std::string &rawsendValue, const int delpos, const int nValue, const int includeUnit, const int metertypein)
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
		sprintf(szData, "%.1f", atof(rawsendValue.c_str()));
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
		sprintf(szData, "%.1f", atof(rawsendValue.c_str()));
	}
	else if (vType == "Total rain")
	{
		sprintf(szData, "%.1f", atof(rawsendValue.c_str()));
	}
	else if (vType == "Counter")
	{
		strcpy(szData, rawsendValue.c_str());
	}	
	else if (vType == "Mode")
	{
		sprintf(szData,"Not supported yet");
	}
	else if (vType == "Sound Level")
	{
		strcpy(szData, rawsendValue.c_str());
	}
	else if (vType == "Distance")
	{
		strcpy(szData, rawsendValue.c_str());
	}
	else if (vType == "Status")
	{
		sprintf(szData, "%d", nValue);
	}	
	else if ((vType == "Current 1") || (vType == "Current 2") || (vType == "Current 3"))
	{
		sprintf(szData, "%.3f", atof(rawsendValue.c_str()));
	}	
	else if (vType == "Instant")
	{
		sprintf(szData, "%.3f", atof(rawsendValue.c_str()));
	}
	else if ((vType == "Usage") || (vType == "Usage 1") || (vType == "Usage 2") )
	{
		strcpy(szData,rawsendValue.c_str());
	}	
	else if ((vType == "Delivery") || (vType == "Delivery 1") || (vType == "Delivery 2") )
	{
		strcpy(szData, rawsendValue.c_str());
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

std::string CBasePush::getUnit(const int delpos, const int metertypein)
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
		strcpy(szData,"");
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
	else if (vType == "Sound Level")
	{
		strcpy(szData, "dB");
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
		strcpy(szData, m_sql.m_weightsign.c_str());
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

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_GetDevicesListOnOff(WebEmSession & session, const request& req, Json::Value &root)
		{
			root["status"] = "OK";
			root["title"] = "GetDevicesListOnOff";
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID, Name, Type, SubType FROM DeviceStatus WHERE (Used == 1) ORDER BY Name");
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
					int dType = atoi(sd[2].c_str());
					int dSubType = atoi(sd[3].c_str());
					std::string sOptions = RFX_Type_SubType_Values(dType, dSubType);
					if (sOptions == "Status")
					{
						root["result"][ii]["name"] = sd[1];
						root["result"][ii]["value"] = sd[0];
						ii++;
					}
				}
			}
		}
	}
}
