#pragma once

/*
based on the work of Roger Light <roger@atchoo.org>
*/

#ifndef MOSQUITTO_HELPER_H
#define MOSQUITTO_HELPER_H

#include <mosquitto.h>

namespace mosqdz {
	const char* strerror(int mosq_errno);
	const char* connack_string(int connack_code);
	int sub_topic_tokenise(const char* subtopic, char*** topics, int* count);
	int sub_topic_tokens_free(char*** topics, int count);
	int lib_version(int* major, int* minor, int* revision);
	int lib_init();
	int lib_cleanup();
	int topic_matches_sub(const char* sub, const char* topic, bool* result);
	int validate_utf8(const char* str, int len);
	int subscribe_simple(struct mosquitto_message **messages, int msg_count, bool retained, const char *topic, int qos = 0, const char *host = "localhost", int port = 1883,
			     const char *client_id = nullptr, int keepalive = 60, bool clean_session = true, const char *username = nullptr, const char *password = nullptr,
			     const struct libmosquitto_will *will = nullptr, const struct libmosquitto_tls *tls = nullptr);

	int subscribe_callback(int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *), void *userdata, const char *topic, int qos = 0, bool retained = true,
			       const char *host = "localhost", int port = 1883, const char *client_id = nullptr, int keepalive = 60, bool clean_session = true, const char *username = nullptr,
			       const char *password = nullptr, const struct libmosquitto_will *will = nullptr, const struct libmosquitto_tls *tls = nullptr);

	/*
	 * Class: mosquittodz
	 *
	 * A mosquitto client class. This is a C++ wrapper class for the mosquitto C
	 * library. Please see mosquitto.h for details of the functions.
	 */
	class mosquittodz {
	public:
		struct mosquitto* m_mosq;
	public:
	  mosquittodz(const char *id = nullptr, bool clean_session = true);
	  virtual ~mosquittodz();

	  int reinitialise(const char *id, bool clean_session);
	  void clear_callbacks();
	  int socket();
	  int will_set(const char *topic, int payloadlen = 0, const void *payload = nullptr, int qos = 0, bool retain = false);
	  int will_clear();
	  int username_pw_set(const char *username, const char *password = nullptr);
	  int connect(const char *host, int port = 1883, int keepalive = 60);
	  int connect_async(const char *host, int port = 1883, int keepalive = 60);
	  int connect(const char *host, int port, int keepalive, const char *bind_address);
	  int connect_async(const char *host, int port, int keepalive, const char *bind_address);
	  int reconnect();
	  int reconnect_async();
	  int disconnect();
	  int publish(int *mid, const char *topic, int payloadlen = 0, const void *payload = nullptr, int qos = 0, bool retain = false);
	  int subscribe(int *mid, const char *sub, int qos = 0);
	  int unsubscribe(int *mid, const char *sub);
	  void reconnect_delay_set(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff);
	  int max_inflight_messages_set(unsigned int max_inflight_messages);
	  void message_retry_set(unsigned int message_retry);
	  void user_data_set(void *userdata);
	  int tls_set(const char *cafile, const char *capath = nullptr, const char *certfile = nullptr, const char *keyfile = nullptr,
		      int (*pw_callback)(char *buf, int size, int rwflag, void *userdata) = nullptr);
	  int tls_opts_set(int cert_reqs, const char *tls_version = nullptr, const char *ciphers = nullptr);
	  int tls_insecure_set(bool value);
	  int tls_psk_set(const char *psk, const char *identity, const char *ciphers = nullptr);
	  int opts_set(enum mosq_opt_t option, void *value);

	  int loop(int timeout = -1, int max_packets = 1);
	  int loop_misc();
	  int loop_read(int max_packets = 1);
	  int loop_write(int max_packets = 1);
	  int loop_forever(int timeout = -1, int max_packets = 1);
	  int loop_start();
	  int loop_stop(bool force = false);
	  bool want_write();
	  int threaded_set(bool threaded = true);
	  int socks5_set(const char *host, int port = 1080, const char *username = nullptr, const char *password = nullptr);

	  // names in the functions commented to prevent unused parameter warning
	  virtual void on_connect(int /*rc*/)
	  {
	  }
	  virtual void on_connect_with_flags(int /*rc*/, int /*flags*/)
	  {
	  }
	  virtual void on_disconnect(int /*rc*/)
	  {
	  }
	  virtual void on_publish(int /*mid*/)
	  {
	  }
	  virtual void on_message(const struct mosquitto_message * /*message*/)
	  {
	  }
	  virtual void on_subscribe(int /*mid*/, int /*qos_count*/, const int * /*granted_qos*/)
	  {
	  }
	  virtual void on_unsubscribe(int /*mid*/)
	  {
	  }
	  virtual void on_log(int /*level*/, const char * /*str*/)
	  {
	  }
	  virtual void on_error()
	  {
	  }
	};
} // namespace mosqdz
#endif

