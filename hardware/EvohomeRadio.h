/**
 * Evohome class for HGI80 gateway, evohome control, monitoring and integration with Domoticz
 *
 *  Copyright 2014 - fullTalgoRythm https://github.com/fullTalgoRythm/Domoticz-evohome
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 * based in part on https://github.com/mouse256/evomon
 * and details available at http://www.domoticaforum.eu/viewtopic.php?f=7&t=5806&start=90#p72564
 */

#pragma once

#include "EvohomeBase.h"

#define EVOHOME_RETRY_DELAY 30

class CEvohomeRadio : public CEvohomeBase
{
public:
	CEvohomeRadio(const int ID, const std::string &UserContID);
	~CEvohomeRadio(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);

	typedef boost::function< bool(CEvohomeMsg &msg) > fnc_evohome_decode;

	enum evoCommands{
		cmdSysInfo=0x10e0,
		cmdZoneTemp=0x30C9,
		cmdZoneName=0x0004,
		cmdZoneHeatDemand=0x3150,//Heat demand sent by an individual zone
		cmdZoneInfo=0x000A,
		cmdZoneWindow=0x12B0,//Open window/ventilation zone function
		cmdSetPoint=0x2309,
		cmdSetpointOverride=0x2349,
		cmdDHWState=0x1F41,
		cmdDHWTemp=0x1260,
		cmdControllerMode=0x2E04,
		cmdControllerHeatDemand=0x0008,//Heat demand sent by the controller for CH / DHW / Boiler  (F9/FA/FC)
		cmdActuatorState=0x3EF0,
		cmdActuatorCheck=0x3B00,
		cmdBinding=0x1FC9,
		cmdExternalSensor=0x0002,
		cmdDeviceInfo=0x0418,
		cmdBatteryInfo=0x1060,
		//0x10a0 //DHW settings sent between controller and DHW sensor can also be requested by the gateway <1:DevNo><2(uint16_t):SetPoint><1:Overrun?><2:Differential>
		//0x1F09 //Unknown fixed message FF070D
		//0x0005
		//0x0006
		//0x0404
	};

	void SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	int Bind(uint8_t nDevNo, unsigned char nDevType);//use CEvohomeID::devType atm but there could be additional specialisations

protected:
	bool HandleLoopData(const char *data, size_t len);
	void Send();

	virtual void Do_Work() = 0;
	virtual void Do_Send(std::string str) = 0;
	virtual void OnConnect();
	virtual void Idle_Work();

	boost::shared_ptr<boost::thread> m_thread;
	volatile bool m_stoprequested;
	int m_retrycntr;
	int m_nBufPtr;
	bool m_bDoRestart;

	struct _tRelayCheck
	{
		_tRelayCheck():m_stLastCheck(boost::posix_time::min_date_time),m_nDemand(0){}
		_tRelayCheck(uint8_t demand):m_stLastCheck(boost::posix_time::min_date_time),m_nDemand(demand){}
		_tRelayCheck(boost::system_time st, uint8_t demand):m_stLastCheck(st),m_nDemand(demand){}
		boost::system_time m_stLastCheck;
		uint8_t m_nDemand;
	};
	typedef std::map < uint8_t, _tRelayCheck > tmap_relay_check;

	unsigned int MultiControllerID[5];
	bool AllSensors;
	boost::mutex m_mtxRelayCheck;
	tmap_relay_check m_RelayCheck;

	bool startup;
	boost::system_time stLastRelayCheck;
	int nStartup;
	int nStarts;

	void RequestCurrentState();
	void RequestZoneState();
	void RequestZoneNames();
	void RequestControllerMode();
	void RequestSysInfo();
	void RequestDHWState();
	void RequestDHWTemp();
	void RequestZoneInfo(uint8_t nZone);
	void RequestZoneTemp(uint8_t nZone);
	void RequestZoneName(uint8_t nZone);
	void RequestZoneStartupInfo(uint8_t nZone);
	void RequestSetPointOverride(uint8_t nZone);
	void RequestDeviceInfo(uint8_t nAddr);

	void CheckRelayHeatDemand();
	void SendRelayKeepAlive();

private:
	bool StartHardware();

	int ProcessBuf(char * buf, int size);
	void ProcessMsg(const char * rawmsg);
	bool DecodePayload(CEvohomeMsg &msg);

	void RegisterDecoder(unsigned int cmd, fnc_evohome_decode fndecoder);
	bool DecodeSetpoint(CEvohomeMsg &msg);
	bool DecodeSetpointOverride(CEvohomeMsg &msg);
	bool DecodeZoneTemp(CEvohomeMsg &msg);
	bool DecodeDHWState(CEvohomeMsg &msg);
	bool DecodeDHWTemp(CEvohomeMsg &msg);
	bool DecodeControllerMode(CEvohomeMsg &msg);
	bool DecodeSysInfo(CEvohomeMsg &msg);
	bool DecodeZoneName(CEvohomeMsg &msg);
	bool DecodeHeatDemand(CEvohomeMsg &msg);
	bool DecodeZoneInfo(CEvohomeMsg &msg);
	bool DecodeBinding(CEvohomeMsg &msg);
	bool DecodeActuatorState(CEvohomeMsg &msg);
	bool DecodeActuatorCheck(CEvohomeMsg &msg);
	bool DecodeZoneWindow(CEvohomeMsg &msg);
	bool DecodeExternalSensor(CEvohomeMsg &msg);
	bool DecodeDeviceInfo(CEvohomeMsg &msg);
	bool DecodeBatteryInfo(CEvohomeMsg &msg);
	bool DumpMessage(CEvohomeMsg &msg);

	void AddSendQueue(const CEvohomeMsg &msg);
	void PopSendQueue(const CEvohomeMsg &msg);

	void SendExternalSensor();
	void SendZoneSensor();

	void RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID=0);
	void SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	void UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);

	static const int m_evoToDczControllerMode[8];
	static const int m_evoToDczOverrideMode[5];
	static const uint8_t m_dczToEvoZoneMode[3];
	static const uint8_t m_dczToEvoControllerMode[6];

	template <typename RT> RT ConvertMode(RT* pArray, uint8_t nIdx){return pArray[nIdx];}

	static const char m_szNameErr[18];
	static const int m_nBufSize=4096;
	char m_buf[m_nBufSize];

	std::map < unsigned int, fnc_evohome_decode > m_Decoders;
	std::list < CEvohomeMsg > m_SendQueue;
	boost::mutex m_mtxSend;
	int m_nSendFail;

	unsigned int m_nBindID;//device ID of bound device
	unsigned char m_nBindIDType;//what type of device to bind
	boost::mutex m_mtxBindNotify;
	boost::condition_variable m_cndBindNotify;

	unsigned int m_UControllerID;

	typedef tmap_relay_check::iterator tmap_relay_check_it;
	typedef tmap_relay_check::value_type tmap_relay_check_pair;

	bool m_bStartup[2];
};
