/*
Domoticz Software : http://domoticz.com/
File : TeleinfoBase.cpp
Author : Blaise Thauvin
Version : 1.6
Description : This class is used by various Teleinfo hardware decoders to process and display data
		  It is currently used by EcoDevices, TeleinfoSerial
		  Detailed information on the Teleinfo protocol (Enedis-NOI-CPT_54E) can be found at (version 3, 01/06/2018)
		  https://www.enedis.fr/media/2035/download

History :
0.1 2017-03-03 : Creation
1.0 2017-03-15 : Release candidate
1.1 2017-03-18 : Updated to benefit from new messages in Alert sensors rather than simple text sensors
1.2 2017-03-21 : Various bug fix and reverting to using P1SmartMeter as users requested
1.3 2017-04-01 : Added RateLimit
1.4 2017-04-13 : Added DataTimeout
1.5 2017-04-20 : Fix bug affecting "demain" for white days
1.6 2017-12-17 : Fix bug affecting meters not providing PAPP, thanks to H. Lertouani
1.7 2019-09-13 : Remove to not update the PAPP value if similar to previous (was causing some values to never update), P. Darré
*/

#include "stdafx.h"
#include "TeleinfoBase.h"
#include "../main/Logger.h"
#include "../main/Helper.h"
#include <bitset>			 // This is necessary to compile on Windows

#ifdef _DEBUG
#define DEBUG_TeleinfoBase
#endif

CTeleinfoBase::CTeleinfoBase()
{
	m_p1power.ID = 1;
	m_p2power.ID = 2;
	m_p3power.ID = 3;
	m_bDisableCRC = false;

	InitTeleinfo();
}

void CTeleinfoBase::InitTeleinfo()
{
	m_bufferpos = 0;
	m_teleinfo.CRCmode1 = 255;	 // Guess the CRC mode at first run
	m_teleinfo.waitingFirstBlock = true; 
}

void CTeleinfoBase::ProcessTeleinfo(Teleinfo& teleinfo)
{
	ProcessTeleinfo("Teleinfo", 1, teleinfo);
}

// Alert level is 1 up to 80% usage, 2 between 80% and 90%, 3 between 90% and 98%, 4 above
int CTeleinfoBase::AlertLevel(int Iinst, int Isousc, int Sinsts, int Pref, char* text)
{
	int level = 1;
	float flevel = 0;

	if (Isousc > 0)
	{
		flevel = (float)(Iinst * 100) / Isousc;
		sprintf(text, " < 80%% de %iA souscrits", Isousc);
	}
	else if (Pref > 0)
	{
		flevel = (float)(Sinsts * 100) / (Pref * 1000);
		sprintf(text, " < 80%% de %iKVA souscrits", Pref);
	}
	else
		sprintf(text, "Pas d'info de souscription !");
	
	if (flevel > 98)
	{
		level = 4;
		if (Isousc > 0)
			sprintf(text, ">98%% de %iA souscrits", Isousc);
		else
			sprintf(text, ">98%% de %iKVA souscrits", Pref);
	}
	else if (flevel > 90)
	{
		level = 3;
		if (Isousc > 0)
			sprintf(text, ">90%% et <98%% de %iA souscrits", Isousc);
		else
			sprintf(text, ">90%% et <98%% de %iKVA souscrits", Pref);
	}
	else if (flevel > 80)
	{
		level = 2;
		if (Isousc > 0)
			sprintf(text, ">80%% et <90%% de %iA souscrits", Isousc);
		else
			sprintf(text, ">80%% et <90%% de %iKVA souscrits", Pref);
	}
	return level;
}

