#include "stdafx.h"
#include "ShellyGen2Push.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <random>
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/json_helper.h"

namespace
{
	constexpr int MAX_BACKOFF_SEC = 60;
	constexpr int CONNECT_TIMEOUT_MS = 10000;
	constexpr int READY_TIMEOUT_MS = 10000;
	constexpr int64_t KEEPALIVE_MS = 30000;
	constexpr int64_t LIVENESS_TIMEOUT_MS = 100000; // three keepalive periods with some slack
	constexpr int64_t POLL_RETRY_MS = 5000;         // minimum spacing of baseline polls
	constexpr int64_t STABLE_SESSION_MS = 30000;    // a session that kept push active this long resets the backoff
	constexpr int WAIT_SLICE_MS = 500;
	constexpr size_t MAX_FRAME_BYTES = 1024 * 1024;
	constexpr int MAX_AUTH_REJECTS = 3;           // rejected auth answers in a row before the session is given up
	constexpr int64_t AUTH_RETRY_MS = 1000;       // spacing of the answers to a rejected auth attempt
	constexpr int SOCKET_FLUSH_MS = 1000;
	constexpr int AUTH_FAIL_NO_PASSWORD = 1;
	constexpr int AUTH_FAIL_REJECTED = 2;
	constexpr int AUTH_FAIL_REJECTED_LATE = 3;
	const char* WS_USER = "admin";
	const char* WS_SRC = "domoticz";
	const char* WS_KEEPALIVE_METHOD = "Sys.GetStatus";

	int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	std::string ToUpperAscii(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return s;
	}

	uint32_t RandomCnonce()
	{
		static std::mutex mutex;
		static std::mt19937 rng{ std::random_device{}() };
		std::lock_guard<std::mutex> lock(mutex);
		return static_cast<uint32_t>(rng());
	}
} // namespace

// ---- Pure helpers ---------------------------------------------------------------------------------------

bool ShellyWsParseFrame(const std::string& text, ShellyWsFrame& frame)
{
	frame = ShellyWsFrame();
	try
	{
		Json::Value doc;
		if (text.empty() || !ParseJSon(text, doc) || !doc.isObject())
			return false;
		if (doc["src"].isString())
			frame.src = doc["src"].asString();
		if (doc["id"].isInt())
		{
			frame.hasId = true;
			frame.id = doc["id"].asInt();
		}
		if (doc["method"].isString())
			frame.method = doc["method"].asString();

		if (!frame.method.empty())
		{
			// A frame with a method and an id is a request to us, not something we handle.
			if (frame.hasId || !doc["params"].isObject())
				return false;
			frame.params = doc["params"];
			if (frame.method == "NotifyStatus")
				frame.type = ShellyWsFrameType::NotifyStatus;
			else if (frame.method == "NotifyFullStatus")
				frame.type = ShellyWsFrameType::NotifyFullStatus;
			else if (frame.method == "NotifyEvent")
				frame.type = ShellyWsFrameType::NotifyEvent;
			else
				frame.type = ShellyWsFrameType::OtherNotification;
			return true;
		}
		if (!frame.hasId)
			return false;
		if (doc["error"].isObject())
		{
			const Json::Value& err = doc["error"];
			frame.type = ShellyWsFrameType::Error;
			frame.errorCode = err["code"].isInt() ? err["code"].asInt() : 0;
			if (err["message"].isString())
				frame.errorMessage = err["message"].asString();
			return true;
		}
		if (doc.isMember("result"))
		{
			frame.type = ShellyWsFrameType::Response;
			frame.params = doc["result"];
			return true;
		}
	}
	catch (...)
	{
	}
	frame = ShellyWsFrame();
	return false;
}

