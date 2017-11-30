/*
 * Security.h
 *
 *  Created on: Mar 20, 2015
 *      Author: justinhammond
 */

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