void CTeleinfoBase::ProcessTeleinfo(const std::string& name, int rank, Teleinfo& teleinfo)
{
	uint32_t m_pappHC = 0, m_pappHP = 0, m_pappHCJB = 0, m_pappHPJB = 0, m_pappHCJW = 0, m_pappHPJW = 0, m_pappHCJR = 0, m_pappHPJR = 0;
	int rate_alert = 0, color_alert = 0, demain_alert = 0;
	int alertI1, alertI2, alertI3, alertEJP, alertPPOT;
	char szTmp[100];
	std::string message;
	time_t atime = mytime(nullptr);

	// We need to limit the number of Teleinfo devices per hardware because of the subID in sensors. i
	if ((rank < 1) || (rank > 4))
	{
		Log(LOG_ERROR, "Invalid rank passed to function (%i), must be between 1 and 4", rank);
		return;
	}
	rank = rank - 1;		// Now it is 0 to 3

	// Guess if we are running with one phase or three
	// some devices like EcoDevices always send all variables so presence/absence of IINSTx is not significant
	// Also, EcoDevices always sends the same value for IINST and IINST1, so check must be done on IINST2 and IINST3
	if (((teleinfo.IINST == 0) && (teleinfo.IINST1 > 0)) || (teleinfo.IINST2 > 0) || (teleinfo.IINST3 > 0))
		teleinfo.triphase = true;

	// PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
	if (teleinfo.withPAPP == false)
		teleinfo.PAPP = (teleinfo.triphase ? (teleinfo.IINST1 + teleinfo.IINST2 + teleinfo.IINST3) : (teleinfo.IINST)) * 230;

	if (teleinfo.PTEC.substr(0, 2) == "TH")
	{
		teleinfo.rate = "Toutes Heures";
		rate_alert = 0;
	}
	else if (teleinfo.PTEC.substr(0, 2) == "HC")
	{
		teleinfo.rate = "Heures Creuses";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
		rate_alert = 1;
	}
	else if (teleinfo.PTEC.substr(0, 2) == "HP")
	{
		teleinfo.rate = "Heures Pleines";
		m_pappHC = 0;
		m_pappHP = teleinfo.PAPP;
		rate_alert = 2;
	}
	else if (teleinfo.PTEC.substr(0, 2) == "HN")
	{
		teleinfo.rate = "Heures Normales";
		m_pappHC = teleinfo.PAPP;
		m_pappHP = 0;
		rate_alert = 1;
	}
	else if (teleinfo.PTEC.substr(0, 2) == "PM")
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

	// Process only if maximum time between updates (5mn) has been reached (or power consumption changed => removed)
	// If it did not, then alerts and intensity have not changed either
#ifdef DEBUG_TeleinfoBase
	Log(LOG_NORM, "TeleinfoBase called. Power changed: %s, last update %.f sec", (teleinfo.pAlertPAPP != teleinfo.PAPP) ? "true" : "false", difftime(atime, teleinfo.last));
#endif
	// 1.6 version: if ((teleinfo.pAlertPAPP != teleinfo.PAPP) || (difftime(atime, teleinfo.last) >= 290))

	teleinfo.pAlertPAPP = teleinfo.PAPP;

	//Send data at mamximum rate specified in settings, and at least every 5mn (minus 10s as a grace period for the watchdog)
	if ((difftime(atime, teleinfo.last) >= m_iRateLimit) || (difftime(atime, teleinfo.last) >= 290))
	{
		teleinfo.last = atime;
		m_p1power.usagecurrent = teleinfo.PAPP;
		if (teleinfo.EAIT > 0)
		{ // Energie Active Injectée Totale
			m_p1power.delivcurrent = teleinfo.SINSTI;
			m_p1power.powerdeliv1 = teleinfo.EAIT;
			m_p1power.powerdeliv2 = 0;
		}

		if (teleinfo.OPTARIF == "BASE")
		{
#ifdef DEBUG_TeleinfoBase
			Log(LOG_STATUS, "Teleinfo Base: %i, PAPP: %i", teleinfo.BASE, teleinfo.PAPP);
#endif
			teleinfo.tariff = "Tarif de Base";
			m_p1power.powerusage1 = teleinfo.BASE;
			m_p1power.powerusage2 = 0;
			sDecodeRXMessage(this, (const unsigned char*)&m_p1power, (name + " kWh Total").c_str(), 255, nullptr);
		}
		else if (teleinfo.OPTARIF == "HC..")
		{
			teleinfo.tariff = "Tarif option Heures Creuses";
			SendKwhMeter(m_HwdID, 32 * rank + 3, 255, m_pappHC, teleinfo.HCHC / 1000.0, name + " kWh Heures Creuses");
			SendKwhMeter(m_HwdID, 32 * rank + 4, 255, m_pappHP, teleinfo.HCHP / 1000.0, name + " kWh Heures Pleines");
			m_p1power.powerusage1 = teleinfo.HCHP;
			m_p1power.powerusage2 = teleinfo.HCHC;
			sDecodeRXMessage(this, (const unsigned char*)&m_p1power, (name + " kWh Total").c_str(), 255, nullptr);
		}
		else if (teleinfo.OPTARIF == "EJP.")
		{
			teleinfo.tariff = "Tarif option EJP";
			SendKwhMeter(m_HwdID, 32 * rank + 6, 255, m_pappHC, teleinfo.EJPHN / 1000.0, name + " kWh Heures Normales");
			SendKwhMeter(m_HwdID, 32 * rank + 7, 255, m_pappHP, teleinfo.EJPHPM / 1000.0, name + " kWh Pointe Mobile");
			m_p1power.powerusage1 = teleinfo.EJPHPM;
			m_p1power.powerusage2 = teleinfo.EJPHN;
			sDecodeRXMessage(this, (const unsigned char*)&m_p1power, (name + " kWh EJP").c_str(), 255, nullptr);
			alertEJP = (teleinfo.PEJP == 30) ? 4 : 1;
			if (alertEJP != teleinfo.pAlertEJP)
			{
				SendAlertSensor(32 * rank + 2, 255, alertEJP, teleinfo.rate, name + " Preannonce Pointe Mobile");
				teleinfo.pAlertEJP = alertEJP;
			}
		}
		else if (teleinfo.OPTARIF.substr(0, 3) == "BBR")
		{
			teleinfo.tariff = "Tarif option TEMPO";
			m_pappHCJB = 0;
			m_pappHPJB = 0;
			m_pappHCJW = 0;
			m_pappHPJW = 0;
			m_pappHCJR = 0;
			m_pappHPJR = 0;
			if (teleinfo.PTEC.substr(3, 1) == "B")
			{
				teleinfo.color = "Bleu";
				color_alert = 1;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJB = teleinfo.PAPP;
				else
					m_pappHPJB = teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3, 1) == "W")
			{
				teleinfo.color = "Blanc";
				color_alert = 2;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJW = teleinfo.PAPP;
				else
					m_pappHPJW = teleinfo.PAPP;
			}
			else if (teleinfo.PTEC.substr(3, 1) == "R")
			{
				teleinfo.color = "Rouge";
				color_alert = 3;
				if (teleinfo.rate == "Heures Creuses")
					m_pappHCJR = teleinfo.PAPP;
				else
					m_pappHPJR = teleinfo.PAPP;
			}
			else
			{
				teleinfo.color = "Unknown";
				color_alert = 3;
			}
			SendKwhMeter(m_HwdID, 32 * rank + 16, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW
				+ teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR) / 1000.0, name + " kWh Total");
			m_p1power.powerusage1 = teleinfo.BBRHPJB;
			m_p1power.powerusage2 = teleinfo.BBRHCJB;
			m_p1power.usagecurrent = m_pappHCJB + m_pappHPJB;
			m_p2power.powerusage1 = teleinfo.BBRHPJW;
			m_p2power.powerusage2 = teleinfo.BBRHCJW;
			m_p2power.usagecurrent = m_pappHCJW + m_pappHPJW;
			m_p3power.powerusage1 = teleinfo.BBRHPJR;
			m_p3power.powerusage2 = teleinfo.BBRHCJR;
			m_p3power.usagecurrent = m_pappHCJR + m_pappHPJR;
			sDecodeRXMessage(this, (const unsigned char*)&m_p1power, (name + "kWh Jours Bleus").c_str(), 255, nullptr);
			sDecodeRXMessage(this, (const unsigned char*)&m_p2power, (name + "kWh Jours Blancs").c_str(), 255, nullptr);
			sDecodeRXMessage(this, (const unsigned char*)&m_p3power, (name + "kWh Jours Rouges").c_str(), 255, nullptr);
			if (color_alert != teleinfo.pAlertColor)
			{
				SendAlertSensor(32 * rank + 2, 255, color_alert, "Jour " + teleinfo.color, name + " Couleur du jour");
				teleinfo.pAlertColor = color_alert;
			}
			if (teleinfo.DEMAIN == "BLEU")
				demain_alert = 1;
			else if (teleinfo.DEMAIN == "BLAN")
			{
				demain_alert = 2;
				teleinfo.DEMAIN = "BLANC";
			}
			else if (teleinfo.DEMAIN == "ROUG")
			{
				demain_alert = 3;
				teleinfo.DEMAIN = "ROUGE";
			}
			else demain_alert = 0;
			if (demain_alert != teleinfo.pAlertDemain)
			{
				SendAlertSensor(32 * rank + 3, 255, demain_alert, "Demain, jour " + teleinfo.DEMAIN, name + " Couleur demain");
				teleinfo.pAlertDemain = demain_alert;
			}
		}

		if (teleinfo.triphase == false)
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST, 0, 0, name + " Courant");
			if (teleinfo.URMS1 > 0)
				SendVoltageSensor(m_HwdID + rank, 0, 255, (float)teleinfo.URMS1, name + " Tension");
			if (teleinfo.ISOUSC > 0)
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.IINST * 100) / float(teleinfo.ISOUSC), name + " Pourcentage de Charge");
			else
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.PAPP * 100) / float(teleinfo.PREF * 1000), name + " Pourcentage de Charge");
		}
		else
		{
			SendCurrentSensor(m_HwdID + rank, 255, (float)teleinfo.IINST1, (float)teleinfo.IINST2, (float)teleinfo.IINST3,
				name + " Courant");
			if (teleinfo.URMS1 > 0)
				SendVoltageSensor(m_HwdID + rank + 1, 0, 255, (float)teleinfo.URMS1, name + " Tension phase 1");
			if (teleinfo.URMS2 > 0)
				SendVoltageSensor(m_HwdID + rank + 2, 0, 255, (float)teleinfo.URMS2, name + " Tension phase 2");
			if (teleinfo.URMS3 > 0)
				SendVoltageSensor(m_HwdID + rank + 3, 0, 255, (float)teleinfo.URMS3, name + " Tension phase 3");
			if (teleinfo.ISOUSC > 0)
			{
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.IINST1 * 100) / float(teleinfo.ISOUSC),
					name + " Charge phase 1");
				SendPercentageSensor(32 * rank + 2, 0, 255, (teleinfo.IINST2 * 100) / float(teleinfo.ISOUSC),
					name + " Charge phase 2");
				SendPercentageSensor(32 * rank + 3, 0, 255, (teleinfo.IINST3 * 100) / float(teleinfo.ISOUSC),
					name + " charge phase 3");
			}
			else
			{
				SendPercentageSensor(32 * rank + 1, 0, 255, (teleinfo.SINSTS1 * 100) / float(teleinfo.PREF * 1000),
					name + " Charge phase 1");
				SendPercentageSensor(32 * rank + 2, 0, 255, (teleinfo.SINSTS2 * 100) / float(teleinfo.PREF * 1000),
					name + " Charge phase 2");
				SendPercentageSensor(32 * rank + 3, 0, 255, (teleinfo.SINSTS3 * 100) / float(teleinfo.PREF * 1000),
					name + " charge phase 3");
			}
		}
	}
	// Common sensors for all rates
	// Alerts can be updated at every call and are not subject to the "rate limit" interval
	if (rate_alert != teleinfo.pAlertRate)
	{
		SendAlertSensor(32 * rank + 1, 255, rate_alert, teleinfo.rate, name + " Tarif en cours");
		teleinfo.pAlertRate = rate_alert;
	}
	if (teleinfo.triphase == false)
	{
		alertI1 = AlertLevel(teleinfo.IINST, teleinfo.ISOUSC, teleinfo.PAPP, teleinfo.PREF, szTmp);
		if (alertI1 != teleinfo.pAlertI1)
		{
			SendAlertSensor(32 * rank + 4, 255, alertI1, szTmp, name + " Alerte courant");
			teleinfo.pAlertI1 = alertI1;
		}
	}
	else
	{
		alertI1 = AlertLevel(teleinfo.IINST1, teleinfo.ISOUSC, teleinfo.PAPP, teleinfo.PREF, szTmp);
		if (alertI1 != teleinfo.pAlertI1)
		{
			SendAlertSensor(32 * rank + 4, 255, alertI1, szTmp, name + " Alerte phase 1");
			teleinfo.pAlertI1 = alertI1;
		}
		alertI2 = AlertLevel(teleinfo.IINST2, teleinfo.ISOUSC, teleinfo.PAPP, teleinfo.PREF, szTmp);
		if (alertI2 != teleinfo.pAlertI2)
		{
			SendAlertSensor(32 * rank + 5, 255, alertI2, szTmp, name + " Alerte phase 2");
			teleinfo.pAlertI2 = alertI2;
		}
		alertI3 = AlertLevel(teleinfo.IINST3, teleinfo.ISOUSC, teleinfo.PAPP, teleinfo.PREF, szTmp);
		if (alertI3 != teleinfo.pAlertI3)
		{
			SendAlertSensor(32 * rank + 6, 255, alertI3, szTmp, name + " Alerte phase 3");
			teleinfo.pAlertI3 = alertI3;
		}
		if (teleinfo.PPOT != teleinfo.pAlertPPOT)
		{
			if (teleinfo.PPOT != 0)
				alertPPOT = 4;
			else
				alertPPOT = 1;
			teleinfo.pAlertPPOT = teleinfo.PPOT;
			teleinfo.PPOT >>= 1;
			std::stringstream ss;
			ss << "Bitmap phases: " << std::bitset<3>(~teleinfo.PPOT);
			message = ss.str();
			SendAlertSensor(32 * rank + 7, 255, alertPPOT, message, " Alerte Potentiels");
		}
	}
	if(teleinfo.STGE != UINT32_MAX)
	{ // Process status register if found (Linky standard mode only, refer to Enedis specs for details)
		if((teleinfo.STGE & 0x1) != (teleinfo.prevSTGE & 0x1) || teleinfo.prevSTGE == UINT32_MAX)
		{
			bool bContactState = teleinfo.STGE & 0x01;
			SendAlertSensor(32 * rank + 8, 255, bContactState, (bContactState == 0)?"Contact fermé":"Contact ouvert", name + " Contact sec");
		}
		if((teleinfo.STGE & 0xE) != (teleinfo.prevSTGE & 0xE) || teleinfo.prevSTGE == UINT32_MAX)
		{
			int iValue = (teleinfo.STGE & 0xE) >> 1;
			const char *sLevelNames[] = { "fermé",
				"ouvert sur surpuissance",
				"ouvert sur surtension",
				"ouvert sur délestage",
				"ouvert sur ordre CPL ou Euridis",
				"ouvert sur une surchauffe avec une valeur du courant supérieure au courant de commutation maximal",
				"ouvert sur une surchauffe avec une valeur de courant inférieure au courant de commutation maximal",
				"Non défini" };
			SendAlertSensor(32 * rank + 9, 255, iValue, sLevelNames[iValue], name + " Organe de coupure");
		}
		if((teleinfo.STGE & 0x10) != (teleinfo.prevSTGE & 0x10) || teleinfo.prevSTGE == UINT32_MAX)
		{
			bool iState = (teleinfo.STGE >> 4) & 0x1;
			SendAlertSensor(32 * rank + 10, 255, iState, (iState == 0)?"fermé":"ouvert", name + " Cache-bornes");
		}
		if((teleinfo.STGE & 0x40) != (teleinfo.prevSTGE & 0x40) || teleinfo.prevSTGE == UINT32_MAX)
		{
			int iOvervoltage = (teleinfo.STGE >> 6) & 0x1;
			SendAlertSensor(32 * rank + 11, 255, iOvervoltage, (iOvervoltage == 0)?"Pas de surtension":"Surtension", name + " Surtension");
		}
		if((teleinfo.STGE & 0x80) != (teleinfo.prevSTGE & 0x80) || teleinfo.prevSTGE == UINT32_MAX)
		{
			int iOverload = (teleinfo.STGE >> 7) & 0x1;
			SendAlertSensor(32 * rank + 12, 255, iOverload, (iOverload == 0)?"Pas de dépassement de la puissance de référence":"Dépassement de la puissance de référence", name + " Dépassement puissance");
		}
		if((teleinfo.STGE & 0x100) != (teleinfo.prevSTGE & 0x100) || teleinfo.prevSTGE == UINT32_MAX)
		{
			int iMode = (teleinfo.STGE >> 8) & 0x1;
			SendAlertSensor(32 * rank + 13, 255, iMode, (iMode == 0)?"Consommateur":"Producteur", name + " Fonctionnement");
		}
		teleinfo.prevSTGE = teleinfo.STGE;
	}
}

