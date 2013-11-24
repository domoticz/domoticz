#pragma once

#ifndef WIN32
#ifdef WITH_LIBUSB
class CVolcraftCO20Tool
{
public:
	CVolcraftCO20Tool(void);
	~CVolcraftCO20Tool(void);

	bool OpenDevice();
	void CloseDevice();
	//bool GetData(Te923DataSet_t *data);
	//void GetPrintData( Te923DataSet_t *data, char *szOutputBuffer);

	bool m_bUSBIsInit;

	struct usb_device *find_VolcraftCO20();
	struct usb_dev_handle *VolcraftCO20_handle();
	void VolcraftCO20_close( usb_dev_handle *devh );
	struct usb_dev_handle *m_device_handle; 
	unsigned short GetVOC();
};

#endif //WITH_LIBUSB
#endif //WIN32
