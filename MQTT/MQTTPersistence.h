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
 *    Ian Craggs - async client updates
 *    Ian Craggs - fix for bug 432903 - queue persistence
 *******************************************************************************/

#if defined(__cplusplus)
 extern "C" {
#endif

#include "Clients.h"

/** Stem of the key for a sent PUBLISH QoS1 or QoS2 */
#define PERSISTENCE_PUBLISH_SENT "s-"
/** Stem of the key for a sent PUBREL */
#define PERSISTENCE_PUBREL "sc-"
/** Stem of the key for a received PUBLISH QoS2 */
#define PERSISTENCE_PUBLISH_RECEIVED "r-"
/** Stem of the key for an async client command */
#define PERSISTENCE_COMMAND_KEY "c-"
/** Stem of the key for an async client message queue */
#define PERSISTENCE_QUEUE_KEY "q-"
#define PERSISTENCE_MAX_KEY_LENGTH 8

int MQTTPersistence_create(MQTTClient_persistence** per, int type, void* pcontext);
int MQTTPersistence_initialize(Clients* c, const char* serverURI);
int MQTTPersistence_close(Clients* c);
int MQTTPersistence_clear(Clients* c);
int MQTTPersistence_restore(Clients* c);
void* MQTTPersistence_restorePacket(char* buffer, size_t buflen);
void MQTTPersistence_insertInOrder(List* list, void* content, size_t size);
int MQTTPersistence_put(int socket, char* buf0, size_t buf0len, int count, 
								 char** buffers, size_t* buflens, int htype, int msgId, int scr);
int MQTTPersistence_remove(Clients* c, char* type, int qos, int msgId);
void MQTTPersistence_wrapMsgID(Clients *c);

typedef struct
{
	char struct_id[4];
	int struct_version;
	int payloadlen;
	void* payload;
	int qos;
	int retained;
	int dup;
	int msgid;
} MQTTPersistence_message;

typedef struct
{
	MQTTPersistence_message* msg;
	char* topicName;
	int topicLen;
	unsigned int seqno; /* only used on restore */
} MQTTPersistence_qEntry;

int MQTTPersistence_unpersistQueueEntry(Clients* client, MQTTPersistence_qEntry* qe);
int MQTTPersistence_persistQueueEntry(Clients* aclient, MQTTPersistence_qEntry* qe);
int MQTTPersistence_restoreMessageQueue(Clients* c);
#ifdef __cplusplus
     }
#endif
