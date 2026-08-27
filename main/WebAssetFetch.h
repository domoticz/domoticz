#pragma once

#include <string>

namespace WebAssetFetch
{
	// szTitle is display metadata only, it never takes part in naming the stored file
	// or in deriving the class prefix. An empty title leaves any existing one alone.
	bool Install(const std::string& szName, const std::string& szURL, const std::string& szTitle, std::string& szError);

	void SetTitle(const std::string& szName, const std::string& szTitle);

	void Forget(const std::string& szName);

	// True if szCandidateName is already recorded as the main asset or a companion of
	// some library other than szOwnerName. Anything writing or deleting a file by name
	// must check this first, a sanitised-name collision would otherwise let one library
	// clobber or delete a file that belongs to another.
	bool IsNameOwnedByOther(const std::string& szCandidateName, const std::string& szOwnerName);

	// Downloading a library can take a while (up to 33 fetches, each with its own
	// timeout), so it is run on a background thread rather than on the web request.
	// StartInstall returns a job id to poll with GetJobStatus; it refuses to start
	// while another job for the same asset name is still running.
	struct JobStatus
	{
		bool bRunning = false;
		bool bSuccess = false;
		std::string szName;
		std::string szError;
	};
	std::string StartInstall(const std::string& szName, const std::string& szURL, const std::string& szTitle, std::string& szError);
	bool GetJobStatus(const std::string& szJobID, JobStatus& status);
	bool IsInstallRunning(const std::string& szName);

	// Waits for any running install jobs; call before the database goes away.
	void Shutdown();
} // namespace WebAssetFetch
