/*
File : MultiblocV8.cpp
Author : X.PONCET
Version : 1.00
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


*/
#include "stdafx.h"
#include "USBtin_MultiblocV8.h"
#include "hardwaretypes.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/SQLHelper.h"

#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

#include <bitset>			 // This is necessary to compile on Windows

#define	TIME_1sec				1000
#define	TIME_500ms				500
#define	TIME_200ms				200
#define	TIME_100ms				100
#define	TIME_50ms				50
#define	TIME_10ms				10
#define	TIME_5ms				5

#define BLOC_ALIVE				1
#define BLOC_NOTALIVE			2

#define MSK_TYPE_TRAME          0x1FFE0000
#define MSK_INDEX_MODULE        0x0001FF80
#define MSK_CODAGE_MODULE       0x00000078
#define MSK_SRES_MODULE         0x00000007

#define SHIFT_TYPE_TRAME        17
#define SHIFT_INDEX_MODULE      7
#define SHIFT_CODAGE_MODULE     3
#define SHIFT_SRES_MODULE       0

#define type_ALIVE_FRAME            0
#define type_LIFE_PHASE_FRAME       1
#define type_TP_DATA                2
#define type_TP_FLOW_CONTROL        3
#define type_DATE_TIME              4
#define type_ETAT_BLOC              5
#define type_COMMANDE_ETAT_BLOC     6

#define type_CTRL_FRAME_BLOC        256
#define type_CTRL_IO_BLOC           257

#define type_E_ANA                  258
#define type_E_ANA_1_TO_4           258
#define type_E_ANA_5_TO_8           259
#define type_E_ANA_9_TO_12          260
#define type_E_ANA_13_TO_16         261
#define type_E_ANA_17_TO_20         262
#define type_E_ANA_21_TO_24         263
#define type_E_ANA_25_TO_28         264
#define type_E_ANA_29_TO_32         265

#define type_E_TOR                  266
#define type_E_TOR_1_TO_64          266

#define type_S_TOR                  267
#define type_STATE_S_TOR_1_TO_2     267
#define type_STATE_S_TOR_3_TO_4     268
#define type_STATE_S_TOR_5_TO_6     269
#define type_STATE_S_TOR_7_TO_8     270
#define type_STATE_S_TOR_9_TO_10    271
#define type_STATE_S_TOR_11_TO_12   272
#define type_STATE_S_TOR_13_TO_14   273
#define type_STATE_S_TOR_15_TO_16   274
#define type_STATE_S_TOR_17_TO_18   275
#define type_STATE_S_TOR_19_TO_20   276
#define type_STATE_S_TOR_21_TO_22   277
#define type_STATE_S_TOR_23_TO_24   278
#define type_STATE_S_TOR_25_TO_26   279
#define type_STATE_S_TOR_27_TO_28   280
#define type_STATE_S_TOR_29_TO_30   281
#define type_STATE_S_TOR_31_TO_32   282

#define type_CMD_S_TOR              283
#define type_CONSO_S_TOR_1_TO_8     284
#define type_CONSO_S_TOR_9_TO_16    285
#define type_CONSO_S_TOR_17_TO_24   286
#define type_CONSO_S_TOR_25_TO_32   287
#define type_CONSO_S_TOR            type_CONSO_S_TOR_1_TO_8

#define type_SFSP_SWITCH            512
#define type_SFSP_LED_CMD           513
#define type_SFSP_CAPTEUR_1BS       514
#define type_SFSP_CAPTEUR_4BS       515
#define type_SFSP_SYNCHRO           516
#define type_SFSP_LearnCommand      517

#define BLOC_NORMAL_STATES			0x00
#define BLOC_STATES_OFF				0x01
#define BLOC_STATES_RESET			0x02
#define BLOC_STATES_LEARNING		0x03
#define BLOC_STATES_LEARNING_STOP	0x04
#define	BLOC_STATES_CLEARING		0x05


//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// REFERENCE INDEX
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#define BLOC_DOMOTICZ                   0x01

#define BLOC_9		                    0x0D
#define BLOC_SFSP_M                     0x14
#define BLOC_SFSP_E                     0x15

constexpr std::array<const char *, 45> NomRefBloc{
	"UNDEFINED",
	"DOMOTICZ",
	"NAVIGRAPH_VER",
	"NAVIGRAPH_HOR",
	"NAVIGRAPH_HOR_RTC",
	"NAVICOLOR",
	"NAVICOLOR_RTC",
	"BLOC_1",
	"BLOC_2",
	"BLOC_4",
	"BLOC_6",
	"BLOC_7",
	"BLOC_8",
	"BLOC_9",
	"BLOC_AU",
	"BLOC_CC",
	"BLOC_TF",
	"BLOC_AC1",
	"BLOC_X98",
	"BLOC_RESPIRE",
	"BLOC_SFSP_M",
	"BLOC_SFSP_E",
	"BLOC_SFSP_A",
	"SELECTEUR_AC1",
	"CHARGEUR_MDP",
	"MULTICOM",
	"MULTICOM_V2",
	"INTERFACE_CAN_CAN",
	"INTERFACE_CAN_CAN_V2",
	"INTERFACE_BLUETOOTH",
	"INTERFACE_LIN",
	"NAVICOLOR_GT",
	"CHARGEUR_CRISTEC",
	"BLOC_VENTIL",
	"BLOC_SIGNAL",
	"BLOC_PROTECT",
	"BLOC_X10",
	"AMPLI_ATOLL",
	"AMPLI_ATOLL_3ZONES",
	"SERVEUR_WIFI",
	"NAVICOLOR_PT",
	"NAVICOLOR_PT2",
	"CLIM_DOMETIC",
	"FACADE_CARLING",
	" ",
	// "SELECTEUR_MDP",
};

