//-----------------------------------------------------------------------------
//
//	ZWSecurity.h
//
//	Common Security/Encryption Routines
//
//	Copyright (c) 2015 Justin Hammond <justin@dynam.ac>
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

#ifndef SECURITY_H_
#define SECURITY_H_


#include <cstdio>
#include <string>
#include <string.h>
#include "Defs.h"
#include "Driver.h"

namespace OpenZWave
{
bool EncyrptBuffer( uint8 *m_buffer, uint8 m_length, Driver *driver, uint8 const _sendingNode, uint8 const _receivingNode, uint8 const m_nonce[8], uint8* e_buffer);
bool DecryptBuffer( uint8 *e_buffer, uint8 e_length, Driver *driver, uint8 const _sendingNode, uint8 const _receivingNode, uint8 const m_nonce[8], uint8* m_buffer );
bool GenerateAuthentication( uint8 const* _data, uint32 const _length, Driver *driver, uint8 const _sendingNode, uint8 const _receivingNode, uint8 *iv, uint8* _authentication);
enum SecurityStrategy
{
	SecurityStrategy_Essential = 0,
	SecurityStrategy_Supported
};
SecurityStrategy ShouldSecureCommandClass(uint8 CommandClass);

}


#endif /* SECURITY_H_ */
