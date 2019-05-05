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
#include <condition_variable>
#include <boost/thread/thread_time.hpp>

#define EVOHOME_RETRY_DELAY 30

class CEvohomeRadio : public CEvohomeBase
{
	struct _tRelayCheck
	{
		_tRelayCheck() :m_stLastCheck(boost::posix_time::min_date_time), m_nDemand(0) {}
		_tRelayCheck(uint8_t demand) :m_stLastCheck(boost::posix_time::min_date_time), m_nDemand(demand) {}
		_tRelayCheck(boost::system_time st, uint8_t demand) :m_stLastCheck(st), m_nDemand(demand) {}
		boost::system_time m_stLastCheck;
		uint8_t m_nDemand;
	};
	typedef std::map < uint8_t, _tRelayCheck > tmap_relay_check;
public:
	CEvohomeRadio(const int ID, const std::string &UserContID);
	~CEvohomeRadio(void);
	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	int Bind(uint8_t nDevNo, unsigned char nDevType);//use CEvohomeID::devType atm but there could be additional specialisations
	void SetRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
protected:
	bool HandleLoopData(const char *data, size_t len);
	virtual void OnConnect();
	virtual void Do_Work() = 0;
	virtual void Do_Send(std::string str) = 0;
	virtual void Idle_Work();
private:
	typedef boost::function< bool(CEvohomeMsg &msg) > fnc_evohome_decode;
	void Send();
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
	bool StartHardware() override;

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
	bool DecodeSync(CEvohomeMsg &msg);
	bool DumpMessage(CEvohomeMsg &msg);

	void AddSendQueue(const CEvohomeMsg &msg);
	void PopSendQueue(const CEvohomeMsg &msg);

	void SendExternalSensor();
	void SendZoneSensor();

	void RXRelay(uint8_t nDevNo, uint8_t nDemand, int nID = 0);
	void SendRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
	void UpdateRelayHeatDemand(uint8_t nDevNo, uint8_t nDemand);
protected:
	std::shared_ptr<std::thread> m_thread;
	int m_retrycntr;
	int m_nBufPtr;
	bool m_bDoRestart;

	unsigned int MultiControllerID[5];
	bool AllSensors;
	std::mutex m_mtxRelayCheck;
	tmap_relay_check m_RelayCheck;

	bool startup;
	boost::system_time stLastRelayCheck;
	int nStartup;
	int nStarts;

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
	std::mutex m_mtxSend;
	int m_nSendFail;

	unsigned int m_nBindID;//device ID of bound device
	unsigned char m_nBindIDType;//what type of device to bind
	std::mutex m_mtxBindNotify;
	std::condition_variable m_cndBindNotify;

	unsigned int m_UControllerID;

	typedef tmap_relay_check::iterator tmap_relay_check_it;
	typedef tmap_relay_check::value_type tmap_relay_check_pair;

	bool m_bStartup[2];
};
