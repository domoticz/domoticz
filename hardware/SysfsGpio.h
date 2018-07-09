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
	int		edge_fd;		// Edge detect fd
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

	CSysfsGpio(const int ID, const int ManualDevices, const int Debounce);
	~CSysfsGpio();

	bool WriteToHardware(const char *pdata, const unsigned char length) override;
	static std::vector<int> GetGpioIds();
	static std::vector<std::string> GetGpioNames();
	static void RequestDbUpdate(int pin);
private:
	bool StartHardware() override;
	bool StopHardware() override;
	void FindGpioExports();
	void Do_Work();
	void EdgeDetectThread();
	void Init();
	void PollGpioInputs(bool PollOnce);
	void CreateDomoticzDevices();
	void UpdateDomoticzInputs(bool forceUpdate);
	void UpdateDomoticzDatabase();
	void UpdateGpioOutputs();
	void UpdateDeviceID(int pin);
	std::vector<std::string> GetGpioDeviceId();
	int GpioRead(int pin, const char* param);
	int GpioReadFd(int fd);
	int GpioWrite(int pin, int value);
	int GpioOpenRw(int gpio_pin);
	int GetReadResult(int bytecount, char* value_str);
	int GpioGetState(int index);
	void GpioSaveState(int index, int value);

	static std::vector<gpio_info> m_saved_state;
	static int m_sysfs_hwdid;
	static int m_sysfs_req_update;
	bool m_polling_enabled;
	bool m_interrupts_enabled;
	volatile bool m_stoprequested;
	int m_debounce_msec;
	int m_auto_configure_devices;
	int m_maxfd;
	fd_set m_rfds;
	tRBUF m_Packet;

	std::shared_ptr<std::thread> m_thread;
	std::shared_ptr<std::thread> m_edge_thread;
	std::mutex m_state_mutex;
};
