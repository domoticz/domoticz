#include "stdafx.h"
#include "ShellyHttp.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <libwebem/Base64.h>
#include "ShellyIdentify.h"
#include "../main/Helper.h"
#include "../main/json_helper.h"
#include "../httpclient/HTTPClient.h"

namespace
{
	constexpr size_t MAX_RESPONSE_HEADER_LINES = 100;
	constexpr size_t MAX_RESPONSE_HEADER_BYTES = 32 * 1024;

	std::string ToLowerAscii(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return s;
	}

	std::string ToUpperAscii(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return s;
	}

	std::string Trim(const std::string& s)
	{
		size_t b = 0;
		size_t e = s.size();
		while (b < e && std::isspace(static_cast<unsigned char>(s[b])))
			b++;
		while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
			e--;
		return s.substr(b, e - b);
	}

	bool StartsWithNoCase(const std::string& s, const std::string& prefix)
	{
		return s.size() >= prefix.size() && ToLowerAscii(s.substr(0, prefix.size())) == ToLowerAscii(prefix);
	}

	std::string HashHex(const std::string& algorithm, const std::string& input)
	{
		return (algorithm == "SHA-256") ? sha256hex(input) : GenerateMD5Hash(input);
	}

	// Escapes a value for use inside a quoted-string.
	std::string QuoteEscape(const std::string& s)
	{
		std::string out;
		out.reserve(s.size());
		for (const char c : s)
		{
			if (c == '"' || c == '\\')
				out += '\\';
			out += c;
		}
		return out;
	}

	// The text of the "X-Transport-Error: <curl code> <message>" header HTTPClient adds when the transfer failed,
	// before or after a status line arrived. Empty when there is no such header.
	std::string TransportErrorText(const std::vector<std::string>& headers)
	{
		static const std::string marker = "X-Transport-Error:";
		for (auto it = headers.rbegin(); it != headers.rend(); ++it)
		{
			if (it->compare(0, marker.size(), marker) != 0)
				continue;
			const std::string value = Trim(it->substr(marker.size()));
			const size_t sp = value.find(' ');
			return (sp == std::string::npos) ? std::string() : Trim(value.substr(sp + 1));
		}
		return std::string();
	}

