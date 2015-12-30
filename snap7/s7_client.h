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
#ifndef s7_client_h
#define s7_client_h
//---------------------------------------------------------------------------
#include "snap_threads.h"
#include "s7_micro_client.h"
//---------------------------------------------------------------------------

extern "C" {
typedef void (S7API *pfn_CliCompletion) (void * usrPtr, int opCode, int opResult);
}
class TSnap7Client;

class TClientThread: public TSnapThread
{
private:
	TSnap7Client * FClient;
public:
     TClientThread(TSnap7Client *Client)
     {
           FClient = Client;
     }
	void Execute();
};
//---------------------------------------------------------------------------
class TSnap7Client: public TSnap7MicroClient
{
private:
    TClientThread *FThread;
    void CloseThread();
    void OpenThread();
    void StartAsyncJob();
protected:
    PSnapEvent EvtJob;
    PSnapEvent EvtComplete;
    pfn_CliCompletion CliCompletion;
    void *FUsrPtr;
    void DoCompletion();
public:
    friend class TClientThread;
    TSnap7Client();
    ~TSnap7Client();
    int Reset(bool DoReconnect);
    int SetAsCallback(pfn_CliCompletion pCompletion, void * usrPtr);
    int GetParam(int ParamNumber, void *pValue);
    int SetParam(int ParamNumber, void *pValue);
    // Async functions
    bool CheckAsCompletion( int & opResult);
    int WaitAsCompletion(unsigned long Timeout);
    int AsReadArea(int Area, int DBNumber, int Start, int Amount, int WordLen,  void * pUsrData);
    int AsWriteArea(int Area, int DBNumber, int Start, int Amount, int WordLen,  void * pUsrData);
    int AsListBlocksOfType(int BlockType,  PS7BlocksOfType pUsrData,   int & ItemsCount);
    int AsReadSZL(int ID, int Index,  PS7SZL pUsrData, int & Size);
    int AsReadSZLList(PS7SZLList pUsrData, int &ItemsCount);
    int AsUpload(int BlockType, int BlockNum,  void * pUsrData,   int & Size);
    int AsFullUpload(int BlockType, int BlockNum,  void * pUsrData,   int & Size);
    int AsDownload(int BlockNum,  void * pUsrData,  int Size);
    int AsCopyRamToRom(int Timeout);
    int AsCompress(int Timeout);
    int AsDBRead(int DBNumber, int Start, int Size,  void * pUsrData);
    int AsDBWrite(int DBNumber, int Start, int Size,  void * pUsrData);
    int AsMBRead(int Start, int Size, void * pUsrData);
    int AsMBWrite(int Start, int Size, void * pUsrData);
    int AsEBRead(int Start, int Size, void * pUsrData);
    int AsEBWrite(int Start, int Size, void * pUsrData);
    int AsABRead(int Start, int Size, void * pUsrData);
    int AsABWrite(int Start, int Size, void * pUsrData);
    int AsTMRead(int Start, int Amount, void * pUsrData);
    int AsTMWrite(int Start, int Amount, void * pUsrData);
    int AsCTRead(int Start, int Amount, void * pUsrData);
    int AsCTWrite(int Start, int Amount, void * pUsrData);
    int AsDBGet(int DBNumber,  void * pUsrData,   int & Size);
    int AsDBFill(int DBNumber,  int FillChar);
};

typedef TSnap7Client *PSnap7Client;

//---------------------------------------------------------------------------
#endif // s7_client_h
