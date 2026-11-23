#include "stdafx.h"
#include "ShellyGen1.h"
#include "ShellyGen1Coiot.h"
#include "ShellyMapping.h"
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include "../main/Logger.h"

namespace
{
	// Reads a numeric param that must lie in [minValue, maxValue] (rejected, not clamped), rounded to an int.
	bool GetIntParam(const Json::Value& params, const char* key, const int minValue, const int maxValue, int& out, std::string& error)
	{
		if (!params.isObject() || !params.isMember(key) || !params[key].isNumeric())
		{
			error = std::string("invalid parameter: ") + key;
			return false;
		}
		const double v = params[key].asDouble();
		if (!std::isfinite(v) || v < minValue || v > maxValue)
		{
			error = std::string("invalid parameter: ") + key;
			return false;
		}
		out = static_cast<int>(v + (v < 0 ? -0.5 : 0.5));
		return true;
	}

	bool IsBulbModel(const std::string& model)
	{
		return ShellyIsGen1BulbModel(model);
	}

	std::string TurnValue(const std::string& action)
	{
		return (action == ShellyCmd::ACTION_ON || action == ShellyCmd::ACTION_OFF || action == ShellyCmd::ACTION_TOGGLE) ? action : std::string();
	}

	std::string FormatTemperature(const double value)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		ss.setf(std::ios::fixed);
		ss.precision(1);
		ss << value;
		return ss.str();
	}
} // namespace

