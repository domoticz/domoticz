#include "stdafx.h"
#include "EnOceanRawValue.h"

namespace enocean
{
	// return the value at bit offset offset length : size
	// as described in eep profile

	uint32_t GetRawValue(uint8_t* data, uint16_t offset, uint8_t size)
	{
		uint32_t value = 0;
		if (size > 32)
			return 0;

		int bitDest = offset + size - 1;
		for (int bitsrc = 0; bitsrc < size; bitsrc++)
		{
			int destByte = bitDest / 8;
			int DestBit = 7 - bitDest % 8;
			if (data[destByte] & (1 << DestBit))
				value |= (1 << bitsrc);
			bitDest--;
		}
		return value;
	}

	// copy the value at bit offset offset length : size
	// as described in eep profile
	// return true if ok
	bool SetRawValue(uint8_t* data, uint32_t value, uint16_t offset, uint8_t size)
	{
		if (size > 32)
			return 0;

		int bitDest = offset + size - 1;
		for (int bitsrc = 0; bitsrc < size; bitsrc++)
		{
			int destByte = bitDest / 8;
			int DestBit = 7 - bitDest % 8;
			if (value & (1 << bitsrc))
				data[destByte] |= (1 << DestBit);
			else
				data[destByte] &= ~(1 << DestBit);
			bitDest--;
		}
		return 1;
	}

	T_DATAFIELD* GetOffsetFromName(char* OffsetName, T_DATAFIELD* OffsetDes)
	{
		uint32_t offsetInd = 0;
		while (OffsetDes[offsetInd].Size != 0)
		{
			if (strstr(OffsetDes[offsetInd].ShortCut.c_str(), OffsetName) != 0)
				return &OffsetDes[offsetInd];
			offsetInd++;
		}
		return nullptr;
	}

	bool SetRawValue(uint8_t* data, uint32_t value, T_DATAFIELD* offset)
	{
		return SetRawValue(data, value, offset->Offset, offset->Size);
	}

	uint32_t GetRawValue(uint8_t* data, T_DATAFIELD* offset)
	{
		return GetRawValue(data, offset->Offset, offset->Size);
	}

	uint32_t GetRawValue(uint8_t* data, T_DATAFIELD* offset, uint32_t offsetIndex)
	{
		return GetRawValue(data, offset[offsetIndex].Offset, offset[offsetIndex].Size);
	}

	bool SetRawValue(uint8_t* data, uint32_t value, char* OffsetName, T_DATAFIELD* OffsetDes)
	{
		T_DATAFIELD* offset = GetOffsetFromName(OffsetName, OffsetDes);
		if (offset)
			return SetRawValue(data, value, offset->Offset, offset->Size);
		else
			return false;
	}

	uint32_t GetRawValue(uint8_t* data, char* OffsetName, T_DATAFIELD* OffsetDes)
	{
		T_DATAFIELD* offset = GetOffsetFromName(OffsetName, OffsetDes);
		if (offset)
			return GetRawValue(data, offset->Offset, offset->Size);
		else
			return false;
	}

	// return the number of byte of data payload
	// 0 if rror
	uint32_t SetRawValuesNb(uint8_t* data, T_DATAFIELD* OffsetDes, int NbParameter, va_list value)
	{
		uint32_t total_bits = 0;
		for (int i = 0; i < NbParameter; i++)
		{
			if (OffsetDes->Size == 0)
				return 0; // erreur

			uint32_t par = va_arg(value, int); // va_arg() gives current parameter
			SetRawValue(data, par, OffsetDes);
			// compute total bit
			if (OffsetDes->Offset + OffsetDes->Size > total_bits)
				total_bits = OffsetDes->Offset + OffsetDes->Size;
			OffsetDes++;
		}

		// test if all variable are sets
		if (OffsetDes->Size != 0)
			return 0; // erreur

		uint32_t total_bytes = (total_bits + 7) / 8;

		return total_bytes;
	}

	uint32_t SetRawValuesNb(uint8_t* data, T_DATAFIELD* OffsetDes, int NbParameter, ...)
	{
		va_list value;

		// Initialize the va_list structure
		va_start(value, NbParameter);
		uint32_t total_bytes = SetRawValuesNb(data, OffsetDes, NbParameter, value);
		va_end(value);

		return total_bytes;
	}

	uint32_t GetNbDataFields(T_DATAFIELD* OffsetDes)
	{

		int i = 0;
		while (OffsetDes++->Size != 0)
			i++;

		return i;
	}

	uint32_t CopyValues(int* data, int SizeData, va_list value)
	{
		int i = 0;
		int par = va_arg(value, int); // va_arg() gives current parameter
		while ((par != END_ARG_DATA) && (i < SizeData))
		{
			data[i] = par;
			par = va_arg(value, int); // va_arg() gives current parameter
			i++;
		}

		if (par != END_ARG_DATA)
			return 0;

		return i;
	}

