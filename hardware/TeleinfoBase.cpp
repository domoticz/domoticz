/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 1.6
Description : This class is used by various Teleinfo hardware decoders to process and display data
		  It is currently used by EcoDevices, TeleinfoSerial
		  Detailed information on the Teleinfo protocol can be found at (version 5, 16/03/2015)
			  http://www.enedis.fr/sites/default/files/Enedis-NOI-CPT_02E.pdf

History :
0.1 2017-03-03 : Creation
1.0 2017-03-15 : Release candidate
1.1 2017-03-18 : Updated to benefit from new messages in Alert sensors rather than simple text sensors
1.2 2017-03-21 : Various bug fix and reverting to using P1SmartMeter as users requested
1.3 2017-04-01 : Added RateLimit
1.4 2017-04-13 : Added DataTimeout
1.5 2017-04-20 : Fix bug affecting "demain" for white days
1.6 2017-12-17 : Fix bug affecting meters not providing PAPP, thanks to H. Lertouani
*/

#include "stdafx.h"
#include "TeleinfoBase.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include <bitset>			 // This is necessary to compile on Windows

#ifdef _DEBUG
#define DEBUG_TeleinfoBase
#endif

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


// Alert level is 1 up to 80% usage, 2 between 80% and 90%, 3 between 90% and 98%, 4 above
int CTeleinfoBase::AlertLevel(int Iinst, int Isousc, char* text)
{
	int level;
	float flevel;

	flevel = (float)(Iinst*100) / Isousc;
	level = 1;
	sprintf(text, " < 80%% de %iA souscrits", Isousc);
	if (flevel > 80)
	{
		level = 2;
		sprintf(text, ">80%% et <90%% de %iA souscrits", Isousc);
	}
	if (level > 90)
	{
		level = 3;
		sprintf(text, ">90%% et <98%% de %iA souscrits", Isousc);
	}
	if (level > 98)
	{
		level = 3;
		sprintf(text, ">98%% de %iA souscrits", Isousc);
	}
	return level;
}