bool ShellyGen1BuildCommandPath(const ShellyCommand& command, const std::string& model, std::string& path, std::string& error)
{
	path.clear();
	error.clear();
	if (!ShellyIsValidChannel(command.channel))
	{
		error = "invalid parameter: channel";
		return false;
	}
	const std::string ch = std::to_string(command.channel);
	const std::string& comp = command.component;
	const std::string& act = command.action;
	const Json::Value& params = command.params;

	if (comp == ShellyCmd::COMPONENT_SWITCH)
	{
		const std::string turn = TurnValue(act);
		if (turn.empty())
		{
			error = "unsupported command";
			return false;
		}
		path = "/relay/" + ch + "?turn=" + turn;
		return true;
	}

	if (comp == ShellyCmd::COMPONENT_LIGHT || comp == ShellyCmd::COMPONENT_RGB || comp == ShellyCmd::COMPONENT_RGBW)
	{
		const bool isLight = (comp == ShellyCmd::COMPONENT_LIGHT);
		const bool bulb = IsBulbModel(model);
		std::string base;
		if (isLight)
			base = (model == "SHRGBW2") ? "/white/" : "/light/";
		else
			base = bulb ? "/light/" : "/color/";
		base += ch;

		const std::string turn = TurnValue(act);
		if (!turn.empty())
		{
			path = base + "?turn=" + turn;
			if (isLight && act == ShellyCmd::ACTION_ON && params.isObject() && params.isMember(ShellyCmd::PARAM_BRIGHTNESS))
			{
				int brightness = 0;
				if (!GetIntParam(params, ShellyCmd::PARAM_BRIGHTNESS, 0, 100, brightness, error))
					return false;
				if (brightness == 0)
					path = base + "?turn=off";
				else
					path += "&brightness=" + std::to_string(brightness);
			}
			return true;
		}
		if (isLight && act == ShellyCmd::ACTION_SET_BRIGHTNESS)
		{
			int brightness = 0;
			if (!GetIntParam(params, ShellyCmd::PARAM_BRIGHTNESS, 0, 100, brightness, error))
				return false;
			path = base + (brightness == 0 ? "?turn=off" : "?turn=on&brightness=" + std::to_string(brightness));
			return true;
		}
		if (!isLight && act == ShellyCmd::ACTION_SET_COLOR && bulb && params.isObject() && params["mode"].isString() && params["mode"].asString() == "white")
		{
			// Bulb white mode: mode=white with the white brightness and, when given, the color temperature in Kelvin (3000..6500).
			path = base + "?turn=on&mode=white";
			if (params.isMember(ShellyCmd::PARAM_BRIGHTNESS))
			{
				int brightness = 0;
				if (!GetIntParam(params, ShellyCmd::PARAM_BRIGHTNESS, 0, 100, brightness, error))
					return false;
				path += "&brightness=" + std::to_string(brightness);
			}
			if (params.isMember("temp"))
			{
				int kelvin = 0;
				if (!GetIntParam(params, "temp", 3000, 6500, kelvin, error))
					return false;
				path += "&temp=" + std::to_string(kelvin);
			}
			return true;
		}
		if (!isLight && act == ShellyCmd::ACTION_SET_COLOR)
		{
			int r = 0, g = 0, b = 0;
			if (!GetIntParam(params, ShellyCmd::PARAM_RED, 0, 255, r, error) || !GetIntParam(params, ShellyCmd::PARAM_GREEN, 0, 255, g, error)
			    || !GetIntParam(params, ShellyCmd::PARAM_BLUE, 0, 255, b, error))
				return false;
			path = base + "?turn=on";
			if (bulb)
				path += "&mode=color";
			path += "&red=" + std::to_string(r) + "&green=" + std::to_string(g) + "&blue=" + std::to_string(b);
			if (comp == ShellyCmd::COMPONENT_RGBW && params.isMember(ShellyCmd::PARAM_WHITE))
			{
				int w = 0;
				if (!GetIntParam(params, ShellyCmd::PARAM_WHITE, 0, 255, w, error))
					return false;
				path += "&white=" + std::to_string(w);
			}
			if (params.isMember(ShellyCmd::PARAM_BRIGHTNESS))
			{
				int gain = 0;
				if (!GetIntParam(params, ShellyCmd::PARAM_BRIGHTNESS, 0, 100, gain, error))
					return false;
				path += "&gain=" + std::to_string(gain);
			}
			return true;
		}
		error = "unsupported command";
		return false;
	}

	if (comp == ShellyCmd::COMPONENT_ROLLER || comp == ShellyCmd::COMPONENT_COVER)
	{
		if (act == ShellyCmd::ACTION_OPEN || act == ShellyCmd::ACTION_CLOSE || act == ShellyCmd::ACTION_STOP)
		{
			path = "/roller/" + ch + "?go=" + act;
			return true;
		}
		if (act == ShellyCmd::ACTION_SET_POSITION)
		{
			int pos = 0;
			if (!GetIntParam(params, ShellyCmd::PARAM_POSITION, 0, 100, pos, error))
				return false;
			path = "/roller/" + ch + "?go=to_pos&roller_pos=" + std::to_string(pos);
			return true;
		}
		error = "unsupported command";
		return false;
	}

	if (comp == ShellyCmd::COMPONENT_THERMOSTAT && act == ShellyCmd::ACTION_SET_TEMPERATURE)
	{
		// Shelly TRV: the target is a setting, /settings/thermostats/N (the /thermostats/N endpoint only reports
		// status and takes boost_minutes). target_t is documented as 4..31 degrees Celsius (Gen2 uses 5..35, also
		// unverified on devices; the ranges are deliberately not shared).
		if (!params.isObject() || !params.isMember(ShellyCmd::PARAM_TEMPERATURE) || !params[ShellyCmd::PARAM_TEMPERATURE].isNumeric())
		{
			error = std::string("invalid parameter: ") + ShellyCmd::PARAM_TEMPERATURE;
			return false;
		}
		const double t = params[ShellyCmd::PARAM_TEMPERATURE].asDouble();
		if (!std::isfinite(t) || t < 4.0 || t > 31.0)
		{
			error = std::string("invalid parameter: ") + ShellyCmd::PARAM_TEMPERATURE;
			return false;
		}
		path = "/settings/thermostats/" + ch + "?target_t_enabled=true&target_t=" + FormatTemperature(t);
		return true;
	}

	error = "unsupported command";
	return false;
}

