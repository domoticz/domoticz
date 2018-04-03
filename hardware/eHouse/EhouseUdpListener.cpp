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
 */

#include "stdafx.h"
#include "../main/Logger.h"
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
#include "../../main/mainworker.h"
#include "../../main/SQLHelper.h"
unsigned char TESTTEST = 0;
unsigned char eHEnableAutoDiscovery = 1;
unsigned char eHEnableProDiscovery = 1;
unsigned char eHEnableAlarmInputs = 0;
int TCPSocket = -1;

//allocate dynamically names structure only during discovery of eHouse PRO controller
void eHouseTCP::eCMaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	//	if (strlen((char *) &ECMn) < 1)
	if (ECMn == NULL)
	{
		LOG(LOG_STATUS, "Allocating CommManager LAN Controller (192.168.%d.%d)", devaddrh, devaddrl);
		ECMn = (struct CommManagerNamesT *) malloc(sizeof(struct CommManagerNamesT));
		if (ECMn == NULL) LOG(LOG_ERROR, "CAN'T Allocate ECM Names Memory");
		ECMn->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
		ECMn->AddrH = devaddrh;
		ECMn->AddrL = devaddrl;
		ECM = (union CMStatusT *) malloc(sizeof(union CMStatusT));
		ECMPrv = (union CMStatusT *) malloc(sizeof(union CMStatusT));
		if (ECM == NULL) LOG(LOG_ERROR, "CAN'T Allocate ECM Memory");
		if (ECMPrv == NULL) LOG(LOG_ERROR, "CAN'T Allocate ECMPrev Memory");
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////
//allocate dynamically names structure only during discovery of eHouse PRO controller
//////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::eHPROaloc(int eHEIndex, int devaddrh, int devaddrl)
{
	//	if (strlen((char *) &eHouseProN) < 1)
	if (eHouseProN == NULL)
	{
		LOG(LOG_STATUS, "Allocating eHouse PRO Controller (192.168.%d.%d)", devaddrh, devaddrl);
		eHouseProN = (struct eHouseProNamesT *) malloc(sizeof(struct eHouseProNamesT));
		if (eHouseProN == NULL) LOG(LOG_ERROR, "CAN'T Allocate PRO Names Memory");
		eHouseProN->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
		eHouseProN->AddrH[0] = devaddrh;
		eHouseProN->AddrL[0] = devaddrl;
		eHouseProStatus = (union eHouseProStatusUT *)  malloc(sizeof(union eHouseProStatusUT));
		eHouseProStatusPrv = (union eHouseProStatusUT *) malloc(sizeof(union eHouseProStatusUT));
		if (eHouseProStatus == NULL) LOG(LOG_ERROR, "CAN'T Allocate PRO Stat Memory");
		if (eHouseProStatusPrv == NULL) LOG(LOG_ERROR, "CAN'T Allocate PRO Stat PRV Memory");
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
		if (AuraN[i] == NULL)
		{
			LOG(LOG_STATUS, "Allocating Aura Thermostat (%d.%d)", devaddrh, i + 1);
			AuraN[i] = (struct AuraNamesT *) malloc(sizeof(struct AuraNamesT));
			if (AuraN[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate AURA Names Memory");
			AuraN[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			AuraN[i]->AddrH = devaddrh;
			AuraN[i]->AddrL = i + 1;
			AuraDev[i] = (struct AURAT *) malloc(sizeof(struct AURAT));
			AuraDevPrv[i] = (struct AURAT *) malloc(sizeof(struct AURAT));
			adcs[i] = (struct CtrlADCT *) malloc(sizeof(struct CtrlADCT));
			if (adcs[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate ADCs Memory");
			if (AuraDev[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate AURA Stat Memory");
			if (AuraDevPrv[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate AURA Stat Prv Memory");
			AuraN[i]->BinaryStatus[0] = 0;	//modified flag
			AuraDevPrv[i]->Addr = 0;		//modified flag
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
		if (eHEn[i] == NULL)
		{
			LOG(LOG_STATUS, "Allocating eHouse LAN controller (192.168.%d.%d)", devaddrh, i + INITIAL_ADDRESS_LAN);
			eHEn[i] = (struct EtherneteHouseNamesT *) malloc(sizeof(struct EtherneteHouseNamesT));
			if (eHEn[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate LAN Names Memory");
			eHEn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			eHEn[i]->AddrH = devaddrh;
			eHEn[i]->AddrL = i + INITIAL_ADDRESS_LAN;
			eHERMs[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			eHERMPrev[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			if (eHERMs[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate LAN Stat Memory");
			if (eHERMPrev[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate LAN Stat Prv Memory");
			eHEn[i]->BinaryStatus[0] = 0;	//modification flags
			eHERMPrev[i]->data[0] = 0;
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
		if (eHn[i] == NULL)
		{
			eHn[i] = (struct eHouse1NamesT *) malloc(sizeof(struct eHouse1NamesT));
			if (eHn[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate RS-485 Names Memory");
			eHn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			if (i == 0)
			{
				eHn[i]->AddrH = 1;
				eHn[i]->AddrL = 1;
			}
			else
				if (i == EHOUSE1_RM_MAX)
				{
					eHn[i]->AddrH = 2;
					eHn[i]->AddrL = 1;
				}
				else
				{
					eHn[i]->AddrH = 55;
					eHn[i]->AddrL = i;
				}
			LOG(LOG_STATUS, "Allocating eHouse RS-485 Controller (%d,%d)", eHn[i]->AddrH, eHn[i]->AddrL);
			eHRMs[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			eHRMPrev[i] = (union ERMFullStatT *) malloc(sizeof(union ERMFullStatT));
			if (eHRMs[i] == NULL) LOG(LOG_ERROR, "CANT Allocate RS-485 Stat Memory");
			if (eHRMPrev[i] == NULL) LOG(LOG_ERROR, "CANT Allocate RS-485 Stat Prev Memory");
			eHn[i]->BinaryStatus[0] = 0;		//modification flags
			eHRMPrev[i]->data[0] = 0;
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
		if (eHWIFIn[i] == NULL)
		{
			LOG(LOG_STATUS, "Allocating eHouse WiFi Controller (192.168.%d.%d)", devaddrh, INITIAL_ADDRESS_WIFI + i);
			eHWIFIn[i] = (struct WiFieHouseNamesT *) malloc(sizeof(struct WiFieHouseNamesT));
			if (eHWIFIn[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate WiFi Names Memory");
			eHWIFIn[i]->INITIALIZED = 'a';	//first byte of structure for detection of allocated memory
			eHWIFIn[i]->AddrH = devaddrh;
			eHWIFIn[i]->AddrL = devaddrl;
			eHWiFi[i] = (union WiFiStatusT *) malloc(sizeof(union WiFiStatusT));
			eHWIFIs[i] = (union WIFIFullStatT *) malloc(sizeof(union WIFIFullStatT));
			eHWIFIPrev[i] = (union WIFIFullStatT *) malloc(sizeof(union WIFIFullStatT));
			if (eHWIFIs[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate WiFi Stat Memory");
			if (eHWIFIPrev[i] == NULL) LOG(LOG_ERROR, "CAN'T Allocate WiFi Stat Memory");
			eHWIFIn[i]->BinaryStatus[0] = 0;
			eHWIFIPrev[i]->data[0] = 0;
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
	if (addrl >= 250u) return 1;
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
	unsigned char size = sizeof(AuraN[index]->Outs) / sizeof(AuraN[index]->Outs[0]);
	size = 8;
	if (index > MAX_AURA_DEVS - 1)
	{
		_log.Log(LOG_STATUS, "Aura to much !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;
	}
	size = 4;
	acurr = (float)(AuraDev[index]->TempSet);
	aprev = (float)(AuraDevPrv[index]->TempSet);
	//printf("\r\n!!!!!Aura [%s]  TempSet =%f, Temp=%f\r\n", Auras[index],acurr, AuraDev[index].Temp);
	if ((acurr != aprev) || (AuraDevPrv[index]->Addr == 0))
	{
		if (CHANGED_DEBUG) _log.Log(LOG_STATUS, "Temp Preset #%d changed to: %d", (int)index, (int)acurr);
		sprintf(sval, "%.1f", (AuraDev[index]->TempSet));
		AddrH = 0x81;
		AddrL = AuraDev[index]->Addr;
		//printf("[AURA] %x,%x\r\n", AddrH, AddrL);
		UpdateSQLStatus(AddrH, AddrL, EH_AURA, VISUAL_AURA_PRESET, 1, AuraDev[index]->RSSI, (int)round(AuraDev[index]->TempSet * 10), sval, (int)round(AuraDev[index]->volt));
	}

	//ADCs
	//for (i = 0; i < size; i++)
	{
		acurr = (float)(AuraDev[index]->Temp);
		aprev = (float)(AuraDevPrv[index]->Temp);
		if ((acurr != aprev) || (AuraDevPrv[index]->Addr == 0))
		{
			sprintf(sval, "%.1f", ((float)acurr));
			AddrH = 0x81;
			AddrL = AuraDev[index]->Addr;
			UpdateSQLStatus(AddrH, AddrL, EH_AURA, VISUAL_AURA_IN, 1, AuraDev[index]->RSSI, (int)round(acurr * 10), sval, (int)round(AuraDev[index]->volt));
			if (CHANGED_DEBUG) _log.Log(LOG_STATUS, "Temp #%d changed to: %f", (int)index, acurr);
		}
	}
	memcpy(&AuraDevPrv[index]->Addr, &AuraDev[index]->Addr, sizeof(struct AURAT));
}
/////////////////////////////////////////////////////////////////////////////////////
// eHouse LAN (CommManager status update)
void eHouseTCP::UpdateCMToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	if (AddrL < 250) return;  //only CM/LM in this routine
	unsigned char size = sizeof(ECMn->Outs) / sizeof(ECMn->Outs[0]);

	for (i = 0; i < size; i++)
	{
		curr = (ECM->CMB.outs[i / 8] >> (i % 8)) & 0x1;
		prev = (ECMPrv->CMB.outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (ECMPrv->data[1] == 0))
		{
			if (CHANGED_DEBUG) if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[CM 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_LAN, 0x21, i, 100, curr, "", 100);
		}
	}
	//inputs
	size = 48;

	{
		//deb((char*) & "Inputs", ECM.CMB.inputs, sizeof(ECM.CMB.inputs));
		for (i = 0; i < size; i++)
		{
			curr = (ECM->CMB.inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->CMB.inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				if (CHANGED_DEBUG) if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}
		if (ECM->CMB.CURRENT_ADC_PROGRAM != ECMPrv->CMB.CURRENT_ADC_PROGRAM)
		{
			curr = ECM->CMB.CURRENT_ADC_PROGRAM;
			if (CHANGED_DEBUG)
				if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		/* if (ECM.CMB.CURRENT_PROFILE!=ECMPrv.CMB.CURRENT_PROFILE)
			{
			curr = eCM.CMB.
					.CURRENT_PROFILE;
			if (CHANGED_DEBUG) if (ECMPrv.data[0]) printf("[LAN 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}*/
		if (ECM->CMB.CURRENT_PROGRAM != ECMPrv->CMB.CURRENT_PROGRAM)
		{
			curr = ECM->CMB.CURRENT_PROGRAM;
			if (CHANGED_DEBUG)
				if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//deb((char*) & "Dimm", ECM.CMB.DIMM, sizeof(ECM.CMB.DIMM));
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (ECM->CMB.DIMM[i]);
			prev = (ECMPrv->CMB.DIMM[i]);
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				if (CHANGED_DEBUG)
					if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_DIMMER_OUT, i, 100, (curr * 100) / 255, "", 100);
			}
		}

		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (ECM->CMB.ADC[i].MSB);
			aprev = (ECMPrv->CMB.ADC[i].MSB);

			acurr = acurr << 8;
			aprev = aprev << 8;

			acurr += (ECM->CMB.ADC[i].LSB);
			aprev += (ECMPrv->CMB.ADC[i].LSB);

			if ((acurr != aprev) || (ECMPrv->data[1] == 0))
			{
				//if (eHERMPrev[index].data[0]) printf("[LAN 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ADC_IN, i, 100, acurr, sval, 100);
			}
		}

	}


	{

		if (ECM->CMB.CURRENT_ZONE != ECMPrv->CMB.CURRENT_ZONE)
		{
			curr = ECM->CMB.CURRENT_ZONE;
			if (CHANGED_DEBUG)
				if (ECMPrv->data[1])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ZONE #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		size = 48;
		//Outputs signals
	//    deb((char*) & "I2C out", (unsigned char *) &ECM.data[STATUS_OUT_I2C], sizeof(20));
		for (i = 0; i < size; i++)
		{
			curr = (ECM->data[STATUS_OUT_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->data[STATUS_OUT_I2C + i / 8] >> (i % 8)) & 0x1;

			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				if (CHANGED_DEBUG)
					if (ECMPrv->data[1])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Outs #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			}
		}

		size = 48;
		//Inputs signals
 //    deb((char*) & "I2C inputs", (unsigned char *) &ECM.data[STATUS_INPUTS_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (ECM->data[STATUS_INPUTS_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->data[STATUS_INPUTS_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				if (CHANGED_DEBUG)
					if (ECMPrv->data[1])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100,curr, "", 100);
			}
		}

		//alarms signals
		//deb((char*) & "I2C alarm", (unsigned char *) &ECM.data[STATUS_ALARM_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (ECM->data[STATUS_ALARM_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->data[STATUS_ALARM_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1] ) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Alarm #%d changed to: %d", (int) AddrH, (int) AddrL, (int)i, (int) curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ALARM_IN, i, 100,curr, "", 100);
			}
		}
		//deb((char*) & "I2C warning", (unsigned char *) &ECM.data[STATUS_WARNING_I2C],8);
		//Warning signals
		for (i = 0; i < size; i++)
		{
			curr = (ECM->data[STATUS_WARNING_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->data[STATUS_WARNING_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Warning #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				//UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_WARNING_IN, i, 100,curr, "", 100);
			}
		}
		//Monitoring signals
		///deb((char*) & "I2C monit", (unsigned char *) &ECM.data[STATUS_MONITORING_I2C],8);
		for (i = 0; i < size; i++)
		{
			curr = (ECM->data[STATUS_MONITORING_I2C + i / 8] >> (i % 8)) & 0x1;
			prev = (ECMPrv->data[STATUS_MONITORING_I2C + i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (ECMPrv->data[1] == 0))
			{
				//if (ECMPrv->data[1]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Monitoring #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				//            UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MONITORING_IN, i, 100,curr, "", 100);
			}
		}
		memcpy(ECMPrv, ECM, sizeof(union CMStatusT));
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
	unsigned char size = sizeof(eHEn[index]->Outs) / sizeof(eHEn[index]->Outs[0]); //for cm only
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
		curr = (eHERMs[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (eHERMPrev[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (eHERMPrev[index]->data[0] == 0))
		{
			if (CHANGED_DEBUG)
				if (eHERMPrev[index]->data[0]) _log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_LAN, 0x21, i, 100, curr, "", 100);
		}
	}
	//inputs
	size = 24;
	{
		for (i = 0; i < size; i++)
		{
			curr = (eHERMs[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (eHERMPrev[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}

		if (eHERMs[index]->eHERM.CURRENT_ADC_PROGRAM != eHERMPrev[index]->eHERM.CURRENT_ADC_PROGRAM)
		{
			curr = eHERMs[index]->eHERM.CURRENT_ADC_PROGRAM;
			if (CHANGED_DEBUG)
				if (eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		if (eHERMs[index]->eHERM.CURRENT_PROFILE != eHERMPrev[index]->eHERM.CURRENT_PROFILE)
		{
			curr = eHERMs[index]->eHERM.CURRENT_PROFILE;
			if (CHANGED_DEBUG)
				if (eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Profile #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		if (eHERMs[index]->eHERM.CURRENT_PROGRAM != eHERMPrev[index]->eHERM.CURRENT_PROGRAM)
		{
			curr = eHERMs[index]->eHERM.CURRENT_PROGRAM;
			if (CHANGED_DEBUG)
				if (eHERMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (eHERMs[index]->eHERM.Dimmers[i]);
			prev = (eHERMPrev[index]->eHERM.Dimmers[i]);
			if ((curr != prev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_DIMMER_OUT, i, 100, (curr * 100) / 255, "", 100);
			}
		}

		size = 16;
		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (eHERMs[index]->eHERM.ADCH[i]);
			aprev = (eHERMPrev[index]->eHERM.ADCH[i]);
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] ADC H Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}
		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (eHERMs[index]->eHERM.ADCL[i]);
			aprev = (eHERMPrev[index]->eHERM.ADCL[i]);
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] ADC Low Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (eHERMs[index]->eHERM.ADC[i]);
			aprev = (eHERMPrev[index]->eHERM.ADC[i]);
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_ADC_IN, i, 100, acurr, sval, 100);
			}
		}

		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (eHERMs[index]->eHERM.TempL[i]);
			aprev = (eHERMPrev[index]->eHERM.TempL[i]);
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] TEMP Low Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
			}
		}

		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (eHERMs[index]->eHERM.TempH[i]);
			aprev = (eHERMPrev[index]->eHERM.TempH[i]);
			int midpoint = eHERMs[index]->eHERM.TempH[i] - eHERMs[index]->eHERM.TempL[i];
			midpoint /= 2;
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHERMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[LAN 192.168.%d.%d] Temp H Preset #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)acurr);
				sprintf(sval, "%.1f", ((float)(eHERMs[index]->eHERM.TempH[i] + eHERMs[index]->eHERM.TempL[i])) / 20);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MCP9700_PRESET, i, 100, midpoint, sval, 100);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (eHERMs[index]->eHERM.Temp[i]);
			aprev = (eHERMPrev[index]->eHERM.Temp[i]);
			if ((acurr != aprev) || (eHERMPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%.1f", ((float)acurr) / 10);
				UpdateSQLStatus(AddrH, AddrL, EH_LAN, VISUAL_MCP9700_IN, i, 100, acurr, sval, 100);
			}
		}
	}
	memcpy(eHERMPrev[index], eHERMs[index], sizeof(union ERMFullStatT));
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// eHouse PRO/BMS - centralized controller + integrations
/////////////////////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::UpdatePROToSQL(unsigned char AddrH, unsigned char AddrL)
{
	unsigned char curr, prev;
	int i = 0;
	int size = sizeof(eHouseProStatus->status.Outputs) / sizeof(eHouseProStatus->status.Outputs[0]);
	size = MAX_OUTPUTS / PRO_HALF_IO;
	//deb("PRO: ", EHouseProStatus.data, sizeof(eHouseProStatus.data));
	for (i = 0u; i < size; i++)
	{
		curr = (eHouseProStatus->status.Outputs[i / 8] >> (i % 8)) & 0x1;
		prev = (eHouseProStatusPrv->status.Outputs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
		{
			if (CHANGED_DEBUG)
				if (eHouseProStatusPrv->data[0])
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
			curr = (eHouseProStatus->status.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//        printf("[LAN 192.168.%d.%d] Input #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				UpdateSQLStatus(AddrH, AddrL, EH_PRO, VISUAL_INPUT_IN, i, 100, curr, "", 100);
			}
		}

		if (eHouseProStatus->status.AdcProgramNr != eHouseProStatusPrv->status.AdcProgramNr)
		{
			curr = eHouseProStatus->status.AdcProgramNr;
			//printf("[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (eHouseProStatus->status.ProgramNr != eHouseProStatusPrv->status.ProgramNr)
		{
			curr = eHouseProStatus->status.ProgramNr;
			//printf("[LAN 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (eHouseProStatus->status.RollerProgram != eHouseProStatusPrv->status.RollerProgram)
		{
			curr = eHouseProStatus->status.RollerProgram;
			//printf("[LAN 192.168.%d.%d] Current Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		if (eHouseProStatus->status.SecuZone != eHouseProStatusPrv->status.SecuZone)
		{
			curr = eHouseProStatus->status.SecuZone;
			//printf("[LAN 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
	}


	{

		//alarms signals
		for (i = 0; i < size; i++)
		{
			curr = (eHouseProStatus->status.Alarm[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.Alarm[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Alarm #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int)i, (int) curr);
			}
		}
		//Warning signals
		for (i = 0; i < size; i++)
		{

			curr = (eHouseProStatus->status.Warning[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.Warning[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Warning #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Monitoring signals
		for (i = 0; i < size; i++)
		{
			curr = (eHouseProStatus->status.Monitoring[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.Monitoring[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Monitoring #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Silent signals
		for (i = 0; i < size; i++)
		{
			curr = (eHouseProStatus->status.Silent[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.Silent[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO 192.168.%d.%d] Silent #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}
		//Early Warning signals
		for (i = 0; i < size; i++)
		{
			curr = (eHouseProStatus->status.EarlyWarning[i / 8] >> (i % 8)) & 0x1;
			prev = (eHouseProStatusPrv->status.EarlyWarning[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHouseProStatusPrv->data[0] == 0))
			{
				//            if (eHouseProStatusPrv.data[0]) printf("[PRO.168.%d.%d] Early Warning #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}


	}
	memcpy(eHouseProStatusPrv, eHouseProStatus, sizeof(union eHouseProStatusUT));

}
/////////////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////////////
// Update eHouse WiFi controllers status to DB
void eHouseTCP::UpdateWiFiToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	unsigned char size = sizeof(eHWIFIn[index]->Outs) / sizeof(eHWIFIn[index]->Outs[0]); //for cm only
	size = 8;


	if (index > EHOUSE_WIFI_MAX - 1)
	{
		_log.Log(LOG_STATUS, " WIFI INDEX !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to much");
		return;
	}
	//deb("WIFI: ", EHWIFIs[index].data, sizeof(eHWIFIs[index].data));
	for (i = 0; i < size; i++)
	{
		curr = (eHWIFIs[index]->eHWIFI.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (eHWIFIPrev[index]->eHWIFI.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (eHWIFIPrev[index]->data[0] == 0))
		{
			if (CHANGED_DEBUG) if (eHWIFIPrev[index]->data[0]) _log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
			UpdateSQLStatus(AddrH, AddrL, EH_WIFI, 0x21, i + 1, 100, curr, "", 100);
		}
	}
	//inputs
	size = 8;
	{
		for (i = 0; i < size; i++)
		{
			curr = (eHWIFIs[index]->eHWIFI.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (eHWIFIPrev[index]->eHWIFI.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHWIFIPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_INPUT_IN, i + 1, 100, curr, "", 100);
			}
		}
		if (eHWIFIs[index]->eHWIFI.CURRENT_ADC_PROGRAM != eHWIFIPrev[index]->eHWIFI.CURRENT_ADC_PROGRAM)
		{
			curr = eHWIFIs[index]->eHWIFI.CURRENT_ADC_PROGRAM;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current ADC Program #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
		if (eHWIFIs[index]->eHWIFI.CURRENT_PROFILE != eHWIFIPrev[index]->eHWIFI.CURRENT_PROFILE)
		{
			curr = eHWIFIs[index]->eHWIFI.CURRENT_PROFILE;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current Profile #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}
		if (eHWIFIs[index]->eHWIFI.CURRENT_PROGRAM != eHWIFIPrev[index]->eHWIFI.CURRENT_PROGRAM)
		{
			curr = eHWIFIs[index]->eHWIFI.CURRENT_PROGRAM;
			//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Current Proram #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
		}

		size = 4;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (eHWIFIs[index]->eHWIFI.Dimmers[i]);
			prev = (eHWIFIPrev[index]->eHWIFI.Dimmers[i]);
			if ((curr != prev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHWIFIPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[WIFI 192.168.%d.%d] Dimmer #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_DIMMER_OUT, i + 1, 100, (curr), "", 100);
			}
		}
		size = 4;
		//ADCs Preset H
		//printf(" ADC:");
		for (i = 0; i < size; i++)
		{
			acurr = (eHWIFIs[index]->eHWIFI.ADCH[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.ADCH[i]);
			if ((curr != prev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC H Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}
		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (eHWIFIs[index]->eHWIFI.ADCL[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.ADCL[i]);
			if ((acurr != aprev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC Low Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{
			acurr = (eHWIFIs[index]->eHWIFI.ADC[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.ADC[i]);
			if ((acurr != aprev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[index].data[0]) printf("[WIFI 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_ADC_IN, i + 1, 100, acurr, sval, 100);
			}
		}

		//ADCs L Preset
		for (i = 0; i < size; i++)
		{
			acurr = (eHWIFIs[index]->eHWIFI.TempL[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.TempL[i]);
			if ((acurr != aprev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] TEMP Low Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
			}
		}

		//ADCs Preset H
		for (i = 0; i < size; i++)
		{
			acurr = (eHWIFIs[index]->eHWIFI.TempH[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.TempH[i]);
			int midpoint = eHWIFIs[index]->eHWIFI.TempH[i] - eHWIFIs[index]->eHWIFI.TempL[i];
			midpoint /= 2;
			if ((acurr != aprev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Temp H Preset #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int)acurr);
				sprintf(sval, "%.1f", ((float)(eHWIFIs[index]->eHWIFI.TempH[i] + eHWIFIs[index]->eHWIFI.TempL[i])) / 20);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_MCP9700_PRESET, i + 1, 100, midpoint, sval, 100);
			}
		}
		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (eHWIFIs[index]->eHWIFI.Temp[i]);
			aprev = (eHWIFIPrev[index]->eHWIFI.Temp[i]);
			if ((acurr != aprev) || (eHWIFIPrev[index]->data[0] == 0))
			{
				sprintf(sval, "%.1f", ((float)acurr) / 10);
				UpdateSQLStatus(AddrH, AddrL, EH_WIFI, VISUAL_MCP9700_IN, i + 1, 100, acurr, sval, 100);
				//if (eHWIFIPrev[[index].data[0]) printf("[WIFI 192.168.%d.%d] Temp #%d changed to: %d", (int) AddrH, (int) AddrL, (int) i, (int) curr);
			}
		}


	}
	memcpy(eHWIFIPrev[index], eHWIFIs[index], sizeof(union WIFIFullStatT));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Update eHouse 1 controller status (HeatManager, RoomManagers, ExternalManager) to DB
void eHouseTCP::UpdateRS485ToSQL(unsigned char AddrH, unsigned char AddrL, unsigned char index)
{
	unsigned char i, curr, prev;
	char sval[10];
	int acurr, aprev;
	unsigned char size = sizeof(eHn[index]->Outs) / sizeof(eHn[index]->Outs[0]); //for cm only
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
		curr = (eHRMs[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		prev = (eHRMPrev[index]->eHERM.Outs[i / 8] >> (i % 8)) & 0x1;
		if ((curr != prev) || (eHRMPrev[index]->data[0] == 0))
		{
			if (CHANGED_DEBUG) if (eHRMPrev[index]->data[0]) printf("[RS485 (%d,%d)] Out #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
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
			curr = (eHRMs[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			prev = (eHRMPrev[index]->eHERM.Inputs[i / 8] >> (i % 8)) & 0x1;
			if ((curr != prev) || (eHRMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHRMPrev[index]->data[0])
						_log.Log(LOG_STATUS, "[RS485 (%d,%d)] Input #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
				UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_INPUT_IN, i + 1, 100, curr, "", 100);
			}
		}
		if (eHRMs[index]->eHERM.CURRENT_PROFILE != eHRMPrev[index]->eHERM.CURRENT_PROFILE)
		{
			curr = eHRMs[index]->eHERM.CURRENT_PROFILE;
			if (CHANGED_DEBUG)
				if (eHRMPrev[index]->data[0])
					_log.Log(LOG_STATUS, "[RS485 (%d,%d)] Current Profile #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}
		if (eHRMs[index]->eHERM.CURRENT_PROGRAM != eHRMPrev[index]->eHERM.CURRENT_PROGRAM)
		{
			curr = eHRMs[index]->eHERM.CURRENT_PROGRAM;
			if (CHANGED_DEBUG) if (eHRMPrev[index]->data[0]) _log.Log(LOG_STATUS, "[RS485 (%d,%d) Current Program #%d changed to: %d", (int)AddrH, (int)AddrL, (int)i, (int)curr);
		}

		size = 3;
		//PWM Dimmers
		for (i = 0; i < size; i++)
		{
			curr = (eHRMs[index]->eHERM.Dimmers[i]);
			prev = (eHRMPrev[index]->eHERM.Dimmers[i]);
			if ((curr != prev) || (eHRMPrev[index]->data[0] == 0))
			{
				if (CHANGED_DEBUG)
					if (eHRMPrev[index]->data[0])
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
			acurr = (eHRMs[index]->eHERM.ADC[i]);
			aprev = (eHRMPrev[index]->eHERM.ADC[i]);
			if ((acurr != aprev) || (eHRMPrev[index]->data[0] == 0))
			{
				//printf("[LAN 192.168.%d.%d] ADC #%d changed to: %d\r\n", (int) AddrH, (int) AddrL, (int) i, (int) curr);
				sprintf(sval, "%d", (acurr));
				UpdateSQLStatus(AddrH, AddrL, EH_RS485, VISUAL_ADC_IN, i + 1, 100, acurr, sval, 100);
			}
		}

		//ADCs
		for (i = 0; i < size; i++)
		{

			acurr = (eHRMs[index]->eHERM.Temp[i]);
			aprev = (eHRMPrev[index]->eHERM.Temp[i]);
			if ((acurr != aprev) || (eHRMPrev[index]->data[0] == 0))
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
		memcpy(eHRMPrev[index], eHRMs[index], sizeof(union ERMFullStatT)); //eHRMs[index]
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
void eHouseTCP::TerminateUDP(void)
{
	_log.Log(LOG_STATUS, "Terminate UDP");
	char opt = 0;

	//struct timeval timeout;
	UDP_terminate_listener = 1;
	m_stoprequested = true;
	if (ViaTCP)
	{
		unsigned long iMode = 1;
#ifdef WIN32
		int status = ioctlsocket(TCPSocket, FIONBIO, &iMode);
#else
		int status = ioctl(TCPSocket, FIONBIO, &iMode);
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

	val = ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (3 << 1)])) & 0x3;		//VREFF
	val = val << 8;
	val += ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (3 << 1) + 1]));


	//val+=eHERMs[index].eHERM.ADC[3].LSB;
	float vae = (float)val;
	float temp;
	VccRef = (float)((2500.0 * 1023.0) / vae);
	VccRef /= 10;
	temp = (float)(3300.0 / 2500.0);
	temp *= vae;
	AdcRefMax = (int)temp;
	CalcCalibration = (float)(1 / ((val * (3300.0 / 2500.0)) / 1024));
	return VccRef;
}
///////////////////////////////////////////////////////////////////////////////
void eHouseTCP::CalculateAdc2(char index)
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
		ivalue = ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)])) & 0x3;
		ivalue = ivalue << 8;
		ivalue += ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1) + 1]));
		adcvalue = ivalue;
		Offset = 0;//eHEn[index].Offset[field];
		OffsetV = 0;//eHEn[index].OffsetV[field];
		Calibration = CalcCalibration;//eHEn[index].Calibration[field];
		CalibrationV = 1;//CalcCalibration;//eHEn[index].CalibrationV[field];
		Vcc = 3300;//eHEn[index].Vcc[field];

		adch = ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 6) & 0x3;
		adcl = ((eHEn[index]->BinaryStatus[STATUS_ADC_ETH + (field << 1)]) >> 4) & 0x3;
		adch = adch << 8;
		adcl = adcl << 8;
		adcl |= eHEn[index]->BinaryStatus[STATUS_ADC_LEVELS + (field << 1)] & 0xff;
		adch |= eHEn[index]->BinaryStatus[STATUS_ADC_LEVELS + (field << 1) + 1] & 0xff;
		eHERMs[index]->eHERM.ADCH[field] = adch;
		eHERMs[index]->eHERM.ADCL[field] = adcl;
		eHERMs[index]->eHERM.ADC[field] = ivalue;

		//    Mod =eHEn[index].Mod[field];
			//temp=-273.16+(temp*5000)/(1023*10);
		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		eHEn[index]->AdcValue[field] = temmp;

		temp = -273.16 + Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHEn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHEn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHEn[index]->SensorTempsMCP9700[field] = temp / 10;
		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHEn[index]->SensorPercents[field] = temp / 10;
		eHEn[index]->SensorInvPercents[field] = 100 - (temp / 10);
		if (field != 9)
		{
			temp = -50.0 + Offset + ((adch * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHERMs[index]->eHERM.TempH[field] = (int)round(temp);

			temp = -50.0 + Offset + ((adcl*Vcc) / (10.0*1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHERMs[index]->eHERM.TempL[field] = (int)round(temp);
			eHERMs[index]->eHERM.Temp[field] = (int)round(eHEn[index]->SensorTempsMCP9700[field] * 10);
			eHERMs[index]->eHERM.Unit[field] = 'C';
		}
		else {
			temp = (adch * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHERMs[index]->eHERM.TempH[field] = (int)round((1000 - (temp)));

			temp = (adcl * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHERMs[index]->eHERM.TempL[field] = (int)round((1000 - (temp)));

			eHERMs[index]->eHERM.Temp[field] = (int)round(eHEn[index]->SensorInvPercents[field] * 10);
			eHERMs[index]->eHERM.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHEn[index]->SensorTempsMCP9701[field] = temp / 10;


		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		eHEn[index]->SensorVolts[field] = temp / 100;
	}
}
///////////////////////////////////////////////////////////////////////////////
void eHouseTCP::CalculateAdcWiFi(char index)
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
		eHWIFIs[index]->eHWIFI.ADC[field] = eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI] & 0x3;
		eHWIFIs[index]->eHWIFI.ADC[field] = eHWIFIs[index]->eHWIFI.ADC[field] << 8;
		eHWIFIs[index]->eHWIFI.ADC[field] += eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + 1];
		ivalue = eHWIFIs[index]->eHWIFI.ADC[field];
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
		Calibration = CalcCalibration;//eHWIFIn[index].Calibration[field];
		CalibrationV = 1;//CalcCalibration;//eHWIFIn[index].CalibrationV[field];
		Vcc = 1000;//eHWIFIn[index].Vcc[field];

		adch = ((eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + (field << 1)]) >> 6) & 0x3;
		adcl = ((eHWIFIn[index]->BinaryStatus[ADC_OFFSET_WIFI + (field << 1)]) >> 4) & 0x3;
		adch = adch << 8;
		adcl = adcl << 8;
		adcl |= eHWIFIn[index]->BinaryStatus[STATUS_ADC_LEVELS_WIFI + (field << 1)] & 0xff;
		adch |= eHWIFIn[index]->BinaryStatus[STATUS_ADC_LEVELS_WIFI + (field << 1) + 1] & 0xff;
		eHWIFIs[index]->eHWIFI.ADCH[field] = adch;
		eHWIFIs[index]->eHWIFI.ADCL[field] = adcl;
		eHWIFIs[index]->eHWIFI.ADC[field] = ivalue;

		//    Mod =eHWIFIn[index].Mod[field];
			//temp=-273.16+(temp*5000)/(1023*10);
		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		eHWIFIn[index]->AdcValue[field] = temmp;

		temp = -273.16 + Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHWIFIn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHWIFIn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHWIFIn[index]->SensorTempsMCP9700[field] = temp / 10;
		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHWIFIn[index]->SensorPercents[field] = temp / 10;
		eHWIFIn[index]->SensorInvPercents[field] = 100 - (temp / 10);
		if (field == 0)
		{
			temp = -50.0 + Offset + ((adch * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHWIFIs[index]->eHWIFI.TempH[field] = (int)round(temp);

			temp = -50.0 + Offset + ((adcl * Vcc) / (10.0 * 1023.0)) * 1;//Calibration;  //mcp9700 10mv/c offset -50
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHWIFIs[index]->eHWIFI.TempL[field] = (int)round(temp);
			eHWIFIs[index]->eHWIFI.Temp[field] = (int)round(eHWIFIn[index]->SensorTempsMCP9700[field] * 10);
			eHWIFIs[index]->eHWIFI.Unit[field] = 'C';
		}
		else {
			temp = (adch * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHWIFIs[index]->eHWIFI.TempH[field] = (int)round((1000 - (temp)));

			temp = (adcl * 100) / 1023;
			temmp = (int)round(temp * 10);
			temp = temmp;
			eHWIFIs[index]->eHWIFI.TempL[field] = (int)round((1000 - (temp)));

			eHWIFIs[index]->eHWIFI.Temp[field] = (int)round(eHWIFIn[index]->SensorInvPercents[field] * 10);
			eHWIFIs[index]->eHWIFI.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHWIFIn[index]->SensorTempsMCP9701[field] = temp / 10;
		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		eHWIFIn[index]->SensorVolts[field] = temp / 100;
	}
}
///////////////////////////////////////////////////////////////////////////////

void eHouseTCP::CalculateAdcEH1(char index)
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
		ivalue = ((eHn[index]->BinaryStatus[RM_STATUS_ADC + (field << 1)])) & 0x3;
		ivalue = ivalue << 8;
		ivalue += ((eHn[index]->BinaryStatus[RM_STATUS_ADC + (field << 1) + 1]));
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
		eHRMs[index]->eHERM.ADCH[field] = 1023;
		eHRMs[index]->eHERM.ADCL[field] = 0;
		eHRMs[index]->eHERM.ADC[field] = ivalue;

		adcvalue *= CalibrationV;
		adcvalue += OffsetV;
		temmp = (int)round(adcvalue);
		eHn[index]->AdcValue[field] = temmp;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) - 273.16;  //LM335 10mv/1c offset -273.16
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHn[index]->SensorTempsLM335[field] = temp / 10;

		temp = Offset + ((adcvalue * Vcc) / (10 * 1023)) * Calibration;          //LM35 10mv/1c offset 0
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHn[index]->SensorTempsLM35[field] = temp / 10;

		temp = -50.0 + Offset + ((adcvalue * Vcc) / (10.0 * 1023.0)) * Calibration;  //mcp9700 10mv/c offset -50
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHn[index]->SensorTempsMCP9700[field] = temp / 10;

		temp = (adcvalue * 100) / 1023;
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHn[index]->SensorPercents[field] = temp / 10;
		eHn[index]->SensorInvPercents[field] = 100 - (temp / 10);

		if (field != 0)
		{
			eHRMs[index]->eHERM.TempH[field] = 1500;
			eHRMs[index]->eHERM.TempL[field] = 0;
			eHRMs[index]->eHERM.Temp[field] = (int)round(eHn[index]->SensorTempsLM335[field] * 10);
			eHRMs[index]->eHERM.Unit[field] = 'C';
		}
		else
		{
			eHRMs[index]->eHERM.TempH[field] = 1000;//(1000 - (temp));
			eHRMs[index]->eHERM.TempL[field] = 0;//(1000 - (temp));

			eHRMs[index]->eHERM.Temp[field] = (int)round(eHn[index]->SensorInvPercents[field] * 10);
			eHRMs[index]->eHERM.Unit[field] = '%';
		}



		temp = -20.513 + Offset + ((adcvalue * Vcc) / (19.5 * 1023)) * Calibration;  //mcp9701 19.5mv/c offset -20.513
		temmp = (int)round(temp * 10);
		temp = temmp;
		eHn[index]->SensorTempsMCP9701[field] = temp / 10;
		temp = Calibration * (adcvalue * Vcc) / 1023;
		temmp = (int)round(temp / 10);
		temp = temmp;
		eHn[index]->SensorVolts[field] = temp / 100;
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
	memset(GetLine, 0, sizeof(GetLine));
	while ((GetNamesDta[GetIndex] != 13) && (GetIndex < GetSize))
	{
		if (strlen(GetLine) < sizeof(GetLine))
			strncat(GetLine, (char *)&GetNamesDta[GetIndex], 1);
		GetIndex++;

	}
	if (GetIndex < GetSize) GetIndex++; //"\r"
	if (GetIndex < GetSize) GetIndex++; //"\n"
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
	int i;
	if (data[3] != 'n') return;
	if (data[4] != '0') return;
	unsigned char nr;
	if (data[1] == 1) nr = 0;						//for HM
	else if (data[1] == 2) nr = STATUS_ARRAYS_SIZE; //for EM
	else nr = data[2] % STATUS_ARRAYS_SIZE;			//addrl for RM

	GetIndex = 7;             //Ignore binary control fields
	GetSize = nbytes;         //size of whole packet
	GetStr(data);			//addr combined
	GetStr(data);			//addr h
	GetStr(data);			//addr l
	GetStr(data);			//name
	i = 1;
	eHaloc(nr, data[1], data[2]);
	PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_RS485, (char *)&GetLine);//for Automatic RoomPlan generation for RoomManager
	if (PlanID < 0) PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_RS485, (char *)&GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&eHn[nr]->Name, (char *)&GetLine, sizeof(eHn[nr]->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(eHn[nr]->ADCs) / sizeof(eHn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&eHn[nr]->ADCs[i], (char *)&GetLine, sizeof(eHn[nr]->ADCs[i]));
		if (nr > 0)
		{
			if (i > 0) UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_LM335_IN, i + 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
			else UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_INVERTED_PERCENT_IN, i + 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
			//        UpdateSQLState(data[1], data[2], EH_RS485, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_LM335_PRESET, i, 1, 0, "20.5", Name, (char *) &GetLine, true, 100);
		}
		else UpdateSQLState(data[1], data[2], EH_RS485, pTypeTEMP, sTypeTEMP5, 0, VISUAL_LM335_IN, i + 1, 0, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	/*    GetStr(data);// #ADC CFG; Not available for eHouse 1
		for (i = 0; i < sizeof(eHn[nr].ADCs) / sizeof(eHn[nr].ADCs[0]); i++)          //ADC Config (sensor type) not used currently
			{
			GetStr(data);
			eHn[nr].ADCConfig[i] =GetLine[0]-'0';
			}
	  */
	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(eHn[nr]->Outs) / sizeof(eHn[nr]->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&eHn[nr]->Outs[i], (char *)&GetLine, sizeof(eHn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i + 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);
	for (i = 0; i < sizeof(eHn[nr]->Inputs) / sizeof(eHn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&eHn[nr]->Inputs[i], (char *)&GetLine, sizeof(eHn[nr]->Inputs[i]));
		if (nr != 0) UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchGeneralSwitch,    //not HM
			STYPE_Contact, VISUAL_INPUT_IN, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(eHn[nr]->Dimmers) / sizeof(eHn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&eHn[nr]->Dimmers[i], (char *)&GetLine, sizeof(eHn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_RS485, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}
	UpdateSQLState(data[1], data[2], EH_RS485, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 1, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);	//rollers names

	for (i = 0; i < sizeof(eHn[nr]->Outs) / sizeof(eHn[nr]->Outs[0]); i += 2)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		if (nr == STATUS_ARRAYS_SIZE) UpdateSQLState(data[1], data[2], EH_RS485, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(eHn[nr]->Programs) / sizeof(eHn[nr]->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&eHn[nr]->Programs[i], (char *)&GetLine, sizeof(eHn[nr]->Programs[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	k = UpdateSQLState(data[1], data[2], EH_RS485, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	/*    strcpy(PGMs,"SelectorStyle:1;LevelNames:"); //Add Requlation Program Selector
		GetStr(data);// "#ADC Programs Names
		for (i = 0; i < sizeof(eHn[nr].ADCPrograms) / sizeof(eHn[nr].ADCPrograms[0]); i++)
			{
			GetStr(data);
			printf("%s\r\n", (char *) &GetLine);
			strncpy((char *) &eHn[nr].ADCPrograms[i], (char *) &GetLine, sizeof(eHn[nr].ADCPrograms[i]));
			if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == NULL))
					{
					k++;
					sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
					if (k<= 10) strncat(PGMs, tmp, strlen(tmp));
					}
			}
		PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
		k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
		k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
		UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs,k);
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
	eHRMPrev[nr]->data[0] = 0;
	eHn[nr]->BinaryStatus[0] = 0;
	//memset((void *) &eHRMPrev[nr], 0, sizeof(union ERMFullStatT));//eHRMPrev[nr]));
	//memset(eHn[nr]->BinaryStatus, 0, sizeof(eHn[nr]->BinaryStatus));
}
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
int PlanID = 0;
void eHouseTCP::GetUDPNamesLAN(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i;
	char Name[80];
	char tmp[96];
	char PGMs[500u];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '1') return;   //other controller type than ERM
	unsigned char nr = (data[2] - INITIAL_ADDRESS_LAN) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	GetIndex = 7;             //Ignore binary control fields
	GetSize = nbytes;         //size of whole packet
	GetStr(data);           //addr combined
	GetStr(data);			//addr h
	GetStr(data);			//addr l
	GetStr(data);			//name
	i = 1;
	eHEaloc(nr, data[1], data[2]);
	PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&GetLine);//for Automatic RoomPlan generation for RoomManager
	if (PlanID < 0) PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&GetLine); //Add RoomPlan for RoomManager (RM)


	strncpy((char *)&eHEn[nr]->Name, (char *)&GetLine, sizeof(eHEn[nr]->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&GetLine, sizeof(Name));
	GetStr(data);			//comment

	for (i = 0; i < sizeof(eHEn[nr]->ADCs) / sizeof(eHEn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&eHEn[nr]->ADCs[i], (char *)&GetLine, sizeof(eHEn[nr]->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i, 1, 0, "0", Name, (char *)&GetLine, true, 100);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i, 1, 0, "20.5", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);// #ADC CFG;
	for (i = 0; i < sizeof(eHEn[nr]->ADCs) / sizeof(eHEn[nr]->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		eHEn[nr]->ADCConfig[i] = GetLine[0] - '0';
	}

	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(eHEn[nr]->Outs) / sizeof(eHEn[nr]->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&eHEn[nr]->Outs[i], (char *)&GetLine, sizeof(eHEn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);
	for (i = 0; i < sizeof(eHEn[nr]->Inputs) / sizeof(eHEn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&eHEn[nr]->Inputs[i], (char *)&GetLine, sizeof(eHEn[nr]->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}


	GetStr(data);		//Description
	for (i = 0; i < sizeof(eHEn[nr]->Dimmers) / sizeof(eHEn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&eHEn[nr]->Dimmers[i], (char *)&GetLine, sizeof(eHEn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}
	UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);		//rollers names

	for (i = 0; i < sizeof(eHEn[nr]->Rollers) / sizeof(eHEn[nr]->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}

	int k = 0;
	GetStr(data);		// #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(eHEn[nr]->Programs) / sizeof(eHEn[nr]->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&eHEn[nr]->Programs[i], (char *)&GetLine, sizeof(eHEn[nr]->Programs[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	strcpy(PGMs, "SelectorStyle:1;LevelNames:"); //Add Regulation Program Selector
	GetStr(data);// "#ADC Programs Names
	for (i = 0; i < sizeof(eHEn[nr]->ADCPrograms) / sizeof(eHEn[nr]->ADCPrograms[0]); i++)
	{
		GetStr(data);
		//printf("%s\r\n", (char *) &GetLine);
		strncpy((char *)&eHEn[nr]->ADCPrograms[i], (char *)&GetLine, sizeof(eHEn[nr]->ADCPrograms[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
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
	eHERMPrev[nr]->data[0] = 0;
	eHEn[nr]->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////
void eHouseTCP::GetUDPNamesCM(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i;
	char Name[80];
	char tmp[96];
	char PGMs[500u];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '2') return;   //other controller type than ERM
	unsigned char nr = (data[2] - INITIAL_ADDRESS_LAN) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	GetIndex = 7;					//Ignore binary control fields
	GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	eCMaloc(0, data[1], data[2]);
	PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&GetLine);		//for Automatic RoomPlan generation for RoomManager
	if (PlanID < 0) PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_LAN, (char *)&GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&ECMn->Name, (char *)&GetLine, sizeof(ECMn->Name)); //RM Controller Name
	strncpy((char *)&Name, (char *)&GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(ECMn->ADCs) / sizeof(ECMn->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&ECMn->ADCs[i], (char *)&GetLine, sizeof(ECMn->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i, 1, 0, "0", Name, (char *)&GetLine, true, 100);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i, 1, 0, "20.5", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);		// #ADC CFG;
	for (i = 0; i < sizeof(ECMn->ADCs) / sizeof(ECMn->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		ECMn->ADCConfig[i] = GetLine[0] - '0';
	}

	GetStr(data);		// #Outs;
	for (i = 0; i < sizeof(ECMn->Outs) / sizeof(ECMn->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&ECMn->Outs[i], (char *)&GetLine, sizeof(ECMn->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);
	for (i = 0; i < sizeof(ECMn->Inputs) / sizeof(ECMn->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&ECMn->Inputs[i], (char *)&GetLine, sizeof(ECMn->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(ECMn->Dimmers) / sizeof(ECMn->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&ECMn->Dimmers[i], (char *)&GetLine, sizeof(ECMn->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}
	UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);//rolers names

	for (i = 0; i < sizeof(ECMn->Rollers) / sizeof(ECMn->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_LAN, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(ECMn->Programs) / sizeof(ECMn->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&ECMn->Programs[i], (char *)&GetLine, sizeof(ECMn->Programs[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	//ISO2UTF8(PGMs);
	UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	k = 0;
	strcpy(PGMs, "SelectorStyle:1;LevelNames:"); //Add Regulation Program Selector
	GetStr(data);// "#ADC Programs Names
	for (i = 0; i < sizeof(ECMn->ADCPrograms) / sizeof(ECMn->ADCPrograms[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&ECMn->ADCPrograms[i], (char *)&GetLine, sizeof(ECMn->ADCPrograms[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
	k = UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
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
	ECMPrv->data[0] = 0;
	ECMn->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////

void eHouseTCP::GetUDPNamesPRO(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i;
	char Name[80];
	char tmp[96];
	char PGMs[500u];

	memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '3') return;   //other controller type than ERM
	unsigned char nr = 0;			//(data[2]-INITIAL_ADDRESS_LAN)%STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	GetIndex = 7;					//Ignore binary control fields
	GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	//eHPROaloc(0, data[1], data[2]);
	PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_PRO, (char *)&GetLine);//for Automatic RoomPlan generation for RoomManager
	if (PlanID < 0) PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_PRO, (char *)&GetLine); //Add RoomPlan for RoomManager (RM)

	strncpy((char *)&eHouseProN->Name, (char *)&GetLine, sizeof(eHouseProN->Name));	//RM Controller Name
	strncpy((char *)&Name, (char *)&GetLine, sizeof(Name));
	GetStr(data);		//comment

	for (i = 0; i < sizeof(eHouseProN->ADCs) / sizeof(eHouseProN->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->ADCs[i], (char *)&GetLine, sizeof(eHouseProN->ADCs[i]));
		if (i < MAX_AURA_DEVS)
		{
			//eAURAaloc(i, 0x81, i + 1);
			UpdateSQLState(0x81, i + 1, EH_AURA, pTypeTEMP, sTypeTEMP5, 0, VISUAL_AURA_IN, 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
			UpdateSQLState(0x81, i + 1, EH_AURA, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_AURA_PRESET, 1, 1, 0, "20.5", Name, (char *)&GetLine, true, 100);
			if (strlen((char *)&AuraN[i]) > 0)
			{
				AuraDevPrv[i]->Addr = 0;
				AuraN[i]->BinaryStatus[0] = 0;
			}
		}
	}

	GetStr(data);// #ADC CFG;
	for (i = 0; i < sizeof(eHouseProN->ADCs) / sizeof(eHouseProN->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		//eHouseProN.ADCType[i] =GetLine[0]-'0';
		if (i < MAX_AURA_DEVS)
			strncpy((char *)&eHouseProN->ADCType[i], (char *)&GetLine, sizeof(eHouseProN->ADCType[i]));//[0]-'0';
	}

	GetStr(data);// #Outs;
	for (i = 0; i < sizeof(eHouseProN->Outs) / sizeof(eHouseProN->Outs[0]); i++)      //binary outputs names
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->Outs[i], (char *)&GetLine, sizeof(eHouseProN->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i, 1, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);
	for (i = 0; i < sizeof(eHouseProN->Inputs) / sizeof(eHouseProN->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->Inputs[i], (char *)&GetLine, sizeof(eHouseProN->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(eHouseProN->Dimmers) / sizeof(eHouseProN->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->Dimmers[i], (char *)&GetLine, sizeof(eHouseProN->Dimmers[i]));
		//    UpdateSQLState(data[1], data[2], EH_PRO, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i, 1, 0, "", Name, (char *) &GetLine, true, 100);
	}
	//UpdateSQLState(data[1], data[2], EH_LAN, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 0, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);//rollers names

	for (i = 0; i < sizeof(eHouseProN->Rollers) / sizeof(eHouseProN->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_PRO, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}

	int k = 0;
	GetStr(data);    // #Programs Names
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");     //Program/scene selector
	for (i = 0; i < sizeof(eHouseProN->Programs) / sizeof(eHouseProN->Programs[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->Programs[i], (char *)&GetLine, sizeof(eHouseProN->Programs[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
			if (strlen(PGMs) > 400) break;
		}
	}

	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_PGM, PGMs, k);
	}
	k = 0;

	strcpy(PGMs, "SelectorStyle:1;LevelNames:");		//Add Regulation Program Selector
	GetStr(data);									// "#ADC Programs Names
	for (i = 0; i < sizeof(eHouseProN->ADCPrograms) / sizeof(eHouseProN->ADCPrograms[0]); i++)
	{
		GetStr(data);
		strncpy((char *)&eHouseProN->ADCPrograms[i], (char *)&GetLine, sizeof(eHouseProN->ADCPrograms[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
			if (strlen(PGMs) > 400) break;
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_APGM, PGMs, k);
	}
	GetStr(data);
	strcat(Name, "#Secu Programs Names\r\n");		//CM only
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");	//Add Regulation Program Selector
	k = 0;
	for (i = 0; i < sizeof((void *)&eHouseProN->SecuPrograms) / sizeof(&eHouseProN->SecuPrograms[0]); i++)
	{
		//strcat(Name, EHouseProN.SecuPrograms[i]);
		//strcat(Name,"\r\n");
		//if (i > 9) break;
		GetStr(data);
		strncpy((char *)&eHouseProN->SecuPrograms[i], (char *)&GetLine, sizeof(eHouseProN->SecuPrograms[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
			if (strlen(PGMs) > 400) break;
		}

	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_SECUPGM, 1, 1, 0, "0", Name, "Pgm. Secu", true, 100);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_SECUPGM, 1, 1, 0, "0", Name, "Pgm. Secu", true, 100);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_SECUPGM, PGMs, k);
	}
	GetStr(data);
	k = 0;
	strcat(Name, "#Zones Programs Names\r\n");		//CM only
	strcpy(PGMs, "SelectorStyle:1;LevelNames:");	//Add Regulation Program Selector
	for (i = 0; i < sizeof((void *)&eHouseProN->Zones) / sizeof(&eHouseProN->Zones[0]); i++)
	{
		//strcat(Name, EHouseProN.SecuPrograms[i]);
		//strcat(Name,"\r\n");
		//if (i > 9) break;
		GetStr(data);
		strncpy((char *)&eHouseProN->Zones[i], (char *)&GetLine, sizeof(eHouseProN->Zones[i]));
		if ((strlen((char *)&GetLine) > 1) && (strstr((char *)&GetLine, "@") == NULL))
		{
			k++;
			sprintf(tmp, "%s (%d)|", (char *)&GetLine, i + 1);
			if (k <= 10) strncat(PGMs, tmp, strlen(tmp));
			if (strlen(PGMs) > 400) break;
		}
	}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'
	if (k > 0)
	{
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_ZONEPGM, 1, 1, 0, "0", Name, "Zones", true, 100);
		k = UpdateSQLState(data[1], data[2], EH_PRO, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_ZONEPGM, 1, 1, 0, "0", Name, "Zones", true, 100);
		//ISO2UTF8(PGMs);
		UpdatePGM(data[1], data[2], VISUAL_ZONEPGM, PGMs, k);
	}
	eHouseProStatusPrv->data[0] = 0;
	eHouseProN->BinaryStatus[0] = 0;
}
////////////////////////////////////////////////////////////////////////////////




void eHouseTCP::GetUDPNamesWiFi(unsigned char *data, int nbytes)

{   //size =data[0]; //not used / invalid only LSB for eHouse UDP protocol compatibility
	//addrh =data[1]; //binary coded not used - for eHouse UDP protocol compatibility
	//addrl =data[2]; //binary coded not used - for eHouse UDP protocol compatibility
	int i;
	char Name[80];
	//char tmp[96];
	//char PGMs[500u];
	//memset(PGMs, 0, sizeof(PGMs));
	gettype(data[1], data[2]);
	if (data[3] != 'n') return;   //not names
	if (data[4] != '4') return;   //other controller type than ERM
	unsigned char nr = (data[2] - INITIAL_ADDRESS_WIFI) % STATUS_ARRAYS_SIZE;  //limited - overlap if more than 32
	GetIndex = 7;					//Ignore binary control fields
	GetSize = nbytes;				//size of whole packet
	GetStr(data);				//addr combined
	GetStr(data);				//addr h
	GetStr(data);				//addr l
	GetStr(data);				//name
	i = 1;
	eHWIFIaloc(nr, data[1], data[2]);
	PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_WIFI, (char *)&GetLine);					//for Automatic RoomPlan generation for RoomManager
	if (PlanID < 0) PlanID = UpdateSQLPlan((int)data[1], (int)data[2], EH_WIFI, (char *)&GetLine);	//Add RoomPlan for RoomManager (RM)

	strncpy((char *)&eHWIFIn[nr]->Name, (char *)&GetLine, sizeof(eHWIFIn[nr]->Name));				//RM Controller Name
	strncpy((char *)&Name, (char *)&GetLine, sizeof(Name));
	GetStr(data);   //comment

	for (i = 0; i < sizeof(eHWIFIn[nr]->ADCs) / sizeof(eHWIFIn[nr]->ADCs[0]); i++)          //ADC Names (measurement+regulation)
	{
		GetStr(data);
		strncpy((char *)&eHWIFIn[nr]->ADCs[i], (char *)&GetLine, sizeof(eHWIFIn[nr]->ADCs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeTEMP, sTypeTEMP5, 0, VISUAL_MCP9700_IN, i + 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeThermostat, sTypeThermSetpoint, 0, VISUAL_MCP9700_PRESET, i + 1, 1, 0, "20.5", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);		// #ADC CFG;
	for (i = 0; i < sizeof(eHWIFIn[nr]->ADCs) / sizeof(eHWIFIn[nr]->ADCs[0]); i++)          //ADC Config (sensor type) not used currently
	{
		GetStr(data);
		eHWIFIn[nr]->ADCConfig[i] = GetLine[0] - '0';
	}

	GetStr(data);		// #Outs;
	for (i = 0; i < sizeof(eHWIFIn[nr]->Outs) / sizeof(eHWIFIn[nr]->Outs[0]); i++)			//binary outputs names
	{
		GetStr(data);
		strncpy((char *)&eHWIFIn[nr]->Outs[i], (char *)&GetLine, sizeof(eHWIFIn[nr]->Outs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeGeneralSwitch, sSwitchTypeAC, STYPE_OnOff, 0x21, i + 1, 1, 0, "0", Name, (char *)&GetLine, true, 100);
	}

	GetStr(data);
	for (i = 0; i < sizeof(eHWIFIn[nr]->Inputs) / sizeof(eHWIFIn[nr]->Inputs[0]); i++)  //binary inputs names
	{
		GetStr(data);
		strncpy((char *)&eHWIFIn[nr]->Inputs[i], (char *)&GetLine, sizeof(eHWIFIn[nr]->Inputs[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeGeneralSwitch, sSwitchGeneralSwitch, STYPE_Contact, VISUAL_INPUT_IN, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}


	GetStr(data);//Description
	for (i = 0; i < sizeof(eHWIFIn[nr]->Dimmers) / sizeof(eHWIFIn[nr]->Dimmers[0]); i++)    //dimmers names
	{
		GetStr(data);
		strncpy((char *)&eHWIFIn[nr]->Dimmers[i], (char *)&GetLine, sizeof(eHWIFIn[nr]->Dimmers[i]));
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeLighting2, sTypeAC, STYPE_Dimmer, VISUAL_DIMMER_OUT, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}
	UpdateSQLState(data[1], data[2], EH_WIFI, pTypeColorSwitch, sTypeColor_RGB_W, STYPE_Dimmer, VISUAL_DIMMER_RGB, 1, 1, 0, "", Name, "RGB", true, 100);  //RGB dimmer
	GetStr(data);//rollers names

	for (i = 0; i < sizeof(eHWIFIn[nr]->Rollers) / sizeof(eHWIFIn[nr]->Rollers[0]); i++)    //Blinds Names (use twin - single outputs) out #1,#2=> blind #1
	{
		GetStr(data);
		UpdateSQLState(data[1], data[2], EH_WIFI, pTypeLighting2, sTypeAC, STYPE_BlindsPercentage, VISUAL_BLINDS, i + 1, 1, 0, "", Name, (char *)&GetLine, true, 100);
	}

	int k = 0;
	/*    GetStr(data);    // #Programs Names
		strcpy(PGMs,"SelectorStyle:1;LevelNames:");     //Program/scene selector
		for (i = 0; i < sizeof(eHWIFIn[nr].Programs) / sizeof(eHWIFIn[nr].Programs[0]); i++)
			{
			GetStr(data);
			printf("%s\r\n", (char *) &GetLine);
			strncpy((char *) &eHWIFIn[nr].Programs[i], (char *) &GetLine, sizeof(eHWIFIn[nr].Programs[i]));
			if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == NULL))
				{
				k++;
				sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
				if (k<= 10) strncat(PGMs, tmp, strlen(tmp));


				}
			}
	*/
	/*PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_PGM, 1, 1, 0, "0", Name, "Scene", true, 100);
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
		if ((strlen((char *) &GetLine)>1) && (strstr((char *) &GetLine,"@") == NULL))
				{
				k++;
				sprintf(tmp,"%s (%d)|", (char *) &GetLine, i + 1);
				if (k<= 10) strncat(PGMs, tmp, strlen(tmp));
				}
		}
	PGMs[strlen(PGMs) - 1] = 0; //remove last '|'

	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
	k=UpdateSQLState(data[1], data[2], EH_LAN, pTypeGeneralSwitch, sSwitchTypeSelector, STYPE_Selector, VISUAL_APGM, 1, 1, 0, "0", Name, "Reg. Scene", true, 100);
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
	eHWIFIPrev[nr]->data[0] = 0;
	eHWIFIn[nr]->BinaryStatus[0] = 0;
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
		ipaddrh = SrvAddrH;
		ipaddrl = SrvAddrL;
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
	if (tf == NULL) return;
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
	UDP_terminate_listener = 0;
#ifndef WIN32
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;

#endif

	if (ViaTCP)
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
		disablers485 = 1;
	}
	unsigned int sum, sum2;
	unsigned char udp_status[MAXMSG], devaddrh, devaddrl, log[300u], dir[10];	// , code[20];
	int i;
	char GetLine[255];
	struct sockaddr_in saddr, caddr;
	socklen_t size;
	int nbytes;
	if (UDP_terminate_listener)
	{
		_log.Log(LOG_STATUS, "[%s] Disabled", LogPrefix);
		return;
	}
	_log.Log(LOG_STATUS, "!!!!! eHouseUDP: Do Work Starting");
	memset(GetLine, 0, sizeof(GetLine));
	memset(&saddr, 0, sizeof(saddr));
	memset(&caddr, 0, sizeof(caddr));
	if (ViaTCP == 0)
	{
		saddr.sin_family = AF_INET;							//initialization of protocol & socket
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		saddr.sin_port = htons(UDP_PORT);
		eHouseUDPSocket = socket(AF_INET, SOCK_DGRAM, 0);	//socket creation

		opt = 1; setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
		opt = 1; setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));

		//opt = 0;	setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_LINGER, &opt, sizeof(int));
#ifndef WIN32
		setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
		setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
#else
		opt = 100;	setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&opt, sizeof(opt));
		opt = 100;	setsockopt(eHouseUDPSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&opt, sizeof(opt));
		opt = 1;	setsockopt(eHouseUDPSocket, IPPROTO_UDP, TCP_NODELAY, (char *)&opt, sizeof(opt));// < 0)
		/*{
		LOG(LOG_ERROR, "[TCP Cli Status] Set TCP No Delay failed");
		perror("[TCP Cli Status] Set TCP No Delay failed\n");
		}

		*/

#endif

		ressock = bind(eHouseUDPSocket, (struct sockaddr *) &saddr, sizeof(saddr));  //bind socket
	}
	m_stoprequested = false;
	int SecIter = 0;
	unsigned char ou = 0;
	//	LOG(LOG_STATUS, "TIM: %d", mytime(NULL) - m_LastHeartbeat);
	m_LastHeartbeat = mytime(NULL);
	int prevtim, tim = clock();
	prevtim = tim;
	time_t tt = time(NULL);
	while (!m_stoprequested)				//main loop
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
		if (ViaTCP)
		{
			if (NoDetectTCPPack > 0) NoDetectTCPPack--;
			m_LastHeartbeat = mytime(NULL);
			if ((SecIter % 100) == 1)		//15-30 sec - send keep alive
			{

				//_log.Log(LOG_STATUS, "[TCP Keep Alive ] ");
				char dta[2];
				dta[0] = 'q';
				send(TCPSocket, dta, 1, 0);
			}
		}
		else
			if ((SecIter % 100) == 1)
			{

				//LOG(LOG_STATUS, "!!!!TTTTIM: %d", time(NULL) - tt);
				//tt = time(NULL);
				m_LastHeartbeat = mytime(NULL);
			}
		char eh1 = 0;
		size = sizeof(caddr);
		memset(udp_status, 0, sizeof(udp_status));
		if (!ViaTCP)
			nbytes = recvfrom(eHouseUDPSocket, (char *)&udp_status, MAXMSG, 0, (struct sockaddr *) & caddr, &size);	//Standard UDP Operation (LAN)
		else
		{
			if (NoDetectTCPPack != 0)
			{
				nbytes = recv(TCPSocket, (char *)&udp_status, MAXMSG, 0);				//TCP Operation Based on Timeout - may loose combined statuses
				if (eH1(udp_status[1], udp_status[2]))
					eh1 = 1;
			}
			else	//recover multiple packets for Slow links (eg. Internet)
			{
				int len = 0;
				nbytes = recv(TCPSocket, (char *)&udp_status, 4, 0);
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
						recv(TCPSocket, (char *)&udp_status, MAXMSG, 0);
						continue;
					}
					else
						if (nbytes < 0)									//SOCKET ERROR
						{
							if (m_stoprequested)					//real termination - end LOOP
							{
								TerminateUDP();
								break;
							}
							comm++;
							//_log.Log(LOG_STATUS, "Nbytes<0");
							if ((ViaTCP) && (comm > 600))							// TCP must try reconnect
							{
								comm = 0;
								_log.Log(LOG_STATUS, "[eHouse TCP] Connection Lost. Reconnecting...");
								closesocket(TCPSocket);				// Try close socket
								TCPSocket = -1;
								ssl(6);
								TCPSocket = ConnectTCP(0);				// Reconect
																		//	goto RETRY;							// Go to start
							}
							continue;								//if UDP just ignore errors
						}



				if ((udp_status[0] == 0) || (udp_status[2] == 0) || (nbytes == 0))
				{
					LOG(LOG_STATUS, "[eHouse TCP] Data Is Zero");
					recv(TCPSocket, (char *)&udp_status, MAXMSG, 0);
					continue;
				}
				if (eH1(udp_status[1], udp_status[2]))					///!!!!! tymczasowo do uruhcmonienia wifi ?? translation in English please!
				{
					//					LOG(LOG_STATUS, "EH 1");
					len--;
					eh1 = 1;
				}
				if ((udp_status[0] < 255) && (udp_status[4] != 'n'))	//not eHouse PRO
				{
					nbytes += recv(TCPSocket, (char *)&udp_status[4], len, 0);
				}
				else
				{									//eHouse PRO
					if (ProSize == 0) ProSize = MAXMSG;
					nbytes += recv(TCPSocket, (char *)&udp_status[4], ProSize, 0);
				}
			}
		}

		if (nbytes == 0) continue;
		if (nbytes < 0)									//SOCKET ERROR
		{
			if (m_stoprequested)					//real termination - end LOOP
			{
				TerminateUDP();
				break;
			}
			comm++;
			//_log.Log(LOG_STATUS, "Nbytes<0");
			if ((ViaTCP) && (comm > 600))							// TCP must try reconnect
			{
				comm = 0;
				_log.Log(LOG_STATUS, "[eHouse TCP] Connection Lost. Reconnecting...");
				closesocket(TCPSocket);				// Try close socket
				TCPSocket = -1;
				ssl(6);
				TCPSocket = ConnectTCP(0);				// Reconnect
														//	goto RETRY;							// Go to start
			}
			continue;								//if UDP just ignore errors
		}
		comm = 0;

		if (ViaTCP)
		{
			devaddrh = ipaddrh = udp_status[1];
			devaddrl = ipaddrl = udp_status[2];
		}
		else
		{
			ipaddrh = (unsigned char)(((caddr.sin_addr.s_addr & 0x00ff0000) >> 16) & 0xff);
			ipaddrl = (caddr.sin_addr.s_addr >> 24) & 0xff;
		}


		if (nbytes > 254)
		{
			//comm = 0;
			if (udp_status[3] == 'n')
			{						//get names
				NoDetectTCPPack = 120;
				_log.Log(LOG_STATUS, "[%s NAMES: 192.168.%d.%d ] ", LogPrefix, udp_status[1], udp_status[2]);
				if ((eHEnableAutoDiscovery))
				{
					//if (!TESTTEST)
					GetUDPNamesLAN(udp_status, nbytes);
					//if (!TESTTEST)
					GetUDPNamesCM(udp_status, nbytes);
					//if (!TESTTEST)
					if (eHEnableProDiscovery) GetUDPNamesPRO(udp_status, nbytes);
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
				for (iii = 0; iii < nbytes - 2u; iii++)               //calculate local checksum
				{
					sum += udp_status[iii];
				}
				devaddrh = udp_status[4];
				devaddrl = udp_status[5];
				if (udp_status[0] != 0xff) continue;
				if (udp_status[1] != 0x55) continue;
				if (udp_status[2] != 0xff) continue;
				if (udp_status[3] != 0x55) continue;
				if (ViaTCP)
				{
					devaddrh = ipaddrh = udp_status[4];
					devaddrl = ipaddrl = udp_status[5];
				}
				//debu(devaddrh, devaddrl, 0, nbytes, sum, sum2, udp_status[0]);
				if (((sum & 0xffff) == (sum2 & 0xffff)) || (ViaTCP))        //if valid then perform
				{
					ProSize = MAXMSG;// nbytes - 4;

					//eHPROaloc(0, devaddrh, devaddrl);
					if (!ViaTCP)
						if ((ipaddrh != SrvAddrH) || (ipaddrl != SrvAddrL))
						{
							if (TESTTEST)
								LOG(LOG_STATUS, "[%s PRO] Ignore other PRO installation from Server: 192.168.%d.%d", LogPrefix, ipaddrh, ipaddrl);
							continue;
						}
					if (StatusDebug) LOG(LOG_STATUS, "[%s PRO] status installation from Server: 192.168.%d.%d", LogPrefix, ipaddrh, ipaddrl);
					memcpy(eHouseProStatus->data, &udp_status, sizeof(eHouseProStatus->data));
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
				HeartBeat++;
				if (disablers485)
				{
					if ((devaddrh == 55) || (devaddrh == 1) || (devaddrh == 2)) continue;
				}
				//if( StatusDebug) printf("[UDP] Status: (%-3d,%-3d)  (%dB)\r\n", devaddrh, devaddrl, nbytes);
				if (devaddrl > 199) i = EHOUSE1_RM_MAX + 3;
				else
					if ((devaddrh == 1))							//HM index in status eH[0]
						i = 0;
					else
						if ((devaddrh == 2))					//EM index in status eH[0]
							i = EHOUSE1_RM_MAX - 1;
						else
							i = devaddrl;                //RM index in status the same as device address low - eH[devaddrl]
				if (udp_status[3] != 'l')
				{
					if (StatusDebug) _log.Log(LOG_STATUS, "[%s] St: (%-3d,%-3d) - OK (%dB)", LogPrefix, devaddrh, devaddrl, nbytes);
				}
				else
					//if    (udp_status[3] == 'l')
					if (nbytes > 6)
					{
						strncpy((char *)&log, (char *)&udp_status[4], nbytes - 6);
						LOG(LOG_STATUS, "[%s Log] (%-3d,%-3d) - %s", LogPrefix, devaddrh, devaddrl, log);
						if (IRPerform)
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
					if (((ipaddrh != SrvAddrH) || (ipaddrl != SrvAddrL)) && (!ViaTCP))
					{
						if (TESTTEST) LOG(LOG_STATUS, "Ignore other instalation from Serwer: 192.168.%d.%d", ipaddrh, ipaddrl);
						continue;
					}


					eHaloc(index, devaddrh, devaddrl);
					if (memcmp(&eHn[index]->BinaryStatus[0], &udp_status, nbytes) != 0) CloudStatusChanged = 1;
					memcpy(&eHn[index]->BinaryStatus[0], udp_status, nbytes);
					memcpy(&eHRMs[index]->data[0], &udp_status, 4);					//control data size,address,code 's'
					eHRMs[index]->eHERM.Outs[0] = udp_status[RM_STATUS_OUT];
					eHRMs[index]->eHERM.Outs[1] = udp_status[RM_STATUS_OUT + 1];
					eHRMs[index]->eHERM.Outs[2] = udp_status[RM_STATUS_OUT + 2];
					eHRMs[index]->eHERM.Outs[3] = udp_status[RM_STATUS_OUT25];
					eHRMs[index]->eHERM.Inputs[0] = udp_status[RM_STATUS_IN];
					eHRMs[index]->eHERM.Inputs[1] = udp_status[RM_STATUS_INT];
					eHRMs[index]->eHERM.Dimmers[0] = udp_status[RM_STATUS_LIGHT];
					eHRMs[index]->eHERM.Dimmers[1] = udp_status[RM_STATUS_LIGHT + 1];
					eHRMs[index]->eHERM.Dimmers[2] = udp_status[RM_STATUS_LIGHT + 2];
					CalculateAdcEH1(index);
					//memcpy(&eHRMs[index].eHERM.More, &udp_status[STATUS_MORE],8);
					//memcpy(&eHRMs[index].eHERM.DMXDimmers[17], &udp_status[STATUS_DMX_DIMMERS2], 15);
					//memcpy(&eHRMs[eHEIndex].eHERM.DaliDimmers, &udp_status[STATUS_DALI],46);

					eHRMs[index]->eHERM.CURRENT_PROFILE = udp_status[RM_STATUS_PROGRAM];
					if (index == 0)			//heat manager
					{
						eHRMs[index]->eHERM.CURRENT_PROFILE = udp_status[HM_STATUS_PROGRAM];
						eHRMs[index]->eHERM.Outs[0] = udp_status[HM_STATUS_OUT];
						eHRMs[index]->eHERM.Outs[1] = udp_status[HM_STATUS_OUT + 1];
						eHRMs[index]->eHERM.Outs[2] = udp_status[HM_STATUS_OUT + 2];
						eHRMs[index]->eHERM.Outs[3] = 0;
						eHRMs[index]->eHERM.Inputs[0] = 0;
						eHRMs[index]->eHERM.Inputs[1] = 0;
					}
					if (index == EHOUSE1_RM_MAX)		//EM
						eHRMs[index]->eHERM.CURRENT_ZONE = udp_status[RM_STATUS_ZONE_PGM];
					//eHRMs[index].eHERM.CURRENT_ADC_PROGRAM= udp_status[STATUS_ADC_PROGRAM];
					if (CloudStatusChanged)
					{
						UpdateRS485ToSQL(devaddrh, devaddrl, index);
						//    CloudStatusChanged = 0;
					}
					eHn[index]->FromIPH = ipaddrh;
					eHn[index]->FromIPL = ipaddrl;
					eHn[index]->BinaryStatusLength = nbytes;
					if (ViaCM)
					{
						eCMaloc(0, COMMANAGER_IP_HIGH, COMMANAGER_IP_LOW);
						memcpy(ECMn->BinaryStatus, udp_status, nbytes);
						ECMn->BinaryStatusLength = nbytes;
						ECMn->TCPQuery++;
						memcpy(&ECM->data[0], &udp_status[70], sizeof(ECM->data));       //eHouse 1 Controllers
					}
					eHn[index]->TCPQuery++;

					if (CloudStatusChanged)
					{
						UpdateCMToSQL(devaddrh, devaddrl, 0);
					}
					CloudStatusChanged = 0;
					//CalculateAdcCM();
					eHStatusReceived++;                                    //Flag of eHouse1 status received
					eHEStatusReceived++;                                    //flag of Ethernet eHouse status received
				}
				else			// ethernet controllers or CM without eHouse1 status
				{
					if (IsCM(devaddrh, devaddrl))        //CommManager/Level Manager
					{
						eCMaloc(0, devaddrh, devaddrl);
						memcpy(ECMn->BinaryStatus, udp_status, nbytes);
						ECMn->BinaryStatusLength = nbytes;
						ECMn->TCPQuery++;
						if (memcmp(ECM->data, &udp_status[70], sizeof(ECM->data) - 70) != 0) CloudStatusChanged = 1;

						memcpy(&ECM->data[0], &udp_status[70], sizeof(ECM->data));          //CommManager stand alone
						ECM->CM.AddrH = devaddrh;                        //Overwrite to default CM address
						ECM->CM.AddrL = devaddrl;
						if (CloudStatusChanged)
						{
							UpdateCMToSQL(devaddrh, devaddrl, 0);
						}
						CloudStatusChanged = 0;
						//CalculateAdcCM();
						eHEStatusReceived++;
					}
					else    //Not CM
					{
						if (devaddrl != 0)
						{
							if (devaddrl >= INITIAL_ADDRESS_LAN)
							{
								if (devaddrl - INITIAL_ADDRESS_LAN <= ETHERNET_EHOUSE_RM_MAX)   //Ethernet eHouse LAN Controllers
								{
									unsigned char eHEIndex = (devaddrl - INITIAL_ADDRESS_LAN) % (STATUS_ARRAYS_SIZE);
									{

										eHEaloc(eHEIndex, devaddrh, devaddrl);
										if (memcmp(eHEn[eHEIndex]->BinaryStatus, udp_status, nbytes) != 0)
											CloudStatusChanged = 1;

										memcpy(eHEn[eHEIndex]->BinaryStatus, udp_status, nbytes);
										eHEn[eHEIndex]->BinaryStatusLength = nbytes;
										eHEn[eHEIndex]->TCPQuery++;

										{
											memcpy(&eHERMs[eHEIndex]->data[0], &udp_status, 24); //control, dimmers
											memcpy(&eHERMs[eHEIndex]->eHERM.ADC[0], &udp_status[STATUS_ADC_ETH], 32);//adcs
											memcpy(&eHERMs[eHEIndex]->eHERM.Outs[0], &udp_status[STATUS_OUT_I2C], 5);
											memcpy(&eHERMs[eHEIndex]->eHERM.Inputs[0], &udp_status[STATUS_INPUTS_I2C], 3);
											//memcpy(&eHERMs[eHEIndex].eHERM.ADC, &udp_status[STATUS_INPUTS_I2C]
											CalculateAdc2(eHEIndex);


											memcpy(eHERMs[eHEIndex]->eHERM.More, &udp_status[STATUS_MORE], 8);
											memcpy(&eHERMs[eHEIndex]->eHERM.DMXDimmers[17], &udp_status[STATUS_DMX_DIMMERS2], 15);
											memcpy(eHERMs[eHEIndex]->eHERM.DaliDimmers, &udp_status[STATUS_DALI], 46);

											//eHERM.CURRENT_PROGRAM= udp_status[STATUS_PROGRAM_NR];
											eHERMs[eHEIndex]->eHERM.CURRENT_PROFILE = udp_status[STATUS_PROFILE_NO];
											eHERMs[eHEIndex]->eHERM.CURRENT_ADC_PROGRAM = udp_status[STATUS_ADC_PROGRAM];
											if (CloudStatusChanged)
											{
												UpdateLanToSQL(devaddrh, devaddrl, eHEIndex);
												CloudStatusChanged = 0;
											}
										}
										eHEStatusReceived++;
									}
								}
							}
							else if (devaddrl >= INITIAL_ADDRESS_WIFI)     //eHouse WiFi Controllers
							{
								if (devaddrl - INITIAL_ADDRESS_WIFI <= EHOUSE_WIFI_MAX)
								{
									unsigned char eHWiFiIndex = (devaddrl - INITIAL_ADDRESS_WIFI) % STATUS_WIFI_ARRAYS_SIZE;
									eHWIFIaloc(eHWiFiIndex, devaddrh, devaddrl);
									{
										if (memcmp(eHWIFIn[eHWiFiIndex]->BinaryStatus, udp_status, nbytes) != 0) CloudStatusChanged = 1;
										memcpy(eHWIFIn[eHWiFiIndex]->BinaryStatus, udp_status, nbytes);
										eHWIFIn[eHWiFiIndex]->BinaryStatusLength = nbytes;
										eHWIFIs[eHWiFiIndex]->eHWIFI.AddrH = devaddrh;
										eHWIFIs[eHWiFiIndex]->eHWIFI.AddrL = devaddrl;
										eHWIFIs[eHWiFiIndex]->eHWIFI.Size = nbytes;
										eHWIFIs[eHWiFiIndex]->eHWIFI.Code = 's';
										eHWIFIs[eHWiFiIndex]->eHWIFI.Outs[0] = udp_status[OUT_OFFSET_WIFI];
										eHWIFIs[eHWiFiIndex]->eHWIFI.Inputs[0] = udp_status[IN_OFFSET_WIFI];
										eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[0] = udp_status[DIMM_OFFSET_WIFI];
										eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[1] = udp_status[DIMM_OFFSET_WIFI + 1];
										eHWIFIs[eHWiFiIndex]->eHWIFI.Dimmers[2] = udp_status[DIMM_OFFSET_WIFI + 2];
										//eHWIFIs[index].eHWIFI.Dimmers[3] = udp_status[DIMM_OFFSET_WIFI+3];
										eHWIFIn[eHWiFiIndex]->TCPQuery++;
										memcpy(&eHWiFi[eHWiFiIndex]->data[0], &udp_status[1], sizeof(union WiFiStatusT));          //ERMs, EHM -Ethernet
										CalculateAdcWiFi(eHWiFiIndex);
										if (CloudStatusChanged)
										{

											UpdateWiFiToSQL(devaddrh, devaddrl, eHWiFiIndex);
											CloudStatusChanged = 0;
										}
										eHWiFiStatusReceived++;
									}
								}
							}
							else
							{
								if ((devaddrh == 0x81))       //Aura Thermostats Via eHouse PRO
								{
									if (devaddrl >= MAX_AURA_DEVS) continue;
									unsigned char aindex = 0;
									aindex = devaddrl - 1;
									eAURAaloc(aindex, devaddrh, devaddrl);
									memcpy(AuraN[aindex]->BinaryStatus, &udp_status, sizeof(AuraN[aindex]->BinaryStatus));

									AuraDev[aindex]->Addr = devaddrl;		// address l
									i = 3;
									AuraDev[aindex]->ID = AuraN[aindex]->BinaryStatus[i++];
									AuraDev[aindex]->ID = AuraDev[aindex]->ID << 8;
									AuraDev[aindex]->ID |= AuraN[aindex]->BinaryStatus[i++];
									AuraDev[aindex]->ID = AuraDev[aindex]->ID << 8;
									AuraDev[aindex]->ID |= AuraN[aindex]->BinaryStatus[i++];
									AuraDev[aindex]->ID = AuraDev[aindex]->ID << 8;
									AuraDev[aindex]->ID |= AuraN[aindex]->BinaryStatus[i++];
									if (DEBUG_AURA) LOG(LOG_STATUS, "[Aura UDP %d] ID: %lx\t", aindex + 1, (long int)AuraDev[aindex]->ID);
									AuraDev[aindex]->DType = AuraN[aindex]->BinaryStatus[i++];            //device type
									if (DEBUG_AURA) LOG(LOG_STATUS, " DevType: %d\t", AuraDev[index]->DType);
									i++;//params count
									int m, k = 0;
									int bkpi = i + 2;
									char FirstTime = 0;
									for (m = 0; m < sizeof(AuraN[aindex]->ParamSymbol); m++)
									{
										//if ((AuraDev[index].ParamSize>2) && (!FirstTime))
										{

											AuraN[aindex]->ParamValue[m] = AuraN[aindex]->BinaryStatus[i++];
											AuraN[aindex]->ParamValue[m] = AuraN[aindex]->ParamValue[m] << 8;
											AuraN[aindex]->ParamValue[m] |= AuraN[aindex]->BinaryStatus[i++];
											//if (AuraDev[index].ParamValue[i]>0x8000)


											AuraN[aindex]->ParamPreset[m] = AuraN[aindex]->BinaryStatus[i++];
											AuraN[aindex]->ParamPreset[m] = AuraN[aindex]->ParamPreset[m] << 8;
											AuraN[aindex]->ParamPreset[m] |= AuraN[aindex]->BinaryStatus[i++];

											AuraN[aindex]->ParamSymbol[m] = AuraN[aindex]->BinaryStatus[i++];


											if (((AuraN[aindex]->ParamSymbol[m] == 'C') || (AuraN[aindex]->ParamSymbol[m] == 'T')) && (!FirstTime))
											{
												FirstTime++;
												AuraN[aindex]->ADCUnit[aindex] = 'T';
												AuraDev[aindex]->Temp = AuraN[aindex]->ParamValue[m];
												AuraDev[aindex]->Temp /= 10;
												//                                                 AuraDev[aindex]->TempSet = AuraN[aindex]->ParamPreset[m];
												//                                               AuraDev[aindex]->TempSet/= 10;
												AuraDev[aindex]->LocalTempSet = AuraN[aindex]->ParamPreset[m];
												AuraDev[aindex]->LocalTempSet /= 10;
												if (AuraDev[aindex]->LocalTempSet != AuraDev[aindex]->PrevLocalTempSet)
												{
													AuraDev[aindex]->PrevLocalTempSet = AuraDev[aindex]->LocalTempSet;
													AuraDev[aindex]->TempSet = AuraDev[aindex]->LocalTempSet;
													AuraDev[aindex]->ServerTempSet = AuraDev[aindex]->LocalTempSet;
													AuraDev[aindex]->PrevServerTempSet = AuraDev[aindex]->LocalTempSet;
													if (DEBUG_AURA) LOG(LOG_STATUS, "LTempC: %f\t", AuraDev[aindex]->TempSet);
												}
												if (AuraDev[aindex]->ServerTempSet != AuraDev[aindex]->PrevServerTempSet)
												{
													//AuraDev[auraindex].PrevLocalTempSet = AuraDev[auraindex].ServerTempSet;
													AuraDev[aindex]->TempSet = AuraDev[aindex]->ServerTempSet;
													//AuraDev[auraindex].LocalTempSet = AuraDev[auraindex].ServerTempSet;
													AuraDev[aindex]->PrevServerTempSet = AuraDev[aindex]->ServerTempSet;
													if (DEBUG_AURA) LOG(LOG_STATUS, "STempC: %f\t", AuraDev[aindex]->TempSet);
													//AuraN[auraindex].ParamPreset[0] = (unsigned int) (AuraDev[auraindex].TempSet*10);
												}
												//for (i = 0; i<AuraDev[auraindex].ParamsCount; i++)
												{
													AuraN[aindex]->ParamPreset[0] = (unsigned int)(AuraDev[aindex]->TempSet * 10);

													//eHPROaloc(0, SrvAddrH, SrvAddrL);

													eHouseProStatus->status.AdcVal[aindex] = AuraN[aindex]->ParamValue[0];// temp * 10;//.MSB<<8) + eHouseProStatus.status.AdcVal[nr_of_ch].LSB;
																														  //adcs[aindex]->ADCValue= (int) AuraDev[aindex]->temp * 10;
													adcs[aindex]->ADCHigh = AuraN[aindex]->ParamPreset[0] + 3;
													adcs[aindex]->ADCLow = AuraN[aindex]->ParamPreset[0] - 3;
													AuraN[aindex]->BinaryStatus[bkpi++] = AuraN[aindex]->ParamPreset[0] >> 8;
													AuraN[aindex]->BinaryStatus[bkpi] = AuraN[aindex]->ParamPreset[0] & 0xff;
													nr_of_ch = aindex;
													//        PerformADC();       //Perform Adc measurement process
													//                                                                    AuraN[aindex]->TextStatus[0] = 0;
												}


											}
										}
									}
									AuraDev[aindex]->RSSI = -(255 - AuraN[aindex]->BinaryStatus[i++]);
									if (DEBUG_AURA) LOG(LOG_STATUS, " RSSI: %d\t", AuraDev[aindex]->RSSI);
									AuraN[aindex]->Vcc = AuraN[aindex]->BinaryStatus[i++];
									AuraDev[aindex]->volt = AuraN[aindex]->Vcc;
									AuraDev[aindex]->volt /= 10;
									if (DEBUG_AURA) LOG(LOG_STATUS, " Vcc: %d\t", AuraN[aindex]->Vcc);
									AuraDev[aindex]->Direction = AuraN[aindex]->BinaryStatus[i++];
									if (DEBUG_AURA) LOG(LOG_STATUS, "Direction: %d\t", AuraDev[aindex]->Direction);
									AuraDev[aindex]->DoorState = AuraN[aindex]->BinaryStatus[i++];
									switch (AuraDev[aindex]->DoorState & 0x7)
									{
									case 0: AuraDev[aindex]->Door = '|';
										adcs[aindex]->door = 0;
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_CLOSE;
										AuraDev[aindex]->WINDOW_CLOSE = 1;
										break;

									case 1: AuraDev[aindex]->Door = '<';
										AuraDev[aindex]->WINDOW_OPEN = 1;
										adcs[aindex]->door = 10;
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 2:
										AuraDev[aindex]->WINDOW_SMALL = 1;
										adcs[aindex]->door = 2;
										AuraDev[aindex]->Door = '/';
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_SMALL;
										adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableFan = 0;    //Disable Ventilation if flag is set
										break;
									case 3:
										adcs[aindex]->door = 10;
										AuraDev[aindex]->WINDOW_OPEN = 1;
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										AuraDev[aindex]->Door = '>';
										adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 4:
										AuraDev[aindex]->WINDOW_UNPROOF = 1;
										AuraDev[aindex]->Door = '~';
										adcs[aindex]->door = 1;
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_UNPROOF;
										break;
									case 5:
										adcs[aindex]->door = 10;
										adcs[aindex]->flagsa |= AURA_STAT_WINDOW_OPEN;
										AuraDev[aindex]->Door = 'X';
										adcs[aindex]->DisableVent = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableHeating = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableRecu = 1;    //Disable Ventilation if flag is set
										adcs[aindex]->DisableCooling = 1; //Disable Heating if flag is set
										adcs[aindex]->DisableFan = 1;    //Disable Ventilation if flag is set
										break;
									case 6: AuraDev[aindex]->Door = '-'; break;
									case 7: AuraDev[aindex]->Door = '-'; break;
									}
									if (AuraDev[aindex]->DoorState & 0x8)
									{
										AuraDev[aindex]->LOCK = 1;
										AuraDev[aindex]->Lock = 1;
									}
									else
									{
										AuraDev[aindex]->Lock = 0;
									}
									if (DEBUG_AURA) LOG(LOG_STATUS, "DoorState: %d\t", AuraDev[aindex]->DoorState);
									AuraDev[aindex]->Door = AuraN[aindex]->BinaryStatus[i++];
									if (DEBUG_AURA) LOG(LOG_STATUS, "Door: %d\t", AuraDev[aindex]->Door);
									adcs[aindex]->flagsa = AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsa = adcs[aindex]->flagsa << 8;
									adcs[aindex]->flagsa |= AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsa = adcs[aindex]->flagsa << 8;
									adcs[aindex]->flagsa |= AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsa = adcs[aindex]->flagsa << 8;
									adcs[aindex]->flagsa |= AuraN[aindex]->BinaryStatus[i++];
									if (DEBUG_AURA) LOG(LOG_STATUS, "FlagsA: %lx\t", adcs[aindex]->flagsa);

									adcs[aindex]->flagsb = AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsb = adcs[aindex]->flagsb << 8;
									adcs[aindex]->flagsb |= AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsb = adcs[aindex]->flagsb << 8;
									adcs[aindex]->flagsb |= AuraN[aindex]->BinaryStatus[i++];
									adcs[aindex]->flagsb = adcs[aindex]->flagsb << 8;
									adcs[aindex]->flagsb |= AuraN[aindex]->BinaryStatus[i++];
									unsigned int time = AuraN[aindex]->BinaryStatus[i++];
									time = time << 8;
									time += AuraN[aindex]->BinaryStatus[i++];
									AuraDev[aindex]->ServerTempSet = time;
									AuraDev[aindex]->ServerTempSet /= 10;
									if (DEBUG_AURA) LOG(LOG_STATUS, "FlagsB: %lx\t", (long unsigned int) adcs[aindex]->flagsb);
									AuraDev[aindex]->LQI = AuraN[aindex]->BinaryStatus[i++];
									if (DEBUG_AURA) LOG(LOG_STATUS, "LQI: %d\t", AuraDev[aindex]->LQI);
									AuraN[aindex]->BinaryStatusLength = AuraN[aindex]->BinaryStatus[0];//nbytes;
									AuraN[aindex]->TCPQuery++;
									AuraN[aindex]->StatusTimeOut = 15u;
									UpdateAuraToSQL(AuraN[aindex]->AddrH, AuraN[aindex]->AddrL, aindex);
								}
							}
						}
					}
				}
			}
			else
			{
				LOG(LOG_STATUS, "[%s] St:  (%03d,%03d) Invalid Check Sum {%d!=%d} (%dB)", LogPrefix, devaddrh, devaddrl, sum, sum2, nbytes);
				if (ViaTCP)
				{
					nbytes = recv(TCPSocket, (char *)&udp_status, MAXMSG, 0);
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
	if (ViaTCP)
	{
		char dta[2];
		dta[0] = 0;
		send(TCPSocket, dta, 1, 0);
#ifdef WIN32
		flushall();
#else
		//fflush(&TCPSocket);
#endif

		unsigned long iMode = 1;
#ifdef WIN32
		int status = ioctlsocket(TCPSocket, FIONBIO, &iMode);
#else
		int status = ioctl(TCPSocket, FIONBIO, &iMode);
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
		msl(1000);
		ressock = shutdown(eHouseUDPSocket, SHUT_RDWR);
	}
	else
		ressock = shutdown(eHouseUDPSocket, SHUT_RDWR);
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
	if (eHouseUDPSocket >= 0)
		ressock = close(eHouseUDPSocket);
	if (TCPSocket >= 0)
		ressock = close(TCPSocket);
#else
	if (eHouseUDPSocket >= 0)
		ressock = closesocket(eHouseUDPSocket);
	if (TCPSocket >= 0)
		ressock = closesocket(TCPSocket);
#endif
	eHouseUDPSocket = -1;
	TCPSocket = -1;

	LOG(LOG_STATUS, "close %d", ressock);
	LOG(LOG_STATUS, "\r\nTERMINATED UDP\r\n");
	//Freeing ALL Dynamic memory
	int eHEIndex = 0;
	for (eHEIndex = 0; eHEIndex < ETHERNET_EHOUSE_RM_MAX + 1; eHEIndex++)
		//	if (strlen((char *) &eHEn[eHEIndex])>0)
		if (eHEn[eHEIndex] != NULL)
		{
			LOG(LOG_STATUS, "Freeing 192.168.%d.%d", eHEn[eHEIndex]->AddrH, eHEn[eHEIndex]->AddrL);
			free(eHEn[eHEIndex]);
			eHEn[eHEIndex] = 0;
			free(eHERMs[eHEIndex]);
			free(eHERMPrev[eHEIndex]);
			eHERMs[eHEIndex] = 0;
			eHERMPrev[eHEIndex] = 0;
		}

	for (eHEIndex = 0; eHEIndex < EHOUSE1_RM_MAX + 1; eHEIndex++)
		//if (strlen((char *) &eHn[eHEIndex])>0)
		if (eHn[eHEIndex] != NULL)
		{
			LOG(LOG_STATUS, "Freeing (%d,%d)", eHn[eHEIndex]->AddrH, eHn[eHEIndex]->AddrL);
			free(eHn[eHEIndex]);
			eHn[eHEIndex] = 0;
			free(eHRMs[eHEIndex]);
			free(eHRMPrev[eHEIndex]);
			eHRMs[eHEIndex] = 0;
			eHRMPrev[eHEIndex] = 0;
		}


	for (eHEIndex = 0; eHEIndex < EHOUSE_WIFI_MAX + 1; eHEIndex++)
		//	if (strlen((char *) &eHWIFIn[eHEIndex]) > 0)
		if (eHWIFIn[eHEIndex] != NULL)
		{
			LOG(LOG_STATUS, "Freeing 192.168.%d.%d", eHWIFIn[eHEIndex]->AddrH, eHWIFIn[eHEIndex]->AddrL);
			free(eHWIFIn[eHEIndex]);
			eHWIFIn[eHEIndex] = 0;
			free(eHWiFi[eHEIndex]);
			free(eHWIFIs[eHEIndex]);
			free(eHWIFIPrev[eHEIndex]);
			eHWiFi[eHEIndex] = 0;
			eHWIFIs[eHEIndex] = 0;
			eHWIFIPrev[eHEIndex] = 0;
		}

	//if (strlen((char *) &ECMn) > 0)
	if (ECMn != NULL)
	{
		LOG(LOG_STATUS, "Freeing 192.168.%d.%d", ECMn->AddrH, ECMn->AddrL);
		free(ECMn);
		ECMn = 0;
		free(ECM);
		free(ECMPrv);
		ECM = 0;
		ECMPrv = 0;
	}

	//if (strlen((char *) &eHouseProN) > 0)
	if (eHouseProN != 0)
	{
		LOG(LOG_STATUS, "Freeing 192.168.%d.%d", eHouseProN->AddrH[0], eHouseProN->AddrL[0]);
		free(eHouseProN);
		eHouseProN = 0;
		free(eHouseProStatus);
		free(eHouseProStatusPrv);
		eHouseProStatus = 0;
		eHouseProStatusPrv = 0;
	}


	for (i = 0; i < EVENT_QUEUE_MAX; i++)
	{

		{
			free(EvQ[i]);
			EvQ[i] = 0;
		}
	}

	for (i = 0; i < MAX_AURA_DEVS; i++)
	{
		//if (strlen((char *) & (AuraN[i])) < 1)
		if (AuraN[i] != NULL)
		{
			LOG(LOG_STATUS, "Free AURA (%d,%d)", 0x81, i + 1);
			free(AuraN[i]);
			AuraN[i] = 0;
			free(AuraDev[i]);
			free(AuraDevPrv[i]);
			AuraDev[i] = 0;
			AuraDevPrv[i] = 0;
			free(adcs[i]);
			adcs[i] = 0;
		}
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////
	LOG(LOG_STATUS, "END LISTENER TCP Client");
	m_bIsStarted = false;
	m_stoprequested = true;
	//pthread_exit(NULL);
	return;
}


