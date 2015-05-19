/*******************************************************************************
 * Copyright (c) 2009, 2013 IBM Corp.
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
 *    Ian Craggs - MQTT 3.1.1 updates
 *    Rong Xiang, Ian Craggs - C++ compatibility
 *******************************************************************************/

#if !defined(MQTTPROTOCOLCLIENT_H)
#define MQTTPROTOCOLCLIENT_H

#include "LinkedList.h"
#include "MQTTPacket.h"
#include "Log.h"
#include "MQTTProtocol.h"
#include "Messages.h"

#define MAX_MSG_ID 65535
#define MAX_CLIENTID_LEN 65535

int MQTTProtocol_startPublish(Clients* pubclient, Publish* publish, int qos, int retained, Messages** m);
Messages* MQTTProtocol_createMessage(Publish* publish, Messages** mm, int qos, int retained);
Publications* MQTTProtocol_storePublication(Publish* publish, int* len);
int messageIDCompare(void* a, void* b);
int MQTTProtocol_assignMsgId(Clients* client);
void MQTTProtocol_removePublication(Publications* p);

int MQTTProtocol_handlePublishes(void* pack, int sock);
int MQTTProtocol_handlePubacks(void* pack, int sock);
int MQTTProtocol_handlePubrecs(void* pack, int sock);
int MQTTProtocol_handlePubrels(void* pack, int sock);
int MQTTProtocol_handlePubcomps(void* pack, int sock);

void MQTTProtocol_keepalive(time_t);
void MQTTProtocol_retry(time_t, int, int);
void MQTTProtocol_freeClient(Clients* client);
void MQTTProtocol_emptyMessageList(List* msgList);
void MQTTProtocol_freeMessageList(List* msgList);

char* MQTTStrncpy(char *dest, const char* src, size_t num);
char* MQTTStrdup(const char* src);
#endif