	// Splits one WWW-Authenticate value that may carry several comma separated challenges
	// ("Basic realm=x, Digest realm=y, nonce=z") into one string per challenge.
	std::vector<std::string> SplitChallenges(const std::string& s)
	{
		std::vector<size_t> starts(1, 0);
		bool inQuote = false;
		for (size_t i = 0; i < s.size(); i++)
		{
			if (inQuote)
			{
				if (s[i] == '\\')
					i++;
				else if (s[i] == '"')
					inQuote = false;
				continue;
			}
			if (s[i] == '"')
			{
				inQuote = true;
				continue;
			}
			if (s[i] != ',')
				continue;
			size_t j = i + 1;
			while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j])))
				j++;
			size_t k = j;
			while (k < s.size() && !std::isspace(static_cast<unsigned char>(s[k])) && s[k] != '=' && s[k] != ',')
				k++;
			if (k == j || k >= s.size() || !std::isspace(static_cast<unsigned char>(s[k])))
				continue;
			size_t m = k;
			while (m < s.size() && std::isspace(static_cast<unsigned char>(s[m])))
				m++;
			// "token =" is a parameter, "token something" starts a new scheme.
			if (m < s.size() && s[m] != '=')
				starts.push_back(j);
		}
		std::vector<std::string> out;
		for (size_t n = 0; n < starts.size(); n++)
		{
			const size_t end = (n + 1 < starts.size()) ? starts[n + 1] : s.size();
			std::string part = Trim(s.substr(starts[n], end - starts[n]));
			while (!part.empty() && part.back() == ',')
				part.pop_back();
			out.push_back(Trim(part));
		}
		return out;
	}

	// Prefers a Digest challenge (SHA-256 over MD5, both over an unusable one), else the first challenge seen.
	std::string PickWwwAuthenticate(const std::vector<std::string>& headers)
	{
		std::string first;
		std::string sha256;
		std::string md5;
		std::string otherDigest;
		for (const auto& line : headers)
		{
			const size_t colon = line.find(':');
			if (colon == std::string::npos || ToLowerAscii(Trim(line.substr(0, colon))) != "www-authenticate")
				continue;
			const std::string value = Trim(line.substr(colon + 1));
			if (first.empty())
				first = value;
			for (const auto& challenge : SplitChallenges(value))
			{
				if (!StartsWithNoCase(challenge, "digest"))
					continue;
				ShellyDigestChallenge parsed;
				if (ShellyParseDigestChallenge(challenge, parsed))
				{
					if (parsed.algorithm == "SHA-256" && sha256.empty())
						sha256 = challenge;
					else if (parsed.algorithm != "SHA-256" && md5.empty())
						md5 = challenge;
				}
				else if (otherDigest.empty())
					otherDigest = challenge;
			}
		}
		if (!sha256.empty())
			return sha256;
		if (!md5.empty())
			return md5;
		if (!otherDigest.empty())
			return otherDigest;
		return first;
	}

	bool HasControlChars(const std::string& s)
	{
		return std::any_of(s.begin(), s.end(), [](const char c) {
			const unsigned char uc = static_cast<unsigned char>(c);
			return uc < 0x20 || uc == 0x7F;
		});
	}

	// One request without any authentication handling beyond the headers already in `headers`.
	ShellyHttpResponse PerformOnce(const ShellyHttpMethod method, const std::string& url, const std::string& body, const ShellyHttpOptions& options,
				       const std::vector<std::string>& headers)
	{
		ShellyHttpResponse response;
		const long timeout = (options.timeoutSec > 0) ? options.timeoutSec : 5;
		std::vector<unsigned char> data;

		// The body and the headers are bounded while they are received, so a hostile or broken peer cannot make us buffer
		// more than the bounds. The per-call timeout leaves the process-wide HTTPClient timeouts untouched.
		HTTPClient::Limits limits;
		limits.maxBodyBytes = options.maxBodyBytes;
		limits.maxHeaderLines = MAX_RESPONSE_HEADER_LINES;
		limits.maxHeaderBytes = MAX_RESPONSE_HEADER_BYTES;
		bool limitExceeded = false;
		if (method == ShellyHttpMethod::Post)
			HTTPClient::POSTBinaryLimited(url, body, headers, data, response.headers, limits, limitExceeded, timeout, options.followRedirects);
		else
			HTTPClient::GETBinaryLimited(url, headers, data, response.headers, limits, limitExceeded, timeout, options.followRedirects);
		response.exchanged = true;

		if (limitExceeded)
		{
			// A peer that answered but sent more than the bounds allow: reachable, the answer is refused.
			response.status = ShellyHttpStatusFromHeaders(response.headers);
			response.received = (response.status != 0);
			response.error = "response too large";
			return response;
		}

		response.status = ShellyHttpStatusFromHeaders(response.headers);
		response.received = (response.status != 0);
		response.wwwAuthenticate = PickWwwAuthenticate(response.headers);
		if (!response.received)
		{
			const std::string text = TransportErrorText(response.headers);
			response.error = text.empty() ? "unreachable" : "unreachable (" + text + ")";
			return response;
		}
		response.body.assign(data.begin(), data.end());
		// A status line followed by a timeout, reset or short body: the marker says the transfer did not complete,
		// so a partial body must not pass as a success. The body is kept for diagnostics.
		const std::string incomplete = TransportErrorText(response.headers);
		if (!incomplete.empty())
			response.error = "transfer incomplete (" + incomplete + ")";
		return response;
	}
} // namespace

