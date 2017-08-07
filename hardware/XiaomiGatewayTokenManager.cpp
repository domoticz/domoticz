#include "stdafx.h"
#include "XiaomiGatewayTokenManager.h"


XiaomiGatewayTokenManager& XiaomiGatewayTokenManager::GetInstance()
{
	static XiaomiGatewayTokenManager instance;
	return instance;
}

 void XiaomiGatewayTokenManager::UpdateTokenSID(const std::string & ip, const std::string & token, const std::string & sid)
{
	bool found = false;
	boost::mutex::scoped_lock lock(m_mutex);
	for (boost::tuple<std::string, std::string, std::string> &tup : m_GatewayTokens) {
		if (boost::get<0>(tup) == ip) {
			boost::get<1>(tup) = token;
			boost::get<2>(tup) = sid;
			found = true;
		}
	}
	if (!found) {
		m_GatewayTokens.push_back(boost::make_tuple(ip, token, sid));
	}

}

std::string XiaomiGatewayTokenManager::GetToken(const std::string & ip)
{
	std::string token = "";
	bool found = false;
	boost::mutex::scoped_lock lock(m_mutex);
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (boost::get<0>(m_GatewayTokens[i]) == ip) {
			token = boost::get<1>(m_GatewayTokens[i]);
		}
	}
	return token;
}

std::string XiaomiGatewayTokenManager::GetSID(const std::string & ip)
{
	std::string sid = "";
	bool found = false;
	boost::mutex::scoped_lock lock(m_mutex);
	for (unsigned i = 0; i < m_GatewayTokens.size(); i++) {
		if (boost::get<0>(m_GatewayTokens[i]) == ip) {
			sid = boost::get<2>(m_GatewayTokens[i]);
		}
	}
	return sid;
}
