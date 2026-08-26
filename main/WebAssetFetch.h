#pragma once

#include <string>

/* Server-side download of a web asset (see WebAssets.h). A stylesheet also has
   every url() reference it makes downloaded and rewritten to the stored copy, so
   nothing is pulled from a CDN at runtime. */
namespace WebAssetFetch
{
	/* Stores szURL as www/assets/<szName> plus the files it references, and
	   records the URL so it can be fetched again. szName must already have
	   passed IsSafeWebAssetName() and IsAllowedWebAssetType(). */
	bool Install(const std::string& szName, const std::string& szURL, std::string& szError);

	// Removes the companion files www/assets/<szName> brought in, plus its metadata row.
	void Forget(const std::string& szName);
} // namespace WebAssetFetch
