/*
	USAGE:
	======
	To use "Generic sysfs gpio" you need to export the inputs and outputs you
	are going to use. On start up "Gerenic sysfs gpio" will pick up the exports
	that are available and starts processing them. You may want to use chmod to
	set gpio permissions

	GPIO Sysfs Interface for Userspace
	==================================

	Platforms which use the "gpiolib" implementors framework may choose to
	configure a sysfs user interface to GPIOs. This is different from the
	debugfs interface, since it provides control over GPIO direction and
	value instead of just showing a gpio state summary. Plus, it could be
	present on production systems without debugging support.

	Given appropriate hardware documentation for the system, userspace could
	know for example that GPIO #23 controls the write protect line used to
	protect boot loader segments in flash memory. System upgrade procedures
	may need to temporarily remove that protection, first importing a GPIO,
	then changing its output state, then updating the code before re-enabling
	the write protection. In normal use, GPIO #23 would never be touched,
	and the kernel would have no need to know about it.

	Again depending on appropriate hardware documentation, on some systems
	userspace GPIO can be used to determine system configuration data that
	standard kernels won't know about. And for some tasks, simple userspace
	GPIO drivers could be all that the system really needs.

	DO NOT ABUSE SYSFS TO CONTROL HARDWARE THAT HAS PROPER KERNEL DRIVERS.
	PLEASE READ THE DOCUMENT NAMED "drivers-on-gpio.txt" IN THIS DOCUMENTATION
	DIRECTORY TO AVOID REINVENTING KERNEL WHEELS IN USERSPACE. I MEAN IT.
	REALLY.

	Paths in Sysfs
	--------------
	There are three kinds of entries in /sys/class/gpio:

	-	Control interfaces used to get userspace control over GPIOs;

	-	GPIOs themselves; and

	-	GPIO controllers ("gpio_chip" instances).

	That's in addition to standard files including the "device" symlink.

	The control interfaces are write-only:

	/sys/class/gpio/

	"export" ... Userspace may ask the kernel to export control of
	a GPIO to userspace by writing its number to this file.

	Example:  "echo 19 > export" will create a "gpio19" node
	for GPIO #19, if that's not requested by kernel code.

	"unexport" ... Reverses the effect of exporting to userspace.

	Example:  "echo 19 > unexport" will remove a "gpio19"
	node exported using the "export" file.

	GPIO signals have paths like /sys/class/gpio/gpio42/ (for GPIO #42)
	and have the following read/write attributes:

	/sys/class/gpio/gpioN/

	"direction" ... reads as either "in" or "out". This value may
	normally be written. Writing as "out" defaults to
	initializing the value as low. To ensure glitch free
	operation, values "low" and "high" may be written to
	configure the GPIO as an output with that initial value.

	Note that this attribute *will not exist* if the kernel
	doesn't support changing the direction of a GPIO, or
	it was exported by kernel code that didn't explicitly
	allow userspace to reconfigure this GPIO's direction.

	"value" ... reads as either 0 (low) or 1 (high). If the GPIO
	is configured as an output, this value may be written;
	any nonzero value is treated as high.

	If the pin can be configured as interrupt-generating interrupt
	and if it has been configured to generate interrupts (see the
	description of "edge"), you can poll(2) on that file and
	poll(2) will return whenever the interrupt was triggered. If
	you use poll(2), set the events POLLPRI and POLLERR. If you
	use select(2), set the file descriptor in exceptfds. After
	poll(2) returns, either lseek(2) to the beginning of the sysfs
	file and read the new value or close the file and re-open it
	to read the value.

	"edge" ... reads as either "none", "rising", "falling", or
	"both". Write these strings to select the signal edge(s)
	that will make poll(2) on the "value" file return.

	This file exists only if the pin can be configured as an
	interrupt generating input pin.

	"active_low" ... reads as either 0 (false) or 1 (true). Write
	any nonzero value to invert the value attribute both
	for reading and writing. Existing and subsequent
	poll(2) support configuration via the edge attribute
	for "rising" and "falling" edges will follow this
	setting.

*/

#include "stdafx.h"

#ifdef WITH_SYSFS_GPIO

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SysfsGPIO.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include "../main/localtime_r.h"

/* 
Note:
HEATBEAT_COUNT, UPDATE_MASTER_COUNT and GPIO_POLL_MSEC are related.
Set values so that that heartbeat occurs every 10 seconds and update 
master occurs once, 30 seconds after startup. 
*/
#define HEARTBEAT_COUNT		40
#define UPDATE_MASTER_COUNT 120
#define GPIO_POLL_MSEC		250

