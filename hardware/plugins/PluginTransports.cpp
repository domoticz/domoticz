#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef USE_PYTHON_PLUGINS

#include "PluginTransports.h"
#include "PluginMessages.h"

#include "../main/Logger.h"
#include <queue>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessage>	PluginMessageQueue;
	extern boost::asio::io_service ios;

	void CPluginTransport::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
	}

	void CPluginTransport::handleRead(const char *data, std::size_t bytes_transferred)
	{
		_log.Log(LOG_ERROR, "CPluginTransport: Base handleRead invoked for Hardware %d", m_HwdID);
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
			CPluginMessage	Message(PMT_Connected, m_HwdID, std::string(e.what()));
			Message.m_iValue = -1;
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
			CPluginMessage	Message(PMT_Connected, m_HwdID, err.message());
			Message.m_iValue = err.value();
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

		ConnectedMessage	Message(m_HwdID);
		Message.m_Message = err.message();
		Message.m_iValue = err.value();
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);
	}

	void CPluginTransportTCP::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
	{
		if (!e)
		{
			CPluginMessage	Message(PMT_Read, m_HwdID, std::string(m_Buffer, bytes_transferred));
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
			if (e.value() != 1236)		// local disconnect cause by hardware reload
			{
				if ((e.value() != 2) && (e.value() != 121))	// Semaphore timeout expiry or end of file aka 'lost contact'
					_log.Log(LOG_ERROR, "Plugin: Async Read Exception: %d, %s", e.value(), e.message().c_str());
			}

			CPluginMessage	DisconnectMessage(PMT_Directive, PDT_Disconnect, m_HwdID);
			{
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(DisconnectMessage);
			}
		}
	}

	void CPluginTransportTCP::handleWrite(const std::string& pMessage)
	{
		if (m_Socket)
		{
			try
			{
				m_Socket->write_some(boost::asio::buffer(pMessage.c_str(), pMessage.length()));
			}
			catch (...)
			{
				_log.Log(LOG_ERROR, "Plugin: Socket error durring 'write_some' operation: '%s'", pMessage.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Plugin: Data not sent to NULL socket: '%s'", pMessage.c_str());
		}
	}

	bool CPluginTransportTCP::handleDisconnect()
	{
		if (m_bConnected)
		{
			if (m_Socket)
			{
				m_Socket->close();
				delete m_Socket;
				m_Socket = NULL;
			}
			m_bConnected = false;
		}
		return true;
	}

	CPluginTransportSerial::CPluginTransportSerial(int HwdID, const std::string & Port, int Baud) : CPluginTransport(HwdID), m_Baud(Baud)
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

				ConnectedMessage	Message(m_HwdID);
				if (m_bConnected)
				{
					Message.m_Message = "SerialPort " + m_Port + " opened successfully.";
					Message.m_iValue = 0;
					setReadCallback(boost::bind(&CPluginTransportSerial::handleRead, this, _1, _2));
				}
				else
				{
					Message.m_Message = "SerialPort " + m_Port + " open failed, check log for details.";
					Message.m_iValue = -1;
				}
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
		}
		catch (std::exception& e)
		{
			CPluginMessage	Message(PMT_Connected, m_HwdID, std::string(e.what()));
			Message.m_iValue = -1;
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
			CPluginMessage	Message(PMT_Read, m_HwdID, std::string(data, bytes_transferred));
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

	void CPluginTransportSerial::handleWrite(const std::string & data)
	{
		write(data.c_str(), data.length());
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