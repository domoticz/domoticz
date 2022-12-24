#pragma once

namespace enocean
{
	// EnOcean manufacturer names
	enum ENOCEAN_MANUFACTURER : uint16_t
	{
		UNKNOWN_MANUFACTURER = 0x000,
		PEHA = 0x001,
		ENOCEAN_MANUFACTURER_FIRST = PEHA, // First manufacturer value
		THERMOKON = 0x002,
		SERVODAN = 0x003,
		ECHOFLEX_SOLUTIONS = 0x004,
		OMNIO_AG = 0x005,
		AWAG_ELEKTROTECHNIK_AG = 0x005, // Merged
		HARDMEIER_ELECTRONICS = 0x006,
		REGULVAR_INC = 0x007,
		AD_HOC_ELECTRONICS = 0x008,
		DISTECH_CONTROLS = 0x009,
		KIEBACK_AND_PETER = 0x00A,
		ENOCEAN_GMBH = 0x00B,
		PROBARE = 0x00C,
		VICOS_GMBH = 0x00C, // Merged
		ELTAKO = 0x00D,
		LEVITON = 0x00E,
		HONEYWELL = 0x00F,
		SPARTAN_PERIPHERAL_DEVICES = 0x010,
		SIEMENS = 0x011,
		T_MAC = 0x012,
		RELIABLE_CONTROLS_CORPORATION = 0x013,
		ELSNER_ELEKTRONIK_GMBH = 0x014,
		DIEHL_CONTROLS = 0x015,
		BSC_COMPUTER = 0x016,
		S_AND_S_REGELTECHNIK_GMBH = 0x017,
		ZENO_CONTROLS = 0x018,
		MASCO_CORPORATION = 0x018, // Reassigned
		INTESIS_SOFTWARE_SL = 0x019,
		VIESSMANN = 0x01A,
		LUTUO_TECHNOLOGY = 0x01B,
		CAN2GO = 0x01C,
		SAUTER = 0x01D,
		BOOT_UP = 0x01E,
		OSRAM_SYLVANIA = 0x01F,
		UNOTECH = 0x020,
		DELTA_CONTROLS_INC = 0x21,
		UNITRONIC_AG = 0x022,
		NANOSENSE = 0x023,
		THE_S4_GROUP = 0x024,
		MSR_SOLUTIONS = 0x025,
		VEISSMANN_HAUSATOMATION_GMBH = 0x025, // Purchased
		GE = 0x26,
		MAICO = 0x027,
		RUSKIN_COMPANY = 0x28,
		MAGNUM_ENERGY_SOLUTIONS = 0x29,
		KMC_CONTROLS = 0x02A,
		ECOLOGIX_CONTROLS = 0x02B,
		TRIO_2_SYS = 0x2C,
		AFRISO_EURO_INDEX = 0x02D,
		WALDMANN_GMBH = 0x02E,
		NEC_PLATFORMS_LTD = 0x030,
		ITEC_CORPORATION = 0x031,
		SIMICX_CO_LTD = 0x32,
		PERMUNDO_GMBH = 0x33,
		EUROTRONIC_TECHNOLOGY_GMBH = 0x34,
		ART_JAPAN_CO_LTD = 0x35,
		TIANSU_AUTOMATION_CONTROL_SYSTE_CO_LTD = 0x36,
		WEINZIERL_ENGINEERING_GMBH = 0x37,
		GRUPPO_GIORDANO_IDEA_SPA = 0x38,
		ALPHAEOS_AG = 0x39,
		TAG_TECHNOLOGIES = 0x3A,
		WATTSTOPPER = 0x3B,
		PRESSAC_COMMUNICATIONS_LTD = 0x3C,
		GIGA_CONCEPT = 0x3E,
		SENSORTEC = 0x3F,
		JAEGER_DIREKT = 0x40,
		AIR_SYSTEM_COMPONENTS_INC = 0x41,
		ERMINE_CORP = 0x042,
		SODA_GMBH = 0x043,
		EKE_AUTOMATION = 0x044,
		HOLTER_REGELARMUTREN = 0x045,
		NODON = 0x046,
		DEUTA_CONTROLS_GMBH = 0x047,
		EWATTCH = 0x048,
		MICROPELT = 0x049,
		CALEFFI_SPA = 0x04A,
		DIGITAL_CONCEPTS = 0x04B,
		EMERSON_CLIMATE_TECHNOLOGIES = 0x04C,
		ADEE_ELECTRONIC = 0x04D,
		ALTECON = 0x04E,
		NANJING_PUTIAN_TELECOMMUNICATIONS = 0x04F,
		TERRALUX = 0x050,
		MENRED = 0x051,
		IEXERGY_GMBH = 0x052,
		OVENTROP_GMBH = 0x053,
		BUILDING_AUTOMATION_PRODUCTS_INC = 0x054,
		FUNCTIONAL_DEVICES_INC = 0x055,
		OGGA = 0x056,
		ITHO_DAALDEROP = 0x057,
		RESOL = 0x058,
		ADVANCED_DEVICES = 0x059,
		AUTANI_LCC = 0x05A,
		DR_RIEDEL_GMBH = 0x05B,
		HOPPE_HOLDING_AG = 0x05C,
		SIEGENIA_AUBI_KG = 0x05D,
		ADEO_SERVICES = 0x05E,
		EIMSIG_EFP_GMBH = 0x05F,
		VIMAR_SPA = 0x060,
		GLEN_DIMLAX_GMBH = 0x061,
		PMDM_GMBH = 0x062,
		HUBBEL_LIGHTNING = 0x063,
		DEBFLEX = 0x64,
		PERIFACTORY_SENSORSYSTEMS = 0x65,
		WATTY_CORP = 0x66,
		WAGO_KONTAKTTECHNIK = 0x67,
		KESSEL = 0x68,
		AUG_WINKHAUS = 0x69,
		DECELECT = 0x6A,
		MST_INDUSTRIES = 0x6B,
		BECKER_ANTRIEBS_GMBH = 0x06C,
		NEXELEC = 0x06D,
		WIELAND_ELECTRIC_GMBH = 0x06E,
		ADVISEN = 0x06F,
		CWS_BCO_INTERNATIONAL_GMBH = 0x070,
		ROTO_FRANCK_AG = 0x071,
		ALM_CONTROLS_E_K = 0x072,
		TOMMASO_TECHNOLOGIES_CO_LTD = 0x073,
		REHAUS_AG_AND_CO = 0x074,
		INABA_DENKI_SANGYO_CO_LTD = 0x075,
		HAGER_CONTROL_SAS = 0x076,
		DOMOTICZ_MANUFACTURER = 0x7FE, // Used as manufacturer for generic nodes
		MULTI_USER_MANUFACTURER = 0x7FF,
	};

