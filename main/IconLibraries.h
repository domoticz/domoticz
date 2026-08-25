#pragma once

#include <string>

/*
 * Icon font libraries (Material Design Icons and friends).
 *
 * Domoticz fetches the library stylesheet itself, pulls down every font it
 * references, stores all of it in www/assets/ and rewrites the stylesheet to
 * point at the stored copies. Nothing is loaded from a CDN at runtime, so the
 * front-end keeps working on an installation without internet access and the
 * browser never talks to a third party.
 *
 * The bookkeeping lives in the IconLibraries table; the files themselves are
 * ordinary web assets (see WebAssets.h) named after the library prefix:
 *
 *   www/assets/mdi.css
 *   www/assets/mdi-materialdesignicons-webfont.woff2
 *
 * so a library can be refreshed in place and removed again without a manifest.
 */
namespace IconLibraries
{
	/* The prefix is used both as a filename component and as the CSS class
	   prefix the front-end matches on, so it is kept to lowercase
	   alphanumerics. */
	bool IsValidPrefix(const std::string& szPrefix);

	/* Downloads szURL plus every font it references, stores everything under
	   www/assets/ and inserts or updates the IconLibraries row for szPrefix.
	   Returns false with a human-readable reason in szError. */
	bool Install(const std::string& szName, const std::string& szPrefix, const std::string& szURL, std::string& szError);

	/* Re-downloads the library with the given row ID from its stored SourceURL. */
	bool Refresh(int iID, std::string& szError);

	/* Removes the row and every file the library stored in www/assets/. */
	bool Remove(int iID, std::string& szError);
} // namespace IconLibraries
