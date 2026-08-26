#pragma once

#include <string>

namespace WebAssetFetch
{
	bool Install(const std::string& szName, const std::string& szURL, std::string& szError);

	void Forget(const std::string& szName);
} // namespace WebAssetFetch
