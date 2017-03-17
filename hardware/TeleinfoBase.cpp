/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 1.0
Description : This class is used by various Teleinfo hardware decoders to process and display data
		  It is currently used by EcoDevices, TeleinfoSerial
		  Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
			  http://www.enedis.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf

History :
0.1 2017-03-03 : Creation
1.0 2017-03-15 : Release candidate
*/

#include "stdafx.h"
#include "TeleinfoBase.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"

CTeleinfoBase::CTeleinfoBase() {}

CTeleinfoBase::~CTeleinfoBase() {}

void CTeleinfoBase::ProcessTeleinfo(Teleinfo &teleinfo)
{
	ProcessTeleinfo("Teleinfo", 1, teleinfo);
}


// Alert level is 1 up to 100% usage, 2 then 3 above 100% load and 4 for maximum load (IMAX)
int CTeleinfoBase::AlertLevel(int Iinst, int Imax, int Isousc)
{
	int level;
	double flevel;

	if ((Imax - Isousc) == 0)
		return 0;
	if ((Imax - Iinst ) <=0)
		level = 4;
	else
		flevel = (3.0 * Iinst + Imax- 4.0 * Isousc) / (Imax - Isousc);
	if (flevel > 4) flevel = 4;
	if (flevel < 1) flevel = 1;
	level = (int)round(flevel + 0.49);
	return level;
}


