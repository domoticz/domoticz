/*******************************************************************************
 * Copyright (c) 2009, 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs, Allan Stockdill-Mander - SSL updates
 *    Ian Craggs - MQTT 3.1.1 support
 *******************************************************************************/

#if !defined(MQTTPACKETOUT_H)
#define MQTTPACKETOUT_H

#include "MQTTPacket.h"

int MQTTPacket_send_connect(Clients* client, int MQTTVersion);
void* MQTTPacket_connack(unsigned char aHeader, char* data, size_t datalen);

int MQTTPacket_send_pingreq(networkHandles* net, const char* clientID);

int MQTTPacket_send_subscribe(List* topics, List* qoss, int msgid, int dup, networkHandles* net, const char* clientID);
void* MQTTPacket_suback(unsigned char aHeader, char* data, size_t datalen);

int MQTTPacket_send_unsubscribe(List* topics, int msgid, int dup, networkHandles* net, const char* clientID);

#endif
