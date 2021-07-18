#include "stdafx.h"

#include <string>

#include "../main/Logger.h"

#include "EnOceanEEP.h"

struct _tManufacturerTable
{
	uint32_t ID;
	const char* label;
	const char* name;
};

static const _tManufacturerTable _manufacturerTable[] = {
	{ UNKNOWN_MANUFACTURER, "UNKNOWN_MANUFACTURER", "Unknown"},
	{ PEHA, "PEHA", "Peha" },
	{ THERMOKON, "THERMOKON", "Thermokon" },
	{ SERVODAN, "SERVODAN", "Servodan" },
	{ ECHOFLEX_SOLUTIONS, "ECHOFLEX_SOLUTIONS", "EchoFlex Solutions" },
	{ OMNIO_AG, "OMNIO_AG", "Omnio AG" },
	{ HARDMEIER_ELECTRONICS, "HARDMEIER_ELECTRONICS", "Hardmeier electronics" },
	{ REGULVAR_INC, "REGULVAR_INC", "Regulvar Inc" },
	{ AD_HOC_ELECTRONICS, "AD_HOC_ELECTRONICS", "Ad Hoc Electronics" },
	{ DISTECH_CONTROLS, "DISTECH_CONTROLS", "Distech Controls" },
	{ KIEBACK_AND_PETER, "KIEBACK_AND_PETER", "Kieback + Peter" },
	{ ENOCEAN_GMBH, "ENOCEAN_GMBH", "EnOcean GmbH" },
	{ PROBARE, "PROBARE", "Probare" },
	{ ELTAKO, "ELTAKO", "Eltako" },
	{ LEVITON, "LEVITON", "Leviton" },
	{ HONEYWELL, "HONEYWELL", "Honeywell" },
	{ SPARTAN_PERIPHERAL_DEVICES, "SPARTAN_PERIPHERAL_DEVICES", "Spartan Peripheral Devices" },
	{ SIEMENS, "SIEMENS", "Siemens" },
	{ T_MAC, "T_MAC", "T-Mac" },
	{ RELIABLE_CONTROLS_CORPORATION, "RELIABLE_CONTROLS_CORPORATION", "Reliable Controls Corporation" },
	{ ELSNER_ELEKTRONIK_GMBH, "ELSNER_ELEKTRONIK_GMBH", "Elsner Elektronik GmbH" },
	{ DIEHL_CONTROLS, "DIEHL_CONTROLS", "Diehl Controls" },
	{ BSC_COMPUTER, "BSC_COMPUTER", "BSC Computer" },
	{ S_AND_S_REGELTECHNIK_GMBH, "S_AND_S_REGELTECHNIK_GMBH", "S+S Regeltechnik GmbH" },
	{ ZENO_CONTROLS, "ZENO_CONTROLS", "Zeno Controls" },
	{ INTESIS_SOFTWARE_SL, "INTESIS_SOFTWARE_SL", "Intesis Software SL" },
	{ VIESSMANN, "VIESSMANN", "Viessmann" },
	{ LUTUO_TECHNOLOGY, "LUTUO_TECHNOLOGY", "Lutuo Technology" },
	{ CAN2GO, "CAN2GO", "CAN2GO" },
	{ SAUTER, "SAUTER", "SAUTER" },
	{ BOOT_UP, "BOOT_UP", "BOOT UP" },
	{ OSRAM_SYLVANIA, "OSRAM_SYLVANIA", "OSRAM SYLVANIA" },
	{ UNOTECH, "UNOTECH", "UNOTECH" },
	{ DELTA_CONTROLS_INC, "DELTA_CONTROLS_INC", "DELTA CONTROLS INC" },
	{ UNITRONIC_AG, "UNITRONIC_AG", "UNITRONIC AG" },
	{ NANOSENSE, "NANOSENSE", "NANOSENSE" },
	{ THE_S4_GROUP, "THE_S4_GROUP", "THE S4 GROUP" },
	{ MSR_SOLUTIONS, "MSR_SOLUTIONS", "MSR SOLUTIONS" },
	{ GE, "GE", "GE" },
	{ MAICO, "MAICO", "MAICO" },
	{ RUSKIN_COMPANY, "RUSKIN_COMPANY", "RUSKIN COMPANY" },
	{ MAGNUM_ENERGY_SOLUTIONS, "MAGNUM_ENERGY_SOLUTIONS", "MAGNUM ENERGY SOLUTIONS" },
	{ KMC_CONTROLS, "KMC_CONTROLS", "KMC CONTROLS" },
	{ ECOLOGIX_CONTROLS, "ECOLOGIX_CONTROLS", "ECOLOGIX CONTROLS" },
	{ TRIO_2_SYS, "TRIO_2_SYS", "Trio2Sys" },
	{ AFRISO_EURO_INDEX, "AFRISO_EURO_INDEX", "AFRISO EURO INDEX" },
	{ WALDMANN_GMBH, "WALDMANN_GMBH", "Waldmann GmbH" },
	{ 0x02F, "2F_RESERVED_MANUFACTURER", nullptr },
	{ NEC_PLATFORMS_LTD, "NEC_PLATFORMS_LTD", "NEC ACCESSTECHNICA LTD" },
	{ ITEC_CORPORATION, "ITEC_CORPORATION", "ITEC CORPORATION" },
	{ SIMICX_CO_LTD, "SIMICX_CO_LTD", "SIMICX CO LTD" },
	{ PERMUNDO_GMBH, "PERMUNDO_GMBH", "Permundo GmbH" },
	{ EUROTRONIC_TECHNOLOGY_GMBH, "EUROTRONIC_TECHNOLOGY_GMBH", "EUROTRONIC TECHNOLOGY GmbH" },
	{ ART_JAPAN_CO_LTD, "ART_JAPAN_CO_LTD", "ART JAPAN CO LTD" },
	{ TIANSU_AUTOMATION_CONTROL_SYSTE_CO_LTD, "TIANSU_AUTOMATION_CONTROL_SYSTE_CO_LTD", "TIANSU AUTOMATION CONTROL SYSTE CO LTD" },
	{ WEINZIERL_ENGINEERING_GMBH, "WEINZIERL_ENGINEERING_GMBH", "Weinzierl Engineering GmbH" },
	{ GRUPPO_GIORDANO_IDEA_SPA, "GRUPPO_GIORDANO_IDEA_SPA", "GRUPPO GIORDANO IDEA SPA" },
	{ ALPHAEOS_AG, "ALPHAEOS_AG", "ALPHAEOS AG" },
	{ TAG_TECHNOLOGIES, "TAG_TECHNOLOGIES", "TAG TECHNOLOGIES" },
	{ WATTSTOPPER, "WATTSTOPPER", "WATTSTOPPER" },
	{ PRESSAC_COMMUNICATIONS_LTD, "PRESSAC_COMMUNICATIONS_LTD", "PRESSAC COMMUNICATIONS LTD" },
	{ 0x03D, "3D_RESERVED_MANUFACTURER", nullptr },
	{ GIGA_CONCEPT, "GIGA_CONCEPT", "GIGA CONCEPT" },
	{ SENSORTEC, "SENSORTEC", "SENSORTEC" },
	{ JAEGER_DIREKT, "JAEGER_DIREKT", "JAEGER DIREKT" },
	{ AIR_SYSTEM_COMPONENTS_INC, "AIR_SYSTEM_COMPONENTS_INC", "AIR SYSTEM COMPONENTS INC" },
	{ ERMINE_CORP, "ERMINE_CORP", "ERMINE Corp." },
	{ SODA_GMBH, "SODA_GMBH", "SODA GmbH" },
	{ EKE_AUTOMATION, "EKE_AUTOMATION", "EKE Automation" },
	{ HOLTER_REGELARMUTREN, "HOLTER_REGELARMUTREN", "Holter Regelarmaturen GmbH Co. KG" },
	{ NODON, "NODON", "NodOn" },
	{ DEUTA_CONTROLS_GMBH, "DEUTA_CONTROLS_GMBH", "DEUTA Controls GmbH" },
	{ EWATTCH, "EWATTCH", "Ewattch" },
	{ MICROPELT, "MICROPELT", "Micropelt GmbH" },
	{ CALEFFI_SPA, "CALEFFI_SPA", "Caleffi Spa." },
	{ DIGITAL_CONCEPTS, "DIGITAL_CONCEPTS", "Digital Concept" },
	{ EMERSON_CLIMATE_TECHNOLOGIES, "EMERSON_CLIMATE_TECHNOLOGIES", "Emerson Climate Technologies" },
	{ ADEE_ELECTRONIC, "ADEE_ELECTRONIC", "ADEE electronic" },
	{ ALTECON, "ALTECON", "ALTECON srl" },
	{ NANJING_PUTIAN_TELECOMMUNICATIONS, "NANJING_PUTIAN_TELECOMMUNICATIONS", "Nanjing Putian elecommunications Co." },
	{ TERRALUX, "TERRALUX", "Terralux" },
	{ MENRED, "MENRED", "MENRED" },
	{ IEXERGY_GMBH, "IEXERGY_GMBH", "iEXERGY GmbH" },
	{ OVENTROP_GMBH, "OVENTROP_GMBH", "Oventrop GmbH Co. KG" },
	{ BUILDING_AUTOMATION_PRODUCTS_INC, "BUILDING_AUTOMATION_PRODUCTS_INC", "Builing Automation Products" },
	{ FUNCTIONAL_DEVICES_INC, "FUNCTIONAL_DEVICES_INC", "Functional Devices, Inc." },
	{ OGGA, "OGGA", "OGGA" },
	{ ITHO_DAALDEROP, "ITHO_DAALDEROP", "itho daalderop" },
	{ RESOL, "RESOL", "Resol" },
	{ ADVANCED_DEVICES, "ADVANCED_DEVICES", "Advanced Devices" },
	{ AUTANI_LCC, "AUTANI_LCC", "Autani LLC." },
	{ DR_RIEDEL_GMBH, "DR_RIEDEL_GMBH", "Dr. Riedel GmbH" },
	{ HOPPE_HOLDING_AG, "HOPPE_HOLDING_AG", "HOPPE Holding AG" },
	{ SIEGENIA_AUBI_KG, "SIEGENIA_AUBI_KG", "SIEGENIA-AUBI KG" },
	{ ADEO_SERVICES, "ADEO_SERVICES", "ADEO Services" },
	{ EIMSIG_EFP_GMBH, "EIMSIG_EFP_GMBH", "EiMSIG EFP GmbH" },
	{ VIMAR_SPA, "VIMAR_SPA", "VIMAR S.p.a." },
	{ GLEN_DIMLAX_GMBH, "GLEN_DIMLAX_GMBH", "Glen Dimplex" },
	{ PMDM_GMBH, "PMDM_GMBH", "PMDM GmbH" },
	{ HUBBEL_LIGHTNING, "HUBBEL_LIGHTNING", "Hubbell Lighting" },
	{ DEBFLEX, "DEBFLEX", "Debflex S.A." },
	{ PERIFACTORY_SENSORSYSTEMS, "PERIFACTORY_SENSORSYSTEMS", "Perfactory Sensorsystems" },
	{ WATTY_CORP, "WATTY_CORP", "Watty Corporation" },
	{ WAGO_KONTAKTTECHNIK, "WAGO_KONTAKTTECHNIK", "WAGO Kontakttechnik GmbH Co. KG" },
	{ KESSEL, "KESSEL", "Kessel AG" },
	{ AUG_WINKHAUS, "AUG_WINKHAUS", "Aug. GmbH Co. KG" },
	{ DECELECT, "DECELECT", "DECELECT" },
	{ MST_INDUSTRIES, "MST_INDUSTRIES", "MST Industries" },
	{ BECKER_ANTRIEBS_GMBH, "BECKER_ANTRIEBS_GMBH", "Becker Antriebs GmbH" },
	{ NEXELEC, "NEXELEC", "Nexelec" },
	{ WIELAND_ELECTRIC_GMBH, "WIELAND_ELECTRIC_GMBH", "Wieland Electric GmbH" },
	{ ADVISEN, "ADVISEN", "AVIDSEN" },
	{ CWS_BCO_INTERNATIONAL_GMBH, "CWS_BCO_INTERNATIONAL_GMBH", "CWS-boco International GmbH" },
	{ ROTO_FRANCK_AG, "ROTO_FRANCK_AG", "Roto Frank AG" },
	{ ALM_CONTROLS_E_K, "ALM_CONTROLS_E_K", "ALM Controls e.k." },
	{ TOMMASO_TECHNOLOGIES_CO_LTD, "TOMMASO_TECHNOLOGIES_CO_LTD", "Tommaso Technologies Ltd." },
	{ REHAUS_AG_AND_CO, "REHAUS_AG_AND_CO", "Rehaus AG + Co." },
	{ INABA_DENKI_SANGYO_CO_LTD, "INABA_DENKI_SANGYO_CO_LTD", "Inaba Denki Sangyo Co. Ltd." },
	{ HAGER_CONTROL_SAS, "HAGER_CONTROL_SAS", "Hager Control SAS" },
	{ MULTI_USER_MANUFACTURER, "MULTI_USER_MANUFACTURER", "Multi user Manufacturer ID" },
	{ 0, nullptr, nullptr },
};

