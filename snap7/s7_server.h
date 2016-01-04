/*=============================================================================|
|  PROJECT SNAP7                                                         1.3.0 |
|==============================================================================|
|  Copyright (C) 2013, 2015 Davide Nardella                                    |
|  All rights reserved.                                                        |
|==============================================================================|
|  SNAP7 is free software: you can redistribute it and/or modify               |
|  it under the terms of the Lesser GNU General Public License as published by |
|  the Free Software Foundation, either version 3 of the License, or           |
|  (at your option) any later version.                                         |
|                                                                              |
|  It means that you can distribute your commercial software linked with       |
|  SNAP7 without the requirement to distribute the source code of your         |
|  application and without the requirement that your application be itself     |
|  distributed under LGPL.                                                     |
|                                                                              |
|  SNAP7 is distributed in the hope that it will be useful,                    |
|  but WITHOUT ANY WARRANTY; without even the implied warranty of              |
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               |
|  Lesser GNU General Public License for more details.                         |
|                                                                              |
|  You should have received a copy of the GNU General Public License and a     |
|  copy of Lesser GNU General Public License along with Snap7.                 |
|  If not, see  http://www.gnu.org/licenses/                                   |
|=============================================================================*/
#ifndef s7_server_h
#define s7_server_h
//---------------------------------------------------------------------------
#include "snap_tcpsrvr.h"
#include "s7_types.h"
#include "s7_isotcp.h"
//---------------------------------------------------------------------------

// Maximum number of DB, change it to increase/decrease the limit.
// The DB table size is 12*MaxDB bytes

#define MaxDB 2048    // Like a S7 318
#define MinPduSize 240
//---------------------------------------------------------------------------
// Server Interface errors
const longword errSrvDBNullPointer      = 0x00200000; // Pssed null as PData
const longword errSrvAreaAlreadyExists  = 0x00300000; // Area Re-registration
const longword errSrvUnknownArea        = 0x00400000; // Unknown area
const longword errSrvInvalidParams      = 0x00500000; // Invalid param(s) supplied
const longword errSrvTooManyDB          = 0x00600000; // Cannot register DB
const longword errSrvInvalidParamNumber = 0x00700000; // Invalid param (srv_get/set_param)
const longword errSrvCannotChangeParam  = 0x00800000; // Cannot change because running

// Server Area ID  (use with Register/unregister - Lock/unlock Area)
const int srvAreaPE = 0;
const int srvAreaPA = 1;
const int srvAreaMK = 2;
const int srvAreaCT = 3;
const int srvAreaTM = 4;
const int srvAreaDB = 5;

typedef struct{
	word   Number; // Number (only for DB)
	word   Size;   // Area size (in bytes)
	pbyte  PData;  // Pointer to area
	PSnapCriticalSection cs;
}TS7Area, *PS7Area;

//------------------------------------------------------------------------------
// ISOTCP WORKER CLASS
//------------------------------------------------------------------------------
class TIsoTcpWorker : public TIsoTcpSocket
{
protected:
	virtual bool IsoPerformCommand(int &Size);
	virtual bool ExecuteSend();
	virtual bool ExecuteRecv();
public:
	TIsoTcpWorker(){};
	~TIsoTcpWorker(){};
	// Worker execution
	bool Execute();
};
//------------------------------------------------------------------------------
// S7 WORKER CLASS
//------------------------------------------------------------------------------

// SZL frame
typedef struct{
    TS7Answer17         Answer;
    PReqFunReadSZLFirst ReqParams;
    PS7ReqSZLData       ReqData;
    PS7ResParams7       ResParams;
    pbyte               ResData;
    int                 ID;
    int                 Index;
    bool                SZLDone;
}TSZL;

// Current Event Info
typedef struct{
    word EvRetCode;
    word EvArea;
    word EvIndex;
    word EvStart;
    word EvSize;
}TEv;

// Current Block info
typedef struct{
  PReqFunGetBlockInfo ReqParams;
  PResFunGetBlockInfo ResParams;
  TS7Answer17         Answer;
  word                evError;
  word                DataLength;
}TCB;

class TSnap7Server; // forward declaration

