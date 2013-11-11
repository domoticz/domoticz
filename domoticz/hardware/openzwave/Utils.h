//-----------------------------------------------------------------------------
//
//	Utils.h
//
//	Miscellaneous helper functions
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

#ifndef _Utils_H
#define _Utils_H

#include <string> 
#include <locale> 
#include <algorithm> 

namespace OpenZWave
{
	/**
	 * Convert a string to all upper-case.
	 * \param _str the string to be converted.
	 * \return the upper-case string.
	 * \see ToLower, Trim
	 */
	string ToUpper( string const& _str );

	/**
	 * Convert a string to all lower-case.
	 * \param _str the string to be converted.
	 * \return the lower-case string.
	 * \see ToUpper, Trim
	 */
	string ToLower( string const& _str );

} // namespace OpenZWave

#endif



