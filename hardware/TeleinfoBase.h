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

#include <iostream>
#include "DomoticzHardware.h"

class CTeleinfoBase : public CDomoticzHardwareBase
{

	public:

		CTeleinfoBase();
		~CTeleinfoBase();

	private:
	protected:
		typedef struct _tTeleinfo
		{
			std::string PTEC;
			std::string OPTARIF;
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
			std::string DEMAIN;
			uint32_t previous;
			uint32_t checksum;
			std::string rate;
			std::string tariff;
			std::string color;
			time_t   last;
                        bool    triphase;
			_tTeleinfo()
			{
				ISOUSC = 0;
				IINST = 0;
				IINST1 = 0;
				IINST2 = 0;
				IINST3 = 0;
				IMAX = 0;
				IMAX1 = 0;
				IMAX2 = 0;
				IMAX3 = 0;
				ADPS = 0;
				PAPP = 0;
				BASE = 0;
				HCHC = 0;
				HCHP = 0;
				PEJP = 0;
				EJPHN = 0;
				EJPHPM = 0;
				BBRHPJB = 0;
				BBRHCJB = 0;
				BBRHPJW = 0;
				BBRHCJW = 0;
				BBRHPJR = 0;
				BBRHCJR = 0;
				previous =0;
				last = 0;
                                triphase = false;
			}
		} Teleinfo;

		void ProcessTeleinfo(Teleinfo &teleinfo);
		void ProcessTeleinfo(const std::string &name, int rank, Teleinfo &teleinfo);
};

/*  Details on Teleinfo variables

		No d'identification du compteur: ADCO (12 caractres)
		Option tarifaire (type dabonnement) : OPTARIF (4 car.)
		Intensite souscrite : ISOUSC ( 2 car. unit = amperes)
		Index si option = base : BASE ( 9 car. unit = Wh)
		Index heures creuses si option = heures creuses : HCHC ( 9 car. unit = Wh)
		Index heures pleines si option = heures creuses : HCHP ( 9 car. unit = Wh)
		Index heures normales si option = EJP : EJP HN ( 9 car. unit = Wh)
		Index heures de pointe mobile si option = EJP : EJP HPM ( 9 car. unit = Wh)
		Index heures creuses jours bleus si option = tempo : BBR HC JB ( 9 car. unit = Wh)
		Index heures pleines jours bleus si option = tempo : BBR HP JB ( 9 car. unit = Wh)
		Index heures creuses jours blancs si option = tempo : BBR HC JW ( 9 car. unit = Wh)
		Index heures pleines jours blancs si option = tempo : BBR HP JW ( 9 car. unit = Wh)
		Index heures creuses jours rouges si option = tempo : BBR HC JR ( 9 car. unit = Wh)
		Index heures pleines jours rouges si option = tempo : BBR HP JR ( 9 car. unit = Wh)
		Pravis EJP si option = EJP : PEJP ( 2 car.) 30mn avant priode EJP
		Periode tarifaire en cours : PTEC ( 4 car.)
		Couleur du lendemain si option = tempo : DEMAIN
		Intensit instantane : IINST ( 3 car. unit = amperes)
		Avertissement de dezassement de puissance souscrite : ADPS ( 3 car. unit = amperes)
		  (message mis uniquement en cas de depassement effectif, dans ce cas il est immediat)
		Intensite maximale : IMAX ( 3 car. unit = amperes)
		Puissance apparente : PAPP ( 5 car. unit = Volt.amperes)
		Groupe horaire si option = heures creuses ou tempo : HHPHC (1 car.)
		Mot d'etat (autocontrole) : MOTDETAT (6 car.)

English
				ADCO     meter id
				OPTARIF  pricing option
				ISOUSC   maximum power subscribed
				BASE     index basic rate
				HCHC     index off-peak rate
				HCHP     index peak rate
				EJPHPM   index "mobile peak rate"
				EJPHN    index normal rate
				BBRHCJB  index off-peak rate blue day
				BBRHPJB  index peak rate blue day
				BBRHCJW  index off-peak rate white day
				BBRHPJW  index peak rate white day
				BBRHCJR  index off-peak rate red day
				BBRHPJR  index peak rate red day
				PTEC     current tariff period
				IINST    instant current
				IMAX     maximum current
				PAPP     apparent power (VA)
				MOTDETAT status code
*/
