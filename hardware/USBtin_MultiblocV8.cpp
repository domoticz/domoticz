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

std::string NomRefBloc[45]={
	"FACADE_1",
	"FACADE_2",
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
	m_stoprequested = false;
	BOOL_DebugInMultiblocV8 = false;
}

USBtin_MultiblocV8::~USBtin_MultiblocV8(){
	StopThread();
}

bool USBtin_MultiblocV8::StartThread()
{
	m_stoprequested = false;
	min_counter = (60*5);
	min_counter2 = (3600*6);
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&USBtin_MultiblocV8::Do_Work, this)));
	_log.Log(LOG_STATUS,"MultiblocV8: thread started");
	return (m_thread != NULL);
}

void USBtin_MultiblocV8::StopThread()
{
	try {
		if (m_thread)
		{
			m_stoprequested = true;
			m_thread->join();
			usleep(20); //wait time
			_log.Log(LOG_STATUS,"MultiblocV8: thread stopped");
		}
		ClearingBlocList();
	}
	catch (...)
	{
		_log.Log(LOG_STATUS,"MultiblocV8: thread not stopped");
		//Don't throw from a Stop command
	}
}

void USBtin_MultiblocV8::ManageThreadV8(bool States){
	if( States == true) StartThread();
	else StopThread();
}

void USBtin_MultiblocV8::ClearingBlocList(){
	_log.Log(LOG_NORM,"MultiblocV8: clearing BlocList");
	//effacement du tableau à l'init
	for(int i = 0;i < MAX_NUMBER_BLOC;i++){
		BlocList_CAN[i].BlocID = 0;
		BlocList_CAN[i].Status = 0;
		BlocList_CAN[i].NbAliveFrameReceived = 0;
		BlocList_CAN[i].VersionH = 0;
		BlocList_CAN[i].VersionM = 0;
		BlocList_CAN[i].VersionL = 0;
		BlocList_CAN[i].CongifurationCrc = 0;
	}
}

