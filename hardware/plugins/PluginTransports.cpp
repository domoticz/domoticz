#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginTransports.h"
#include "PythonObjects.h"

#include "../../main/Logger.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace Plugins {

	extern PyTypeObject* CConnectionType;

	void CPluginTransport::configureTimeout()
	{
		if (m_pConnection->Timeout)
		{
			// Set up timeout if one was requested
			if (!m_Timer)
			{
				m_Timer = new boost::asio::deadline_timer(ios);
			}
			m_Timer->expires_from_now(boost::posix_time::milliseconds(m_pConnection->Timeout));
			m_Timer->async_wait([this](const boost::system::error_code &ec) { handleTimeout(ec); });
		}
		else
		{
			// Cancel timer if timeout has been set to zero
			if (m_Timer)
			{
				m_Timer->cancel();
				delete m_Timer;
				m_Timer = NULL;
			}
		}
	}

	void CPluginTransport::handleTimeout(const boost::system::error_code &ec)
	{
		CPlugin *pPlugin = m_pConnection->pPlugin;
		if (!pPlugin)
			return;

		if (!ec) // Timeout, no response
		{
			if (pPlugin->m_bDebug & PDM_CONNECTION)
			{
				pPlugin->Log(LOG_NORM, "Timeout for port '%s'", m_Port.c_str());
			}
			pPlugin->MessagePlugin(new onTimeoutCallback(m_pConnection));
			configureTimeout();
		}
		else if (ec != boost::asio::error::operation_aborted) // Timer canceled by message arriving
		{
			pPlugin->Log(LOG_ERROR, "Timer error for port '%s': %d, %s", m_Port.c_str(), ec.value(), ec.message().c_str());
		}
		else if ((pPlugin->m_bDebug & PDM_CONNECTION) && (ec == boost::asio::error::operation_aborted))
		{
			pPlugin->Log(LOG_NORM, "Timer aborted (%s).", m_Port.c_str());
		}
	}

	void CPluginTransport::handleRead(const boost::system::error_code &e, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
	}

	void CPluginTransport::handleRead(const char *data, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
	}

	void CPluginTransport::VerifyConnection()
	{
		// If the Python CConnection object reference count ever drops to one the the connection is out of scope so shut it down
		CConnection*	pConnection = (CConnection*)m_pConnection;
		CPlugin *pPlugin = pConnection ? pConnection->pPlugin : nullptr;
		if (!pPlugin)
			return;

		if ((pPlugin->m_bDebug & PDM_CONNECTION) && m_pConnection && (m_pConnection->ob_base.ob_refcnt <= 1))
		{
			// GIL is not held normal conversion to string via PyBorrowedRef cannot be used
			std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
			std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if ((sTransport == "Serial") || (!sPort.length()))
				pPlugin->Log(LOG_NORM, "Connection '%s' released by Python, reference count is %d.", sAddress.c_str(), (int)m_pConnection->ob_base.ob_refcnt);
			else
				pPlugin->Log(LOG_NORM, "Connection '%s:%s' released by Python, reference count is %d.", sAddress.c_str(), sPort.c_str(), (int)m_pConnection->ob_base.ob_refcnt);
		}
		if (!m_bDisconnectQueued && m_pConnection && (m_pConnection->ob_base.ob_refcnt <= 1))
		{
			pPlugin->MessagePlugin(new DisconnectDirective(m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	bool CPluginTransportTCP::handleConnect()
	{
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return false;

		try
		{
			if (!m_Socket)
			{
				m_bConnecting = false;
				m_bConnected = false;
				m_Socket = new boost::asio::ip::tcp::socket(ios);

				boost::system::error_code ec;
				boost::asio::ip::tcp::resolver::query query(m_IP, m_Port);
				auto iter = m_Resolver.resolve(query);
				boost::asio::ip::tcp::endpoint endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/example/http/client/async_client.cpp
				//
				m_Resolver.async_resolve(query, [this](auto &&err, auto end) { handleAsyncResolve(err, end); });
			}
		}
		catch (std::exception& e)
		{
			pPlugin->Log(LOG_ERROR, "Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, std::string(e.what())));
			return false;
		}

		m_bConnecting = true;

		return true;
	}

	void CPluginTransportTCP::handleAsyncResolve(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		CPlugin*		pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleAsyncResolve");

		if (!err)
		{
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			m_Socket->async_connect(endpoint, [this, endpoint_iterator](auto &&err) mutable { handleAsyncConnect(err, ++endpoint_iterator); });
		}
		else
		{
			m_bConnecting = false;

			if (pPlugin)
			{
				// Notify plugin of failure and trigger cleanup
				pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, err.value(), err.message()));
				pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));

				if ((pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
					pPlugin->Log(LOG_NORM, "Asynchronous resolve aborted (%s:%s).", m_IP.c_str(), m_Port.c_str());
			}
			else
			{
				_log.Log(LOG_ERROR, "%s: Connection to '%s:%s' not associated with a plugin", __func__, m_IP.c_str(), m_Port.c_str());
			}
		}
	}

	void CPluginTransportTCP::handleAsyncConnect(const boost::system::error_code &err, const boost::asio::ip::tcp::resolver::iterator &endpoint_iterator)
	{
		CPlugin*		pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleAsyncResolve");
		if (!pPlugin)
			return;

		pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, err.value(), err.message()));

		if (!err)
		{
			m_bConnected = true;
			m_tLastSeen = time(nullptr);
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [this](auto &&err, auto bytes) { handleRead(err, bytes); });
			configureTimeout();
		}
		else
		{
			m_bConnected = false;
			if ((pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
				pPlugin->Log(LOG_NORM, "Asynchronous connect aborted (%s:%s).", m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
		}

		m_bConnecting = false;
	}

	bool CPluginTransportTCP::handleListen()
	{
		try
		{
			if (!m_Socket)
			{
				if (!m_Acceptor)
				{
					m_Acceptor = new boost::asio::ip::tcp::acceptor(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), atoi(m_Port.c_str())));
				}
				boost::system::error_code ec;

				//
				//	Acceptor based on http://www.boost.org/doc/libs/1_62_0/doc/html/boost_asio/tutorial/tutdaytime3/src.html
				//
				auto pSocket = new boost::asio::ip::tcp::socket(ios);
				m_Acceptor->async_accept(*pSocket, [this, pSocket](auto &&err) { handleAsyncAccept(pSocket, err); });
				m_bConnecting = true;
			}
		}
		catch (std::exception& e)
		{
			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportTCP::handleAsyncAccept(boost::asio::ip::tcp::socket* pSocket, const boost::system::error_code& err)
	{
		CPlugin*		pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleAsyncAccept");
		m_tLastSeen = time(nullptr);

		if (!err)
		{
			boost::asio::ip::tcp::endpoint remote_ep = pSocket->remote_endpoint();
			std::string sAddress = remote_ep.address().to_string();
			std::string sPort = std::to_string(remote_ep.port());

			PyNewRef nrArgList = Py_BuildValue("(sssss)",
				std::string(sAddress+":"+sPort).c_str(),
				PyUnicode_AsUTF8(((CConnection*)m_pConnection)->Transport),
				PyUnicode_AsUTF8(((CConnection*)m_pConnection)->Protocol),
				sAddress.c_str(),
				sPort.c_str());
			if (!nrArgList)
			{
				pPlugin->Log(LOG_ERROR, "Building connection argument list failed for TCP %s:%s.", sAddress.c_str(), sPort.c_str());
			}
			CConnection* pConnection = (CConnection*)PyObject_CallObject((PyObject*)CConnectionType, nrArgList);
			if (!pConnection)
			{
				pPlugin->Log(LOG_ERROR, "Connection object creation failed for TCP %s:%s.", sAddress.c_str(), sPort.c_str());
			}
			CPluginTransportTCP* pTcpTransport = new CPluginTransportTCP(m_HwdID, pConnection, sAddress, sPort);
			Py_DECREF(pConnection);

			// Configure transport object
			pTcpTransport->m_pConnection = pConnection;
			pTcpTransport->m_Socket = pSocket;
			pTcpTransport->m_bConnected = true;
			pTcpTransport->m_tLastSeen = time(nullptr);

			// Configure Python Connection object
			pConnection->pTransport = pTcpTransport;

			Py_XDECREF(pConnection->Parent);
			pConnection->Parent = (PyObject*)m_pConnection;
			Py_INCREF(m_pConnection);
			pConnection->Target = ((CConnection *)m_pConnection)->Target;
			if (pConnection->Target)
				Py_INCREF(pConnection->Target);
			pConnection->pPlugin = ((CConnection *)m_pConnection)->pPlugin;

			// Add it to the plugins list of connections
			pConnection->pPlugin->AddConnection(pTcpTransport);

			// Create Protocol object to handle connection's traffic
			{
				pConnection->pPlugin->MessagePlugin(new ProtocolDirective(pConnection));
				//  and signal connection
				pConnection->pPlugin->MessagePlugin(new onConnectCallback(pConnection, err.value(), err.message()));
			}

			pTcpTransport->m_Socket->async_read_some(boost::asio::buffer(pTcpTransport->m_Buffer, sizeof pTcpTransport->m_Buffer),
								 [pTcpTransport](auto &&err, auto bytes) { pTcpTransport->handleRead(err, bytes); });

			// Requeue listener
			if (m_Acceptor)
			{
				handleListen();
			}
		}
		else
		{
			if (!pPlugin)
				return;

			if (err != boost::asio::error::operation_aborted)
				pPlugin->Log(LOG_ERROR, "Accept Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());

			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin) return;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleRead");

		if (!e)
		{
			pPlugin->MessagePlugin(new ReadEvent(m_pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(nullptr);
			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_Socket)
			{
				m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [this](auto &&err, auto bytes) { handleRead(err, bytes); });
				configureTimeout();
			}
		}
		else
		{
			if ((pPlugin->m_bDebug & PDM_CONNECTION) && 
				(
					(e == boost::asio::error::operation_aborted) || // Client side connections (from connecting)
					(e == boost::asio::error::eof)		     // Server side connections (from listening)
			    )
			)
			{
				_log.Log(LOG_NORM, "Queued asynchronous read aborted (%s:%s), [%d] %s.", m_IP.c_str(), m_Port.c_str(), e.value(), e.message().c_str());
			}
			else
			{
				if (
					(e != boost::asio::error::eof) && 
					(e.value() != 121) && // Semaphore timeout expiry or end of file aka 'lost contact'
				    (e.value() != 125) && // Operation canceled
				    (e != boost::asio::error::address_in_use) &&
					(e != boost::asio::error::operation_aborted) &&
					// (e != boost::asio::error::connection_reset) &&
				    (e.value() != 1236) // local disconnect cause by hardware reload
					) 
				{
					pPlugin->Log(LOG_ERROR, "Async Read Exception (%s:%s): %d, %s", m_IP.c_str(), m_Port.c_str(), e.value(), e.message().c_str());
				}
			}

			// Timer events can still trigger even after errors so cancel explicitly
			if (m_Timer)
			{
				m_Timer->cancel();
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleWrite(const std::vector<byte>& pMessage)
	{
		if (m_Socket)
		{
			try
			{
				int	iSentBytes = boost::asio::write(*m_Socket, boost::asio::buffer(pMessage, pMessage.size()));
				m_iTotalBytes += iSentBytes;
				if (iSentBytes != pMessage.size())
				{
					CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
					if (!pPlugin)
						return;
					pPlugin->Log(LOG_ERROR, "Not all data written to socket (%s:%s). %d expected, %d written", m_IP.c_str(), m_Port.c_str(), int(pMessage.size()), iSentBytes);
				}
			}
			catch (std::exception & e)
			{
				_log.Log(LOG_ERROR, "%s: Exception thrown: '%s'", __func__, std::string(e.what()).c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "%s: Data not sent to NULL socket.", __func__);
		}
	}

	bool CPluginTransportTCP::handleDisconnect()
	{
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return false;

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			pPlugin->Log(LOG_NORM, "Handling TCP disconnect, socket (%s:%s) is %sconnected", m_IP.c_str(), m_Port.c_str(), (m_bConnected ? "" : "not "));
		}

		m_tLastSeen = time(nullptr);

		if (m_Timer)
		{
			m_Timer->cancel();
		}

		if (m_Socket && m_bConnecting)
		{
			m_Socket->close();
		}

		if (m_Socket && m_bConnected)
		{
			boost::system::error_code e;
			m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
			if (e)
			{
				pPlugin->Log(LOG_ERROR, "Socket Shutdown Error: %d, %s", e.value(), e.message().c_str());
			}
			else
			{
				m_Socket->close();
			}
		}

		if (m_Acceptor)
		{
			m_Acceptor->cancel();
		}

		m_bConnected = false;

		return true;
	}

	void CPluginTransportTCPSecure::handleWrite(const std::vector<byte>& pMessage)
	{
		if (m_TLSSock && m_Socket)
		{
			try
			{
				int		iSentBytes = boost::asio::write(*m_TLSSock, boost::asio::buffer(pMessage, pMessage.size()));
				m_iTotalBytes += iSentBytes;
				if (iSentBytes != pMessage.size())
				{
					CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
					if (!pPlugin)
						return;
					pPlugin->Log(LOG_ERROR, "Not all data written to secure socket (%s:%s). %d expected, %d written", m_IP.c_str(), m_Port.c_str(), int(pMessage.size()),
						     iSentBytes);
				}
			}
			catch (std::exception & e)
			{
				_log.Log(LOG_ERROR, "%s: Exception thrown: '%s'", __func__, std::string(e.what()).c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "%s: Data not sent to NULL socket.", __func__);
		}
	}

	CPluginTransportTCP::~CPluginTransportTCP()
	{
		if (m_Acceptor)
		{
			delete m_Acceptor;
			m_Acceptor = nullptr;
		}

		if (m_Socket)
		{
			delete m_Socket;
			m_Socket = nullptr;
		}
	};

	void CPluginTransportTCPSecure::handleAsyncConnect(const boost::system::error_code &err, const boost::asio::ip::tcp::resolver::iterator &endpoint_iterator)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin) return;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleAsyncConnect");

		if (!err)
		{
			m_Context = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);
			m_Context->set_verify_mode(boost::asio::ssl::verify_peer);
			m_Context->set_default_verify_paths();

			m_TLSSock = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>(*m_Socket, *m_Context);
			m_TLSSock->lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
			SSL_set_tlsext_host_name(m_TLSSock->native_handle(), m_IP.c_str());			// Enable SNI

			m_TLSSock->set_verify_mode(boost::asio::ssl::verify_none);
			m_TLSSock->set_verify_callback(boost::asio::ssl::rfc2818_verification(m_IP));
			// m_TLSSock->set_verify_callback([this](auto v, auto &c){ VerifyCertificate(v, c);});
			try
			{
#ifdef WWW_ENABLE_SSL
				// RK: todo: What if openssl is not compiled in?
				m_TLSSock->handshake(ssl_socket::client);
#endif

				m_bConnected = true;
				pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, err.value(), err.message()));

				m_tLastSeen = time(nullptr);
				m_TLSSock->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [this](auto &&err, auto bytes) { handleRead(err, bytes); });
				configureTimeout();
			}
			catch (boost::system::system_error se)
			{
				_log.Log(LOG_ERROR, "TLS Handshake Exception: '%s' connecting to '%s:%s'", se.what(), m_IP.c_str(), m_Port.c_str());
				pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
			}
		}
		else
		{
			m_bConnected = false;
			if ((pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
				_log.Log(LOG_NORM, "Asynchronous secure connect aborted (%s:%s).", m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, err.value(), err.message()));
			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
		}

		m_bConnecting = false;
	}

	bool CPluginTransportTCPSecure::VerifyCertificate(bool preverified, boost::asio::ssl::verify_context& ctx)
	{
		// The verify callback can be used to check whether the certificate that is
		// being presented is valid for the peer. For example, RFC 2818 describes
		// the steps involved in doing this for HTTPS. Consult the OpenSSL
		// documentation for more details. Note that the callback is called once
		// for each certificate in the certificate chain, starting from the root
		// certificate authority.

		CPlugin *pPlugin = ((CConnection *)m_pConnection)->pPlugin;
		if (!pPlugin)
			return false;

		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

		if (m_pConnection && pPlugin->m_bDebug & PDM_CONNECTION)
		{
			pPlugin->Log(LOG_NORM, "TLS Certificate found '%s'", subject_name);
		}

		// TODO: Add some certificate checking

		return true;
	}

	void CPluginTransportTCPSecure::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin) return;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleRead");

		if (!e)
		{
			pPlugin->MessagePlugin(new ReadEvent(m_pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(nullptr);
			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_TLSSock)
			{
				m_TLSSock->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer), [this](auto &&err, auto bytes) { handleRead(err, bytes); });
				configureTimeout();
			}
		}
		else
		{
			if ((pPlugin->m_bDebug & PDM_CONNECTION) &&
				((e == boost::asio::error::operation_aborted) || (e == boost::asio::error::eof)))
			{
				pPlugin->Log(LOG_NORM, "Queued asynchronous secure read aborted.");
			}
			else
			{
				if ((e.value() != boost::asio::error::eof) &&
					(e.value() != 1) &&	// Stream truncated, this exception occurs when third party closes the connection
					(e.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(e.value() != 125) &&	// Operation canceled
					(e.value() != 335544539) &&	// SSL/TLS short read, should be ignored
					(e != boost::asio::error::operation_aborted) &&
					(e.value() != 1236))	// local disconnect cause by hardware reload
					pPlugin->Log(LOG_ERROR, "Async Secure Read Exception: %d, %s", e.value(), e.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));

			m_bDisconnectQueued = true;
		}
	}

	CPluginTransportTCPSecure::~CPluginTransportTCPSecure()
	{
		if (m_Timer)
		{
			m_Timer->cancel();
		}

		if (m_TLSSock)
		{
			delete m_TLSSock;
			m_TLSSock = nullptr;
		}

		if (m_Context)
		{
			delete m_Context;
			m_Context = nullptr;
		}

	};

	bool CPluginTransportUDP::handleListen()
	{
		try
		{
			if (!m_Socket)
			{
				boost::system::error_code ec;
				int	iPort = atoi(m_Port.c_str());

				// Handle broadcast messages
				if (m_IP == "255.255.255.255")
				{
					m_Socket = new boost::asio::ip::udp::socket(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::any(), iPort));
					m_Socket->set_option(boost::asio::ip::udp::socket::socket_base::broadcast(true));
					m_Socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
				}
				else
				{
					m_Socket = new boost::asio::ip::udp::socket(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), iPort));
					m_Socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
					// Hanlde multicast
					if (((m_IP.substr(0, 4) >= "224.") && (m_IP.substr(0, 4) <= "239.")) || (m_IP.substr(0, 4) == "255."))
					{
						m_Socket->set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string(m_IP.c_str())), ec);
						m_Socket->set_option(boost::asio::ip::multicast::hops(2), ec);
					}
				}
			}

			m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_remote_endpoint, [this](auto &&err, auto bytes) { handleRead(err, bytes); });

			m_bConnected = true;
		}
		catch (std::exception& e)
		{
			m_bConnected = false;

			_log.Log(LOG_ERROR, "Plugin: UDP Listen Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportUDP::handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		AccessPython	Guard(pPlugin, "CPluginTransportTCP::handleRead");

		if (!ec)
		{
			std::string sAddress = m_remote_endpoint.address().to_string();
			std::string sPort = std::to_string(m_remote_endpoint.port());

			PyNewRef nrArgList = Py_BuildValue("(sssss)", 
												PyUnicode_AsUTF8(((CConnection*)m_pConnection)->Name), 
												PyUnicode_AsUTF8(((CConnection*)m_pConnection)->Transport),
												PyUnicode_AsUTF8(((CConnection*)m_pConnection)->Protocol),
												sAddress.c_str(),
												sPort.c_str());
			if (!nrArgList)
			{
				pPlugin->Log(LOG_ERROR, "Building connection argument list failed for UDP %s:%s.", sAddress.c_str(), sPort.c_str());
			}
			CConnection* pConnection = (CConnection*)PyObject_CallObject((PyObject*)CConnectionType, nrArgList);
			if (!pConnection)
			{
				pPlugin->Log(LOG_ERROR, "Connection object creation failed for UDP %s:%s.", sAddress.c_str(), sPort.c_str());
			}

			pConnection->Target = ((CConnection *)m_pConnection)->Target;
			if (pConnection->Target)
				Py_INCREF(pConnection->Target);
			pConnection->pPlugin = ((CConnection *)m_pConnection)->pPlugin;

			// Create Protocol object to handle connection's traffic
			pConnection->pPlugin->MessagePlugin(new ProtocolDirective(pConnection));
			pConnection->pPlugin->MessagePlugin(new ReadEvent(pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(nullptr);
			m_iTotalBytes += bytes_transferred;

			// Make sure only the only Message objects are referring to Connection so that it is cleaned up right after plugin onMessage
			Py_DECREF(pConnection);

			// Set up listener again
			if (m_bConnected)
				handleListen();
			else
			{
				// should only happen if async_receive_from doesn't call handleRead with 'abort' condition
				pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection, false));
				m_bDisconnectQueued = true;
			}
		}
		else
		{
			if (!pPlugin)
				return;
			if (pPlugin->m_bDebug & PDM_CONNECTION)
				pPlugin->Log(LOG_NORM, "Queued asynchronous UDP read aborted (%s:%s).", m_IP.c_str(), m_Port.c_str());
			else
			{
				if ((ec.value() != boost::asio::error::eof) &&
					(ec.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(ec.value() != 125) &&	// Operation canceled
					(ec.value() != boost::asio::error::operation_aborted) &&	// Abort due to shutdown during disconnect
					(ec.value() != 1236))	// local disconnect cause by hardware reload
					pPlugin->Log(LOG_ERROR, "Async UDP Read Exception: %d, %s", ec.value(), ec.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection, false));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportUDP::handleWrite(const std::vector<byte>& pMessage)
	{
		try
		{
			if (!m_Socket)
			{
				boost::system::error_code  err;
				m_Socket = new boost::asio::ip::udp::socket(ios);
				m_Socket->open(boost::asio::ip::udp::v4(), err);
				m_Socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
			}

			// Different handling for multi casting
			if (((m_IP.substr(0, 4) >= "224.") && (m_IP.substr(0, 4) <= "239.")) || (m_IP.substr(0, 4) == "255."))
			{
				m_Socket->set_option(boost::asio::socket_base::broadcast(true));
				boost::asio::ip::udp::endpoint destination(boost::asio::ip::address_v4::broadcast(), atoi(m_Port.c_str()));
				int bytes_transferred = m_Socket->send_to(boost::asio::buffer(pMessage, pMessage.size()), destination);
			}
			else
			{
				boost::asio::ip::udp::endpoint destination(boost::asio::ip::address::from_string(m_IP.c_str()), atoi(m_Port.c_str()));
				int bytes_transferred = m_Socket->send_to(boost::asio::buffer(pMessage, pMessage.size()), destination);
			}
		}
		catch (boost::system::system_error err)
		{
			_log.Log(LOG_ERROR, "%s: '%s' during 'send_to' operation: %d bytes", __func__, err.what(), (int)pMessage.size());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Socket error during 'send_to' operation: %d bytes", __func__, (int)pMessage.size());
		}
	}

	bool CPluginTransportUDP::handleDisconnect()
	{
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return false;

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			pPlugin->Log(LOG_NORM, "Handling UDP disconnect, socket (%s:%s) is %sconnected", m_IP.c_str(), m_Port.c_str(), (m_bConnected ? "" : "not "));
		}

		m_tLastSeen = time(nullptr);
		if (m_bConnected)
		{
			if (m_Socket)
			{
				// Returns an 'endpoint not connected' error on Linux
				boost::system::error_code e;
				m_Socket->shutdown(boost::asio::ip::udp::socket::shutdown_both, e);
				if (e)
				{
					if (e.value() != boost::asio::error::not_connected)		// Linux always reports error 107, Windows does not
						pPlugin->Log(LOG_ERROR, "Socket Shutdown Error: %d, %s", e.value(), e.message().c_str());
					else
						m_Socket->close();
				}
				else
				{
					m_Socket->close();
				}
			}
		}

		m_bConnected = false;

		return true;
	}

	CPluginTransportUDP::~CPluginTransportUDP()
	{
		if (m_Socket)
		{
			delete m_Socket;
			m_Socket = nullptr;
		}
	};

	void CPluginTransportICMP::handleAsyncResolve(const boost::system::error_code &ec, const boost::asio::ip::icmp::resolver::iterator &endpoint_iterator)
	{
		if (!ec)
		{
			m_bConnected = true;
			m_IP = endpoint_iterator->endpoint().address().to_string();

			// Listen will fail (10022 - bad parameter) unless something has been sent(?)
			std::string body("ping");
			std::vector<byte>	vBody(&body[0], &body[body.length()]);
			handleWrite(vBody);

			m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint, [this](auto &&err, auto bytes) { handleRead(err, bytes); });
		}
		else
		{
			CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			AccessPython	Guard(pPlugin, "CPluginTransportICMP::handleAsyncResolve");
			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection));
		}
		m_bConnecting = false;
	}

	bool CPluginTransportICMP::handleListen()
	{
		try
		{
			if (!m_bConnected)
			{
				m_bConnecting = true;
				m_Socket = new boost::asio::ip::icmp::socket(ios, boost::asio::ip::icmp::v4());

				boost::system::error_code ec;
				boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), m_IP, "");
				auto iter = m_Resolver.resolve(query);
				m_Endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/example/icmp/ping.cpp
				//
				m_Resolver.async_resolve(query, [this](auto &&err, auto i) { handleAsyncResolve(err, i); });
			}
			else
			{
				m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint, [this](auto &&err, auto bytes) { handleRead(err, bytes); });
			}

			m_pConnection->pPlugin->MessagePlugin(new ProtocolDirective(m_pConnection));
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "%s Exception: '%s' failed connecting to '%s'", __func__, e.what(), m_IP.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportICMP::handleTimeout(const boost::system::error_code& ec)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin) return;
		AccessPython	Guard(pPlugin, "CPluginTransportICMP::handleTimeout");

		if (!ec)  // Timeout, no response
		{
			if (pPlugin->m_bDebug & PDM_CONNECTION)
			{
				pPlugin->Log(LOG_NORM, "ICMP timeout for address '%s'", m_IP.c_str());
			}

			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new ReadEvent(m_pConnection, 0, nullptr));
			pPlugin->MessagePlugin(new DisconnectDirective(m_pConnection));
		}
		else if (ec != boost::asio::error::operation_aborted)  // Timer canceled by message arriving
		{
			pPlugin->Log(LOG_ERROR, "ICMP timer error for address '%s': %d, %s", m_IP.c_str(), ec.value(), ec.message().c_str());
		}
		else if ((pPlugin->m_bDebug & PDM_CONNECTION) && (ec == boost::asio::error::operation_aborted))
		{
			pPlugin->Log(LOG_NORM, "ICMP timer aborted (%s).", m_IP.c_str());
		}
	}

	void CPluginTransportICMP::handleRead(const boost::system::error_code & ec, std::size_t bytes_transferred)
	{
		CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin) return;
		AccessPython	Guard(pPlugin, "CPluginTransportICMP::handleRead");

		if (!ec)
		{
			clock_t	ElapsedTime = clock() - m_Clock;
			int		iMsElapsed = (ElapsedTime*1000)/CLOCKS_PER_SEC;
			ipv4_header*	pIPv4 = (ipv4_header*)&m_Buffer;
			icmp_header*	pICMP = (icmp_header*)(&m_Buffer[0] + 20);
			std::string		sAddress;

			// Under Linux all ICMP traffic will be seen so filter out extra traffic
			if (pICMP->type() == icmp_header::echo_reply)						// Successful Echo Reply for the requested address
			{
				sAddress = pIPv4->source_address().to_string();
			}
			else if (pICMP->type() == icmp_header::destination_unreachable)		// Unsuccessful Echo Reply for the requested address
			{
				// on failure part of the original request is appended to the ICMP header
				ipv4_header*	pIPv4 = (ipv4_header*)(pICMP+1);
				sAddress = pIPv4->destination_address().to_string();
			}

			if (sAddress == m_IP)
			{
				// Cancel timeout
				if (m_Timer)
				{
					m_Timer->cancel();
				}

				pPlugin->MessagePlugin(new ReadEvent(m_pConnection, bytes_transferred, m_Buffer, (iMsElapsed ? iMsElapsed : 1)));

				m_tLastSeen = time(nullptr);
				m_iTotalBytes += bytes_transferred;
			}

			// Set up listener again
			handleListen();
		}
		else
		{
			if ((pPlugin->m_bDebug & PDM_CONNECTION) &&
				((ec == boost::asio::error::operation_aborted) || (ec == boost::asio::error::eof)))
				pPlugin->Log(LOG_NORM, "Queued asynchronous ICMP read aborted (%s).", m_IP.c_str());
			else
			{
				if ((ec.value() != boost::asio::error::eof) && (ec.value() != 121) && // Semaphore timeout expiry or end of file aka 'lost contact'
				    (ec.value() != 125) &&					      // Operation canceled
				    (ec.value() != boost::asio::error::operation_aborted) &&	      // Abort due to shutdown during disconnect
				    (ec.value() != 1236))					      // local disconnect cause by hardware reload
				{
					pPlugin->Log(LOG_ERROR, "Async Receive From Exception: %d, %s", ec.value(), ec.message().c_str());
				}
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(m_pConnection, false));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportICMP::handleWrite(const std::vector<byte>& pMessage)
	{
		// Check transport is usable
		if (!m_bConnected)
		{
			CConnection*	pConnection = (CConnection*)this->m_pConnection;
			CPlugin *pPlugin = pConnection->pPlugin;
			if (!pPlugin)
				return;
			std::string sConnection = PyUnicode_AsUTF8(pConnection->Name);

			pPlugin->Log(LOG_ERROR, "Transport not initialized, write directive to '%s' ignored. Connectionless transport should be Listening.", sConnection.c_str());
		}

		// Reset timeout if one is set or set one
		if (!m_Timer)
		{
			m_Timer = new boost::asio::deadline_timer(ios);
		}
		m_Timer->expires_from_now(boost::posix_time::seconds(5));
		m_Timer->async_wait([this](auto &&err) { handleTimeout(err); });

		// Create an ICMP header for an echo request.
		icmp_header echo_request;
		echo_request.type(icmp_header::echo_request);
		echo_request.code(0);
#if defined(BOOST_ASIO_WINDOWS)
		echo_request.identifier(static_cast<unsigned short>(::GetCurrentProcessId()));
#else
		echo_request.identifier(::getpid());
#endif
		echo_request.sequence_number(++m_SequenceNo);
		compute_checksum(echo_request, pMessage.begin(), pMessage.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		std::string	 sData(pMessage.begin(), pMessage.end());
		os << echo_request << sData;

		// Send the request and mark the time
		m_Clock = clock();
		m_Socket->send_to(request_buffer.data(), m_Endpoint);
	}

	bool CPluginTransportICMP::handleDisconnect()
	{
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return false;

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			pPlugin->Log(LOG_NORM, "Handling ICMP disconnect, socket (%s) is %sconnected", m_IP.c_str(), (m_bConnected ? "" : "not "));
		}

		m_tLastSeen = time(nullptr);
		if (m_Timer)
		{
			m_Timer->cancel();
		}

		if (m_Socket)
		{
			boost::system::error_code e;
			m_Socket->shutdown(boost::asio::ip::icmp::socket::shutdown_both, e);
			if (e)
			{
#ifdef WIN32
				if (e.value() != 10009)		// Windows can report 10009, The file handle supplied is not valid
#else
				if (e.value() != boost::asio::error::not_connected)		// Linux always reports error 107, Windows does not
#endif
					pPlugin->Log(LOG_ERROR, "Socket Shutdown Error: %d, %s", e.value(), e.message().c_str());
			}
			else
			{
				m_Socket->close();
			}
		}

		return true;
	}

	CPluginTransportICMP::~CPluginTransportICMP()
	{
		if (m_Timer)
		{
			m_Timer->cancel();
			delete m_Timer;
			m_Timer = nullptr;
		}

		if (m_Socket)
		{
			delete m_Socket;
			m_Socket = nullptr;
		}
	}

	CPluginTransportSerial::CPluginTransportSerial(int HwdID, CConnection* pConnection, const std::string & Port, int Baud) : CPluginTransport(HwdID, pConnection), m_Baud(Baud)
	{
		m_Port = Port;
	}

	bool CPluginTransportSerial::handleConnect()
	{
		try
		{
			if (!isOpen())
			{
				m_bConnected = false;
				open(m_Port, m_Baud,
					boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none),
					boost::asio::serial_port_base::character_size(8),
					boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none),
					boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

				m_tLastSeen = time(nullptr);
				m_bConnected = isOpen();

				CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
				if (m_bConnected)
				{
					pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, 0, "SerialPort " + m_Port + " opened successfully."));
					setReadCallback([this](auto err, auto bytes) { handleRead(err, bytes); });
					configureTimeout();
				}
				else
				{
					pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, "SerialPort " + m_Port + " open failed, check log for details."));
				}
			}
		}
		catch (std::exception& e)
		{
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return m_bConnected;
	}

	void CPluginTransportSerial::handleRead(const char *data, std::size_t bytes_transferred)
	{
		if (bytes_transferred)
		{
			CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			AccessPython	Guard(pPlugin, "CPluginTransportSerial::handleRead");
			pPlugin->MessagePlugin(new ReadEvent(m_pConnection, bytes_transferred, (const unsigned char*)data));
			configureTimeout();
			m_tLastSeen = time(nullptr);
			m_iTotalBytes += bytes_transferred;
		}
		else
		{
			_log.Log(LOG_ERROR, "CPluginTransportSerial: handleRead called with no data.");
		}
	}

	void CPluginTransportSerial::handleWrite(const std::vector<byte>& data)
	{
		if (!data.empty())
		{
			write((const char *)&data[0], data.size());
		}
	}

	bool CPluginTransportSerial::handleDisconnect()
	{
		m_tLastSeen = time(nullptr);
		if (m_bConnected)
		{
			if (isOpen())
			{
				// Cancel timeout
				if (m_Timer)
				{
					m_Timer->cancel();
				}
				terminate();
				CPlugin *pPlugin = m_pConnection->pPlugin;
				if (pPlugin)
				{
					pPlugin->MessagePlugin(new onDisconnectCallback(m_pConnection));
				}
				else
				{
					_log.Log(LOG_ERROR, "CPluginTransportSerial: %s, onDisconnect not queued. Plugin was NULL.", __func__);
				}
			}
			m_bConnected = false;
		}
		return true;
	}
} // namespace Plugins
#endif
