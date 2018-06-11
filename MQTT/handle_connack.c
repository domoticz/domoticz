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

#include "mosquitto.h"
#include "logging_mosq.h"
#include "memory_mosq.h"
#include "messages_mosq.h"
#include "net_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"

int handle__connack(struct mosquitto *mosq)
{
	uint8_t connect_flags;
	uint8_t result;
	int rc;

	assert(mosq);
	rc = packet__read_byte(&mosq->in_packet, &connect_flags);
	if(rc) return rc;
	rc = packet__read_byte(&mosq->in_packet, &result);
	if(rc) return rc;
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received CONNACK (%d)", mosq->id, result);
	pthread_mutex_lock(&mosq->callback_mutex);
	if(mosq->on_connect){
		mosq->in_callback = true;
		mosq->on_connect(mosq, mosq->userdata, result);
		mosq->in_callback = false;
	}
	if(mosq->on_connect_with_flags){
		mosq->in_callback = true;
		mosq->on_connect_with_flags(mosq, mosq->userdata, result, connect_flags);
		mosq->in_callback = false;
	}
	pthread_mutex_unlock(&mosq->callback_mutex);
	switch(result){
		case 0:
			if(mosq->state != mosq_cs_disconnecting){
				mosq->state = mosq_cs_connected;
			}
			message__retry_check(mosq);
			return MOSQ_ERR_SUCCESS;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			return MOSQ_ERR_CONN_REFUSED;
		default:
			return MOSQ_ERR_PROTOCOL;
	}
}

