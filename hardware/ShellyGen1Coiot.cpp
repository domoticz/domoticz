#include "stdafx.h"
#include "ShellyGen1Coiot.h"
#include <algorithm>
#include <chrono>
#include <boost/asio.hpp>
#include <set>
#include <vector>
#include "ShellyIdentify.h"
#include "../main/Logger.h"

#ifdef WIN32
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif
#endif

namespace
{
	std::mutex s_listenersMutex;
	std::condition_variable s_listenersCv; // signalled when a listener finished closing its socket
	std::map<int, std::weak_ptr<ShellyCoiotListener>> s_listeners;
	std::set<int> s_openFailureLogged; // ports whose bind failure was logged, guarded by s_listenersMutex (Open runs under it)

	constexpr int CONFIGURE_RETRY_SEC = 60;
	constexpr int MIN_FETCH_INTERVAL_MS = 1000;
	constexpr int MAX_RECHECK_SEC = 3600;
	constexpr int RECEIVE_ERROR_STREAK = 20; // consecutive failed receives before the socket is reopened
	constexpr int MAX_REOPEN_BACKOFF_SEC = 60;
	constexpr int64_t DROP_LOG_INTERVAL_MS = 60000;

	int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() + 1;
	}
} // namespace

bool ShellyCoiotFrameBelongsToNode(const std::string& sourceIp, const std::string& frameMac, const std::string& nodeHost, const std::string& nodeMac)
{
	if (!frameMac.empty() && !ShellyCoiotMacMatches(frameMac, nodeMac))
		return false;
	boost::system::error_code ec;
	const boost::asio::ip::address nodeAddress = boost::asio::ip::make_address(nodeHost, ec);
	if (ec)
		return !frameMac.empty(); // a host name cannot be compared with the source, the MAC has to decide
	const boost::asio::ip::address source = boost::asio::ip::make_address(sourceIp, ec);
	return !ec && source == nodeAddress;
}

// ---------------------------------------------------------------------------------------------------------
// ShellyCoiotListener

struct ShellyCoiotListener::Net
{
	Net()
		: socket(io)
		, retryTimer(io)
	{
	}

	boost::asio::io_context io;
	boost::asio::ip::udp::socket socket;
	boost::asio::ip::udp::endpoint sender;
	uint8_t buffer[SHELLY_COAP_MAX_DATAGRAM + 1];
	boost::asio::steady_timer retryTimer;
};

ShellyCoiotListener::ShellyCoiotListener(const int port)
	: m_port(port)
	, m_net(std::make_unique<Net>())
{
}

ShellyCoiotListener::~ShellyCoiotListener()
{
	Close();
}

std::shared_ptr<ShellyCoiotListener> ShellyCoiotListener::Acquire(const int port)
{
	if (port < 1 || port > 65535)
		return nullptr;
	std::unique_lock<std::mutex> lock(s_listenersMutex);
	// An expired entry belongs to a listener whose last reference is gone but whose socket may still be open: wait
	// until its deleter removed the entry, otherwise the bind below would fail with "address in use".
	while (true)
	{
		const auto it = s_listeners.find(port);
		if (it == s_listeners.end())
			break;
		if (std::shared_ptr<ShellyCoiotListener> existing = it->second.lock())
			return existing;
		s_listenersCv.wait(lock);
	}
	ShellyCoiotListener* created = new ShellyCoiotListener(port);
	if (!created->Open())
	{
		delete created;
		return nullptr;
	}
	std::shared_ptr<ShellyCoiotListener> listener(created,
						      [](ShellyCoiotListener* p)
						      {
							      const int listenPort = p->Port();
							      delete p; // closes the socket and joins the receiver thread, without the registry lock
							      std::lock_guard<std::mutex> l(s_listenersMutex);
							      s_listeners.erase(listenPort);
							      s_listenersCv.notify_all();
						      });
	s_listeners[port] = listener;
	return listener;
}

bool ShellyCoiotListener::BindSocket(std::string& error)
{
	boost::system::error_code ec;
	boost::asio::ip::udp::socket& socket = m_net->socket;
	const boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), static_cast<unsigned short>(m_port));
	socket.open(endpoint.protocol(), ec);
	if (ec)
	{
		error = ec.message();
		return false;
	}