bool ShellyWsParseChallenge(const std::string& errorMessage, ShellyWsChallenge& challenge)
{
	challenge = ShellyWsChallenge();
	try
	{
		Json::Value doc;
		if (errorMessage.empty() || !ParseJSon(errorMessage, doc) || !doc.isObject())
			return false;
		if (!doc["auth_type"].isString() || ToUpperAscii(doc["auth_type"].asString()) != "DIGEST")
			return false;
		const Json::Value& nonce = doc["nonce"];
		if (nonce.isString())
			challenge.nonce = nonce.asString();
		else if (nonce.isNumeric())
		{
			challenge.nonce = nonce.asString();
			challenge.nonceIsNumber = nonce.isUInt64();
		}
		if (!doc["realm"].isString())
			return false;
		challenge.realm = doc["realm"].asString();
		if (challenge.realm.empty() || challenge.nonce.empty())
			return false;
		if (doc["algorithm"].isString() && !doc["algorithm"].asString().empty())
			challenge.algorithm = ToUpperAscii(doc["algorithm"].asString());
		const Json::Value& stale = doc["stale"];
		challenge.stale = (stale.isBool() && stale.asBool()) || (stale.isString() && ToUpperAscii(stale.asString()) == "TRUE");
		challenge.valid = true;
		return true;
	}
	catch (...)
	{
	}
	challenge = ShellyWsChallenge();
	return false;
}

Json::Value ShellyWsBuildAuth(const ShellyWsChallenge& challenge, const std::string& username, const std::string& password, const uint32_t nonceCount, const uint32_t cnonce)
{
	if (!challenge.valid || ToUpperAscii(challenge.algorithm) != "SHA-256")
		return Json::Value();
	char nc[16];
	snprintf(nc, sizeof(nc), "%08x", static_cast<unsigned int>(nonceCount));
	const std::string ha1 = sha256hex(username + ":" + challenge.realm + ":" + password);
	const std::string ha2 = sha256hex("dummy_method:dummy_uri");
	const std::string response = sha256hex(ha1 + ":" + challenge.nonce + ":" + nc + ":" + std::to_string(cnonce) + ":auth:" + ha2);

	Json::Value auth(Json::objectValue);
	auth["realm"] = challenge.realm;
	auth["username"] = username;
	if (challenge.nonceIsNumber)
		auth["nonce"] = static_cast<Json::UInt64>(std::strtoull(challenge.nonce.c_str(), nullptr, 10));
	else
		auth["nonce"] = challenge.nonce;
	auth["cnonce"] = static_cast<Json::UInt>(cnonce);
	auth["nc"] = std::string(nc);
	auth["response"] = response;
	auth["algorithm"] = "SHA-256";
	return auth;
}

std::string ShellyWsBuildRequest(const int id, const std::string& src, const std::string& method, const Json::Value& params, const Json::Value& auth)
{
	Json::Value request(Json::objectValue);
	request["id"] = id;
	request["src"] = src;
	request["method"] = method;
	if (!params.isNull())
		request["params"] = params;
	if (!auth.isNull())
		request["auth"] = auth;
	return JSonToRawString(request);
}

int ShellyWsNextBackoffSec(const int currentSec, const int maxSeconds)
{
	if (currentSec < 1)
		return 1;
	const int doubled = currentSec > maxSeconds / 2 ? maxSeconds : currentSec * 2;
	return std::min(doubled, maxSeconds);
}

// ---- Connection -----------------------------------------------------------------------------------------

ShellyGen2PushConnection::ShellyGen2PushConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink)
	: ShellyGen2Connection(std::move(node), std::move(credentials), sink)
	, ASyncWebsocket()
{
	SetMaxInboundMessageSize(MAX_FRAME_BYTES);
}

ShellyGen2PushConnection::~ShellyGen2PushConnection()
{
	// The base destructor can no longer reach our StopPush().
	Stop();
}

bool ShellyGen2PushConnection::StartPush()
{
	if (m_thread.joinable())
		return true;
	SetReconnectDelay(0); // the manager thread drives reconnects
	m_quit = false;
	try
	{
		// The thread owns a reference, so the object cannot be destroyed while the manager loop runs. When the owner
		// drops its last reference from a sink callback, the manager thread releases the last one as its final act
		// and the destructor then runs on that thread (see StopPush()).
		m_thread = std::thread([this, self = shared_from_this()] { ManagerLoop(); });
	}
	catch (...)
	{
		_log.Log(LOG_ERROR, "Shelly %s: unable to start the WebSocket thread", m_node.ip.c_str());
		return false;
	}
	return true;
}

