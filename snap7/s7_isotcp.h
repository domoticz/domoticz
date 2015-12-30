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
#ifndef s7_isotcp_h
#define s7_isotcp_h
//---------------------------------------------------------------------------
#include "snap_msgsock.h"
//---------------------------------------------------------------------------
#pragma pack(1)

#define isoTcpVersion    	3      // RFC 1006
#define isoTcpPort    		102    // RFC 1006
#define isoInvalidHandle        0
#define MaxTSAPLength    	16     // Max Lenght for Src and Dst TSAP
#define MaxIsoFragments         64     // Max fragments
#define IsoPayload_Size    	4096   // Iso telegram Buffer size

#define noError    			0

const longword errIsoMask    	        = 0x000F0000;
const longword errIsoBase               = 0x0000FFFF;

const longword errIsoConnect            = 0x00010000; // Connection error
const longword errIsoDisconnect         = 0x00020000; // Disconnect error
const longword errIsoInvalidPDU         = 0x00030000; // Bad format
const longword errIsoInvalidDataSize    = 0x00040000; // Bad Datasize passed to send/recv : buffer is invalid
const longword errIsoNullPointer    	= 0x00050000; // Null passed as pointer
const longword errIsoShortPacket    	= 0x00060000; // A short packet received
const longword errIsoTooManyFragments   = 0x00070000; // Too many packets without EoT flag
const longword errIsoPduOverflow    	= 0x00080000; // The sum of fragments data exceded maximum packet size
const longword errIsoSendPacket         = 0x00090000; // An error occurred during send
const longword errIsoRecvPacket         = 0x000A0000; // An error occurred during recv
const longword errIsoInvalidParams    	= 0x000B0000; // Invalid TSAP params
const longword errIsoResvd_1    	    = 0x000C0000; // Unassigned
const longword errIsoResvd_2    	    = 0x000D0000; // Unassigned
const longword errIsoResvd_3    	    = 0x000E0000; // Unassigned
const longword errIsoResvd_4    	    = 0x000F0000; // Unassigned

const longword ISO_OPT_TCP_NODELAY   	= 0x00000001; // Disable Nagle algorithm
const longword ISO_OPT_INSIDE_MTU    	= 0x00000002; // Max packet size < MTU ethernet card

// TPKT Header - ISO on TCP - RFC 1006 (4 bytes)
typedef struct{
	u_char Version;    // Always 3 for RFC 1006
	u_char Reserved;   // 0
	u_char HI_Lenght;  // High part of packet lenght (entire frame, payload and TPDU included)
	u_char LO_Lenght;  // Low part of packet lenght (entire frame, payload and TPDU included)
} TTPKT;               // Packet length : min 7 max 65535

typedef struct {
	u_char PduSizeCode;
	u_char PduSizeLen;
	u_char PduSizeVal;
	u_char TSAP[245]; // We don't know in advance these fields....
} TCOPT_Params ;

// PDU Type constants - ISO 8073, not all are mentioned in RFC 1006
// For our purposes we use only those labeled with **
// These constants contains 4 low bit order 0 (credit nibble)
//
//     $10 ED : Expedited Data
//     $20 EA : Expedited Data Ack
//     $40 UD : CLTP UD
//     $50 RJ : Reject
//     $70 AK : Ack data
// **  $80 DR : Disconnect request (note : S7 doesn't use it)
// **  $C0 DC : Disconnect confirm (note : S7 doesn't use it)
// **  $D0 CC : Connection confirm
// **  $E0 CR : Connection request
// **  $F0 DT : Data
//

