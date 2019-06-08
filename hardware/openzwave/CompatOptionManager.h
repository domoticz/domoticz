//-----------------------------------------------------------------------------
//
//	CompatOptionManager.h
//
//	Handles Compatibility Flags in Config Files
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

#ifndef CPP_SRC_COMPATOPTIONMANAGER_H_
#define CPP_SRC_COMPATOPTIONMANAGER_H_

#include "Defs.h"
#include "tinyxml.h"

#include <map>

namespace OpenZWave
{

	namespace Internal
	{
		namespace CC
		{
			class CommandClass;
		}

		enum CompatOptionFlags
		{
			COMPAT_FLAG_GETSUPPORTED,
			COMPAT_FLAG_OVERRIDEPRECISION,
			COMPAT_FLAG_FORCEVERSION,
			COMPAT_FLAG_CREATEVARS,
			COMPAT_FLAG_REFRESHONWAKEUP,
			COMPAT_FLAG_BASIC_IGNOREREMAPPING,
			COMPAT_FLAG_BASIC_SETASREPORT,
			COMPAT_FLAG_BASIC_MAPPING,
			COMPAT_FLAG_COLOR_IDXBUG,
			COMPAT_FLAG_MCA_FORCEINSTANCES,
			COMPAT_FLAG_MI_MAPROOTTOENDPOINT,
			COMPAT_FLAG_MI_FORCEUNIQUEENDPOINTS,
			COMPAT_FLAG_MI_IGNMCCAPREPORTS,
			COMPAT_FLAG_MI_ENDPOINTHINT,
			COMPAT_FLAG_TSSP_BASE,
			COMPAT_FLAG_TSSP_ALTTYPEINTERPRETATION,
			COMPAT_FLAG_UC_EXPOSERAWVALUE,
			COMPAT_FLAG_VERSION_GETCLASSVERSION,
			COMPAT_FLAG_WAKEUP_DELAYNMI,
			STATE_FLAG_CCVERSION,
			STATE_FLAG_STATIC_REQUESTS,
			STATE_FLAG_AFTERMARK,
			STATE_FLAG_ENCRYPTED,
			STATE_FLAG_INNIF,
			STATE_FLAG_CS_SCENECOUNT,
			STATE_FLAG_CS_CLEARTIMEOUT,
			STATE_FLAG_CCS_CHANGECOUNTER,
			STATE_FLAG_COLOR_CHANNELS,
			STATE_FLAG_DOORLOCK_TIMEOUT,
			STATE_FLAG_DOORLOCK_INSIDEMODE,
			STATE_FLAG_DOORLOCK_OUTSIDEMODE,
			STATE_FLAG_DOORLOCK_TIMEOUTMINS,
			STATE_FLAG_DOORLOCK_TIMEOUTSECS,
			STATE_FLAG_DOORLOCKLOG_MAXRECORDS,
			STATE_FLAG_USERCODE_COUNT,
		};

		enum CompatOptionFlagType
		{
			COMPAT_FLAG_TYPE_BOOL,
			COMPAT_FLAG_TYPE_BYTE,
			COMPAT_FLAG_TYPE_SHORT,
			COMPAT_FLAG_TYPE_INT
		};

		enum CompatOptionType
		{
			CompatOptionType_Compatibility,
			CompatOptionType_Discovery
		};

		struct CompatOptionFlagStorage
		{
				CompatOptionFlags flag;
				CompatOptionFlagType type;
				bool changed;
				union
				{
						bool valBool;
						uint8_t valByte;
						uint16_t valShort;
						uint32_t valInt;
				};
		};

		struct CompatOptionFlagDefintions
		{
				string name;
				CompatOptionFlags flag;
				CompatOptionFlagType type;
		};

		class CompatOptionManager
		{
			public:
				CompatOptionManager(CompatOptionType type, Internal::CC::CommandClass *cc);
				virtual ~CompatOptionManager();

				void SetNodeAndCC(uint8_t node, uint8_t cc);
				void EnableFlag(CompatOptionFlags flag, uint32_t defaultval);

				void ReadXML(TiXmlElement const* _ccElement);
				void WriteXML(TiXmlElement* _ccElement);
				bool GetFlagBool(CompatOptionFlags flag) const;
				uint8_t GetFlagByte(CompatOptionFlags flag) const;
				uint16_t GetFlagShort(CompatOptionFlags flag) const;
				uint32_t GetFlagInt(CompatOptionFlags flag) const;
				bool SetFlagBool(CompatOptionFlags flag, bool value);
				bool SetFlagByte(CompatOptionFlags flag, uint8_t value);
				bool SetFlagShort(CompatOptionFlags flag, uint16_t value);
				bool SetFlagInt(CompatOptionFlags flag, uint32_t value);
			private:
				string GetFlagName(CompatOptionFlags flag) const;
				string GetXMLTagName();
				map<CompatOptionFlags, CompatOptionFlagStorage> m_CompatVals;
				map<string, CompatOptionFlags> m_enabledCompatFlags;
				Internal::CC::CommandClass *m_owner;
				CompatOptionType m_comtype;
				CompatOptionFlagDefintions *m_availableFlags;
				uint32_t m_availableFlagsCount;
		};
	} // namespace Internal
} /* namespace OpenZWave */

#endif /* CPP_SRC_COMPATOPTIONMANAGER_H_ */
