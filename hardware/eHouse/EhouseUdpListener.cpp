//#define DYNAMICMEM 1
/* eHouse Server for Linux
 * ONLY for usage with eHouse system controllers
 * Author: Robert Jarzabek
 * http://Smart.eHouse.Pro/ eHouse DIY EN
 * http://eHouse.Biz/ eHouse Shop
 *
 * Reception of UDP broadcast status directly from "eHouse" Ethernet, WiFi, PRO controllers
 * eHouse 1, CAN, RF, AURA controllers under eHouse PRO supervision support
 * Tested linux Ubuntu, RPI1/2/3, Banana PI/PRO, WINDOWS 7++
 * Netbeans / Visual Studio S2017
 Recent update: 2018-08-16
 */

#include "stdafx.h"
#include "globals.h"
#include "status.h"

#ifndef WIN32
#include <unistd.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#else
#include <io.h>
#define F_OK		0
#define SHUT_RDWR	SD_BOTH
#endif

#include "../eHouseTCP.h"
#include "../hardwaretypes.h"
#include "../../main/RFXtrx.h"
#include "../../main/localtime_r.h"
#include "../../main/Logger.h"
#include "../../main/mainworker.h"
#include "../../main/SQLHelper.h"
unsigned char TESTTEST = 0;
/*unsigned char eHEnableAutoDiscovery = 1;
unsigned char eHEnableProDiscovery = 1;
unsigned char eHEnableAlarmInputs = 0;
int TCPSocket = -1;
*/
//allocate dynamically names structure only during discovery of eHouse PRO controller
void eHouseTCP::eCMaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	//	if (strlen((char *) &ECMn) < 1)
	if (m_ECMn == nullptr)
	{
		LOG(LOG_STATUS, "Allocating CommManager LAN Controller (192.168.%d.%d)", devaddrh, devaddrl);
		m_ECMn = (struct CommManagerNamesT *) malloc(sizeof(struct CommManagerNamesT));
		if (m_ECMn == nullptr)
		{
			LOG(LOG_ERROR, "CAN'T Allocate ECM Names Memory");
			return;
		}
		m_ECMn->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
		m_ECMn->AddrH = devaddrh;
		m_ECMn->AddrL = devaddrl;
		m_ECM = (union CMStatusT *) malloc(sizeof(union CMStatusT));
		m_ECMPrv = (union CMStatusT *) malloc(sizeof(union CMStatusT));
		if (m_ECM == nullptr)
			LOG(LOG_ERROR, "CAN'T Allocate ECM Memory");
		if (m_ECMPrv == nullptr)
			LOG(LOG_ERROR, "CAN'T Allocate ECMPrev Memory");
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//allocate dynamically names structure only during discovery of eHouse PRO controller
//////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eHPROaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	//	if (strlen((char *) &eHouseProN) < 1)
	if (m_eHouseProN == nullptr)
	{
		LOG(LOG_STATUS, "Allocating eHouse PRO Controller (192.168.%d.%d)", devaddrh, devaddrl);
		m_eHouseProN = (struct eHouseProNamesT *) malloc(sizeof(struct eHouseProNamesT)); //Giz: ?? why use mallocs ?
		if (m_eHouseProN == nullptr)
		{
			LOG(LOG_ERROR, "CAN'T Allocate PRO Names Memory");
			return;
		}
		m_eHouseProN->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
		m_eHouseProN->AddrH[0] = devaddrh;
		m_eHouseProN->AddrL[0] = devaddrl;
		m_eHouseProStatus = (union eHouseProStatusUT *)  malloc(sizeof(union eHouseProStatusUT));
		m_eHouseProStatusPrv = (union eHouseProStatusUT *) malloc(sizeof(union eHouseProStatusUT));
		if (m_eHouseProStatus == nullptr)
			LOG(LOG_ERROR, "CAN'T Allocate PRO Stat Memory");
		if (m_eHouseProStatusPrv == nullptr)
			LOG(LOG_ERROR, "CAN'T Allocate PRO Stat PRV Memory");
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//allocate dynamically names structure only during discovery of LAN controller
//////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eAURAaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	int i;
	if (eHEIndex >= MAX_AURA_DEVS) return;
	if (eHEIndex < 0) return;
	for (i = 0; i <= eHEIndex; i++)
	{
		//if (strlen((char *) & (AuraN[i])) < 1)
		if (m_AuraN[i] == nullptr)
		{
			LOG(LOG_STATUS, "Allocating Aura Thermostat (%d.%d)", devaddrh, i + 1);
			m_AuraN[i] = (struct AuraNamesT *) malloc(sizeof(struct AuraNamesT));
			if (m_AuraN[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate AURA Names Memory");
				return;
			}
			m_AuraN[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			m_AuraN[i]->AddrH = devaddrh;
			m_AuraN[i]->AddrL = i + 1;
			m_AuraDev[i] = (struct AURAT *) malloc(sizeof(struct AURAT));
			m_AuraDevPrv[i] = (struct AURAT *) malloc(sizeof(struct AURAT));
			m_adcs[i] = (struct CtrlADCT *) malloc(sizeof(struct CtrlADCT));
			if (m_adcs[i] == nullptr)
				LOG(LOG_ERROR, "CAN'T Allocate ADCs Memory");
			if (m_AuraDev[i] == nullptr)
				LOG(LOG_ERROR, "CAN'T Allocate AURA Stat Memory");
			if (m_AuraDevPrv[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate AURA Stat Prv Memory");
				return;
			}
			m_AuraN[i]->BinaryStatus[0] = 0;	//modified flag
			m_AuraDevPrv[i]->Addr = 0;		//modified flag
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//allocate dynamically names structure only during discovery of LAN controller
///////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eHEaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	int i;
	if (eHEIndex >= ETHERNET_EHOUSE_RM_MAX) return;
	if (eHEIndex < 0) return;
	for (i = 0; i <= eHEIndex; i++)
	{
		//	if (strlen((char *) & (eHEn[i])) < 1)
		if (m_eHEn[i] == nullptr)
		{
			LOG(LOG_STATUS, "Allocating eHouse LAN controller (192.168.%d.%d)", devaddrh, i + m_INITIAL_ADDRESS_LAN);
			m_eHEn[i] = (struct EtherneteHouseNamesT *) malloc(sizeof(struct EtherneteHouseNamesT));
			if (m_eHEn[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate LAN Names Memory");
				return;
			}
			m_eHEn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			m_eHEn[i]->AddrH = devaddrh;
			m_eHEn[i]->AddrL = i + m_INITIAL_ADDRESS_LAN;
			m_eHERMs[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			m_eHERMPrev[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			if (m_eHERMs[i] == nullptr)
				LOG(LOG_ERROR, "CAN'T Allocate LAN Stat Memory");
			if (m_eHERMPrev[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate LAN Stat Prv Memory");
				return;
			}
			m_eHEn[i]->BinaryStatus[0] = 0;	//modification flags
			m_eHERMPrev[i]->data[0] = 0;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
//alocate dynamically names structure only during discovery of RS-485 controller
//////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eHaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	int i;
	if (eHEIndex >= EHOUSE1_RM_MAX) return;
	if (eHEIndex < 0) return;
	for (i = 0; i <= eHEIndex; i++)
	{
		//		if (strlen((char *) &eHn[i]) < 1)
		if (m_eHn[i] == nullptr)
		{
			m_eHn[i] = (struct eHouse1NamesT *) malloc(sizeof(struct eHouse1NamesT));
			if (m_eHn[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate RS-485 Names Memory");
				return;
			}
			m_eHn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			if (i == 0)
			{
				m_eHn[i]->AddrH = 1;
				m_eHn[i]->AddrL = 1;
			}
			else
				if (i == EHOUSE1_RM_MAX)
				{
					m_eHn[i]->AddrH = 2;
					m_eHn[i]->AddrL = 1;
				}
				else
				{
					m_eHn[i]->AddrH = 55;
					m_eHn[i]->AddrL = i;
				}
			LOG(LOG_STATUS, "Allocating eHouse RS-485 Controller (%d,%d)", m_eHn[i]->AddrH, m_eHn[i]->AddrL);
			m_eHRMs[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			m_eHRMPrev[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			if (m_eHRMs[i] == nullptr)
				LOG(LOG_ERROR, "CANT Allocate RS-485 Stat Memory");
			if (m_eHRMPrev[i] == nullptr)
			{
				LOG(LOG_ERROR, "CANT Allocate RS-485 Stat Prev Memory");
				return;
			}
			m_eHn[i]->BinaryStatus[0] = 0;		//modification flags
			m_eHRMPrev[i]->data[0] = 0;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//alocate dynamically names structure only during discovery of LAN controller
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eHWIFIaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	int i;
	if (eHEIndex >= EHOUSE_WIFI_MAX) return;
	if (eHEIndex < 0) return;
	for (i = 0; i <= eHEIndex; i++)
	{
		//			if (strlen((char *) &eHWIFIn[i]) < 1)
		if (m_eHWIFIn[i] == nullptr)
		{
			LOG(LOG_STATUS, "Allocating eHouse WiFi Controller (192.168.%d.%d)", devaddrh, m_INITIAL_ADDRESS_WIFI + i);
			m_eHWIFIn[i] = (struct WiFieHouseNamesT *) malloc(sizeof(struct WiFieHouseNamesT));
			if (m_eHWIFIn[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate WiFi Names Memory");
				return;
			}
			m_eHWIFIn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			m_eHWIFIn[i]->AddrH = devaddrh;
			m_eHWIFIn[i]->AddrL = devaddrl;
			m_eHWiFi[i] = (union WiFiStatusT *) malloc(sizeof(union WiFiStatusT));
			m_eHWIFIs[i] = (union WIFIFullStatT *) malloc(sizeof(union WIFIFullStatT));
			m_eHWIFIPrev[i] = (union WIFIFullStatT *) malloc(sizeof(union WIFIFullStatT));
			if (m_eHWIFIs[i] == nullptr)
				LOG(LOG_ERROR, "CAN'T Allocate WiFi Stat Memory");
			if (m_eHWIFIPrev[i] == nullptr)
			{
				LOG(LOG_ERROR, "CAN'T Allocate WiFi Stat Memory");
				return;
			}
			m_eHWIFIn[i]->BinaryStatus[0] = 0;
			m_eHWIFIPrev[i]->data[0] = 0;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// IsCM() detecting by address if device status is CommManager || LevelManager - restricted for CMs/LMs
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char eHouseTCP::IsCM(unsigned char addrh, unsigned char addrl)
{
	if (addrl >= 250U)
		return 1;
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////
// Index of eHouse RS-485 controllers in array (0-HM, 1..n-1-RM, nmax=EM)
//////////////////////////////////////////////////////////////////////////////////////////////////
signed int eHouseTCP::IndexOfeHouseRS485(unsigned char devh, unsigned char devl)
{
	int i = 0;
	if ((devh != 55) && (devh != 1) && (devh != 2)) return -1;
	if ((devh == 1) && (devl == 1)) return 0;
	if ((devh == 2) && (devl == 1)) return EHOUSE1_RM_MAX;
	if (devl < 0) return -1;
	if (devl < EHOUSE1_RM_MAX) return devl;
	return -1;								//NOT AN  RS485 controller
}
////////////////////////////////////////////////////////////////////////////////////////
// Update Aura thermostat to DevStatus
//////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::UpdateAuraToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	char sval[10];
	float acurr, aprev;
	unsigned char size = sizeof(m_AuraN[index]->Outs) / sizeof(m_AuraN[index]->Outs[0]);
	size = 8;
	if (index > MAX_AURA_DEVS - 1)
	{
		_log.Log(LOG_STATUS, "Aura to much !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;
	}
	size = 4;
	acurr = (float)(m_AuraDev[index]->TempSet);
	aprev = (float)(m_AuraDevPrv[index]->TempSet);
	//printf("\r\n!!!!!Aura [%s]  TempSet =%f, Temp=%f\r\n", Auras[index],acurr, AuraDev[index].Temp);
	if ((acurr != aprev) || (m_AuraDevPrv[index]->Addr == 0))
	{
		if (m_CHANGED_DEBUG) _log.Log(LOG_STATUS, "Temp Preset #%d changed to: %d", (int)index, (int)acurr);
		sprintf(sval, "%.1f", (m_AuraDev[index]->TempSet));
		AddrH = 0x81;
		AddrL = m_AuraDev[index]->Addr;
		//printf("[AURA] %x,%x\r\n", AddrH, AddrL);
		UpdateSQLStatus(AddrH, AddrL, EH_AURA, VISUAL_AURA_PRESET, 1, m_AuraDev[index]->RSSI, (int)round(m_AuraDev[index]->TempSet * 10), sval, (int)round(m_AuraDev[index]->volt));
	}

	//ADCs
	//for (i = 0; i < size; i++)
	{
		acurr = (float)(m_AuraDev[index]->Temp);
		aprev = (float)(m_AuraDevPrv[index]->Temp);
		if ((acurr != aprev) || (m_AuraDevPrv[index]->Addr == 0))
		{
			sprintf(sval, "%.1f", ((float)acurr));
			AddrH = 0x81;
			AddrL = m_AuraDev[index]->Addr;
			UpdateSQLStatus(AddrH, AddrL, EH_AURA, VISUAL_AURA_IN, 1, m_AuraDev[index]->RSSI, (int)round(acurr * 10), sval, (int)round(m_AuraDev[index]->volt));
			if (m_CHANGED_DEBUG) _log.Log(LOG_STATUS, "Temp #%d changed to: %f", (int)index, acurr);
		}
	}
	memcpy(&m_AuraDevPrv[index]->Addr, &m_AuraDev[index]->Addr, sizeof(struct AURAT));
}
/////////////////////////////////////////////////////////////////////////////////////
// eHouse LAN (CommManager status update)
void eHouseTCP::UpdateCMToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	if (AddrL < 250) return;  //only CM/LM in this routine
	unsigned char size = sizeof(m_ECMn->Outs) / sizeof(m_ECMn->Outs[0]);

	for (i = 0; i < size; i++)
	{
		curr = (m_ECM->CMB.outs[i / 8] >> (i % 8)) & 0x1;
		prev = (m_ECMPrv->CMB.outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (m_ECMPrv->data[1] == 0))
		{
			if (m_CHANGED_DEBUG) if (m_ECMPrv->data[1]) _log.Log(LOG_STATUS, "[CM 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_LAN, 0x21, i, 100, curr, "", 100);
		}
	}
	//inputs
	size = 48;

	{
		//deb((char*) & "Inputs", ECM.CMB.inputs, sizeof(ECM.CMB.inputs));
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->CMB.inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->CMB.inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				if (m_CHANGED_DEBUG) if (m_ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}
		if (m_ECM->CMB.CURRENT_ADC_PROGRAM != m_ECMPrv->CMB.CURRENT_ADC_PROGRAM)
		{
			curr = m_ECM->CMB.CURRENT_ADC_PROGRAM;
			if (m_CHANGED_DEBUG)
				if (m_ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		/* if (ECM.CMB.CURRENT_PROFILE!=ECMPrv.CMB.CURRENT_PROFILE)
			{
			curr = eCM.CMB.
					.CURRENT_PROFILE;
			if (CHANGED_DEBUG) if (ECMPrv.data[0]) printf("[LAN 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}*/
		if (m_ECM->CMB.CURRENT_PROGRAM != m_ECMPrv->CMB.CURRENT_PROGRAM)
		{
			curr = m_ECM->CMB.CURRENT_PROGRAM;
			if (m_CHANGED_DEBUG)
				if (m_ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//deb((char*) & "Dimm", ECM.CMB.DIMM, sizeof(ECM.CMB.DIMM));
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->CMB.DIMM[i]);
			prev = (m_ECMPrv->CMB.DIMM[i]);
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_DIMMER_OUT, i, 100, (curr * 100) / 255, "", 100);
			}
		}

		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (m_ECM->CMB.ADC[i].MSB);
			aprev = (m_ECMPrv->CMB.ADC[i].MSB);

			acurr = acurr << 8;
			aprev = aprev << 8;

			acurr += (m_ECM->CMB.ADC[i].LSB);
			aprev += (m_ECMPrv->CMB.ADC[i].LSB);

			if ((acurr != aprev) || (m_ECMPrv->data[1] == 0))
			{
				//if (eHERMPrev[index].data[0]) printf("[LAN 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ADC_IN, i, 100, acurr, sval, 100);
			}
		}

	}


	{

		if (m_ECM->CMB.CURRENT_ZONE != m_ECMPrv->CMB.CURRENT_ZONE)
		{
			curr = m_ECM->CMB.CURRENT_ZONE;
			if (m_CHANGED_DEBUG)
				if (m_ECMPrv->data[1])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ZONE #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		size = 48;
		//Outputs signals
	//    deb((char*) & "I2C out", (unsigned char *) &ECM.data[STATUS_OUT_I2C], sizeof(20));
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->data[STATUS_OUT_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->data[STATUS_OUT_I2C + i / 8] >> (i % 8)) & 0x1;

			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_ECMPrv->data[1])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Outs #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			}
		}

		size = 48;
		//Inputs signals
 //    deb((char*) & "I2C inputs", (unsigned char *) &ECM.data[STATUS_INPUTS_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->data[STATUS_INPUTS_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->data[STATUS_INPUTS_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_ECMPrv->data[1])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100,curr, "", 100);
			}
		}

		//alarms signals
		//deb((char*) & "I2C alarm", (unsigned char *) &ECM.data[STATUS_ALARM_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->data[STATUS_ALARM_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->data[STATUS_ALARM_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1] ) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Alarm #%d changed to: %d", (int) AddrH, (int) AddrL, (int)i, (int) curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ALARM_IN, i, 100,curr, "", 100);
			}
		}
		//deb((char*) & "I2C warning", (unsigned char *) &ECM.data[STATUS_WARNING_I2C],8);
		//Warning signals
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->data[STATUS_WARNING_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->data[STATUS_WARNING_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Warning #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_WARNING_IN, i, 100,curr, "", 100);
			}
		}
		//Monitoring signals
		///deb((char*) & "I2C monit", (unsigned char *) &ECM.data[STATUS_MONITORING_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (m_ECM->data[STATUS_MONITORING_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (m_ECMPrv->data[STATUS_MONITORING_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Monitoring #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				//            UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MONITORING_IN, i, 100,curr, "", 100);
			}
		}
		memcpy(m_ECMPrv, m_ECM, sizeof(union CMStatusT));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// Update ERM/PoolManager status to DB
/////////////////////////////////////////////////////////////////////////////////////

void eHouseTCP::UpdateLanToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	unsigned char size = sizeof(m_eHEn[index]->Outs) / sizeof(m_eHEn[index]->Outs[0]); //for cm only
	if (AddrL < 250) size = 32;
	else size = 77;
	if (index > ETHERNET_EHOUSE_RM_MAX - 1)
	{
		_log.Log(LOG_ERROR, "LAN INDEX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;
	}
	if (AddrL > 249)  //No CM/LM in this function
	{
		return;
	}
	//deb("ERM: ", EHERMs[index].data, sizeof(eHERMs[index].data));
	for (i = 0; i < size; i++)
	{
		curr = (m_eHERMs[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (m_eHERMPrev[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (m_eHERMPrev[index]->data[0] == 0))
		{
			if (m_CHANGED_DEBUG)
				if (m_eHERMPrev[index]->data[0]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_LAN, 0x21, i, 100, curr, "", 100);
		}
	}
	//inputs
	size = 24;
	{
		for (i = 0; i < size; i++)
		{
			curr = (m_eHERMs[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHERMPrev[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}

		if (m_eHERMs[index]->eHERM.CURRENT_ADC_PROGRAM != m_eHERMPrev[index]->eHERM.CURRENT_ADC_PROGRAM)
		{
			curr = m_eHERMs[index]->eHERM.CURRENT_ADC_PROGRAM;
			if (m_CHANGED_DEBUG)
				if (m_eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		if (m_eHERMs[index]->eHERM.CURRENT_PROFILE != m_eHERMPrev[index]->eHERM.CURRENT_PROFILE)
		{
			curr = m_eHERMs[index]->eHERM.CURRENT_PROFILE;
			if (m_CHANGED_DEBUG)
				if (m_eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Profile #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		if (m_eHERMs[index]->eHERM.CURRENT_PROGRAM != m_eHERMPrev[index]->eHERM.CURRENT_PROGRAM)
		{
			curr = m_eHERMs[index]->eHERM.CURRENT_PROGRAM;
			if (m_CHANGED_DEBUG)
				if (m_eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (m_eHERMs[index]->eHERM.Dimmers[i]);
			prev = (m_eHERMPrev[index]->eHERM.Dimmers[i]);
			if ((curr != prev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_DIMMER_OUT, i, 100, (curr * 100) / 255, "", 100);
			}
		}

		size = 16;
		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHERMs[index]->eHERM.ADCH[i]);
			aprev = (m_eHERMPrev[index]->eHERM.ADCH[i]);
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] ADC H Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}
		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHERMs[index]->eHERM.ADCL[i]);
			aprev = (m_eHERMPrev[index]->eHERM.ADCL[i]);
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] ADC Low Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHERMs[index]->eHERM.ADC[i]);
			aprev = (m_eHERMPrev[index]->eHERM.ADC[i]);
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ADC_IN, i, 100, acurr, sval, 100);
			}
		}

		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHERMs[index]->eHERM.TempL[i]);
			aprev = (m_eHERMPrev[index]->eHERM.TempL[i]);
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] TEMP Low Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}

		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHERMs[index]->eHERM.TempH[i]);
			aprev = (m_eHERMPrev[index]->eHERM.TempH[i]);
			int midpoint = m_eHERMs[index]->eHERM.TempH[i] - m_eHERMs[index]->eHERM.TempL[i];
			midpoint /= 2;
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Temp H Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
				sprintf(sval, "%.1f", ((float)(m_eHERMs[index]->eHERM.TempH[i] + m_eHERMs[index]->eHERM.TempL[i])) / 20);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MCP9700_PRESET, i, 100, midpoint, sval, 100);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (m_eHERMs[index]->eHERM.Temp[i]);
			aprev = (m_eHERMPrev[index]->eHERM.Temp[i]);
			if ((acurr != aprev) || (m_eHERMPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%.1f", ((float)acurr) / 10);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MCP9700_IN, i, 100, acurr, sval, 100);
			}
		}
	}
	memcpy(m_eHERMPrev[index], m_eHERMs[index], sizeof(union ERMFullStatT));
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// eHouse PRO/BMS - centralized controller + integrations
/////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::UpdatePROToSQL(unsigned char AddrH, unsigned char AddrL)
{
	unsigned char curr, prev;
	int i = 0;
	int size = sizeof(m_eHouseProStatus->status.Outputs) / sizeof(m_eHouseProStatus->status.Outputs[0]);
	size = MAX_OUTPUTS / PRO_HALF_IO;
	//deb("PRO: ", EHouseProStatus.data, sizeof(eHouseProStatus.data));
	for (i = 0U; i < size; i++)
	{
		curr = (m_eHouseProStatus->status.Outputs[i / 8] >> (i % 8)) & 0x1;
		prev = (m_eHouseProStatusPrv->status.Outputs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
		{
			if (m_CHANGED_DEBUG)
				if (m_eHouseProStatusPrv->data[0])
					_log.Log(LOG_STATUS, "[PRO 192.168.%d.%d] OUT #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_PRO, 0x21, i, 100, curr, "", 100);
		}
	}
	//inputs
	//size= 128;
	size = MAX_INPUTS / PRO_HALF_IO;
	{
		for (i = 0; i < size; i++)
		{
			curr = (m_eHouseProStatus->status.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//        printf("[LAN 192.168.%d.%d] Input #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				UpdateSQLStatus(AddrH, AddrL, EH_PRO, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}

		if (m_eHouseProStatus->status.AdcProgramNr != m_eHouseProStatusPrv->status.AdcProgramNr)
		{
			curr = m_eHouseProStatus->status.AdcProgramNr;
			//printf("[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (m_eHouseProStatus->status.ProgramNr != m_eHouseProStatusPrv->status.ProgramNr)
		{
			curr = m_eHouseProStatus->status.ProgramNr;
			//printf("[LAN 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (m_eHouseProStatus->status.RollerProgram != m_eHouseProStatusPrv->status.RollerProgram)
		{
			curr = m_eHouseProStatus->status.RollerProgram;
			//printf("[LAN 192.168.%d.%d] Current Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (m_eHouseProStatus->status.SecuZone != m_eHouseProStatusPrv->status.SecuZone)
		{
			curr = m_eHouseProStatus->status.SecuZone;
			//printf("[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
	}


	{

		//alarms signals
		for (i = 0; i < size; i++)
		{
			curr = (m_eHouseProStatus->status.Alarm[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.Alarm[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Alarm #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int)i, (int) curr);
			}
		}
		//Warning signals
		for (i = 0; i < size; i++)
		{

			curr = (m_eHouseProStatus->status.Warning[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.Warning[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Warning #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Monitoring signals
		for (i = 0; i < size; i++)
		{
			curr = (m_eHouseProStatus->status.Monitoring[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.Monitoring[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Monitoring #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Silent signals
		for (i = 0; i < size; i++)
		{
			curr = (m_eHouseProStatus->status.Silent[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.Silent[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Silent #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Early Warning signals
		for (i = 0; i < size; i++)
		{
			curr = (m_eHouseProStatus->status.EarlyWarning[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHouseProStatusPrv->status.EarlyWarning[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO.168.%d.%d] Early Warning #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}


	}
	memcpy(m_eHouseProStatusPrv, m_eHouseProStatus, sizeof(union eHouseProStatusUT));

}
/////////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////////////
// Update eHouse WiFi controllers status to DB
void eHouseTCP::UpdateWiFiToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	unsigned char size = sizeof(m_eHWIFIn[index]->Outs) / sizeof(m_eHWIFIn[index]->Outs[0]); //for cm only
	size = 8;


	if (index > EHOUSE_WIFI_MAX - 1)
	{
		_log.Log(LOG_STATUS, " WIFI INDEX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;
	}
	//deb("WIFI: ", EHWIFIs[index].data, sizeof(eHWIFIs[index].data));
	for (i = 0; i < size; i++)
	{
		curr = (m_eHWIFIs[index]->eHWIFI.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (m_eHWIFIPrev[index]->eHWIFI.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (m_eHWIFIPrev[index]->data[0] == 0))
		{
			if (m_CHANGED_DEBUG) if (m_eHWIFIPrev[index]->data[0]) _log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_WIFI, 0x21, i + 1, 100, curr, "", 100);
		}
	}
	//inputs
	size = 8;
	{
		for (i = 0; i < size; i++)
		{
			curr = (m_eHWIFIs[index]->eHWIFI.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHWIFIPrev[index]->eHWIFI.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHWIFIPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_INPUT_IN, i + 1, 100, curr, "", 100);
			}
		}
		if (m_eHWIFIs[index]->eHWIFI.CURRENT_ADC_PROGRAM != m_eHWIFIPrev[index]->eHWIFI.CURRENT_ADC_PROGRAM)
		{
			curr = m_eHWIFIs[index]->eHWIFI.CURRENT_ADC_PROGRAM;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
		if (m_eHWIFIs[index]->eHWIFI.CURRENT_PROFILE != m_eHWIFIPrev[index]->eHWIFI.CURRENT_PROFILE)
		{
			curr = m_eHWIFIs[index]->eHWIFI.CURRENT_PROFILE;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
		if (m_eHWIFIs[index]->eHWIFI.CURRENT_PROGRAM != m_eHWIFIPrev[index]->eHWIFI.CURRENT_PROGRAM)
		{
			curr = m_eHWIFIs[index]->eHWIFI.CURRENT_PROGRAM;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current Proram #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		size = 4;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (m_eHWIFIs[index]->eHWIFI.Dimmers[i]);
			prev = (m_eHWIFIPrev[index]->eHWIFI.Dimmers[i]);
			if ((curr != prev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHWIFIPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_DIMMER_OUT, i + 1, 100, (curr), "", 100);
			}
		}
		size = 4;
		//ADCs Preset H
		//printf(" ADC:");
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHWIFIs[index]->eHWIFI.ADCH[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.ADCH[i]);
			if ((curr != prev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC H Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}
		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHWIFIs[index]->eHWIFI.ADCL[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.ADCL[i]);
			if ((acurr != aprev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC Low Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHWIFIs[index]->eHWIFI.ADC[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.ADC[i]);
			if ((acurr != aprev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_ADC_IN, i + 1, 100, acurr, sval, 100);
			}
		}

		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHWIFIs[index]->eHWIFI.TempL[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.TempL[i]);
			if ((acurr != aprev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] TEMP Low Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}

		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHWIFIs[index]->eHWIFI.TempH[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.TempH[i]);
			int midpoint = m_eHWIFIs[index]->eHWIFI.TempH[i] - m_eHWIFIs[index]->eHWIFI.TempL[i];
			midpoint /= 2;
			if ((acurr != aprev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Temp H Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
				sprintf(sval, "%.1f", ((float)(m_eHWIFIs[index]->eHWIFI.TempH[i] + m_eHWIFIs[index]->eHWIFI.TempL[i])) / 20);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_MCP9700_PRESET, i + 1, 100, midpoint, sval, 100);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (m_eHWIFIs[index]->eHWIFI.Temp[i]);
			aprev = (m_eHWIFIPrev[index]->eHWIFI.Temp[i]);
			if ((acurr != aprev) || (m_eHWIFIPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%.1f", ((float)acurr) / 10);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_MCP9700_IN, i + 1, 100, acurr, sval, 100);
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Temp #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}


	}
	memcpy(m_eHWIFIPrev[index], m_eHWIFIs[index], sizeof(union WIFIFullStatT));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Update eHouse 1 controller status (HeatManager, RoomManagers, ExternalManager) to DB
void eHouseTCP::UpdateRS485ToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	unsigned char size = sizeof(m_eHn[index]->Outs) / sizeof(m_eHn[index]->Outs[0]); //for cm only
	size = 32;
	if (AddrH == 1) size = 21;
	if (AddrH == 2) size = 32;
	if (index > EHOUSE1_RM_MAX - 1)
	{
		_log.Log(LOG_STATUS, "RS485 to much !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;

	}

	for (i = 0; i < size; i++)
	{
		curr = (m_eHRMs[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (m_eHRMPrev[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (m_eHRMPrev[index]->data[0] == 0))
		{
			if (m_CHANGED_DEBUG) if (m_eHRMPrev[index]->data[0]) printf("[RS485 (%d,%d)] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_RS485, 0x21, i + 1, 100, curr, "", 100);
		}
	}
	//inputs
	size = 16;
	if (AddrH == 1) size = 0;
	if (AddrH == 2) size = 16;

	{
		for (i = 0; i < size; i++)
		{
			curr = (m_eHRMs[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (m_eHRMPrev[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (m_eHRMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHRMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[RS485 (%d,%d)] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_INPUT_IN, i + 1, 100, curr, "", 100);
			}
		}
		if (m_eHRMs[index]->eHERM.CURRENT_PROFILE != m_eHRMPrev[index]->eHERM.CURRENT_PROFILE)
		{
			curr = m_eHRMs[index]->eHERM.CURRENT_PROFILE;
			if (m_CHANGED_DEBUG)
				if (m_eHRMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[RS485 (%d,%d)] Current Profile #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		if (m_eHRMs[index]->eHERM.CURRENT_PROGRAM != m_eHRMPrev[index]->eHERM.CURRENT_PROGRAM)
		{
			curr = m_eHRMs[index]->eHERM.CURRENT_PROGRAM;
			if (m_CHANGED_DEBUG) if (m_eHRMPrev[index]->data[0]) _log.Log(LOG_STATUS, "[RS485 (%d,%d) Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (m_eHRMs[index]->eHERM.Dimmers[i]);
			prev = (m_eHRMPrev[index]->eHERM.Dimmers[i]);
			if ((curr != prev) || (m_eHRMPrev[index]->data[0] == 0))
			{
				if (m_CHANGED_DEBUG)
					if (m_eHRMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[RS485 (%d,%d)] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_DIMMER_OUT, i + 1, 100, (curr * 100) / 255, "", 100);
			}
		}
		size = 16;
		//ADCs Preset H
		//ADCs L Preset
		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (m_eHRMs[index]->eHERM.ADC[i]);
			aprev = (m_eHRMPrev[index]->eHERM.ADC[i]);
			if ((acurr != aprev) || (m_eHRMPrev[index]->data[0] == 0))
			{
				//printf("[LAN 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_ADC_IN, i + 1, 100, acurr, sval, 100);
			}
		}

		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (m_eHRMs[index]->eHERM.Temp[i]);
			aprev = (m_eHRMPrev[index]->eHERM.Temp[i]);
			if ((acurr != aprev) || (m_eHRMPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%.1f", ((float)acurr) / 10);
				if (i > 0)
				{
					sprintf(sval, "%.1f", ((float)acurr) / 10);
					UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_LM335_IN, i + 1, 100, acurr, sval, 100);
				}
				else
				{
					sprintf(sval, "%.1f", ((float)acurr) / 10);
					UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_INVERTED_PERCENT_IN, i, 100, acurr, sval, 100);
				}
				//printf("[LAN 192.168.%d.%d] Temp #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}


	}


	//if (eHn[index].AddrH!=2)  //RM
	{
		memcpy(m_eHRMPrev[index], m_eHRMs[index], sizeof(union ERMFullStatT)); //eHRMs[index]
	}
	/*else
		{

		if (eHRMs[index].eHERM.CURRENT_ADC_PROGRAM!=   eHRMPrev[index].eHERM.CURRENT_ADC_PROGRAM)
			{
			curr = eHRMs[index].eHERM.CURRENT_ADC_PROGRAM;
			printf("[RS485 (%d,%d)] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		if (eHRMs[index].eHERM.CURRENT_PROFILE!=eHRMPrev[index].eHERM.CURRENT_PROFILE)
			{
			curr = eHRMs[index].eHERM.CURRENT_PROFILE;
			printf("[RS485 (%d,%d)] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		if (eHRMs[index].eHERM.CURRENT_PROGRAM!=eHRMPrev[index].eHERM.CURRENT_PROGRAM)
			{
			curr = eHRMs[index].eHERM.CURRENT_PROGRAM;
			printf("[RS485 (%d,%d)] Current Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		if (eHRMs[index].eHERM.CURRENT_ZONE!=eHRMPrev[index].eHERM.CURRENT_ZONE)
			{
			curr = eHRMs[index].eHERM.CURRENT_ZONE;
			printf("[RS485 (%d,%d)] Current ZONE #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		size= 16;


		}*/


		//memcpy(eHEn[index].eHERMPrev, EHEn[index].eHERMs, sizeof(eHEn[index].eHERMs));
	//// printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
}

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// Terminate UDP listener / TCP/IP Client for statuses (eHouse PRO/CM connection directly)
// Preconditions: none
//
////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::TerminateUDP()
{
	_log.Log(LOG_STATUS, "Terminate UDP");
	char opt = 0;

	//struct timeval timeout;
	m_UDP_terminate_listener = 1;
	RequestStop();
	if (m_ViaTCP)
	{
		unsigned long iMode = 1;
#ifdef WIN32
		int status = ioctlsocket(m_TCPSocket, FIONBIO, &iMode);
#else
		int status = ioctl(m_TCPSocket, FIONBIO, &iMode);
#endif
		if (status == SOCKET_ERROR)
		{
#ifdef WIN32
			_log.Log(LOG_STATUS, "ioctlsocket failed with error: %d", WSAGetLastError());
#else
			_log.Log(LOG_STATUS, "ioctlsocket failed with error");
#endif
			//closesocket(TCPSocket);
			//WSACleanup();
			//return -1;
		}
		/*timeout.tv_sec = 0;
		timeout.tv_usec = 100000ul;	//100ms for delay
		if (setsockopt(TCPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)   //Set socket Read operation Timeout
			{
			LOG(LOG_ERROR, "[TCP Client Status] Set Read Timeout failed");
			perror("[TCP Client Status] Set Read Timeout failed\n");
			}
		if (setsockopt(TCPSocket, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
			{
			LOG(LOG_ERROR, "[TCP Client Status] Set Write Timeout failed");
			perror("[TCP Client Status] Set Write Timeout failed\n");
			}
		opt = 1;
		//status = 1L;
		if (setsockopt(TCPSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)   //Set socket send data imediatelly
			{
			LOG(LOG_ERROR, "[TCP Cli Status] Set TCP No Delay failed");
			perror("[TCP Cli Status] Set TCP No Delay failed\n");
			}*/
	}
}


////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::IntToHex(unsigned char *buf, const unsigned char *inbuf, int received)
{

	const char *table = "0123456789ABCDEF";
	int i, index;
	index = 0;
	for (i = 0; i < received; i++)
	{
		buf[index] = table[inbuf[i] >> 4]; index++;
		buf[index] = table[inbuf[i] & 0xf]; index++;
	}
	buf[index] = 13; index++;
	//buf[index] = 10; index++;

}
////////////////////////////////////////////////////////////////////////////////////////
// Update eHE[] status matrix of structures for Ethernet eHouse controllers
// Update ECM status structure for CM and eHouse1 under CM supervision
// Update eH[] status matrix of structures for eHouse1 under CM supervision
// Preconditions: none
// Termination: run TerminateUDP() function
//
////////////////////////////////////////////////////////////////////////////////////////
/*
float getAdcVolt(int index)
{
int val = (eHE[index].CM.ADC[3].MSB&0x3)<<8;
val+=eHE[index].CM.ADC[3].LSB;
float vae= (float)val;
float temp;
VccRef= (float) ((2500.0*1023.0)/vae);
VccRef/= 10;
temp= (float) (3300.0/2500.0);
temp*=vae;
AdcRefMax= (int) temp;
CalcCalibration= (float) (1/((val*(3300.0/2500.0))/1024));
return VccRef;
}



*/
float eHouseTCP::getAdcVolt2(int index)
{
	int val;// = (eHERMs[index].eHERM.ADC[3] & 0x3ff);

	val = ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (3 << 1)])) & 0x3;		//VREFF
	val = val << 8;
	val += ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (3 << 1) + 1]));


	//val+=eHERMs[index].eHERM.ADC[3].LSB;
	float vae = (float)val;
	float temp;
	m_VccRef = (float)((2500.0 * 1023.0) / vae);
	m_VccRef /= 10;
	temp = (float)(3300.0 / 2500.0);
	temp *= vae;
	m_AdcRefMax = (int)temp;
	m_CalcCalibration = (float)(1 / ((val * (3300.0 / 2500.0)) / 1024));
	return m_VccRef;
}
///////////////////////////////////////////////////////////////////////////////
void eHouseTCP::CalculateAdc2(unsigned char index)
{
	double temp;
	int   temmp;
	double adcvalue;
	int ivalue;
	double Calibration, CalibrationV, Vcc, Offset, OffsetV;
	int field, adch, adcl;
	getAdcVolt2(index);
	for (field = 0; field < 16; field++)
	{
		ivalue = ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)])) & 0x3;
		ivalue = ivalue << 8;
		ivalue += ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1) + 1]));
		adcvalue = ivalue;
		Offset = 0;//eHEn[index].Offset[field];
		OffsetV = 0;//eHEn[index].OffsetV[field];
		Calibration = m_CalcCalibration;//eHEn[index].Calibration[field];
		CalibrationV = 1;//CalcCalibration;//eHEn[index].CalibrationV[field];
		Vcc = 3300;//eHEn[index].Vcc[field];

		adch = ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 6) & 0x3;
		adcl = ((m_eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 4) & 0x3;
		adch = adch << 8;
		adcl = adcl << 8;
		adcl |= m_eHEn[index]->BinaryStatus[STATUS_ADC_LEVELS + (field << 1)] & 0xff;
		adch |= m_eHEn[index]->BinaryStatus[STATUS_ADC_LEVELS + (field << 1) + 1] & 0xff;
		m_eHERMs[index]->eHERM.ADCH[field] = adch;
		m_eHERMs[index]->eHERM.ADCL[field] = adcl;
		m_eHERMs[index]->eHERM.ADC[field] = ivalue;

		//    Mod =eHEn[index].Mod[field];
			//temp=-273.16+(temp*5000)/(1023*10);
		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		m_eHEn[index]->AdcValue[field] = temmp;

		temp = -273.16 + Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHEn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHEn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHEn[index]->SensorTempsMCP9700[field] = temp / 10;
		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHEn[index]->SensorPercents[field] = temp / 10;
		m_eHEn[index]->SensorInvPercents[field] = 100 - (temp / 10);
		if (field != 9)
		{
			temp = -50.0 + Offset + ((adch * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHERMs[index]->eHERM.TempH[field] = (int)round(temp);

			temp = -50.0 + Offset + ((adcl*Vcc) / (10.0*1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHERMs[index]->eHERM.TempL[field] = (int)round(temp);
			m_eHERMs[index]->eHERM.Temp[field] = (int)round(m_eHEn[index]->SensorTempsMCP9700[field] * 10);
			m_eHERMs[index]->eHERM.Unit[field] = 'C';
		}
		else {
			temp = (adch * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHERMs[index]->eHERM.TempH[field] = (int)round((1000 - (temp)));

			temp = (adcl * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHERMs[index]->eHERM.TempL[field] = (int)round((1000 - (temp)));

			m_eHERMs[index]->eHERM.Temp[field] = (int)round(m_eHEn[index]->SensorInvPercents[field] * 10);
			m_eHERMs[index]->eHERM.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHEn[index]->SensorTempsMCP9701[field] = temp / 10;


		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		m_eHEn[index]->SensorVolts[field] = temp / 100;
	}
}
///////////////////////////////////////////////////////////////////////////////
void eHouseTCP::CalculateAdcWiFi(unsigned char index)
{
	double temp;
	int   temmp;
	double adcvalue;
	int ivalue;
	double Calibration, CalibrationV, Vcc, Offset, OffsetV;
	int field, adch, adcl;
	//getAdcVolt(index);
	for (field = 0; field < 4; field++)
	{
		m_eHWIFIs[index]->eHWIFI.ADC[field] = m_eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI] & 0x3;
		m_eHWIFIs[index]->eHWIFI.ADC[field] = m_eHWIFIs[index]->eHWIFI.ADC[field] << 8;
		m_eHWIFIs[index]->eHWIFI.ADC[field] += m_eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + 1];
		ivalue = m_eHWIFIs[index]->eHWIFI.ADC[field];
		/*eHWIFIs[index].eHWIFI.ADCL[0] = udp_status[ADC_OFFSET_WIFI+2];
		eHWIFIs[index].eHWIFI.ADCL[0] = eHWIFIs[index].eHWIFI.ADCL[1]<<8;
		eHWIFIs[index].eHWIFI.ADCL[0]+= udp_status[ADC_OFFSET_WIFI+3];
		eHWIFIs[index].eHWIFI.ADCH[0] = udp_status[ADC_OFFSET_WIFI+4];
		eHWIFIs[index].eHWIFI.ADCH[0] = eHWIFIs[index].eHWIFI.ADCH[0]<<8;
		eHWIFIs[index].eHWIFI.ADCH[0]+= udp_status[ADC_OFFSET_WIFI+5];
		  */
		adcvalue = ivalue;
		Offset = 0;//eHWIFIn[index].Offset[field];
		OffsetV = 0;//eHWIFIn[index].OffsetV[field];
		Calibration = m_CalcCalibration;//eHWIFIn[index].Calibration[field];
		CalibrationV = 1;//CalcCalibration;//eHWIFIn[index].CalibrationV[field];
		Vcc = 1000;//eHWIFIn[index].Vcc[field];

		adch = ((m_eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + (field << 1)]) >> 6) & 0x3;
		adcl = ((m_eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + (field << 1)]) >> 4) & 0x3;
		adch = adch << 8;
		adcl = adcl << 8;
		adcl |= m_eHWIFIn[index]->BinaryStatus[STATUS_ADC_LEVELS_WIFI + (field << 1)] & 0xff;
		adch |= m_eHWIFIn[index]->BinaryStatus[STATUS_ADC_LEVELS_WIFI + (field << 1) + 1] & 0xff;
		m_eHWIFIs[index]->eHWIFI.ADCH[field] = adch;
		m_eHWIFIs[index]->eHWIFI.ADCL[field] = adcl;
		m_eHWIFIs[index]->eHWIFI.ADC[field] = ivalue;

		//    Mod =eHWIFIn[index].Mod[field];
			//temp=-273.16+(temp*5000)/(1023*10);
		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		m_eHWIFIn[index]->AdcValue[field] = temmp;

		temp = -273.16 + Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorTempsMCP9700[field] = temp / 10;
		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorPercents[field] = temp / 10;
		m_eHWIFIn[index]->SensorInvPercents[field] = 100 - (temp / 10);
		if (field == 0)
		{
			temp = -50.0 + Offset + ((adch * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHWIFIs[index]->eHWIFI.TempH[field] = (int)round(temp);

			temp = -50.0 + Offset + ((adcl * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHWIFIs[index]->eHWIFI.TempL[field] = (int)round(temp);
			m_eHWIFIs[index]->eHWIFI.Temp[field] = (int)round(m_eHWIFIn[index]->SensorTempsMCP9700[field] * 10);
			m_eHWIFIs[index]->eHWIFI.Unit[field] = 'C';
		}
		else {
			temp = (adch * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHWIFIs[index]->eHWIFI.TempH[field] = (int)round((1000 - (temp)));

			temp = (adcl * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			m_eHWIFIs[index]->eHWIFI.TempL[field] = (int)round((1000 - (temp)));

			m_eHWIFIs[index]->eHWIFI.Temp[field] = (int)round(m_eHWIFIn[index]->SensorInvPercents[field] * 10);
			m_eHWIFIs[index]->eHWIFI.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorTempsMCP9701[field] = temp / 10;
		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		m_eHWIFIn[index]->SensorVolts[field] = temp / 100;
	}
}
///////////////////////////////////////////////////////////////////////////////

void eHouseTCP::CalculateAdcEH1(unsigned char index)
{
	double temp;
	int   temmp;
	double adcvalue;
	int ivalue;
	double Calibration, CalibrationV, Vcc, Offset, OffsetV;
	int field;// , adch, adcl;
	//Vcc=
			//getAdcVolt(index);
 //   printf("//////////////////////////////////////////\r\n");
	for (field = 0; field < 16; field++)
	{
		ivalue = ((m_eHn[index]->BinaryStatus[RM_STATUS_ADC + (field << 1)])) & 0x3;
		ivalue = ivalue << 8;
		ivalue += ((m_eHn[index]->BinaryStatus[RM_STATUS_ADC + (field << 1) + 1]));
		adcvalue = ivalue;
		Offset = 0;//eHn[index].Offset[field];
		OffsetV = 0;//eHn[index].OffsetV[field];
		Calibration = 1;//CalcCalibration;//eHn[index].Calibration[field];
		CalibrationV = 1;//CalcCalibration;//eHn[index].CalibrationV[field];
		Vcc = 5000;//eHn[index].Vcc[field];

		/*adch = ((eHn[index].BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 6) &0x3;
		adcl = ((eHn[index].BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 4) &0x3;
		adch = adch << 8;
		adcl = adcl << 8;
		adcl|=eHn[index].BinaryStatus[STATUS_ADC_LEVELS+(field << 1)] & 0xff;
		adch|=eHn[index].BinaryStatus[STATUS_ADC_LEVELS+(field << 1) + 1] & 0xff;*/
		m_eHRMs[index]->eHERM.ADCH[field] = 1023;
		m_eHRMs[index]->eHERM.ADCL[field] = 0;
		m_eHRMs[index]->eHERM.ADC[field] = ivalue;

		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		m_eHn[index]->AdcValue[field] = temmp;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) - 273.16;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHn[index]->SensorTempsMCP9700[field] = temp / 10;

		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHn[index]->SensorPercents[field] = temp / 10;
		m_eHn[index]->SensorInvPercents[field] = 100 - (temp / 10);

		if (field != 0)
		{
			m_eHRMs[index]->eHERM.TempH[field] = 1500;
			m_eHRMs[index]->eHERM.TempL[field] = 0;
			m_eHRMs[index]->eHERM.Temp[field] = (int)round(m_eHn[index]->SensorTempsLM335[field] * 10);
			m_eHRMs[index]->eHERM.Unit[field] = 'C';
		}
		else
		{
			m_eHRMs[index]->eHERM.TempH[field] = 1000;//(1000 - (temp));
			m_eHRMs[index]->eHERM.TempL[field] = 0;//(1000 - (temp));

			m_eHRMs[index]->eHERM.Temp[field] = (int)round(m_eHn[index]->SensorInvPercents[field] * 10);
			m_eHRMs[index]->eHERM.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		m_eHn[index]->SensorTempsMCP9701[field] = temp / 10;
		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		m_eHn[index]->SensorVolts[field] = temp / 100;
	}
}
/////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::deb(char *prefix, unsigned char *dta, int size)
{
	int i;
	printf("%s", prefix);
	for (i = 0; i < size; i++) printf("%02x,", dta[i]);
	printf("\r\n");
}
/////////////////////////////////////////////////////////////////////////////////
// WE Do not buffer data - work on original buffer byte-by-byte
// could be Slow but we do not allocate additional memory (about 100kB) for device discovery
////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::GetStr(unsigned char *GetNamesDta)
{
	memset(m_GetLine, 0, sizeof(m_GetLine));
	while ((GetNamesDta[m_GetIndex] != 13) && (m_GetIndex < m_GetSize))
	{
		if (strlen(m_GetLine) < sizeof(m_GetLine))
			strncat(m_GetLine, (char *)&GetNamesDta[m_GetIndex], 1);
		m_GetIndex++;

	}
	if (m_GetIndex < m_GetSize) m_GetIndex++; //"\r"
	if (m_GetIndex < m_GetSize) m_GetIndex++; //"\n"
}
//////////////////////////////////////////////////////////////////////////////////
// Get udp auto name change and discovery
//////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::GetUDPNamesRS485(unsigned char *data, int nbytes)

{
	//addrh =data[1];
	//addrl =data[2];
	//char GetLine[255];
	char PGMs[500];
	char Name[80];
	char tmp[96];
	int i, m_PlanID;
	if (data[3] != 'n') return;
	if (data[4] != '0') return;
	unsigned char nr;
	if (data[1] == 1) nr = 0;						//for HM
	else if (data[1] == 2) nr = STATUS_ARRAYS_SIZE; //for EM
	else nr = data[2] % STATUS_ARRAYS_SIZE;			//addrl for RM

	m_GetIndex = 7;             //Ignore binary control fields
	m_GetSize = nbytes;         //size of whole packet
	GetStr(data);			//addr combined
	GetStr(data);			//addr h
	GetStr(data);			//addr l
	GetStr(data);			//name
	i = 1;
	eHaloc(nr, data[1], data[2]);
	m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_RS485, (char *)&m_GetLine);//for Automatic RoomPlan generation for RoomManager
	if (m_PlanID < 0) m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_RS485, (char *)&m_GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&m_eHn[nr]->Name, (char *)&m_GetLine, sizeof(m_eHn[nr]->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&m_GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(m_eHn[nr]->ADCs) / sizeof(m_eHn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&m_eHn[nr]->ADCs[i], (char *)&m_GetLine, sizeof(m_eHn[nr]->ADCs[i]));
		if (nr > 0)
		{
			if (i > 0) UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_LM335_IN, i + 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
			else UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_INVERTED_PERCENT_IN, i + 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
			//        UpdateSQLState(data[1], data[2], EH_RS485, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_LM335_PRESET, i, 1, 0, "20.5", Name, (char *) &GetLine, true, 100);
		}
		else UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_LM335_IN, i + 1, 0, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	/*    GetStr(data);// #ADC CFG; Not available for eHouse 1
		for (i = 0; i < sizeof(eHn[nr].ADCs) / sizeof(eHn[nr].ADCs[0]); i++)          //ADC Config (sensor type) not used currently
			{
			GetStr(data);
			eHn[nr].ADCConfig[i] =GetLine[0]-'0';
			}
	  */
	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(m_eHn[nr]->Outs) / sizeof(m_eHn[nr]->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHn[nr]->Outs[i], (char *)&m_GetLine, sizeof(m_eHn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i + 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);
	for (i = 0; i < sizeof(m_eHn[nr]->Inputs) / sizeof(m_eHn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHn[nr]->Inputs[i], (char *)&m_GetLine, sizeof(m_eHn[nr]->Inputs[i]));
		if (nr != 0) UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchGeneralSwitch,    //not HM
			STYPE_Contact, VISUAL_INPUT_IN, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(m_eHn[nr]->Dimmers) / sizeof(m_eHn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&m_eHn[nr]->Dimmers[i], (char *)&m_GetLine, sizeof(m_eHn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_RS485, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}
	UpdateSQLState(data[1], data[2], EH_RS485, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 1, 1, 0, "", Name, "RGB", true, 100, m_PlanID);  //RGB dimmer
	GetStr(data);	//rollers names

	for (i = 0; i < sizeof(m_eHn[nr]->Outs) / sizeof(m_eHn[nr]->Outs[0]); i += 2)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		if (nr == STATUS_ARRAYS_SIZE) UpdateSQLState(data[1], data[2], EH_RS485, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(m_eHn[nr]->Programs) / sizeof(m_eHn[nr]->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_eHn[nr]->Programs[i], (char *)&m_GetLine, sizeof(m_eHn[nr]->Programs[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
#ifdef UNLIMITED_PGM
			if (k <= 10) 
#endif
				strcat(PGMs, tmp);
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	k = UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	/*    strcpy(PGMs,"SelectorStyle:1;LevelNames:"); //Add Regulation Program Selector
		GetStr(data);// "#ADC Programs Names
		for (i = 0; i < sizeof(eHn[nr].ADCPrograms) / sizeof(eHn[nr].ADCPrograms[0]); i++)
			{
			GetStr(data);
			printf("%s\r\n", (char *) &GetLine);
			strncpy((char *) &eHn[nr].ADCPrograms[i], (char *) &GetLine, sizeof(eHn[nr].ADCPrograms[i]));
			if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == nullptr))
					{
					k++;
					sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
					if (k<= 10) strncat(PGMs, tmp, strlen(tmp));
					}
			}
		PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
		k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0,
	   "0", Name, "Reg. Scene", true, 100); k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector,
	   STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100); UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs,k);
	*/
	/*strcat(Names,"#Secu Programs Names\r\n"); //CM only
	for (i = 0; i < sizeof((void *) &eHn[nr]) / sizeof(&eHn[nr].SecuPrograms[0]); i++)
		{
		strcat(Names, EHn[nr].SecuPrograms[i]);
		strcat(Names,"\r\n");
		}

	strcat(Names,"#Zones Programs Names\r\n");  //CM only
	for (i = 0; i < sizeof(eHn[nr].) / sizeof(eHn[nr].Zones[0]); i++)
		{
		strcat(Names, EHn[nr].Zones[i]);
		strcat(Names,"\r\n");
		}*/
	m_eHRMPrev[nr]->data[0] = 0;
	m_eHn[nr]->BinaryStatus[0] = 0;
	//memset((void *) &eHRMPrev[nr], 0, sizeof(union ERMFullStatT));//eHRMPrev[nr]));
	//memset(eHn[nr]->BinaryStatus, 0, sizeof(eHn[nr]->BinaryStatus));
}
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::GetUDPNamesLAN(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i,m_PlanID;
	char Name[80];
	char tmp[96];
	char PGMs[500U];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '1') return;   //other controller type than ERM
	unsigned char nr = (data[2] - m_INITIAL_ADDRESS_LAN) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	m_GetIndex = 7;             //Ignore binary control fields
	m_GetSize = nbytes;         //size of whole packet
	GetStr(data);           //addr combined
	GetStr(data);			//addr h
	GetStr(data);			//addr l
	GetStr(data);			//name
	i = 1;
	eHEaloc(nr, data[1], data[2]);
	m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&m_GetLine);//for Automatic RoomPlan generation for RoomManager
	if (m_PlanID < 0) m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&m_GetLine); //Add RoomPlan for RoomManager (RM)


	strncpy((char *)&m_eHEn[nr]->Name, (char *)&m_GetLine, sizeof(m_eHEn[nr]->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&m_GetLine, sizeof(Name));
	GetStr(data);			//comment

	for (i = 0; i < sizeof(m_eHEn[nr]->ADCs) / sizeof(m_eHEn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&m_eHEn[nr]->ADCs[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i, 1, 0, "20.5", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);// #ADC CFG;
	for (i = 0; i < sizeof(m_eHEn[nr]->ADCs) / sizeof(m_eHEn[nr]->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		m_eHEn[nr]->ADCConfig[i] = m_GetLine[0] - '0';
	}

	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(m_eHEn[nr]->Outs) / sizeof(m_eHEn[nr]->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHEn[nr]->Outs[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);
	for (i = 0; i < sizeof(m_eHEn[nr]->Inputs) / sizeof(m_eHEn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHEn[nr]->Inputs[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}


	GetStr(data);		//Description
	for (i = 0; i < sizeof(m_eHEn[nr]->Dimmers) / sizeof(m_eHEn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&m_eHEn[nr]->Dimmers[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}
	UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100, m_PlanID);  //RGB dimmer
	GetStr(data);		//rollers names

	for (i = 0; i < sizeof(m_eHEn[nr]->Rollers) / sizeof(m_eHEn[nr]->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	
	}

	int k = 0;
	GetStr(data);		// #Programs Names
	strcpy(PGMs, (char *)&"SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(m_eHEn[nr]->Programs) / sizeof(m_eHEn[nr]->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_eHEn[nr]->Programs[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->Programs[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
//			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);
		}
	}
	
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	//_log.Log(LOG_ERROR, "[PRG %d] %s", strlen(PGMs), PGMs);

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	strcpy(PGMs, (char *)&"SelectorStyle:1;LevelNames:"); //Add Regulation Program Selector
	GetStr(data);// "#ADC Programs Names
	for (i = 0; i < sizeof(m_eHEn[nr]->ADCPrograms) / sizeof(m_eHEn[nr]->ADCPrograms[0]); i++)
	{
		GetStr(data);
		//printf("%s\r\n", (char *) &GetLine);
		strncpy((char *)&m_eHEn[nr]->ADCPrograms[i], (char *)&m_GetLine, sizeof(m_eHEn[nr]->ADCPrograms[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	//_log.Log(LOG_ERROR, "[PRG %d] %s", strlen(PGMs), PGMs);


	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs, k);

	/*strcat(Names,"#Secu Programs Names\r\n"); //CM only
	for (i = 0; i < sizeof((void *) &eHEn[nr]) / sizeof(&eHEn[nr].SecuPrograms[0]); i++)
		{
		strcat(Names, EHEn[nr].SecuPrograms[i]);
		strcat(Names,"\r\n");
		}

	strcat(Names,"#Zones Programs Names\r\n");  //CM only
	for (i = 0; i < sizeof(eHEn[nr].) / sizeof(eHEn[nr].Zones[0]); i++)
		{
		strcat(Names, EHEn[nr].Zones[i]);
		strcat(Names,"\r\n");
		}*/
	m_eHERMPrev[nr]->data[0] = 0;
	m_eHEn[nr]->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::GetUDPNamesCM(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i, m_PlanID;
	char Name[80];
	char tmp[96];
	char PGMs[500U];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '2') return;   //other controller type than ERM
	unsigned char nr = (data[2] - m_INITIAL_ADDRESS_LAN) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	m_GetIndex = 7;					//Ignore binary control fields
	m_GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	eCMaloc(0, data[1], data[2]);
	m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&m_GetLine);		//for Automatic RoomPlan generation for RoomManager
	if (m_PlanID < 0) m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&m_GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&m_ECMn->Name, (char *)&m_GetLine, sizeof(m_ECMn->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&m_GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(m_ECMn->ADCs) / sizeof(m_ECMn->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->ADCs[i], (char *)&m_GetLine, sizeof(m_ECMn->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i, 1, 0, "20.5", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);		// #ADC CFG;
	for (i = 0; i < sizeof(m_ECMn->ADCs) / sizeof(m_ECMn->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		m_ECMn->ADCConfig[i] = m_GetLine[0] - '0';
	}

	GetStr(data);		// #Outs;
	for (i = 0; i < sizeof(m_ECMn->Outs) / sizeof(m_ECMn->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->Outs[i], (char *)&m_GetLine, sizeof(m_ECMn->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);
	for (i = 0; i < sizeof(m_ECMn->Inputs) / sizeof(m_ECMn->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->Inputs[i], (char *)&m_GetLine, sizeof(m_ECMn->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(m_ECMn->Dimmers) / sizeof(m_ECMn->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->Dimmers[i], (char *)&m_GetLine, sizeof(m_ECMn->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}
	UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100, m_PlanID);  //RGB dimmer
	GetStr(data);//rolers names

	for (i = 0; i < sizeof(m_ECMn->Rollers) / sizeof(m_ECMn->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(m_ECMn->Programs) / sizeof(m_ECMn->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->Programs[i], (char *)&m_GetLine, sizeof(m_ECMn->Programs[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	strcpy(PGMs, "SelectorStyle:1;LevelNames:"); //Add Regulation Program Selector
	GetStr(data);// "#ADC Programs Names
	for (i = 0; i < sizeof(m_ECMn->ADCPrograms) / sizeof(m_ECMn->ADCPrograms[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_ECMn->ADCPrograms[i], (char *)&m_GetLine, sizeof(m_ECMn->ADCPrograms[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs, k);

	/*strcat(Names,"#Secu Programs Names\r\n"); //CM only
	for (i = 0; i < sizeof((void *) &eHEn[nr]) / sizeof(&eHEn[nr].SecuPrograms[0]); i++)
		{
		strcat(Names, EHEn[nr].SecuPrograms[i]);
		strcat(Names,"\r\n");
		}

	strcat(Names,"#Zones Programs Names\r\n");  //CM only
	for (i = 0; i < sizeof(eHEn[nr].) / sizeof(eHEn[nr].Zones[0]); i++)
		{
		strcat(Names, EHEn[nr].Zones[i]);
		strcat(Names,"\r\n");
		}*/
	m_ECMPrv->data[0] = 0;
	m_ECMn->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////

void eHouseTCP::GetUDPNamesPRO(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i, m_PlanID;
	char Name[80];
	char tmp[96];
	char PGMs[500U];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '3') return;   //other controller type than ERM
	unsigned char nr = 0;			//(data[2]-INITIAL_ADDRESS_LAN)%STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	m_GetIndex = 7;					//Ignore binary control fields
	m_GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	//eHPROaloc(0, data[1], data[2]);
	m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_PRO, (char *)&m_GetLine);//for Automatic RoomPlan generation for RoomManager
	if (m_PlanID < 0) m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_PRO, (char *)&m_GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&m_eHouseProN->Name, (char *)&m_GetLine, sizeof(m_eHouseProN->Name));	//RM Controller Name
	strncpy((char *)&Name, (char *)&m_GetLine, sizeof(Name));
	GetStr(data);		//comment

	for (i = 0; i < sizeof(m_eHouseProN->ADCs) / sizeof(m_eHouseProN->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->ADCs[i], (char *)&m_GetLine, sizeof(m_eHouseProN->ADCs[i]));
		if (i < MAX_AURA_DEVS)
		{
			//eAURAaloc(i, 0x81, i + 1);
			UpdateSQLState(0x81, i + 1, EH_AURA, pTypeTEMP, sTypeTEMP5, 0, VISUAL_AURA_IN, 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
			UpdateSQLState(0x81, i + 1, EH_AURA, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_AURA_PRESET, 1, 1, 0, "20.5", Name, (char *)&m_GetLine, true, 100, m_PlanID);
			if (strlen((char *)&m_AuraN[i]) > 0)
			{
				m_AuraDevPrv[i]->Addr = 0;
				m_AuraN[i]->BinaryStatus[0] = 0;
			}
		}
	}

	GetStr(data);// #ADC CFG;
	for (i = 0; i < sizeof(m_eHouseProN->ADCs) / sizeof(m_eHouseProN->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		//eHouseProN.ADCType[i] =GetLine[0]-'0';
		if (i < MAX_AURA_DEVS)
			strncpy((char *)&m_eHouseProN->ADCType[i], (char *)&m_GetLine, sizeof(m_eHouseProN->ADCType[i]));//[0]-'0';
	}

	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(m_eHouseProN->Outs) / sizeof(m_eHouseProN->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->Outs[i], (char *)&m_GetLine, sizeof(m_eHouseProN->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);
	for (i = 0; i < sizeof(m_eHouseProN->Inputs) / sizeof(m_eHouseProN->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->Inputs[i], (char *)&m_GetLine, sizeof(m_eHouseProN->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(m_eHouseProN->Dimmers) / sizeof(m_eHouseProN->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->Dimmers[i], (char *)&m_GetLine, sizeof(m_eHouseProN->Dimmers[i]));
		//    UpdateSQLState(data[1], data[2], EH_PRO, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *) &GetLine, true, 100);
	}
	//UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);//rollers names

	for (i = 0; i < sizeof(m_eHouseProN->Rollers) / sizeof(m_eHouseProN->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(m_eHouseProN->Programs) / sizeof(m_eHouseProN->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->Programs[i], (char *)&m_GetLine, sizeof(m_eHouseProN->Programs[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);

			if (strlen(PGMs) > 400) break;
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100, m_PlanID);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	}
	k = 0;

	strcpy(PGMs, "SelectorStyle:1;LevelNames:");		//Add Regulation Program Selector
	GetStr(data);									// "#ADC Programs Names
	for (i = 0; i < sizeof(m_eHouseProN->ADCPrograms) / sizeof(m_eHouseProN->ADCPrograms[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&m_eHouseProN->ADCPrograms[i], (char *)&m_GetLine, sizeof(m_eHouseProN->ADCPrograms[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);

			if (strlen(PGMs) > 400) break;
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100, m_PlanID);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs, k);
	}
	GetStr(data);
	strcat(Name, "#Secu Programs Names\r\n");		//CM only
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");	//Add Regulation Program Selector
	k = 0;
	for (i = 0; i < sizeof((void *)&m_eHouseProN->SecuPrograms) / sizeof(&m_eHouseProN->SecuPrograms[0]); i++)
	{
		//strcat(Name, EHouseProN.SecuPrograms[i]);
		//strcat(Name,"\r\n");
		//if (i > 9) break;
		GetStr(data);
		strncpy((char *)&m_eHouseProN->SecuPrograms[i], (char *)&m_GetLine, sizeof(m_eHouseProN->SecuPrograms[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);

			if (strlen(PGMs) > 400) break;
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_SECUPGM, 1, 1, 0, "0", Name, "Pgm. Secu", true, 100, m_PlanID);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_SECUPGM, 1, 1, 0, "0", Name, "Pgm. Secu", true, 100, m_PlanID);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_SECUPGM, PGMs, k);
	}
	GetStr(data);
	k = 0;
	strcat(Name, "#Zones Programs Names\r\n");		//CM only
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");	//Add Regulation Program Selector
	for (i = 0; i < sizeof((void *)&m_eHouseProN->Zones) / sizeof(&m_eHouseProN->Zones[0]); i++)
	{
		//strcat(Name, EHouseProN.SecuPrograms[i]);
		//strcat(Name,"\r\n");
		//if (i > 9) break;
		GetStr(data);
		strncpy((char *)&m_eHouseProN->Zones[i], (char *)&m_GetLine, sizeof(m_eHouseProN->Zones[i]));
		if ((strlen((char *)&m_GetLine) > 1) && (strstr((char *)&m_GetLine, "@") == nullptr))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&m_GetLine, i + 1);
			//if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
#ifdef UNLIMITED_PGM
			if (k <= 10)
#endif
				strcat(PGMs, tmp);

			if (strlen(PGMs) > 400) break;
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_ZONEPGM, 1, 1, 0, "0", Name, "Zones", true, 100, m_PlanID);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_ZONEPGM, 1, 1, 0, "0", Name, "Zones", true, 100, m_PlanID);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_ZONEPGM, PGMs, k);
	}
	m_eHouseProStatusPrv->data[0] = 0;
	m_eHouseProN->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////




void eHouseTCP::GetUDPNamesWiFi(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i, m_PlanID;
	char Name[80];
	//char tmp[96];
	//char PGMs[500u];
	//memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '4') return;   //other controller type than ERM
	unsigned char nr = (data[2] - m_INITIAL_ADDRESS_WIFI) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	m_GetIndex = 7;					//Ignore binary control fields
	m_GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	eHWIFIaloc(nr, data[1], data[2]);
	m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_WIFI, (char *)&m_GetLine);					//for Automatic RoomPlan generation for RoomManager
	if (m_PlanID < 0) m_PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_WIFI, (char *)&m_GetLine);	//Add RoomPlan for RoomManager (RM)

	strncpy((char *)&m_eHWIFIn[nr]->Name, (char *)&m_GetLine, sizeof(m_eHWIFIn[nr]->Name));				//RM Controller Name
	strncpy((char *)&Name, (char *)&m_GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(m_eHWIFIn[nr]->ADCs) / sizeof(m_eHWIFIn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&m_eHWIFIn[nr]->ADCs[i], (char *)&m_GetLine, sizeof(m_eHWIFIn[nr]->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i + 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i + 1, 1, 0, "20.5", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);		// #ADC CFG;
	for (i = 0; i < sizeof(m_eHWIFIn[nr]->ADCs) / sizeof(m_eHWIFIn[nr]->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		m_eHWIFIn[nr]->ADCConfig[i] = m_GetLine[0] - '0';
	}

	GetStr(data);		// #Outs;
	for (i = 0; i < sizeof(m_eHWIFIn[nr]->Outs) / sizeof(m_eHWIFIn[nr]->Outs[0]); i++)			//binary outputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHWIFIn[nr]->Outs[i], (char *)&m_GetLine, sizeof(m_eHWIFIn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i + 1, 1, 0, "0", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	GetStr(data);
	for (i = 0; i < sizeof(m_eHWIFIn[nr]->Inputs) / sizeof(m_eHWIFIn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&m_eHWIFIn[nr]->Inputs[i], (char *)&m_GetLine, sizeof(m_eHWIFIn[nr]->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(m_eHWIFIn[nr]->Dimmers) / sizeof(m_eHWIFIn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&m_eHWIFIn[nr]->Dimmers[i], (char *)&m_GetLine, sizeof(m_eHWIFIn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}
	UpdateSQLState(data[1], data[2], EH_WIFI, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 1, 1, 0, "", Name, "RGB", true, 100, m_PlanID);  //RGB dimmer
	GetStr(data);//rollers names

	for (i = 0; i < sizeof(m_eHWIFIn[nr]->Rollers) / sizeof(m_eHWIFIn[nr]->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i + 1, 1, 0, "", Name, (char *)&m_GetLine, true, 100, m_PlanID);
	}

	int k = 0;
	/*    GetStr(data);    // #Programs Names
		strcpy(PGMs,"SelectorStyle:1;LevelNames:");     //Program/scene selector
		for (i = 0; i < sizeof(eHWIFIn[nr].Programs) / sizeof(eHWIFIn[nr].Programs[0]); i++)
			{
			GetStr(data);
			printf("%s\r\n", (char *) &GetLine);
			strncpy((char *) &eHWIFIn[nr].Programs[i], (char *) &GetLine, sizeof(eHWIFIn[nr].Programs[i]));
			if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == nullptr))
				{
				k++;
				sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
				if (k<= 10) strncat(PGMs, tmp, strlen(tmp));


				}
			}
	*/
	/*PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name,
	"Scene", true, 100); k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM,
	1, 1, 0, "0", Name, "Scene", true, 100);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs,k);
	k= 0;
	strcpy(PGMs,"SelectorStyle:1;LevelNames:"); //Add Requlation Program Selector
	GetStr(data);// "#ADC Programs Names
	for (i = 0; i < sizeof(eHWIFIn[nr].ADCPrograms) / sizeof(eHWIFIn[nr].ADCPrograms[0]); i++)
		{
		GetStr(data);
		//printf("%s\r\n", (char *) &GetLine);
		strncpy((char *) &eHWIFIn[nr].ADCPrograms[i], (char *) &GetLine, sizeof(eHWIFIn[nr].ADCPrograms[i]));
		if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == nullptr))
				{
				k++;
				sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
				if (k<= 10) strncat(PGMs, tmp, strlen(tmp));
				}
		}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name,
	"Reg. Scene", true, 100); k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector,
	VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs,k);
*/
	/*strcat(Names,"#Secu Programs Names\r\n"); //CM only
	for (i = 0; i < sizeof((void *) &eHEn[nr]) / sizeof(&eHEn[nr].SecuPrograms[0]); i++)
		{
		strcat(Names, EHEn[nr].SecuPrograms[i]);
		strcat(Names,"\r\n");
		}

	strcat(Names,"#Zones Programs Names\r\n");  //CM only
	for (i = 0; i < sizeof(eHEn[nr].) / sizeof(eHEn[nr].Zones[0]); i++)
		{
		strcat(Names, EHEn[nr].Zones[i]);
		strcat(Names,"\r\n");
		}*/
	m_eHWIFIPrev[nr]->data[0] = 0;
	m_eHWIFIn[nr]->BinaryStatus[0] = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// UDP listener service - listen up broadcast from Ethernet, WiFi eHouse controllers and
// eHouse1 via CommManager (under CM supervision)
// Infinite loop - run in separate Thread
// Update eHE[] status matrix of structures for Ethernet eHouse controllers
// Update ECM status structure for CM and eHouse1 under CM supervision
// Update eH[] status matrix of structures for eHouse1 under CM supervision
// Preconditions: none
// Termination: run TerminateUDP() function
//
char eHouseTCP::eH1(unsigned char addrh, unsigned char addrl)
{
	char eh1 = 0;
	if (addrh == 55) eh1 = 1;
	if (addrh == 2) eh1 = 1;
	if (addrh == 1) eh1 = 1;
	if (addrh == 129) eh1 = 1;
	if ((eh1) || ((addrh == 0x55) && (addrl == 0xff)))	// via pro or pro
	{
		m_ipaddrh = m_SrvAddrH;
		m_ipaddrl = m_SrvAddrL;
	}
	if (eh1)
	{
		eh1++;
	}
	return eh1;
}

/* //Debug communication  errors to file
char FIRST_TIME = 1;
void debu(unsigned char adrh, unsigned char adrl, unsigned char err, int size, unsigned int sum, unsigned int sum2, unsigned int siz2)
{
	FILE *tf;
	if (FIRST_TIME) tf = fopen("c:\\temp\\log.log", "w+");
	else tf = fopen("c:\\temp\\log.log", "a+");
	FIRST_TIME = 0;
	if (tf == nullptr) return;
	if (sum2 == sum)
		fprintf(tf, "(%d, %d) #%d, %d [B] OK\n", adrh, adrl, size, siz2);
	else
		fprintf(tf, "(%d, %d) #%d, %d [B] Invalid checksum: %d <> %d\n", adrh, adrl, siz2, size, sum , sum2);

	//if (err == 1) fprintf(tf, "ERR Checksum (%d, %d) #%d [B] OK\r\n", adrh, adrl, size);
	fflush(tf);
	fclose(tf);
}*/
void eHouseTCP::Do_Work()
//void UDPListener(void)  //for eHouse4Ethernet devices and eHouse1 via CommManager

{
	char LogPrefix[20];
	int heartbeatInterval = 0;
	unsigned char localstatus = 0;
	//#ifndef WIN32
	unsigned long opt = 0;
	//#else
	//	char opt = 0;
	//	status
	//#endif
	int comm = 0;
	int ressock = 0;
	m_UDP_terminate_listener = 0;
#ifndef WIN32
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

#endif

	if (m_ViaTCP)
	{
		sprintf(LogPrefix, "eHouse TCP");
		LOG(LOG_STATUS, "[eHouse] TCP");
	}
	else
	{
		sprintf(LogPrefix, "eHouse UDP");
		LOG(LOG_STATUS, "[eHouse] UDP");
	}

	if (access("/usr/local/ehouse/disable_udp_rs485.cfg", F_OK) != -1)        //file exists read cfg
	{
		m_disablers485 = 1;
	}
	unsigned int sum, sum2;
	unsigned char udp_status[MAXMSG], devaddrh, devaddrl, log[300U], dir[10]; // , code[20];
	int i;
	char GetLine[255];
	struct sockaddr_in saddr, caddr;
	socklen_t size;
	int nbytes;
	if (m_UDP_terminate_listener)
	{
		_log.Log(LOG_STATUS, "[%s] Disabled", LogPrefix);
		return;
	}
	_log.Log(LOG_STATUS, "!!!!! eHouseUDP: Do Work Starting");
	memset(GetLine, 0, sizeof(GetLine));
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	if (m_ViaTCP == 0)
	{
		saddr.sin_family = AF_INET;							//initialization of protocol & socket
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		saddr.sin_port = htons(m_UDP_PORT);
		m_eHouseUDPSocket = socket(AF_INET, SOCK_DGRAM, 0);	//socket creation

		opt = 1; setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
		opt = 1; setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));

		//opt = 0;	setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_LINGER, &opt, sizeof(int));
#ifndef WIN32
		setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
		setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
#else
		opt = 100;	setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&opt, sizeof(opt));
		opt = 100;	setsockopt(m_eHouseUDPSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&opt, sizeof(opt));
		opt = 1;	setsockopt(m_eHouseUDPSocket, IPPROTO_UDP, TCP_NODELAY, (char *)&opt, sizeof(opt));// < 0)
		/*{
		LOG(LOG_ERROR, "[TCP Cli Status] Set TCP No Delay failed");
		perror("[TCP Cli Status] Set TCP No Delay failed\n");
		}

		*/

#endif

		ressock = bind(m_eHouseUDPSocket, (struct sockaddr *) &saddr, sizeof(saddr));  //bind socket
	}
	int SecIter = 0;
	unsigned char ou = 0;
	//	LOG(LOG_STATUS, "TIM: %d", mytime(nullptr) - m_LastHeartbeat);
	m_LastHeartbeat = mytime(nullptr);
	int prevtim, tim = clock();
	prevtim = tim;
	time_t tt = time(nullptr);
	while (!IsStopRequested(0))				//main loop
	{
		tim = clock();
		//if (tim-prevtim>= 100)
							//    0.1-0.3 sec - based on TCP Socket Timeout depending on Status counts
							//   similar for UDP listener
							//   non critical value (used for retransmission events)
		{
			//LOG(LOG_STATUS, "TIM: %d", tim-prevtim);
			//prevtim = tim;
			SecIter++;
			//ExecQueuedEvents();				//perform Event Queue
			performTCPClientThreads();		//perform TCP events and submit to Ethernet/WiFi/PRO controllers
			ExecQueuedEvents();
		}
		if ((SecIter % 20) == 1)		//timing tests
		{
			//	performTCPClientThreads();
				//LOG(LOG_STATUS, "TIM: %d", tim - prevtim);
				//prevtim = tim;
		}
		if (m_ViaTCP)
		{
			if (m_NoDetectTCPPack > 0) m_NoDetectTCPPack--;
			m_LastHeartbeat = mytime(nullptr);
			if ((SecIter % 100) == 1)		//15-30 sec - send keep alive
			{

				//_log.Log(LOG_STATUS, "[TCP Keep Alive ] ");
				char dta[2];
				dta[0] = 'q';
				send(m_TCPSocket, dta, 1, 0);
			}
		}
		else
			if ((SecIter % 100) == 1)
			{

				// LOG(LOG_STATUS, "!!!!TTTTIM: %d", time(nullptr) - tt);
				// tt = time(nullptr);
				m_LastHeartbeat = mytime(nullptr);
			}
		char eh1 = 0;
		size = sizeof(caddr);
		memset(udp_status, 0, sizeof(udp_status));
		if (!m_ViaTCP)
			nbytes = recvfrom(m_eHouseUDPSocket, (char *)&udp_status, MAXMSG, 0, (struct sockaddr *) & caddr, &size);	//Standard UDP Operation (LAN)
		else
		{
			if (m_NoDetectTCPPack != 0)
			{
				nbytes = recv(m_TCPSocket, (char *)&udp_status, MAXMSG, 0);				//TCP Operation Based on Timeout - may loose combined statuses
				if (eH1(udp_status[1], udp_status[2]))
					eh1 = 1;
			}
			else	//recover multiple packets for Slow links (eg. Internet)
			{
				int len = 0;
				nbytes = recv(m_TCPSocket, (char *)&udp_status, 4, 0);
				/*				if (udp_status[2] == 202)
									{
									len ++;
									}*/
				if (nbytes == 4)	//size + address
				{
					len = udp_status[0] - 2;
					if (udp_status[0] == 22)
					{
						len -= 2;						///  Old wifi Workaround
					}
				}
				else        //problem receive data
					if (nbytes > 0)
					{
						LOG(LOG_STATUS, "Problem receive data<3");
						recv(m_TCPSocket, (char *)&udp_status, MAXMSG, 0);
						continue;
					}
					else
						if (nbytes < 0)									//SOCKET ERROR
						{
							if (IsStopRequested(0))					//real termination - end LOOP
							{
								TerminateUDP();
								break;
							}
							comm++;
							//_log.Log(LOG_STATUS, "Nbytes<0");
							if ((m_ViaTCP) && (comm > 600))							// TCP must try reconnect
							{
								comm = 0;
								_log.Log(LOG_STATUS, "[eHouse TCP] Connection Lost. Reconnecting...");
								closesocket(m_TCPSocket);				// Try close socket
								m_TCPSocket = -1;
								sleep_seconds(6);
								m_TCPSocket = ConnectTCP(0);				// Reconect
																		//	goto RETRY;							// Go to start
							}
							continue;								//if UDP just ignore errors
						}



				if ((udp_status[0] == 0) || (udp_status[2] == 0) || (nbytes == 0))
				{
					LOG(LOG_STATUS, "[eHouse TCP] Data Is Zero");
					recv(m_TCPSocket, (char *)&udp_status, MAXMSG, 0);
					continue;
				}
				if (eH1(udp_status[1], udp_status[2]))					///Check if via eHouse PRO Gateway (eHouse RS-485, CAN, RF, Thermostat, etc)
				{
					//					LOG(LOG_STATUS, "EH 1");
					len--;
					eh1 = 1;
				}
				if ((udp_status[0] < 255) && (udp_status[4] != 'n'))	//not eHouse PRO
				{
					nbytes += recv(m_TCPSocket, (char *)&udp_status[4], len, 0);
				}
				else
				{									//eHouse PRO
					if (m_ProSize == 0) m_ProSize = MAXMSG;
					nbytes += recv(m_TCPSocket, (char *)&udp_status[4], m_ProSize, 0);
				}
			}
		}

		if (nbytes == 0) continue;
		if (nbytes < 0)									//SOCKET ERROR
		{
			if (IsStopRequested(0))					//real termination - end LOOP
			{
				TerminateUDP();
				break;
			}
			comm++;
			//_log.Log(LOG_STATUS, "Nbytes<0");
			if ((m_ViaTCP) && (comm > 600))							// TCP must try reconnect
			{
				comm = 0;
				_log.Log(LOG_STATUS, "[eHouse TCP] Connection Lost. Reconnecting...");
				closesocket(m_TCPSocket);				// Try close socket
				m_TCPSocket = -1;
				sleep_seconds(6);
				m_TCPSocket = ConnectTCP(0);				// Reconnect
														//	goto RETRY;							// Go to start
			}
			continue;								//if UDP just ignore errors
		}
		comm = 0;

		if (m_ViaTCP)
		{
			devaddrh = m_ipaddrh = udp_status[1];
			devaddrl = m_ipaddrl = udp_status[2];
		}
		else
		{
			m_ipaddrh = (unsigned char)(((caddr.sin_addr.s_addr & 0x00ff0000) >> 16) & 0xff);
			m_ipaddrl = (caddr.sin_addr.s_addr >> 24) & 0xff;
		}


		if (nbytes > 254)
		{
			//comm = 0;
			if (udp_status[3] == 'n')
			{						//get names
				m_NoDetectTCPPack = 120;
				_log.Log(LOG_STATUS, "[%s NAMES: 192.168.%d.%d ] ", LogPrefix, udp_status[1], udp_status[2]);
				if ((m_eHEnableAutoDiscovery))
				{
					//if (!TESTTEST)
					GetUDPNamesLAN(udp_status, nbytes);
					//if (!TESTTEST)
					GetUDPNamesCM(udp_status, nbytes);
					//if (!TESTTEST)
					if (m_eHEnableProDiscovery) GetUDPNamesPRO(udp_status, nbytes);
					//if (!TESTTEST)
					GetUDPNamesRS485(udp_status, nbytes);
					//if (!TESTTEST)
					GetUDPNamesWiFi(udp_status, nbytes);
				}
			}
			else
			{   //eHouse PRO  status
				sum = 0;
				sum2 = ((unsigned int)udp_status[nbytes - 2]) << 8;   //get sent checksum
				sum2 += udp_status[nbytes - 1];
				unsigned int iii;
				for (iii = 0; iii < nbytes - 2U; iii++) // calculate local checksum
				{
					sum += udp_status[iii];
				}
				devaddrh = udp_status[4];
				devaddrl = udp_status[5];
				if (udp_status[0] != 0xff) continue;
				if (udp_status[1] != 0x55) continue;
				if (udp_status[2] != 0xff) continue;
				if (udp_status[3] != 0x55) continue;
				if (m_ViaTCP)
				{
					devaddrh = m_ipaddrh = udp_status[4];
					devaddrl = m_ipaddrl = udp_status[5];
				}
				//debu(devaddrh, devaddrl, 0, nbytes, sum, sum2, udp_status[0]);
				if (((sum & 0xffff) == (sum2 & 0xffff)) || (m_ViaTCP))        //if valid then perform
				{
					m_ProSize = MAXMSG;// nbytes - 4;

					//eHPROaloc(0, devaddrh, devaddrl);
					if (!m_ViaTCP)
						if ((m_ipaddrh != m_SrvAddrH) || (m_ipaddrl != m_SrvAddrL))
						{
							if (TESTTEST)
								LOG(LOG_STATUS, "[%s PRO] Ignore other PRO installation from Server: 192.168.%d.%d", LogPrefix, m_ipaddrh, m_ipaddrl);
							continue;
						}
					
					if (m_StatusDebug) LOG(LOG_STATUS, "[%s PRO] status installation from Server: 192.168.%d.%d", LogPrefix, m_ipaddrh, m_ipaddrl);
					memcpy(m_eHouseProStatus->data, &udp_status, sizeof(m_eHouseProStatus->data));
					UpdatePROToSQL(devaddrh, devaddrl);
				}
				else _log.Log(LOG_STATUS, "[%s Pro] Invalid Checksum %d,%d", LogPrefix, devaddrh, devaddrl);
			}
			continue;
		}


		//received status
		{										//size more than 0
			sum = 0;
			sum2 = ((unsigned int)udp_status[nbytes - 2]);
			sum2 = sum2 << 8;							//get sent checksum
			sum2 += udp_status[nbytes - 1];
			for (i = 0; i < nbytes - 2; i++)               //calculate local checksum
			{
				sum += (unsigned int)udp_status[i];
			}
			devaddrh = udp_status[1];
			devaddrl = udp_status[2];
			//debu(devaddrh, devaddrl, 0, nbytes, sum, sum2, udp_status[0]);
			if (((sum & 0xffff) == (sum2 & 0xffff)) || (eh1)) //if valid then perform
			{
				m_HeartBeat++;
				if (m_disablers485)
				{
					if ((devaddrh == 55) || (devaddrh == 1) || (devaddrh == 2)) continue;
				}
				//if( StatusDebug) printf("[UDP] Status: (%-3d,%-3d)  (%dB)\r\n", devaddrh, devaddrl, nbytes);
				if (devaddrl > 199) i = EHOUSE1_RM_MAX + 3;
				else if (devaddrh == 1) // HM index in status eH[0]
					i = 0;
				else if (devaddrh == 2) // EM index in status eH[0]
					i = EHOUSE1_RM_MAX - 1;
				else
					i = devaddrl; // RM index in status the same as device address low - eH[devaddrl]
				if (udp_status[3] != 'l')
				{
					if (m_StatusDebug) _log.Log(LOG_STATUS, "[%s] St: (%-3d,%-3d) - OK (%dB)", LogPrefix, devaddrh, devaddrl, nbytes);
				}
				else
					//if    (udp_status[3] == 'l')
					if (nbytes > 6)
					{
						strncpy((char *)&log, (char *)&udp_status[4], nbytes - 6);
						LOG(LOG_STATUS, "[%s Log] (%-3d,%-3d) - %s", LogPrefix, devaddrh, devaddrl, log);
						if (m_IRPerform)
						{
							if (strstr((char *)&log, "[IR]"))
							{
								sprintf((char *)&dir, "%02x_%02x", devaddrh, devaddrl);
								/*                                                mf("IR", dir,"captured", &log[6], 0);
																				mf("IR", dir,"events", &log[6], 1);*/
							}
							continue;
						}
					}
				int index = IndexOfeHouseRS485(devaddrh, devaddrl);

				if (index >= 0)
				{
					if (((m_ipaddrh != m_SrvAddrH) || (m_ipaddrl != m_SrvAddrL)) && (!m_ViaTCP))
					{
						if (TESTTEST) LOG(LOG_STATUS, "Ignore other instalation from Serwer: 192.168.%d.%d", m_ipaddrh, m_ipaddrl);
						continue;
					}


					eHaloc(index, devaddrh, devaddrl);
					if (memcmp(&m_eHn[index]->BinaryStatus[0], &udp_status, nbytes) != 0) m_CloudStatusChanged = 1;
					memcpy(&m_eHn[index]->BinaryStatus[0], udp_status, nbytes);
					memcpy(&m_eHRMs[index]->data[0], &udp_status, 4);					//control data size,address,code 's'
					m_eHRMs[index]->eHERM.Outs[0] = udp_status[RM_STATUS_OUT];
					m_eHRMs[index]->eHERM.Outs[1] = udp_status[RM_STATUS_OUT + 1];
					m_eHRMs[index]->eHERM.Outs[2] = udp_status[RM_STATUS_OUT + 2];
					m_eHRMs[index]->eHERM.Outs[3] = udp_status[RM_STATUS_OUT25];
					m_eHRMs[index]->eHERM.Inputs[0] = udp_status[RM_STATUS_IN];
					m_eHRMs[index]->eHERM.Inputs[1] = udp_status[RM_STATUS_INT];
					m_eHRMs[index]->eHERM.Dimmers[0] = udp_status[RM_STATUS_LIGHT];
					m_eHRMs[index]->eHERM.Dimmers[1] = udp_status[RM_STATUS_LIGHT + 1];
					m_eHRMs[index]->eHERM.Dimmers[2] = udp_status[RM_STATUS_LIGHT + 2];
					CalculateAdcEH1(index);
					//memcpy(&eHRMs[index].eHERM.More, &udp_status[STATUS_MORE],8);
					//memcpy(&eHRMs[index].eHERM.DMXDimmers[17], &udp_status[STATUS_DMX_DIMMERS2], 15);
					//memcpy(&eHRMs[eHEIndex].eHERM.DaliDimmers, &udp_status[STATUS_DALI],46);

					m_eHRMs[index]->eHERM.CURRENT_PROFILE = udp_status[RM_STATUS_PROGRAM];
					if (index == 0)			//HeatManager Controller (eHouse RS-485)
					{
						m_eHRMs[index]->eHERM.CURRENT_PROFILE = udp_status[HM_STATUS_PROGRAM];
						m_eHRMs[index]->eHERM.Outs[0] = udp_status[HM_STATUS_OUT];
						m_eHRMs[index]->eHERM.Outs[1] = udp_status[HM_STATUS_OUT + 1];
						m_eHRMs[index]->eHERM.Outs[2] = udp_status[HM_STATUS_OUT + 2];
						m_eHRMs[index]->eHERM.Outs[3] = 0;
						m_eHRMs[index]->eHERM.Inputs[0] = 0;
						m_eHRMs[index]->eHERM.Inputs[1] = 0;
					}
					if (index == EHOUSE1_RM_MAX)		//EM
						m_eHRMs[index]->eHERM.CURRENT_ZONE = udp_status[RM_STATUS_ZONE_PGM];
					//eHRMs[index].eHERM.CURRENT_ADC_PROGRAM= udp_status[STATUS_ADC_PROGRAM];
					if (m_CloudStatusChanged)
					{
						UpdateRS485ToSQL(devaddrh, devaddrl, index);
						//    CloudStatusChanged = 0;
					}
					m_eHn[index]->FromIPH = m_ipaddrh;
					m_eHn[index]->FromIPL = m_ipaddrl;
					m_eHn[index]->BinaryStatusLength = nbytes;
					if (m_ViaCM)
					{
						eCMaloc(0, m_COMMANAGER_IP_HIGH, m_COMMANAGER_IP_LOW);
						memcpy(m_ECMn->BinaryStatus, udp_status, nbytes);
						m_ECMn->BinaryStatusLength = nbytes;
						m_ECMn->TCPQuery++;
						memcpy(&m_ECM->data[0], &udp_status[70], sizeof(m_ECM->data));       //eHouse 1 Controllers
					}
					m_eHn[index]->TCPQuery++;

					if (m_CloudStatusChanged)
					{
						UpdateCMToSQL(devaddrh, devaddrl, 0);
					}
					m_CloudStatusChanged = 0;
					//CalculateAdcCM();
					m_eHStatusReceived++;                                    //Flag of eHouse1 status received
					m_eHEStatusReceived++;                                    //flag of Ethernet eHouse status received
				}
				else			// ethernet controllers or CM without eHouse1 status
				{
					if (IsCM(devaddrh, devaddrl))        //CommManager/Level Manager
					{
						eCMaloc(0, devaddrh, devaddrl);
						memcpy(m_ECMn->BinaryStatus, udp_status, nbytes);
						m_ECMn->BinaryStatusLength = nbytes;
						m_ECMn->TCPQuery++;
						if (memcmp(m_ECM->data, &udp_status[70], sizeof(m_ECM->data) - 70) != 0) m_CloudStatusChanged = 1;

						memcpy(&m_ECM->data[0], &udp_status[70], sizeof(m_ECM->data));          //CommManager stand alone
						m_ECM->CM.AddrH = devaddrh;                        //Overwrite to default CM address
						m_ECM->CM.AddrL = devaddrl;
						if (m_CloudStatusChanged)
						{
							UpdateCMToSQL(devaddrh, devaddrl, 0);
						}
						m_CloudStatusChanged = 0;
						//CalculateAdcCM();
						m_eHEStatusReceived++;
					}
					else    //Not CM
					{
						if (devaddrl != 0)
						{
							if (devaddrl >= m_INITIAL_ADDRESS_LAN)
							{
								if (devaddrl - m_INITIAL_ADDRESS_LAN <= ETHERNET_EHOUSE_RM_MAX)   //Ethernet eHouse LAN Controllers
								{
									unsigned char eHEIndex = (devaddrl - m_INITIAL_ADDRESS_LAN) % (STATUS_ARRAYS_SIZE);
									{

										eHEaloc(eHEIndex, devaddrh, devaddrl);
										if (memcmp(m_eHEn[eHEIndex]->BinaryStatus, udp_status, nbytes) != 0)
											m_CloudStatusChanged = 1;

										memcpy(m_eHEn[eHEIndex]->BinaryStatus, udp_status, nbytes);
										m_eHEn[eHEIndex]->BinaryStatusLength = nbytes;
										m_eHEn[eHEIndex]->TCPQuery++;

										{
											memcpy(&m_eHERMs[eHEIndex]->data[0], &udp_status, 24); //control, dimmers
											memcpy(&m_eHERMs[eHEIndex]->eHERM.ADC[0], &udp_status[STATUS_ADC_ETH], 32);//adcs
											memcpy(&m_eHERMs[eHEIndex]->eHERM.Outs[0], &udp_status[STATUS_OUT_I2C], 5);
											memcpy(&m_eHERMs[eHEIndex]->eHERM.Inputs[0], &udp_status[STATUS_INPUTS_I2C], 3);
											//memcpy(&eHERMs[eHEIndex].eHERM.ADC, &udp_status[STATUS_INPUTS_I2C]
											CalculateAdc2(eHEIndex);


											memcpy(m_eHERMs[eHEIndex]->eHERM.More, &udp_status[STATUS_MORE], 8);
											memcpy(&m_eHERMs[eHEIndex]->eHERM.DMXDimmers[17], &udp_status[STATUS_DMX_DIMMERS2], 15);
											memcpy(m_eHERMs[eHEIndex]->eHERM.DaliDimmers, &udp_status[STATUS_DALI], 46);

											//eHERM.CURRENT_PROGRAM= udp_status[STATUS_PROGRAM_NR];
											m_eHERMs[eHEIndex]->eHERM.CURRENT_PROFILE = udp_status[STATUS_PROFILE_NO];
											m_eHERMs[eHEIndex]->eHERM.CURRENT_ADC_PROGRAM = udp_status[STATUS_ADC_PROGRAM];
											if (m_CloudStatusChanged)
											{
												UpdateLanToSQL(devaddrh, devaddrl, eHEIndex);
												m_CloudStatusChanged = 0;
											}
										}
										m_eHEStatusReceived++;
									}
								}
							}
							else if (devaddrl >= m_INITIAL_ADDRESS_WIFI)     //eHouse WiFi Controllers
							{
								if (devaddrl - m_INITIAL_ADDRESS_WIFI <= EHOUSE_WIFI_MAX)
								{
									unsigned char eHWiFiIndex = (devaddrl - m_INITIAL_ADDRESS_WIFI) % STATUS_WIFI_ARRAYS_SIZE;
									eHWIFIaloc(eHWiFiIndex, devaddrh, devaddrl);
									{
										if (memcmp(m_eHWIFIn[eHWiFiIndex]->BinaryStatus, udp_status, nbytes) != 0) m_CloudStatusChanged = 1;
										memcpy(m_eHWIFIn[eHWiFiIndex]->BinaryStatus, udp_status, nbytes);
										m_eHWIFIn[eHWiFiIndex]->BinaryStatusLength = nbytes;
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.AddrH = devaddrh;
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.AddrL = devaddrl;
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Size = nbytes;
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Code = 's';
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Outs[0] = udp_status[OUT_OFFSET_WIFI];
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Inputs[0] = udp_status[IN_OFFSET_WIFI];
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[0] = udp_status[DIMM_OFFSET_WIFI];
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[1] = udp_status[DIMM_OFFSET_WIFI + 1];
										m_eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[2] = udp_status[DIMM_OFFSET_WIFI + 2];
										//eHWIFIs[index].eHWIFI.Dimmers[3] = udp_status[DIMM_OFFSET_WIFI+3];
										m_eHWIFIn[eHWiFiIndex]->TCPQuery++;
										memcpy(&m_eHWiFi[eHWiFiIndex]->data[0], &udp_status[1], sizeof(union WiFiStatusT));          //eHouse WiFi Status
										CalculateAdcWiFi(eHWiFiIndex);
										if (m_CloudStatusChanged)
										{

											UpdateWiFiToSQL(devaddrh, devaddrl, eHWiFiIndex);
											m_CloudStatusChanged = 0;
										}
										m_eHWiFiStatusReceived++;
									}
								}
							}
							else
							{
								if (devaddrh == 0x81) // Aura Thermostats Via eHouse PRO
								{
									if (devaddrl >= MAX_AURA_DEVS) continue;
									unsigned char aindex = 0;
									aindex = devaddrl - 1;
									eAURAaloc(aindex, devaddrh, devaddrl);
									memcpy(m_AuraN[aindex]->BinaryStatus, &udp_status, sizeof(m_AuraN[aindex]->BinaryStatus));

									m_AuraDev[aindex]->Addr = devaddrl;		// address l
									i = 3;
									m_AuraDev[aindex]->ID = m_AuraN[aindex]->BinaryStatus[i++];
									m_AuraDev[aindex]->ID = m_AuraDev[aindex]->ID << 8;
									m_AuraDev[aindex]->ID |= m_AuraN[aindex]->BinaryStatus[i++];
									m_AuraDev[aindex]->ID = m_AuraDev[aindex]->ID << 8;
									m_AuraDev[aindex]->ID |= m_AuraN[aindex]->BinaryStatus[i++];
									m_AuraDev[aindex]->ID = m_AuraDev[aindex]->ID << 8;
									m_AuraDev[aindex]->ID |= m_AuraN[aindex]->BinaryStatus[i++];
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "[Aura UDP %d] ID: %lx\t", aindex + 1, (long int)m_AuraDev[aindex]->ID);
									m_AuraDev[aindex]->DType = m_AuraN[aindex]->BinaryStatus[i++];            //device type
									if (m_DEBUG_AURA) LOG(LOG_STATUS, " DevType: %d\t", m_AuraDev[index]->DType);
									i++;//params count
									int m, k = 0;
									int bkpi = i + 2;
									char FirstTime = 0;
									for (m = 0; m < sizeof(m_AuraN[aindex]->ParamSymbol); m++)
									{
										//if ((AuraDev[index].ParamSize>2) && (!FirstTime))
										{

											m_AuraN[aindex]->ParamValue[m] = m_AuraN[aindex]->BinaryStatus[i++];
											m_AuraN[aindex]->ParamValue[m] = m_AuraN[aindex]->ParamValue[m] << 8;
											m_AuraN[aindex]->ParamValue[m] |= m_AuraN[aindex]->BinaryStatus[i++];
											//if (AuraDev[index].ParamValue[i]>0x8000)


											m_AuraN[aindex]->ParamPreset[m] = m_AuraN[aindex]->BinaryStatus[i++];
											m_AuraN[aindex]->ParamPreset[m] = m_AuraN[aindex]->ParamPreset[m] << 8;
											m_AuraN[aindex]->ParamPreset[m] |= m_AuraN[aindex]->BinaryStatus[i++];

											m_AuraN[aindex]->ParamSymbol[m] = m_AuraN[aindex]->BinaryStatus[i++];


											if (((m_AuraN[aindex]->ParamSymbol[m] == 'C') || (m_AuraN[aindex]->ParamSymbol[m] == 'T')) && (!FirstTime))
											{
												FirstTime++;
												m_AuraN[aindex]->ADCUnit[aindex] = 'T';
												m_AuraDev[aindex]->Temp = m_AuraN[aindex]->ParamValue[m];
												m_AuraDev[aindex]->Temp /= 10;
												//                                                 AuraDev[aindex]->TempSet = AuraN[aindex]->ParamPreset[m];
												//                                               AuraDev[aindex]->TempSet/= 10;
												m_AuraDev[aindex]->LocalTempSet = m_AuraN[aindex]->ParamPreset[m];
												m_AuraDev[aindex]->LocalTempSet /= 10;
												if (m_AuraDev[aindex]->LocalTempSet != m_AuraDev[aindex]->PrevLocalTempSet)
												{
													m_AuraDev[aindex]->PrevLocalTempSet = m_AuraDev[aindex]->LocalTempSet;
													m_AuraDev[aindex]->TempSet = m_AuraDev[aindex]->LocalTempSet;
													m_AuraDev[aindex]->ServerTempSet = m_AuraDev[aindex]->LocalTempSet;
													m_AuraDev[aindex]->PrevServerTempSet = m_AuraDev[aindex]->LocalTempSet;
													if (m_DEBUG_AURA) LOG(LOG_STATUS, "LTempC: %f\t", m_AuraDev[aindex]->TempSet);
												}
												if (m_AuraDev[aindex]->ServerTempSet != m_AuraDev[aindex]->PrevServerTempSet)
												{
													//AuraDev[auraindex].PrevLocalTempSet = AuraDev[auraindex].ServerTempSet;
													m_AuraDev[aindex]->TempSet = m_AuraDev[aindex]->ServerTempSet;
													//AuraDev[auraindex].LocalTempSet = AuraDev[auraindex].ServerTempSet;
													m_AuraDev[aindex]->PrevServerTempSet = m_AuraDev[aindex]->ServerTempSet;
													if (m_DEBUG_AURA) LOG(LOG_STATUS, "STempC: %f\t", m_AuraDev[aindex]->TempSet);
													//AuraN[auraindex].ParamPreset[0] = (unsigned int) (AuraDev[auraindex].TempSet*10);
												}
												//for (i = 0; i<AuraDev[auraindex].ParamsCount; i++)
												{
													m_AuraN[aindex]->ParamPreset[0] = (unsigned int)(m_AuraDev[aindex]->TempSet * 10);

													//eHPROaloc(0, SrvAddrH, SrvAddrL);

													m_eHouseProStatus->status.AdcVal[aindex] = m_AuraN[aindex]->ParamValue[0];// temp * 10;//.MSB<<8) + eHouseProStatus.status.AdcVal[nr_of_ch].LSB;
																														  //adcs[aindex]->ADCValue= (int) AuraDev[aindex]->temp * 10;
													m_adcs[aindex]->ADCHigh = m_AuraN[aindex]->ParamPreset[0] + 3;
													m_adcs[aindex]->ADCLow = m_AuraN[aindex]->ParamPreset[0] - 3;
													m_AuraN[aindex]->BinaryStatus[bkpi++] = m_AuraN[aindex]->ParamPreset[0] >> 8;
													m_AuraN[aindex]->BinaryStatus[bkpi] = m_AuraN[aindex]->ParamPreset[0] & 0xff;
													m_nr_of_ch = aindex;
													//        PerformADC();       //Perform Adc measurement process
													//                                                                    AuraN[aindex]->TextStatus[0] = 0;
												}


											}
										}
									}
									m_AuraDev[aindex]->RSSI = -(255 - m_AuraN[aindex]->BinaryStatus[i++]);
									if (m_DEBUG_AURA) LOG(LOG_STATUS, " RSSI: %d\t", m_AuraDev[aindex]->RSSI);
									m_AuraN[aindex]->Vcc = m_AuraN[aindex]->BinaryStatus[i++];
									m_AuraDev[aindex]->volt = m_AuraN[aindex]->Vcc;
									m_AuraDev[aindex]->volt /= 10;
									if (m_DEBUG_AURA) LOG(LOG_STATUS, " Vcc: %d\t", m_AuraN[aindex]->Vcc);
									m_AuraDev[aindex]->Direction = m_AuraN[aindex]->BinaryStatus[i++];
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "Direction: %d\t", m_AuraDev[aindex]->Direction);
									m_AuraDev[aindex]->DoorState = m_AuraN[aindex]->BinaryStatus[i++];
									switch (m_AuraDev[aindex]->DoorState & 0x7)
									{
									case 0: m_AuraDev[aindex]->Door = '|';
										m_adcs[aindex]->door = 0;
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_CLOSE;
										m_AuraDev[aindex]->WINDOW_CLOSE = 1;
										break;

									case 1: m_AuraDev[aindex]->Door = '<';
										m_AuraDev[aindex]->WINDOW_OPEN = 1;
										m_adcs[aindex]->door = 10;
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										m_adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 2:
										m_AuraDev[aindex]->WINDOW_SMALL = 1;
										m_adcs[aindex]->door = 2;
										m_AuraDev[aindex]->Door = '/';
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_SMALL;
										m_adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableFan = 0;    //Disable Ventilation if flag is set
										break;
									case 3:
										m_adcs[aindex]->door = 10;
										m_AuraDev[aindex]->WINDOW_OPEN = 1;
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										m_AuraDev[aindex]->Door = '>';
										m_adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 4:
										m_AuraDev[aindex]->WINDOW_UNPROOF = 1;
										m_AuraDev[aindex]->Door = '~';
										m_adcs[aindex]->door = 1;
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_UNPROOF;
										break;
									case 5:
										m_adcs[aindex]->door = 10;
										m_adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										m_AuraDev[aindex]->Door = 'X';
										m_adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										m_adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										m_adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 6: m_AuraDev[aindex]->Door = '-'; break;
									case 7: m_AuraDev[aindex]->Door = '-'; break;
									}
									if (m_AuraDev[aindex]->DoorState & 0x8)
									{
										m_AuraDev[aindex]->LOCK = 1;
										m_AuraDev[aindex]->Lock = 1;
									}
									else
									{
										m_AuraDev[aindex]->Lock = 0;
									}
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "DoorState: %d\t", m_AuraDev[aindex]->DoorState);
									m_AuraDev[aindex]->Door = m_AuraN[aindex]->BinaryStatus[i++];
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "Door: %d\t", m_AuraDev[aindex]->Door);
									m_adcs[aindex]->flagsa = m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsa = m_adcs[aindex]->flagsa << 8;
									m_adcs[aindex]->flagsa |= m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsa = m_adcs[aindex]->flagsa << 8;
									m_adcs[aindex]->flagsa |= m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsa = m_adcs[aindex]->flagsa << 8;
									m_adcs[aindex]->flagsa |= m_AuraN[aindex]->BinaryStatus[i++];
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "FlagsA: %lx\t", m_adcs[aindex]->flagsa);

									m_adcs[aindex]->flagsb = m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsb = m_adcs[aindex]->flagsb << 8;
									m_adcs[aindex]->flagsb |= m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsb = m_adcs[aindex]->flagsb << 8;
									m_adcs[aindex]->flagsb |= m_AuraN[aindex]->BinaryStatus[i++];
									m_adcs[aindex]->flagsb = m_adcs[aindex]->flagsb << 8;
									m_adcs[aindex]->flagsb |= m_AuraN[aindex]->BinaryStatus[i++];
									unsigned int time = m_AuraN[aindex]->BinaryStatus[i++];
									time = time << 8;
									time += m_AuraN[aindex]->BinaryStatus[i++];
									m_AuraDev[aindex]->ServerTempSet = time;
									m_AuraDev[aindex]->ServerTempSet /= 10;
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "FlagsB: %lx\t", (long unsigned int) m_adcs[aindex]->flagsb);
									m_AuraDev[aindex]->LQI = m_AuraN[aindex]->BinaryStatus[i++];
									if (m_DEBUG_AURA) LOG(LOG_STATUS, "LQI: %d\t", m_AuraDev[aindex]->LQI);
									m_AuraN[aindex]->BinaryStatusLength = m_AuraN[aindex]->BinaryStatus[0];//nbytes;
									m_AuraN[aindex]->TCPQuery++;
									m_AuraN[aindex]->StatusTimeOut = 15U;
									UpdateAuraToSQL(m_AuraN[aindex]->AddrH, m_AuraN[aindex]->AddrL, aindex);
								}
							}
						}
					}
				}
			}
			else
			{
				LOG(LOG_STATUS, "[%s] St:  (%03d,%03d) Invalid Check Sum {%d!=%d} (%dB)", LogPrefix, devaddrh, devaddrl, sum, sum2, nbytes);
				if (m_ViaTCP)
				{
					nbytes = recv(m_TCPSocket, (char *)&udp_status, MAXMSG, 0);
					continue;
				}
				//if (ViaTCP) LOG(LOG_STATUS, "[TCP n=%d] #%d (%d,%d) Data=%c", nbytes, udp_status[0], udp_status[1], udp_status[2], udp_status[4]);
			}
			//EthernetPerformData();
			devaddrl = 0;
			devaddrh = 0;
			//memset(udp_status, 0, sizeof(udp_status));
		}
	}
	//close(connected);
	if (m_ViaTCP)
	{
		char dta[2];
		dta[0] = 0;
		send(m_TCPSocket, dta, 1, 0);
#ifdef WIN32
		flushall();
#else
		//fflush(&TCPSocket);
#endif

		unsigned long iMode = 1;
#ifdef WIN32
		int status = ioctlsocket(m_TCPSocket, FIONBIO, &iMode);
#else
		int status = ioctl(m_TCPSocket, FIONBIO, &iMode);
#endif
		if (status == SOCKET_ERROR)
		{
#ifdef WIN32
			_log.Log(LOG_STATUS, "ioctlsocket failed with error: %d", WSAGetLastError());
#else
			_log.Log(LOG_STATUS, "ioctlsocket failed with error");
#endif
			//closesocket(TCPSocket);
			//WSACleanup();
			//return -1;
		}
		//after connection change timeout

/*	  struct timeval timeout;
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 100000ul;	//100ms for delay
	  if (setsockopt(TCPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) < 0)   //Set socket Read operation Timeout
			{
			LOG(LOG_ERROR, "[TCP Client Status] Set Read Timeout failed");
			perror("[TCP Client Status] Set Read Timeout failed\n");
			}
	 if (setsockopt(TCPSocket, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout)) < 0)   //Set Socket Write operation Timeout
			{
			LOG(LOG_ERROR, "[TCP Client Status] Set Write Timeout failed");
			perror("[TCP Client Status] Set Write Timeout failed\n");
			}*/
		sleep_seconds(1);
		ressock = shutdown(m_eHouseUDPSocket, SHUT_RDWR);
	}
	else
		ressock = shutdown(m_eHouseUDPSocket, SHUT_RDWR);
	LOG(LOG_STATUS, "[%s] Shut %d/ERROR CODE: %d", LogPrefix, (unsigned int)ressock, errno);
	switch (errno)
	{
	case EBADF:
		LOG(LOG_STATUS, "EBADF");
		break;
	case EINVAL:
		LOG(LOG_STATUS, "Einval");
		break;
	case ENOTCONN:
		LOG(LOG_STATUS, "enotcon");
		break;
	case ENOTSOCK:
		LOG(LOG_STATUS, "Enotsock");
		break;
	case ENOBUFS:
		LOG(LOG_STATUS, "Enobufs");
		break;
	}
#ifndef WIN32
	if (m_eHouseUDPSocket >= 0)
		ressock = close(m_eHouseUDPSocket);
	if (m_TCPSocket >= 0)
		ressock = close(m_TCPSocket);
#else
	if (m_eHouseUDPSocket >= 0)
		ressock = closesocket(m_eHouseUDPSocket);
	if (m_TCPSocket >= 0)
		ressock = closesocket(m_TCPSocket);
#endif
	m_eHouseUDPSocket = -1;
	m_TCPSocket = -1;

	LOG(LOG_STATUS, "close %d", ressock);
	LOG(LOG_STATUS, "\r\nTERMINATED UDP\r\n");
	//Freeing ALL Dynamic memory
	int eHEIndex = 0;
	for (eHEIndex = 0; eHEIndex < ETHERNET_EHOUSE_RM_MAX + 1; eHEIndex++)
		//	if (strlen((char *) &eHEn[eHEIndex])>0)
		if (m_eHEn[eHEIndex] != nullptr)
		{
			LOG(LOG_STATUS, "Freeing 192.168.%d.%d", m_eHEn[eHEIndex]->AddrH, m_eHEn[eHEIndex]->AddrL);
			free(m_eHEn[eHEIndex]);
			m_eHEn[eHEIndex] = nullptr;
			free(m_eHERMs[eHEIndex]);
			free(m_eHERMPrev[eHEIndex]);
			m_eHERMs[eHEIndex] = nullptr;
			m_eHERMPrev[eHEIndex] = nullptr;
		}

	for (eHEIndex = 0; eHEIndex < EHOUSE1_RM_MAX + 1; eHEIndex++)
		//if (strlen((char *) &eHn[eHEIndex])>0)
		if (m_eHn[eHEIndex] != nullptr)
		{
			LOG(LOG_STATUS, "Freeing (%d,%d)", m_eHn[eHEIndex]->AddrH, m_eHn[eHEIndex]->AddrL);
			free(m_eHn[eHEIndex]);
			m_eHn[eHEIndex] = nullptr;
			free(m_eHRMs[eHEIndex]);
			free(m_eHRMPrev[eHEIndex]);
			m_eHRMs[eHEIndex] = nullptr;
			m_eHRMPrev[eHEIndex] = nullptr;
		}

	for (eHEIndex = 0; eHEIndex < EHOUSE_WIFI_MAX + 1; eHEIndex++)
		//	if (strlen((char *) &eHWIFIn[eHEIndex]) > 0)
		if (m_eHWIFIn[eHEIndex] != nullptr)
		{
			LOG(LOG_STATUS, "Freeing 192.168.%d.%d", m_eHWIFIn[eHEIndex]->AddrH, m_eHWIFIn[eHEIndex]->AddrL);
			free(m_eHWIFIn[eHEIndex]);
			m_eHWIFIn[eHEIndex] = nullptr;
			free(m_eHWiFi[eHEIndex]);
			free(m_eHWIFIs[eHEIndex]);
			free(m_eHWIFIPrev[eHEIndex]);
			m_eHWiFi[eHEIndex] = nullptr;
			m_eHWIFIs[eHEIndex] = nullptr;
			m_eHWIFIPrev[eHEIndex] = nullptr;
		}

	//if (strlen((char *) &ECMn) > 0)
	if (m_ECMn != nullptr)
	{
		LOG(LOG_STATUS, "Freeing 192.168.%d.%d", m_ECMn->AddrH, m_ECMn->AddrL);
		free(m_ECMn);
		m_ECMn = nullptr;
		free(m_ECM);
		free(m_ECMPrv);
		m_ECM = nullptr;
		m_ECMPrv = nullptr;
	}

	//if (strlen((char *) &eHouseProN) > 0)
	if (m_eHouseProN != nullptr)
	{
		LOG(LOG_STATUS, "Freeing 192.168.%d.%d", m_eHouseProN->AddrH[0], m_eHouseProN->AddrL[0]);
		free(m_eHouseProN);
		m_eHouseProN = nullptr;
		free(m_eHouseProStatus);
		free(m_eHouseProStatusPrv);
		m_eHouseProStatus = nullptr;
		m_eHouseProStatusPrv = nullptr;
	}

	for (i = 0; i < EVENT_QUEUE_MAX; i++)
	{

		{
			free(m_EvQ[i]);
			m_EvQ[i] = nullptr;
		}
	}

	for (i = 0; i < MAX_AURA_DEVS; i++)
	{
		//if (strlen((char *) & (AuraN[i])) < 1)
		if (m_AuraN[i] != nullptr)
		{
			LOG(LOG_STATUS, "Free AURA (%d,%d)", 0x81, i + 1);
			free(m_AuraN[i]);
			m_AuraN[i] = nullptr;
			free(m_AuraDev[i]);
			free(m_AuraDevPrv[i]);
			m_AuraDev[i] = nullptr;
			m_AuraDevPrv[i] = nullptr;
			free(m_adcs[i]);
			m_adcs[i] = nullptr;
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////
	LOG(LOG_STATUS, "END LISTENER TCP Client");
	m_bIsStarted = false;
}


