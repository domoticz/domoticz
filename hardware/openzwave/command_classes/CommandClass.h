//-----------------------------------------------------------------------------
//
//	CommandClass.h
//
//	Base class for all Z-Wave Command Classes
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

#ifndef _CommandClass_H
#define _CommandClass_H

#include <string>
#include <vector>
#include <map>
#include "Defs.h"
#include "Bitfield.h"
#include "Driver.h"
#include "CompatOptionManager.h"

namespace OpenZWave
{
class Msg;
class Node;
class Value;
/** \defgroup CommandClass Z-Wave CommandClass Support
 *
 * This is the CommandClasses that OZW currently supports. Typically, a Application does not need
 * to be aware of the CommandClasses a Device exposes, as they would be transparently exposed to the
 * application as ValueID's
 */


/** \brief Base class for all Z-Wave command classes.
 * \ingroup CommandClass
 */
class OPENZWAVE_EXPORT CommandClass
{

public:
	enum
	{
		RequestFlag_Static		= 0x00000001,	/**< Values that never change. */
		RequestFlag_Session		= 0x00000002,	/**< Values that change infrequently, and so only need to be requested at start up, or via a manual refresh. */
		RequestFlag_Dynamic		= 0x00000004,	/**< Values that change and will be requested if polling is enabled on the node. */
		RequestFlag_AfterMark	= 0x00000008	/**< Values relevent to Controlling CC, not Controlled CC. */
	};

	CommandClass( uint32 const _homeId, uint8 const _nodeId );
	virtual ~CommandClass();

	virtual void ReadXML( TiXmlElement const* _ccElement );
	virtual void WriteXML( TiXmlElement* _ccElement );
	virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue ){ return false; }
	virtual bool RequestValue( uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue ) { return false; }
	virtual void refreshValuesOnWakeup();


	virtual uint8 const GetCommandClassId()const = 0;
	virtual string const GetCommandClassName()const = 0;
	string const GetCommandClassLabel();
	void SetCommandClassLabel(string label);
	virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) = 0;
	virtual bool HandleIncomingMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
	virtual bool SetValue( Value const& _value ){ return false; }
	virtual void SetValueBasic( uint8 const _instance, uint8 const _level ){}		// Class specific handling of BASIC value mapping
	virtual void SetVersion( uint8 const _version ); // Made out-of-line to allow checks against downgrade

	bool RequestStateForAllInstances( uint32 const _requestFlags, Driver::MsgQueue const _queue );
	bool CheckForRefreshValues(Value const* _value );

	// The highest version number of the command class implemented by OpenZWave.  We only need
	// to do version gets on command classes that override this with a number greater than one.
	virtual uint8 GetMaxVersion(){ return 1; }

	uint8 GetVersion()const{ return m_dom.GetFlagByte(STATE_FLAG_CCVERSION); }
	Bitfield const* GetInstances()const{ return &m_instances; }
	uint32 GetHomeId()const{ return m_homeId; }
	uint8 GetNodeId()const{ return m_nodeId; }
	Driver* GetDriver()const;
	Node* GetNodeUnsafe()const;
	Value* GetValue( uint8 const _instance, uint16 const _index );
	bool RemoveValue( uint8 const _instance, uint16 const _index );
	uint8 GetEndPoint( uint8 const _instance )
	{
		map<uint8,uint8>::iterator it = m_endPointMap.find( _instance );
		return( it == m_endPointMap.end() ? 0 : it->second );
	}
	uint8 GetInstance( uint8 const _endPoint )
	{
		for( map<uint8,uint8>::iterator it = m_endPointMap.begin(); it != m_endPointMap.end(); ++it )
		{
			if( _endPoint == it->second )
			{
				return it->first;
			}
		}
		return 0;
	}

	void SetInstances( uint8 const _instances );
	void SetInstance( uint8 const _endPoint );
	/* overridden in the MultiInstance CC to set the Global Label for each Instance */
	virtual void SetInstanceLabel(uint8 const _instance, char *label);
	string GetInstanceLabel(uint8 const _instance);
	uint8 GetNumInstances() { return (uint8)m_endPointMap.size(); };
	void SetAfterMark(){ m_dom.SetFlagBool(STATE_FLAG_AFTERMARK, true); }
	void SetEndPoint( uint8 const _instance, uint8 const _endpoint){ m_endPointMap[_instance] = _endpoint; }
	bool IsAfterMark()const{ return m_dom.GetFlagBool(STATE_FLAG_AFTERMARK); }
	bool IsSecured()const{ return m_dom.GetFlagBool(STATE_FLAG_ENCRYPTED); }
	void SetSecured(){ m_dom.SetFlagBool(STATE_FLAG_ENCRYPTED, true); }
	bool IsSecureSupported()const { return m_SecureSupport; }
	void ClearSecureSupport() { m_SecureSupport = false; }
	void SetSecureSupport() { m_SecureSupport = true; }
	void SetInNIF() { m_dom.SetFlagBool(STATE_FLAG_INNIF, true); }
	bool IsInNIF() { return m_dom.GetFlagBool(STATE_FLAG_INNIF); }

	// Helper methods
	string ExtractValue( uint8 const* _data, uint8* _scale, uint8* _precision, uint8 _valueOffset = 1 )const;

	/**
	 *  Append a floating-point value to a message.
	 *  \param _msg The message to which the value should be appended.
	 *  \param _value A string containing a decimal number to be appended.
	 *  \param _scale A byte indicating the scale corresponding to this value (e.g., 1=F and 0=C for temperatures).
	 *  \see Msg
	 */
	void AppendValue( Msg* _msg, string const& _value, uint8 const _scale )const;
	uint8 const GetAppendValueSize( string const& _value )const;
	int32 ValueToInteger( string const& _value, uint8* o_precision, uint8* o_size )const;

	void UpdateMappedClass( uint8 const _instance, uint8 const _classId, uint8 const _value );		// Update mapped class's value from BASIC class

	typedef struct RefreshValue {
		uint8 cc;
		uint8 genre;
		uint8 instance;
		uint16 index;
		std::vector<RefreshValue *> RefreshClasses;
	} RefreshValue;

