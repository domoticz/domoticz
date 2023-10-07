#pragma once
#include <iostream>
#include <string>
#include "DomoticzHardware.h"
#include "hardwaretypes.h"

#define MAX_NUMBER_BLOC 50
#define MAX_BUFFER_SFSP_SWITCH 50

class USBtin_MultiblocV8 : public CDomoticzHardwareBase
{
public:
	USBtin_MultiblocV8();
	~USBtin_MultiblocV8() override;
	bool WriteToHardware(const char *pdata, unsigned char length) override;

protected:
	void ManageThreadV8(bool States, bool debugmode);
	void Traitement_MultiblocV8(int IDhexNumber, unsigned int rxDLC, unsigned int Buffer_Octets[8]);
	unsigned long m_V8switch_id_base;

private:
	void StopThread();
	bool StartThread();

	virtual bool writeFrame(const std::string &) = 0;

	void Do_Work();
	void ClearingBlocList();
	void BlocList_CheckBloc();
	void SendRequest(int sID);
	void Traitement_Trame_Vie(unsigned int RefBloc, char Codage, char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8]);
	void Traitement_Etat_S_TOR_Recu(unsigned int FrameType, unsigned int RefBloc, char Codage, char Ssreseau, unsigned int bufferdata[8]);
	void Traitement_E_ANA_Recu(unsigned int FrameType, unsigned int RefBloc, char Codage, char Ssreseau, unsigned int bufferdata[8]);
	void Traitement_SFSP_Switch_Recu(unsigned int FrameType, unsigned int RefBloc, char Codage, char Ssreseau, unsigned int bufferdata[8]);
	bool CheckOutputChange(unsigned long sID, int OutputNumber, bool CdeReceive, int LevelReceive);
	void OutputNewStates(unsigned long sID, int OutputNumber, bool Comandeonoff, int Level);
	void BlocList_GetInfo(unsigned int RefBloc, char Codage, char Ssreseau, unsigned int bufferdata[8]);
	void USBtin_MultiblocV8_Send_SFSPSwitch_OnCAN(long sID_ToSend, char CodeTouche);
	void USBtin_MultiblocV8_Send_CommandBlocState_OnCAN(long sID_ToSend, char Commande);
	void USBtin_MultiblocV8_Send_SFSP_LearnCommand_OnCAN(long baseID_ToSend, char Commande);
	void InsertUpdateControlSwitch(int NodeID, int ChildID, const std::string &defaultname);
	void SetOutputBlinkInDomoticz(unsigned long sID, int OutputNumber, bool Blink);
	void Traitement_Trame_EtatBloc(unsigned int RefBloc, char Codage, char Ssreseau, unsigned int rxDLC, unsigned int bufferdata[8]);
	int getIndexFromBlocname(std::string blocname);
	void FillBufferSFSP_toSend(int Sid, char KeyCode);
	const char* getBlocnameFromIndex(int indexreference);
	void Traitement_IBS(const unsigned int FrameType, const unsigned int RefBloc, const char Codage, const char Ssreseau, unsigned int bufferdata[8]);
	void SendIBSVoltageSensor(const int NodeID, const uint32_t ChildID, const int BatteryLevel, const float Volt, const std::string& defaultname);
	void SendIBSCurrentSensor(int NodeID, uint8_t ChildID, float Amp, const std::string &defaultname);
	void SendIBTemperatureSensor(const int NodeID, uint8_t ChildID, float Temp, const std::string &defaultname);
	std::string GetCompleteBlocNameSource( const unsigned int RefBloc, const char Codage, const char Ssreseau );
	std::string GetBatteryTypeName(int index);
	std::string GetIBSTypeName(int index);
	float TimeLeftInMinutes(float current,int DischargeableAh, int lastavailableAh );
	float GetInformationFromId(int NodeId,int sType);
	void ComputeTimeLeft(const unsigned int RefBloc, const char Codage, const char Ssreseau, const int ibsindex, const std::string& defaultname);
	void StoreSupplyVoltage(int sID, int VoltageLevel, std::string defaultname );
	void Traitement_E_TOR_Recu(unsigned int FrameType, unsigned int RefBloc, char Codage, char Ssreseau, unsigned int bufferdata[8]);

	bool m_BOOL_DebugInMultiblocV8;
	bool m_BOOL_TaskAGo;
	bool m_BOOL_TaskRqEtorGo;
	bool m_BOOL_TaskRqStorGo;
	bool m_BOOL_GlobalBlinkOutputs;
	char m_CHAR_CommandBlocToSend;

	int m_INT_SidPushoffToSend;
	int m_V8secCounterBase;
	int m_V8secCounter1;
	int m_V8secCounter2;
	int m_V8minCounterBase;
	int m_V8minCounter1;

	bool m_BOOL_parsing_buffer_go;

	struct
	{
		bool m_BOOL_SendSFSP_Switch;
		int m_INT_PushOffBufferToSend;
		char m_CHAR_CodeTouchePushOff_ToSend;
	} m_sfsp_switch_tosend[MAX_BUFFER_SFSP_SWITCH];

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
	} m_BlocList_CAN[MAX_NUMBER_BLOC]; // 30 blocs Maxi

	std::shared_ptr<std::thread> m_thread;
};
