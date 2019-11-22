/*
Based on the work of Roger Light <roger@atchoo.org>
*/
#include "stdafx.h"
#include "mosquitto_helper.h"

#define UNUSED(A) (void)(A)

namespace mosqdz {

static void on_connect_wrapper(struct mosquitto *mosq, void *userdata, int rc)
{
	class mosquittodz *m = (class mosquittodz *)userdata;

	UNUSED(mosq);

	m->on_connect(rc);
}

static void on_connect_with_flags_wrapper(struct mosquitto *mosq, void *userdata, int rc, int flags)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_connect_with_flags(rc, flags);
}

static void on_disconnect_wrapper(struct mosquitto *mosq, void *userdata, int rc)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_disconnect(rc);
}

static void on_publish_wrapper(struct mosquitto *mosq, void *userdata, int mid)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_publish(mid);
}

static void on_message_wrapper(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_message(message);
}

static void on_subscribe_wrapper(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_subscribe(mid, qos_count, granted_qos);
}

static void on_unsubscribe_wrapper(struct mosquitto *mosq, void *userdata, int mid)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_unsubscribe(mid);
}


static void on_log_wrapper(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	class mosquittodz *m = (class mosquittodz *)userdata;
	UNUSED(mosq);
	m->on_log(level, str);
}

int lib_version(int *major, int *minor, int *revision)
{
	if(major) *major = LIBMOSQUITTO_MAJOR;
	if(minor) *minor = LIBMOSQUITTO_MINOR;
	if(revision) *revision = LIBMOSQUITTO_REVISION;
	return LIBMOSQUITTO_VERSION_NUMBER;
}

int lib_init()
{
	return mosquitto_lib_init();
}

int lib_cleanup()
{
	return mosquitto_lib_cleanup();
}

const char* strerror(int mosq_errno)
{
	return mosquitto_strerror(mosq_errno);
}

const char* connack_string(int connack_code)
{
	return mosquitto_connack_string(connack_code);
}

int sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
	return mosquitto_sub_topic_tokenise(subtopic, topics, count);
}

int sub_topic_tokens_free(char ***topics, int count)
{
	return mosquitto_sub_topic_tokens_free(topics, count);
}

int topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	return mosquitto_topic_matches_sub(sub, topic, result);
}

int validate_utf8(const char *str, int len)
{
	return mosquitto_validate_utf8(str, len);
}

