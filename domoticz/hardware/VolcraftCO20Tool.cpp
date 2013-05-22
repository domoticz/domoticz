#include "stdafx.h"

#ifndef WIN32

#include "VolcraftCO20Tool.h"
#include <iostream>     /* standard I/O functions                         */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "../main/Logger.h"

#ifdef WIN32
	#include "libusbwinusbbridge.h"
#else
	#include <usb.h>
	#include <unistd.h> //usleep
#endif

#define VolcraftCO20_VENDOR    0x03eb
#define VolcraftCO20_PRODUCT   0x2013
#define BUFLEN          35

#define MAX_LOOP_RETRY 20

struct usb_device *CVolcraftCO20Tool::find_VolcraftCO20() 
{
	usb_find_busses();
	usb_find_devices();
	struct usb_bus *bus;
	struct usb_device *dev;
	for ( bus = usb_get_busses(); bus; bus = bus->next ) {
		for ( dev = bus->devices; dev; dev = dev->next ) {
			if ( dev->descriptor.idVendor == VolcraftCO20_VENDOR && dev->descriptor.idProduct == VolcraftCO20_PRODUCT ) {
				return dev;
			}
		}
	}
	return NULL;
}

CVolcraftCO20Tool::CVolcraftCO20Tool(void)
{
	m_device_handle=NULL;
	usb_init();
}

CVolcraftCO20Tool::~CVolcraftCO20Tool(void)
{
	CloseDevice();
}

bool CVolcraftCO20Tool::OpenDevice()
{
	int ret;
	struct usb_device *dev;

	dev = find_VolcraftCO20();
	if ( dev == NULL ) {
		_log.Log(LOG_ERROR, "VolcraftCO20: Hardware not found.");
		return false;
	}

	m_device_handle = usb_open( dev );
	if ( m_device_handle == NULL ) {
		_log.Log(LOG_ERROR, "VolcraftCO20: Error while opening USB port and getting a device handler." );
		return false;
	}
#ifndef WIN32
	char driver_name[30];
	ret = usb_get_driver_np(m_device_handle, 0, driver_name, 30);
	if ( ret == 0 ) {
		usb_detach_kernel_driver_np( m_device_handle, 0 );
	}
#endif
	ret = usb_claim_interface( m_device_handle, 0 );
	if ( ret != 0 ) {
		_log.Log(LOG_ERROR, "VolcraftCO20: Error while claiming device interface (%d)." , ret );
		return false;
	}
	char buf[1000];
	ret = usb_set_altinterface(m_device_handle, 0);

	ret = usb_get_descriptor(m_device_handle, 0x0000001, 0x0000000, buf, 0x0000012);
	usleep(3*1000);
	ret = usb_get_descriptor(m_device_handle, 0x0000002, 0x0000000, buf, 0x0000009);
	usleep(2*1000);
	ret = usb_get_descriptor(m_device_handle, 0x0000002, 0x0000000, buf, 0x0000029);
	usleep(2*1000);
	ret = usb_release_interface(m_device_handle, 0);
	ret = usb_set_configuration(m_device_handle, 0x0000001);
	ret = usb_claim_interface(m_device_handle, 0);
	ret = usb_set_altinterface(m_device_handle, 0);
	usleep(34*1000);
	ret = usb_control_msg(m_device_handle, USB_TYPE_CLASS + USB_RECIP_INTERFACE, 0x000000a, 0x0000000, 0x0000000, buf, 0x0000000, 1000);
	usleep(11*1000);
	ret = usb_get_descriptor(m_device_handle, 0x0000022, 0x0000000, buf, 0x0000075);
	usleep(4*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000); 
	return true;
}

void CVolcraftCO20Tool::CloseDevice()
{
	if (m_device_handle!=NULL)
	{
#ifndef WIN32
		usb_release_interface( m_device_handle, 0 );
#endif
		usb_close(m_device_handle);
	}
	m_device_handle=NULL;
}

int is_big_endian(void)
{
	union {
		uint32_t i;
		char c[4];
	} bint = {0x01020304};

	return bint.c[0] == 1; 
}

unsigned short CVolcraftCO20Tool::GetVOC()
{
	if (m_device_handle==NULL)
		return 0;

	char buf[1000];

	int c0 = 104;
	int c1 = 37; 
	memcpy(buf, "\x40\x68\x2a\x54\x52\x0a\x40\x40\x40\x40\x40\x40\x40\x40\x40\x40", 0x0000010);
	buf[1] = (char) c0;
	c0++;
	if (c0 == 256) c0 = 103;
	ret = usb_interrupt_write(m_device_handle, 0x00000002, buf, 0x0000010, 1000);
	usleep(94*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000); 

	if ( !((ret == 0) || (ret == 16)))
	{
		return 0;
	}
	unsigned short iresult=0;
	memcpy(&iresult,buf+2,2);
	unsigned short voc = iresult;
	if (is_big_endian())
	{
		voc=((voc&0xFF)<<8)|((voc&0xFF00)>>8);
	}

	usleep(14*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000);

	usleep(3*1000);
	memcpy(buf, "\x40\x30\x30\x30\x37\x46\x4c\x41\x47\x47\x45\x54\x3f\x0a\x40\x40", 0x0000010);
	buf[4] = (char) c1;
	c1++;
	if (c1 == 40) c1++;
	else if (c1 == 47) c1=30;
	ret = usb_interrupt_write(m_device_handle, 0x00000002, buf, 0x0000010, 1000);

	usleep(23*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000);
	usleep(14*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000);
	usleep(8*1000);
	ret = usb_interrupt_read(m_device_handle, 0x00000081, buf, 0x0000010, 1000); 

	if ((voc<400)||(voc>2000))
		return 3000;
	return voc;
}

#endif
