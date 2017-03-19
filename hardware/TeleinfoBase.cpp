/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 1.1
Description : This class is used by various Teleinfo hardware decoders to process and display data
		  It is currently used by EcoDevices, TeleinfoSerial
		  Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
			  http://www.enedis.fr/sites/default/files/ERDF-NOI-CPT_02E.pdf

History :
0.1 2017-03-03 : Creation
1.0 2017-03-15 : Release candidate
1.1 2017-03-18 : Updated to benefit from new messages in Alert sensors rather than simple text sensors
*/

#include "stdafx.h"
#include "TeleinfoBase.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"


CTeleinfoBase::CTeleinfoBase()
{
	m_p1power.ID = 1;
	m_p2power.ID = 2;
	m_p3power.ID = 3;
}

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
	int  rate_alert = 0, color_alert = 0, demain = 0;
	std::stringstream ss;
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
	if ((teleinfo.PAPP == 0) && ((teleinfo.IINST > 0)||(teleinfo.IINST1 > 0)||(teleinfo.IINST2 > 0)||(teleinfo.IINST3 > 0)))
		teleinfo.PAPP = (teleinfo.triphase ? (teleinfo.IINST1 + teleinfo.IINST2 + teleinfo.IINST3) : (teleinfo.IINST)) * 230;

	if (teleinfo.PTEC.substr(0,2) == "TH")
	{
		teleinfo.rate="Toutes Heures";
		rate_alert=0;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HC")
	{
		teleinfo.rate = "Heures Creuses";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
		rate_alert = 1;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HP")
	{
		teleinfo.rate = "Heures Pleines";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
		rate_alert = 4;
	}
	else if (teleinfo.PTEC.substr(0,2) == "HN")
	{
		teleinfo.rate = "Heures Normales";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
		rate_alert = 1;
	}
	else if (teleinfo.PTEC.substr(0,2) == "PM")
	{
		teleinfo.rate = "Pointe Mobile";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
		rate_alert = 4;
	}
	else
	{
		teleinfo.rate = "Unknown";
		teleinfo.tariff = "Undefined";
		rate_alert = 4;
	}

	// Checksum to detect changes between measures
	checksum=teleinfo.BASE + teleinfo.HCHC + teleinfo.HCHP + teleinfo.EJPHN + teleinfo.EJPHPM + teleinfo.BBRHCJB + teleinfo.PEJP\
		+ teleinfo.BBRHPJB + teleinfo.BBRHCJW + teleinfo.BBRHPJB + teleinfo.BBRHCJR + teleinfo.BBRHPJR + teleinfo.PAPP;

	//Send data if value changed, at most every minute and at least every 5 minutes
	if (((teleinfo.previous != checksum) && (difftime(atime, teleinfo.last) >= 60)) || (difftime(atime, teleinfo.last) >= 300))
	{
		teleinfo.previous = checksum;
		teleinfo.last = atime;
		m_p1power.usagecurrent = teleinfo.PAPP;
		if (teleinfo.OPTARIF == "BASE")
		{
			teleinfo.tariff="Tarif de Base";
			m_p1power.powerusage1 = teleinfo.BASE;
			m_p1power.powerusage2 = 0;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " Total").c_str(), 255);
			SendKwhMeter(m_HwdID, 32*rank + 2, 255, teleinfo.PAPP, teleinfo.BASE/1000.0, name + " Index kWh");
		}
		else if (teleinfo.OPTARIF == "HC..")
		{
			teleinfo.tariff="Tarif option Heures Creuses";
			SendKwhMeter(m_HwdID, 32*rank + 3, 255, m_pappHC, teleinfo.HCHC/1000.0, name + " kWh Heures Creuses");
			SendKwhMeter(m_HwdID, 32*rank + 4, 255, m_pappHP, teleinfo.HCHP/1000.0, name + " kWh Heures Pleines");
			m_p1power.powerusage1 = teleinfo.HCHC;
			m_p1power.powerusage2 = teleinfo.HCHP;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " kWh Total").c_str(), 255);
			//	SendKwhMeter(m_HwdID, 32*rank + 5, 255, teleinfo.PAPP, (teleinfo.HCHP + teleinfo.HCHC)/1000.0, name + " kWh Total");
		}
		else if (teleinfo.OPTARIF == "EJP.")
		{
			teleinfo.tariff="Tarif option EJP";
			SendKwhMeter(m_HwdID, 32*rank + 6, 255, m_pappHC, teleinfo.EJPHN/1000.0, name + " kWh Heures Normales");
			SendKwhMeter(m_HwdID, 32*rank + 7, 255, m_pappHP, teleinfo.EJPHPM/1000.0, name + " kWh Pointe Mobile");
			m_p1power.powerusage1 = teleinfo.EJPHN;
			m_p1power.powerusage2 = teleinfo.EJPHPM;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " kWh EJP").c_str(), 255);
			//		SendKwhMeter(m_HwdID, 32*rank + 8, 255, teleinfo.PAPP, (teleinfo.EJPHN + teleinfo.EJPHPM)/1000.0, name + " Total");
			SendAlertSensor(32*rank + 2, 255, ((teleinfo.PEJP == 30) ? 4 : 1), teleinfo.rate.c_str(), (name + " Pr\303\251annonce Pointe Mobile").c_str());
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
				color_alert = 1;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJB=teleinfo.PAPP;
				else
					m_pappHPJB=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "W")
			{
				teleinfo.color="Blanc";
				color_alert = 2;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJW=teleinfo.PAPP;
				else
					m_pappHPJW=teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3,1) == "R")
			{
				teleinfo.color="Rouge";
				color_alert = 4;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJR=teleinfo.PAPP;
				else
					m_pappHPJR=teleinfo.PAPP;
			}
			else
			{
				teleinfo.color = "Unknown";
				color_alert = 3;
			}
			//SendKwhMeter(m_HwdID, 32*rank + 10, 255, m_pappHCJB, teleinfo.BBRHCJB/1000.0, name + " Jour Bleu, Creux");
			//SendKwhMeter(m_HwdID, 32*rank + 11, 255, m_pappHPJB, teleinfo.BBRHPJB/1000.0, name + " Jour Bleu, Plein");
			//SendKwhMeter(m_HwdID, 32*rank + 12, 255, m_pappHCJW, teleinfo.BBRHCJW/1000.0, name + " Jour Blanc, Creux");
			//SendKwhMeter(m_HwdID, 32*rank + 13, 255, m_pappHPJW, teleinfo.BBRHPJW/1000.0, name + " Jour Blanc, Plein");
			//SendKwhMeter(m_HwdID, 32*rank + 14, 255, m_pappHCJR, teleinfo.BBRHCJR/1000.0, name + " Jour Rouge, Creux");
			//SendKwhMeter(m_HwdID, 32*rank + 15, 255, m_pappHPJR, teleinfo.BBRHCJR/1000.0, name + " Jour Rouge, Plein");
			SendKwhMeter(m_HwdID, 32*rank + 16, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW
				+ teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR)/1000.0, name + " kWh Total");
			m_p1power.powerusage1 = teleinfo.BBRHCJB;
			m_p1power.powerusage2 = teleinfo.BBRHPJB;
			m_p1power.usagecurrent = m_pappHCJB + m_pappHPJB;
			m_p2power.powerusage1 = teleinfo.BBRHCJW;
			m_p2power.powerusage2 = teleinfo.BBRHPJW;
			m_p2power.usagecurrent = m_pappHCJW + m_pappHPJW;
			m_p3power.powerusage1 = teleinfo.BBRHCJR;
			m_p3power.powerusage2 = teleinfo.BBRHPJR;
			m_p3power.usagecurrent = m_pappHCJR + m_pappHPJR;
			sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + "Jours Bleus").c_str(), 255);
			sDecodeRXMessage(this, (const unsigned char *)&m_p2power, (name + "Jours Blancs").c_str(), 255);
			sDecodeRXMessage(this, (const unsigned char *)&m_p3power, (name + "Jours Rouges").c_str(), 255);
			SendAlertSensor(32*rank + 2, 255, color_alert, ("Jour " + teleinfo.color).c_str(), (name + " Couleur du jour").c_str());
			if (teleinfo.DEMAIN == "BLEU")
				demain = 1;
			else if  (teleinfo.DEMAIN == "BLANC")
				demain = 2;
			else if  (teleinfo.DEMAIN == "ROUG")
			{
				demain = 3;
				teleinfo.DEMAIN = "ROUGE";
			}
			else demain = 0;
			SendAlertSensor(32*rank + 3, 255, demain, ("Demain, jour " + teleinfo.DEMAIN).c_str() , (name + " Couleur demain").c_str());
		}
		// Common sensors for all rates
		SendAlertSensor(32*rank + 1, 255, rate_alert, teleinfo.rate.c_str(), (name + " Tarif en cours").c_str());
		if (teleinfo.triphase == false)
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST, 0, 0, name + " Courant");
			ss.clear();
			ss << "Courant " << teleinfo.IINST << "A, " << teleinfo.ISOUSC << "A souscrits";
			SendAlertSensor(32*rank + 4, 255, AlertLevel(teleinfo.IMAX, teleinfo.IINST, teleinfo.ISOUSC),
				ss.str().c_str(), (name + " Alerte courant maximal").c_str());
			SendPercentageSensor(32* rank + 1, 0, 255, (teleinfo.IINST*100)/float(teleinfo.IMAX), name + " Intensite souscrite");
		}
		else
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST1, (float)teleinfo.IINST2, (float)teleinfo.IINST3,
				name + " Courant");
			ss.clear();
			ss << "Courant " << teleinfo.IINST1 << "A, " << teleinfo.ISOUSC << "A souscrits";
			SendAlertSensor(32*rank + 4, 255, AlertLevel(teleinfo.IMAX1, teleinfo.IINST1, teleinfo.ISOUSC),
				ss.str().c_str(), (name + " Alerte courant phase 1").c_str());
			ss.clear();
			ss << "Courant " << teleinfo.IINST2 << "A, " << teleinfo.ISOUSC << "A souscrits";
			SendAlertSensor(32*rank + 5, 255, AlertLevel(teleinfo.IMAX2, teleinfo.IINST2, teleinfo.ISOUSC),
				ss.str().c_str(), (name + " Alerte courant phase 2").c_str());
			ss.clear();
			ss << "Courant " << teleinfo.IINST3 << "A, " << teleinfo.ISOUSC << "A souscrits";
			SendAlertSensor(32*rank + 6, 255, AlertLevel(teleinfo.IMAX3, teleinfo.IINST3, teleinfo.ISOUSC),
				ss.str().c_str(), (name + " Alerte courant phase 3").c_str());
			if (teleinfo.IMAX1 > 0)
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.IINST1*100)/float(teleinfo.IMAX1),
					name + " Charge phase 1");
			if (teleinfo.IMAX2 > 0)
				SendPercentageSensor(32 * rank + 2, 0, 255, (teleinfo.IINST2*100)/float(teleinfo.IMAX2),
					name + " Charge phase 2");
			if (teleinfo.IMAX3 > 0)
				SendPercentageSensor(32 * rank + 3, 0, 255, (teleinfo.IINST3*100)/float(teleinfo.IMAX3),
					name + " charge phase 3");
		}
	}
}
