/*
File : MultiblocV8.cpp
Author : X.PONCET
Version : 2.00
Description : This class manage the CAN MultiblocV8 Layer
- Management of the receiveing Frame
- Treatment of each Life frame receive
- Up to 30 blocs can be manage actually !
- Complete management of input/output of SFSP Blocs (Master and Slave) up to 8 Master and 8 slaves in one CAN network
- On one SFSP blocs : 6 PWM output, 1 Power Voltage reading, 4 wired input and for Master blocs : each wireless switch receive is created in the domoticz dbb.
- Description of the SFSP bloc : http://www.scheiber.com/doc_technique/sfsp-2012-a1/

History :
- 2017-10-01 : Creation by X.PONCET


- 2018-01-22 : Update :
# add feature : manual creation up to 127 virtual switch, ability to learn eatch switch to any blocks output
# add feature : now possibility to Enter/Exit Learn mode Or Clear Mode for all SFSP Blocks
	(each blocks detected blocks automatically creates 3 associated buttons for Learn/Exit Learn and Clear, usefull if the blocks is not accessible )

- 2021-04-03 : V2.00 Update :
* Remove auto request on current consumed (no enough precision on mosfet)
* Update devices name list (NomRefBloc)
* Bloc 9 now supported
* Increase max number of blocs
* New methode to auto generate "release switch code" (old methode was limited to 1 button so risk of override this switch if to client are connected).
* with this update: increase of the USBtin buffer reception (see usbtin.h)
*
- 2021-04-14 : Formatted code to default style

- 2021-08-03 : V3.00 Update :
* Add new method to get bloc's name with security (out of table return "UNKNOWN") and complete name (with coding) when first devices creations.
* Fix error on all methods who use "RefBloc" , old was type char, must be type int !
* Add support of IBS Frame from Bloc 9 (Intelligent Battery Sensor) up to 6 sensors by bloc (IBS1 to 6) (Bloc 9 must be configured before use)
* With IBS: management of a "time left before charge/discharge" for each IBS detected

- 2023-10-07 : V4.00 Update  :
* Add support of Bloc 7 (Bloc with 6 analog input + 6xIBS + Supply voltage, fully configurable)
* Add digital input processing for frame receive by bloc 7 (and future used)
* So input must be set before use with configuration tool.
* Fixed bad sID write for supply voltage receive by bloc 9/sfsp (so old id used were incorrect )

*/

#include "stdafx.h"
#include "USBtin_MultiblocV8.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include <bitset> // This is necessary to compile on Windows

#define round(a) (int)(a + .5)

#define MULTIBLOC_V8_VERSION "04.00.00"

#define TIME_1sec 1000
#define TIME_500ms 500
#define TIME_200ms 200
#define TIME_100ms 100
#define TIME_50ms 50
#define TIME_10ms 10
#define TIME_5ms 5
#define TIME_2ms 2

#define BLOC_ALIVE 1
#define BLOC_NOTALIVE 2

#define MSK_TYPE_TRAME 0x1FFE0000
#define MSK_INDEX_MODULE 0x0001FF80
#define MSK_CODAGE_MODULE 0x00000078
#define MSK_SRES_MODULE 0x00000007

#define SHIFT_TYPE_TRAME 17
#define SHIFT_INDEX_MODULE 7
#define SHIFT_CODAGE_MODULE 3
#define SHIFT_SRES_MODULE 0

#define type_ALIVE_FRAME 0
#define type_LIFE_PHASE_FRAME 1
#define type_TP_DATA 2
#define type_TP_FLOW_CONTROL 3
#define type_DATE_TIME 4
#define type_ETAT_BLOC 5
#define type_COMMANDE_ETAT_BLOC 6

#define type_CTRL_FRAME_BLOC 256
#define type_CTRL_IO_BLOC 257

#define type_E_ANA 258
#define type_E_ANA_1_TO_4 258
#define type_E_ANA_5_TO_8 259
#define type_E_ANA_9_TO_12 260
#define type_E_ANA_13_TO_16 261
#define type_E_ANA_17_TO_20 262
#define type_E_ANA_21_TO_24 263
#define type_E_ANA_25_TO_28 264
#define type_E_ANA_29_TO_32 265

#define type_E_TOR 266
#define type_E_TOR_1_TO_64 266

#define type_S_TOR 267
#define type_STATE_S_TOR_1_TO_2 267
#define type_STATE_S_TOR_3_TO_4 268
#define type_STATE_S_TOR_5_TO_6 269
#define type_STATE_S_TOR_7_TO_8 270
#define type_STATE_S_TOR_9_TO_10 271
#define type_STATE_S_TOR_11_TO_12 272
#define type_STATE_S_TOR_13_TO_14 273
#define type_STATE_S_TOR_15_TO_16 274
#define type_STATE_S_TOR_17_TO_18 275
#define type_STATE_S_TOR_19_TO_20 276
#define type_STATE_S_TOR_21_TO_22 277
#define type_STATE_S_TOR_23_TO_24 278
#define type_STATE_S_TOR_25_TO_26 279
#define type_STATE_S_TOR_27_TO_28 280
#define type_STATE_S_TOR_29_TO_30 281
#define type_STATE_S_TOR_31_TO_32 282

#define type_CMD_S_TOR 283
#define type_CONSO_S_TOR_1_TO_8 284
#define type_CONSO_S_TOR_9_TO_16 285
#define type_CONSO_S_TOR_17_TO_24 286
#define type_CONSO_S_TOR_25_TO_32 287
#define type_CONSO_S_TOR type_CONSO_S_TOR_1_TO_8

#define type_IBS 768
#define type_IBS_1_1 769
#define type_IBS_1_2 770
#define type_IBS_2_1 771
#define type_IBS_2_2 772
#define type_IBS_3_1 773
#define type_IBS_3_2 774
#define type_IBS_4_1 775
#define type_IBS_4_2 776
#define type_IBS_5_1 777
#define type_IBS_5_2 778
#define type_IBS_6_1 779
#define type_IBS_6_2 780
#define type_IBS_1_3 781
#define type_IBS_2_3 782
#define type_IBS_3_3 783
#define type_IBS_4_3 784
#define type_IBS_5_3 785
#define type_IBS_6_3 786

#define type_SFSP_SWITCH 512
#define type_SFSP_LED_CMD 513
#define type_SFSP_CAPTEUR_1BS 514
#define type_SFSP_CAPTEUR_4BS 515
#define type_SFSP_SYNCHRO 516
#define type_SFSP_LearnCommand 517

#define BLOC_NORMAL_STATES 0x00
#define BLOC_STATES_OFF 0x01
#define BLOC_STATES_RESET 0x02
#define BLOC_STATES_LEARNING 0x03
#define BLOC_STATES_LEARNING_STOP 0x04
#define BLOC_STATES_CLEARING 0x05

// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// REFERENCE INDEX
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#define BLOC_DOMOTICZ 0x01

#define BLOC_7 0x0B
#define BLOC_9 0x0D
#define BLOC_SFSP_M 0x14
#define BLOC_SFSP_E 0x15

#define NomRefBloc_MAX_SIZE 78

constexpr std::array<const char*, NomRefBloc_MAX_SIZE> NomRefBloc{
	"UNDEFINED",
	"DOMOTICZ",
	"NAVIGRAPH_VER",
	"NAVIGRAPH_HOR",
	"NAVIGRAPH_HOR_RTC",
	"NAVICOLOR",
	"NAVICOLOR_RTC",
	"BLOC_1",
	"BLOC_2",
	"BLOC_4", // 10
	"BLOC_6",
	"BLOC_7",
	"BLOC_8",
	"BLOC_9",
	"BLOC_AU",
	"BLOC_CC",
	"BLOC_TF",
	"BLOC_AC1",
	"BLOC_X98",
	"BLOC_RESPIRE", // 20
	"BLOC_SFSP_M",
	"BLOC_SFSP_E",
	"BLOC_SFSP_A",
	"SELECTEUR_AC1",
	"CHARGEUR_MDP",
	"MULTICOM",
	"MULTICOM_V2",
	"INTERFACE_CAN_CAN",
	"INTERFACE_CAN_CAN_V2",
	"INTERFACE_BLUETOOTH", // 30
	"INTERFACE_LIN",
	"NAVICOLOR_GT",
	"CHARGEUR_CRISTEC",
	"BLOC_VENTIL",
	"BLOC_SIGNAL",
	"BLOC_PROTECT",
	"BLOC_X10",
	"AMPLI_ATOLL",
	"AMPLI_ATOLL_3ZONES",
	"SERVEUR_WIFI", // 40
	"NAVICOLOR_PT",
	"NAVICOLOR_PT2",
	"CLIM_DOMETIC",
	"FACADE_CARLING",
	"REPARTITEUR_MDP",
	"SELECTEUR_MDP",
	"INTERFACE_CAN_CAN_CONFIGURABLE",
	"INTERFACE_CAN_CAN_SD",
	"TABLEAU_VOILIER",
	"TABLEAU_COUPE_BATTERIE", // 50
	"COFFRET_COUPE_BATTERIE_MDP",
	"CLIMATISEUR_WEBASTO",
	"INTERFACE_CAN_CAN_MULTIRESEAUX",
	"BLOC_SFSP_2G4",
	"BLOC_MULTICOM",
	"NAVICOLOR_ST3",
	"NAVICOLOR_GT2",
	"FACADE_2_4T",
	"TABLEAU_CATA",
	"BLOC_EMB", // 60
	"WATERMAKER_DESSALATOR",
	"BATTERIE_EZA",
	"WATERMAKER_ECO_SISTEMS",
	"FACADE_SILICONNE_6T_ETANCHE",
	"BLOC_10",
	"BLOC_11",
	"BATTERIE_EPSILOR",
	"CLIMATISEUR_VENTILO_CONVECTEUR_FRIGOMAR",
	"BLOC_GALVATEST",
	"CONVERTISSEUR_CRISTEC", // 70
	"MB_BOX",
	"INTERFACE_CAN_CAN_NMEA2000",
	"BLOC_9_V3",
	"TOOLCHAIN_V8",
	"CONFIGURATEUR_V8",
	"INTERFACE_BLE_CAN",
	"CENTRALE_FROID_FRIGOMAR",
	"CLIMATISEUR_MONOBLOC_FRIGOMAR",
};

