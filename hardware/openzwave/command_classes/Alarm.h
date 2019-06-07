//-----------------------------------------------------------------------------
//
//	Alarm.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
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

#ifndef _Alarm_H
#define _Alarm_H

#include "command_classes/CommandClass.h"
#include "TimerThread.h"

namespace OpenZWave
{
	class ValueByte;

	/** \brief Implements COMMAND_CLASS_NOTIFICATION (0x71), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class Alarm: public CommandClass, private Timer
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Alarm( _homeId, _nodeId ); }
		virtual ~Alarm(){}

		/** \brief Get command class ID (1 byte) identifying this command class. */
		static uint8 const StaticGetCommandClassId() { return 0x71; }
		/** \brief Get a string containing the name of this command class. */
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_NOTIFICATION"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		virtual bool RequestValue( uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		/** \brief Get command class ID (1 byte) identifying this command class. (Inherited from CommandClass) */
		virtual uint8 const GetCommandClassId()const override { return StaticGetCommandClassId(); }
		/** \brief Get a string containing the name of this command class. (Inherited from CommandClass) */
		virtual string const GetCommandClassName()const override { return StaticGetCommandClassName(); }
		/** \brief Handle a response to a message associated with this command class. (Inherited from CommandClass) */
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override;
		virtual uint8 GetMaxVersion() override { return 8; }
		virtual bool SetValue( Value const& _value ) override;


	private:
		Alarm( uint32 const _homeId, uint8 const _nodeId );
		void SetupEvents(uint32 type, uint32 index, vector<ValueList::Item> *_items, uint32 const _instance);
		void ClearAlarm(uint32 type);
		void ClearEventParams(uint32 const _instance);
		bool m_v1Params;
		std::vector<uint32> m_ParamsSet;
		uint32 m_ClearTimeout;
		std::map<uint32, uint32> m_TimersToInstances;


	};

} // namespace OpenZWave

#endif
