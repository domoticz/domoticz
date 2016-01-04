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
#ifndef s7_partner_h
#define s7_partner_h
//---------------------------------------------------------------------------
#include "snap_threads.h"
#include "s7_peer.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------

#define MaxPartners 256
#define MaxAdapters 256
#define csTimeout   1500 // Connection server destruction timeout

const int par_stopped         = 0;   // stopped
const int par_connecting      = 1;   // running and active connecting
const int par_waiting         = 2;   // running and waiting for a connection
const int par_linked          = 3;   // running and connected
const int par_sending         = 4;   // sending data
const int par_receiving       = 5;   // receiving data
const int par_binderror       = 6;   // error starting passive partner

const longword errParMask               = 0xFFF00000;
const longword errParBase               = 0x000FFFFF;

const longword errParAddressInUse       = 0x00200000;
const longword errParNoRoom             = 0x00300000;
const longword errServerNoRoom          = 0x00400000;
const longword errParInvalidParams      = 0x00500000;
const longword errParNotLinked          = 0x00600000;
const longword errParBusy               = 0x00700000;
const longword errParFrameTimeout       = 0x00800000;
const longword errParInvalidPDU         = 0x00900000;
const longword errParSendTimeout        = 0x00A00000;
const longword errParRecvTimeout        = 0x00B00000;
const longword errParSendRefused        = 0x00C00000;
const longword errParNegotiatingPDU     = 0x00D00000;
const longword errParSendingBlock       = 0x00E00000;
const longword errParRecvingBlock       = 0x00F00000;
const longword errParBindError          = 0x01000000;
const longword errParDestroying         = 0x01100000;
const longword errParInvalidParamNumber = 0x01200000; // Invalid param (par_get/set_param)
const longword errParCannotChangeParam  = 0x01300000; // Cannot change because running
const longword errParBufferTooSmall     = 0x01400000; // Raised by LabVIEW wrapper

class TSnap7Partner;
typedef TSnap7Partner *PSnap7Partner;

class TConnectionServer;
typedef TConnectionServer *PConnectionServer;

//------------------------------------------------------------------------------
// CONNECTION SERVERS MANAGER
//------------------------------------------------------------------------------
class TServersManager
{
private:
    PConnectionServer Servers[MaxAdapters];
    TSnapCriticalSection *cs;
    void Lock();
    void Unlock();
    int CreateServer(longword BindAddress, PConnectionServer &Server);
    void AddServer(PConnectionServer Server);
public:
    int ServersCount;
    TServersManager();
    ~TServersManager();

    int GetServer(longword BindAddress, PConnectionServer &Server);
    void RemovePartner(PConnectionServer Server, PSnap7Partner Partner);

};
typedef TServersManager *PServersManager;

//------------------------------------------------------------------------------
// CONNECTION SERVER (Don't inherit from TcpSrv to avoid dependence)
//------------------------------------------------------------------------------

class TConnListenerThread : public TSnapThread
{
private:
	TMsgSocket *FListener;
	TConnectionServer *FServer;
public:
	TConnListenerThread(TMsgSocket *Listener, TConnectionServer *Server)
    {
        FServer=Server;
        FListener=Listener;
        FreeOnTerminate=false;
    };
	void Execute();
};
typedef TConnListenerThread *PConnListenerThread;


class TConnectionServer
{
private:
    TSnapCriticalSection *cs;
    bool FRunning;
    // Bind Address
	char FLocalAddress[16];
	// Server listener
	PConnListenerThread ServerThread;
	// Socket listener
	PMsgSocket SockListener;
    // Finds a partner bound to the address
    PSnap7Partner FindPartner(longword Address);
    // Locks the Partner list
    void Lock();
    // Unlocks the Partner list
    void Unlock();
protected:
	// Workers list
	PSnap7Partner Partners[MaxPartners];
    bool Destroying;
    void Incoming(socket_t Sock);
    int Start();
    int FirstFree();
public:
    int PartnersCount;
    longword LocalBind;
    TConnectionServer();
    ~TConnectionServer();