// COTP Header for CONNECTION REQUEST/CONFIRM - DISCONNECT REQUEST/CONFIRM
typedef struct {
	u_char  HLength;     // Header length : initialized to 6 (length without params - 1)
						 // descending classes that add values in params field must update it.
	u_char  PDUType;     // 0xE0 Connection request
						 // 0xD0 Connection confirm
						 // 0x80 Disconnect request
						 // 0xDC Disconnect confirm
	u_short DstRef;      // Destination reference : Always 0x0000
	u_short SrcRef;      // Source reference : Always 0x0000
	u_char  CO_R;        // If the telegram is used for Connection request/Confirm,
						 // the meaning of this field is CLASS+OPTION :
						 //   Class (High 4 bits) + Option (Low 4 bits)
						 //   Class : Always 4 (0100) but is ignored in input (RFC States this)
						 //   Option : Always 0, also this in ignored.
						 // If the telegram is used for Disconnect request,
						 // the meaning of this field is REASON :
						 //    1     Congestion at TSAP
						 //    2     Session entity not attached to TSAP
						 //    3     Address unknown (at TCP connect time)
						 //  128+0   Normal disconnect initiated by the session
						 //          entity.
						 //  128+1   Remote transport entity congestion at connect
						 //          request time
						 //  128+3   Connection negotiation failed
						 //  128+5   Protocol Error
						 //  128+8   Connection request refused on this network
						 //          connection
	// Parameter data : depending on the protocol implementation.
	// ISO 8073 define several type of parameters, but RFC 1006 recognizes only
	// TSAP related parameters and PDU size.  See RFC 0983 for more details.
	TCOPT_Params Params;
	/* Other params not used here, list only for completeness
		ACK_TIME     	   = 0x85,  1000 0101 Acknowledge Time
		RES_ERROR    	   = 0x86,  1000 0110 Residual Error Rate
		PRIORITY           = 0x87,  1000 0111 Priority
		TRANSIT_DEL  	   = 0x88,  1000 1000 Transit Delay
		THROUGHPUT   	   = 0x89,  1000 1001 Throughput
		SEQ_NR       	   = 0x8A,  1000 1010 Subsequence Number (in AK)
		REASSIGNMENT 	   = 0x8B,  1000 1011 Reassignment Time
		FLOW_CNTL    	   = 0x8C,  1000 1100 Flow Control Confirmation (in AK)
		TPDU_SIZE    	   = 0xC0,  1100 0000 TPDU Size
		SRC_TSAP     	   = 0xC1,  1100 0001 TSAP-ID / calling TSAP ( in CR/CC )
		DST_TSAP     	   = 0xC2,  1100 0010 TSAP-ID / called TSAP
		CHECKSUM     	   = 0xC3,  1100 0011 Checksum
		VERSION_NR   	   = 0xC4,  1100 0100 Version Number
		PROTECTION   	   = 0xC5,  1100 0101 Protection Parameters (user defined)
		OPT_SEL            = 0xC6,  1100 0110 Additional Option Selection
		PROTO_CLASS  	   = 0xC7,  1100 0111 Alternative Protocol Classes
		PREF_MAX_TPDU_SIZE = 0xF0,  1111 0000
		INACTIVITY_TIMER   = 0xF2,  1111 0010
		ADDICC             = 0xe0   1110 0000 Additional Information on Connection Clearing
	*/
} TCOTP_CO ;
typedef TCOTP_CO *PCOTP_CO;

// COTP Header for DATA EXCHANGE
typedef struct {
	u_char HLength;   // Header length : 3 for this header
	u_char PDUType;   // 0xF0 for this header
	u_char EoT_Num;   // EOT (bit 7) + PDU Number (bits 0..6)
         		  // EOT = 1 -> End of Trasmission Packet (This packet is complete)
			  // PDU Number : Always 0
} TCOTP_DT;
typedef TCOTP_DT *PCOTP_DT;

// Info part of a PDU, only common parts. We use it to check the consistence
// of a telegram regardless of it's nature (control or data).
typedef struct {
	TTPKT TPKT; 	// TPKT Header
			// Common part of any COTP
	u_char HLength; // Header length : 3 for this header
	u_char PDUType; // Pdu type
} TIsoHeaderInfo ;
typedef TIsoHeaderInfo *PIsoHeaderInfo;

// PDU Type consts (Code + Credit)
const byte pdu_type_CR    	= 0xE0;  // Connection request
const byte pdu_type_CC    	= 0xD0;  // Connection confirm
const byte pdu_type_DR    	= 0x80;  // Disconnect request
const byte pdu_type_DC    	= 0xC0;  // Disconnect confirm
const byte pdu_type_DT    	= 0xF0;  // Data transfer

