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
#ifndef snap7_libmain_h
#define snap7_libmain_h
//---------------------------------------------------------------------------
#include "s7_client.h"
#include "s7_server.h"
#include "s7_partner.h"
#include "s7_text.h"
//---------------------------------------------------------------------------

const int mkEvent  = 0;
const int mkLog    = 1;

typedef uintptr_t S7Object; // multi platform/processor object reference

//==============================================================================
// CLIENT EXPORT LIST - Sync functions
//==============================================================================
EXPORTSPEC S7Object S7API Cli_Create();
EXPORTSPEC void S7API Cli_Destroy(S7Object &Client);
EXPORTSPEC int S7API Cli_Connect(S7Object Client);
EXPORTSPEC int S7API Cli_SetConnectionParams(S7Object Client, const char *Address, word LocalTSAP, word RemoteTSAP);
EXPORTSPEC int S7API Cli_SetConnectionType(S7Object Client, word ConnectionType);
EXPORTSPEC int S7API Cli_ConnectTo(S7Object Client, const char *Address, int Rack, int Slot);
EXPORTSPEC int S7API Cli_Disconnect(S7Object Client);
EXPORTSPEC int S7API Cli_GetParam(S7Object Client, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Cli_SetParam(S7Object Client, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Cli_SetAsCallback(S7Object Client, pfn_CliCompletion pCompletion, void *usrPtr);
// Data I/O functions
EXPORTSPEC int S7API Cli_ReadArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
EXPORTSPEC int S7API Cli_WriteArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
EXPORTSPEC int S7API Cli_ReadMultiVars(S7Object Client, PS7DataItem Item, int ItemsCount);
EXPORTSPEC int S7API Cli_WriteMultiVars(S7Object Client, PS7DataItem Item, int ItemsCount);
// Data I/O Lean functions
EXPORTSPEC int S7API Cli_DBRead(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_DBWrite(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_MBRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_MBWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_EBRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_EBWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_ABRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_ABWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_TMRead(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_TMWrite(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_CTRead(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_CTWrite(S7Object Client, int Start, int Amount, void *pUsrData);
// Directory functions
EXPORTSPEC int S7API Cli_ListBlocks(S7Object Client, TS7BlocksList *pUsrData);
EXPORTSPEC int S7API Cli_GetAgBlockInfo(S7Object Client, int BlockType, int BlockNum, TS7BlockInfo *pUsrData);
EXPORTSPEC int S7API Cli_GetPgBlockInfo(S7Object Client, void *pBlock, TS7BlockInfo *pUsrData, int Size);
EXPORTSPEC int S7API Cli_ListBlocksOfType(S7Object Client, int BlockType, TS7BlocksOfType *pUsrData, int &ItemsCount);
// Blocks functions
EXPORTSPEC int S7API Cli_Upload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_FullUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_Download(S7Object Client, int BlockNum, void *pUsrData, int Size);
EXPORTSPEC int S7API Cli_Delete(S7Object Client, int BlockType, int BlockNum);
EXPORTSPEC int S7API Cli_DBGet(S7Object Client, int DBNumber, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_DBFill(S7Object Client, int DBNumber, int FillChar);
// Date/Time functions
EXPORTSPEC int S7API Cli_GetPlcDateTime(S7Object Client, tm &DateTime);
EXPORTSPEC int S7API Cli_SetPlcDateTime(S7Object Client, tm *DateTime);
EXPORTSPEC int S7API Cli_SetPlcSystemDateTime(S7Object Client);
// System Info functions
EXPORTSPEC int S7API Cli_GetOrderCode(S7Object Client, TS7OrderCode *pUsrData);
EXPORTSPEC int S7API Cli_GetCpuInfo(S7Object Client, TS7CpuInfo *pUsrData);
EXPORTSPEC int S7API Cli_GetCpInfo(S7Object Client, TS7CpInfo *pUsrData);
EXPORTSPEC int S7API Cli_ReadSZL(S7Object Client, int ID, int Index, TS7SZL *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_ReadSZLList(S7Object Client, TS7SZLList *pUsrData, int &ItemsCount);
// Control functions
EXPORTSPEC int S7API Cli_PlcHotStart(S7Object Client);
EXPORTSPEC int S7API Cli_PlcColdStart(S7Object Client);
EXPORTSPEC int S7API Cli_PlcStop(S7Object Client);
EXPORTSPEC int S7API Cli_CopyRamToRom(S7Object Client, int Timeout);
EXPORTSPEC int S7API Cli_Compress(S7Object Client, int Timeout);
EXPORTSPEC int S7API Cli_GetPlcStatus(S7Object Client, int &Status);
// Security functions
EXPORTSPEC int S7API Cli_GetProtection(S7Object Client, TS7Protection *pUsrData);
EXPORTSPEC int S7API Cli_SetSessionPassword(S7Object Client, char *Password);
EXPORTSPEC int S7API Cli_ClearSessionPassword(S7Object Client);
// Low level
EXPORTSPEC int S7API Cli_IsoExchangeBuffer(S7Object Client, void *pUsrData, int &Size);
// Misc
EXPORTSPEC int S7API Cli_GetExecTime(S7Object Client, int &Time);
EXPORTSPEC int S7API Cli_GetLastError(S7Object Client, int &LastError);
EXPORTSPEC int S7API Cli_GetPduLength(S7Object Client, int &Requested, int &Negotiated);
EXPORTSPEC int S7API Cli_ErrorText(int Error, char *Text, int TextLen);
EXPORTSPEC int S7API Cli_GetConnected(S7Object Client, int &Connected);
//==============================================================================
//  CLIENT EXPORT LIST - Async functions
//==============================================================================
EXPORTSPEC int S7API Cli_AsReadArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
EXPORTSPEC int S7API Cli_AsWriteArea(S7Object Client, int Area, int DBNumber, int Start, int Amount, int WordLen, void *pUsrData);
EXPORTSPEC int S7API Cli_AsDBRead(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsDBWrite(S7Object Client, int DBNumber, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsMBRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsMBWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsEBRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsEBWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsABRead(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsABWrite(S7Object Client, int Start, int Size, void *pUsrData);
EXPORTSPEC int S7API Cli_AsTMRead(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_AsTMWrite(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_AsCTRead(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_AsCTWrite(S7Object Client, int Start, int Amount, void *pUsrData);
EXPORTSPEC int S7API Cli_AsListBlocksOfType(S7Object Client, int BlockType, TS7BlocksOfType *pUsrData, int &ItemsCount);
EXPORTSPEC int S7API Cli_AsReadSZL(S7Object Client, int ID, int Index, TS7SZL *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_AsReadSZLList(S7Object Client, TS7SZLList *pUsrData, int &ItemsCount);
EXPORTSPEC int S7API Cli_AsUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_AsFullUpload(S7Object Client, int BlockType, int BlockNum, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_AsDownload(S7Object Client, int BlockNum, void *pUsrData, int Size);
EXPORTSPEC int S7API Cli_AsCopyRamToRom(S7Object Client, int Timeout);
EXPORTSPEC int S7API Cli_AsCompress(S7Object Client, int Timeout);
EXPORTSPEC int S7API Cli_AsDBGet(S7Object Client, int DBNumber, void *pUsrData, int &Size);
EXPORTSPEC int S7API Cli_AsDBFill(S7Object Client, int DBNumber, int FillChar);
EXPORTSPEC int S7API Cli_CheckAsCompletion(S7Object Client, int &opResult);
EXPORTSPEC int S7API Cli_WaitAsCompletion(S7Object Client, int Timeout);
//==============================================================================
//  SERVER EXPORT LIST
//==============================================================================
EXPORTSPEC S7Object S7API Srv_Create();
EXPORTSPEC void S7API Srv_Destroy(S7Object &Server);
EXPORTSPEC int S7API Srv_GetParam(S7Object Server, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Srv_SetParam(S7Object Server, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Srv_Start(S7Object Server);
EXPORTSPEC int S7API Srv_StartTo(S7Object Server, const char *Address);
EXPORTSPEC int S7API Srv_Stop(S7Object Server);
// Data
EXPORTSPEC int S7API Srv_RegisterArea(S7Object Server, int AreaCode, word Index, void *pUsrData, int Size);
EXPORTSPEC int S7API Srv_UnregisterArea(S7Object Server, int AreaCode, word Index);
EXPORTSPEC int S7API Srv_LockArea(S7Object Server, int AreaCode, word Index);
EXPORTSPEC int S7API Srv_UnlockArea(S7Object Server, int AreaCode, word Index);
// Events
EXPORTSPEC int S7API Srv_ClearEvents(S7Object Server);
EXPORTSPEC int S7API Srv_PickEvent(S7Object Server, TSrvEvent *pEvent, int &EvtReady);
EXPORTSPEC int S7API Srv_GetMask(S7Object Server, int MaskKind, longword &Mask);
EXPORTSPEC int S7API Srv_SetMask(S7Object Server, int MaskKind, longword Mask);
EXPORTSPEC int S7API Srv_SetEventsCallback(S7Object Server, pfn_SrvCallBack pCallback, void *usrPtr);
EXPORTSPEC int S7API Srv_SetReadEventsCallback(S7Object Server, pfn_SrvCallBack pCallback, void *usrPtr);
EXPORTSPEC int S7API Srv_EventText(TSrvEvent &Event, char *Text, int TextLen);
// Misc
EXPORTSPEC int S7API Srv_GetStatus(S7Object Server, int &ServerStatus, int &CpuStatus, int &ClientsCount);
EXPORTSPEC int S7API Srv_SetCpuStatus(S7Object Server, int CpuStatus);
EXPORTSPEC int S7API Srv_ErrorText(int Error, char *Text, int TextLen);
//==============================================================================
//  PARTNER EXPORT LIST
//==============================================================================
EXPORTSPEC S7Object S7API Par_Create(int Active);
EXPORTSPEC void S7API Par_Destroy(S7Object &Partner);
EXPORTSPEC int S7API Par_GetParam(S7Object Partner, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Par_SetParam(S7Object Partner, int ParamNumber, void *pValue);
EXPORTSPEC int S7API Par_Start(S7Object Partner);
EXPORTSPEC int S7API Par_StartTo(S7Object Partner, const char *LocalAddress, const char *RemoteAddress, 
	word LocTsap, word RemTsap);
EXPORTSPEC int S7API Par_Stop(S7Object Partner);
// BSend
EXPORTSPEC int S7API Par_BSend(S7Object Partner, longword R_ID, void *pUsrData, int Size);
EXPORTSPEC int S7API Par_AsBSend(S7Object Partner, longword R_ID, void *pUsrData, int Size);
EXPORTSPEC int S7API Par_CheckAsBSendCompletion(S7Object Partner, int &opResult);
EXPORTSPEC int S7API Par_WaitAsBSendCompletion(S7Object Partner, longword Timeout);
EXPORTSPEC int S7API Par_SetSendCallback(S7Object Partner, pfn_ParBSendCompletion pCompletion, void *usrPtr);
// BRecv
EXPORTSPEC int S7API Par_BRecv(S7Object Partner, longword &R_ID, void *pData, int &Size, longword Timeout);
EXPORTSPEC int S7API Par_CheckAsBRecvCompletion(S7Object Partner, int &opResult, longword &R_ID,
    void *pData, int &Size);
EXPORTSPEC int S7API Par_SetRecvCallback(S7Object Partner, pfn_ParBRecvCallBack pCompletion, void *usrPtr);
// Stat
EXPORTSPEC int S7API Par_GetTimes(S7Object Partner, longword &SendTime, longword &RecvTime);
EXPORTSPEC int S7API Par_GetStats(S7Object Partner, longword &BytesSent, longword &BytesRecv,
    longword &SendErrors, longword &RecvErrors);
EXPORTSPEC int S7API Par_GetLastError(S7Object Partner, int &LastError);
EXPORTSPEC int S7API Par_GetStatus(S7Object Partner, int &Status);
EXPORTSPEC int S7API Par_ErrorText(int Error, char *Text, int TextLen);



#endif // snap7_libmain_h
