/*
Copyright (c) 2009-2019 Roger Light <roger@atchoo.org>

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

#include "config.h"

#include <assert.h>
#include <string.h>

#ifdef WITH_BROKER
#  include "mosquitto_broker_internal.h"
#endif

#include "mosquitto.h"
#include "mosquitto_internal.h"
#include "logging_mosq.h"
#include "memory_mosq.h"
#include "mqtt_protocol.h"
#include "packet_mosq.h"
#include "property_mosq.h"
#include "util_mosq.h"


int send__subscribe(struct mosquitto *mosq, int *mid, int topic_count, const char **topic, int topic_qos, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;
	int i;
	int proplen, varbytes;

	assert(mosq);
	assert(topic);

	packet = mosquitto__calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packetlen = 2;
	if(mosq->protocol == mosq_p_mqtt5){
		proplen = property__get_length_all(properties);
		varbytes = packet__varint_bytes(proplen);
		packetlen += proplen + varbytes;
	}
	for(i=0; i<topic_count; i++){
		packetlen += 2+strlen(topic[i]) + 1;
	}

	packet->command = CMD_SUBSCRIBE | (1<<1);
	packet->remaining_length = packetlen;
	rc = packet__alloc(packet);
	if(rc){
		mosquitto__free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = mosquitto__mid_generate(mosq);
	if(mid) *mid = (int)local_mid;
	packet__write_uint16(packet, local_mid);

	if(mosq->protocol == mosq_p_mqtt5){
		property__write_all(packet, properties, true);
	}

	/* Payload */
	for(i=0; i<topic_count; i++){
		packet__write_string(packet, topic[i], strlen(topic[i]));
		packet__write_byte(packet, topic_qos);
	}

#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	log__printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d, Options: 0x%02x)", mosq->id, local_mid, topic[0], topic_qos&0x03, topic_qos&0xFC);
# endif
#else
	for(i=0; i<topic_count; i++){
		log__printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d, Options: 0x%02x)", mosq->id, local_mid, topic[i], topic_qos&0x03, topic_qos&0xFC);
	}
#endif

	return packet__queue(mosq, packet);
}

