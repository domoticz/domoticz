#include "stdafx.h"
#include "WebAssetFetch.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "Helper.h"
#include "Logger.h"
#include "SQLHelper.h"
#include "WebAssets.h"
#include "../httpclient/HTTPClient.h"

namespace WebAssetFetch
{
	namespace
	{
		/* An admin-typed URL points the server at whatever it can reach, so the
		   reach is bounded rather than trusted: http/https only, a timeout per
		   request, caps on file count and total size, every fetch logged. */
		const int WEBASSET_MAX_COMPANIONS = 32;
		const size_t WEBASSET_MAX_TOTAL_SIZE = 16 * 1024 * 1024; // for the whole bundle
		const long WEBASSET_HTTP_TIMEOUT = 20;			 // seconds per request

		const char* LOGTAG = "WebAssetFetch";

		struct _tPendingAsset
		{
			std::string szName;
			std::string szContent;
		};

		struct _tDownloadContext
		{
			std::string szPrefix;
			std::string szBaseScheme;
			std::string szBaseAuthority;
			std::string szBaseDir; // directory part of the stylesheet URL, with trailing '/'
			/* Collected in memory and written only once the whole bundle
			   downloaded, so a failed install cannot leave a stylesheet pointing
			   at files that are not there. */
			std::vector<_tPendingAsset> assets;
			/* Keyed on the stored filename, not the URL: the same font is often
			   referenced twice with a different cache-buster ("?v=1", "?#iefix"). */
			std::map<std::string, std::string> storedNames;
			size_t totalSize = 0;
			std::string szError;
		};

		bool HttpGet(const std::string& szURL, std::string& szContent)
		{
			// Audit trail: log before the request goes out.
			_log.Log(LOG_STATUS, "%s: fetching %s", LOGTAG, szURL.c_str());

			std::vector<unsigned char> vResponse;
			const std::vector<std::string> vExtraHeaders;
			if (!HTTPClient::GETBinary(szURL, vExtraHeaders, vResponse, WEBASSET_HTTP_TIMEOUT))
			{
				_log.Log(LOG_ERROR, "%s: could not fetch %s", LOGTAG, szURL.c_str());
				return false;
			}
			szContent.assign(vResponse.begin(), vResponse.end());
			return true;
		}

