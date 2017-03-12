#pragma once

#include "DomoticzHardware.h"
#include "hardwaretypes.h"
#include "TeleinfoBase.h"
#include <iostream>

class CEcoDevices : public CDomoticzHardwareBase
{
friend class CTeleinfoBase;

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

		std::string m_hostname;	 // EcoDevices configured hostname
		std::string m_version;	 // EcoDevices firmware version
		uint32_t    m_flow1;	 // current flow l/mn counter 1
		uint32_t    m_flow2;	 // current flow l/mn counter 2
		uint32_t    m_index1;	 // index counter 1, liters
		uint32_t    m_index2;	 // index counter 2, liters
		std::string m_t1_ptec;	 // Subcription on input Teleinfo 1
		std::string m_t2_ptec;	 // subscription on input Teleifo 2
		uint32_t    m_pflow1;	 // previous current flow counter 1
		uint32_t    m_pflow2;	 // previous current flow counter 2
		uint32_t    m_pindex1;	 // previous index counter 1
		uint32_t    m_pindex2;	 // previous index counter 2
		time_t      m_time1;	 // time counter 1 sent
		time_t      m_time2;	 // time counter 2 sent

		CTeleinfoBase *p_teleinfo1;
		CTeleinfoBase *p_teleinfo2;

		void Init();
		bool StartHardware();
		bool StopHardware();
		void Do_Work();
		void DecodeXML2Teleinfo(const std::string &sResult, CTeleinfoBase *teleinfo);
		void GetMeterDetails();
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
	Preavis EJP si option = EJP : PEJP ( 2 car.) 30mn avant priode EJP
	Periode tarifaire en cours : PTEC ( 4 car.)
	Couleur du lendemain si option = tempo : DEMAIN
	Intensite instantanee : IINST ( 3 car. unit = amperes)
	Avertissement de depassement de puissance souscrite : ADPS ( 3 car. unit = amperes)
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
