//-----------------------------------------------------------------------------
//
//	BarrierOperator.h
//
//	Implementation of the COMMAND_CLASS_BARRIER_OPERATOR
//
//	Copyright (c) 2016 srirams (https://github.com/srirams)
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

#ifndef _BarrierOperator_H
#define _BarrierOperator_H

#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
			/** \brief Implements COMMAND_CLASS_BARRIER_OPERATOR (0x66), a Z-Wave device command class.
			 * \ingroup CommandClass
			 */
			class BarrierOperator: public CommandClass
			{
				public:
					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new BarrierOperator(_homeId, _nodeId);
					}
					virtual ~BarrierOperator()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x66;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_BARRIER_OPERATOR";
					}

					// From CommandClass
					virtual bool RequestState(uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue) override;
					virtual bool RequestValue(uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue) override;
					bool RequestSignalSupport(uint8 const _instance, Driver::MsgQueue const _queue);
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

					virtual uint8 GetMaxVersion() override
					{
						return 3;
					}

				protected:
					virtual void CreateVars(uint8 const _instance) override;

				private:
					BarrierOperator(uint32 const _homeId, uint8 const _nodeId);
			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif
