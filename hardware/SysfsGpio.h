/*
	This code implements basic I/O hardware via the Raspberry Pi's GPIO port, using sysfs.
*/
#pragma once

#include "DomoticzHardware.h"
#include "../main/RFXtrx.h"

struct gpio_info
{
	int		pin_number;		// GPIO Pin number
	int		read_value_fd;	// Fast read fd
	int8_t	request_update;	// Request update
	int8_t	db_state;		// Database Value
	int8_t	value;			// GPIO pin Value
	int8_t	direction;		// GPIO IN or OUT
	int8_t	active_low;		// GPIO ActiveLow
	int8_t	edge;			// GPIO int Edge
	int8_t	id1;			// Device id1
	int8_t	id2;			// Device id2
	int8_t	id3;			// Device id3
	int8_t	id4;			// Device id4
	int8_t	id_valid;		// Device valid
};

class CSysfsGpio : public CDomoticzHardwareBase
{

public:

	CSysfsGpio(const int ID, const int ManualDevices);
	~CSysfsGpio();

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
