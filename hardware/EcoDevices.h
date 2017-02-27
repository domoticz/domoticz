#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include "hardwaretypes.h"

class CEcoDevices : public CDomoticzHardwareBase
{
	public:
		CEcoDevices(const int ID, const std::string &IPAddress, const unsigned short usIPPort);
		~CEcoDevices(void);
		bool WriteToHardware(const char *pdata, const unsigned char length);
	private:
		bool Login();
		void Logout();

		std::string m_szIPAddress;
		unsigned short m_usIPPort;
		bool m_stoprequested;
		boost::shared_ptr<boost::thread> m_thread;

		time_t m_lastSendDataT1, m_lastSendDataT2;
		typedef struct _tStatus
		{
			uint8_t     len;
			std::string hostname;// EcoDevices configured hostname
			std::string version; // EcoDevices firmware version
			uint32_t    meter2;	 // current flow l/mn counter 1
			uint32_t    meter3;	 // current flow l/mn counter 2
			uint32_t    count0;	 // index counter 1, liters
			uint32_t    count1;	 // index counter 2, liters
			uint32_t    pmeter2; // previous current flow counter 1
			uint32_t    pmeter3; // previous current flow counter 2
			uint32_t    pcount0; // previous index counter 1
			uint32_t    pcount1; // previous index counter 2
								 // Time last counter 1 sent
			time_t      lastsend0;
								 // Time last counter 2 sent
			time_t      lastsend1;
			_tStatus()
			{
				len = sizeof(_tStatus) - 1;
				pmeter2 = 0;
				pmeter3 = 0;
				pcount0 = 0;
				pcount1 = 0;
				lastsend0 = 0;
				lastsend1 = 0;
			}
		} Status;

		Status m_status;

		typedef struct _tTeleinfo
		{
			uint8_t len;
			uint8_t type;
			uint8_t subtype;
			int32_t ID;
			std::string PTEC;
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
			std::string rate;
			std::string tariff;
			std::string color;
			_tTeleinfo()
			{
				len = sizeof(_tTeleinfo) - 1;
				type = 0;		 //pTypeP1Power;
				subtype = 0;	 //sTypeP1Power;
				ID=0;
				PTEC="";
				PAPP=0;
				BASE=0;
				HCHC=0;
				HCHP=0;
				PEJP=0;
				EJPHN=0;
				EJPHPM=0;
				BBRHPJB=0;
				BBRHCJB=0;
				BBRHPJW=0;
				BBRHCJW=0;
				BBRHPJR=0;
				BBRHCJR=0;
				DEMAIN="";
				previous=0;
				rate="";
				tariff="";
				color="";
			}
		} Teleinfo;

		Teleinfo m_teleinfo1, m_teleinfo2;

		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();
		void GetMeterDetails();
};

/*  Details on Teleinfo variables

	N didentification du compteur: ADCO (12 caractres)
	Option tarifaire (type dabonnement) : OPTARIF (4 car.)
	Intensit souscrite : ISOUSC ( 2 car. unit = ampres)
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
	Priode tarifaire en cours : PTEC ( 4 car.)
	Couleur du lendemain si option = tempo : DEMAIN
	Intensit instantane : IINST ( 3 car. unit = ampres)
	Avertissement de dpassement de puissance souscrite : ADPS ( 3 car. unit = ampres)
	  (message mis uniquement en cas de dpassement effectif, dans ce cas il est immdiat)
	Intensit maximale : IMAX ( 3 car. unit = ampres)
	Puissance apparente : PAPP ( 5 car. unit = Volt.ampres)
	Groupe horaire si option = heures creuses ou tempo : HHPHC (1 car.)
	Mot dtat (autocontrle) : MOTDETAT (6 car.)

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
