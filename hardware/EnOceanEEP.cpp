#include "stdafx.h"

#include "../main/Logger.h"
#include "../main/WebServer.h"

#include "EnOceanEEP.h"

#include "../main/Logger.h"

namespace enocean
{
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
		{ DOMOTICZ_MANUFACTURER, "DOMOTICZ", "Domoticz" },
		{ MULTI_USER_MANUFACTURER, "MULTI_USER_MANUFACTURER", "Multi user Manufacturer ID" },
		{ 0, nullptr, nullptr },
	};

	const char* CEnOceanEEP::GetManufacturerLabel(uint32_t ID)
	{
		for (const _tManufacturerTable* pTable = _manufacturerTable; pTable->ID != 0 || pTable->label != nullptr; pTable++)
			if (pTable->ID == ID)
				return pTable->label;

		return "UNKNOWN";
	}

	const char* CEnOceanEEP::GetManufacturerName(uint32_t ID)
	{
		for (const _tManufacturerTable* pTable = _manufacturerTable; pTable->ID != 0 || pTable->name != nullptr; pTable++)
			if (pTable->ID == ID)
				return pTable->name;

		return ">>Unkown manufacturer... Please report!<<";
	}

	struct _tRORGTable
	{
		uint8_t RORG;
		const char* label;
		const char* description;
	};

	static const _tRORGTable _RORGTable[] =
	{
		{UNKNOWN_RORG, "UNKNOWN", "Unknown RORG"},
		{RORG_ST, "ST", "Secure telegram"},
		{RORG_ST_WE, "ST_WE", "Secure telegram with RORG encapsulation"},
		{RORG_STT_FW, "STT_FW", "Secure teach-in telegram for switch"},
		{RORG_4BS, "4BS", "4 Bytes Communication"},
		{RORG_ADT, "ADT", "Adressing Destination Telegram"},
		{RORG_SM_REC, "SM_REC", "Smart Ack Reclaim"},
		{RORG_GP_SD, "GP_SD", "Generic Profiles selective data"},
		{RORG_SM_LRN_REQ, "SM_LRN_REQ", "Smart Ack Learn Request"},
		{RORG_SM_LRN_ANS, "SM_LRN_ANS", "Smart Ack Learn Answer"},
		{RORG_SM_ACK_SGNL, "SM_ACK_SGNL", "Smart Acknowledge Signal telegram"},
		{RORG_MSC, "MSC", "Manufacturer Specific Communication"},
		{RORG_VLD, "VLD", "Variable length data telegram"},
		{RORG_UTE, "UTE", "Universal teach-in EEP based"},
		{RORG_1BS, "1BS", "1 Byte Communication"},
		{RORG_RPS, "RPS", "Repeated Switch Communication"},
		{RORG_SYS_EX, "SYS_EX", "Remote Management"},
		{UNKNOWN_RORG, nullptr, nullptr},
	};

	const char* CEnOceanEEP::GetRORGLabel(uint8_t RORG)
	{
		for (const _tRORGTable* pTable = _RORGTable; pTable->RORG != UNKNOWN_RORG || pTable->label != nullptr; pTable++)
			if (pTable->RORG == RORG)
				return pTable->label;

		return "UNKNOWN";
	}

	const char* CEnOceanEEP::GetRORGDescription(uint8_t RORG)
	{
		for (const _tRORGTable* pTable = _RORGTable; pTable->RORG != UNKNOWN_RORG || pTable->description != nullptr; pTable++)
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
		{ RORG_4BS, 0x02, 0x01, "A5-02-01", "Temperature (-40..0°C)", "Temperature.01"},
		{ RORG_4BS, 0x02, 0x02, "A5-02-02", "Temperature (-30..10°C)", "Temperature.02"},
		{ RORG_4BS, 0x02, 0x03, "A5-02-03", "Temperature (-20..20°C)", "Temperature.03"},
		{ RORG_4BS, 0x02, 0x04, "A5-02-04", "Temperature (-10..30°C)", "Temperature.04"},
		{ RORG_4BS, 0x02, 0x05, "A5-02-05", "Temperature (0..40°C)", "Temperature.05"},
		{ RORG_4BS, 0x02, 0x06, "A5-02-06", "Temperature (10..50°C)", "Temperature.06"},
		{ RORG_4BS, 0x02, 0x07, "A5-02-07", "Temperature (20..60°C)", "Temperature.07"},
		{ RORG_4BS, 0x02, 0x08, "A5-02-08", "Temperature (30..70°C)", "Temperature.08"},
		{ RORG_4BS, 0x02, 0x09, "A5-02-09", "Temperature (40..80°C)", "Temperature.09"},
		{ RORG_4BS, 0x02, 0x0A, "A5-02-0A", "Temperature (50..90°C)", "Temperature.0A"},
		{ RORG_4BS, 0x02, 0x0B, "A5-02-0B", "Temperature (60..100°C)", "Temperature.0B"},
		{ RORG_4BS, 0x02, 0x10, "A5-02-10", "Temperature (-60..20°C)", "Temperature.10"},
		{ RORG_4BS, 0x02, 0x11, "A5-02-11", "Temperature (-50..30°C)", "Temperature.11"},
		{ RORG_4BS, 0x02, 0x12, "A5-02-12", "Temperature (-40..40°C)", "Temperature.12"},
		{ RORG_4BS, 0x02, 0x13, "A5-02-13", "Temperature (-30..50°C)", "Temperature.13"},
		{ RORG_4BS, 0x02, 0x14, "A5-02-14", "Temperature (-20..60°C)", "Temperature.14"},
		{ RORG_4BS, 0x02, 0x15, "A5-02-15", "Temperature (-10..70°C)", "Temperature.15"},
		{ RORG_4BS, 0x02, 0x16, "A5-02-16", "Temperature (0..80°C)", "Temperature.16"},
		{ RORG_4BS, 0x02, 0x17, "A5-02-17", "Temperature (10..90°C)", "Temperature.17"},
		{ RORG_4BS, 0x02, 0x18, "A5-02-18", "Temperature (20..100°C)", "Temperature.18"},
		{ RORG_4BS, 0x02, 0x19, "A5-02-19", "Temperature (30..110°C)", "Temperature.19"},
		{ RORG_4BS, 0x02, 0x1A, "A5-02-1A", "Temperature (40..120°C)", "Temperature.1A"},
		{ RORG_4BS, 0x02, 0x1B, "A5-02-1B", "Temperature (50..130°C)", "Temperature.1B"},
		{ RORG_4BS, 0x02, 0x20, "A5-02-20", "Temperature (-10..41.2°C)", "Temperature.20"},
		{ RORG_4BS, 0x02, 0x30, "A5-02-30", "Temperature (-40..62.3°C)", "Temperature.30"},

		// A5-04, Temperature and Humidity Sensor
		{ RORG_4BS, 0x04, 0x01, "A5-04-01", "Temperature (0..40°C), Humidity (0..100%)", "TempHum.01"},
		{ RORG_4BS, 0x04, 0x02, "A5-04-02", "Temperature (-20..60°C), Humidity (0..100%)", "TempHum.02"},
		{ RORG_4BS, 0x04, 0x03, "A5-04-03", "Temperature (-20..60°C), Humidity (0..100%)", "TempHum.03"},
		{ RORG_4BS, 0x04, 0x04, "A5-04-04", "Temperature (-40..120°C), Humidity (0..100%)", "TempHum.04"},

		// A5-05, Barometric Sensor
		{ RORG_4BS, 0x05, 0x01, "A5-05-01", "Barometric sensor (500..1150hPa)", "Barometric.01"},

		// A5-06, Light Sensor
		{ RORG_4BS, 0x06, 0x01, "A5-06-01", "Illumination (300..60000lx)", "LightSensor.01"},
		{ RORG_4BS, 0x06, 0x02, "A5-06-02", "Illumination (0..1020lx)", "LightSensor.02"},
		{ RORG_4BS, 0x06, 0x03, "A5-06-03", "Illumination (0..1000lx)", "LightSensor.03"},
		{ RORG_4BS, 0x06, 0x04, "A5-06-04", "Illumination (0..65535x), Temperature (-20..60°C)", "LightSensor.04"},
		{ RORG_4BS, 0x06, 0x05, "A5-06-05", "Illumination (0..10200lx)", "LightSensor.05"},

		// A5-07, Occupancy Sensor
		{ RORG_4BS, 0x07, 0x01, "A5-07-01", "Occupancy sensor", "OccupancySensor.01"},
		{ RORG_4BS, 0x07, 0x02, "A5-07-02", "Occupancy sensor", "OccupancySensor.02"},
		{ RORG_4BS, 0x07, 0x03, "A5-07-03", "Occupancy sensor, Illumination (0..1000lx)", "OccupancySensor.03"},

		// A5-08, Light, Temperature and Occupancy Sensor
		{ RORG_4BS, 0x08, 0x01, "A5-08-01", "Illumination (0..510lx), Temperature (0..51°C), Occupancy", "TempOccupancySensor.01"},
		{ RORG_4BS, 0x08, 0x02, "A5-08-02", "Illumination (0..1020lx), Temperature (0..51°C), Occupancy", "TempOccupancySensor.02"},
		{ RORG_4BS, 0x08, 0x03, "A5-08-03", "Illumination (0..1530lx), Temperature (-30..50°C), Occupancy", "TempOccupancySensor.03"},

		// A5-09, Gas Sensor
		{ RORG_4BS, 0x09, 0x01, "A5-09-01", "CO sensor", "GasSensor.01" },
		{ RORG_4BS, 0x09, 0x02, "A5-09-02", "CO sensor (0..1020ppm), Temperature (0..51°C)", "GasSensor.02"},
		{ RORG_4BS, 0x09, 0x04, "A5-09-04", "CO2 sensor (0..2550ppm), Temperature (0..51°C), Humidity (0..100%)", "GasSensor.04"},
		{ RORG_4BS, 0x09, 0x05, "A5-09-05", "VOC sensor (0..65535ppb)", "GasSensor.05"},
		{ RORG_4BS, 0x09, 0x06, "A5-09-06", "Radon sensor (0..1023Bq/m3)", "GasSensor.06"},
		{ RORG_4BS, 0x09, 0x07, "A5-09-07", "Particles sensor (0..511µg/m3)", "GasSensor.07"},
		{ RORG_4BS, 0x09, 0x08, "A5-09-08", "Pure CO2 sensor (0..2000ppm)", "GasSensor.08"},
		{ RORG_4BS, 0x09, 0x09, "A5-09-09", "Pure CO2 sensor (0..2000ppm)", "GasSensor.09"},
		{ RORG_4BS, 0x09, 0x0A, "A5-09-0A", "H2 sensor (0..65535ppm), Temperature (-20..60°C)", "GasSensor.0A"},
		{ RORG_4BS, 0x09, 0x0B, "A5-09-0B", "Radioactivity sensor (0..6553unit)", "GasSensor.0B"},
		{ RORG_4BS, 0x09, 0x0C, "A5-09-0C", "VOC sensor (0..65535unit)", "GasSensor.0C"},

		// A5-10, Room Operating Panel
		{ RORG_4BS, 0x10, 0x01, "A5-10-01", "Temperature/Set point (0..40°C), Fan control, Occupancy", "RoomOperatingPanel.01"},
		{ RORG_4BS, 0x10, 0x02, "A5-10-02", "Temperature/Set point (0..40°C), Fan control, Day/Night control", "RoomOperatingPanel.02"},
		{ RORG_4BS, 0x10, 0x03, "A5-10-03", "Temperature/Set point (0..40°C)", "RoomOperatingPanel.03"},
		{ RORG_4BS, 0x10, 0x04, "A5-10-04", "Temperature/Set point (0..40°C), Fan control", "RoomOperatingPanel.04"},
		{ RORG_4BS, 0x10, 0x05, "A5-10-05", "Temperature/Set point (0..40°C), Occupancy", "RoomOperatingPanel.05"},
		{ RORG_4BS, 0x10, 0x06, "A5-10-06", "Temperature/Set point (0..40°C), Day/Night control", "RoomOperatingPanel.06"},
		{ RORG_4BS, 0x10, 0x07, "A5-10-07", "Temperature (0..40°C), Fan control", "RoomOperatingPanel.07"},
		{ RORG_4BS, 0x10, 0x08, "A5-10-08", "Temperature (0..40°C), Fan control, Occupancy", "RoomOperatingPanel.08"},
		{ RORG_4BS, 0x10, 0x09, "A5-10-09", "Temperature (0..40°C), Fan control, Day/Night control", "RoomOperatingPanel.09"},
		{ RORG_4BS, 0x10, 0x0A, "A5-10-0A", "Temperature/Set point (0..40°C), Single input contact", "RoomOperatingPanel.0A"},
		{ RORG_4BS, 0x10, 0x0B, "A5-10-0B", "Temperature (0..40°C), Single input contact", "RoomOperatingPanel.0B"},
		{ RORG_4BS, 0x10, 0x0C, "A5-10-0C", "Temperature (0..40°C), Occupancy", "RoomOperatingPanel.0C"},
		{ RORG_4BS, 0x10, 0x0D, "A5-10-0D", "Temperature (0..40°C), Day/Night control", "RoomOperatingPanel.0D"},
		{ RORG_4BS, 0x10, 0x10, "A5-10-10", "Temperature/Set point (0..40°C), Humidity (0..100%), Occupancy", "RoomOperatingPanel.10"},
		{ RORG_4BS, 0x10, 0x11, "A5-10-11", "Temperature/Set point (0..40°C), Humidity (0..100%), Day/Night control", "RoomOperatingPanel.11"},
		{ RORG_4BS, 0x10, 0x12, "A5-10-12", "Temperature/Set point (0..40°C), Humidity (0..100%)", "RoomOperatingPanel.12"},
		{ RORG_4BS, 0x10, 0x13, "A5-10-13", "Temperature (0..40°C), Humidity (0..100%), Occupancy", "RoomOperatingPanel.13"},
		{ RORG_4BS, 0x10, 0x14, "A5-10-14", "Temperature (0..40°C), Humidity (0..100%), Day/Night control", "RoomOperatingPanel.14"},
		{ RORG_4BS, 0x10, 0x15, "A5-10-15", "Temperature/Set point (-10..41.2°C)", "RoomOperatingPanel.15"},
		{ RORG_4BS, 0x10, 0x16, "A5-10-16", "Temperature/Set point (-10..41.2°C), Occupancy", "RoomOperatingPanel.16"},
		{ RORG_4BS, 0x10, 0x17, "A5-10-17", "Temperature (-10..41.2°C), Occupancy", "RoomOperatingPanel.17"},
		{ RORG_4BS, 0x10, 0x18, "A5-10-18", "Temperature/Set point (0..40°C), Illumination (0..1000lx), Fan control, Occupancy", "RoomOperatingPanel.18"},
		{ RORG_4BS, 0x10, 0x19, "A5-10-19", "Temperature/Set point (0..40°C), Humidity (0..100%), Fan control, Occupancy", "RoomOperatingPanel.19"},
		{ RORG_4BS, 0x10, 0x1A, "A5-10-1A", "Temperature/Set point (0..40°C), Fan control, Occupancy", "RoomOperatingPanel.1A"},
		{ RORG_4BS, 0x10, 0x1B, "A5-10-1B", "Temperature (0..40°C), Illumination (0..1000lx), Fan control, Occupancy", "RoomOperatingPanel.1B"},
		{ RORG_4BS, 0x10, 0x1C, "A5-10-1C", "Temperature (0..40°C), Illumination/Set point (0..1000lx), Fan control, Occupancy", "RoomOperatingPanel.1C"},
		{ RORG_4BS, 0x10, 0x1D, "A5-10-1D", "Temperature (0..40°C), Humidity/Set point (0..100%), Fan control, Occupancy", "RoomOperatingPanel.1D"},
		{ RORG_4BS, 0x10, 0x1E, "A5-10-1E", "Temperature (0..40°C), Illumination (0..1000lx), Fan control, Occupancy", "RoomOperatingPanel.1B"}, // same as 1B
		{ RORG_4BS, 0x10, 0x1F, "A5-10-1F", "Temperature/Set point (0..40°C), Fan control, Occupancy, Uunoccupancy", "RoomOperatingPanel.1F"},
		{ RORG_4BS, 0x10, 0x20, "A5-10-20", "Temperature/Set point (0..40°C), Heating modes control", "RoomOperatingPanel.20"},
		{ RORG_4BS, 0x10, 0x21, "A5-10-21", "Temperature (0..40°C), Humidity, Heating modes control", "RoomOperatingPanel.21"},
		{ RORG_4BS, 0x10, 0x22, "A5-10-22", "Temperature/Set point (0..40°C), Humidity (0..100%), Fan control", "RoomOperatingPanel.22"},
		{ RORG_4BS, 0x10, 0x23, "A5-10-23", "Temperature/Set point (0..40°C), Humidity (0..100%), Fan control, Occupancy", "RoomOperatingPanel.23"},

		// A5-11, Controller Status
		{ RORG_4BS, 0x11, 0x01, "A5-11-01", "Lighting controller status", "ControllerStatus.01"},
		{ RORG_4BS, 0x11, 0x02, "A5-11-02", "Temperature controller status", "ControllerStatus.02"},
		{ RORG_4BS, 0x11, 0x03, "A5-11-03", "Blind controller status", "ControllerStatus.03"},
		{ RORG_4BS, 0x11, 0x04, "A5-11-04", "Extended lighting status", "ControllerStatus.04"},
		{ RORG_4BS, 0x11, 0x05, "A5-11-05", "Switch actuator relay status (2 channels)", "ControllerStatus.05"},

		// A5-12, Automated Meter Reading (AMR)
		{ RORG_4BS, 0x12, 0x00, "A5-12-00", "AMR: Counter", "AMR.Counter"},
		{ RORG_4BS, 0x12, 0x01, "A5-12-01", "AMR: Electricity", "AMR.Electricity"},
		{ RORG_4BS, 0x12, 0x02, "A5-12-02", "AMR: Gas", "AMR.Gas"},
		{ RORG_4BS, 0x12, 0x03, "A5-12-03", "AMR: Water", "AMR.Water"},
		{ RORG_4BS, 0x12, 0x04, "A5-12-04", "AMR: Temperature (-40..40°C), Load sensor (0..16383g)", "AMR.TempLoad"},
		{ RORG_4BS, 0x12, 0x05, "A5-12-05", "AMR: Temperature (-40..40°C), Container sensor (10 channels)", "AMR.TempContainer"},
		{ RORG_4BS, 0x12, 0x10, "A5-12-10", "AMR: Current meter (16 channels)", "AMR.16Current"},

		// A5-13, Environmental Applications
		{ RORG_4BS, 0x13, 0x01, "A5-13-01", "Weather station", "EnvironmentalApplications.01"},
		{ RORG_4BS, 0x13, 0x02, "A5-13-02", "Sun intensity", "EnvironmentalApplications.02"},
		{ RORG_4BS, 0x13, 0x03, "A5-13-03", "Date exchange", "EnvironmentalApplications.03"},
		{ RORG_4BS, 0x13, 0x04, "A5-13-04", "Time & Day exchange", "EnvironmentalApplications.04"},
		{ RORG_4BS, 0x13, 0x05, "A5-13-05", "Direction exchange", "EnvironmentalApplications.05"},
		{ RORG_4BS, 0x13, 0x06, "A5-13-06", "Geographic position exchange", "EnvironmentalApplications.06"},
		{ RORG_4BS, 0x13, 0x07, "A5-13-07", "Wind sensor", "EnvironmentalApplications.07"},
		{ RORG_4BS, 0x13, 0x08, "A5-13-08", "Rain sensor", "EnvironmentalApplications.08"},
		{ RORG_4BS, 0x13, 0x10, "A5-13-10", "Sun position & radiation", "EnvironmentalApplications.10"},

		// A5-14, Multi-Func Sensor
		{ RORG_4BS, 0x14, 0x01, "A5-14-01", "Window/Door contact", "MultiFuncSensor.01"},
		{ RORG_4BS, 0x14, 0x02, "A5-14-02", "Window/Door contact, Illumination (0..1000lx)", "MultiFuncSensor.02"},
		{ RORG_4BS, 0x14, 0x03, "A5-14-03", "Window/Door contact, Vibration detection", "MultiFuncSensor.03"},
		{ RORG_4BS, 0x14, 0x04, "A5-14-04", "Window/Door contact, Illumination (0..1000lx), Vibration detection", "MultiFuncSensor.04"},
		{ RORG_4BS, 0x14, 0x05, "A5-14-05", "Vibration/Tilt", "MultiFuncSensor.05"},
		{ RORG_4BS, 0x14, 0x06, "A5-14-06", "Vibration/Tilt, Illumination (0..1000lx)", "MultiFuncSensor.06"},
		{ RORG_4BS, 0x14, 0x07, "A5-14-07", "Dual-door contact", "MultiFuncSensor.07"},
		{ RORG_4BS, 0x14, 0x08, "A5-14-08", "Dual-door contact, Vibration detection", "MultiFuncSensor.08"},
		{ RORG_4BS, 0x14, 0x09, "A5-14-09", "Window/Door state", "MultiFuncSensor.09"},
		{ RORG_4BS, 0x14, 0x0A, "A5-14-0A", "Window/Door state, Vibration detection", "MultiFuncSensor.0A"},

		// A5-20, HVAC Components
		{ RORG_4BS, 0x20, 0x01, "A5-20-01", "HVAC: Battery powered actuator", "HVAC.01"},
		{ RORG_4BS, 0x20, 0x02, "A5-20-02", "HVAC: Basic actuator", "HVAC.02"},
		{ RORG_4BS, 0x20, 0x03, "A5-20-03", "HVAC: Line powered actuator", "HVAC.03"},
		{ RORG_4BS, 0x20, 0x04, "A5-20-04", "HVAC: Heating radiator valve actuator, Feed & room temperature/Set point, Display", "HVAC.04"},
		{ RORG_4BS, 0x20, 0x05, "A5-20-05", "HVAC: Ventilation unit", "HVAC.05"},
		{ RORG_4BS, 0x20, 0x06, "A5-20-06", "HVAC: Harvesting-powered actuator, Temperature offset control", "HVAC.06"},
		{ RORG_4BS, 0x20, 0x10, "A5-20-10", "HVAC: Generic interface", "HVAC.10"},
		{ RORG_4BS, 0x20, 0x11, "A5-20-11", "HVAC: Generic interface - Error control", "HVAC.11"},
		{ RORG_4BS, 0x20, 0x12, "A5-20-12", "HVAC: Temperature controller input", "HVAC.12"},

		// A5-30, Digital Input
		{ RORG_4BS, 0x30, 0x01, "A5-30-01", "Single input contact", "DigitalInput.01"},
		{ RORG_4BS, 0x30, 0x02, "A5-30-02", "Single input contact", "DigitalInput.02"},
		{ RORG_4BS, 0x30, 0x03, "A5-30-03", "4 Digital inputs, Wake, Temperature (0..40°C)", "DigitalInput.03"},
		{ RORG_4BS, 0x30, 0x04, "A5-30-04", "3 Digital inputs, 1 Digital value input (8 bit)", "DigitalInput.04"},
		{ RORG_4BS, 0x30, 0x05, "A5-30-05", "Single input contact, Retransmission", "DigitalInput.05"},
		{ RORG_4BS, 0x30, 0x06, "A5-30-06", "Single alternate input contact, Retransmission, Heartbeat", "DigitalInput.06"},

		// A5-37, Energy Management
		{ RORG_4BS, 0x37, 0x01, "A5-37-01", "Energy management: Demand response", "EnergyManagement.01"},

		// A5-38, Central Command
		{ RORG_4BS, 0x38, 0x08, "A5-38-08", "Central command: Gateway", "CentralCommand.08"},
		{ RORG_4BS, 0x38, 0x09, "A5-38-09", "Central command: Extended lighting control", "CentralCommand.09"},

		// A5-3F, Universal
		{ RORG_4BS, 0x3F, 0x00, "A5-3F-00", "Radio link test", "Universal.00"},
		{ RORG_4BS, 0x3F, 0x7F, "A5-3F-7F", "Universal (manufacturer sprecific)", "Universal.7F"},

		// D2, VLD Telegrams

		// D2-00, Room Control Panel (RCP)
		{ RORG_VLD, 0x00, 0x01, "D2-00-01", "Room conntol panel, Temperature, Display", "RCP.01"},

		// D2-01, Electronic Switches and Dimmers with Local Control
		{ RORG_VLD, 0x01, 0x00, "D2-01-00", "Switch actuator", "Switch/Dimmer.00"},
		{ RORG_VLD, 0x01, 0x01, "D2-01-01", "Switch actuator", "Switch/Dimmer.01"},
		{ RORG_VLD, 0x01, 0x02, "D2-01-02", "Dimmer actuator", "Switch/Dimmer.02"},
		{ RORG_VLD, 0x01, 0x03, "D2-01-03", "Dimmer actuator", "Switch/Dimmer.03"},
		{ RORG_VLD, 0x01, 0x04, "D2-01-04", "Dimmer actuator", "Switch/Dimmer.04"},
		{ RORG_VLD, 0x01, 0x05, "D2-01-05", "Dimmer actuator", "Switch/Dimmer.05"},
		{ RORG_VLD, 0x01, 0x06, "D2-01-06", "Switch actuator", "Switch/Dimmer.06"},
		{ RORG_VLD, 0x01, 0x07, "D2-01-07", "Switch actuator", "Switch/Dimmer.07"},
		{ RORG_VLD, 0x01, 0x08, "D2-01-08", "Switch actuator", "Switch/Dimmer.08"},
		{ RORG_VLD, 0x01, 0x09, "D2-01-09", "Dimmer actuator", "Switch/Dimmer.09"},
		{ RORG_VLD, 0x01, 0x0A, "D2-01-0A", "Switch actuator", "Switch/Dimmer.0A"},
		{ RORG_VLD, 0x01, 0x0B, "D2-01-0B", "Switch actuator", "Switch/Dimmer.0B"},
		{ RORG_VLD, 0x01, 0x0C, "D2-01-0C", "Pilot wire actuator", "Switch/Dimmer.0C"},
		{ RORG_VLD, 0x01, 0x0D, "D2-01-0D", "Switch actuator", "Switch/Dimmer.0D"},
		{ RORG_VLD, 0x01, 0x0E, "D2-01-0E", "Switch actuator", "Switch/Dimmer.0E"},
		{ RORG_VLD, 0x01, 0x0F, "D2-01-0F", "Switch actuator", "Switch/Dimmer.0F"},
		{ RORG_VLD, 0x01, 0x10, "D2-01-10", "Switch actuator (2 channels)", "Switch/Dimmer.10"},
		{ RORG_VLD, 0x01, 0x11, "D2-01-11", "Switch actuator (2 channels)", "Switch/Dimmer.11"},
		{ RORG_VLD, 0x01, 0x12, "D2-01-12", "Switch actuator (2 channels)", "Switch/Dimmer.12"},
		{ RORG_VLD, 0x01, 0x13, "D2-01-13", "Switch actuator (4 channels)", "Switch/Dimmer.13"},
		{ RORG_VLD, 0x01, 0x14, "D2-01-14", "Switch actuator (8 channels)", "Switch/Dimmer.14"},
		{ RORG_VLD, 0x01, 0x15, "D2-01-15", "Switch actuator (4 channels)", "Switch/Dimmer.15"},
		{ RORG_VLD, 0x01, 0x16, "D2-01-16", "Dimmer actuator (2 channels)", "Switch/Dimmer.16"},
		{ RORG_VLD, 0x01, 0x17, "D2-01-17", "Switch actuator (2 channels)", "Switch/Dimmer.17"},

		// D2-02, Sensors for Temperature, Illumination, Occupancy And Smoke
		{ RORG_VLD, 0x02, 0x00, "D2-02-00", "Temperature (-40..120°C), Illumination (0..2047lx), Occupancy, Smoke", "TempHumIllumOccupSmoke.00"},
		{ RORG_VLD, 0x02, 0x01, "D2-02-01", "Temperature (-40..120°C), Illumination (0..2047lx), Smoke", "TempHumIllumOccupSmoke.01"},
		{ RORG_VLD, 0x02, 0x02, "D2-02-02", "Temperature (-40..120°C), Smoke", "TempHumIllumOccupSmoke.02"},

		// D2-03, Light Switching + Blind Control
		{ RORG_VLD, 0x03, 0x00, "D2-03-00", "2 Rocker switch", "LightSwitchingBlind.00"},
		{ RORG_VLD, 0x03, 0x0A, "D2-03-0A", "Push button", "LightSwitchingBlind.0A"},
		{ RORG_VLD, 0x03, 0x10, "D2-03-10", "Mechanical handle", "LightSwitchingBlind.10"},
		{ RORG_VLD, 0x03, 0x20, "D2-03-20", "Beacon with vibration detection", "LightSwitchingBlind.20"},

		// D2-04, CO2, Humidity, Temperature, Day/Night and Autonomy
		{ RORG_VLD, 0x04, 0x00, "D2-04-00", "CO2 (0..2000ppm), Humidity, Temperature, Day/Night", "CO2HumTempDayNightAuto.00"},
		{ RORG_VLD, 0x04, 0x01, "D2-04-01", "CO2 (0..2000ppm), Humidity, Day/Night", "CO2HumTempDayNightAuto.01"},
		{ RORG_VLD, 0x04, 0x02, "D2-04-02", "CO2 (0..2000ppm), Temperature, Day/Night", "CO2HumTempDayNightAuto.02"},
		{ RORG_VLD, 0x04, 0x03, "D2-04-03", "CO2 (0..2000ppm), Temperature", "CO2HumTempDayNightAuto.03"},
		{ RORG_VLD, 0x04, 0x04, "D2-04-04", "CO2 (0..2000ppm), Temperature", "CO2HumTempDayNightAuto.04"},
		{ RORG_VLD, 0x04, 0x05, "D2-04-05", "CO2 (0..2000ppm), Temperature, Day/Night", "CO2HumTempDayNightAuto.05"},
		{ RORG_VLD, 0x04, 0x06, "D2-04-06", "CO2 (0..2000ppm), Day/Night", "CO2HumTempDayNightAuto.06"},
		{ RORG_VLD, 0x04, 0x07, "D2-04-07", "CO2 (0..2000ppm), Day/Night", "CO2HumTempDayNightAuto.07"},
		{ RORG_VLD, 0x04, 0x08, "D2-04-08", "CO2 (0..5000ppm), Humidity, Temperature, Day/Night", "CO2HumTempDayNightAuto.08"},
		{ RORG_VLD, 0x04, 0x09, "D2-04-09", "CO2 (0..5000ppm), Humidity, Day/Night", "CO2HumTempDayNightAuto.09"},
		{ RORG_VLD, 0x04, 0x0A, "D2-04-0A", "CO2, Humidity, Temperature, Day/Night, Push button", "CO2HumTempDayNightAuto.0A"},
		{ RORG_VLD, 0x04, 0x10, "D2-04-10", "CO2 (0..5000ppm), Temperature, Day/Night", "CO2HumTempDayNightAuto.10"},
		{ RORG_VLD, 0x04, 0x1A, "D2-04-1A", "CO2 (0..5000ppm), Temperature", "CO2HumTempDayNightAuto.1A"},
		{ RORG_VLD, 0x04, 0x1B, "D2-04-1B", "CO2 (0..5000ppm), Temperature", "CO2HumTempDayNightAuto.1B"},
		{ RORG_VLD, 0x04, 0x1C, "D2-04-1C", "CO2 (0..5000ppm), Temperature, Day/Night", "CO2HumTempDayNightAuto.1C"},
		{ RORG_VLD, 0x04, 0x1D, "D2-04-1D", "CO2 (0..5000ppm), Day/Night", "CO2HumTempDayNightAuto.1D"},
		{ RORG_VLD, 0x04, 0x1E, "D2-04-1E", "CO2 (0..5000ppm), Day/Night,", "CO2HumTempDayNightAuto.1E"},
		{ RORG_VLD, 0x04, 0x1F, "D2-04-1F", "CO2, Humidity, Temperature, Day/Night", "CO2HumTempDayNightAuto.1F"},

		// D2-05, Blinds Control for Position and Angle
		{ RORG_VLD, 0x05, 0x00, "D2-05-00", "Blinds position/angle control", "BlindPosAngle.00"},
		{ RORG_VLD, 0x05, 0x01, "D2-05-01", "Blinds position/angle control (4 channels)", "BlindPosAngle.01"},
		{ RORG_VLD, 0x05, 0x02, "D2-05-02", "Blinds position/angle control", "BlindPosAngle.02"},
		{ RORG_VLD, 0x05, 0x03, "D2-05-03", "Smart window", "BlindPosAngle.03"},
		{ RORG_VLD, 0x05, 0x04, "D2-05-04", "Blind actuator", "BlindPosAngle.04"},
		{ RORG_VLD, 0x05, 0x05, "D2-05-05", "Blind actuator (4 channels)", "BlindPosAngle.05"},

		// D2-06, Multisensor Window/Door Handle and Sensors
		{ RORG_VLD, 0x06, 0x01, "D2-06-01", "Alarm, Position, Vacation mode, Optional sensors", "WindowDoorHandle.01"},
		{ RORG_VLD, 0x06, 0x10, "D2-06-10", "Window intrusion detection sensor", "WindowDoorHandle.10"},
		{ RORG_VLD, 0x06, 0x11, "D2-06-11", "Tilt detection window intrusion detection, PIR sensitivity adjustment", "WindowDoorHandle.11"},
		{ RORG_VLD, 0x06, 0x20, "D2-06-20", "Electric window drive controller", "WindowDoorHandle.20"},
		{ RORG_VLD, 0x06, 0x40, "D2-06-40", "Lockable window handle", "WindowDoorHandle.40"},
		{ RORG_VLD, 0x06, 0x50, "D2-06-50", "Window sash, Hardware position sensor", "WindowDoorHandle.50"},

		// D2-07, Locking Systems Control
		{ RORG_VLD, 0x07, 0x00, "D2-07-00", "Locking systems control: Mortise lock", "LockingSystemsControl.07"},

		// D2-0A, MultiChannel Temperature Sensor
		{ RORG_VLD, 0x0A, 0x00, "D2-0A-00", "Temperature (0..80°C, 3 channels)", "MultiTemp.00"},
		{ RORG_VLD, 0x0A, 0x01, "D2-0A-01", "Temperature (-20..120°C, 3 channels)", "MultiTemp.01"},

		// D2-10, Room Control Panels with Temperature and Fan Speed Control, Room Status Information, Time Program
		{ RORG_VLD, 0x10, 0x00, "D2-10-00", "Room conntol panel", "RoomCtrPanel.00"},
		{ RORG_VLD, 0x10, 0x01, "D2-10-01", "Room conntol panel", "RoomCtrPanel.01"},
		{ RORG_VLD, 0x10, 0x02, "D2-10-02", "Room conntol panel", "RoomCtrPanel.02"},
		{ RORG_VLD, 0x10, 0x30, "D2-10-30", "Room conntol panel", "RoomCtrPanel.30"},
		{ RORG_VLD, 0x10, 0x31, "D2-10-31", "Room conntol panel", "RoomCtrPanel.31"},
		{ RORG_VLD, 0x10, 0x32, "D2-10-32", "Room conntol panel", "RoomCtrPanel.32"},

		// D2-11, Bidirectional Room Operating Panel
		{ RORG_VLD, 0x11, 0x01, "D2-11-01", "Temperature/Set point", "RoomOpPanel.01"},
		{ RORG_VLD, 0x11, 0x02, "D2-11-02", "Temperature/Set point, Humidity", "RoomOpPanel.02"},
		{ RORG_VLD, 0x11, 0x03, "D2-11-03", "Temperature/Set point, Fan control", "RoomOpPanel.03"},
		{ RORG_VLD, 0x11, 0x04, "D2-11-04", "Temperature/Set point, Humidity, Fan control", "RoomOpPanel.04"},
		{ RORG_VLD, 0x11, 0x05, "D2-11-05", "Temperature/Set point, Fan control, Occupancy", "RoomOpPanel.05"},
		{ RORG_VLD, 0x11, 0x06, "D2-11-06", "Temperature/Set point, Humidity, Fan control, Occupancy", "RoomOpPanel.06"},
		{ RORG_VLD, 0x11, 0x07, "D2-11-07", "Temperature/Set point, Occupancy", "RoomOpPanel.07"},
		{ RORG_VLD, 0x11, 0x08, "D2-11-08", "Temperature/Set point, Humidity, Occupancy", "RoomOpPanel.08"},
		{ RORG_VLD, 0x11, 0x20, "D2-11-20", "Temperature, Air condition, Floor heating, Fan Ventilation", "RoomOpPanel.20"},

		// D2-14, Multi Function Sensors
		{ RORG_VLD, 0x14, 0x00, "D2-14-00", "Temperature, Humidity", "MultiFunction.00"},
		{ RORG_VLD, 0x14, 0x01, "D2-14-01", "Temperature, Humidity, Button A/B", "MultiFunction.01"},
		{ RORG_VLD, 0x14, 0x02, "D2-14-02", "VOC sensor", "MultiFunction.02"},
		{ RORG_VLD, 0x14, 0x03, "D2-14-03", "Temperature, Humidity, VOC", "MultiFunction.03"},
		{ RORG_VLD, 0x14, 0x04, "D2-14-04", "Temperature, Humidity, Illumination", "MultiFunction.04"},
		{ RORG_VLD, 0x14, 0x05, "D2-14-05", "Temperature, Humidity, Illumination, VOC", "MultiFunction.05"},
		{ RORG_VLD, 0x14, 0x06, "D2-14-06", "CO2 sensor", "MultiFunction.06"},
		{ RORG_VLD, 0x14, 0x07, "D2-14-07", "Temperature, Humidity, CO2", "MultiFunction.07"},
		{ RORG_VLD, 0x14, 0x08, "D2-14-08", "Temperature, Humidity, Illumination, CO2", "MultiFunction.08"},
		{ RORG_VLD, 0x14, 0x09, "D2-14-09", "Temperature, Humidity, VOC, CO2", "MultiFunction.09"},
		{ RORG_VLD, 0x14, 0x0A, "D2-14-0A", "Temperature, Humidity, Illumination, VOC, CO2", "MultiFunction.0A"},
		{ RORG_VLD, 0x14, 0x0B, "D2-14-0B", "Temperature, Humidity, VOC/TVOC", "MultiFunction.0B"},
		{ RORG_VLD, 0x14, 0x0C, "D2-14-0C", "CO2 sensor", "MultiFunction.0C"},
		{ RORG_VLD, 0x14, 0x0D, "D2-14-0D", "Temperature, Humidity, VOC, Button A", "MultiFunction.0D"},
		{ RORG_VLD, 0x14, 0x0E, "D2-14-0E", "Temperature, Humidity, VOC, CO2, Button A", "MultiFunction.0E"},
		{ RORG_VLD, 0x14, 0x0F, "D2-14-0F", "Temperature, Humidity, Barometric sensor", "MultiFunction.0F"},
		{ RORG_VLD, 0x14, 0x10, "D2-14-10", "Temperature, Humidity, VOC, Occupancy", "MultiFunction.10"},
		{ RORG_VLD, 0x14, 0x1A, "D2-14-1A", "Temperature, Humidity", "MultiFunction.1A"},
		{ RORG_VLD, 0x14, 0x1B, "D2-14-1B", "Temperature, Humidity, Illumination", "MultiFunction.1B"},
		{ RORG_VLD, 0x14, 0x1C, "D2-14-1C", "Temperature, Humidity, Barometric sensor", "MultiFunction.1C"},
		{ RORG_VLD, 0x14, 0x1D, "D2-14-1D", "Temperature, Humidity, Illumination, Barometric sensor", "MultiFunction.1D"},
		{ RORG_VLD, 0x14, 0x20, "D2-14-20", "Temperature, Humidity, Illumination", "MultiFunction.20"},
		{ RORG_VLD, 0x14, 0x21, "D2-14-21", "Temperature", "MultiFunction.21"},
		{ RORG_VLD, 0x14, 0x22, "D2-14-22", "Temperature, Humidity", "MultiFunction.22"},
		{ RORG_VLD, 0x14, 0x23, "D2-14-23", "Temperature, Illumination", "MultiFunction.23"},
		{ RORG_VLD, 0x14, 0x24, "D2-14-24", "Illumination", "MultiFunction.24"},
		{ RORG_VLD, 0x14, 0x25, "D2-14-25", "Light Intensity, Color semperature sensor", "MultiFunction.25"},
		{ RORG_VLD, 0x14, 0x30, "D2-14-30", "Temperature, Humidity, Smoke, Air quality, Hygrothermal comfort", "MultiFunction.30"},
		{ RORG_VLD, 0x14, 0x31, "D2-14-31", "Temperature, Humidity, CO, Air quality, Hygrothermal comfort", "MultiFunction.31"},
		{ RORG_VLD, 0x14, 0x40, "D2-14-40", "Temperature, Humidity, XYZ acceleration, Illumination", "MultiFunction.40"},
		{ RORG_VLD, 0x14, 0x41, "D2-14-41", "Temperature, Humidity, XYZ acceleration, Illumination", "MultiFunction.41"},
		{ RORG_VLD, 0x14, 0x50, "D2-14-50", "Basic water properties, Temperature, PH sensor", "MultiFunction.50"},
		{ RORG_VLD, 0x14, 0x51, "D2-14-51", "Basic water properties, Temperature, Dissolved oxygen sensor", "MultiFunction.51"},
		{ RORG_VLD, 0x14, 0x52, "D2-14-52", "Temperature, Sound, Pressure, Illumination, Occupancy", "MultiFunction.52"},
		{ RORG_VLD, 0x14, 0x53, "D2-14-53", "Leak detector", "MultiFunction.53"},
		{ RORG_VLD, 0x14, 0x54, "D2-14-54", "Temperature, Humidity, Leak detector,", "MultiFunction.54"},
		{ RORG_VLD, 0x14, 0x55, "D2-14-55", "Power use monitor", "MultiFunction.55"},
		{ RORG_VLD, 0x14, 0x56, "D2-14-56", "Light intensity, CCT, PPFD sensor", "MultiFunction.56"},

		// D2-15, People Activity
		{ RORG_VLD, 0x15, 0x00, "D2-15-00", "People activity counter", "PeopleActivity.15"},

		// D2-20, Fan Control
		{ RORG_VLD, 0x20, 0x00, "D2-20-00", "Fan control", "FanCtrl.00"},
		{ RORG_VLD, 0x20, 0x01, "D2-20-01", "Fan control", "FanCtrl.01"},
		{ RORG_VLD, 0x20, 0x02, "D2-20-02", "Fan control", "FanCtrl.02"},

		// D2-30, Floor Heating Controls and Automated Meter Reading
		{ RORG_VLD, 0x30, 0x00, "D2-30-00", "Floor heating control / Automated meter reading", "FloorHeating.00"},
		{ RORG_VLD, 0x30, 0x01, "D2-30-01", "Floor heating control / Automated meter reading", "FloorHeating.01"},
		{ RORG_VLD, 0x30, 0x02, "D2-30-02", "Floor heating control / Automated meter reading", "FloorHeating.02"},
		{ RORG_VLD, 0x30, 0x03, "D2-30-03", "Floor heating control / Automated meter reading", "FloorHeating.03"},
		{ RORG_VLD, 0x30, 0x04, "D2-30-04", "Floor heating control / Automated meter reading", "FloorHeating.04"},
		{ RORG_VLD, 0x30, 0x05, "D2-30-05", "Floor heating control / Automated meter reading", "FloorHeating.05"},
		{ RORG_VLD, 0x30, 0x06, "D2-30-06", "Floor heating control / Automated meter reading", "FloorHeating.06"},

		// D2-31, Automated Meter Reading Gateway
		{ RORG_VLD, 0x31, 0x00, "D2-31-00", "Automated meter reading gateway", "AutoMeterReading.00"},
		{ RORG_VLD, 0x31, 0x01, "D2-31-01", "Automated meter reading gateway", "AutoMeterReading.010"},

		// D2-32, A.C. Current Clamp
		{ RORG_VLD, 0x32, 0x00, "D2-32-00", "A.C. current clamp", "ACCurrent.00"},
		{ RORG_VLD, 0x32, 0x01, "D2-32-01", "A.C. current clamp (2 channels)", "ACCurrent.01"},
		{ RORG_VLD, 0x32, 0x02, "D2-32-02", "A.C. current clamp (3 channels)", "ACCurrent.02"},

		// D2-33, Intelligent, Bi-directional Heaters and Controllers
		{ RORG_VLD, 0x33, 0x00, "D2-33-00", "Intelligent heaters and controllers", "BidirHeatersControllers.00"},

		// D2-34, Heating Actuator
		{ RORG_VLD, 0x34, 0x00, "D2-34-00", "Heating actuator", "HeatingActuator.00"},
		{ RORG_VLD, 0x34, 0x01, "D2-34-01", "Heating actuator (2 channels)", "HeatingActuator.x2"},
		{ RORG_VLD, 0x34, 0x02, "D2-34-02", "Heating actuator (8 channels)", "HeatingActuator.x8"},

		// D2-40, LED Controller Status
		{ RORG_VLD, 0x40, 0x00, "D2-40-00", "LED controller status", "LEDControlStatus.00"},
		{ RORG_VLD, 0x40, 0x01, "D2-40-01", "LED controller status", "LEDControlStatus.01"},

		// D2-50, Heat Recovery Ventilation
		{ RORG_VLD, 0x50, 0x00, "D2-50-00", "Heat recovery ventilation", "HeatRecovVent.00"},
		{ RORG_VLD, 0x50, 0x01, "D2-50-01", "Heat recovery ventilation", "HeatRecovVent.01"},
		{ RORG_VLD, 0x50, 0x10, "D2-50-10", "Heat recovery ventilation", "HeatRecovVent.10"},
		{ RORG_VLD, 0x50, 0x11, "D2-50-11", "Heat recovery ventilation", "HeatRecovVent.11"},

		// D2-A0, Standard Valve
		{ RORG_VLD, 0xA0, 0x01, "D2-A0-01", "Valve control", "BidirValveCtrl.01"},

		// D2-B0, Liquid Leakage Sensor
		{ RORG_VLD, 0xB0, 0x51, "D2-B0-51", "Liquid leakage sensor", "MechanicHarvester.51"},

		// D2-B1, Level Sensor
		{ RORG_VLD, 0xB1, 0x00, "D2-B1-00", "Level sensor dispenser", "LevelSensor.00"},

		// D5, 1BS Telegrams

		// D5-00, Contacts and Switches
		{ RORG_1BS, 0x00, 0x01, "D5-00-01", "Single input contact", "ContactSwitch.01"},

		// F6, RPS Telegrams

		// F6-01, Switch Buttons
		{ RORG_RPS, 0x01, 0x01, "F6-01-01", "Push button", "SwitchButton.0"},

		// F6-02, Rocker Switch, 2 Rocker
		{ RORG_RPS, 0x02, 0x01, "F6-02-01", "2 Rocker switch, application style 1", "LightBlind2.01"},
		{ RORG_RPS, 0x02, 0x02, "F6-02-02", "2 Rocker switch, application style 2", "LightBlind2.02"},
		{ RORG_RPS, 0x02, 0x03, "F6-02-03", "Dimmer control", "LightBlind2.03"},
		{ RORG_RPS, 0x02, 0x04, "F6-02-04", "2 Rocker switch", "LightBlind.04"},

		// F6-03, Rocker Switch, 4 Rocker
		{ RORG_RPS, 0x03, 0x01, "F6-03-01", "4 Rocker switch, application style 1", "LightBlind4.01"},
		{ RORG_RPS, 0x03, 0x02, "F6-03-02", "4 Rocker switch, application style 2", "LightBlind4.02"},

		// F6-04, Position Switch, Home and Office Application
		{ RORG_RPS, 0x04, 0x01, "F6-04-01", "Key card activated switch", "KeyCardSwitch.01"},
		{ RORG_RPS, 0x04, 0x02, "F6-04-02", "Key card activated switch", "KeyCardSwitch.02"},

		// F6-05, Detectors
		{ RORG_RPS, 0x05, 0x00, "F6-05-00", "Wind speed threshold detector", "Detector.00"},
		{ RORG_RPS, 0x05, 0x01, "F6-05-01", "Liquid leakage sensor", "Detector.01"},
		{ RORG_RPS, 0x05, 0x02, "F6-05-02", "Smoke detector", "Detector.02"},

		// F6-10, Mechanical Handle
		{ RORG_RPS, 0x10, 0x00, "F6-10-00", "Window handle", "WindowHandle.00"},
		{ RORG_RPS, 0x10, 0x01, "F6-10-01", "Window handle", "WindowHandle.01"},

		// End of table
		{ UNKNOWN_RORG, 0, 0, nullptr, nullptr, nullptr },
	};

	const char* CEnOceanEEP::GetEEP(const int RORG, const int func, const int type)
	{
		for (const _tEEPTable* pTable = (const _tEEPTable*)&_EEPTable; pTable->RORG != UNKNOWN_RORG || pTable->EEP != nullptr; pTable++)
			if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
				return pTable->EEP;

		return nullptr;
	}

	const char* CEnOceanEEP::GetEEPLabel(const int RORG, const int func, const int type)
	{
		for (const _tEEPTable* pTable = (const _tEEPTable*)&_EEPTable; pTable->RORG != UNKNOWN_RORG || pTable->label != nullptr; pTable++)
			if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
				return pTable->label;

		return "UNKNOWN";
	}

	const char* CEnOceanEEP::GetEEPDescription(const int RORG, const int func, const int type)
	{
		for (const _tEEPTable* pTable = (const _tEEPTable*)&_EEPTable; pTable->RORG != UNKNOWN_RORG || pTable->description != nullptr; pTable++)
			if ((pTable->RORG == RORG) && (pTable->func == func) && (pTable->type == type))
				return pTable->description;

		return ">>Unkown EEP... Please report!<<";
	}

	uint32_t CEnOceanEEP::GetNodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0)
	{
		return (uint32_t)((ID3 << 24) | (ID2 << 16) | (ID1 << 8) | ID0);
	}

	std::string CEnOceanEEP::GetDeviceID(const uint32_t nodeID)
	{
		// TODO: adapt following code in case devices with less than 4 bytes ID...
		char szDeviceID[10];
		sprintf(szDeviceID, (nodeID & 0xF0000000) ? "%08X" : "%07X", nodeID);
		std::string sDeviceID = szDeviceID;
		return sDeviceID;
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

		float multiplyer = (scaleMax - scaleMin) / ((float)(rangeMax - rangeMin));

		return multiplyer * ((float)(validRawValue - rangeMin)) + scaleMin;
	}

	//convert device ID id from  buffer[] to unsigned int
	unsigned int DeviceArrayToInt(unsigned char m_buffer[])
	{
		unsigned int id = (m_buffer[0] << 24) + (m_buffer[1] << 16) + (m_buffer[2] << 8) + m_buffer[3];
		return id;
	}
	//convert device ID id from   unsigned int to buffer[]
	void DeviceIntToArray(unsigned int sID, unsigned char buf[])
	{

		buf[0] = (sID >> 24) & 0xff;
		buf[1] = (sID >> 16) & 0xff;
		buf[2] = (sID >> 8) & 0xff;
		buf[3] = sID & 0xff;
	}
	//convert divice ID string to long

	unsigned int DeviceIdStringToUInt(std::string DeviceID)
	{
		unsigned int ID;
		sscanf(DeviceID.c_str(), "%x", &ID);
		return ID;
	}

	void ProfileToRorgFuncType(int EEP, int& Rorg, int& Func, int& Type)
	{
		Type = EEP & 0xff;
		EEP >>= 8;
		Func = EEP & 0xff;
		EEP >>= 8;
		Rorg = EEP & 0xff;
	}
	int RorgFuncTypeToProfile(int Rorg, int Func, int Type)
	{
		return (Rorg * 256 * 256 + Func * 256 + Type);
	}
	int getRorg(int EEP)
	{
		return (EEP >> 16) & 0xFF;
	}
	int getFunc(int EEP)
	{
		return (EEP >> 8) & 0xFF;
	}
	int getType(int EEP)
	{
		return (EEP) & 0xFF;
	}

	std::string GetEnOceanIDToString(unsigned int DeviceID)
	{
		char szDeviceID[20];
		sprintf(szDeviceID, "%08X", (unsigned int)DeviceID);
		return szDeviceID;
	}
	void setDestination(unsigned char* opt, unsigned int destID)
	{
		//optionnal data
		opt[0] = 0x03; //subtel
		DeviceIntToArray(destID, &opt[1]);
		opt[5] = 0xff;
		opt[6] = 00; //RSI
	}
} //end namespace enocean

