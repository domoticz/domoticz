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
 *    Ian Craggs - bug 384016 - segv setting will message
 *    Ian Craggs - bug 384053 - v1.0.0.7 - stop MQTTClient_receive on socket error 
 *    Ian Craggs, Allan Stockdill-Mander - add ability to connect with SSL
 *    Ian Craggs - multiple server connection support
 *    Ian Craggs - fix for bug 413429 - connectionLost not called
 *    Ian Craggs - fix for bug 421103 - trying to write to same socket, in publish/retries
 *    Ian Craggs - fix for bug 419233 - mutexes not reporting errors
 *    Ian Craggs - fix for bug 420851
 *    Ian Craggs - fix for bug 432903 - queue persistence
 *    Ian Craggs - MQTT 3.1.1 support
 *    Ian Craggs - fix for bug 438176 - MQTT version selection
 *    Rong Xiang, Ian Craggs - C++ compatibility
 *    Ian Craggs - fix for bug 443724 - stack corruption
 *******************************************************************************/

/**
 * @file
 * \brief Synchronous API implementation
 *
 */

#define _GNU_SOURCE /* for pthread_mutexattr_settype */
#include <stdlib.h>
#if !defined(WIN32) && !defined(WIN64)
	#include <sys/time.h>
#endif

#include "MQTTClient.h"
#if !defined(NO_PERSISTENCE)
#include "MQTTPersistence.h"
#endif

#include "utf-8.h"
#include "MQTTProtocol.h"
#include "MQTTProtocolOut.h"
#include "Thread.h"
#include "SocketBuffer.h"
#include "StackTrace.h"
#include "Heap.h"

#if defined(OPENSSL)
#include <openssl/ssl.h>
#endif

#define URI_TCP "tcp://"

#define BUILD_TIMESTAMP "##MQTTCLIENT_BUILD_TAG##"
#define CLIENT_VERSION  "##MQTTCLIENT_VERSION_TAG##"

char* client_timestamp_eye = "MQTTClientV3_Timestamp " BUILD_TIMESTAMP;
char* client_version_eye = "MQTTClientV3_Version " CLIENT_VERSION;

static ClientStates ClientState =
{
	CLIENT_VERSION, /* version */
	NULL /* client list */
};

ClientStates* bstate = &ClientState;

MQTTProtocol state;

#if defined(WIN32) || defined(WIN64)
static mutex_type mqttclient_mutex = NULL;
extern mutex_type stack_mutex;
extern mutex_type heap_mutex;
extern mutex_type log_mutex;
BOOL APIENTRY DllMain(HANDLE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			Log(TRACE_MAX, -1, "DLL process attach");
			if (mqttclient_mutex == NULL)
			{
				mqttclient_mutex = CreateMutex(NULL, 0, NULL);
				stack_mutex = CreateMutex(NULL, 0, NULL);
				heap_mutex = CreateMutex(NULL, 0, NULL);
				log_mutex = CreateMutex(NULL, 0, NULL);
			}
		case DLL_THREAD_ATTACH:
			Log(TRACE_MAX, -1, "DLL thread attach");
		case DLL_THREAD_DETACH:
			Log(TRACE_MAX, -1, "DLL thread detach");
		case DLL_PROCESS_DETACH:
			Log(TRACE_MAX, -1, "DLL process detach");
	}
	return TRUE;
}
#else
static pthread_mutex_t mqttclient_mutex_store = PTHREAD_MUTEX_INITIALIZER;
static mutex_type mqttclient_mutex = &mqttclient_mutex_store;

void MQTTClient_init()
{
	pthread_mutexattr_t attr;
	int rc;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	if ((rc = pthread_mutex_init(mqttclient_mutex, &attr)) != 0)
		printf("MQTTClient: error %d initializing client_mutex\n", rc);
}

#define WINAPI
#endif

static volatile int initialized = 0;
static List* handles = NULL;
static time_t last;
static int running = 0;
static int tostop = 0;
static thread_id_type run_id = 0;

MQTTPacket* MQTTClient_waitfor(MQTTClient handle, int packet_type, int* rc, long timeout);
MQTTPacket* MQTTClient_cycle(int* sock, unsigned long timeout, int* rc);
int MQTTClient_cleanSession(Clients* client);
void MQTTClient_stop();
int MQTTClient_disconnect_internal(MQTTClient handle, int timeout);
int MQTTClient_disconnect1(MQTTClient handle, int timeout, int internal, int stop);
void MQTTClient_writeComplete(int socket);

typedef struct
{
	MQTTClient_message* msg;
	char* topicName;
	int topicLen;
	unsigned int seqno; /* only used on restore */
} qEntry;


typedef struct
{
	char* serverURI;
#if defined(OPENSSL)
	int ssl;
#endif
	Clients* c;
	MQTTClient_connectionLost* cl;
	MQTTClient_messageArrived* ma;
	MQTTClient_deliveryComplete* dc;
	void* context;

	sem_type connect_sem;
	int rc; /* getsockopt return code in connect */
	sem_type connack_sem;
	sem_type suback_sem;
	sem_type unsuback_sem;
	MQTTPacket* pack;

} MQTTClients;

void MQTTClient_sleep(long milliseconds)
{
	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	Sleep(milliseconds);
#else
	usleep(milliseconds*1000);
#endif
	FUNC_EXIT;
}


#if defined(WIN32) || defined(WIN64)
#define START_TIME_TYPE DWORD
START_TIME_TYPE MQTTClient_start_clock(void)
{
	return GetTickCount();
}
#elif defined(AIX)
#define START_TIME_TYPE struct timespec
START_TIME_TYPE MQTTClient_start_clock(void)
{
	static struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	return start;
}
#else
#define START_TIME_TYPE struct timeval
START_TIME_TYPE MQTTClient_start_clock(void)
{
	static struct timeval start;
	gettimeofday(&start, NULL);
	return start;
}
#endif


#if defined(WIN32) || defined(WIN64)
long MQTTClient_elapsed(DWORD milliseconds)
{
	return GetTickCount() - milliseconds;
}
#elif defined(AIX)
#define assert(a)
long MQTTClient_elapsed(struct timespec start)
{
	struct timespec now, res;

	clock_gettime(CLOCK_REALTIME, &now);
	ntimersub(now, start, res);
	return (res.tv_sec)*1000L + (res.tv_nsec)/1000000L;
}
#else
long MQTTClient_elapsed(struct timeval start)
{
	struct timeval now, res;

	gettimeofday(&now, NULL);
	timersub(&now, &start, &res);
	return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}
#endif


