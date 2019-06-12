#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginTransports.h"
#include "PythonObjects.h"

#include "../main/Logger.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

namespace Plugins {

	void CPluginTransport::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
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
		CPlugin*		pPlugin = pConnection ? pConnection->pPlugin : NULL;
		if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) && m_pConnection && (m_pConnection->ob_refcnt <= 1))
		{
			std::string	sTransport = PyUnicode_AsUTF8(pConnection->Transport);
			std::string	sAddress = PyUnicode_AsUTF8(pConnection->Address);
			std::string	sPort = PyUnicode_AsUTF8(pConnection->Port);
			if ((sTransport == "Serial") || (!sPort.length()))
				_log.Log(LOG_NORM, "(%s) Connection '%s' released by Python, reference count is %d.", pPlugin->m_Name.c_str(), sAddress.c_str(), (int)m_pConnection->ob_refcnt);
			else
				_log.Log(LOG_NORM, "(%s) Connection '%s:%s' released by Python, reference count is %d.", pPlugin->m_Name.c_str(), sAddress.c_str(), sPort.c_str(), (int)m_pConnection->ob_refcnt);
		}
		if (!m_bDisconnectQueued && m_pConnection && (m_pConnection->ob_refcnt <= 1) && pPlugin)
		{
			pPlugin->MessagePlugin(new DisconnectDirective(pPlugin, m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	bool CPluginTransportTCP::handleConnect()
	{
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;

		try
		{
			if (!m_Socket)
			{
				m_bConnecting = false;
				m_bConnected = false;
				m_Socket = new boost::asio::ip::tcp::socket(ios);

				boost::system::error_code ec;
				boost::asio::ip::tcp::resolver::query query(m_IP, m_Port);
				boost::asio::ip::tcp::resolver::iterator iter = m_Resolver.resolve(query);
				boost::asio::ip::tcp::endpoint endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/example/http/client/async_client.cpp
				//
				m_Resolver.async_resolve(query, boost::bind(&CPluginTransportTCP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
			}
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, std::string(e.what())));
			return false;
		}

		m_bConnecting = true;

		return true;
	}

	void CPluginTransportTCP::handleAsyncResolve(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;

		if (!err)
		{
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			m_Socket->async_connect(endpoint, boost::bind(&CPluginTransportTCP::handleAsyncConnect, this, boost::asio::placeholders::error, ++endpoint_iterator));
		}
		else
		{
			m_bConnecting = false;

			// Notify plugin of failure and trigger cleanup
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, err.value(), err.message()));
			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));

			if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
				_log.Log(LOG_NORM, "(%s) Asyncronous resolve aborted (%s:%s).", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str());
		}
	}

	void CPluginTransportTCP::handleAsyncConnect(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;

		pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, err.value(), err.message()));

		if (!err)
		{
			m_bConnected = true;
			m_tLastSeen = time(0);
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CPluginTransportTCP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			m_bConnected = false;
			if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
				_log.Log(LOG_NORM, "(%s) Asyncronous connect aborted (%s:%s).", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
		}

		m_bConnecting = false;
	}

	bool CPluginTransportTCP::handleListen()
	{
		try
		{
			PyType_Ready(&CConnectionType);

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
				boost::asio::ip::tcp::socket*	pSocket = new boost::asio::ip::tcp::socket(ios);
				m_Acceptor->async_accept((boost::asio::ip::tcp::socket&)*pSocket, boost::bind(&CPluginTransportTCP::handleAsyncAccept, this, pSocket, boost::asio::placeholders::error));
				m_bConnecting = true;
			}
		}
		catch (std::exception& e)
		{
			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportTCP::handleAsyncAccept(boost::asio::ip::tcp::socket* pSocket, const boost::system::error_code& err)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		m_tLastSeen = time(0);

		if (!err)
		{
			boost::asio::ip::tcp::endpoint remote_ep = pSocket->remote_endpoint();
			std::string sAddress = remote_ep.address().to_string();
			std::string sPort = std::to_string(remote_ep.port());

			CConnection* pConnection = (CConnection*)CConnection_new(&CConnectionType, (PyObject*)NULL, (PyObject*)NULL);
			CPluginTransportTCP* pTcpTransport = new CPluginTransportTCP(m_HwdID, (PyObject*)pConnection, sAddress, sPort);
			Py_DECREF(pConnection);

			// Configure transport object
			pTcpTransport->m_pConnection = (PyObject*)pConnection;
			pTcpTransport->m_Socket = pSocket;
			pTcpTransport->m_bConnected = true;
			pTcpTransport->m_tLastSeen = time(0);

			// Configure Python Connection object
			pConnection->pTransport = pTcpTransport;
			Py_XDECREF(pConnection->Name);
			pConnection->Name = PyUnicode_FromString(std::string(sAddress+":"+sPort).c_str());
			Py_XDECREF(pConnection->Address);
			pConnection->Address = PyUnicode_FromString(sAddress.c_str());
			Py_XDECREF(pConnection->Port);
			pConnection->Port = PyUnicode_FromString(sPort.c_str());

			Py_XDECREF(pConnection->Parent);
			pConnection->Parent = m_pConnection;
			Py_INCREF(m_pConnection);
			pConnection->Transport = ((CConnection*)m_pConnection)->Transport;
			Py_INCREF(pConnection->Transport);
			pConnection->Protocol = ((CConnection*)m_pConnection)->Protocol;
			Py_INCREF(pConnection->Protocol);
			pConnection->pPlugin = ((CConnection*)m_pConnection)->pPlugin;

			// Add it to the plugins list of connections
			pConnection->pPlugin->AddConnection(pTcpTransport);

			// Create Protocol object to handle connection's traffic
			{
				pConnection->pPlugin->MessagePlugin(new ProtocolDirective(pConnection->pPlugin, (PyObject*)pConnection));
				//  and signal connection
				pConnection->pPlugin->MessagePlugin(new onConnectCallback(pConnection->pPlugin, (PyObject*)pConnection, err.value(), err.message()));
			}

			pTcpTransport->m_Socket->async_read_some(boost::asio::buffer(pTcpTransport->m_Buffer, sizeof pTcpTransport->m_Buffer),
				boost::bind(&CPluginTransportTCP::handleRead, pTcpTransport, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

			// Requeue listener
			if (m_Acceptor)
			{
				handleListen();
			}
		}
		else
		{
			if (err != boost::asio::error::operation_aborted)
				_log.Log(LOG_ERROR, "Plugin: Accept Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());

			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!e)
		{
			pPlugin->MessagePlugin(new ReadEvent(pPlugin, m_pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(0);
			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_Socket)
				m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
					boost::bind(&CPluginTransportTCP::handleRead,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) &&
					((e == boost::asio::error::operation_aborted) ||	// Client side connections (from connecting)
					 (e == boost::asio::error::eof)))					// Server side connections (from listening)
				_log.Log(LOG_NORM, "(%s) Queued asyncronous read aborted (%s:%s), [%d] %s.", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str(), e.value(), e.message().c_str());
			else
			{
				if ((e != boost::asio::error::eof) &&
					(e.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(e.value() != 125) &&	// Operation canceled
					(e != boost::asio::error::address_in_use) &&
					(e != boost::asio::error::operation_aborted) &&
					(e.value() != 1236))	// local disconnect cause by hardware reload
					_log.Log(LOG_ERROR, "(%s): Async Read Exception (%s:%s): %d, %s", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str(), e.value(), e.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleWrite(const std::vector<byte>& pMessage)
	{
		if (m_Socket)
		{
			try
			{
				int		iSentBytes = boost::asio::write(*m_Socket, boost::asio::buffer(pMessage, pMessage.size()));
				m_iTotalBytes += iSentBytes;
				if (iSentBytes != pMessage.size())
				{
					CPlugin* pPlugin = ((CConnection*)m_pConnection)->pPlugin;
					_log.Log(LOG_ERROR, "(%s) Not all data written to socket (%s:%s). %d expected,  %d written", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str(), pMessage.size(), iSentBytes);
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

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			_log.Log(LOG_NORM, "(%s) Handling TCP disconnect, socket (%s:%s) is %sconnected", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str(), (m_bConnected?"":"not "));
		}

		m_tLastSeen = time(0);

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
				_log.Log(LOG_ERROR, "(%s): Socket Shutdown Error: %d, %s", pPlugin->m_Name.c_str(), e.value(), e.message().c_str());
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

	CPluginTransportTCP::~CPluginTransportTCP()
	{
		if (m_Acceptor)
		{
			delete m_Acceptor;
			m_Acceptor = NULL;
		}

		if (m_Socket)
		{
			delete m_Socket;
			m_Socket = NULL;
		}
	};

	void CPluginTransportTCPSecure::handleAsyncConnect(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;

		if (!err)
		{
			m_Context = new boost::asio::ssl::context(boost::asio::ssl::context::sslv23);
			m_Context->set_verify_mode(boost::asio::ssl::verify_peer);
			m_Context->set_default_verify_paths();

			m_TLSSock = new boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>(*m_Socket, *m_Context);
			m_TLSSock->lowest_layer().set_option(boost::asio::ip::tcp::no_delay(true));
			SSL_set_tlsext_host_name(m_TLSSock->native_handle(), m_IP.c_str());			// Enable SNI

			m_TLSSock->set_verify_mode(boost::asio::ssl::verify_none);
			m_TLSSock->set_verify_callback(boost::asio::ssl::rfc2818_verification(m_IP.c_str()));
			//m_TLSSock->set_verify_callback(boost::bind(&CPluginTransportTCPSecure::VerifyCertificate, this, _1, _2));
			try
			{
				m_TLSSock->handshake(ssl_socket::client);

				m_bConnected = true;
				pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, err.value(), err.message()));

				m_tLastSeen = time(0);
				m_TLSSock->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
					boost::bind(&CPluginTransportTCP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			}
			catch (boost::system::system_error se)
			{
				_log.Log(LOG_ERROR, "(%s) TLS Handshake Exception: '%s' connecting to '%s:%s'", pPlugin->m_Name.c_str(), se.what(), m_IP.c_str(), m_Port.c_str());
				pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
			}
		}
		else
		{
			m_bConnected = false;
			if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) && (err == boost::asio::error::operation_aborted))
				_log.Log(LOG_NORM, "(%s) Asyncronous secure connect aborted (%s:%s).", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str());
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, err.value(), err.message()));
			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
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

		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		if (m_pConnection && ((CConnection*)m_pConnection)->pPlugin->m_bDebug & PDM_CONNECTION)
		{
			_log.Log(LOG_NORM, "(%s) TLS Certificate found '%s'", ((CConnection*)m_pConnection)->pPlugin->m_Name.c_str(), subject_name);
		}

		// TODO: Add some certificate checking

		return true;
	}

	void CPluginTransportTCPSecure::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return;
		if (!e)
		{
			pPlugin->MessagePlugin(new ReadEvent(pPlugin, m_pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(0);
			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_TLSSock)
				m_TLSSock->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
					boost::bind(&CPluginTransportTCPSecure::handleRead,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			if ((pPlugin->m_bDebug & PDM_CONNECTION) &&
				((e == boost::asio::error::operation_aborted) || (e == boost::asio::error::eof)))
			{
				_log.Log(LOG_NORM, "(%s) Queued asyncronous secure read aborted.", pPlugin->m_Name.c_str());
			}
			else
			{
				if ((e.value() != boost::asio::error::eof) &&
					(e.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(e.value() != 125) &&	// Operation canceled
					(e != boost::asio::error::operation_aborted) &&
					(e.value() != 1236))	// local disconnect cause by hardware reload
					_log.Log(LOG_ERROR, "(%s): Async Secure Read Exception: %d, %s", pPlugin->m_Name.c_str(), e.value(), e.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));

			m_bDisconnectQueued = true;
		}
	}

	CPluginTransportTCPSecure::~CPluginTransportTCPSecure()
	{
		if (m_TLSSock)
		{
			delete m_TLSSock;
			m_TLSSock = NULL;
		}

		if (m_Context)
		{
			delete m_Context;
			m_Context = NULL;
		}

	};

	bool CPluginTransportUDP::handleListen()
	{
		try
		{
			PyType_Ready(&CConnectionType);

			if (!m_Socket)
			{
				boost::system::error_code ec;
				int	iPort = atoi(m_Port.c_str());

				m_Socket = new boost::asio::ip::udp::socket(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), iPort));
				m_Socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
				if ((m_IP.substr(0, 4) >= "224.") && (m_IP.substr(0, 4) <= "239.") || (m_IP.substr(0, 4) == "255."))
				{
					m_Socket->set_option(boost::asio::ip::multicast::join_group(boost::asio::ip::address::from_string(m_IP.c_str())), ec);
					m_Socket->set_option(boost::asio::ip::multicast::hops(2), ec);
				}
			}

			m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_remote_endpoint,
											boost::bind(&CPluginTransportUDP::handleRead, this,
												boost::asio::placeholders::error,
												boost::asio::placeholders::bytes_transferred));

			m_bConnected = true;
		}
		catch (std::exception& e)
		{
			m_bConnected = false;

			_log.Log(LOG_ERROR, "Plugin: UDP Listen Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportUDP::handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!ec)
		{
			std::string sAddress = m_remote_endpoint.address().to_string();
			std::string sPort = std::to_string(m_remote_endpoint.port());

			CConnection* pConnection = (CConnection*)CConnection_new(&CConnectionType, (PyObject*)NULL, (PyObject*)NULL);

			// Configure temporary Python Connection object
			Py_XDECREF(pConnection->Name);
			pConnection->Name = ((CConnection*)m_pConnection)->Name;
			Py_INCREF(pConnection->Name);
			Py_XDECREF(pConnection->Address);
			pConnection->Address = PyUnicode_FromString(sAddress.c_str());
			Py_XDECREF(pConnection->Port);
			pConnection->Port = PyUnicode_FromString(sPort.c_str());
			pConnection->Transport = ((CConnection*)m_pConnection)->Transport;
			Py_INCREF(pConnection->Transport);
			pConnection->Protocol = ((CConnection*)m_pConnection)->Protocol;
			Py_INCREF(pConnection->Protocol);
			pConnection->pPlugin = ((CConnection*)m_pConnection)->pPlugin;

			// Create Protocol object to handle connection's traffic
			pConnection->pPlugin->MessagePlugin(new ProtocolDirective(pConnection->pPlugin, (PyObject*)pConnection));
			pConnection->pPlugin->MessagePlugin(new ReadEvent(pConnection->pPlugin, (PyObject*)pConnection, bytes_transferred, m_Buffer));

			m_tLastSeen = time(0);
			m_iTotalBytes += bytes_transferred;

			// Make sure only the only Message objects are referring to Connection so that it is cleaned up right after plugin onMessage
			Py_DECREF(pConnection);

			// Set up listener again
			if (m_bConnected)
				handleListen();
			else
			{
				// should only happen if async_receive_from doesn't call handleRead with 'abort' condition
				pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection, false));
				m_bDisconnectQueued = true;
			}
		}
		else
		{
			if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION))
				_log.Log(LOG_NORM, "(%s) Queued asyncronous UDP read aborted (%s:%s).", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str());
			else
			{
				if ((ec.value() != boost::asio::error::eof) &&
					(ec.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(ec.value() != 125) &&	// Operation canceled
					(ec.value() != boost::asio::error::operation_aborted) &&	// Abort due to shutdown during disconnect
					(ec.value() != 1236))	// local disconnect cause by hardware reload
					_log.Log(LOG_ERROR, "(%s): Async UDP Read Exception: %d, %s", pPlugin->m_Name.c_str(), ec.value(), ec.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection, false));
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
			if ((m_IP.substr(0, 4) >= "224.") && (m_IP.substr(0, 4) <= "239.") || (m_IP.substr(0, 4) == "255."))
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

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			_log.Log(LOG_NORM, "(%s) Handling UDP disconnect, socket (%s:%s) is %sconnected", pPlugin->m_Name.c_str(), m_IP.c_str(), m_Port.c_str(), (m_bConnected ? "" : "not "));
		}

		m_tLastSeen = time(0);
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
						_log.Log(LOG_ERROR, "(%s) Socket Shutdown Error: %d, %s", pPlugin->m_Name.c_str(), e.value(), e.message().c_str());
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
			m_Socket = NULL;
		}
	};

	void CPluginTransportICMP::handleAsyncResolve(const boost::system::error_code &ec, boost::asio::ip::icmp::resolver::iterator endpoint_iterator)
	{
		if (!ec)
		{
			m_bConnected = true;
			m_IP = endpoint_iterator->endpoint().address().to_string();

			// Listen will fail (10022 - bad parameter) unless something has been sent(?)
			std::string body("ping");
			std::vector<byte>	vBody(&body[0], &body[body.length()]);
			handleWrite(vBody);

			m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint,
				boost::bind(&CPluginTransportICMP::handleRead, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection));
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
				boost::asio::ip::icmp::resolver::iterator iter = m_Resolver.resolve(query);
				m_Endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/example/icmp/ping.cpp
				//
				m_Resolver.async_resolve(query, boost::bind(&CPluginTransportICMP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
			}
			else
			{
				m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint,
					boost::bind(&CPluginTransportICMP::handleRead, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "%s Exception: '%s' failed connecting to '%s'", __func__, e.what(), m_IP.c_str());
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return true;
	}

	void CPluginTransportICMP::handleTimeout(const boost::system::error_code& ec)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;

		if (!ec)  // Timeout, no response
		{
			if (pPlugin->m_bDebug & PDM_CONNECTION)
			{
				_log.Log(LOG_NORM, "(%s) ICMP timeout for address '%s'", pPlugin->m_Name.c_str(), m_IP.c_str());
			}

			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new ReadEvent(pPlugin, m_pConnection, 0, NULL));
			pPlugin->MessagePlugin(new DisconnectDirective(pPlugin, m_pConnection));
		}
		else if (ec != boost::asio::error::operation_aborted)  // Timer canceled by message arriving
		{
			_log.Log(LOG_ERROR, "(%s) ICMP timer error for address '%s': %d, %s", pPlugin->m_Name.c_str(), m_IP.c_str(), ec.value(), ec.message().c_str());
		}
		else if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION) && (ec == boost::asio::error::operation_aborted))
		{
			_log.Log(LOG_NORM, "(%s) ICMP timer aborted (%s).", pPlugin->m_Name.c_str(), m_IP.c_str());
		}
	}

	void CPluginTransportICMP::handleRead(const boost::system::error_code & ec, std::size_t bytes_transferred)
	{
		std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
		CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
		if (!pPlugin)
			return;

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

				pPlugin->MessagePlugin(new ReadEvent(pPlugin, m_pConnection, bytes_transferred, m_Buffer, (iMsElapsed ? iMsElapsed : 1)));

				m_tLastSeen = time(0);
				m_iTotalBytes += bytes_transferred;
			}

			// Set up listener again
			handleListen();
		}
		else
		{
			if ((pPlugin->m_bDebug & PDM_CONNECTION) &&
				((ec == boost::asio::error::operation_aborted) || (ec == boost::asio::error::eof)))
				_log.Log(LOG_NORM, "(%s) Queued asyncronous ICMP read aborted (%s).", pPlugin->m_Name.c_str(), m_IP.c_str());
			else
			{
				if ((ec.value() != boost::asio::error::eof) &&
					(ec.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
					(ec.value() != 125) &&	// Operation canceled
					(ec.value() != boost::asio::error::operation_aborted) &&	// Abort due to shutdown during disconnect
					(ec.value() != 1236))	// local disconnect cause by hardware reload
						_log.Log(LOG_ERROR, "(%s): Async Receive From Exception: %d, %s", pPlugin->m_Name.c_str(), ec.value(), ec.message().c_str());
			}

			pPlugin->MessagePlugin(new DisconnectedEvent(pPlugin, m_pConnection, false));
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportICMP::handleWrite(const std::vector<byte>& pMessage)
	{
		// Check transport is usable
		if (!m_bConnected)
		{
			CConnection*	pConnection = (CConnection*)this->m_pConnection;
			std::string	sConnection = PyUnicode_AsUTF8(pConnection->Name);
			_log.Log(LOG_ERROR, "(%s) Transport not initialized, write directive to '%s' ignored. Connectionless transport should be Listening.", pConnection->pPlugin->m_Name.c_str(), sConnection.c_str());
		}

		// Reset timeout if one is set or set one
		if (!m_Timer)
		{
			m_Timer = new boost::asio::deadline_timer(ios);
		}
		m_Timer->expires_from_now(boost::posix_time::seconds(5));
		m_Timer->async_wait(boost::bind(&CPluginTransportICMP::handleTimeout, this, boost::asio::placeholders::error));

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

		if (pPlugin->m_bDebug & PDM_CONNECTION)
		{
			_log.Log(LOG_NORM, "(%s) Handling ICMP disconnect, socket (%s) is %sconnected", pPlugin->m_Name.c_str(), m_IP.c_str(), (m_bConnected ? "" : "not "));
		}

		m_tLastSeen = time(0);
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
					_log.Log(LOG_ERROR, "(%s) Socket Shutdown Error: %d, %s", pPlugin->m_Name.c_str(), e.value(), e.message().c_str());
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
			m_Timer = NULL;
		}

		if (m_Socket)
		{
			delete m_Socket;
			m_Socket = NULL;
		}
	}

	CPluginTransportSerial::CPluginTransportSerial(int HwdID, PyObject* pConnection, const std::string & Port, int Baud) : CPluginTransport(HwdID, pConnection), m_Baud(Baud)
	{
		m_Port = Port;
	}

	CPluginTransportSerial::~CPluginTransportSerial(void)
	{
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

				m_tLastSeen = time(0);
				m_bConnected = isOpen();

				CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
				if (m_bConnected)
				{
					pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, 0, "SerialPort " + m_Port + " opened successfully."));
					setReadCallback(boost::bind(&CPluginTransportSerial::handleRead, this, _1, _2));
				}
				else
				{
					pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, "SerialPort " + m_Port + " open failed, check log for details."));
				}
			}
		}
		catch (std::exception& e)
		{
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new onConnectCallback(pPlugin, m_pConnection, -1, std::string(e.what())));
			return false;
		}

		return m_bConnected;
	}

	void CPluginTransportSerial::handleRead(const char *data, std::size_t bytes_transferred)
	{
		if (bytes_transferred)
		{
			std::lock_guard<std::mutex> l(PythonMutex); // Take mutex to guard access to CPluginTransport::m_pConnection
			CPlugin*	pPlugin = ((CConnection*)m_pConnection)->pPlugin;
			pPlugin->MessagePlugin(new ReadEvent(pPlugin, m_pConnection, bytes_transferred, (const unsigned char*)data));

			m_tLastSeen = time(0);
			m_iTotalBytes += bytes_transferred;
		}
		else
		{
			_log.Log(LOG_ERROR, "CPluginTransportSerial: handleRead called with no data.");
		}
	}

	void CPluginTransportSerial::handleWrite(const std::vector<byte>& data)
	{
		if (data.size())
		{
			write((const char *)&data[0], data.size());
		}
	}

	bool CPluginTransportSerial::handleDisconnect()
	{
		m_tLastSeen = time(0);
		if (m_bConnected)
		{
			if (isOpen())
			{
				terminate();
			}
			m_bConnected = false;
		}
		return true;
	}
}
#endif
