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
 *    Ian Craggs - initial implementation and documentation
 *    Ian Craggs, Allan Stockdill-Mander - SSL support
 *    Ian Craggs - multiple server connection support
 *    Ian Craggs - fix for bug 413429 - connectionLost not called
 *    Ian Craggs - fix for bug 415042 - using already freed structure
 *    Ian Craggs - fix for bug 419233 - mutexes not reporting errors
 *    Ian Craggs - fix for bug 420851
 *    Ian Craggs - fix for bug 432903 - queue persistence
 *    Ian Craggs - MQTT 3.1.1 support
 *    Rong Xiang, Ian Craggs - C++ compatibility
 *    Ian Craggs - fix for bug 442400: reconnecting after network cable unplugged
 *    Ian Craggs - fix for bug 444934 - incorrect free in freeCommand1
 *    Ian Craggs - fix for bug 445891 - assigning msgid is not thread safe
 *******************************************************************************/
/**
 * @file
 * \brief Asynchronous API implementation
 *
 */

#define _GNU_SOURCE /* for pthread_mutexattr_settype */
#include <stdlib.h>
#if !defined(WIN32) && !defined(WIN64)
	#include <sys/time.h>
#endif

#if !defined(NO_PERSISTENCE)
#include "MQTTPersistence.h"
#endif
#include "MQTTAsync.h"
#include "utf-8.h"
#include "MQTTProtocol.h"
#include "MQTTProtocolOut.h"
#include "Thread.h"
#include "SocketBuffer.h"
#include "StackTrace.h"
#include "Heap.h"

#define URI_TCP "tcp://"

#define BUILD_TIMESTAMP "##MQTTCLIENT_BUILD_TAG##"
#define CLIENT_VERSION  "##MQTTCLIENT_VERSION_TAG##"

char* client_timestamp_eye = "MQTTAsyncV3_Timestamp " BUILD_TIMESTAMP;
char* client_version_eye = "MQTTAsyncV3_Version " CLIENT_VERSION;

extern Sockets s;

static ClientStates ClientState =
{
	CLIENT_VERSION, /* version */
	NULL /* client list */	
};

ClientStates* bstate = &ClientState;

MQTTProtocol state;

enum MQTTAsync_threadStates
{
	STOPPED, STARTING, RUNNING, STOPPING
};

enum MQTTAsync_threadStates sendThread_state = STOPPED;
enum MQTTAsync_threadStates receiveThread_state = STOPPED;
static thread_id_type sendThread_id = 0,
					receiveThread_id = 0;

#if defined(WIN32) || defined(WIN64)
static mutex_type mqttasync_mutex = NULL;
static mutex_type mqttcommand_mutex = NULL;
static sem_type send_sem = NULL;
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
			if (mqttasync_mutex == NULL)
			{
				mqttasync_mutex = CreateMutex(NULL, 0, NULL);
				mqttcommand_mutex = CreateMutex(NULL, 0, NULL);
				send_sem = CreateEvent(
		        NULL,               /* default security attributes */
		        FALSE,              /* manual-reset event? */
		        FALSE,              /* initial state is nonsignaled */
		        NULL                /* object name */
		        );
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
static pthread_mutex_t mqttasync_mutex_store = PTHREAD_MUTEX_INITIALIZER;
static mutex_type mqttasync_mutex = &mqttasync_mutex_store;
static pthread_mutex_t mqttcommand_mutex_store = PTHREAD_MUTEX_INITIALIZER;
static mutex_type mqttcommand_mutex = &mqttcommand_mutex_store;
static cond_type_struct send_cond_store = { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER };
static cond_type send_cond = &send_cond_store;

void MQTTAsync_init()
{
	pthread_mutexattr_t attr;
	int rc;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	if ((rc = pthread_mutex_init(mqttasync_mutex, &attr)) != 0)
		printf("MQTTAsync: error %d initializing async_mutex\n", rc);
	if ((rc = pthread_mutex_init(mqttcommand_mutex, &attr)) != 0)
		printf("MQTTAsync: error %d initializing command_mutex\n", rc);
}

#define WINAPI
#endif

static volatile int initialized = 0;
static List* handles = NULL;
static int tostop = 0;
static List* commands = NULL;

MQTTPacket* MQTTAsync_cycle(int* sock, unsigned long timeout, int* rc);
int MQTTAsync_cleanSession(Clients* client);
void MQTTAsync_stop();
int MQTTAsync_disconnect_internal(MQTTAsync handle, int timeout);
void MQTTAsync_closeOnly(Clients* client);
void MQTTAsync_closeSession(Clients* client);
void MQTTProtocol_closeSession(Clients* client, int sendwill);
void MQTTAsync_writeComplete(int socket);

#if defined(WIN32) || defined(WIN64)
#define START_TIME_TYPE DWORD
START_TIME_TYPE MQTTAsync_start_clock(void)
{
	return GetTickCount();
}
#elif defined(AIX)
#define START_TIME_TYPE struct timespec
START_TIME_TYPE MQTTAsync_start_clock(void)
{
	static struct timespec start;
	clock_gettime(CLOCK_REALTIME, &start);
	return start;
}
#else
#define START_TIME_TYPE struct timeval
START_TIME_TYPE MQTTAsync_start_clock(void)
{
	static struct timeval start;
	gettimeofday(&start, NULL);
	return start;
}
#endif


#if defined(WIN32) || defined(WIN64)
long MQTTAsync_elapsed(DWORD milliseconds)
{
	return GetTickCount() - milliseconds;
}
#elif defined(AIX)
#define assert(a)
long MQTTAsync_elapsed(struct timespec start)
{
	struct timespec now, res;

	clock_gettime(CLOCK_REALTIME, &now);
	ntimersub(now, start, res);
	return (res.tv_sec)*1000L + (res.tv_nsec)/1000000L;
}
#else
long MQTTAsync_elapsed(struct timeval start)
{
	struct timeval now, res;

	gettimeofday(&now, NULL);
	timersub(&now, &start, &res);
	return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}
#endif


typedef struct
{
	MQTTAsync_message* msg;
	char* topicName;
	int topicLen;
	unsigned int seqno; /* only used on restore */
} qEntry;

typedef struct
{
	int type;
	MQTTAsync_onSuccess* onSuccess;
	MQTTAsync_onFailure* onFailure;
	MQTTAsync_token token;
	void* context;
	START_TIME_TYPE start_time;
	union
	{
		struct
		{
			int count;
			char** topics;
			int* qoss;
		} sub;
		struct
		{
			int count;
			char** topics;
		} unsub;
		struct
		{
			char* destinationName;
			int payloadlen;
			void* payload;
			int qos;
			int retained;
		} pub;
		struct
		{
			int internal;
			int timeout;
		} dis;
		struct
		{
			int timeout;
			int serverURIcount;
			char** serverURIs;
			int currentURI;
			int MQTTVersion; /**< current MQTT version being used to connect */
		} conn;
	} details;
} MQTTAsync_command;


typedef struct MQTTAsync_struct
{
	char* serverURI;
	int ssl;
	Clients* c;
	
	/* "Global", to the client, callback definitions */
	MQTTAsync_connectionLost* cl;
	MQTTAsync_messageArrived* ma;
	MQTTAsync_deliveryComplete* dc;
	void* context; /* the context to be associated with the main callbacks*/
	
	MQTTAsync_command connect;				/* Connect operation properties */
	MQTTAsync_command disconnect;			/* Disconnect operation properties */
	MQTTAsync_command* pending_write;       /* Is there a socket write pending? */
	
	List* responses;
	unsigned int command_seqno;						

	MQTTPacket* pack;

} MQTTAsyncs;


typedef struct
{
	MQTTAsync_command command;
	MQTTAsyncs* client;
	unsigned int seqno; /* only used on restore */
} MQTTAsync_queuedCommand;

void MQTTAsync_freeCommand(MQTTAsync_queuedCommand *command);
void MQTTAsync_freeCommand1(MQTTAsync_queuedCommand *command);
int MQTTAsync_deliverMessage(MQTTAsyncs* m, char* topicName, size_t topicLen, MQTTAsync_message* mm);
#if !defined(NO_PERSISTENCE)
int MQTTAsync_restoreCommands(MQTTAsyncs* client);
#endif

void MQTTAsync_sleep(long milliseconds)
{
	FUNC_ENTRY;
#if defined(WIN32) || defined(WIN64)
	Sleep(milliseconds);
#else
	usleep(milliseconds*1000);
#endif
	FUNC_EXIT;
}


/**
 * List callback function for comparing clients by socket
 * @param a first integer value
 * @param b second integer value
 * @return boolean indicating whether a and b are equal
 */
int clientSockCompare(void* a, void* b)
{
	MQTTAsyncs* m = (MQTTAsyncs*)a;
	return m->c->net.socket == *(int*)b;
}


void MQTTAsync_lock_mutex(mutex_type amutex)
{
	int rc = Thread_lock_mutex(amutex);
	if (rc != 0)
		Log(LOG_ERROR, 0, "Error %s locking mutex", strerror(rc));
}


void MQTTAsync_unlock_mutex(mutex_type amutex)
{
	int rc = Thread_unlock_mutex(amutex);
	if (rc != 0)
		Log(LOG_ERROR, 0, "Error %s unlocking mutex", strerror(rc));
}


int MQTTAsync_checkConn(MQTTAsync_command* command, MQTTAsyncs* client)
{
  int rc;

  FUNC_ENTRY;
  rc = command->details.conn.currentURI < command->details.conn.serverURIcount ||
      (command->details.conn.MQTTVersion == 4 && client->c->MQTTVersion == MQTTVERSION_DEFAULT);
  FUNC_EXIT_RC(rc);
  return rc;
}


int MQTTAsync_create(MQTTAsync* handle, const char* serverURI, const char* clientId,
		int persistence_type, void* persistence_context)
{
	int rc = 0;
	MQTTAsyncs *m = NULL;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (serverURI == NULL || clientId == NULL)
	{
		rc = MQTTASYNC_NULL_PARAMETER;
		goto exit;
	}

	if (!UTF8_validateString(clientId))
	{
		rc = MQTTASYNC_BAD_UTF8_STRING;
		goto exit;
	}

	if (!initialized)
	{
		#if defined(HEAP_H)
			Heap_initialize();
		#endif
		Log_initialize((Log_nameValue*)MQTTAsync_getVersionInfo());
		bstate->clients = ListInitialize();
		Socket_outInitialize();
		Socket_setWriteCompleteCallback(MQTTAsync_writeComplete);
		handles = ListInitialize();
		commands = ListInitialize();
#if defined(NS_ENABLE_SSL)
		SSLSocket_initialize();
#endif
		initialized = 1;
	}
	m = malloc(sizeof(MQTTAsyncs));
	*handle = m;
	memset(m, '\0', sizeof(MQTTAsyncs));
	if (strncmp(URI_TCP, serverURI, strlen(URI_TCP)) == 0)
		serverURI += strlen(URI_TCP);
#if defined(NS_ENABLE_SSL)
	else if (strncmp(URI_SSL, serverURI, strlen(URI_SSL)) == 0)
	{
		serverURI += strlen(URI_SSL);
		m->ssl = 1;
	}
#endif
	m->serverURI = MQTTStrdup(serverURI);
	m->responses = ListInitialize();
	ListAppend(handles, m, sizeof(MQTTAsyncs));

	m->c = malloc(sizeof(Clients));
	memset(m->c, '\0', sizeof(Clients));
	m->c->context = m;
	m->c->outboundMsgs = ListInitialize();
	m->c->inboundMsgs = ListInitialize();
	m->c->messageQueue = ListInitialize();
	m->c->clientID = MQTTStrdup(clientId);

#if !defined(NO_PERSISTENCE)
	rc = MQTTPersistence_create(&(m->c->persistence), persistence_type, persistence_context);
	if (rc == 0)
	{
		rc = MQTTPersistence_initialize(m->c, m->serverURI);
		if (rc == 0)
		{
			MQTTAsync_restoreCommands(m);
			MQTTPersistence_restoreMessageQueue(m->c);
		}
	}
#endif
	ListAppend(bstate->clients, m->c, sizeof(Clients) + 3*sizeof(List));

exit:
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTAsync_terminate(void)
{
	FUNC_ENTRY;
	MQTTAsync_stop();
	if (initialized)
	{
		ListElement* elem = NULL;
		ListFree(bstate->clients);
		ListFree(handles);
		while (ListNextElement(commands, &elem))
			MQTTAsync_freeCommand1((MQTTAsync_queuedCommand*)(elem->content));
		ListFree(commands);
		handles = NULL;
		Socket_outTerminate();
#if defined(NS_ENABLE_SSL)
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


#if !defined(NO_PERSISTENCE)
int MQTTAsync_unpersistCommand(MQTTAsync_queuedCommand* qcmd)
{
	int rc = 0;
	char key[PERSISTENCE_MAX_KEY_LENGTH + 1];
	
	FUNC_ENTRY;
	sprintf(key, "%s%d", PERSISTENCE_COMMAND_KEY, qcmd->seqno);
	if ((rc = qcmd->client->c->persistence->premove(qcmd->client->c->phandle, key)) != 0)
		Log(LOG_ERROR, 0, "Error %d removing command from persistence", rc);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_persistCommand(MQTTAsync_queuedCommand* qcmd)
{
	int rc = 0;
	MQTTAsyncs* aclient = qcmd->client;
	MQTTAsync_command* command = &qcmd->command;
	int* lens = NULL;
	void** bufs = NULL;
	int bufindex = 0, i, nbufs = 0;
	char key[PERSISTENCE_MAX_KEY_LENGTH + 1];
	
	FUNC_ENTRY;
	switch (command->type)
	{
		case SUBSCRIBE:
			nbufs = 3 + (command->details.sub.count * 2);
				
			lens = (int*)malloc(nbufs * sizeof(int));
			bufs = malloc(nbufs * sizeof(char *));
				
			bufs[bufindex] = &command->type;
			lens[bufindex++] = sizeof(command->type);

			bufs[bufindex] = &command->token;
			lens[bufindex++] = sizeof(command->token);
				
			bufs[bufindex] = &command->details.sub.count;
			lens[bufindex++] = sizeof(command->details.sub.count);		
				
			for (i = 0; i < command->details.sub.count; ++i)
			{
				bufs[bufindex] = command->details.sub.topics[i];
				lens[bufindex++] = strlen(command->details.sub.topics[i]) + 1;	
				bufs[bufindex] = &command->details.sub.qoss[i];
				lens[bufindex++] = sizeof(command->details.sub.qoss[i]);
			}			
			sprintf(key, "%s%d", PERSISTENCE_COMMAND_KEY, ++aclient->command_seqno);
			break;
				
		case UNSUBSCRIBE:
			nbufs = 3 + command->details.unsub.count;
			
			lens = (int*)malloc(nbufs * sizeof(int));
			bufs = malloc(nbufs * sizeof(char *));
				
			bufs[bufindex] = &command->type;
			lens[bufindex++] = sizeof(command->type);

			bufs[bufindex] = &command->token;
			lens[bufindex++] = sizeof(command->token);
				
			bufs[bufindex] = &command->details.unsub.count;
			lens[bufindex++] = sizeof(command->details.unsub.count);		
				
			for (i = 0; i < command->details.unsub.count; ++i)
			{
				bufs[bufindex] = command->details.unsub.topics[i];
				lens[bufindex++] = strlen(command->details.unsub.topics[i]) + 1;	
			}	
			sprintf(key, "%s%d", PERSISTENCE_COMMAND_KEY, ++aclient->command_seqno);
			break;	
			
		case PUBLISH:
			nbufs = 7;
				
			lens = (int*)malloc(nbufs * sizeof(int));
			bufs = malloc(nbufs * sizeof(char *));
				
			bufs[bufindex] = &command->type;
			lens[bufindex++] = sizeof(command->type);

			bufs[bufindex] = &command->token;
			lens[bufindex++] = sizeof(command->token);
				
			bufs[bufindex] = command->details.pub.destinationName;
			lens[bufindex++] = strlen(command->details.pub.destinationName) + 1;
				
			bufs[bufindex] = &command->details.pub.payloadlen;
			lens[bufindex++] = sizeof(command->details.pub.payloadlen);
				
			bufs[bufindex] = command->details.pub.payload;
			lens[bufindex++] = command->details.pub.payloadlen;
				
			bufs[bufindex] = &command->details.pub.qos;
			lens[bufindex++] = sizeof(command->details.pub.qos);
				
			bufs[bufindex] = &command->details.pub.retained;
			lens[bufindex++] = sizeof(command->details.pub.retained);				
		
			sprintf(key, "%s%d", PERSISTENCE_COMMAND_KEY, ++aclient->command_seqno);
			break;	
	}
	if (nbufs > 0)
	{
		if ((rc = aclient->c->persistence->pput(aclient->c->phandle, key, nbufs, (char**)bufs, lens)) != 0)
			Log(LOG_ERROR, 0, "Error persisting command, rc %d", rc);
		qcmd->seqno = aclient->command_seqno;
	}
	if (lens)
		free(lens);
	if (bufs)
		free(bufs);
	FUNC_EXIT_RC(rc);
	return rc;
}


MQTTAsync_queuedCommand* MQTTAsync_restoreCommand(char* buffer, int buflen)
{
	MQTTAsync_command* command = NULL;
	MQTTAsync_queuedCommand* qcommand = NULL;
	char* ptr = buffer;
	int i, data_size;
	
	FUNC_ENTRY;
	qcommand = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(qcommand, '\0', sizeof(MQTTAsync_queuedCommand));
	command = &qcommand->command;
	
	command->type = *(int*)ptr;
	ptr += sizeof(int);
	
	command->token = *(MQTTAsync_token*)ptr;
	ptr += sizeof(MQTTAsync_token);

	switch (command->type)
	{
		case SUBSCRIBE:
			command->details.sub.count = *(int*)ptr;
			ptr += sizeof(int);
				
			for (i = 0; i < command->details.sub.count; ++i)
			{
				data_size = strlen(ptr) + 1;
				
				command->details.sub.topics[i] = malloc(data_size);
				strcpy(command->details.sub.topics[i], ptr);
				ptr += data_size;
				
				command->details.sub.qoss[i] = *(int*)ptr;
				ptr += sizeof(int);
			}			
			break;
			
		case UNSUBSCRIBE:
			command->details.sub.count = *(int*)ptr;
			ptr += sizeof(int);
				
			for (i = 0; i < command->details.unsub.count; ++i)
			{
				int data_size = strlen(ptr) + 1;
				
				command->details.unsub.topics[i] = malloc(data_size);
				strcpy(command->details.unsub.topics[i], ptr);
				ptr += data_size;
			}			
			break;
			
		case PUBLISH:
			data_size = strlen(ptr) + 1;	
			command->details.pub.destinationName = malloc(data_size);
			strcpy(command->details.pub.destinationName, ptr);
			ptr += data_size;
			
			command->details.pub.payloadlen = *(int*)ptr;
			ptr += sizeof(int);
			
			data_size = command->details.pub.payloadlen;
			command->details.pub.payload = malloc(data_size);
			memcpy(command->details.pub.payload, ptr, data_size);
			ptr += data_size;
			
			command->details.pub.qos = *(int*)ptr;
			ptr += sizeof(int);
			
			command->details.pub.retained = *(int*)ptr;
			ptr += sizeof(int);	
			break;
			
		default:
			free(qcommand);
			qcommand = NULL;
			
	}
	
	FUNC_EXIT;
	return qcommand;
}


void MQTTAsync_insertInOrder(List* list, void* content, int size)
{
	ListElement* index = NULL;
	ListElement* current = NULL;

	FUNC_ENTRY;
	while (ListNextElement(list, &current) != NULL && index == NULL)
	{
		if (((MQTTAsync_queuedCommand*)content)->seqno < ((MQTTAsync_queuedCommand*)current->content)->seqno)
			index = current;
	}

	ListInsert(list, content, size, index);
	FUNC_EXIT;
}


int MQTTAsync_restoreCommands(MQTTAsyncs* client)
{
	int rc = 0;
	char **msgkeys;
	int nkeys;
	int i = 0;
	Clients* c = client->c;
	int commands_restored = 0;

	FUNC_ENTRY;
	if (c->persistence && (rc = c->persistence->pkeys(c->phandle, &msgkeys, &nkeys)) == 0)
	{
		while (rc == 0 && i < nkeys)
		{
			char *buffer = NULL;
			int buflen;
					
			if (strncmp(msgkeys[i], PERSISTENCE_COMMAND_KEY, strlen(PERSISTENCE_COMMAND_KEY)) != 0)
				;
			else if ((rc = c->persistence->pget(c->phandle, msgkeys[i], &buffer, &buflen)) == 0)
			{
				MQTTAsync_queuedCommand* cmd = MQTTAsync_restoreCommand(buffer, buflen);
				
				if (cmd)
				{
					cmd->client = client;	
					cmd->seqno = atoi(msgkeys[i]+2);
					MQTTPersistence_insertInOrder(commands, cmd, sizeof(MQTTAsync_queuedCommand));
					free(buffer);
					client->command_seqno = max(client->command_seqno, cmd->seqno);
					commands_restored++;
				}
			}
			if (msgkeys[i])
				free(msgkeys[i]);
			i++;
		}
		if (msgkeys != NULL)
			free(msgkeys);
	}
	Log(TRACE_MINIMUM, -1, "%d commands restored for client %s", commands_restored, c->clientID);
	FUNC_EXIT_RC(rc);
	return rc;
}
#endif


int MQTTAsync_addCommand(MQTTAsync_queuedCommand* command, int command_size)
{
	int rc = 0;
	
	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttcommand_mutex);
	command->command.start_time = MQTTAsync_start_clock();
	if (command->command.type == CONNECT || 
		(command->command.type == DISCONNECT && command->command.details.dis.internal))
	{
		MQTTAsync_queuedCommand* head = NULL; 
		
		if (commands->first)
			head = (MQTTAsync_queuedCommand*)(commands->first->content);
		
		if (head != NULL && head->client == command->client && head->command.type == command->command.type)
			MQTTAsync_freeCommand(command); /* ignore duplicate connect or disconnect command */
		else
			ListInsert(commands, command, command_size, commands->first); /* add to the head of the list */
	}
	else
	{
		ListAppend(commands, command, command_size);
#if !defined(NO_PERSISTENCE)
		if (command->client->c->persistence)
			MQTTAsync_persistCommand(command);
#endif
	}
	MQTTAsync_unlock_mutex(mqttcommand_mutex);
#if !defined(WIN32) && !defined(WIN64)
	Thread_signal_cond(send_cond);
#else
	if (!Thread_check_sem(send_sem))
		Thread_post_sem(send_sem);
#endif
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTAsync_checkDisconnect(MQTTAsync handle, MQTTAsync_command* command)
{
	MQTTAsyncs* m = handle;

	FUNC_ENTRY;
	/* wait for all inflight message flows to finish, up to timeout */;
	if (m->c->outboundMsgs->count == 0 || MQTTAsync_elapsed(command->start_time) >= command->details.dis.timeout)
	{	
		int was_connected = m->c->connected;
		MQTTAsync_closeSession(m->c);
		if (command->details.dis.internal && m->cl && was_connected)
		{
			Log(TRACE_MIN, -1, "Calling connectionLost for client %s", m->c->clientID);
			(*(m->cl))(m->context, NULL);
		}
		else if (!command->details.dis.internal && command->onSuccess)
		{
			Log(TRACE_MIN, -1, "Calling disconnect complete for client %s", m->c->clientID);
			(*(command->onSuccess))(command->context, NULL);
		}
	}
	FUNC_EXIT;
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


void MQTTAsync_freeConnect(MQTTAsync_command command)
{
	if (command.type == CONNECT)
	{
		int i;
		
		for (i = 0; i < command.details.conn.serverURIcount; ++i)
			free(command.details.conn.serverURIs[i]);
		if (command.details.conn.serverURIs)
			free(command.details.conn.serverURIs); 
	}
}


void MQTTAsync_freeCommand1(MQTTAsync_queuedCommand *command)
{
	if (command->command.type == SUBSCRIBE)
	{
		int i;
		
		for (i = 0; i < command->command.details.sub.count; i++)
			free(command->command.details.sub.topics[i]);

		free(command->command.details.sub.topics);
		free(command->command.details.sub.qoss);
	}
	else if (command->command.type == UNSUBSCRIBE)
	{
		int i;
		
		for (i = 0; i < command->command.details.unsub.count; i++)
			free(command->command.details.unsub.topics[i]);

		free(command->command.details.unsub.topics);
	}
	else if (command->command.type == PUBLISH)
	{
		/* qos 1 and 2 topics are freed in the protocol code when the flows are completed */
		if (command->command.details.pub.destinationName)
			free(command->command.details.pub.destinationName); 
		free(command->command.details.pub.payload);
	}
}

void MQTTAsync_freeCommand(MQTTAsync_queuedCommand *command)
{
	MQTTAsync_freeCommand1(command);
	free(command);
}


void MQTTAsync_writeComplete(int socket)				
{
	ListElement* found = NULL;
	
	FUNC_ENTRY;
	/* a partial write is now complete for a socket - this will be on a publish*/
	
	MQTTProtocol_checkPendingWrites();
	
	/* find the client using this socket */
	if ((found = ListFindItem(handles, &socket, clientSockCompare)) != NULL)
	{
		MQTTAsyncs* m = (MQTTAsyncs*)(found->content);
		
		time(&(m->c->net.lastSent));
				
		/* see if there is a pending write flagged */
		if (m->pending_write)
		{
			ListElement* cur_response = NULL;
			MQTTAsync_command* command = m->pending_write;
			MQTTAsync_queuedCommand* com = NULL;
					
			while (ListNextElement(m->responses, &cur_response))
			{
				com = (MQTTAsync_queuedCommand*)(cur_response->content);
				if (com->client->pending_write == m->pending_write)
					break;
			}
					
			if (cur_response && command->onSuccess)
			{
				MQTTAsync_successData data;
								
				data.token = command->token;
				data.alt.pub.destinationName = command->details.pub.destinationName;
				data.alt.pub.message.payload = command->details.pub.payload;
				data.alt.pub.message.payloadlen = command->details.pub.payloadlen;
				data.alt.pub.message.qos = command->details.pub.qos;
				data.alt.pub.message.retained = command->details.pub.retained;
				Log(TRACE_MIN, -1, "Calling publish success for client %s", m->c->clientID);
				(*(command->onSuccess))(command->context, &data);
			}		
			m->pending_write = NULL;
			
			ListDetach(m->responses, com);
			MQTTAsync_freeCommand(com);
		}
	}
	FUNC_EXIT;
}
			

void MQTTAsync_processCommand()
{
	int rc = 0;
	MQTTAsync_queuedCommand* command = NULL;
	ListElement* cur_command = NULL;
	List* ignored_clients = NULL;
	
	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);
	MQTTAsync_lock_mutex(mqttcommand_mutex);
	
	/* only the first command in the list must be processed for any particular client, so if we skip
	   a command for a client, we must skip all following commands for that client.  Use a list of 
	   ignored clients to keep track
	*/
	ignored_clients = ListInitialize();
	
	/* don't try a command until there isn't a pending write for that client, and we are not connecting */
	while (ListNextElement(commands, &cur_command))
	{
		MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)(cur_command->content);
		
		if (ListFind(ignored_clients, cmd->client))
			continue;
		
		if (cmd->command.type == CONNECT || cmd->command.type == DISCONNECT || (cmd->client->c->connected && 
			cmd->client->c->connect_state == 0 && Socket_noPendingWrites(cmd->client->c->net.socket)))
		{
			if ((cmd->command.type == PUBLISH || cmd->command.type == SUBSCRIBE || cmd->command.type == UNSUBSCRIBE) &&
				cmd->client->c->outboundMsgs->count >= MAX_MSG_ID - 1)
				; /* no more message ids available */
			else
			{
				command = cmd;
				break;
			}
		}
		ListAppend(ignored_clients, cmd->client, sizeof(cmd->client));
	}
	ListFreeNoContent(ignored_clients);
	if (command)
	{
		ListDetach(commands, command);
#if !defined(NO_PERSISTENCE)
		if (command->client->c->persistence)
			MQTTAsync_unpersistCommand(command);
#endif
	}
	MQTTAsync_unlock_mutex(mqttcommand_mutex);
	
	if (!command)
		goto exit; /* nothing to do */
	
	if (command->command.type == CONNECT)
	{
		if (command->client->c->connect_state != 0 || command->client->c->connected)
			rc = 0;
		else
		{
			char* serverURI = command->client->serverURI;

			if (command->command.details.conn.serverURIcount > 0)
			{
				if (command->client->c->MQTTVersion == MQTTVERSION_DEFAULT)
				{
					if (command->command.details.conn.MQTTVersion == 3)
					{
						command->command.details.conn.currentURI++;
						command->command.details.conn.MQTTVersion = 4;
					} 
				}
				else
					command->command.details.conn.currentURI++;
				serverURI = command->command.details.conn.serverURIs[command->command.details.conn.currentURI];
					
				if (strncmp(URI_TCP, serverURI, strlen(URI_TCP)) == 0)
					serverURI += strlen(URI_TCP);
#if defined(NS_ENABLE_SSL)
				else if (strncmp(URI_SSL, serverURI, strlen(URI_SSL)) == 0)
				{
					serverURI += strlen(URI_SSL);
					command->client->ssl = 1;
				}
#endif
			}

			if (command->client->c->MQTTVersion == MQTTVERSION_DEFAULT)
			{
				if (command->command.details.conn.MQTTVersion == 0)
					command->command.details.conn.MQTTVersion = MQTTVERSION_3_1_1;
				else if (command->command.details.conn.MQTTVersion == MQTTVERSION_3_1_1)
					command->command.details.conn.MQTTVersion = MQTTVERSION_3_1;
			}
			else
				command->command.details.conn.MQTTVersion = command->client->c->MQTTVersion;

			Log(TRACE_MIN, -1, "Connecting to serverURI %s with MQTT version %d", serverURI, command->command.details.conn.MQTTVersion);
#if defined(NS_ENABLE_SSL)
			rc = MQTTProtocol_connect(serverURI, command->client->c, command->client->ssl, command->command.details.conn.MQTTVersion);
#else
			rc = MQTTProtocol_connect(serverURI, command->client->c, command->command.details.conn.MQTTVersion);
#endif
			if (command->client->c->connect_state == 0)
				rc = SOCKET_ERROR;
			
			/* if the TCP connect is pending, then we must call select to determine when the connect has completed,
			which is indicated by the socket being ready *either* for reading *or* writing.  The next couple of lines
			make sure we check for writeability as well as readability, otherwise we wait around longer than we need to
			in Socket_getReadySocket() */ 
			if (rc == EINPROGRESS)
				Socket_addPendingWrite(command->client->c->net.socket);
		}
	}
	else if (command->command.type == SUBSCRIBE)
	{
		List* topics = ListInitialize();
		List* qoss = ListInitialize();
		int i;
		
		for (i = 0; i < command->command.details.sub.count; i++)
		{
			ListAppend(topics, command->command.details.sub.topics[i], strlen(command->command.details.sub.topics[i]));
			ListAppend(qoss, &command->command.details.sub.qoss[i], sizeof(int));
		}
		rc = MQTTProtocol_subscribe(command->client->c, topics, qoss, command->command.token);
		ListFreeNoContent(topics);
		ListFreeNoContent(qoss);
	}
	else if (command->command.type == UNSUBSCRIBE)
	{
		List* topics = ListInitialize();
		int i;
			
		for (i = 0; i < command->command.details.unsub.count; i++)
			ListAppend(topics, command->command.details.unsub.topics[i], strlen(command->command.details.unsub.topics[i]));
			
		rc = MQTTProtocol_unsubscribe(command->client->c, topics, command->command.token);
		ListFreeNoContent(topics);
	}
	else if (command->command.type == PUBLISH)
	{
		Messages* msg = NULL;
		Publish* p = NULL;
	
		p = malloc(sizeof(Publish));

		p->payload = command->command.details.pub.payload;
		p->payloadlen = command->command.details.pub.payloadlen;
		p->topic = command->command.details.pub.destinationName;
		p->msgId = command->command.token;

		rc = MQTTProtocol_startPublish(command->client->c, p, command->command.details.pub.qos, command->command.details.pub.retained, &msg);
		
		if (command->command.details.pub.qos == 0)
		{ 
			if (rc == TCPSOCKET_COMPLETE)
			{		
				if (command->command.onSuccess)
				{
					MQTTAsync_successData data;
								
					data.token = command->command.token;
					data.alt.pub.destinationName = command->command.details.pub.destinationName;
					data.alt.pub.message.payload = command->command.details.pub.payload;
					data.alt.pub.message.payloadlen = command->command.details.pub.payloadlen;
					data.alt.pub.message.qos = command->command.details.pub.qos;
					data.alt.pub.message.retained = command->command.details.pub.retained;
					Log(TRACE_MIN, -1, "Calling publish success for client %s", command->client->c->clientID);
					(*(command->command.onSuccess))(command->command.context, &data);
				}
			}
			else
			{
				command->command.details.pub.destinationName = NULL; /* this will be freed by the protocol code */
				command->client->pending_write = &command->command;
			}
		}
		else
			command->command.details.pub.destinationName = NULL; /* this will be freed by the protocol code */
		free(p); /* should this be done if the write isn't complete? */
	}
	else if (command->command.type == DISCONNECT)
	{
		if (command->client->c->connect_state != 0 || command->client->c->connected != 0)
		{
			command->client->c->connect_state = -2;
			MQTTAsync_checkDisconnect(command->client, &command->command);
		}
	}

	if (command->command.type == CONNECT && rc != SOCKET_ERROR && rc != MQTTASYNC_PERSISTENCE_ERROR)
	{
		command->client->connect = command->command;
		MQTTAsync_freeCommand(command);
	}
	else if (command->command.type == DISCONNECT)
	{
		command->client->disconnect = command->command;
		MQTTAsync_freeCommand(command);
	}
	else if (command->command.type == PUBLISH && command->command.details.pub.qos == 0)
	{
		if (rc == TCPSOCKET_INTERRUPTED)
			ListAppend(command->client->responses, command, sizeof(command));
		else
			MQTTAsync_freeCommand(command);
	}
	else if (rc == SOCKET_ERROR || rc == MQTTASYNC_PERSISTENCE_ERROR)
	{
		if (command->command.type == CONNECT)
		{
			MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
			MQTTAsync_disconnect(command->client, &opts); /* not "internal" because we don't want to call connection lost */
		}
		else
			MQTTAsync_disconnect_internal(command->client, 0);
			
		if (command->command.type == CONNECT && MQTTAsync_checkConn(&command->command, command->client))
		{
			Log(TRACE_MIN, -1, "Connect failed, more to try");
			/* put the connect command back to the head of the command queue, using the next serverURI */
			rc = MQTTAsync_addCommand(command, sizeof(command->command.details.conn));
		}
		else
		{
			if (command->command.onFailure)
			{
				Log(TRACE_MIN, -1, "Calling command failure for client %s", command->client->c->clientID);
				(*(command->command.onFailure))(command->command.context, NULL);
			}
			MQTTAsync_freeConnect(command->command);
			MQTTAsync_freeCommand(command);  /* free up the command if necessary */
		}
	}
	else /* put the command into a waiting for response queue for each client, indexed by msgid */
		ListAppend(command->client->responses, command, sizeof(command));

exit:
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT;
}


void MQTTAsync_checkTimeouts()
{
	ListElement* current = NULL;
	static time_t last = 0L;
	time_t now;

	FUNC_ENTRY;
	time(&(now));
	if (difftime(now, last) < 3)
		goto exit;

	MQTTAsync_lock_mutex(mqttasync_mutex);
	last = now;
	while (ListNextElement(handles, &current))		/* for each client */
	{
		ListElement* cur_response = NULL;
		int i = 0, 
			timed_out_count = 0;
		
		MQTTAsyncs* m = (MQTTAsyncs*)(current->content);
		
		/* check connect timeout */
		if (m->c->connect_state != 0 && MQTTAsync_elapsed(m->connect.start_time) > (m->connect.details.conn.timeout * 1000))
		{
			if (MQTTAsync_checkConn(&m->connect, m))
			{
				MQTTAsync_queuedCommand* conn;
				
				MQTTAsync_closeOnly(m->c);
				/* put the connect command back to the head of the command queue, using the next serverURI */
				conn = malloc(sizeof(MQTTAsync_queuedCommand));
				memset(conn, '\0', sizeof(MQTTAsync_queuedCommand));
				conn->client = m;
				conn->command = m->connect;
				Log(TRACE_MIN, -1, "Connect failed with timeout, more to try");
				MQTTAsync_addCommand(conn, sizeof(m->connect));
			}
			else
			{
				MQTTAsync_closeSession(m->c);
				MQTTAsync_freeConnect(m->connect);
				if (m->connect.onFailure)
				{
					Log(TRACE_MIN, -1, "Calling connect failure for client %s", m->c->clientID);
					(*(m->connect.onFailure))(m->connect.context, NULL);
				}
			}
			continue;
		}
	
		/* check disconnect timeout */
		if (m->c->connect_state == -2)
			MQTTAsync_checkDisconnect(m, &m->disconnect);
	
		timed_out_count = 0;
		/* check response timeouts */
		while (ListNextElement(m->responses, &cur_response))
		{
			MQTTAsync_queuedCommand* com = (MQTTAsync_queuedCommand*)(cur_response->content);
			
			if (1 /*MQTTAsync_elapsed(com->command.start_time) < 120000*/)	
				break; /* command has not timed out */
			else
			{
				if (com->command.onFailure)
				{		
					Log(TRACE_MIN, -1, "Calling %s failure for client %s", 
								MQTTPacket_name(com->command.type), m->c->clientID);
					(*(com->command.onFailure))(com->command.context, NULL);
				}
				timed_out_count++;
			}
		}
		for (i = 0; i < timed_out_count; ++i)
			ListRemoveHead(m->responses);	/* remove the first response in the list */
	}
	MQTTAsync_unlock_mutex(mqttasync_mutex);
exit:
	FUNC_EXIT;
}


thread_return_type WINAPI MQTTAsync_sendThread(void* n)
{
	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);
	sendThread_state = RUNNING;
	sendThread_id = Thread_getid();
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	while (!tostop)
	{
		int rc;
		
		while (commands->count > 0)
		{
			int before = commands->count;
			MQTTAsync_processCommand();
			if (before == commands->count)
				break;  /* no commands were processed, so go into a wait */
		}
#if !defined(WIN32) && !defined(WIN64)
		rc =  Thread_wait_cond(send_cond, 1);
		if ((rc = Thread_wait_cond(send_cond, 1)) != 0 && rc != ETIMEDOUT)
			Log(LOG_ERROR, -1, "Error %d waiting for condition variable", rc);
#else
		if ((rc = Thread_wait_sem(send_sem, 1000)) != 0 && rc != ETIMEDOUT)
			Log(LOG_ERROR, -1, "Error %d waiting for semaphore", rc);
#endif
			
		MQTTAsync_checkTimeouts();
	}
	sendThread_state = STOPPING;
	MQTTAsync_lock_mutex(mqttasync_mutex);
	sendThread_state = STOPPED;
	sendThread_id = 0;
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT;
	return 0;
}


void MQTTAsync_emptyMessageQueue(Clients* client)
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


void MQTTAsync_removeResponsesAndCommands(MQTTAsyncs* m)
{
	int count = 0;	
	ListElement* current = NULL;
	ListElement *next = NULL;

	FUNC_ENTRY;
	if (m->responses)
	{
		ListElement* elem = NULL;

		while (ListNextElement(m->responses, &elem))
		{
			MQTTAsync_freeCommand1((MQTTAsync_queuedCommand*) (elem->content));
			count++;
		}
	}
	ListEmpty(m->responses);
	Log(TRACE_MINIMUM, -1, "%d responses removed for client %s", count, m->c->clientID);
	
	/* remove commands in the command queue relating to this client */
	count = 0;
	current = ListNextElement(commands, &next);
	ListNextElement(commands, &next);
	while (current)
	{
		MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)(current->content);
		
		if (cmd->client == m)
		{
			ListDetach(commands, cmd);
			MQTTAsync_freeCommand(cmd);
			count++;
		}
		current = next;
		ListNextElement(commands, &next);
	}
	Log(TRACE_MINIMUM, -1, "%d commands removed for client %s", count, m->c->clientID);
	FUNC_EXIT;
}


void MQTTAsync_destroy(MQTTAsync* handle)
{
	MQTTAsyncs* m = *handle;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (m == NULL)
		goto exit;

	MQTTAsync_removeResponsesAndCommands(m);
	ListFree(m->responses);
	
	if (m->c)
	{
		int saved_socket = m->c->net.socket;
		char* saved_clientid = MQTTStrdup(m->c->clientID);
#if !defined(NO_PERSISTENCE)
		MQTTPersistence_close(m->c);
#endif
		MQTTAsync_emptyMessageQueue(m->c);
		MQTTProtocol_freeClient(m->c);
		if (!ListRemove(bstate->clients, m->c))
			Log(LOG_ERROR, 0, NULL);
		else
			Log(TRACE_MIN, 1, NULL, saved_clientid, saved_socket);
		free(saved_clientid);
	}
		
	if (m->serverURI)
		free(m->serverURI);
	if (!ListRemove(handles, m))
		Log(LOG_ERROR, -1, "free error");
	*handle = NULL;
	if (bstate->clients->count == 0)
		MQTTAsync_terminate();

exit:
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT;
}


void MQTTAsync_freeMessage(MQTTAsync_message** message)
{
	FUNC_ENTRY;
	free((*message)->payload);
	free(*message);
	*message = NULL;
	FUNC_EXIT;
}


void MQTTAsync_free(void* memory)
{
	FUNC_ENTRY;
	free(memory);
	FUNC_EXIT;
}


int MQTTAsync_completeConnection(MQTTAsyncs* m, MQTTPacket* pack)
{
	int rc = MQTTASYNC_FAILURE;

	FUNC_ENTRY;
	if (m->c->connect_state == 3) /* MQTT connect sent - wait for CONNACK */
	{
		Connack* connack = (Connack*)pack;
		Log(LOG_PROTOCOL, 1, NULL, m->c->net.socket, m->c->clientID, connack->rc);
		if ((rc = connack->rc) == MQTTASYNC_SUCCESS)
		{
			m->c->connected = 1;
			m->c->good = 1;
			m->c->connect_state = 0;
			if (m->c->cleansession)
				rc = MQTTAsync_cleanSession(m->c);
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
					rc = MQTTASYNC_DISCONNECTED;
			}
		}
		free(connack);
		m->pack = NULL;
	}
	FUNC_EXIT_RC(rc);
	return rc;
}

/* This is the thread function that handles the calling of callback functions if set */
thread_return_type WINAPI MQTTAsync_receiveThread(void* n)
{
	long timeout = 10L; /* first time in we have a small timeout.  Gets things started more quickly */

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);
	receiveThread_state = RUNNING;
	receiveThread_id = Thread_getid();
	while (!tostop)
	{
		int rc = SOCKET_ERROR;
		int sock = -1;
		MQTTAsyncs* m = NULL;
		MQTTPacket* pack = NULL;

		MQTTAsync_unlock_mutex(mqttasync_mutex);
		pack = MQTTAsync_cycle(&sock, timeout, &rc);
		MQTTAsync_lock_mutex(mqttasync_mutex);
		if (tostop)
			break;
		timeout = 1000L;

		if (sock == 0)
			continue;
		/* find client corresponding to socket */
		if (ListFindItem(handles, &sock, clientSockCompare) == NULL)
		{
			Log(TRACE_MINIMUM, -1, "Could not find client corresponding to socket %d", sock);
			/* Socket_close(sock); - removing socket in this case is not necessary (Bug 442400) */
			continue;
		}
		m = (MQTTAsyncs*)(handles->current->content);
		if (m == NULL)
		{
			Log(LOG_ERROR, -1, "Client structure was NULL for socket %d - removing socket", sock);
			Socket_close(sock);
			continue;
		}
		if (rc == SOCKET_ERROR)
		{
			Log(TRACE_MINIMUM, -1, "Error from MQTTAsync_cycle() - removing socket %d", sock);
			if (m->c->connected == 1)
			{
				MQTTAsync_unlock_mutex(mqttasync_mutex);
				MQTTAsync_disconnect_internal(m, 0);
				MQTTAsync_lock_mutex(mqttasync_mutex);
			}
			else /* calling disconnect_internal won't have any effect if we're already disconnected */
				MQTTAsync_closeOnly(m->c);
		}
		else
		{
			if (m->c->messageQueue->count > 0)
			{
				qEntry* qe = (qEntry*)(m->c->messageQueue->first->content);
				int topicLen = qe->topicLen;

				if (strlen(qe->topicName) == topicLen)
					topicLen = 0;

				if (m->ma)
					rc = MQTTAsync_deliverMessage(m, qe->topicName, topicLen, qe->msg);
				else 
					rc = 1;
					
				if (rc)
				{
					ListRemove(m->c->messageQueue, qe);
#if !defined(NO_PERSISTENCE)
					if (m->c->persistence)
						MQTTPersistence_unpersistQueueEntry(m->c, (MQTTPersistence_qEntry*)qe);
#endif
				}
				else
					Log(TRACE_MIN, -1, "False returned from messageArrived for client %s, message remains on queue",
						m->c->clientID);
			}
			if (pack)
			{
				if (pack->header.bits.type == CONNACK)
				{
					int sessionPresent = ((Connack*)pack)->flags.bits.sessionPresent;
					int rc = MQTTAsync_completeConnection(m, pack);
					
					if (rc == MQTTASYNC_SUCCESS)
					{
						if (m->connect.details.conn.serverURIcount > 0)
							Log(TRACE_MIN, -1, "Connect succeeded to %s", 
								m->connect.details.conn.serverURIs[m->connect.details.conn.currentURI]);
						MQTTAsync_freeConnect(m->connect);
						if (m->connect.onSuccess)
						{
							MQTTAsync_successData data;
							memset(&data, '\0', sizeof(data));
							Log(TRACE_MIN, -1, "Calling connect success for client %s", m->c->clientID);
							if (m->connect.details.conn.serverURIcount > 0)
								data.alt.connect.serverURI = m->connect.details.conn.serverURIs[m->connect.details.conn.currentURI];
							else
								data.alt.connect.serverURI = m->serverURI;
							data.alt.connect.MQTTVersion = m->connect.details.conn.MQTTVersion;
							data.alt.connect.sessionPresent = sessionPresent;
							(*(m->connect.onSuccess))(m->connect.context, &data);
						}
					}
					else
					{
						if (MQTTAsync_checkConn(&m->connect, m))
						{
							MQTTAsync_queuedCommand* conn;
							
							MQTTAsync_closeOnly(m->c);
							/* put the connect command back to the head of the command queue, using the next serverURI */
							conn = malloc(sizeof(MQTTAsync_queuedCommand));
							memset(conn, '\0', sizeof(MQTTAsync_queuedCommand));
							conn->client = m;
							conn->command = m->connect; 
							Log(TRACE_MIN, -1, "Connect failed, more to try");
							MQTTAsync_addCommand(conn, sizeof(m->connect));
						}
						else
						{
							MQTTAsync_closeSession(m->c);
							MQTTAsync_freeConnect(m->connect);
							if (m->connect.onFailure)
							{
								MQTTAsync_failureData data;
						
								data.token = 0;
								data.code = rc;
								data.message = "CONNACK return code";
								Log(TRACE_MIN, -1, "Calling connect failure for client %s", m->c->clientID);
								(*(m->connect.onFailure))(m->connect.context, &data);
							}
						}
					}
				}
				else if (pack->header.bits.type == SUBACK)
				{
					ListElement* current = NULL;
									
					/* use the msgid to find the callback to be called */
					while (ListNextElement(m->responses, &current))
					{
						MQTTAsync_queuedCommand* command = (MQTTAsync_queuedCommand*)(current->content);
						if (command->command.token == ((Suback*)pack)->msgId)
						{	
							Suback* sub = (Suback*)pack;
							if (!ListDetach(m->responses, command)) /* remove the response from the list */
								Log(LOG_ERROR, -1, "Subscribe command not removed from command list");

							/* Call the failure callback if there is one subscribe in the MQTT packet and
							 * the return code is 0x80 (failure).  If the MQTT packet contains >1 subscription
							 * request, then we call onSuccess with the list of returned QoSs, which inelegantly,
							 * could include some failures, or worse, the whole list could have failed.
							 */
							if (sub->qoss->count == 1 && *(int*)(sub->qoss->first->content) == MQTT_BAD_SUBSCRIBE)
							{
								if (command->command.onFailure)
								{
									MQTTAsync_failureData data;

									data.token = command->command.token;
									data.code = *(int*)(sub->qoss->first->content);
									Log(TRACE_MIN, -1, "Calling subscribe failure for client %s", m->c->clientID);
									(*(command->command.onFailure))(command->command.context, &data);
								}
							}
							else if (command->command.onSuccess)
							{
								MQTTAsync_successData data;
								int* array = NULL;
								
								if (sub->qoss->count == 1)
									data.alt.qos = *(int*)(sub->qoss->first->content);
								else if (sub->qoss->count > 1)
								{
									ListElement* cur_qos = NULL;
									int* element = array = data.alt.qosList = malloc(sub->qoss->count * sizeof(int));
									while (ListNextElement(sub->qoss, &cur_qos))
										*element++ = *(int*)(cur_qos->content);
								} 
								data.token = command->command.token;
								Log(TRACE_MIN, -1, "Calling subscribe success for client %s", m->c->clientID);
								(*(command->command.onSuccess))(command->command.context, &data);
								if (array)
									free(array);
							}
							MQTTAsync_freeCommand(command);
							break;
						}
					}
					rc = MQTTProtocol_handleSubacks(pack, m->c->net.socket);
				}
				else if (pack->header.bits.type == UNSUBACK)
				{
					ListElement* current = NULL;
					int handleCalled = 0;
					
					/* use the msgid to find the callback to be called */
					while (ListNextElement(m->responses, &current))
					{
						MQTTAsync_queuedCommand* command = (MQTTAsync_queuedCommand*)(current->content);
						if (command->command.token == ((Unsuback*)pack)->msgId)
						{		
							if (!ListDetach(m->responses, command)) /* remove the response from the list */
								Log(LOG_ERROR, -1, "Unsubscribe command not removed from command list");
							if (command->command.onSuccess)
							{
								rc = MQTTProtocol_handleUnsubacks(pack, m->c->net.socket);
								handleCalled = 1;
								Log(TRACE_MIN, -1, "Calling unsubscribe success for client %s", m->c->clientID);
								(*(command->command.onSuccess))(command->command.context, NULL);
							}
							MQTTAsync_freeCommand(command);
							break;
						}
					}
					if (!handleCalled)
						rc = MQTTProtocol_handleUnsubacks(pack, m->c->net.socket);
				}
			}
		}
	}
	receiveThread_state = STOPPED;
	receiveThread_id = 0;
	MQTTAsync_unlock_mutex(mqttasync_mutex);
#if !defined(WIN32) && !defined(WIN64)
	if (sendThread_state != STOPPED)
		Thread_signal_cond(send_cond);
#else
	if (sendThread_state != STOPPED && !Thread_check_sem(send_sem))
		Thread_post_sem(send_sem);
#endif
	FUNC_EXIT;
	return 0;
}


