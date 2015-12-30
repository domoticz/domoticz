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
#ifndef s7_types_h
#define s7_types_h
//------------------------------------------------------------------------------
#include "s7_isotcp.h"
//------------------------------------------------------------------------------
//                               EXPORT CONSTANTS
// Everything added in this section has to be copied into wrappers interface
//------------------------------------------------------------------------------

#ifdef OS_WINDOWS
#define SM7API __stdcall
#else
#define SM7API
#endif

  // Area ID
const byte S7AreaPE   =	0x81;
const byte S7AreaPA   =	0x82;
const byte S7AreaMK   =	0x83;
const byte S7AreaDB   =	0x84;
const byte S7AreaCT   =	0x1C;
const byte S7AreaTM   =	0x1D;

const int MaxVars     = 20;

const int S7WLBit     = 0x01;
const int S7WLByte    = 0x02;
const int S7WLChar    = 0x03;
const int S7WLWord    = 0x04;
const int S7WLInt     = 0x05;
const int S7WLDWord   = 0x06;
const int S7WLDInt    = 0x07;
const int S7WLReal    = 0x08;
const int S7WLCounter = 0x1C;
const int S7WLTimer   = 0x1D;

  // Block type
const byte Block_OB   = 0x38;
const byte Block_DB   = 0x41;
const byte Block_SDB  = 0x42;
const byte Block_FC   = 0x43;
const byte Block_SFC  = 0x44;
const byte Block_FB   = 0x45;
const byte Block_SFB  = 0x46;

  // Sub Block Type
const byte SubBlk_OB  = 0x08;
const byte SubBlk_DB  = 0x0A;
const byte SubBlk_SDB = 0x0B;
const byte SubBlk_FC  = 0x0C;
const byte SubBlk_SFC = 0x0D;
const byte SubBlk_FB  = 0x0E;
const byte SubBlk_SFB = 0x0F;

  // Block languages
const byte BlockLangAWL       = 0x01;
const byte BlockLangKOP       = 0x02;
const byte BlockLangFUP       = 0x03;
const byte BlockLangSCL       = 0x04;
const byte BlockLangDB        = 0x05;
const byte BlockLangGRAPH     = 0x06;

  // CPU status
const byte S7CpuStatusUnknown = 0x00;
const byte S7CpuStatusRun     = 0x08;
const byte S7CpuStatusStop    = 0x04;

const longword evcSnap7Base           = 0x00008000;
// S7 Server Event Code
const longword evcPDUincoming  	      = 0x00010000;
const longword evcDataRead            = 0x00020000;
const longword evcDataWrite    	      = 0x00040000;
const longword evcNegotiatePDU        = 0x00080000;
const longword evcReadSZL             = 0x00100000;
const longword evcClock               = 0x00200000;
const longword evcUpload              = 0x00400000;
const longword evcDownload            = 0x00800000;
const longword evcDirectory           = 0x01000000;
const longword evcSecurity            = 0x02000000;
const longword evcControl             = 0x04000000;
const longword evcReserved_08000000   = 0x08000000;
const longword evcReserved_10000000   = 0x10000000;
const longword evcReserved_20000000   = 0x20000000;
const longword evcReserved_40000000   = 0x40000000;
const longword evcReserved_80000000   = 0x80000000;
// Event SubCodes
const word evsUnknown                 = 0x0000;
const word evsStartUpload             = 0x0001;
const word evsStartDownload           = 0x0001;
const word evsGetBlockList            = 0x0001;
const word evsStartListBoT            = 0x0002;
const word evsListBoT                 = 0x0003;
const word evsGetBlockInfo            = 0x0004;
const word evsGetClock                = 0x0001;
const word evsSetClock                = 0x0002;
const word evsSetPassword             = 0x0001;
const word evsClrPassword             = 0x0002;
// Event Result
const word evrNoError                 = 0;
const word evrFragmentRejected        = 0x0001;
const word evrMalformedPDU            = 0x0002;
const word evrSparseBytes             = 0x0003;
const word evrCannotHandlePDU         = 0x0004;
const word evrNotImplemented          = 0x0005;
const word evrErrException            = 0x0006;
const word evrErrAreaNotFound         = 0x0007;
const word evrErrOutOfRange           = 0x0008;
const word evrErrOverPDU              = 0x0009;
const word evrErrTransportSize        = 0x000A;
const word evrInvalidGroupUData       = 0x000B;
const word evrInvalidSZL              = 0x000C;
const word evrDataSizeMismatch        = 0x000D;
const word evrCannotUpload            = 0x000E;
const word evrCannotDownload          = 0x000F;
const word evrUploadInvalidID         = 0x0010;
const word evrResNotFound             = 0x0011;

  // Async mode