//Example of data received from power meter
//ADCO 271028237723 C
//OPTARIF HC.. <
//ISOUSC 45 ?
//HCHC 013149843 '
//HCHP 013016759 3
//PTEC HP..
//IINST 002 Y
//IMAX 049 L
//PAPP 00450 *
//HHPHC D /
//MOTDETAT 000000 B

/* Explanation of the checksum computation issued from the official EDF specification

a "checksum" is calculated on the set of characters from the beginning of the label field to the end of the field given character SP included.
We first make ??the sum of all ASCII codes of all characters.
to avoid introduce ASCII (00 to 1F hex) functions, it retains only the six least significant bits of
result (this translates into a logical AND between the amount previously calculated and 03Fh).
Finally, we added 20 hexadecimal. The result will always be a printable ASCII character (sign, digit,
capital letter) of from 0x20 to hexadecimal 0x5F

Le "checksum" est calcule sur l'ensemble des caracteres allant du debut du champ etiquette a la fin du champ
donnee, caractere SP inclus. On fait tout d'abord la somme des codes ASCII de tous ces caracteres. Pour eviter
d'introduire des fonctions ASCII (00  1F en hexadcimal), on ne conserve que les six bits de poids faible du
resultat obtenu (cette operation se traduit par un ET logique entre la somme precedemment calculee et 03Fh).
Enfin, on ajoute 20 en hexadecimal. Le resultat sera donc toujours un caractre ASCII imprimable (signe, chiffre,
lettre majuscule) allant de 20 a 5F en hexadcimal.

Un deuxime mode de calcul existe qui prend aussi le caractre de sparation final dans le calcul.
*/

