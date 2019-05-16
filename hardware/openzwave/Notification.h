//-----------------------------------------------------------------------------
//
//	Notification.h
//
//	Contains details of a Z-Wave event reported to the user
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

#ifndef _Notification_H
#define _Notification_H

#include <iostream>

#include "Defs.h"
#include "value_classes/ValueID.h"

namespace OpenZWave
{
	/** \brief Provides a container for data sent via the notification callback
	 *    handler installed by a call to Manager::AddWatcher.
	 *
	 *    A notification object is only ever created or deleted internally by
	 *    OpenZWave.
	 */
	class OPENZWAVE_EXPORT Notification
	{
		friend class Manager;
		friend class Driver;
		friend class Node;
		friend class Group;
		friend class Value;
		friend class ValueStore;
		friend class Basic;
		friend class ManufacturerSpecific;
		friend class NodeNaming;
		friend class NoOperation;
		friend class SceneActivation;
		friend class WakeUp;
		friend class ApplicationStatus;
		friend class ManufacturerSpecificDB;
		/* allow us to Stream a Notification */
		//friend std::ostream &operator<<(std::ostream &os, const Notification &dt);


	public:
		/**
		 * Notification types.
		 * Notifications of various Z-Wave events sent to the watchers
		 * registered with the Manager::AddWatcher method.
		 * \see Manager::AddWatcher
		 * \see Manager::BeginControllerCommand
	     */
		enum NotificationType
		{
			Type_ValueAdded = 0,				/**< A new node value has been added to OpenZWave's list. These notifications occur after a node has been discovered, and details of its command classes have been received.  Each command class may generate one or more values depending on the complexity of the item being represented.  */
			Type_ValueRemoved,					/**< A node value has been removed from OpenZWave's list.  This only occurs when a node is removed. */
			Type_ValueChanged,					/**< A node value has been updated from the Z-Wave network and it is different from the previous value. */
			Type_ValueRefreshed,				/**< A node value has been updated from the Z-Wave network. */
			Type_Group,							/**< The associations for the node have changed. The application should rebuild any group information it holds about the node. */
			Type_NodeNew,						/**< A new node has been found (not already stored in zwcfg*.xml file) */
			Type_NodeAdded,						/**< A new node has been added to OpenZWave's list.  This may be due to a device being added to the Z-Wave network, or because the application is initializing itself. */
			Type_NodeRemoved,					/**< A node has been removed from OpenZWave's list.  This may be due to a device being removed from the Z-Wave network, or because the application is closing. */
			Type_NodeProtocolInfo,				/**< Basic node information has been received, such as whether the node is a listening device, a routing device and its baud rate and basic, generic and specific types. It is after this notification that you can call Manager::GetNodeType to obtain a label containing the device description. */
			Type_NodeNaming,					/**< One of the node names has changed (name, manufacturer, product). */
			Type_NodeEvent,						/**< A node has triggered an event.  This is commonly caused when a node sends a Basic_Set command to the controller.  The event value is stored in the notification. */
			Type_PollingDisabled,				/**< Polling of a node has been successfully turned off by a call to Manager::DisablePoll */
			Type_PollingEnabled,				/**< Polling of a node has been successfully turned on by a call to Manager::EnablePoll */
			Type_SceneEvent,					/**< Scene Activation Set received (Depreciated in 1.8) */
			Type_CreateButton,					/**< Handheld controller button event created */
			Type_DeleteButton,					/**< Handheld controller button event deleted */
			Type_ButtonOn,						/**< Handheld controller button on pressed event */
			Type_ButtonOff,						/**< Handheld controller button off pressed event */
			Type_DriverReady,					/**< A driver for a PC Z-Wave controller has been added and is ready to use.  The notification will contain the controller's Home ID, which is needed to call most of the Manager methods. */
			Type_DriverFailed,					/**< Driver failed to load */
			Type_DriverReset,					/**< All nodes and values for this driver have been removed.  This is sent instead of potentially hundreds of individual node and value notifications. */
			Type_EssentialNodeQueriesComplete,	/**< The queries on a node that are essential to its operation have been completed. The node can now handle incoming messages. */
			Type_NodeQueriesComplete,			/**< All the initialization queries on a node have been completed. */
			Type_AwakeNodesQueried,				/**< All awake nodes have been queried, so client application can expected complete data for these nodes. */
			Type_AllNodesQueriedSomeDead,		/**< All nodes have been queried but some dead nodes found. */
			Type_AllNodesQueried,				/**< All nodes have been queried, so client application can expected complete data. */
			Type_Notification,					/**< An error has occurred that we need to report. */
			Type_DriverRemoved,					/**< The Driver is being removed. (either due to Error or by request) Do Not Call Any Driver Related Methods after receiving this call */
			Type_ControllerCommand,					/**< When Controller Commands are executed, Notifications of Success/Failure etc are communicated via this Notification
										 * Notification::GetEvent returns Driver::ControllerCommand and Notification::GetNotification returns Driver::ControllerState */
			Type_NodeReset,						/**< The Device has been reset and thus removed from the NodeList in OZW */
			Type_UserAlerts,					/**< Warnings and Notifications Generated by the library that should be displayed to the user (eg, out of date config files) */
			Type_ManufacturerSpecificDBReady			/**< The ManufacturerSpecific Database Is Ready */
		};

