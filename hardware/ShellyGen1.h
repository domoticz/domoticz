#pragma once

// Gen1 (block devices) REST client. Implements IShellyConnection.
//
// Poll()      GET /status   -> IShellyEventSink::OnShellyStatus (the /status document), result.data = /status
// FetchInfo() GET /settings (+ best effort GET /shelly) -> IShellyEventSink::OnShellyNodeInfo, result.data =
//             the /settings document with two added members:
//               "shelly":   the /shelly document (only when it could be read)
//               "dz_sleep": { "sleepy": bool, "period_sec": int } derived from settings.sleep_mode
// SendCommand() translates a ShellyCommand to a GET request (see ShellyGen1BuildCommandPath).
//
// Availability: online on the first successful request, offline after SHELLY_GEN1_OFFLINE_AFTER consecutive
// failed requests (Poll() and FetchInfo(): unreachable or HTTP 5xx, see ShellyResult::IsReachabilityFailure).
// Sleepy (battery) nodes are never reported offline from failed requests; the driver decides for them from
// ShellyNode::sleepPeriodSec.
//
// Push seam: a push source (for example a CoIoT unicast listener) derives from ShellyGen1Connection, overrides
// OnStartPush()/OnStopPush(), and on receipt of a frame takes a sequence with BeginPushReport() and then reports
// the full /status shaped document with ReportPushStatus(seq, status) (so a slow Poll() that started later but
// finishes earlier cannot overwrite a fresher frame). It flags SetPushActive(true) while it works so that
// HasPush() lets the driver relax polling. A derived class must call Stop() from its own destructor, because the
// base destructor can no longer reach the derived overrides.
//
// Stop() from within a sink callback only marks the connection stopped; OnStopPush() is then NOT called from
// that call but from the next Stop() (or the destructor) made outside a callback. OnStopPush() may therefore join
// the push thread, but it is never invoked from a sink callback, and it must not be called by the push thread
// itself. A push thread that may call Stop() outside a callback registers itself with SetPushThread(id): Stop()
// from that thread then also only marks stopped and leaves the teardown to the next Stop() from another thread.
// Ending push: a push source that loses its channel (or finishes) calls SetPushActive(false), which makes
// HasPush() false so the driver goes back to polling, and leaves the connection usable. Stop() is reserved for the
// owner of the connection: it is permanent (the connection never works again), so a push thread must not call it
// to merely end push. If it does call Stop() anyway, the connection is marked stopped for good and the teardown is
// deferred as described above.
// Stop() and Start() are serialized by a lifecycle mutex: concurrent Stop() calls all return after OnStopPush()
// completed and in-flight Poll()/FetchInfo()/SendCommand()/ReportPushStatus() calls drained, and a Start() racing
// a Stop() either fails or is torn down by it. OnStartPush() runs under that mutex, so it must not wait for a
// thread that is blocked in Stop().
//
// Sink callbacks are serialized and none is delivered after Stop(). A /status document older than one already
// delivered (a slow Poll() that finishes after a push report) is dropped.

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include "ShellyConnection.h"
#include "ShellyHttp.h"

constexpr int SHELLY_GEN1_OFFLINE_AFTER = 3;
constexpr int SHELLY_GEN1_TIMEOUT_SEC = 5;

// node and credentials are copied by the connection; the sink must outlive it.
// With coiot.enabled the connection also listens for CoIoT frames (see ShellyGen1Coiot.h).
std::shared_ptr<IShellyConnection> CreateShellyGen1Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink,
							      const ShellyCoiotOptions& coiot = ShellyCoiotOptions());

// Pure helpers (exposed for tests).

// Request path with query ("/relay/0?turn=on") for a command. `model` is the Gen1 model id (ShellyNode::model):
// SHRGBW2 maps the light component to /white/N, bulbs (SHBLB-1, SHCB-1, SHBDUO-1, SHVIN-1) map light, rgb and rgbw to
// /light/N, every other model maps light to /light/N and rgb/rgbw to /color/N. Returns false and sets `error`
// ("unsupported command" or "invalid parameter: <name>") when the command cannot be expressed.
bool ShellyGen1BuildCommandPath(const ShellyCommand& command, const std::string& model, std::string& path, std::string& error);

struct ShellyGen1SleepInfo
{
	bool sleepy = false;
	int periodSec = 0; // 0 when unknown
};
// Reads settings.sleep_mode ({"period": 12, "unit": "h"|"m"}). The unit defaults to hours; a period that is not
// a finite number in (0, 100000] leaves periodSec at 0 (unknown).
ShellyGen1SleepInfo ShellyGen1ParseSleepInfo(const Json::Value& settings);

// Http options for a node: Basic auth is sent preemptively when the node needs auth or a password is configured.
// An empty user name defaults to "admin".
ShellyHttpOptions ShellyGen1HttpOptions(const ShellyNode& node, const ShellyCredentials& credentials, int timeoutSec = SHELLY_GEN1_TIMEOUT_SEC);

// True when a failed request counts against availability (a real HTTP exchange with no answer at all, or a server
// error; never a local rejection). Same as ShellyResult::IsReachabilityFailure().
bool ShellyGen1IsReachabilityFailure(const ShellyResult& result);

