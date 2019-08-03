//-----------------------------------------------------------------------------
//
//	SensorMultiLevelCCTypes.h
//
//	SensorMultiLevelCCTypes for SensorMultiLevel Command Class
//
//	Copyright (c) 2019 Justin Hammond <justin@dynam.ac>
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

#ifndef SENSORMULTILEVELCCTYPES_H
#define SENSORMULTILEVELCCTYPES_H

#include <cstdio>
#include <string>
#include <map>
#include "Defs.h"
#include "Driver.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	namespace Internal
	{

		class SensorMultiLevelCCTypes
		{
			public:
				class SensorMultiLevelScales
				{
					public:
						uint8 id;
						string name;
						string unit;
				};
				typedef std::map<uint8, std::shared_ptr<SensorMultiLevelCCTypes::SensorMultiLevelScales> > SensorScales;
				class SensorMultiLevelTypes
				{
					public:
						uint32 id;
						string name;
						SensorScales allSensorScales;
				};

				//-----------------------------------------------------------------------------
				// Construction
				//-----------------------------------------------------------------------------
			private:
				SensorMultiLevelCCTypes();
				~SensorMultiLevelCCTypes();
				static void ReadXML();
			public:
				static SensorMultiLevelCCTypes* Get();
				static bool Create();
				string GetSensorName(uint32);
				string GetSensorUnit(uint32, uint8);
				string GetSensorUnitName(uint32, uint8);
				const SensorScales GetSensorScales(uint32);

				//-----------------------------------------------------------------------------
				// Instance Functions
				//-----------------------------------------------------------------------------
			private:
				static SensorMultiLevelCCTypes* m_instance;
				static std::map<uint32, std::shared_ptr<SensorMultiLevelCCTypes::SensorMultiLevelTypes> > SensorTypes;
				static uint32 m_revision;
		};
	} // namespace Internal
} // namespace OpenZWave
#endif // SENSORMULTILEVELCCTYPES_H
