//-----------------------------------------------------------------------------
//
//	ValueInt.h
//
//	Represents a 32-bit value
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

#ifndef _ValueInt_H
#define _ValueInt_H

#include <string>
#include "Defs.h"
#include "value_classes/Value.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;

	/** \brief Integer value sent to/received from a node.
	 */
	class ValueInt: public Value
	{
	public:
	  	ValueInt( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint8 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, int32 const _value, uint8 const _pollIntensity );
		ValueInt();
		virtual ~ValueInt(){}

		bool Set( int32 const _value );
		void OnValueRefreshed( int32 const _value );

		// From Value
		virtual string const GetAsString() const;
		virtual bool SetFromString( string const& _value );
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		int32 GetValue()const{ return m_value; }

	private:
		int32	m_value;				// the current value
		int32	m_valueCheck;			// the previous value (used for double-checking spurious value reads)
		int32	m_newValue;				// a new value to be set on the appropriate device
	};

} // namespace OpenZWave

#endif