const int amPolling   = 0;
const int amEvent     = 1;
const int amCallBack  = 2;

//------------------------------------------------------------------------------
//                                  PARAMS LIST            
// Notes for Local/Remote Port
//   If the local port for a server and remote port for a client is != 102 they
//   will be *no more compatible with S7 IsoTCP*
//   A good reason to change them could be inside a debug session under Unix.
//   Increasing the port over 1024 avoids the need of be root. 
//   Obviously you need to work with the couple Snap7Client/Snap7Server and change
//   both, or, use iptable and nat the port.
//------------------------------------------------------------------------------
const int p_u16_LocalPort  	    = 1;
const int p_u16_RemotePort 	    = 2;
const int p_i32_PingTimeout	    = 3;
const int p_i32_SendTimeout     = 4;
const int p_i32_RecvTimeout     = 5;
const int p_i32_WorkInterval    = 6;
const int p_u16_SrcRef          = 7;
const int p_u16_DstRef          = 8;
const int p_u16_SrcTSap         = 9;
const int p_i32_PDURequest      = 10;
const int p_i32_MaxClients      = 11;
const int p_i32_BSendTimeout    = 12;
const int p_i32_BRecvTimeout    = 13;
const int p_u32_RecoveryTime    = 14;
const int p_u32_KeepAliveTime   = 15;

// Bool param is passed as int32_t : 0->false, 1->true
// String param (only set) is passed as pointer

typedef int16_t   *Pint16_t;     
typedef uint16_t  *Puint16_t;     
typedef int32_t   *Pint32_t;     
typedef uint32_t  *Puint32_t;     
typedef int64_t   *Pint64_t;     
typedef uint64_t  *Puint64_t;     
typedef uintptr_t *Puintptr_t;
//-----------------------------------------------------------------------------
//                               INTERNALS CONSTANTS
//------------------------------------------------------------------------------

const word DBMaxName = 0xFFFF; // max number (name) of DB

const longword errS7Mask         = 0xFFF00000;
const longword errS7Base         = 0x000FFFFF;
const longword errS7notConnected = errS7Base+0x0001; // Client not connected
const longword errS7InvalidMode  = errS7Base+0x0002; // Requested a connection to...
const longword errS7InvalidPDUin = errS7Base+0x0003; // Malformed input PDU

// S7 outcoming Error code
const word Code7Ok                      = 0x0000;
const word Code7AddressOutOfRange       = 0x0005;
const word Code7InvalidTransportSize    = 0x0006;
const word Code7WriteDataSizeMismatch   = 0x0007;
const word Code7ResItemNotAvailable   	= 0x000A;
const word Code7ResItemNotAvailable1    = 0xD209;
const word Code7InvalidValue   	        = 0xDC01;
const word Code7NeedPassword            = 0xD241;
const word Code7InvalidPassword         = 0xD602;
const word Code7NoPasswordToClear   	= 0xD604;
const word Code7NoPasswordToSet         = 0xD605;
const word Code7FunNotAvailable         = 0x8104;
const word Code7DataOverPDU             = 0x8500;

// Result transport size
const byte TS_ResBit   = 0x03;
const byte TS_ResByte  = 0x04;
const byte TS_ResInt   = 0x05;
const byte TS_ResReal  = 0x07;
const byte TS_ResOctet = 0x09;

// Client Job status (lib internals, not S7)
const int JobComplete  = 0;
const int JobPending   = 1;

// Control codes
const word CodeControlUnknown   = 0;
const word CodeControlColdStart = 1;      // Cold start
const word CodeControlWarmStart = 2;      // Warm start
const word CodeControlStop      = 3;      // Stop
const word CodeControlCompress  = 4;      // Compress
const word CodeControlCpyRamRom = 5;      // Copy Ram to Rom
const word CodeControlInsDel    = 6;      // Insert in working ram the block downloaded
					  // Delete from working ram the block selected
// PDU Type
const byte PduType_request      = 1;      // family request
const byte PduType_response     = 3;      // family response
const byte PduType_userdata     = 7;      // family user data