#ifdef WIN32
	// Without this an ICMP "port unreachable" answer to one of our (ACK) datagrams makes the next receive fail with
	// WSAECONNRESET on Windows.
	DWORD bytesReturned = 0;
	BOOL newBehavior = FALSE;
	if (WSAIoctl(socket.native_handle(), SIO_UDP_CONNRESET, &newBehavior, sizeof(newBehavior), nullptr, 0, &bytesReturned, nullptr, nullptr) != 0)
		_log.Debug(DEBUG_HARDWARE, "Shelly CoIoT: could not disable UDP connection reset reports (error %d)", WSAGetLastError());
#endif
	socket.bind(endpoint, ec);
	if (ec)
	{
		error = ec.message();
		boost::system::error_code ignored;
		socket.close(ignored);
		return false;
	}
	return true;
}

bool ShellyCoiotListener::Open()
{
	boost::system::error_code ec;
	std::string bindError;
	if (!BindSocket(bindError))
	{
		// Every Gen1 node asks for the listener, so the failure is logged once per port (Acquire holds s_listenersMutex).
		if (s_openFailureLogged.insert(m_port).second)
			_log.Log(LOG_ERROR, "Shelly CoIoT: cannot listen on UDP port %d: %s", m_port, bindError.c_str());
		return false;
	}
	s_openFailureLogged.erase(m_port);
	StartReceive();
	try
	{
		m_thread = std::thread(
			[this]
			{
				try
				{
					m_net->io.run();
				}
				catch (const std::exception& e)
				{
					_log.Log(LOG_ERROR, "Shelly CoIoT: the receiver on UDP port %d stopped: %s", m_port, e.what());
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "Shelly CoIoT: the receiver on UDP port %d stopped on an unknown error", m_port);
				}
			});
	}
	catch (const std::exception& e)
	{
		_log.Log(LOG_ERROR, "Shelly CoIoT: cannot start the receiver thread: %s", e.what());
		m_net->socket.close(ec);
		return false;
	}
	_log.Log(LOG_STATUS, "Shelly CoIoT: listening on UDP port %d", m_port);
	return true;
}

void ShellyCoiotListener::Close()
{
	if (m_thread.joinable())
	{
		boost::asio::post(m_net->io,
				  [this]
				  {
					  boost::system::error_code ec;
					  m_net->retryTimer.cancel();
					  m_net->socket.close(ec);
				  });
		m_thread.join();
	}
	boost::system::error_code ec;
	m_net->retryTimer.cancel();
	m_net->socket.close(ec);
}

void ShellyCoiotListener::StartReceive()
{
	m_net->socket.async_receive_from(boost::asio::buffer(m_net->buffer, sizeof(m_net->buffer)), m_net->sender,
				    [this](const boost::system::error_code& ec, const size_t size)
				    {
					    if (ec == boost::asio::error::operation_aborted)
						    return;
					    if (!ec)
					    {
						    m_errors = 0;
						    m_backoffSec = 1;
						    try
						    {
							    HandleDatagram(size);
						    }
						    catch (const std::exception& e)
						    {
							    _log.Log(LOG_ERROR, "Shelly CoIoT: error while handling a datagram: %s", e.what());
						    }
						    catch (...)
						    {
							    _log.Log(LOG_ERROR, "Shelly CoIoT: unknown error while handling a datagram");
						    }
					    }
					    else if (ec == boost::asio::error::message_size)
					    {
						    // A datagram larger than the buffer (sent by anything on the network) is a dropped datagram, not a socket failure.
						    ++m_dropped;
					    }
					    else if (++m_errors >= RECEIVE_ERROR_STREAK)
					    {
						    _log.Log(LOG_ERROR, "Shelly CoIoT: receiving on UDP port %d keeps failing (%s), reopening the socket", m_port, ec.message().c_str());
						    boost::system::error_code closeEc;
						    m_net->socket.close(closeEc);
						    ScheduleReopen();
						    return;
					    }
					    if (m_net->socket.is_open())
						    StartReceive();
				    });
}

void ShellyCoiotListener::ScheduleReopen()
{
	const int delay = m_backoffSec;
	m_backoffSec = std::min(m_backoffSec * 2, MAX_REOPEN_BACKOFF_SEC);
	m_net->retryTimer.expires_after(std::chrono::seconds(delay));
	m_net->retryTimer.async_wait(
		[this](const boost::system::error_code& waitEc)
		{
			if (!waitEc)
				Reopen();
		});
}

