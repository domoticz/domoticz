//-----------------------------------------------------------------------------
//
//	NotificationCCTypes.h
//
//	NotificationCCTypes for Notification Command Class
//
//	Copyright (c) 2018 Justin Hammond <justin@dynam.ac>
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

#ifndef NOTIFICATIONCCTYPES_H
#define NOTIFICATIONCCTYPES_H

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

		class NotificationCCTypes
		{
			public:
				enum NotificationEventParamTypes
				{
					NEPT_Location = 0x01,
					NEPT_List,
					NEPT_UserCodeReport,
					NEPT_Byte,
					NEPT_String,
					NEPT_Time
				};

				class NotificationEventParams
				{
					public:
						uint32 id;
						string name;
						NotificationEventParamTypes type;
						std::map<uint32, string> ListItems;
				};
				class NotificationEvents
				{
					public:
						uint32 id;
						string name;
						std::map<uint32, std::shared_ptr<NotificationCCTypes::NotificationEventParams> > EventParams;
				};
				class NotificationTypes
				{
					public:
						uint32 id;
						string name;
						std::map<uint32, std::shared_ptr<NotificationCCTypes::NotificationEvents> > Events;
				};

				//-----------------------------------------------------------------------------
				// Construction
				//-----------------------------------------------------------------------------
			private:
				NotificationCCTypes();
				~NotificationCCTypes();
				static void ReadXML();
			public:
				static NotificationCCTypes* Get();
				static bool Create();
				static string GetEventParamNames(NotificationEventParamTypes);
				string GetAlarmType(uint32);
				string GetEventForAlarmType(uint32, uint32);
				const std::shared_ptr<NotificationCCTypes::NotificationTypes> GetAlarmNotificationTypes(uint32);
				const std::shared_ptr<NotificationEvents> GetAlarmNotificationEvents(uint32, uint32);
				const std::map<uint32, std::shared_ptr<NotificationCCTypes::NotificationEventParams>> GetAlarmNotificationEventParams(uint32, uint32);

				//-----------------------------------------------------------------------------
				// Instance Functions
				//-----------------------------------------------------------------------------
			private:
				static NotificationCCTypes* m_instance;
				static std::map<uint32,std::shared_ptr<NotificationCCTypes::NotificationTypes> > Notifications;
				static uint32 m_revision;
		};
	} // namespace Internal
} // namespace OpenZWave
#endif // NOTIFICATIONCCTYPES_H