const char *CEnOceanEEP::GetManufacturerLabel(uint32_t ID)
{
	for (const _tManufacturerTable *pTable = _manufacturerTable; pTable->ID != 0 || pTable->label != nullptr; pTable++)
		if (pTable->ID == ID)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanEEP::GetManufacturerName(uint32_t ID)
{
	for (const _tManufacturerTable *pTable = _manufacturerTable; pTable->ID != 0 || pTable->name != nullptr; pTable++)
		if (pTable->ID == ID)
			return pTable->name;

	return ">>Unkown manufacturer... Please report!<<";
}

struct _tRORGTable
{
	uint8_t RORG;
	const char *label;
	const char *description;
};

static const _tRORGTable _RORGTable[] = { { RORG_ST, "ST", "Secure telegram" },
					  { RORG_ST_WE, "ST_WE", "Secure telegram with RORG encapsulation" },
					  { RORG_STT_FW, "STT_FW", "Secure teach-in telegram for switch" },
					  { RORG_4BS, "4BS", "4 Bytes Communication" },
					  { RORG_ADT, "ADT", "Adressing Destination Telegram" },
					  { RORG_SM_REC, "SM_REC", "Smart Ack Reclaim" },
					  { RORG_GP_SD, "GP_SD", "Generic Profiles selective data" },
					  { RORG_SM_LRN_REQ, "SM_LRN_REQ", "Smart Ack Learn Request" },
					  { RORG_SM_LRN_ANS, "SM_LRN_ANS", "Smart Ack Learn Answer" },
					  { RORG_SM_ACK_SGNL, "SM_ACK_SGNL", "Smart Acknowledge Signal telegram" },
					  { RORG_MSC, "MSC", "Manufacturer Specific Communication" },
					  { RORG_VLD, "VLD", "Variable length data telegram" },
					  { RORG_UTE, "UTE", "Universal teach-in EEP based" },
					  { RORG_1BS, "1BS", "1 Byte Communication" },
					  { RORG_RPS, "RPS", "Repeated Switch Communication" },
					  { RORG_SYS_EX, "SYS_EX", "Remote Management" },
					  { 0, nullptr, nullptr } };

const char *CEnOceanEEP::GetRORGLabel(uint8_t RORG)
{
	for (const _tRORGTable *pTable = _RORGTable; pTable->RORG; pTable++)
		if (pTable->RORG == RORG)
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanEEP::GetRORGDescription(uint8_t RORG)
{
	for (const _tRORGTable *pTable = _RORGTable; pTable->RORG; pTable++)
		if (pTable->RORG == RORG)
			return pTable->description;

	return ">>Unkown RORG... Please report!<<";
}

// EEP 2021-05-01 from EnOcean website

struct _tEEPTable
{
	const uint8_t RORG;
	const uint8_t func;
	const uint8_t type;
	const char* EEP;
	const char* description;
	const char* label;
};

static const _tEEPTable _EEPTable[] = {
	// A5, 4BS Telegrams

    // A5-02, Temperature Sensors
    { RORG_4BS, 0x02, 0x01, "A5-02-01", "Temperature Sensor Range -40C to 0C", "Temperature.01"},
    { RORG_4BS, 0x02, 0x02, "A5-02-02", "Temperature Sensor Range -30C to +10C", "Temperature.02"},
    { RORG_4BS, 0x02, 0x03, "A5-02-03", "Temperature Sensor Range -20C to +20C", "Temperature.03"},
    { RORG_4BS, 0x02, 0x04, "A5-02-04", "Temperature Sensor Range -10C to +30C", "Temperature.04"},
    { RORG_4BS, 0x02, 0x05, "A5-02-05", "Temperature Sensor Range 0C to +40C", "Temperature.05"},
    { RORG_4BS, 0x02, 0x06, "A5-02-06", "Temperature Sensor Range +10C to +50C", "Temperature.06"},
    { RORG_4BS, 0x02, 0x07, "A5-02-07", "Temperature Sensor Range +20C to +60C", "Temperature.07"},
    { RORG_4BS, 0x02, 0x08, "A5-02-08", "Temperature Sensor Range +30C to +70C", "Temperature.08"},
    { RORG_4BS, 0x02, 0x09, "A5-02-09", "Temperature Sensor Range +40C to +80C", "Temperature.09"},
    { RORG_4BS, 0x02, 0x0A, "A5-02-0A", "Temperature Sensor Range +50C to +90C", "Temperature.0A"},
    { RORG_4BS, 0x02, 0x0B, "A5-02-0B", "Temperature Sensor Range +60C to +100C", "Temperature.0B"},
    { RORG_4BS, 0x02, 0x10, "A5-02-10", "Temperature Sensor Range -60C to +20C", "Temperature.10"},
    { RORG_4BS, 0x02, 0x11, "A5-02-11", "Temperature Sensor Range -50C to +30C", "Temperature.11"},
    { RORG_4BS, 0x02, 0x12, "A5-02-12", "Temperature Sensor Range -40C to +40C", "Temperature.12"},
    { RORG_4BS, 0x02, 0x13, "A5-02-13", "Temperature Sensor Range -30C to +50C", "Temperature.13"},
    { RORG_4BS, 0x02, 0x14, "A5-02-14", "Temperature Sensor Range -20C to +60C", "Temperature.14"},
    { RORG_4BS, 0x02, 0x15, "A5-02-15", "Temperature Sensor Range -10C to +70C", "Temperature.15"},
    { RORG_4BS, 0x02, 0x16, "A5-02-16", "Temperature Sensor Range 0C to +80C", "Temperature.16"},
    { RORG_4BS, 0x02, 0x17, "A5-02-17", "Temperature Sensor Range +10C to +90C", "Temperature.17"},
    { RORG_4BS, 0x02, 0x18, "A5-02-18", "Temperature Sensor Range +20C to +100C", "Temperature.18"},
    { RORG_4BS, 0x02, 0x19, "A5-02-19", "Temperature Sensor Range +30C to +110C", "Temperature.19"},
    { RORG_4BS, 0x02, 0x1A, "A5-02-1A", "Temperature Sensor Range +40C to +120C", "Temperature.1A"},
    { RORG_4BS, 0x02, 0x1B, "A5-02-1B", "Temperature Sensor Range +50C to +130C", "Temperature.1B"},
    { RORG_4BS, 0x02, 0x20, "A5-02-20", "10 Bit Temperature Sensor Range -10C to +41.2C", "Temperature.20"},
    { RORG_4BS, 0x02, 0x30, "A5-02-30", "10 Bit Temperature Sensor Range -40C to +62.3C", "Temperature.30"},

    // A5-04, Temperature and Humidity Sensor
    { RORG_4BS, 0x04, 0x01, "A5-04-01", "Range 0C to +40C and 0% to 100%", "TempHum.01"},
    { RORG_4BS, 0x04, 0x02, "A5-04-02", "Range -20C to +60C and 0% to 100%", "TempHum.02"},
    { RORG_4BS, 0x04, 0x03, "A5-04-03", "Range -20C to +60C 10bit-measurement and 0% to 100%", "TempHum.03"},
    { RORG_4BS, 0x04, 0x04, "A5-04-04", "Temp. -40°C to +120°C 12bit, Humidity 0% to 100%", "TempHum.04"},

    // A5-05, Barometric Sensor
    { RORG_4BS, 0x05, 0x01, "A5-05-01", "Range 500 to 1150 hPa", "Barometric.01"},

    // A5-06, Light Sensor
    { RORG_4BS, 0x06, 0x01, "A5-06-01", "Range 300lx to 60.000lx", "LightSensor.01"},
    { RORG_4BS, 0x06, 0x02, "A5-06-02", "Range 0lx to 1.020lx", "LightSensor.02"},
    { RORG_4BS, 0x06, 0x03, "A5-06-03", "10-bit measurement (1-Lux resolution) with range 0lx to 1000lx", "LightSensor.03"},
    { RORG_4BS, 0x06, 0x04, "A5-06-04", "Curtain Wall Brightness Sensor", "LightSensor.04"},
    { RORG_4BS, 0x06, 0x05, "A5-06-05", "Range 0lx to 10.200lx", "LightSensor.05"},

    // A5-07, Occupancy Sensor
    { RORG_4BS, 0x07, 0x01, "A5-07-01", "Occupancy with Supply voltage monitor", "OccupancySensor.01"},
    { RORG_4BS, 0x07, 0x02, "A5-07-02", "Occupancy with Supply voltage monitor", "OccupancySensor.02"},
    { RORG_4BS, 0x07, 0x03, "A5-07-03", "Occupancy with Supply voltage monitor and 10-bit illumination measurement", "OccupancySensor.03"},

    // A5-08, Light, Temperature and Occupancy Sensor
    { RORG_4BS, 0x08, 0x01, "A5-08-01", "Range 0lx to 510lx, 0C to +51C and Occupancy Button", "TempOccupancySensor.01"},
    { RORG_4BS, 0x08, 0x02, "A5-08-02", "Range 0lx to 1020lx, 0C to +51C and Occupancy Button", "TempOccupancySensor.02"},
    { RORG_4BS, 0x08, 0x03, "A5-08-03", "Range 0lx to 1530lx, -30C to +50C and Occupancy Button", "TempOccupancySensor.03"},

    // A5-09, Gas Sensor
 	{ RORG_4BS, 0x09, 0x01, "A5-09-01", "CO Sensor (not in use)", "GasSensor.01" },
    { RORG_4BS, 0x09, 0x02, "A5-09-02", "CO Sensor 0 ppm to 1020 ppm", "GasSensor.02"},
    { RORG_4BS, 0x09, 0x04, "A5-09-04", "CO2 Sensor", "GasSensor.04"},
    { RORG_4BS, 0x09, 0x05, "A5-09-05", "VOC Sensor", "GasSensor.05"},
    { RORG_4BS, 0x09, 0x06, "A5-09-06", "Radon", "GasSensor.06"},
    { RORG_4BS, 0x09, 0x07, "A5-09-07", "Particles", "GasSensor.07"},
    { RORG_4BS, 0x09, 0x08, "A5-09-08", "Pure CO2 Sensor", "GasSensor.08"},
    { RORG_4BS, 0x09, 0x09, "A5-09-09", "Pure CO2 Sensor with Power Failure Detection", "GasSensor.09"},
    { RORG_4BS, 0x09, 0x0A, "A5-09-0A", "Hydrogen Gas Sensor", "GasSensor.0A"},
    { RORG_4BS, 0x09, 0x0B, "A5-09-0B", "Radioactivity Sensor", "GasSensor.0B"},
    { RORG_4BS, 0x09, 0x0C, "A5-09-0C", "VOC Sensor", "GasSensor.0C"},

    // A5-10, Room Operating Panel
    { RORG_4BS, 0x10, 0x01, "A5-10-01", "Temperature Sensor, Set Point, Fan Speed and Occupancy Control", "RoomOperatingPanel.01"},
    { RORG_4BS, 0x10, 0x02, "A5-10-02", "Temperature Sensor, Set Point, Fan Speed and Day/Night Control", "RoomOperatingPanel.02"},
    { RORG_4BS, 0x10, 0x03, "A5-10-03", "Temperature Sensor, Set Point Control", "RoomOperatingPanel.03"},
    { RORG_4BS, 0x10, 0x04, "A5-10-04", "Temperature Sensor, Set Point and Fan Speed Control", "RoomOperatingPanel.04"},
    { RORG_4BS, 0x10, 0x05, "A5-10-05", "Temperature Sensor, Set Point and Occupancy Control", "RoomOperatingPanel.05"},
    { RORG_4BS, 0x10, 0x06, "A5-10-06", "Temperature Sensor, Set Point and Day/Night Control", "RoomOperatingPanel.06"},
    { RORG_4BS, 0x10, 0x07, "A5-10-07", "Temperature Sensor, Fan Speed Control", "RoomOperatingPanel.07"},
    { RORG_4BS, 0x10, 0x08, "A5-10-08", "Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.08"},
    { RORG_4BS, 0x10, 0x09, "A5-10-09", "Temperature Sensor, Fan Speed and Day/Night Control", "RoomOperatingPanel.09"},
    { RORG_4BS, 0x10, 0x0A, "A5-10-0A", "Temperature Sensor, Set Point Adjust and Single Input Contact", "RoomOperatingPanel.0A"},
    { RORG_4BS, 0x10, 0x0B, "A5-10-0B", "Temperature Sensor and Single Input Contact", "RoomOperatingPanel.0B"},
    { RORG_4BS, 0x10, 0x0C, "A5-10-0C", "Temperature Sensor and Occupancy Control", "RoomOperatingPanel.0C"},
    { RORG_4BS, 0x10, 0x0D, "A5-10-0D", "Temperature Sensor and Day/Night Control", "RoomOperatingPanel.0D"},
    { RORG_4BS, 0x10, 0x10, "A5-10-10", "Temperature and Humidity Sensor, Set Point and Occupancy Control", "RoomOperatingPanel.10"},
    { RORG_4BS, 0x10, 0x11, "A5-10-11", "Temperature and Humidity Sensor, Set Point and Day/Night Control", "RoomOperatingPanel.11"},
    { RORG_4BS, 0x10, 0x12, "A5-10-12", "Temperature and Humidity Sensor and Set Point", "RoomOperatingPanel.12"},
    { RORG_4BS, 0x10, 0x13, "A5-10-13", "Temperature and Humidity Sensor, Occupancy Control", "RoomOperatingPanel.13"},
    { RORG_4BS, 0x10, 0x14, "A5-10-14", "Temperature and Humidity Sensor, Day/Night Control", "RoomOperatingPanel.14"},
    { RORG_4BS, 0x10, 0x15, "A5-10-15", "10 Bit Temperature Sensor, 6 bit Set Point Control", "RoomOperatingPanel.15"},
    { RORG_4BS, 0x10, 0x16, "A5-10-16", "10 Bit Temperature Sensor, 6 bit Set Point Control;Occupancy Control", "RoomOperatingPanel.16"},
    { RORG_4BS, 0x10, 0x17, "A5-10-17", "10 Bit Temperature Sensor, Occupancy Control", "RoomOperatingPanel.17"},
    { RORG_4BS, 0x10, 0x18, "A5-10-18", "Illumination, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.18"},
    { RORG_4BS, 0x10, 0x19, "A5-10-19", "Humidity, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.19"},
    { RORG_4BS, 0x10, 0x1A, "A5-10-1A", "Supply voltage monitor, Temperature Set Point, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.1A"},
    { RORG_4BS, 0x10, 0x1B, "A5-10-1B", "Supply Voltage Monitor, Illumination, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.1B"},
    { RORG_4BS, 0x10, 0x1C, "A5-10-1C", "Illumination, Illumination Set Point, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.1C"},
    { RORG_4BS, 0x10, 0x1D, "A5-10-1D", "Humidity, Humidity Set Point, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.1D"},
    { RORG_4BS, 0x10, 0x1E, "A5-10-1E", "Supply Voltage Monitor, Illumination, Temperature Sensor, Fan Speed and Occupancy Control", "RoomOperatingPanel.1B"}, // same as 1B
    { RORG_4BS, 0x10, 0x1F, "A5-10-1F", "Temperature Sensor, Set Point, Fan Speed, Occupancy and Unoccupancy Control", "RoomOperatingPanel.1F"},
    { RORG_4BS, 0x10, 0x20, "A5-10-20", "Temperature and Set Point with Special Heating States", "RoomOperatingPanel.20"},
    { RORG_4BS, 0x10, 0x21, "A5-10-21", "Temperature, Humidity and Set Point with Special Heating States", "RoomOperatingPanel.21"},
    { RORG_4BS, 0x10, 0x22, "A5-10-22", "Temperature, Setpoint, Humidity and Fan Speed", "RoomOperatingPanel.22"},
    { RORG_4BS, 0x10, 0x23, "A5-10-23", "Temperature, Setpoint, Humidity, Fan Speed and Occupancy", "RoomOperatingPanel.23"},

    // A5-11, Controller Status
    { RORG_4BS, 0x11, 0x01, "A5-11-01", "Lighting Controller", "ControllerStatus.01"},
    { RORG_4BS, 0x11, 0x02, "A5-11-02", "Temperature Controller Output", "ControllerStatus.02"},
    { RORG_4BS, 0x11, 0x03, "A5-11-03", "Blind Status", "ControllerStatus.03"},
    { RORG_4BS, 0x11, 0x04, "A5-11-04", "Extended Lighting Status", "ControllerStatus.04"},
    { RORG_4BS, 0x11, 0x05, "A5-11-05", "Dual-Channel Switch Actuator (BI-DIR)", "ControllerStatus.05"},

	// A5-12, Automated Meter Reading (AMR)
    { RORG_4BS, 0x12, 0x00, "A5-12-00", "Counter", "AMR.Counter"},
    { RORG_4BS, 0x12, 0x01, "A5-12-01", "Electricity", "AMR.Electricity"},
    { RORG_4BS, 0x12, 0x02, "A5-12-02", "Gas", "AMR.Gas"},
    { RORG_4BS, 0x12, 0x03, "A5-12-03", "Water", "AMR.Water"},
    { RORG_4BS, 0x12, 0x04, "A5-12-04", "Temperature and Load Sensor", "AMR.TempLoad"},
    { RORG_4BS, 0x12, 0x05, "A5-12-05", "Temperature and Container Sensor", "AMR.TempContainer"},
    { RORG_4BS, 0x12, 0x10, "A5-12-10", "Current meter 16 channels", "AMR.16Current"},

    // A5-13, Environmental Applications
    { RORG_4BS, 0x13, 0x01, "A5-13-01", "Weather Station", "EnvironmentalApplications.01"},
    { RORG_4BS, 0x13, 0x02, "A5-13-02", "Sun Intensity", "EnvironmentalApplications.02"},
    { RORG_4BS, 0x13, 0x03, "A5-13-03", "Date Exchange", "EnvironmentalApplications.03"},
    { RORG_4BS, 0x13, 0x04, "A5-13-04", "Time and Day Exchange", "EnvironmentalApplications.04"},
    { RORG_4BS, 0x13, 0x05, "A5-13-05", "Direction Exchange", "EnvironmentalApplications.05"},
    { RORG_4BS, 0x13, 0x06, "A5-13-06", "Geographic Position Exchange", "EnvironmentalApplications.06"},
    { RORG_4BS, 0x13, 0x07, "A5-13-07", "Wind Sensor", "EnvironmentalApplications.07"},
    { RORG_4BS, 0x13, 0x08, "A5-13-08", "Rain Sensor", "EnvironmentalApplications.08"},
    { RORG_4BS, 0x13, 0x10, "A5-13-10", "Sun position and radiation", "EnvironmentalApplications.10"},

    // A5-14, Multi-Func Sensor
    { RORG_4BS, 0x14, 0x01, "A5-14-01", "Single Input Contact (Window/Door), Supply voltage monitor", "MultiFuncSensor.01"},
    { RORG_4BS, 0x14, 0x02, "A5-14-02", "Single Input Contact (Window/Door), Supply voltage monitor and Illumination", "MultiFuncSensor.02"},
    { RORG_4BS, 0x14, 0x03, "A5-14-03", "Single Input Contact (Window/Door), Supply voltage monitor and Vibration", "MultiFuncSensor.03"},
    { RORG_4BS, 0x14, 0x04, "A5-14-04", "Single Input Contact (Window/Door), Supply voltage monitor, Vibration and Illumination", "MultiFuncSensor.04"},
    { RORG_4BS, 0x14, 0x05, "A5-14-05", "Vibration/Tilt, Supply voltage monitor", "MultiFuncSensor.05"},
    { RORG_4BS, 0x14, 0x06, "A5-14-06", "Vibration/Tilt, Illumination and Supply voltage monitor", "MultiFuncSensor.06"},
    { RORG_4BS, 0x14, 0x07, "A5-14-07", "Dual-door-contact with States Open/Closed and Locked/Unlocked, Supply voltage monitor", "MultiFuncSensor.07"},
    { RORG_4BS, 0x14, 0x08, "A5-14-08", "Dual-door-contact with States Open/Closed and Locked/Unlocked, Supply voltage monitor and Vibration detection", "MultiFuncSensor.08"},
    { RORG_4BS, 0x14, 0x09, "A5-14-09", "Window/Door-Sensor with States Open/Closed/Tilt, Supply voltage monitor", "MultiFuncSensor.09"},
    { RORG_4BS, 0x14, 0x0A, "A5-14-0A", "Window/Door-Sensor with States Open/Closed/Tilt, Supply voltage monitor and Vibration detection", "MultiFuncSensor.0A"},

    // A5-20, HVAC Components
    { RORG_4BS, 0x20, 0x01, "A5-20-01", "Battery Powered Actuator (BI-DIR)", "HVAC.01"},
    { RORG_4BS, 0x20, 0x02, "A5-20-02", "Basic Actuator (BI-DIR)", "HVAC.02"},
    { RORG_4BS, 0x20, 0x03, "A5-20-03", "Line powered Actuator (BI-DIR)", "HVAC.03"},
    { RORG_4BS, 0x20, 0x04, "A5-20-04", "Heating Radiator Valve Actuating Drive with Feed and Room Temperature Measurement, Local Set Point Control and Display (BI-DIR)", "HVAC.04"},
    { RORG_4BS, 0x20, 0x05, "A5-20-05", "Ventilation Unit (BI-DIR)", "HVAC.05"},
    { RORG_4BS, 0x20, 0x06, "A5-20-06", "Harvesting-powered actuator with local temperature offset control (BI-DIR)", "HVAC.06"},
   	{ RORG_4BS, 0x20, 0x10, "A5-20-10", "Generic HVAC Interface (BI-DIR)", "HVAC.10"},
    { RORG_4BS, 0x20, 0x11, "A5-20-11", "Generic HVAC Interface - Error Control (BI-DIR)", "HVAC.11"},
    { RORG_4BS, 0x20, 0x12, "A5-20-12", "Temperature Controller Input", "HVAC.12"},

    // A5-30, Digital Input
    { RORG_4BS, 0x30, 0x01, "A5-30-01", "Single Input Contact, Battery Monitor", "DigitalInput.01"},
    { RORG_4BS, 0x30, 0x02, "A5-30-02", "Single Input Contact", "DigitalInput.02"},
    { RORG_4BS, 0x30, 0x03, "A5-30-03", "4 Digital Inputs, Wake and Temperature", "DigitalInput.03"},
    { RORG_4BS, 0x30, 0x04, "A5-30-04", "3 Digital Inputs, 1 Digital Input 8 Bits", "DigitalInput.04"},
    { RORG_4BS, 0x30, 0x05, "A5-30-05", "Single Input Contact, Retransmission, Battery Monitor", "DigitalInput.05"},
    { RORG_4BS, 0x30, 0x06, "A5-30-06", "Single Alternate Input Contact, Retransmission, Heartbeat", "DigitalInput.06"},

    // A5-37, Energy Management
    { RORG_4BS, 0x37, 0x01, "A5-37-01", "Demand Response", "EnergyManagement.01"},

    // A5-38, Central Command
    { RORG_4BS, 0x38, 0x08, "A5-38-08", "Gateway", "CentralCommand.08"},
    { RORG_4BS, 0x38, 0x09, "A5-38-09", "Extended Lighting-Control", "CentralCommand.09"},

    // A5-3F, Universal
    { RORG_4BS, 0x3F, 0x00, "A5-3F-00", "Radio Link Test (BI-DIR)", "Universal.00"},
    { RORG_4BS, 0x3F, 0x7F, "A5-3F-7F", "Universal", "Universal.7F"},

	// D2, VLD Telegrams

    // D2-00, Room Control Panel (RCP)
    { RORG_VLD, 0x00, 0x01, "D2-00-01", "RCP with Temperature Measurement and Display (BI-DIR)", "RCP.01"},

    // D2-01, Electronic Switches and Dimmers with Local Control
    { RORG_VLD, 0x01, 0x00, "D2-01-00", "Type 0x00", "SwitchesDimmers.00"},
    { RORG_VLD, 0x01, 0x01, "D2-01-01", "Type 0x01", "SwitchesDimmers.01"},
    { RORG_VLD, 0x01, 0x02, "D2-01-02", "Type 0x02", "SwitchesDimmers.02"},
    { RORG_VLD, 0x01, 0x03, "D2-01-03", "Type 0x03", "SwitchesDimmers.03"},
    { RORG_VLD, 0x01, 0x04, "D2-01-04", "Type 0x04", "SwitchesDimmers.04"},
    { RORG_VLD, 0x01, 0x05, "D2-01-05", "Type 0x05", "SwitchesDimmers.05"},
    { RORG_VLD, 0x01, 0x06, "D2-01-06", "Type 0x06", "SwitchesDimmers.06"},
    { RORG_VLD, 0x01, 0x07, "D2-01-07", "Type 0x07", "SwitchesDimmers.07"},
    { RORG_VLD, 0x01, 0x08, "D2-01-08", "Type 0x08", "SwitchesDimmers.08"},
    { RORG_VLD, 0x01, 0x09, "D2-01-09", "Type 0x09", "SwitchesDimmers.09"},
    { RORG_VLD, 0x01, 0x0A, "D2-01-0A", "Type 0x0A", "SwitchesDimmers.0A"},
    { RORG_VLD, 0x01, 0x0B, "D2-01-0B", "Type 0x0B", "SwitchesDimmers.0B"},
    { RORG_VLD, 0x01, 0x0C, "D2-01-0C", "Type 0x0C", "SwitchesDimmers.0C"},
    { RORG_VLD, 0x01, 0x0D, "D2-01-0D", "Type 0x0D", "SwitchesDimmers.0D"},
    { RORG_VLD, 0x01, 0x0E, "D2-01-0E", "Type 0x0E", "SwitchesDimmers.0E"},
    { RORG_VLD, 0x01, 0x0F, "D2-01-0F", "Type 0x0F", "SwitchesDimmers.0F"},
    { RORG_VLD, 0x01, 0x10, "D2-01-10", "Type 0x10", "SwitchesDimmers.10"},
    { RORG_VLD, 0x01, 0x11, "D2-01-11", "Type 0x11", "SwitchesDimmers.11"},
    { RORG_VLD, 0x01, 0x12, "D2-01-12", "Type 0x12", "SwitchesDimmers.12"},
    { RORG_VLD, 0x01, 0x13, "D2-01-13", "Type 0x13", "SwitchesDimmers.13"},
    { RORG_VLD, 0x01, 0x14, "D2-01-14", "Type 0x14", "SwitchesDimmers.14"},
    { RORG_VLD, 0x01, 0x15, "D2-01-15", "Type 0x15", "SwitchesDimmers.15"},
    { RORG_VLD, 0x01, 0x16, "D2-01-16", "Type 0x16", "SwitchesDimmers.16"},

    // D2-02, Sensors for Temperature, Illumination, Occupancy And Smoke
    { RORG_VLD, 0x02, 0x00, "D2-02-00", "Type 0x00", "TempHumIllumOccupSmoke.00"},
    { RORG_VLD, 0x02, 0x01, "D2-02-01", "Type 0x01", "TempHumIllumOccupSmoke.01"},
    { RORG_VLD, 0x02, 0x02, "D2-02-02", "Type 0x02", "TempHumIllumOccupSmoke.02"},

    // D2-03, Light, Switching + Blind Control
    { RORG_VLD, 0x03, 0x00, "D2-03-00", "Type 0x00", "LightSwitchingBlind.00"},
    { RORG_VLD, 0x03, 0x0A, "D2-03-0A", "Push Button – Single Button", "LightSwitchingBlind.0A"},
    { RORG_VLD, 0x03, 0x10, "D2-03-10", "Mechanical Handle", "LightSwitchingBlind.10"},
    { RORG_VLD, 0x03, 0x20, "D2-03-20", "Beacon with Vibration Detection", "LightSwitchingBlind.20"},

    // D2-04, CO2, Humidity, Temperature, Day/Night and Autonomy
    { RORG_VLD, 0x04, 0x00, "D2-04-00", "Type 0x00", "CO2HumTempDayNightAuto.00"},
    { RORG_VLD, 0x04, 0x01, "D2-04-01", "Type 0x01", "CO2HumTempDayNightAuto.01"},
    { RORG_VLD, 0x04, 0x02, "D2-04-02", "Type 0x02", "CO2HumTempDayNightAuto.02"},
    { RORG_VLD, 0x04, 0x03, "D2-04-03", "Type 0x03", "CO2HumTempDayNightAuto.03"},
    { RORG_VLD, 0x04, 0x04, "D2-04-04", "Type 0x04", "CO2HumTempDayNightAuto.04"},
    { RORG_VLD, 0x04, 0x05, "D2-04-05", "Type 0x05", "CO2HumTempDayNightAuto.05"},
    { RORG_VLD, 0x04, 0x06, "D2-04-06", "Type 0x06", "CO2HumTempDayNightAuto.06"},
    { RORG_VLD, 0x04, 0x07, "D2-04-07", "Type 0x07", "CO2HumTempDayNightAuto.07"},
    { RORG_VLD, 0x04, 0x08, "D2-04-08", "Type 0x08", "CO2HumTempDayNightAuto.08"},
    { RORG_VLD, 0x04, 0x09, "D2-04-09", "Type 0x09", "CO2HumTempDayNightAuto.09"},
    { RORG_VLD, 0x04, 0x0A, "D2-04-0A", "Push Button – Single Button", "CO2HumTempDayNightAuto.0A"},
    { RORG_VLD, 0x04, 0x10, "D2-04-10", "Type 0x10", "CO2HumTempDayNightAuto.10"},
    { RORG_VLD, 0x04, 0x1A, "D2-04-1A", "Type 0x1A", "CO2HumTempDayNightAuto.1A"},
    { RORG_VLD, 0x04, 0x1B, "D2-04-1B", "Type 0x1B", "CO2HumTempDayNightAuto.1B"},
    { RORG_VLD, 0x04, 0x1C, "D2-04-1C", "Type 0x1C", "CO2HumTempDayNightAuto.1C"},
    { RORG_VLD, 0x04, 0x1D, "D2-04-1D", "Type 0x1D", "CO2HumTempDayNightAuto.1D"},
    { RORG_VLD, 0x04, 0x1E, "D2-04-1E", "Type 0x1E", "CO2HumTempDayNightAuto.1E"},
    { RORG_VLD, 0x04, 0x1F, "D2-04-1F", "CO2, Humidity, Temperature, Day/Night and Autonomy Type 0x1F", "CO2HumTempDayNightAuto.1F"},

    // D2-05, Blinds Control for Position and Angle
    { RORG_VLD, 0x05, 0x00, "D2-05-00", "Type 0x00", "BlindPosAngle.00"},
    { RORG_VLD, 0x05, 0x01, "D2-05-01", "Type 0x01", "BlindPosAngle.01"},
    { RORG_VLD, 0x05, 0x02, "D2-05-02", "Type 0x02", "BlindPosAngle.02"},
    { RORG_VLD, 0x05, 0x03, "D2-05-03", "Smart Window", "BlindPosAngle.03"},
    { RORG_VLD, 0x05, 0x04, "D2-05-04", "1-channel blind actuator", "BlindPosAngle.04"},
    { RORG_VLD, 0x05, 0x05, "D2-05-05", "4-channel blind actuator", "BlindPosAngle.05"},

    // D2-06, Multisensor Window / Door Handle and Sensors
    { RORG_VLD, 0x06, 0x01, "D2-06-01", "Alarm, Position Sensor, Vacation Mode, Optional Sensors", "WindowDoorHandle.01"},
    { RORG_VLD, 0x06, 0x10, "D2-06-10", "Sensor for intrusion detection at windows.", "WindowDoorHandle.10"},
    { RORG_VLD, 0x06, 0x20, "D2-06-20", "Electric Window Drive Controller", "WindowDoorHandle.20"},
    { RORG_VLD, 0x06, 0x40, "D2-06-40", "Lockable Window Handle with Status Reporting", "WindowDoorHandle.40"},
    { RORG_VLD, 0x06, 0x50, "D2-06-50", "Window Sash and Hardware Position Sensor", "WindowDoorHandle.50"},

    // D2-07, Locking Systems Control
    { RORG_VLD, 0x07, 0x00, "D2-07-00", "Locking Systems Control - Mortise lock", "LockingSystemsControl.07"},

    // D2-0A, Multichannel Temperature Sensor
    { RORG_VLD, 0x0A, 0x00, "D2-0A-00", "Type 0x00", "MultiTemp.00"},
    { RORG_VLD, 0x0A, 0x01, "D2-0A-01", "Type 0x01", "MultiTemp.01"},

    // D2-10, Room Control Panels with Temperature & Fan Speed Control, Room Status Information and Time Program
    { RORG_VLD, 0x10, 0x00, "D2-10-00", "Type 0x00", "RoomPanel.00"},
    { RORG_VLD, 0x10, 0x01, "D2-10-01", "Type 0x01", "RoomPanel.01"},
    { RORG_VLD, 0x10, 0x02, "D2-10-02", "Type 0x02", "RoomPanel.02"},
    { RORG_VLD, 0x10, 0x30, "D2-10-30", "Type 0x30", "RoomPanel.30"},
    { RORG_VLD, 0x10, 0x31, "D2-10-31", "Type 0x31", "RoomPanel.30"},
    { RORG_VLD, 0x10, 0x32, "D2-10-32", "Type 0x32", "RoomPanel.32"},

    // D2-11, Bidirectional Room Operating Panel
    { RORG_VLD, 0x11, 0x01, "D2-11-01", "Type 0x01", "BidirRoomPanel.01"},
    { RORG_VLD, 0x11, 0x02, "D2-11-02", "Type 0x02", "BidirRoomPanel.02"},
    { RORG_VLD, 0x11, 0x03, "D2-11-03", "Type 0x03", "BidirRoomPanel.03"},
    { RORG_VLD, 0x11, 0x04, "D2-11-04", "Type 0x04", "BidirRoomPanel.04"},
    { RORG_VLD, 0x11, 0x05, "D2-11-05", "Type 0x05", "BidirRoomPanel.05"},
    { RORG_VLD, 0x11, 0x06, "D2-11-06", "Type 0x06", "BidirRoomPanel.06"},
    { RORG_VLD, 0x11, 0x07, "D2-11-07", "Type 0x07", "BidirRoomPanel.07"},
    { RORG_VLD, 0x11, 0x08, "D2-11-08", "Type 0x08", "BidirRoomPanel.08"},
    { RORG_VLD, 0x11, 0x20, "D2-11-20", "Type 0x20", "BidirRoomPanel.20"},

    // D2-14, Multi Function Sensors
    { RORG_VLD, 0x14, 0x00, "D2-14-00", "Sensor for Temperature and Humidity, line-powered", "MultiFunction.00"},
    { RORG_VLD, 0x14, 0x01, "D2-14-01", "Sensor for Temperature, Humidity, Illumination and Button A and B, line-powered", "MultiFunction.01"},
    { RORG_VLD, 0x14, 0x02, "D2-14-02", "Sensor for VOC, line-powered", "MultiFunction.02"},
    { RORG_VLD, 0x14, 0x03, "D2-14-03", "Sensor for Temperature, Humidity and VOC, line-powered", "MultiFunction.03"},
    { RORG_VLD, 0x14, 0x04, "D2-14-04", "Sensor for Temperature, Humidity and Illumination, line-powered", "MultiFunction.04"},
    { RORG_VLD, 0x14, 0x05, "D2-14-05", "Sensor for Temperature, Humidity, Illumination and VOC, line-powered", "MultiFunction.05"},
    { RORG_VLD, 0x14, 0x06, "D2-14-06", "Sensor for CO2, line-powered", "MultiFunction.06"},
    { RORG_VLD, 0x14, 0x07, "D2-14-07", "Sensor for Temperature, Humidity and CO2, line-powered", "MultiFunction.07"},
    { RORG_VLD, 0x14, 0x08, "D2-14-08", "Sensor for Temperature, Humidity, Illumination and CO2, line-powered", "MultiFunction.08"},
    { RORG_VLD, 0x14, 0x09, "D2-14-09", "Sensor for Temperature, Humidity, VOC and CO2, line-powered", "MultiFunction.09"},
    { RORG_VLD, 0x14, 0x0A, "D2-14-0A", "Sensor for Temperature, Humidity, Illumination, VOC and CO2, line-powered", "MultiFunction.0A"},
    { RORG_VLD, 0x14, 0x0B, "D2-14-0B", "Sensor for Temperature, Humidity, and VOC, line-powered", "MultiFunction.0B"},
    { RORG_VLD, 0x14, 0x0C, "D2-14-0C", "Sensor for CO2, line-powered", "MultiFunction.0C"},
    { RORG_VLD, 0x14, 0x0D, "D2-14-0D", "Sensor for Temperature, Humidity and VOC and Button A, line-powered", "MultiFunction.0D"},
    { RORG_VLD, 0x14, 0x0E, "D2-14-0E", "Sensor for Temperature, Humidity, VOC, CO2 and Button A, line-powered", "MultiFunction.0E"},
    { RORG_VLD, 0x14, 0x0F, "D2-14-0F", "Sensor for Temperature, Humidity and Barometric Pressure, line-powered", "MultiFunction.0F"},
    { RORG_VLD, 0x14, 0x10, "D2-14-10", "Sensor for Temperature, Humidity, VOC and Presence, line-powered", "MultiFunction.10"},
    { RORG_VLD, 0x14, 0x1A, "D2-14-1A", "Sensor for Temperature, Humidity and Energy Storage", "MultiFunction.1A"},
    { RORG_VLD, 0x14, 0x1B, "D2-14-1B", "Sensor for Temperature, Humidity, Illumination and Energy Storage", "MultiFunction.1B"},
    { RORG_VLD, 0x14, 0x1C, "D2-14-1C", "Sensor for Temperature, Humidity, Energy Storage and Barometric Pressure", "MultiFunction.1C"},
    { RORG_VLD, 0x14, 0x1D, "D2-14-1D", "Sensor for Temperature, Humidity, Illumination, Energy Storage and Barometric Pressure", "MultiFunction.1D"},
    { RORG_VLD, 0x14, 0x20, "D2-14-20", "Outdoor Sensor for Temperature, Humidity, Illumination and Energy Storage", "MultiFunction.20"},
    { RORG_VLD, 0x14, 0x21, "D2-14-21", "Outdoor Sensor for Temperature and Energy Storage", "MultiFunction.21"},
    { RORG_VLD, 0x14, 0x22, "D2-14-22", "Outdoor Sensor for Temperature, Humidity and Energy Storage", "MultiFunction.22"},
    { RORG_VLD, 0x14, 0x23, "D2-14-23", "Outdoor Sensor for Temperature, Illumination and Energy Storage", "MultiFunction.23"},
    { RORG_VLD, 0x14, 0x24, "D2-14-24", "Outdoor Sensor for Illumination and Energy Storage", "MultiFunction.24"},
    { RORG_VLD, 0x14, 0x25, "D2-14-25", "Light Intensity and Color Temperature Sensor", "MultiFunction.25"},
    { RORG_VLD, 0x14, 0x30, "D2-14-30", "Sensor for Smoke, Air quality, Hygrothermal comfort, Temperature and Humidity", "MultiFunction.30"},
    { RORG_VLD, 0x14, 0x31, "D2-14-31", "Sensor for CO, Air quality, Hygrothermal comfort, Temperature and Humidity", "MultiFunction.31"},
    { RORG_VLD, 0x14, 0x40, "D2-14-40", "Indoor -Temperature, Humidity XYZ Acceleration, Illumination Sensor", "MultiFunction.40"},
    { RORG_VLD, 0x14, 0x41, "D2-14-41", "Indoor -Temperature, Humidity XYZ Acceleration, Illumination Sensor", "MultiFunction.41"},
    { RORG_VLD, 0x14, 0x50, "D2-14-50", "Basic Water Properties Sensor, Temperature and PH", "MultiFunction.50"},
    { RORG_VLD, 0x14, 0x51, "D2-14-51", "Basic Water Properties Sensor, Temperature and Dissolved Oxygen", "MultiFunction.51"},
    { RORG_VLD, 0x14, 0x52, "D2-14-52", "Sound, Pressure, Illumination, Presence and Temperature Sensor", "MultiFunction.52"},
    { RORG_VLD, 0x14, 0x53, "D2-14-53", "Leak Detector", "MultiFunction.53"},
    { RORG_VLD, 0x14, 0x54, "D2-14-54", "Leak Detector with Temperature and Humidity Sensing", "MultiFunction.54"},
    { RORG_VLD, 0x14, 0x55, "D2-14-55", "Power Use Monitor", "MultiFunction.55"},

    // D2-15, People Activity
    { RORG_VLD, 0x15, 0x00, "D2-15-00", "People Activity Counter", "PeopleActivity.15"},

    // D2-20, Fan Control
    { RORG_VLD, 0x20, 0x00, "D2-20-00", "Type 0x00", "FanCtrl.00"},
    { RORG_VLD, 0x20, 0x01, "D2-20-01", "Type 0x01", "FanCtrl.01"},
    { RORG_VLD, 0x20, 0x02, "D2-20-02", "Type 0x02", "FanCtrl.02"},

    // D2-30, Floor Heating Controls and Automated Meter Reading
    { RORG_VLD, 0x30, 0x00, "D2-30-00", "Type 0x00", "FloorHeating.00"},
    { RORG_VLD, 0x30, 0x01, "D2-30-01", "Type 0x01", "FloorHeating.01"},
    { RORG_VLD, 0x30, 0x02, "D2-30-02", "Type 0x02", "FloorHeating.02"},
    { RORG_VLD, 0x30, 0x03, "D2-30-03", "Type 0x03", "FloorHeating.03"},
    { RORG_VLD, 0x30, 0x04, "D2-30-04", "Type 0x04", "FloorHeating.04"},
    { RORG_VLD, 0x30, 0x05, "D2-30-05", "Type 0x05", "FloorHeating.05"},
    { RORG_VLD, 0x30, 0x06, "D2-30-06", "Type 0x06", "FloorHeating.06"},

    // D2-31, Automated Meter Reading Gateway
    { RORG_VLD, 0x31, 0x00, "D2-31-00", "Type 0x00", "AutoMeterReading.00"},
    { RORG_VLD, 0x31, 0x01, "D2-31-01", "Type 0x01", "AutoMeterReading.010"},

    // D2-32, A.C. Current Clamp
    { RORG_VLD, 0x32, 0x00, "D2-32-00", "Type 0x00", "ACCurrent.00"},
    { RORG_VLD, 0x32, 0x01, "D2-32-01", "Type 0x01", "ACCurrent.01"},
    { RORG_VLD, 0x32, 0x02, "D2-32-02", "Type 0x02", "ACCurrent.02"},

    // D2-33, Intelligent, Bi-directional Heaters and Controllers
    { RORG_VLD, 0x33, 0x00, "D2-33-00", "Type 0x00", "BidirHeatersControllers.00"},

    // D2-34, Heating Actuator
    { RORG_VLD, 0x34, 0x00, "D2-34-00", "1 Output Channel", "HeatingActuator.00"},
    { RORG_VLD, 0x34, 0x01, "D2-34-01", "2 Output Channels", "HeatingActuator.x2.01"},
    { RORG_VLD, 0x34, 0x02, "D2-34-02", "8 Output Channels", "HeatingActuator.x8.02"},

    // D2-40, LED Controller Status
    { RORG_VLD, 0x40, 0x00, "D2-40-00", "Type 0x00", "LEDControlStatus.00"},
    { RORG_VLD, 0x40, 0x01, "D2-40-01", "Type 0x01", "LEDControlStatus.01"},

    // D2-50, Heat Recovery Ventilation
    { RORG_VLD, 0x50, 0x00, "D2-50-00", "Type 0x00", "HeatRecovVent.00"},
    { RORG_VLD, 0x50, 0x01, "D2-50-01", "Type 0x01", "HeatRecovVent.01"},
    { RORG_VLD, 0x50, 0x10, "D2-50-10", "Type 0x10", "HeatRecovVent.10"},
    { RORG_VLD, 0x50, 0x11, "D2-50-11", "Type 0x11", "HeatRecovVent.11"},

    // D2-A0, Standard Valve
    { RORG_VLD, 0xA0, 0x01, "D2-A0-01", "Valve Control (BI-DIR)", "BidirValveCtrl.01"},

    // D2-B0, Liquid Leakage Sensor
    { RORG_VLD, 0xB0, 0x51, "D2-B0-51", "Mechanic Harvester", "MechanicHarvester.51"},

    // D2-B1, Level Sensor
    { RORG_VLD, 0xB1, 0x00, "D2-B1-00", "Level Sensor Dispenser", "LevelSensor.00"},

    // D5, 1BS Telegrams

    // D5-00, Contacts and Switches
    { RORG_1BS, 0x00, 0x01, "D5-00-01", "Single Input Contact", "ContactSwitch.01"},

    // F6, RPS Telegrams

    // F6-01, Switch Buttons
    { RORG_RPS, 0x01, 0x01, "F6-01-01", "Push Button", "SwitchButton.0"},

    // F6-02, Rocker Switch, 2 Rocker
    { RORG_RPS, 0x02, 0x01, "F6-02-01", "Light and Blind Control - Application Style 1", "LightBlind2.01"},
    { RORG_RPS, 0x02, 0x02, "F6-02-02", "Light and Blind Control - Application Style 2", "LightBlind2.02"},
    { RORG_RPS, 0x02, 0x03, "F6-02-03", "Light Control - Application Style 1", "LightBlind2.03"},
    { RORG_RPS, 0x02, 0x04, "F6-02-04", "Light and blind control ERP2", "LightBlind.04"},

    // F6-03, Rocker Switch, 4 Rocker
    { RORG_RPS, 0x03, 0x01, "F6-03-01", "Light and Blind Control - Application Style 1", "LightBlind4.01"},
    { RORG_RPS, 0x03, 0x02, "F6-03-02", "Light and Blind Control - Application Style 2", "LightBlind4.02"},

    // F6-04, Position Switch, Home and Office Application
    { RORG_RPS, 0x04, 0x01, "F6-04-01", "Key Card Activated Switch", "KeyCardSwitch.01"},
    { RORG_RPS, 0x04, 0x02, "F6-04-02", "Key Card Activated Switch ERP2", "KeyCardSwitch.02"},

    // F6-05, Detectors
    { RORG_RPS, 0x05, 0x00, "F6-05-00", "Wind Speed Threshold Detector", "Detector.00"},
    { RORG_RPS, 0x05, 0x01, "F6-05-01", "Liquid Leakage Sensor (mechanic harvester)", "Detector.01"},
    { RORG_RPS, 0x05, 0x02, "F6-05-02", "Smoke Detector", "Detector.02"},

    // F6-10, Mechanical Handle
    { RORG_RPS, 0x10, 0x00, "F6-10-00", "Window Handle", "WindowHandle.00"},
    { RORG_RPS, 0x10, 0x01, "F6-10-01", "Window Handle ERP2", "WindowHandle.01"},

	// End of table
	{ 0, 0, 0, nullptr, nullptr, nullptr },
};

const char *CEnOceanEEP::GetEEP(const int RORG, const int func, const int type)
{
	for (const _tEEPTable *pTable = (const _tEEPTable *)&_EEPTable; pTable->RORG; pTable++)
		if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
			return pTable->EEP;

	return nullptr;
}

const char *CEnOceanEEP::GetEEPLabel(const int RORG, const int func, const int type)
{
	for (const _tEEPTable *pTable = (const _tEEPTable *)&_EEPTable; pTable->RORG; pTable++)
		if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
			return pTable->label;

	return "UNKNOWN";
}

const char *CEnOceanEEP::GetEEPDescription(const int RORG, const int func, const int type)
{
	for (const _tEEPTable *pTable = (const _tEEPTable *)&_EEPTable; pTable->RORG; pTable++)
		if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
			return pTable->description;

	return ">>Unkown EEP... Please report!<<";
}

uint32_t CEnOceanEEP::GetINodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0)
{
	return (uint32_t) ((ID3 << 24) | (ID2 << 16) | (ID1 << 8) | ID0);
}

std::string CEnOceanEEP::GetNodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0)
{
	char szNodeID[10];
	sprintf(szNodeID, "%02X%02X%02X%02X", ID3, ID2, ID1, ID0);
	std::string nodeID = szNodeID;
	return nodeID;
}

