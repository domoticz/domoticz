#include "stdafx.h"
#include "IconLibraries.h"

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

namespace IconLibraries
{
	namespace
	{
		/* ── Limits ──────────────────────────────────────────────────────────
		   This runs behind an admin-only endpoint that fetches a URL the admin
		   typed, which means the Domoticz process can be pointed at anything it
		   can reach -- including hosts on the local network that the browser
		   cannot reach itself. There is no way to make that safe while still
		   allowing "install from jsdelivr", so the damage is bounded instead:
		   only http/https is accepted, every request has its own timeout, and
		   the number of files and the total number of bytes we are willing to
		   store are capped. Every URL that is actually fetched is logged so an
		   administrator can see afterwards what the server pulled and from
		   where.

		   Note that libcurl is configured globally in HTTPClient to follow
		   redirects, so a hostile server can still bounce us to another
		   http(s) location. The caps and the audit log apply to whatever we
		   end up fetching; tightening redirect handling would mean changing
		   HTTPClient for every other consumer, which is out of scope here. */
		const int ICONLIB_MAX_ASSETS = 32;			      // font files per library
		const size_t ICONLIB_MAX_TOTAL_SIZE = 16 * 1024 * 1024;	      // 16 MiB for the whole library
		const long ICONLIB_HTTP_TIMEOUT = 20;			      // seconds per request

		const char* LOGTAG = "IconLibraries";

		/* One asset we decided to store: the name it gets in www/assets/ and
		   its content. Everything is collected in memory first and only
		   written once the whole library downloaded cleanly, so a failed
		   install cannot leave a stylesheet pointing at fonts that are not
		   there. The total is capped above, so this stays bounded. */
		struct _tPendingAsset
		{
			std::string szName;
			std::string szContent;
		};

		/* Working state for one install/refresh. */
		struct _tDownloadContext
		{
			std::string szPrefix;
			std::string szBaseScheme;
			std::string szBaseAuthority;
			std::string szBaseDir; // directory part of the CSS URL, with trailing '/'
			std::vector<_tPendingAsset> assets;
			/* Keyed on the name the file gets in www/assets/ rather than on the
			   URL it came from: a library commonly points at the same font
			   twice with a different cache-busting query ("?v=1" and
			   "?#iefix"), and those are one and the same download. */
			std::map<std::string, std::string> storedNames; // stored filename -> URL it came from
			size_t totalSize = 0;
			std::string szError;
		};

		bool HttpGet(const std::string& szURL, std::string& szContent)
		{
			/* Audit trail: an admin must be able to tell which URLs the server
			   was made to fetch, so log before the request goes out. */
			_log.Log(LOG_STATUS, "%s: fetching %s", LOGTAG, szURL.c_str());

			std::vector<unsigned char> vResponse;
			const std::vector<std::string> vExtraHeaders;
			if (!HTTPClient::GETBinary(szURL, vExtraHeaders, vResponse, ICONLIB_HTTP_TIMEOUT))
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

		/* Rejects control characters and whitespace anywhere in a URL. Such a
		   URL is handed to libcurl and written to the log verbatim, and neither
		   should have to cope with an embedded newline. */
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

		/* Splits an absolute http(s) URL. Returns false for every other scheme:
		   file://, ftp:// and friends are not something an icon library needs,
		   and refusing them here keeps them away from libcurl. */
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
			/* An authority may legitimately hold user:pass@host:port, but not
			   whitespace or a path separator we did not put there. */
			if (szAuthority.find_first_of(" \t\r\n\\") != std::string::npos)
				return false;
			return true;
		}