// PDU Functions
const byte pduResponse    	= 0x02;   // Response (when error)
const byte pduFuncRead    	= 0x04;   // Read area
const byte pduFuncWrite   	= 0x05;   // Write area
const byte pduNegotiate   	= 0xF0;   // Negotiate PDU length
const byte pduStart         = 0x28;   // CPU start
const byte pduStop          = 0x29;   // CPU stop
const byte pduStartUpload   = 0x1D;   // Start Upload
const byte pduUpload        = 0x1E;   // Upload
const byte pduEndUpload     = 0x1F;   // EndUpload
const byte pduReqDownload   = 0x1A;   // Start Download request
const byte pduDownload      = 0x1B;   // Download request
const byte pduDownloadEnded = 0x1C;   // Download end request
const byte pduControl   	= 0x28;   // Control (insert/delete..)

// PDU SubFunctions
const byte SFun_ListAll   	= 0x01;   // List all blocks
const byte SFun_ListBoT   	= 0x02;   // List Blocks of type
const byte SFun_BlkInfo   	= 0x03;   // Get Block info
const byte SFun_ReadSZL   	= 0x01;   // Read SZL 
const byte SFun_ReadClock   = 0x01;   // Read Clock (Date and Time)
const byte SFun_SetClock  	= 0x02;   // Set Clock (Date and Time)
const byte SFun_EnterPwd    = 0x01;   // Enter password    for this session
const byte SFun_CancelPwd   = 0x02;   // Cancel password    for this session
const byte SFun_Insert   	= 0x50;   // Insert block
const byte SFun_Delete   	= 0x42;   // Delete block

typedef tm *PTimeStruct;

//==============================================================================
//                                   HEADERS
//==============================================================================
#pragma pack(1)

// Incoming header, it will be mapped onto IsoPDU payload
typedef struct {
	byte    P;        // Telegram ID, always 32
	byte    PDUType;  // Header type 1 or 7
	word    AB_EX;    // AB currently unknown, maybe it can be used for long numbers.
	word    Sequence; // Message ID. This can be used to make sure a received answer
	word    ParLen;   // Length of parameters which follow this header
	word    DataLen;  // Length of data which follow the parameters
}TS7ReqHeader;

typedef TS7ReqHeader* PS7ReqHeader;

// Outcoming 12 bytes header , response for Request type 1
typedef struct{
	byte    P;        // Telegram ID, always 32
	byte    PDUType;  // Header type 2 or 3
	word    AB_EX;    // AB currently unknown, maybe it can be used for long numbers.
	word    Sequence; // Message ID. This can be used to make sure a received answer
	word    ParLen;   // Length of parameters which follow this header
	word    DataLen;  // Length of data which follow the parameters
	word    Error;    // Error code
} TS7ResHeader23;

typedef TS7ResHeader23* PS7ResHeader23;

// Outcoming 10 bytes header , response for Request type 7
typedef struct{
	byte    P;        // Telegram ID, always 32
	byte    PDUType;  // Header type 1 or 7
	word    AB_EX;    // AB currently unknown, maybe it can be used for long numbers.
	word    Sequence; // Message ID. This can be used to make sure a received answer
	word    ParLen;   // Length of parameters which follow this header
	word    DataLen;  // Length of data which follow the parameters
}TS7ResHeader17;

typedef TS7ResHeader17* PS7ResHeader17;

// Outcoming 10 bytes header , response for Request type 8 (server control)
typedef struct {
	byte    P;        // Telegram ID, always 32
	byte    PDUType;  // Header type 8
	word    AB_EX;    // Zero
	word    Sequence; // Message ID. This can be used to make sure a received answer
	word    DataLen;  // Length of data which follow this header
	word    Error;    // Error code
} TS7ResHeader8;

typedef TS7ResHeader8* PS7ResHeader8;

// Outcoming answer buffer header type 2 or header type 3
typedef struct{
	TS7ResHeader23 Header;
	byte   ResData [IsoPayload_Size - sizeof(TS7ResHeader23)];
} TS7Answer23;

typedef TS7Answer23* PS7Answer23;

// Outcoming buffer header type 1 or header type 7
typedef struct {
	TS7ResHeader17 Header;
	byte   ResData [IsoPayload_Size - sizeof(TS7ResHeader17)];
} TS7Answer17;

typedef TS7Answer17* PS7Answer17;

typedef byte   TTimeBuffer[8];
typedef byte   *PTimeBuffer[8];

typedef struct{
   byte bcd_year;
   byte bcd_mon;
   byte bcd_day;
   byte bcd_hour;
   byte bcd_min;
   byte bcd_sec;
   byte bcd_himsec;
   byte bcd_dow;
}TS7Time, *PS7Time;

typedef byte   TS7Buffer[65536];
typedef byte   *PS7Buffer;