void ShellyCoiotListener::Reopen()
{
	std::string bindError;
	if (!BindSocket(bindError))
	{
		// Logged when the state changes (the first failed attempt), not on every retry.
		if (!m_reopenFailureLogged)
			_log.Log(LOG_ERROR, "Shelly CoIoT: cannot reopen UDP port %d: %s", m_port, bindError.c_str());
		m_reopenFailureLogged = true;
		ScheduleReopen();
		return;
	}
	m_reopenFailureLogged = false;
	m_errors = 0;
	_log.Log(LOG_STATUS, "Shelly CoIoT: listening on UDP port %d again", m_port);
	StartReceive();
}

void ShellyCoiotListener::NoteDropped(const ShellyCoapMessage& message, const char* reason)
{
	++m_dropped;
	const int64_t now = NowMs();
	if (m_lastDropLogMs != 0 && now - m_lastDropLogMs < DROP_LOG_INTERVAL_MS)
		return;
	m_lastDropLogMs = now;
	std::string path = message.path.substr(0, 64);
	_log.Debug(DEBUG_HARDWARE, "Shelly CoIoT: dropped %u datagram(s), last from %s: %s (code %d.%02d, path '%s')", m_dropped, m_net->sender.address().to_string().c_str(), reason,
		   message.code >> 5, message.code & 0x1F, path.c_str());
	m_dropped = 0;
}

bool ShellyCoiotListener::HasMatchingRegistration(const std::string& sourceIp, const std::string& frameMac)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& entry : m_registrations)
	{
		if (ShellyCoiotFrameBelongsToNode(sourceIp, frameMac, entry.second.host, entry.second.mac))
			return true;
	}
	return false;
}

void ShellyCoiotListener::HandleDatagram(const size_t size)
{
	ShellyCoapMessage message;
	if (!ShellyCoapParse(m_net->buffer, size, message))
		return;
	boost::system::error_code ec;
	const bool confirmable = (message.type == ShellyCoapType::Confirmable);

	if (!message.IsStatusFrame())
	{
		if (confirmable)
		{
			const std::vector<uint8_t> reset = ShellyCoapBuildReset(message.messageId);
			m_net->socket.send_to(boost::asio::buffer(reset), m_net->sender, 0, ec);
		}
		NoteDropped(message, "not a status frame (path or code mismatch)");
		return;
	}

	// Cheap source check before anything is answered or parsed: frames of unknown devices cost nothing more.
	const std::string sourceIp = m_net->sender.address().to_string();
	const std::string deviceMac = ShellyCoiotMacFromDeviceId(message.deviceId);
	if (!HasMatchingRegistration(sourceIp, deviceMac))
	{
		NoteDropped(message, "no node matches the sender");
		return;
	}
	if (confirmable)
	{
		const std::vector<uint8_t> ack = ShellyCoapBuildAck(message.messageId);
		m_net->socket.send_to(boost::asio::buffer(ack), m_net->sender, 0, ec);
	}

	std::vector<ShellyCoiotSensor> sensors;
	if (!ShellyCoiotParseStatus(message.payload, sensors))
		return;

	ShellyCoiotFrame frame;
	frame.sourceIp = sourceIp;
	frame.deviceMac = deviceMac;
	frame.sensorCount = sensors.size();

	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& entry : m_registrations)
	{
		const Registration& reg = entry.second;
		if (ShellyCoiotFrameBelongsToNode(frame.sourceIp, frame.deviceMac, reg.host, reg.mac))
			reg.handler(frame);
	}
}

int ShellyCoiotListener::Register(const std::string& nodeHost, const std::string& nodeMac, Handler handler)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	const int token = m_nextToken++;
	m_registrations[token] = Registration{ nodeHost, nodeMac, std::move(handler) };
	return token;
}

void ShellyCoiotListener::Unregister(const int token)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_registrations.erase(token);
}

// ---------------------------------------------------------------------------------------------------------
// ShellyGen1CoiotConnection

ShellyGen1CoiotConnection::ShellyGen1CoiotConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink, ShellyCoiotOptions options)
	: ShellyGen1Connection(node, credentials, sink)
	, m_creds(std::move(credentials))
	, m_options(std::move(options))
	, m_periodSec(ShellyClampCoiotPeriod(m_options.updatePeriodSec))
{
}

