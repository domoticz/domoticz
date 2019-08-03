/*
This code implements basic I/O hardware via the Raspberry Pi's GPIO port, using the wiringpi library.
WiringPi is Copyright (c) 2012-2013 Gordon Henderson. <projects@drogon.net>
It must be installed beforehand following instructions at http://wiringpi.com/download-and-install/

	Copyright (C) 2014 Vicne <vicnevicne@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
	MA 02110-1301 USA.

This is a derivative work based on the samples included with wiringPi where distributed
under the GNU Lesser General Public License version 3
Source: http://wiringpi.com

Connection information:
	This hardware uses the pins of the Raspebrry Pi's GPIO port.
	Please read:
	http://wiringpi.com/pins/special-pin-functions/
	http://wiringpi.com/pins/ for more information
	As we cannot assume domoticz runs as root (which is bad), we won't take advantage of wiringPi's numbering.
	Consequently, we will always use the internal GPIO pin numbering as noted the board, including in the commands below

	Pins have to be exported and configured beforehand and upon each reboot of the Pi, like so:
	- For output pins, only one command is needed:
	gpio export <pin> out

	- For input pins, 2 commands are needed (one to export as input, and one to trigger interrupts on both edges):
	gpio export <pin> in
	gpio edge <pin> both

	Note: If you wire a pull-up, make sure you use 3.3V from P1-01, NOT the 5V pin ! The inputs are 3.3V max !
*/
#include "stdafx.h"
#ifndef WIN32
#ifdef WITH_GPIO
#include "Gpio.h"
#include "GpioPin.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "hardwaretypes.h"
#include "../main/RFXtrx.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/SQLHelper.h"
#include <sys/syscall.h>
#include <sys/resource.h>
#include <poll.h>
#include <sched.h>

#define NO_INTERRUPT		-1
#define DELAYED_STARTUP_SEC	30

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

// List of GPIO pin numbers, ordered as listed
std::vector<CGpioPin> CGpio::pins;

std::shared_ptr<std::thread> m_thread_interrupt[GPIO_PIN_MAX + 1];

/*
 * Direct GPIO implementation, inspired by other hardware implementations such as PiFace and EnOcean
 */
CGpio::CGpio(const int ID, const int debounce, const int period, const int pollinterval)
{
	m_HwdID = ID;
	m_debounce = debounce;
	m_period = period;
	m_pollinterval = pollinterval;

	//Prepare a generic packet info for LIGHTING1 packet, so we do not have to do it for every packet
	IOPinStatusPacket.LIGHTING1.packetlength = sizeof(IOPinStatusPacket.LIGHTING1) - 1;
	IOPinStatusPacket.LIGHTING1.housecode = 0;
	IOPinStatusPacket.LIGHTING1.packettype = pTypeLighting1;
	IOPinStatusPacket.LIGHTING1.subtype = sTypeIMPULS;
	IOPinStatusPacket.LIGHTING1.rssi = 12;
	IOPinStatusPacket.LIGHTING1.seqnbr = 0;
}

CGpio::~CGpio(void)
{
}

/*
 * interrupt handlers:
 *********************************************************************************
 */