// Webserver helpers

namespace http
{
	namespace server
	{
		void CWebServer::Cmd_EnOceanGetManufacturers(WebEmSession& session, const request& req, Json::Value& root)
		{
			//			_log.Log(LOG_NORM, "EnOcean: Cmd_EnOceanGetManufacturers");

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			int i = 0;
			for (const enocean::_tManufacturerTable* pTable = enocean::_manufacturerTable; pTable->ID != 0 || pTable->name != nullptr; pTable++)
				if (pTable->name != nullptr)
				{
					root["mantbl"][i]["idx"] = pTable->ID;
					root["mantbl"][i]["name"] = pTable->name;
					i++;
				}
			root["status"] = "OK";
			root["title"] = "EnOceanGetManufacturers";
		}

		void CWebServer::Cmd_EnOceanGetRORGs(WebEmSession& session, const request& req, Json::Value& root)
		{
			_log.Log(LOG_NORM, "EnOcean: Cmd_EnOceanGetRORGs");

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			int i = 0;
			for (const enocean::_tRORGTable* pTable = enocean::_RORGTable; pTable->RORG != enocean::UNKNOWN_RORG || pTable->label != nullptr; pTable++)
			{
				if (pTable->RORG != enocean::RORG_4BS && pTable->RORG != enocean::RORG_VLD && pTable->RORG != enocean::RORG_1BS && pTable->RORG != enocean::RORG_RPS)
					continue;

				//				_log.Log(LOG_NORM, "EnOcean: Cmd_EnOceanGetRORGs: %d RORG_%s (%02X)", pTable->RORG, pTable->label, pTable->RORG);

				root["rorgtbl"][i]["rorg"] = pTable->RORG;
				root["rorgtbl"][i]["label"] = pTable->label;
				root["rorgtbl"][i]["description"] = pTable->description;
				i++;
			}
			root["status"] = "OK";
			root["title"] = "Cmd_EnOceanGetRORGs";
		}

