#pragma once

#include "DomoticzHardware.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class XiaomiGatewayTokenManager {
public:
	static XiaomiGatewayTokenManager& GetInstance();
	void UpdateTokenSID(const std::string &ip, const std::string &token, const std::string &sid);
	std::string GetToken(const std::string &ip);
	std::string GetSID(const std::string &sid);

private:
	std::vector<std::tuple<std::string, std::string, std::string>> m_GatewayTokens;
};