void CGpio::InterruptHandler()
{
	char c;
	int pin, fd, count, counter = 0, sched, priority;
	uint32_t diff = 1;
	bool bRead;
	struct timeval tvBegin, tvEnd, tvDiff;

	// Higher priority and real-time scheduling is only effective if we run as root
	GetSchedPriority(&sched, &priority);
	SetSchedPriority(SCHED_RR, (sched == SCHED_RR ? priority : 1), (sched == SCHED_RR));

	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
		if (it->GetPin() == pinPass)
		{
			if ((fd = it->GetReadValueFd()) == -1)
			{
				_log.Log(LOG_STATUS, "GPIO: Could not open file descriptor for GPIO %d", pinPass);
				pinPass = -1;
				return;
			}
			break;
		}

	pin = pinPass;
	pinPass = -1;

	if (ioctl(fd, FIONREAD, &count) != -1)
		for (int i = 0; i < count; i++) // Clear any initial pending interrupt
			bRead = read(fd, &c, 1); // Catch value to suppress compiler unused warning

	_log.Log(LOG_STATUS, "GPIO: Interrupt handler for GPIO %d started (TID: %d)", pin, (pid_t)syscall(SYS_gettid));
	while (!IsStopRequested(0))
	{
		if (waitForInterrupt(fd, 2000) > 0)
		{
			if (counter > 100)
			{
				_log.Log(LOG_STATUS, "GPIO: Suppressing interruptstorm on GPIO %d, sleeping for 2 seconds..", pin);
				counter = -1;
				sleep_milliseconds(2000);
			}
			if (m_period > 0)
			{
				getclock(&tvEnd);
				if (timeval_subtract(&tvDiff, &tvEnd, &tvBegin)) {
					tvDiff.tv_sec = 0;
					tvDiff.tv_usec = 0;
				}
				diff = tvDiff.tv_usec + tvDiff.tv_sec * 1000000;
				getclock(&tvBegin);
			}
			if (diff > m_period * 1000 && counter != -1)
			{
				_log.Log(LOG_NORM, "GPIO: Processing interrupt for GPIO %d...", pin);
				if (m_debounce > 0)
					sleep_milliseconds(m_debounce); // Debounce reading
				UpdateSwitch(pin, GPIOReadFd(fd));
				_log.Log(LOG_NORM, "GPIO: Done processing interrupt for GPIO %d.", pin);

				if (counter > 0)
				{
					//_log.Log(LOG_STATUS, "GPIO: Suppressed %d interrupts on previous call for GPIO %d.", counter, pin);
					counter = 0;
				}
			}
			else
				counter++;
		}
		else
			if (fd != -1)
				close(fd);
	}
	_log.Log(LOG_STATUS, "GPIO: Interrupt handler for GPIO %d stopped. TID: %d", pin, (pid_t)syscall(SYS_gettid));
	return;
}

int CGpio::waitForInterrupt(int fd, const int mS)
{
	bool bRead;
	int x = 0;
	uint8_t c;
	struct pollfd polls;

	if (fd == -1)
		return -2;

	// Setup poll structure
	polls.fd = fd;
	polls.events = POLLPRI;
	polls.revents = POLLERR | POLLNVAL;

	// Wait for it ...
	while (x == 0)
	{
		x = poll(&polls, 1, mS);
		if (IsStopRequested(0))
			return -1;
	}

	// If no error, do a dummy read to clear the interrupt
	//	A one character read appears to be enough.
	if (x > 0)
	{
		lseek(fd, 0, SEEK_SET);	// Rewind
		bRead = read(fd, &c, 1); // Read & clear
	}
	return x;
}

void CGpio::UpdateSwitch(const int pin, const bool value)
{
	value ? IOPinStatusPacket.LIGHTING1.cmnd = light1_sOn : IOPinStatusPacket.LIGHTING1.cmnd = light1_sOff;
	IOPinStatusPacket.LIGHTING1.seqnbr++;
	IOPinStatusPacket.LIGHTING1.unitcode = pin;

	sDecodeRXMessage(this, (const unsigned char *)&IOPinStatusPacket, NULL, 255);

	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
	{
		if (it->GetPin() == pin)
		{
			it->SetDBState(value);
			break;
		}
	}
}

bool CGpio::StartHardware()
{
	RequestStart();

	//	_log.Log(LOG_NORM,"GPIO: Starting hardware (debounce: %d ms, period: %d ms, poll interval: %d sec)", m_debounce, m_period, m_pollinterval);

	_log.Log(LOG_STATUS, "GPIO: This hardware is deprecated. Please transfer to the new SysFS hardware type!");

	if (InitPins())
	{
		/* Disabled for now, devices should be added manually (this was the old behaviour, which we'll follow for now). Keep code for possible future usage.
		 if (!CreateDomoticzDevices())
		 {
			_log.Log(LOG_NORM, "GPIO: Error creating pins in DB, aborting...");
			RequestStop();
		 }*/
		m_thread_updatestartup = std::make_shared<std::thread>(&CGpio::UpdateStartup, this);
		SetThreadName(m_thread_updatestartup->native_handle(), "GPIO_UpdStartup");

		if (m_pollinterval > 0)
		{
			m_thread_poller = std::make_shared<std::thread>(&CGpio::Poller, this);
			SetThreadName(m_thread_poller->native_handle(), "GPIO_Poller");
		}
	}
	else
	{
		_log.Log(LOG_NORM, "GPIO: No exported pins found, aborting...");
		RequestStop();
	}
	m_bIsStarted = true;
	sOnConnected(this);
	StartHeartbeatThread();
	return (m_thread != nullptr);
}