		size_t FindCaseInsensitive(const std::string& szHaystack, const std::string& szNeedle, size_t iFrom)
		{
			auto itt = std::search(szHaystack.begin() + iFrom, szHaystack.end(), szNeedle.begin(), szNeedle.end(),
					       [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
			if (itt == szHaystack.end())
				return std::string::npos;
			return static_cast<size_t>(itt - szHaystack.begin());
		}

		// A URL goes to libcurl and to the log verbatim: no control characters or whitespace.
		bool IsCleanURL(const std::string& szURL)
		{
			if (szURL.empty() || (szURL.size() > 1024))
				return false;
			for (const char c : szURL)
			{
				const unsigned char u = static_cast<unsigned char>(c);
				if ((u <= 0x20) || (u == 0x7F))
					return false;
			}
			return true;
		}

		// Returns false for any scheme but http(s), so file:// never reaches libcurl.
		bool ParseHttpURL(const std::string& szURL, std::string& szScheme, std::string& szAuthority, std::string& szPath)
		{
			const size_t iSchemeEnd = szURL.find("://");
			if (iSchemeEnd == std::string::npos)
				return false;
			szScheme = szURL.substr(0, iSchemeEnd);
			stdlower(szScheme);
			if ((szScheme != "http") && (szScheme != "https"))
				return false;

			const size_t iAuthStart = iSchemeEnd + 3;
			const size_t iPathStart = szURL.find('/', iAuthStart);
			if (iPathStart == std::string::npos)
			{
				szAuthority = szURL.substr(iAuthStart);
				szPath = "/";
			}
			else
			{
				szAuthority = szURL.substr(iAuthStart, iPathStart - iAuthStart);
				szPath = szURL.substr(iPathStart);
			}
			if (szAuthority.empty())
				return false;
			// An authority may hold user:pass@host:port, but not whitespace or a path separator.
			if (szAuthority.find_first_of(" \t\r\n\\") != std::string::npos)
				return false;
			return true;
		}

		// Collapses "." and ".." so a reference cannot climb out of its own site.
		std::string NormalisePath(const std::string& szPath)
		{
			std::vector<std::string> segments;
			size_t iPos = 0;
			while (iPos <= szPath.size())
			{
				const size_t iNext = szPath.find('/', iPos);
				const std::string szSeg = (iNext == std::string::npos) ? szPath.substr(iPos) : szPath.substr(iPos, iNext - iPos);
				if (szSeg == "..")
				{
					if (!segments.empty())
						segments.pop_back();
				}
				else if ((szSeg != ".") && !szSeg.empty())
					segments.push_back(szSeg);

				if (iNext == std::string::npos)
					break;
				iPos = iNext + 1;
			}

			std::string szOut;
			for (const auto& szSeg : segments)
				szOut += "/" + szSeg;
			if (szOut.empty())
				szOut = "/";
			return szOut;
		}

		/* Resolves a CSS url(...) reference against the stylesheet's own URL.
		   Anything that is not http(s) is rejected, and the caller then leaves
		   the reference as it is. */
		bool ResolveReference(const _tDownloadContext& ctx, const std::string& szRef, std::string& szOut, std::string& szFileName)
		{
			if (!IsCleanURL(szRef))
				return false;

			// ?query and #fragment are part of the request but never of the filename.
			std::string szPathPart = szRef;
			std::string szSuffix;
			const size_t iCut = szPathPart.find_first_of("?#");
			if (iCut != std::string::npos)
			{
				szSuffix = szPathPart.substr(iCut);
				szPathPart = szPathPart.substr(0, iCut);
			}
			// The fragment is browser-side only (the "#iefix" trick), never send it.
			const size_t iFrag = szSuffix.find('#');
			if (iFrag != std::string::npos)
				szSuffix = szSuffix.substr(0, iFrag);
			if (szSuffix == "?")
				szSuffix.clear();
			if (szPathPart.empty())
				return false;

			std::string szScheme;
			std::string szAuthority;
			std::string szPath;

			if (szPathPart.compare(0, 2, "//") == 0)
			{
				// Protocol-relative: inherits the stylesheet's scheme.
				if (!ParseHttpURL(ctx.szBaseScheme + ":" + szPathPart, szScheme, szAuthority, szPath))
					return false;
			}
			else
			{
				// A bare colon before the first slash means the reference has a scheme of its own.
				const size_t iColon = szPathPart.find(':');
				const size_t iSlash = szPathPart.find('/');
				if ((iColon != std::string::npos) && ((iSlash == std::string::npos) || (iColon < iSlash)))
				{
					if (!ParseHttpURL(szPathPart, szScheme, szAuthority, szPath))
						return false;
				}
				else
				{
					szScheme = ctx.szBaseScheme;
					szAuthority = ctx.szBaseAuthority;
					szPath = (szPathPart.front() == '/') ? szPathPart : (ctx.szBaseDir + szPathPart);
				}
			}

			szPath = NormalisePath(szPath);
			const size_t iSlash = szPath.rfind('/');
			const std::string szBase = (iSlash == std::string::npos) ? szPath : szPath.substr(iSlash + 1);
			if (szBase.empty())
				return false;

			/* Stored as "<stem>-<remote basename>", so two bundles shipping a
			   file of the same name stay apart. */
			std::string szSanitised;
			for (const char c : szBase)
			{
				if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '.') || (c == '-') || (c == '_'))
					szSanitised += c;
				else
					szSanitised += '_';
			}
			while (!szSanitised.empty() && (szSanitised.front() == '.'))
				szSanitised.erase(szSanitised.begin());
			if (szSanitised.empty())
				return false;

			szFileName = ctx.szPrefix + "-" + szSanitised;
			if (szFileName.size() > 128)
				szFileName = szFileName.substr(0, 128);
			if (!IsSafeWebAssetName(szFileName) || !IsAllowedWebAssetType(szFileName))
				return false;

			szOut = szScheme + "://" + szAuthority + szPath + szSuffix;
			return true;
		}

		/* Returns false when this reference should be left as it is; ctx.szError
		   is set only for a failure that must abort the whole install. */
		bool StoreReference(_tDownloadContext& ctx, const std::string& szRef, std::string& szStoredName)
		{
			std::string szAbsURL;
			std::string szFileName;
			if (!ResolveReference(ctx, szRef, szAbsURL, szFileName))
				return false;

			/* Not followed: an @import's own references are relative to where it
			   came from, so vendoring it without rewriting those too would only
			   look like it worked. */
			std::string szLower = szFileName;
			stdlower(szLower);
			if ((szLower.size() > 4) && (szLower.compare(szLower.size() - 4, 4, ".css") == 0))
			{
				_log.Log(LOG_STATUS, "%s: not following nested stylesheet %s", LOGTAG, szAbsURL.c_str());
				return false;
			}

			if (ctx.storedNames.count(szFileName) != 0)
			{
				// Already downloaded in this pass.
				szStoredName = szFileName;
				return true;
			}

			if (static_cast<int>(ctx.assets.size()) >= WEBASSET_MAX_COMPANIONS)
			{
				ctx.szError = "Asset references too many files";
				return false;
			}

			std::string szContent;
			if (!HttpGet(szAbsURL, szContent))
				return false; // a missing legacy font should not fail the install
			if (szContent.empty())
				return false;
			if (szContent.size() > WEB_ASSET_MAX_SIZE)
			{
				_log.Log(LOG_ERROR, "%s: %s is too large, skipping", LOGTAG, szAbsURL.c_str());
				return false;
			}
			if (ctx.totalSize + szContent.size() > WEBASSET_MAX_TOTAL_SIZE)
			{
				ctx.szError = "Asset bundle is larger than allowed";
				return false;
			}

			ctx.totalSize += szContent.size();
			ctx.assets.push_back({ szFileName, szContent });
			ctx.storedNames[szFileName] = szAbsURL;
			szStoredName = szFileName;
			return true;
		}

