//-----------------------------------------------------------------------------
//
//	FatalErrorException.h
//
//	Exception Handling Code
//
//	Copyright (c) 2014 Justin Hammond <justin@dynam.ac>
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

#ifndef _FatalErrorException_H
#define _FatalErrorException_H

#include <stdexcept>
#include <iostream>
#include <string>
#include <sstream>



namespace OpenZWave
{
	/** \brief Exception Handling Interface.
	 *
	 * This class is for exporting errors etc when using the OpenZWave API. It can report incorrect API usage
	 * (such as passing incorrect ValueID's to the Manager::SetValue methods) or
	 */

OPENZWAVE_EXPORT_WARNINGS_OFF
	class OPENZWAVE_EXPORT OZWException : public std::runtime_error
	{
		public:
			enum ExceptionType {
				OZWEXCEPTION_OPTIONS,
				OZWEXCEPTION_CONFIG,
				OZWEXCEPTION_INVALID_HOMEID = 100,
				OZWEXCEPTION_INVALID_VALUEID,
				OZWEXCEPTION_CANNOT_CONVERT_VALUEID,
				OZWEXCEPTION_SECURITY_FAILED
			};

			//-----------------------------------------------------------------------------
			// Construction
			//-----------------------------------------------------------------------------
			OZWException(std::string file, int line, ExceptionType exitCode, std::string msg) :
				std::runtime_error(OZWException::GetExceptionText(file, line, exitCode, msg)),
				m_exitCode(exitCode),
				m_file(file),
				m_line(line),
				m_msg(msg)
			{
			}

			~OZWException() throw()
			{
			}

			//-----------------------------------------------------------------------------
			// Accessor methods
			//-----------------------------------------------------------------------------
			ExceptionType GetType() { return m_exitCode; }
			std::string GetFile() { return m_file; }
			uint32 GetLine() { return m_line; }
			std::string GetMsg() { return m_msg; }


		private:
			static std::string GetExceptionText(std::string file, int line, ExceptionType exitCode, std::string msg)
			{
				std::stringstream ss;
				ss << file.substr(file.find_last_of("/\\") + 1) << ":" << line;
				switch (exitCode) {
					case OZWEXCEPTION_OPTIONS:
						ss << " - OptionsError (" << exitCode << ") Msg: " << msg;
						break;
					case OZWEXCEPTION_CONFIG:
						ss << " - ConfigError (" << exitCode << ") Msg: " << msg;
						break;
					case OZWEXCEPTION_INVALID_HOMEID:
						ss << " - InvalidHomeIDError (" << exitCode << ") Msg: " << msg;
						break;
					case OZWEXCEPTION_INVALID_VALUEID:
						ss << " - InvalidValueIDError (" << exitCode << ") Msg: " << msg;
						break;
					case OZWEXCEPTION_CANNOT_CONVERT_VALUEID:
						ss << " - CannotConvertValueIDError (" << exitCode << ") Msg: " << msg;
						break;
					case OZWEXCEPTION_SECURITY_FAILED:
						ss << " - Security Initilization Failed (" << exitCode << ") Msg: " << msg;
						break;
				}
				return ss.str();
			}

			//-----------------------------------------------------------------------------
			// Member variables
			//-----------------------------------------------------------------------------
			ExceptionType   m_exitCode;
			std::string 	m_file;
			uint32			m_line;
			std::string 	m_msg;
	};
OPENZWAVE_EXPORT_WARNINGS_ON
}

#endif // _FatalErrorException_H
