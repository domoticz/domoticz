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
*/

#include "stdafx.h"
#ifdef WITH_GPIO
#include "../main/Logger.h"
#include "GpioPin.h"

CGpioPin::CGpioPin(const int pin_number, const std::string &label, const int value, const bool direction, const int edge, const int active_low, const int read_value_fd, const bool db_state):
m_label(label)
{
	m_pin_number = pin_number;
	m_value = value;			// GPIO pin Value
	m_direction = direction;
	m_edge = edge;
	m_active_low = active_low;		// GPIO ActiveLow
	m_read_value_fd = read_value_fd;	// Fast read fd
	m_db_state = db_state;		// Database Value
}

CGpioPin::~CGpioPin()
{
}


int CGpioPin::GetPin()
{
	return m_pin_number;
}

int CGpioPin::GetReadValueFd()
{
	return m_read_value_fd;
}

int CGpioPin::GetEdge()
{
	return m_edge;
}

int CGpioPin::GetActiveLow()
{
	return m_active_low;
}

bool CGpioPin::GetDBState()
{
	return m_db_state;
}

int CGpioPin::SetReadValueFd(int value)
{
	m_read_value_fd = value;
	return m_read_value_fd;
}

void CGpioPin::SetDBState(int db_state)
{
	m_db_state = db_state;
}

bool CGpioPin::GetIsInput()
{
	return (m_direction == 0);
}

std::string CGpioPin::ToString()
{
	if (m_direction == 0)
		return m_label + " (INPUT)";
	else if (m_direction == 1)
		return m_label + " (OUTPUT)";
	return m_label + " Unknown!?";
}

#endif // WITH_GPIO
