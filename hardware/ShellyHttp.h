#pragma once

// HTTP helper shared by Shelly device identification, the Gen1 REST client and the Gen2+ RPC client.
//
// Every request sets its own timeout through the per-call timeout parameter of HTTPClient, so the
// process-wide HTTPClient timeouts are never modified. HTTPClient cannot take credentials, so Basic
// authentication is sent as an explicit Authorization header and Digest authentication (SHA-256 and MD5,
// as used by Gen2+ devices) is computed here (see ShellyDigestAuth).

#include <mutex>
#include <string>
#include <vector>
#include "ShellyTypes.h"

enum class ShellyHttpMethod
{
	Get,
	Post
};

enum class ShellyHttpAuth
{
	None,
	Basic,  // preemptive: Authorization header sent with every request
	Digest  // needs a ShellyDigestAuth state; answers a 401 challenge once
};

class ShellyDigestAuth;

constexpr size_t SHELLY_HTTP_DEFAULT_MAX_BODY = 64 * 1024;

struct ShellyHttpOptions
{
	int timeoutSec = 5;                                  // total time allowed for one request (connect included)
	size_t maxBodyBytes = SHELLY_HTTP_DEFAULT_MAX_BODY;  // larger answers are rejected (the transfer is aborted once the bound is crossed)
	bool followRedirects = false;                        // device APIs do not redirect
	ShellyHttpAuth auth = ShellyHttpAuth::None;
	std::string username;                                // Basic or Digest user
	std::string password;
	ShellyDigestAuth* digest = nullptr;                  // required for ShellyHttpAuth::Digest, must outlive the call
	std::vector<std::string> headers;                    // extra request header lines ("Name: value")
	std::string contentType = "application/json";        // POST only
};

struct ShellyHttpResponse
{
	bool exchanged = false;                  // the request reached HTTPClient (false for a local rejection before any I/O)
	bool received = false;                   // an HTTP status line came back
	int status = 0;                          // HTTP status, 0 when none was received
	std::string body;
	std::vector<std::string> headers;        // raw header lines, status lines included
	std::string wwwAuthenticate;             // the Digest challenge when present, else the first WWW-Authenticate
	std::string error;                       // transport or size error; empty when an HTTP answer was received

	bool IsSuccess() const
	{
		return received && error.empty() && status >= 200 && status < 300;
	}
	// ok for 2xx. data holds the parsed body when it is JSON. Failures carry the HTTP status (0 when unreachable),
	// an error text, wwwAuthenticate and the raw body.
	ShellyResult ToResult() const;
};

// Base URL of a node: "http://host:port" (IPv6 literals bracketed, host names allowed).
std::string ShellyBaseUrl(const ShellyNode& node);

// The HTTP status of the last status line (100..999), or 0 when none came back. HTTPClient reports a failed transfer
// with an "X-Transport-Error: <curl code> <text>" header, not with a status line.
int ShellyHttpStatusFromHeaders(const std::vector<std::string>& headers);

// Value of the first header called `name` (case insensitive), trimmed; empty when absent.
std::string ShellyHeaderValue(const std::vector<std::string>& headers, const std::string& name);

// "Authorization: Basic <base64(user:password)>"
std::string ShellyBasicAuthHeader(const std::string& username, const std::string& password);

// The request target of a URL as used in a Digest "uri" field: path and query, without the fragment
// ("http://host?x=1" gives "/?x=1", "http://host" gives "/").
std::string ShellyUriFromUrl(const std::string& url);

// Base URL "http://host:port" for a bare host (IPv6 literals bracketed) and port. The host must be valid.
std::string ShellyBaseUrl(const std::string& host, int port);

// Performs one request. Never throws. Bytes below 0x20 or 0x7F in the credentials (when authentication is used)
// or in a header fail the request before any I/O. With ShellyHttpAuth::Digest a 401 carrying a Digest challenge is
// answered once with the computed Authorization header (the state keeps the challenge for later requests); among
// several Digest challenges SHA-256 is preferred over MD5. Only the "MD5" and "SHA-256" algorithms are supported,
// "-sess" variants are not. A request that is retried can take up to twice the timeout.
ShellyHttpResponse ShellyHttpRequest(ShellyHttpMethod method, const std::string& url, const std::string& body, const ShellyHttpOptions& options);
ShellyHttpResponse ShellyHttpGet(const std::string& url, const ShellyHttpOptions& options = ShellyHttpOptions());
ShellyHttpResponse ShellyHttpPost(const std::string& url, const std::string& body, const ShellyHttpOptions& options = ShellyHttpOptions());

// ---------------------------------------------------------------------------------------------------------
// HTTP Digest authentication (RFC 7616). HTTPClient/libcurl cannot be given credentials and is not relied on
// for SHA-256, so the Authorization header is computed here.

struct ShellyDigestChallenge
{
	bool valid = false;
	std::string realm;
	std::string nonce;
	std::string opaque;
	std::string algorithm = "MD5"; // "MD5" or "SHA-256" (as sent, normalized to upper case)
	std::string qop;               // "auth" when the server offers it (or lists it), empty for the legacy RFC 2069 form
	bool stale = false;
};

// Parses the value of a WWW-Authenticate header (with or without the "WWW-Authenticate:" prefix). Fails (valid is
// false, returns false) unless it is a Digest challenge with realm and nonce, with algorithm MD5 or SHA-256
// and with qop absent or offering "auth". Pure.
bool ShellyParseDigestChallenge(const std::string& wwwAuthenticate, ShellyDigestChallenge& challenge);

// Value of the Authorization header ("Digest username=..."), without the header name. Pure: the caller supplies
// the request method ("GET"/"POST"), the uri (see ShellyUriFromUrl), the nonce count and the client nonce.
// Returns an empty string for an invalid challenge.
std::string ShellyDigestAuthorization(const ShellyDigestChallenge& challenge, const std::string& username, const std::string& password, const std::string& method,
				      const std::string& uri, unsigned int nonceCount, const std::string& cnonce);

// A fresh random client nonce (32 hex digits).
std::string ShellyNewCnonce();

// Digest state of one device: the last challenge and the nonce counter. Thread safe.
class ShellyDigestAuth
{
      public:
	// Stores a new challenge. The nonce count restarts only when the nonce differs from the stored one.
	// Returns false when it is not a usable Digest challenge.
	bool SetChallenge(const std::string& wwwAuthenticate);
	bool HasChallenge() const;
	// Nonce of the stored challenge, empty without one.
	std::string Nonce() const;
	void Reset();
	// Authorization header value for the next request with the stored challenge, empty without one.
	// `usedNonce` (optional) receives the nonce the returned header was computed with, read atomically with it.
	std::string Authorize(const std::string& username, const std::string& password, const std::string& method, const std::string& uri, std::string* usedNonce = nullptr);

      private:
	mutable std::mutex m_mutex;
	ShellyDigestChallenge m_challenge;
	unsigned int m_nonceCount = 0;
};