	// return the number of byte of data payload
	// 0 if error
	uint32_t setRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes, int value[], int NbData)
	{
		int i = 0;
		uint32_t total_bits = 0;
		while (OffsetDes->Size != 0)
		{

			if (i >= NbData)
				return 0;
			int par = value[i++];
			// not enough argument
			SetRawValue(data, par, OffsetDes);
			// compute total bit
			if (OffsetDes->Offset + OffsetDes->Size > total_bits)
				total_bits = OffsetDes->Offset + OffsetDes->Size;
			OffsetDes++;
		}
		if (i != NbData)
			return 0;

		uint32_t total_bytes = (total_bits + 7) / 8;

		return total_bytes;
	}

	uint32_t getRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes, int value[], int NbData)
	{
		int i = 0;
		while (OffsetDes->Size != 0)
		{

			if (i >= NbData)
				return i;
			value[i++] = GetRawValue(data, OffsetDes);

			OffsetDes++;
		}
		return i;
	}

	std::string printRawDataValues(uint8_t* data, T_DATAFIELD* OffsetDes)
	{
		std::string message = "";
		char line[256];
		int i = 0;
		while (OffsetDes->Size != 0)
		{
			uint32_t value = GetRawValue(data, OffsetDes);
			snprintf(line, sizeof(line) - 1, "offset:%2d = %5d (%04X) : %s : %s \n", OffsetDes->Offset, value, value, OffsetDes->ShortCut.c_str(), OffsetDes->description.c_str());
			message += line;
			OffsetDes++;
		}
		return message;
	}

	uint32_t SetRawValues(uint8_t* data, T_DATAFIELD* OffsetDes, va_list value)
	{
		int Value[256];
		// get value in arg line ...
		int nbValue = CopyValues(Value, sizeof(Value) / sizeof(int), value);

		return setRawDataValues(data, OffsetDes, Value, nbValue);
	}

	uint32_t SetRawValues(uint8_t* data, T_DATAFIELD* OffsetDes, ...)
	{
		va_list value;

		/* Initialize the va_list structure */
		va_start(value, OffsetDes);
		return SetRawValues(data, OffsetDes, value);
	}

	// map
	uint32_t GetRawValue(uint8_t* data, _T_EEP_CASE* offset, uint32_t offsetIndex)
	{
		if (offsetIndex < offset->size())
			return GetRawValue(data, offset->at(offsetIndex).Offset, offset->at(offsetIndex).Size);

		return 0;
	}

	uint32_t SetRawValues(uint8_t* data, _T_EEP_CASE* EEP_case, ...)
	{
		va_list value;
		uint32_t total_bits = 0;

		T_DATAFIELD* OffsetDes;

		/* Initialize the va_list structure */
		va_start(value, EEP_case);

		for (uint32_t i = 0; i < EEP_case->size(); i++)
		{
			OffsetDes = &(EEP_case->at(i));

			int par = va_arg(value, int); /*   va_arg() gives current parameter    */
			// not enough argument
			if (par == END_ARG_DATA)
				return 0;
			SetRawValue(data, par, OffsetDes);
			// compute total bit
			if (OffsetDes->Offset + OffsetDes->Size > total_bits)
				total_bits = OffsetDes->Offset + OffsetDes->Size;
		}

		int par = va_arg(value, int);
		if (par != END_ARG_DATA)
			return 0;

		// last bit offset
		int total_bytes = (total_bits + 7) / 8;

		va_end(value);

		return total_bytes;
	}
	/*
	// vld D2-03-0A : len=2 offset 0 battery level 1= action : //1 = simple press, 2=double press, 3=long press, 4=press release
	T_DATAFIELD D2030A[] = {
		{0, 8, 0, 0, 0, 0, "BAT", "battert level"},
		{8, 8, 0, 0, 0, 0, "BUT", "button       "}, //
		{0, 0, 0, 0, 0, 0, "", ""}					//
	};

	// TEACHIN_4BS vld
	T_DATAFIELD TEACHIN_4BS[] = {
		{0, 6, 0, 0, 0, 0, "FUNC", "function"},
		{6, 7, 0, 0, 0, 0, "TYPE", "type    "},			 //
		{13, 11, 0, 0, 0, 0, "MANU", "manufacturer   "}, //
		{24, 1, 0, 0, 0, 0, "LRNT", "  learn type "},	 //  0:WithOut EEP 1: with EEP
		{28, 1, 0, 0, 0, 0, "LRNB", "  learn bite "},	 //  0 TeachIn telegram 1 DataLelegram
		{0, 0, 0, 0, 0, 0, "", ""}						 //
	};

	T_DATAFIELD TEACHIN_UTE[] = {
		{0, 1, 0, 0, 0, 0, "EEPO", "EEP operation"},
		{1, 1, 0, 0, 0, 0, "EEPTEACH", "    "},							   //
		{2, 2, 0, 0, 0, 0, "TEACHREQ", "TeachIn request"},				   //
		{4, 4, 0, 0, 0, 0, "CMDID", "Command Identifier"},				   //
		{8, 8, 0, 0, 0, 0, "NBCHAN", "Nimber of channel to be taught in"}, //
		{16, 8, 0, 0, 0, 0, "MANIDLSB", "MAn ID lsb"},					   //
		{29, 3, 0, 0, 0, 0, "MANIFMSB", "Man Id Msb"},					   //
		{32, 8, 0, 0, 0, 0, "TYPE", ""},								   //
		{40, 8, 0, 0, 0, 0, "FUNC", ""},								   //
		{48, 8, 0, 0, 0, 0, "RORG", ""},								   //

		{0, 0, 0, 0, 0, 0, "", ""} //
	};
*/
} //end namespace enocean
