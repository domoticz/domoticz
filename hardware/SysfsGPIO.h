/*
	This code implements basic I/O hardware via the Raspberry Pi's GPIO port, using sysfs.
*/
#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

struct gpio_info
{
	uint8_t	value;		// GPIO pin Value
	uint8_t	direction;	// GPIO IN or OUT
	uint8_t	active_low;	// GPIO ActiveLow
	uint8_t	edge;		// GPIO int Edge
	uint8_t	db_state;	// Database Value
	uint8_t	db_state;	// Database Value id1;			// Device id1
	uint8_t	db_state;	// Database Value id2;			// Device id2
	uint8_t	db_state;	// Database Valueid3;			// Device id3
	uint8_t	db_state;	// Database Valueid4;			// Device id4
	uint8_t	db_state;	// Database Valueid_valid;		// Device valid
	uint8_t	db_state;	// Database Valueint request_update; // Request update
	int	read_value_fd;	// Fast read fd
	int	pin_number;		// GPIO Pin number
};

class CSysfsGPIO : public CDomoticzHardwareBase
{

public:

	CSysfsGPIO(const int ID, const int ManualDevices);
	~CSysfsGPIO();

	bool WriteToHardware(const char *pdata, const unsigned char length);
	static std::vector<int> GetGpioIds();
	static std::vector<std::string> GetGpioNames();
	static void RequestDbUpdate(int pin);

private:
	bool StartHardware();
	bool StopHardware();
	void FindGpioExports();
	void Do_Work();
	void Init();
	void PollGpioInputs();
	void CreateDomoticzDevices();
	void UpdateDomoticzInputs(bool forceUpdate);
	void UpdateDomoticzDatabase();
	void UpdateGpioOutputs();
	void UpdateDeviceID(int pin);
	std::vector<std::string> GetGpioDeviceId();
	int GPIORead(int pin, const char* param);
	int GPIOReadFd(int fd);
	int GPIOWrite(int pin, int value);
	int GetReadResult(int bytecount, char* value_str);
	boost::shared_ptr<boost::thread> m_thread;
	static std::vector<gpio_info> GpioSavedState;
	static int sysfs_hwdid;
	static int sysfs_req_update;
	volatile bool m_stoprequested;
	int m_auto_configure_devices;
	tRBUF m_Packet;
};
