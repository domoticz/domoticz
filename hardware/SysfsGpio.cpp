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
	interrupt generating input pin.SysfsGpio

	"active_low" ... reads as either 0 (false) or 1 (true). Write
	any nonzero value to invert the value attribute both
	for reading and writing. Existing and subsequent
	poll(2) support configuration via the edge attribute
	for "rising" and "falling" edges will follow this
	setting.

	History:
	03-jun-2017	HvB		Add interrupt support for edge = rising, falling or both.
	24-jun-2017	HvB		Add hardware debounce parameter, range 10..750 milli sec.
	04-jul-2017	HvB		Poll after an interrupt received to recover missed interrupts
	06-jul-2017 HvB		Removed log message for interrupt state change, forum request
*/

#include "stdafx.h"

#ifdef WITH_GPIO

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "SysfsGpio.h"
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
#define DB_UPDATE_DELAY		4
#define GPIO_POLL_MSEC		250

#define GPIO_IN				0
#define GPIO_OUT			1
#define GPIO_LOW			0
#define GPIO_HIGH			1
#define GPIO_EDGE_NONE		0
#define GPIO_EDGE_RISING	1
#define GPIO_EDGE_FALLING	2
#define GPIO_EDGE_BOTH		3
#define GPIO_EDGE_UNKNOWN	-1
#define GPIO_PIN_MIN		0
#define GPIO_PIN_MAX		1023
#define GPIO_MAX_VALUE_SIZE	16
#define GPIO_MAX_PATH		64
#define GPIO_PATH			"/sys/class/gpio/gpio"
#define GPIO_DEVICE_ID_BASE	0x030E0E00
using namespace std;

vector<gpio_info> CSysfsGpio::m_saved_state;
int CSysfsGpio::m_sysfs_hwdid;
int CSysfsGpio::m_sysfs_req_update;

CSysfsGpio::CSysfsGpio(const int ID, const int AutoConfigureDevices, const int Debounce)
{
	m_stoprequested = false;
	m_bIsStarted = false;
	m_polling_enabled = false;
	m_interrupts_enabled = false;
	m_HwdID = ID;
	m_sysfs_hwdid = ID;
	m_auto_configure_devices = AutoConfigureDevices;
	m_debounce_msec = Debounce;
}

CSysfsGpio::~CSysfsGpio(void)
{
}

bool CSysfsGpio::StartHardware()
{
	Init();

	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSysfsGpio::Do_Work, this)));
	m_bIsStarted = true;

	return (m_thread != NULL);
}

bool CSysfsGpio::StopHardware()
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
		_log.Log(LOG_STATUS, "Sysfs GPIO: Worker - error during rundown");
	}

	try
	{
		if (m_edge_thread)
		{
			m_edge_thread->join();
		}
	}
	catch (...)
	{
		_log.Log(LOG_STATUS, "Sysfs GPIO: Edge detection - error during rundown");
	}

	m_bIsStarted = false;

	return true;
}

bool CSysfsGpio::WriteToHardware(const char *pdata, const unsigned char length)
{
	bool bOk = false;
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	unsigned char packettype = pSen->ICMND.packettype;
	int gpio_pin = pSen->LIGHTING2.unitcode;

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if ((m_saved_state[i].pin_number == gpio_pin) &&
			(m_saved_state[i].direction == GPIO_OUT) &&
			(packettype == pTypeLighting2))
		{
			int state = pSen->LIGHTING2.cmnd == light2_sOn ? GPIO_HIGH : GPIO_LOW;
			GpioWrite(gpio_pin, state);
			m_saved_state[i].db_state = state;
			m_saved_state[i].value = state;
			bOk = true;
			break;
		}
	}
	return bOk;
}