ShellyResult ShellyHttpResponse::ToResult() const
{
	if (!received)
	{
		ShellyResult unreachable = ShellyResult::Failure(0, error.empty() ? "unreachable" : error);
		// Only a request that really went through HTTPClient says something about reachability.
		unreachable.transport = exchanged;
		return unreachable;
	}

	ShellyResult result;
	if (!error.empty())
		result = ShellyResult::Failure(status, error);
	else if (status < 200 || status >= 300)
		result = ShellyResult::Failure(status, "HTTP error " + std::to_string(status));
	else if (body.empty())
		result = ShellyResult::Failure(status, "empty response");
	else
		result = ShellyResult::Success(status);

	result.transport = exchanged;
	result.body = body;
	result.wwwAuthenticate = wwwAuthenticate;
	if (!body.empty())
	{
		Json::Value parsed;
		if (ParseJSon(body, parsed))
			result.data = parsed;
		else if (result.ok)
		{
			result.ok = false;
			result.error = "invalid JSON response";
		}
	}
	return result;
}

std::string ShellyBaseUrl(const std::string& host, const int port)
{
	return "http://" + ShellyHostForUrl(host) + ":" + std::to_string(port);
}

std::string ShellyBaseUrl(const ShellyNode& node)
{
	return ShellyBaseUrl(node.ip, node.port);
}

int ShellyHttpStatusFromHeaders(const std::vector<std::string>& headers)
{
	int status = 0;
	for (const auto& line : headers)
	{
		if (line.compare(0, 5, "HTTP/") != 0)
			continue;
		const size_t sp = line.find(' ');
		if (sp == std::string::npos)
			continue;
		const long code = strtol(line.c_str() + sp + 1, nullptr, 10);
		if (code >= 100 && code <= 999)
			status = static_cast<int>(code);
	}
	return status;
}

std::string ShellyHeaderValue(const std::vector<std::string>& headers, const std::string& name)
{
	const std::string wanted = ToLowerAscii(name);
	for (const auto& line : headers)
	{
		const size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		if (ToLowerAscii(Trim(line.substr(0, colon))) == wanted)
			return Trim(line.substr(colon + 1));
	}
	return std::string();
}

std::string ShellyBasicAuthHeader(const std::string& username, const std::string& password)
{
	return "Authorization: Basic " + base64_encode(username + ":" + password);
}

std::string ShellyUriFromUrl(const std::string& url)
{
	size_t start = url.find("://");
	start = (start == std::string::npos) ? 0 : start + 3;
	const size_t pathStart = url.find_first_of("/?#", start);
	if (pathStart == std::string::npos || url[pathStart] == '#')
		return "/";
	std::string uri = url.substr(pathStart);
	if (uri[0] == '?')
		uri.insert(0, "/");
	const size_t fragment = uri.find('#');
	if (fragment != std::string::npos)
		uri.resize(fragment);
	return uri;
}

ShellyHttpResponse ShellyHttpRequest(const ShellyHttpMethod method, const std::string& url, const std::string& body, const ShellyHttpOptions& options)
{
	// Control characters would allow header injection; refuse before any I/O.
	bool invalid = HasControlChars(url) || HasControlChars(options.contentType) || std::any_of(options.headers.begin(), options.headers.end(), HasControlChars);
	if (options.auth != ShellyHttpAuth::None)
		invalid = invalid || HasControlChars(options.username) || HasControlChars(options.password);
	if (invalid)
	{
		ShellyHttpResponse rejected;
		rejected.error = "invalid control character in url, credentials or headers";
		return rejected;
	}

	std::vector<std::string> headers = options.headers;
	if (method == ShellyHttpMethod::Post && !options.contentType.empty())
		headers.push_back("Content-Type: " + options.contentType);

	if (options.auth == ShellyHttpAuth::Basic)
	{
		headers.push_back(ShellyBasicAuthHeader(options.username, options.password));
		return PerformOnce(method, url, body, options, headers);
	}
	if (options.auth != ShellyHttpAuth::Digest)
		return PerformOnce(method, url, body, options, headers);

	if (!options.digest)
	{
		ShellyHttpResponse response;
		response.error = "digest authentication needs a state object";
		return response;
	}

	const std::string methodName = (method == ShellyHttpMethod::Post) ? "POST" : "GET";
	const std::string uri = ShellyUriFromUrl(url);

	std::vector<std::string> first = headers;
	std::string usedNonce;
	const std::string preemptive = options.digest->Authorize(options.username, options.password, methodName, uri, &usedNonce);
	if (!preemptive.empty())
		first.push_back("Authorization: " + preemptive);
	ShellyHttpResponse response = PerformOnce(method, url, body, options, first);
	if (response.status != 401 || response.wwwAuthenticate.empty())
		return response;

	ShellyDigestChallenge challenge;
	if (!ShellyParseDigestChallenge(response.wwwAuthenticate, challenge))
		return response;
	// Retry only when the answer can differ: no credentials were sent yet, or the server issued a new nonce.
	const bool retry = preemptive.empty() || challenge.stale || challenge.nonce != usedNonce;
	if (!retry)
		return response;
	options.digest->SetChallenge(response.wwwAuthenticate);

	const std::string authorization = options.digest->Authorize(options.username, options.password, methodName, uri);
	if (authorization.empty())
		return response;
	std::vector<std::string> second = headers;
	second.push_back("Authorization: " + authorization);
	return PerformOnce(method, url, body, options, second);
}

