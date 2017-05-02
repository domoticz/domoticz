#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "PluginMessages.h"
#include "PluginTransports.h"

#include "../main/Logger.h"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessage*>	PluginMessageQueue;
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
			DisconnectDirective*	DisconnectMessage = new DisconnectDirective(m_HwdID, m_pConnection);
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
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
			}
		}
		catch (std::exception& e)
		{
			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

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
			delete m_Resolver;
			m_Resolver = NULL;
			delete m_Socket;
			m_Socket = NULL;

			//			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, err.value(), err.message());
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
			m_Socket->async_read_some(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CPluginTransportTCP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
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

		ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, err.value(), err.message());
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		if (!e)
		{
			ReadMessage*	Message = new ReadMessage(m_HwdID, m_pConnection, bytes_transferred, m_Buffer);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

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
				(e.value() != 995) &&	// Abort due to shutdown during disconnect
				(e.value() != 1236))	// local disconnect cause by hardware reload
				_log.Log(LOG_ERROR, "Plugin: Async Read Exception: %d, %s", e.value(), e.message().c_str());

			DisconnectDirective*	DisconnectMessage = new DisconnectDirective(m_HwdID, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
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
		if (m_bConnected)
		{
			if (m_Socket)
			{
				boost::system::error_code e;
				m_Socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
				m_Socket->close();
				delete m_Socket;
				m_Socket = NULL;
			}
			m_bConnected = false;
			m_bDisconnectQueued = false;

		}
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
	};

	bool CPluginTransportUDP::handleConnect()
	{
		try
		{
			if (!m_Socket)
			{
				m_bConnected = false;
				int	iPort = atoi(m_Port.c_str());
				m_Socket = new boost::asio::ip::udp::socket(ios, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), iPort));

				m_Socket->async_receive_from(boost::asio::buffer(m_Buffer, sizeof m_Buffer), m_remote_endpoint,
											 boost::bind(&CPluginTransportUDP::handleRead, this,
													boost::asio::placeholders::error,
													boost::asio::placeholders::bytes_transferred));


/*
				m_Resolver = new boost::asio::ip::udp::resolver(ios);
				m_Socket = new boost::asio::ip::udp::socket(ios);

				boost::system::error_code ec;
				boost::asio::ip::udp::resolver::query query(m_IP, m_Port);
				boost::asio::ip::udp::resolver::iterator iter = m_Resolver->resolve(query);
				boost::asio::ip::udp::endpoint endpoint = *iter;

				//
				//	Async resolve/connect based on http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio/example/http/client/async_client.cpp
				//
				m_Resolver->async_resolve(query, boost::bind(&CPluginTransportUDP::handleAsyncResolve, this, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
				if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
				{
					ios.reset();
					_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
					boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
				}
*/
			}
		}
		catch (std::exception& e)
		{
			//	_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", e.what(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, -1, std::string(e.what()));
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
			return false;
		}

		return true;
	}

	void CPluginTransportUDP::handleAsyncResolve(const boost::system::error_code & err, boost::asio::ip::udp::resolver::iterator endpoint_iterator)
	{
		if (!err)
		{
			boost::asio::ip::udp::endpoint endpoint = *endpoint_iterator;
			m_Socket->async_connect(endpoint, boost::bind(&CPluginTransportUDP::handleAsyncConnect, this, boost::asio::placeholders::error, ++endpoint_iterator));
		}
		else
		{
			delete m_Resolver;
			m_Resolver = NULL;
			delete m_Socket;
			m_Socket = NULL;

			//	_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
			ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, err.value(), err.message());
			boost::lock_guard<boost::mutex> l(PluginMutex);
			PluginMessageQueue.push(Message);
		}
	}

	void CPluginTransportUDP::handleAsyncConnect(const boost::system::error_code & err, boost::asio::ip::udp::resolver::iterator endpoint_iterator)
	{
		delete m_Resolver;
		m_Resolver = NULL;

		if (!err)
		{
			m_bConnected = true;
			m_Socket->async_receive(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
				boost::bind(&CPluginTransportUDP::handleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
			if (ios.stopped())  // make sure that there is a boost thread to service i/o operations
			{
				ios.reset();
				_log.Log(LOG_NORM, "PluginSystem: Starting I/O service thread.");
				boost::thread bt(boost::bind(&boost::asio::io_service::run, &ios));
			}
		}
		else
		{
			delete m_Socket;
			m_Socket = NULL;
			_log.Log(LOG_ERROR, "Plugin: Connection Exception: '%s' connecting to '%s:%s'", err.message().c_str(), m_IP.c_str(), m_Port.c_str());
		}

		ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, err.value(), err.message());
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);
	}

	void CPluginTransportUDP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		if (!e)
		{
			ReadMessage*	Message = new ReadMessage(m_HwdID, m_pConnection, bytes_transferred, m_Buffer);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

			m_iTotalBytes += bytes_transferred;

			//ready for next read
			if (m_Socket)
				m_Socket->async_receive(boost::asio::buffer(m_Buffer, sizeof m_Buffer),
					boost::bind(&CPluginTransportUDP::handleRead,
						this,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
		}
		else
		{
			if ((e.value() != 2) &&
				(e.value() != 121) &&	// Semaphore timeout expiry or end of file aka 'lost contact'
				(e.value() != 125) &&	// Operation cancelled
				(e.value() != 995) &&	// Abort due to shutdown during disconnect
				(e.value() != 1236))	// local disconnect cause by hardware reload
				_log.Log(LOG_ERROR, "Plugin: Async Read Exception: %d, %s", e.value(), e.message().c_str());

			DisconnectDirective*	DisconnectMessage = new DisconnectDirective(m_HwdID, m_pConnection);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}
		}
	}

	void CPluginTransportUDP::handleWrite(const std::vector<byte>& pMessage)
	{
		if (m_Socket)
		{
			try
			{
//				m_Socket->async_send_to(boost::asio::buffer(pMessage, pMessage.size()));
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

	bool CPluginTransportUDP::handleDisconnect()
	{
		if (m_bConnected)
		{
			if (m_Socket)
			{
				boost::system::error_code e;
				m_Socket->shutdown(boost::asio::ip::udp::socket::shutdown_both, e);
				m_Socket->close();
				delete m_Socket;
				m_Socket = NULL;
			}
			m_bConnected = false;
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

				m_bConnected = isOpen();

				ConnectedMessage*	Message = NULL;
				if (m_bConnected)
				{
					Message = new ConnectedMessage(m_HwdID, m_pConnection, 0, "SerialPort " + m_Port + " opened successfully.");
					setReadCallback(boost::bind(&CPluginTransportSerial::handleRead, this, _1, _2));
				}
				else
				{
					Message = new ConnectedMessage(m_HwdID, m_pConnection, -1, "SerialPort " + m_Port + " open failed, check log for details.");
				}
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}
		catch (std::exception& e)
		{
			ConnectedMessage*	Message = new ConnectedMessage(m_HwdID, m_pConnection, -1, std::string(e.what()));
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
			ReadMessage*	Message = new ReadMessage(m_HwdID, m_pConnection, bytes_transferred, (const unsigned char*)data);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}

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