#define BatteryType_MAX_SIZE 5

constexpr std::array<const char*, BatteryType_MAX_SIZE> BatteryType{
	"Flooded", "AGM", "Enhanced flooded", "Lithium", "Undefined",
};

#define IBSType_MAX_SIZE 5

constexpr std::array<const char*, IBSType_MAX_SIZE> IBSType{
	"Absent/Missing",
	"IBS_12V",
	"IBS_24V",
	"UNDEFINED",
};

USBtin_MultiblocV8::USBtin_MultiblocV8()
{
	m_BOOL_DebugInMultiblocV8 = false;
}

USBtin_MultiblocV8::~USBtin_MultiblocV8()
{
	StopThread();
}

bool USBtin_MultiblocV8::StartThread()
{
	// Id Base for manual creation switch from domoticz...
	m_V8switch_id_base = (type_SFSP_SWITCH << SHIFT_TYPE_TRAME) + (1 << SHIFT_INDEX_MODULE) + (0 << SHIFT_CODAGE_MODULE) + 0;

	m_V8minCounterBase = (60 * 5);
	m_V8minCounter1 = (3600 * 6);
	m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
	SetThreadNameInt(m_thread->native_handle());
	Log(LOG_STATUS, "MultiblocV8: thread started, app version: %s", MULTIBLOC_V8_VERSION);
	return (m_thread != nullptr);
}

void USBtin_MultiblocV8::StopThread()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	ClearingBlocList();
}

void USBtin_MultiblocV8::ManageThreadV8(bool States, bool debugmode)
{
	if (debugmode == true)
		m_BOOL_DebugInMultiblocV8 = true;

	if (States == true)
		StartThread();
	else
		StopThread();
}

void USBtin_MultiblocV8::ClearingBlocList()
{
	Log(LOG_NORM, "MultiblocV8: clearing BlocList");
	// effacement du tableau à l'init
	for (auto& i : m_BlocList_CAN)
	{
		i.BlocID = 0;
		i.Status = 0;
		i.NbAliveFrameReceived = 0;
		i.VersionH = 0;
		i.VersionM = 0;
		i.VersionL = 0;
		i.CongifurationCrc = 0;
	}
}

void USBtin_MultiblocV8::Do_Work()
{
	unsigned char i;

	ClearingBlocList();
	// call every 100ms.... so...
	while (!IsStopRequested(TIME_100ms))
	{
		m_V8secCounterBase++;
		if (m_V8secCounterBase >= 10)
		{
			m_V8secCounterBase = 0;
			m_V8minCounterBase--;
			m_V8minCounter1--;

			if (m_V8minCounterBase == 0)
			{
				// each 5 minutes the analog and etor of all blocs are requested to update data
				m_V8minCounterBase = (60 * 5);
				m_BOOL_TaskAGo = true;
				m_BOOL_TaskRqEtorGo = true;
			}

			if (m_V8minCounter1 == 0)
			{
				// each 6 hours the ouputs states of all blocs are requested
				m_V8minCounter1 = (3600 * 6);
				m_BOOL_TaskRqStorGo = true;
			}

			m_V8secCounter1++;
			if (m_V8secCounter1 >= 3)
			{
				m_V8secCounter1 = 0;
				// each 3 seconds check the state of all blocs (alive or not)
				BlocList_CheckBloc();
			}
		}

		/*if( m_BOOL_SendPushOffSwitch ){ //if a Send push off switch is request
			m_BOOL_SendPushOffSwitch = false;
			USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(m_INT_SidPushoffToSend,m_CHAR_CodeTouchePushOff_ToSend);
		}*/

		for (i = 0; i < MAX_BUFFER_SFSP_SWITCH; i++)
		{
			if (m_sfsp_switch_tosend[i].m_BOOL_SendSFSP_Switch == true)
			{
				m_sfsp_switch_tosend[i].m_BOOL_SendSFSP_Switch = false;
				int Sid = m_sfsp_switch_tosend[i].m_INT_PushOffBufferToSend;
				char KeyCode = m_sfsp_switch_tosend[i].m_CHAR_CodeTouchePushOff_ToSend;
				USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(Sid, KeyCode);
			}
		}
	}
}

void USBtin_MultiblocV8::FillBufferSFSP_toSend(int Sid, char KeyCode)
{
	unsigned char i;
	for (i = 0; i < MAX_BUFFER_SFSP_SWITCH; i++)
	{
		if (m_sfsp_switch_tosend[i].m_BOOL_SendSFSP_Switch == false)
		{
			m_sfsp_switch_tosend[i].m_INT_PushOffBufferToSend = Sid;
			m_sfsp_switch_tosend[i].m_CHAR_CodeTouchePushOff_ToSend = KeyCode;
			m_sfsp_switch_tosend[i].m_BOOL_SendSFSP_Switch = true;
			break; // for filling only one free line in the table
		}
	}
}

/* this methods used to send request RTR on Can bus */
void USBtin_MultiblocV8::SendRequest(int sID)
{
	char szDeviceID[10];
	std::string szTrameToSend = "R"; // "R02180A008"; //for debug to force request
	sprintf(szDeviceID, "%08X", (unsigned int)sID);
	szTrameToSend += szDeviceID;
	szTrameToSend += "8";
	// Log(LOG_NORM,"MultiblocV8: write frame to Can: #%s# ",szTrameToSend.c_str());
	writeFrame(szTrameToSend);
	sleep_milliseconds(TIME_2ms); // wait time to not overrun can bus if many request are send...
}

