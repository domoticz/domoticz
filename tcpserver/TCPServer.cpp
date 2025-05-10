#include "stdafx.h"
#include <iostream>
#include "TCPServer.h"
#include "TCPClient.h"
#include "../main/RFXNames.h"
#include "../main/RFXtrx.h"
#include "../main/Helper.h"
#include "../main/json_helper.h"
#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../hardware/DomoticzTCP.h"
#include "../main/mainworker.h"
#include <boost/asio.hpp>
#include <algorithm>

namespace tcp {
	namespace server {

		CTCPServerInt::CTCPServerInt(const std::string& address, const std::string& port, CTCPServer* pRoot) :
			CTCPServerIntBase(pRoot),
			io_context_(),
			acceptor_(io_context_)
		{
			// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
			boost::asio::ip::tcp::resolver resolver(io_context_);
			boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::results_type endpoints = resolver.resolve(address, port);
			auto endpoint = *endpoints.begin();
			acceptor_.open(endpoint.endpoint().protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			// bind to both ipv6 and ipv4 sockets for the "::" address only
			if (address == "::")
			{
				acceptor_.set_option(boost::asio::ip::v6_only(false));
			}
			acceptor_.bind(endpoint);
			acceptor_.listen();

			new_connection_ = std::make_shared<CTCPClient>(io_context_, this);
			if (new_connection_ == nullptr)
			{
				_log.Log(LOG_ERROR, "CTCPServerInt: Error creating new client!");
				return;
			}

			acceptor_.async_accept(*(new_connection_->socket()), [this](auto&& err) { handleAccept(err); });
		}

		void CTCPServerInt::start()
		{
			// The io_context::run() call will block until all asynchronous operations
			// have finished. While the server is running, there is always at least one
			// asynchronous operation outstanding: the asynchronous accept call waiting
			// for new incoming connections.
			io_context_.run();
		}

		void CTCPServerInt::stop()
		{
			// Post a call to the stop function so that server::stop() is safe to call
			// from any thread.
			flghandle_stop_Completed=false;
			boost::asio::post([this] { handle_stop(); });
		}

		void CTCPServerInt::handle_stop()
		{
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_context::run() call
			// will exit.
			acceptor_.close();
			stopAllClients();
			flghandle_stop_Completed=true;
		}

		void CTCPServerInt::handleAccept(const boost::system::error_code& error)
		{
			if (error)
				return;
			std::lock_guard<std::mutex> l(connectionMutex);
			std::string s = new_connection_->socket()->remote_endpoint().address().to_string();

			if (s.substr(0, 7) == "::ffff:") {
				s = s.substr(7);
			}

			new_connection_->m_endpoint = s;

			_log.Log(LOG_STATUS, "Incoming Domoticz connection from: %s", s.c_str());

			connections_.insert(new_connection_);
			new_connection_->start();

			new_connection_.reset(new CTCPClient(io_context_, this));

			acceptor_.async_accept(*(new_connection_->socket()), [this](auto&& err) { handleAccept(err); });
		}

		void CTCPServerInt::stopClient(CTCPClient_ptr c)
		{
			std::lock_guard<std::mutex> l(connectionMutex);
			connections_.erase(c);
			c->stop();
		}

		CTCPServerIntBase::CTCPServerIntBase(CTCPServer* pRoot)
		{
			m_pRoot = pRoot;
		}

		_tRemoteShareUser* CTCPServerIntBase::FindUser(const std::string& username)
		{
			int ii = 0;
			for (const auto& user : m_users)
			{
				if (user.Username == username)
					return &m_users[ii];
				ii++;
			}
			return nullptr;
		}

		bool CTCPServerIntBase::HandleAuthentication(const CTCPClient_ptr& c, const std::string& username, const std::string& password)
		{
			_tRemoteShareUser* pUser = FindUser(username);
			if (pUser == nullptr)
				return false;

			return (pUser->Password == password);
		}

		void CTCPServerIntBase::DoDecodeMessage(const CTCPClientBase* pClient, const uint8_t* pData, size_t len)
		{
			m_pRoot->DoDecodeMessage(pClient, pData, len);
		}

		void CTCPServerIntBase::stopAllClients()
		{
			std::lock_guard<std::mutex> l(connectionMutex);
			if (connections_.empty())
				return;
			for (const auto& c : connections_)
			{
				CTCPClientBase* pClient = c.get();
				if (pClient)
					pClient->stop();
			}
			connections_.clear();
		}

		std::vector<_tRemoteShareUser> CTCPServerIntBase::GetRemoteUsers()
		{
			return m_users;
		}

		void CTCPServerIntBase::SetRemoteUsers(const std::vector<_tRemoteShareUser>& users)
		{
			std::lock_guard<std::mutex> l(connectionMutex);
			m_users = users;
		}

		unsigned int CTCPServerIntBase::GetUserDevicesCount(const std::string& username)
		{
			_tRemoteShareUser* pUser = FindUser(username);
			if (pUser == nullptr)
				return 0;
			return (unsigned int)pUser->Devices.size();
		}

		std::string CTCPServerIntBase::AssambleDeviceInfo(int HardwareID, uint64_t DeviceRowID)
		{
			auto result = m_sql.safe_query("SELECT [DeviceID],[Unit],[Name],[Type],[SubType],[SwitchType],[SignalLevel],[BatteryLevel],"
				"[nValue],[sValue],[LastUpdate],[LastLevel],[Options],[Color] FROM DeviceStatus WHERE (HardwareID==%d) AND (ID == %d)",
				HardwareID, DeviceRowID);
			if (result.empty())
				return ""; //that's odd!

			Json::Value root;
			root["OrgHardwareID"] = HardwareID;
			root["OrgDeviceRowID"] = DeviceRowID;

			int iIndex = 0;
			root["DeviceID"] = result[0][iIndex++];
			root["Unit"] = atoi(result[0][iIndex++].c_str());
			root["Name"] = result[0][iIndex++];
			root["Type"] = atoi(result[0][iIndex++].c_str());
			root["SubType"] = atoi(result[0][iIndex++].c_str());
			root["SwitchType"] = atoi(result[0][iIndex++].c_str());
			root["SignalLevel"] = atoi(result[0][iIndex++].c_str());
			root["BatteryLevel"] = atoi(result[0][iIndex++].c_str());
			root["nValue"] = atoi(result[0][iIndex++].c_str());
			root["sValue"] = result[0][iIndex++];
			root["LastUpdate"] = result[0][iIndex++];
			root["LastLevel"] = atoi(result[0][iIndex++].c_str());
			root["Options"] = result[0][iIndex++];
			root["Color"] = result[0][iIndex++];

			return JSonToRawString(root);
		}

		void CTCPServerIntBase::SendToClient(CTCPClientBase* pClient, std::string& szData)
		{
			//encrypt the payload with the users password
			_tRemoteShareUser* pUser = FindUser(pClient->m_username);
			if (pUser == nullptr)
				return;

			std::vector<char> uhash = HexToBytes(pUser->Password);
			std::string szEncrypted;
			AESEncryptData(szData, szEncrypted, (const uint8_t*)uhash.data());
			pClient->write(szEncrypted.c_str(), szEncrypted.size());
		}

		void CTCPServerIntBase::SendToAll(const int HardwareID, const uint64_t DeviceRowID, const CTCPClientBase* pClient2Ignore)
		{
			std::lock_guard<std::mutex> l(connectionMutex);
			//if (connections_.empty())
				//return;

			std::string szSend = AssambleDeviceInfo(HardwareID, DeviceRowID);

			for (const auto& c : connections_)
			{
				CTCPClientBase* pClient = c.get();
				if (pClient == nullptr)
					continue;
				if (pClient == pClient2Ignore)
					continue;
				if (pClient->m_bIsLoggedIn == false)
					continue;

				_tRemoteShareUser* pUser = FindUser(pClient->m_username);
				if (pUser == nullptr)
					continue;

				//check if we are allowed to get this device
				bool bOk2Send = false;
				if (pUser->Devices.empty())
					bOk2Send = true;
				else
					bOk2Send = std::any_of(pUser->Devices.begin(), pUser->Devices.end(), [DeviceRowID](uint64_t d) { return d == DeviceRowID; });

				if (!bOk2Send)
					continue;

				SendToClient(pClient, szSend);
			}
		}

		//Out main (wrapper) server
		CTCPServer::CTCPServer()
		{
		}

		CTCPServer::CTCPServer(const int /*ID*/)
		{
		}

		CTCPServer::~CTCPServer()
		{
			StopServer();
		}

		bool CTCPServer::StartServer(const std::string& address, const std::string& port)
		{
			int tries = 0;
			bool exception = false;
			std::string listen_address = address;

			do {
				try
				{
					exception = false;
					StopServer();
					m_TCPServer = std::make_shared<CTCPServerInt>(listen_address, port, this);
				}
				catch (std::exception& e)
				{
					exception = true;
					switch (tries) {
					case 0:
						listen_address = "::";
						break;
					case 1:
						listen_address = "0.0.0.0";
						break;
					case 2:
						_log.Log(LOG_ERROR, "Exception starting shared server: %s", e.what());
						return false;
					}
					tries++;
				}
			} while (exception);
			_log.Log(LOG_NORM, "Starting shared server on: %s:%s", listen_address.c_str(), port.c_str());
			//Start worker thread
			m_thread = std::make_shared<std::thread>([this] { Do_Work(); });
			SetThreadName(m_thread->native_handle(), "TCPServer");
			return (m_thread != nullptr);
		}

		void CTCPServer::StopServer()
		{
			std::lock_guard<std::mutex> l(m_server_mutex);
			if (m_TCPServer) {
				m_TCPServer->stop();
			}
			if (m_thread)
			{
				m_thread->join();
				m_thread.reset();
			}
			if (m_TCPServer) {
				// wait until completion of handle_stop with a 5 seconds timeout
				int i=0;
				for (; !m_TCPServer->flghandle_stop_Completed && i<50; i++)
					sleep_milliseconds(100);
				if (!m_TCPServer->flghandle_stop_Completed)
					_log.Log(LOG_ERROR, "TCPServer: shared server handle_stop not completed");
				else
					_log.Log(LOG_STATUS, "TCPServer: shared server handle_stop completed after %d milliseconds", i*100);
				// This is the only time to delete it
				m_TCPServer.reset();
				_log.Log(LOG_STATUS, "TCPServer: shared server stopped");
			}
		}

		void CTCPServer::Do_Work()
		{
			if (m_TCPServer) {
				_log.Log(LOG_STATUS, "TCPServer: shared server started...");
				m_TCPServer->start();
			}
		}

		void CTCPServer::SendToAll(const int HardwareID, const uint64_t DeviceRowID, const CTCPClientBase* pClient2Ignore)
		{
			std::lock_guard<std::mutex> l(m_server_mutex);
			if (m_TCPServer)
				m_TCPServer->SendToAll(HardwareID, DeviceRowID, pClient2Ignore);
		}

		void CTCPServer::SetRemoteUsers(const std::vector<_tRemoteShareUser>& users)
		{
			std::lock_guard<std::mutex> l(m_server_mutex);
			if (m_TCPServer)
				m_TCPServer->SetRemoteUsers(users);
		}

		unsigned int CTCPServer::GetUserDevicesCount(const std::string& username)
		{
			std::lock_guard<std::mutex> l(m_server_mutex);
			if (m_TCPServer) {
				return m_TCPServer->GetUserDevicesCount(username);
			}
			return 0;
		}

		void CTCPServer::stopAllClients()
		{
			if (m_TCPServer)
				m_TCPServer->stopAllClients();
		}

		void CTCPServer::DoDecodeMessage(const CTCPClientBase* pClient, const uint8_t* pData, size_t len)
		{
			std::string szEncoded = std::string((const char*)pData, len);
			std::string szDecoded;

			_tRemoteShareUser* pUser = m_TCPServer->FindUser(pClient->m_username);
			if (pUser == nullptr)
				return;

			std::vector<char> uhash = HexToBytes(pUser->Password);

			AESDecryptData(szEncoded, szDecoded, (const uint8_t*)uhash.data());

			Json::Value root;

			bool ret = ParseJSon(szDecoded, root);
			if ((!ret) || (!root.isObject()))
			{
				Log(LOG_ERROR, "Invalid data received!");
				return;
			}

			if (root["action"].empty() == true)
			{
				Log(LOG_ERROR, "Invalid data received, or no data returned!");
				return;
			}

			std::string szAction = root["action"].asString();
			int HardwareID = root["HardwareID"].asInt();
			std::string DeviceID = root["DeviceID"].asString();
			int Unit = root["Unit"].asInt();
			int Type = root["Type"].asInt();
			int SubType = root["SubType"].asInt();

			auto result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q') AND (Unit==%d) AND (Type==%d) AND (SubType==%d)",	HardwareID, DeviceID.c_str(), Unit, Type, SubType);
			if (result.empty())
			{
				Log(LOG_ERROR, "Remotely send Device not found!");
				return;
			}

			std::string szIdx = result[0][0];
			uint64_t idx = std::stoull(szIdx);

			if (szAction == "SwitchLight")
			{
				std::string switchcmd = root["switchcmd"].asString();
				int level = root["level"].asInt();
				_tColor color;
				color.fromJSON(root["color"]);
				bool ooc = root["ooc"].asBool();
				std::string User = root["User"].asString();

				m_mainworker.SwitchLight(idx, switchcmd, level, color, ooc, 0, User);
			}
			else if (szAction == "SetSetpoint")
			{
				float TempValue = root["TempValue"].asFloat();
				m_mainworker.SetSetPoint(szIdx, TempValue);
			}
			else if (szAction == "SetSetPointEvo")
			{
				float TempValue = root["TempValue"].asFloat();
				std::string newMode = root["newMode"].asString();
				std::string until = root["until"].asString();
				m_mainworker.SetSetPointEvo(szIdx, TempValue, newMode, until);
			}
			else if (szAction == "SetThermostatState")
			{
				int newState = root["newState"].asInt();
				m_mainworker.SetThermostatState(szIdx, newState);

			}
			else if (szAction == "SwitchEvoModal")
			{
				std::string status = root["status"].asString();
				std::string action = root["evo_action"].asString();
				std::string ooc = root["ooc"].asString();
				std::string until = root["until"].asString();

				m_mainworker.SwitchEvoModal(szIdx, status, action, ooc, until);
			}
			else if (szAction == "SetTextDevice")
			{
				std::string text = root["text"].asString();
				m_mainworker.SetTextDevice(szIdx, text);
			}
#ifdef WITH_OPENZWAVE
			else if (szAction == "SetZWaveThermostatMode")
			{
				int tMode = root["tMode"].asInt();
				m_mainworker.SetZWaveThermostatMode(szIdx, tMode);
			}
			else if (szAction == "SetZWaveThermostatFanMode")
			{
				int fMode = root["fMode"].asInt();
				m_mainworker.SetZWaveThermostatFanMode(szIdx, fMode);
			}
#endif
			else
			{
				Log(LOG_ERROR, "Unhandled action received: %s", szAction.c_str());
				return;
			}
		}

	} // namespace server
} // namespace tcp
