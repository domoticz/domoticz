/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 0.1
Description : This class is used by various Teleinfo hardware decoders to process and display data
			  It is used by EcoDevices, TeleinfoSerial and more to come

History :
0.1 2017-03-03 : Creation
*/

#include "stdafx.h"
#include "TeleinfoBase.h"
#include "../main/Logger.h"

#ifdef _DEBUG
#define DEBUG_TeleinfoBase
#endif

#define DEBUG_TeleinfoBase

// Poll every 30s, and update counters at least every 5mn
#define TELEINFO_MAX_POLL_INTERVAL 300


CTeleinfoBase::CTeleinfoBase(const int ID)
{
	m_HwdID = ID;
	m_rank = 1;
	m_name = "Teleinfo";

	Init();
}


CTeleinfoBase::CTeleinfoBase(const int ID, const int rank, const std::string name)
{
	m_HwdID = ID;
	m_name = name;
	m_rank = rank;

	Init();
}


void CTeleinfoBase::Init()
{
	OPTARIF = "";
	PTEC = "";
	DEMAIN = "";
	PAPP = 0;
	BASE = 0;
	HCHC = 0;
	HCHP = 0;
	PEJP = 0;
	EJPHN= 0;
	EJPHPM = 0;
	BBRHPJB = 0;
	BBRHCJB = 0;
	BBRHPJW = 0;
	BBRHCJW = 0;
	BBRHPJR = 0;
	BBRHCJR = 0;
	IINST = 0;
	IINST1 = 0;
	IINST2 = 0;
	IINST3 = 0;
	IMAX = 0;
	IMAX1 = 0;
	IMAX2 = 0;
	IMAX3 = 0;

	m_previous = 0;
	m_rate = "";
	m_tariff = "";
	m_color = "";
	m_last = 0;
	m_triphase = false;
}


CTeleinfoBase::~CTeleinfoBase(void) {}

bool CTeleinfoBase::WriteToHardware(const char*, unsigned char)
{
	return true;
}


bool CTeleinfoBase::StartHardware()
{
	return true;
}


bool CTeleinfoBase::StopHardware()
{
	return true;
}


