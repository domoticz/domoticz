#pragma once
#include <iostream>
#include <string>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"

#define MAX_NUMBER_BLOC 		30

class USBtin_MultiblocV8 : public CDomoticzHardwareBase
{
public:
	USBtin_MultiblocV8();
	~USBtin_MultiblocV8();
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
protected:
	void ManageThreadV8(bool States);
	void Traitement_MultiblocV8(int IDhexNumber, unsigned int rxDLC, unsigned int Buffer_Octets[8]);
	unsigned long m_V8switch_id_base;
private:
	void StopThread();
	bool StartThread();

	virtual bool writeFrame(const std::string&) = 0;

	void Do_Work();
	void ClearingBlocList();
	void BlocList_CheckBloc();
	void SendRequest(int sID);
	void Traitement_Trame_Vie(const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8]);
	void Traitement_Etat_S_TOR_Recu(const unsigned int FrameType, const unsigned char RefeBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
	void Traitement_E_ANA_Recu(const unsigned int FrameType, const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
	void Traitement_SFSP_Switch_Recu(const unsigned int FrameType, const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
	bool CheckOutputChange(unsigned long sID, int OutputNumber, bool CdeReceive, int LevelReceive);
	void OutputNewStates(unsigned long sID, int OutputNumber, bool Comandeonoff, int Level);
	void BlocList_GetInfo(const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
	void USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(long sID_ToSend, char CodeTouche);
	void USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(long sID_ToSend, char Commande);
	void USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(long baseID_ToSend, char Commande);
	void InsertUpdateControlSwitch(const int NodeID, const int ChildID, const std::string &defaultname);
	void SetOutputBlinkInDomoticz(unsigned long sID, int OutputNumber, bool Blink);
	void Traitement_Trame_EtatBloc(const unsigned char RefBloc, const char Codage, const char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8]);

	bool m_BOOL_DebugInMultiblocV8;
	bool m_BOOL_TaskAGo;
	bool m_BOOL_TaskRqStorGo;
	bool m_BOOL_SendPushOffSwitch;
	bool m_BOOL_GlobalBlinkOutputs;
	char m_CHAR_CommandBlocToSend;
	char m_CHAR_CodeTouchePushOff_ToSend;
	int m_INT_SidPushoffToSend;
	int m_V8secCounterBase;
	int m_V8secCounter1;
	int m_V8secCounter2;
	int m_V8minCounterBase;
	int m_V8minCounter1;

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
		bool IsOutputBlink[12];
	} m_BlocList_CAN[MAX_NUMBER_BLOC]; //30 blocs Maxi

	std::shared_ptr<std::thread> m_thread;
};