		void CWebServer::Cmd_EnOceanGetProfiles(WebEmSession& session, const request& req, Json::Value& root)
		{
			_log.Log(LOG_NORM, "EnOcean: Cmd_EnOceanGetProfiles");

			if (session.rights != 2)
			{
				session.reply_status = reply::forbidden;
				return; // Only admin user allowed
			}

			int i = 0;
			for (const enocean::_tEEPTable* pTable = (const enocean::_tEEPTable*)&enocean::_EEPTable; pTable->RORG != enocean::UNKNOWN_RORG || pTable->EEP != nullptr; pTable++)
			{
				//				_log.Log(LOG_NORM, "EnOcean: Cmd_EnOceanGetProfiles: %d %s", idx, pTable->eep);

				root["eeptbl"][i]["rorg"] = pTable->RORG;
				root["eeptbl"][i]["func"] = pTable->func;
				root["eeptbl"][i]["type"] = pTable->type;
				root["eeptbl"][i]["eep"] = pTable->EEP;
				root["eeptbl"][i]["description"] = pTable->description;
				root["eeptbl"][i]["label"] = pTable->label;
				i++;
			}

			root["status"] = "OK";
			root["title"] = "Cmd_EnOceanGetProfiles";
		}

	} // namespace server
} // namespace http