void CSysfsGpio::Do_Work()
{
	bool bUpdateMaster = true;
	int counter = 0;

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
			if (m_polling_enabled)
			{
				PollGpioInputs(false);
				UpdateDomoticzInputs(false);
			}
		}

		if (bUpdateMaster)
		{
			if (counter == UPDATE_MASTER_COUNT)	/* only executes once, and only if we have a master/slave configuration */
			{
				bUpdateMaster = false;
				vector<vector<string> > result;
				result = m_sql.safe_query("SELECT ID FROM Users WHERE (RemoteSharing==1) AND (Active==1)");

				if (result.size() > 0)
				{
					_log.Log(LOG_STATUS, "Sysfs GPIO: Update master devices");
					UpdateDomoticzInputs(true);
				}
			}
		}

		if (m_sysfs_req_update) /* to assist manual device adding */
		{
			m_sysfs_req_update--;

			if (m_sysfs_req_update == 0)
			{
				UpdateDomoticzDatabase();
			}
		}
	}

	/* termination, close all open fd's */
	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if (m_saved_state[i].read_value_fd != -1)
		{
			close(m_saved_state[i].read_value_fd);
			m_saved_state[i].read_value_fd = -1;
		}
	}

	_log.Log(LOG_STATUS, "Sysfs GPIO: Worker stopped");
}

void CSysfsGpio::EdgeDetectThread()
{
	/*
	Notes:
	Changes in line state are signaled as an exception condition rather than a write. Not all
	GPIOs are interrupt-capable. If the GPIO you're working with doesn't support interrupts
	you will need to read it periodically. This is for example the case when the GPIO line is
	implemented with an I2C peripheral or audio codec.

	After one or more GPIO state change interrupts have been detected a poll is done to make
	sure the domoticz database reflects the actual states of all GPIO pins in all cases. A
	missed interrupt can occur when a GPIO pin changes state twice within m_debounce_msec. 
	Therefore it is a good practice to set m_debounce_msec to a value not higher then needed
	depending on the input switch behavior.
	*/

	ssize_t nb;
	fd_set tmp_fds;
	timeval tv;
	int fd;
	bool poll_once = false;

	FD_ZERO(&m_rfds);
	m_maxfd = 0;

	for (int i = 0; i < m_saved_state.size(); i++) /* setup gpio pins */
	{
		if ((m_saved_state[i].direction == GPIO_IN) && (m_saved_state[i].edge != GPIO_EDGE_NONE))
		{
			fd = GpioOpenRw(m_saved_state[i].pin_number);
			m_saved_state[i].edge_fd = fd; /* store file descriptor, closed after stoprequested */

			if ((fd != -1) && (fd <= FD_SETSIZE))
			{
				FD_SET(fd, &m_rfds);
				m_maxfd = (m_maxfd > fd ? m_maxfd : fd);
				GpioWrite(m_saved_state[i].pin_number, 1); /* enable interrupts */
			}
		}
		else
		{
			m_saved_state[i].edge_fd = -1;
		}
	}

	_log.Log(LOG_STATUS, "Sysfs GPIO: Edge detection started");

	while (!m_stoprequested) /* detect gpio state changes */
	{
		tv.tv_sec = 1;	// Set select timeout to 1 second.
		tv.tv_usec = 0;
		memcpy(&tmp_fds, &m_rfds, sizeof(tmp_fds));

		//-------------------------------------------------------------------
		//	Wait for GPIO interrupt.
		//
		int retval = select(m_maxfd + 1, NULL, NULL, &tmp_fds, &tv);

		if (m_stoprequested)
		{
			break;
		}

		if (retval > 0)
		{
			//---------------------------------------------------------------
			//	GPIO interrupt received.
			//
			sleep_milliseconds(m_debounce_msec); /* debounce */

			for (int i = 0; i < m_saved_state.size(); i++)
			{
				if ((FD_ISSET(m_saved_state[i].edge_fd, &tmp_fds) &&
					(m_saved_state[i].direction == GPIO_IN) &&
					(m_saved_state[i].edge != GPIO_EDGE_NONE)))
				{
					int value = GpioReadFd(m_saved_state[i].edge_fd);
					GpioSaveState(i, value);
					FD_CLR(m_saved_state[i].edge_fd, &tmp_fds);
					GpioWrite(m_saved_state[i].edge_fd, 1);
				}
			}

			if (!m_polling_enabled)
			{
				UpdateDomoticzInputs(false);
				poll_once = true;
			}
		}
		else
		{
			//---------------------------------------------------------------
			//	Select timeout, no GPIO interrupt received.
			//
			if (poll_once)
			{
				PollGpioInputs(true);
				UpdateDomoticzInputs(false);
				poll_once = false;
			}
		}
	}

	for (int i = 0; i < m_saved_state.size(); i++) /* close all open edge detect fd's */
	{
		if (m_saved_state[i].edge_fd > 0)
		{
			close(m_saved_state[i].edge_fd);
			m_saved_state[i].edge_fd = -1;
		}
	}

	_log.Log(LOG_STATUS, "Sysfs GPIO: Edge detection stopped");
}

