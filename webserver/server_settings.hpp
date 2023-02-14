/*
 * server_settings.hpp
 *
 *  Created on: 15 févr. 2016
 *      Author: gaudryc
 */
#pragma once
#ifndef WEBSERVER_SERVER_SETTINGS_HPP_
#define WEBSERVER_SERVER_SETTINGS_HPP_

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/algorithm/string.hpp>

namespace http {
namespace server {

struct server_settings {
public:
  server_settings() = default;
  server_settings(const server_settings &s) = default;
  virtual ~server_settings() = default;
  server_settings &operator=(const server_settings &s) = default;
  bool is_secure() const
  {
	  return is_secure_;
  }
	bool is_enabled() const {
		return ((listening_port != "0") && (!listening_port.empty()));
	}
	bool is_php_enabled() const {
		return !php_cgi_path.empty();
	}
	/**
	 * Set relevant values
	 */
	virtual void set(const server_settings & settings) {
		www_root = get_valid_value(listening_address, settings.www_root);
		listening_address = get_valid_value(listening_address, settings.listening_address);
		listening_port = get_valid_value(listening_port, settings.listening_port);
		vhostname = get_valid_value(vhostname, settings.vhostname);
		php_cgi_path = get_valid_value(php_cgi_path, settings.php_cgi_path);
		if (listening_port == "0") {
			listening_port.clear();// server NOT enabled
		}
	}

	virtual std::string to_string() const {
		return std::string("'server_settings[is_secure_=") + (is_secure_ == true ? "true" : "false") +
			", www_root='" + www_root + "'" +
			", listening_address='" + listening_address + "'" +
			", listening_port='" + listening_port + "'" +
			", vhostname='" + vhostname + "'" +
			", php_cgi_path='" + php_cgi_path + "'" +
			"]'";
	}

protected:
	explicit server_settings(bool is_secure) :
		is_secure_(is_secure) {}
	std::string get_valid_value(const std::string & old_value, const std::string & new_value) {
		if ((!new_value.empty()) && (new_value != old_value))
		{
			return new_value;
		}
		return old_value;
	}
public:
	std::string www_root;
	std::string vhostname;
	std::string listening_address;
	std::string listening_port;

	std::string php_cgi_path; //if not empty, php files are handled
	//feature
	//std::string fastcgi_php_server; (like nginx)
private:
  bool is_secure_{ false };
};

#ifdef WWW_ENABLE_SSL

struct ssl_server_settings : public server_settings {
public:
	std::string ssl_method;
	std::string certificate_chain_file_path;
	std::string ca_cert_file_path;
	std::string cert_file_path;

	std::string private_key_file_path;
	std::string private_key_pass_phrase;

	std::string ssl_options;
	std::string tmp_dh_file_path;

	bool verify_peer{ false };
	bool verify_fail_if_no_peer_cert{ false };
	std::string verify_file_path;
	std::string cipher_list;

	ssl_server_settings()
		: server_settings(true)
	{
	}
	ssl_server_settings(const ssl_server_settings &s) = default;
	~ssl_server_settings() override = default;
	ssl_server_settings &operator=(const ssl_server_settings &s) = default;

	boost::asio::ssl::context::method get_ssl_method() const {
		boost::asio::ssl::context::method method;
		if (ssl_method == "tlsv1")
		{
			method = boost::asio::ssl::context::tlsv1;
		}
		else if (ssl_method == "tlsv1_server")
		{
			method = boost::asio::ssl::context::tlsv1_server;
		}
		else if (ssl_method == "sslv23")
		{
			method = boost::asio::ssl::context::sslv23;
		}
		else if (ssl_method == "sslv23_server")
		{
			method = boost::asio::ssl::context::sslv23_server;
		}
		else if (ssl_method == "tlsv11")
		{
			method = boost::asio::ssl::context::tlsv11;
		}
		else if (ssl_method == "tlsv11_server")
		{
			method = boost::asio::ssl::context::tlsv11_server;
		}
		else if (ssl_method == "tlsv12")
		{
			method = boost::asio::ssl::context::tlsv12;
		}
		else if (ssl_method == "tlsv12_server")
		{
			method = boost::asio::ssl::context::tlsv12_server;
		}
		else if (ssl_method == "tlsv13")
		{
			method = boost::asio::ssl::context::tlsv13;
		}
		else if (ssl_method == "tlsv13_server")
		{
			method = boost::asio::ssl::context::tlsv13_server;
		}
		else if (ssl_method == "tls")
		{
			method = boost::asio::ssl::context::tls;
		}
		else if (ssl_method == "tls_server")
		{
			method = boost::asio::ssl::context::tls_server;
		}
		else
		{
			std::string error_message("invalid SSL method ");
			error_message.append("'").append(ssl_method).append("'");
			throw std::invalid_argument(error_message);
		}
		return method;
	}

