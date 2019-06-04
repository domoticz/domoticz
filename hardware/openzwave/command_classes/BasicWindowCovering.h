//-----------------------------------------------------------------------------
//
//	BasicWindowCovering.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_BASIC_WINDOW_COVERING
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
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

#ifndef _BasicWindowCovering_H
#define _BasicWindowCovering_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	class ValueButton;

	/** \brief Implements COMMAND_CLASS_BASIC_WINDOW_COVERING (0x50), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class BasicWindowCovering: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new BasicWindowCovering( _homeId, _nodeId ); }
		virtual ~BasicWindowCovering(){}

		static uint8 const StaticGetCommandClassId(){ return 0x50; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_BASIC_WINDOW_COVERING"; }

		// From CommandClass
		virtual uint8 const GetCommandClassId() const override{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName() const override{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override { return false; }
		virtual bool SetValue( Value const& _value ) override;

	protected:
		virtual void CreateVars( uint8 const _instance ) override;

	private:
		BasicWindowCovering( uint32 const _homeId, uint8 const _nodeId ): CommandClass( _homeId, _nodeId ){}
	};

} // namespace OpenZWave

#endif

