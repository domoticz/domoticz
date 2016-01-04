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
#ifndef s7_micro_client_h
#define s7_micro_client_h
//---------------------------------------------------------------------------
#include "s7_peer.h"
//---------------------------------------------------------------------------

const longword errCliMask                   = 0xFFF00000;
const longword errCliBase                   = 0x000FFFFF;

const longword errCliInvalidParams          = 0x00200000;
const longword errCliJobPending             = 0x00300000;
const longword errCliTooManyItems           = 0x00400000;
const longword errCliInvalidWordLen         = 0x00500000;
const longword errCliPartialDataWritten     = 0x00600000;
const longword errCliSizeOverPDU            = 0x00700000;
const longword errCliInvalidPlcAnswer       = 0x00800000;
const longword errCliAddressOutOfRange      = 0x00900000;
const longword errCliInvalidTransportSize   = 0x00A00000;
const longword errCliWriteDataSizeMismatch  = 0x00B00000;
const longword errCliItemNotAvailable       = 0x00C00000;
const longword errCliInvalidValue           = 0x00D00000;
const longword errCliCannotStartPLC         = 0x00E00000;
const longword errCliAlreadyRun             = 0x00F00000;
const longword errCliCannotStopPLC          = 0x01000000;
const longword errCliCannotCopyRamToRom     = 0x01100000;
const longword errCliCannotCompress         = 0x01200000;
const longword errCliAlreadyStop            = 0x01300000;
const longword errCliFunNotAvailable        = 0x01400000;
const longword errCliUploadSequenceFailed   = 0x01500000;
const longword errCliInvalidDataSizeRecvd   = 0x01600000;
const longword errCliInvalidBlockType       = 0x01700000;
const longword errCliInvalidBlockNumber     = 0x01800000;
const longword errCliInvalidBlockSize       = 0x01900000;
const longword errCliDownloadSequenceFailed = 0x01A00000;
const longword errCliInsertRefused          = 0x01B00000;
const longword errCliDeleteRefused          = 0x01C00000;
const longword errCliNeedPassword           = 0x01D00000;
const longword errCliInvalidPassword        = 0x01E00000;
const longword errCliNoPasswordToSetOrClear = 0x01F00000;
const longword errCliJobTimeout             = 0x02000000;
const longword errCliPartialDataRead        = 0x02100000;
const longword errCliBufferTooSmall         = 0x02200000;
const longword errCliFunctionRefused        = 0x02300000;
const longword errCliDestroying             = 0x02400000;
const longword errCliInvalidParamNumber     = 0x02500000;
const longword errCliCannotChangeParam      = 0x02600000;

const time_t DeltaSecs = 441763200; // Seconds between 1970/1/1 (C time base) and 1984/1/1 (Siemens base)

#pragma pack(1)

// Read/Write Multivars
typedef struct{
   int   Area;
   int   WordLen;
   int   Result;
   int   DBNumber;
   int   Start;
   int   Amount;
   void  *pdata;
} TS7DataItem, *PS7DataItem;

typedef int TS7ResultItems[MaxVars];
typedef TS7ResultItems *PS7ResultItems;

typedef struct {
   int OBCount;
   int FBCount;
   int FCCount;
   int SFBCount;
   int SFCCount;
   int DBCount;
   int SDBCount;
} TS7BlocksList, *PS7BlocksList;

typedef struct {
   int BlkType;
   int BlkNumber;
   int BlkLang;
   int BlkFlags;
   int MC7Size;  // The real size in bytes
   int LoadSize;
   int LocalData;
   int SBBLength;
   int CheckSum;
   int Version;
   // Chars info
   char CodeDate[11];
   char IntfDate[11];
   char Author[9];
   char Family[9];
   char Header[9];
} TS7BlockInfo, *PS7BlockInfo ;

typedef word TS7BlocksOfType[0x2000];
typedef TS7BlocksOfType *PS7BlocksOfType;

typedef struct {
   char Code[21]; // Order Code
   byte V1;       // Version V1.V2.V3
   byte V2;
   byte V3;
} TS7OrderCode, *PS7OrderCode;

typedef struct {
   char ModuleTypeName[33];
   char SerialNumber[25];
   char ASName[25];
   char Copyright[27];
   char ModuleName[25];
} TS7CpuInfo, *PS7CpuInfo;

