//-----------------------------------------------------------------------------
//
//	Security.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_Security
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

#ifndef _Security_H
#define _Security_H

#include <ctime>
#include "aes/aescpp.h"
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_SECURITY (0x98), a Z-Wave device command class.
	 */


	enum SecurityCmd
	{
		SecurityCmd_SupportedGet			= 0x02,
		SecurityCmd_SupportedReport			= 0x03,
		SecurityCmd_SchemeGet				= 0x04,
		SecurityCmd_SchemeReport			= 0x05,
		SecurityCmd_NetworkKeySet			= 0x06,
		SecurityCmd_NetworkKeyVerify		= 0x07,
		SecurityCmd_SchemeInherit			= 0x08,
		SecurityCmd_NonceGet				= 0x40,
		SecurityCmd_NonceReport				= 0x80,
		SecurityCmd_MessageEncap			= 0x81,
		SecurityCmd_MessageEncapNonceGet	= 0xc1
	};

	enum SecurityScheme
	{
		SecurityScheme_Zero					= 0x00,
	};



	class Security: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new Security( _homeId, _nodeId ); }
		virtual ~Security();

		static uint8 const StaticGetCommandClassId(){ return 0x98; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_SECURITY"; }
		bool Init();
		bool ExchangeNetworkKeys();
		// From CommandClass
		virtual uint8 const GetCommandClassId()const{ return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName()const{ return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 );
		void ReadXML(TiXmlElement const* _ccElement);
		void WriteXML(TiXmlElement* _ccElement);
		void SendMsg( Msg* _msg );

	protected:
		void CreateVars( uint8 const _instance );

	private:
		Security( uint32 const _homeId, uint8 const _nodeId );
		bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue);
		bool RequestValue( uint32 const _requestFlags, uint8 const _index, uint8 const _instance, Driver::MsgQueue const _queue);
		bool HandleSupportedReport(uint8 const* _data, uint32 const _length);

		bool m_schemeagreed;
		bool m_secured;





	};

} // namespace OpenZWave

#endif