ShellyGen1CoiotConnection::~ShellyGen1CoiotConnection()
{
	// The worker holds a reference to this object while it runs, so it is not running here: the thread object is joinable but
	// finished (or it is the calling thread, which is about to leave the worker, see the file comment).
	ShellyGen1CoiotConnection::Stop();
	if (m_listener)
		m_listener->Unregister(m_token); // only when the teardown was deferred, see ShellyGen1.h
	{
		std::lock_guard<std::mutex> lock(m_state.mutex);
		m_state.quit = true;
	}
	m_state.cv.notify_all();
	JoinOrDetachWorker();
}

void ShellyGen1CoiotConnection::JoinOrDetachWorker()
{
	if (!m_thread.joinable())
		return;
	if (m_thread.get_id() == std::this_thread::get_id())
		m_thread.detach(); // the last reference was released by the worker itself, it has nothing left to do
	else
		m_thread.join();
}

bool ShellyGen1CoiotConnection::HasPush() const
{
	if (!ShellyGen1Connection::HasPush())
		return false;
	const int64_t last = m_lastFrameMs.load();
	return last != 0 && (NowMs() - last) <= static_cast<int64_t>(m_periodSec) * 3000;
}

bool ShellyGen1CoiotConnection::OnStartPush()
{
	m_listener = ShellyCoiotListener::Acquire(m_options.listenPort);
	if (!m_listener)
	{
		// The reason (port in use, no permission) was logged once per port by the listener.
		_log.Debug(DEBUG_HARDWARE, "Shelly %s: CoIoT push unavailable, using REST polling", GetNode().ip.c_str());
		return true; // not fatal: the node keeps working by polling
	}
	m_token = m_listener->Register(GetNode().ip, GetNode().id, [this](const ShellyCoiotFrame& frame) { OnFrame(frame); });
	try
	{
		m_thread = std::thread(
			[this, self = shared_from_this()]
			{
				try
				{
					WorkerLoop();
				}
				catch (const std::exception& e)
				{
					_log.Log(LOG_ERROR, "Shelly CoIoT worker stopped: %s", e.what());
				}
				catch (...)
				{
					_log.Log(LOG_ERROR, "Shelly CoIoT worker stopped on an unknown error");
				}
			});
	}
	catch (const std::exception& e)
	{
		_log.Log(LOG_ERROR, "Shelly %s: cannot start the CoIoT worker: %s", GetNode().ip.c_str(), e.what());
		m_listener->Unregister(m_token);
		m_listener.reset();
	}
	return true;
}

void ShellyGen1CoiotConnection::OnStopPush()
{
	if (m_listener)
		m_listener->Unregister(m_token); // no handler call is in flight after this returns
	{
		std::lock_guard<std::mutex> lock(m_state.mutex);
		m_state.quit = true;
	}
	m_state.cv.notify_all();
	JoinOrDetachWorker();
	m_listener.reset();
}

void ShellyGen1CoiotConnection::OnFrame(const ShellyCoiotFrame& /*frame*/)
{
	m_lastFrameMs = NowMs();
	{
		std::lock_guard<std::mutex> lock(m_state.mutex);
		m_state.trigger = true;
	}
	m_state.cv.notify_one();
}

bool ShellyGen1CoiotConnection::WaitFor(WorkState& state, const std::chrono::milliseconds duration)
{
	std::unique_lock<std::mutex> lock(state.mutex);
	state.cv.wait_for(lock, duration, [&state] { return state.quit; });
	return state.quit;
}