void CTeleinfoBase::ProcessTeleinfo(const std::string &name, int rank, Teleinfo &teleinfo)
{
	uint32_t m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR, checksum;
	time_t atime = mytime(NULL);

	// We need to limit the number of Teleinfo devices per hardware because of the subID in sensors. i
	if ((rank < 1) || (rank > 4))
	{
		_log.Log(LOG_ERROR,"TeleinfoBase: Invalid rank passed to function (%i), must be between 1 and 4", rank);
		return;
	}
	rank = rank -1;				 // Now it is 0 to 3

	// Guess if we are running with one phase or three
	// (some devices like EcoDevices always send all variables regardless or the real setting)
	if ((teleinfo.triphase == true) || (teleinfo.IINST2 > 0) || (teleinfo.IINST3 > 0) || (teleinfo.IMAX2 > 0) || (teleinfo.IMAX3 > 0))
		teleinfo.triphase=true;

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if ((teleinfo.PAPP == 0) && (teleinfo.IINST > 0))
		teleinfo.PAPP = (teleinfo.triphase ? (teleinfo.IINST1 + teleinfo.IINST2 + teleinfo.IINST3) : (teleinfo.IINST)) * 230;

	if (teleinfo.PTEC.substr(0,2) == "TH")
		teleinfo.rate="Toutes Heures";
	else if (teleinfo.PTEC.substr(0,2) == "HC")
	{
		teleinfo.rate = "Heures Creuses";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HP")
	{
		teleinfo.rate = "Heures Pleines";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HN")
	{
		teleinfo.rate = "Heures Normales";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
	}
	else if (teleinfo.PTEC.substr(0,2) == "PM")
	{
		teleinfo.rate = "Pointe Mobile";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
	}
	else
	{
		teleinfo.rate = "Unknown";
		teleinfo.tariff = "Undefined";
	}

	// Checksum to detect changes between measures
	checksum=teleinfo.BASE + teleinfo.HCHC + teleinfo.HCHP + teleinfo.EJPHN + teleinfo.EJPHPM + teleinfo.BBRHCJB + teleinfo.PEJP\
		+ teleinfo.BBRHPJB + teleinfo.BBRHCJW + teleinfo.BBRHPJB + teleinfo.BBRHCJR + teleinfo.BBRHPJR + teleinfo.PAPP;

	//Send data if value changed or at least every 5 minutes
	if ((teleinfo.previous != checksum) || (difftime(atime, teleinfo.last) >= 300))
	{
		teleinfo.previous = checksum;
		teleinfo.last = atime;
		if (teleinfo.OPTARIF == "BASE")
		{
			teleinfo.tariff="Tarif de Base";
			SendKwhMeter(m_HwdID, 32*rank + 2, 255, teleinfo.PAPP, teleinfo.BASE/1000.0, name + " Index");
			SendTextSensor(m_HwdID, 32*rank + 1, 255, teleinfo.rate, name + " Tarif en cours");
		}
		else if (teleinfo.OPTARIF == "HC..")
		{
			teleinfo.tariff="Tarif option Heures Creuses";
			SendKwhMeter(m_HwdID, 32*rank + 3, 255, m_pappHC, teleinfo.HCHC/1000.0, name + " Heures Creuses");
			SendKwhMeter(m_HwdID, 32*rank + 4, 255, m_pappHP, teleinfo.HCHP/1000.0, name + " Heures Pleines");
			SendKwhMeter(m_HwdID, 32*rank + 5, 255, teleinfo.PAPP, (teleinfo.HCHP + teleinfo.HCHC)/1000.0, name + " Total");
			SendTextSensor(m_HwdID, 32*rank + 1, 255, teleinfo.rate, name + " Tarif en cours");
		}
		else if (teleinfo.OPTARIF == "EJP.")
		{
			teleinfo.tariff="Tarif option EJP";
			SendKwhMeter(m_HwdID, 32*rank + 6, 255, m_pappHC, teleinfo.EJPHN/1000.0, name + " Heures Normales");
			SendKwhMeter(m_HwdID, 32*rank + 7, 255, m_pappHP, teleinfo.EJPHPM/1000.0, name + " Heures Pointe Mobile");
			SendKwhMeter(m_HwdID, 32*rank + 8, 255, teleinfo.PAPP, (teleinfo.EJPHN + teleinfo.EJPHPM)/1000.0, name + " Total");
			SendTextSensor(m_HwdID, 32*rank + 1, 255, teleinfo.rate, name + " Tarif en cours");
			SendAlertSensor(32*rank + 4, 255, ((teleinfo.PEJP == 30) ? 4 : 1), teleinfo.rate.c_str(), (name + " Alerte Pointe Mobile").c_str());
		}
		else if (teleinfo.OPTARIF.substr(0,3) == "BBR")
		{
			teleinfo.tariff="Tarif option TEMPO";
			m_pappHCJB=0;
			m_pappHPJB=0;
			m_pappHCJW=0;
			m_pappHPJW=0;
			m_pappHCJR=0;
			m_pappHPJR=0;
			if (teleinfo.PTEC.substr(3,1) == "B")
			{
				teleinfo.color="Bleu";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJB=teleinfo.PAPP;
				else
					m_pappHPJB=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "W")
			{
				teleinfo.color="Blanc";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJW=teleinfo.PAPP;
				else
					m_pappHPJW=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "R")
			{
				teleinfo.color="Rouge";
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJR=teleinfo.PAPP;
				else
					m_pappHPJR=teleinfo.PAPP;
			}
			else
			{
				teleinfo.color="Unknown";
			}
			SendKwhMeter(m_HwdID, 32*rank + 10, 255, m_pappHCJB, teleinfo.BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
			SendKwhMeter(m_HwdID, 32*rank + 11, 255, m_pappHPJB, teleinfo.BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
			SendKwhMeter(m_HwdID, 32*rank + 12, 255, m_pappHCJW, teleinfo.BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
			SendKwhMeter(m_HwdID, 32*rank + 13, 255, m_pappHPJW, teleinfo.BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
			SendKwhMeter(m_HwdID, 32*rank + 14, 255, m_pappHCJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
			SendKwhMeter(m_HwdID, 32*rank + 15, 255, m_pappHPJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
			SendKwhMeter(m_HwdID, 32*rank + 16, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW
				+ teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR)/1000.0, name + " Total");
			SendTextSensor(m_HwdID, 32*rank + 1, 255, "Jour " + teleinfo.color + ", " + teleinfo.rate, name + " Tarif en cours ");
			SendTextSensor(m_HwdID, 32*rank + 2, 255, "Demain, jour " + teleinfo.DEMAIN , name + " couleur demain");
		}
		// Common sensors for all rates
		if (teleinfo.triphase == false)
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST, 0, 0, name + " Courant");
			SendAlertSensor(32*rank + 1, 255, AlertLevel(teleinfo.IMAX, teleinfo.IINST, teleinfo.ISOUSC), "",
				(name + " Alerte courant maximal").c_str());
			SendPercentageSensor(32* rank + 1, 0, 255, (teleinfo.IINST*100)/teleinfo.IMAX, name + " Intensite souscrite");
		}
		else
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST1, (float)teleinfo.IINST2, (float)teleinfo.IINST3,
				name + " Courant");
			SendAlertSensor(32*rank + 1, 255, AlertLevel(teleinfo.IMAX1, teleinfo.IINST1, teleinfo.ISOUSC), "",
				(name + " Alerte courant maximal phase 1").c_str());
			SendAlertSensor(32*rank + 2, 255, AlertLevel(teleinfo.IMAX2, teleinfo.IINST2, teleinfo.ISOUSC), "",
				(name + " Alerte courant maximal phase 2").c_str());
			SendAlertSensor(32*rank + 3, 255, AlertLevel(teleinfo.IMAX3, teleinfo.IINST3, teleinfo.ISOUSC), "",
				(name + " Alerte courant maximal phase 3").c_str());
			if (teleinfo.IMAX1 > 0)
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.IINST1*100)/teleinfo.IMAX1,
					name + " Intensite souscrite phase 1");
			if (teleinfo.IMAX2 > 0)
				SendPercentageSensor(32 * rank + 2, 0, 255, (teleinfo.IINST2*100)/teleinfo.IMAX2,
					name + " Intensite souscrite phase 2");
			if (teleinfo.IMAX3 > 0)
				SendPercentageSensor(32 * rank + 3, 0, 255, (teleinfo.IINST3*100)/teleinfo.IMAX3,
					name + " Intensite souscrite phase 3");
		}
	}
}