const int ReqHeaderSize   = sizeof(TS7ReqHeader);
const int ResHeaderSize23 = sizeof(TS7ResHeader23);
const int ResHeaderSize17 = sizeof(TS7ResHeader17);

// Most used request type parameters record
typedef struct {
	byte   Head[3];// 0x00 0x01 0x12
	byte   Plen;   // par len 0x04
	byte   Uk;     // unknown
	byte   Tg;     // type and group  (4 bits type and 4 bits group)
	byte   SubFun; // subfunction
	byte   Seq;    // sequence
}TReqFunTypedParams;

//==============================================================================
//                            FUNCTION NEGOTIATE
//==============================================================================
typedef struct {
	byte    FunNegotiate;
	byte    Unknown;
	word    ParallelJobs_1;
	word    ParallelJobs_2;
	word    PDULength;
}TReqFunNegotiateParams;

typedef TReqFunNegotiateParams* PReqFunNegotiateParams;

typedef struct {
	byte    FunNegotiate;
	byte    Unknown;
	word    ParallelJobs_1;
	word    ParallelJobs_2;
	word    PDULength;
}TResFunNegotiateParams;

typedef TResFunNegotiateParams* PResFunNegotiateParams;

//==============================================================================
//                               FUNCTION READ
//==============================================================================
typedef struct {
	byte    ItemHead[3];
	byte    TransportSize;
	word    Length;
	word    DBNumber;
	byte    Area;
	byte    Address[3];
}TReqFunReadItem, * PReqFunReadItem;

//typedef TReqFunReadItem;

typedef struct {
	byte   FunRead;
	byte   ItemsCount;
	TReqFunReadItem Items[MaxVars];
}TReqFunReadParams;

typedef TReqFunReadParams* PReqFunReadParams;

typedef struct {
	byte   FunRead;
	byte   ItemCount;
}TResFunReadParams;

typedef TResFunReadParams* PResFunReadParams;

typedef struct {
	byte    ReturnCode;
	byte    TransportSize;
	word    DataLength;
	byte    Data[IsoPayload_Size - 17]; // 17 = header + params + data header - 1
}TResFunReadItem, *PResFunReadItem;

typedef PResFunReadItem TResFunReadData[MaxVars];

//==============================================================================
//                               FUNCTION WRITE
//==============================================================================
typedef struct {
	byte    ItemHead[3];
	byte    TransportSize;
	word    Length;
	word    DBNumber;
	byte    Area;
	byte    Address[3];
}TReqFunWriteItem, * PReqFunWriteItem;

typedef struct {
	byte   FunWrite;
	byte   ItemsCount;
	TReqFunWriteItem Items[MaxVars];
}TReqFunWriteParams;

typedef TReqFunWriteParams* PReqFunWriteParams;

typedef struct {
	byte    ReturnCode;
	byte    TransportSize;
	word    DataLength;
	byte    Data [IsoPayload_Size - 17]; // 17 = header + params + data header -1
}TReqFunWriteDataItem, *PReqFunWriteDataItem;

typedef PReqFunWriteDataItem TReqFunWriteData[MaxVars];

typedef struct {
	byte   FunWrite;
	byte   ItemCount;
	byte   Data[MaxVars];
}TResFunWrite;

typedef TResFunWrite* PResFunWrite;

//==============================================================================
//                                 GROUP UPLOAD
//==============================================================================
typedef struct {
	byte   FunSUpld;    // function start upload 0x1D
	byte   Uk6 [6];     // Unknown 6 bytes
	byte   Upload_ID;
	byte   Len_1;
	byte   Prefix;
	byte   BlkPrfx;     // always 0x30
	byte   BlkType;
	byte   AsciiBlk[5]; // BlockNum in ascii
	byte   A;           // always 0x41 ('A')
}TReqFunStartUploadParams;

typedef TReqFunStartUploadParams* PReqFunStartUploadParams;

typedef struct {
	byte   FunSUpld;  // function start upload 0x1D
	byte   Data_1[6];
	byte   Upload_ID;
	byte   Uk[3];
	byte   LenLoad[5];
}TResFunStartUploadParams;

typedef TResFunStartUploadParams* PResFunStartUploadParams;

typedef struct {
	byte   FunUpld;  // function upload 0x1E
	byte   Uk6[6];   // Unknown 6 bytes
	byte   Upload_ID;
}TReqFunUploadParams;

typedef TReqFunUploadParams* PReqFunUploadParams;

typedef struct {
	byte   FunUpld; // function upload 0x1E
	byte   EoU;     // 0 = End Of Upload, 1 = Upload in progress
}TResFunUploadParams;

