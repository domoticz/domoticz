/*
Copyright (c) 2009-2018 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <string.h>

#ifdef WIN32
#  include <winsock2.h>
#  include <aclapi.h>
#  include <io.h>
#  include <lmcons.h>
#else
#  include <sys/stat.h>
#endif


#ifdef WITH_BROKER
#include "mosquitto_broker_internal.h"
#endif

#include "mosquitto.h"
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"
#include "time_mosq.h"
#include "tls_mosq.h"
#include "util_mosq.h"

#ifdef WITH_WEBSOCKETS
#include <libwebsockets.h>
#endif

#ifdef WITH_BROKER
void mosquitto__check_keepalive(struct mosquitto_db *db, struct mosquitto *mosq)
#else
void mosquitto__check_keepalive(struct mosquitto *mosq)
#endif
{
	time_t next_msg_out;
	time_t last_msg_in;
	time_t now = mosquitto_time();
#ifndef WITH_BROKER
	int rc;
#endif

	assert(mosq);
#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	/* Check if a lazy bridge should be timed out due to idle. */
	if(mosq->bridge && mosq->bridge->start_type == bst_lazy
				&& mosq->sock != INVALID_SOCKET
				&& now - mosq->next_msg_out - mosq->keepalive >= mosq->bridge->idle_timeout){

		log__printf(NULL, MOSQ_LOG_NOTICE, "Bridge connection %s has exceeded idle timeout, disconnecting.", mosq->id);
		net__socket_close(db, mosq);
		return;
	}
#endif
	pthread_mutex_lock(&mosq->msgtime_mutex);
	next_msg_out = mosq->next_msg_out;
	last_msg_in = mosq->last_msg_in;
	pthread_mutex_unlock(&mosq->msgtime_mutex);
	if(mosq->keepalive && mosq->sock != INVALID_SOCKET &&
			(now >= next_msg_out || now - last_msg_in >= mosq->keepalive)){

		if(mosq->state == mosq_cs_connected && mosq->ping_t == 0){
			send__pingreq(mosq);
			/* Reset last msg times to give the server time to send a pingresp */
			pthread_mutex_lock(&mosq->msgtime_mutex);
			mosq->last_msg_in = now;
			mosq->next_msg_out = now + mosq->keepalive;
			pthread_mutex_unlock(&mosq->msgtime_mutex);
		}else{
#ifdef WITH_BROKER
			if(mosq->listener){
				mosq->listener->client_count--;
				assert(mosq->listener->client_count >= 0);
			}
			mosq->listener = NULL;
			net__socket_close(db, mosq);
#else
			net__socket_close(mosq);
			pthread_mutex_lock(&mosq->state_mutex);
			if(mosq->state == mosq_cs_disconnecting){
				rc = MOSQ_ERR_SUCCESS;
			}else{
				rc = 1;
			}
			pthread_mutex_unlock(&mosq->state_mutex);
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_disconnect){
				mosq->in_callback = true;
				mosq->on_disconnect(mosq, mosq->userdata, rc);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
#endif
		}
	}
}

uint16_t mosquitto__mid_generate(struct mosquitto *mosq)
{
	/* FIXME - this would be better with atomic increment, but this is safer
	 * for now for a bug fix release.
	 *
	 * If this is changed to use atomic increment, callers of this function
	 * will have to be aware that they may receive a 0 result, which may not be
	 * used as a mid.
	 */
	uint16_t mid;
	assert(mosq);

	pthread_mutex_lock(&mosq->mid_mutex);
	mosq->last_mid++;
	if(mosq->last_mid == 0) mosq->last_mid++;
	mid = mosq->last_mid;
	pthread_mutex_unlock(&mosq->mid_mutex);

	return mid;
}