void USBtin_MultiblocV8::Traitement_MultiblocV8(int IDhexNumber, unsigned int rxDLC, unsigned int Buffer_Octets[8])
{
	// décomposition de l'identifiant qui contient les informations suivante :
	// identifier parsing which contains the following information :
	int FrameType = (IDhexNumber & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
	int RefBloc = (IDhexNumber & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
	char Codage = (IDhexNumber & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE;
	char SsReseau = IDhexNumber & MSK_SRES_MODULE;

	if (m_thread)
	{
		switch (FrameType)
		{ // First switch !
		case type_ALIVE_FRAME:
			// création/update d'un composant détecter grace à sa trame de vie :
			// creates/updates of a blocs detected with his life frame :
			// Log(LOG_NORM,"MultiblocV8: Life Frame, ref: %#x Codage : %d Network: %d",RefBloc,Codage,SsReseau);
			Traitement_Trame_Vie(RefBloc, Codage, SsReseau, rxDLC, Buffer_Octets);
			break;

		case type_ETAT_BLOC:
			// traitement ETAT BLOC reçu :
			Traitement_Trame_EtatBloc(RefBloc, Codage, SsReseau, rxDLC, Buffer_Octets);
			break;

		case type_STATE_S_TOR_1_TO_2:
		case type_STATE_S_TOR_3_TO_4:
		case type_STATE_S_TOR_5_TO_6:
		case type_STATE_S_TOR_7_TO_8:
		case type_STATE_S_TOR_9_TO_10:
			// Log(LOG_NORM,"MultiblocV8: States Frame S_TOR, ref: %#x Codage : %d S/Réseau: %d",RefBloc,Codage,SsReseau);
			Traitement_Etat_S_TOR_Recu(FrameType, RefBloc, Codage, SsReseau, Buffer_Octets);
			break;

		case type_E_ANA_1_TO_4:
		case type_E_ANA_5_TO_8:
			Traitement_E_ANA_Recu(FrameType, RefBloc, Codage, SsReseau, Buffer_Octets);
			break;
		case type_E_TOR:
			Traitement_E_TOR_Recu(FrameType, RefBloc, Codage, SsReseau, Buffer_Octets);
			break;
		case type_SFSP_SWITCH:
			if (rxDLC == 5)
				Traitement_SFSP_Switch_Recu(FrameType, RefBloc, Codage, SsReseau, Buffer_Octets);
			break;

		case type_IBS_1_1:
		case type_IBS_1_2:
		case type_IBS_2_1:
		case type_IBS_2_2:
		case type_IBS_3_1:
		case type_IBS_3_2:
		case type_IBS_4_1:
		case type_IBS_4_2:
		case type_IBS_5_1:
		case type_IBS_5_2:
		case type_IBS_6_1:
		case type_IBS_6_2:
		case type_IBS_1_3:
		case type_IBS_2_3:
		case type_IBS_3_3:
		case type_IBS_4_3:
		case type_IBS_5_3:
		case type_IBS_6_3:
			Traitement_IBS(FrameType, RefBloc, Codage, SsReseau, Buffer_Octets);
			break;
		}
	}
}

// Traitement trame contenant une info d'un interrupteur SFSP (filaire ou sans fils, EnOcean, etc...)
// Treatment of Frame containing switch information from wireles or hardware input of a blocs :
void USBtin_MultiblocV8::Traitement_SFSP_Switch_Recu(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned long sID = (FrameType << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	int i = 0;

	// Log(LOG_NORM,"MultiblocV8: receive SFSP Switch: ID: %x Data: %x %x %x %x %x",sID, bufferdata[0], bufferdata[1],bufferdata[2], bufferdata[3], bufferdata[4]);
	uint32_t SwitchId = (bufferdata[0] << 24) + (bufferdata[1] << 16) + (bufferdata[2] << 8) + bufferdata[3];
	unsigned int codetouche = bufferdata[4];
	std::string defaultname = " ";

	Log(LOG_NORM, "MultiblocV8: Receiving SFSP Switch Frame: Id: %s Codage: %d Ssreseau: %d SwitchID: %08X CodeTouche: %02X", getBlocnameFromIndex(RefBloc), Codage, Ssreseau, SwitchId,
		codetouche);

	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;

	if (SwitchId == 0x0001)
	{ // hardware input of a blocs :
		lcmd.LIGHTING2.id1 = (sID >> 24) & 0xff;
		lcmd.LIGHTING2.id2 = (sID >> 16) & 0xff;
		lcmd.LIGHTING2.id3 = (sID >> 8) & 0xff;
		lcmd.LIGHTING2.id4 = sID & 0xff;
		defaultname = "Bloc input ";
	}
	else
	{ // it's a wireless switch (like EnOcean) :
		lcmd.LIGHTING2.id1 = bufferdata[0] & 0xff;
		lcmd.LIGHTING2.id2 = bufferdata[1] & 0xff;
		lcmd.LIGHTING2.id3 = bufferdata[2] & 0xff;
		lcmd.LIGHTING2.id4 = bufferdata[3] & 0xff;
		defaultname = "Wireless switch";
	}

	int CodeNumber = codetouche & 0x7F;
	lcmd.LIGHTING2.unitcode = CodeNumber;
	if (codetouche & 0x80)
		lcmd.LIGHTING2.cmnd = light2_sOn;
	else
		lcmd.LIGHTING2.cmnd = light2_sOff;
	lcmd.LIGHTING2.level = 0;
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	// default name creation :

	std::ostringstream convert; // stream used for the conversion
	convert << CodeNumber;
	defaultname += convert.str();

	sDecodeRXMessage(this, (const unsigned char*)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

// For listing of detected blocs :
void USBtin_MultiblocV8::BlocList_GetInfo(const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned long sID = (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	int i = 0;
	bool BIT_FIND_BLOC = false;
	int IndexBLoc = 0;
	unsigned long Rqid = 0;

	for (i = 0; i < MAX_NUMBER_BLOC; i++)
	{
		if (m_BlocList_CAN[i].BlocID == sID)
		{
			BIT_FIND_BLOC = true;
			IndexBLoc = i;
			break;
		}
	}
	// if the blocs allready exist :
	if (BIT_FIND_BLOC == true)
	{
		// update bloc values:
		m_BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
		m_BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
		m_BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
		m_BlocList_CAN[IndexBLoc].CongifurationCrc = (bufferdata[3] << 8) + bufferdata[4];
		m_BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
		m_BlocList_CAN[IndexBLoc].NbAliveFrameReceived++;
	}
	else
	{
		i = 0;
		// or just been detected :
		for (i = 0; i < MAX_NUMBER_BLOC; i++)
		{
			if (m_BlocList_CAN[i].BlocID == 0)
			{
				m_BlocList_CAN[i].BlocID = sID;
				m_BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
				m_BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
				m_BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
				m_BlocList_CAN[IndexBLoc].CongifurationCrc = (bufferdata[3] << 8) + bufferdata[4];
				m_BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
				m_BlocList_CAN[IndexBLoc].NbAliveFrameReceived = 1;
				Log(LOG_NORM, "MultiblocV8: new bloc detected: %s Coding: %d network: %d", getBlocnameFromIndex(RefBloc), Codage, Ssreseau);

				// first, when first is detected we creates a general reset switch:
				unsigned long generalID = type_COMMANDE_ETAT_BLOC << SHIFT_TYPE_TRAME; // creates a unique Reset Switch to restart all presents blocks
				InsertUpdateControlSwitch(generalID, BLOC_STATES_RESET, "RESET ALL MULTIBLOC V8 Blocks");

				// checking if we must send request, to refresh the hardware in domoticz devices :
				switch (RefBloc)
				{
				case BLOC_7:
					Rqid = (type_E_ANA_1_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					Rqid = (type_E_ANA_5_TO_8 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					Rqid = (type_E_TOR << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					Rqid = (type_STATE_S_TOR_1_TO_2 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					// send IBS request...
					if (m_BOOL_DebugInMultiblocV8 == true)
					{
						Log(LOG_NORM, "MultiblocV8: Send IBS request to: %s", GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau).c_str());
					}
					Rqid = (type_IBS << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					break;

				case BLOC_9:
					// send IBS request...
					if (m_BOOL_DebugInMultiblocV8 == true)
					{
						Log(LOG_NORM, "MultiblocV8: Send IBS request to: %s", GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau).c_str());
					}
					Rqid = (type_IBS << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);

					// bloc SFSP M / E / 9  1 analog input avec 6 outputs
				case BLOC_SFSP_M:
				case BLOC_SFSP_E:
					// requete analogique (tension alim) / analog send request for Power tension of SFSP Blocs
					Rqid = (type_E_ANA_1_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					// Envoi 6 requetes STOR vers les SFSP : / sending 3 request to obtains states of the 6 outputs
					Rqid = (type_STATE_S_TOR_1_TO_2 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					Rqid = (type_STATE_S_TOR_3_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);
					Rqid = (type_STATE_S_TOR_5_TO_6 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
					SendRequest(Rqid);

					// Créates 3 switch for Learning, Learning Exit and Clearing switches store into blocs
					// defaultname will contain the bloc adress source (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
					std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
					std::string defaultnamenormal = defaultname + " LEARN EXIT";
					std::string defaultnamelearn = defaultname + " LEARN ENTRY";
					std::string defaultnameclear = defaultname + " CLEAR ALL";
					std::string defaultnamenextlearning = defaultname + " NEXT LEARNING OUTPUT";

					unsigned long sID_CommandBase = sID + (type_COMMANDE_ETAT_BLOC << SHIFT_TYPE_TRAME);
					InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_LEARNING_STOP, defaultnamenormal);
					InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_LEARNING, defaultnamelearn);
					InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_CLEARING, defaultnameclear);
					break;
				}
				break;
			}
		}
	}
}

void USBtin_MultiblocV8::InsertUpdateControlSwitch(const int NodeID, const int ChildID, const std::string& defaultname)
{

	// make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)", m_HwdID, szIdx, ChildID,
		int(pTypeLighting2), int(sTypeAC));

	if (result.empty())
	{
		m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
			"VALUES (%d,'%q',%d,%d,%d,%d,0, 12,255,'%q',0,' ')",
			m_HwdID, szIdx, ChildID, pTypeLighting2, sTypeAC, int(STYPE_PushOn), defaultname.c_str());
	}
	else
	{ // sinon on remet à 0 les état ici
		m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)", 0, m_HwdID, szIdx, pTypeLighting2,
			sTypeAC, ChildID);
	}
}

// call every 3 sec...
void USBtin_MultiblocV8::BlocList_CheckBloc()
{
	int i;
	// int RefBlocAlive = 0;
	unsigned long Rqid = 0;
	int RefBloc;
	char Codage;
	char Subnetwork;

	for (i = 0; i < MAX_NUMBER_BLOC; i++)
	{
		if (m_BlocList_CAN[i].BlocID != 0)
		{ // Si présence d'un ID
			// we extract the blocs reference :
			// RefBlocAlive = (( m_BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
			RefBloc = (m_BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
			Codage = (m_BlocList_CAN[i].BlocID & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE;
			Subnetwork = m_BlocList_CAN[i].BlocID & MSK_SRES_MODULE;
			// and check the bloc state :
			if (m_BlocList_CAN[i].Status == BLOC_NOTALIVE && m_BlocList_CAN[i].NbAliveFrameReceived > 0)
			{
				// le bloc a été perdu / bloc lost...
				// Log(LOG_ERROR,"MultiblocV8: Bloc Lost with ref #%d# ",RefBloc);
				Log(LOG_ERROR, "MultiblocV8: bloc lost: %s Coding: %d Network: %d", getBlocnameFromIndex(RefBloc), Codage, Subnetwork);
				m_BlocList_CAN[i].NbAliveFrameReceived = 0;
			}
			else
			{ // le bloc est en vie / Alive !
				// on vérifie si il y'a des requetes automatique à envoyer : / checking if we must send request
				// Log(LOG_NORM,"MultiblocV8: BlocAlive with ref #%d# ",RefBloc);
				switch (RefBloc)
				{
				case BLOC_7:
					if (m_BOOL_TaskAGo == true)
					{
						Rqid = (type_E_ANA_1_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid = (type_E_ANA_5_TO_8 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
					}
					if (m_BOOL_TaskRqEtorGo == true)
					{
						Rqid = (type_E_TOR_1_TO_64 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
					}
					if (m_BOOL_TaskRqStorGo == true)
					{
						m_BlocList_CAN[i].ForceUpdateSTOR[0] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[1] = true;
						Rqid = (type_STATE_S_TOR_1_TO_2 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
					}
					break;
				case BLOC_SFSP_M:
				case BLOC_SFSP_E:
				case BLOC_9:
					if (m_BOOL_TaskAGo == true)
					{
						Rqid = (type_E_ANA_1_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
					}
					if (m_BOOL_TaskRqStorGo == true)
					{
						m_BlocList_CAN[i].ForceUpdateSTOR[0] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[1] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[2] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[3] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[4] = true;
						m_BlocList_CAN[i].ForceUpdateSTOR[5] = true;
						Rqid = (type_STATE_S_TOR_1_TO_2 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid = (type_STATE_S_TOR_3_TO_4 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid = (type_STATE_S_TOR_5_TO_6 << SHIFT_TYPE_TRAME) + m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
					}
					break;
				}
				m_BlocList_CAN[i].Status = BLOC_NOTALIVE; // RAZ ici de l'info toutes les 3 sec !
			}
		}
	}
	m_BOOL_TaskAGo = false;
	m_BOOL_TaskRqStorGo = false;
	m_BOOL_TaskRqEtorGo = false;
}

// traitement sur réception trame de vie / Life frame treatment :
void USBtin_MultiblocV8::Traitement_Trame_Vie(const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8])
{
	// treatment only for a frame of 5 bytes receive !
	if (rxDLC == 5)
		BlocList_GetInfo(RefBloc, Codage, Ssreseau, bufferdata);
}

// Traitement trame Etat_Bloc / States_Bloc frame treatement  :
void USBtin_MultiblocV8::Traitement_Trame_EtatBloc(const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8])
{
	int i = 0;
	int Command = 0;
	char szDeviceID[10];
	std::vector<std::vector<std::string>> result;
	// for searching the good commands switches :
	unsigned long sID = (type_COMMANDE_ETAT_BLOC << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	sprintf(szDeviceID, "%07X", (unsigned int)sID);
	if (rxDLC == 1)
	{
		Log(LOG_NORM, "MultiblocV8: Check Etat Bloc with hardwarId: %d and id: %s receive: %d", m_HwdID, szDeviceID, bufferdata[0]);
		// search the 3 switches (LEARN ENTRY / EXIT and CLEAR) associates to this return EtatBloc frame
		// in fact we can return each switch to it's normal states after a good sequence !
		if (bufferdata[0] == BLOC_NORMAL_STATES)
		{ // retour en mode normal / return in normal mode
			for (i = 0; i < (BLOC_STATES_CLEARING + 1); i++)
			{
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)", 0, m_HwdID,
					szDeviceID, pTypeLighting2, sTypeAC, i);
				/*result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)",
				m_HwdID, szDeviceID,pTypeLighting2,sTypeAC, i);
				//if(!result.empty() ){ //if command exist in db :
					//Refresh it !
					tRBUF lcmd;
					memset(&lcmd, 0, sizeof(RBUF));
					lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
					lcmd.LIGHTING2.packettype = pTypeLighting2;
					lcmd.LIGHTING2.subtype = sTypeAC;
					lcmd.LIGHTING2.id1 = (sID>>24)&0xff;
					lcmd.LIGHTING2.id2 = (sID>>16)&0xff;
					lcmd.LIGHTING2.id3 = (sID>>8)&0xff;
					lcmd.LIGHTING2.id4 = sID&0xff;
					lcmd.LIGHTING2.cmnd = light2_sOff; //Off !
					lcmd.LIGHTING2.unitcode = i; //SubUnit = command index
					lcmd.LIGHTING2.level = 0; //level_value;
					lcmd.LIGHTING2.filler = 2;
					lcmd.LIGHTING2.rssi = 12;
					sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, NULL, 255);

				}*/
			}
		}
	}
}

// check if an output has changed...
bool USBtin_MultiblocV8::CheckOutputChange(unsigned long sID, int OutputNumber, bool CdeReceive, int LevelReceive)
{
	char szDeviceID[10];
	int i;
	bool returnvalue = true;
	std::vector<std::vector<std::string>> result;
	bool ForceUpdate = false;
	int slevel = 0;
	int nvalue = 0;
	unsigned long StoreIdToFind = sID & (MSK_INDEX_MODULE + MSK_CODAGE_MODULE + MSK_SRES_MODULE);
	// serching for the bloc in bloclist :
	for (i = 0; i < MAX_NUMBER_BLOC; i++)
	{
		if (m_BlocList_CAN[i].BlocID == StoreIdToFind)
		{
			// bloc trouvé on vérifie si on doit forcer l'update ou non des composants associés :
			// bloc find, check if update is necessary :
			ForceUpdate = m_BlocList_CAN[i].ForceUpdateSTOR[OutputNumber];
			m_BlocList_CAN[i].ForceUpdateSTOR[OutputNumber] = false; // RAZ ici
			break;
		}
	}
	unsigned long IdFordbSearch = StoreIdToFind;
	unsigned int outputindex = OutputNumber - 1; // because OutputNumber is the real out number
	// but we start to 0 for the management !
	switch (outputindex)
	{
	case 0:
	case 1:
		IdFordbSearch += (type_STATE_S_TOR_1_TO_2 << SHIFT_TYPE_TRAME);
		break;
	case 2:
	case 3:
		IdFordbSearch += (type_STATE_S_TOR_3_TO_4 << SHIFT_TYPE_TRAME);
		break;
	case 4:
	case 5:
		IdFordbSearch += (type_STATE_S_TOR_5_TO_6 << SHIFT_TYPE_TRAME);
		break;
	case 6:
	case 7:
		IdFordbSearch += (type_STATE_S_TOR_7_TO_8 << SHIFT_TYPE_TRAME);
		break;
	case 8:
	case 9:
		IdFordbSearch += (type_STATE_S_TOR_9_TO_10 << SHIFT_TYPE_TRAME);
		break;
	case 10:
	case 11:
		IdFordbSearch += (type_STATE_S_TOR_11_TO_12 << SHIFT_TYPE_TRAME);
		break;
	}

	sprintf(szDeviceID, "%07X", (unsigned int)IdFordbSearch);
	// Log(LOG_NORM,"MultiblocV8: Check states for output: %d with hardwarId: %d and id: %s ",OutputNumber,m_HwdID,szDeviceID);
	result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)", m_HwdID, szDeviceID,
		pTypeLighting2, sTypeAC, OutputNumber); // Unit = 1 = sortie n°1
	if (!result.empty())
	{ // if output exist in db and no forceupdate
		if (!ForceUpdate)
		{
			// check if we have a change, if not do not update it
			nvalue = atoi(result[0][1].c_str());
			slevel = atoi(result[0][2].c_str());
			// Log(LOG_NORM,"MultiblocV8: Output 1 nvalue : %d ",nvalue);
			if ((!CdeReceive) && (nvalue == 0))
				returnvalue = false; // still off, nothing to do
			else
			{
				// Check Level changed
				// Log(LOG_NORM,"MultiblocV8: Output 1 slevel : %d ",slevel);
				// level check only if cde and states = 1
				if (CdeReceive && (nvalue > 0))
				{
					if (slevel == LevelReceive)
						returnvalue = false;
				}
			}
		}
		/*else{
			Log(LOG_NORM,"MultiblocV8: ForceUpdate is active for this output, we are maybe just started system !?");
		}*/
	}
	else
	{
		Log(LOG_ERROR, "MultiblocV8: No output find in db for: %s", szDeviceID);
	}
	// Log(LOG_NORM,"MultiblocV8: Check states Output: %d Actual States/Lvl: %d / %d Compare to: %d / %d Update?: %d",OutputNumber,CdeReceive,LevelReceive,nvalue,slevel,returnvalue);
	return returnvalue;
}

// store information outputs new states...
void USBtin_MultiblocV8::OutputNewStates(unsigned long sID, int OutputNumber, bool Comandeonoff, int Level)
{
	double rlevel = (15.0 / 255) * Level;
	int level = int(rlevel);
	// Extract the RefBloc Type
	int RefBloc = (uint16_t)((sID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
	int Codage = (uint8_t)((sID & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE);
	int Ssreseau = (uint8_t)((sID & MSK_SRES_MODULE) >> SHIFT_SRES_MODULE);
	// if (RefBloc >= NomRefBloc.size())
	//	return;

	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = (sID >> 24) & 0xff;
	lcmd.LIGHTING2.id2 = (sID >> 16) & 0xff;
	lcmd.LIGHTING2.id3 = (sID >> 8) & 0xff;
	lcmd.LIGHTING2.id4 = sID & 0xff;

	if (Comandeonoff == true)
		lcmd.LIGHTING2.cmnd = light2_sOn;
	else
		lcmd.LIGHTING2.cmnd = light2_sOff;

	lcmd.LIGHTING2.unitcode = OutputNumber & 0xff; // SubUnit;	//output number
	lcmd.LIGHTING2.level = level;		       // level_value;
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	// default name creation :
	// defaultname will contain the bloc adress source (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
	std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
	defaultname += " output S";
	std::ostringstream convert; // stream used for the conversion
	convert << OutputNumber;
	defaultname += convert.str();

	sDecodeRXMessage(this, (const unsigned char*)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

// The STOR Frame always contain a maximum of 2 stor States. 4 Low bytes = STOR 1 / 4 high bytes = STOR2
//( etc... for STOR3-4 ///  5/6  /// 7/8 ...)
void USBtin_MultiblocV8::Traitement_Etat_S_TOR_Recu(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	// char Subtype = 0;
	int level_value = 0;
	unsigned long sID = (FrameType << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;

	bool OutputCde;
	bool IsBlink;
	// bool OutputPWM;

	switch (FrameType)
	{
	case type_STATE_S_TOR_1_TO_2:
		if (bufferdata[2] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;

		// if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[0]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 1, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 1, OutputCde, level_value);
		}

		if (bufferdata[2] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,1,IsBlink);

		if (bufferdata[6] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;

		// if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[4]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 2, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 2, OutputCde, level_value);
		}

		if (bufferdata[6] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,2,IsBlink);
		break;

	case type_STATE_S_TOR_3_TO_4:
		if (bufferdata[2] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;
		// if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[0]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 3, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 3, OutputCde, level_value);
		}

		if (bufferdata[2] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,3,IsBlink);

		if (bufferdata[6] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;
		// if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[4]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 4, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 4, OutputCde, level_value);
		}

		if (bufferdata[6] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,4,IsBlink);
		break;

	case type_STATE_S_TOR_5_TO_6:
		if (bufferdata[2] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;
		// if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[0]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 5, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 5, OutputCde, level_value);
		}

		if (bufferdata[2] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,5,IsBlink);

		if (bufferdata[6] & 0x01)
			OutputCde = true;
		else
			OutputCde = false;
		// if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
		// else OutputPWM = false;
		level_value = bufferdata[4]; // variable niveau (lvl) 0-254 à convertir en 0 - 100%
		level_value /= 16;
		if (level_value > 15)
			level_value = 15;
		if (CheckOutputChange(sID, 6, OutputCde, level_value) == true)
		{ // on met à jours si nécessaire !
			OutputNewStates(sID, 6, OutputCde, level_value);
		}

		if (bufferdata[6] & 0x02)
			IsBlink = true;
		else
			IsBlink = false;
		// SetOutputBlinkInDomoticz( sID,6,IsBlink);
		break;
	}
}

void USBtin_MultiblocV8::SetOutputBlinkInDomoticz(unsigned long sID, int OutputNumber, bool Blink)
{
	int i;
	unsigned long StoreIdToFind = sID & (MSK_INDEX_MODULE + MSK_CODAGE_MODULE + MSK_SRES_MODULE);
	// serching for the bloc to :
	for (i = 0; i < MAX_NUMBER_BLOC; i++)
	{
		if (m_BlocList_CAN[i].BlocID == StoreIdToFind)
		{
			// bloc trouvé:
			// we extract the blocs reference :
			int RefBlocAlive = ((m_BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
			switch (RefBlocAlive)
			{ // Switch because the Number of output can be different for over ref blocks !
			case BLOC_SFSP_M:
			case BLOC_SFSP_E:
			case BLOC_9:
				// 6 outputs on sfsp blocks //OutputNumber
				m_BlocList_CAN[i].IsOutputBlink[OutputNumber] = Blink;
				break;
			}
		}
	}
}

/*
 * Voltage is in 1/10 volt receive from Can
 *
 */
void USBtin_MultiblocV8::StoreSupplyVoltage(int sID, int VoltageLevel, std::string defaultname) {
	int percent = ((VoltageLevel * 100) / 125);
	float voltage = (float)VoltageLevel / 10;
	// defaultname will contain the bloc adress source (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
	defaultname += " Supply";
	SendVoltageSensor(sID, 1, percent, voltage, defaultname);
}

// traitement d'une trame info analogique reçue
void USBtin_MultiblocV8::Traitement_E_ANA_Recu(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned int sID = (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;

	switch (RefBloc)
	{
	case BLOC_7:
		if (FrameType == type_E_ANA_1_TO_4) {
			//we have receive analog input 1 to 4 :
			for (uint8_t i = 0; i < 4; i++) {
				int index = i * 2;
				int value = (bufferdata[index] << 8) + bufferdata[index + 1];

				// defaultname will contain the bloc adress source (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
				std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
				defaultname += " Eana";
				defaultname += std::to_string(i + 1);
				SendCustomSensor(sID, i, 255, static_cast<float>(value), defaultname, "", 12);
			}
		}
		else if (FrameType == type_E_ANA_5_TO_8) {
			//we have receivee eanalog input 5 to 6 and supply voltage on 7
			for (uint8_t i = 0; i < 2; i++) {
				int index = i * 2;
				int value = (bufferdata[index] << 8) + bufferdata[index + 1];

				// defaultname will contain the bloc adress source (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
				std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
				defaultname += " Eana";
				defaultname += std::to_string(i + 5);
				SendCustomSensor(sID, (i + 5), 255, static_cast<float>(value), defaultname, "", 12);
			}

			int VoltageLevel = bufferdata[4];
			VoltageLevel <<= 8;
			VoltageLevel += bufferdata[5];
			std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
			StoreSupplyVoltage(sID, VoltageLevel, defaultname);
		}
		break;

		//analog information is only on D0+D1 for this bloc: (supply voltage in 1/10Volt)
	case BLOC_SFSP_M:
	case BLOC_SFSP_E:
	case BLOC_9:
		if (m_BOOL_DebugInMultiblocV8 == true)
		{
			Log(LOG_NORM, "MultiblocV8: receive ANA (alimentation) %s: D0: %d D1: %d#", GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau).c_str(), bufferdata[0], bufferdata[1]);
		}
		int VoltageLevel = bufferdata[0];
		VoltageLevel <<= 8;
		VoltageLevel += bufferdata[1];
		// Log(LOG_NORM,"MultiblocV8: receive ANA1 (alimentation) sfsp: #%d# ",VoltageLevel);
		std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
		StoreSupplyVoltage(sID, VoltageLevel, defaultname);
		break;
	}
}

void USBtin_MultiblocV8::Traitement_E_TOR_Recu(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned int sID = (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;

	switch (RefBloc)
	{
	case BLOC_7: //Bloc 7 is abble to send itss 3 digital input states
		//only the first byte contains informations
		std::string sourcename = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);
		sourcename += " E";
		uint8_t State = bufferdata[0] & 0x01;
		uint8_t level = 0;
		if (State > 0) level = 100;
		std::string defaultname = sourcename + "1";
		SendGeneralSwitch(sID, 1, 100, State, level, defaultname, "", 12);

		State = (bufferdata[0] >> 1 & 0x01);
		level = 0;
		if (State > 0) level = 100;
		defaultname = sourcename + "2";
		SendGeneralSwitch(sID, 2, 100, State, level, defaultname, "", 12);

		State = (bufferdata[0] >> 2 & 0x01);
		level = 0;
		if (State > 0) level = 100;
		defaultname = sourcename + "3";
		SendGeneralSwitch(sID, 3, 100, State, level, defaultname, "", 12);


		break;

	}
}

// Envoi d'une trame suite à une commande dans domoticz...
// sending Frame in response to a domoticz action :
// frame sent to drive standard stor outputs only for supported bloc (like bloc sfsp, bllloc 9, bloc 7)
bool USBtin_MultiblocV8::WriteToHardware(const char* pdata, const unsigned char length)
{
	const tRBUF* pSen = reinterpret_cast<const tRBUF*>(pdata);
	char szDeviceID[10];
	char DataToSend[16];
	unsigned char packettype = pSen->ICMND.packettype;
	unsigned char packetsubtype = pSen->ICMND.subtype;
	long sID_EnBase;
	int FrameType;
	int ReferenceBloc;
	int Codage;
	int Ssreseau;
	bool bIsOn;
	bool bIsOff;
	unsigned char KeyCode;

	if (packettype == pTypeLighting2)
	{
		// if it's light command
		// retreive the ID information of the blocs (Rebloc+Codage+Ssréseau ):
		sID_EnBase = (pSen->LIGHTING2.id1 << 24) + (pSen->LIGHTING2.id2 << 16) + (pSen->LIGHTING2.id3 << 8) + (pSen->LIGHTING2.id4);
		FrameType = (sID_EnBase & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
		ReferenceBloc = (sID_EnBase & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
		Codage = (sID_EnBase & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE;
		Ssreseau = (sID_EnBase & MSK_SRES_MODULE) >> SHIFT_SRES_MODULE;
		bIsOn = (pSen->LIGHTING2.cmnd == light2_sOn);
		bIsOff = (pSen->LIGHTING2.cmnd == light2_sOff);

		Log(LOG_NORM, "MultiblocV8: WriteToHardware for: 0x%08X", (int)sID_EnBase);

		// only if thread is active: (avoid CAN Tx error if thread is down).
		if (m_thread)
		{
			// test to know wich type of frame to send...
			if (FrameType == type_STATE_S_TOR_1_TO_2 || FrameType == type_STATE_S_TOR_3_TO_4 || FrameType == type_STATE_S_TOR_5_TO_6 || FrameType == type_STATE_S_TOR_7_TO_8 ||
				FrameType == type_STATE_S_TOR_9_TO_10 || FrameType == type_STATE_S_TOR_11_TO_12)
			{ // if it's directly a STOR Command...

				unsigned long sID = (type_CMD_S_TOR << SHIFT_TYPE_TRAME) + (sID_EnBase & (MSK_INDEX_MODULE + MSK_CODAGE_MODULE + MSK_SRES_MODULE));
				sprintf(szDeviceID, "%08X", (unsigned int)sID);
				unsigned int OutputNumber = (pSen->LIGHTING2.unitcode) - 1; // output number for command
				unsigned int Command = 0;
				unsigned int Reserve = 0;
				int iLevel = pSen->LIGHTING2.level;

				if (iLevel > 15)
					iLevel = 15;
				float fLevel = (255.0F / 15.0F) * float(iLevel);
				if (fLevel > 254.0F)
					fLevel = 255.0F;
				iLevel = int(fLevel);

				if (pSen->LIGHTING2.cmnd == light2_sSetLevel)
				{
					Command = 0x11; // ON + SetLevel
				}
				else if (bIsOn)
				{
					Command = 0x01;
				}
				else
				{ // Off...
					Command = 0;
				}

				int tDataToSend = (OutputNumber << 24) + (Command << 16) + (Reserve << 8) + iLevel;

				sprintf(DataToSend, "%08X", tDataToSend);
				std::string szTrameToSend = "T"; //
				szTrameToSend += szDeviceID;
				szTrameToSend += "4";
				szTrameToSend += DataToSend;
				if (m_BOOL_DebugInMultiblocV8 == true)
				{
					Log(LOG_NORM, "MultiblocV8: Sending Frame: %s", szTrameToSend.c_str());
				}
				writeFrame(szTrameToSend);
				return true;
			}
			else if (FrameType == type_SFSP_SWITCH) // if it's a SFSP Switch...
			{
				// no sending frame for switch created by the CAN, they are real switch not virtual ! it's like enocean
				// if this is a switch created in domoticz (virtual switch), send it on the CanBus !
				if (ReferenceBloc == BLOC_DOMOTICZ && Codage == 0 && Ssreseau == 0)
				{ // this is a virtual switch !
					if (bIsOn || bIsOff)
					{ // If commande ON of OFF... for a sfsp switch no difference (we send a
						// use directly the baseId to send a "press" command, the "release" command is send automatically
						KeyCode = (pSen->LIGHTING2.unitcode); // the Key Code is the unitcode
						char KeyCodeAppui = KeyCode;
						KeyCodeAppui |= 0x80; // send a "press" command
						USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(sID_EnBase, KeyCodeAppui);
						FillBufferSFSP_toSend(sID_EnBase, KeyCode); // send the Release sfsp on Can
						return true;
					}

					if (pSen->LIGHTING2.cmnd == light2_sSetLevel)
					{
						// to do : if user set the level we must send the command By Outpu direct command and not by SFSP Frame
						Log(LOG_ERROR, "MultiblocV8: Dimmer level not supported !");
						return false;
					}
				}
				else
				{
					Log(LOG_ERROR, "MultiblocV8: Can not switch with this DeviceID,this is not a virtual switch...");
					return false;
				}
			}
			else if (FrameType == type_COMMANDE_ETAT_BLOC)
			{ // specific command send to blocks
				if (ReferenceBloc == BLOC_SFSP_M || ReferenceBloc == BLOC_SFSP_E || ReferenceBloc == BLOC_9)
				{
					// on each "push on" switch send a bloc command starting with learn on OUTPUT 1
					// to OUTPUT 6 and return to Normal State Bloc.
					// In learning mode the output return states : ON with BLINK MODE ON(1sec)
					// but we can't show the blink mode in domoticz on a SWITCH because the output will only turn on !
					char Commande = (pSen->LIGHTING2.unitcode);
					// sID_EnBase + commande
					USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(sID_EnBase, Commande);
					// Lerning mode entrance : automatically reset and start on Output 1 !
					/*if( Commande == BLOC_STATES_LEARNING ){
						m_V8_INT_PushCountLearnMode = 0;
					} */
					return true;
				}
				Log(LOG_ERROR, "MultiblocV8: Error Command BLoc not allowed !");
				return false;
			}
			else if (FrameType == type_SFSP_LearnCommand)
			{ // specific command for sfsp to jump from one output to the next
				if (ReferenceBloc == BLOC_SFSP_M || ReferenceBloc == BLOC_SFSP_E || ReferenceBloc == BLOC_9)
				{
					char Commande = (pSen->LIGHTING2.unitcode);
					USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(sID_EnBase, Commande); //
					return true;
				}
				Log(LOG_ERROR, "MultiblocV8: Error Command SFSP Learn not allowed !");
				return false;
			}
		}
		else
		{
			Log(LOG_ERROR, "MultiblocV8: This Can engine is disabled, please re-check MultiblocV8...");
		}
	}
	else
	{
		Log(LOG_ERROR, "MultiblocV8: This type of switch is not yet supported in MultiblocV8: %c", packettype);
	}
	return false;
}

/* methods to generate an sfsp switch action on the Can BUS
 * Used by virtual sitches created by manual add on this hardware, type On/Off EnOcean
 */
void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(long sID_ToSend, char CodeTouche)
{
	char szDeviceID[10];
	char DataToSend[16];
	sprintf(szDeviceID, "%08X", (unsigned int)sID_ToSend);
	// unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	// differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend, "%02X", (unsigned char)CodeTouche);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "5";	     // DLC always to 5 for SFSP_SWITCH Frame
	szTrameToSend += "00000001"; // always set to 1 because it's an internal switch (from domoticz)
	szTrameToSend += DataToSend; // contain data code + send On information
	if (m_BOOL_DebugInMultiblocV8 == true)
	{
		Log(LOG_NORM, "MultiblocV8: Sending Frame: %s", szTrameToSend.c_str());
	}
	writeFrame(szTrameToSend);
}

void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(long sID_ToSend, char Commande)
{
	char szDeviceID[10];
	char DataToSend[16];
	sprintf(szDeviceID, "%08X", (unsigned int)sID_ToSend);
	// unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	// differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend, "%02X", (unsigned char)Commande);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "1";	     // DLC always to 5 for SFSP_SWITCH Frame
	szTrameToSend += DataToSend; // contain data code + send On information
	if (m_BOOL_DebugInMultiblocV8 == true)
	{
		Log(LOG_NORM, "MultiblocV8: Sending BlocState command: %s", szTrameToSend.c_str());
	}
	writeFrame(szTrameToSend);
}

/* to send a Learn command to a bloc that support SFSP protocol */
void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(long baseID_ToSend, char Commande)
{
	char szDeviceID[10];
	char DataToSend[16];
	unsigned long sID = (type_SFSP_LearnCommand << SHIFT_TYPE_TRAME) + (baseID_ToSend & (MSK_INDEX_MODULE + MSK_CODAGE_MODULE + MSK_SRES_MODULE));
	sprintf(szDeviceID, "%08X", (unsigned int)sID);
	// unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	// differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend, "%02X", (unsigned char)Commande);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "1";	     // DLC always to 1 for this frame
	szTrameToSend += DataToSend; // contain data code for the bloc state to change
	if (m_BOOL_DebugInMultiblocV8 == true)
	{
		Log(LOG_NORM, "MultiblocV8: Sending SFSP learn command: %s", szTrameToSend.c_str());
	}
	writeFrame(szTrameToSend);
	/*
	char RefBloc = (baseID_ToSend & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE; //retreive the refblock

	switch(RefBloc){
		case BLOC_SFSP_M :
		case BLOC_SFSP_E :
		case BLOC_9 :
			//6 push max and automatically go out of Learning Mode !
			if( m_V8_INT_PushCountLearnMode >= 6 ){
				m_V8_INT_PushCountLearnMode = 0;
				//return to normal states...
				USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(baseID_ToSend,BLOC_STATES_LEARNING_STOP);
			}
			else m_V8_INT_PushCountLearnMode++;
			break;
	}*/
}

int USBtin_MultiblocV8::getIndexFromBlocname(std::string blocname)
{
	for (size_t i = 0; i < NomRefBloc.size(); i++)
	{
		if (NomRefBloc[i] == blocname)
		{
			return i;
		}
	}
	return 0;
}

const char* USBtin_MultiblocV8::getBlocnameFromIndex(int indexreference)
{
	if (indexreference >= (int)NomRefBloc.size())
	{
		return "UNKNOWN";
	}
	else
	{
		return NomRefBloc[indexreference];
	}
}

// processing of information frame IBS informations
void USBtin_MultiblocV8::Traitement_IBS(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	uint32_t sID = (FrameType << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	sID <<= 4; // to reserved lowbyte area for id

	uint32_t ChildId = 0;
	// defaultname will contain the bloc adress source of the IBS (ex: BLOC_9@1_0 for BLOC_9 coding 1 in the subnetwork 0 (base))
	std::string defaultname = GetCompleteBlocNameSource(RefBloc, Codage, Ssreseau);

	if (FrameType == type_IBS_1_1 || FrameType == type_IBS_1_2 || type_IBS_1_3)
	{
		defaultname += " IBS1: ";
		ChildId = 1;
	}
	else if (FrameType == type_IBS_2_1 || FrameType == type_IBS_2_2 || type_IBS_2_3)
	{
		defaultname += " IBS2: ";
		ChildId = 2;
	}
	else if (FrameType == type_IBS_3_1 || FrameType == type_IBS_3_2 || type_IBS_3_3)
	{
		defaultname += " IBS3: ";
		ChildId = 3;
	}
	else if (FrameType == type_IBS_4_1 || FrameType == type_IBS_4_2 || type_IBS_4_3)
	{
		defaultname += " IBS4: ";
		ChildId = 4;
	}
	else if (FrameType == type_IBS_5_1 || FrameType == type_IBS_5_2 || type_IBS_5_3)
	{
		defaultname += " IBS5: ";
		ChildId = 5;
	}
	else if (FrameType == type_IBS_6_1 || FrameType == type_IBS_6_2 || type_IBS_6_3)
	{
		defaultname += " IBS6: ";
		ChildId = 6;
	}

	// Switch to determine data treatement
	if (FrameType == type_IBS_1_1 || FrameType == type_IBS_2_1 || FrameType == type_IBS_3_1 || FrameType == type_IBS_4_1 || FrameType == type_IBS_5_1 || FrameType == type_IBS_6_1)
	{

		if (m_BOOL_DebugInMultiblocV8 == true) {
			Log(LOG_NORM, "MultiblocV8: %s IBS_Frametype 1 [D0: %02X D1: %02X D2: %02X D3: %02X D4: %02X D5: %02X]", defaultname.c_str(), bufferdata[0], bufferdata[1], bufferdata[2],
				bufferdata[3], bufferdata[4], bufferdata[5]);
		}

		// frame type 1 contain :
		// D0+D1 voltage (/100eme volt)
		// D2+D3 current (/10eme A with offset +20000, so range from -2000.0 to +2000.0 A)
		// D4 temperature (°C with offset +40, so range from -40 to +60°C)
		// D5 central tape voltage in case of IBS24V
		int VoltageLevel = bufferdata[1];
		VoltageLevel <<= 8;
		VoltageLevel += bufferdata[0];
		float voltage = (float)VoltageLevel / 100;
		std::string voltage_name = defaultname;
		voltage_name += " Voltage";
		SendIBSVoltageSensor(sID + ChildId, ChildId, 255, voltage, voltage_name);

		int CurrentValue = bufferdata[3];
		CurrentValue <<= 8;
		CurrentValue += bufferdata[2];
		CurrentValue -= 20000;
		float current = (float)CurrentValue / 10;
		std::string current_name = defaultname;
		current_name += " Current";
		SendIBSCurrentSensor(sID + ChildId, ChildId, current, current_name);

		int temperature = bufferdata[4];
		temperature -= 40;
		std::string temperature_name = defaultname;
		temperature_name += " Temperature";
		SendIBTemperatureSensor(((sID >> 16) & 0xFFF0) + (ChildId), ChildId, (float)temperature, temperature_name);

		int ibs24_central_voltage = bufferdata[5];
		float central_voltage = (float)ibs24_central_voltage / 10;
		std::string central_volta_name = defaultname;
		central_volta_name += " Central Volt (IBS24V only)";
		SendIBSVoltageSensor(sID + ChildId + 1, ChildId + 1, 255, central_voltage, central_volta_name);

		// compute time left with actual current and last AvailableAh and DischargeableAh known
		ComputeTimeLeft(RefBloc, Codage, Ssreseau, ChildId, defaultname);
	}
	else if (FrameType == type_IBS_1_2 || FrameType == type_IBS_2_2 || FrameType == type_IBS_3_2 || FrameType == type_IBS_4_2 || FrameType == type_IBS_5_2 || FrameType == type_IBS_6_2)
	{

		if (m_BOOL_DebugInMultiblocV8 == true) {
			Log(LOG_NORM, "MultiblocV8: %s IBS_Frametype 2 [D0: %02X D1: %02X D2: %02X D3: %02X D4: %02X D5: %02X]", defaultname.c_str(), bufferdata[0], bufferdata[1], bufferdata[2],
				bufferdata[3], bufferdata[4], bufferdata[5]);
		}
		int Soc = (int)bufferdata[0];
		std::string soc_name = defaultname;
		soc_name += " SoC";
		SendPercentageSensor(sID + ChildId, ChildId, 255, static_cast<float>(Soc), soc_name);

		int Soh = (int)bufferdata[1];
		std::string soh_name = defaultname;
		soh_name += " SoH";
		SendPercentageSensor(sID + ChildId + 1, ChildId, 255, static_cast<float>(Soh), soh_name);

		int DischargeableAh = bufferdata[2];
		std::string dAh_name = defaultname;
		dAh_name += " Dischargeable";
		SendCustomSensor(((sID + ChildId) >> 8), (sID + ChildId) & 0xff, 255, static_cast<float>(DischargeableAh), dAh_name, "Ah");

		int AvailableAh = bufferdata[3];
		std::string AAh_name = defaultname;
		AAh_name += " Available";
		SendCustomSensor(((sID + ChildId + 1) >> 8), (sID + ChildId + 1) & 0xff, 255, static_cast<float>(AvailableAh), AAh_name, "Ah");

		int FunctionId = bufferdata[5];
		FunctionId <<= 8;
		FunctionId += bufferdata[4];
		std::stringstream stream;
		stream << std::hex << FunctionId;
		std::string result(stream.str());
		std::string functionid_name = defaultname;
		functionid_name += " Function_Id";
		SendTextSensor(((sID + ChildId + 1) >> 8), (sID + ChildId + 1) & 0xff, 255, result, functionid_name);

		// compute time left with actual current and last AvailableAh and DischargeableAh known
		ComputeTimeLeft(RefBloc, Codage, Ssreseau, ChildId, defaultname);
	}
	else if (FrameType == type_IBS_1_3 || FrameType == type_IBS_2_3 || FrameType == type_IBS_3_3 || FrameType == type_IBS_4_3 || FrameType == type_IBS_5_3 || FrameType == type_IBS_6_3)
	{

		if (m_BOOL_DebugInMultiblocV8 == true) {
			Log(LOG_NORM, "MultiblocV8: %s IBS_Frametype 3 [D0: %02X D1: %02X D2: %02X D3: %02X]", defaultname.c_str(), bufferdata[0], bufferdata[1], bufferdata[2], bufferdata[3]);
		}

		int NominalCapacity = bufferdata[1];
		NominalCapacity <<= 8;
		NominalCapacity += bufferdata[0];
		std::string settings_name = defaultname;
		settings_name += "Ibs settings";

		std::string result = "Ibs: " + GetIBSTypeName(bufferdata[3]);
		result += " / Battery type: " + GetBatteryTypeName(bufferdata[2]);
		result += " / Nominal Capacity " + std::to_string(NominalCapacity) + " Ah";

		SendTextSensor(((sID + ChildId + 1) >> 8), (sID + ChildId + 1) & 0xff, 255, result, settings_name);
	}
}

void USBtin_MultiblocV8::SendIBSVoltageSensor(const int NodeID, const uint32_t ChildID, const int BatteryLevel, const float Volt, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeVoltage;
	gDevice.id = ChildID;
	gDevice.intval1 = NodeID;
	gDevice.floatval1 = Volt;
	sDecodeRXMessage(this, (const unsigned char*)&gDevice, defaultname.c_str(), BatteryLevel, nullptr);
}

void USBtin_MultiblocV8::SendIBSCurrentSensor(const int NodeID, const uint8_t ChildID, float Amp, const std::string& defaultname)
{
	_tGeneralDevice gDevice;
	gDevice.subtype = sTypeCurrent;
	gDevice.id = ChildID;
	gDevice.intval1 = NodeID;
	gDevice.floatval1 = Amp;
	sDecodeRXMessage(this, (const unsigned char*)&gDevice, defaultname.c_str(), 255, nullptr);
}

void USBtin_MultiblocV8::SendIBTemperatureSensor(const int NodeID, const uint8_t ChildID, float Temp, const std::string& defaultname)
{
	RBUF tsen;
	memset(&tsen, 0, sizeof(RBUF));
	tsen.TEMP.packetlength = sizeof(tsen.TEMP) - 1;
	tsen.TEMP.packettype = pTypeTEMP;
	tsen.TEMP.subtype = sTypeTEMP5;
	// tsen.TEMP.battery_level = 9;
	tsen.TEMP.rssi = (const int)12;
	tsen.TEMP.id1 = (NodeID & 0xff00) >> 8;
	tsen.TEMP.id2 = NodeID & 0xff;
	tsen.TEMP.tempsign = (Temp >= 0) ? 0 : 1;
	int at10 = round(std::abs(Temp * 10.0F));
	tsen.TEMP.temperatureh = (BYTE)(at10 / 256);
	at10 -= (tsen.TEMP.temperatureh * 256);
	tsen.TEMP.temperaturel = (BYTE)(at10);
	sDecodeRXMessage(this, (const unsigned char*)&tsen.TEMP, defaultname.c_str(), 255, nullptr);
}

std::string USBtin_MultiblocV8::GetCompleteBlocNameSource(const unsigned int RefBloc, const char Codage, const char Ssreseau)
{
	std::string defaultname = getBlocnameFromIndex(RefBloc);
	defaultname += "@";
	defaultname += std::to_string(Codage);
	defaultname += "_";
	defaultname += std::to_string(Ssreseau);
	return defaultname;
}

std::string USBtin_MultiblocV8::GetBatteryTypeName(int index)
{
	if (index >= (int)BatteryType.size())
	{
		return "UNKNOWN";
	}
	else
	{
		return BatteryType[index];
	}
}

std::string USBtin_MultiblocV8::GetIBSTypeName(int index)
{
	if (index >= (int)IBSType.size())
	{
		return "UNKNOWN";
	}
	else
	{
		return IBSType[index];
	}
}

void USBtin_MultiblocV8::ComputeTimeLeft(const unsigned int RefBloc, const char Codage, const char Ssreseau, const int ibsindex, const std::string& defaultname)
{
	int FrameTypeForRequest = 0;

	if (ibsindex == 1)
		FrameTypeForRequest = type_IBS_1_1;
	else if (ibsindex == 2)
		FrameTypeForRequest = type_IBS_2_1;
	else if (ibsindex == 3)
		FrameTypeForRequest = type_IBS_3_1;
	else if (ibsindex == 4)
		FrameTypeForRequest = type_IBS_4_1;
	else if (ibsindex == 5)
		FrameTypeForRequest = type_IBS_5_1;
	else if (ibsindex == 6)
		FrameTypeForRequest = type_IBS_6_1;
	uint32_t sIDforRequestCurrent = (FrameTypeForRequest << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	sIDforRequestCurrent <<= 4;
	float current = GetInformationFromId(((sIDforRequestCurrent)+ibsindex), sTypeCurrent);

	uint32_t sID = sIDforRequestCurrent;

	if (ibsindex == 1)
		FrameTypeForRequest = type_IBS_1_2;
	else if (ibsindex == 2)
		FrameTypeForRequest = type_IBS_2_2;
	else if (ibsindex == 3)
		FrameTypeForRequest = type_IBS_3_2;
	else if (ibsindex == 4)
		FrameTypeForRequest = type_IBS_4_2;
	else if (ibsindex == 5)
		FrameTypeForRequest = type_IBS_5_2;
	else if (ibsindex == 6)
		FrameTypeForRequest = type_IBS_6_2;
	uint32_t sIDforRequest = (FrameTypeForRequest << SHIFT_TYPE_TRAME) + (RefBloc << SHIFT_INDEX_MODULE) + (Codage << SHIFT_CODAGE_MODULE) + Ssreseau;
	int lastdischargeableah = (int)GetInformationFromId(((sIDforRequest << 4) + ibsindex), sTypeCustom);
	int lastavailableAh = (int)GetInformationFromId(((sIDforRequest << 4) + ibsindex + 1), sTypeCustom);

	// fordebug
	// Log(LOG_NORM, "MultiblocV8: ComputeTimeLeft with data Id: %08X",sIDforRequestCurrent);
	// Log(LOG_NORM, "MultiblocV8: ComputeTimeLeft with: %f Amp, %d DischargeableAh, %d AvailableAh",current,lastdischargeableah,lastavailableAh);

	float timeleft = TimeLeftInMinutes(current, lastdischargeableah, lastavailableAh);

	std::string timeleftD_name = defaultname;
	timeleftD_name += " Time left before discharge (40%)";
	std::string timeleftC_name = defaultname;
	timeleftC_name += " Time left before charge (100%)";

	if (timeleft < 0)
	{ // negative time indicate consumption, no charge
		timeleft = std::fabs(timeleft); //to obtain positive value in minute
		SendCustomSensor(((sID + ibsindex + 3) >> 8), (sID + ibsindex + 3) & 0xff, 255, static_cast<float>(timeleft), timeleftD_name, "minutes");
		SendCustomSensor(((sID + ibsindex + 4) >> 8), (sID + ibsindex + 4) & 0xff, 255, 0, timeleftC_name, "minutes");
	}
	else
	{ // else positive indicate current is charging
		SendCustomSensor(((sID + ibsindex + 3) >> 8), (sID + ibsindex + 3) & 0xff, 255, 0, timeleftD_name, "minutes");
		SendCustomSensor(((sID + ibsindex + 4) >> 8), (sID + ibsindex + 4) & 0xff, 255, static_cast<float>(timeleft), timeleftC_name, "minutes");
	}
}

float USBtin_MultiblocV8::TimeLeftInMinutes(float current, int DischargeableAh, int lastavailableAh)
{
	float result = 0;
	if (current < 0)
	{
		float timeleft = (float)DischargeableAh;
		timeleft -= ((float)lastavailableAh * 40 / 100);
		timeleft /= current; //timeleft in 1/100 hours so calculate time in minute:
		int hour = static_cast<int>(timeleft);
		int minute = static_cast<int>((timeleft - hour) * 60);
		result = (float)(hour * 60);
		result += minute;
		//Log(LOG_NORM, "MultiblocV8: TimeLeftInMinutes (negative) %f",result);
		return result; //return negative time to indicate time before discharge
	}
	else
	{
		float timeleft = (float)lastavailableAh - (float)DischargeableAh; // Ah to charge...
		timeleft /= current;						  // timeleft in 1/100 hours
		int hour = static_cast<int>(timeleft);
		int minute = static_cast<int>((timeleft - hour) * 60);
		result = (float)(hour * 60);
		result += minute;
		//Log(LOG_NORM, "MultiblocV8: TimeLeftInMinutes (positive) %f",result);
		return result;
	}
}

float USBtin_MultiblocV8::GetInformationFromId(int NodeId, int sType)
{
	float value = 0;
	_tGeneralDevice gDevice;
	gDevice.subtype = sType; // sTypeCustom;
	gDevice.id = NodeId;
	gDevice.intval1 = NodeId;

	std::string sTmp = std_format("%08X", gDevice.intval1);
	std::vector<std::vector<std::string>> result;
	result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d)", m_HwdID, sTmp.c_str(), int(pTypeGeneral),
		int(sType));

	if (!result.empty())
	{
		value = static_cast<float>(atof(result[0][2].c_str()));
	}
	return value;
}