void ShellyGen2PushConnection::StopPush()
{
	m_quit = true;
	Wake();
	if (m_thread.joinable())
	{
		// Only the destructor running on the manager thread gets here with that thread as the caller (Stop() from the
		// push thread is deferred). The loop has finished by then, so the thread object is detached instead of joined.
		if (m_thread.get_id() == std::this_thread::get_id())
			m_thread.detach();
		else
			m_thread.join();
	}
	terminate();
}

void ShellyGen2PushConnection::Wake()
{
	std::lock_guard<std::mutex> lock(m_wsMutex);
	m_wsCv.notify_all();
}

bool ShellyGen2PushConnection::ShouldQuit()
{
	if (m_quit)
		return true;
	// Stop() called from a callback only marks the connection stopped.
	OperationGuard operation(*this);
	return !operation.Active();
}

bool ShellyGen2PushConnection::SocketUp() const
{
	return m_open;
}

bool ShellyGen2PushConnection::ConnectAttemptDone() const
{
	return m_open || m_connectFailed;
}

bool ShellyGen2PushConnection::SessionReady() const
{
	return m_ready;
}

bool ShellyGen2PushConnection::SessionSettled() const
{
	return m_ready || m_authFailed || !m_open;
}

void ShellyGen2PushConnection::ServiceAuthRetry()
{
	if (!m_authRetryPending || !m_open || NowMs() < m_authRetryAtMs)
		return;
	if (m_authRetryPending.exchange(false))
		SendKeepalive();
}

void ShellyGen2PushConnection::ResetSocket()
{
	disconnectWS();
	// disconnect() only posts the close to the I/O thread, and terminate() stops the thread without running
	// what is still queued. Wait until the close ran so that the socket is really shut before the thread is stopped.
	auto closed = std::make_shared<std::promise<void>>();
	std::future<void> done = closed->get_future();
	boost::asio::post(m_io_context, [closed] { closed->set_value(); });
	done.wait_for(std::chrono::milliseconds(SOCKET_FLUSH_MS));
	terminate();
}

void ShellyGen2PushConnection::LogAuthFailure()
{
	const int reason = m_authFailReason;
	if (!m_authFailed || reason == 0 || reason == m_loggedAuthReason)
		return;
	m_loggedAuthReason = reason;
	if (reason == AUTH_FAIL_NO_PASSWORD)
		_log.Log(LOG_ERROR, "Shelly %s: WebSocket authentication failed: no password configured", m_node.ip.c_str());
	else if (reason == AUTH_FAIL_REJECTED_LATE)
		_log.Log(LOG_ERROR, "Shelly %s: WebSocket authentication rejected after the session was established, reconnecting", m_node.ip.c_str());
	else
		_log.Log(LOG_ERROR, "Shelly %s: WebSocket authentication failed: password rejected (check the password)", m_node.ip.c_str());
}

// Waits up to `milliseconds` for the condition. False when it did not hold in time or the connection is quitting.
bool ShellyGen2PushConnection::WaitUntil(const int milliseconds, bool (ShellyGen2PushConnection::*condition)() const)
{
	const int64_t deadline = NowMs() + milliseconds;
	for (;;)
	{
		if (ShouldQuit())
			return false;
		// Outside m_wsMutex: sending the retry takes m_authMutex, so the two mutexes are never nested.
		ServiceAuthRetry();
		std::unique_lock<std::mutex> lock(m_wsMutex);
		if ((this->*condition)())
			return true;
		const int64_t remaining = deadline - NowMs();
		if (remaining <= 0)
			return false;
		m_wsCv.wait_for(lock, std::chrono::milliseconds(std::min<int64_t>(remaining, WAIT_SLICE_MS)));
	}
}

void ShellyGen2PushConnection::ManagerLoop()
{
	SetPushThread(std::this_thread::get_id());
	{
		// Held for the life of the loop so that Stop() waits for it.
		OperationGuard operation(*this);
		if (operation.Active())
		{
			int backoff = 1;
			while (!ShouldQuit())
			{
				const bool stable = RunSession();
				if (ShouldQuit())
					break;
				if (stable)
					backoff = 1;
				WaitUntil(backoff * 1000, &ShellyGen2PushConnection::Never);
				backoff = ShellyWsNextBackoffSec(backoff, MAX_BACKOFF_SEC);
			}
		}
		SetPushActive(false);
		disconnectWS();
	}
	SetPushThread(std::thread::id());
}

