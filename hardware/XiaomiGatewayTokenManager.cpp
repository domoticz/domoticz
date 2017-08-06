#include "stdafx.h"
#include "XiaomiGatewayTokenManager.h"


XiaomiGatewayTokenManager& XiaomiGatewayTokenManager::GetInstance()
{
	// TODO: insert return statement here
	static XiaomiGatewayTokenManager instance;
	return instance;
}

 void XiaomiGatewayTokenManager::UpdateTokenSID(const std::string & ip, const std::string & token, const std::string & sid)
{
	bool found = false;
	/*for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (std::get<0>(m_GatewayTokens[i]) == ip) {
			std::get<1>(m_GatewayTokens[i]) = token;
			std::get<2>(m_GatewayTokens[i]) = sid;
			found = true;
		}
	}*/

	for (std::tuple<std::string, std::string, std::string> &tup : m_GatewayTokens) {
		if (std::get<0>(tup) == ip) {
			std::get<1>(tup) = token;
			std::get<2>(tup) = sid;
			found = true;
		}
	}

	if (!found) {
		m_GatewayTokens.push_back(std::make_tuple(ip, token, sid));
		//m_GatewayTokens.push_back(std::make_pair(ip, token));
	}
}

std::string XiaomiGatewayTokenManager::GetToken(const std::string & ip)
{
	std::string token = "";
	bool found = false;
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (std::get<0>(m_GatewayTokens[i]) == ip) {
			token = std::get<1>(m_GatewayTokens[i]);
		}
	}
	return token;
}

std::string XiaomiGatewayTokenManager::GetSID(const std::string & ip)
{
	std::string sid = "";
	bool found = false;
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (std::get<0>(m_GatewayTokens[i]) == ip) {
			sid = std::get<2>(m_GatewayTokens[i]);
		}
	}
	return sid;
}