void CSysfsGpio::Init()
{
	int id = GPIO_DEVICE_ID_BASE + m_HwdID;
	int input_count = 0;
	int output_count = 0;
	char path[GPIO_MAX_PATH];
	m_polling_enabled = false;
	m_interrupts_enabled = false;
	m_sysfs_req_update = 0;

	m_saved_state.clear();

	/* default ligthing2 packet */
	memset(&m_Packet, 0, sizeof(tRBUF));
	m_Packet.LIGHTING2.packetlength = sizeof(m_Packet.LIGHTING2) - 1;
	m_Packet.LIGHTING2.packettype = pTypeLighting2;
	m_Packet.LIGHTING2.subtype = sTypeAC;
	m_Packet.LIGHTING2.unitcode = 0;
	m_Packet.LIGHTING2.id1 = (id >> 24) & 0xFF;
	m_Packet.LIGHTING2.id2 = (id >> 16) & 0xFF;
	m_Packet.LIGHTING2.id3 = (id >> 8)  & 0xFF;
	m_Packet.LIGHTING2.id4 = (id >> 0)  & 0xFF;
	m_Packet.LIGHTING2.cmnd = 0;
	m_Packet.LIGHTING2.level = 0;
	m_Packet.LIGHTING2.filler = 0;
	m_Packet.LIGHTING2.rssi = 12;

	FindGpioExports();

	if (m_auto_configure_devices)
	{
		CreateDomoticzDevices();
	}

	UpdateGpioOutputs();

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, m_saved_state[i].pin_number);
		m_saved_state[i].read_value_fd = open(path, O_RDONLY);
		if ((m_saved_state[i].direction == GPIO_IN) ? input_count++ : output_count++);

		/*	Enable polling if at least one input edge is set to NONE or is INVALID. */
		if	(!m_polling_enabled &&
			(m_saved_state[i].direction == GPIO_IN) &&
			((m_saved_state[i].edge == GPIO_EDGE_NONE) || (m_saved_state[i].edge == GPIO_EDGE_UNKNOWN)))
		{
			m_polling_enabled = true;
		}

		/*	Enable interrupts if at least one input edge is set to RISING FALLING or BOTH. */
		if	(!m_interrupts_enabled &&
			(m_saved_state[i].direction == GPIO_IN) &&
			((m_saved_state[i].edge == GPIO_EDGE_RISING) ||
			(m_saved_state[i].edge == GPIO_EDGE_FALLING) ||
			(m_saved_state[i].edge == GPIO_EDGE_BOTH)))
		{
			m_interrupts_enabled = true;
		}
	}

	UpdateDomoticzInputs(false); /* Make sure database inputs are in sync with actual hardware */

	_log.Log(LOG_STATUS, "Sysfs GPIO: Startup - polling:%s interrupts:%s debounce:%dmsec inputs:%d outputs:%d",
		m_polling_enabled ? "yes":"no", 
		m_interrupts_enabled ? "yes":"no", 
		m_debounce_msec, 
		input_count, 
		output_count);

	if (m_interrupts_enabled)
	{
		m_edge_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CSysfsGpio::EdgeDetectThread, this)));
	}
}

void CSysfsGpio::FindGpioExports()
{
	m_saved_state.clear();

	for (int gpio_pin = GPIO_PIN_MIN; gpio_pin <= GPIO_PIN_MAX; gpio_pin++)
	{
		char path[GPIO_MAX_PATH];
		int fd;

		snprintf(path, GPIO_MAX_PATH, "%s%d", GPIO_PATH, gpio_pin);
		fd = open(path, O_RDONLY);

		if (-1 != fd) /* GPIO export found */
		{
			gpio_info gi;
			memset(&gi, 0, sizeof(gi));

			gi.pin_number = gpio_pin;
			gi.value = GpioRead(gpio_pin, "value");
			gi.direction = GpioRead(gpio_pin, "direction");
			gi.active_low = GpioRead(gpio_pin, "active_low");
			gi.edge = GpioRead(gpio_pin, "edge");
			gi.read_value_fd = -1;
			gi.db_state = -1;
			gi.id_valid = -1;
			gi.id1 = 0;
			gi.id2 = 0;
			gi.id3 = 0;
			gi.id4 = 0;
			gi.request_update = -1;

			m_saved_state.push_back(gi);
			close(fd);
		}
	}
}

