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
#ifndef WIN32
#ifndef WINRT
#ifdef DEBUG
#include <execinfo.h>
#include <cxxabi.h>
#endif
#endif
#endif

namespace OpenZWave
{
	namespace Internal
	{
		/**
		 * Convert a string to all upper-case.
		 * \param _str the string to be converted.
		 * \return the upper-case string.
		 * \see ToLower, Trim
		 */
		std::string ToUpper(string const& _str);

		/**
		 * Convert a string to all lower-case.
		 * \param _str the string to be converted.
		 * \return the lower-case string.
		 * \see ToUpper, Trim
		 */
		std::string ToLower(string const& _str);

		/**
		 * Split a String into a Vector, separated by separators
		 * \param lst the vector to store the results in
		 * \param input the input string to split
		 * \param separators a string containing a list of valid separators
		 * \param remove_empty if after splitting a string, the any of the results are a empty string, should we preserve them or not
		 */
		void split(std::vector<std::string>& lst, const std::string& input, const std::string& separators, bool remove_empty = true);

		/**
		 * remove all Whitespace from of a string.
		 * \param s the string to trim
		 * \return the trimmed string
		 */
		std::string &removewhitespace(std::string &s);

		/**
		 * @brief Left Trim
		 *
		 * Trims whitespace from the left end of the provided std::string
		 *
		 * @param[out] s The std::string to trim
		 *
		 * @return The modified std::string&
		 */
		std::string& ltrim(std::string& s);

		/**
		 * @brief Right Trim
		 *
		 * Trims whitespace from the right end of the provided std::string
		 *
		 * @param[out] s The std::string to trim
		 *
		 * @return The modified std::string&
		 */
		std::string& rtrim(std::string& s);

		/**
		 * @brief Trim
		 *
		 * Trims whitespace from both ends of the provided std::string
		 *
		 * @param[out] s The std::string to trim
		 *
		 * @return The modified std::string&
		 */
		std::string& trim(std::string& s);

		void PrintHex(std::string prefix, uint8_t const *data, uint32 const length);
		string PktToString(uint8 const *data, uint32 const length);

		struct LockGuard
		{
				LockGuard(Internal::Platform::Mutex* mutex) :
						_ref(mutex)
				{
					//std::cout << "Locking" << std::endl;
					_ref->Lock();
				}
				;

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
				LockGuard& operator =(LockGuard const&);

				Internal::Platform::Mutex* _ref;
		};

		string ozwdirname(string);

		string intToString(int x);

		const char* rssi_to_string(uint8 _data);

#ifndef WIN32
#ifndef WINRT
#ifdef DEBUG
		class StackTraceGenerator
		{
		private:

		    //  this is a pure utils class
		    //  cannot be instantiated
		    //
		    StackTraceGenerator() = delete;
		    StackTraceGenerator(const StackTraceGenerator&) = delete;
		    StackTraceGenerator& operator=(const StackTraceGenerator&) = delete;
		    ~StackTraceGenerator() = delete;

		public:

		    static std::vector<std::string> GetTrace()
		    {
		        //  record stack trace upto 128 frames
		        int callstack[128] = {};

		        // collect stack frames
		        int  frames = backtrace((void**) callstack, 5);

		        // get the human-readable symbols (mangled)
		        char** strs = backtrace_symbols((void**) callstack, frames);

		        std::vector<std::string> stackFrames;
		        stackFrames.reserve(frames);

		        for (int i = 2; i < frames; ++i)
		        {
		            char functionSymbol[1024] = {};
		            char moduleName[1024] = {};
		            int  offset = 0;
		            char addr[48] = {};

		            /*

		             Typically this is how the backtrace looks like:

		             0   <app/lib-name>     0x0000000100000e98 _Z5tracev + 72
		             1   <app/lib-name>     0x00000001000015c1 _ZNK7functorclEv + 17
		             2   <app/lib-name>     0x0000000100000f71 _Z3fn0v + 17
		             3   <app/lib-name>     0x0000000100000f89 _Z3fn1v + 9
		             4   <app/lib-name>     0x0000000100000f99 _Z3fn2v + 9
		             5   <app/lib-name>     0x0000000100000fa9 _Z3fn3v + 9
		             6   <app/lib-name>     0x0000000100000fb9 _Z3fn4v + 9
		             7   <app/lib-name>     0x0000000100000fc9 _Z3fn5v + 9
		             8   <app/lib-name>     0x0000000100000fd9 _Z3fn6v + 9
		             9   <app/lib-name>     0x0000000100001018 main + 56
		             10  libdyld.dylib      0x00007fff91b647e1 start + 0

		             */

		            // split the string, take out chunks out of stack trace
		            // we are primarily interested in module, function and address
		            sscanf(strs[i], "%*s %s %s %s %*s %d",
		                   moduleName, addr, functionSymbol, &offset);

		            int   validCppName = 0;
		            //  if this is a C++ library, symbol will be demangled
		            //  on success function returns 0
		            //
		            char* functionName = abi::__cxa_demangle(functionSymbol,
		                                                     NULL, 0, &validCppName);

		            char stackFrame[4096] = {};
		            if (validCppName == 0) // success
		            {
		                sprintf(stackFrame, "(%s)\t0x%s — %s + %d",
		                        moduleName, addr, functionName, offset);
		            }
		            else
		            {
		                //  in the above traceback (in comments) last entry is not
		                //  from C++ binary, last frame, libdyld.dylib, is printed
		                //  from here
		                sprintf(stackFrame, "(%s)\t0x%s — %s + %d",
		                        moduleName, addr, functionName, offset);
		            }

		            if (functionName)
		            {
		                free(functionName);
		            }
		            Log::Write(LogLevel_Warning, "Stack: %s", stackFrame);
		            std::string frameStr(stackFrame);
		            stackFrames.push_back(frameStr);
		        }
		        free(strs);

		        return stackFrames;
		    }
		};
#endif
#endif
#endif
	} // namespace Internal
} // namespace OpenZWave

/* keep this outside of the namespace */
#if (defined _WINDOWS || defined WIN32 || defined _MSC_VER) && (!defined MINGW && !defined __MINGW32__ && !defined __MINGW64__)
#include <ctime>
struct tm *localtime_r(const time_t *_clock, struct tm *_result);
#endif

#endif