void MQTTAsync_stop()
{
	int rc = 0;

	FUNC_ENTRY;
	if (sendThread_state != STOPPED || receiveThread_state != STOPPED)
	{
		int conn_count = 0;
		ListElement* current = NULL;

		if (handles != NULL)
		{
			/* find out how many handles are still connected */
			while (ListNextElement(handles, &current))
			{
				if (((MQTTAsyncs*)(current->content))->c->connect_state > 0 ||
						((MQTTAsyncs*)(current->content))->c->connected)
					++conn_count;
			}
		}
		Log(TRACE_MIN, -1, "Conn_count is %d", conn_count);
		/* stop the background thread, if we are the last one to be using it */
		if (conn_count == 0)
		{
			int count = 0;
			tostop = 1;
			while ((sendThread_state != STOPPED || receiveThread_state != STOPPED) && ++count < 100)
			{
				MQTTAsync_unlock_mutex(mqttasync_mutex);
				Log(TRACE_MIN, -1, "sleeping");
				MQTTAsync_sleep(100L);
				MQTTAsync_lock_mutex(mqttasync_mutex);
			}
			rc = 1;
			tostop = 0;
		}
	}
	FUNC_EXIT_RC(rc);
}


int MQTTAsync_setCallbacks(MQTTAsync handle, void* context,
									MQTTAsync_connectionLost* cl,
									MQTTAsync_messageArrived* ma,
									MQTTAsync_deliveryComplete* dc)
{
	int rc = MQTTASYNC_SUCCESS;
	MQTTAsyncs* m = handle;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (m == NULL || ma == NULL || m->c->connect_state != 0)
		rc = MQTTASYNC_FAILURE;
	else
	{
		m->context = context;
		m->cl = cl;
		m->ma = ma;
		m->dc = dc;
	}

	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTAsync_closeOnly(Clients* client)
{
	FUNC_ENTRY;
	client->good = 0;
	client->ping_outstanding = 0;
	if (client->net.socket > 0)
	{
		if (client->connected)
			MQTTPacket_send_disconnect(&client->net, client->clientID);
#if defined(NS_ENABLE_SSL)
		SSLSocket_close(&client->net);
#endif
		Socket_close(client->net.socket);
		client->net.socket = 0;
#if defined(NS_ENABLE_SSL)
		client->net.ssl = NULL;
#endif
	}
	client->connected = 0;
	client->connect_state = 0;		
	FUNC_EXIT;
}


void MQTTAsync_closeSession(Clients* client)
{
	FUNC_ENTRY;
	MQTTAsync_closeOnly(client);

	if (client->cleansession)
		MQTTAsync_cleanSession(client);
		
	FUNC_EXIT;
}


/**
 * List callback function for comparing clients by client structure
 * @param a Async structure
 * @param b Client structure
 * @return boolean indicating whether a and b are equal
 */
int clientStructCompare(void* a, void* b)
{
	MQTTAsyncs* m = (MQTTAsyncs*)a;
	return m->c == (Clients*)b;
}


int MQTTAsync_cleanSession(Clients* client)
{
	int rc = 0;
	ListElement* found = NULL;

	FUNC_ENTRY;
#if !defined(NO_PERSISTENCE)
	rc = MQTTPersistence_clear(client);
#endif
	MQTTProtocol_emptyMessageList(client->inboundMsgs);
	MQTTProtocol_emptyMessageList(client->outboundMsgs);
	MQTTAsync_emptyMessageQueue(client);
	client->msgID = 0;
	
	if ((found = ListFindItem(handles, client, clientStructCompare)) != NULL)
	{
		MQTTAsyncs* m = (MQTTAsyncs*)(found->content);
		MQTTAsync_removeResponsesAndCommands(m);
	}
	else
		Log(LOG_ERROR, -1, "cleanSession: did not find client structure in handles list");
	FUNC_EXIT_RC(rc);
	return rc;
}





int MQTTAsync_deliverMessage(MQTTAsyncs* m, char* topicName, size_t topicLen, MQTTAsync_message* mm)
{
	int rc;
					
	Log(TRACE_MIN, -1, "Calling messageArrived for client %s, queue depth %d",
					m->c->clientID, m->c->messageQueue->count);
	rc = (*(m->ma))(m->context, topicName, topicLen, mm);
	/* if 0 (false) is returned by the callback then it failed, so we don't remove the message from
	 * the queue, and it will be retried later.  If 1 is returned then the message data may have been freed,
	 * so we must be careful how we use it.
	 */
	return rc;
}


void Protocol_processPublication(Publish* publish, Clients* client)
{
	MQTTAsync_message* mm = NULL;
	int rc = 0;

	FUNC_ENTRY;
	mm = malloc(sizeof(MQTTAsync_message));

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
	
	if (client->messageQueue->count == 0 && client->connected)
	{
		ListElement* found = NULL;
		
		if ((found = ListFindItem(handles, client, clientStructCompare)) == NULL)
			Log(LOG_ERROR, -1, "processPublication: did not find client structure in handles list");
		else
		{
			MQTTAsyncs* m = (MQTTAsyncs*)(found->content);

			if (m->ma)
				rc = MQTTAsync_deliverMessage(m, publish->topic, publish->topiclen, mm);
		} 
	}

	if (rc == 0) /* if message was not delivered, queue it up */
	{
		qEntry* qe = malloc(sizeof(qEntry));	
		qe->msg = mm;
		qe->topicName = publish->topic;
		qe->topicLen = publish->topiclen;
		ListAppend(client->messageQueue, qe, sizeof(qe) + sizeof(mm) + mm->payloadlen + strlen(qe->topicName)+1);
#if !defined(NO_PERSISTENCE)
		if (client->persistence)
			MQTTPersistence_persistQueueEntry(client, (MQTTPersistence_qEntry*)qe);
#endif
	}
	publish->topic = NULL;	
	FUNC_EXIT;
}


int MQTTAsync_connect(MQTTAsync handle, const MQTTAsync_connectOptions* options)
{
	MQTTAsyncs* m = handle;
	int rc = MQTTASYNC_SUCCESS;
	MQTTAsync_queuedCommand* conn;

	FUNC_ENTRY;
	if (options == NULL)
	{
		rc = MQTTASYNC_NULL_PARAMETER;
		goto exit;
	}

	if (strncmp(options->struct_id, "MQTC", 4) != 0 || 
		(options->struct_version != 0 && options->struct_version != 1 && options->struct_version != 2 && 
         options->struct_version != 3))
	{
		rc = MQTTASYNC_BAD_STRUCTURE;
		goto exit;
	}
	if (options->will) /* check validity of will options structure */
	{
		if (strncmp(options->will->struct_id, "MQTW", 4) != 0 || options->will->struct_version != 0)
		{
			rc = MQTTASYNC_BAD_STRUCTURE;
			goto exit;
		}
		if (options->will->qos < 0 || options->will->qos > 2)
		{
			rc = MQTTASYNC_BAD_QOS;
			goto exit;
		}
	}
	if (options->struct_version != 0 && options->ssl) /* check validity of SSL options structure */
	{
		if (strncmp(options->ssl->struct_id, "MQTS", 4) != 0 || options->ssl->struct_version != 0)
		{
			rc = MQTTASYNC_BAD_STRUCTURE;
			goto exit;
		}
	}
	if ((options->username && !UTF8_validateString(options->username)) ||
		(options->password && !UTF8_validateString(options->password)))
	{
		rc = MQTTASYNC_BAD_UTF8_STRING;
		goto exit;
	}

	m->connect.onSuccess = options->onSuccess;
	m->connect.onFailure = options->onFailure;
	m->connect.context = options->context;
	
	tostop = 0;
	if (sendThread_state != STARTING && sendThread_state != RUNNING)
	{
		MQTTAsync_lock_mutex(mqttasync_mutex);
		sendThread_state = STARTING;
		Thread_start(MQTTAsync_sendThread, NULL);
		MQTTAsync_unlock_mutex(mqttasync_mutex);
	}
	if (receiveThread_state != STARTING && receiveThread_state != RUNNING)
	{
		MQTTAsync_lock_mutex(mqttasync_mutex);
		receiveThread_state = STARTING;
		Thread_start(MQTTAsync_receiveThread, handle);
		MQTTAsync_unlock_mutex(mqttasync_mutex);
	}

	m->c->keepAliveInterval = options->keepAliveInterval;
	m->c->cleansession = options->cleansession;
	m->c->maxInflightMessages = options->maxInflight;
	if (options->struct_version == 3)
		m->c->MQTTVersion = options->MQTTVersion;
	else
		m->c->MQTTVersion = 0;

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
	
#if defined(NS_ENABLE_SSL)
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
		free((void*)m->c->sslopts);
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
	
	/* Add connect request to operation queue */
	conn = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(conn, '\0', sizeof(MQTTAsync_queuedCommand));
	conn->client = m;
	if (options)
	{
		conn->command.onSuccess = options->onSuccess;
		conn->command.onFailure = options->onFailure;
		conn->command.context = options->context;
		conn->command.details.conn.timeout = options->connectTimeout;
			
		if (options->struct_version >= 2 && options->serverURIcount > 0)
		{
			int i;
			
			conn->command.details.conn.serverURIcount = options->serverURIcount;
			conn->command.details.conn.serverURIs = malloc(options->serverURIcount * sizeof(char*));
			for (i = 0; i < options->serverURIcount; ++i)
				conn->command.details.conn.serverURIs[i] = MQTTStrdup(options->serverURIs[i]);
			conn->command.details.conn.currentURI = 0;
		}
	}
	conn->command.type = CONNECT;
	rc = MQTTAsync_addCommand(conn, sizeof(conn));

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_disconnect1(MQTTAsync handle, const MQTTAsync_disconnectOptions* options, int internal)
{
	MQTTAsyncs* m = handle;
	int rc = MQTTASYNC_SUCCESS;
	MQTTAsync_queuedCommand* dis;

	FUNC_ENTRY;
	if (m == NULL || m->c == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTASYNC_DISCONNECTED;
		goto exit;
	}
	
	/* Add disconnect request to operation queue */
	dis = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(dis, '\0', sizeof(MQTTAsync_queuedCommand));
	dis->client = m;
	if (options)
	{
		dis->command.onSuccess = options->onSuccess;
		dis->command.onFailure = options->onFailure;
		dis->command.context = options->context;
		dis->command.details.dis.timeout = options->timeout;
	}
	dis->command.type = DISCONNECT;
	dis->command.details.dis.internal = internal;
	rc = MQTTAsync_addCommand(dis, sizeof(dis));
	
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_disconnect_internal(MQTTAsync handle, int timeout)
{
	MQTTAsync_disconnectOptions options = MQTTAsync_disconnectOptions_initializer;
	
	options.timeout = timeout;
	return MQTTAsync_disconnect1(handle, &options, 1);
}


void MQTTProtocol_closeSession(Clients* c, int sendwill)
{
	MQTTAsync_disconnect_internal((MQTTAsync)c->context, 0);
}


int MQTTAsync_disconnect(MQTTAsync handle, const MQTTAsync_disconnectOptions* options)
{	
	return MQTTAsync_disconnect1(handle, options, 0);
}


int MQTTAsync_isConnected(MQTTAsync handle)
{
	MQTTAsyncs* m = handle;
	int rc = 0;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);
	if (m && m->c)
		rc = m->c->connected;
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int cmdMessageIDCompare(void* a, void* b)
{
	MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)a;
	return cmd->command.token == *(int*)b;
}


/**
 * Assign a new message id for a client.  Make sure it isn't already being used and does
 * not exceed the maximum.
 * @param m a client structure
 * @return the next message id to use, or 0 if none available
 */
int MQTTAsync_assignMsgId(MQTTAsyncs* m)
{
	int start_msgid = m->c->msgID;
	int msgid = start_msgid;
	thread_id_type thread_id = 0;
	int locked = 0;

	/* need to check: commands list and response list for a client */
	FUNC_ENTRY;
	/* We might be called in a callback. In which case, this mutex will be already locked. */
	thread_id = Thread_getid();
	if (thread_id != sendThread_id && thread_id != receiveThread_id)
	{
		MQTTAsync_lock_mutex(mqttasync_mutex);
		locked = 1;
	}

	msgid = (msgid == MAX_MSG_ID) ? 1 : msgid + 1;
	while (ListFindItem(commands, &msgid, cmdMessageIDCompare) ||
			ListFindItem(m->responses, &msgid, cmdMessageIDCompare))
	{
		msgid = (msgid == MAX_MSG_ID) ? 1 : msgid + 1;
		if (msgid == start_msgid)
		{ /* we've tried them all - none free */
			msgid = 0;
			break;
		}
	}
	if (msgid != 0)
		m->c->msgID = msgid;
	if (locked)
		MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(msgid);
	return msgid;
}


int MQTTAsync_subscribeMany(MQTTAsync handle, int count, char* const* topic, int* qos, MQTTAsync_responseOptions* response)
{
	MQTTAsyncs* m = handle;
	int i = 0;
	int rc = MQTTASYNC_FAILURE;
	MQTTAsync_queuedCommand* sub;
	int msgid = 0;

	FUNC_ENTRY;
	if (m == NULL || m->c == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTASYNC_DISCONNECTED;
		goto exit;
	}
	for (i = 0; i < count; i++)
	{
		if (!UTF8_validateString(topic[i]))
		{
			rc = MQTTASYNC_BAD_UTF8_STRING;
			goto exit;
		}
		if (qos[i] < 0 || qos[i] > 2)
		{
			rc = MQTTASYNC_BAD_QOS;
			goto exit;
		}
	}
	if ((msgid = MQTTAsync_assignMsgId(m)) == 0)
	{
		rc = MQTTASYNC_NO_MORE_MSGIDS;
		goto exit;
	}

	/* Add subscribe request to operation queue */
	sub = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(sub, '\0', sizeof(MQTTAsync_queuedCommand));
	sub->client = m;
	sub->command.token = msgid;
	if (response)
	{
		sub->command.onSuccess = response->onSuccess;
		sub->command.onFailure = response->onFailure;
		sub->command.context = response->context;
		response->token = sub->command.token;
	}
	sub->command.type = SUBSCRIBE;
	sub->command.details.sub.count = count;
	sub->command.details.sub.topics = malloc(sizeof(char*) * count);
	sub->command.details.sub.qoss = malloc(sizeof(int) * count);
	for (i = 0; i < count; ++i)
	{
		sub->command.details.sub.topics[i] = MQTTStrdup(topic[i]);
		sub->command.details.sub.qoss[i] = qos[i];	
	}
	rc = MQTTAsync_addCommand(sub, sizeof(sub));

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_subscribe(MQTTAsync handle, const char* topic, int qos, MQTTAsync_responseOptions* response)
{
	int rc = 0;
	char *const topics[] = {(char*)topic};
	FUNC_ENTRY;
	rc = MQTTAsync_subscribeMany(handle, 1, topics, &qos, response);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_unsubscribeMany(MQTTAsync handle, int count, char* const* topic, MQTTAsync_responseOptions* response)
{
	MQTTAsyncs* m = handle;
	int i = 0;
	int rc = SOCKET_ERROR;
	MQTTAsync_queuedCommand* unsub;
	int msgid = 0;

	FUNC_ENTRY;
	if (m == NULL || m->c == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTASYNC_DISCONNECTED;
		goto exit;
	}
	for (i = 0; i < count; i++)
	{
		if (!UTF8_validateString(topic[i]))
		{
			rc = MQTTASYNC_BAD_UTF8_STRING;
			goto exit;
		}
	}
	if ((msgid = MQTTAsync_assignMsgId(m)) == 0)
	{
		rc = MQTTASYNC_NO_MORE_MSGIDS;
		goto exit;
	}
	
	/* Add unsubscribe request to operation queue */
	unsub = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(unsub, '\0', sizeof(MQTTAsync_queuedCommand));
	unsub->client = m;
	unsub->command.type = UNSUBSCRIBE;
	unsub->command.token = msgid;
	if (response)
	{
		unsub->command.onSuccess = response->onSuccess;
		unsub->command.onFailure = response->onFailure;
		unsub->command.context = response->context;
		response->token = unsub->command.token;
	}
	unsub->command.details.unsub.count = count;
	unsub->command.details.unsub.topics = malloc(sizeof(char*) * count);
	for (i = 0; i < count; ++i)
		unsub->command.details.unsub.topics[i] = MQTTStrdup(topic[i]);
	rc = MQTTAsync_addCommand(unsub, sizeof(unsub));

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_unsubscribe(MQTTAsync handle, const char* topic, MQTTAsync_responseOptions* response)
{
	int rc = 0;
	char *const topics[] = {(char*)topic};
	FUNC_ENTRY;
	rc = MQTTAsync_unsubscribeMany(handle, 1, topics, response);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_send(MQTTAsync handle, const char* destinationName, int payloadlen, void* payload,
							 int qos, int retained, MQTTAsync_responseOptions* response)
{
	int rc = MQTTASYNC_SUCCESS;
	MQTTAsyncs* m = handle;
	MQTTAsync_queuedCommand* pub;
	int msgid = 0;

	FUNC_ENTRY;
	if (m == NULL || m->c == NULL)
		rc = MQTTASYNC_FAILURE;
	else if (m->c->connected == 0)
		rc = MQTTASYNC_DISCONNECTED;
	else if (!UTF8_validateString(destinationName))
		rc = MQTTASYNC_BAD_UTF8_STRING;
	else if (qos < 0 || qos > 2)
		rc = MQTTASYNC_BAD_QOS;
	else if (qos > 0 && (msgid = MQTTAsync_assignMsgId(m)) == 0)
		rc = MQTTASYNC_NO_MORE_MSGIDS;

	if (rc != MQTTASYNC_SUCCESS)
		goto exit;
	
	/* Add publish request to operation queue */
	pub = malloc(sizeof(MQTTAsync_queuedCommand));
	memset(pub, '\0', sizeof(MQTTAsync_queuedCommand));
	pub->client = m;
	pub->command.type = PUBLISH;
	pub->command.token = msgid;
	if (response)
	{
		pub->command.onSuccess = response->onSuccess;
		pub->command.onFailure = response->onFailure;
		pub->command.context = response->context;
		response->token = pub->command.token;
	}
	pub->command.details.pub.destinationName = MQTTStrdup(destinationName);
	pub->command.details.pub.payloadlen = payloadlen;
	pub->command.details.pub.payload = malloc(payloadlen);
	memcpy(pub->command.details.pub.payload, payload, payloadlen);
	pub->command.details.pub.qos = qos;
	pub->command.details.pub.retained = retained;
	rc = MQTTAsync_addCommand(pub, sizeof(pub));

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



int MQTTAsync_sendMessage(MQTTAsync handle, const char* destinationName, const MQTTAsync_message* message,
													 MQTTAsync_responseOptions* response)
{
	int rc = MQTTASYNC_SUCCESS;

	FUNC_ENTRY;
	if (message == NULL)
	{
		rc = MQTTASYNC_NULL_PARAMETER;
		goto exit;
	}
	if (strncmp(message->struct_id, "MQTM", 4) != 0 || message->struct_version != 0)
	{
		rc = MQTTASYNC_BAD_STRUCTURE;
		goto exit;
	}

	rc = MQTTAsync_send(handle, destinationName, message->payloadlen, message->payload,
								message->qos, message->retained, response);
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}


void MQTTAsync_retry(void)
{
	static time_t last = 0L;
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


int MQTTAsync_connecting(MQTTAsyncs* m)
{
	int rc = -1;

	FUNC_ENTRY;
	if (m->c->connect_state == 1) /* TCP connect started - check for completion */
	{
		int error;
		socklen_t len = sizeof(error);

		if ((rc = getsockopt(m->c->net.socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len)) == 0)
			rc = error;

		if (rc != 0)
			goto exit;
			
		Socket_clearPendingWrite(m->c->net.socket);

#if defined(NS_ENABLE_SSL)
		if (m->ssl)
		{
			if (SSLSocket_setSocketForSSL(&m->c->net, m->c->sslopts) != MQTTASYNC_SUCCESS)
			{
				if (m->c->session != NULL)
					if ((rc = SSL_set_session(m->c->net.ssl, m->c->session)) != 1)
						Log(TRACE_MIN, -1, "Failed to set SSL session with stored data, non critical");
				rc = SSLSocket_connect(m->c->net.ssl, m->c->net.socket);
				if (rc == TCPSOCKET_INTERRUPTED)
				{
					rc = MQTTCLIENT_SUCCESS; /* the connect is still in progress */
					m->c->connect_state = 2;
				}
				else if (rc == SSL_FATAL)
				{
					rc = SOCKET_ERROR;
					goto exit;
				}
				else if (rc == 1) 
				{
					rc = MQTTCLIENT_SUCCESS;
					m->c->connect_state = 3;
					if (MQTTPacket_send_connect(m->c, m->connect.details.conn.MQTTVersion) == SOCKET_ERROR)
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
			m->c->connect_state = 3; /* TCP/SSL connect completed, in which case send the MQTT connect packet */
			if ((rc = MQTTPacket_send_connect(m->c, m->connect.details.conn.MQTTVersion)) == SOCKET_ERROR)
				goto exit;
#if defined(NS_ENABLE_SSL)
		}
#endif
	}
#if defined(NS_ENABLE_SSL)
	else if (m->c->connect_state == 2) /* SSL connect sent - wait for completion */
	{
		if ((rc = SSLSocket_connect(m->c->net.ssl, m->c->net.socket)) != 1)
			goto exit;

		if(!m->c->cleansession && m->c->session == NULL)
			m->c->session = SSL_get1_session(m->c->net.ssl);
		m->c->connect_state = 3; /* SSL connect completed, in which case send the MQTT connect packet */
		if ((rc = MQTTPacket_send_connect(m->c, m->connect.details.conn.MQTTVersion)) == SOCKET_ERROR)
			goto exit;
	}
#endif

exit:
	if ((rc != 0 && rc != TCPSOCKET_INTERRUPTED && m->c->connect_state != 2) || (rc == SSL_FATAL))
	{
		if (MQTTAsync_checkConn(&m->connect, m))
		{
			MQTTAsync_queuedCommand* conn;
				
			MQTTAsync_closeOnly(m->c);
			/* put the connect command back to the head of the command queue, using the next serverURI */
			conn = malloc(sizeof(MQTTAsync_queuedCommand));
			memset(conn, '\0', sizeof(MQTTAsync_queuedCommand));
			conn->client = m;
			conn->command = m->connect; 
			Log(TRACE_MIN, -1, "Connect failed, more to try");
			MQTTAsync_addCommand(conn, sizeof(m->connect));
		}
		else
		{
			MQTTAsync_closeSession(m->c);
			MQTTAsync_freeConnect(m->connect);
			if (m->connect.onFailure)
			{
				Log(TRACE_MIN, -1, "Calling connect failure for client %s", m->c->clientID);
				(*(m->connect.onFailure))(m->connect.context, NULL);
			}
		}
	}
	FUNC_EXIT_RC(rc);
	return rc;
}


MQTTPacket* MQTTAsync_cycle(int* sock, unsigned long timeout, int* rc)
{
	struct timeval tp = {0L, 0L};
	static Ack ack;
	MQTTPacket* pack = NULL;
	static int nosockets_count = 0;

	FUNC_ENTRY;
	if (timeout > 0L)
	{
		tp.tv_sec = timeout / 1000;
		tp.tv_usec = (timeout % 1000) * 1000; /* this field is microseconds! */
	}

#if defined(NS_ENABLE_SSL)
	if ((*sock = SSLSocket_getPendingRead()) == -1)
	{
#endif
		/* 0 from getReadySocket indicates no work to do, -1 == error, but can happen normally */
		*sock = Socket_getReadySocket(0, &tp);
		if (!tostop && *sock == 0 && (tp.tv_sec > 0L || tp.tv_usec > 0L))
		{
			MQTTAsync_sleep(100L);
#if 0
			if (s.clientsds->count == 0)
			{
				if (++nosockets_count == 50) /* 5 seconds with no sockets */
					tostop = 1;
			}
#endif
		}
		else
			nosockets_count = 0;
#if defined(NS_ENABLE_SSL)
	}
#endif
	MQTTAsync_lock_mutex(mqttasync_mutex);
	if (*sock > 0)
	{
		MQTTAsyncs* m = NULL;
		if (ListFindItem(handles, sock, clientSockCompare) != NULL)
			m = (MQTTAsync)(handles->current->content);
		if (m != NULL)
		{
			if (m->c->connect_state == 1 || m->c->connect_state == 2)
				*rc = MQTTAsync_connecting(m);
			else
				pack = MQTTPacket_Factory(&m->c->net, rc);
			if (m->c->connect_state == 3 && *rc == SOCKET_ERROR)
			{
				Log(TRACE_MINIMUM, -1, "CONNECT sent but MQTTPacket_Factory has returned SOCKET_ERROR");
				if (MQTTAsync_checkConn(&m->connect, m))
				{
					MQTTAsync_queuedCommand* conn;

					MQTTAsync_closeOnly(m->c);
					/* put the connect command back to the head of the command queue, using the next serverURI */
					conn = malloc(sizeof(MQTTAsync_queuedCommand));
					memset(conn, '\0', sizeof(MQTTAsync_queuedCommand));
					conn->client = m;
					conn->command = m->connect;
					Log(TRACE_MIN, -1, "Connect failed, more to try");
					MQTTAsync_addCommand(conn, sizeof(m->connect));
				}
				else
				{
					MQTTAsync_closeSession(m->c);
					MQTTAsync_freeConnect(m->connect);
					if (m->connect.onFailure)
					{
						Log(TRACE_MIN, -1, "Calling connect failure for client %s", m->c->clientID);
						(*(m->connect.onFailure))(m->connect.context, NULL);
					}
				}
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
				if (!m)
					Log(LOG_ERROR, -1, "PUBCOMP or PUBACK received for no client, msgid %d", msgid);
				if (m)
				{
					ListElement* current = NULL;
					
					if (m->dc)
					{
						Log(TRACE_MIN, -1, "Calling deliveryComplete for client %s, msgid %d", m->c->clientID, msgid);
						(*(m->dc))(m->context, msgid);
					}
					/* use the msgid to find the callback to be called */
					while (ListNextElement(m->responses, &current))
					{
						MQTTAsync_queuedCommand* command = (MQTTAsync_queuedCommand*)(current->content);
						if (command->command.token == msgid)
						{		
							if (!ListDetach(m->responses, command)) /* then remove the response from the list */
								Log(LOG_ERROR, -1, "Publish command not removed from command list");
							if (command->command.onSuccess)
							{
								MQTTAsync_successData data;
								
								data.token = command->command.token;
								data.alt.pub.destinationName = command->command.details.pub.destinationName;
								data.alt.pub.message.payload = command->command.details.pub.payload;
								data.alt.pub.message.payloadlen = command->command.details.pub.payloadlen;
								data.alt.pub.message.qos = command->command.details.pub.qos;
								data.alt.pub.message.retained = command->command.details.pub.retained;
								Log(TRACE_MIN, -1, "Calling publish success for client %s", m->c->clientID);
								(*(command->command.onSuccess))(command->command.context, &data);
							}
							MQTTAsync_freeCommand(command);
							break;
						}
					}
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
	MQTTAsync_retry();
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(*rc);
	return pack;
}


int pubCompare(void* a, void* b)
{
	Messages* msg = (Messages*)a;
	return msg->publish == (Publications*)b;
}


int MQTTAsync_getPendingTokens(MQTTAsync handle, MQTTAsync_token **tokens)
{
	ListElement *current = NULL;
	MQTTAsyncs* m = handle;
	int rc = MQTTASYNC_SUCCESS;
	int count = 0;
	*tokens = NULL;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (m == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}

	/* calculate the number of pending tokens - commands plus inflight */
	while (ListNextElement(commands, &current))
	{
		MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)(current->content);

		if (cmd->client == m)
			count++;
	}
	if (m->c)
		count += m->c->outboundMsgs->count;
	if (count == 0)
		goto exit; /* no tokens to return */
	*tokens = malloc(sizeof(MQTTAsync_token) * (count + 1));  /* add space for sentinel at end of list */

	/* First add the unprocessed commands to the pending tokens */
	current = NULL;
	count = 0;
	while (ListNextElement(commands, &current))
	{
		MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)(current->content);

		if (cmd->client == m)
			(*tokens)[count++] = cmd->command.token;
	}

	/* Now add the inflight messages */
	if (m->c && m->c->outboundMsgs->count > 0)
	{
		current = NULL;
		while (ListNextElement(m->c->outboundMsgs, &current))
		{
			Messages* m = (Messages*)(current->content);
			(*tokens)[count++] = m->msgid;
		}
	}
	(*tokens)[count] = -1; /* indicate end of list */

exit:
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_isComplete(MQTTAsync handle, MQTTAsync_token dt)
{
	int rc = MQTTASYNC_SUCCESS;
	MQTTAsyncs* m = handle;
	ListElement* current = NULL;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (m == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}

	/* First check unprocessed commands */
	current = NULL;
	while (ListNextElement(commands, &current))
	{
		MQTTAsync_queuedCommand* cmd = (MQTTAsync_queuedCommand*)(current->content);

		if (cmd->client == m && cmd->command.token == dt)
			goto exit;
	}

	/* Now check the inflight messages */
	if (m->c && m->c->outboundMsgs->count > 0)
	{
		current = NULL;
		while (ListNextElement(m->c->outboundMsgs, &current))
		{
			Messages* m = (Messages*)(current->content);
			if (m->msgid == dt)
				goto exit;
		}
	}
	rc = MQTTASYNC_TRUE; /* Can't find it, so it must be complete */

exit:
	MQTTAsync_unlock_mutex(mqttasync_mutex);
	FUNC_EXIT_RC(rc);
	return rc;
}


int MQTTAsync_waitForCompletion(MQTTAsync handle, MQTTAsync_token dt, unsigned long timeout)
{
	int rc = MQTTASYNC_FAILURE;
	START_TIME_TYPE start = MQTTAsync_start_clock();
	unsigned long elapsed = 0L;
	MQTTAsyncs* m = handle;

	FUNC_ENTRY;
	MQTTAsync_lock_mutex(mqttasync_mutex);

	if (m == NULL || m->c == NULL)
	{
		rc = MQTTASYNC_FAILURE;
		goto exit;
	}
	if (m->c->connected == 0)
	{
		rc = MQTTASYNC_DISCONNECTED;
		goto exit;
	}
	MQTTAsync_unlock_mutex(mqttasync_mutex);

	if (MQTTAsync_isComplete(handle, dt) == 1)
	{
		rc = MQTTASYNC_SUCCESS; /* well we couldn't find it */
		goto exit;
	}

	elapsed = MQTTAsync_elapsed(start);
	while (elapsed < timeout)
	{
		MQTTAsync_sleep(100);
		if (MQTTAsync_isComplete(handle, dt) == 1)
		{
			rc = MQTTASYNC_SUCCESS; /* well we couldn't find it */
			goto exit;
		}
		elapsed = MQTTAsync_elapsed(start);
	}
exit:
	FUNC_EXIT_RC(rc);
	return rc;
}



void MQTTAsync_setTraceLevel(enum MQTTASYNC_TRACE_LEVELS level)
{
	Log_setTraceLevel((enum LOG_LEVELS)level);
}


void MQTTAsync_setTraceCallback(MQTTAsync_traceCallback* callback)
{
	Log_setTraceCallback((Log_traceCallback*)callback);
}


MQTTAsync_nameValue* MQTTAsync_getVersionInfo()
{
	#define MAX_INFO_STRINGS 8
	static MQTTAsync_nameValue libinfo[MAX_INFO_STRINGS + 1];
	int i = 0; 
	
	libinfo[i].name = "Product name";
	libinfo[i++].value = "Paho Asynchronous MQTT C Client Library";
	
	libinfo[i].name = "Version";
	libinfo[i++].value = CLIENT_VERSION;
	
	libinfo[i].name = "Build level";
	libinfo[i++].value = BUILD_TIMESTAMP;
#if defined(NS_ENABLE_SSL)
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