bool ShellyGen2PushConnection::RunSession()
{
	{
		std::lock_guard<std::mutex> lock(m_authMutex);
		m_challenge = ShellyWsChallenge();
		m_nonceCount = 0;
		m_authSent = false;
	}
	m_open = false;
	m_connectFailed = false;
	m_ready = false;
	m_authFailed = false;
	m_authFailReason = 0;
	m_authRejects = 0;
	m_authRetryPending = false;
	m_wantPoll = false;
	m_lastKeepaliveId = 0;

	connectWS(m_node.ip, static_cast<uint16_t>(m_node.port), "/rpc");
	if (!WaitUntil(CONNECT_TIMEOUT_MS, &ShellyGen2PushConnection::ConnectAttemptDone) || !m_open)
	{
		_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket connect failed", m_node.ip.c_str());
		// terminate() also drops a resolve or connect that is still in flight, so it cannot complete during the backoff.
		ResetSocket();
		return false;
	}
	m_lastRxMs = NowMs();
	SendKeepalive();
	WaitUntil(READY_TIMEOUT_MS, &ShellyGen2PushConnection::SessionSettled);
	if (!m_ready)
	{
		if (m_authFailed)
			LogAuthFailure();
		else
			_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket did not answer", m_node.ip.c_str());
		ResetSocket();
		return false;
	}
	m_loggedAuthReason = 0;
	ReportReachable(true);

	bool baseline = false;
	int64_t activeSince = 0;
	int64_t lastKeepalive = NowMs();
	int64_t lastPoll = 0;
	while (!ShouldQuit())
	{
		if (!m_open || !isConnectedWS() || m_authFailed)
			break;
		const int64_t now = NowMs();
		if (now - m_lastRxMs > LIVENESS_TIMEOUT_MS)
		{
			_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket silent, reconnecting", m_node.ip.c_str());
			break;
		}
		if (now - lastKeepalive >= KEEPALIVE_MS)
		{
			SendKeepalive();
			SendWebsocketPing();
			lastKeepalive = now;
		}
		if ((!baseline || m_wantPoll || ResyncNeeded()) && now - lastPoll >= POLL_RETRY_MS)
		{
			lastPoll = now;
			m_wantPoll = false;
			const ShellyResult result = Poll();
			if (result.ok)
			{
				if (!baseline)
				{
					baseline = true;
					activeSince = NowMs();
					SetPushActive(true);
					_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket push active", m_node.ip.c_str());
				}
			}
			else
				m_wantPoll = true; // try again, HTTP may be briefly unavailable
		}
		WaitUntil(1000, &ShellyGen2PushConnection::Never);
	}
	const bool stable = baseline && (NowMs() - activeSince >= STABLE_SESSION_MS);
	SetPushActive(false);
	if (m_authFailed)
		LogAuthFailure();
	ResetSocket();
	_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket session ended, polling continues", m_node.ip.c_str());
	return stable;
}

void ShellyGen2PushConnection::SendKeepalive()
{
	Json::Value auth;
	{
		std::lock_guard<std::mutex> lock(m_authMutex);
		if (m_challenge.valid)
		{
			auth = ShellyWsBuildAuth(m_challenge, WS_USER, m_credentials.password, ++m_nonceCount, RandomCnonce());
			m_authSent = !auth.isNull();
		}
	}
	const int id = m_nextWsId++;
	m_lastKeepaliveId = id;
	SendWebsocketMessage(ShellyWsBuildRequest(id, WS_SRC, WS_KEEPALIVE_METHOD, Json::Value(), auth));
}

