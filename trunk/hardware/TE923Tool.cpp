#include "stdafx.h"

#ifndef WIN32
#ifdef WITH_LIBUSB

#include "TE923Tool.h"
#include <iostream>     /* standard I/O functions                         */
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "../main/Logger.h"

//Part of this class is written by Sebastian John
/***************************************************************************
 *   Copyright (C) 2010 by Sebastian John                                  *
 *   te923@fukz.org                                                        *
 *   for more informations visit http://te923.fukz.org                     *
 ***************************************************************************/

#ifdef WIN32
	#include "libusbwinusbbridge.h"
#else
	#include <usb.h>
#endif

#define TE923_VENDOR    0x1130
#define TE923_PRODUCT   0x6801
#define BUFLEN          35

#define MAX_LOOP_RETRY 20

struct usb_device *CTE923Tool::find_te923() 
{
	usb_find_busses();
	usb_find_devices();
	struct usb_bus *bus;
	struct usb_device *dev;
	for ( bus = usb_get_busses(); bus; bus = bus->next ) {
		for ( dev = bus->devices; dev; dev = dev->next ) {
			if ( dev->descriptor.idVendor == TE923_VENDOR && dev->descriptor.idProduct == TE923_PRODUCT ) {
				return dev;
			}
		}
	}
	return NULL;
}

CTE923Tool::CTE923Tool(void)
{
	m_device_handle=NULL;
	usb_init();
}

CTE923Tool::~CTE923Tool(void)
{
	CloseDevice();
}

bool CTE923Tool::OpenDevice()
{
	int ret;
	struct usb_device *dev;

	dev = find_te923();
	if ( dev == NULL ) {
		_log.Log(LOG_ERROR, "TE923: Weather station not found.");
		return false;
	}

	m_device_handle = usb_open( dev );
	if ( m_device_handle == NULL ) {
		_log.Log(LOG_ERROR, "TE923: Error while opening USB port and getting a device handler." );
		return false;
	}
#ifndef WIN32
	char driver_name[30];
	ret = usb_get_driver_np(m_device_handle, 0, driver_name, 30);
	if ( ret == 0 ) {
		usb_detach_kernel_driver_np( m_device_handle, 0 );
	}
#endif
	ret = usb_set_configuration( m_device_handle, 1 );
	if ( ret != 0 ) {
		_log.Log(LOG_ERROR, "TE923: Error while setting device configuration (%d)." , ret );
		return false;
	}

	ret = usb_claim_interface( m_device_handle, 0 );
	if ( ret != 0 ) {
		_log.Log(LOG_ERROR, "TE923: Error while claiming device interface (%d)." , ret );
		return false;
	}
#ifndef WIN32
	ret = usb_set_altinterface( m_device_handle, 0 );
	if ( ret != 0 ) {
		_log.Log(LOG_ERROR, "TE923: Error while setting alternative device interface (%d)." , ret );
		return false;
	}
	sleep(0.5);
#endif
	return true;
}

void CTE923Tool::CloseDevice()
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

int bcd2int( char bcd ) {
	return (( int )(( bcd & 0xF0 ) >> 4 ) * 10 + ( int )( bcd & 0x0F ) );
}

