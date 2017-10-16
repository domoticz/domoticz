#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "PluginMessages.h"
#include "PluginTransports.h"
#include "PythonObjects.h"

#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include "icmp_header.hpp"
#include "ipv4_header.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

#define SSTR( x ) dynamic_cast< std::ostringstream & >(( std::ostringstream() << std::dec << x ) ).str()

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessageBase*>	PluginMessageQueue;
	extern boost::asio::io_service ios;

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
		// If the Python CConecction object reference count ever drops to one the the connection is out of scope so shut it down
		if (!m_bDisconnectQueued && (m_pConnection->ob_refcnt <= 1))
		{
			DisconnectDirective*	DisconnectMessage = new DisconnectDirective(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}
			m_bDisconnectQueued = true;
		}
	}

	bool CPluginTransportTCP::handleConnect()
	{
		try
		{
			if (!m_Socket)
			{
				m_bConnecting = false;
				m_bConnected = false;
				m_Resolver = new boost::asio::ip::tcp::resolver(ios);
				m_Socket = new boost::asio::ip::tcp::socket(ios);

				boost::system::error_code ec;
				boost::asio::ip::tcp::resolver::query query(m_IP, m_Port);
				boost::asio::ip::tcp::resolver::iterator iter = m_Resolver->resolve(query);
				boost::asio::ip::tcp::endpoint endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/example/http/client/async_client.cpp
				//
				m_Resolver->async_resolve(query, boost::bind(&CPluginTransportTCP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
				if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
				{
					ios.reset();
					if (((CConnection*)m_pConnection)->pPlugin->m_bDebug)
						_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		m_bConnecting = true;

		return true;
	}

	void CPluginTransportTCP::handleAsyncResolve(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
			m_Socket->async_connect(endpoint, boost::bind(&CPluginTransportTCP::handleAsyncConnect, this, boost::asio::placeholders::error, ++endpoint_iterator));
		}
		else
		{
			m_bConnecting = false;

			delete m_Resolver;
			m_Resolver = NULL;
			delete m_Socket;
			m_Socket = NULL;

			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, err.value(), err.message());
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	void CPluginTransportTCP::handleAsyncConnect(const boost::system::error_code & err, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		delete m_Resolver;
		m_Resolver = NULL;

		if (!err)
		{
			m_bConnected = true;
			m_tLastSeen = time(0);
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CPluginTransportTCP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
				if (((CConnection*)m_pConnection)->pPlugin->m_bDebug)
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
			}
		}
		else
		{
			delete m_Socket;
			m_Socket = NULL;
			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
		}

		ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, err.value(), err.message());
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);

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
				boost::asio::ip::tcp::socket*	pSocket = new boost::asio::ip::tcp::socket(ios);
				m_Acceptor->async_accept((boost::asio::ip::tcp::socket&)*pSocket, boost::bind(&CPluginTransportTCP::handleAsyncAccept, this, pSocket, boost::asio::placeholders::error));

				if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
				{
					ios.reset();
					if (((CConnection*)m_pConnection)->pPlugin->m_bDebug)
						_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}
		}
		catch (std::exception& e)
		{
			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return true;
	}

	void CPluginTransportTCP::handleAsyncAccept(boost::asio::ip::tcp::socket* pSocket, const boost::system::error_code& err)
	{
		m_tLastSeen = time(0);

		if (!err)
		{
			boost::asio::ip::tcp::endpoint remote_ep = pSocket->remote_endpoint();
			std::string sAddress = remote_ep.address().to_string();
			std::string sPort = SSTR(remote_ep.port());

			PyType_Ready(&CConnectionType);
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
			pConnection->Transport = ((CConnection*)m_pConnection)->Transport;
			Py_INCREF(pConnection->Transport);
			pConnection->Protocol = ((CConnection*)m_pConnection)->Protocol;
			Py_INCREF(pConnection->Protocol);
			pConnection->pPlugin = ((CConnection*)m_pConnection)->pPlugin;

			// Add it to the plugins list of connections
			pConnection->pPlugin->AddConnection(pTcpTransport);

			// Create Protocol object to handle connection's traffic
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				ProtocolDirective*	pMessage = new ProtocolDirective(pConnection->pPlugin, (PyObject*)pConnection);
				PluginMessageQueue.push(pMessage);
				//  and signal connection
				ConnectedMessage*	Message = new ConnectedMessage(pConnection->pPlugin, (PyObject*)pConnection, err.value(), err.message());
				PluginMessageQueue.push(Message);
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

			DisconnectedEvent*	pDisconnectedEvent = new DisconnectedEvent(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(pDisconnectedEvent);
			}
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		if (!e)
		{
			ReadMessage*	Message = new ReadMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, bytes_transferred, m_Buffer);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

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
			if ((e.value() != 2) && 
				(e.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
				(e.value() != 125) &&	// Operation cancelled
				(e != boost::asio::error::operation_aborted) &&
				(e.value() != 1236))	// local disconnect cause by hardware reload
				_log.Log(LOG_ERROR, "(%s): Async Read Exception: %d, %s", ((CConnection*)m_pConnection)->pPlugin->Name.c_str(), e.value(), e.message().c_str());

			DisconnectedEvent*	pDisconnectedEvent = new DisconnectedEvent(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(pDisconnectedEvent);
			}
			m_bDisconnectQueued = true;
		}
	}

	void CPluginTransportTCP::handleWrite(const std::vector<byte>& pMessage)
	{
		if (m_Socket)
		{
			try
			{
				m_Socket->write_some(boost::asio::buffer(pMessage, pMessage.size()));
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "%s: Socket error during 'write_some' operation: %d bytes", __func__, pMessage.size());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "%s: Data not sent to NULL socket.", __func__);
		}
	}

	bool CPluginTransportTCP::handleDisconnect()
	{
		m_tLastSeen = time(0);
		if (m_bConnected)
		{
			m_bConnected = false;

			if (m_Socket)
			{
				boost::system::error_code e;
				m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
				if (e)
				{
					_log.Log(LOG_ERROR, "(%s): Socket Shutdown Error: %d, %s", ((CConnection*)m_pConnection)->pPlugin->Name.c_str(), e.value(), e.message().c_str());
				}
				else
				{
					m_Socket->close();
				}
				delete m_Socket;
				m_Socket = NULL;
			}
		}

		if (m_Acceptor)
		{
			m_Acceptor->cancel();
		}

		if (m_Resolver)
		{
			delete m_Resolver;
		}

		m_bDisconnectQueued = false;

		return true;
	}

	CPluginTransportTCP::~CPluginTransportTCP()
	{
		if (m_Socket)
		{
			handleDisconnect();
			delete m_Socket;
		}
		if (m_Resolver) delete m_Resolver;
		if (m_Acceptor)
		{
			delete m_Acceptor;
		}
	};

	bool CPluginTransportUDP::handleListen()
	{
		try
		{
			if (!m_Socket)
			{
				boost::system::error_code ec;
				m_bConnected = true;
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

			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
				if (((CConnection*)m_pConnection)->pPlugin->m_bDebug)
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
			}
		}
		catch (std::exception& e)
		{
			//	_log.Log(LOG_ERROR, "Plugin: Listen Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return true;
	}

	void CPluginTransportUDP::handleRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
	{
		if (!ec)
		{
			std::string sAddress = m_remote_endpoint.address().to_string();
			std::string sPort = SSTR(m_remote_endpoint.port());

			PyType_Ready(&CConnectionType);
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
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				ProtocolDirective*	pMessage = new ProtocolDirective(pConnection->pPlugin, (PyObject*)pConnection);
				PluginMessageQueue.push(pMessage);
			}

			ReadMessage*	Message = new ReadMessage(((CConnection*)pConnection)->pPlugin, (PyObject*)pConnection, bytes_transferred, m_Buffer);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			m_tLastSeen = time(0);
			m_iTotalBytes += bytes_transferred;

			// Make sure only the only Message objects are refering to Connection so that it is cleaned up right after plugin onMessage
			Py_DECREF(pConnection);

			// Set up listener again
			handleListen();
		}
		else
		{
			if ((ec.value() != 2) &&
				(ec.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
				(ec.value() != 125) &&	// Operation cancelled
				(ec.value() != boost::asio::error::operation_aborted) &&	// Abort due to shutdown during disconnect
				(ec.value() != 1236))	// local disconnect cause by hardware reload
				_log.Log(LOG_ERROR, "(%s): Async Read Exception: %d, %s", ((CConnection*)m_pConnection)->pPlugin->Name.c_str(), ec.value(), ec.message().c_str());

			if (!m_bDisconnectQueued)
			{
				m_bDisconnectQueued = true;
				DisconnectDirective*	DisconnectMessage = new DisconnectDirective(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(DisconnectMessage);
				}
			}
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
			_log.Log(LOG_ERROR, "%s: '%s' during 'send_to' operation: %d bytes", __func__, err.what(), pMessage.size());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Socket error during 'send_to' operation: %d bytes", __func__, pMessage.size());
		}
	}

	bool CPluginTransportUDP::handleDisconnect()
	{
		m_tLastSeen = time(0);
		if (m_bConnected)
		{
			m_bConnected = false;

			if (m_Socket)
			{
				boost::system::error_code e;
				m_Socket->shutdown(boost::asio::ip::udp::socket::shutdown_both, e);
				m_Socket->close();
				delete m_Socket;
				m_Socket = NULL;
			}
		}
		return true;
	}

	CPluginTransportUDP::~CPluginTransportUDP()
	{
		if (m_Socket)
		{
			handleDisconnect();
			delete m_Socket;
		}
		if (m_Resolver) delete m_Resolver;
	};

	void CPluginTransportICMP::handleAsyncResolve(const boost::system::error_code &ec, boost::asio::ip::icmp::resolver::iterator endpoint_iterator)
	{
		if (!ec)
		{
			//m_bConnected = true;
			m_IP = endpoint_iterator->endpoint().address().to_string();

			// Listen will fail (10022 - bad parameter) unless something has been sent(?)
			std::string body("ping");
			handleWrite(std::vector<byte>(&body[0], &body[body.length()]));

			m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint,
				boost::bind(&CPluginTransportICMP::handleRead, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			DisconnectDirective*	DisconnectMessage = new DisconnectDirective(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}
		}
	}

	bool CPluginTransportICMP::handleListen()
	{
		try
		{
			if (!m_Initialised)
			{
				m_bConnecting = false;
				m_bConnected = false;
				m_Resolver = new boost::asio::ip::icmp::resolver(ios);
				m_Socket = new boost::asio::ip::icmp::socket(ios, boost::asio::ip::icmp::v4());

				boost::system::error_code ec;
				boost::asio::ip::icmp::resolver::query query(boost::asio::ip::icmp::v4(), m_IP, "");
				boost::asio::ip::icmp::resolver::iterator iter = m_Resolver->resolve(query);
				m_Endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/example/icmp/ping.cpp
				//
				m_Resolver->async_resolve(query, boost::bind(&CPluginTransportICMP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));

				m_Initialised = true;
			}
			else
			{
				m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_Endpoint,
					boost::bind(&CPluginTransportICMP::handleRead, this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
			}

			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
				if (((CConnection*)m_pConnection)->pPlugin->m_bDebug)
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
			}
		}
		catch (std::exception& e)
		{
			_log.Log(LOG_ERROR, "%s Exception: '%s' failed connecting to '%s'", __func__, e.what(), m_IP.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return true;
	}

	void CPluginTransportICMP::handleTimeout(const boost::system::error_code& ec)
	{
		if (!ec)  // Timeout, no response
		{
			ReadMessage*	Message = new ReadMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, 0, NULL);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

		}
		else if (ec != boost::asio::error::operation_aborted)  // Timer cancelled by message arriving
		{
			_log.Log(LOG_ERROR, "Plugin: %s: %d, %s", __func__, ec.value(), ec.message().c_str());
		}
	}

	void CPluginTransportICMP::handleRead(const boost::system::error_code & ec, std::size_t bytes_transferred)
	{
		if (!ec)
		{
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

				ReadMessage*	Message = new ReadMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, bytes_transferred, m_Buffer);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}

				m_tLastSeen = time(0);
				m_iTotalBytes += bytes_transferred;
			}

			// Set up listener again
			handleListen();
		}
		else
		{
			if ((ec.value() != 2) &&
				(ec.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
				(ec.value() != 125) &&	// Operation cancelled
				(ec.value() != boost::asio::error::operation_aborted) &&	// Abort due to shutdown during disconnect
				(ec.value() != 1236))	// local disconnect cause by hardware reload
				_log.Log(LOG_ERROR, "(%s): Async Receive From Exception: %d, %s", ((CConnection*)m_pConnection)->pPlugin->Name.c_str(), ec.value(), ec.message().c_str());

			if (!m_bDisconnectQueued)
			{
				m_bDisconnectQueued = true;
				DisconnectDirective*	DisconnectMessage = new DisconnectDirective(((CConnection*)m_pConnection)->pPlugin, m_pConnection);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(DisconnectMessage);
				}
			}
		}
	}

	void CPluginTransportICMP::handleWrite(const std::vector<byte>& pMessage)
	{
		// Check transport is usable
		if (!m_Initialised)
		{
			CConnection*	pConnection = (CConnection*)this->m_pConnection;
			std::string	sConnection = PyUnicode_AsUTF8(pConnection->Name);
			_log.Log(LOG_ERROR, "(%s) Transport not initialised, write directive to '%s' ignored. Connectionless transport should be Listening.", pConnection->pPlugin->Name.c_str(), sConnection.c_str());
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
		echo_request.sequence_number(m_SequenceNo++);
		compute_checksum(echo_request, pMessage.begin(), pMessage.end());

		// Encode the request packet.
		boost::asio::streambuf request_buffer;
		std::ostream os(&request_buffer);
		std::string	 sData(pMessage.begin(), pMessage.end());
		os << echo_request << sData;

		// Send the request
		m_Socket->send_to(request_buffer.data(), m_Endpoint);
	}

	bool CPluginTransportICMP::handleDisconnect()
	{
		m_tLastSeen = time(0);
		if (m_Timer)
		{
			m_Timer->cancel();
			delete m_Timer;
			m_Timer = NULL;
		}

		if (m_Socket)
		{
			boost::system::error_code e;
			m_Socket->shutdown(boost::asio::ip::icmp::socket::shutdown_both, e);
			m_Socket->close();
			delete m_Socket;
			m_Socket = NULL;
		}

		if (m_Resolver)
		{
			delete m_Resolver;
			m_Resolver = NULL;
		}

		return true;
	}

	CPluginTransportICMP::~CPluginTransportICMP()
	{
		if (m_Socket)
		{
			handleDisconnect();
			delete m_Socket;
		}
		if (m_Resolver) delete m_Resolver;
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

				ConnectedMessage*	Message = NULL;
				if (m_bConnected)
				{
					Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, 0, "SerialPort " + m_Port + " opened successfully.");
					setReadCallback(boost::bind(&CPluginTransportSerial::handleRead, this, _1, _2));
				}
				else
				{
					Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, "SerialPort " + m_Port + " open failed, check log for details.");
				}
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}
		catch (std::exception& e)
		{
			ConnectedMessage*	Message = new ConnectedMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return m_bConnected;
	}

	void CPluginTransportSerial::handleRead(const char *data, std::size_t bytes_transferred)
	{
		if (bytes_transferred)
		{
			ReadMessage*	Message = new ReadMessage(((CConnection*)m_pConnection)->pPlugin, m_pConnection, bytes_transferred, (const unsigned char*)data);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

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
		write((const char *)&data[0], data.size());
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