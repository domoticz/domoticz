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

class CTeleinfoBase
{

	public:
		uint8_t ID;
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

		CTeleinfoBase();
		~CTeleinfoBase();

		bool UpdateDevices();

	private:
		uint32_t previous;
		std::string rate;
		std::string tariff;
		std::string color;
		time_t   last;
};