std::string CEnOceanEEP::GetNodeID(const uint32_t iNodeID)
{
	char szNodeID[10];
	sprintf(szNodeID, "%08X", iNodeID);
	std::string nodeID = szNodeID;
	return nodeID;
}

std::string CEnOceanEEP::GetNodeID(const std::string deviceID)
{
	// DeviceID to NodeID
    // Pad to 8 characters inserting leading '0' if necessary
	std::stringstream s_strid;
	s_strid << std::setw(8) << std::setfill('0') << deviceID;
	std::string nodeID;
	s_strid >> nodeID;
	return nodeID;
}

std::string CEnOceanEEP::GetDeviceID(const std::string nodeID)
{
	// NodeID to DeviceID
    // If a leading '0' if present, remove it and pad to 7 characters
    // else deviceID = nodeID
	return (nodeID[0] == '0') ? nodeID.substr(1, nodeID.length() - 1) : nodeID;
}

float CEnOceanEEP::GetDeviceValue(const uint32_t rawValue, const uint32_t rangeMin, const uint32_t rangeMax, const float scaleMin, const float scaleMax)
{
	if (rangeMax == rangeMin) // Should never happend
		return (scaleMax + scaleMin) / 2.0F;

    if (rangeMin > rangeMax)
        return GetDeviceValue(rawValue, rangeMax, rangeMin, scaleMax, scaleMin);

    uint32_t validRawValue;

    if (rawValue < rangeMin)
        validRawValue = rangeMin;
    else if (rawValue > rangeMax)
        validRawValue = rangeMax;
    else
        validRawValue = rawValue;

 	float multiplyer = (scaleMax - scaleMin) / ((float) (rangeMax - rangeMin));

 	return multiplyer * ((float) (validRawValue - rangeMin)) + scaleMin;
}
