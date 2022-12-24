#pragma once

#include <stdarg.h>

namespace enocean
{
	// Profile descriptor from eep.xml
	class T_DATAFIELD
	{
	public:
		uint32_t Offset;
		uint32_t Size;
		double RangeMin;
		double RangeMax;
		double ScaleMin;
		double ScaleMax;

		std::string ShortCut;
		std::string description;
		std::string enumerate;
	};

	// Profile descriptor from eep.xml

	typedef struct
	{
		T_DATAFIELD* Dataf;
		std::string Title;
		std::string Desc;
	} T_EEP_CASE_;

	class T_PROFIL_LIST
	{
	public:
		int Profil;
		int Rorg;
		int Func;
		int type;
		T_EEP_CASE_** cases;
		int nbCases;
		std::string FuncTitle;
		std::string TypeTitle;
	};

	// descriptor for a eep case
	// a profil a as several case
	//  a case as several datafield
	typedef std::vector<T_DATAFIELD> _T_EEP_CASE;

	// value for the last argument of a telegram data payload
#define END_ARG_DATA (((unsigned int)1 << 31) - 1)

// return the value at bit offset offset length : size
// as described in eep profile
	uint32_t GetRawValue(uint8_t* data, uint16_t offset, uint8_t size);

	// copy the value at bit offset offset length : size
	// as described in eep profile
	// return true if ok
	bool SetRawValue(uint8_t* data, uint32_t value, uint16_t offset, uint8_t size);

	T_DATAFIELD* GetOffsetFromName(char* OffsetName, T_DATAFIELD* OffsetDes);

	bool SetRawValue(uint8_t* data, uint32_t value, T_DATAFIELD* offset);

	uint32_t GetRawValue(uint8_t* data, T_DATAFIELD* offset);

	uint32_t GetRawValue(uint8_t* data, T_DATAFIELD* offset, uint32_t offsetIndex);

	bool SetRawValue(uint8_t* data, uint32_t value, char* OffsetName, T_DATAFIELD* OffsetDes);

	uint32_t GetRawValue(uint8_t* data, char* OffsetName, T_DATAFIELD* OffsetDes);

	uint32_t SetRawValuesNb(uint8_t* data, T_DATAFIELD* OffsetDes, int NbParameter, va_list value);

	uint32_t SetRawValuesNb(uint8_t* data, T_DATAFIELD* OffsetDes, int NbParameter, ...);

	uint32_t SetRawValues(uint8_t* data, T_DATAFIELD* OffsetDes, va_list value);

	uint32_t SetRawValues(uint8_t* data, T_DATAFIELD* OffsetDes, ...);

	uint32_t GetRawValue(uint8_t* data, _T_EEP_CASE* offset, uint32_t offsetIndex);

	uint32_t SetRawValues(uint8_t* data, _T_EEP_CASE* EEP_case, ...);

	uint32_t setRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes, int value[], int NbData);

	uint32_t getRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes, int value[], int NbData);

	std::string printRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes);

	// vld D2-03-0A : len=2 offset 0 battery level 1= action : //1 = simple press, 2=double press, 3=long press, 4=press release
/*
	extern T_DATAFIELD D2030A[];

#define D2030A_BAT 0
#define D2030A_BUT 1
#define D2030A_NB_DATA 1
#define D2030A_DATA_SIZE 2

	extern T_DATAFIELD TEACHIN_4BS[];

#define WITHOUT_EEP 0
#define WITH_EEP 1
#define TEACHIN 0
#define DATA_TELEG 1

	extern T_DATAFIELD TEACHIN_UTE[];
*/
} //end namespace enocean