	boost::asio::ssl::context::options get_ssl_options() const {
		boost::asio::ssl::context::options opts(0x0L);

		std::string error_message;

		std::vector<std::string> options_array;
		boost::split(options_array, ssl_options, boost::is_any_of(","), boost::token_compress_on);
		for (const auto &option : options_array)
		{
			if (option == "default_workarounds")
			{
				update_options(opts, boost::asio::ssl::context::default_workarounds);
			}
			else if (option == "single_dh_use")
			{
				update_options(opts, boost::asio::ssl::context::single_dh_use);
			}
			else if (option == "no_sslv2")
			{
				update_options(opts, boost::asio::ssl::context::no_sslv2);
			}
			else if (option == "no_sslv3")
			{
				update_options(opts, boost::asio::ssl::context::no_sslv3);
			}
			else if (option == "no_tlsv1")
			{
				update_options(opts, boost::asio::ssl::context::no_tlsv1);
			}
			else if (option == "no_tlsv1_1")
			{
				update_options(opts, boost::asio::ssl::context::no_tlsv1_1);
			}
			else if (option == "no_tlsv1_2")
			{
				update_options(opts, boost::asio::ssl::context::no_tlsv1_2);
			}
			else if (option == "no_compression")
			{
				update_options(opts, boost::asio::ssl::context::no_compression);
			}
			else
			{
				if (error_message.empty()) {
					error_message.append("unknown SSL option(s) : ");
				}
				if (error_message.find('\'') != std::string::npos)
				{
					error_message.append(", ");
				}
				error_message.append("'").append(option).append("'");
			}
		}
		if (!error_message.empty()) {
			throw std::invalid_argument(error_message);
		}
		return opts;
	}

	/**
	 * Set relevant values
	 */
	using http::server::server_settings::set;
	virtual void set(const ssl_server_settings & ssl_settings) {
		server_settings::set(ssl_settings);

		ssl_method = server_settings::get_valid_value(ssl_method, ssl_settings.ssl_method);

		std::string path = server_settings::get_valid_value(cert_file_path, ssl_settings.cert_file_path);
		bool update_cert = path == ssl_settings.cert_file_path;
		if (update_cert) {
			cert_file_path = ssl_settings.cert_file_path;
			// use certificate file for all usage by default
			certificate_chain_file_path = ssl_settings.cert_file_path;
			ca_cert_file_path = ssl_settings.cert_file_path;
			private_key_file_path = ssl_settings.private_key_file_path;
			tmp_dh_file_path = ssl_settings.cert_file_path;
			verify_file_path = ssl_settings.cert_file_path;
		}

		certificate_chain_file_path = server_settings::get_valid_value(certificate_chain_file_path, ssl_settings.certificate_chain_file_path);
		ca_cert_file_path = server_settings::get_valid_value(ca_cert_file_path, ssl_settings.ca_cert_file_path);
		private_key_file_path = server_settings::get_valid_value(private_key_file_path, ssl_settings.private_key_file_path);
		private_key_pass_phrase = server_settings::get_valid_value(private_key_pass_phrase, ssl_settings.private_key_pass_phrase);

		ssl_options = server_settings::get_valid_value(ssl_options, ssl_settings.ssl_options);
		tmp_dh_file_path = server_settings::get_valid_value(tmp_dh_file_path, ssl_settings.tmp_dh_file_path);

		verify_peer = ssl_settings.verify_peer;
		verify_fail_if_no_peer_cert = ssl_settings.verify_fail_if_no_peer_cert;
		verify_file_path = server_settings::get_valid_value(verify_file_path, ssl_settings.verify_file_path);
	}

	std::string to_string() const override
	{
		return std::string("ssl_server_settings[") + server_settings::to_string() +
				", ssl_method='" + ssl_method + "'" +
				", certificate_chain_file_path='" + certificate_chain_file_path + "'" +
				", ca_cert_file_path='" + ca_cert_file_path + "'" +
				", cert_file_path=" + cert_file_path + "'" +
				", private_key_file_path='" + private_key_file_path + "'" +
				", private_key_pass_phrase='" + private_key_pass_phrase + "'" +
				", ssl_options='" + ssl_options + "'" +
				", tmp_dh_file_path='" + tmp_dh_file_path + "'" +
				", verify_peer=" + (verify_peer == true ? "true" : "false") +
				", verify_fail_if_no_peer_cert=" + (verify_fail_if_no_peer_cert == true ? "true" : "false") +
				", verify_file_path='" + verify_file_path + "'" +
				"]";
	}

protected:
	void update_options(boost::asio::ssl::context::options & opts, boost::asio::ssl::context::options option) const {
		if (opts != 0x0L) {
			opts |= option;
		} else {
			opts = option;
		}
	}
};

#endif //#ifdef WWW_ENABLE_SSL

} // namespace server
} // namespace http

#endif /* WEBSERVER_SERVER_SETTINGS_HPP_ */
