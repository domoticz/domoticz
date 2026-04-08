#include "stdafx.h"
#include "TCPClient.h"
#include "TCPServer.h"
#include "../hardware/DomoticzTCP.h"
#include "../main/Helper.h"
#include "../main/Logger.h"

namespace tcp {
	namespace server {

		CTCPClientBase::CTCPClientBase(CTCPServerIntBase* pManager)
			: pConnectionManager(pManager)
		{
			socket_ = nullptr;
			m_bIsLoggedIn = false;
		}

		CTCPClientBase::~CTCPClientBase()
		{
			delete socket_;
		}

		CTCPClient::CTCPClient(boost::asio::io_context& ios, CTCPServerIntBase* pManager)
			: CTCPClientBase(pManager)
		{
			socket_ = new boost::asio::ip::tcp::socket(ios);
		}

		void CTCPClient::start()
		{
			socket_->async_read_some(boost::asio::buffer(buffer_), [self = shared_from_this()](auto&& err, auto&& bytes) { self->handleRead(err, bytes); });
		}

		void CTCPClient::stop()
		{
			socket_->close();
		}

		void CTCPClient::handleRead(const boost::system::error_code& e,
			std::size_t bytes_transferred)
		{
			auto self = shared_from_this();
			if (!e)
			{
				m_recvBuffer.append(buffer_.data(), bytes_transferred);

				// Guard against unbounded growth from a stuck incomplete frame
				if (m_recvBuffer.size() > 4096)
				{
					_log.Log(LOG_ERROR, "Receive buffer overflow from %s, resetting connection", m_endpoint.c_str());
					m_recvBuffer.clear();
					pConnectionManager->stopClient(self);
					return;
				}

				// Detect old unframed SIGNv* clients (version < REMOTE_PROTOCOL_VERSION)
				if (!m_bIsLoggedIn && m_recvBuffer.size() >= 6 && m_recvBuffer.substr(0, 5) == "SIGNv")
				{
					int remoteVer = atoi(m_recvBuffer.substr(5).c_str());
					_log.Log(LOG_ERROR, "Remote Domoticz at %s uses legacy protocol (SIGNv%d, expected SIGNv%d). Please update the remote Domoticz instance.",
						m_endpoint.c_str(), remoteVer, REMOTE_PROTOCOL_VERSION);
					m_recvBuffer.clear();
					pConnectionManager->stopClient(self);
					return;
				}

				while (m_recvBuffer.size() >= 4)
				{
					uint32_t msgLen;
					memcpy(&msgLen, m_recvBuffer.data(), 4);
					msgLen = ntohl(msgLen);

					if (msgLen == 0 || msgLen > 1048576)
					{
						m_recvBuffer.clear();
						break;
					}

					if (m_recvBuffer.size() < 4 + (size_t)msgLen)
						break; // wait for more data

					std::string payload(m_recvBuffer.data() + 4, msgLen);
					m_recvBuffer.erase(0, 4 + msgLen);

					if (!m_bIsLoggedIn)
					{
						// Authentication message: "SIGNv{REMOTE_PROTOCOL_VERSION};username;password"
						if (payload.find(std_format("SIGNv%d", REMOTE_PROTOCOL_VERSION)) == 0)
						{
							std::vector<std::string> strarray;
							StringSplit(payload, ";", strarray);
							if (strarray.size() == 3)
							{
								m_bIsLoggedIn = pConnectionManager->HandleAuthentication(self, strarray[1], strarray[2]);
								if (!m_bIsLoggedIn)
								{
									boost::asio::async_write(*socket_, boost::asio::buffer("NOAUTH", 6), [self](auto&& err, auto) { self->handleWrite(err); });
									pConnectionManager->stopClient(self);
									return;
								}
								m_username = strarray[1];
								_log.Log(LOG_STATUS, "Authentication succeeded for user %s on %s", m_username.c_str(), m_endpoint.c_str());
							}
						}
					}
					else
					{
						pConnectionManager->DoDecodeMessage(this, (const uint8_t*)payload.data(), payload.size());
					}
				}

				socket_->async_read_some(boost::asio::buffer(buffer_), [self](auto&& err, auto bytes) { self->handleRead(err, bytes); });
			}
			else if (e != boost::asio::error::operation_aborted)
			{
				pConnectionManager->stopClient(self);
			}
		}

		void CTCPClient::write(const char* pData, size_t Length)
		{
			if (!m_bIsLoggedIn)
				return;
			boost::asio::async_write(*socket_, boost::asio::buffer(pData, Length), [self = shared_from_this()](auto&& err, auto) { self->handleWrite(err); });
		}

		void CTCPClient::handleWrite(const boost::system::error_code& error)
		{
			if (error)
			{
				pConnectionManager->stopClient(shared_from_this());
			}
		}

	} // namespace server
} // namespace tcp