USBtin_MultiblocV8::USBtin_MultiblocV8()
{
	m_BOOL_DebugInMultiblocV8 = false;
}

USBtin_MultiblocV8::~USBtin_MultiblocV8(){
	StopThread();
}

bool USBtin_MultiblocV8::StartThread()
{
  //Id Base for manual creation switch from domoticz...
	m_V8switch_id_base = (type_SFSP_SWITCH<<SHIFT_TYPE_TRAME)+(1<<SHIFT_INDEX_MODULE)+(0<<SHIFT_CODAGE_MODULE)+0;

	m_V8minCounterBase = (60*5);
	m_V8minCounter1 = (3600*6);
	m_thread = std::make_shared<std::thread>(&USBtin_MultiblocV8::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	_log.Log(LOG_STATUS,"MultiblocV8: thread started");
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

void USBtin_MultiblocV8::ManageThreadV8(bool States)
{
	if( States == true)
		StartThread();
	else
		StopThread();
}

void USBtin_MultiblocV8::ClearingBlocList(){
	_log.Log(LOG_NORM,"MultiblocV8: clearing BlocList");
	//effacement du tableau à l'init
	for (auto &i : m_BlocList_CAN)
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
	ClearingBlocList();
	//call every 100ms.... so...
	while( !IsStopRequested(TIME_100ms))
	{
		m_V8secCounterBase++;
		if (m_V8secCounterBase >= 10)
		{
			m_V8secCounterBase = 0;
			m_V8minCounterBase--;
			m_V8minCounter1--;

			if( m_V8minCounterBase == 0 ){
				//each 5 minutes
				m_V8minCounterBase = (60*5);
				m_BOOL_TaskAGo = true;
			}

			if( m_V8minCounter1 == 0 ){
				//each 6 hours...
				m_V8minCounter1 = (3600*6);
				m_BOOL_TaskRqStorGo = true;
			}

			m_V8secCounter1++;
			if( m_V8secCounter1 >= 3 ){
				m_V8secCounter1 = 0;
				//each 3 seconds
				BlocList_CheckBloc();
			}
		}

		if( m_BOOL_SendPushOffSwitch ){ //if a Send push off switch is request
			m_BOOL_SendPushOffSwitch = false;
			USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(m_INT_SidPushoffToSend,m_CHAR_CodeTouchePushOff_ToSend);
		}
	}
}

void USBtin_MultiblocV8::SendRequest(int sID ){
	char szDeviceID[10];
	std::string szTrameToSend = "R"; // "R02180A008"; //for debug to force request
	sprintf(szDeviceID,"%08X",(unsigned int)sID);
	szTrameToSend += szDeviceID;
	szTrameToSend += "8";
	//_log.Log(LOG_NORM,"MultiblocV8: write frame to Can: #%s# ",szTrameToSend.c_str());
	writeFrame(szTrameToSend);
	sleep_milliseconds(TIME_5ms); //wait time to not overrun can bus if many request are send...
}

void USBtin_MultiblocV8::Traitement_MultiblocV8(int IDhexNumber,unsigned int rxDLC, unsigned int Buffer_Octets[8]){
	// décomposition de l'identifiant qui contient les informations suivante :
	// identifier parsing which contains the following information :
	int FrameType = (IDhexNumber & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
	char RefBloc = (IDhexNumber & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
	char Codage = (IDhexNumber & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE;
	char SsReseau = IDhexNumber & MSK_SRES_MODULE;

	if( m_thread ){
		switch(FrameType){ //First switch !
			case type_ALIVE_FRAME:
				// création/update d'un composant détecter grace à sa trame de vie :
				// creates/updates of a blocs detected with his life frame :
				//_log.Log(LOG_NORM,"MultiblocV8: Life Frame, ref: %#x Codage : %d Network: %d",RefBloc,Codage,SsReseau);
				Traitement_Trame_Vie(RefBloc,Codage,SsReseau,rxDLC,Buffer_Octets);
				break;

			case type_ETAT_BLOC:
				//traitement ETAT BLOC reçu :
				Traitement_Trame_EtatBloc(RefBloc,Codage,SsReseau,rxDLC,Buffer_Octets);
				break;

			case type_STATE_S_TOR_1_TO_2 :
			case type_STATE_S_TOR_3_TO_4 :
			case type_STATE_S_TOR_5_TO_6 :
			case type_STATE_S_TOR_7_TO_8 :
			case type_STATE_S_TOR_9_TO_10:
				//_log.Log(LOG_NORM,"MultiblocV8: States Frame S_TOR, ref: %#x Codage : %d S/Réseau: %d",RefBloc,Codage,SsReseau);
				Traitement_Etat_S_TOR_Recu(FrameType,RefBloc,Codage,SsReseau,Buffer_Octets);
				break;

			case type_E_ANA_1_TO_4 :
			case type_E_ANA_5_TO_8 :
				Traitement_E_ANA_Recu(FrameType,RefBloc,Codage,SsReseau,Buffer_Octets);
				break;

			case type_SFSP_SWITCH :
				if( rxDLC == 5 ) Traitement_SFSP_Switch_Recu(FrameType,RefBloc,Codage,SsReseau,Buffer_Octets);
				break;

		}
	}
}

//Traitement trame contenant une info d'un interrupteur SFSP (filaire ou sans fils, EnOcean, etc...)
//Treatment of Frame containing switch information from wireles or hardware input of a blocs :
void USBtin_MultiblocV8::Traitement_SFSP_Switch_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned long sID=(FrameType<<SHIFT_TYPE_TRAME)+(RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	int i = 0;

	//_log.Log(LOG_NORM,"MultiblocV8: receive SFSP Switch: ID: %x Data: %x %x %x %x %x",sID, bufferdata[0], bufferdata[1],bufferdata[2], bufferdata[3], bufferdata[4]);
	uint32_t SwitchId = (bufferdata[0]<<24)+(bufferdata[1]<<16)+(bufferdata[2]<<8)+bufferdata[3];
	unsigned int codetouche = bufferdata[4];
	std::string defaultname=" ";

	_log.Log(LOG_NORM, "MultiblocV8: Receiving SFSP Switch Frame: Id: %s Codage: %d Ssreseau: %d SwitchID: %08X CodeTouche: %02X", NomRefBloc[RefBloc], Codage, Ssreseau, SwitchId, codetouche);

	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;

	if( SwitchId == 0x0001 ){ //hardware input of a blocs :
		lcmd.LIGHTING2.id1 = (sID>>24)&0xff;
		lcmd.LIGHTING2.id2 = (sID>>16)&0xff;
		lcmd.LIGHTING2.id3 = (sID>>8)&0xff;
		lcmd.LIGHTING2.id4 = sID&0xff;
		defaultname = "Bloc input ";
	}
	else{ //it's a wireless switch (like EnOcean) :
		lcmd.LIGHTING2.id1 = bufferdata[0]&0xff;
		lcmd.LIGHTING2.id2 = bufferdata[1]&0xff;
		lcmd.LIGHTING2.id3 = bufferdata[2]&0xff;
		lcmd.LIGHTING2.id4 = bufferdata[3]&0xff;
		defaultname = "Wireless switch";
	}

	int CodeNumber = codetouche&0x7F;
	lcmd.LIGHTING2.unitcode = CodeNumber;
	if( codetouche&0x80 ) lcmd.LIGHTING2.cmnd = light2_sOn;
	else lcmd.LIGHTING2.cmnd = light2_sOff;
	lcmd.LIGHTING2.level = 0;
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	//default name creation :

	std::ostringstream convert;   // stream used for the conversion
	convert << CodeNumber;
	defaultname += convert.str();

	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());

}

//For listing of detected blocs :
void  USBtin_MultiblocV8::BlocList_GetInfo(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	unsigned long sID=(RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	int i = 0;
	bool BIT_FIND_BLOC = false;
	int IndexBLoc = 0;
	unsigned long Rqid = 0;

	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( m_BlocList_CAN[i].BlocID == sID ){
			BIT_FIND_BLOC = true;
			IndexBLoc = i;
			break;
		}
	}
	//if the blocs allready exist :
	if( BIT_FIND_BLOC == true ){
		//on le met à jours :
		m_BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
		m_BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
		m_BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
		m_BlocList_CAN[IndexBLoc].CongifurationCrc = ( bufferdata[3]<<8 )+bufferdata[4];
		m_BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
		m_BlocList_CAN[IndexBLoc].NbAliveFrameReceived++;
	}
	else{
		i = 0;
		//or just been detected :
		for(i = 0;i < MAX_NUMBER_BLOC;i++){
			if( m_BlocList_CAN[i].BlocID == 0 ){
				m_BlocList_CAN[i].BlocID = sID;
				m_BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
				m_BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
				m_BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
				m_BlocList_CAN[IndexBLoc].CongifurationCrc = ( bufferdata[3]<<8 )+bufferdata[4];
				m_BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
				m_BlocList_CAN[IndexBLoc].NbAliveFrameReceived = 0;
				_log.Log(LOG_NORM, "MultiblocV8: new bloc detected: %s# Coding: %d network: %d", NomRefBloc[RefBloc], Codage, Ssreseau);
				//checking if we must send request, to refresh the hardware in domoticz dispositifs :
				switch(RefBloc){
					case BLOC_SFSP_M :
					case BLOC_SFSP_E :
						//requete analogique (tension alim) / analog send request for Power tension of SFSP Blocs
						Rqid= (type_E_ANA_1_TO_4<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						//Envoi 6 requetes STOR vers les SFSP : / sending 3 request to obtains states of the 6 outputs
						Rqid= (type_STATE_S_TOR_1_TO_2<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid= (type_STATE_S_TOR_3_TO_4<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid= (type_STATE_S_TOR_5_TO_6<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
						SendRequest(Rqid);

						//Créates 3 switch for Learning, Learning Exit and Clearing switches store into blocs

						std::string defaultname = NomRefBloc[RefBloc];
						std::string defaultnamenormal = defaultname + " LEARN EXIT";
						std::string defaultnamelearn = defaultname + " LEARN ENTRY";
						std::string defaultnameclear = defaultname + " CLEAR ALL";
						std::string defaultnamenextlearning = defaultname + " NEXT LEARNING OUTPUT";

						unsigned long sID_CommandBase = sID + (type_COMMANDE_ETAT_BLOC<<SHIFT_TYPE_TRAME);
						InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_LEARNING_STOP, defaultnamenormal);
						InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_LEARNING, defaultnamelearn);
						InsertUpdateControlSwitch(sID_CommandBase, BLOC_STATES_CLEARING, defaultnameclear);

						//not necessary : because the CommandBase LEARN ENTRY handles both the entry in Learn Mode
						//and after that, the jump from first to seconde output to learn, etc... and Go out automatically at end
						//so need to cretes the switch to jump in learn mode !
						//unsigned long sID_SFSPCommandBase = sID + (type_SFSP_LearnCommand<<SHIFT_TYPE_TRAME);
						//InsertUpdateControlSwitch(sID_SFSPCommandBase, 0, defaultnamenextlearning.c_str() );

						sID = type_COMMANDE_ETAT_BLOC<<SHIFT_TYPE_TRAME; //creates a unique Reset Switch to restart all presents blocks
						InsertUpdateControlSwitch(sID, BLOC_STATES_RESET, "RESET ALL MULTIBLOC V8 Blocks" );
						break;
				}
				break;
			}
		}
	}
}


void USBtin_MultiblocV8::InsertUpdateControlSwitch(const int NodeID, const int ChildID, const std::string &defaultname ){

	//make device ID
	unsigned char ID1 = (unsigned char)((NodeID & 0xFF000000) >> 24);
	unsigned char ID2 = (unsigned char)((NodeID & 0xFF0000) >> 16);
	unsigned char ID3 = (unsigned char)((NodeID & 0xFF00) >> 8);
	unsigned char ID4 = (unsigned char)NodeID & 0xFF;

	char szIdx[10];
	sprintf(szIdx, "%X%02X%02X%02X", ID1, ID2, ID3, ID4);
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT Name,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit == %d) AND (Type==%d) AND (Subtype==%d)",
		m_HwdID, szIdx, ChildID, int(pTypeLighting2), int(sTypeAC) );

	if (result.empty())
	{
		m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d,'%q',%d,%d,%d,%d,0, 12,255,'%q',0,' ')",
		m_HwdID, szIdx, ChildID, pTypeLighting2, sTypeAC, int(STYPE_PushOn), defaultname.c_str() );
	}
	else{ //sinon on remet à 0 les état ici
		m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)",
			0, m_HwdID, szIdx,pTypeLighting2,sTypeAC, ChildID);
	}
}

