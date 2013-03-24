

/*
 * libusb-winusb-bridge.h
 *
 * Simple backend for libusb using MS WinUsb. Only the basic functions
 * necessary for apcupsd are implemented, although the others could be added
 * fairly easily.
 */

/*
 * Copyright (C) 2010 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef __LIBUSB_WINUSB_BRIDGE_H
#define __LIBUSB_WINUSB_BRIDGE_H

//#include "winusb.h"
#include <limits.h>

// Make old libusb constants from WinUSB constants
#define USB_ENDPOINT_IN         USB_ENDPOINT_DIRECTION_MASK
#define USB_ENDPOINT_OUT        0
#define USB_RECIP_INTERFACE     (BMREQUEST_TO_INTERFACE << 0)
#define USB_TYPE_CLASS          (BMREQUEST_CLASS << 5)
#define USB_REQ_GET_DESCRIPTOR  USB_REQUEST_GET_DESCRIPTOR
#define USB_DT_REPORT           0x22
#define USB_REQ_GET_REPORT      0x01
#define USB_REQ_SET_REPORT      0x09

// usb device descriptor
struct usb_device_descriptor
{
   unsigned char  bLength;
   unsigned char  bDescriptorType;
   unsigned short bcdUSB;
   unsigned char  bDeviceClass;
   unsigned char  bDeviceSubClass;
   unsigned char  bDeviceProtocol;
   unsigned char  bMaxPacketSize0;
   unsigned short idVendor;
   unsigned short idProduct;
   unsigned short bcdDevice;
   unsigned char  iManufacturer;
   unsigned char  iProduct;
   unsigned char  iSerialNumber;
   unsigned char  bNumConfigurations;
};

// libusb structures, abbreviated
struct usb_bus;
struct usb_device
{
  struct usb_device *next, *prev;
  char filename[512 + 1];
  struct usb_bus *bus;
  struct usb_device_descriptor descriptor;
};

struct usb_bus
{
  struct usb_bus *next, *prev;
  char dirname[512 + 1];
  struct usb_device *devices;
};

struct usb_dev_handle;
typedef struct usb_dev_handle usb_dev_handle;

// libusb function prototypes (just the ones we support)
usb_dev_handle *usb_open(struct usb_device *dev);
int usb_close(usb_dev_handle *dev);
int usb_get_string_simple(
   usb_dev_handle *dev, int index, char *buf, size_t buflen);
int usb_interrupt_read(
   usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
int usb_control_msg(
   usb_dev_handle *dev, int requesttype, int request, int value, 
   int index, char *bytes, int size, int timeout);
int usb_set_configuration(usb_dev_handle *dev, int configuration);
int usb_claim_interface(usb_dev_handle *dev, int iface);
int usb_reset(usb_dev_handle *dev);
char *usb_strerror(void);
void usb_init(void);
void usb_set_debug(int level);
int usb_find_busses(void);
int usb_find_devices(void);
struct usb_bus *usb_get_busses(void);

#endif  // __LIBUSB_WINUSB_BRIDGE_H
