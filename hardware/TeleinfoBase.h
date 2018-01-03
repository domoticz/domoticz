/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.h
Author : Blaise Thauvin
Version : 1.4
Description : This class is used by various Teleinfo hardware decoders to process and display data
		  It is currently used by EcoDevices, TeleinfoSerial
		  Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
		  http://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_02E.pdf

History :
0.1 2017-03-03 : Creation
1.0 2017-03-15 : Release candidate
1.3 2017-04-01 : Added RateLimit
1.4 2017-04-13 : Added DataTimeout
*/

#pragma once

#include <iosfwd>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"

class CTeleinfoBase : public CDomoticzHardwareBase
{

	public:

		CTeleinfoBase();
		~CTeleinfoBase();

	private:
		int AlertLevel(int Iinst, int Isousc, char* text);
		P1Power m_p1power, m_p2power, m_p3power;

	protected:
		int m_iRateLimit;
		int m_iDataTimeout;

		typedef struct _tTeleinfo
		{
			std::string ADCO;
			std::string PTEC;
			std::string OPTARIF;
			uint32_t ISOUSC;
			uint32_t IINST;
			uint32_t IINST1;
			uint32_t IINST2;
			uint32_t IINST3;
			uint32_t PPOT;
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
			uint32_t pAlertPAPP;
			uint32_t pAlertI1;
			uint32_t pAlertI2;
			uint32_t pAlertI3;
			uint32_t pAlertPPOT;
			uint32_t pAlertRate;
			uint32_t pAlertColor;
			uint32_t pAlertEJP;
			uint32_t pAlertDemain;
			std::string rate;
			std::string tariff;
			std::string color;
			time_t   last;
			bool    triphase;
			int    CRCmode1;	 // really a bool, but with a special "un-initialized state"
			_tTeleinfo()
			{
				ISOUSC = 0;
				IINST = 0;
				IINST1 = 0;
				IINST2 = 0;
				IINST3 = 0;
				PPOT = 0;
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
				pAlertPAPP = 10;
				pAlertI1 = 10;
				pAlertI2 = 10;
				pAlertI3 = 10;
				pAlertPPOT = 10;
				pAlertRate = 10;
				pAlertColor = 10;
				pAlertEJP = 10;
				pAlertDemain = 10;
				last = 0;
				triphase = false;
				CRCmode1 = 255;	 // means "bool not initialized yet", will be when running CRC Check for the first time
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
				PPOT     Potental (tri phases only)
				PAPP     apparent power (VA)
				MOTDETAT status code
*/