const byte pdu_EoT    		= 0x80;  // End of Trasmission Packet (This packet is complete)

const longword DataHeaderSize  = sizeof(TTPKT)+sizeof(TCOTP_DT);
const longword IsoFrameSize    = IsoPayload_Size+DataHeaderSize;

typedef struct {
	TTPKT 	 TPKT; // TPKT Header
	TCOTP_CO COTP; // COPT Header for CONNECTION stuffs
} TIsoControlPDU;
typedef TIsoControlPDU *PIsoControlPDU;

typedef u_char TIsoPayload[IsoPayload_Size];

typedef struct {
	TTPKT 	    TPKT; // TPKT Header
	TCOTP_DT    COTP; // COPT Header for DATA EXCHANGE
	TIsoPayload Payload; // Payload
} TIsoDataPDU ;

typedef TIsoDataPDU *PIsoDataPDU;
typedef TIsoPayload *PIsoPayload;

typedef enum {
	pkConnectionRequest,
	pkDisconnectRequest,
	pkEmptyFragment,
	pkInvalidPDU,
	pkUnrecognizedType,
	pkValidData
} TPDUKind ;

#pragma pack()

void ErrIsoText(int Error, char *Msg, int len);

class TIsoTcpSocket : public TMsgSocket
{
private:

	TIsoControlPDU FControlPDU;
        int IsoMaxFragments; // max fragments allowed for an ISO telegram
	// Checks the PDU format
	int CheckPDU(void *pPDU, u_char PduTypeExpected);
	// Receives the next fragment
	int isoRecvFragment(void *From, int Max, int &Size, bool &EoT);
protected:
	TIsoDataPDU PDU;
	int SetIsoError(int Error);
	// Builds the control PDU starting from address properties
	virtual int BuildControlPDU();
	// Calcs the PDU size
	int PDUSize(void *pPDU);
	// Parses the connection request PDU to extract TSAP and PDU size info
	virtual void IsoParsePDU(TIsoControlPDU PDU);
	// Confirms the connection, override this method for special pourpose
	// By default it checks the PDU format and resend it changing the pdu type
	int IsoConfirmConnection(u_char PDUType);
    void ClrIsoError();
	virtual void FragmentSkipped(int Size);
public:
	word SrcTSap;  // Source TSAP
	word DstTSap;  // Destination TSAP
	word SrcRef;   // Source Reference
	word DstRef;   // Destination Reference
	int IsoPDUSize;
	int LastIsoError;
	//--------------------------------------------------------------------------
	TIsoTcpSocket();
	~TIsoTcpSocket();
	// HIGH Level functions (work on payload hiding the underlying protocol)
	// Connects with a peer, the connection PDU is automatically built starting from address scheme (see below)
	int isoConnect();
	// Disconnects from a peer, if OnlyTCP = true, only a TCP disconnect is performed,
	// otherwise a disconnect PDU is built and send.
	int isoDisconnect(bool OnlyTCP);
	// Sends a buffer, a valid header is created
	int isoSendBuffer(void *Data, int Size);
	// Receives a buffer
	int isoRecvBuffer(void *Data, int & Size);
	// Exchange cycle send->receive
	int isoExchangeBuffer(void *Data, int & Size);
	// A PDU is ready (at least its header) to be read
	bool IsoPDUReady();
	// Same as isoSendBuffer, but the entire PDU has to be provided (in any case a check is performed)
	int isoSendPDU(PIsoDataPDU Data);
	// Same as isoRecvBuffer, but it returns the entire PDU, automatically enques the fragments
	int isoRecvPDU(PIsoDataPDU Data);
	// Same as isoExchangeBuffer, but the entire PDU has to be provided (in any case a check is performed)
	int isoExchangePDU(PIsoDataPDU Data);
	// Peeks an header info to know which kind of telegram is incoming
	void IsoPeek(void *pPDU, TPDUKind &PduKind);
};

#endif // s7_isotcp_h