void CSysfsGpio::PollGpioInputs(bool PollOnce)
{
	if (m_saved_state.size())
	{
		for (int i = 0; i < m_saved_state.size(); i++)
		{
			if ((m_saved_state[i].direction == GPIO_IN) &&
				(m_saved_state[i].read_value_fd != -1) &&
				(PollOnce || (m_saved_state[i].edge == GPIO_EDGE_NONE) || (m_saved_state[i].edge == GPIO_EDGE_UNKNOWN)))
			{
				GpioSaveState(i, GpioReadFd(m_saved_state[i].read_value_fd));
			}
		}
	}
}

void CSysfsGpio::CreateDomoticzDevices()
{
	vector<vector<string> > result;
	vector<string> deviceid;

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		bool  createNewDevice = false;
		deviceid = GetGpioDeviceId();

		if (m_saved_state[i].direction == GPIO_IN)
		{
			/* Input */

			result = m_sql.safe_query("SELECT nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID,
				m_saved_state[i].pin_number);

			if (result.empty())
			{
				createNewDevice = true;
			}
			else
			{
				if (result.size() > 0) /* found */
				{
					vector<string> sd = result[0];

					if (sd[1].empty())
					{
						/* update manual created device */
						m_saved_state[i].request_update = 1;
						UpdateDomoticzDatabase();
					}
					else
					{
						if (atoi(sd[1].c_str()) != GPIO_IN) /* device was not an input, delete it */
						{
							m_sql.safe_query(
								"DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
								m_HwdID, m_saved_state[i].pin_number);

							createNewDevice = true;
						}
					}
				}
			}

			if (createNewDevice)
			{
				/* create new input device */
				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
					"VALUES (%d,'%q',%d, %d, %d, %d, 0, 12, 255, '%q', %d, ' ', '0')",
					m_HwdID, deviceid[0].c_str(), m_saved_state[i].pin_number, pTypeLighting2, sTypeAC, int(STYPE_Contact), "Input", m_saved_state[i].value);
			}
		}
		else
		{
			/* Output */

			result = m_sql.safe_query("SELECT nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID,
				m_saved_state[i].pin_number);

			if (result.empty())
			{
				createNewDevice = true;
			}
			else
			{
				if (result.size() > 0) /* found */
				{
					vector<string> sd = result[0];

					if (sd[1].empty())
					{
						/* update manual created output device */
						m_saved_state[i].request_update = 1;
						UpdateDomoticzDatabase();
					}
					else
					{
						if ((atoi(sd[1].c_str()) != GPIO_OUT)) /* device was not an output, delete it */
						{
							m_sql.safe_query(
								"DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
								m_HwdID, m_saved_state[i].pin_number);

							createNewDevice = true;
						}
						else
						{
							GpioWrite(m_saved_state[i].pin_number, atoi(sd[0].c_str()));
						}
					}
				}
			}

			if (createNewDevice)
			{
				/* create new output device */
				m_sql.safe_query(
					"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
					"VALUES (%d,'%q',%d, %d, %d, %d, 0, 12, 255, '%q', %d, ' ', '1')",
					m_HwdID, deviceid[0].c_str(), m_saved_state[i].pin_number, pTypeLighting2, sTypeAC, int(STYPE_OnOff), "Output", m_saved_state[i].value);
			}
		}
	}
}