		/**
		 * Notification codes.
		 * Notifications of the type Type_Notification convey some
		 * extra information defined here.
		 */
		enum NotificationCode
		{
			Code_MsgComplete = 0,			/**< Completed messages */
			Code_Timeout,					/**< Messages that timeout will send a Notification with this code. */
			Code_NoOperation,				/**< Report on NoOperation message sent completion  */
			Code_Awake,						/**< Report when a sleeping node wakes up */
			Code_Sleep,						/**< Report when a node goes to sleep */
			Code_Dead,						/**< Report when a node is presumed dead */
			Code_Alive						/**< Report when a node is revived */
		};

		/**
		 * User Alert Types - These are messages that should be displayed to users to inform them of
		 * potential issues such as Out of Date configuration files etc
		 */
		enum UserAlertNotification
		{
			Alert_None,							/**< No Alert Currently Present */
			Alert_ConfigOutOfDate,				/**< One of the Config Files is out of date. Use GetNodeId to determine which node is affected. */
			Alert_MFSOutOfDate,					/**< the manufacturer_specific.xml file is out of date. */
			Alert_ConfigFileDownloadFailed, 	/**< A Config File failed to download */
			Alert_DNSError,						/**< A error occurred performing a DNS Lookup */
			Alert_NodeReloadReqired,			/**< A new Config file has been discovered for this node, and its pending a Reload to Take Effect */
			Alert_UnsupportedController,		/**< The Controller is not running a Firmware Library we support */
			Alert_ApplicationStatus_Retry,  	/**< Application Status CC returned a Retry Later Message */
			Alert_ApplicationStatus_Queued, 	/**< Command Has been Queued for execution later */
			Alert_ApplicationStatus_Rejected,	/**< Command has been rejected */
		};

		/**
		 * Get the type of this notification.
		 * \return the notification type.
		 * \see NotificationType
	     */
		NotificationType GetType()const{ return m_type; }

		/**
		 * Get the Home ID of the driver sending this notification.
		 * \return the driver Home ID
	     */
		uint32 GetHomeId()const{ return m_valueId.GetHomeId(); }

		/**
		 * Get the ID of any node involved in this notification.
		 * \return the node's ID
	     */
		uint8 GetNodeId()const{ return m_valueId.GetNodeId(); }

		/**
		 * Get the unique ValueID of any value involved in this notification.
		 * \return the value's ValueID
		 */
		ValueID const& GetValueID()const{ return m_valueId; }

		/**
		 * Get the index of the association group that has been changed.  Only valid in Notification::Type_Group notifications.
		 * \return the group index.
		 */
		uint8 GetGroupIdx()const{ assert(Type_Group==m_type); return m_byte; }

		/**
		 * Get the event value of a notification.  Only valid in Notification::Type_NodeEvent and Notification::Type_ControllerCommand notifications.
		 * \return the event value.
		 */
		uint8 GetEvent()const{ assert((Type_NodeEvent==m_type) || (Type_ControllerCommand == m_type)); return m_event; }

		/**
		 * Get the button id of a notification.  Only valid in Notification::Type_CreateButton, Notification::Type_DeleteButton,
		 * Notification::Type_ButtonOn and Notification::Type_ButtonOff notifications.
		 * \return the button id.
		 */
		uint8 GetButtonId()const{ assert(Type_CreateButton==m_type || Type_DeleteButton==m_type || Type_ButtonOn==m_type || Type_ButtonOff==m_type); return m_byte; }