int MQTTClient_create(MQTTClient* handle, const char* serverURI, const char* clientId,
		int persistence_type, void* persistence_context)
{
	int rc = 0;
	MQTTClients *m = NULL;

	FUNC_ENTRY;
	rc = Thread_lock_mutex(mqttclient_mutex);

	if (serverURI == NULL || clientId == NULL)
	{
		rc = MQTTCLIENT_NULL_PARAMETER;
		goto exit;
	}

	if (!UTF8_validateString(clientId))
	{
		rc = MQTTCLIENT_BAD_UTF8_STRING;
		goto exit;
	}

	if (!initialized)
	{
		#if defined(HEAP_H)
			Heap_initialize();
		#endif
		Log_initialize((Log_nameValue*)MQTTClient_getVersionInfo());
		bstate->clients = ListInitialize();
		Socket_outInitialize();
		Socket_setWriteCompleteCallback(MQTTClient_writeComplete);
		handles = ListInitialize();
#if defined(OPENSSL)
		SSLSocket_initialize();
#endif
		initialized = 1;
	}
	m = malloc(sizeof(MQTTClients));
	*handle = m;
	memset(m, '\0', sizeof(MQTTClients));
	if (strncmp(URI_TCP, serverURI, strlen(URI_TCP)) == 0)
		serverURI += strlen(URI_TCP);
#if defined(OPENSSL)
	else if (strncmp(URI_SSL, serverURI, strlen(URI_SSL)) == 0)
	{
		serverURI += strlen(URI_SSL);
		m->ssl = 1;
	}
#endif
	m->serverURI = MQTTStrdup(serverURI);
	ListAppend(handles, m, sizeof(MQTTClients));

	m->c = malloc(sizeof(Clients));
	memset(m->c, '\0', sizeof(Clients));
	m->c->context = m;
	m->c->outboundMsgs = ListInitialize();
	m->c->inboundMsgs = ListInitialize();
	m->c->messageQueue = ListInitialize();
	m->c->clientID = MQTTStrdup(clientId);
	m->connect_sem = Thread_create_sem();
	m->connack_sem = Thread_create_sem();
	m->suback_sem = Thread_create_sem();
	m->unsuback_sem = Thread_create_sem();

#if !defined(NO_PERSISTENCE)
	rc = MQTTPersistence_create(&(m->c->persistence), persistence_type, persistence_context);
	if (rc == 0)
	{
		rc = MQTTPersistence_initialize(m->c, m->serverURI);
		if (rc == 0)
			MQTTPersistence_restoreMessageQueue(m->c);
	}
#endif
	ListAppend(bstate->clients, m->c, sizeof(Clients) + 3*sizeof(List));

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTClient_terminate(void)
{
	FUNC_ENTRY;
	MQTTClient_stop();
	if (initialized)
	{
		ListFree(bstate->clients);
		ListFree(handles);
		handles = NULL;
		Socket_outTerminate();
#if defined(OPENSSL)
		SSLSocket_terminate();
#endif
		#if defined(HEAP_H)
			Heap_terminate();
		#endif
		Log_terminate();
		initialized = 0;
	}
	FUNC_EXIT;
}


void MQTTClient_emptyMessageQueue(Clients* client)
{
	FUNC_ENTRY;
	/* empty message queue */
	if (client->messageQueue->count > 0)
	{
		ListElement* current = NULL;
		while (ListNextElement(client->messageQueue, &current))
		{
			qEntry* qe = (qEntry*)(current->content);
			free(qe->topicName);
			free(qe->msg->payload);
			free(qe->msg);
		}
		ListEmpty(client->messageQueue);
	}
	FUNC_EXIT;
}


void MQTTClient_destroy(MQTTClient* handle)
{
	MQTTClients* m = *handle;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL)
		goto exit;

	if (m->c)
	{
		int saved_socket = m->c->net.socket;
		char* saved_clientid = MQTTStrdup(m->c->clientID);
#if !defined(NO_PERSISTENCE)
		MQTTPersistence_close(m->c);
#endif
		MQTTClient_emptyMessageQueue(m->c);
		MQTTProtocol_freeClient(m->c);
		if (!ListRemove(bstate->clients, m->c))
			Log(LOG_ERROR, 0, NULL);
		else
			Log(TRACE_MIN, 1, NULL, saved_clientid, saved_socket);
		free(saved_clientid);
	}
	if (m->serverURI)
		free(m->serverURI);
	Thread_destroy_sem(m->connect_sem);
	Thread_destroy_sem(m->connack_sem);
	Thread_destroy_sem(m->suback_sem);
	Thread_destroy_sem(m->unsuback_sem);
	if (!ListRemove(handles, m))
		Log(LOG_ERROR, -1, "free error");
	*handle = NULL;
	if (bstate->clients->count == 0)
		MQTTClient_terminate();

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT;
}


void MQTTClient_freeMessage(MQTTClient_message** message)
{
	FUNC_ENTRY;
	free((*message)->payload);
	free(*message);
	*message = NULL;
	FUNC_EXIT;
}


void MQTTClient_free(void* memory)
{
	FUNC_ENTRY;
	free(memory);
	FUNC_EXIT;
}


int MQTTClient_deliverMessage(int rc, MQTTClients* m, char** topicName, int* topicLen, MQTTClient_message** message)
{
	qEntry* qe = (qEntry*)(m->c->messageQueue->first->content);

	FUNC_ENTRY;
	*message = qe->msg;
	*topicName = qe->topicName;
	*topicLen = qe->topicLen;
	if (strlen(*topicName) != *topicLen)
		rc = MQTTCLIENT_TOPICNAME_TRUNCATED;
#if !defined(NO_PERSISTENCE)
	if (m->c->persistence)
		MQTTPersistence_unpersistQueueEntry(m->c, (MQTTPersistence_qEntry*)qe);
#endif
	ListRemove(m->c->messageQueue, m->c->messageQueue->first->content);
	FUNC_EXIT_RC(rc);
	return rc;
}


/**
 * List callback function for comparing clients by socket
 * @param a first integer value
 * @param b second integer value
 * @return boolean indicating whether a and b are equal
 */
int clientSockCompare(void* a, void* b)
{
	MQTTClients* m = (MQTTClients*)a;
	return m->c->net.socket == *(int*)b;
}


/**
 * Wrapper function to call connection lost on a separate thread.  A separate thread is needed to allow the
 * connectionLost function to make API calls (e.g. connect)
 * @param context a pointer to the relevant client
 * @return thread_return_type standard thread return value - not used here
 */
thread_return_type WINAPI connectionLost_call(void* context)
{
	MQTTClients* m = (MQTTClients*)context;

	(*(m->cl))(m->context, NULL);
	return 0;
}


