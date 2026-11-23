#pragma once

// Gen2/Gen3/Gen4 JSON-RPC client. Implements IShellyConnection.
//
// Requests are JSON-RPC 2.0 POSTs to http://<node>/rpc, authenticated with HTTP digest (user "admin").
// Poll() reports the Shelly.GetStatus result object; FetchInfo() reports { "deviceinfo": Shelly.GetDeviceInfo,
// "config": Shelly.GetConfig }.
//
// Push (WebSocket NotifyStatus/NotifyEvent) attaches through the protected seam of ShellyGen2Connection:
// StartPush()/StopPush() are called from Start()/Stop(), and the push thread feeds frames through
// HandleNotifyStatus()/HandleNotifyEvent()/SetPushActive(). HandleNotify* and ReportReachable() bracket
// themselves with an OperationGuard; a push thread should hold one around the rest of its work so that Stop()
// waits for it. A derived class must call Stop() from its own destructor, because the base destructor can no
// longer reach the derived StopPush(). StopPush() may join the push thread but is never invoked from a sink
// callback (Stop() from within a callback only marks the connection stopped, the next Stop() outside a callback,
// or the destructor, calls StopPush() and drains), and the push thread must not call it itself.
// A push thread that may call Stop() outside a callback registers itself with SetPushThread(id): Stop() from
// that thread then also only marks the connection stopped and defers the teardown to the next Stop() from
// another thread. Stop() waits on a lifecycle mutex, so concurrent Stop() calls all return after the teardown
// and Start() cannot race a Stop() into a push thread that is never joined. StartPush() runs under that mutex
// and so must not wait for a thread that is blocked in Stop().
// Ending push: a push source that loses its channel (or finishes) calls SetPushActive(false), which makes HasPush()
// false so the driver goes back to polling, and leaves the connection usable. Stop() is reserved for the owner of
// the connection: it is permanent (the connection never works again), so a push thread must not call it to merely
// end push. If it does call Stop() anyway, the connection is marked stopped for good and the teardown is deferred as
// described above.
//
// Availability: online on the first successful request, offline after OFFLINE_AFTER_FAILURES consecutive
// failures where the node did not answer or answered HTTP 5xx (ShellyResult::IsReachabilityFailure). Bad
// credentials, RPC application errors (whatever their code) and local rejections never mark a node offline.
// The sleepy flag of ShellyNode is not used here: it is a Gen1 notion (battery RPC devices are not handled by
// this client), the driver decides for them.
//
// Sink callbacks are serialized with the state change they report, and none is delivered after Stop() was called
// (also not the remaining ones of an operation whose callback called Stop()). A status older than one already
// delivered (a slow Poll() that finishes after a NotifyStatus) is dropped; HasPush() is then false until a later
// Poll() has been delivered, so that the driver resynchronizes.

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include "ShellyConnection.h"
#include "ShellyHttp.h"

// ---- Pure helpers (no I/O, unit testable) ---------------------------------------------------------------

// Serialized JSON-RPC 2.0 request. `params` is omitted when null.
std::string ShellyRpcBuildRequest(int id, const std::string& method, const Json::Value& params);

// Turns the outcome of the HTTP POST into the outcome of the RPC call. On success `data` holds the "result" member.
// An RPC error (in the body of any HTTP status) becomes a failure whose `error` is "RPC error <code>: <message>";
// its httpStatus is the RPC code when that is a 4xx/5xx code (401 for failed authentication), else the HTTP status.
// Such a failure has `transport` false, so it never counts as a reachability failure, whatever its code.
ShellyResult ShellyRpcInterpretResponse(const ShellyResult& http);

// Translates a generation agnostic command to an RPC method and params. Returns false and sets `error`
// ("unsupported command" or "invalid parameter: <name>") when it cannot.
bool ShellyRpcTranslateCommand(const ShellyCommand& command, std::string& method, Json::Value& params, std::string& error);

// Merges a Gen2+ NotifyStatus params object (partial status, keyed by component, top level "ts" ignored) into a
// full status object. Nested objects are merged recursively, everything else is replaced.
void ShellyRpcMergeStatus(Json::Value& status, const Json::Value& delta);