bool CTeleinfoBase::UpdateDevices(CDomoticzHardwareBase *CallingHardware)
{
	uint32_t pappHC, pappHP, pappHCJB, pappHPJB, pappHCJW, pappHPJW, pappHCJR, pappHPJR, checksum, level;
	double flevel;
	time_t atime = mytime(NULL);

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if ((PAPP == 0) && (IINST > 0))
		PAPP = (IINST * 230);

	// Guess if power supply is three phases or one.
	if ((IINST2 + IINST3) > 0) m_triphase=true;

	if (PTEC.substr(0,2) == "TH")
		m_rate="Toutes Heures";
	else if (PTEC.substr(0,2) == "HC")
	{
		m_rate = "Heures Creuses";
		pappHC = PAPP;
		pappHP = 0;
	}
	else if (PTEC.substr(0,2) == "HP")
	{
		m_rate = "Heures Pleines";
		pappHC = 0;
		pappHP = PAPP;
	}
	else if (PTEC.substr(0,2) == "HN")
	{
		m_rate = "Heures Normales";
		pappHC = PAPP;
		pappHP = 0;
	}
	else if (PTEC.substr(0,2) == "PM")
	{
		m_rate = "Pointe Mobile";
		pappHC = 0;
		pappHP = PAPP;
	}
	else
	{
		m_rate = "Unknown";
		m_tariff = "Undefined";
	}

	// Checksum to detect changes between measures
	checksum=BASE + HCHC + HCHP + EJPHN + EJPHPM + BBRHCJB + BBRHPJB + BBRHCJW + BBRHPJB + BBRHCJR + BBRHPJR + PAPP;

	//Send data if value changed or at least every 5 minutes
	if ((m_previous != checksum) || (difftime(atime, m_last) >= TELEINFO_MAX_POLL_INTERVAL))
	{
		m_previous = checksum;
		m_last = atime;
		if (OPTARIF == "BASE")
		{
			m_tariff="Tarif de Base";
			CallingHardware->SendKwhMeter(m_HwdID, 10, 255, PAPP, BASE/1000.0, m_name + " Index");
		}
		else if (OPTARIF == "HC..")
		{
			m_tariff="Tarif option Heures Creuses";
			SendKwhMeter(m_HwdID, 12, 255, pappHC, HCHC/1000.0, m_name + " Heures Creuses");
			SendKwhMeter(m_HwdID, 13, 255, pappHP, HCHP/1000.0, m_name + " Heures Pleines");
			SendKwhMeter(m_HwdID, 14, 255, PAPP, (HCHP + HCHC)/1000.0, m_name + " Total");
			SendTextSensor(m_HwdID, 11, 255, m_rate, m_name + " Tarif en cours");
		}
		else if (OPTARIF == "EJP.")
		{
			m_tariff="Tarif option EJP";
			SendKwhMeter(m_HwdID, 5, 255, pappHC, EJPHN/1000.0, m_name + " Heures Normales");
			SendKwhMeter(m_HwdID, 16, 255, pappHP, EJPHPM/1000.0, m_name + " Heures Pointe Mobile");
			SendKwhMeter(m_HwdID, 17, 255, PAPP, (EJPHN + EJPHPM)/1000.0, m_name + " Total");
			SendTextSensor(m_HwdID, 11, 255, m_rate, m_name + " Tarif en cours");
			SendAlertSensor(10+m_rank, 255, ((PEJP == 30) ? 4 : 1), (m_name + " Alerte Pointe Mobile").c_str());
		}
		else if (OPTARIF.substr(0,3) == "BBR")
		{
			m_tariff="Tarif option TEMPO";
			pappHCJB=0;
			pappHPJB=0;
			pappHCJW=0;
			pappHPJW=0;
			pappHCJR=0;
			pappHPJR=0;
			if (PTEC.substr(3,1) == "B")
			{
				m_color="Bleu";
				if (m_rate == "Heures Creuses")
					pappHCJB=PAPP;
				else
					pappHPJB=PAPP;
			}
			else if (PTEC.substr(3,1) == "W")
			{
				m_color="Blanc";
				if (m_rate == "Heures Creuses")
					pappHCJW=PAPP;
				else
					pappHPJW=PAPP;
			}
			else if (PTEC.substr(3,1) == "R")
			{
				m_color="Rouge";
				if (m_rate == "Heures Creuses")
					pappHCJR=PAPP;
				else
					pappHPJR=PAPP;
			}
			else
			{
				m_color="Unknown";
			}
			SendKwhMeter(m_HwdID, 18, 255, pappHCJB, BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
			SendKwhMeter(m_HwdID, 19, 255, pappHPJB, BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
			SendKwhMeter(m_HwdID, 20, 255, pappHCJW, BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
			SendKwhMeter(m_HwdID, 21, 255, pappHPJW, BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
			SendKwhMeter(m_HwdID, 22, 255, pappHCJR, BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
			SendKwhMeter(m_HwdID, 23, 255, pappHPJR, BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
			SendKwhMeter(m_HwdID, 24, 255, PAPP, (BBRHCJB + BBRHPJB + BBRHCJW + BBRHPJW + BBRHCJR + BBRHPJR)/1000.0, m_name + " Total");\
			SendTextSensor(m_HwdID, 11, 255, "Jour " + m_color + ", " + m_rate, m_name + " Tarif en cours ");
			SendTextSensor(m_HwdID, 25, 255, "Demain, jour " + DEMAIN , m_name + " couleur demain");
		}
		// Common sensors for all rates
		SendCurrentSensor(m_HwdID, 255, (float)IINST1, (float)IINST2, (float)IINST3, m_name + " Courant");

		// Alert level is 1 up to 100% usage, 2 then 3 above 100% load and 4 for maximum load (IMAX)
		if ((IMAX - IINST ) <=0)
			level = 4;
		else
			flevel = (3.0 * IINST + IMAX - 4.0 * ISOUSC) / (IMAX - ISOUSC);
		if (flevel > 4) flevel = 4;
		if (flevel < 1) flevel = 1;
		level = (int)round(flevel + 0.49);
		#ifdef DEBUG_TeleinfoBase
		_log.Log(LOG_NORM,"TeleinfoBase: HdwID=%i, rank=%i, PAPP=%i",m_HwdID, m_rank, PAPP);
		CallingHardware->SendAlertSensor(0, 255, 4, " Alerte courant maximal");
                #endif
		CallingHardware->SendAlertSensor(m_HwdID, 255, level, (m_name + " Alerte courant maximal").c_str());

		// If triphase, then send three alerts, one for each phase
		//TODO
	}
}
