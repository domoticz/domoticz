//-----------------------------------------------------------------------------
//
//	SimpleAVCommandItem.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SIMPLE_AV_CONTROL
//
//	Copyright (c) 2018
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
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
			/** Implements COMMAND_CLASS_SIMPLE_AV_CONTROL (0x94), a Z-Wave device command class. */
			class SimpleAV: public CommandClass
			{
				public:
					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new SimpleAV(_homeId, _nodeId);
					}
					virtual ~SimpleAV()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x94;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_SIMPLE_AV_CONTROL";
					}

					// From CommandClass
					virtual uint8 const GetCommandClassId() const override
					{
						return StaticGetCommandClassId();
					}
					virtual string const GetCommandClassName() const override
					{
						return StaticGetCommandClassName();
					}
					virtual bool HandleMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;
					virtual bool SetValue(Internal::VC::Value const& _value) override;

				protected:
					virtual void CreateVars(uint8 const _instance) override;

				private:
					SimpleAV(uint32 const _homeId, uint8 const _nodeId) :
							CommandClass(_homeId, _nodeId), m_sequence(0)
					{
					}
					uint8 m_sequence;
			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave
