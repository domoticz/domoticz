#pragma once

// Gen2/Gen3/Gen4 connection with WebSocket push. Derives from ShellyGen2Connection and attaches to its push seam.
//
// A client WebSocket is opened to ws://<ip>:<port>/rpc. The device pushes NotifyStatus, NotifyFullStatus and
// NotifyEvent frames to a client that has sent at least one request with a "src". The connection therefore sends a
// small keepalive request (Sys.GetStatus) every KEEPALIVE_SEC seconds together with a WebSocket ping; the answer
// to it is also the liveness signal (the socket is dropped when nothing was received for LIVENESS_TIMEOUT_SEC).
// Devices with authentication answer a request without credentials with a 401 error frame carrying a digest
// challenge; the request is then repeated with an "auth" object (SHA-256, see ShellyWsBuildAuth).
//
// Baseline: after the socket is usable the connection calls Poll() (HTTP Shelly.GetStatus) so that HandleNotifyStatus
// has a status to merge into; push is reported active (HasPush) only once that baseline was delivered. A notification
// that cannot be applied (no baseline, resync pending) schedules a new Poll().
//
// Fallback: when the socket is down HasPush() is false and the driver keeps polling over HTTP as before. The socket
// is reopened with an exponential backoff (1, 2, 4 ... capped at MAX_BACKOFF_SEC seconds).
//
// Threads: one manager thread (connect, keepalive, baseline polls, backoff) and the I/O thread of ASyncTCP, which
// delivers the frames. Everything shared between them is atomic or guarded by m_wsMutex.

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "ASyncWebsocket.h"
#include "ShellyGen2.h"

// ---- Pure helpers (no I/O, unit testable) ---------------------------------------------------------------

enum class ShellyWsFrameType
{
	Invalid,        // not JSON, not an object, or neither a response nor a notification
	Response,       // has "id" and "result"
	Error,          // has "id" and "error"
	NotifyStatus,   // method NotifyStatus, params is an object
	NotifyFullStatus, // method NotifyFullStatus, params is an object
	NotifyEvent,    // method NotifyEvent, params is an object
	OtherNotification // any other method with no id
};

struct ShellyWsFrame
{
	ShellyWsFrameType type = ShellyWsFrameType::Invalid;
	bool hasId = false;
	int id = 0;
	std::string src;
	std::string method;
	Json::Value params;   // notifications: "params"; Response: "result"
	int errorCode = 0;    // Error only
	std::string errorMessage;
};

// Parses one text frame. Never throws; returns false (type Invalid) for anything that is not a usable frame.
bool ShellyWsParseFrame(const std::string& text, ShellyWsFrame& frame);

struct ShellyWsChallenge
{
	bool valid = false;
	std::string realm;
	std::string nonce;
	bool nonceIsNumber = false; // pre-2.0 firmware sends the nonce as a JSON integer and expects it echoed as one
	std::string algorithm = "SHA-256";
	bool stale = false;
};

// Parses the "message" of a 401 error frame (a JSON string with auth_type, nonce, realm, algorithm, stale).
// Returns false unless it is a digest challenge with a realm and a nonce. Never throws.
bool ShellyWsParseChallenge(const std::string& errorMessage, ShellyWsChallenge& challenge);

// The "auth" object of a request: realm, username, nonce, cnonce, nc ("%08x"), response, algorithm. Only
// SHA-256 is supported: returns a null value for another algorithm or an invalid challenge.
// response = SHA256(HA1:nonce:nc:cnonce:auth:HA2), HA1 = SHA256(user:realm:password), HA2 = SHA256("dummy_method:dummy_uri").
Json::Value ShellyWsBuildAuth(const ShellyWsChallenge& challenge, const std::string& username, const std::string& password, uint32_t nonceCount, uint32_t cnonce);

// Serialized request frame: id, src, method, params (omitted when null) and auth (omitted when null).
std::string ShellyWsBuildRequest(int id, const std::string& src, const std::string& method, const Json::Value& params, const Json::Value& auth);

// 1 s, doubling, capped at maxSeconds.
int ShellyWsNextBackoffSec(int currentSec, int maxSeconds);

// Must be owned by a shared_ptr (it is created with make_shared): the manager thread holds a reference for as long
// as it runs, so Stop() is mandatory before the owner releases its reference. Stop() joins the manager thread, so it
// must not be called from that thread or from an I/O callback.
class ShellyGen2PushConnection : public ShellyGen2Connection, private ASyncWebsocket, public std::enable_shared_from_this<ShellyGen2PushConnection>
{
      public:
	ShellyGen2PushConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink);
	~ShellyGen2PushConnection() override;

      protected:
	bool StartPush() override;
	void StopPush() override;

      private:
	// ASyncWebsocket / ASyncTCP callbacks, run on the I/O thread.
	void OnWebsocketConnected() override;
	void OnWebsocketMessage(WSOpcode opcode, const std::vector<uint8_t>& payload) override;
	void OnDisconnect() override;
	void OnWebsocketUpgradeFailed() override;
	void OnError(const boost::system::error_code& error) override;

	void ManagerLoop();
	bool RunSession(); // true when the session reached the baseline (push was active)
	bool WaitUntil(int milliseconds, bool (ShellyGen2PushConnection::*condition)() const);
	bool ShouldQuit();
	bool SocketUp() const;
	bool ConnectAttemptDone() const; // the socket is up or the attempt failed
	bool SessionReady() const;
	bool SessionSettled() const; // ready, rejected or the socket is gone
	void ServiceAuthRetry();     // sends the delayed answer to a rejected auth attempt, manager thread
	void ResetSocket();          // closes the socket and the I/O thread so the next session starts fresh
	void LogAuthFailure();       // once per change of the failure reason
	bool Never() const
	{
		return false;
	}
	void Wake();

	void SendKeepalive();
	void HandleAuthChallenge(const ShellyWsFrame& frame);
	void ProcessFrame(const ShellyWsFrame& frame);

	std::thread m_thread;
	std::atomic<bool> m_quit{ false };
	std::mutex m_wsMutex;
	std::condition_variable m_wsCv;

	std::atomic<bool> m_open{ false };         // upgraded socket is up
	std::atomic<bool> m_connectFailed{ false }; // the current connect attempt failed (TCP error or refused upgrade)
	std::atomic<bool> m_ready{ false };        // the device answered a request (authenticated when needed)
	std::atomic<bool> m_authFailed{ false };   // credentials missing or rejected for this session
	std::atomic<bool> m_wantPoll{ false };     // a notification could not be applied, refresh the baseline
	std::atomic<int64_t> m_lastRxMs{ 0 };      // steady clock, milliseconds
	std::atomic<int> m_authFailReason{ 0 };    // AUTH_FAIL_* of the current session
	std::atomic<int> m_authRejects{ 0 };       // auth answers rejected in a row
	std::atomic<bool> m_authRetryPending{ false };
	std::atomic<int64_t> m_authRetryAtMs{ 0 };
	int m_loggedAuthReason = 0;                // manager thread only
	std::atomic<int> m_nextWsId{ 1 };
	std::atomic<int> m_lastKeepaliveId{ 0 }; // only the answer to the latest keepalive is acted upon

	std::mutex m_authMutex; // guards the three members below
	ShellyWsChallenge m_challenge;
	uint32_t m_nonceCount = 0;
	bool m_authSent = false; // the current nonce was already used in a request
};
