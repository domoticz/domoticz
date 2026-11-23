#include "stdafx.h"
#include "ShellyGen2.h"
#include "ShellyGen2Push.h"
#include <algorithm>
#include <cmath>
#include "../main/json_helper.h"
#include "../main/Logger.h"
#include "ShellyMapping.h"

namespace
{
	constexpr int RPC_TIMEOUT_SEC = 5;
	constexpr size_t RPC_MAX_BODY_BYTES = 512 * 1024;
	const char* RPC_USER = "admin";

	// Thermostat target range in degrees Celsius. Differs from Gen1 (4..31) and is unverified on real devices;
	// the device rejects values outside what it supports.
	constexpr double THERMOSTAT_MIN_C = 5.0;
	constexpr double THERMOSTAT_MAX_C = 35.0;

	std::string InvalidParameter(const char* key)
	{
		return std::string("invalid parameter: ") + key;
	}

	// Reads a numeric param that must lie in [lo, hi] (rejected, not clamped). False when it is missing, not a
	// finite number or out of range.
	bool GetNumber(const Json::Value& params, const char* key, const double lo, const double hi, double& out)
	{
		if (!params.isObject() || !params.isMember(key) || !params[key].isNumeric())
			return false;
		const double v = params[key].asDouble();
		if (!std::isfinite(v) || v < lo || v > hi)
			return false;
		out = v;
		return true;
	}

	bool HasParam(const Json::Value& params, const char* key)
	{
		return params.isObject() && params.isMember(key);
	}

	bool IsCoverComponent(const std::string& component)
	{
		return component == ShellyCmd::COMPONENT_COVER || component == ShellyCmd::COMPONENT_ROLLER;
	}

	bool TranslateColor(const ShellyCommand& command, const bool withWhite, Json::Value& params, std::string& error)
	{
		double red, green, blue;
		if (!GetNumber(command.params, ShellyCmd::PARAM_RED, 0, 255, red))
		{
			error = InvalidParameter(ShellyCmd::PARAM_RED);
			return false;
		}
		if (!GetNumber(command.params, ShellyCmd::PARAM_GREEN, 0, 255, green))
		{
			error = InvalidParameter(ShellyCmd::PARAM_GREEN);
			return false;
		}
		if (!GetNumber(command.params, ShellyCmd::PARAM_BLUE, 0, 255, blue))
		{
			error = InvalidParameter(ShellyCmd::PARAM_BLUE);
			return false;
		}
		params["on"] = true;
		params["rgb"].append(static_cast<int>(red + 0.5));
		params["rgb"].append(static_cast<int>(green + 0.5));
		params["rgb"].append(static_cast<int>(blue + 0.5));
		if (withWhite)
		{
			// Optional, as for Gen1: without it the device keeps its white channel.
			if (HasParam(command.params, ShellyCmd::PARAM_WHITE))
			{
				double white;
				if (!GetNumber(command.params, ShellyCmd::PARAM_WHITE, 0, 255, white))
				{
					error = InvalidParameter(ShellyCmd::PARAM_WHITE);
					return false;
				}
				params["white"] = static_cast<int>(white + 0.5);
			}
		}
		if (HasParam(command.params, ShellyCmd::PARAM_BRIGHTNESS))
		{
			double brightness;
			if (!GetNumber(command.params, ShellyCmd::PARAM_BRIGHTNESS, 0, 100, brightness))
			{
				error = InvalidParameter(ShellyCmd::PARAM_BRIGHTNESS);
				return false;
			}
			params["brightness"] = static_cast<int>(brightness + 0.5);
		}
		return true;
	}
} // namespace

std::string ShellyRpcBuildRequest(const int id, const std::string& method, const Json::Value& params)
{
	Json::Value request(Json::objectValue);
	request["jsonrpc"] = "2.0";
	request["id"] = id;
	request["method"] = method;
	if (!params.isNull())
		request["params"] = params;
	return JSonToRawString(request);
}

ShellyResult ShellyRpcInterpretResponse(const ShellyResult& http)
{
	const Json::Value& doc = http.data;
	if (doc.isObject() && doc.isMember("error") && doc["error"].isObject())
	{
		const Json::Value& err = doc["error"];
		const int code = err.isMember("code") && err["code"].isInt() ? err["code"].asInt() : 0;
		const std::string message = err.isMember("message") && err["message"].isString() ? err["message"].asString() : std::string("unknown");
		ShellyResult r = ShellyResult::Failure((code >= 400 && code < 600) ? code : http.httpStatus, "RPC error " + std::to_string(code) + ": " + message);
		r.body = http.body;
		r.wwwAuthenticate = http.wwwAuthenticate;
		r.data = doc["error"];
		return r;
	}
	if (!http.ok)
		return http;
	if (!doc.isObject() || !doc.isMember("result"))
	{
		ShellyResult r = ShellyResult::Failure(http.httpStatus, "invalid RPC response");
		r.body = http.body;
		return r;
	}
	ShellyResult r = ShellyResult::Success(http.httpStatus, doc["result"]);
	r.body = http.body;
	return r;
}