int CTE923Tool::decode_te923_data( unsigned char buf[], Te923DataSet_t *data ) 
{
	//decode temperature and humidity from all sensors
	int i;
	for ( i = 0; i <= 5; i++ ) {
		int offset = i * 3;
		data->_t[i] = 0;
		if ( bcd2int( buf[0+offset] & 0x0F ) > 9 ) 
		{
			if ((( buf[0+offset] & 0x0F ) == 0x0C ) || (( buf[0+offset] & 0x0F ) == 0x0B ) ) {
				data->_t[i] = -2;
			} else {
				data->_t[i] = -1;
			}
		}
		if ((( buf[1+offset] & 0x40 ) != 0x40 ) && i > 0 ) {
			data->_t[i] = -2;
		}
		if ( data->_t[i] == 0 ) {
			data->t[i] = (float)(( bcd2int( buf[0+offset] ) / 10.0 ) + ( bcd2int( buf[1+offset] & 0x0F ) * 10.0 ));
			if (( buf[1+offset] & 0x20 ) == 0x20 )
				data->t[i] += 0.05f;
			if (( buf[1+offset] & 0x80 ) != 0x80 )
				data->t[i] *= -1;
		} else
			data->t[i] = 0;

		if ( data->_t[i] <= -2 ) {
			data->_h[i] = -2;
			data->h[i] = 0;
		} else if ( bcd2int( buf[2+offset] & 0x0F ) > 9 ) {
			data->_h[i] = -3;
			data->h[i] = 0;
		} else {
			data->h[i] = bcd2int( buf[2+offset] );
			data->_h[i] = 0;
		}
	}

	//decode value from UV sensor
	if (( buf[18] == 0xAA ) && ( buf[19] == 0x0A ) ) {
		data->_uv = -3;
		data->uv = 0;
	} else if (( bcd2int( buf[18] ) > 99 ) || ( bcd2int( buf[19] ) > 99 ) ) {
		data->_uv = -1;
		data->uv = 0;
	}

	else {
		data->uv = (float)(bcd2int( buf[18] & 0x0F ) / 10.0 + bcd2int( buf[18] & 0xF0 ) + bcd2int( buf[19] & 0x0F ) * 10.0);
		data->_uv = 0;
	}

	//decode pressure
	if (( buf[21] & 0xF0 ) == 0xF0 ) {
		data->press = 0;
		data->_press = -1;
	} else {
		data->press = (float)(( int )( buf[21] * 0x100 + buf[20] ) * 0.0625);
		data->_press = 0;
	}

	//decode weather status and storm warning
	if (( buf[22] & 0x0F ) == 0x0F ) {
		data->_storm = -1;
		data->_forecast = -1;
		data->storm = 0;
		data->forecast = 0;
	} else {
		data->_storm = 0;
		data->_forecast = 0;
		if (( buf[22] & 0x08 ) == 0x08 )
			data->storm = 1;
		else
			data->storm = 0;

		data->forecast = ( int )( buf[22] & 0x07 );
	}

	//decode wind chill
	if (( bcd2int( buf[23] & 0xF0 ) > 90 ) || ( bcd2int( buf[23] & 0x0F ) > 9 ) ) {
		if (( buf[23] == 0xAA ) && ( buf[24] == 0x8A ) )
			data->_wChill = -1;
		else if (( buf[23] == 0xBB ) && ( buf[24] == 0x8B ) )
			data->_wChill = -2;
		else if (( buf[23] == 0xEE ) && ( buf[24] == 0x8E ) )
			data->_wChill = -3;
		else
			data->_wChill = -4;
	} else
		data->_wChill = 0;
	if ((( buf[24] & 0x40 ) != 0x40 ) )
		data->_wChill = -2;
	if ( data->_wChill == 0 ) {
		data->wChill = float(( bcd2int( buf[23] ) / 10.0 ) + ( bcd2int( buf[24] & 0x0F ) * 10.0 ));
		if (( buf[24] & 0x20 ) == 0x20 )
			data->wChill += 0.05f;
		if (( buf[24] & 0x80 ) != 0x80 )
			data->wChill *= -1;
		data->_wChill = 0;
	} else
		data->wChill = 0;

	//decode wind gust
	if (( bcd2int( buf[25] & 0xF0 ) > 90 ) || ( bcd2int( buf[25] & 0x0F ) > 9 ) ) {
		data->_wGust = -1;
		if (( buf[25] == 0xBB ) && ( buf[26] == 0x8B ) )
			data->_wGust = -2;
		else if (( buf[25] == 0xEE ) && ( buf[26] == 0x8E ) )
			data->_wGust = -3;
		else
			data->_wGust = -4;
	} else
		data->_wGust = 0;

	if ( data->_wGust == 0 ) {

		int offset = 0;
		if (( buf[26] & 0x10 ) == 0x10 )
			offset = 100;
		data->wGust = (float)((( bcd2int( buf[25] ) / 10.0 ) + ( bcd2int( buf[26] & 0x0F ) * 10.0 ) + offset ) / 2.23694);
	} else
		data->wGust = 0;

	//decode wind speed
	if (( bcd2int( buf[27] & 0xF0 ) > 90 ) || ( bcd2int( buf[27] & 0x0F ) > 9 ) ) {
		data->_wSpeed = -1;
		if (( buf[27] == 0xBB ) && ( buf[28] == 0x8B ) )
			data->_wSpeed = -2;
		else if (( buf[27] == 0xEE ) && ( buf[28] == 0x8E ) )
			data->_wSpeed = -3;
		else
			data->_wSpeed = -4;
	} else
		data->_wSpeed = 0;

	if ( data->_wSpeed == 0 ) {

		int offset = 0;
		if (( buf[28] & 0x10 ) == 0x10 )
			offset = 100;
		data->wSpeed = (float)((( bcd2int( buf[27] ) / 10.0 ) + ( bcd2int( buf[28] & 0x0F ) * 10.0 ) + offset ) / 2.23694);
	} else
		data->wSpeed = 0;

	//decode wind direction
	if (( data->_wGust <= -3 ) || ( data->_wSpeed <= -3 ) ) {
		data->_wDir = -3;
		data->wDir = 0;
	} else {
		data->wDir = ( int )buf[29] & 0x0F;
		data->_wDir = 0;
	}

	//decode rain counter
	//don't know to find out it sensor link missing, but is no problem, because the counter is inside
	//the station, not in the sensor.
	data->_RainCount = 0;
	data->RainCount = ( int )( buf[31] * 0x100 + buf[30] );
	return 0;
}