bool CGpio::StopHardware()
{
	RequestStop();

	try
	{
		if (m_thread_updatestartup)
		{
			m_thread_updatestartup->join();
			m_thread_updatestartup.reset();
		}
	}
	catch (...)
	{
	}
	try
	{
		if (m_thread_poller)
		{
			m_thread_poller->join();
			m_thread_poller.reset();
		}
	}
	catch (...)
	{
	}


	std::unique_lock<std::mutex> lock(m_pins_mutex);
	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
	{
		if (m_thread_interrupt[it->GetPin()] != NULL)
		{
			m_thread_interrupt[it->GetPin()]->join();
			m_thread_interrupt[it->GetPin()].reset();
		}
	}

	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
		if (it->GetReadValueFd() != -1)
			close(it->GetReadValueFd());

	pins.clear();
	m_bIsStarted = false;
	StopHeartbeatThread();
	_log.Log(LOG_NORM, "GPIO: Hardware stopped");
	return true;
}

bool CGpio::WriteToHardware(const char *pdata, const unsigned char length)
{
	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	int pin = pSen->LIGHTING1.unitcode;
	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
	{
		if (it->GetPin() == pin && !it->GetIsInput())
		{
			unsigned char packettype = pSen->ICMND.packettype;

			if (packettype == pTypeLighting1)
			{
				GPIOWrite(pin, (pSen->LIGHTING1.cmnd == light1_sOn));
				return true;
			}
		}
	}
	return false;
}

void CGpio::UpdateStartup()
{
	//  Read all exported GPIO ports and set the device status accordingly.
	//  No need for delayed startup and force update when no masters are able to connect.
	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT ID FROM Users WHERE (RemoteSharing==1) AND (Active==1)");
	if (!result.empty())
	{
		for (int i = 0; i < DELAYED_STARTUP_SEC; ++i)
		{
			if (IsStopRequested(1000))
				return;
		}
		_log.Log(LOG_NORM, "GPIO: Optional connected Master Domoticz now updates its status");
		UpdateDeviceStates(true);
	}
	else
		UpdateDeviceStates(false);
}

void CGpio::Poller()
{
	//
	//	This code has been added for robustness like for example when gpio is
	//	used in alarm systems. In case a state change event (interrupt) is
	//	missed this code will make up for it.
	//
	int sec_counter = 0;

	_log.Log(LOG_STATUS, "GPIO: Poller started (interval: %d sec, TID: %d)", m_pollinterval, (pid_t)syscall(SYS_gettid));
	while (!IsStopRequested(1000))
	{
		sec_counter++;
		if (sec_counter % m_pollinterval == 0)
			UpdateDeviceStates(false);
	}
	_log.Log(LOG_STATUS, "GPIO: Poller stopped. TID: %d", (pid_t)syscall(SYS_gettid));
}

