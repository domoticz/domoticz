//-----------------------------------------------------------------------------
//
//	DeviceResetLocally.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_DEVICE_RESET_LOCALLY
//
//	Copyright (c) 2015
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#ifndef _DeviceResetLocally_H
#define _DeviceResetLocally_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_DEVICE_RESET_LOCALLY (0x5a), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class DeviceResetLocally: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new DeviceResetLocally( _homeId, _nodeId ); }
		virtual ~DeviceResetLocally(){}

		static uint8 const StaticGetCommandClassId(){ return 0x5a; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_DEVICE_RESET_LOCALLY"; }

		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		virtual bool IsDeviceReset() { return m_deviceReset; };
	private:
		DeviceResetLocally( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ), m_deviceReset(false) {};
		bool m_deviceReset;
	};

} // namespace OpenZWave

#endif

