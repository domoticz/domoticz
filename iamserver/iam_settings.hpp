/*
 * iam_settings.hpp
 *
 *  Created on: 16 Januari 2023
 *      Author: kiddigital
 */
#pragma once
#ifndef IAMSERVER_SERVER_SETTINGS_HPP_
#define IAMSERVER_SERVER_SETTINGS_HPP_

#define OAUTH2_AUTH_URL "/oauth2/v1/authorize"
#define OAUTH2_TOKEN_URL "/oauth2/v1/token"
#define OAUTH2_DISCOVERY_URL "/.well-known/openid-configuration"
#define OAUTH2_USERIDX_OFFSET 20000
#define OAUTH2_AUTHCODE_EXPIRETIME 60
#define OAUTH2_AUTHTOKEN_EXPIRETIME 3600
#define OAUTH2_REFRESHTOKEN_EXPIRETIME 86400

#include <string>

namespace iamserver {

struct iam_settings {
public:
	iam_settings() = default;
	iam_settings(const iam_settings &s) = default;
	virtual ~iam_settings() = default;
	iam_settings &operator=(const iam_settings &s) = default;
	bool is_enabled() const {
		return is_enabled_;
	}
	uint32_t getUserIdxOffset() const {
		return useridx_offset;
	}
	std::string getAuthPageContent() const {
		std::string sAuthPage(auth_content);
		return sAuthPage;
	}
	/**
	 * Set relevant values
	 */
	virtual void set(const iam_settings & iamsettings) {
		auth_url = get_valid_value(auth_url, iamsettings.auth_url);
		token_url = get_valid_value(token_url, iamsettings.token_url);
		discovery_url = get_valid_value(discovery_url, iamsettings.discovery_url);
		code_expire_seconds = get_valid_value(code_expire_seconds, iamsettings.code_expire_seconds);
		token_expire_seconds = get_valid_value(token_expire_seconds, iamsettings.token_expire_seconds);
		refresh_expire_seconds = get_valid_value(refresh_expire_seconds, iamsettings.refresh_expire_seconds);
		is_enabled_ = false;
		if (auth_url.compare(OAUTH2_AUTH_URL) == 0 )
			is_enabled_ = true;
	}

	virtual std::string to_string() const {
		return std::string("'iam_settings[is_enabled_=") + (is_enabled_ == true ? "true" : "false") +
			", auth_url='" + auth_url + "'" +
			", token_url='" + token_url + "'" +
			", discovery_url='" + discovery_url + "'" +
			", code_expire_seconds='" + std::to_string(code_expire_seconds) + "'" +
			", token_expire_seconds='" + std::to_string(token_expire_seconds) + "'" +
			", refresh_expire_seconds='" + std::to_string(refresh_expire_seconds) + "'" +
			"]'";
	}

protected:
	explicit iam_settings(bool is_enabled) :
		is_enabled_(is_enabled) {}
	std::string get_valid_value(const std::string & old_value, const std::string & new_value) {
		if ((!new_value.empty()) && (new_value != old_value))
		{
			return new_value;
		}
		return old_value;
	}
	uint32_t get_valid_value(const uint32_t & old_value, const uint32_t & new_value) {
		if ((new_value > 0) && (new_value != old_value))
		{
			return new_value;
		}
		return old_value;
	}
public:
	std::string auth_url = OAUTH2_AUTH_URL;
	std::string token_url = OAUTH2_TOKEN_URL;
	std::string discovery_url = OAUTH2_DISCOVERY_URL;
	uint32_t code_expire_seconds = OAUTH2_AUTHCODE_EXPIRETIME;
	uint32_t token_expire_seconds = OAUTH2_AUTHTOKEN_EXPIRETIME;
	uint32_t refresh_expire_seconds = OAUTH2_REFRESHTOKEN_EXPIRETIME;
private:
	bool is_enabled_{ false };
	uint32_t useridx_offset = OAUTH2_USERIDX_OFFSET;
	const char *auth_content =
	#include "views/iam_auth.html"
	;
};

} // namespace iamserver

#endif /* IAMSERVER_SERVER_SETTINGS_HPP_ */
