//-----------------------------------------------------------------------------
//
//	FileOps.h
//
//	Cross-platform File Operations
//
//	Copyright (c) 2012 Greg Satz <satz@iranger.com>
//	All rights reserved.
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
#ifndef _FileOps_H
#define _FileOps_H

#include <stdarg.h>
#include <string>
#include "Defs.h"

namespace OpenZWave
{
	namespace Internal
	{
		namespace Platform
		{
			class FileOpsImpl;

			/** \brief Implements platform-independent File Operations.
			 * \ingroup Platform
			 */
			class FileOps
			{
				public:
					/**
					 * Create a FileOps cross-platform singleton.
					 * \return a pointer to the file operations object.
					 * \see Destroy.
					 */
					static FileOps* Create();

					/**
					 * Destroys the FileOps singleton.
					 * \see Create.
					 */
					static void Destroy();

					/**
					 * FolderExists. Check for the existence of a folder.
					 * \param string. Folder name.
					 * \return Bool value indicating existence.
					 */
					static bool FolderExists(const string &_folderName);

					/**
					 * FileExists. Check for the existence of a file.
					 * \param string. file name.
					 * \return Bool value indicating existence.
					 */
					static bool FileExists(const string &_fileName);

					/**
					 * FileWriteable. Check if we can write to a file.
					 * \param string. file name.
					 * \return Bool value indicating write permissions.
					 */
					static bool FileWriteable(const string &_fileName);

					/**
					 * FileRotate. Rotate a File
					 * \param string. file name.
					 * \return Bool value indicating write permissions.
					 */
					static bool FileRotate(const string &_fileName);

					/**
					 * FileCopy. Copy a File
					 * \param string. source file name.
					 * \param string. destination file name
					 * \return Bool value indicating success.
					 */
					static bool FileCopy(const string &_fileName, const string &_destinationfile);

					/**
					 * FolderCreate. Create a Folder
					 * \param string. folder name
					 * \return Bool value indicating success.
					 */
					static bool FolderCreate(const string &_folderName);

				private:
					FileOps();
					~FileOps();

					static FileOpsImpl* m_pImpl;					// Pointer to an object that encapsulates the platform-specific implementation of the FileOps.
					static FileOps* s_instance;
			};
		} // namespace Platform
	} // namespace Internal
} // namespace OpenZWave

#endif //_FileOps_H