#define GPIO_IN				0
#define GPIO_OUT			1
#define GPIO_LOW			0
#define GPIO_HIGH			1
#define GPIO_NONE			0
#define GPIO_RISING			1
#define GPIO_FALLING		2
#define GPIO_BOTH			3
#define GPIO_PIN_MIN		0
#define GPIO_PIN_MAX		31
#define GPIO_MAX_VALUE_SIZE	16		
#define GPIO_MAX_PATH		64
#define GPIO_PATH			"/sys/class/gpio/gpio"

CSysfsGPIO::CSysfsGPIO(const int ID)
{
	m_stoprequested = false;
	m_bIsStarted = false;
	m_HwdID = ID;
}

CSysfsGPIO::~CSysfsGPIO(void)
{
}

bool CSysfsGPIO::StartHardware()
{
	GpioSavedState.clear();

	Init();

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSysfsGPIO::Do_Work, this)));
	m_bIsStarted = true;

	return (m_thread != NULL);
}

bool CSysfsGPIO::StopHardware()
{
	m_stoprequested = true;

	try
	{
		if (m_thread)
		{
			m_thread->join();
		}
	}
	catch (...)
	{
	}

	m_bIsStarted = false;

	return true;
}

bool CSysfsGPIO::WriteToHardware(const char *pdata, const unsigned char length)
{
	bool bOk = false;
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;
	int gpio_pin = pSen->LIGHTING2.unitcode;

	for (int i = 0; i < GpioSavedState.size(); i++)
	{
		if ((GpioSavedState[i].pin_number == gpio_pin) &&
			(GpioSavedState[i].direction == GPIO_OUT) && 
			(packettype == pTypeLighting2))
		{
			if (pSen->LIGHTING2.cmnd == light2_sOn)
			{
				GPIOWrite(gpio_pin, true);
			}
			else
			{
				GPIOWrite(gpio_pin, false);
			}
			bOk = true;
			break;
		}
	}
	return bOk;
}

void CSysfsGPIO::Do_Work()
{
	char path[GPIO_MAX_PATH];
	char value_str[3];
	int fd;
	int input_count = 0;
	int output_count = 0;

	for (int i = 0; i < GpioSavedState.size(); i++)
	{
		if (GpioSavedState[i].direction == 0)
		{
			input_count++;
		}

		if (GpioSavedState[i].direction == 1)
		{
			output_count++;
		}
	}

	/* create fd's used for fast reads */
	for (int i = 0; i < GpioSavedState.size(); i++)
	{
		snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, GpioSavedState[i].pin_number);
		GpioSavedState[i].read_value_fd = open(path, O_RDONLY);
	}

	int counter = 0;
	_log.Log(LOG_STATUS, "SysfsGPIO: Input poller started, inputs:%d outputs:%d", input_count, output_count);

	while (!m_stoprequested)
	{
		sleep_milliseconds(GPIO_POLL_MSEC);
		counter++;

		if (counter % HEARTBEAT_COUNT == 0)	/* Heartbeat maintenance */
		{
			m_LastHeartbeat = mytime(NULL);
		}

		if (!m_stoprequested)
		{
			PollGpioInputs();
			UpdateDomoticzInputs(false);
		}

		if (counter == UPDATE_MASTER_COUNT)	/* only executes when master/slave configuration*/
		{
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT ID FROM Users WHERE (RemoteSharing==1) AND (Active==1)");

			if (result.size() > 0)
			{
				_log.Log(LOG_STATUS, "SysfsGPIO: Update master devices");
				UpdateDomoticzInputs(true);
			}
		}
	}

	for (int i = 0; i < GpioSavedState.size(); i++) /* close all fast read fd's */
	{
		snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, GpioSavedState[i].pin_number);
		close(GpioSavedState[i].read_value_fd);
		GpioSavedState[i].read_value_fd = -1;
	}

	_log.Log(LOG_STATUS, "SysfsGPIO: Input poller stopped");
}

void CSysfsGPIO::Init()
{
	BYTE id1 = 0x03;
	BYTE id2 = 0x0E;
	BYTE id3 = 0x0E;
	BYTE id4 = m_HwdID & 0xFF;

	memset(&m_Packet, 0, sizeof(tRBUF)); /* Prepare packet for LIGHTING2 relay status packet */

	if (m_HwdID > 0xFF)
	{
		id3 = (m_HwdID >> 8) & 0xFF;
	}

	if (m_HwdID > 0xFFFF)
	{
		id3 = (m_HwdID >> 16) & 0xFF;
	}

	m_Packet.LIGHTING2.packetlength = sizeof(m_Packet.LIGHTING2) - 1;
	m_Packet.LIGHTING2.packettype = pTypeLighting2;
	m_Packet.LIGHTING2.subtype = sTypeAC;
	m_Packet.LIGHTING2.unitcode = 0;
	m_Packet.LIGHTING2.id1 = id1;
	m_Packet.LIGHTING2.id2 = id2;
	m_Packet.LIGHTING2.id3 = id3;
	m_Packet.LIGHTING2.id4 = id4;
	m_Packet.LIGHTING2.cmnd = 0;
	m_Packet.LIGHTING2.level = 0;
	m_Packet.LIGHTING2.filler = 0;
	m_Packet.LIGHTING2.rssi = 12;

	FindGpioExports();
	CreateDomoticzDevices();
	UpdateDomoticzInputs(false);
}

