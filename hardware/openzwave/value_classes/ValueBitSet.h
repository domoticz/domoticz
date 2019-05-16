//-----------------------------------------------------------------------------
//
//	ValueBitSet.h
//
//	Represents a Range of Bits
//
//	Copyright (c) 2017 Justin Hammond <justin@dynam.ac>
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

#ifndef _ValueBitSet_H
#define _ValueBitSet_H

#include <string>
#include <map>
#include "Defs.h"
#include "value_classes/Value.h"
#include "Bitfield.h"

class TiXmlElement;

namespace OpenZWave
{
	class Msg;
	class Node;
	class CommandClass;

	/** \brief BitSet value sent to/received from a node.
	 * \ingroup ValueID
	 */
	class ValueBitSet: public Value
	{
	public:
		ValueBitSet( uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, uint32 const _value, uint8 const _pollIntensity );
		ValueBitSet(){}
		virtual ~ValueBitSet(){}

		bool Set( uint32 const _value );
		uint32 GetValue() const;

		bool SetBit( uint8 const _idx);
		bool ClearBit(uint8 const _idx);
		bool GetBit(uint8 _idx) const;

		bool SetBitMask(uint32 _bitMask);
		uint32 GetBitMask() const;

		void OnValueRefreshed( uint32 const _value );

		// From Value
		virtual string const GetAsString() const;
		virtual string const GetAsBinaryString() const;
		virtual bool SetFromString( string const& _value );
		virtual void ReadXML( uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement );
		virtual void WriteXML( TiXmlElement* _valueElement );

		string GetBitHelp(uint8 _idx);
		bool SetBitHelp(uint8 _idx, string help);
		string GetBitLabel(uint8 _idx);
		bool SetBitLabel(uint8 _idx, string label);

		uint8 GetSize() const;
		void SetSize(uint8 size);

	private:
		bool isValidBit(uint8 _idx) const;
		Bitfield		m_value;				// the current index in the m_items vector
		Bitfield		m_valueCheck;			// the previous value (used for double-checking spurious value reads)
		Bitfield		m_newValue;				// a new value to be set on the appropriate device
		uint32			m_BitMask;				// Valid Bits
		uint8 			m_size;					// Number of bytes in size
		vector<int32>   m_bits;
	};

} // namespace OpenZWave

#endif



