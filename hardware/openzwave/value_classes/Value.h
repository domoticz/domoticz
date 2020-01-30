//-----------------------------------------------------------------------------
//
//	Value.h
//
//	Base class for all OpenZWave Value Classes
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

#ifndef _Value_H
#define _Value_H

#include <string>
#ifdef __FreeBSD__
#include <time.h>
#endif
#include "Defs.h"
#include "platform/Ref.h"
#include "value_classes/ValueID.h"
#include "platform/Log.h"

class TiXmlElement;

namespace OpenZWave
{
	class Driver;
	namespace Internal
	{
		namespace VC
		{

			/** \brief Base class for values associated with a node.
			 * \ingroup ValueID
			 */
			class Value: public Internal::Platform::Ref
			{
					friend class OpenZWave::Driver;
					friend class ValueStore;

				public:
					Value(uint32 const _homeId, uint8 const _nodeId, ValueID::ValueGenre const _genre, uint8 const _commandClassId, uint8 const _instance, uint16 const _index, ValueID::ValueType const _type, string const& _label, string const& _units, bool const _readOnly, bool const _writeOnly, bool const _isset, uint8 const _pollIntensity);
					Value();

					virtual void ReadXML(uint32 const _homeId, uint8 const _nodeId, uint8 const _commandClassId, TiXmlElement const* _valueElement);
					virtual void WriteXML(TiXmlElement* _valueElement);

					ValueID const& GetID() const
					{
						return m_id;
					}
					bool IsReadOnly() const
					{
						return m_readOnly;
					}
					bool IsWriteOnly() const
					{
						return m_writeOnly;
					}
					bool IsSet() const
					{
						return m_isSet;
					}
					bool IsPolled() const
					{
						return m_pollIntensity != 0;
					}

					string const GetLabel() const;
					void SetLabel(string const& _label, string const lang = "");

					string const& GetUnits() const
					{
						return m_units;
					}
					void SetUnits(string const& _units)
					{
						m_units = _units;
					}

					string const GetHelp() const;
					void SetHelp(string const& _help, string const lang = "");

					uint8 const& GetPollIntensity() const
					{
						return m_pollIntensity;
					}
					void SetPollIntensity(uint8 const& _intensity)
					{
						m_pollIntensity = _intensity;
					}

					int32 GetMin() const
					{
						return m_min;
					}
					int32 GetMax() const
					{
						return m_max;
					}

					void SetChangeVerified(bool _verify)
					{
						m_verifyChanges = _verify;
					}
					bool GetChangeVerified()
					{
						return m_verifyChanges;
					}

					void SetRefreshAfterSet(bool _refreshAfterSet)
					{
						m_refreshAfterSet = _refreshAfterSet;
					}
					bool GetRefreshAfterSet()
					{
						return m_refreshAfterSet;
					}

					virtual string const GetAsString() const
					{
						return "";
					}
					virtual bool SetFromString(string const&)
					{
						return false;
					}

					bool Set();							// For the user to change a value in a device

					// Helpers
					static OpenZWave::ValueID::ValueGenre GetGenreEnumFromName(char const* _name);
					static char const* GetGenreNameFromEnum(ValueID::ValueGenre _genre);
					static OpenZWave::ValueID::ValueType GetTypeEnumFromName(char const* _name);
					static char const* GetTypeNameFromEnum(ValueID::ValueType _type);
#if 0
					inline void AddRef() {
						Ref::AddRef();
						Log::Write(LogLevel_Warning, "Add Ref %d - %d %s", m_refs, m_id.GetId(), __func__);
						Internal::StackTraceGenerator::GetTrace();
					}
					inline int32 Release() {
						Log::Write(LogLevel_Warning, "Del Ref %d - %d %s", m_refs -1, m_id.GetId(), __func__);
						Internal::StackTraceGenerator::GetTrace();
						return Ref::Release();
					}
#endif
				protected:
					virtual ~Value();

					bool IsCheckingChange() const
					{
						return m_checkChange;
					}
					void SetCheckingChange(bool _check)
					{
						m_checkChange = _check;
					}
					void OnValueRefreshed();			// A value in a device has been refreshed
					void OnValueChanged();				// The refreshed value actually changed
					int VerifyRefreshedValue(void* _originalValue, void* _checkValue, void* _newValue, ValueID::ValueType _type, int _originalValueLength = 0, int _checkValueLength = 0, int _newValueLength = 0);

					int32 m_min;
					int32 m_max;

					time_t m_refreshTime;			// time_t identifying when this value was last refreshed
					bool m_verifyChanges;		// if true, apparent changes are verified; otherwise, they're not
					bool m_refreshAfterSet;		// if true, all value sets are followed by a get to refresh the value manually
					ValueID m_id;

				private:
					string m_units;
					bool m_readOnly;
					bool m_writeOnly;
					bool m_isSet;
					uint8 m_affectsLength;
					uint8* m_affects;
					bool m_affectsAll;
					bool m_checkChange;
					uint8 m_pollIntensity;
			};
		} // namespace VC
	} // namespace Internal
} // namespace OpenZWave

#endif