/* Disabled for now, devices should be added manually (this was the old behaviour, which we'll follow for now). Keep code for possible future usage.
bool CGpio::CreateDomoticzDevices()
{
	std::vector<std::vector<std::string> > result;
	std::unique_lock<std::mutex> lock(m_pins_mutex);
	for(std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
	{
		bool createNewDevice = false;
		result = m_sql.safe_query("SELECT Name,nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", m_HwdID, it->GetPin());
		if (result.empty())
			createNewDevice = true;
		else
		{
			if (!result.empty()) // input found
			{
				std::vector<std::string> sd = result[0];
				bool bIsInput = (atoi(sd[2].c_str()) == 0);
				if ((!bIsInput && it->GetIsInput()) ||
					(bIsInput && !it->GetIsInput()))
				{
					m_sql.safe_query(
						"DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
						m_HwdID, it->GetPin());
					createNewDevice = true;
				}
				else if (!it->GetIsInput()) // write actual db state to hardware in case of output
					(atoi(sd[1].c_str()) == 1) ? GPIOWrite(it->GetPin(), !it->GetActiveLow()) : GPIOWrite(it->GetPin(), it->GetActiveLow());
			}
		}
		if (createNewDevice)
			m_sql.safe_query(
				"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
				"VALUES (%d, 0, %d, %d, %d, %d, 0, 12, 255, '%q', %d, '', %d)",
				m_HwdID, it->GetPin(), pTypeLighting1, sTypeIMPULS, int(STYPE_OnOff), it->GetIsInput() ? "Input" : "Output", GPIORead(it->GetPin(), "value"), it->GetIsInput() ? GPIO_IN : GPIO_OUT);

		result = m_sql.safe_query("SELECT Name,nValue,Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",	m_HwdID, it->GetPin());
		if (result.empty())
			return false;
	}
	return true;
}*/

/* static */
std::vector<CGpioPin> CGpio::GetPinList()
{
	return pins;
}

/* static */
CGpioPin* CGpio::GetPPinById(int id)
{
	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
		if (it->GetPin() == id)
			return &(*it);
	return NULL;
}

void CGpio::UpdateDeviceStates(bool forceUpdate)
{
	std::unique_lock<std::mutex> lock(m_pins_mutex);
	for (std::vector<CGpioPin>::iterator it = pins.begin(); it != pins.end(); ++it)
	{
		if (it->GetIsInput())
		{
			int value = GPIOReadFd(it->GetReadValueFd());
			if (value == -1)
				continue;
			bool updateDatabase = forceUpdate;
			bool state = false;

			if (it->GetActiveLow() && !value)
				state = true;
			else if (value)
				state = true;

			if (it->GetDBState() != state || updateDatabase)
			{
				std::vector<std::vector<std::string> > result;
				char szIdx[10];

				result = m_sql.safe_query("SELECT Name,nValue,sValue,Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
					m_HwdID, it->GetPin());

				if ((!result.empty()) && (result.size() > 0))
				{
					std::vector<std::string> sd = result[0];

					if (atoi(sd[3].c_str()) == 1) /* Check if device is used */
					{
						bool db_state = (atoi(sd[1].c_str()) == 1);
						if (db_state != state)
							updateDatabase = true;

						if (updateDatabase)
							UpdateSwitch(it->GetPin(), state);
					}
				}
			}
		}
	}
}

bool CGpio::InitPins()
{
	int fd;
	bool db_state = false;
	char path[GPIO_MAX_PATH];
	char szIdx[10];
	char label[12];
	std::vector<std::vector<std::string> > result;
	std::unique_lock<std::mutex> lock(m_pins_mutex);
	pins.clear();

	for (int gpio_pin = GPIO_PIN_MIN; gpio_pin <= GPIO_PIN_MAX; gpio_pin++)
	{
		snprintf(path, GPIO_MAX_PATH, "%s%d", GPIO_PATH, gpio_pin);
		fd = open(path, O_RDONLY);

		if (fd != -1)
		{
			result = m_sql.safe_query("SELECT nValue FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)",
				m_HwdID, gpio_pin);
			if (!result.empty())
				db_state = atoi(result[0][0].c_str());

			snprintf(label, sizeof(label), "GPIO pin %d", gpio_pin);
			pins.push_back(CGpioPin(gpio_pin, label, GPIORead(gpio_pin, "value"), GPIORead(gpio_pin, "direction"), GPIORead(gpio_pin, "edge"), GPIORead(gpio_pin, "active_low"), -1, db_state));
			//_log.Log(LOG_NORM, "GPIO: Pin %d added (value: %d, direction: %d, edge: %d, active_low: %d, db_state: %d)",
			//	gpio_pin, GPIORead(gpio_pin, "value"), GPIORead(gpio_pin, "direction"), GPIORead(gpio_pin, "edge"), GPIORead(gpio_pin, "active_low"), db_state);
			close(fd);

			if (GPIORead(gpio_pin, "direction") != 0)
				continue;

			snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, gpio_pin);
			fd = pins.back().SetReadValueFd(open(path, O_RDWR)); // O_RDWR seems mandatory to clear interrupt (not sure why?)

			if (fd != -1)
			{
				pinPass = gpio_pin;
				m_thread_interrupt[gpio_pin] = std::make_shared<std::thread>(&CGpio::InterruptHandler, this);
				SetThreadName(m_thread_interrupt[gpio_pin]->native_handle(), "GPIO_Interrupt");
				while (pinPass != -1)
					sleep_milliseconds(1);
			}
		}
	}
	return (pins.size() > 0);
}

