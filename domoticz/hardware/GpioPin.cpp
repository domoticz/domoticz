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

This is a derivative work based on the samples included with wiringPi and distributed 
under the GNU Lesser General Public License version 3
Source: http://wiringpi.com
*/

#include "stdafx.h"
#ifdef WITH_GPIO
#include "../main/Logger.h"
#include "GpioPin.h"

CGpioPin::CGpioPin(int id, std::string label, bool isInput, bool isOutput, bool isExported)
{
	m_id = id;
	m_label = label;
	m_isInput = isInput;
	m_isOutput = isOutput;
	m_isExported = isExported;
}

CGpioPin::~CGpioPin()
{
}


int CGpioPin::GetId() 
{
	return m_id;
}

std::string CGpioPin::GetLabel()
{
	return m_label;
}

bool CGpioPin::GetIsInput()
{
	return m_isInput;
}

bool CGpioPin::GetIsOutput()
{
	return m_isOutput;
}

bool CGpioPin::GetIsExported()
{
	return m_isExported;
}


std::string CGpioPin::ToString() 
{
	if (!m_isExported) {
		return "! NOT EXPORTED - " + m_label;
	}
	if (!m_isInput && !m_isOutput) {
		return "! NOT CONFIGURED AS GPIO - " + m_label;
	}
	return m_label + " (" + (m_isInput?"INPUT":"") + (m_isOutput?"OUTPUT":"") + ")";
}

#endif // WITH_GPIO