bool ShellyRpcTranslateCommand(const ShellyCommand& command, std::string& method, Json::Value& params, std::string& error)
{
	namespace C = ShellyCmd;
	method.clear();
	params = Json::Value(Json::objectValue);
	params["id"] = command.channel;
	const std::string& comp = command.component;
	const std::string& act = command.action;
	error = "unsupported command";

	if (!ShellyIsValidChannel(command.channel))
	{
		error = "invalid parameter: channel";
		return false;
	}

	if (comp == C::COMPONENT_SWITCH)
	{
		if (act == C::ACTION_ON || act == C::ACTION_OFF)
		{
			method = "Switch.Set";
			params["on"] = (act == C::ACTION_ON);
		}
		else if (act == C::ACTION_TOGGLE)
			method = "Switch.Toggle";
		else
			return false;
		return true;
	}
	if (comp == C::COMPONENT_LIGHT || comp == SHELLY_COMPONENT_CCT)
	{
		// cct:n takes the same parameters as light:n (the colour temperature is optional and not set here)
		const std::string prefix = (comp == SHELLY_COMPONENT_CCT) ? "CCT" : "Light";
		double brightness;
		if (act == C::ACTION_ON || act == C::ACTION_OFF)
		{
			method = prefix + ".Set";
			params["on"] = (act == C::ACTION_ON);
			if (act == C::ACTION_ON && HasParam(command.params, C::PARAM_BRIGHTNESS))
			{
				if (!GetNumber(command.params, C::PARAM_BRIGHTNESS, 0, 100, brightness))
					{
						error = InvalidParameter(C::PARAM_BRIGHTNESS);
						return false;
					}
				// Brightness 0 means off, as for Gen1.
				const int level = static_cast<int>(brightness + 0.5);
				if (level == 0)
					params["on"] = false;
				else
					params["brightness"] = level;
			}
		}
		else if (act == C::ACTION_TOGGLE)
			method = prefix + ".Toggle";
		else if (act == C::ACTION_SET_BRIGHTNESS)
		{
			if (!GetNumber(command.params, C::PARAM_BRIGHTNESS, 0, 100, brightness))
			{
				error = InvalidParameter(C::PARAM_BRIGHTNESS);
				return false;
			}
			method = prefix + ".Set";
			const int level = static_cast<int>(brightness + 0.5);
			params["on"] = level > 0;
			if (level > 0)
				params["brightness"] = level;
		}
		else
			return false;
		return true;
	}
	if (comp == C::COMPONENT_RGB || comp == C::COMPONENT_RGBW)
	{
		const bool rgbw = (comp == C::COMPONENT_RGBW);
		const std::string prefix = rgbw ? "RGBW" : "RGB";
		if (act == C::ACTION_ON || act == C::ACTION_OFF)
		{
			method = prefix + ".Set";
			params["on"] = (act == C::ACTION_ON);
		}
		else if (act == C::ACTION_TOGGLE)
			method = prefix + ".Toggle";
		else if (act == C::ACTION_SET_COLOR)
		{
			if (!TranslateColor(command, rgbw, params, error))
				return false;
			method = prefix + ".Set";
		}
		else
			return false;
		return true;
	}
	if (IsCoverComponent(comp))
	{
		if (act == C::ACTION_OPEN)
			method = "Cover.Open";
		else if (act == C::ACTION_CLOSE)
			method = "Cover.Close";
		else if (act == C::ACTION_STOP)
			method = "Cover.Stop";
		else if (act == C::ACTION_SET_POSITION)
		{
			double position;
			if (!GetNumber(command.params, C::PARAM_POSITION, 0, 100, position))
			{
				error = InvalidParameter(C::PARAM_POSITION);
				return false;
			}
			method = "Cover.GoToPosition";
			params["pos"] = static_cast<int>(position + 0.5);
		}
		else
			return false;
		return true;
	}
	if (comp == C::COMPONENT_THERMOSTAT)
	{
		if (act != C::ACTION_SET_TEMPERATURE)
			return false;
		double temperature;
		if (!GetNumber(command.params, C::PARAM_TEMPERATURE, THERMOSTAT_MIN_C, THERMOSTAT_MAX_C, temperature))
		{
			error = InvalidParameter(C::PARAM_TEMPERATURE);
			return false;
		}
		method = "Thermostat.SetConfig";
		params["config"]["target_C"] = temperature;
		return true;
	}
	return false;
}