		/* A reference we could not store is copied through untouched: the stored
		   CSS then holds one that does not resolve, which is how those legacy
		   fallbacks behave in a modern browser anyway. */
		bool RewriteStylesheet(_tDownloadContext& ctx, const std::string& szCss, std::string& szOut)
		{
			szOut.clear();
			szOut.reserve(szCss.size());

			size_t iPos = 0;
			while (true)
			{
				const size_t iFound = FindCaseInsensitive(szCss, "url(", iPos);
				if (iFound == std::string::npos)
				{
					szOut.append(szCss, iPos, std::string::npos);
					break;
				}
				const size_t iOpen = iFound + 4;
				const size_t iClose = szCss.find(')', iOpen);
				if (iClose == std::string::npos)
				{
					szOut.append(szCss, iPos, std::string::npos);
					break;
				}

				szOut.append(szCss, iPos, iOpen - iPos); // everything up to and including "url("

				const std::string szRaw = szCss.substr(iOpen, iClose - iOpen);
				std::string szRef = szRaw;
				stdstring_trimws(szRef);
				char cQuote = 0;
				if ((szRef.size() >= 2) && ((szRef.front() == '"') || (szRef.front() == '\'')) && (szRef.back() == szRef.front()))
				{
					cQuote = szRef.front();
					szRef = szRef.substr(1, szRef.size() - 2);
					stdstring_trimws(szRef);
				}

				std::string szStoredName;
				if (StoreReference(ctx, szRef, szStoredName))
				{
					if (cQuote != 0)
						szOut += cQuote;
					szOut += szStoredName;
					if (cQuote != 0)
						szOut += cQuote;
				}
				else
				{
					if (!ctx.szError.empty())
						return false;
					szOut += szRaw;
					// A data: URI can be enormous, so keep the log line readable.
					const std::string szLogged = (szRef.size() > 120) ? (szRef.substr(0, 120) + "...") : szRef;
					_log.Log(LOG_STATUS, "%s: leaving reference '%s' as is", LOGTAG, szLogged.c_str());
				}

				szOut += ')';
				iPos = iClose + 1;
			}
			return true;
		}

		bool IsStylesheet(const std::string& szName)
		{
			std::string szLower = szName;
			stdlower(szLower);
			return ((szLower.size() > 4) && (szLower.compare(szLower.size() - 4, 4, ".css") == 0));
		}

		// "mdi.css" -> "mdi": the stem every companion file of this asset is named after.
		std::string AssetStem(const std::string& szName)
		{
			const size_t iDot = szName.find('.');
			return (iDot == std::string::npos) ? szName : szName.substr(0, iDot);
		}

		/* The stored list is the only record of which files came in with which
		   asset, so removing one bundle cannot take another one's files. */
		std::vector<std::string> LoadCompanions(const std::string& szName)
		{
			std::vector<std::string> companions;
			auto result = m_sql.safe_query("SELECT Companions FROM WebAssets WHERE (Name=='%q')", szName.c_str());
			if (result.empty() || result[0][0].empty())
				return companions;
			StringSplit(result[0][0], "\n", companions);
			return companions;
		}
	} // namespace