typedef TResFunUploadParams* PResFunUploadParams;

typedef struct {
	word    Length;   // Payload length - 4
	byte    Uk_00;    // Unknown 0x00
	byte    Uk_FB;    // Unknown 0xFB
	// from here is the same of TS7CompactBlockInfo
	word    Cst_pp;
	byte    Uk_01;    // Unknown 0x01
	byte    BlkFlags;
	byte    BlkLang;
	byte    SubBlkType;
	word    BlkNum;
	u_int   LenLoadMem;
	u_int   BlkSec;
	u_int   CodeTime_ms;
	word    CodeTime_dy;
	u_int   IntfTime_ms;
	word    IntfTime_dy;
	word    SbbLen;
	word    AddLen;
	word    LocDataLen;
	word    MC7Len;
}TResFunUploadDataHeaderFirst;

typedef TResFunUploadDataHeaderFirst* PResFunUploadDataHeaderFirst;

typedef struct {
	word    Length;// Payload length - 4
	byte    Uk_00; // Unknown 0x00
	byte    Uk_FB; // Unknown 0xFB
}TResFunUploadDataHeaderNext;

typedef TResFunUploadDataHeaderNext* PResFunUploadDataHeaderNext;

typedef struct {
	word    Length;// Payload length - 4
	byte    Uk_00; // Unknown 0x00
	byte    Uk_FB; // Unknown 0xFB
}TResFunUploadDataHeader;

typedef TResFunUploadDataHeader* PResFunUploadDataHeader;

typedef struct {
	byte    ID;  // 0x65
	word    Seq; // Sequence
	byte    Const_1[10];
	word    Lo_bound;
	word    Hi_Bound;
	byte    u_shortLen;// 0x02 byte
			   // 0x04 word
			   // 0x05 int
			   // 0x06 dword
			   // 0x07 dint
			   // 0x08 real
	byte    c1, c2;
	char    Author[8];
	char    Family[8];
	char    Header[8];
	byte    B1; // 0x11
	byte    B2; // 0x00
	word    Chksum;
	byte    Uk_8[8];
}TArrayUpldFooter;

typedef TArrayUpldFooter* PArrayUpldFooter;

typedef struct {
	byte   FunEUpld; // function end upload 0x1F
	byte   Uk6[6];   // Unknown 6 bytes
	byte   Upload_ID;
}TReqFunEndUploadParams;

typedef TReqFunEndUploadParams* PReqFunEndUploadParams;

typedef struct {
	byte   FunEUpld;  // function end upload 0x1F
}TResFunEndUploadParams;

typedef TResFunEndUploadParams* PResFunEndUploadParams;

//==============================================================================
//                               GROUP DOWNLOAD
//==============================================================================
typedef struct {
	byte   FunSDwnld;   // function start Download 0x1A
	byte   Uk6[6];      // Unknown 6 bytes
	byte   Dwnld_ID;
	byte   Len_1;       // 0x09
	byte   Prefix;      // 0x5F
	byte   BlkPrfx;     // always 0x30
	byte   BlkType;
	byte   AsciiBlk[5]; // BlockNum in ascii
	byte   P;           // 0x50 ('P')
	byte   Len_2;       // 0x0D
	byte   Uk1;         // 0x01
	byte   AsciiLoad[6];// load memory size (MC7 size + 92)
	byte   AsciiMC7[6]; // Block size in bytes
}TReqStartDownloadParams;

typedef TReqStartDownloadParams* PReqStartDownloadParams;
typedef byte  TResStartDownloadParams;
typedef TResStartDownloadParams* PResStartDownloadParams;

typedef struct {
	byte   Fun;         // pduDownload or pduDownloadEnded
	byte   Uk7[7];
	byte   Len_1;       // 0x09
	byte   Prefix;      // 0x5F
	byte   BlkPrfx;     // always 0x30
	byte   BlkType;
	byte   AsciiBlk[5]; // BlockNum in ascii
	byte   P;           // 0x50 ('P')
}TReqDownloadParams;

typedef TReqDownloadParams* PReqDownloadParams;

typedef struct {
	byte   FunDwnld; // 0x1B
	byte   EoS;      // End of sequence : 0x00 - Sequence in progress : 0x01
}TResDownloadParams;

typedef TResDownloadParams* PResDownloadParams;

typedef struct {
	word    DataLen;
	word    FB_00;   // 0x00 0xFB
}TResDownloadDataHeader;

typedef TResDownloadDataHeader* PResDownloadDataHeader;
typedef byte   TResEndDownloadParams;
typedef TResEndDownloadParams* PResEndDownloadParams;