static void MergeStatusLevel(Json::Value& status, const Json::Value& delta, const bool topLevel)
{
	if (!delta.isObject())
		return;
	if (!status.isObject())
		status = Json::Value(Json::objectValue);
	for (const auto& key : delta.getMemberNames())
	{
		// "ts" is the frame timestamp at the top level only; a component may have a member of that name.
		if (topLevel && key == "ts")
			continue;
		const Json::Value& value = delta[key];
		if (value.isObject() && status.isMember(key) && status[key].isObject())
			MergeStatusLevel(status[key], value, false);
		else
			status[key] = value;
	}
}

void ShellyRpcMergeStatus(Json::Value& status, const Json::Value& delta)
{
	MergeStatusLevel(status, delta, true);
}

ShellyGen2Connection::ShellyGen2Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink)
	: m_node(std::move(node))
	, m_credentials(std::move(credentials))
	, m_sink(sink)
{
}

ShellyGen2Connection::~ShellyGen2Connection()
{
	const std::thread::id self = std::this_thread::get_id();
	if (m_callbackThread.load() == self || m_pushThread.load() == self)
		_log.Debug(DEBUG_HARDWARE, "Shelly %s: destroyed from a callback or push thread, push teardown deferred (the owner must Stop() from another thread first)", m_node.ip.c_str());
	Stop();
}

bool ShellyGen2Connection::BeginOperation()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_stopped)
		return false;
	++m_inFlight;
	return true;
}

void ShellyGen2Connection::EndOperation()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (--m_inFlight == 0)
		m_idle.notify_all();
}

bool ShellyGen2Connection::IsStopped() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_stopped;
}

bool ShellyGen2Connection::Start()
{
	// Serialized with Stop(): a Start() that wins the race is fully torn down by the Stop() that waits for it.
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_stopped)
			return false;
		m_started = true;
	}
	return StartPush();
}

void ShellyGen2Connection::Stop()
{
	// From within a sink callback of this connection, or from the registered push thread: only mark stopped.
	// StopPush() may join a thread that is waiting for the callback lock this thread holds (or is this thread),
	// and the drain below would wait for our own operation. No lifecycle lock wait either, the thread that is
	// tearing down may be waiting for us. The next Stop() from another thread (or the destructor) does the teardown.
	const std::thread::id self = std::this_thread::get_id();
	const bool deferTeardown = (m_callbackThread.load() == self) || (m_pushThread.load() == self);
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_stopped = true;
	}
	m_pushActive = false;
	if (deferTeardown)
		return;
	// Every concurrent Stop() waits here until the first one finished the teardown.
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	bool stopPush;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		stopPush = m_started;
		m_started = false;
	}
	// The push channel is shut down first (it joins its thread); in-flight operations then drain.
	if (stopPush)
		StopPush();
	std::unique_lock<std::mutex> lock(m_mutex);
	m_idle.wait(lock, [this] { return m_inFlight == 0; });
}

bool ShellyGen2Connection::HasPush() const
{
	return m_pushActive && !m_resyncNeeded && !IsStopped();
}

void ShellyGen2Connection::SetPushActive(const bool active)
{
	m_pushActive = active;
}

ShellyResult ShellyGen2Connection::RpcCallImpl(const std::string& method, const Json::Value& params, bool& reachabilityFailure)
{
	reachabilityFailure = false;
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");

	ShellyHttpOptions options;
	options.timeoutSec = RPC_TIMEOUT_SEC;
	options.maxBodyBytes = RPC_MAX_BODY_BYTES;
	if (!m_credentials.password.empty())
	{
		options.auth = ShellyHttpAuth::Digest;
		options.username = RPC_USER;
		options.password = m_credentials.password;
		options.digest = &m_digest;
	}
	const std::string body = ShellyRpcBuildRequest(m_nextId++, method, params);
	const ShellyResult http = ShellyHttpPost(ShellyBaseUrl(m_node) + "/rpc", body, options).ToResult();
	// Judged on the HTTP exchange: an RPC application error (or 401) says the node is there.
	reachabilityFailure = http.IsReachabilityFailure();
	return ShellyRpcInterpretResponse(http);
}

ShellyResult ShellyGen2Connection::RpcCall(const std::string& method, const Json::Value& params)
{
	bool reachabilityFailure;
	return RpcCallImpl(method, params, reachabilityFailure);
}

void ShellyGen2Connection::RecordSuccess()
{
	CallbackScope callback(*this);
	bool changed;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_stopped)
			return;
		m_failures = 0;
		changed = (m_availability != 1);
		m_availability = 1;
	}
	if (changed && !IsStopped())
		m_sink.OnShellyAvailability(m_node, true);
}

void ShellyGen2Connection::RecordFailure()
{
	CallbackScope callback(*this);
	bool changed = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_stopped)
			return;
		if (++m_failures >= OFFLINE_AFTER_FAILURES)
		{
			m_failures = OFFLINE_AFTER_FAILURES;
			changed = (m_availability != 0);
			m_availability = 0;
		}
	}
	if (changed && !IsStopped())
		m_sink.OnShellyAvailability(m_node, false);
}