/* This is the thread function that handles the calling of callback functions if set */
thread_return_type WINAPI MQTTClient_run(void* n)
{
	long timeout = 10L; /* first time in we have a small timeout.  Gets things started more quickly */

	FUNC_ENTRY;
	running = 1;
	run_id = Thread_getid();

	Thread_lock_mutex(mqttclient_mutex);
	while (!tostop)
	{
		int rc = SOCKET_ERROR;
		int sock = -1;
		MQTTClients* m = NULL;
		MQTTPacket* pack = NULL;

		Thread_unlock_mutex(mqttclient_mutex);
		pack = MQTTClient_cycle(&sock, timeout, &rc);
		Thread_lock_mutex(mqttclient_mutex);
		if (tostop)
			break;
		timeout = 1000L;

		/* find client corresponding to socket */
		if (ListFindItem(handles, &sock, clientSockCompare) == NULL)
		{
			/* assert: should not happen */
			continue;
		}
		m = (MQTTClient)(handles->current->content);
		if (m == NULL)
		{
			/* assert: should not happen */
			continue;
		}
		if (rc == SOCKET_ERROR)
		{
			if (m->c->connected)
			{
				Thread_unlock_mutex(mqttclient_mutex);
				MQTTClient_disconnect_internal(m, 0);
				Thread_lock_mutex(mqttclient_mutex);
			}
			else 
			{
				if (m->c->connect_state == 2 && !Thread_check_sem(m->connect_sem))
				{
					Log(TRACE_MIN, -1, "Posting connect semaphore for client %s", m->c->clientID);
					Thread_post_sem(m->connect_sem);
				}
				if (m->c->connect_state == 3 && !Thread_check_sem(m->connack_sem))
				{
					Log(TRACE_MIN, -1, "Posting connack semaphore for client %s", m->c->clientID);
					Thread_post_sem(m->connack_sem);
				}
			}
		}
		else
		{
			if (m->c->messageQueue->count > 0)
			{
				qEntry* qe = (qEntry*)(m->c->messageQueue->first->content);
				int topicLen = qe->topicLen;

				if (strlen(qe->topicName) == topicLen)
					topicLen = 0;

				Log(TRACE_MIN, -1, "Calling messageArrived for client %s, queue depth %d",
					m->c->clientID, m->c->messageQueue->count);
				Thread_unlock_mutex(mqttclient_mutex);
				rc = (*(m->ma))(m->context, qe->topicName, topicLen, qe->msg);
				Thread_lock_mutex(mqttclient_mutex);
				/* if 0 (false) is returned by the callback then it failed, so we don't remove the message from
				 * the queue, and it will be retried later.  If 1 is returned then the message data may have been freed,
				 * so we must be careful how we use it.
				 */
				if (rc)
					ListRemove(m->c->messageQueue, qe);
				else
					Log(TRACE_MIN, -1, "False returned from messageArrived for client %s, message remains on queue",
						m->c->clientID);
			}
			if (pack)
			{
				if (pack->header.bits.type == CONNACK && !Thread_check_sem(m->connack_sem))
				{
					Log(TRACE_MIN, -1, "Posting connack semaphore for client %s", m->c->clientID);
					m->pack = pack;
					Thread_post_sem(m->connack_sem);
				}
				else if (pack->header.bits.type == SUBACK)
				{
					Log(TRACE_MIN, -1, "Posting suback semaphore for client %s", m->c->clientID);
					m->pack = pack;
					Thread_post_sem(m->suback_sem);
				}
				else if (pack->header.bits.type == UNSUBACK)
				{
					Log(TRACE_MIN, -1, "Posting unsuback semaphore for client %s", m->c->clientID);
					m->pack = pack;
					Thread_post_sem(m->unsuback_sem);
				}
			}
			else if (m->c->connect_state == 1 && !Thread_check_sem(m->connect_sem))
			{
				int error;
				socklen_t len = sizeof(error);

				if ((m->rc = getsockopt(m->c->net.socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len)) == 0)
					m->rc = error;
				Log(TRACE_MIN, -1, "Posting connect semaphore for client %s rc %d", m->c->clientID, m->rc);
				Thread_post_sem(m->connect_sem);
			}
#if defined(OPENSSL)
			else if (m->c->connect_state == 2 && !Thread_check_sem(m->connect_sem))
			{			
				rc = SSLSocket_connect(m->c->net.ssl, m->c->net.socket);
				if (rc == 1 || rc == SSL_FATAL)
				{
					if (rc == 1 && !m->c->cleansession && m->c->session == NULL)
						m->c->session = SSL_get1_session(m->c->net.ssl);
					m->rc = rc;
					Log(TRACE_MIN, -1, "Posting connect semaphore for SSL client %s rc %d", m->c->clientID, m->rc);
					Thread_post_sem(m->connect_sem);
				}
			}
#endif
		}
	}
	run_id = 0;
	running = tostop = 0;
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT;
	return 0;
}


void MQTTClient_stop()
{
	int rc = 0;

	FUNC_ENTRY;
	if (running == 1 && tostop == 0)
	{
		int conn_count = 0;
		ListElement* current = NULL;

		if (handles != NULL)
		{
			/* find out how many handles are still connected */
			while (ListNextElement(handles, &current))
			{
				if (((MQTTClients*)(current->content))->c->connect_state > 0 ||
						((MQTTClients*)(current->content))->c->connected)
					++conn_count;
			}
		}
		Log(TRACE_MIN, -1, "Conn_count is %d", conn_count);
		/* stop the background thread, if we are the last one to be using it */
		if (conn_count == 0)
		{
			int count = 0;
			tostop = 1;
			if (Thread_getid() != run_id)
			{
				while (running && ++count < 100)
				{
					Thread_unlock_mutex(mqttclient_mutex);
					Log(TRACE_MIN, -1, "sleeping");
					MQTTClient_sleep(100L);
					Thread_lock_mutex(mqttclient_mutex);
				}
			}
			rc = 1;
		}
	}
	FUNC_EXIT_RC(rc);
}


int MQTTClient_setCallbacks(MQTTClient handle, void* context, MQTTClient_connectionLost* cl,
														MQTTClient_messageArrived* ma, MQTTClient_deliveryComplete* dc)
{
	int rc = MQTTCLIENT_SUCCESS;
	MQTTClients* m = handle;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || ma == NULL || m->c->connect_state != 0)
		rc = MQTTCLIENT_FAILURE;
	else
	{
		m->context = context;
		m->cl = cl;
		m->ma = ma;
		m->dc = dc;
	}

	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTClient_closeSession(Clients* client)
{
	FUNC_ENTRY;
	client->good = 0;
	client->ping_outstanding = 0;
	if (client->net.socket > 0)
	{
		if (client->connected)
			MQTTPacket_send_disconnect(&client->net, client->clientID);
#if defined(OPENSSL)
		SSLSocket_close(&client->net);
#endif
		Socket_close(client->net.socket);
		client->net.socket = 0;
#if defined(OPENSSL)
		client->net.ssl = NULL;
#endif
	}
	client->connected = 0;
	client->connect_state = 0;

	if (client->cleansession)
		MQTTClient_cleanSession(client);
	FUNC_EXIT;
}


int MQTTClient_cleanSession(Clients* client)
{
	int rc = 0;

	FUNC_ENTRY;
#if !defined(NO_PERSISTENCE)
	rc = MQTTPersistence_clear(client);
#endif
	MQTTProtocol_emptyMessageList(client->inboundMsgs);
	MQTTProtocol_emptyMessageList(client->outboundMsgs);
	MQTTClient_emptyMessageQueue(client);
	client->msgID = 0;
	FUNC_EXIT_RC(rc);
	return rc;
}


void Protocol_processPublication(Publish* publish, Clients* client)
{
	qEntry* qe = NULL;
	MQTTClient_message* mm = NULL;

	FUNC_ENTRY;
	qe = malloc(sizeof(qEntry));
	mm = malloc(sizeof(MQTTClient_message));

	qe->msg = mm;

	qe->topicName = publish->topic;
	qe->topicLen = publish->topiclen;
	publish->topic = NULL;

	/* If the message is QoS 2, then we have already stored the incoming payload
	 * in an allocated buffer, so we don't need to copy again.
	 */
	if (publish->header.bits.qos == 2)
		mm->payload = publish->payload;
	else
	{
		mm->payload = malloc(publish->payloadlen);
		memcpy(mm->payload, publish->payload, publish->payloadlen);
	}

	mm->payloadlen = publish->payloadlen;
	mm->qos = publish->header.bits.qos;
	mm->retained = publish->header.bits.retain;
	if (publish->header.bits.qos == 2)
		mm->dup = 0;  /* ensure that a QoS2 message is not passed to the application with dup = 1 */
	else
		mm->dup = publish->header.bits.dup;
	mm->msgid = publish->msgId;

	ListAppend(client->messageQueue, qe, sizeof(qe) + sizeof(mm) + mm->payloadlen + strlen(qe->topicName)+1);
#if !defined(NO_PERSISTENCE)
	if (client->persistence)
		MQTTPersistence_persistQueueEntry(client, (MQTTPersistence_qEntry*)qe);
#endif
	FUNC_EXIT;
}