	bool Install(const std::string& szName, const std::string& szURL, std::string& szError)
	{
		szError.clear();

		if (!IsSafeWebAssetName(szName) || !IsAllowedWebAssetType(szName))
		{
			szError = "Invalid asset name";
			return false;
		}
		if (szURL.size() > 500)
		{
			szError = "URL too long"; // the column holds 500 characters
			return false;
		}
		if (!IsCleanURL(szURL))
		{
			szError = "The URL contains invalid characters";
			return false;
		}

		_tDownloadContext ctx;
		ctx.szPrefix = AssetStem(szName);
		if (ctx.szPrefix.empty())
		{
			szError = "Invalid asset name";
			return false;
		}
		std::string szBasePath;
		if (!ParseHttpURL(szURL, ctx.szBaseScheme, ctx.szBaseAuthority, szBasePath))
		{
			szError = "Only http:// and https:// URLs are supported";
			return false;
		}
		szBasePath = NormalisePath(szBasePath.substr(0, szBasePath.find_first_of("?#")));
		ctx.szBaseDir = szBasePath.substr(0, szBasePath.rfind('/') + 1);

		std::string szContent;
		if (!HttpGet(szURL, szContent))
		{
			szError = "Could not download " + szURL;
			return false;
		}
		if (szContent.empty())
		{
			szError = "The downloaded asset is empty";
			return false;
		}
		if (szContent.size() > WEB_ASSET_MAX_SIZE)
		{
			szError = "The downloaded asset is too large";
			return false;
		}

		if (IsStylesheet(szName))
		{
			std::string szRewritten;
			if (!RewriteStylesheet(ctx, szContent, szRewritten))
			{
				szError = ctx.szError.empty() ? "Could not process the stylesheet" : ctx.szError;
				return false;
			}
			szContent = szRewritten;
		}

		if (!EnsureWebAssetFolder())
		{
			szError = "Could not create the assets folder";
			return false;
		}

		const std::vector<std::string> previous = LoadCompanions(szName);
		auto existing = m_sql.safe_query("SELECT ID FROM WebAssets WHERE (Name=='%q')", szName.c_str());
		const bool bIsUpdate = !existing.empty();

		/* Companions first, the asset last, so an interrupted write leaves the
		   previous version working. A failed first install is rolled back; a
		   failed update is not, as those files still serve a working asset. */
		std::vector<std::string> written;
		bool bWriteOk = true;
		for (const auto& asset : ctx.assets)
		{
			if (!WriteWebAssetFile(asset.szName, asset.szContent, LOGTAG))
			{
				szError = "Could not store " + asset.szName;
				bWriteOk = false;
				break;
			}
			written.push_back(asset.szName);
		}
		if (bWriteOk && !WriteWebAssetFile(szName, szContent, LOGTAG))
		{
			szError = "Could not store " + szName;
			bWriteOk = false;
		}
		if (!bWriteOk)
		{
			if (!bIsUpdate)
			{
				for (const auto& szDone : written)
					RemoveWebAssetFile(szDone);
			}
			return false;
		}

		std::string szCompanions;
		for (const auto& asset : ctx.assets)
		{
			if (!szCompanions.empty())
				szCompanions += "\n";
			szCompanions += asset.szName;
		}

		if (!bIsUpdate)
		{
			m_sql.safe_query("INSERT INTO WebAssets (Name, SourceURL, Companions, LastUpdate) VALUES ('%q','%q','%q',datetime('now','localtime'))", szName.c_str(), szURL.c_str(),
					 szCompanions.c_str());
		}
		else
		{
			m_sql.safe_query("UPDATE WebAssets SET SourceURL='%q', Companions='%q', LastUpdate=datetime('now','localtime') WHERE (ID==%d)", szURL.c_str(), szCompanions.c_str(),
					 atoi(existing[0][0].c_str()));

			// A new version can reference fewer files; drop the ones it no longer needs.
			for (const auto& szOld : previous)
			{
				if (ctx.storedNames.count(szOld) != 0)
					continue;
				if (!IsSafeWebAssetName(szOld) || !IsAllowedWebAssetType(szOld))
					continue;
				RemoveWebAssetFile(szOld);
			}
		}

		_log.Log(LOG_STATUS, "%s: stored '%s' with %d companion file(s), %d bytes", LOGTAG, szName.c_str(), static_cast<int>(ctx.assets.size()), static_cast<int>(ctx.totalSize));
		return true;
	}

	void Forget(const std::string& szName)
	{
		const std::vector<std::string> companions = LoadCompanions(szName);
		int iRemoved = 0;
		for (const auto& szCompanion : companions)
		{
			// Re-validated: it comes back out of the database as a path component.
			if (!IsSafeWebAssetName(szCompanion) || !IsAllowedWebAssetType(szCompanion))
				continue;
			RemoveWebAssetFile(szCompanion);
			iRemoved++;
		}

		m_sql.safe_query("DELETE FROM WebAssets WHERE (Name=='%q')", szName.c_str());

		if (iRemoved != 0)
			_log.Log(LOG_STATUS, "%s: removed '%s' and %d companion file(s)", LOGTAG, szName.c_str(), iRemoved);
	}
} // namespace WebAssetFetch