typedef struct {
   int MaxPduLengt;
   int MaxConnections;
   int MaxMpiRate;
   int MaxBusRate;
} TS7CpInfo, *PS7CpInfo;

// See §33.1 of "System Software for S7-300/400 System and Standard Functions"
// and see SFC51 description too
typedef struct {
   word LENTHDR;
   word N_DR;
} SZL_HEADER, *PSZL_HEADER;

typedef struct {
   SZL_HEADER Header;
   byte Data[0x4000-4];
} TS7SZL, *PS7SZL;

// SZL List of available SZL IDs : same as SZL but List items are big-endian adjusted
typedef struct {
   SZL_HEADER Header;
   word List[0x2000-2];
} TS7SZLList, *PS7SZLList;

// See §33.19 of "System Software for S7-300/400 System and Standard Functions"
typedef struct {
   word  sch_schal;
   word  sch_par;
   word  sch_rel;
   word  bart_sch;
   word  anl_sch;
} TS7Protection, *PS7Protection;

#define s7opNone               0
#define s7opReadArea           1
#define s7opWriteArea          2
#define s7opReadMultiVars      3
#define s7opWriteMultiVars     4
#define s7opDBGet              5
#define s7opUpload             6
#define s7opDownload           7
#define s7opDelete             8
#define s7opListBlocks         9
#define s7opAgBlockInfo       10
#define s7opListBlocksOfType  11
#define s7opReadSzlList       12
#define s7opReadSZL           13
#define s7opGetDateTime       14
#define s7opSetDateTime       15
#define s7opGetOrderCode      16
#define s7opGetCpuInfo        17
#define s7opGetCpInfo         18
#define s7opGetPlcStatus      19
#define s7opPlcHotStart       20
#define s7opPlcColdStart      21
#define s7opCopyRamToRom      22
#define s7opCompress          23
#define s7opPlcStop           24
#define s7opGetProtection     25
#define s7opSetPassword       26
#define s7opClearPassword     27
#define s7opDBFill            28

// Param Number (to use with setparam)

// Low level : change them to experiment new connections, their defaults normally work well
const int pc_iso_SendTimeout   = 6;
const int pc_iso_RecvTimeout   = 7;
const int pc_iso_ConnTimeout   = 8;
const int pc_iso_SrcRef        = 1;
const int pc_iso_DstRef        = 2;
const int pc_iso_SrcTSAP       = 3;
const int pc_iso_DstTSAP       = 4;
const int pc_iso_IsoPduSize    = 5;

// Client Connection Type
const word CONNTYPE_PG         = 0x01;  // Connect to the PLC as a PG
const word CONNTYPE_OP         = 0x02;  // Connect to the PLC as an OP
const word CONNTYPE_BASIC      = 0x03;  // Basic connection 

#pragma pack()

// Internal struct for operations
// Commands are not executed directly in the function such as "DBRead(...",
// but this struct is filled and then PerformOperation() is called.
// This allow us to implement async function very easily.

struct TSnap7Job 
{
    int Op;        // Operation Code
    int Result;    // Operation result
    bool Pending;  // A Job is pending
    longword Time; // Job Execution time
    // Read/Write
    int Area;      // Also used for Block type and Block of type
    int Number;    // Used for DB Number, Block number
    int Start;     // Offset start
    int WordLen;   // Word length
    // SZL
    int ID;        // SZL ID
    int Index;     // SZL Index
    // ptr info
    void * pData;  // User data pointer
    int Amount;    // Items amount/Size in input
    int *pAmount;  // Items amount/Size in output
    // Generic
    int IParam;   // Used for full upload and CopyRamToRom extended timeout
};

