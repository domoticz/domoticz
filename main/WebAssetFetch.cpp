#include "stdafx.h"
#include "WebAssetFetch.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "Helper.h"
#include "Logger.h"
#include "localtime_r.h"
#include "SQLHelper.h"
#include "WebAssets.h"
#include "../httpclient/HTTPClient.h"

namespace WebAssetFetch
{
	namespace
	{
		const int WEBASSET_MAX_COMPANIONS = 32;
		const size_t WEBASSET_MAX_TOTAL_SIZE = 16 * 1024 * 1024;
		const long WEBASSET_HTTP_TIMEOUT = 20;
		const int WEBASSET_MAX_REDIRECTS = 5;

		const char* LOGTAG = "WebAssetFetch";

		// Serialises Install/Forget/SetTitle so two admin requests for the same
		// library cannot interleave their file and metadata writes.
		std::mutex g_installMutex;

		const size_t WEBASSET_MAX_JOBS = 8;
		const time_t WEBASSET_JOB_RETENTION = 10 * 60; // seconds a finished job stays pollable

		struct _tJob
		{
			std::string szID;
			std::string szName;
			std::string szURL;
			std::string szTitle;
			std::string szError;
			bool bDone = false;
			bool bSuccess = false;
			time_t finished = 0;
			std::thread thread;
		};

		std::mutex g_jobsMutex;
		std::map<std::string, std::shared_ptr<_tJob>> g_jobs;

		// Must be called with g_jobsMutex held. Joins and drops jobs whose result
		// nobody asked for within the retention window.
		void PruneJobs()
		{
			const time_t now = mytime(nullptr);
			for (auto itt = g_jobs.begin(); itt != g_jobs.end();)
			{
				auto& pJob = itt->second;
				if (pJob->bDone && ((now - pJob->finished) > WEBASSET_JOB_RETENTION))
				{
					if (pJob->thread.joinable())
						pJob->thread.join();
					itt = g_jobs.erase(itt);
				}
				else
					++itt;
			}
		}

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
			std::string szBaseDir;
			std::vector<_tPendingAsset> assets;
			std::map<std::string, std::string> storedNames;
			size_t totalSize = 0;
			std::string szError;
		};