    int StartTo(const char *Address);
    void Stop();
    int RegisterPartner(PSnap7Partner Partner);
    void RemovePartner(PSnap7Partner Partner);
    friend class TConnListenerThread;
};
typedef TConnectionServer * PConnectionServer;

//------------------------------------------------------------------------------
// PARTNER THREAD
//------------------------------------------------------------------------------
class TPartnerThread : public TSnapThread
{
private:
    TSnap7Partner *FPartner;
    longword FRecoveryTime;
    longword FKaElapsed;
protected:
    void Execute();
public:
    TPartnerThread(TSnap7Partner *Partner, longword RecoveryTime)
    {
        FPartner = Partner;
        FRecoveryTime =RecoveryTime;
        FreeOnTerminate =false;
    };
    ~TPartnerThread(){};
};
typedef TPartnerThread *PPartnerThread;
//------------------------------------------------------------------------------
// S7 PARTNER
//------------------------------------------------------------------------------

typedef struct{
    bool      First;
    bool      Done;
    uintptr_t Offset;
    longword  TotalLength;
    longword  In_R_ID;
    longword  Elapsed;
    byte      Seq_Out;
}TRecvStatus;

typedef struct{
    bool     Done;
    int      Size;
    int      Result;
    longword R_ID;
    longword Count;
}TRecvLast;

extern "C" {
typedef void (S7API *pfn_ParBRecvCallBack)(void * usrPtr, int opResult, longword R_ID, void *pdata, int Size);
typedef void (S7API *pfn_ParBSendCompletion)(void * usrPtr, int opResult);
}

class TSnap7Partner : public TSnap7Peer
{
private:
    PS7ReqHeader PDUH_in;
    void *FRecvUsrPtr;
    void *FSendUsrPtr;
    PSnapEvent SendEvt;
    PSnapEvent RecvEvt;
    PConnectionServer FServer;
    PPartnerThread FWorkerThread;
    bool FSendPending;
    bool FRecvPending;
    TRecvStatus FRecvStatus;
    TRecvLast FRecvLast;
    TPendingBuffer TxBuffer;
    TPendingBuffer RxBuffer;
    longword FSendElapsed;
    bool BindError;
    byte NextByte;
    pfn_ParBRecvCallBack OnBRecv;
    pfn_ParBSendCompletion OnBSend;
    void ClearRecv();
    byte GetNextByte();
    void CloseWorker();
    bool BlockSend();
    bool PickData();
    bool BlockRecv();
    bool ConnectionConfirm();
protected:
    bool Stopping;
    bool Execute();
    void Disconnect();
    bool ConnectToPeer();
    bool PerformFunctionNegotiate();
public:
    bool Active;
    bool Running;
    longword PeerAddress;
    longword SrcAddress;
    int BRecvTimeout;
    int BSendTimeout;
    longword SendTime;
    longword RecvTime;
    longword RecoveryTime;
    longword KeepAliveTime;
    longword BytesSent;
    longword BytesRecv;
    longword SendErrors;
    longword RecvErrors;
    // The partner is linked when the init sequence is terminated
    //(TCP connection + ISO connection + PDU Length negotiation)
    bool Linked;
    TSnap7Partner(bool CreateActive);
    ~TSnap7Partner();
    // Control
    int Start();
    int StartTo(const char *LocAddress, const char *RemAddress, word LocTsap, word RemTsap);
    int Stop();
    int Status();
	int GetParam(int ParamNumber, void * pValue);
	int SetParam(int ParamNumber, void * pValue);
    // Block send
    int BSend(longword R_ID, void *pUsrData, int Size);
    int AsBSend(longword R_ID, void *pUsrData, int Size);
    bool CheckAsBSendCompletion(int &opResult);
    int WaitAsBSendCompletion(longword Timeout);
    int SetSendCallback(pfn_ParBSendCompletion pCompletion, void *usrPtr);
    // Block recv
    int BRecv(longword &R_ID, void *pData, int &Size, longword Timeout);
    bool CheckAsBRecvCompletion(int &opResult, longword &R_ID,
        void *pData, int &Size);
    int SetRecvCallback(pfn_ParBRecvCallBack pCompletion, void *usrPtr);

    friend class TConnectionServer;
    friend class TPartnerThread;
};


#endif // s7_partner_h