ShellyHttpResponse ShellyHttpGet(const std::string& url, const ShellyHttpOptions& options)
{
	return ShellyHttpRequest(ShellyHttpMethod::Get, url, std::string(), options);
}

ShellyHttpResponse ShellyHttpPost(const std::string& url, const std::string& body, const ShellyHttpOptions& options)
{
	return ShellyHttpRequest(ShellyHttpMethod::Post, url, body, options);
}

bool ShellyParseDigestChallenge(const std::string& wwwAuthenticate, ShellyDigestChallenge& challenge)
{
	challenge = ShellyDigestChallenge();

	std::string s = Trim(wwwAuthenticate);
	if (StartsWithNoCase(s, "www-authenticate:"))
		s = Trim(s.substr(17));
	if (!StartsWithNoCase(s, "digest") || s.size() <= 6 || !std::isspace(static_cast<unsigned char>(s[6])))
		return false;

	std::map<std::string, std::string> params;
	size_t i = 6;
	while (i < s.size())
	{
		while (i < s.size() && (std::isspace(static_cast<unsigned char>(s[i])) || s[i] == ','))
			i++;
		if (i >= s.size())
			break;
		const size_t keyStart = i;
		while (i < s.size() && s[i] != '=' && s[i] != ',')
			i++;
		const std::string key = ToLowerAscii(Trim(s.substr(keyStart, i - keyStart)));
		std::string value;
		if (i < s.size() && s[i] == '=')
		{
			i++;
			while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
				i++;
			if (i < s.size() && s[i] == '"')
			{
				i++;
				while (i < s.size() && s[i] != '"')
				{
					if (s[i] == '\\' && i + 1 < s.size())
						i++;
					value += s[i++];
				}
				if (i < s.size())
					i++; // closing quote
			}
			else
			{
				const size_t valueStart = i;
				while (i < s.size() && s[i] != ',')
					i++;
				value = Trim(s.substr(valueStart, i - valueStart));
			}
		}
		if (!key.empty())
			params[key] = value;
	}

	ShellyDigestChallenge c;
	c.realm = params["realm"];
	c.nonce = params["nonce"];
	c.opaque = params["opaque"];
	if (c.realm.empty() || c.nonce.empty())
		return false;
	if (HasControlChars(c.realm) || HasControlChars(c.nonce) || HasControlChars(c.opaque))
		return false;

	if (params.find("algorithm") != params.end())
		c.algorithm = ToUpperAscii(params["algorithm"]);
	if (c.algorithm != "MD5" && c.algorithm != "SHA-256")
		return false;

	const auto qopIt = params.find("qop");
	if (qopIt != params.end() && !qopIt->second.empty())
	{
		bool hasAuth = false;
		size_t pos = 0;
		while (pos <= qopIt->second.size())
		{
			const size_t comma = qopIt->second.find(',', pos);
			const std::string token = ToLowerAscii(Trim(qopIt->second.substr(pos, (comma == std::string::npos) ? std::string::npos : comma - pos)));
			if (token == "auth")
				hasAuth = true;
			if (comma == std::string::npos)
				break;
			pos = comma + 1;
		}
		if (!hasAuth)
			return false;
		c.qop = "auth";
	}
	c.stale = ToLowerAscii(params["stale"]) == "true";
	c.valid = true;
	challenge = c;
	return true;
}