void CSysfsGpio::UpdateDomoticzDatabase()
{
	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if (m_saved_state[i].request_update == 1)
		{
			vector<vector<string> > result = m_sql.safe_query("SELECT nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID,
				m_saved_state[i].pin_number);

			if (!result.empty())
			{
				//vector<string> sd = result[0];
				m_saved_state[i].value = GpioRead(m_saved_state[i].pin_number, "value");

				m_sql.safe_query(
					"UPDATE DeviceStatus SET nValue=%d,Options=%d WHERE (HardwareID==%d) AND (Unit==%d)",
					m_saved_state[i].value, m_saved_state[i].direction, m_HwdID, m_saved_state[i].pin_number);

				m_saved_state[i].db_state = m_saved_state[i].value;
				m_saved_state[i].id_valid = -1;
			}

			m_saved_state[i].request_update = -1;
		}
	}
}

void CSysfsGpio::UpdateDomoticzInputs(bool forceUpdate)
{
	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if ((m_saved_state[i].direction == GPIO_IN) && (m_saved_state[i].value != -1))
		{
			int		state = GPIO_LOW;

			if (m_saved_state[i].active_low)
			{
				if (m_saved_state[i].value == GPIO_LOW)
				{
					state = GPIO_HIGH;
				}
			}
			else
			{
				if (m_saved_state[i].value == GPIO_HIGH)
				{
					state = GPIO_HIGH;
				}
			}

			if ((m_saved_state[i].db_state != state) || (forceUpdate))
			{
				bool	updateDatabase = false;
				bool	log_db_change = false;
				vector< vector<string> > result = m_sql.safe_query("SELECT nValue,Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
					m_HwdID,
					m_saved_state[i].pin_number);

				if ((!result.empty()) && (result.size() > 0))
				{
					vector<string> sd = result[0];

					if (atoi(sd[1].c_str()) == 1) /* Check if device is used */
					{
						int db_state = GPIO_HIGH;

						if (atoi(sd[0].c_str()) == GPIO_LOW) /* determine database state*/
						{
							db_state = GPIO_LOW;
						}

						if ((db_state != state) || (forceUpdate)) /* check if db update is required */
						{
							if (db_state != state)
							{
								log_db_change = true;
							}
							updateDatabase = true;
						}

						m_saved_state[i].db_state = state; /* save new database state */
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

					UpdateDeviceID(m_saved_state[i].pin_number);
					m_Packet.LIGHTING2.unitcode = (char)m_saved_state[i].pin_number;
					m_Packet.LIGHTING2.seqnbr++;
					sDecodeRXMessage(this, (const unsigned char *)&m_Packet.LIGHTING2, "Input", 255);

					if (log_db_change)
					{
						_log.Log(LOG_STATUS, "Sysfs GPIO: gpio%d new state = %s",
							m_saved_state[i].pin_number,
							state ? "on" : "off");
					}
				}
			}
		}
	}
}

void CSysfsGpio::UpdateDeviceID(int pin)
{
	int index;
	bool pin_found = false;

	/* Note: Support each pin is allowed to have a different device id */

	for (index = 0; index < m_saved_state.size(); index++)
	{
		if (m_saved_state[index].pin_number == pin)
		{
			pin_found = true;
			break;
		}
	}

	if (pin_found)
	{
		/* Existing pin was found */

		if (m_saved_state[index].id_valid == -1)
		{
			string sdeviceid;
			vector< vector<string> > result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID,
				pin);

			if ((!result.empty() && (result.size() > 0)))
			{
				/* use database device id */
				vector<string> sd = result[0];
				sdeviceid = sd[0];
			}
			else
			{
				/* use generated device id */
				vector<string> deviceid = GetGpioDeviceId();
				sdeviceid = deviceid[0];
			}

			/* extract hex device sub-id's */
			m_saved_state[index].id1 = strtol(sdeviceid.substr(0, 1).c_str(), NULL, 16) & 0xFF;
			m_saved_state[index].id2 = strtol(sdeviceid.substr(1, 2).c_str(), NULL, 16) & 0xFF;
			m_saved_state[index].id3 = strtol(sdeviceid.substr(3, 2).c_str(), NULL, 16) & 0xFF;
			m_saved_state[index].id4 = strtol(sdeviceid.substr(5, 2).c_str(), NULL, 16) & 0xFF;
			m_saved_state[index].id_valid = 1;
		}

		/* update device sub-id's in packet */
		m_Packet.LIGHTING2.id1 = m_saved_state[index].id1;
		m_Packet.LIGHTING2.id2 = m_saved_state[index].id2;
		m_Packet.LIGHTING2.id3 = m_saved_state[index].id3;
		m_Packet.LIGHTING2.id4 = m_saved_state[index].id4;
	}
}

