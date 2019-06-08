//-----------------------------------------------------------------------------
//
//	Bitfield.h
//
//	Variable length bitfield implementation
//
//	Copyright (c) 2011 Mal Lansell <openzwave@lansell.org>
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

#ifndef _Bitfield_H
#define _Bitfield_H

#include <vector>
#include "Defs.h" 

namespace OpenZWave
{
	namespace Internal
	{

		class OPENZWAVE_EXPORT Bitfield
		{
				friend class Iterator;

			public:
				Bitfield();
				Bitfield(uint32 value);
				~Bitfield();
				bool Set(uint8 _idx);
				bool Clear(uint8 _idx);
				bool IsSet(uint8 _idx) const;
				uint32 GetNumSetBits() const;
				uint32 GetValue() const;
				bool SetValue(uint32 val);
				uint32 GetSize() const;
				class Iterator
				{
						friend class Bitfield;

					public:
						uint32 operator *() const;
						Iterator& operator++();
						Iterator operator++(int);

						bool operator ==(const Iterator &rhs);
						bool operator !=(const Iterator &rhs);
					private:
						Iterator(Bitfield const* _bitfield, uint32 _idx);

						void NextSetBit();

						uint32 m_idx;
						Bitfield const* m_bitfield;
				};

				Iterator Begin() const;
				Iterator End() const;

			private:
				OPENZWAVE_EXPORT_WARNINGS_OFF
				vector<uint32> m_bits;OPENZWAVE_EXPORT_WARNINGS_ON
				uint32 m_numSetBits;
				uint32 m_value;
		};
	} // namespace OpenZWave
} // namespace OpenZWave

#endif

