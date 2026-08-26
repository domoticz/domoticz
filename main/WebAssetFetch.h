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
} // namespace WebAssetFetch