bool CTeleinfoBase::isCheckSumOk(const std::string& sLine, int& isMode1)
{
	if (sLine.size() < 2)
		return false;
	unsigned int checksum, mode1 = 0x00, mode2 = 0x00;
	bool line_ok = false;

	checksum = sLine[sLine.size() - 1];
	int i = 0;
	for (i = 0; i < (int)sLine.size() - 2; i++)
	{
		mode1 += sLine[i];
	}
	mode2 = ((mode1 + sLine[i]) & 0x3F) + 0x20;
	mode1 = (mode1 & 0x3F) + 0x20;

	if (mode1 == checksum)
	{
		line_ok = true;
		if (isMode1 != (int)true)// This will evaluate to false when isMode still equals to 255 at second run
		{
			isMode1 = true;
			Log(LOG_STATUS, "CRC check mode set to 1");
		}
	}
	else if (mode2 == checksum)
	{
		line_ok = true;
		if (isMode1 != false)	 // if this is first run, will still be at 255
		{
			isMode1 = false;
			Log(LOG_STATUS, "CRC check mode set to 2");
		}
	}
	else						 // Don't send an error on the first run as the line is probably truncated, wait for mode to be initialised
		if (isMode1 != 255)
			Log(LOG_ERROR, "CRC check failed on Teleinfo line '%s' using both modes 1 and 2. Line skipped.", sLine.c_str());

	if (line_ok)
	{
		Debug(DEBUG_HARDWARE, "CRC check passed on Teleinfo line '%s'. Line processed", sLine.c_str());
	}

	return line_ok;
}