/* Check that a topic used for publishing is valid.
 * Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_pub_topic_check(const char *str)
{
	int len = 0;
	while(str && str[0]){
		if(str[0] == '+' || str[0] == '#'){
			return MOSQ_ERR_INVAL;
		}
		len++;
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_pub_topic_check2(const char *str, size_t len)
{
	int i;

	if(len > 65535) return MOSQ_ERR_INVAL;

	for(i=0; i<len; i++){
		if(str[i] == '+' || str[i] == '#'){
			return MOSQ_ERR_INVAL;
		}
	}

	return MOSQ_ERR_SUCCESS;
}

/* Check that a topic used for subscriptions is valid.
 * Search for + or # in a topic, check they aren't in invalid positions such as
 * foo/#/bar, foo/+bar or foo/bar#.
 * Return MOSQ_ERR_INVAL if invalid position found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_sub_topic_check(const char *str)
{
	char c = '\0';
	int len = 0;
	while(str && str[0]){
		if(str[0] == '+'){
			if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')){
				return MOSQ_ERR_INVAL;
			}
		}else if(str[0] == '#'){
			if((c != '\0' && c != '/')  || str[1] != '\0'){
				return MOSQ_ERR_INVAL;
			}
		}
		len++;
		c = str[0];
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_sub_topic_check2(const char *str, size_t len)
{
	char c = '\0';
	int i;

	if(len > 65535) return MOSQ_ERR_INVAL;

	for(i=0; i<len; i++){
		if(str[i] == '+'){
			if((c != '\0' && c != '/') || (i<len-1 && str[i+1] != '/')){
				return MOSQ_ERR_INVAL;
			}
		}else if(str[i] == '#'){
			if((c != '\0' && c != '/')  || i<len-1){
				return MOSQ_ERR_INVAL;
			}
		}
		c = str[i];
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	int slen, tlen;

	if(!result) return MOSQ_ERR_INVAL;
	*result = false;

	if(!sub || !topic){
		return MOSQ_ERR_INVAL;
	}

	slen = strlen(sub);
	tlen = strlen(topic);

	return mosquitto_topic_matches_sub2(sub, slen, topic, tlen, result);
}

/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub2(const char *sub, size_t sublen, const char *topic, size_t topiclen, bool *result)
{
	int spos, tpos;
	bool multilevel_wildcard = false;

	if(!result) return MOSQ_ERR_INVAL;
	*result = false;

	if(!sub || !topic){
		return MOSQ_ERR_INVAL;
	}

	if(!sublen || !topiclen){
		*result = false;
		return MOSQ_ERR_INVAL;
	}

	if(sublen && topiclen){
		if((sub[0] == '$' && topic[0] != '$')
				|| (topic[0] == '$' && sub[0] != '$')){

			return MOSQ_ERR_SUCCESS;
		}
	}

	spos = 0;
	tpos = 0;

	while(spos < sublen && tpos <= topiclen){
		if(sub[spos] == topic[tpos]){
			if(tpos == topiclen-1){
				/* Check for e.g. foo matching foo/# */
				if(spos == sublen-3
						&& sub[spos+1] == '/'
						&& sub[spos+2] == '#'){
					*result = true;
					multilevel_wildcard = true;
					return MOSQ_ERR_SUCCESS;
				}
			}
			spos++;
			tpos++;
			if(spos == sublen && tpos == topiclen){
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}else if(tpos == topiclen && spos == sublen-1 && sub[spos] == '+'){
				if(spos > 0 && sub[spos-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				spos++;
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}
		}else{
			if(sub[spos] == '+'){
				/* Check for bad "+foo" or "a/+foo" subscription */
				if(spos > 0 && sub[spos-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				/* Check for bad "foo+" or "foo+/a" subscription */
				if(spos < sublen-1 && sub[spos+1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				spos++;
				while(tpos < topiclen && topic[tpos] != '/'){
					tpos++;
				}
				if(tpos == topiclen && spos == sublen){
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else if(sub[spos] == '#'){
				if(spos > 0 && sub[spos-1] != '/'){
					return MOSQ_ERR_INVAL;
				}
				multilevel_wildcard = true;
				if(spos+1 != sublen){
					return MOSQ_ERR_INVAL;
				}else{
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else{
				/* Check for e.g. foo/bar matching foo/+/# */
				if(spos > 0
						&& spos+2 == sublen
						&& tpos == topiclen
						&& sub[spos-1] == '+'
						&& sub[spos] == '/'
						&& sub[spos+1] == '#')
				{
					*result = true;
					multilevel_wildcard = true;
					return MOSQ_ERR_SUCCESS;
				}
				return MOSQ_ERR_SUCCESS;
			}
		}
	}
	if(multilevel_wildcard == false && (tpos < topiclen || spos < sublen)){
		*result = false;
	}

	return MOSQ_ERR_SUCCESS;
}

#ifdef WITH_TLS_PSK
int mosquitto__hex2bin(const char *hex, unsigned char *bin, int bin_max_len)
{
	BIGNUM *bn = NULL;
	int len;
	int leading_zero = 0;
	int start = 0;
	int i = 0;

	/* Count the number of leading zero */
	for(i=0; i<strlen(hex); i=i+2) {
		if(strncmp(hex + i, "00", 2) == 0) {
			leading_zero++;
			/* output leading zero to bin */
			bin[start++] = 0;
		}else{
			break;
		}
	}

	if(BN_hex2bn(&bn, hex) == 0){
		if(bn) BN_free(bn);
		return 0;
	}
	if(BN_num_bytes(bn) + leading_zero > bin_max_len){
		BN_free(bn);
		return 0;
	}

	len = BN_bn2bin(bn, bin + leading_zero);
	BN_free(bn);
	return len + leading_zero;
}
#endif

FILE *mosquitto__fopen(const char *path, const char *mode, bool restrict_read)
{
#ifdef WIN32
	char buf[4096];
	int rc;
	rc = ExpandEnvironmentStrings(path, buf, 4096);
	if(rc == 0 || rc > 4096){
		return NULL;
	}else{
		if (restrict_read) {
			HANDLE hfile;
			SECURITY_ATTRIBUTES sec;
			EXPLICIT_ACCESS ea;
			PACL pacl = NULL;
			char username[UNLEN + 1];
			int ulen = UNLEN;
			SECURITY_DESCRIPTOR sd;

			GetUserName(username, &ulen);
			if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
				return NULL;
			}
			BuildExplicitAccessWithName(&ea, username, GENERIC_ALL, SET_ACCESS, NO_INHERITANCE);
			if (SetEntriesInAcl(1, &ea, NULL, &pacl) != ERROR_SUCCESS) {
				return NULL;
			}
			if (!SetSecurityDescriptorDacl(&sd, TRUE, pacl, FALSE)) {
				LocalFree(pacl);
				return NULL;
			}

			sec.nLength = sizeof(SECURITY_ATTRIBUTES);
			sec.bInheritHandle = FALSE;
			sec.lpSecurityDescriptor = &sd;

			hfile = CreateFile(buf, GENERIC_READ | GENERIC_WRITE, 0,
				&sec,
				CREATE_NEW,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			LocalFree(pacl);

			int fd = _open_osfhandle((intptr_t)hfile, 0);
			if (fd < 0) {
				return NULL;
			}

			FILE *fptr = _fdopen(fd, mode);
			if (!fptr) {
				_close(fd);
				return NULL;
			}
			return fptr;

		}else {
			return fopen(buf, mode);
		}
	}
#else
	if (restrict_read) {
		FILE *fptr;
		mode_t old_mask;

		old_mask = umask(0077);
		fptr = fopen(path, mode);
		umask(old_mask);

		return fptr;
	}else{
		return fopen(path, mode);
	}
#endif
}