ShellyGen1SleepInfo ShellyGen1ParseSleepInfo(const Json::Value& settings)
{
	ShellyGen1SleepInfo info;
	if (!settings.isObject() || !settings.isMember("sleep_mode"))
		return info;
	const Json::Value& sm = settings["sleep_mode"];
	if (!sm.isObject())
		return info;
	info.sleepy = true;
	if (sm["period"].isNumeric())
	{
		// Read as double: asInt() throws for a value outside the int range. 100000 hours * 3600 fits an int.
		const double period = sm["period"].asDouble();
		const std::string unit = sm["unit"].isString() ? sm["unit"].asString() : "h";
		if (std::isfinite(period) && period > 0 && period <= 100000)
			info.periodSec = std::max(1, static_cast<int>(period * (unit == "m" ? 60 : 3600) + 0.5));
	}
	return info;
}

ShellyHttpOptions ShellyGen1HttpOptions(const ShellyNode& node, const ShellyCredentials& credentials, const int timeoutSec)
{
	ShellyHttpOptions options;
	options.timeoutSec = timeoutSec;
	if (node.auth || !credentials.password.empty())
	{
		options.auth = ShellyHttpAuth::Basic;
		options.username = credentials.username.empty() ? "admin" : credentials.username;
		options.password = credentials.password;
	}
	return options;
}

bool ShellyGen1IsReachabilityFailure(const ShellyResult& result)
{
	return result.IsReachabilityFailure();
}

ShellyGen1Connection::ShellyGen1Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink)
	: m_node(std::move(node))
	, m_credentials(std::move(credentials))
	, m_sink(sink)
{
}

ShellyGen1Connection::~ShellyGen1Connection()
{
	const std::thread::id self = std::this_thread::get_id();
	if (m_callbackThread.load() == self || m_pushThread.load() == self)
		_log.Debug(DEBUG_HARDWARE, "Shelly %s: destroyed from a callback or push thread, push teardown deferred (the owner must Stop() from another thread first)", m_node.ip.c_str());
	ShellyGen1Connection::Stop();
}

bool ShellyGen1Connection::BeginOperation()
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	if (m_stopped)
		return false;
	++m_inFlight;
	return true;
}

void ShellyGen1Connection::EndOperation()
{
	std::lock_guard<std::mutex> lock(m_stateMutex);
	if (--m_inFlight == 0)
		m_idle.notify_all();
}

bool ShellyGen1Connection::Start()
{
	// Serialized with Stop(): a Start() that wins the race is fully torn down by the Stop() that waits for it.
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	if (m_stopped)
		return false;
	m_pushStarted = true;
	return OnStartPush();
}

void ShellyGen1Connection::Stop()
{
	// From within a sink callback of this connection (this thread holds the callback lock) or from the registered
	// push thread: only mark stopped. Joining the push thread here could deadlock (it may be waiting for the
	// callback lock we hold, or be this thread), and the drain below would wait for our own operation. No
	// lifecycle lock wait either. The next Stop() from another thread (or the destructor) does the teardown.
	const std::thread::id self = std::this_thread::get_id();
	const bool deferTeardown = (m_callbackThread.load() == self) || (m_pushThread.load() == self);
	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		m_stopped = true;
	}
	m_pushActive = false;
	if (deferTeardown)
		return;
	// Every concurrent Stop() waits here until the first one finished the teardown.
	std::lock_guard<std::mutex> lifecycle(m_lifecycleMutex);
	// OnStopPush() only pairs with an OnStartPush() that ran, and runs once.
	if (m_pushStarted)
	{
		m_pushStarted = false;
		OnStopPush();
	}
	// In-flight Poll/FetchInfo/SendCommand/ReportPushStatus calls (and the callbacks they run) finish first.
	std::unique_lock<std::mutex> lock(m_stateMutex);
	m_idle.wait(lock, [this] { return m_inFlight == 0; });
}

ShellyResult ShellyGen1Connection::Get(const std::string& path)
{
	if (m_stopped)
		return ShellyResult::Failure(0, "stopped");
	return ShellyHttpGet(ShellyBaseUrl(m_node) + path, ShellyGen1HttpOptions(m_node, m_credentials)).ToResult();
}

