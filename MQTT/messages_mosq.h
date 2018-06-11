/*
Copyright (c) 2010-2018 Roger Light <roger@atchoo.org>

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
#ifndef MESSAGES_MOSQ_H
#define MESSAGES_MOSQ_H

#include "mosquitto_internal.h"
#include "mosquitto.h"

void message__cleanup_all(struct mosquitto *mosq);
void message__cleanup(struct mosquitto_message_all **message);
int message__delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir);
int message__queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir);
void message__reconnect_reset(struct mosquitto *mosq);
int message__remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message);
void message__retry_check(struct mosquitto *mosq);
int message__out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state);

#endif