void CSysfsGPIO::FindGpioExports()
{
	GpioSavedState.clear();

	for (int gpio_pin = GPIO_PIN_MIN; gpio_pin <= GPIO_PIN_MAX; gpio_pin++)
	{
		int fd;
		char path[GPIO_MAX_PATH];

		snprintf(path, GPIO_MAX_PATH, "%s%d", GPIO_PATH, gpio_pin);
		fd = open(path, O_RDONLY);

		if (-1 != fd) /* GPIO export found */
		{
			gpio_info gi;
			gi.pin_number = gpio_pin;
			gi.value = GPIORead(gpio_pin, "value");
			gi.direction = GPIORead(gpio_pin, "direction");
			gi.active_low = GPIORead(gpio_pin, "active_low");
			gi.edge = GPIORead(gpio_pin, "edge");
			gi.read_value_fd = -1;
			GpioSavedState.push_back(gi);

			close(fd);
		}
	}

	int pin_count = GpioSavedState.size();
}

void CSysfsGPIO::PollGpioInputs()
{
	if (GpioSavedState.size())
	{
		for (int i = 0; i < GpioSavedState.size(); i++)
		{
			if ((GpioSavedState[i].direction == GPIO_IN) && (GpioSavedState[i].read_value_fd != -1))
			{
				GpioSavedState[i].value = GPIOReadFd(GpioSavedState[i].read_value_fd);
			}
		}
	}
}

void CSysfsGPIO::CreateDomoticzDevices()
{
	std::vector<std::vector<std::string> > result;
	char szIdx[10];

	sprintf(szIdx, "%X%02X%02X%02X",
		m_Packet.LIGHTING2.id1,
		m_Packet.LIGHTING2.id2,
		m_Packet.LIGHTING2.id3,
		m_Packet.LIGHTING2.id4);

	for (int i = 0; i < GpioSavedState.size(); i++)
	{
		bool  createNewDevice = false;

		if (GpioSavedState[i].direction == GPIO_IN)
		{
			/* Inputs */
			result = m_sql.safe_query("SELECT Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
				m_HwdID, szIdx, GpioSavedState[i].pin_number); 

			if (result.empty())
			{
				createNewDevice = true;
			}
			else
			{
				if (result.size() > 0) /* input found */
				{
					std::vector<std::string> sd = result[0];

					if (atoi(sd[0].c_str()) != 0) /* delete if previous db device was an output */
					{
						m_sql.safe_query(
							"DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
							m_HwdID, szIdx, GpioSavedState[i].pin_number);

						createNewDevice = true;
					}
				}
			}

			if (createNewDevice)
			{
				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
					"VALUES (%d,'%q',%d, %d, %d, %d, 0, 12, 255, '%q', %d, ' ', '0')",
					m_HwdID, szIdx, GpioSavedState[i].pin_number, pTypeLighting2, sTypeAC, int(STYPE_Contact), "Input", GpioSavedState[i].value);
			}
		}
		else
		{	
			/* Outputs */
			result = m_sql.safe_query("SELECT nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
				m_HwdID, szIdx, GpioSavedState[i].pin_number);

			if (result.empty())
			{
				createNewDevice = true;
			}
			else
			{
				if (result.size() > 0) /* output found */
				{
					std::vector<std::string> sd = result[0];

					if (atoi(sd[1].c_str()) != 1) /* delete if previous db device was an input */
					{
						m_sql.safe_query(
							"DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
							m_HwdID, szIdx, GpioSavedState[i].pin_number);

						createNewDevice = true;
					}
					else
					{
						GPIOWrite(GpioSavedState[i].pin_number, atoi(sd[0].c_str()));
					}
				}
			}

			if (createNewDevice)
			{
				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
					"VALUES (%d,'%q',%d, %d, %d, %d, 0, 12, 255, '%q', %d, ' ', '1')",
					m_HwdID, szIdx, GpioSavedState[i].pin_number, pTypeLighting2, sTypeAC, int(STYPE_OnOff), "Output", GpioSavedState[i].value);
			}
		}
	}
}