typedef struct {
	word    Cst_pp;
	byte    Uk_01; // Unknown 0x01
	byte    BlkFlags;
	byte    BlkLang;
	byte    SubBlkType;
	word    BlkNum;
	u_int   LenLoadMem;
	u_int   BlkSec;
	u_int   CodeTime_ms;
	word    CodeTime_dy;
	u_int   IntfTime_ms;
	word    IntfTime_dy;
	word    SbbLen;
	word    AddLen;
	word    LocDataLen;
	word    MC7Len;
}TS7CompactBlockInfo;

typedef TS7CompactBlockInfo* PS7CompactBlockInfo;

typedef struct {
	byte    Uk_20[20];
	byte    Author[8];
	byte    Family[8];
	byte    Header[8];
	byte    B1; // 0x11
	byte    B2; // 0x00
	word    Chksum;
	byte    Uk_12[8];
}TS7BlockFooter;

typedef TS7BlockFooter* PS7BlockFooter;

//==============================================================================
//                          FUNCTION INSERT/DELETE
//==============================================================================
typedef struct {
	byte    Fun;         // plc control 0x28
	byte    Uk7[7];      // unknown 7
	word    Len_1;       // Length part 1 : 10
	byte    NumOfBlocks; // number of blocks to insert
	byte    ByteZero;    // 0x00
	byte    AsciiZero;   // 0x30 '0'
	byte    BlkType;
	byte    AsciiBlk[5]; // BlockNum in ascii
	byte    SFun;        // 0x50 or 0x42
	byte    Len_2;       // Length part 2 : 0x05 bytes
	char    Cmd[5];      // ascii '_INSE' or '_DELE'
}TReqControlBlockParams;

typedef TReqControlBlockParams* PReqControlBlockParams;

//==============================================================================
//                FUNCTIONS START/STOP/COPY RAM TO ROM/COMPRESS
//==============================================================================
typedef struct {
	byte   Fun;     // stop 0x29
	byte   Uk_5[5]; // unknown 5 bytes 0x00
	byte   Len_2;   // Length part 2 : 0x09
	char   Cmd[9];  // ascii 'P_PROGRAM'
}TReqFunPlcStop;

typedef TReqFunPlcStop* PReqFunPlcStop;

typedef struct {
	byte    Fun;     // start 0x28
	byte    Uk_7[7]; // unknown 7
	word    Len_1;   // Length part 1 : 0x0000
	byte    Len_2;   // Length part 2 : 0x09
	char    Cmd [9]; // ascii 'P_PROGRAM'
}TReqFunPlcHotStart;

typedef TReqFunPlcHotStart* PReqFunPlcHotStart;

typedef struct {
	byte    Fun;     // start 0x28
	byte    Uk_7[7]; // unknown 7
	word    Len_1;   // Length part 1 : 0x0002
	word    SFun;    // 'C ' 0x4320
	byte    Len_2;   // Length part 2 : 0x09
	char    Cmd[9];  // ascii 'P_PROGRAM'
}TReqFunPlcColdStart;

typedef TReqFunPlcColdStart* PReqFunPlcColdStart;

typedef struct {
	byte    Fun;     // pduControl 0x28
	byte    Uk_7[7]; // unknown 7
	word    Len_1;   // Length part 1 : 0x0002
	word    SFun;    // 'EP' 0x4550
	byte    Len_2;   // Length part 2 : 0x05
	char    Cmd[5];  // ascii '_MODU'
}TReqFunCopyRamToRom;

typedef TReqFunCopyRamToRom* PReqFunCopyRamToRom;

typedef struct {
	byte    Fun;     // pduControl 0x28
	byte    Uk_7[7]; // unknown 7
	word    Len_1;   // Length part 1 : 0x00
	byte    Len_2;   // Length part 2 : 0x05
	char    Cmd[5];  // ascii '_GARB'
}TReqFunCompress;

typedef TReqFunCompress* PReqFunCompress;

typedef struct {
	byte   ResFun;
	byte   para;
}TResFunCtrl;

typedef TResFunCtrl* PResFunCtrl;

//==============================================================================
//                            FUNCTIONS USERDATA
//==============================================================================
typedef struct {
	byte    Head[3]; // Always 0x00 0x01 0x12
	byte    Plen;    // par len 0x04 or 0x08
	byte    Uk;      // unknown
	byte    Tg;      // type and group  (4 bits type and 4 bits group)
	byte    SubFun;  // subfunction
	byte    Seq;     // sequence
	word    resvd;   // present if plen=0x08 (S7 manager online functions)
	word    Err;     // present if plen=0x08 (S7 manager online functions)
}TS7Params7;

