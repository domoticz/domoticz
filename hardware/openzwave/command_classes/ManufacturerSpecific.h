//-----------------------------------------------------------------------------
//
//	ManufacturerSpecific.h
//
//	Implementation of the Z-Wave COMMAND_CLASS_MANUFACTURER_SPECIFIC
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

#ifndef _ManufacturerSpecific_H
#define _ManufacturerSpecific_H

#include <map>
#include "command_classes/CommandClass.h"

namespace OpenZWave
{
	/** \brief Implements COMMAND_CLASS_MANUFACTURER_SPECIFIC (0x72), a Z-Wave device command class.
	 * \ingroup CommandClass
	 */
	class ManufacturerSpecific: public CommandClass
	{
	public:
		static CommandClass* Create( uint32 const _homeId, uint8 const _nodeId ){ return new ManufacturerSpecific( _homeId, _nodeId ); }
		virtual ~ManufacturerSpecific(){ }

		static uint8 const StaticGetCommandClassId(){ return 0x72; }
		static string const StaticGetCommandClassName(){ return "COMMAND_CLASS_MANUFACTURER_SPECIFIC"; }

		// From CommandClass
		virtual bool RequestState( uint32 const _requestFlags, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		virtual bool RequestValue( uint32 const _requestFlags, uint16 const _index, uint8 const _instance, Driver::MsgQueue const _queue ) override;
		virtual uint8 const GetCommandClassId() const override { return StaticGetCommandClassId(); }
		virtual string const GetCommandClassName() const override { return StaticGetCommandClassName(); }
		virtual bool HandleMsg( uint8 const* _data, uint32 const _length, uint32 const _instance = 1 ) override;
		virtual uint8 GetMaxVersion() override { return 2; }


		void SetProductDetails( uint16 _manufacturerId, uint16 _productType, uint16 _productId );
		bool LoadConfigXML();
		
		void ReLoadConfigXML();

		void setLatestConfigRevision(uint32 rev);
		void setFileConfigRevision(uint32 rev);
		void setLoadedConfigRevision(uint32 rev);



	protected:
		virtual void CreateVars( uint8 const _instance ) override ;


	private:
		ManufacturerSpecific( uint32 const _homeId, uint8 const _nodeId );

		uint32 m_fileConfigRevision;
		uint32 m_loadedConfigRevision;
		uint32 m_latestConfigRevision;
	};

} // namespace OpenZWave

#endif