void CSysfsGpio::UpdateGpioOutputs()
{
	/* make sure actual gpio output values match database */

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if (m_saved_state[i].direction == GPIO_OUT)
		{
			vector<vector<string> > result = m_sql.safe_query("SELECT nValue,Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID,
				m_saved_state[i].pin_number);

			if ((!result.empty()) && (result.size() > 0))
			{
				vector<string> sd = result[0];
				m_saved_state[i].db_state = atoi(sd[0].c_str());

				if (atoi(sd[1].c_str()))
				{
					/* device is used, update gpio output pin */
					GpioWrite(m_saved_state[i].pin_number, m_saved_state[i].db_state);
					m_saved_state[i].value = m_saved_state[i].db_state;
				}
			}
		}
	}
}

vector<string> CSysfsGpio::GetGpioDeviceId()
{
	vector<string> gpio_deviceid;
	char szIdx[10];
	int id = GPIO_DEVICE_ID_BASE + m_sysfs_hwdid;

	snprintf(szIdx, sizeof(szIdx), "%7X", id);
	gpio_deviceid.push_back(szIdx);

	return gpio_deviceid;
}

//---------------------------------------------------------------------------
//	sysfs gpio helper functions
//
int CSysfsGpio::GetReadResult(int bytecount, char* value_str)
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

int CSysfsGpio::GpioRead(int gpio_pin, const char *param)
{
	char path[GPIO_MAX_PATH];
	char value_str[GPIO_MAX_VALUE_SIZE];
	int fd;
	int bytecount = -1;

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

int CSysfsGpio::GpioReadFd(int fd)
{
	int bytecount = -1;
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

int CSysfsGpio::GpioWrite(int gpio_pin, int value)
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

int CSysfsGpio::GpioOpenRw(int gpio_pin)
{
	char path[GPIO_MAX_PATH];

	snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, gpio_pin);
	int fd = open(path, O_RDWR);

	return(fd);
}

//---------------------------------------------------------------------------
//	Called by WebServer when devices are manually configured.
//
vector<int> CSysfsGpio::GetGpioIds()
{
	vector<int> gpio_ids;

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		vector<vector<string> > result = m_sql.safe_query("SELECT ID, Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
			m_sysfs_hwdid,
			m_saved_state[i].pin_number);

		if (result.empty())
		{
			/* add pin to the list only if it does not exist in the db */
			gpio_ids.push_back(m_saved_state[i].pin_number);
		}
	}

	return gpio_ids;
}

vector<string> CSysfsGpio::GetGpioNames()
{
	vector<string> gpio_names;

	for (int i = 0; i < m_saved_state.size(); i++)
	{
		vector<vector<string> > result = m_sql.safe_query("SELECT ID, Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
			m_sysfs_hwdid,
			m_saved_state[i].pin_number);

		if (result.empty())
		{
			/* add name to the list only if it does not exist in the db */
			char name[32];
			snprintf(name, sizeof(name), "gpio%d-%s", m_saved_state[i].pin_number, m_saved_state[i].direction ? "output" : "input");
			gpio_names.push_back(name);
		}
	}

	return gpio_names;
}

void CSysfsGpio::RequestDbUpdate(int pin)
{
	for (int i = 0; i < m_saved_state.size(); i++)
	{
		if (m_saved_state[i].pin_number == pin)
		{
			m_saved_state[i].request_update = 1;
		}
	}

	m_sysfs_req_update = DB_UPDATE_DELAY;
}

//---------------------------------------------------------------------------

int CSysfsGpio::GpioGetState(int index)
{
	m_state_mutex.lock();
	int value = m_saved_state[index].value;
	m_state_mutex.unlock();

	return value;
}

void CSysfsGpio::GpioSaveState(int index, int value)
{
	m_state_mutex.lock();
	m_saved_state[index].value = value;
	m_state_mutex.unlock();
}

//---------------------------------------------------------------------------

#endif // WITH_GPIO