void USBtin_MultiblocV8::Do_Work()
{
	ClearingBlocList();
	//call every 100ms.... so...
	while( !m_stoprequested ){
		usleep(TIME_100ms);
		
		sec_counter++;
		if (sec_counter >= 10)
		{
			sec_counter = 0;
			min_counter--;
			min_counter2--;
			
			if( min_counter == 0 ){
				//toutes les 5 minutes
				min_counter = (60*5);
				BOOL_TaskAGo = true;
			}
			
			if( min_counter2 == 0 ){
				//toutes les 6 heures
				min_counter2 = (3600*6);
				BOOL_TaskRqStorGo = true;
			}
			
			Asec_counter++;
			if( Asec_counter >= 3 ){
				Asec_counter = 0;
				//toutes les 3 secondes :
				BlocList_CheckBloc();
			}
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
	usleep(TIME_5ms); //wait time to not overrun can bus if many request are send...
}

void USBtin_MultiblocV8::Traitement_MultiblocV8(int IDhexNumber,unsigned int rxDLC, unsigned int Buffer_Octets[8]){
	// écomposition de l'identifiant qui contient les informations suivante :
	// identifier parsing which contains the following information :
	int FrameType = (IDhexNumber & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
	char RefBloc = (IDhexNumber & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
	char Codage = (IDhexNumber & MSK_CODAGE_MODULE) >> SHIFT_CODAGE_MODULE;
	char SsReseau = IDhexNumber & MSK_SRES_MODULE;

	if( m_thread ){
		switch(FrameType){ //First switch !
			case type_ALIVE_FRAME:
				//création/update d'un composant détecter grace à sa trame de vie :
				//_log.Log(LOG_NORM,"MultiblocV8: Trame de vie, ref: %#x Codage : %d S/Réseau: %d",RefBloc,Codage,SsReseau);
				Traitement_Trame_Vie(RefBloc,Codage,SsReseau,rxDLC,Buffer_Octets);
				break;
				
			case type_STATE_S_TOR_1_TO_2 :
			case type_STATE_S_TOR_3_TO_4 :
			case type_STATE_S_TOR_5_TO_6 :
			case type_STATE_S_TOR_7_TO_8 :
			case type_STATE_S_TOR_9_TO_10:
				//_log.Log(LOG_NORM,"MultiblocV8: Trame d'état S_TOR, ref: %#x Codage : %d S/Réseau: %d",RefBloc,Codage,SsReseau);
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
void USBtin_MultiblocV8::Traitement_SFSP_Switch_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8])
{
	unsigned long sID=(FrameType<<SHIFT_TYPE_TRAME)+(RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	int i = 0;
	
	//_log.Log(LOG_NORM,"MultiblocV8: receive SFSP Switch: ID: %x Data: %x %x %x %x %x",sID, bufferdata[0], bufferdata[1],bufferdata[2], bufferdata[3], bufferdata[4]);
	unsigned long SwitchId = (bufferdata[0]<<24)+(bufferdata[1]<<16)+(bufferdata[2]<<8)+bufferdata[3];
	std::string defaultname=" ";
	
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	
	if( SwitchId == 0x0001 ){ //il s'agit d'un interrupteur filaire issue d'un bloc Multibloc
		lcmd.LIGHTING2.id1 = (sID>>24)&0xff;
		lcmd.LIGHTING2.id2 = (sID>>16)&0xff;
		lcmd.LIGHTING2.id3 = (sID>>8)&0xff;
		lcmd.LIGHTING2.id4 = sID&0xff;
		defaultname = "Bloc input ";
	}
	else{ //Sinon il s'agit d'un interrupteur sans fil (EnOcean, etc...)
		lcmd.LIGHTING2.id1 = bufferdata[0]&0xff;
		lcmd.LIGHTING2.id2 = bufferdata[1]&0xff;
		lcmd.LIGHTING2.id3 = bufferdata[2]&0xff;
		lcmd.LIGHTING2.id4 = bufferdata[3]&0xff;
		defaultname = "Wireless switch";
	}
	int CodeNumber = bufferdata[4]&0x7F;
	lcmd.LIGHTING2.unitcode = CodeNumber;
	if( bufferdata[4]&0x80 ) lcmd.LIGHTING2.cmnd = light2_sOn;
	else lcmd.LIGHTING2.cmnd = light2_sOff;
	lcmd.LIGHTING2.level = 0;		
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	//default name creation :
	
	std::ostringstream convert;   // stream used for the conversion
	convert << CodeNumber;
	defaultname += convert.str();
	
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
	
}

//pour le listage des blocs présents !
void  USBtin_MultiblocV8::BlocList_GetInfo(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	unsigned long sID=(RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	int i = 0;
	bool BIT_FIND_BLOC = false;
	int IndexBLoc = 0;
	unsigned long Rqid = 0;
	
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( BlocList_CAN[i].BlocID == sID ){
			BIT_FIND_BLOC = true;
			IndexBLoc = i;
			break;
		}
	}
	//si le bloc est déjà listé
	if( BIT_FIND_BLOC == true ){
		//on le met à jours :
		BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
		BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
		BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
		BlocList_CAN[IndexBLoc].CongifurationCrc = ( bufferdata[3]<<8 )+bufferdata[4];
		BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
		BlocList_CAN[IndexBLoc].NbAliveFrameReceived++;
	}
	else{
		i = 0;
		//nouveau bloc on l'inscrit après le dernier inscrit
		for(i = 0;i < MAX_NUMBER_BLOC;i++){
			if( BlocList_CAN[i].BlocID == 0 ){
				BlocList_CAN[i].BlocID = sID;
				BlocList_CAN[IndexBLoc].VersionH = bufferdata[0];
				BlocList_CAN[IndexBLoc].VersionM = bufferdata[1];
				BlocList_CAN[IndexBLoc].VersionL = bufferdata[2];
				BlocList_CAN[IndexBLoc].CongifurationCrc = ( bufferdata[3]<<8 )+bufferdata[4];
				BlocList_CAN[IndexBLoc].Status = BLOC_ALIVE;
				BlocList_CAN[IndexBLoc].NbAliveFrameReceived = 0;
				_log.Log(LOG_NORM,"MultiblocV8: new bloc detected: %s# Coding: %d network: %d", NomRefBloc[RefBloc].c_str(),Codage,Ssreseau);
				//on envoit les requetes immédiatement :
				switch(RefBloc){
					case BLOC_SFSP_M :
					case BLOC_SFSP_E :
						//requete analogique (tension alim)
						Rqid= (type_E_ANA_1_TO_4<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						//Envoi 6 requetes STOR vers les SFSP :
						Rqid= (type_STATE_S_TOR_1_TO_2<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid= (type_STATE_S_TOR_3_TO_4<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						Rqid= (type_STATE_S_TOR_5_TO_6<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
						SendRequest(Rqid);
						break;
				}
				break;
			}
		}
	}
}

//call every 3 sec...
void  USBtin_MultiblocV8::BlocList_CheckBloc(){
	int i;
	int RefBlocAlive = 0;
	unsigned long Rqid = 0;
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( BlocList_CAN[i].BlocID != 0 ){ //Si présence d'un ID
			//on récupère l'ID du bloc :
			RefBlocAlive = (( BlocList_CAN[i].BlocID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE);
			if( BlocList_CAN[i].Status == BLOC_NOTALIVE ){
				//le bloc a été perdu / bloc lost...
				_log.Log(LOG_ERROR,"MultiblocV8: Bloc Lost with ref #%d# ",RefBlocAlive);
			}
			else{ //le bloc est en vie
				//on vérifie si il y'a des requetes automatique à envoyer :
				//_log.Log(LOG_NORM,"MultiblocV8: BlocAlive with ref #%d# ",RefBlocAlive);
				switch(RefBlocAlive){
					case BLOC_SFSP_M :
					case BLOC_SFSP_E :
						if( BOOL_TaskAGo == true ){
							Rqid= (type_E_ANA_1_TO_4<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
						}
						if( BOOL_TaskRqStorGo == true ){
							//Envoi 6 requetes STOR vers les SFSP :
							BlocList_CAN[i].ForceUpdateSTOR[0] = true;
							BlocList_CAN[i].ForceUpdateSTOR[1] = true;
							BlocList_CAN[i].ForceUpdateSTOR[2] = true;
							BlocList_CAN[i].ForceUpdateSTOR[3] = true;
							BlocList_CAN[i].ForceUpdateSTOR[4] = true;
							BlocList_CAN[i].ForceUpdateSTOR[5] = true;
							Rqid= (type_STATE_S_TOR_1_TO_2<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
							Rqid= (type_STATE_S_TOR_3_TO_4<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
							Rqid= (type_STATE_S_TOR_5_TO_6<<SHIFT_TYPE_TRAME)+ BlocList_CAN[i].BlocID;
							SendRequest(Rqid);
						}
						break;
				}
				BlocList_CAN[i].Status = BLOC_NOTALIVE; //RAZ ici de l'info toutes les 3 sec !
			}
		}
	}
	BOOL_TaskAGo = false;
	BOOL_TaskRqStorGo = false;
}

//traitement sur réception trame de vie
void USBtin_MultiblocV8::Traitement_Trame_Vie(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int rxDLC,unsigned int bufferdata[8])
{
	//on créé une ligne concernant l'info présence du module connecté et on vérifie certaines actions à faire
	if( rxDLC == 5 ) BlocList_GetInfo(RefBloc,Codage,Ssreseau,bufferdata);
}

//check if an output has changed...
bool USBtin_MultiblocV8::CheckOutputChange(unsigned long sID,int LightingType,int OutputNumber,int CdeReceive,int CommandPWM,int LevelReceive){
	char szDeviceID[10];
	int i;
	bool returnvalue = true;
	sprintf(szDeviceID,"%07X",(unsigned int)sID);
	std::vector<std::vector<std::string> > result;
	bool ForceUpdate = false;
	
	unsigned long StoreIdToFind = sID &(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE);
	//on recherche le bloc dans la liste Bloclist :
	for(i = 0;i < MAX_NUMBER_BLOC;i++){
		if( BlocList_CAN[i].BlocID == StoreIdToFind ){
			//bloc trouvé on vérifie si on doit forcer l'update ou non des composants associés :
			ForceUpdate = BlocList_CAN[i].ForceUpdateSTOR[OutputNumber];
			BlocList_CAN[i].ForceUpdateSTOR[OutputNumber] = false; //RAZ ici
			break;
		}
	}
	
	result = m_sql.safe_query("SELECT ID,nValue,sValue FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Type==%d) AND (Unit==%d)", 
			m_HwdID, szDeviceID, LightingType, OutputNumber); //Unit = 1 = sortie n°1
		if(!result.empty() && !ForceUpdate ){ //if output exist in db and no forceupdate
			//check if we have a change, if not do not update it
			int nvalue = atoi(result[0][1].c_str());
			//_log.Log(LOG_NORM,"MultiblocV8: Output 1 nvalue : %d ",nvalue);
			if ( (!CdeReceive) && (nvalue == 0) ) returnvalue = false;
			else{
				//Check Level
				int slevel = atoi(result[0][2].c_str());
				//_log.Log(LOG_NORM,"MultiblocV8: Output 1 slevel : %d ",slevel);
				if (CommandPWM && (slevel == LevelReceive) ) returnvalue = false;
			}
		}
	return returnvalue;
}

//store information outputs new states...
void USBtin_MultiblocV8::OutputNewStates(unsigned long sID,int OutputNumber,int Comandeonoff,int CommandPWM,int Level ){
	double rlevel = (15.0 / 255)*Level;
	int level = int(rlevel);
	//Extract the RefBloc Type
	char RefBloc = (sID & MSK_INDEX_MODULE) >> SHIFT_INDEX_MODULE;
	
	tRBUF lcmd;
	memset(&lcmd, 0, sizeof(RBUF));
	lcmd.LIGHTING2.packetlength = sizeof(lcmd.LIGHTING2) - 1;
	lcmd.LIGHTING2.packettype = pTypeLighting2;
	lcmd.LIGHTING2.subtype = sTypeAC;
	lcmd.LIGHTING2.id1 = (sID>>24)&0xff;
	lcmd.LIGHTING2.id2 = (sID>>16)&0xff;
	lcmd.LIGHTING2.id3 = (sID>>8)&0xff;
	lcmd.LIGHTING2.id4 = sID&0xff;
	if ( ( Comandeonoff & 0x01 ) ) lcmd.LIGHTING2.cmnd = light2_sOn;
	else lcmd.LIGHTING2.cmnd = light2_sOff;
	//Si état PWM actif:
	if( ( CommandPWM & 0x01 ) ) lcmd.LIGHTING2.cmnd =  light2_sSetLevel;
	//else level_value = 0;
	//int level_value = Level;
	//level_value /= 16;
	//if( level_value > 15 ) level_value = 15;
		

	
	lcmd.LIGHTING2.unitcode = OutputNumber & 0xff; //SubUnit;	//output number
	lcmd.LIGHTING2.level = level; //level_value;		
	lcmd.LIGHTING2.filler = 2;
	lcmd.LIGHTING2.rssi = 12;
	//default name creation :
	std::string defaultname=NomRefBloc[RefBloc].c_str();
	defaultname += " output S";
	std::ostringstream convert;   // stream used for the conversion
	convert << OutputNumber;
	defaultname += convert.str();
	
	sDecodeRXMessage(this, (const unsigned char *)&lcmd.LIGHTING2, defaultname.c_str(), 255);
}

//The STOR Frame always contain a maximum of 2 stor States. 4 Low bytes = STOR 1 / 4 high bytes = STOR2 
//( etc... for STOR3-4 ///  5/6  /// 7/8 ...)
void USBtin_MultiblocV8::Traitement_Etat_S_TOR_Recu(const unsigned int FrameType,const unsigned char RefeBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	//char Subtype = 0;
	int level_value = 0;
		
	unsigned long sID = (FrameType<<SHIFT_TYPE_TRAME)+(RefeBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;
	int OutputCde;
	int OutputPWM;
	
	/*switch(RefeBloc){ //Switch on ref bloc to find the bloc product who answer
		case BLOC_SFSP_M :
			//_log.Log(LOG_NORM,"MultiblocV8: receive STOR states SFSPM: Data: %d ", bufferdata);
			Subtype = sTypeSFSP_M; //the subtype store in domoticz (part of LIGHTING2 structure)
			break;
		case BLOC_SFSP_E :
			//_log.Log(LOG_NORM,"MultiblocV8: receive STOR states SFSPEM: Data: %d ", bufferdata);
			Subtype = sTypeSFSP_E;
			break;
	}*/
	
	switch(FrameType){
		case type_STATE_S_TOR_1_TO_2:
			OutputCde = (bufferdata[2] & 0x01);
			OutputPWM = ((bufferdata[2]>>4) & 0x01);
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,1,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 1,OutputCde,OutputPWM,level_value );
			}
			
			OutputCde = (bufferdata[6] & 0x01);
			OutputPWM = ((bufferdata[6]>>4) & 0x01);
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,2,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 2,OutputCde,OutputPWM,level_value );
			}
			break;
			
		case type_STATE_S_TOR_3_TO_4:
			OutputCde = (bufferdata[2] & 0x01);
			OutputPWM = ((bufferdata[2]>>4) & 0x01);
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,3,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 3,OutputCde,OutputPWM,level_value );
			}
			
			OutputCde = (bufferdata[6] & 0x01);
			OutputPWM = ((bufferdata[6]>>4) & 0x01);
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,4,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 4,OutputCde,OutputPWM,level_value );
			}
			break;
			
		case type_STATE_S_TOR_5_TO_6:
			OutputCde = (bufferdata[2] & 0x01);
			OutputPWM = ((bufferdata[2]>>4) & 0x01);
			level_value = bufferdata[0];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,5,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 5,OutputCde,OutputPWM,level_value );
			}
			
			OutputCde = (bufferdata[6] & 0x01);
			OutputPWM = ((bufferdata[6]>>4) & 0x01);
			level_value = bufferdata[4];			//variable niveau (lvl) 0-254 à convertir en 0 - 100%
			level_value /= 16;
			if( level_value > 15 ) level_value = 15;
			if( CheckOutputChange(sID,pTypeLighting2,6,OutputCde,OutputPWM,level_value) == true ) { //on met à jours si nécessaire !
				OutputNewStates( sID, 6,OutputCde,OutputPWM,level_value );
			}
			break;
	}
	
}

//traitement d'une trame info analogique reçue
void USBtin_MultiblocV8::Traitement_E_ANA_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8])
{
	unsigned long sID = (RefBloc<<SHIFT_INDEX_MODULE)+(Codage<<SHIFT_CODAGE_MODULE)+Ssreseau;	
	
	if( BOOL_DebugInMultiblocV8 == true ) _log.Log(LOG_NORM,"MultiblocV8: receive ANA (alimentation) sfsp: D0: %d D1: %d# ", bufferdata[0], bufferdata[1]);
	int VoltageLevel = bufferdata[0];
	VoltageLevel <<= 8;
	VoltageLevel += bufferdata[1];
	
	//_log.Log(LOG_NORM,"MultiblocV8: receive ANA1 (alimentation) sfsp: #%d# ",VoltageLevel);
	int percent = ((VoltageLevel * 100) / 125);
	float voltage = (float)VoltageLevel/10;
	SendVoltageSensor(((sID>>8)&0xffff), (sID&0xff), percent, voltage, "SFSP Voltage");
}

//Envoi d'une trame suite à une commande dans domoticz...
bool USBtin_MultiblocV8::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	char szDeviceID[10];
	char DataToSend[16];
	unsigned char packettype = pSen->ICMND.packettype;
	//unsigned char subtype = pSen->ICMND.subtype;

	if( packettype == pTypeLighting2 )
	{
		//light command
		unsigned long sID_EnBase;
		//Récupère l'info ID stockée qui contient l'identifiation Rebloc+Codage+Ssréseau du bloc)
		sID_EnBase = (pSen->LIGHTING2.id1<<24)+(pSen->LIGHTING2.id2<<16)+(pSen->LIGHTING2.id3<<8)+(pSen->LIGHTING2.id4);
		//on est autorisé à envoyer les commandes STOR uniquement
		int FrameType =  ( sID_EnBase & MSK_TYPE_TRAME) >> SHIFT_TYPE_TRAME;
		if( m_thread ){
			if( FrameType == type_STATE_S_TOR_1_TO_2 ||
				FrameType == type_STATE_S_TOR_3_TO_4 ||
				FrameType == type_STATE_S_TOR_5_TO_6 ||
				FrameType == type_STATE_S_TOR_7_TO_8 ||
				FrameType == type_STATE_S_TOR_9_TO_10 ||
				FrameType == type_STATE_S_TOR_11_TO_12 ){
				//on ajoute la commande S_TOR pour créer l'identifiant d'envoi :
				//on ne conserve que la base : RefProduit+codage+ssreseau (on supprime le Frametype)
				unsigned long sID = (type_CMD_S_TOR<<SHIFT_TYPE_TRAME)+ (sID_EnBase&(MSK_INDEX_MODULE+MSK_CODAGE_MODULE+MSK_SRES_MODULE));
				sprintf(szDeviceID,"%08X",(unsigned int)sID);
				unsigned int OutputNumber = (pSen->LIGHTING2.unitcode) - 1; //output number for command
				unsigned int Command = 0;
				unsigned int Reserve = 0;
				//unsigned int Level = pSen->LIGHTING2.level;
				int iLevel=pSen->LIGHTING2.level;
				//if( Level >= 0x0F ) Level = 255;
				//else Level *= 16;
				
				//iLevel=tsen->LIGHTING2.level;
				if (iLevel>15)
					iLevel=15;
				float fLevel=(255.0f/15.0f)*float(iLevel);
				if (fLevel>254.0f)
					fLevel=255.0f;
				iLevel=int(fLevel);
				
				
				if( pSen->LIGHTING2.cmnd == light2_sOn ){
					Command = 0x01;
				}
				else if( pSen->LIGHTING2.cmnd == light2_sSetLevel ){
					Command = 0x11; //ON + SetLevel
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
				if( BOOL_DebugInMultiblocV8 == true ) _log.Log(LOG_NORM,"MultiblocV8: Sending Frame: %s ",szTrameToSend.c_str() );
				writeFrame(szTrameToSend);
				return true;				
			}
			else if( FrameType == type_SFSP_SWITCH ){
				_log.Log(LOG_ERROR,"MultiblocV8: Can not switch with this DeviceID,this is not a virtual switch...");
				return false;
			}
		}
		else{
			_log.Log(LOG_ERROR,"MultiblocV8: This Can engine is disabled, please re-check MultiblocV8...");
		}
	}
	return false;
}

