#pragma once

#include <string>

namespace WebAssetFetch
{
	// szTitle is display metadata only, it never takes part in naming the stored file
	// or in deriving the class prefix. An empty title leaves any existing one alone.
	bool Install(const std::string& szName, const std::string& szURL, const std::string& szTitle, std::string& szError);

	void SetTitle(const std::string& szName, const std::string& szTitle);

	void Forget(const std::string& szName);
} // namespace WebAssetFetch