	// ESP3 RORGs
	enum ESP3_RORG_TYPE : uint8_t
	{
		UNKNOWN_RORG = 0x00,	// RORG unknown (or not set) => WARNING, do not change this value !!
		RORG_ST = 0x30,			// Secure telegram
		RORG_ST_WE = 0x31,		// Secure telegram with RORG encapsulation
		RORG_STT_FW = 0x35,		// Secure teach-in telegram for switch
		RORG_4BS = 0xA5,		// 4 Bytes Communication
		RORG_ADT = 0xA6,		// Adressing Destination Telegram
		RORG_SM_REC = 0xA7,		// Smart Ack Reclaim
		RORG_GP_SD = 0xB3,		// Generic Profiles selective data
		RORG_SM_LRN_REQ = 0xC6,	// Smart Ack Learn Request
		RORG_SM_LRN_ANS = 0xC7,	// Smart Ack Learn Answer
		RORG_SM_ACK_SGNL = 0xD0, // Smart Acknowledge Signal telegram
		RORG_MSC = 0xD1,		// Manufacturer Specific Communication
		RORG_VLD = 0xD2,		// Variable length data telegram
		RORG_UTE = 0xD4,		// Universal teach-in EEP based
		RORG_1BS = 0xD5,		// 1 Byte Communication
		RORG_RPS = 0xF6,		// Repeated Switch Communication
		RORG_SYS_EX = 0xC5,		// Remote Management
	};

	class CEnOceanEEP
	{
	public:
		const char* GetManufacturerLabel(const uint32_t ID);
		const char* GetManufacturerName(const uint32_t ID);

		const char* GetRORGLabel(const uint8_t RORG);
		const char* GetRORGDescription(const uint8_t RORG);

		const char* GetEEP(const int RORG, const int func, const int type);
		const char* GetEEPLabel(const int RORG, const int func, const int type);
		const char* GetEEPDescription(const int RORG, const int func, const int type);

		uint32_t GetNodeID(const uint8_t ID3, const uint8_t ID2, const uint8_t ID1, const uint8_t ID0);

		std::string GetDeviceID(const uint32_t nodeID);

		float GetDeviceValue(const uint32_t rawValue, const uint32_t rangeMin, const uint32_t rangeMax, const float scaleMin, const float scaleMax);
	};

	//convert id from  buffer[] to unsigned int
	unsigned int DeviceArrayToInt(unsigned char m_buffer[]);
	void         DeviceIntToArray(unsigned int sID, unsigned char buf[]);
	unsigned int DeviceIdStringToUInt(std::string DeviceID);
	void         ProfileToRorgFuncType(int EEP, int& Rorg, int& Func, int& Type);
	int          RorgFuncTypeToProfile(int Rorg, int Func, int Type);
	int          getRorg(int EEP);
	int          getFunc(int EEP);
	int          getType(int EEP);

#define CheckIsGatewayAdress( deviceid) (((deviceid > m_id_base) && (deviceid < m_id_base + 128))? true: false)

	std::string GetEnOceanIDToString(unsigned int DeviceID);

	void setDestination(unsigned char* opt, unsigned int destID);
} //end namespace enocean