typedef TS7Params7* PS7ReqParams7;
typedef TS7Params7* PS7ResParams7;

// for convenience Hi order bit of type are included (0x4X)
const byte   grProgrammer  = 0x41;
const byte   grCyclicData  = 0x42;
const byte   grBlocksInfo  = 0x43;
const byte   grSZL         = 0x44;
const byte   grPassword    = 0x45;
const byte   grBSend       = 0x46;
const byte   grClock       = 0x47;
const byte   grSecurity    = 0x45;

//==============================================================================
//                             GROUP SECURITY
//==============================================================================
typedef TReqFunTypedParams TReqFunSecurity;
typedef TReqFunSecurity* PReqFunSecurity;

typedef char TS7Password[8];

typedef struct {
	byte    Ret;    // 0xFF for request
	byte    TS;     // 0x09 Transport size
	word    DLen;   // Data len  : 8 bytes
	byte    Pwd[8]; // Password encoded into "AG" format
}TReqDataSecurity;

typedef TReqDataSecurity* PReqDataSecurity;
typedef TS7Params7 TResParamsSecurity;
typedef TResParamsSecurity* PResParamsSecurity;

typedef struct {
	byte    Ret;
	byte    TS;
	word    DLen;
}TResDataSecurity;

typedef TResDataSecurity* PResDataSecurity;

//==============================================================================
//                             GROUP BLOCKS SZL
//==============================================================================
typedef TReqFunTypedParams TReqFunReadSZLFirst;
typedef TReqFunReadSZLFirst* PReqFunReadSZLFirst;

typedef struct {
	byte    Head[3]; // 0x00 0x01 0x12
	byte    Plen;    // par len 0x04
	byte    Uk;      // unknown
	byte    Tg;      // type and group (4 bits type and 4 bits group)
	byte    SubFun;  // subfunction
	byte    Seq;     // sequence
	word    Rsvd;    // Reserved 0x0000
	word    ErrNo;   // Error Code
}TReqFunReadSZLNext;

typedef TReqFunReadSZLNext* PReqFunReadSZLNext;

typedef struct {
	byte    Ret;  // 0xFF for request
	byte    TS;   // 0x09 Transport size
	word    DLen; // Data len
	word    ID;   // SZL-ID
	word    Index;// SZL-Index
}TS7ReqSZLData;

typedef TS7ReqSZLData* PS7ReqSZLData;

typedef struct {
	byte    Ret;
	byte    TS;
	word    DLen;
	word    ID;
	word    Index;
	word    ListLen;
	word    ListCount;
	word    Data[32747];
}TS7ResSZLDataFirst;

typedef TS7ResSZLDataFirst* PS7ResSZLDataFirst;

typedef struct {
	byte    Ret;
	byte    TS;
	word    DLen;
	word    Data[32751];
}TS7ResSZLDataNext;

typedef TS7ResSZLDataNext* PS7ResSZLDataNext;

typedef struct {
	byte    Ret;
	byte    OtherInfo[9];
	word    Count;
	word    Items[32747];
}TS7ResSZLData_0;

typedef TS7ResSZLData_0* PS7ResSZLData_0;

//==============================================================================
//                               GROUP CLOCK
//==============================================================================
typedef TReqFunTypedParams TReqFunDateTime;
typedef TReqFunDateTime* PReqFunDateTime;

typedef byte   TReqDataGetDateTime[4];

typedef longword *PReqDataGetDateTime;

typedef struct {
	byte    RetVal;
	byte    TSize;
	word    Length;
	byte    Rsvd;
	byte    HiYear;
	TTimeBuffer Time;
}TResDataGetTime;

typedef TResDataGetTime* PResDataGetTime;
typedef TResDataGetTime TReqDataSetTime;
typedef TReqDataSetTime* PReqDataSetTime;

typedef struct {
	byte    RetVal;
	byte    TSize;
	word    Length;
}TResDataSetTime;

typedef TResDataSetTime* PResDataSetTime;

//==============================================================================
//                            GROUP BLOCKS INFO
//==============================================================================
typedef TReqFunTypedParams TReqFunGetBlockInfo;
typedef TReqFunGetBlockInfo* PReqFunGetBlockInfo;

typedef byte   TReqDataFunBlocks[4];
typedef u_char* PReqDataFunBlocks;