int CGpio::GetReadResult(int bytecount, char *value_str)
{
	if (bytecount > 1)
	{
		if ((memcmp("0", value_str, strlen("0")) == 0) ||
			(memcmp("in", value_str, strlen("in")) == 0) ||
			(memcmp("none", value_str, strlen("none")) == 0))
			return 0;

		if ((memcmp("1", value_str, strlen("1")) == 0) ||
			(memcmp("out", value_str, strlen("out")) == 0) ||
			(memcmp("rising", value_str, strlen("rising")) == 0))
			return 1;

		if (memcmp("falling", value_str, strlen("falling")) == 0)
			return 2;

		if (memcmp("both", value_str, strlen("both")) == 0)
			return 3;
	}
	return -1;
}

int CGpio::GPIORead(int gpio_pin, const char *param)
{
	char path[GPIO_MAX_PATH];
	char value_str[GPIO_MAX_VALUE_SIZE];
	int fd;
	int bytecount = -1;

	snprintf(path, GPIO_MAX_PATH, "%s%d/%s", GPIO_PATH, gpio_pin, param);
	fd = open(path, O_RDONLY);

	if (fd == -1)
		return -1;

	bytecount = read(fd, value_str, GPIO_MAX_VALUE_SIZE);
	close(fd);

	if (bytecount == -1)
	{
		close(fd);
		return -1;
	}

	return(GetReadResult(bytecount, &value_str[0]));
}

int CGpio::GPIOReadFd(int fd)
{
	int bytecount = -1;
	char value_str[GPIO_MAX_VALUE_SIZE];

	if (fd == -1)
		return -1;

	if (lseek(fd, 0, SEEK_SET) == -1)
		return -1;

	bytecount = read(fd, value_str, GPIO_MAX_VALUE_SIZE);

	return(GetReadResult(bytecount, &value_str[0]));
}

int CGpio::GPIOWrite(int gpio_pin, bool value)
{
	char path[GPIO_MAX_PATH];

	snprintf(path, GPIO_MAX_PATH, "%s%d/value", GPIO_PATH, gpio_pin);
	int fd = open(path, O_WRONLY);

	if (fd == -1)
		return -1;

	if (write(fd, value ? "1" : "0", 1) != 1)
	{
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int CGpio::SetSchedPriority(const int s, const int pri, const int x)
{
	//Scheduler can be SCHED_OTHER, SCHED_BATCH, SCHED_IDLE, SCHED_FIFO or SCHED_RR
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));

	if (s == SCHED_OTHER || s == SCHED_IDLE || s == SCHED_BATCH)
	{
		sched.sched_priority = 0;
		setpriority(PRIO_PROCESS, 0, pri - x);
	}
	else
	{
		if (pri + x > sched_get_priority_max(s))
			sched.sched_priority = sched_get_priority_max(s);
		else if (pri + x < sched_get_priority_min(s))
			sched.sched_priority = sched_get_priority_min(s);
		else
			sched.sched_priority = pri + x;
	}
	return (sched_setscheduler(0, s, &sched));
}

void CGpio::GetSchedPriority(int *s, int *pri)
{
	struct sched_param sched;
	memset(&sched, 0, sizeof(sched));
	sched_getparam(0, &sched);
	*s = sched_getscheduler(0);
	if (*s == SCHED_OTHER || *s == SCHED_IDLE || *s == SCHED_BATCH)
		*pri = getpriority(PRIO_PROCESS, 0);
	else
		*pri = sched.sched_priority;
	return;
}

#endif
#endif
