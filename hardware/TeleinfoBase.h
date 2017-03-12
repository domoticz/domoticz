/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.h
Author : Blaise Thauvin
Version : 0.1
Description : This class is used by various Teleinfo hardware decoders to process and display data
			  It is used by EcoDevices, TeleinfoSerial and more to come

History :
0.1  2017-03-03 : Creation
*/

#pragma once

#include "../main/localtime_r.h"
#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CTeleinfoBase : public CDomoticzHardwareBase
{
friend class CEcoDevices;

	public:
		std::string PTEC;
		std::string OPTARIF;
		std::string DEMAIN;
		uint32_t ISOUSC;
		uint32_t IINST;
		uint32_t IINST1;
		uint32_t IINST2;
		uint32_t IINST3;
		uint32_t IMAX;
		uint32_t IMAX1;
		uint32_t IMAX2;
		uint32_t IMAX3;
		uint32_t ADPS;
		uint32_t PAPP;
		uint32_t BASE;
		uint32_t HCHC;
		uint32_t HCHP;
		uint32_t PEJP;
		uint32_t EJPHN;
		uint32_t EJPHPM;
		uint32_t BBRHPJB;
		uint32_t BBRHCJB;
		uint32_t BBRHPJW;
		uint32_t BBRHCJW;
		uint32_t BBRHPJR;
		uint32_t BBRHCJR;

		CTeleinfoBase(const int ID);
		CTeleinfoBase(const int ID, const int rank, const std::string name);
		~CTeleinfoBase();
		void Init();
		bool WriteToHardware(const char *pdata, const unsigned char length);

		bool UpdateDevices(CDomoticzHardwareBase *CCalling);

	private:
		uint32_t m_previous;
		int m_rank;
		bool m_triphase;
		std::string m_name;
		std::string m_rate;
		std::string m_tariff;
		std::string m_color;
		time_t   m_last;

		bool StartHardware();
		bool StopHardware();
};