protected:
	virtual void CreateVars( uint8 const _instance ){}
	void ReadValueRefreshXML ( TiXmlElement const* _ccElement );
	CompatOptionManager m_com;
	CompatOptionManager m_dom;


public:
	virtual void CreateVars( uint8 const _instance, uint8 const _index ){}

private:
	uint32		m_homeId;
	uint8		m_nodeId;
	Bitfield	m_instances;
	OPENZWAVE_EXPORT_WARNINGS_OFF
	map<uint8,uint8> m_endPointMap;
	map<uint8, string> m_instanceLabel;
	OPENZWAVE_EXPORT_WARNINGS_ON
	bool		m_SecureSupport; 	// Does this commandclass support secure encryption (eg, the Security CC doesn't encrypt itself, so it doesn't support encryption)
	std::vector<RefreshValue *> m_RefreshClassValues; // what Command Class Values should we refresh ?
	string		m_commandClassLabel;
	//-----------------------------------------------------------------------------
	// Record which items of static data have been read from the device
	//-----------------------------------------------------------------------------
public:
	enum StaticRequest
	{
		StaticRequest_Instances		= 0x01,
		StaticRequest_Values		= 0x02,
		StaticRequest_Version		= 0x04
	};

	bool HasStaticRequest( uint8_t _request )const{ return( (m_dom.GetFlagByte(STATE_FLAG_STATIC_REQUESTS) & _request) != 0 ); }
	void SetStaticRequest( uint8_t _request );
	void ClearStaticRequest( uint8_t _request );


	//-----------------------------------------------------------------------------
	//	Statistics
	//-----------------------------------------------------------------------------
public:
	uint32 GetSentCnt()const{ return m_sentCnt; }
	uint32 GetReceivedCnt()const{ return m_receivedCnt; }
	void SentCntIncr(){ m_sentCnt++; }
	void ReceivedCntIncr(){ m_receivedCnt++; }

private:
	uint32 m_sentCnt;				// Number of messages sent from this command class.
	uint32 m_receivedCnt;				// Number of messages received from this commandclass.

};
//@}
} // namespace OpenZWave

#endif