void ShellyGen2Connection::DeliverStatus(const uint64_t seq, const Json::Value& status)
{
	// A sink may have called Stop() from an earlier callback of the same operation.
	if (IsStopped())
		return;
	m_deliveredSeq = seq;
	m_sink.OnShellyStatus(m_node, status);
}

void ShellyGen2Connection::ReportReachable(const bool reachable)
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return;
	if (reachable)
		RecordSuccess();
	else
		RecordFailure();
}

ShellyResult ShellyGen2Connection::Poll()
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");
	// The sequence is taken before the request: a notification delivered while it is in flight is newer.
	const uint64_t seq = ++m_nextSeq;
	bool reachabilityFailure;
	ShellyResult result = RpcCallImpl("Shelly.GetStatus", Json::Value(), reachabilityFailure);
	if (result.ok && !result.data.isObject())
		result = ShellyResult::Failure(result.httpStatus, "invalid status");
	if (!result.ok)
	{
		if (reachabilityFailure)
			RecordFailure();
		return result;
	}
	CallbackScope callback(*this);
	if (IsStopped())
		return result;
	RecordSuccess();
	if (seq >= m_deliveredSeq)
	{
		m_status = result.data;
		m_hasStatus = true;
		m_resyncNeeded = false;
		DeliverStatus(seq, m_status);
	}
	else
	{
		// A newer notification already went out and this poll was dropped: the notification state is not
		// confirmed by a poll, so HasPush() stays false until one is delivered.
		m_resyncNeeded = true;
	}
	return result;
}

ShellyResult ShellyGen2Connection::FetchInfo()
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");
	bool reachabilityFailure = false;
	ShellyResult info = RpcCallImpl("Shelly.GetDeviceInfo", Json::Value(), reachabilityFailure);
	ShellyResult config;
	if (info.ok && !info.data.isObject())
		info = ShellyResult::Failure(info.httpStatus, "invalid device info");
	if (info.ok)
	{
		config = RpcCallImpl("Shelly.GetConfig", Json::Value(), reachabilityFailure);
		if (config.ok && !config.data.isObject())
			config = ShellyResult::Failure(config.httpStatus, "invalid config");
	}
	const ShellyResult& failed = info.ok ? config : info;
	if (!failed.ok)
	{
		if (reachabilityFailure)
			RecordFailure();
		return failed;
	}
	Json::Value doc(Json::objectValue);
	doc["deviceinfo"] = std::move(info.data);
	doc["config"] = std::move(config.data);
	CallbackScope callback(*this);
	if (!IsStopped())
	{
		RecordSuccess();
		// RecordSuccess() may have delivered a callback that stopped the connection.
		if (!IsStopped())
			m_sink.OnShellyNodeInfo(m_node, doc);
	}
	return ShellyResult::Success(config.httpStatus, std::move(doc));
}

ShellyResult ShellyGen2Connection::SendCommand(const ShellyCommand& command)
{
	std::string method, error;
	Json::Value params;
	if (!ShellyRpcTranslateCommand(command, method, params, error))
		return ShellyResult::Failure(0, std::move(error)); // local rejection, never a reachability failure
	ShellyResult result = RpcCall(method, params);
	if (result.ok)
	{
		OperationGuard operation(*this);
		if (operation.Active())
			RecordSuccess();
	}
	return result;
}

bool ShellyGen2Connection::HandleNotifyStatus(const Json::Value& params)
{
	// Not a sign of life: there is nothing to merge.
	if (!params.isObject())
		return false;
	OperationGuard operation(*this);
	if (!operation.Active())
		return false;
	CallbackScope callback(*this);
	// While the baseline is known to be stale (resync pending) a partial update must not extend it.
	if (IsStopped() || !m_hasStatus || m_resyncNeeded)
		return false;
	ShellyRpcMergeStatus(m_status, params);
	RecordSuccess();
	// m_status is only touched under the callback lock, so the sink can read it without a copy.
	DeliverStatus(++m_nextSeq, m_status);
	return true;
}

bool ShellyGen2Connection::HandleNotifyEvent(const Json::Value& params)
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return false;
	CallbackScope callback(*this);
	if (IsStopped())
		return false;
	if (params.isObject() && params["events"].isArray())
	{
		for (const Json::Value& event : params["events"])
		{
			if (IsStopped())
				return false;
			m_sink.OnShellyEvent(m_node, event);
		}
	}
	return true;
}

std::shared_ptr<IShellyConnection> CreateShellyGen2Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink)
{
	return std::make_shared<ShellyGen2PushConnection>(std::move(node), std::move(credentials), sink);
}