std::string ShellyDigestAuthorization(const ShellyDigestChallenge& challenge, const std::string& username, const std::string& password, const std::string& method,
				      const std::string& uri, const unsigned int nonceCount, const std::string& cnonce)
{
	if (!challenge.valid)
		return std::string();

	const std::string ha1 = HashHex(challenge.algorithm, username + ":" + challenge.realm + ":" + password);
	const std::string ha2 = HashHex(challenge.algorithm, method + ":" + uri);

	char nc[16];
	snprintf(nc, sizeof(nc), "%08x", nonceCount);

	std::string response;
	if (challenge.qop.empty())
		response = HashHex(challenge.algorithm, ha1 + ":" + challenge.nonce + ":" + ha2);
	else
		response = HashHex(challenge.algorithm, ha1 + ":" + challenge.nonce + ":" + nc + ":" + cnonce + ":" + challenge.qop + ":" + ha2);

	std::string header = "Digest username=\"" + QuoteEscape(username) + "\", realm=\"" + QuoteEscape(challenge.realm) + "\", nonce=\"" + QuoteEscape(challenge.nonce)
			     + "\", uri=\"" + QuoteEscape(uri) + "\", algorithm=" + challenge.algorithm;
	if (!challenge.qop.empty())
		header += ", qop=" + challenge.qop + ", nc=" + nc + ", cnonce=\"" + QuoteEscape(cnonce) + "\"";
	header += ", response=\"" + response + "\"";
	if (!challenge.opaque.empty())
		header += ", opaque=\"" + QuoteEscape(challenge.opaque) + "\"";
	return header;
}

std::string ShellyNewCnonce()
{
	std::string uuid = GenerateUUID();
	uuid.erase(std::remove(uuid.begin(), uuid.end(), '-'), uuid.end());
	return uuid;
}

bool ShellyDigestAuth::SetChallenge(const std::string& wwwAuthenticate)
{
	ShellyDigestChallenge challenge;
	if (!ShellyParseDigestChallenge(wwwAuthenticate, challenge))
		return false;
	std::lock_guard<std::mutex> l(m_mutex);
	// The same nonce keeps counting: restarting at 1 would repeat an (nonce, nc) pair the server has already seen.
	if (!m_challenge.valid || m_challenge.nonce != challenge.nonce)
		m_nonceCount = 0;
	m_challenge = challenge;
	return true;
}

bool ShellyDigestAuth::HasChallenge() const
{
	std::lock_guard<std::mutex> l(m_mutex);
	return m_challenge.valid;
}

std::string ShellyDigestAuth::Nonce() const
{
	std::lock_guard<std::mutex> l(m_mutex);
	return m_challenge.nonce;
}

void ShellyDigestAuth::Reset()
{
	std::lock_guard<std::mutex> l(m_mutex);
	m_challenge = ShellyDigestChallenge();
	m_nonceCount = 0;
}

std::string ShellyDigestAuth::Authorize(const std::string& username, const std::string& password, const std::string& method, const std::string& uri,
				       std::string* usedNonce)
{
	std::lock_guard<std::mutex> l(m_mutex);
	if (usedNonce)
		*usedNonce = m_challenge.valid ? m_challenge.nonce : std::string();
	if (!m_challenge.valid)
		return std::string();
	m_nonceCount++;
	return ShellyDigestAuthorization(m_challenge, username, password, method, uri, m_nonceCount, ShellyNewCnonce());
}