int CTE923Tool::read_from_te923( int adr, unsigned char *rbuf ) 
{
	int timeout = 50;
	int i, ret, bytes;
	int count = 0;
	unsigned char crc;
	unsigned char buf[] = {0x05, 0x0AF, 0x00, 0x00, 0x00, 0x00, 0xAF, 0xFE};
	buf[4] = adr / 0x10000;
	buf[3] = ( adr - ( buf[4] * 0x10000 ) ) / 0x100;
	buf[2] = adr - ( buf[4] * 0x10000 ) - ( buf[3] * 0x100 );
	buf[5] = ( buf[1] ^ buf[2] ^ buf[3] ^ buf[4] );
	ret = usb_control_msg( m_device_handle, 0x21, 0x09, 0x0200, 0x0000, (char*)&buf, 0x08, timeout );
	if ( ret < 0 )
		return -1;
#ifndef WIN32
	sleep( 0.3 );
#endif
	while ( usb_interrupt_read( m_device_handle, 0x01, (char*)&buf, 0x8, timeout ) > 0 ) 
	{
#ifndef WIN32
		sleep( 0.15 );
#endif
		bytes = ( int )buf[0];
		if (( count + bytes ) < BUFLEN )
			memcpy( rbuf + count, buf + 1, bytes );
		count += bytes;
	}
	crc = 0x00;
	for ( i = 0; i <= 32; i++ )
		crc = crc ^ rbuf[i];
	if ( crc != rbuf[33] )
		return -2;
	if ( rbuf[0] != 0x5a )
		return -3;
	return count;
}

int CTE923Tool::get_te923_lifedata( Te923DataSet_t *data )
{
	int ret;
	int adr = 0x020001;
	unsigned char buf[BUFLEN];
	unsigned char readretries=0;
	do
	{
		ret = read_from_te923( adr, (unsigned char*)&buf );
		readretries++;
	}
	while (( ret <= 0 )&&(readretries<MAX_LOOP_RETRY));
	if ( buf[0] != 0x5A )
		return -1;
	memmove( buf, buf + 1, BUFLEN - 1 );
	data->timestamp = (unsigned long)time( NULL );
	decode_te923_data( buf, data );
	return 0;
}

