//-----------------------------------------------------------------------------
//
//	ValueIDIndexes.h
//
//	List of all Possible ValueID Indexes in OZW
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

/* This file is includes the pre-processed ValueIDIndexesDefines.h to avoid problems
 * with MSVC not supporting enough arguments with Macro's.
 * If you are adding a ValueID, you should add its index ENUM to ValuIDIndexDefines.def and the run
 * 'make updateIndexDefines' to regenerate the the ValueIDIndexDefines.h file
 */

#include <cstring>

#ifndef _ValueIDIndexes_H
#define _ValueIDIndexes_H
namespace OpenZWave
{

#ifdef _MSC_VER
#define strncpy(x, y, z) strncpy_s(x, sizeof(x), y, sizeof(x)-1)
#endif

#include "ValueIDIndexesDefines.h"

}

#endif
