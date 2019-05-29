//-----------------------------------------------------------------------------
//
//	SceneActivation.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_SCENE_ACTIVATION
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
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

#ifndef _SceneActivation_H
#define _SceneActivation_H

#include "command_classes/CommandClass.h"
#include "TimerThread.h"

namespace OpenZWave
{
	class ValueByte;

	/** \brief Implements COMMAND_CLASS_SCENEACTIVATION (0x2B), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class SceneActivation: public CommandClass, private Timer
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new SceneActivation( _homeId, _nodeId ); }
		virtual ~SceneActivation(){}

		/** \brief Get command class ID (1 byte) identifying this command class. */
		static uint8 const StaticGetCommandClassId(){ return 0x2B; }		
		/** \brief Get a string containing the name of this command class. */
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SCENE_ACTIVATION"; }

		// From CommandClass
		/** \brief Get command class ID (1 byte) identifying this command class. (Inherited from CommandClass) */
		virtual uint8 const GetCommandClassId() const override { return StaticGetCommandClassId(); }
		/** \brief Get a string containing the name of this command class. (Inherited from CommandClass) */
		virtual string const GetCommandClassName() const override { return StaticGetCommandClassName(); }
		/** \brief Handle a response to a message associated with this command class. (Inherited from CommandClass) */
		virtual bool HandleIncomingMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override;
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override;

	protected:
		virtual void CreateVars( uint8 const _instance ) override;

	private:
		void ClearScene(uint32 id);
		SceneActivation( uint32 const _homeId, uint8 const _nodeId );

	};

} // namespace OpenZWave

#endif