int MQTTClient_connectURIVersion(MQTTClient handle, MQTTClient_connectOptions* options, const char* serverURI, int MQTTVersion,
	START_TIME_TYPE start, long millisecsTimeout)
{
	MQTTClients* m = handle;
	int rc = SOCKET_ERROR;
	int sessionPresent = 0;

	FUNC_ENTRY;
	if (m->ma && !running)
	{
		Thread_start(MQTTClient_run, handle);
		if (MQTTClient_elapsed(start) >= millisecsTimeout)
		{
			rc = SOCKET_ERROR;
			goto exit;
		}
		MQTTClient_sleep(100L);
	}

	Log(TRACE_MIN, -1, "Connecting to serverURI %s with MQTT version %d", serverURI, MQTTVersion);
#if defined(OPENSSL)
	rc = MQTTProtocol_connect(serverURI, m->c, m->ssl, MQTTVersion);
#else
	rc = MQTTProtocol_connect(serverURI, m->c, MQTTVersion);
#endif
	if (rc == SOCKET_ERROR)
		goto exit;

	if (m->c->connect_state == 0)
	{
		rc = SOCKET_ERROR;
		goto exit;
	}

	if (m->c->connect_state == 1) /* TCP connect started - wait for completion */
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_waitfor(handle, CONNECT, &rc, millisecsTimeout - MQTTClient_elapsed(start));
		Thread_lock_mutex(mqttclient_mutex);
		if (rc != 0)
		{
			rc = SOCKET_ERROR;
			goto exit;
		}
		
#if defined(OPENSSL)
		if (m->ssl)
		{
			if (SSLSocket_setSocketForSSL(&m->c->net, m->c->sslopts) != MQTTCLIENT_SUCCESS)
			{
				if (m->c->session != NULL)
					if ((rc = SSL_set_session(m->c->net.ssl, m->c->session)) != 1)
						Log(TRACE_MIN, -1, "Failed to set SSL session with stored data, non critical");
				rc = SSLSocket_connect(m->c->net.ssl, m->c->net.socket);
				if (rc == TCPSOCKET_INTERRUPTED)
					m->c->connect_state = 2;  /* the connect is still in progress */
				else if (rc == SSL_FATAL)
				{
					rc = SOCKET_ERROR;
					goto exit;
				}
				else if (rc == 1) 
				{
					rc = MQTTCLIENT_SUCCESS;
					m->c->connect_state = 3;
					if (MQTTPacket_send_connect(m->c, MQTTVersion) == SOCKET_ERROR)
					{
						rc = SOCKET_ERROR;
						goto exit;
					}
					if (!m->c->cleansession && m->c->session == NULL)
						m->c->session = SSL_get1_session(m->c->net.ssl);
				}
			}
			else
			{
				rc = SOCKET_ERROR;
				goto exit;
			}
		}
		else
		{
#endif
			m->c->connect_state = 3; /* TCP connect completed, in which case send the MQTT connect packet */
			if (MQTTPacket_send_connect(m->c, MQTTVersion) == SOCKET_ERROR)
			{
				rc = SOCKET_ERROR;
				goto exit;
			}
#if defined(OPENSSL)
		}
#endif
	}
	
#if defined(OPENSSL)
	if (m->c->connect_state == 2) /* SSL connect sent - wait for completion */
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_waitfor(handle, CONNECT, &rc, millisecsTimeout - MQTTClient_elapsed(start));
		Thread_lock_mutex(mqttclient_mutex);
		if (rc != 1)
		{
			rc = SOCKET_ERROR;
			goto exit;
		}
		if(!m->c->cleansession && m->c->session == NULL)
			m->c->session = SSL_get1_session(m->c->net.ssl);
		m->c->connect_state = 3; /* TCP connect completed, in which case send the MQTT connect packet */
		if (MQTTPacket_send_connect(m->c, MQTTVersion) == SOCKET_ERROR)
		{
			rc = SOCKET_ERROR;
			goto exit;
		}
	}
#endif

	if (m->c->connect_state == 3) /* MQTT connect sent - wait for CONNACK */
	{
		MQTTPacket* pack = NULL;

		Thread_unlock_mutex(mqttclient_mutex);
		pack = MQTTClient_waitfor(handle, CONNACK, &rc, millisecsTimeout - MQTTClient_elapsed(start));
		Thread_lock_mutex(mqttclient_mutex);
		if (pack == NULL)
			rc = SOCKET_ERROR;
		else
		{
			Connack* connack = (Connack*)pack;
			Log(TRACE_PROTOCOL, 1, NULL, m->c->net.socket, m->c->clientID, connack->rc);
			if ((rc = connack->rc) == MQTTCLIENT_SUCCESS)
			{
				m->c->connected = 1;
				m->c->good = 1;
				m->c->connect_state = 0;
				if (MQTTVersion == 4)
					sessionPresent = connack->flags.bits.sessionPresent;
				if (m->c->cleansession)
					rc = MQTTClient_cleanSession(m->c);
				if (m->c->outboundMsgs->count > 0)
				{
					ListElement* outcurrent = NULL;

					while (ListNextElement(m->c->outboundMsgs, &outcurrent))
					{
						Messages* m = (Messages*)(outcurrent->content);
						m->lastTouch = 0;
					}
					MQTTProtocol_retry((time_t)0, 1, 1);
					if (m->c->connected != 1)
						rc = MQTTCLIENT_DISCONNECTED;
				}
			}
			free(connack);
			m->pack = NULL;
		}
	}
exit:
	if (rc == MQTTCLIENT_SUCCESS)
	{
		if (options->struct_version == 4) /* means we have to fill out return values */
		{			
			options->returned.serverURI = serverURI;
			options->returned.MQTTVersion = MQTTVersion;    
			options->returned.sessionPresent = sessionPresent;
		}
	}
	else
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_disconnect1(handle, 0, 0, (MQTTVersion == 3)); /* not "internal" because we don't want to call connection lost */
		Thread_lock_mutex(mqttclient_mutex);
	}
	FUNC_EXIT_RC(rc);
  return rc;
}