void ShellyGen1Connection::DeliverStatus(const uint64_t seq, const Json::Value& status)
{
	if (m_stopped || seq < m_deliveredSeq)
		return;
	m_deliveredSeq = seq;
	m_sink.OnShellyStatus(m_node, status);
}

void ShellyGen1Connection::NoteReachability(const bool reachable, const bool countFailure)
{
	CallbackScope cb(*this);
	if (m_stopped)
		return;
	bool report = false;
	bool online = false;
	{
		std::lock_guard<std::mutex> lock(m_stateMutex);
		if (reachable)
		{
			m_failures = 0;
			if (m_availability != Availability::Online)
			{
				m_availability = Availability::Online;
				report = online = true;
			}
		}
		else if (countFailure)
		{
			if (m_failures < SHELLY_GEN1_OFFLINE_AFTER)
				m_failures++;
			if (m_failures >= SHELLY_GEN1_OFFLINE_AFTER && m_availability != Availability::Offline && !m_node.sleepy)
			{
				m_availability = Availability::Offline;
				report = true;
			}
		}
	}
	if (report)
		m_sink.OnShellyAvailability(m_node, online);
}

ShellyResult ShellyGen1Connection::Poll()
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");
	// The sequence is taken before the request: a push report delivered while this request is in flight is newer.
	const uint64_t seq = ++m_nextSeq;
	ShellyResult result = Get("/status");
	if (result.ok && !result.data.isObject())
	{
		result.ok = false;
		result.error = "invalid response";
	}
	if (result.ok)
	{
		// Availability first, so the sink sees the node online before its status.
		CallbackScope cb(*this);
		NoteReachability(true, false);
		DeliverStatus(seq, result.data);
	}
	else
		NoteReachability(false, ShellyGen1IsReachabilityFailure(result));
	return result;
}

ShellyResult ShellyGen1Connection::FetchInfo()
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");
	ShellyResult result = Get("/settings");
	if (result.ok && !result.data.isObject())
	{
		result.ok = false;
		result.error = "invalid response";
	}
	if (!result.ok)
	{
		NoteReachability(false, ShellyGen1IsReachabilityFailure(result));
		return result;
	}
	ShellyResult shelly = Get("/shelly");
	if (shelly.ok && shelly.data.isObject())
		result.data["shelly"] = std::move(shelly.data);
	const ShellyGen1SleepInfo sleep = ShellyGen1ParseSleepInfo(result.data);
	result.data["dz_sleep"]["sleepy"] = sleep.sleepy;
	result.data["dz_sleep"]["period_sec"] = sleep.periodSec;

	CallbackScope cb(*this);
	NoteReachability(true, false);
	if (!m_stopped)
		m_sink.OnShellyNodeInfo(m_node, result.data);
	return result;
}

ShellyResult ShellyGen1Connection::SendCommand(const ShellyCommand& command)
{
	OperationGuard operation(*this);
	if (!operation.Active())
		return ShellyResult::Failure(0, "stopped");
	std::string path, error;
	if (!ShellyGen1BuildCommandPath(command, m_node.model, path, error))
		return ShellyResult::Failure(0, std::move(error)); // local rejection, never a reachability failure
	const ShellyResult result = Get(path);
	if (result.ok)
		NoteReachability(true, false);
	return result;
}

uint64_t ShellyGen1Connection::BeginPushReport()
{
	return ++m_nextSeq;
}

void ShellyGen1Connection::ReportPushStatus(const uint64_t seq, const Json::Value& status)
{
	if (!status.isObject())
		return;
	OperationGuard operation(*this);
	if (!operation.Active())
		return;
	CallbackScope cb(*this);
	NoteReachability(true, false);
	DeliverStatus(seq, status);
}

std::shared_ptr<IShellyConnection> CreateShellyGen1Connection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink, const ShellyCoiotOptions& coiot)
{
	if (coiot.enabled)
		return CreateShellyGen1CoiotConnection(std::move(node), std::move(credentials), sink, coiot);
	return std::make_shared<ShellyGen1Connection>(std::move(node), std::move(credentials), sink);
}