typedef struct {
	byte    Head[3]; // 0x00 0x01 0x12
	byte    Plen;    // par len 0x04
	byte    Uk;      // unknown
	byte    Tg;      // type and group  (4 bits type and 4 bits group)
	byte    SubFun;  // subfunction
	byte    Seq;     // sequence
	word    Rsvd;    // Reserved 0x0000
	word    ErrNo;   // Error Code
}TResFunGetBlockInfo;

typedef TResFunGetBlockInfo* PResFunGetBlockInfo;

typedef struct {
	byte    Zero;   // always 0x30 -> Ascii 0
	byte    BType;  // Block Type
	word    BCount; // Block count
}TResFunGetBlockItem;

typedef struct {
	byte    RetVal;
	byte    TRSize;
	word    Length;
	TResFunGetBlockItem Blocks[7];
}TDataFunListAll;

typedef TDataFunListAll* PDataFunListAll;

typedef struct {
	word    BlockNum;
	byte    Unknown;
	byte    BlockLang;
}TDataFunGetBotItem;

typedef struct {
	byte    RetVal;
	byte    TSize;
	word    DataLen;
	TDataFunGetBotItem Items[(IsoPayload_Size - 29 ) / 4];
}TDataFunGetBot;
// Note : 29 is the size of headers iso, COPT, S7 header, params, data

typedef TDataFunGetBot* PDataFunGetBot;

typedef struct {
	byte   RetVal;  // 0xFF
	byte   TSize;   // Octet (0x09)
	word   Length;  // 0x0002
	byte   Zero;    // Ascii '0' (0x30)
	byte   BlkType;
}TReqDataBlockOfType;

typedef TReqDataBlockOfType* PReqDataBlockOfType;

typedef struct {
	byte    RetVal;
	byte    TSize;
	word    DataLen;
	byte    BlkPrfx;     // always 0x30
	byte    BlkType;
	byte    AsciiBlk[5]; // BlockNum in ascii
	byte    A;           // always 0x41 ('A')
}TReqDataBlockInfo;

typedef TReqDataBlockInfo* PReqDataBlockInfo;

typedef struct {
	byte    RetVal;
	byte    TSize;
	word    Length;
	byte    Cst_b;
	byte    BlkType;
	word    Cst_w1;
	word    Cst_w2;
	word    Cst_pp;
	byte    Unknown_1;
	byte    BlkFlags;
	byte    BlkLang;
	byte    SubBlkType;
	word    BlkNumber;
	u_int   LenLoadMem;
	byte    BlkSec[4];
	u_int   CodeTime_ms;
	word    CodeTime_dy;
	u_int   IntfTime_ms;
	word    IntfTime_dy;
	word    SbbLen;
	word    AddLen;
	word    LocDataLen;
	word    MC7Len;
	byte    Author[8];
	byte    Family[8];
	byte    Header[8];
	byte    Version;
	byte    Unknown_2;
	word    BlkChksum;
	byte    Resvd1[4];
	byte    Resvd2[4];
}TResDataBlockInfo;

typedef TResDataBlockInfo* PResDataBlockInfo;

//==============================================================================
//                                 BSEND / BRECV
//==============================================================================
typedef struct {
	int       Size;
	longword  R_ID;
	byte      Data[65536];
}TPendingBuffer;

typedef struct {
	TTPKT    TPKT;
	TCOTP_DT COTP;
	byte     P;
	byte     PDUType;
}TPacketInfo;

typedef struct {
	byte    Head[3];// Always 0x00 0x01 0x12
	byte    Plen;   // par len 0x04 or 0x08
	byte    Uk;     // unknown  (0x12)
	byte    Tg;     // type and group, 4 bits type and 4 bits group  (0x46)
	byte    SubFun; // subfunction (0x01)
	byte    Seq;    // sequence
	byte    IDSeq;  // ID Sequence (come from partner)
	byte    EoS;    // End of Sequence = 0x00 Sequence in progress = 0x01;
	word    Err;    //
}TBSendParams;

typedef TBSendParams* PBSendReqParams;
typedef TBSendParams* PBSendResParams;

// Data frame

typedef struct {
	byte    FF;      // 0xFF
	byte    TRSize;  // Transport Size 0x09 (octet)
	word    Len;     // This Telegram Length
	byte    DHead[4];// sequence 0x12 0x06 0x13 0x00
	u_int   R_ID;    // R_ID
}TBsendRequestData;

typedef TBsendRequestData* PBsendRequestData;

typedef struct {
	byte   DHead[4]; // sequence 0x0A 0x00 0x00 0x00
}TBSendResData;

typedef TBSendResData* PBSendResData;

#pragma pack()
#endif // s7_types_h
