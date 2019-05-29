//-----------------------------------------------------------------------------
//
//	TimeParameters.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_TIME_PARAMETERS
//
//	Copyright (c) 2014 Justin Hammond <Justin@dynam.ac>
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

#ifndef _TimeParameters_H
#define _TimeParameters_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	class ValueByte;
	class ValueList;

	/** \brief Implements COMMAND_CLASS_TIME_PARAMETERS (0x8B), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class TimeParameters: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new TimeParameters( _homeId, _nodeId ); }
		virtual ~TimeParameters(){}

		static uint8 const StaticGetCommandClassId(){ return 0x8B; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_TIME_PARAMETERS"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		virtual bool RequestValue( uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		virtual uint8 const GetCommandClassId() const override { return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName() const override { return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override;
		virtual bool SetValue( Value const& _value ) override;

	protected:
		virtual void CreateVars( uint8 const _instance ) override;

	private:
		TimeParameters( uint32 const _homeId, uint8 const _nodeId );
	};

} // namespace OpenZWave

#endif

