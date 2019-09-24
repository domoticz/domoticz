//-----------------------------------------------------------------------------
//
//	Group.h
//
//	A set of associations in a Z-Wave device.
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

#ifndef _Group_H
#define _Group_H

#include <string>
#include <vector>
#include <map>
#include "Defs.h"

class TiXmlElement;

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{
			class Association;
			class MultiChannelAssociation;
		}
	}

	class Node;

	typedef struct InstanceAssociation
	{
			uint8 m_nodeId;
			uint8 m_instance;
	} InstanceAssociation;

	/** \brief Manages a group of devices (various nodes associated with each other).
	 */
	class Group
	{
			friend class Node;
			friend class Internal::CC::Association;
			friend class Internal::CC::MultiChannelAssociation;

			//-----------------------------------------------------------------------------
			// Construction
			//-----------------------------------------------------------------------------
		public:
			Group(uint32 const _homeId, uint8 const _nodeId, uint8 const _groupIdx, uint8 const _maxAssociations);
			Group(uint32 const _homeId, uint8 const _nodeId, TiXmlElement const* _valueElement);
			~Group()
			{
			}

			void WriteXML(TiXmlElement* _groupElement);

			//-----------------------------------------------------------------------------
			// Association methods	(COMMAND_CLASS_ASSOCIATION)
			//-----------------------------------------------------------------------------
		public:
			string const& GetLabel() const
			{
				return m_label;
			}
			uint32 GetAssociations(uint8** o_associations);
			uint32 GetAssociations(InstanceAssociation** o_associations);
			uint8 GetMaxAssociations() const
			{
				return m_maxAssociations;
			}
			uint8 GetIdx() const
			{
				return m_groupIdx;
			}
			bool Contains(uint8 const _nodeId, uint8 const _instance = 0x00);
			bool IsMultiInstance() const
			{
				return m_multiInstance;
			}

		private:
			bool IsAuto() const
			{
				return m_auto;
			}
			void SetAuto(bool const _state)
			{
				m_auto = _state;
			}
			void CheckAuto();

			void SetMultiInstance(bool const _state)
			{
				m_multiInstance = _state;
			}

			void AddAssociation(uint8 const _nodeId, uint8 const _instance = 0x00);
			void RemoveAssociation(uint8 const _nodeId, uint8 const _instance = 0x00);
			void OnGroupChanged(vector<uint8> const& _associations);
			void OnGroupChanged(vector<InstanceAssociation> const& _associations);

			//-----------------------------------------------------------------------------
			// Command methods (COMMAND_CLASS_ASSOCIATION_COMMAND_CONFIGURATION)
			//-----------------------------------------------------------------------------
		public:
			bool ClearCommands(uint8 const _nodeId, uint8 const _instance = 0x00);
			bool AddCommand(uint8 const _nodeId, uint8 const _length, uint8 const* _data, uint8 const _instance = 0x00);

		private:
			class AssociationCommand
			{
				public:
					AssociationCommand(uint8 const _length, uint8 const* _data);
					~AssociationCommand();

				private:
					uint8* m_data;
			};

			typedef vector<AssociationCommand> AssociationCommandVec;
			struct classcomp
			{
					bool operator()(const InstanceAssociation& lhs, const InstanceAssociation& rhs) const
					{
						return lhs.m_nodeId == rhs.m_nodeId ? lhs.m_instance < rhs.m_instance : lhs.m_nodeId < rhs.m_nodeId;
					}
			};

			//-----------------------------------------------------------------------------
			// Member variables
			//-----------------------------------------------------------------------------
		private:
			string m_label;
			uint32 m_homeId;
			uint8 m_nodeId;
			uint8 m_groupIdx;
			uint8 m_maxAssociations;
			bool m_auto;				// If true, the controller will automatically be associated with the group
			bool m_multiInstance;    // If true, the group is MultiInstance capable
			map<InstanceAssociation, AssociationCommandVec, classcomp> m_associations;
	};

} //namespace OpenZWave

#endif //_Group_H

