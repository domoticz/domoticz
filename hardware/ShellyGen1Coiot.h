#pragma once

// Gen1 connection with CoIoT push (unicast CoAP over UDP). Derives from ShellyGen1Connection and attaches to its
// push seam. Only created when the hardware opted in (ShellyCoiotOptions::enabled); otherwise Gen1 stays REST polled.
//
// Design: a CoIoT status frame ({"G":[[block,id,value],...]}, POST /cit/s) is used as a change trigger only. The
// sensor ids are model specific and only meaningful together with the /cit/d description of each device, and the
// frame lacks parts of the /status document that the mapping layer consumes (wifi, update, per-meter fields...).
// A validated frame from this node therefore wakes a worker thread that fetches /status over HTTP (Poll(), at
// most one fetch per second), so the sink keeps receiving the same full /status object as with plain polling and
// no second decoder has to be kept in sync with the device firmware.
//
// One UDP socket per listen port is shared by all Gen1 connections of the process (ShellyCoiotListener, reference
// counted). Frames are routed to a connection by source address and, when the frame names its device, by MAC.
//
// A status frame is a non-confirmable request with the non-standard CoAP code 0.30 on /cit/s (Shelly Gen1 API
// documentation); a standard POST (0.02) is accepted too. A confirmable status frame from a registered node is
// acknowledged; a confirmable message that is not a status frame is answered with a reset; a status frame from a
// sender that matches no registered node is dropped without any reply. A socket that keeps failing is reopened with
// a growing delay.
//
// HasPush() is true only while a frame arrived within three update periods and the last triggered fetch worked, so
// the driver relaxes polling only while push demonstrably works and falls back to polling by itself otherwise.
//
// Device configuration (best effort, only when ShellyCoiotOptions::peerHost is set): the worker thread reads
// /settings and, when the CoIoT settings differ, writes /settings?coiot_enable=true&coiot_update_period=N&coiot_peer=host:port,
// retrying every minute until it succeeds. While push does not work the settings are checked again now and then
// (the interval doubles up to an hour). Failures never affect polling.
//
// Lifetime: the owner must Stop() the connection from a thread other than the push worker or a sink callback before it
// releases its last reference (that Stop() joins the worker). The connection must be owned by a shared_ptr (it is created
// with make_shared): the worker holds a reference to it for as long as it runs, so the destructor can never run while
// the worker is active. When the owner drops its reference from a callback or the worker thread, the worker (which then
// has been told to leave by the deferred Stop()) releases the last reference as its final act and the destructor runs on
// the worker thread, which detaches the already finished thread object.
// Callers of Stop() must hold a shared_ptr reference to the connection for the duration of the call: Stop() joins
// the worker, and the worker's own reference is released only after it left, so a caller using a raw pointer or a
// reference to an object that may be destroyed concurrently would race with the destructor.
// Not stopping at all keeps the worker, and with it the connection, alive and polling: Stop() is mandatory.

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include "ShellyCoap.h"
#include "ShellyGen1.h"

// What a registered handler learns about a validated frame.
struct ShellyCoiotFrame
{
	std::string sourceIp;
	std::string deviceMac; // MAC part of the device id option, empty when absent
	size_t sensorCount = 0;
};

// Pure routing rule (exposed for tests): does a frame from `sourceIp` naming `frameMac` belong to the node?
// `nodeHost` is the node address (literal or host name), `nodeMac` its normalized MAC. A literal node address must
// equal the source; a host name can only be matched through the MAC. A frame MAC that is present must match.
bool ShellyCoiotFrameBelongsToNode(const std::string& sourceIp, const std::string& frameMac, const std::string& nodeHost, const std::string& nodeMac);

// Shared UDP receiver. Handlers are invoked on the receiver thread with the registry lock held, so they must be
// quick and must not call back into the listener. Unregister() returns only when no handler call is in flight.
class ShellyCoiotListener
{
      public:
	using Handler = std::function<void(const ShellyCoiotFrame&)>;

	// Returns the listener bound to `port`, creating it when needed; nullptr when the port cannot be bound.
	static std::shared_ptr<ShellyCoiotListener> Acquire(int port);

	int Register(const std::string& nodeHost, const std::string& nodeMac, Handler handler);
	void Unregister(int token);
	int Port() const
	{
		return m_port;
	}

	ShellyCoiotListener(const ShellyCoiotListener&) = delete;
	ShellyCoiotListener& operator=(const ShellyCoiotListener&) = delete;

      private:
	explicit ShellyCoiotListener(int port);
	~ShellyCoiotListener();
	bool Open();
	void Close();
	bool BindSocket(std::string& error);
	void StartReceive();
	void ScheduleReopen();
	void Reopen();
	void HandleDatagram(size_t size);
	bool HasMatchingRegistration(const std::string& sourceIp, const std::string& frameMac);
	void NoteDropped(const ShellyCoapMessage& message, const char* reason);

	struct Registration
	{
		std::string host;
		std::string mac;
		Handler handler;
	};

	// The Boost.Asio objects (io context, socket, sender endpoint, retry timer, receive buffer) live in the
	// implementation file so that this header does not pull in Boost.Asio.
	struct Net;

	const int m_port;
	std::unique_ptr<Net> m_net;
	int m_errors = 0;         // receiver thread only, consecutive receive errors
	int m_backoffSec = 1;     // receiver thread only, delay before the next reopen attempt
	bool m_reopenFailureLogged = false; // receiver thread only, a failed reopen was logged and the socket is still down
	int64_t m_lastDropLogMs = 0; // receiver thread only
	unsigned m_dropped = 0;      // receiver thread only, frames dropped since the last log line
	std::thread m_thread;
	std::mutex m_mutex; // guards m_registrations
	std::map<int, Registration> m_registrations;
	int m_nextToken = 1;
};

class ShellyGen1CoiotConnection : public ShellyGen1Connection, public std::enable_shared_from_this<ShellyGen1CoiotConnection>
{
      public:
	ShellyGen1CoiotConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink, ShellyCoiotOptions options);
	~ShellyGen1CoiotConnection() override;

	bool HasPush() const override;

      protected:
	bool OnStartPush() override;
	void OnStopPush() override;

      private:
	struct WorkState
	{
		std::mutex mutex;
		std::condition_variable cv;
		bool quit = false;    // guarded by mutex
		bool trigger = false; // guarded by mutex
	};

	void OnFrame(const ShellyCoiotFrame& frame);
	void WorkerLoop();
	void JoinOrDetachWorker();
	bool ConfigurePeer(); // true when the device is (now) configured or nothing needs to be done
	static bool WaitFor(WorkState& state, std::chrono::milliseconds duration); // true when the worker must quit

	const ShellyCredentials m_creds;
	const ShellyCoiotOptions m_options;
	const int m_periodSec;

	std::shared_ptr<ShellyCoiotListener> m_listener;
	int m_token = 0;
	std::thread m_thread;
	WorkState m_state;
	std::atomic<int64_t> m_lastFrameMs{ 0 }; // steady clock, 0: none yet
};

std::shared_ptr<IShellyConnection> CreateShellyGen1CoiotConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink,
								   const ShellyCoiotOptions& options);