//call every 3 sec...
void  USBtin_MultiblocV8::BlocList_CheckBloc(){
	int i;
	int RefBlocAlive = 0;
	unsigned long Rqid = 0;
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( m_BlocList_CAN[i].BlocID != 0 ){ //Si présence d'un ID
			//we extract the blocs reference :
			RefBlocAlive = (( m_BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
			//and check the bloc state :
			if( m_BlocList_CAN[i].Status == BLOC_NOTALIVE ){
				//le bloc a été perdu / bloc lost...
				_log.Log(LOG_ERROR,"MultiblocV8: Bloc Lost with ref #%d# ",RefBlocAlive);
			}
			else{ //le bloc est en vie / Alive !
				//on vérifie si il y'a des requetes automatique à envoyer : / checking if we must send request
				//_log.Log(LOG_NORM,"MultiblocV8: BlocAlive with ref #%d# ",RefBlocAlive);
				switch(RefBlocAlive){
					case BLOC_SFSP_M :
					case BLOC_SFSP_E :
						if( m_BOOL_TaskAGo == true ){
							Rqid= (type_E_ANA_1_TO_4<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
						}
						if( m_BOOL_TaskRqStorGo == true ){
							m_BlocList_CAN[i].ForceUpdateSTOR[0] = true;
							m_BlocList_CAN[i].ForceUpdateSTOR[1] = true;
							m_BlocList_CAN[i].ForceUpdateSTOR[2] = true;
							m_BlocList_CAN[i].ForceUpdateSTOR[3] = true;
							m_BlocList_CAN[i].ForceUpdateSTOR[4] = true;
							m_BlocList_CAN[i].ForceUpdateSTOR[5] = true;
							Rqid= (type_STATE_S_TOR_1_TO_2<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
							Rqid= (type_STATE_S_TOR_3_TO_4<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
							Rqid= (type_STATE_S_TOR_5_TO_6<<SHIFT_TYPE_TRAME)+ m_BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
						}
						break;
				}
				m_BlocList_CAN[i].Status = BLOC_NOTALIVE; //RAZ ici de l'info toutes les 3 sec !
			}
		}
	}
	m_BOOL_TaskAGo = false;
	m_BOOL_TaskRqStorGo = false;
}

//traitement sur réception trame de vie / Life frame treatment :
void USBtin_MultiblocV8::Traitement_Trame_Vie(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int rxDLC,unsigned int bufferdata[8])
{
	//treatment only for a frame of 5 bytes receive !
	if( rxDLC == 5 ) BlocList_GetInfo(RefBloc,Codage,Ssreseau,bufferdata);
}

//Traitement trame Etat_Bloc / States_Bloc frame treatement  :
void USBtin_MultiblocV8::Traitement_Trame_EtatBloc(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int rxDLC,unsigned int bufferdata[8])
{
	int i = 0;
	int Command = 0;
	char szDeviceID[10];
	std::vector<std::vector<std::string> > result;
	//for searching the good commands switches :
	unsigned long sID=(type_COMMANDE_ETAT_BLOC<<SHIFT_TYPE_TRAME)+(RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	sprintf(szDeviceID,"%07X",(unsigned int)sID);
	if( rxDLC == 1 ){
		_log.Log(LOG_NORM,"MultiblocV8: Check Etat Bloc with hardwarId: %d and id: %s receive: %d",m_HwdID,szDeviceID, bufferdata[0] );
		//search the 3 switches (LEARN ENTRY / EXIT and CLEAR) associates to this return EtatBloc frame
		//in fact we can return each switch to it's normal states after a good sequence !
		if( bufferdata[0] == BLOC_NORMAL_STATES ){ //retour en mode normal / return in normal mode
			for(i = 0;i < (BLOC_STATES_CLEARING+1); i++){
				result = m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)",
					0, m_HwdID, szDeviceID,pTypeLighting2,sTypeAC, i);
				/*result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND
				(DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)", m_HwdID,
				szDeviceID,pTypeLighting2,sTypeAC, i);
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
					sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, nullptr, 255, m_Name.c_str());

				}*/
			}
		}

	}
}
//check if an output has changed...

bool USBtin_MultiblocV8::CheckOutputChange(unsigned long sID,int OutputNumber,bool CdeReceive,int LevelReceive){
	char szDeviceID[10];
	int i;
	bool returnvalue = true;
	std::vector<std::vector<std::string> > result;
	bool ForceUpdate = false;
	int slevel = 0;
	int nvalue = 0;
	unsigned long StoreIdToFind = sID &(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE);
	//serching for the bloc in bloclist :
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( m_BlocList_CAN[i].BlocID == StoreIdToFind ){
			//bloc trouvé on vérifie si on doit forcer l'update ou non des composants associés :
			//bloc find, check if update is necessary :
			ForceUpdate = m_BlocList_CAN[i].ForceUpdateSTOR[OutputNumber];
			m_BlocList_CAN[i].ForceUpdateSTOR[OutputNumber] = false; //RAZ ici
			break;
		}
	}
	unsigned long IdFordbSearch = StoreIdToFind;
	unsigned int outputindex = OutputNumber-1; //because OutputNumber is the real out number
	//but we start to 0 for the management !
	switch(outputindex){
		case 0 :case 1 : IdFordbSearch += (type_STATE_S_TOR_1_TO_2<<SHIFT_TYPE_TRAME); break;
		case 2 :case 3 : IdFordbSearch += (type_STATE_S_TOR_3_TO_4<<SHIFT_TYPE_TRAME); break;
		case 4 :case 5 : IdFordbSearch += (type_STATE_S_TOR_5_TO_6<<SHIFT_TYPE_TRAME); break;
		case 6 :case 7 : IdFordbSearch += (type_STATE_S_TOR_7_TO_8<<SHIFT_TYPE_TRAME); break;
		case 8 :case 9 : IdFordbSearch += (type_STATE_S_TOR_9_TO_10<<SHIFT_TYPE_TRAME); break;
		case 10 :case 11 : IdFordbSearch += (type_STATE_S_TOR_11_TO_12<<SHIFT_TYPE_TRAME); break;
	}

	sprintf(szDeviceID,"%07X",(unsigned int)IdFordbSearch);
	//_log.Log(LOG_NORM,"MultiblocV8: Check states for output: %d with hardwarId: %d and id: %s ",OutputNumber,m_HwdID,szDeviceID);
	result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Subtype==%d) AND (Unit==%d)",
		m_HwdID, szDeviceID,pTypeLighting2,sTypeAC, OutputNumber); //Unit = 1 = sortie n°1
	if(!result.empty() ){ //if output exist in db and no forceupdate
		if( !ForceUpdate ){
			//check if we have a change, if not do not update it
			nvalue = atoi(result[0][1].c_str());
			slevel = atoi(result[0][2].c_str());
			//_log.Log(LOG_NORM,"MultiblocV8: Output 1 nvalue : %d ",nvalue);
			if ( (!CdeReceive) && (nvalue == 0) ) returnvalue = false; //still off, nothing to do
			else{
				//Check Level changed
				//_log.Log(LOG_NORM,"MultiblocV8: Output 1 slevel : %d ",slevel);
				//level check only if cde and states = 1
				if( CdeReceive && (nvalue > 0) ){
					if (slevel == LevelReceive)
						returnvalue = false;
				}
			}
		}
		/*else{
			_log.Log(LOG_NORM,"MultiblocV8: ForceUpdate is active for this output, we are maybe just started system !?");
		}*/
	}
	else{
		_log.Log(LOG_ERROR,"MultiblocV8: No output find in db");
	}
	//_log.Log(LOG_NORM,"MultiblocV8: Check states Output: %d Actual States/Lvl: %d / %d Compare to: %d / %d Update?: %d",OutputNumber,CdeReceive,LevelReceive,nvalue,slevel,returnvalue);
	return returnvalue;
}

