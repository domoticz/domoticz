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

#ifndef _SimpleAVCommandItem_H
#define _SimpleAVCommandItem_H

#include <string>
#include <vector>
#include "Defs.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace CC
		{

			class SimpleAVCommandItem
			{
				public:
					SimpleAVCommandItem(uint16 const _code, string _name, string _description, uint16 const _version);
					uint16 GetCode();
					string GetName();
					string GetDescription();
					uint16 GetVersion();

					static vector<SimpleAVCommandItem> GetCommands();

				private:
					uint16 m_code;
					string m_name;
					string m_description;
					uint16 m_version;
			};
		} // namespace CC
	} // namespace Internal
} // namespace CC

#endif