class ShellyGen2Connection : public IShellyConnection
{
      public:
	ShellyGen2Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink);
	~ShellyGen2Connection() override;

	bool Start() override;
	void Stop() override;
	ShellyResult Poll() override;
	ShellyResult FetchInfo() override;
	ShellyResult SendCommand(const ShellyCommand& command) override;
	bool HasPush() const override;
	const ShellyNode& GetNode() const override
	{
		return m_node;
	}

	// One RPC call over HTTP. After Stop() fails without I/O. Does not touch the availability state.
	ShellyResult RpcCall(const std::string& method, const Json::Value& params = Json::Value());

      protected:
	// Push seam, default implementations do nothing. StartPush returns false to fail Start().
	virtual bool StartPush()
	{
		return true;
	}
	virtual void StopPush()
	{
	}

	// For the push channel. Both return false when the frame could not be applied (no baseline status yet,
	// stopped, or params that is not an object for a status): the caller should then trigger a Poll().
	bool HandleNotifyStatus(const Json::Value& params);
	bool HandleNotifyEvent(const Json::Value& params);
	// The way for a push source to end (or begin) push without stopping the connection, see the file comment.
	void SetPushActive(bool active);
	// Registers the push thread so that Stop() called by it (outside a callback) defers the teardown instead of
	// joining itself. Call with std::thread::id() to unregister.
	void SetPushThread(const std::thread::id id)
	{
		m_pushThread = id;
	}
	// Reports a reachability change from the push channel.
	void ReportReachable(bool reachable);
	// True while a poll was dropped as stale and no newer status was delivered yet; the push source should Poll() again.
	bool ResyncNeeded() const
	{
		return m_resyncNeeded;
	}

	// False once Stop() was called. Every successful BeginOperation() must be matched by EndOperation();
	// prefer OperationGuard, which is exception safe.
	bool BeginOperation();
	void EndOperation();

	class OperationGuard
	{
	      public:
		explicit OperationGuard(ShellyGen2Connection& connection)
			: m_connection(connection)
			, m_active(connection.BeginOperation())
		{
		}
		~OperationGuard()
		{
			if (m_active)
				m_connection.EndOperation();
		}
		OperationGuard(const OperationGuard&) = delete;
		OperationGuard& operator=(const OperationGuard&) = delete;
		// False when the connection was already stopped; no operation is registered then.
		bool Active() const
		{
			return m_active;
		}

	      private:
		ShellyGen2Connection& m_connection;
		const bool m_active;
	};

	const ShellyNode m_node;
	const ShellyCredentials m_credentials;
	IShellyEventSink& m_sink;

      private:
	// Runs sink callbacks with the callback lock held and records which thread is inside one.
	class CallbackScope
	{
	      public:
		explicit CallbackScope(ShellyGen2Connection& c)
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
		ShellyGen2Connection& m_c;
		std::lock_guard<std::recursive_mutex> m_lock;
		std::thread::id m_previous;
	};

	// RpcCall that also tells whether a failure counts against availability (transport failure or HTTP 5xx of
	// the HTTP exchange itself, never an RPC application error).
	ShellyResult RpcCallImpl(const std::string& method, const Json::Value& params, bool& reachabilityFailure);
	bool IsStopped() const;
	// Record* take the callback lock (recursive) and report a change to the sink while holding it.
	void RecordSuccess();
	void RecordFailure();
	// Delivers a status and records its sequence. Caller holds the callback lock and has checked that the
	// sequence is not older than m_deliveredSeq.
	void DeliverStatus(uint64_t seq, const Json::Value& status);

	static constexpr int OFFLINE_AFTER_FAILURES = 3;

	ShellyDigestAuth m_digest;
	std::atomic<int> m_nextId{ 1 };
	std::atomic<bool> m_pushActive{ false };
	std::atomic<bool> m_resyncNeeded{ false }; // a poll was dropped as stale; HasPush() is false until one is delivered
	std::atomic<std::thread::id> m_pushThread{ std::thread::id() };

	// Serializes Start() and the teardown of Stop() (StopPush and drain). Taken first, before every other lock;
	// never taken by Stop() from a sink callback or from the registered push thread.
	std::mutex m_lifecycleMutex;

	// Serializes sink callbacks together with the state change they report, and guards m_status,
	// m_hasStatus and m_deliveredSeq. Lock order: m_callbackMutex before m_mutex.
	std::recursive_mutex m_callbackMutex;
	std::atomic<std::thread::id> m_callbackThread{ std::thread::id() };
	std::atomic<uint64_t> m_nextSeq{ 0 };
	uint64_t m_deliveredSeq = 0;
	Json::Value m_status; // last full status, baseline for NotifyStatus merges
	bool m_hasStatus = false;

	mutable std::mutex m_mutex; // guards the members below
	std::condition_variable m_idle;
	bool m_stopped = false;
	bool m_started = false;
	int m_inFlight = 0;
	int m_failures = 0;
	int m_availability = -1; // -1 unknown, 0 offline, 1 online
};

// node and credentials are copied by the connection; the sink must outlive it.
std::shared_ptr<IShellyConnection> CreateShellyGen2Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink);