int MQTTClient_connectURI(MQTTClient handle, MQTTClient_connectOptions* options, const char* serverURI)
{
	MQTTClients* m = handle;
	START_TIME_TYPE start;
	long millisecsTimeout = 30000L;
	int rc = SOCKET_ERROR;
	int MQTTVersion = 0;

	FUNC_ENTRY;
	millisecsTimeout = options->connectTimeout * 1000;
	start = MQTTClient_start_clock();

	m->c->keepAliveInterval = options->keepAliveInterval;
	m->c->cleansession = options->cleansession;
	m->c->maxInflightMessages = (options->reliable) ? 1 : 10;

	if (m->c->will)
	{
		free(m->c->will->msg);
		free(m->c->will->topic);
		free(m->c->will);
		m->c->will = NULL;
	}

	if (options->will && options->will->struct_version == 0)
	{
		m->c->will = malloc(sizeof(willMessages));
		m->c->will->msg = MQTTStrdup(options->will->message);
		m->c->will->qos = options->will->qos;
		m->c->will->retained = options->will->retained;
		m->c->will->topic = MQTTStrdup(options->will->topicName);
	}
	
#if defined(OPENSSL)
	if (m->c->sslopts)
	{
		if (m->c->sslopts->trustStore)
			free((void*)m->c->sslopts->trustStore);
		if (m->c->sslopts->keyStore)
			free((void*)m->c->sslopts->keyStore);
		if (m->c->sslopts->privateKey)
			free((void*)m->c->sslopts->privateKey);
		if (m->c->sslopts->privateKeyPassword)
			free((void*)m->c->sslopts->privateKeyPassword);
		if (m->c->sslopts->enabledCipherSuites)
			free((void*)m->c->sslopts->enabledCipherSuites);
		free(m->c->sslopts);
		m->c->sslopts = NULL;
	}

	if (options->struct_version != 0 && options->ssl)
	{
		m->c->sslopts = malloc(sizeof(MQTTClient_SSLOptions));
		memset(m->c->sslopts, '\0', sizeof(MQTTClient_SSLOptions));
		if (options->ssl->trustStore)
			m->c->sslopts->trustStore = MQTTStrdup(options->ssl->trustStore);
		if (options->ssl->keyStore)
			m->c->sslopts->keyStore = MQTTStrdup(options->ssl->keyStore);
		if (options->ssl->privateKey)
			m->c->sslopts->privateKey = MQTTStrdup(options->ssl->privateKey);
		if (options->ssl->privateKeyPassword)
			m->c->sslopts->privateKeyPassword = MQTTStrdup(options->ssl->privateKeyPassword);
		if (options->ssl->enabledCipherSuites)
			m->c->sslopts->enabledCipherSuites = MQTTStrdup(options->ssl->enabledCipherSuites);
		m->c->sslopts->enableServerCertAuth = options->ssl->enableServerCertAuth;
	}
#endif

	m->c->username = options->username;
	m->c->password = options->password;
	m->c->retryInterval = options->retryInterval;

	if (options->struct_version >= 3)
		MQTTVersion = options->MQTTVersion;
	else
		MQTTVersion = MQTTVERSION_DEFAULT;

	if (MQTTVersion == MQTTVERSION_DEFAULT)
	{
		if ((rc = MQTTClient_connectURIVersion(handle, options, serverURI, 4, start, millisecsTimeout)) != MQTTCLIENT_SUCCESS)
			rc = MQTTClient_connectURIVersion(handle, options, serverURI, 3, start, millisecsTimeout);
	}
	else
		rc = MQTTClient_connectURIVersion(handle, options, serverURI, MQTTVersion, start, millisecsTimeout);

	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_connect(MQTTClient handle, MQTTClient_connectOptions* options)
{
	MQTTClients* m = handle;
	int rc = SOCKET_ERROR;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (options == NULL)
	{
		rc = MQTTCLIENT_NULL_PARAMETER;
		goto exit;
	}

	if (strncmp(options->struct_id, "MQTC", 4) != 0 || 
		(options->struct_version != 0 && options->struct_version != 1 && options->struct_version != 2
			&& options->struct_version != 3 && options->struct_version != 4))
	{
		rc = MQTTCLIENT_BAD_STRUCTURE;
		goto exit;
	}

	if (options->will) /* check validity of will options structure */
	{
		if (strncmp(options->will->struct_id, "MQTW", 4) != 0 || options->will->struct_version != 0)
		{
			rc = MQTTCLIENT_BAD_STRUCTURE;
			goto exit;
		}
	}
	
#if defined(OPENSSL)
	if (options->struct_version != 0 && options->ssl) /* check validity of SSL options structure */
	{
		if (strncmp(options->ssl->struct_id, "MQTS", 4) != 0 || options->ssl->struct_version != 0)
		{
			rc = MQTTCLIENT_BAD_STRUCTURE;
			goto exit;
		}
	}
#endif

	if ((options->username && !UTF8_validateString(options->username)) ||
		(options->password && !UTF8_validateString(options->password)))
	{
		rc = MQTTCLIENT_BAD_UTF8_STRING;
		goto exit;
	}

	if (options->struct_version < 2 || options->serverURIcount == 0)
		rc = MQTTClient_connectURI(handle, options, m->serverURI);
	else
	{
		int i;
		
		for (i = 0; i < options->serverURIcount; ++i)
		{
			char* serverURI = options->serverURIs[i];
			
			if (strncmp(URI_TCP, serverURI, strlen(URI_TCP)) == 0)
				serverURI += strlen(URI_TCP);
#if defined(OPENSSL)
			else if (strncmp(URI_SSL, serverURI, strlen(URI_SSL)) == 0)
			{
				serverURI += strlen(URI_SSL);
				m->ssl = 1;
			}
#endif
			if ((rc = MQTTClient_connectURI(handle, options, serverURI)) == MQTTCLIENT_SUCCESS)
				break;
		}	
	}

exit:
	if (m->c->will)
	{
		free(m->c->will);
		m->c->will = NULL;
	}
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_disconnect1(MQTTClient handle, int timeout, int internal, int stop)
{
	MQTTClients* m = handle;
	START_TIME_TYPE start;
	int rc = MQTTCLIENT_SUCCESS;
	int was_connected = 0;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || m->c == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0 && m->c->connect_state == 0)
	{
		rc = MQTTCLIENT_DISCONNECTED;
		goto exit;
	}
	was_connected = m->c->connected; /* should be 1 */
	if (m->c->connected != 0)
	{
		start = MQTTClient_start_clock();
		m->c->connect_state = -2; /* indicate disconnecting */
		while (m->c->inboundMsgs->count > 0 || m->c->outboundMsgs->count > 0)
		{ /* wait for all inflight message flows to finish, up to timeout */
			if (MQTTClient_elapsed(start) >= timeout)
				break;
			Thread_unlock_mutex(mqttclient_mutex);
			MQTTClient_yield();
			Thread_lock_mutex(mqttclient_mutex);
		}
	}

	MQTTClient_closeSession(m->c);

	while (Thread_check_sem(m->connect_sem))
		Thread_wait_sem(m->connect_sem, 100);
	while (Thread_check_sem(m->connack_sem))
		Thread_wait_sem(m->connack_sem, 100);
	while (Thread_check_sem(m->suback_sem))
		Thread_wait_sem(m->suback_sem, 100);
	while (Thread_check_sem(m->unsuback_sem))
		Thread_wait_sem(m->unsuback_sem, 100);
exit:
	if (stop)
		MQTTClient_stop();
	if (internal && m->cl && was_connected)
	{
		Log(TRACE_MIN, -1, "Calling connectionLost for client %s", m->c->clientID);
		Thread_start(connectionLost_call, m);
	}
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_disconnect_internal(MQTTClient handle, int timeout)
{
	return MQTTClient_disconnect1(handle, timeout, 1, 1);
}


void MQTTProtocol_closeSession(Clients* c, int sendwill)
{
	MQTTClient_disconnect_internal((MQTTClient)c->context, 0);
}


int MQTTClient_disconnect(MQTTClient handle, int timeout)
{
	return MQTTClient_disconnect1(handle, timeout, 0, 1);
}


int MQTTClient_isConnected(MQTTClient handle)
{
	MQTTClients* m = handle;
	int rc = 0;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);
	if (m && m->c)
		rc = m->c->connected;
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_subscribeMany(MQTTClient handle, int count, char* const* topic, int* qos)
{
	MQTTClients* m = handle;
	List* topics = ListInitialize();
	List* qoss = ListInitialize();
	int i = 0;
	int rc = MQTTCLIENT_FAILURE;
	int msgid = 0;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || m->c == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTCLIENT_DISCONNECTED;
		goto exit;
	}
	for (i = 0; i < count; i++)
	{
		if (!UTF8_validateString(topic[i]))
		{
			rc = MQTTCLIENT_BAD_UTF8_STRING;
			goto exit;
		}
		
		if(qos[i] < 0 || qos[i] > 2)
		{
			rc = MQTTCLIENT_BAD_QOS;
			goto exit;
		}
	}
	if ((msgid = MQTTProtocol_assignMsgId(m->c)) == 0)
	{
		rc = MQTTCLIENT_MAX_MESSAGES_INFLIGHT;
		goto exit;
	}

	for (i = 0; i < count; i++)
	{
		ListAppend(topics, topic[i], strlen(topic[i]));
		ListAppend(qoss, &qos[i], sizeof(int));
	}

	rc = MQTTProtocol_subscribe(m->c, topics, qoss, msgid);
	ListFreeNoContent(topics);
	ListFreeNoContent(qoss);

	if (rc == TCPSOCKET_COMPLETE)
	{
		MQTTPacket* pack = NULL;

		Thread_unlock_mutex(mqttclient_mutex);
		pack = MQTTClient_waitfor(handle, SUBACK, &rc, 10000L);
		Thread_lock_mutex(mqttclient_mutex);
		if (pack != NULL)
		{
			Suback* sub = (Suback*)pack;	
			ListElement* current = NULL;
			i = 0;
			while (ListNextElement(sub->qoss, &current))
			{
				int* reqqos = (int*)(current->content);
				qos[i++] = *reqqos;
			}	
			rc = MQTTProtocol_handleSubacks(pack, m->c->net.socket);
			m->pack = NULL;
		}
		else
			rc = SOCKET_ERROR;
	}

	if (rc == SOCKET_ERROR)
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_disconnect_internal(handle, 0);
		Thread_lock_mutex(mqttclient_mutex);
	}
	else if (rc == TCPSOCKET_COMPLETE)
		rc = MQTTCLIENT_SUCCESS;

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_subscribe(MQTTClient handle, const char* topic, int qos)
{
	int rc = 0;
	char *const topics[] = {(char*)topic};

	FUNC_ENTRY;
	rc = MQTTClient_subscribeMany(handle, 1, topics, &qos);
	if (qos == MQTT_BAD_SUBSCRIBE) /* addition for MQTT 3.1.1 - error code from subscribe */
		rc = MQTT_BAD_SUBSCRIBE;
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_unsubscribeMany(MQTTClient handle, int count, char* const* topic)
{
	MQTTClients* m = handle;
	List* topics = ListInitialize();
	int i = 0;
	int rc = SOCKET_ERROR;
	int msgid = 0;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || m->c == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTCLIENT_DISCONNECTED;
		goto exit;
	}
	for (i = 0; i < count; i++)
	{
		if (!UTF8_validateString(topic[i]))
		{
			rc = MQTTCLIENT_BAD_UTF8_STRING;
			goto exit;
		}
	}
	if ((msgid = MQTTProtocol_assignMsgId(m->c)) == 0)
	{
		rc = MQTTCLIENT_MAX_MESSAGES_INFLIGHT;
		goto exit;
	}

	for (i = 0; i < count; i++)
		ListAppend(topics, topic[i], strlen(topic[i]));
	rc = MQTTProtocol_unsubscribe(m->c, topics, msgid);
	ListFreeNoContent(topics);

	if (rc == TCPSOCKET_COMPLETE)
	{
		MQTTPacket* pack = NULL;

		Thread_unlock_mutex(mqttclient_mutex);
		pack = MQTTClient_waitfor(handle, UNSUBACK, &rc, 10000L);
		Thread_lock_mutex(mqttclient_mutex);
		if (pack != NULL)
		{
			rc = MQTTProtocol_handleUnsubacks(pack, m->c->net.socket);
			m->pack = NULL;
		}
		else
			rc = SOCKET_ERROR;
	}

	if (rc == SOCKET_ERROR)
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_disconnect_internal(handle, 0);
		Thread_lock_mutex(mqttclient_mutex);
	}

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_unsubscribe(MQTTClient handle, const char* topic)
{
	int rc = 0;
	char *const topics[] = {(char*)topic};
	FUNC_ENTRY;
	rc = MQTTClient_unsubscribeMany(handle, 1, topics);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_publish(MQTTClient handle, const char* topicName, int payloadlen, void* payload,
							 int qos, int retained, MQTTClient_deliveryToken* deliveryToken)
{
	int rc = MQTTCLIENT_SUCCESS;
	MQTTClients* m = handle;
	Messages* msg = NULL;
	Publish* p = NULL;
	int blocked = 0;
	int msgid = 0;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || m->c == NULL)
		rc = MQTTCLIENT_FAILURE;
	else if (m->c->connected == 0)
		rc = MQTTCLIENT_DISCONNECTED;
	else if (!UTF8_validateString(topicName))
		rc = MQTTCLIENT_BAD_UTF8_STRING;
	if (rc != MQTTCLIENT_SUCCESS)
		goto exit;

	/* If outbound queue is full, block until it is not */
	while (m->c->outboundMsgs->count >= m->c->maxInflightMessages || 
         Socket_noPendingWrites(m->c->net.socket) == 0) /* wait until the socket is free of large packets being written */
	{
		if (blocked == 0)
		{
			blocked = 1;
			Log(TRACE_MIN, -1, "Blocking publish on queue full for client %s", m->c->clientID);
		}
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_yield();
		Thread_lock_mutex(mqttclient_mutex);
		if (m->c->connected == 0)
		{
			rc = MQTTCLIENT_FAILURE;
			goto exit;
		}
	}
	if (blocked == 1)
		Log(TRACE_MIN, -1, "Resuming publish now queue not full for client %s", m->c->clientID);
	if (qos > 0 && (msgid = MQTTProtocol_assignMsgId(m->c)) == 0)
	{	/* this should never happen as we've waited for spaces in the queue */
		rc = MQTTCLIENT_MAX_MESSAGES_INFLIGHT;
		goto exit;
	}

	p = malloc(sizeof(Publish));

	p->payload = payload;
	p->payloadlen = payloadlen;
	p->topic = (char*)topicName;
	p->msgId = msgid;

	rc = MQTTProtocol_startPublish(m->c, p, qos, retained, &msg);

	/* If the packet was partially written to the socket, wait for it to complete.
	 * However, if the client is disconnected during this time and qos is not 0, still return success, as
	 * the packet has already been written to persistence and assigned a message id so will
	 * be sent when the client next connects.
	 */
	if (rc == TCPSOCKET_INTERRUPTED)
	{
		while (m->c->connected == 1 && SocketBuffer_getWrite(m->c->net.socket))
		{
			Thread_unlock_mutex(mqttclient_mutex);
			MQTTClient_yield();
			Thread_lock_mutex(mqttclient_mutex);
		}
		rc = (qos > 0 || m->c->connected == 1) ? MQTTCLIENT_SUCCESS : MQTTCLIENT_FAILURE;
	}

	if (deliveryToken && qos > 0)
		*deliveryToken = msg->msgid;

	free(p);

	if (rc == SOCKET_ERROR)
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_disconnect_internal(handle, 0);
		Thread_lock_mutex(mqttclient_mutex);
		/* Return success for qos > 0 as the send will be retried automatically */
		rc = (qos > 0) ? MQTTCLIENT_SUCCESS : MQTTCLIENT_FAILURE;
	}

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}



