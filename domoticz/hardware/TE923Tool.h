#pragma once

#ifndef WIN32

typedef struct {
	// this are the values from station
	unsigned long timestamp;                 // timestamp for this dataset
	float t[6];                             // temperature of the sensors in °C
	signed short h[6];                      // humidity of the sensors in % rel
	float uv;                               // UV index
	float press;                            // air pressure in mBar
	unsigned char forecast;                 // weather forecast from station
	//                                      0 => heavy snow
	//                                      1 => snow
	//                                      2 => heavy rain
	//                                      3 => rain
	//                                      4 => cloudy
	//                                      5 => some clouds
	//                                      6 => sunny
	unsigned char storm;                    // storm warning if value = 1 else 0
	float wChill;                           // wind chill in °C
	float wGust;                            // wind gusts in m/s
	float wSpeed;                           // wind speed in m/s
	unsigned char wDir;                     // wind direction in x*22.5°; 0 => north
	unsigned int RainCount;                 // Raincounter of station as number up to 65535 * 0.7 mm/m²
	// status of sensors, names are _<sensor>; if status is <> 0, the value should be 0
	// 0  => value of sensor should be ok
	// -1 => data is invalid
	// -2 => sensor is out of range
	// -3 => missing link
	// -4 => any other error
	signed char _t[6], _h[6];
	signed char _press, _uv, _wDir, _wSpeed, _wGust, _wChill, _RainCount, _storm, _forecast;
	unsigned int __src;                     // source address of dataset (needed for dump)
} Te923DataSet_t;

class CTE923Tool
{
public:
	CTE923Tool(void);
	~CTE923Tool(void);

	bool OpenDevice();
	void CloseDevice();
	bool GetData(Te923DataSet_t *data);
	void GetPrintData( Te923DataSet_t *data, char *szOutputBuffer);

	bool m_bUSBIsInit;

	struct usb_device *find_te923();
	struct usb_dev_handle *te923_handle();
	void te923_close( usb_dev_handle *devh );
	struct usb_dev_handle *m_device_handle; 
private:
	int get_te923_lifedata( Te923DataSet_t *data );
	int get_te923_memdata( Te923DataSet_t *data );
	int read_from_te923( int adr, unsigned char *rbuf );
	int decode_te923_data( unsigned char buf[], Te923DataSet_t *data );
};

#endif