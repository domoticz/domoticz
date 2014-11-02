/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

        wusb.h

Abstract:

        Public interface to winusb.dll

Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        11/19/2002 : created

Authors:

--*/

#ifndef __WUSB_H__
#define __WUSB_H__

#ifdef __cplusplus
extern "C" {
#endif

#if(NTDDI_VERSION >= NTDDI_WINXP)

#include <winusbio.h>


typedef PVOID WINUSB_INTERFACE_HANDLE, *PWINUSB_INTERFACE_HANDLE;

   
#pragma pack(1)

typedef struct _WINUSB_SETUP_PACKET {
    UCHAR   RequestType;
    UCHAR   Request;
    USHORT  Value;
    USHORT  Index;
    USHORT  Length;
} WINUSB_SETUP_PACKET, *PWINUSB_SETUP_PACKET;

#pragma pack()



BOOL __stdcall
WinUsb_Initialize(
    __in  HANDLE DeviceHandle,
    __out PWINUSB_INTERFACE_HANDLE InterfaceHandle
    );


BOOL __stdcall
WinUsb_Free(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle
    );


BOOL __stdcall
WinUsb_GetAssociatedInterface(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AssociatedInterfaceIndex,
    __out PWINUSB_INTERFACE_HANDLE AssociatedInterfaceHandle
    );



BOOL __stdcall
WinUsb_GetDescriptor(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR DescriptorType,
    __in  UCHAR Index,
    __in  USHORT LanguageID,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out PULONG LengthTransferred
    );

BOOL __stdcall
WinUsb_QueryInterfaceSettings(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __out PUSB_INTERFACE_DESCRIPTOR UsbAltInterfaceDescriptor
    );

BOOL __stdcall
WinUsb_QueryDeviceInformation(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG InformationType,
    __inout PULONG BufferLength,
    __out_bcount(*BufferLength) PVOID Buffer
    );

BOOL __stdcall
WinUsb_SetCurrentAlternateSetting(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR SettingNumber
    );

BOOL __stdcall
WinUsb_GetCurrentAlternateSetting(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __out PUCHAR SettingNumber
    );


BOOL __stdcall
WinUsb_QueryPipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR AlternateInterfaceNumber,
    __in  UCHAR PipeIndex,
    __out PWINUSB_PIPE_INFORMATION PipeInformation
    );


BOOL __stdcall
WinUsb_SetPipePolicy(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value
    );

BOOL __stdcall
WinUsb_GetPipePolicy(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value
    );

BOOL __stdcall
WinUsb_ReadPipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __out_bcount_part_opt(BufferLength,*LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped
    );

BOOL __stdcall
WinUsb_WritePipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID,
    __in_bcount(BufferLength) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt LPOVERLAPPED Overlapped    
    );

BOOL __stdcall
WinUsb_ControlTransfer(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  WINUSB_SETUP_PACKET SetupPacket,
    __out_bcount_part_opt(BufferLength, *LengthTransferred) PUCHAR Buffer,
    __in  ULONG BufferLength,
    __out_opt PULONG LengthTransferred,
    __in_opt  LPOVERLAPPED Overlapped    
    );

BOOL __stdcall
WinUsb_ResetPipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_AbortPipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_FlushPipe(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  UCHAR PipeID
    );

BOOL __stdcall
WinUsb_SetPowerPolicy(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __in  ULONG ValueLength,
    __in_bcount(ValueLength) PVOID Value
    );

BOOL __stdcall
WinUsb_GetPowerPolicy(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  ULONG PolicyType,
    __inout PULONG ValueLength,
    __out_bcount(*ValueLength) PVOID Value
    );

BOOL __stdcall
WinUsb_GetOverlappedResult(
    __in  WINUSB_INTERFACE_HANDLE InterfaceHandle,
    __in  LPOVERLAPPED lpOverlapped,
    __out LPDWORD lpNumberOfBytesTransferred,
    __in  BOOL bWait
    );
    

PUSB_INTERFACE_DESCRIPTOR __stdcall
WinUsb_ParseConfigurationDescriptor(
    __in  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    __in  PVOID StartPosition,
    __in  LONG InterfaceNumber,
    __in  LONG AlternateSetting,
    __in  LONG InterfaceClass,
    __in  LONG InterfaceSubClass,
    __in  LONG InterfaceProtocol
    );

PUSB_COMMON_DESCRIPTOR __stdcall
WinUsb_ParseDescriptors(
    __in_bcount(TotalLength) PVOID    DescriptorBuffer,
    __in  ULONG    TotalLength,
    __in  PVOID    StartPosition,
    __in  LONG     DescriptorType
    );


#endif // (NTDDI_VERSION >= NTDDI_WINXP)

#ifdef __cplusplus
}
#endif

                   
#endif //__WUSB_H__