class TSnap7MicroClient: public TSnap7Peer
{
private:
    void FillTime(word SiemensTime, char *PTime);
    byte BCDtoByte(byte B);
    byte WordToBCD(word Value);
    int opReadArea();
    int opWriteArea();
    int opReadMultiVars();
    int opWriteMultiVars();
    int opListBlocks();
    int opListBlocksOfType();
    int opAgBlockInfo();
    int opDBGet();
    int opDBFill();
    int opUpload();
    int opDownload();
    int opDelete();
    int opReadSZL();
    int opReadSZLList();
    int opGetDateTime();
    int opSetDateTime();
    int opGetOrderCode();
    int opGetCpuInfo();
    int opGetCpInfo();
    int opGetPlcStatus();
    int opPlcStop();
    int opPlcHotStart();
    int opPlcColdStart();
    int opCopyRamToRom();
    int opCompress();
    int opGetProtection();
    int opSetPassword();
    int opClearPassword();
    int CpuError(int Error);
    longword DWordAt(void * P);
    int CheckBlock(int BlockType, int BlockNum,  void *pBlock,  int Size);
    int SubBlockToBlock(int SBB);
protected:
    word ConnectionType;
    longword JobStart;
    TSnap7Job Job;
    int DataSizeByte(int WordLength);
    int opSize; // last operation size
    int PerformOperation();
public:
    TS7Buffer opData;
	TSnap7MicroClient();
    ~TSnap7MicroClient();
    int Reset(bool DoReconnect);
    void SetConnectionParams(const char *RemAddress, word LocalTSAP, word RemoteTsap);
    void SetConnectionType(word ConnType);
	int ConnectTo(const char *RemAddress, int Rack, int Slot);
    int Connect();
	int Disconnect();
	int GetParam(int ParamNumber, void *pValue);
	int SetParam(int ParamNumber, void *pValue);
    // Fundamental Data I/O functions
    int ReadArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void * pUsrData);
    int WriteArea(int Area, int DBNumber, int Start, int Amount, int WordLen, void * pUsrData);
    int ReadMultiVars(PS7DataItem Item, int ItemsCount);
    int WriteMultiVars(PS7DataItem Item, int ItemsCount);
    // Data I/O Helper functions
    int DBRead(int DBNumber, int Start, int Size, void * pUsrData);
    int DBWrite(int DBNumber, int Start, int Size, void * pUsrData);
    int MBRead(int Start, int Size, void * pUsrData);
    int MBWrite(int Start, int Size, void * pUsrData);
    int EBRead(int Start, int Size, void * pUsrData);
    int EBWrite(int Start, int Size, void * pUsrData);
    int ABRead(int Start, int Size, void * pUsrData);
    int ABWrite(int Start, int Size, void * pUsrData);
    int TMRead(int Start, int Amount, void * pUsrData);
    int TMWrite(int Start, int Amount, void * pUsrData);
    int CTRead(int Start, int Amount, void * pUsrData);
    int CTWrite(int Start, int Amount, void * pUsrData);
    // Directory functions
    int ListBlocks(PS7BlocksList pUsrData);
    int GetAgBlockInfo(int BlockType, int BlockNum, PS7BlockInfo pUsrData);
    int GetPgBlockInfo(void * pBlock, PS7BlockInfo pUsrData, int Size);
    int ListBlocksOfType(int BlockType, TS7BlocksOfType *pUsrData, int & ItemsCount);
    // Blocks functions
    int Upload(int BlockType, int BlockNum, void * pUsrData, int & Size);
    int FullUpload(int BlockType, int BlockNum, void * pUsrData, int & Size);
    int Download(int BlockNum, void * pUsrData, int Size);
    int Delete(int BlockType, int BlockNum);
    int DBGet(int DBNumber, void * pUsrData, int & Size);
    int DBFill(int DBNumber, int FillChar);
    // Date/Time functions
    int GetPlcDateTime(tm &DateTime);
    int SetPlcDateTime(tm * DateTime);
    int SetPlcSystemDateTime();
    // System Info functions
    int GetOrderCode(PS7OrderCode pUsrData);
    int GetCpuInfo(PS7CpuInfo pUsrData);
    int GetCpInfo(PS7CpInfo pUsrData);
    int ReadSZL(int ID, int Index, PS7SZL pUsrData, int &Size);
    int ReadSZLList(PS7SZLList pUsrData, int &ItemsCount);
    // Control functions
    int PlcHotStart();
    int PlcColdStart();
    int PlcStop();
    int CopyRamToRom(int Timeout);
    int Compress(int Timeout);
    int GetPlcStatus(int &Status);
    // Security functions
    int GetProtection(PS7Protection pUsrData);
    int SetSessionPassword(char *Password);
    int ClearSessionPassword();
    // Properties
    bool Busy(){ return Job.Pending; };
    int Time(){ return int(Job.Time);}
};

typedef TSnap7MicroClient *PSnap7MicroClient;

//---------------------------------------------------------------------------
#endif // s7_micro_client_h