void CSysfsGPIO::UpdateDomoticzInputs(bool forceUpdate)
{
	std::vector<std::vector<std::string> > result;
	char szIdx[10];

	sprintf(szIdx, "%X%02X%02X%02X",
		m_Packet.LIGHTING2.id1,
		m_Packet.LIGHTING2.id2,
		m_Packet.LIGHTING2.id3,
		m_Packet.LIGHTING2.id4);

	for (int i = 0; i < GpioSavedState.size(); i++)
	{
		if ((GpioSavedState[i].direction == GPIO_IN) && (GpioSavedState[i].value != -1))
		{
			bool	updateDatabase = false;
			int		state = 0;

			if (GpioSavedState[i].active_low)
			{
				if (GpioSavedState[i].value == 0)
				{
					state = 1;
				}
			}
			else
			{
				if (GpioSavedState[i].value == 1)
				{
					state = 1;
				}
			}

			if ((GpioSavedState[i].db_state != state) || (forceUpdate))
			{
				result = m_sql.safe_query("SELECT nValue,Used FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d)",
					m_HwdID, szIdx, GpioSavedState[i].pin_number);

				if ((!result.empty()) && (result.size() > 0))
				{
					std::vector<std::string> sd = result[0];

					if (atoi(sd[1].c_str()) == 1) /* Check if device is used */
					{
						int db_state = 1;

						if (atoi(sd[0].c_str()) == 0) /* determine database state*/
						{
							db_state = 0;
						}

						if ((db_state != state) || (forceUpdate)) /* check if db update is required */
						{
							updateDatabase = true;
						}

						GpioSavedState[i].db_state = state; /* save new database state */
					}
				}

				if (updateDatabase) /* send packet to Domoticz */
				{
					if (state)
					{
						m_Packet.LIGHTING2.cmnd = light2_sOn;
						m_Packet.LIGHTING2.level = 100;
					}
					else
					{
						m_Packet.LIGHTING2.cmnd = light2_sOff;
						m_Packet.LIGHTING2.level = 0;
					}

					m_Packet.LIGHTING2.unitcode = (char)GpioSavedState[i].pin_number;
					m_Packet.LIGHTING2.seqnbr++;

					sDecodeRXMessage(this, (const unsigned char *)&m_Packet.LIGHTING2, "Input", 255);
				}
			}
		}
	}
}

int CSysfsGPIO::GetReadResult(int bytecount, char* value_str)
{
	int retval = -1;

	switch (bytecount)
	{
		case -1:
		case  0:
		case  1:
		{
			break;
		}

		default:
		{
			if ((memcmp("0", value_str, strlen("0")) == 0) || 
				(memcmp("in", value_str, strlen("in")) == 0) ||
				(memcmp("none", value_str, strlen("none")) == 0))
			{
				retval = 0;
				break;
			}

			if ((memcmp("1", value_str, strlen("1")) == 0) ||
				(memcmp("out", value_str, strlen("out")) == 0) ||
				(memcmp("rising", value_str, strlen("rising")) == 0))
			{
				retval = 1;
				break;
			}

			if (memcmp("falling", value_str, strlen("falling")) == 0)
			{
				retval = 2;
				break;
			}

			if (memcmp("both", value_str, strlen("both")) == 0)
			{
				retval = 3;
				break;
			}
		}
	}

	return (retval);
}

int CSysfsGPIO::GPIORead(int gpio_pin, const char *param)
{
	char path[GPIO_MAX_PATH];
	char value_str[GPIO_MAX_VALUE_SIZE];
	int fd;
	int bytecount = -1;
	int retval = -1;

	snprintf(path, GPIO_MAX_PATH, "%s%d/%s", GPIO_PATH, gpio_pin, param);
	fd = open(path, O_RDONLY);

	if (fd == -1)
	{
		return(-1);
	}

	bytecount = read(fd, value_str, GPIO_MAX_VALUE_SIZE);
	close(fd);

	if (-1 == bytecount)
	{
		close(fd);
		return(-1);
	}

	return(GetReadResult(bytecount, &value_str[0]));
}

int CSysfsGPIO::GPIOReadFd(int fd)
{
	int bytecount = -1;
	int retval = -1;
	char value_str[GPIO_MAX_VALUE_SIZE];

	if (fd == -1)
	{
		return(-1);
	}

	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		return (-1);
	}

	bytecount = read(fd, value_str, GPIO_MAX_VALUE_SIZE);

	return(GetReadResult(bytecount, &value_str[0]));
}

int CSysfsGPIO::GPIOWrite(int gpio_pin, int value)
{
	char path[GPIO_MAX_PATH];
	int fd;

	snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, gpio_pin);
	fd = open(path, O_WRONLY);

	if (fd == -1)
	{
		return(-1);
	}

	if (1 != write(fd, value ? "1" : "0", 1))
	{
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

#endif // WITH_SYSFS_GPIO