int subscribe_simple(
		struct mosquitto_message **messages,
		int msg_count,
		bool retained,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls)
{
	return mosquitto_subscribe_simple(
			messages, msg_count, retained,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
}

int subscribe_callback(
		int (*callback)(struct mosquitto *, void *, const struct mosquitto_message *),
		void *userdata,
		const char *topic,
		int qos,
		const char *host,
		int port,
		const char *client_id,
		int keepalive,
		bool clean_session,
		const char *username,
		const char *password,
		const struct libmosquitto_will *will,
		const struct libmosquitto_tls *tls)
{
	return mosquitto_subscribe_callback(
			callback, userdata,
			topic, qos,
			host, port, client_id, keepalive, clean_session,
			username, password,
			will, tls);
}


mosquittodz::mosquittodz(const char *id, bool clean_session)
{
	m_mosq = mosquitto_new(id, clean_session, this);
	mosquitto_connect_callback_set(m_mosq, on_connect_wrapper);
	mosquitto_connect_with_flags_callback_set(m_mosq, on_connect_with_flags_wrapper);
	mosquitto_disconnect_callback_set(m_mosq, on_disconnect_wrapper);
	mosquitto_publish_callback_set(m_mosq, on_publish_wrapper);
	mosquitto_message_callback_set(m_mosq, on_message_wrapper);
	mosquitto_subscribe_callback_set(m_mosq, on_subscribe_wrapper);
	mosquitto_unsubscribe_callback_set(m_mosq, on_unsubscribe_wrapper);
	mosquitto_log_callback_set(m_mosq, on_log_wrapper);
}

mosquittodz::~mosquittodz()
{
	mosquitto_destroy(m_mosq);
}

int mosquittodz::reinitialise(const char *id, bool clean_session)
{
	int rc;
	rc = mosquitto_reinitialise(m_mosq, id, clean_session, this);
	if(rc == MOSQ_ERR_SUCCESS){
		mosquitto_connect_callback_set(m_mosq, on_connect_wrapper);
		mosquitto_connect_with_flags_callback_set(m_mosq, on_connect_with_flags_wrapper);
		mosquitto_disconnect_callback_set(m_mosq, on_disconnect_wrapper);
		mosquitto_publish_callback_set(m_mosq, on_publish_wrapper);
		mosquitto_message_callback_set(m_mosq, on_message_wrapper);
		mosquitto_subscribe_callback_set(m_mosq, on_subscribe_wrapper);
		mosquitto_unsubscribe_callback_set(m_mosq, on_unsubscribe_wrapper);
		mosquitto_log_callback_set(m_mosq, on_log_wrapper);
	}
	return rc;
}

void mosquittodz::clear_callbacks()
{
	mosquitto_connect_callback_set(m_mosq, nullptr);
	mosquitto_connect_with_flags_callback_set(m_mosq, nullptr);
	mosquitto_disconnect_callback_set(m_mosq, nullptr);
	mosquitto_publish_callback_set(m_mosq, nullptr);
	mosquitto_message_callback_set(m_mosq, nullptr);
	mosquitto_subscribe_callback_set(m_mosq, nullptr);
	mosquitto_unsubscribe_callback_set(m_mosq, nullptr);
	mosquitto_log_callback_set(m_mosq, nullptr);
}

int mosquittodz::connect(const char *host, int port, int keepalive)
{
	return mosquitto_connect(m_mosq, host, port, keepalive);
}

int mosquittodz::connect(const char *host, int port, int keepalive, const char *bind_address)
{
	return mosquitto_connect_bind(m_mosq, host, port, keepalive, bind_address);
}

int mosquittodz::connect_async(const char *host, int port, int keepalive)
{
	return mosquitto_connect_async(m_mosq, host, port, keepalive);
}

int mosquittodz::connect_async(const char *host, int port, int keepalive, const char *bind_address)
{
	return mosquitto_connect_bind_async(m_mosq, host, port, keepalive, bind_address);
}

int mosquittodz::reconnect()
{
	return mosquitto_reconnect(m_mosq);
}

int mosquittodz::reconnect_async()
{
	return mosquitto_reconnect_async(m_mosq);
}

int mosquittodz::disconnect()
{
	return mosquitto_disconnect(m_mosq);
}

int mosquittodz::socket()
{
	return mosquitto_socket(m_mosq);
}

int mosquittodz::will_set(const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return mosquitto_will_set(m_mosq, topic, payloadlen, payload, qos, retain);
}

int mosquittodz::will_clear()
{
	return mosquitto_will_clear(m_mosq);
}

int mosquittodz::username_pw_set(const char *username, const char *password)
{
	return mosquitto_username_pw_set(m_mosq, username, password);
}

int mosquittodz::publish(int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	return mosquitto_publish(m_mosq, mid, topic, payloadlen, payload, qos, retain);
}

void mosquittodz::reconnect_delay_set(unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
{
	mosquitto_reconnect_delay_set(m_mosq, reconnect_delay, reconnect_delay_max, reconnect_exponential_backoff);
}

int mosquittodz::max_inflight_messages_set(unsigned int max_inflight_messages)
{
	return mosquitto_max_inflight_messages_set(m_mosq, max_inflight_messages);
}

void mosquittodz::message_retry_set(unsigned int message_retry)
{
	mosquitto_message_retry_set(m_mosq, message_retry);
}

int mosquittodz::subscribe(int *mid, const char *sub, int qos)
{
	return mosquitto_subscribe(m_mosq, mid, sub, qos);
}

int mosquittodz::unsubscribe(int *mid, const char *sub)
{
	return mosquitto_unsubscribe(m_mosq, mid, sub);
}

int mosquittodz::loop(int timeout, int max_packets)
{
	return mosquitto_loop(m_mosq, timeout, max_packets);
}

int mosquittodz::loop_misc()
{
	return mosquitto_loop_misc(m_mosq);
}

int mosquittodz::loop_read(int max_packets)
{
	return mosquitto_loop_read(m_mosq, max_packets);
}

int mosquittodz::loop_write(int max_packets)
{
	return mosquitto_loop_write(m_mosq, max_packets);
}

int mosquittodz::loop_forever(int timeout, int max_packets)
{
	return mosquitto_loop_forever(m_mosq, timeout, max_packets);
}

int mosquittodz::loop_start()
{
	return mosquitto_loop_start(m_mosq);
}

int mosquittodz::loop_stop(bool force)
{
	return mosquitto_loop_stop(m_mosq, force);
}

bool mosquittodz::want_write()
{
	return mosquitto_want_write(m_mosq);
}

int mosquittodz::opts_set(enum mosq_opt_t option, void *value)
{
	return mosquitto_opts_set(m_mosq, option, value);
}

int mosquittodz::threaded_set(bool threaded)
{
	return mosquitto_threaded_set(m_mosq, threaded);
}

void mosquittodz::user_data_set(void *userdata)
{
	mosquitto_user_data_set(m_mosq, userdata);
}

int mosquittodz::socks5_set(const char *host, int port, const char *username, const char *password)
{
	return mosquitto_socks5_set(m_mosq, host, port, username, password);
}


int mosquittodz::tls_set(const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
	return mosquitto_tls_set(m_mosq, cafile, capath, certfile, keyfile, pw_callback);
}

int mosquittodz::tls_opts_set(int cert_reqs, const char *tls_version, const char *ciphers)
{
	return mosquitto_tls_opts_set(m_mosq, cert_reqs, tls_version, ciphers);
}

int mosquittodz::tls_insecure_set(bool value)
{
	return mosquitto_tls_insecure_set(m_mosq, value);
}

int mosquittodz::tls_psk_set(const char *psk, const char *identity, const char *ciphers)
{
	return mosquitto_tls_psk_set(m_mosq, psk, identity, ciphers);
}

}