		/* Collapses "." and ".." so a relative font reference cannot climb out
		   of the site it came from. */
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
		   Only http/https results are produced; anything else (data:, about:,
		   an unknown scheme) is rejected so the caller leaves the reference
		   alone. */
		bool ResolveReference(const _tDownloadContext& ctx, const std::string& szRef, std::string& szOut, std::string& szFileName)
		{
			if (!IsCleanURL(szRef))
				return false;

			// Split off ?query and #fragment: they are part of the request but never of the filename.
			std::string szPathPart = szRef;
			std::string szSuffix;
			const size_t iCut = szPathPart.find_first_of("?#");
			if (iCut != std::string::npos)
			{
				szSuffix = szPathPart.substr(iCut);
				szPathPart = szPathPart.substr(0, iCut);
			}
			// The fragment is a browser-side thing (the "#iefix" trick), never send it.
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
				/* Anything with a scheme of its own must be http(s); a bare
				   colon before the first slash means there is one. */
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

			/* Filename in www/assets/: the library prefix plus the remote
			   basename, with anything unusual folded to '_'. Prefixing keeps
			   two libraries that ship a font of the same name apart, and makes
			   removing a library a matter of matching on "<prefix>-". */
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

		/* Downloads one referenced font and queues it for writing. Returns the
		   name it will have in www/assets/, or false when this particular
		   reference should be left as it is. ctx.szError is set only for
		   failures that must abort the whole install. */
		bool StoreReference(_tDownloadContext& ctx, const std::string& szRef, std::string& szStoredName)
		{
			std::string szAbsURL;
			std::string szFileName;
			if (!ResolveReference(ctx, szRef, szAbsURL, szFileName))
				return false;

			/* A nested stylesheet (an @import) is not followed: its own url()
			   references are relative to where it came from, so vendoring it
			   without rewriting those as well would only look like it worked.
			   Left as it is, and logged, so it stays visible. */
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

			if (static_cast<int>(ctx.assets.size()) >= ICONLIB_MAX_ASSETS)
			{
				ctx.szError = "Library references too many files";
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
			if (ctx.totalSize + szContent.size() > ICONLIB_MAX_TOTAL_SIZE)
			{
				ctx.szError = "Library is larger than allowed";
				return false;
			}

			ctx.totalSize += szContent.size();
			ctx.assets.push_back({ szFileName, szContent });
			ctx.storedNames[szFileName] = szAbsURL;
			szStoredName = szFileName;
			return true;
		}

		/* Walks the stylesheet, replacing every url(...) that we managed to
		   store with the local filename. References we could not or would not
		   store (an SVG fallback, a data: URI, a host we could not reach) are
		   copied through untouched: the stored CSS then holds a reference that
		   simply does not resolve, which is how those legacy fallbacks behave
		   in a modern browser anyway. */
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
	} // namespace

	bool IsValidPrefix(const std::string& szPrefix)
	{
		if (szPrefix.empty() || (szPrefix.size() > 32))
			return false;
		for (const char c : szPrefix)
		{
			if ((c >= '0') && (c <= '9'))
				continue;
			if ((c >= 'a') && (c <= 'z'))
				continue;
			return false;
		}
		return true;
	}

	bool Install(const std::string& szName, const std::string& szPrefix, const std::string& szURL, std::string& szError)
	{
		szError.clear();

		if (!IsValidPrefix(szPrefix))
		{
			szError = "Invalid library prefix";
			return false;
		}
		if (szName.empty() || (szName.size() > 100))
		{
			szError = "Invalid library name";
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
		ctx.szPrefix = szPrefix;
		std::string szBasePath;
		if (!ParseHttpURL(szURL, ctx.szBaseScheme, ctx.szBaseAuthority, szBasePath))
		{
			szError = "Only http:// and https:// URLs are supported";
			return false;
		}
		szBasePath = NormalisePath(szBasePath.substr(0, szBasePath.find_first_of("?#")));
		ctx.szBaseDir = szBasePath.substr(0, szBasePath.rfind('/') + 1);

		const std::string szCssFile = szPrefix + ".css";
		if (!IsSafeWebAssetName(szCssFile) || !IsAllowedWebAssetType(szCssFile))
		{
			szError = "Invalid library prefix";
			return false;
		}

		std::string szCss;
		if (!HttpGet(szURL, szCss))
		{
			szError = "Could not download the stylesheet";
			return false;
		}
		if (szCss.empty())
		{
			szError = "The stylesheet is empty";
			return false;
		}
		if (szCss.size() > WEB_ASSET_MAX_SIZE)
		{
			szError = "The stylesheet is too large";
			return false;
		}

		std::string szRewritten;
		if (!RewriteStylesheet(ctx, szCss, szRewritten))
		{
			szError = ctx.szError.empty() ? "Could not process the stylesheet" : ctx.szError;
			return false;
		}

		if (!EnsureWebAssetFolder())
		{
			szError = "Could not create the assets folder";
			return false;
		}

		/* Keyed on the prefix rather than the name: the prefix is what the
		   stored files and the icon classes are named after, so re-installing
		   the same prefix updates the existing library instead of creating a
		   second row pointing at the same files. */
		auto result = m_sql.safe_query("SELECT ID FROM IconLibraries WHERE (Prefix=='%q')", szPrefix.c_str());
		const bool bIsRefresh = !result.empty();

		/* Fonts first, stylesheet last: until the CSS is in place nothing
		   refers to the new fonts, so an interrupted write leaves the previous
		   version of the library working.

		   A failed first install is rolled back so it does not leave files
		   behind that no database row points at. A failed refresh is not:
		   the files it overwrote belong to a library that is still installed,
		   and deleting them would break something that worked a moment ago. */
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
		if (bWriteOk && !WriteWebAssetFile(szCssFile, szRewritten, LOGTAG))
		{
			szError = "Could not store " + szCssFile;
			bWriteOk = false;
		}
		if (!bWriteOk)
		{
			if (!bIsRefresh)
			{
				for (const auto& szDone : written)
					RemoveWebAssetFile(szDone);
			}
			return false;
		}

		if (!bIsRefresh)
		{
			m_sql.safe_query("INSERT INTO IconLibraries (Name, Prefix, CssFile, SourceURL, LastUpdate) VALUES ('%q','%q','%q','%q',datetime('now','localtime'))", szName.c_str(),
					 szPrefix.c_str(), szCssFile.c_str(), szURL.c_str());
		}
		else
		{
			m_sql.safe_query("UPDATE IconLibraries SET Name='%q', CssFile='%q', SourceURL='%q', LastUpdate=datetime('now','localtime') WHERE (ID==%d)", szName.c_str(),
					 szCssFile.c_str(), szURL.c_str(), atoi(result[0][0].c_str()));
		}

		_log.Log(LOG_STATUS, "%s: installed '%s' (prefix '%s'): %d font file(s), %d bytes", LOGTAG, szName.c_str(), szPrefix.c_str(), static_cast<int>(ctx.assets.size()),
			 static_cast<int>(ctx.totalSize));
		return true;
	}

	bool Refresh(int iID, std::string& szError)
	{
		szError.clear();

		auto result = m_sql.safe_query("SELECT Name, Prefix, SourceURL FROM IconLibraries WHERE (ID==%d)", iID);
		if (result.empty())
		{
			szError = "Unknown icon library";
			return false;
		}
		const std::string szName = result[0][0];
		const std::string szPrefix = result[0][1];
		const std::string szURL = result[0][2];
		if (szURL.empty())
		{
			szError = "This library has no source URL to refresh from";
			return false;
		}
		return Install(szName, szPrefix, szURL, szError);
	}

	bool Remove(int iID, std::string& szError)
	{
		szError.clear();

		auto result = m_sql.safe_query("SELECT Prefix, CssFile FROM IconLibraries WHERE (ID==%d)", iID);
		if (result.empty())
		{
			szError = "Unknown icon library";
			return false;
		}
		const std::string szPrefix = result[0][0];
		const std::string szCssFile = result[0][1];

		if (!IsValidPrefix(szPrefix))
		{
			/* Should not happen: the prefix is validated before it is stored.
			   Refuse rather than guess which files to delete. */
			szError = "Stored library prefix is invalid";
			return false;
		}

		if (!szCssFile.empty() && IsSafeWebAssetName(szCssFile) && IsAllowedWebAssetType(szCssFile))
			RemoveWebAssetFile(szCssFile);

		/* Every font this library stored was named "<prefix>-...", so the
		   directory listing is enough to find them; no manifest to keep in
		   sync. Names are re-validated because they come from the filesystem. */
		const std::string szMatch = szPrefix + "-";
		std::vector<std::string> toRemove;
		DIR* lDir = opendir(WebAssetFolder().c_str());
		if (lDir != nullptr)
		{
			struct dirent* ent;
			while ((ent = readdir(lDir)) != nullptr)
			{
				const std::string szFileName = ent->d_name;
				if (szFileName.compare(0, szMatch.size(), szMatch) != 0)
					continue;
				if (!IsSafeWebAssetName(szFileName) || !IsAllowedWebAssetType(szFileName))
					continue;
				toRemove.push_back(szFileName);
			}
			closedir(lDir);
		}
		for (const auto& szFileName : toRemove)
			RemoveWebAssetFile(szFileName);

		m_sql.safe_query("DELETE FROM IconLibraries WHERE (ID==%d)", iID);

		_log.Log(LOG_STATUS, "%s: removed library '%s' and %d stored file(s)", LOGTAG, szPrefix.c_str(), static_cast<int>(toRemove.size()));
		return true;
	}
} // namespace IconLibraries