class TS7Worker : public TIsoTcpWorker
{
private:
    PS7ReqHeader PDUH_in;
	int DBCnt;
    byte LastBlk;
    TSZL SZL;
    TS7Buffer Buffer;
    byte BCD(word Value);
    // Checks the consistence of the incoming PDU
    bool CheckPDU_in(int PayloadSize);
    void FillTime(PS7Time PTime);
protected:
    int DataSizeByte(int WordLength);
    bool ExecuteRecv();
    void DoEvent(longword Code, word RetCode, word Param1, word Param2,
      word Param3, word Param4);
    void DoReadEvent(longword Code, word RetCode, word Param1, word Param2,
      word Param3, word Param4);
    void FragmentSkipped(int Size);
    // Entry parse
    bool IsoPerformCommand(int &Size);
    // First stage parse
    bool PerformPDUAck(int &Size);
    bool PerformPDURequest(int &Size);
    bool PerformPDUUsrData(int &Size);
    // Second stage parse : PDU Request
    PS7Area GetArea(byte S7Code, word index);
    // Group Read Area
    bool PerformFunctionRead();
    // Subfunctions Read Data
    word ReadArea(PResFunReadItem ResItemData, PReqFunReadItem ReqItemPar,
    int &PDURemainder,TEv &EV);
    word RA_NotFound(PResFunReadItem ResItem, TEv &EV);
    word RA_OutOfRange(PResFunReadItem ResItem, TEv &EV);
    word RA_SizeOverPDU(PResFunReadItem ResItem, TEv &EV);
    // Group Write Area
    bool PerformFunctionWrite();
    // Subfunctions Write Data
    byte WriteArea(PReqFunWriteDataItem ReqItemData, PReqFunWriteItem ReqItemPar,
         TEv &EV);
    byte WA_NotFound(TEv &EV);
    byte WA_InvalidTransportSize(TEv &EV);
    byte WA_OutOfRange(TEv &EV);
    byte WA_DataSizeMismatch(TEv &EV);
    // Negotiate PDU Length
    bool PerformFunctionNegotiate();
    // Control
    bool PerformFunctionControl(byte PduFun);
    // Up/Download
	bool PerformFunctionUpload();
    bool PerformFunctionDownload();
    // Second stage parse : PDU User data
    bool PerformGroupProgrammer();
    bool PerformGroupCyclicData();
    bool PerformGroupSecurity();
    // Group Block(s) Info
    bool PerformGroupBlockInfo();
    // Subfunctions Block info
    void BLK_ListAll(TCB &CB);
    void BLK_ListBoT(byte BlockType, bool Start, TCB &CB);
    void BLK_NoResource_ListBoT(PDataFunGetBot Data, TCB &CB);
    void BLK_GetBlkInfo(TCB &CB);
    void BLK_NoResource_GetBlkInfo(PResDataBlockInfo Data, TCB &CB);
    void BLK_GetBlockNum_GetBlkInfo(int &BlkNum, PReqDataBlockInfo ReqData);
    void BLK_DoBlockInfo_GetBlkInfo(PS7Area DB, PResDataBlockInfo Data, TCB &CB);
    // Clock Group
    bool PerformGetClock();
    bool PerformSetClock();
    // SZL Group
    bool PerformGroupSZL();
    // Subfunctions (called by PerformGroupSZL)
    void SZLNotAvailable();
	void SZLSystemState();
	void SZLData(void *P, int len);
	void SZL_ID424();
	void SZL_ID131_IDX003();
public:
    TSnap7Server *FServer;
    int FPDULength;
    TS7Worker();
    ~TS7Worker(){};
};

typedef TS7Worker *PS7Worker;
//------------------------------------------------------------------------------
// S7 SERVER CLASS
//------------------------------------------------------------------------------
class TSnap7Server : public TCustomMsgServer
{
private:
    // Read Callback related
    pfn_SrvCallBack OnReadEvent;
    void *FReadUsrPtr;
    void DisposeAll();
    int FindFirstFreeDB();
    int IndexOfDB(word DBNumber);
protected:
    int DBCount;
    int DBLimit;
    PS7Area DB[MaxDB-1]; // DB
    PS7Area HA[5];     // MK,PE,PA,TM,CT
    PS7Area FindDB(word DBNumber);
    PWorkerSocket CreateWorkerSocket(socket_t Sock);
    int RegisterDB(word Number, void *pUsrData, word Size);
    int RegisterSys(int AreaCode, void *pUsrData, word Size);
    int UnregisterDB(word DBNumber);
    int UnregisterSys(int AreaCode);
    // The Read event
    void DoReadEvent(int Sender, longword Code, word RetCode, word Param1,
      word Param2, word Param3, word Param4);
public:
    int WorkInterval;
    byte CpuStatus;
    TSnap7Server();
    ~TSnap7Server();
    int StartTo(const char *Address);
    int GetParam(int ParamNumber, void *pValue);
    int SetParam(int ParamNumber, void *pValue);
    int RegisterArea(int AreaCode, word Index, void *pUsrData, word Size);
    int UnregisterArea(int AreaCode, word Index);
    int LockArea(int AreaCode, word DBNumber);
    int UnlockArea(int AreaCode, word DBNumber);
    // Sets Event callback
    int SetReadEventsCallBack(pfn_SrvCallBack PCallBack, void *UsrPtr);
    friend class TS7Worker;
};
typedef TSnap7Server *PSnap7Server;

#endif // s7_server_h