		/**
		 * Get the scene Id of a notification.  Only valid in Notification::Type_SceneEvent notifications.
		 * The SceneActivation CC now exposes ValueID's that convey this information
		 * \return the event value.
		 */
		DEPRECATED uint8 GetSceneId()const{ assert(Type_SceneEvent==m_type); return m_byte; }

		/**
		 * Get the notification code from a notification. Only valid for Notification::Type_Notification or Notification::Type_ControllerCommand notifications.
		 * \return the notification code.
		 */
		uint8 GetNotification()const{ assert((Type_Notification==m_type) || (Type_ControllerCommand == m_type)); return m_byte; }

        /**
         * Get the (controller) command from a notification. Only valid for Notification::Type_ControllerCommand notifications.
         * \return the (controller) command code.
         */
        uint8 GetCommand()const{ assert(Type_ControllerCommand == m_type); return m_command; }
		
		/**
		 * Helper function to simplify wrapping the notification class.  Should not normally need to be called.
		 * \return the internal byte value of the notification.
		 */
		uint8 GetByte()const{ return m_byte; }

		/**
		 * Helper function to return the Timeout to wait for. Only valid for Notification::Type_UserAlerts - Notification::Alert_ApplicationStatus_Retry
		 * \return The time to wait before retrying
		 */
		uint8 GetRetry()const{ assert((Type_UserAlerts == m_type) && (Alert_ApplicationStatus_Retry == m_useralerttype)); return m_byte; }

		/**
		 * Helper Function to return the Notification as a String
		 * \return A string representation of this Notification
		 */
		string GetAsString()const;

		/**
		 * Retrieve the User Alert Type Enum to determine what this message is about
		 * \return UserAlertNotification Enum describing the Alert Type
		 */
		UserAlertNotification GetUserAlertType()const {return m_useralerttype;};

		/**
		 * Return the Comport associated with the DriverFailed Message
		 * \return a string representing the Comport
		 */
		string GetComPort()const { return m_comport; };

	private:
		Notification( NotificationType _type ): m_type( _type ), m_byte(0), m_event(0), m_command(0), m_useralerttype(Alert_None) {}
		~Notification(){}

		void SetHomeAndNodeIds( uint32 const _homeId, uint8 const _nodeId ){ m_valueId = ValueID( _homeId, _nodeId ); }
		void SetHomeNodeIdAndInstance ( uint32 const _homeId, uint8 const _nodeId, uint32 const _instance ){ m_valueId = ValueID( _homeId, _nodeId, _instance ); }
		void SetValueId( ValueID const& _valueId ){ m_valueId = _valueId; }
		void SetGroupIdx( uint8 const _groupIdx ){ assert(Type_Group==m_type); m_byte = _groupIdx; }
		void SetEvent( uint8 const _event ){ assert(Type_NodeEvent==m_type || Type_ControllerCommand == m_type); m_event = _event; }
		void SetSceneId( uint8 const _sceneId ){ assert(Type_SceneEvent==m_type); m_byte = _sceneId; }
		void SetButtonId( uint8 const _buttonId ){ assert(Type_CreateButton==m_type||Type_DeleteButton==m_type||Type_ButtonOn==m_type||Type_ButtonOff==m_type); m_byte = _buttonId; }
		void SetNotification( uint8 const _noteId ){ assert((Type_Notification==m_type) || (Type_ControllerCommand == m_type)); m_byte = _noteId; }
		void SetUserAlertNotification(UserAlertNotification const alerttype){ assert(Type_UserAlerts==m_type); m_useralerttype = alerttype; }
		void SetCommand( uint8 const _command ){ assert(Type_ControllerCommand == m_type); m_command = _command; }
		void SetComPort( string comport) { assert(Type_DriverFailed == m_type); m_comport = comport; }
		void SetRetry (uint8 const timeout) { assert(Type_UserAlerts == m_type); m_byte = timeout; }

		NotificationType		m_type;
		ValueID				m_valueId;
		uint8				m_byte;
		uint8				m_event;
		uint8				m_command;
		UserAlertNotification m_useralerttype;
		string				m_comport;
	};

} //namespace OpenZWave

std::ostream& operator<<(std::ostream &os, const OpenZWave::Notification &dt);
std::ostream& operator<<(std::ostream &os, const OpenZWave::Notification *dt);

#endif //_Notification_H