//store information outputs new states...
void USBtin_MultiblocV8::OutputNewStates(unsigned long sID,int OutputNumber,bool Comandeonoff,int Level ){
	double rlevel = (15.0 / 255)*Level;
	int level = int(rlevel);
	//Extract the RefBloc Type
	uint8_t RefBloc = (uint8_t)((sID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
	if (RefBloc >= NomRefBloc.size())
		return;

	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = (sID>>24)&0xff;
	lcmd.LIGHTING2.id2 = (sID>>16)&0xff;
	lcmd.LIGHTING2.id3 = (sID>>8)&0xff;
	lcmd.LIGHTING2.id4 = sID&0xff;

	if ( Comandeonoff == true ) lcmd.LIGHTING2.cmnd = light2_sOn;
	else lcmd.LIGHTING2.cmnd = light2_sOff;

	lcmd.LIGHTING2.unitcode = OutputNumber & 0xff; //SubUnit;	//output number
	lcmd.LIGHTING2.level = level; //level_value;
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	//default name creation :
	std::string defaultname = NomRefBloc[RefBloc];
	defaultname += " output S";
	std::ostringstream convert;   // stream used for the conversion
	convert << OutputNumber;
	defaultname += convert.str();

	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255, m_Name.c_str());
}

//The STOR Frame always contain a maximum of 2 stor States. 4 Low bytes = STOR 1 / 4 high bytes = STOR2
//( etc... for STOR3-4 ///  5/6  /// 7/8 ...)
void USBtin_MultiblocV8::Traitement_Etat_S_TOR_Recu(const unsigned int FrameType,const unsigned char RefeBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	//char Subtype = 0;
	int level_value = 0;
	unsigned long sID = (FrameType<<SHIFT_TYPE_TRAME)+(RefeBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;

  bool OutputCde;
	bool IsBlink;
	//bool OutputPWM;

	switch(FrameType){
		case type_STATE_S_TOR_1_TO_2:
			if(bufferdata[2] & 0x01) OutputCde = true;
			else OutputCde = false;

			//if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,1,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 1,OutputCde,level_value );
			}

			if(bufferdata[2] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,1,IsBlink);

			if(bufferdata[6] & 0x01) OutputCde = true;
			else OutputCde = false;

			//if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,2,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 2,OutputCde,level_value );
			}

			if(bufferdata[6] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,2,IsBlink);
			break;

		case type_STATE_S_TOR_3_TO_4:
			if(bufferdata[2] & 0x01) OutputCde = true;
			else OutputCde = false;
			//if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,3,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 3,OutputCde,level_value );
			}

			if(bufferdata[2] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,3,IsBlink);

			if(bufferdata[6] & 0x01) OutputCde = true;
			else OutputCde = false;
			//if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,4,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 4,OutputCde,level_value );
			}

			if(bufferdata[6] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,4,IsBlink);
			break;

		case type_STATE_S_TOR_5_TO_6:
			if(bufferdata[2] & 0x01) OutputCde = true;
			else OutputCde = false;
			//if( ((bufferdata[2]>>4) & 0x01) )OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,5,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 5,OutputCde,level_value );
			}

			if(bufferdata[2] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,5,IsBlink);

			if(bufferdata[6] & 0x01) OutputCde = true;
			else OutputCde = false;
			//if((bufferdata[6]>>4) & 0x01)OutputPWM = true;
			//else OutputPWM = false;
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,6,OutputCde,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 6,OutputCde,level_value );
			}

			if(bufferdata[6] & 0x02) IsBlink = true;
			else IsBlink = false;
			//SetOutputBlinkInDomoticz( sID,6,IsBlink);
			break;
	}
}