void ShellyGen2PushConnection::HandleAuthChallenge(const ShellyWsFrame& frame)
{
	ShellyWsChallenge challenge;
	if (m_credentials.password.empty())
	{
		m_authFailReason = AUTH_FAIL_NO_PASSWORD;
		m_authFailed = true;
		Wake();
		return;
	}
	if (!ShellyWsParseChallenge(frame.errorMessage, challenge) || ToUpperAscii(challenge.algorithm) != "SHA-256")
	{
		m_authFailReason = AUTH_FAIL_REJECTED;
		m_authFailed = true;
		Wake();
		return;
	}
	bool rejected = false;
	int failReason = 0;
	{
		// Decide under the lock, act after releasing it: SendKeepalive() takes m_authMutex and Wake() takes
		// m_wsMutex, so they are called here without m_authMutex held and the two mutexes are never nested.
		std::lock_guard<std::mutex> lock(m_authMutex);
		rejected = m_authSent;
		// The same nonce rejected again after we answered it: wrong credentials, do not loop.
		const bool sameNonce = m_authSent && m_challenge.valid && !challenge.stale && challenge.nonce == m_challenge.nonce;
		if (sameNonce || (rejected && m_authRejects + 1 >= MAX_AUTH_REJECTS))
		{
			// A session that was already ready had its password accepted before, so say that it was rejected later.
			failReason = m_ready ? AUTH_FAIL_REJECTED_LATE : AUTH_FAIL_REJECTED;
		}
		else
		{
			m_challenge = challenge;
			m_nonceCount = 0;
			m_authSent = false;
		}
	}
	if (failReason != 0)
	{
		m_authFailReason = failReason;
		m_authFailed = true;
		Wake();
		return;
	}
	if (!rejected)
	{
		SendKeepalive(); // first answer to a challenge
		return;
	}
	// A rejected answer: retry after a pause, sent from the manager thread, never in a loop on the I/O thread.
	++m_authRejects;
	m_authRetryAtMs = NowMs() + AUTH_RETRY_MS;
	m_authRetryPending = true;
	Wake();
}

void ShellyGen2PushConnection::ProcessFrame(const ShellyWsFrame& frame)
{
	switch (frame.type)
	{
		case ShellyWsFrameType::NotifyStatus:
		case ShellyWsFrameType::NotifyFullStatus:
			if (!HandleNotifyStatus(frame.params))
				m_wantPoll = true;
			break;
		case ShellyWsFrameType::NotifyEvent:
			HandleNotifyEvent(frame.params);
			break;
		case ShellyWsFrameType::Response:
			if (frame.hasId && frame.id == m_lastKeepaliveId)
			{
				m_authRejects = 0;
				m_ready = true;
				Wake();
			}
			break;
		case ShellyWsFrameType::Error:
			if (frame.hasId && frame.id == m_lastKeepaliveId)
			{
				if (frame.errorCode == 401)
					HandleAuthChallenge(frame);
				else
				{
					// Any other answer proves the channel works.
					m_authRejects = 0;
					m_ready = true;
					Wake();
				}
			}
			break;
		default:
			break;
	}
}

void ShellyGen2PushConnection::OnWebsocketConnected()
{
	m_lastRxMs = NowMs();
	m_open = true;
	Wake();
}

void ShellyGen2PushConnection::OnWebsocketMessage(const WSOpcode opcode, const std::vector<uint8_t>& payload)
{
	try
	{
		m_lastRxMs = NowMs();
		if (opcode != WSOpcode::Text || payload.empty() || payload.size() > MAX_FRAME_BYTES)
			return;
		ShellyWsFrame frame;
		if (!ShellyWsParseFrame(std::string(payload.begin(), payload.end()), frame))
			return;
		ProcessFrame(frame);
	}
	catch (...)
	{
		// a malformed frame must never take the I/O thread down
	}
}

void ShellyGen2PushConnection::OnDisconnect()
{
	m_open = false;
	Wake();
}

void ShellyGen2PushConnection::OnWebsocketUpgradeFailed()
{
	m_connectFailed = true;
	Wake();
}

void ShellyGen2PushConnection::OnError(const boost::system::error_code& error)
{
	_log.Debug(DEBUG_HARDWARE, "Shelly %s: WebSocket error: %s", m_node.ip.c_str(), error.message().c_str());
	m_connectFailed = true; // only consulted while connecting
	Wake();
}