void CTeleinfoBase::MatchLine()
{
	std::string label, vString;
	std::vector<std::string> splitresults;
	unsigned long value;
	std::string sline(m_buffer);

	Debug(DEBUG_HARDWARE, "Frame : #%s#", sline.c_str());

	// Is the line we got worth analysing any further?
	if ((sline.size() < 4) || (sline[0] == 0x0a))
	{
		Debug(DEBUG_HARDWARE, "Frame #%s# ignored, too short or irrelevant", sline.c_str());
		return;
	}

	// Extract the elements, return if not enough and line is invalid
	StringSplit(sline, "	", splitresults); // standard mode is using TAB (0x09) as delimiter
	if (splitresults.size() < 3)
	{ // Decoding using standard mode failed, fall back using historic mode
		splitresults.clear();
		StringSplit(sline, " ", splitresults); // historic mode is using space (0x20) as delimiter
	}
	if (splitresults.size() < 3)
	{
		Log(LOG_ERROR, "Frame #%s# passed the checksum test but failed analysis", sline.c_str());
		return;
	}

	label = splitresults[0];
	vString = splitresults[1];
	value = atoi(splitresults[1].c_str());

	// Historic mode
	if (label == "ADCO" || label == "ADSC") 
	{
		m_teleinfo.ADCO = vString;
		// waiting a complete Block received to have m_teleinfo structure completely filled before processing anything => avoid strange behavior at init
		if (m_teleinfo.waitingFirstBlock)
			m_teleinfo.waitingFirstBlock = false;
		else
		{
			Debug(DEBUG_HARDWARE, "frame complete, PAPP: %i, PTEC: %s, OPTARIF: %s", m_teleinfo.PAPP, m_teleinfo.PTEC.c_str(), m_teleinfo.OPTARIF.c_str());
			ProcessTeleinfo(m_teleinfo);
		}
	}
	else if (label == "OPTARIF") m_teleinfo.OPTARIF = vString;
	else if (label == "ISOUSC") m_teleinfo.ISOUSC = value;
	else if (label == "PAPP")
	{
		m_teleinfo.PAPP = value;
		m_teleinfo.withPAPP = true;
	}
	else if (label == "PTEC")  m_teleinfo.PTEC = vString;
	else if (label == "IINST") m_teleinfo.IINST = value;
	else if (label == "BASE") m_teleinfo.BASE = value;
	else if (label == "HCHC") m_teleinfo.HCHC = value;
	else if (label == "HCHP") m_teleinfo.HCHP = value;
	else if (label == "EJPHPM") m_teleinfo.EJPHPM = value;
	else if (label == "EJPHN") m_teleinfo.EJPHN = value;
	else if (label == "BBRHCJB") m_teleinfo.BBRHCJB = value;
	else if (label == "BBRHPJB") m_teleinfo.BBRHPJB = value;
	else if (label == "BBRHCJW") m_teleinfo.BBRHCJW = value;
	else if (label == "BBRHPJW") m_teleinfo.BBRHPJW = value;
	else if (label == "BBRHCJR") m_teleinfo.BBRHCJR = value;
	else if (label == "BBRHPJR") m_teleinfo.BBRHPJR = value;
	else if (label == "DEMAIN") m_teleinfo.DEMAIN = vString;
	else if (label == "IINST1") m_teleinfo.IINST1 = value;
	else if (label == "IINST2") m_teleinfo.IINST2 = value;
	else if (label == "IINST3") m_teleinfo.IINST3 = value;
	else if (label == "PPOT")  m_teleinfo.PPOT = value;

	// Standard mode
	else if (label == "EAST") m_teleinfo.BASE = value;
	else if (label == "EASF01") 
	{
		// With the TEMPO subscription (= BBR : Blue White Red) this counter is for peak Blue hours
		if (m_teleinfo.OPTARIF == "BBR")
			m_teleinfo.BBRHCJB = value;
		else
			m_teleinfo.HCHC = value;
	}
	else if (label == "EASF02") 
	{
		// With the TEMPO subscription (= BBR : Blue White Red) this counter is for off-peak Blue hours
		if (m_teleinfo.OPTARIF == "BBR")
			m_teleinfo.BBRHPJB = value;
		else
			m_teleinfo.HCHP = value;
	}
	else if (label == "EASF03") m_teleinfo.BBRHCJW = value; // TEMPO peak White hours
	else if (label == "EASF04") m_teleinfo.BBRHPJW = value; // TEMPO off-peak White hours
	else if (label == "EASF05") m_teleinfo.BBRHCJR = value; // TEMPO peak Red hours
	else if (label == "EASF06") m_teleinfo.BBRHPJR = value; // TEMPO off-peak Red hours
	else if (label == "PREF") m_teleinfo.PREF = value;
	else if (label == "IRMS1") m_teleinfo.IINST = value;
	else if (label == "IRMS2")
	{ // triphase
		m_teleinfo.IINST1 = m_teleinfo.IINST;
		m_teleinfo.IINST = 0;
		m_teleinfo.IINST2 = value;
	}
	else if (label == "IRMS3") m_teleinfo.IINST3 = value;
	else if (label == "NGTF") 
	{
		std::string ngtfString = stdstring_trim(vString);
		// Different subscriptions exist and it becomes difficult to remain compatible with historical behavior
		// The NGTF string is vendor dependent...
		// Example : At the seller TotalEnergie there is a subscription with super off-peak hours which are on the 3rd meter EASF03
		// I still put it in off-peak behavior for the moment: It will just miss super off-peak hours: @TODO
		if (ngtfString == "H PLEINE/CREUSE" || ngtfString == "H SUPER CREUSES" || ngtfString == "HC SEM ET HC WE")
			m_teleinfo.OPTARIF = "HC..";
		else if (ngtfString == "TEMPO")
			// In standard mode, TEMPO string is identical to BBR in historical mode (EDF only)
			m_teleinfo.OPTARIF = "BBR";
		else if (ngtfString == "PRODUCTEUR")
			// subscription for electricity production only
			// OPTARIF must be set to BASE to have a display on producer meters
			m_teleinfo.OPTARIF = "BASE";
		else
			// For historic mode compatibility
			m_teleinfo.OPTARIF = ngtfString;
	}
	else if (label == "SINSTS")
	{
		m_teleinfo.PAPP = value;
		m_teleinfo.withPAPP = true;
	}
	else if (label == "SINSTS1") m_teleinfo.SINSTS1 = value;
	else if (label == "SINSTS2") m_teleinfo.SINSTS2 = value;
	else if (label == "SINSTS3") m_teleinfo.SINSTS3 = value;
	else if (label == "URMS1") m_teleinfo.URMS1 = value;
	else if (label == "URMS2") m_teleinfo.URMS2 = value;
	else if (label == "URMS3") m_teleinfo.URMS3 = value;
	else if (label == "NTARF" && m_teleinfo.OPTARIF != "")
	{
		// BASE subscription 
		if (m_teleinfo.OPTARIF == "BASE") m_teleinfo.PTEC = "TH..";
		// TEMPO subscription 
		else if (m_teleinfo.OPTARIF == "BBR")
			switch (value)
			{
			case 1: m_teleinfo.PTEC = "HC B"; break;
			case 2: m_teleinfo.PTEC = "HP B"; break;
			case 3: m_teleinfo.PTEC = "HC W"; break;
			case 4: m_teleinfo.PTEC = "HP W"; break;
			case 5: m_teleinfo.PTEC = "HC R"; break;
			case 6: m_teleinfo.PTEC = "HP R"; break;
			default: break;
			}
		// Others subscription like (HP / HC) 
		else if (value == 1)
			m_teleinfo.PTEC = "HC..";
		else if (value == 2)
			m_teleinfo.PTEC = "HP..";
	}
	else if (label == "EAIT") m_teleinfo.EAIT = value;
	else if (label == "SINSTI") m_teleinfo.SINSTI = value;
	else if (label == "STGE")
	{  // Status register, hexadecimal string (without 0x)
		m_teleinfo.STGE = strtoul(splitresults[1].c_str(), nullptr, 16);

		// Color of tomorow
		int tomorow = ( m_teleinfo.STGE & 0x0C000000 ) >> 26;

		// If 0 then tomorow color is identical to today's color 
		if ( tomorow == 0 ) tomorow = ( m_teleinfo.STGE & 0x03000000 ) >> 24;

		switch (tomorow)
		{
			case 1: m_teleinfo.DEMAIN = "BLEU"; break;
			case 2: m_teleinfo.DEMAIN = "BLAN"; break;
			case 3: m_teleinfo.DEMAIN = "ROUG"; break;		
			default: break;
		}
	}
	
	mytime(&m_LastHeartbeat); // keep heartbeat happy
}

void CTeleinfoBase::ParseTeleinfoData(const char* pData, int Len)
{
	int ii = 0;
	while (ii < Len)
	{
		const char c = pData[ii];

		if ((c == 0x0d) || (c == 0x00) || (c == 0x02) || (c == 0x03))
		{
			ii++;
			continue;
		}

		m_buffer[m_bufferpos] = c;
		if (c == 0x0a || m_bufferpos == sizeof(m_buffer) - 1)
		{
			// discard newline, close string, parse line and clear it.
			m_buffer[m_bufferpos] = 0;

			// process the line only if the checksum is ok and user did not request to bypass CRC verification
			if ((m_bDisableCRC) || isCheckSumOk(std::string(m_buffer), m_teleinfo.CRCmode1))
			{
				MatchLine();
			}
			m_bufferpos = 0;
		}
		else
		{
			m_bufferpos++;
		}
		ii++;
	}
}
