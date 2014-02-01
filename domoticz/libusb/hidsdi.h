/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    HIDSDI.H

Abstract:

    This module contains the PUBLIC definitions for the
    code that implements the HID dll.

Environment:

    Kernel & user mode

--*/


#ifndef _HIDSDI_H
#define _HIDSDI_H

#include <pshpack4.h>

//#include "wtypes.h"

//#include <windef.h>
//#include <win32.h>
//#include <basetyps.h>

#ifndef NTSTATUS
	typedef LONG NTSTATUS;
#endif
#include "hidusage.h"
#include "hidpi.h"

typedef struct _HIDD_CONFIGURATION {
    PVOID    cookie;
    ULONG    size;
    ULONG    RingBufferSize;
} HIDD_CONFIGURATION, *PHIDD_CONFIGURATION;

typedef struct _HIDD_ATTRIBUTES {
    ULONG   Size; // = sizeof (struct _HIDD_ATTRIBUTES)

    //
    // Vendor ids of this hid device
    //
    USHORT  VendorID;
    USHORT  ProductID;
    USHORT  VersionNumber;

    //
    // Additional fields will be added to the end of this structure.
    //
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

__checkReturn
BOOLEAN __stdcall
HidD_GetAttributes (
    __in  HANDLE              HidDeviceObject,
    __out PHIDD_ATTRIBUTES    Attributes
    );
/*++
Routine Description:
    Fill in the given HIDD_ATTRIBUTES structure with the attributes of the
    given hid device.

--*/


void __stdcall
HidD_GetHidGuid (
   __out  LPGUID   HidGuid
   );

__checkReturn
BOOLEAN __stdcall
HidD_GetPreparsedData (
   __in   HANDLE                  HidDeviceObject,
   __out __drv_when(return!=0, __drv_allocatesMem(Mem))  PHIDP_PREPARSED_DATA  * PreparsedData
   );
/*++
Routine Description:
    Given a handle to a valid Hid Class Device Object, retrieve the preparsed
    data for the device.  This routine will allocate the appropriately 
    sized buffer to hold this preparsed data.  It is up to client to call
    HidP_FreePreparsedData to free the memory allocated to this structure when
    it is no longer needed.

Arguments:
   HidDeviceObject A handle to a Hid Device that the client obtains using 
                   a call to CreateFile on a valid Hid device string name.
                   The string name can be obtained using standard PnP calls.

   PreparsedData   An opaque data structure used by other functions in this 
                   library to retrieve information about a given device.

Return Value:
   TRUE if successful.
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
BOOLEAN __stdcall
HidD_FreePreparsedData (
   __in __drv_freesMem(Mem) PHIDP_PREPARSED_DATA PreparsedData
   );

BOOLEAN __stdcall
HidD_FlushQueue (
   __in    HANDLE                HidDeviceObject
   );
/*++
Routine Description:
    Flush the input queue for the given HID device.

Arguments:
   HidDeviceObject A handle to a Hid Device that the client obtains using 
                   a call to CreateFile on a valid Hid device string name.
                   The string name can be obtained using standard PnP calls.

Return Value:
   TRUE if successful
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetConfiguration (
   __in   HANDLE               HidDeviceObject,
   __out_bcount(ConfigurationLength) PHIDD_CONFIGURATION Configuration,
   __in   ULONG                ConfigurationLength
   );
/*++
Routine Description:
    Get the configuration information for this Hid device

Arguments:
   HidDeviceObject      A handle to a Hid Device Object.

   Configuration        A configuration structure.  HidD_GetConfiguration MUST
                        be called before the configuration can be modified and
                        set using HidD_SetConfiguration

   ConfigurationLength  That is ``sizeof (HIDD_CONFIGURATION)''. Using this
                        parameter, we can later increase the length of the 
                        configuration array and not break older apps.

Return Value:
   TRUE if successful
   FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_SetConfiguration (
   __in   HANDLE               HidDeviceObject,
   __in_bcount(ConfigurationLength) PHIDD_CONFIGURATION Configuration,
   __in   ULONG                ConfigurationLength
   );
/*++
Routine Description:
   Set the configuration information for this Hid device...
   
   NOTE: HidD_GetConfiguration must be called to retrieve the current 
         configuration information before this information can be modified 
         and set.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Configuration        A configuration structure.  HidD_GetConfiguration MUST
                         be called before the configuration can be modified and
                         set using HidD_SetConfiguration
 
    ConfigurationLength  That is ``sizeof (HIDD_CONFIGURATION)''. Using this
                         parameter, we can later increase the length of the 
                         configuration array and not break older apps.

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetFeature (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(ReportBufferLength) PVOID ReportBuffer,
   __in    ULONG    ReportBufferLength
   );
/*++
Routine Description:
    Retrieve a feature report from a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer that the feature report should be placed 
                         into.  The first byte of the buffer should be set to
                         the report ID of the desired report
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         FeatureReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_SetFeature (
   __in    HANDLE   HidDeviceObject,
   __in_bcount(ReportBufferLength) PVOID ReportBuffer,
   __in    ULONG    ReportBufferLength
   );
/*++
Routine Description:
    Send a feature report to a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer of the feature report to send to the device
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         FeatureReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/

#if (NTDDI_VERSION >= NTDDI_WINXP) 
__checkReturn
BOOLEAN __stdcall
HidD_GetInputReport (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(ReportBufferLength) PVOID ReportBuffer,
   __in    ULONG    ReportBufferLength
   );
/*++
Routine Description:
    Retrieve an input report from a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer that the input report should be placed 
                         into.  The first byte of the buffer should be set to
                         the report ID of the desired report
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         InputReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_SetOutputReport (
   __in    HANDLE   HidDeviceObject,
   __in_bcount(ReportBufferLength) PVOID ReportBuffer,
   __in    ULONG    ReportBufferLength
   );
/*++
Routine Description:
    Send an output report to a HID device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    ReportBuffer         The buffer of the output report to send to the device
 
    ReportBufferLength   The size (in bytes) of ReportBuffer.  This value 
                         should be greater than or equal to the 
                         OutputReportByteLength field as specified in the 
                         HIDP_CAPS structure for the device
Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/

#endif
__checkReturn
BOOLEAN __stdcall
HidD_GetNumInputBuffers (
    __in  HANDLE  HidDeviceObject,
    __out PULONG  NumberBuffers
    );
/*++
Routine Description:
    This function returns the number of input buffers used by the specified
    file handle to the Hid device.  Each file object has a number of buffers
    associated with it to queue reports read from the device but which have
    not yet been read by the user-mode app with a handle to that device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    NumberBuffers        Number of buffers currently being used for this file
                         handle to the Hid device

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_SetNumInputBuffers (
    __in HANDLE HidDeviceObject,
    __in ULONG  NumberBuffers
    );
/*++

Routine Description:
    This function sets the number of input buffers used by the specified
    file handle to the Hid device.  Each file object has a number of buffers
    associated with it to queue reports read from the device but which have
    not yet been read by the user-mode app with a handle to that device.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    NumberBuffers        New number of buffers to use for this file handle to
                         the Hid device

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetPhysicalDescriptor (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves the raw physical descriptor for the specified
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the physical
                         descriptor if one exists for the specified device
                         handle

    BufferLength         Length of buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetManufacturerString (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves the manufacturer string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the manufacturer
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetProductString (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves the product string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the product
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)


Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetIndexedString (
   __in    HANDLE   HidDeviceObject,
   __in    ULONG    StringIndex,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves a string from the specified Hid device that is
    specified with a certain string index.

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    StringIndex          Index of the string to retrieve

    Buffer               Buffer which on return will contain the product
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
__checkReturn
BOOLEAN __stdcall
HidD_GetSerialNumberString (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves the serial number string from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the serial number
                         string returned from the device.  This string is a 
                         wide-character string

    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/

#if (NTDDI_VERSION >= NTDDI_WINXP) 
__checkReturn
BOOLEAN __stdcall
HidD_GetMsGenreDescriptor (
   __in    HANDLE   HidDeviceObject,
   __out_bcount(BufferLength) PVOID Buffer,
   __in    ULONG    BufferLength
   );
/*++
Routine Description:
    This function retrieves the Microsoft Genre descriptor from the specified 
    Hid device.  

Arguments:
    HidDeviceObject      A handle to a Hid Device Object.
 
    Buffer               Buffer which on return will contain the descriptor
                         returned from the device.
                         
    BufferLength         Length of Buffer (in bytes)

Return Value:
    TRUE if successful
    FALSE otherwise  -- Use GetLastError() to get extended error information
--*/
#endif


#include <poppack.h>

#endif