void ShellyGen1CoiotConnection::WorkerLoop()
{
	WorkState& state = m_state;
	SetPushThread(std::this_thread::get_id());
	// Cleared on every exit, an exception included.
	struct PushThreadGuard
	{
		ShellyGen1CoiotConnection& self;
		~PushThreadGuard()
		{
			self.SetPushThread(std::thread::id());
		}
	} pushThreadGuard{ *this };
	using Clock = std::chrono::steady_clock;
	const bool canConfigure = !m_options.peerHost.empty();
	bool configured = !canConfigure;
	const auto initialRecheck = std::chrono::seconds(std::max(3 * m_periodSec, 120));
	auto recheckInterval = initialRecheck;
	Clock::time_point nextConfigure = Clock::now();
	Clock::time_point lastPushSeen = Clock::now();
	Clock::time_point lastFetch = Clock::time_point::min();

	while (!IsStopped())
	{
		if (canConfigure)
		{
			// Push that does not work although the device was configured: look at its settings again, with a growing
			// interval so an unreachable or unsupported device is not hammered.
			if (HasPush())
			{
				lastPushSeen = Clock::now();
				recheckInterval = initialRecheck;
			}
			else if (configured && Clock::now() - lastPushSeen >= recheckInterval)
			{
				_log.Debug(DEBUG_HARDWARE, "Shelly %s: no CoIoT push for %d s, checking the device settings again", GetNode().ip.c_str(), static_cast<int>(recheckInterval.count()));
				configured = false;
				nextConfigure = Clock::now();
				lastPushSeen = Clock::now();
				recheckInterval = std::min(recheckInterval * 2, std::chrono::seconds(MAX_RECHECK_SEC));
			}
		}
		if (!configured && Clock::now() >= nextConfigure)
		{
			// Blocks this worker for up to two HTTP timeouts (read, then write the settings), so a quit request is
			// only noticed afterwards.
			configured = ConfigurePeer();
			nextConfigure = Clock::now() + std::chrono::seconds(CONFIGURE_RETRY_SEC);
		}
		{
			std::unique_lock<std::mutex> lock(state.mutex);
			state.cv.wait_for(lock, std::chrono::seconds(5), [&state] { return state.quit || state.trigger; });
			if (state.quit)
				break;
			if (!state.trigger)
				continue;
		}
		// At most one fetch per second; frames arriving meanwhile are coalesced into it.
		if (lastFetch != Clock::time_point::min())
		{
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - lastFetch);
			if (elapsed.count() < MIN_FETCH_INTERVAL_MS && WaitFor(state, std::chrono::milliseconds(MIN_FETCH_INTERVAL_MS) - elapsed))
				break;
		}
		{
			std::lock_guard<std::mutex> lock(state.mutex);
			state.trigger = false;
		}
		lastFetch = Clock::now();
		const ShellyResult result = Poll();
		SetPushActive(result.ok);
	}
}

bool ShellyGen1CoiotConnection::ConfigurePeer()
{
	const std::string& host = m_options.peerHost;
	if (!ShellyIsValidHost(host) || host.find(':') != std::string::npos)
	{
		_log.Log(LOG_ERROR, "Shelly %s: the CoIoT peer address '%s' is not valid (an IPv4 address or host name is required), the device is not configured", GetNode().ip.c_str(),
			 host.c_str());
		return true; // retrying cannot help
	}
	const int port = m_options.peerPort > 0 ? m_options.peerPort : m_options.listenPort;
	if (!ShellyIsValidPort(port))
	{
		_log.Log(LOG_ERROR, "Shelly %s: the CoIoT peer port %d is not valid, the device is not configured", GetNode().ip.c_str(), port);
		return true; // retrying cannot help
	}
	const std::string target = host + ":" + std::to_string(port);
	const ShellyHttpOptions http = ShellyGen1HttpOptions(GetNode(), m_creds);
	const std::string base = ShellyBaseUrl(GetNode());

	if (IsStopped())
		return false;
	const ShellyResult current = ShellyHttpGet(base + "/settings", http).ToResult();
	if (!current.ok)
	{
		_log.Debug(DEBUG_HARDWARE, "Shelly %s: CoIoT peer not configured yet: %s", GetNode().ip.c_str(), current.error.c_str());
		return false;
	}
	if (current.data.isObject() && current.data["coiot"].isObject())
	{
		const Json::Value& c = current.data["coiot"];
		if (c["enabled"].isBool() && c["enabled"].asBool() && c["peer"].isString() && c["peer"].asString() == target && c["update_period"].isNumeric()
		    && static_cast<int>(c["update_period"].asDouble()) == m_periodSec)
			return true; // already as wanted
	}
	if (IsStopped())
		return false;
	const std::string url = base + "/settings?coiot_enable=true&coiot_update_period=" + std::to_string(m_periodSec) + "&coiot_peer=" + target;
	const ShellyResult written = ShellyHttpGet(url, http).ToResult();
	if (!written.ok)
	{
		_log.Log(LOG_ERROR, "Shelly %s: could not set the CoIoT peer: %s", GetNode().ip.c_str(), written.error.c_str());
		return false;
	}
	_log.Log(LOG_STATUS, "Shelly %s: CoIoT peer set to %s (the device may need a restart before it sends)", GetNode().ip.c_str(), target.c_str());
	return true;
}

std::shared_ptr<IShellyConnection> CreateShellyGen1CoiotConnection(ShellyNode node, ShellyCredentials credentials, IShellyEventSink& sink,
								   const ShellyCoiotOptions& options)
{
	return std::make_shared<ShellyGen1CoiotConnection>(std::move(node), std::move(credentials), sink, options);
}