void USBtin_MultiblocV8::SetOutputBlinkInDomoticz (unsigned long sID,int OutputNumber,bool Blink){
	int i;
	unsigned long StoreIdToFind = sID &(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE);
	//serching for the bloc to :
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( m_BlocList_CAN[i].BlocID == StoreIdToFind ){
			//bloc trouvé:
			//we extract the blocs reference :
			int RefBlocAlive = (( m_BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
			switch(RefBlocAlive){ //Switch because the Number of output can be different for over ref blocks !
					case BLOC_SFSP_M :
					case BLOC_SFSP_E :
						//6 outputs on sfsp blocks //OutputNumber
						m_BlocList_CAN[i].IsOutputBlink[OutputNumber] = Blink;
					break;
			}
		}
	}
}

//traitement d'une trame info analogique reçue
void USBtin_MultiblocV8::Traitement_E_ANA_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	unsigned long sID = (RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;

	if( m_BOOL_DebugInMultiblocV8 == true ) _log.Log(LOG_NORM,"MultiblocV8: receive ANA (alimentation) sfsp: D0: %d D1: %d# ", bufferdata[0], bufferdata[1]);
	int VoltageLevel = bufferdata[0];
	VoltageLevel <<= 8;
	VoltageLevel += bufferdata[1];
	//_log.Log(LOG_NORM,"MultiblocV8: receive ANA1 (alimentation) sfsp: #%d# ",VoltageLevel);
	int percent = ((VoltageLevel * 100) / 125);
	float voltage = (float)VoltageLevel/10;
	SendVoltageSensor(((sID>>8)&0xffff), (sID&0xff), percent, voltage, "SFSP Voltage");
}

//Envoi d'une trame suite à une commande dans domoticz...
//sending Frame in respons to a domoticz action :
bool USBtin_MultiblocV8::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	char szDeviceID[10];
	char DataToSend[16];
	unsigned char packettype = pSen->ICMND.packettype;

	if( packettype == pTypeLighting2 )
	{
		//light command
  	long sID_EnBase;
		// Récupère l'info ID stockée qui contient l'identifiation Rebloc+Codage+Ssréseau du bloc)
		// retreive the ID information of the blocs :
		sID_EnBase = (pSen->LIGHTING2.id1<<24)+(pSen->LIGHTING2.id2<<16)+(pSen->LIGHTING2.id3<<8)+(pSen->LIGHTING2.id4);
		int FrameType =  ( sID_EnBase & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
		int ReferenceBloc = ( sID_EnBase & MSK_INDEX_MODULE ) >> SHIFT_INDEX_MODULE;
		int Codage = ( sID_EnBase & MSK_CODAGE_MODULE ) >> SHIFT_CODAGE_MODULE;
		int Ssreseau = ( sID_EnBase & MSK_SRES_MODULE ) >> SHIFT_SRES_MODULE;

		// on est autorisé à envoyer les commandes STOR uniquement si le thread est actif
		if( m_thread ){
			//pour envoyer une commande STOR : // for sending STOR Frame :
			if( FrameType == type_STATE_S_TOR_1_TO_2 ||
				FrameType == type_STATE_S_TOR_3_TO_4 ||
				FrameType == type_STATE_S_TOR_5_TO_6 ||
				FrameType == type_STATE_S_TOR_7_TO_8 ||
				FrameType == type_STATE_S_TOR_9_TO_10 ||
				FrameType == type_STATE_S_TOR_11_TO_12 ){

				unsigned long sID = (type_CMD_S_TOR<<SHIFT_TYPE_TRAME)+ (sID_EnBase&(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE));
				sprintf(szDeviceID,"%08X",(unsigned int)sID);
				unsigned int OutputNumber = (pSen->LIGHTING2.unitcode) - 1; //output number for command
				unsigned int Command = 0;
				unsigned int Reserve = 0;
				int iLevel=pSen->LIGHTING2.level;

				if (iLevel>15)
					iLevel=15;
				float fLevel = (255.0F / 15.0F) * float(iLevel);
				if (fLevel > 254.0F)
					fLevel = 255.0F;
				iLevel=int(fLevel);


				if( pSen->LIGHTING2.cmnd == light2_sSetLevel ){
					Command = 0x11; //ON + SetLevel
				}
				else if( pSen->LIGHTING2.cmnd == light2_sOn ){
					Command = 0x01;
				}

				else{ //Off...
					Command = 0;
				}

				unsigned long LongDataToSend = (OutputNumber<<24)+(Command<<16)+(Reserve<<8)+iLevel;

				sprintf(DataToSend,"%08X",(unsigned int)LongDataToSend);
				std::string szTrameToSend = "T"; //
				szTrameToSend += szDeviceID;
				szTrameToSend += "4";
				szTrameToSend += DataToSend;
				if( m_BOOL_DebugInMultiblocV8 == true ) _log.Log(LOG_NORM,"MultiblocV8: Sending Frame: %s ",szTrameToSend.c_str() );
				writeFrame(szTrameToSend);
				return true;
			}
			if (FrameType == type_SFSP_SWITCH)
			{
				//Pas d'envoi des switch créé sur réception de trames, ce sont des switch réel
				//no sending frame for switch created by the CAN, they are real switch not virtual ! it's like enocean

				//if this is a switch created in domoticz, send it on the CanBus !
				if( ReferenceBloc == BLOC_DOMOTICZ && Codage == 0 && Ssreseau == 0 ){
					if( pSen->LIGHTING2.cmnd == light2_sOn || pSen->LIGHTING2.cmnd == light2_sOff ){
						//use directly the baseId to send a "push on" command, the send "push off" is automatic:
						unsigned char CodeTouche = (pSen->LIGHTING2.unitcode);
						m_INT_SidPushoffToSend = sID_EnBase;
						m_CHAR_CodeTouchePushOff_ToSend = CodeTouche;
						CodeTouche |= 0x80; //send a "push ON" command
						USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(sID_EnBase,CodeTouche);
						m_BOOL_SendPushOffSwitch = true; //Auto push off switch because it works like EnOcean (Press and Released info on one switch).
						return true;
					}
					if (pSen->LIGHTING2.cmnd == light2_sSetLevel)
					{
						//to do : if user set the level we must send the command By Outpu direct command and not by SFSP Frame
						_log.Log(LOG_ERROR,"MultiblocV8: Dimmer level not yet supported !");
						return false;
					}
				}
				else{
					_log.Log(LOG_ERROR,"MultiblocV8: Can not switch with this DeviceID,this is not a virtual switch...");
					return false;
				}
			}
			else if( FrameType == type_COMMANDE_ETAT_BLOC ){ //specific command send to blocks
				if( ReferenceBloc == BLOC_SFSP_M || ReferenceBloc == BLOC_SFSP_E ){
					//on each "push on" switch send a bloc command starting with learn on OUTPUT 1
					//to OUTPUT 6 and return to Normal State Bloc.
					//In learning mode the output return states : ON with BLINK MODE ON(1sec)
					//but we can't show the blink mode in domoticz on a SWITCH because the output will only turn on !
					char Commande = (pSen->LIGHTING2.unitcode);
					//sID_EnBase + commande
					USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(sID_EnBase,Commande);
					//Lerning mode entrance : automatically reset and start on Output 1 !
					/*if( Commande == BLOC_STATES_LEARNING ){
						m_V8_INT_PushCountLearnMode = 0;
					} */
					return true;
				}
				_log.Log(LOG_ERROR, "MultiblocV8: Error Command BLoc not allowed !");
				return false;
			}
			else if( FrameType == type_SFSP_LearnCommand ){ //specific command for sfsp to jump from one output to the next
				if( ReferenceBloc == BLOC_SFSP_M || ReferenceBloc == BLOC_SFSP_E ){
					char Commande = (pSen->LIGHTING2.unitcode);
					USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(sID_EnBase,Commande); //
					return true;
				}
				_log.Log(LOG_ERROR, "MultiblocV8: Error Command SFSP Learn not allowed !");
				return false;
			}
		}
		else{
			_log.Log(LOG_ERROR,"MultiblocV8: This Can engine is disabled, please re-check MultiblocV8...");
		}
	}
	return false;
}

void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(long sID_ToSend,char CodeTouche){
	char szDeviceID[10];
	char DataToSend[16];
	sprintf(szDeviceID,"%08X",(unsigned int)sID_ToSend);
	//unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	//differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend,"%02X",(unsigned char)CodeTouche);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "5";		 //DLC always to 5 for SFSP_SWITCH Frame
	szTrameToSend += "00000001"; //always set to 1 because it's an internal switch (from domoticz)
	szTrameToSend += DataToSend; //contain data code + send On information
	//if( m_BOOL_DebugInMultiblocV8 == true ) _log.Log(LOG_NORM,"MultiblocV8: Sending Frame: %s ",szTrameToSend.c_str() );
	writeFrame(szTrameToSend);
}

void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(long sID_ToSend,char Commande){
	char szDeviceID[10];
	char DataToSend[16];
	sprintf(szDeviceID,"%08X",(unsigned int)sID_ToSend);
	//unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	//differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend,"%02X",(unsigned char)Commande);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "1";		 //DLC always to 5 for SFSP_SWITCH Frame
	szTrameToSend += DataToSend; //contain data code + send On information
	//if( m_BOOL_DebugInMultiblocV8 == true )
	_log.Log(LOG_NORM,"MultiblocV8: Sending BlocState command: %s ",szTrameToSend.c_str() );
	writeFrame(szTrameToSend);
}

void USBtin_MultiblocV8::USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(long baseID_ToSend,char Commande){
	char szDeviceID[10];
	char DataToSend[16];
	unsigned long sID = (type_SFSP_LearnCommand<<SHIFT_TYPE_TRAME)+ (baseID_ToSend&(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE));
	sprintf(szDeviceID,"%08X",(unsigned int)sID);
	//unsigned int DevIdOnCan = 0x00000001; //on the CAN a wired Input always send a DevId at 0x01 on a u32
	//differ from a real wireless EnOcean receive switch send directly its (u32)DeviceId
	sprintf(DataToSend,"%02X",(unsigned char)Commande);
	std::string szTrameToSend = "T"; //
	szTrameToSend += szDeviceID;
	szTrameToSend += "1";		 //DLC always to 5 for SFSP_SWITCH Frame
	szTrameToSend += DataToSend; //contain data code + send On information
	//if( m_BOOL_DebugInMultiblocV8 == true )
	_log.Log(LOG_NORM,"MultiblocV8: Sending SFSP learn command: %s ",szTrameToSend.c_str() );
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
