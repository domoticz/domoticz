#pragma once

#include "DomoticzHardware.h"
#include <memory>
#include <string>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/mutex.hpp>

class XiaomiGatewayTokenManager {
public:
	static XiaomiGatewayTokenManager& GetInstance();
	void UpdateTokenSID(const std::string &ip, const std::string &token, const std::string &sid);
	std::string GetToken(const std::string &ip);
	std::string GetSID(const std::string &sid);

	// delete copy and move constructors and assign operators
	XiaomiGatewayTokenManager(XiaomiGatewayTokenManager const&) = delete;             // Copy construct
	XiaomiGatewayTokenManager(XiaomiGatewayTokenManager&&) = delete;                  // Move construct
	XiaomiGatewayTokenManager& operator=(XiaomiGatewayTokenManager const&) = delete;  // Copy assign
	XiaomiGatewayTokenManager& operator=(XiaomiGatewayTokenManager &&) = delete;      // Move assign

protected:
	XiaomiGatewayTokenManager() {}
	~XiaomiGatewayTokenManager() {}

private:
	boost::mutex m_mutex;
	std::vector<boost::tuple<std::string, std::string, std::string> > m_GatewayTokens;
};
