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
#include <stdio.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "mosquitto_broker_internal.h"
#endif

#include "mosquitto.h"
#include "logging_mosq.h"
#include "memory_mosq.h"
#include "messages_mosq.h"
#include "mqtt3_protocol.h"
#include "net_mosq.h"
#include "packet_mosq.h"
#include "read_handle.h"
#include "send_mosq.h"
#include "util_mosq.h"

int handle__pingreq(struct mosquitto *mosq)
{
	assert(mosq);
#ifdef WITH_BROKER
	log__printf(NULL, MOSQ_LOG_DEBUG, "Received PINGREQ from %s", mosq->id);
#else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGREQ", mosq->id);
#endif
	return send__pingresp(mosq);
}

int handle__pingresp(struct mosquitto *mosq)
{
	assert(mosq);
	mosq->ping_t = 0; /* No longer waiting for a PINGRESP. */
#ifdef WITH_BROKER
	log__printf(NULL, MOSQ_LOG_DEBUG, "Received PINGRESP from %s", mosq->id);
#else
	log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGRESP", mosq->id);
#endif
	return MOSQ_ERR_SUCCESS;
}

