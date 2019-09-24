//-----------------------------------------------------------------------------
//
//	MultiInstance.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MULTI_INSTANCE
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

#ifndef _MultiInstance_H
#define _MultiInstance_H

#include <set>
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			/** \brief Implements COMMAND_CLASS_MULTI_INSTANCE (0x60), a Z-Wave device command class.
			 * \ingroup CommandClass
			 */
			class MultiInstance: public CommandClass
			{
				public:
					enum MultiInstanceCmd
					{
						MultiInstanceCmd_Get = 0x04,
						MultiInstanceCmd_Report = 0x05,
						MultiInstanceCmd_Encap = 0x06,

						// Version 2
						MultiChannelCmd_EndPointGet = 0x07,
						MultiChannelCmd_EndPointReport = 0x08,
						MultiChannelCmd_CapabilityGet = 0x09,
						MultiChannelCmd_CapabilityReport = 0x0a,
						MultiChannelCmd_EndPointFind = 0x0b,
						MultiChannelCmd_EndPointFindReport = 0x0c,
						MultiChannelCmd_Encap = 0x0d
					};

					static CommandClass* Create(uint32 const _homeId, uint8 const _nodeId)
					{
						return new MultiInstance(_homeId, _nodeId);
					}
					virtual ~MultiInstance()
					{
					}

					static uint8 const StaticGetCommandClassId()
					{
						return 0x60;
					}
					static string const StaticGetCommandClassName()
					{
						return "COMMAND_CLASS_MULTI_INSTANCE/CHANNEL";
					}

					bool RequestInstances();

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
					virtual bool HandleIncomingMsg(uint8 const* _data, uint32 const _length, uint32 const _instance = 1) override;
					virtual uint8 GetMaxVersion() override
					{
						return 2;
					}
					void SetInstanceLabel(uint8 const _instance, char *label) override;

				private:
					MultiInstance(uint32 const _homeId, uint8 const _nodeId);

					void HandleMultiInstanceReport(uint8 const* _data, uint32 const _length);
					void HandleMultiInstanceEncap(uint8 const* _data, uint32 const _length);
					void HandleMultiChannelEndPointReport(uint8 const* _data, uint32 const _length);
					void HandleMultiChannelCapabilityReport(uint8 const* _data, uint32 const _length);
					void HandleMultiChannelEndPointFindReport(uint8 const* _data, uint32 const _length);
					void HandleMultiChannelEncap(uint8 const* _data, uint32 const _length);

					bool m_numEndPointsCanChange;
					bool m_endPointsAreSameClass;
					uint8 m_numEndPoints;

					// Finding endpoints
					uint8 m_endPointFindIndex;
					uint8 m_numEndPointsFound;
					set<uint8> m_endPointCommandClasses;

			};
		} // namespace CC
	} // namespace Internal
} // namespace OpenZWave

#endif

