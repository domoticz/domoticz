#pragma once

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#ifdef WIN32
#include <windows.h>
#include "dirent_windows.h"
#else
#include <dirent.h>
#endif

#include "Helper.h"
#include "Logger.h"

extern std::string szWWWFolder;

static const size_t WEB_ASSET_MAX_SIZE = 8 * 1024 * 1024;

inline bool IsSafeWebAssetName(const std::string& szName)
{
	if (szName.empty() || (szName.size() > 128))
		return false;
	if (szName.front() == '.')
		return false;
	if (szName.find("..") != std::string::npos)
		return false;

	for (const char c : szName)
	{
		if ((c >= '0') && (c <= '9'))
			continue;
		if ((c >= 'a') && (c <= 'z'))
			continue;
		if ((c >= 'A') && (c <= 'Z'))
			continue;
		if ((c == '.') || (c == '-') || (c == '_'))
			continue;
		return false;
	}
	return true;
}

inline bool IsAllowedWebAssetType(const std::string& szName)
{
	std::string szLower = szName;
	stdlower(szLower);
	const std::vector<std::string> allowed = { ".css", ".woff2", ".woff", ".ttf", ".otf", ".eot" };
	for (const auto& ext : allowed)
	{
		if ((szLower.size() > ext.size()) &&
			(szLower.compare(szLower.size() - ext.size(), ext.size(), ext) == 0))
			return true;
	}
	return false;
}

inline bool WebAssetDirExists(const std::string& szPath)
{
	DIR* pDir = opendir(szPath.c_str());
	if (pDir == nullptr)
		return false;
	closedir(pDir);
	return true;
}

inline std::string WebAssetFolder()
{
	return szWWWFolder + "/assets";
}

inline bool EnsureWebAssetFolder()
{
	const std::string szFolder = WebAssetFolder();
	if (WebAssetDirExists(szFolder))
		return true;
	mkdir_deep(szFolder.c_str(), 0755);
	return WebAssetDirExists(szFolder);
}

// Writes szContent to a temp file next to the final destination, without touching any
// existing file. Callers may stage several files this way and only commit them (see
// CommitWebAssetFile) once every one of them has staged successfully, so a failure part
// way through an install never leaves some files replaced and others left as they were.
inline bool StageWebAssetFile(const std::string& szName, const std::string& szContent, const char* szLogTag, std::string& szTmpFile)
{
	if (!EnsureWebAssetFolder())
	{
		_log.Log(LOG_ERROR, "%s: could not create assets folder %s", szLogTag, WebAssetFolder().c_str());
		return false;
	}

	const std::string szFile = WebAssetFolder() + "/" + szName;
	szTmpFile = szFile + "." + GenerateUUID() + ".tmp";

	std::ofstream outfile(szTmpFile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if (!outfile.is_open())
	{
		_log.Log(LOG_ERROR, "%s: could not write %s", szLogTag, szTmpFile.c_str());
		return false;
	}
	outfile.write(szContent.data(), static_cast<std::streamsize>(szContent.size()));
	outfile.flush();
	if (!outfile.good())
	{
		outfile.close();
		_log.Log(LOG_ERROR, "%s: write failed for %s", szLogTag, szTmpFile.c_str());
		std::remove(szTmpFile.c_str());
		return false;
	}
	outfile.close();
	if (!outfile.good())
	{
		_log.Log(LOG_ERROR, "%s: close failed for %s", szLogTag, szTmpFile.c_str());
		std::remove(szTmpFile.c_str());
		return false;
	}
	return true;
}

inline bool CommitWebAssetFile(const std::string& szName, const std::string& szTmpFile, const char* szLogTag)
{
	const std::string szFile = WebAssetFolder() + "/" + szName;
#ifdef WIN32
	bool bRenameOk = (MoveFileExA(szTmpFile.c_str(), szFile.c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
#else
	bool bRenameOk = (std::rename(szTmpFile.c_str(), szFile.c_str()) == 0);
#endif
	if (!bRenameOk)
	{
		_log.Log(LOG_ERROR, "%s: could not replace %s", szLogTag, szFile.c_str());
		std::remove(szTmpFile.c_str());
		return false;
	}
	return true;
}

// Moves any existing szName out of the way into szBackupFile before it gets overwritten,
// so a failure further along a multi-file commit can put it back with RestoreWebAssetFile.
// szBackupFile is left empty when there was nothing to preserve.
inline bool BackupWebAssetFile(const std::string& szName, std::string& szBackupFile, const char* szLogTag)
{
	szBackupFile.clear();
	const std::string szFile = WebAssetFolder() + "/" + szName;
	if (!file_exist(szFile.c_str()))
		return true;

	szBackupFile = szFile + "." + GenerateUUID() + ".bak";
#ifdef WIN32
	bool bRenameOk = (MoveFileExA(szFile.c_str(), szBackupFile.c_str(), MOVEFILE_REPLACE_EXISTING) != 0);
#else
	bool bRenameOk = (std::rename(szFile.c_str(), szBackupFile.c_str()) == 0);
#endif
	if (!bRenameOk)
	{
		_log.Log(LOG_ERROR, "%s: could not back up %s before replacing it", szLogTag, szFile.c_str());
		szBackupFile.clear();
		return false;
	}
	return true;
}

// Undoes a commit: puts back the backup made by BackupWebAssetFile (or, if there was
// none, removes whatever CommitWebAssetFile put in that name's place) so a partially
// failed multi-file commit does not leave a library on a mix of old and new files.
inline void RestoreWebAssetFile(const std::string& szName, const std::string& szBackupFile)
{
	const std::string szFile = WebAssetFolder() + "/" + szName;
	if (szBackupFile.empty())
	{
		std::remove(szFile.c_str());
		return;
	}
#ifdef WIN32
	MoveFileExA(szBackupFile.c_str(), szFile.c_str(), MOVEFILE_REPLACE_EXISTING);
#else
	std::rename(szBackupFile.c_str(), szFile.c_str());
#endif
}

inline void DiscardWebAssetBackup(const std::string& szBackupFile)
{
	if (!szBackupFile.empty())
		std::remove(szBackupFile.c_str());
}

inline void DiscardStagedWebAssetFile(const std::string& szTmpFile)
{
	if (!szTmpFile.empty())
		std::remove(szTmpFile.c_str());
}

inline bool WriteWebAssetFile(const std::string& szName, const std::string& szContent, const char* szLogTag)
{
	std::string szTmpFile;
	if (!StageWebAssetFile(szName, szContent, szLogTag, szTmpFile))
		return false;
	return CommitWebAssetFile(szName, szTmpFile, szLogTag);
}

inline bool RemoveWebAssetFile(const std::string& szName)
{
	const std::string szFile = WebAssetFolder() + "/" + szName;
	if (!file_exist(szFile.c_str()))
		return true;
	return (std::remove(szFile.c_str()) == 0);
}
