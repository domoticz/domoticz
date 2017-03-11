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

#include "TeleinfoBase.h"

#ifdef _DEBUG
#define DEBUG_TeleinfoBase
#endif

// Poll every 30s, and update counters at least every 5mn
#define TELEINFO_MAX_POLL_INTERVAL 300

CTeleinfoBase::CTeleinfoBase()
{
}


CTeleinfoBase::CTeleinfoBase(const int c_HwdID)
{
	//	HwdID = c_HwdID;
	//	rank = 1;
	//	name = "Teleinfo";
	//	Init();
}


CTeleinfoBase::CTeleinfoBase(const int c_HwdID, const int c_rank, const std::string c_name)
{
	name = c_name;
	rank = c_rank;
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

	previous = 0;
	rate = "";
	tariff = "";
	color = "";
	last = 0;
	triphase = false;
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


bool CTeleinfoBase::UpdateDevices()
{
	uint32_t m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR, checksum, level;
	double flevel;
	time_t atime = mytime(NULL);

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if ((PAPP == 0) && (IINST > 0))
		PAPP = (IINST * 230);

	// Guess if power supply is three phases or one.
	if ((IINST2 + IINST3) > 0) triphase=true;

	if (PTEC.substr(0,2) == "TH")
		rate="Toutes Heures";
	else if (PTEC.substr(0,2) == "HC")
	{
		rate = "Heures Creuses";
		m_pappHC = PAPP;
		m_pappHP = 0;
	}
	else if (PTEC.substr(0,2) == "HP")
	{
		rate = "Heures Pleines";
		m_pappHC = 0;
		m_pappHP = PAPP;
	}
	else if (PTEC.substr(0,2) == "HN")
	{
		rate = "Heures Normales";
		m_pappHC = PAPP;
		m_pappHP = 0;
	}
	else if (PTEC.substr(0,2) == "PM")
	{
		rate = "Pointe Mobile";
		m_pappHC = 0;
		m_pappHP = PAPP;
	}
	else
	{
		rate = "Unknown";
		tariff = "Undefined";
	}

	// Checksum to detect changes between measures
	checksum=BASE + HCHC + HCHP + EJPHN + EJPHPM + BBRHCJB + BBRHPJB + BBRHCJW + BBRHPJB + BBRHCJR + BBRHPJR + PAPP;

	//Send data if value changed or at least every 5 minutes
	if ((previous != checksum) || (difftime(atime, last) >= TELEINFO_MAX_POLL_INTERVAL))
	{
		previous = checksum;
		last = atime;
		if (OPTARIF == "BASE")
		{
			tariff="Tarif de Base";
			SendKwhMeter(HwdID, 10, 255, PAPP, BASE/1000.0, name + " Index");
		}
		else if (OPTARIF == "HC..")
		{
			tariff="Tarif option Heures Creuses";
			SendKwhMeter(HwdID, 12, 255, m_pappHC, HCHC/1000.0, name + " Heures Creuses");
			SendKwhMeter(HwdID, 13, 255, m_pappHP, HCHP/1000.0, name + " Heures Pleines");
			SendKwhMeter(HwdID, 14, 255, PAPP, (HCHP + HCHC)/1000.0, name + " Total");
			SendTextSensor(HwdID, 11, 255, rate, name + " Tarif en cours");
		}
		else if (OPTARIF == "EJP.")
		{
			tariff="Tarif option EJP";
			SendKwhMeter(HwdID, 5, 255, m_pappHC, EJPHN/1000.0, name + " Heures Normales");
			SendKwhMeter(HwdID, 16, 255, m_pappHP, EJPHPM/1000.0, name + " Heures Pointe Mobile");
			SendKwhMeter(HwdID, 17, 255, PAPP, (EJPHN + EJPHPM)/1000.0, name + " Total");
			SendTextSensor(HwdID, 11, 255, rate, name + " Tarif en cours");
			SendAlertSensor(10+rank, 255, ((PEJP == 30) ? 4 : 1), (name + " Alerte Pointe Mobile").c_str());
		}
		else if (OPTARIF.substr(0,3) == "BBR")
		{
			tariff="Tarif option TEMPO";
			m_pappHCJB=0;
			m_pappHPJB=0;
			m_pappHCJW=0;
			m_pappHPJW=0;
			m_pappHCJR=0;
			m_pappHPJR=0;
			if (PTEC.substr(3,1) == "B")
			{
				color="Bleu";
				if (rate == "Heures Creuses")
					m_pappHCJB=PAPP;
				else
					m_pappHPJB=PAPP;
			}
			else if (PTEC.substr(3,1) == "W")
			{
				color="Blanc";
				if (rate == "Heures Creuses")
					m_pappHCJW=PAPP;
				else
					m_pappHPJW=PAPP;
			}
			else if (PTEC.substr(3,1) == "R")
			{
				color="Rouge";
				if (rate == "Heures Creuses")
					m_pappHCJR=PAPP;
				else
					m_pappHPJR=PAPP;
			}
			else
			{
				color="Unknown";
			}
			SendKwhMeter(HwdID, 18, 255, m_pappHCJB, BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
			SendKwhMeter(HwdID, 19, 255, m_pappHPJB, BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
			SendKwhMeter(HwdID, 20, 255, m_pappHCJW, BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
			SendKwhMeter(HwdID, 21, 255, m_pappHPJW, BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
			SendKwhMeter(HwdID, 22, 255, m_pappHCJR, BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
			SendKwhMeter(HwdID, 23, 255, m_pappHPJR, BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
			SendKwhMeter(HwdID, 24, 255, PAPP, (BBRHCJB + BBRHPJB + BBRHCJW + BBRHPJW + BBRHCJR + BBRHPJR)/1000.0, name + " Total");\
			SendTextSensor(HwdID, 11, 255, "Jour " + color + ", " + rate, name + " Tarif en cours ");
			SendTextSensor(HwdID, 25, 255, "Demain, jour " + DEMAIN , name + " couleur demain");
		}
		// Common sensors for all rates
		SendCurrentSensor(HwdID, 255, (float)IINST1, (float)IINST2, (float)IINST3, name + " Courant");

		// Alert level is 1 up to 100% usage, 2 then 3 above 100% load and 4 for maximum load (IMAX)
		if ((IMAX - IINST ) <=0)
			level = 4;
		else
			flevel = (3.0 * IINST + IMAX - 4.0 * ISOUSC) / (IMAX - ISOUSC);
		if (flevel > 4) flevel = 4;
		if (flevel < 1) flevel = 1;
		level = (int)round(flevel + 0.49);
		SendAlertSensor(rank, 255, level, (name + " Alerte courant maximal").c_str());

		// If triphase, then send three alerts, one for each phase
		//TODO
	}
}
