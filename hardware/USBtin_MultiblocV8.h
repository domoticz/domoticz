/*
File : USBtin.h
Author : Xavier PONCET BIJONNET
Version : 1.0
Description : This class manage the USBtin Gateway with Scheiber CanBus protocol

History :
- 2017-10-01 : Creation base

*/
#pragma once
#include <iostream>
#include <string>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"

#define	TIME_1sec				1000000
#define	TIME_500ms				500000
#define	TIME_200ms				200000
#define	TIME_100ms				100000
#define	TIME_50ms				50000
#define	TIME_10ms				10000
#define	TIME_5ms				5000

#define MAX_NUMBER_BLOC 		30
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

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// REFERENCE INDEX
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#define BLOC_SFSP_M                     0x14
#define BLOC_SFSP_E                     0x15
#define BLOC_SFSP_A                     0x16  // bloc autonome

class USBtin_MultiblocV8 : public CDomoticzHardwareBase
{
	public:
		USBtin_MultiblocV8();
		~USBtin_MultiblocV8();
		bool WriteToHardware(const char *pdata, const unsigned char length);
		void ManageThreadV8(bool States);
		void Traitement_MultiblocV8(int IDhexNumber,unsigned int rxDLC, unsigned int Buffer_Octets[8]);
		void StopThread();
		bool StartThread();
		bool m_stoprequested;
		
	private:
		boost::shared_ptr<boost::thread> m_thread;
		void Do_Work();
		
		int sec_counter;
		int Asec_counter;
		int min_counter;
		int min_counter2;
		bool BOOL_TaskAGo;
		bool BOOL_TaskRqStorGo;
							
		struct
		{
			long BlocID;
			int VersionH;
			int VersionM;
			int VersionL;
			int CongifurationCrc;
			int ConfigurationCHanged;
			int Status;
			int AliveFrameReceived;
			int NbAliveFrameReceived;
			bool ForceUpdateSTOR[12];
			
		} BlocList_CAN[MAX_NUMBER_BLOC]; //30 blocs Maxi
				
		void ClearingBlocList();
		void BlocList_CheckBloc();
		
		virtual bool writeFrame(const std::string&) = 0;
		
		bool BOOL_DebugInMultiblocV8;
		void SendRequest(int sID );
		
		void Traitement_Trame_Vie(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int rxDLC,unsigned int bufferdata[8]);
		void Traitement_Etat_S_TOR_Recu(const unsigned int FrameType,const unsigned char RefeBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
		void Traitement_E_ANA_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
		void Traitement_SFSP_Switch_Recu(const unsigned int FrameType,const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
		
		bool CheckOutputChange(unsigned long sID,int LightingType,int OutputNumber,int CdeReceive,int CommandPWM,int LevelReceive);
		void OutputNewStates(unsigned long sID,int OutputNumber,int Comandeonoff,int CommandPWM,int Level );
		
		void BlocList_GetInfo(const unsigned char RefBloc, const char Codage, const char Ssreseau,unsigned int bufferdata[8]);
};

