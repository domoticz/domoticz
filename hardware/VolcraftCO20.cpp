
#include "stdafx.h"
#ifndef WIN32
#ifdef WITH_LIBUSB
#include "VolcraftCO20.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include <usb.h>
#include <stdint.h>

#define VolcraftCO20_POLL_INTERVAL 30

#define DRIVER_NAME_LEN	1024
#define USB_BUF_LEN	1024

#define USB_VENDOR_CO2_STICK	0x03eb
#define USB_PRODUCT_CO2_STICK	0x2013

#define round(a) ( int ) ( a + .5 )

//code taken and modified form https://github.com/bwildenhain/air-quality-sensor/blob/master/src/air01.c
//at this moment it does not work under windows... no idea why... help appreciated!

CVolcraftCO20::CVolcraftCO20(const int ID)
{
	m_HwdID=ID;
}

CVolcraftCO20::~CVolcraftCO20(void)
{
}

bool CVolcraftCO20::StartHardware()
{
	RequestStart();

	//Start worker thread
	m_thread = std::make_shared<std::thread>(&CVolcraftCO20::Do_Work, this);
	SetThreadNameInt(m_thread->native_handle());
	m_bIsStarted=true;
	sOnConnected(this);

	return (m_thread != nullptr);
}

bool CVolcraftCO20::StopHardware()
{
	if (m_thread)
	{
		RequestStop();
		m_thread->join();
		m_thread.reset();
	}
	m_bIsStarted=false;
    return true;
}

void CVolcraftCO20::Do_Work()
{
	int sec_counter=VolcraftCO20_POLL_INTERVAL-5;
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter%12==0)
		{
			m_LastHeartbeat=mytime(NULL);
		}

		if (sec_counter%VolcraftCO20_POLL_INTERVAL==0)
		{
			GetSensorDetails();
		}
	}
	_log.Log(LOG_STATUS,"Voltcraft CO-20: Worker stopped...");
}

bool CVolcraftCO20::WriteToHardware(const char *pdata, const unsigned char length)
{
	return false;
}

static int read_one_sensor (struct usb_device *dev, uint16_t &value)
{
	int ret;
	struct usb_dev_handle *devh;
	char driver_name[DRIVER_NAME_LEN] = "";
	char usb_io_buf[USB_BUF_LEN] =
		"\x40\x68\x2a\x54"
		"\x52\x0a\x40\x40"
		"\x40\x40\x40\x40"
		"\x40\x40\x40\x40";

	/* Open USB device.  */
	devh = usb_open (dev);
	if (! dev) {
		fprintf (stderr, "Failed to usb_open(%p)\n", (void *) dev);
		ret = -1;
		goto out;
	}

#ifndef __NetBSD__
	/* Ensure that the device isn't claimed.  */
	ret = usb_get_driver_np (devh, 0/*intrf*/, driver_name, sizeof (driver_name));
	if (! ret) {
		_log.Log(LOG_ERROR,"Voltcraft CO-20: Warning: device is claimed by driver %s, trying to unbind it.", driver_name);
		ret = usb_detach_kernel_driver_np (devh, 0/*intrf*/);
		if (ret) {
			_log.Log(LOG_ERROR,"Voltcraft CO-20: Warning: Failed to detatch kernel driver.");
			ret = -2;
			goto out;
		}
	}
#endif

	/* Claim device.  */
	ret = usb_claim_interface (devh, 0/*intrf*/);
	if (ret) {
		_log.Log(LOG_ERROR,"Voltcraft CO-20: usb_claim_interface() failed with error %d=%s",	ret, strerror (-ret));
		ret = -3;
		goto out;
	}

	/* Send query command.  */
	ret = usb_interrupt_write(devh, 0x0002/*endpoint*/,
		usb_io_buf, 0x10/*len*/, 1000/*msec*/);
	if (ret < 0) {
		_log.Log(LOG_ERROR,"Voltcraft CO-20:  Failed to usb_interrupt_write() the initial buffer, ret = %d", ret);
		ret = -4;
		goto out_unlock;
	}

	/* Read answer.  */
	ret = usb_interrupt_read(devh, 0x0081/*endpoint*/,
		usb_io_buf, 0x10/*len*/, 1000/*msec*/);
	if (ret < 0) {
		_log.Log(LOG_ERROR,"Voltcraft CO-20: Failed to usb_interrupt_read() #1");
		ret = -5;
		goto out_unlock;
	}

	/* On empty read, read again.  */
	if (ret == 0) {
		ret = usb_interrupt_read(devh, 0x0081/*endpoint*/,
			usb_io_buf, 0x10/*len*/, 1000/*msec*/);
		if (ret == 0) {
			//give up!
			goto out_unlock;
		}
	}

	/* Prepare value from first read.  */
	value = ((unsigned char *)usb_io_buf)[3] << 8
		| ((unsigned char *)usb_io_buf)[2] << 0;

	/* Dummy read.  */
	usb_interrupt_read(devh, 0x0081/*endpoint*/,
		usb_io_buf, 0x10/*len*/, 1000/*msec*/);
out_unlock:
	ret = usb_release_interface(devh, 0/*intrf*/);
	if (ret) {
		fprintf(stderr, "Failed to usb_release_interface()\n");
		ret = -5;
	}
out:
	if (devh)
		usb_close (devh);
	return ret;
}

static int get_voc_value(int vendor, int product, uint16_t &voc)
{
	voc=0;
	struct usb_bus *bus;
	struct usb_device *dev;
	int ret = 0;

	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor == vendor && dev->descriptor.idProduct == product)
			{
				ret |= read_one_sensor(dev,voc);
			}
		}
	}
	return ret;
}

void CVolcraftCO20::GetSensorDetails()
{
	try
	{
		uint16_t voc;

		usb_init ();
		usb_set_debug (0);
		usb_find_busses ();
		usb_find_devices ();

		get_voc_value(USB_VENDOR_CO2_STICK,USB_PRODUCT_CO2_STICK,voc);
		if (voc!=0)
		{
			if (voc==3000)
			{
				_log.Log(LOG_ERROR, "Voltcraft CO-20: Sensor data out of range!!");
				return;
			}
			SendAirQualitySensor(1, 1, 255, voc, "Air Quality");
		}
	}
	catch (...)
	{

	}
}

#endif //WITH_LIBUSB
#endif //WIN32
