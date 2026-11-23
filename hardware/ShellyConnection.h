#pragma once

// Generation agnostic connection to one Shelly device.
// The Gen1 (REST) and Gen2+ (RPC) clients implement IShellyConnection; the Shelly driver owns one
// connection per node and receives status through IShellyEventSink.

#include <memory>
#include "ShellyTypes.h"

// Implemented by the driver.
//  - Callbacks may arrive synchronously from within IShellyConnection::Poll() (on the driver thread) and from
//    any thread the connection owns (for example a websocket thread).
//  - No driver lock is held while a callback runs, so a sink may call back into the driver.
//  - Sinks must not block: they are called on the connection's I/O thread.
//  - The node and JSON references are only valid for the duration of the call; copy what is needed.
//  - Every callback has a default implementation that does nothing, except status and availability.
//  - Callbacks of one connection are serialized (never concurrent) and arrive in the order the state changed:
//    availability changes and statuses are not reordered, and a status that is older than one already delivered
//    is dropped instead of delivered late.
//  - A sink must not call Poll() or FetchInfo() of the same connection from within a callback (the status
//    reference passed to OnShellyStatus is only valid during that call and is owned by the connection).
//  - A sink may call Stop() from within a callback. That call only marks the connection stopped (no further
//    callbacks are delivered after the current one returns) and does not join or wait for anything (it does not
//    take the lifecycle lock either); the owner must still call Stop() (or destroy the connection) from another
//    thread to tear down threads owned by the connection. Push implementations must therefore never join their
//    own thread from a callback. The same holds for Stop() called by a registered push thread outside a callback
//    (see SetPushThread() in the Gen1/Gen2 clients).
class IShellyEventSink
{
      public:
	virtual ~IShellyEventSink() = default;
	// Full status of a node. The sink always receives a complete status object: connections that receive
	// partial updates (Gen2+ NotifyStatus) merge them into the previous status themselves before calling.
	// node.gen >= 2: the Shelly.GetStatus result object, keyed by component (for example "switch:0", "cover:0").
	// node.gen == 1: the /status document.
	virtual void OnShellyStatus(const ShellyNode& node, const Json::Value& status) = 0;
	// A node became reachable or unreachable.
	virtual void OnShellyAvailability(const ShellyNode& node, bool online) = 0;
	// A discrete device event (Gen2+ NotifyEvent entry such as a button press, or a Gen1 input event).
	// `event` is the event object as the device reported it.
	virtual void OnShellyEvent(const ShellyNode& /*node*/, const Json::Value& /*event*/)
	{
	}
	// Static device information (Gen2+ Shelly.GetDeviceInfo/GetConfig, Gen1 /settings), reported when a
	// connection fetched it through FetchInfo() or learned of a change.
	virtual void OnShellyNodeInfo(const ShellyNode& /*node*/, const Json::Value& /*info*/)
	{
	}
};

// A connection is shared between the driver and its worker threads (shared_ptr). Poll(), FetchInfo(),
// SendCommand() and Stop() may be called concurrently from different threads and must be safe to do so.
class IShellyConnection
{
      public:
	virtual ~IShellyConnection() = default;
	// Prepare the connection (open push channel where supported). Must not block for long.
	// Returns false when the connection could not be started; the driver then calls Stop() and discards it.
	virtual bool Start() = 0;
	// Callers must hold a shared_ptr reference to the connection while calling Stop(), so the object cannot be destroyed under the call.
	// Stop the connection. Must join any thread the connection owns and must not return while a sink
	// callback on another thread can still run or be in flight. Must be idempotent. Concurrent calls from several
	// threads all return only after the teardown is complete, and a Start() racing with Stop() either fails or is
	// fully torn down by that Stop(). When called from within a sink callback of this connection it only marks the
	// connection stopped and returns (see IShellyEventSink).
	virtual void Stop() = 0;
	// Fetch the full status synchronously and report it to the sink (OnShellyStatus, possibly OnShellyAvailability).
	// After Stop() this returns a failure without any I/O.
	virtual ShellyResult Poll() = 0;
	// Fetch static device information: Gen2+ Shelly.GetConfig/GetDeviceInfo, Gen1 /settings. The result's
	// `data` holds the document. After Stop() this returns a failure without any I/O.
	virtual ShellyResult FetchInfo()
	{
		return ShellyResult::Failure(0, "not supported");
	}
	// Execute a control request. Success means the device accepted it. After Stop() this returns a failure
	// without any I/O.
	virtual ShellyResult SendCommand(const ShellyCommand& command) = 0;
	// True when a push channel is active and trustworthy (not stopped, no resync pending), so that polling can be
	// relaxed.
	virtual bool HasPush() const
	{
		return false;
	}
	virtual const ShellyNode& GetNode() const = 0;
};

// Optional Gen1 CoIoT (unicast CoAP push) settings of a hardware instance. With `enabled` false (the default) Gen1
// nodes are pure REST polled and nothing is bound.
struct ShellyCoiotOptions
{
	bool enabled = false;
	int listenPort = 5683;     // UDP port Domoticz listens on (shared by all nodes of this process)
	std::string peerHost;      // address of this host as the devices see it; empty: do not configure the devices
	int peerPort = 0;          // port written to the device, 0 means listenPort (differs behind a port mapping)
	int updatePeriodSec = 15;  // coiot_update_period written to the device (see ShellyClampCoiotPeriod)
};

constexpr int SHELLY_COIOT_MIN_PERIOD_SEC = 15; // the lowest coiot_update_period a device accepts, and the default
constexpr int SHELLY_COIOT_MAX_PERIOD_SEC = 65535;

// The coiot_update_period to use for a requested value: 15 when it is missing or not positive, else clamped to 15..65535.
inline int ShellyClampCoiotPeriod(const int seconds)
{
	if (seconds <= 0)
		return SHELLY_COIOT_MIN_PERIOD_SEC;
	return seconds < SHELLY_COIOT_MIN_PERIOD_SEC ? SHELLY_COIOT_MIN_PERIOD_SEC : (seconds > SHELLY_COIOT_MAX_PERIOD_SEC ? SHELLY_COIOT_MAX_PERIOD_SEC : seconds);
}

// Creates the client matching node.gen (see ShellyConnection.cpp). Returns nullptr for an unknown generation
// or when the client for that generation is not available. The factory copies everything it needs from its
// arguments (node and credentials are taken by value); only the sink must outlive the connection.
// `coiot` only affects Gen1 nodes.
std::shared_ptr<IShellyConnection> CreateShellyConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink,
							  const ShellyCoiotOptions& coiot = ShellyCoiotOptions());