int MQTTClient_publishMessage(MQTTClient handle, const char* topicName, MQTTClient_message* message,
															 MQTTClient_deliveryToken* deliveryToken)
{
	int rc = MQTTCLIENT_SUCCESS;

	FUNC_ENTRY;
	if (message == NULL)
	{
		rc = MQTTCLIENT_NULL_PARAMETER;
		goto exit;
	}

	if (strncmp(message->struct_id, "MQTM", 4) != 0 || message->struct_version != 0)
	{
		rc = MQTTCLIENT_BAD_STRUCTURE;
		goto exit;
	}

	rc = MQTTClient_publish(handle, topicName, message->payloadlen, message->payload,
								message->qos, message->retained, deliveryToken);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTClient_retry(void)
{
	time_t now;

	FUNC_ENTRY;
	time(&(now));
	if (difftime(now, last) > 5)
	{
		time(&(last));
		MQTTProtocol_keepalive(now);
		MQTTProtocol_retry(now, 1, 0);
	}
	else
		MQTTProtocol_retry(now, 0, 0);
	FUNC_EXIT;
}


MQTTPacket* MQTTClient_cycle(int* sock, unsigned long timeout, int* rc)
{
	struct timeval tp = {0L, 0L};
	static Ack ack;
	MQTTPacket* pack = NULL;

	FUNC_ENTRY;
	if (timeout > 0L)
	{
		tp.tv_sec = timeout / 1000;
		tp.tv_usec = (timeout % 1000) * 1000; /* this field is microseconds! */
	}

#if defined(OPENSSL)
	if ((*sock = SSLSocket_getPendingRead()) == -1)
	{
		/* 0 from getReadySocket indicates no work to do, -1 == error, but can happen normally */
#endif
		*sock = Socket_getReadySocket(0, &tp);
#if defined(OPENSSL)
	}
#endif
	Thread_lock_mutex(mqttclient_mutex);
	if (*sock > 0)
	{
		MQTTClients* m = NULL;
		if (ListFindItem(handles, sock, clientSockCompare) != NULL)
			m = (MQTTClient)(handles->current->content);
		if (m != NULL)
		{
			if (m->c->connect_state == 1 || m->c->connect_state == 2)
				*rc = 0;  /* waiting for connect state to clear */
			else
			{
				pack = MQTTPacket_Factory(&m->c->net, rc);
				if (*rc == TCPSOCKET_INTERRUPTED)
					*rc = 0;
			}
		}
		if (pack)
		{
			int freed = 1;

			/* Note that these handle... functions free the packet structure that they are dealing with */
			if (pack->header.bits.type == PUBLISH)
				*rc = MQTTProtocol_handlePublishes(pack, *sock);
			else if (pack->header.bits.type == PUBACK || pack->header.bits.type == PUBCOMP)
			{
				int msgid;

				ack = (pack->header.bits.type == PUBCOMP) ? *(Pubcomp*)pack : *(Puback*)pack;
				msgid = ack.msgId;
				*rc = (pack->header.bits.type == PUBCOMP) ?
						MQTTProtocol_handlePubcomps(pack, *sock) : MQTTProtocol_handlePubacks(pack, *sock);
				if (m && m->dc)
				{
					Log(TRACE_MIN, -1, "Calling deliveryComplete for client %s, msgid %d", m->c->clientID, msgid);
					(*(m->dc))(m->context, msgid);
				}
			}
			else if (pack->header.bits.type == PUBREC)
				*rc = MQTTProtocol_handlePubrecs(pack, *sock);
			else if (pack->header.bits.type == PUBREL)
				*rc = MQTTProtocol_handlePubrels(pack, *sock);
			else if (pack->header.bits.type == PINGRESP)
				*rc = MQTTProtocol_handlePingresps(pack, *sock);
			else
				freed = 0;
			if (freed)
				pack = NULL;
		}
	}
	MQTTClient_retry();
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(*rc);
	return pack;
}


MQTTPacket* MQTTClient_waitfor(MQTTClient handle, int packet_type, int* rc, long timeout)
{
	MQTTPacket* pack = NULL;
	MQTTClients* m = handle;
	START_TIME_TYPE start = MQTTClient_start_clock();

	FUNC_ENTRY;
	if (((MQTTClients*)handle) == NULL)
	{
		*rc = MQTTCLIENT_FAILURE;
		goto exit;
	}

	if (running)
	{
		if (packet_type == CONNECT)
		{
			if ((*rc = Thread_wait_sem(m->connect_sem, timeout)) == 0)
				*rc = m->rc;
		}
		else if (packet_type == CONNACK)
			*rc = Thread_wait_sem(m->connack_sem, timeout);
		else if (packet_type == SUBACK)
			*rc = Thread_wait_sem(m->suback_sem, timeout);
		else if (packet_type == UNSUBACK)
			*rc = Thread_wait_sem(m->unsuback_sem, timeout);
		if (*rc == 0 && packet_type != CONNECT && m->pack == NULL)
			Log(LOG_ERROR, -1, "waitfor unexpectedly is NULL for client %s, packet_type %d, timeout %ld", m->c->clientID, packet_type, timeout);
		pack = m->pack;
	}
	else
	{
		*rc = TCPSOCKET_COMPLETE;
		while (1)
		{
			int sock = -1;
			pack = MQTTClient_cycle(&sock, 100L, rc);
			if (sock == m->c->net.socket)
			{
				if (*rc == SOCKET_ERROR)
					break;
				if (pack && (pack->header.bits.type == packet_type))
					break;
				if (m->c->connect_state == 1)
				{
					int error;
					socklen_t len = sizeof(error);

					if ((*rc = getsockopt(m->c->net.socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len)) == 0)
						*rc = error;
					break;
				}
#if defined(OPENSSL)
				else if (m->c->connect_state == 2)
				{
					*rc = SSLSocket_connect(m->c->net.ssl, sock);
					if (*rc == SSL_FATAL)
						break;
					else if (*rc == 1) /* rc == 1 means SSL connect has finished and succeeded */
					{
						if (!m->c->cleansession && m->c->session == NULL)
							m->c->session = SSL_get1_session(m->c->net.ssl);
						break;
					}
				}
#endif
				else if (m->c->connect_state == 3)
				{
					int error;
					socklen_t len = sizeof(error);

					if (getsockopt(m->c->net.socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) == 0)
					{
						if (error)
						{
							*rc = error;
							break;
						}
					}
				}
			}
			if (MQTTClient_elapsed(start) > timeout)
			{
				pack = NULL;
				break;
			}
		}
	}

exit:
	FUNC_EXIT_RC(*rc);
	return pack;
}


int MQTTClient_receive(MQTTClient handle, char** topicName, int* topicLen, MQTTClient_message** message,
											 unsigned long timeout)
{
	int rc = TCPSOCKET_COMPLETE;
	START_TIME_TYPE start = MQTTClient_start_clock();
	unsigned long elapsed = 0L;
	MQTTClients* m = handle;

	FUNC_ENTRY;
	if (m == NULL || m->c == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTCLIENT_DISCONNECTED;
		goto exit;
	}

	*topicName = NULL;
	*message = NULL;

	/* if there is already a message waiting, don't hang around but still do some packet handling */
	if (m->c->messageQueue->count > 0)
		timeout = 0L;

	elapsed = MQTTClient_elapsed(start);
	do
	{
		int sock = 0;
		MQTTClient_cycle(&sock, (timeout > elapsed) ? timeout - elapsed : 0L, &rc);
		
		if (rc == SOCKET_ERROR)
		{
			if (ListFindItem(handles, &sock, clientSockCompare) && 	/* find client corresponding to socket */
			  (MQTTClient)(handles->current->content) == handle)
				break; /* there was an error on the socket we are interested in */
		}
		elapsed = MQTTClient_elapsed(start);
	}
	while (elapsed < timeout && m->c->messageQueue->count == 0);

	if (m->c->messageQueue->count > 0)
		rc = MQTTClient_deliverMessage(rc, m, topicName, topicLen, message);

	if (rc == SOCKET_ERROR)
		MQTTClient_disconnect_internal(handle, 0);

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTClient_yield(void)
{
	START_TIME_TYPE start = MQTTClient_start_clock();
	unsigned long elapsed = 0L;
	unsigned long timeout = 100L;
	int rc = 0;

	FUNC_ENTRY;
	if (running)
	{
		MQTTClient_sleep(timeout);
		goto exit;
	}

	elapsed = MQTTClient_elapsed(start);
	do
	{
		int sock = -1;
		MQTTClient_cycle(&sock, (timeout > elapsed) ? timeout - elapsed : 0L, &rc);
		if (rc == SOCKET_ERROR && ListFindItem(handles, &sock, clientSockCompare))
		{
			MQTTClients* m = (MQTTClient)(handles->current->content);
			if (m->c->connect_state != -2)
				MQTTClient_disconnect_internal(m, 0);
		}
		elapsed = MQTTClient_elapsed(start);
	}
	while (elapsed < timeout);
exit:
	FUNC_EXIT;
}


int pubCompare(void* a, void* b)
{
	Messages* msg = (Messages*)a;
	return msg->publish == (Publications*)b;
}


int MQTTClient_waitForCompletion(MQTTClient handle, MQTTClient_deliveryToken mdt, unsigned long timeout)
{
	int rc = MQTTCLIENT_FAILURE;
	START_TIME_TYPE start = MQTTClient_start_clock();
	unsigned long elapsed = 0L;
	MQTTClients* m = handle;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL || m->c == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTCLIENT_DISCONNECTED;
		goto exit;
	}

	if (ListFindItem(m->c->outboundMsgs, &mdt, messageIDCompare) == NULL)
	{
		rc = MQTTCLIENT_SUCCESS; /* well we couldn't find it */
		goto exit;
	}

	elapsed = MQTTClient_elapsed(start);
	while (elapsed < timeout)
	{
		Thread_unlock_mutex(mqttclient_mutex);
		MQTTClient_yield();
		Thread_lock_mutex(mqttclient_mutex);
		if (ListFindItem(m->c->outboundMsgs, &mdt, messageIDCompare) == NULL)
		{
			rc = MQTTCLIENT_SUCCESS; /* well we couldn't find it */
			goto exit;
		}
		elapsed = MQTTClient_elapsed(start);
	}

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTClient_getPendingDeliveryTokens(MQTTClient handle, MQTTClient_deliveryToken **tokens)
{
	int rc = MQTTCLIENT_SUCCESS;
	MQTTClients* m = handle;
	*tokens = NULL;

	FUNC_ENTRY;
	Thread_lock_mutex(mqttclient_mutex);

	if (m == NULL)
	{
		rc = MQTTCLIENT_FAILURE;
		goto exit;
	}

	if (m->c && m->c->outboundMsgs->count > 0)
	{
		ListElement* current = NULL;
		int count = 0;

		*tokens = malloc(sizeof(MQTTClient_deliveryToken) * (m->c->outboundMsgs->count + 1));
		/*Heap_unlink(__FILE__, __LINE__, *tokens);*/
		while (ListNextElement(m->c->outboundMsgs, &current))
		{
			Messages* m = (Messages*)(current->content);
			(*tokens)[count++] = m->msgid;
		}
		(*tokens)[count] = -1;
	}

exit:
	Thread_unlock_mutex(mqttclient_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}

MQTTClient_nameValue* MQTTClient_getVersionInfo()
{
	#define MAX_INFO_STRINGS 8
	static MQTTClient_nameValue libinfo[MAX_INFO_STRINGS + 1];
	int i = 0;

	libinfo[i].name = "Product name";
	libinfo[i++].value = "Paho Synchronous MQTT C Client Library";

	libinfo[i].name = "Version";
	libinfo[i++].value = CLIENT_VERSION;

	libinfo[i].name = "Build level";
	libinfo[i++].value = BUILD_TIMESTAMP;
#if defined(OPENSSL)
	libinfo[i].name = "OpenSSL version";
	libinfo[i++].value = SSLeay_version(SSLEAY_VERSION);

	libinfo[i].name = "OpenSSL flags";
	libinfo[i++].value = SSLeay_version(SSLEAY_CFLAGS);

	libinfo[i].name = "OpenSSL build timestamp";
	libinfo[i++].value = SSLeay_version(SSLEAY_BUILT_ON);

	libinfo[i].name = "OpenSSL platform";
	libinfo[i++].value = SSLeay_version(SSLEAY_PLATFORM);

	libinfo[i].name = "OpenSSL directory";
	libinfo[i++].value = SSLeay_version(SSLEAY_DIR);
#endif
	libinfo[i].name = NULL;
	libinfo[i].value = NULL;
	return libinfo;
}


/**
 * See if any pending writes have been completed, and cleanup if so.
 * Cleaning up means removing any publication data that was stored because the write did
 * not originally complete.
 */
void MQTTProtocol_checkPendingWrites()
{
	FUNC_ENTRY;
	if (state.pending_writes.count > 0)
	{
		ListElement* le = state.pending_writes.first;
		while (le)
		{
			if (Socket_noPendingWrites(((pending_write*)(le->content))->socket))
			{
				MQTTProtocol_removePublication(((pending_write*)(le->content))->p);
				state.pending_writes.current = le;
				ListRemove(&(state.pending_writes), le->content); /* does NextElement itself */
				le = state.pending_writes.current;
			}
			else
				ListNextElement(&(state.pending_writes), &le);
		}
	}
	FUNC_EXIT;
}


void MQTTClient_writeComplete(int socket)				
{
	ListElement* found = NULL;
	
	FUNC_ENTRY;
	/* a partial write is now complete for a socket - this will be on a publish*/
	
	MQTTProtocol_checkPendingWrites();
	
	/* find the client using this socket */
	if ((found = ListFindItem(handles, &socket, clientSockCompare)) != NULL)
	{
		MQTTClients* m = (MQTTClients*)(found->content);
		
		time(&(m->c->net.lastSent));
	}
	FUNC_EXIT;
}
