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

#include "platform/Mutex.h"
#include "platform/Log.h"

#include <string>
#include <locale>
#include <algorithm>
#include <sstream>
#include <vector>

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

	/**
	 * Split a String into a Vector, seperated by seperators
	 * \param lst the vector to store the results in
	 * \param input the input string to split
	 * \param seperators a string containing a list of valid seperators
	 * \param remove_empty if after spliting a string, the any of the results are a empty string, should we preseve them or not
	 */
	void split (std::vector<std::string>& lst, const std::string& input, const std::string& separators, bool remove_empty = true);

	/**
	 * Trim Whitespace from the start and end of a string.
	 * \param s the string to trim
	 * \return the trimmed string
	 */
	std::string &trim ( std::string &s );


	void PrintHex(std::string prefix, uint8_t const *data, uint32 const length);
	string PktToString(uint8 const *data, uint32 const length);

	struct LockGuard
	{
			LockGuard(Mutex* mutex) : _ref(mutex)
			{
				//std::cout << "Locking" << std::endl;
				_ref->Lock();
			};

			~LockGuard()
			{
#if 0
				if (_ref->IsSignalled())
					std::cout << "Already Unlocked" << std::endl;
				else
					std::cout << "Unlocking" << std::endl;
#endif
				if (!_ref->IsSignalled())
					_ref->Unlock();
			}
			void Unlock()
			{
//				std::cout << "Unlocking" << std::endl;
				_ref->Unlock();
			}
		private:
			LockGuard(const LockGuard&);
			LockGuard& operator = ( LockGuard const& );


			Mutex* _ref;
	};



} // namespace OpenZWave

#endif



