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
#include "../main/localtime_r.h"

CTeleinfoBase::CTeleinfoBase() {}

CTeleinfoBase::~CTeleinfoBase() {}


void CTeleinfoBase::ProcessTeleinfo(Teleinfo &teleinfo) 
{
ProcessTeleinfo("Teleinfo", 1, teleinfo);
}

void CTeleinfoBase::ProcessTeleinfo(const std::string &name, int rank, Teleinfo &teleinfo)
{
        uint32_t m_pappHC, m_pappHP, m_pappHCJB, m_pappHPJB, m_pappHCJW, m_pappHPJW, m_pappHCJR, m_pappHPJR, checksum, level;
        double flevel;
        time_t atime = mytime(NULL);

        // PAPP only exist on some meter versions. If not present, we can approximate it as (current x 230V)
        if ((teleinfo.PAPP == 0) && (teleinfo.IINST > 0))
                teleinfo.PAPP = (teleinfo.IINST * 230);

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
        checksum=teleinfo.BASE + teleinfo.HCHC + teleinfo.HCHP + teleinfo.EJPHN + teleinfo.EJPHPM + teleinfo.BBRHCJB \
                + teleinfo.BBRHPJB + teleinfo.BBRHCJW + teleinfo.BBRHPJB + teleinfo.BBRHCJR + teleinfo.BBRHPJR + teleinfo.PAPP;

        //Send data if value changed or at least every 5 minutes
        if ((teleinfo.previous != checksum) || (difftime(atime, teleinfo.last) >= 300))
        {
                teleinfo.previous = checksum;
                teleinfo.last = atime;
                if (teleinfo.OPTARIF == "BASE")
                {
                        teleinfo.tariff="Tarif de Base";
                        SendKwhMeter(m_HwdID, 10, 255, teleinfo.PAPP, teleinfo.BASE/1000.0, name + " Index");
                }
                else if (teleinfo.OPTARIF == "HC..")
                {
                        teleinfo.tariff="Tarif option Heures Creuses";
                        SendKwhMeter(m_HwdID, 12, 255, m_pappHC, teleinfo.HCHC/1000.0, name + " Heures Creuses");
                        SendKwhMeter(m_HwdID, 13, 255, m_pappHP, teleinfo.HCHP/1000.0, name + " Heures Pleines");
                        SendKwhMeter(m_HwdID, 14, 255, teleinfo.PAPP, (teleinfo.HCHP + teleinfo.HCHC)/1000.0, name + " Total");
                        SendTextSensor(m_HwdID, 11, 255, teleinfo.rate, name + " Tarif en cours");
                }
                else if (teleinfo.OPTARIF == "EJP.")
                {
                        teleinfo.tariff="Tarif option EJP";
                        SendKwhMeter(m_HwdID, 5, 255, m_pappHC, teleinfo.EJPHN/1000.0, name + " Heures Normales");
                        SendKwhMeter(m_HwdID, 16, 255, m_pappHP, teleinfo.EJPHPM/1000.0, name + " Heures Pointe Mobile");
                        SendKwhMeter(m_HwdID, 17, 255, teleinfo.PAPP, (teleinfo.EJPHN + teleinfo.EJPHPM)/1000.0, name + " Total");
                        SendTextSensor(m_HwdID, 11, 255, teleinfo.rate, name + " Tarif en cours");
                        SendAlertSensor(10+rank, 255, ((teleinfo.PEJP == 30) ? 4 : 1), (name + " Alerte Pointe Mobile").c_str());
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
                        SendKwhMeter(m_HwdID, 18, 255, m_pappHCJB, teleinfo.BBRHCJB/1000.0, "Teleinfo 1 Jour Bleu, Creux");
                        SendKwhMeter(m_HwdID, 19, 255, m_pappHPJB, teleinfo.BBRHPJB/1000.0, "Teleinfo 1 Jour Bleu, Plein");
                        SendKwhMeter(m_HwdID, 20, 255, m_pappHCJW, teleinfo.BBRHCJW/1000.0, "Teleinfo 1 Jour Blanc, Creux");
                        SendKwhMeter(m_HwdID, 21, 255, m_pappHPJW, teleinfo.BBRHPJW/1000.0, "Teleinfo 1 Jour Blanc, Plein");
                        SendKwhMeter(m_HwdID, 22, 255, m_pappHCJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Creux");
                        SendKwhMeter(m_HwdID, 23, 255, m_pappHPJR, teleinfo.BBRHCJR/1000.0, "Teleinfo 1 Jour Rouge, Plein");
                        SendKwhMeter(m_HwdID, 24, 255, teleinfo.PAPP, (teleinfo.BBRHCJB + teleinfo.BBRHPJB + teleinfo.BBRHCJW \
                                + teleinfo.BBRHPJW + teleinfo.BBRHCJR + teleinfo.BBRHPJR)/1000.0, name + " Total");
                        SendTextSensor(m_HwdID, 11, 255, "Jour " + teleinfo.color + ", " + teleinfo.rate, name + " Tarif en cours ");
                        SendTextSensor(m_HwdID, 25, 255, "Demain, jour " + teleinfo.DEMAIN , name + " couleur demain");
                }
                // Common sensors for all rates
                SendCurrentSensor(m_HwdID, 255, (float)teleinfo.IINST1, (float)teleinfo.IINST2, (float)teleinfo.IINST3, name + " Courant");

                // Alert level is 1 up to 100% usage, 2 then 3 above 100% load and 4 for maximum load (IMAX)
                if ((teleinfo.IMAX - teleinfo.IINST ) <=0)
                        level = 4;
                else
                        flevel = (3.0 * teleinfo.IINST + teleinfo.IMAX - 4.0 * teleinfo.ISOUSC) / (teleinfo.IMAX - teleinfo.ISOUSC);
                if (flevel > 4) flevel = 4;
                if (flevel < 1) flevel = 1;
                level = (int)round(flevel + 0.49);
                SendAlertSensor(rank, 255, level, (name + " Alerte courant maximal").c_str());
        }
}