void CTeleinfoBase::ProcessTeleinfo(const std::string &name, int rank, Teleinfo &teleinfo)
{
	uint32_t m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR;
	int rate_alert = 0, color_alert = 0, demain_alert = 0;
	int alertI1, alertI2, alertI3, alertEJP, alertPPOT;
	char szTmp[100];
	std::string message;
	time_t atime = mytime(NULL);

	// We need to limit the number of Teleinfo devices per hardware because of the subID in sensors. i
	if ((rank < 1) || (rank > 4))
	{
		_log.Log(LOG_ERROR,"(%s) TeleinfoBase: Invalid rank passed to function (%i), must be between 1 and 4", m_Name.c_str(), rank);
		return;
	}
	rank = rank -1;		// Now it is 0 to 3
    
	// Guess if we are running with one phase or three
	// some devices like EcoDevices always send all variables so presence/absence of IINSTx is not significant
	// Also, EcoDevices always sends the same value for IINST and IINST1, so check must be done on IINST2 and IINST3
	if (((teleinfo.IINST == 0) && (teleinfo.IINST1 > 0)) || (teleinfo.IINST2 > 0) || (teleinfo.IINST3 > 0))
		teleinfo.triphase  = true;

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if (teleinfo.withPAPP == false)
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
		rate_alert = 2;
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
		rate_alert = 3;
	}

	// Process only if maximum time between updates (5mn) has been reached or power consumption changed
	// If it did not, then alerts and intensity have not changed either
	#ifdef DEBUG_TeleinfoBase
	_log.Log(LOG_NORM,"(%s) TeleinfoBase called. Power changed: %s, last update %.f sec", m_Name.c_str(), (teleinfo.pAlertPAPP != teleinfo.PAPP)?"true":"false", difftime(atime, teleinfo.last));
	#endif
	if ((teleinfo.pAlertPAPP != teleinfo.PAPP) || (difftime(atime, teleinfo.last) >= 290))
	{
		teleinfo.pAlertPAPP = teleinfo.PAPP;

		//Send data at mamximum rate specified in settings, and at least every 5mn (minus 10s as a grace period for the watchdog)
		if ((difftime(atime, teleinfo.last) >= m_iRateLimit) || (difftime(atime, teleinfo.last) >= 290))
		{
			teleinfo.last = atime;
			m_p1power.usagecurrent = teleinfo.PAPP;
			if (teleinfo.OPTARIF == "BASE")
			{
				#ifdef DEBUG_TeleinfoBase
				_log.Log(LOG_STATUS,"Teleinfo Base: %i, PAPP: %i", teleinfo.BASE, teleinfo.PAPP);
				#endif
				teleinfo.tariff="Tarif de Base";
				m_p1power.powerusage1 = teleinfo.BASE;
				m_p1power.powerusage2 = 0;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " kWh Total").c_str(), 255);
			}
			else if (teleinfo.OPTARIF == "HC..")
			{
				teleinfo.tariff="Tarif option Heures Creuses";
				SendKwhMeter(m_HwdID, 32*rank + 3, 255, m_pappHC, teleinfo.HCHC/1000.0, name + " kWh Heures Creuses");
				SendKwhMeter(m_HwdID, 32*rank + 4, 255, m_pappHP, teleinfo.HCHP/1000.0, name + " kWh Heures Pleines");
				m_p1power.powerusage1 = teleinfo.HCHP;
				m_p1power.powerusage2 = teleinfo.HCHC;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " kWh Total").c_str(), 255);
			}
			else if (teleinfo.OPTARIF == "EJP.")
			{
				teleinfo.tariff="Tarif option EJP";
				SendKwhMeter(m_HwdID, 32*rank + 6, 255, m_pappHC, teleinfo.EJPHN/1000.0, name + " kWh Heures Normales");
				SendKwhMeter(m_HwdID, 32*rank + 7, 255, m_pappHP, teleinfo.EJPHPM/1000.0, name + " kWh Pointe Mobile");
				m_p1power.powerusage1 = teleinfo.EJPHPM;
				m_p1power.powerusage2 = teleinfo.EJPHN;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + " kWh EJP").c_str(), 255);
				alertEJP =  (teleinfo.PEJP == 30) ? 4 : 1;
				if (alertEJP != teleinfo.pAlertEJP)
				{
					SendAlertSensor(32*rank + 2, 255, alertEJP, teleinfo.rate, name + " Preannonce Pointe Mobile");
					teleinfo.pAlertEJP = alertEJP;
				}
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
					color_alert = 3;
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
				SendKwhMeter(m_HwdID, 32*rank + 16, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW
					+ teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR)/1000.0, name + " kWh Total");
				m_p1power.powerusage1 = teleinfo.BBRHPJB;
				m_p1power.powerusage2 = teleinfo.BBRHCJB;
				m_p1power.usagecurrent = m_pappHCJB + m_pappHPJB;
				m_p2power.powerusage1 = teleinfo.BBRHPJW;
				m_p2power.powerusage2 = teleinfo.BBRHCJW;
				m_p2power.usagecurrent = m_pappHCJW + m_pappHPJW;
				m_p3power.powerusage1 = teleinfo.BBRHPJR;
				m_p3power.powerusage2 = teleinfo.BBRHCJR;
				m_p3power.usagecurrent = m_pappHCJR + m_pappHPJR;
				sDecodeRXMessage(this, (const unsigned char *)&m_p1power, (name + "kWh Jours Bleus").c_str(), 255);
				sDecodeRXMessage(this, (const unsigned char *)&m_p2power, (name + "kWh Jours Blancs").c_str(), 255);
				sDecodeRXMessage(this, (const unsigned char *)&m_p3power, (name + "kWh Jours Rouges").c_str(), 255);
				if (color_alert != teleinfo.pAlertColor)
				{
					SendAlertSensor(32*rank + 2, 255, color_alert, "Jour " + teleinfo.color, name + " Couleur du jour");
					teleinfo.pAlertColor = color_alert;
				}
				if (teleinfo.DEMAIN == "BLEU")
					demain_alert = 1;
				else if  (teleinfo.DEMAIN == "BLAN")
				{
					demain_alert = 2;
                                        teleinfo.DEMAIN = "BLANC";
                                }
				else if  (teleinfo.DEMAIN == "ROUG")
				{
					demain_alert = 3;
					teleinfo.DEMAIN = "ROUGE";
				}
				else demain_alert = 0;
				if (demain_alert != teleinfo.pAlertDemain)
				{
					SendAlertSensor(32*rank + 3, 255, demain_alert, "Demain, jour " + teleinfo.DEMAIN, name + " Couleur demain");
					teleinfo.pAlertDemain = demain_alert;
				}
			}
			if (teleinfo.triphase == false)
			{
				SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST, 0, 0, name + " Courant");
				SendPercentageSensor(32* rank + 1, 0, 255, (teleinfo.IINST*100)/float(teleinfo.ISOUSC), name + " Pourcentage de Charge");
			}
			else
			{
				SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST1, (float)teleinfo.IINST2, (float)teleinfo.IINST3,
					name + " Courant");
				if (teleinfo.ISOUSC > 0)
				{
					SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.IINST1*100)/float(teleinfo.ISOUSC),
						name + " Charge phase 1");
					SendPercentageSensor(32 * rank + 2, 0, 255, (teleinfo.IINST2*100)/float(teleinfo.ISOUSC),
						name + " Charge phase 2");
					SendPercentageSensor(32 * rank + 3, 0, 255, (teleinfo.IINST3*100)/float(teleinfo.ISOUSC),
						name + " charge phase 3");
				}
			}
		}
		// Common sensors for all rates
		// Alerts can be updated at every call and are not subject to the "rate limit" interval
		if (rate_alert != teleinfo.pAlertRate)
		{
			SendAlertSensor(32*rank + 1, 255, rate_alert, teleinfo.rate, name + " Tarif en cours");
			teleinfo.pAlertRate = rate_alert;
		}
		if (teleinfo.triphase == false)
		{
			alertI1 =  AlertLevel(teleinfo.IINST, teleinfo.ISOUSC, szTmp);
			if (alertI1 != teleinfo.pAlertI1)
			{
				SendAlertSensor(32*rank + 4, 255, alertI1, szTmp, name + " Alerte courant");
				teleinfo.pAlertI1 = alertI1;
			}
		}
		else
		{
			alertI1 = AlertLevel(teleinfo.IINST1, teleinfo.ISOUSC, szTmp);
			if (alertI1 != teleinfo.pAlertI1)
			{
				SendAlertSensor(32*rank + 4, 255, alertI1, szTmp, name + " Alerte phase 1");
				teleinfo.pAlertI1 = alertI1;
			}
			alertI2 = AlertLevel(teleinfo.IINST2, teleinfo.ISOUSC, szTmp);
			if (alertI2 != teleinfo.pAlertI2)
			{
				SendAlertSensor(32*rank + 5, 255, alertI2, szTmp, name + " Alerte phase 2");
				teleinfo.pAlertI2 = alertI2;
			}
			alertI3 = AlertLevel(teleinfo.IINST3, teleinfo.ISOUSC, szTmp);
			if (alertI3 != teleinfo.pAlertI3)
			{
				SendAlertSensor(32*rank + 6, 255, alertI3, szTmp, name + " Alerte phase 3");
				teleinfo.pAlertI3 = alertI3;
			}
			if (teleinfo.PPOT != teleinfo.pAlertPPOT)
			{
				if (teleinfo.PPOT !=0)
					alertPPOT = 4;
				else
					alertPPOT = 1;
				teleinfo.pAlertPPOT = teleinfo.PPOT;
				teleinfo.PPOT>>=1;
				std::stringstream ss;
				ss << "Bitmap phases: " <<std::bitset<3>(~teleinfo.PPOT);
				message = ss.str();
				SendAlertSensor(32*rank+7, 255, alertPPOT, message, " Alerte Potentiels");
			}
		}
	}
}