		size_t FindCaseInsensitive(const std::string& szHaystack, const std::string& szNeedle, size_t iFrom)
		{
			auto itt = std::search(szHaystack.begin() + iFrom, szHaystack.end(), szNeedle.begin(), szNeedle.end(),
					       [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
			if (itt == szHaystack.end())
				return std::string::npos;
			return static_cast<size_t>(itt - szHaystack.begin());
		}

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
			if (szAuthority.find_first_of(" \t\r\n\\") != std::string::npos)
				return false;
			return true;
		}

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

		// The IANA special-purpose registry rather than just the familiar private
		// ranges: a deployment is free to route any of these internally.
		bool IsBlockedIPv4(const uint8_t* pAddr)
		{
			if (pAddr[0] == 0) // 0.0.0.0/8
				return true;
			if (pAddr[0] == 10) // 10.0.0.0/8
				return true;
			if (pAddr[0] == 127) // 127.0.0.0/8
				return true;
			if ((pAddr[0] == 100) && ((pAddr[1] & 0xC0) == 64)) // 100.64.0.0/10
				return true;
			if ((pAddr[0] == 169) && (pAddr[1] == 254)) // 169.254.0.0/16
				return true;
			if ((pAddr[0] == 172) && ((pAddr[1] & 0xF0) == 16)) // 172.16.0.0/12
				return true;
			if ((pAddr[0] == 192) && (pAddr[1] == 0) && (pAddr[2] == 0)) // 192.0.0.0/24
				return true;
			if ((pAddr[0] == 192) && (pAddr[1] == 0) && (pAddr[2] == 2)) // 192.0.2.0/24
				return true;
			if ((pAddr[0] == 192) && (pAddr[1] == 88) && (pAddr[2] == 99)) // 192.88.99.0/24
				return true;
			if ((pAddr[0] == 192) && (pAddr[1] == 168)) // 192.168.0.0/16
				return true;
			if ((pAddr[0] == 198) && ((pAddr[1] & 0xFE) == 18)) // 198.18.0.0/15
				return true;
			if ((pAddr[0] == 198) && (pAddr[1] == 51) && (pAddr[2] == 100)) // 198.51.100.0/24
				return true;
			if ((pAddr[0] == 203) && (pAddr[1] == 0) && (pAddr[2] == 113)) // 203.0.113.0/24
				return true;
			if (pAddr[0] >= 224) // multicast, the 240/4 reserved block and broadcast
				return true;
			return false;
		}

		bool IsBlockedIPv6(const uint8_t* pAddr)
		{
			static const uint8_t v4Mapped[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };
			if (memcmp(pAddr, v4Mapped, sizeof(v4Mapped)) == 0)
				return IsBlockedIPv4(pAddr + 12);

			// NAT64 and 6to4 carry the IPv4 destination inside the IPv6 address, so
			// without these they would reach a blocked v4 host past the check above.
			static const uint8_t nat64[4] = { 0, 0x64, 0xFF, 0x9B };
			if (memcmp(pAddr, nat64, sizeof(nat64)) == 0)
			{
				// Only 64:ff9b::/96 puts the address at an offset we can rely on. Anything
				// else below 64:ff9b::/32, the local-use 64:ff9b:1::/48 included, embeds it
				// at a length we would have to guess, so the rest of the range is refused.
				static const uint8_t zero8[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
				if (memcmp(pAddr + 4, zero8, sizeof(zero8)) != 0)
					return true;
				return IsBlockedIPv4(pAddr + 12);
			}
			if ((pAddr[0] == 0x20) && (pAddr[1] == 0x02)) // 2002::/16
				return IsBlockedIPv4(pAddr + 2);

			bool bTopZero = true;
			for (int ii = 0; ii < 10; ii++)
			{
				if (pAddr[ii] != 0)
				{
					bTopZero = false;
					break;
				}
			}
			if (bTopZero)
				return true;

			if ((pAddr[0] & 0xFE) == 0xFC) // fc00::/7 unique local
				return true;
			if ((pAddr[0] == 0xFE) && ((pAddr[1] & 0xC0) == 0x80)) // fe80::/10 link local
				return true;
			if (pAddr[0] == 0xFF) // ff00::/8 multicast
				return true;

			static const uint8_t zero6[6] = { 0, 0, 0, 0, 0, 0 };
			if ((pAddr[0] == 0x01) && (pAddr[1] == 0x00) && (memcmp(pAddr + 2, zero6, sizeof(zero6)) == 0)) // 100::/64 discard
				return true;
			if ((pAddr[0] == 0x20) && (pAddr[1] == 0x01) && (pAddr[2] == 0x00))
			{
				if (pAddr[3] == 0x00) // 2001::/32 Teredo
					return true;
				if (((pAddr[3] & 0xF0) == 0x10) || ((pAddr[3] & 0xF0) == 0x20)) // 2001:10::/28, 2001:20::/28 ORCHID
					return true;
			}
			if ((pAddr[0] == 0x20) && (pAddr[1] == 0x01) && (pAddr[2] == 0x0D) && (pAddr[3] == 0xB8)) // 2001:db8::/32
				return true;
			return false;
		}

		std::string HostFromAuthority(const std::string& szAuthority)
		{
			std::string szHost = szAuthority;
			const size_t iAt = szHost.rfind('@');
			if (iAt != std::string::npos)
				szHost = szHost.substr(iAt + 1);
			if (!szHost.empty() && (szHost.front() == '['))
			{
				const size_t iEnd = szHost.find(']');
				if (iEnd == std::string::npos)
					return "";
				return szHost.substr(1, iEnd - 1);
			}
			const size_t iColon = szHost.find(':');
			if (iColon != std::string::npos)
				szHost = szHost.substr(0, iColon);
			return szHost;
		}

		std::string PortFromAuthority(const std::string& szAuthority, const std::string& szScheme)
		{
			std::string szHostPort = szAuthority;
			const size_t iAt = szHostPort.rfind('@');
			if (iAt != std::string::npos)
				szHostPort = szHostPort.substr(iAt + 1);

			size_t iColon = std::string::npos;
			if (!szHostPort.empty() && (szHostPort.front() == '['))
			{
				const size_t iEnd = szHostPort.find(']');
				if (iEnd != std::string::npos)
					iColon = szHostPort.find(':', iEnd);
			}
			else
				iColon = szHostPort.find(':');

			if (iColon != std::string::npos)
			{
				const std::string szPort = szHostPort.substr(iColon + 1);
				if (!szPort.empty() && (szPort.size() <= 5) && (szPort.find_first_not_of("0123456789") == std::string::npos))
					return szPort;
			}
			return (szScheme == "https") ? "443" : "80";
		}

		// szPinAddress stays empty for literal IPs, they carry no name that could be rebound.
		bool IsPublicHost(const std::string& szAuthority, std::string& szPinAddress)
		{
			szPinAddress.clear();

			const std::string szHost = HostFromAuthority(szAuthority);
			if (szHost.empty() || (szHost.size() > 255))
				return false;

			uint8_t addr4[4];
			if (inet_pton(AF_INET, szHost.c_str(), addr4) == 1)
				return !IsBlockedIPv4(addr4);
			uint8_t addr6[16];
			if (inet_pton(AF_INET6, szHost.c_str(), addr6) == 1)
				return !IsBlockedIPv6(addr6);

			struct addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;

			struct addrinfo* pResult = nullptr;
			if (getaddrinfo(szHost.c_str(), nullptr, &hints, &pResult) != 0)
				return false;

			std::string szFirstAllowed;
			bool bAllowed = (pResult != nullptr);
			for (struct addrinfo* pInfo = pResult; pInfo != nullptr; pInfo = pInfo->ai_next)
			{
				char szText[INET6_ADDRSTRLEN];
				memset(szText, 0, sizeof(szText));

				if (pInfo->ai_family == AF_INET)
				{
					const struct sockaddr_in* pSa = reinterpret_cast<const struct sockaddr_in*>(pInfo->ai_addr);
					bAllowed = !IsBlockedIPv4(reinterpret_cast<const uint8_t*>(&pSa->sin_addr));
					if (bAllowed && szFirstAllowed.empty() && (inet_ntop(AF_INET, &pSa->sin_addr, szText, sizeof(szText)) != nullptr))
						szFirstAllowed = szText;
				}
				else if (pInfo->ai_family == AF_INET6)
				{
					const struct sockaddr_in6* pSa6 = reinterpret_cast<const struct sockaddr_in6*>(pInfo->ai_addr);
					bAllowed = !IsBlockedIPv6(reinterpret_cast<const uint8_t*>(&pSa6->sin6_addr));
					if (bAllowed && szFirstAllowed.empty() && (inet_ntop(AF_INET6, &pSa6->sin6_addr, szText, sizeof(szText)) != nullptr))
						szFirstAllowed = "[" + std::string(szText) + "]";
				}
				else
					bAllowed = false;
				if (!bAllowed)
					break;
			}
			freeaddrinfo(pResult);

			if (!bAllowed || szFirstAllowed.empty())
				return false;
			szPinAddress = szFirstAllowed;
			return true;
		}

		bool CheckDestination(const std::string& szScheme, const std::string& szAuthority, std::string& szResolve, std::string& szError)
		{
			szResolve.clear();

			std::string szPinAddress;
			if (!IsPublicHost(szAuthority, szPinAddress))
			{
				_log.Log(LOG_ERROR, "%s: refusing to fetch from '%s', it does not resolve to a public internet address", LOGTAG, szAuthority.c_str());
				szError = "'" + szAuthority + "' is not a public internet address. Assets on your own network can be installed with the file upload option instead of a URL";
				return false;
			}

			if (!szPinAddress.empty())
				szResolve = HostFromAuthority(szAuthority) + ":" + PortFromAuthority(szAuthority, szScheme) + ":" + szPinAddress;
			return true;
		}

		bool ResolveAgainst(const std::string& szBaseScheme, const std::string& szBaseAuthority, const std::string& szBasePath, const std::string& szRef, std::string& szOut)
		{
			if (!IsCleanURL(szRef))
				return false;

			std::string szScheme;
			std::string szAuthority;
			std::string szPath;

			if (szRef.compare(0, 2, "//") == 0)
			{
				if (!ParseHttpURL(szBaseScheme + ":" + szRef, szScheme, szAuthority, szPath))
					return false;
			}
			else
			{
				const size_t iColon = szRef.find(':');
				const size_t iSlash = szRef.find('/');
				if ((iColon != std::string::npos) && ((iSlash == std::string::npos) || (iColon < iSlash)))
				{
					if (!ParseHttpURL(szRef, szScheme, szAuthority, szPath))
						return false;
				}
				else
				{
					szScheme = szBaseScheme;
					szAuthority = szBaseAuthority;
					if (szRef.front() == '/')
						szPath = szRef;
					else
					{
						const std::string szClean = szBasePath.substr(0, szBasePath.find_first_of("?#"));
						szPath = szClean.substr(0, szClean.rfind('/') + 1) + szRef;
					}
				}
			}

			std::string szSuffix;
			const size_t iCut = szPath.find_first_of("?#");
			if (iCut != std::string::npos)
			{
				szSuffix = szPath.substr(iCut);
				szPath = szPath.substr(0, iCut);
			}
			const size_t iFrag = szSuffix.find('#');
			if (iFrag != std::string::npos)
				szSuffix = szSuffix.substr(0, iFrag);
			if (szSuffix == "?")
				szSuffix.clear();

			szOut = szScheme + "://" + szAuthority + NormalisePath(szPath) + szSuffix;
			return true;
		}

		int HeaderStatusCode(const std::vector<std::string>& vHeaderData)
		{
			int iCode = 0;
			for (const auto& szLine : vHeaderData)
			{
				if (szLine.compare(0, 5, "HTTP/") != 0)
					continue;
				const size_t iSpace = szLine.find(' ');
				if (iSpace != std::string::npos)
					iCode = atoi(szLine.substr(iSpace + 1).c_str());
			}
			return iCode;
		}

		std::string HeaderLocation(const std::vector<std::string>& vHeaderData)
		{
			const std::string szNeedle = "location:";
			for (auto itt = vHeaderData.rbegin(); itt != vHeaderData.rend(); ++itt)
			{
				if (itt->size() <= szNeedle.size())
					continue;
				if (FindCaseInsensitive(*itt, szNeedle, 0) != 0)
					continue;
				std::string szValue = itt->substr(szNeedle.size());
				stdstring_trimws(szValue);
				return szValue;
			}
			return "";
		}

		bool HttpGet(const std::string& szURL, std::string& szContent, std::string& szError, std::string* pszFinalURL = nullptr)
		{
			std::string szCurrent = szURL;
			for (int iHop = 0; iHop <= WEBASSET_MAX_REDIRECTS; iHop++)
			{
				std::string szScheme;
				std::string szAuthority;
				std::string szPath;
				if (!IsCleanURL(szCurrent) || !ParseHttpURL(szCurrent, szScheme, szAuthority, szPath))
				{
					szError = "Only http:// and https:// URLs are supported";
					return false;
				}
				std::string szResolve;
				if (!CheckDestination(szScheme, szAuthority, szResolve, szError))
					return false;

				_log.Log(LOG_STATUS, "%s: fetching %s", LOGTAG, szCurrent.c_str());

				std::vector<unsigned char> vResponse;
				std::vector<std::string> vHeaderData;
				const std::vector<std::string> vExtraHeaders;
				if (!HTTPClient::GETBinary(szCurrent, vExtraHeaders, vResponse, vHeaderData, WEBASSET_HTTP_TIMEOUT, false, false, szResolve))
				{
					_log.Log(LOG_ERROR, "%s: could not fetch %s", LOGTAG, szCurrent.c_str());
					szError = "Could not download " + szCurrent;
					return false;
				}

				const int iStatus = HeaderStatusCode(vHeaderData);
				const std::string szLocation = ((iStatus >= 300) && (iStatus < 400)) ? HeaderLocation(vHeaderData) : "";
				if (szLocation.empty())
				{
					szContent.assign(vResponse.begin(), vResponse.end());
					if (pszFinalURL != nullptr)
						*pszFinalURL = szCurrent;
					return true;
				}

				std::string szNext;
				if (!ResolveAgainst(szScheme, szAuthority, szPath, szLocation, szNext))
				{
					_log.Log(LOG_ERROR, "%s: could not follow the redirect returned by %s", LOGTAG, szCurrent.c_str());
					szError = "Could not download " + szURL;
					return false;
				}
				_log.Log(LOG_STATUS, "%s: %d redirect to %s", LOGTAG, iStatus, szNext.c_str());
				szCurrent = szNext;
			}

			_log.Log(LOG_ERROR, "%s: too many redirects while fetching %s", LOGTAG, szURL.c_str());
			szError = "Too many redirects while downloading " + szURL;
			return false;
		}

		bool ResolveReference(const _tDownloadContext& ctx, const std::string& szRef, std::string& szOut, std::string& szFileName)
		{
			if (!IsCleanURL(szRef))
				return false;

			std::string szPathPart = szRef;
			std::string szSuffix;
			const size_t iCut = szPathPart.find_first_of("?#");
			if (iCut != std::string::npos)
			{
				szSuffix = szPathPart.substr(iCut);
				szPathPart = szPathPart.substr(0, iCut);
			}
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
				if (!ParseHttpURL(ctx.szBaseScheme + ":" + szPathPart, szScheme, szAuthority, szPath))
					return false;
			}
			else
			{
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

		// A reference the browser cannot turn into a network request: an inline data
		// URI or a same-document fragment. Anything else we failed to store would
		// still be fetched when the stylesheet loads, which is both what installing
		// locally is meant to avoid and how a refused destination would be reached
		// from the browser instead of from us.
		bool IsInertReference(const std::string& szRef)
		{
			if (szRef.empty())
				return false;
			if (szRef.front() == '#')
				return true;
			std::string szLower = szRef;
			stdlower(szLower);
			return (szLower.compare(0, 5, "data:") == 0);
		}

		bool StoreReference(_tDownloadContext& ctx, const std::string& szRef, std::string& szStoredName)
		{
			std::string szAbsURL;
			std::string szFileName;
			if (!ResolveReference(ctx, szRef, szAbsURL, szFileName))
				return false;

			std::string szLower = szFileName;
			stdlower(szLower);
			if ((szLower.size() > 4) && (szLower.compare(szLower.size() - 4, 4, ".css") == 0))
			{
				_log.Log(LOG_STATUS, "%s: not following nested stylesheet %s", LOGTAG, szAbsURL.c_str());
				return false;
			}

			if (ctx.storedNames.count(szFileName) != 0)
			{
				szStoredName = szFileName;
				return true;
			}

			if (static_cast<int>(ctx.assets.size()) >= WEBASSET_MAX_COMPANIONS)
			{
				ctx.szError = "Asset references too many files";
				return false;
			}

			std::string szContent;
			std::string szFetchError;
			if (!HttpGet(szAbsURL, szContent, szFetchError))
				return false;
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

		// CSS lets any character of an identifier be written as a backslash escape, so
		// a literal search for "@import" or "url(" can be stepped around by writing
		// @\69mport instead. Genuine icon stylesheets only use escapes inside quoted
		// strings, for the glyph codepoint, so an escape anywhere else is treated as
		// obfuscation and the sheet is refused rather than matched token by token.
		bool HasEscapeOutsideString(const std::string& szCss)
		{
			char cQuote = 0;
			bool bComment = false;
			for (size_t ii = 0; ii < szCss.size(); ii++)
			{
				const char c = szCss[ii];
				if (bComment)
				{
					if ((c == '*') && ((ii + 1) < szCss.size()) && (szCss[ii + 1] == '/'))
					{
						bComment = false;
						ii++;
					}
					continue;
				}
				if (cQuote != 0)
				{
					if (c == '\\')
						ii++;
					else if (c == cQuote)
						cQuote = 0;
					continue;
				}
				if ((c == '"') || (c == '\''))
				{
					cQuote = c;
					continue;
				}
				if ((c == '/') && ((ii + 1) < szCss.size()) && (szCss[ii + 1] == '*'))
				{
					bComment = true;
					ii++;
					continue;
				}
				if (c == '\\')
					return true;
			}
			return false;
		}

		// Nested stylesheets are never followed, so an @import can only ever point at
		// something we would refuse to fetch. Dropping the whole at-rule also covers
		// the bare-string form, which carries no url() for the rewrite pass to catch.
		std::string StripImports(const std::string& szCss)
		{
			std::string szOut;
			szOut.reserve(szCss.size());

			size_t iPos = 0;
			while (true)
			{
				const size_t iFound = FindCaseInsensitive(szCss, "@import", iPos);
				if (iFound == std::string::npos)
				{
					szOut.append(szCss, iPos, std::string::npos);
					break;
				}

				const size_t iAfter = iFound + 7;
				const char cNext = (iAfter < szCss.size()) ? szCss[iAfter] : ' ';
				// Do not match a longer identifier that merely starts with @import.
				if ((isspace(static_cast<unsigned char>(cNext)) == 0) && (cNext != '"') && (cNext != '\'') && (cNext != 'u') && (cNext != 'U'))
				{
					szOut.append(szCss, iPos, iAfter - iPos);
					iPos = iAfter;
					continue;
				}

				szOut.append(szCss, iPos, iFound - iPos);

				// A statement at-rule, so run to its terminating semicolon, stepping over
				// quoted sections so a ';' inside a URL cannot end it early. A '}' also
				// stops the scan, so malformed input cannot swallow the rest of the sheet.
				size_t iScan = iAfter;
				char cQuote = 0;
				while (iScan < szCss.size())
				{
					const char c = szCss[iScan];
					if (cQuote != 0)
					{
						if (c == cQuote)
							cQuote = 0;
					}
					else if ((c == '"') || (c == '\''))
						cQuote = c;
					else if ((c == ';') || (c == '}'))
						break;
					iScan++;
				}
				if ((iScan < szCss.size()) && (szCss[iScan] == ';'))
					iScan++;

				_log.Log(LOG_STATUS, "%s: dropped an @import rule while storing a stylesheet", LOGTAG);
				iPos = iScan;
			}
			return szOut;
		}

		bool RewriteStylesheet(_tDownloadContext& ctx, const std::string& szSourceCss, std::string& szOut)
		{
			if (HasEscapeOutsideString(szSourceCss))
			{
				_log.Log(LOG_ERROR, "%s: refusing a stylesheet that escapes syntax outside a string", LOGTAG);
				ctx.szError = "Stylesheet uses escaped syntax that cannot be checked safely";
				return false;
			}

			const std::string szCss = StripImports(szSourceCss);
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

				szOut.append(szCss, iPos, iOpen - iPos);

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
					const std::string szLogged = (szRef.size() > 120) ? (szRef.substr(0, 120) + "...") : szRef;
					if (IsInertReference(szRef))
					{
						szOut += szRaw;
						_log.Log(LOG_STATUS, "%s: leaving reference '%s' as is", LOGTAG, szLogged.c_str());
					}
					else
					{
						szOut += "about:invalid";
						_log.Log(LOG_STATUS, "%s: replaced reference '%s' that could not be stored", LOGTAG, szLogged.c_str());
					}
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

		std::string AssetStem(const std::string& szName)
		{
			const size_t iDot = szName.find('.');
			return (iDot == std::string::npos) ? szName : szName.substr(0, iDot);
		}

		const size_t WEBASSET_MAX_TITLE = 128;

		std::string SanitiseTitle(const std::string& szTitle)
		{
			std::string szOut;
			szOut.reserve(szTitle.size());
			for (const char c : szTitle)
			{
				const unsigned char u = static_cast<unsigned char>(c);
				if ((u < 0x20) || (u == 0x7F))
					continue;
				szOut += c;
			}
			stdstring_trimws(szOut);

			if (szOut.size() > WEBASSET_MAX_TITLE)
			{
				// Cutting a UTF-8 sequence in half would leave a byte string the JSON
				// reply cannot carry, so step back to the start of that sequence.
				size_t iCut = WEBASSET_MAX_TITLE;
				while ((iCut > 0) && ((static_cast<unsigned char>(szOut[iCut]) & 0xC0) == 0x80))
					iCut--;
				szOut.erase(iCut);
				stdstring_trimws(szOut);
			}
			return szOut;
		}

		std::vector<std::string> LoadCompanions(const std::string& szName)
		{
			std::vector<std::string> companions;
			auto result = m_sql.safe_query("SELECT Companions FROM WebAssets WHERE (Name=='%q')", szName.c_str());
			if (result.empty() || result[0][0].empty())
				return companions;
			StringSplit(result[0][0], "\n", companions);
			return companions;
		}

		// A generated companion filename, or the main asset name, can only be written or
		// removed on behalf of szOwnerName if no *other* library already claims it, either
		// as its own Name or as one of its Companions. Without this, two libraries whose
		// sanitised names collide can overwrite or delete each other's files.
		bool IsWebAssetNameOwnedByOther(const std::string& szCandidateName, const std::string& szOwnerName)
		{
			auto result = m_sql.safe_query("SELECT Name, Companions FROM WebAssets WHERE (Name!='%q')", szOwnerName.c_str());
			for (const auto& sd : result)
			{
				if (sd[0] == szCandidateName)
					return true;
				if (sd[1].empty())
					continue;
				std::vector<std::string> companions;
				StringSplit(sd[1], "\n", companions);
				if (std::find(companions.begin(), companions.end(), szCandidateName) != companions.end())
					return true;
			}
			return false;
		}
	} // namespace

	bool Install(const std::string& szName, const std::string& szURL, const std::string& szTitle, std::string& szError)
	{
		std::lock_guard<std::mutex> l(g_installMutex);
		szError.clear();

		if (!IsSafeWebAssetName(szName) || !IsAllowedWebAssetType(szName))
		{
			szError = "Invalid asset name";
			return false;
		}
		if (szURL.size() > 500)
		{
			szError = "URL too long";
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
		std::string szFinalURL;
		if (!HttpGet(szURL, szContent, szError, &szFinalURL))
		{
			if (szError.empty())
				szError = "Could not download " + szURL;
			return false;
		}

		std::string szFinalPath;
		if (ParseHttpURL(szFinalURL, ctx.szBaseScheme, ctx.szBaseAuthority, szFinalPath))
		{
			szFinalPath = NormalisePath(szFinalPath.substr(0, szFinalPath.find_first_of("?#")));
			ctx.szBaseDir = szFinalPath.substr(0, szFinalPath.rfind('/') + 1);
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

		// Every generated companion name is checked against files owned by some other
		// library before anything is written, so a sanitised-name collision cannot let
		// this install overwrite (or, later, delete) a file that belongs to it instead.
		for (const auto& asset : ctx.assets)
		{
			if (IsWebAssetNameOwnedByOther(asset.szName, szName))
			{
				szError = "Asset file name '" + asset.szName + "' is already used by another installed library";
				return false;
			}
		}
		if (IsWebAssetNameOwnedByOther(szName, szName))
		{
			szError = "Asset file name '" + szName + "' is already used by another installed library";
			return false;
		}

		// Every file is staged to a temp file first and only committed (renamed into
		// place) once all of them, companions and main asset alike, have staged
		// successfully. This keeps a failed refresh from leaving some files replaced
		// against updated metadata and others left with their old content.
		std::vector<std::pair<std::string, std::string>> staged; // name -> temp file
		bool bStageOk = true;
		for (const auto& asset : ctx.assets)
		{
			std::string szTmpFile;
			if (!StageWebAssetFile(asset.szName, asset.szContent, LOGTAG, szTmpFile))
			{
				szError = "Could not store " + asset.szName;
				bStageOk = false;
				break;
			}
			staged.emplace_back(asset.szName, szTmpFile);
		}
		if (bStageOk)
		{
			std::string szTmpFile;
			if (!StageWebAssetFile(szName, szContent, LOGTAG, szTmpFile))
			{
				szError = "Could not store " + szName;
				bStageOk = false;
			}
			else
				staged.emplace_back(szName, szTmpFile);
		}
		if (!bStageOk)
		{
			for (const auto& sd : staged)
				DiscardStagedWebAssetFile(sd.second);
			return false;
		}

		bool bCommitOk = true;
		std::vector<std::pair<std::string, std::string>> committed; // name -> backup file (empty if none existed)
		for (const auto& sd : staged)
		{
			std::string szBackupFile;
			if (!BackupWebAssetFile(sd.first, szBackupFile, LOGTAG))
			{
				szError = "Could not store " + sd.first;
				bCommitOk = false;
				break;
			}
			if (!CommitWebAssetFile(sd.first, sd.second, LOGTAG))
			{
				// Nothing was replaced for this name, put the original straight back.
				RestoreWebAssetFile(sd.first, szBackupFile);
				szError = "Could not store " + sd.first;
				bCommitOk = false;
				break;
			}
			committed.emplace_back(sd.first, szBackupFile);
		}
		if (!bCommitOk)
		{
			// Roll every already-committed file in this batch back to what it was before,
			// so a failure part way through leaves the library exactly as it was rather
			// than on a mix of old and new files.
			for (const auto& cd : committed)
				RestoreWebAssetFile(cd.first, cd.second);
			for (const auto& sd : staged)
				DiscardStagedWebAssetFile(sd.second);
			return false;
		}
		for (const auto& cd : committed)
		{
			DiscardWebAssetBackup(cd.second);
			WriteWebAssetGzip(cd.first, LOGTAG);
		}

		std::string szCompanions;
		for (const auto& asset : ctx.assets)
		{
			if (!szCompanions.empty())
				szCompanions += "\n";
			szCompanions += asset.szName;
		}

		const std::string szCleanTitle = SanitiseTitle(szTitle);

		if (!bIsUpdate)
		{
			m_sql.safe_query("INSERT INTO WebAssets (Name, SourceURL, Companions, LastUpdate, Title) VALUES ('%q','%q','%q',datetime('now','localtime'),'%q')", szName.c_str(),
					 szURL.c_str(), szCompanions.c_str(), szCleanTitle.c_str());
		}
		else
		{
			m_sql.safe_query("UPDATE WebAssets SET SourceURL='%q', Companions='%q', LastUpdate=datetime('now','localtime') WHERE (ID==%d)", szURL.c_str(), szCompanions.c_str(),
					 atoi(existing[0][0].c_str()));
			// Refreshing from the stored source URL sends no title, that must not blank it.
			if (!szCleanTitle.empty())
				m_sql.safe_query("UPDATE WebAssets SET Title='%q' WHERE (ID==%d)", szCleanTitle.c_str(), atoi(existing[0][0].c_str()));

			for (const auto& szOld : previous)
			{
				if (ctx.storedNames.count(szOld) != 0)
					continue;
				if (!IsSafeWebAssetName(szOld) || !IsAllowedWebAssetType(szOld))
					continue;
				// Metadata recorded before this ownership check existed may list a companion
				// that another library has since come to own (or always shared, on a name
				// collision); such a file must be left alone rather than deleted out from
				// under that other library.
				if (IsWebAssetNameOwnedByOther(szOld, szName))
					continue;
				RemoveWebAssetFile(szOld);
			}
		}

		_log.Log(LOG_STATUS, "%s: stored '%s' with %d companion file(s), %d bytes", LOGTAG, szName.c_str(), static_cast<int>(ctx.assets.size()), static_cast<int>(ctx.totalSize));
		return true;
	}

	void SetTitle(const std::string& szName, const std::string& szTitle)
	{
		std::lock_guard<std::mutex> l(g_installMutex);
		const std::string szCleanTitle = SanitiseTitle(szTitle);
		if (szCleanTitle.empty())
			return;

		auto existing = m_sql.safe_query("SELECT ID FROM WebAssets WHERE (Name=='%q')", szName.c_str());
		if (existing.empty())
		{
			// Uploaded assets have no metadata row until something is stored about them.
			m_sql.safe_query("INSERT INTO WebAssets (Name, LastUpdate, Title) VALUES ('%q',datetime('now','localtime'),'%q')", szName.c_str(), szCleanTitle.c_str());
			return;
		}
		m_sql.safe_query("UPDATE WebAssets SET Title='%q' WHERE (ID==%d)", szCleanTitle.c_str(), atoi(existing[0][0].c_str()));
	}

	bool IsNameOwnedByOther(const std::string& szCandidateName, const std::string& szOwnerName)
	{
		return IsWebAssetNameOwnedByOther(szCandidateName, szOwnerName);
	}

	void Forget(const std::string& szName)
	{
		std::lock_guard<std::mutex> l(g_installMutex);
		const std::vector<std::string> companions = LoadCompanions(szName);
		int iRemoved = 0;
		for (const auto& szCompanion : companions)
		{
			if (!IsSafeWebAssetName(szCompanion) || !IsAllowedWebAssetType(szCompanion))
				continue;
			// Metadata recorded before ownership checks existed may list a companion that
			// another library has since come to own; that file must survive this deletion.
			if (IsWebAssetNameOwnedByOther(szCompanion, szName))
				continue;
			RemoveWebAssetFile(szCompanion);
			iRemoved++;
		}

		m_sql.safe_query("DELETE FROM WebAssets WHERE (Name=='%q')", szName.c_str());

		if (iRemoved != 0)
			_log.Log(LOG_STATUS, "%s: removed '%s' and %d companion file(s)", LOGTAG, szName.c_str(), iRemoved);
	}

	std::string StartInstall(const std::string& szName, const std::string& szURL, const std::string& szTitle, std::string& szError)
	{
		szError.clear();

		// Cheap checks up front, so an obviously bad request fails immediately instead
		// of only after polling. Install repeats them on the worker thread.
		if (!IsSafeWebAssetName(szName) || !IsAllowedWebAssetType(szName))
		{
			szError = "Invalid asset name";
			return "";
		}
		if (szURL.size() > 500)
		{
			szError = "URL too long";
			return "";
		}
		if (!IsCleanURL(szURL))
		{
			szError = "The URL contains invalid characters";
			return "";
		}

		std::lock_guard<std::mutex> l(g_jobsMutex);
		PruneJobs();

		size_t iRunning = 0;
		for (const auto& jd : g_jobs)
		{
			if (jd.second->bDone)
				continue;
			iRunning++;
			if (jd.second->szName == szName)
			{
				szError = "This library is already being installed";
				return "";
			}
		}
		if (iRunning >= WEBASSET_MAX_JOBS)
		{
			szError = "Too many library installs are running, try again later";
			return "";
		}

		auto pJob = std::make_shared<_tJob>();
		pJob->szID = GenerateUUID();
		pJob->szName = szName;
		pJob->szURL = szURL;
		pJob->szTitle = szTitle;
		g_jobs[pJob->szID] = pJob;

		pJob->thread = std::thread([pJob]() {
			std::string szJobError;
			const bool bOK = Install(pJob->szName, pJob->szURL, pJob->szTitle, szJobError);
			std::lock_guard<std::mutex> lj(g_jobsMutex);
			pJob->bSuccess = bOK;
			pJob->szError = szJobError;
			pJob->finished = mytime(nullptr);
			pJob->bDone = true;
		});
		SetThreadName(pJob->thread.native_handle(), "WebAssetFetch");
		return pJob->szID;
	}

	bool GetJobStatus(const std::string& szJobID, JobStatus& status)
	{
		std::lock_guard<std::mutex> l(g_jobsMutex);
		auto itt = g_jobs.find(szJobID);
		if (itt == g_jobs.end())
			return false;
		const auto& pJob = itt->second;
		status.bRunning = !pJob->bDone;
		status.bSuccess = pJob->bSuccess;
		status.szName = pJob->szName;
		status.szError = pJob->szError;
		return true;
	}

	bool IsInstallRunning(const std::string& szName)
	{
		std::lock_guard<std::mutex> l(g_jobsMutex);
		for (const auto& jd : g_jobs)
		{
			if (!jd.second->bDone && (jd.second->szName == szName))
				return true;
		}
		return false;
	}

	void Shutdown()
	{
		// Take the threads out from under the lock so a job that is just finishing
		// can still take g_jobsMutex to record its result.
		std::vector<std::shared_ptr<_tJob>> jobs;
		{
			std::lock_guard<std::mutex> l(g_jobsMutex);
			for (auto& jd : g_jobs)
				jobs.push_back(jd.second);
			g_jobs.clear();
		}
		for (auto& pJob : jobs)
		{
			if (pJob->thread.joinable())
				pJob->thread.join();
		}
	}
} // namespace WebAssetFetch