class ShellyGen1Connection : public IShellyConnection
{
      public:
	ShellyGen1Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink);
	~ShellyGen1Connection() override;

	bool Start() override;
	void Stop() override;
	ShellyResult Poll() override;
	ShellyResult FetchInfo() override;
	ShellyResult SendCommand(const ShellyCommand& command) override;
	bool HasPush() const override
	{
		return m_pushActive.load() && !m_stopped.load();
	}
	const ShellyNode& GetNode() const override
	{
		return m_node;
	}

      protected:
	// Push seam, see the file comment. OnStartPush is called from Start(), OnStopPush once from Stop().
	virtual bool OnStartPush()
	{
		return true;
	}
	virtual void OnStopPush()
	{
	}
	// Sequence for one push frame; take it when the frame is received, before it is parsed or queued.
	uint64_t BeginPushReport();
	// Delivers a full /status shaped document to the sink and counts as a sign of life. Ignored after Stop() and
	// when a status with a newer sequence was already delivered.
	void ReportPushStatus(uint64_t seq, const Json::Value& status);
	// Registers the push thread so that Stop() called by it (outside a callback) defers the teardown instead of
	// joining itself. Call with std::thread::id() to unregister.
	void SetPushThread(const std::thread::id id)
	{
		m_pushThread = id;
	}
	// The way for a push source to end (or begin) push without stopping the connection, see the file comment.
	void SetPushActive(const bool active)
	{
		m_pushActive = active;
	}
	// True once Stop() was called; a push source checks it between steps so its teardown stays bounded.
	bool IsStopped() const
	{
		return m_stopped.load();
	}
	// True when the calling thread is inside a sink callback of this connection or is the registered push thread.
	bool OnCallbackOrPushThread() const
	{
		const std::thread::id self = std::this_thread::get_id();
		return m_callbackThread.load() == self || m_pushThread.load() == self;
	}

      private:
	enum class Availability
	{
		Unknown,
		Online,
		Offline
	};

	// Registers an in-flight operation; false once stopped. Stop() waits until all registered operations ended.
	bool BeginOperation();
	void EndOperation();
	class OperationGuard
	{
	      public:
		explicit OperationGuard(ShellyGen1Connection& c)
			: m_c(c)
			, m_active(c.BeginOperation())
		{
		}
		~OperationGuard()
		{
			if (m_active)
				m_c.EndOperation();
		}
		OperationGuard(const OperationGuard&) = delete;
		OperationGuard& operator=(const OperationGuard&) = delete;
		bool Active() const
		{
			return m_active;
		}

	      private:
		ShellyGen1Connection& m_c;
		const bool m_active;
	};

	ShellyResult Get(const std::string& path);
	// Records reachability and reports a change to the sink. Takes the callback lock, call without other locks.
	void NoteReachability(bool reachable, bool countFailure);
	// Delivers a status unless it is older than the last delivered one. Caller holds the callback lock.
	void DeliverStatus(uint64_t seq, const Json::Value& status);

	// Runs sink callbacks with the callback lock held and records which thread is inside one.
	class CallbackScope
	{
	      public:
		explicit CallbackScope(ShellyGen1Connection& c)
			: m_c(c)
			, m_lock(c.m_callbackMutex)
			, m_previous(c.m_callbackThread.load())
		{
			c.m_callbackThread = std::this_thread::get_id();
		}
		~CallbackScope()
		{
			m_c.m_callbackThread = m_previous;
		}
		CallbackScope(const CallbackScope&) = delete;
		CallbackScope& operator=(const CallbackScope&) = delete;

	      private:
		ShellyGen1Connection& m_c;
		std::lock_guard<std::recursive_mutex> m_lock;
		std::thread::id m_previous;
	};

	const ShellyNode m_node;
	const ShellyCredentials m_credentials;
	IShellyEventSink& m_sink;
	std::atomic<bool> m_stopped{ false };
	std::atomic<bool> m_pushActive{ false };

	// Serializes sink callbacks; Stop() takes it as a barrier so no callback is in flight after it returns.
	// Recursive so a sink may call back into the connection (Stop, SendCommand) from within a callback.
	std::recursive_mutex m_callbackMutex;
	std::atomic<std::thread::id> m_callbackThread{ std::thread::id() }; // thread currently inside a sink callback
	std::atomic<uint64_t> m_nextSeq{ 0 };
	uint64_t m_deliveredSeq = 0; // guarded by m_callbackMutex
	std::atomic<std::thread::id> m_pushThread{ std::thread::id() };
	// Serializes Start() and the teardown of Stop() (OnStopPush and drain). Taken before every other lock; never
	// taken by Stop() from a sink callback or from the registered push thread.
	std::mutex m_lifecycleMutex;
	bool m_pushStarted = false; // OnStartPush() was called and OnStopPush() is still due; guarded by m_lifecycleMutex
	std::mutex m_stateMutex; // guards m_availability, m_failures, m_inFlight and the m_stopped transition
	std::condition_variable m_idle;
	int m_inFlight = 0;
	Availability m_availability = Availability::Unknown;
	int m_failures = 0;
};