int CTE923Tool::get_te923_memdata( Te923DataSet_t *data )
{
	int adr, ret;
	unsigned char buf[BUFLEN];
	unsigned char databuf[BUFLEN];
	if ( data->__src == 0 ) {
		//first read, get oldest data
		int last_adr = 0x0000FB;
		unsigned char readretries=0;
		do
		{
			ret = read_from_te923( last_adr, buf );
			readretries++;
		}
		while (( ret <= 0 )&&(readretries<MAX_LOOP_RETRY));
		if (readretries>=MAX_LOOP_RETRY)
			return -1;

		adr = (((( int )buf[3] ) * 0x100 + ( int )buf[5] ) * 0x26 ) + 0x101;
	} else
		adr = data->__src;

	time_t tm = time( NULL );
	struct tm *timeinfo = localtime( &tm );
	int sysyear = timeinfo->tm_year;
	int sysmon = timeinfo->tm_mon;
	unsigned char readretries=0;
	do
	{
		ret = read_from_te923( adr, buf );
		readretries++;
	}
	while (( ret <= 0 )&&(readretries<MAX_LOOP_RETRY));
	if (readretries>=MAX_LOOP_RETRY)
		return -1;

	int day = bcd2int( buf[2] );
	int mon = ( int )( buf[1] & 0x0F );
	int year = sysyear;
	if ( mon > sysmon + 1 )
		year--;
	int hour = bcd2int( buf[3] );
	int minute = bcd2int( buf[4] );

	struct tm newtime;
	newtime.tm_year = year;
	newtime.tm_mon  = mon - 1;
	newtime.tm_mday = day;
	newtime.tm_hour = hour;
	newtime.tm_min  = minute;
	newtime.tm_sec  = 0;
	newtime.tm_isdst = -1;

	data->timestamp = (unsigned long)mktime( &newtime );
	memcpy( databuf, buf + 5, 11 );
	adr += 0x10;
	readretries=0;
	do
	{
		ret = read_from_te923( adr, buf );
		readretries++;
	}
	while (( ret <= 0 )&&(readretries<MAX_LOOP_RETRY));
	if (readretries>=MAX_LOOP_RETRY)
		return -1;

	memcpy( databuf + 11, buf + 1, 21 );
	decode_te923_data( databuf, data );
	adr += 0x16;
	if ( adr > 0x001FBB )
		adr = 0x000101;
	data->__src = adr;

	return 0;
}

void CTE923Tool::GetPrintData( Te923DataSet_t *data, char *szOutputBuffer) 
{
	char szTmp[100];
	const char *iText="i";
	int i;
	sprintf(szOutputBuffer, "%ld:"  , data->timestamp );
	for ( i = 0; i <= 5; i++ ) {

		if ( data->_t[i] == 0 )  
			sprintf(szTmp, "%0.2f:", data->t[i] );
		else
			sprintf(szTmp, "%s:", iText );
		strcat(szOutputBuffer,szTmp);

		if ( data->_h[i] == 0 )  
			sprintf(szTmp, "%d:", data->h[i] );
		else
			sprintf(szTmp, "%s:", iText );
		strcat(szOutputBuffer,szTmp);
	}

	if ( data->_press == 0 ) 
		sprintf(szTmp, "%0.1f:", data->press );
	else
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_uv == 0 ) 
		sprintf(szTmp, "%0.1f:", data->uv );
	else
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_forecast == 0 ) 
		sprintf(szTmp, "%d:", data->forecast );
	else
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_storm == 0 ) 
		sprintf(szTmp, "%d:", data->storm );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_wDir == 0 ) 
		sprintf(szTmp, "%d:", data->wDir );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_wSpeed == 0 ) 
		sprintf(szTmp, "%0.1f:", data->wSpeed );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_wGust == 0 ) 
		sprintf(szTmp, "%0.1f:", data->wGust );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_wChill == 0 ) 
		sprintf(szTmp, "%0.1f:", data->wChill );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);

	if ( data->_RainCount == 0 ) 
		sprintf(szTmp, "%d", data->RainCount );
	else 
		sprintf(szTmp, "%s:", iText );
	strcat(szOutputBuffer,szTmp);
}

bool CTE923Tool::GetData(Te923DataSet_t *data)
{
	if (m_device_handle==NULL)
		return false;

	memset (data, 0, sizeof ( Te923DataSet_t ) );
	//int ret=get_te923_memdata(data);
	int ret=get_te923_lifedata( data );
	return (ret!=-1);
	//printData( data, iText);
}

#endif //WITH_LIBUSB
#endif //WIN32
