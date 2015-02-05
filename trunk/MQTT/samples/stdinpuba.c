/*******************************************************************************
 * Copyright (c) 2012, 2013 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/
 
 /*
 stdin publisher
 
 compulsory parameters:
 
  --topic topic to publish on
 
 defaulted parameters:
 
	--host localhost
	--port 1883
	--qos 0
	--delimiters \n
	--clientid stdin_publisher
	--maxdatalen 100
	
	--userid none
	--password none
 
*/

#include "MQTTAsync.h"

#include <stdio.h>
#include <signal.h>
#include <memory.h>


#if defined(WIN32)
#include <Windows.h>
#define sleep Sleep
#else
#include <sys/time.h>
#include <stdlib.h>
#endif


volatile int toStop = 0;


void usage()
{
	printf("MQTT stdin publisher\n");
	printf("Usage: stdinpub topicname <options>, where options are:\n");
	printf("  --host <hostname> (default is localhost)\n");
	printf("  --port <port> (default is 1883)\n");
	printf("  --qos <qos> (default is 0)\n");
	printf("  --retained (default is off)\n");
	printf("  --delimiter <delim> (default is \\n)");
	printf("  --clientid <clientid> (default is hostname+timestamp)");
	printf("  --maxdatalen 100\n");
	printf("  --username none\n");
	printf("  --password none\n");
	exit(-1);
}



void cfinish(int sig)
{
	signal(SIGINT, NULL);
	toStop = 1;
}


struct
{
	char* clientid;
	char* delimiter;
	int maxdatalen;
	int qos;
	int retained;
	char* username;
	char* password;
	char* host;
	char* port;
  int verbose;
} opts =
{
	"publisher", "\n", 100, 0, 0, NULL, NULL, "localhost", "1883", 0
};

void getopts(int argc, char** argv);

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{
	/* not expecting any messages */
	return 1;
}


static int disconnected = 0;

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	disconnected = 1;
}


static int connected = 0;

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? -1 : response->code);
	connected = -1; 
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	int rc;
	
	printf("Connected\n");
	connected = 1;
}


void myconnect(MQTTAsync* client)
{
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;

	printf("Connecting\n");
	conn_opts.keepAliveInterval = 10;
	conn_opts.cleansession = 1;
	conn_opts.username = opts.username;
	conn_opts.password = opts.password;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	ssl_opts.enableServerCertAuth = 0;
	conn_opts.ssl = &ssl_opts;
	connected = 0;
	if ((rc = MQTTAsync_connect(*client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(-1);	
	}
	while	(connected == 0)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
}


static int published = 0;

void onPublishFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Publish failed, rc %d\n", response ? -1 : response->code);
	published = -1; 
}


void onPublish(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;

	published = 1;
}


void connectionLost(void* context, char* cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_SSLOptions ssl_opts = MQTTAsync_SSLOptions_initializer;
	int rc = 0;

	printf("Connecting\n");
	conn_opts.keepAliveInterval = 10;
	conn_opts.cleansession = 1;
	conn_opts.username = opts.username;
	conn_opts.password = opts.password;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	ssl_opts.enableServerCertAuth = 0;
	conn_opts.ssl = &ssl_opts;
	connected = 0;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(-1);	
	}
}

#if !defined(_WINDOWS)
	#include <sys/time.h>
  	#include <sys/socket.h>
	#include <unistd.h>
  	#include <errno.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTCONN WSAENOTCONN
#define ECONNRESET WSAECONNRESET
#define setenv(a, b, c) _putenv_s(a, b)
#endif


#if !defined(SOCKET_ERROR)
#define SOCKET_ERROR -1
#endif


typedef struct
{
	int socket;
	time_t lastContact;
#if defined(OPENSSL)
	SSL* ssl;
	SSL_CTX* ctx;
#endif
} networkHandles;


typedef struct
{
	char* clientID;					/**< the string id of the client */
	char* username;					/**< MQTT v3.1 user name */
	char* password;					/**< MQTT v3.1 password */
	unsigned int cleansession : 1;	/**< MQTT clean session flag */
	unsigned int connected : 1;		/**< whether it is currently connected */
	unsigned int good : 1; 			/**< if we have an error on the socket we turn this off */
	unsigned int ping_outstanding : 1;
	int connect_state : 4;
	networkHandles net;
/* ... */
} Clients;


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
	#if 0
	MQTTAsync_command connect;				/* Connect operation properties */
	MQTTAsync_command disconnect;			/* Disconnect operation properties */
	MQTTAsync_command* pending_write;       /* Is there a socket write pending? */
	
	List* responses;
	unsigned int command_seqno;						

	MQTTPacket* pack;
#endif
} MQTTAsyncs;

int test6_socket_error(char* aString, int sock)
{
#if defined(WIN32)
	int errno;
#endif

#if defined(WIN32)
	errno = WSAGetLastError();
#endif
	if (errno != EINTR && errno != EAGAIN && errno != EINPROGRESS && errno != EWOULDBLOCK)
	{
		if (strcmp(aString, "shutdown") != 0 || (errno != ENOTCONN && errno != ECONNRESET))
			printf("Socket error %d in %s for socket %d", errno, aString, sock);
	}
	return errno;
}


int test6_socket_close(int socket)
{
	int rc;

#if defined(WIN32)
	if (shutdown(socket, SD_BOTH) == SOCKET_ERROR)
		test6_socket_error("shutdown", socket);
	if ((rc = closesocket(socket)) == SOCKET_ERROR)
		test6_socket_error("close", socket);
#else
	if (shutdown(socket, SHUT_RDWR) == SOCKET_ERROR)
		test6_socket_error("shutdown", socket);
	if ((rc = close(socket)) == SOCKET_ERROR)
		test6_socket_error("close", socket);
#endif
	return rc;
}


int main(int argc, char** argv)
{
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync client;
	char* topic = NULL;
	char* buffer = NULL;
	int rc = 0;
	char url[100];

	if (argc < 2)
		usage();
	
	getopts(argc, argv);
	
	sprintf(url, "%s:%s", opts.host, opts.port);
	if (opts.verbose)
		printf("URL is %s\n", url);
	
	topic = argv[1];
	printf("Using topic %s\n", topic);

	rc = MQTTAsync_create(&client, url, opts.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);

	rc = MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, NULL);

	myconnect(&client);

	buffer = malloc(opts.maxdatalen);
	
	while (!toStop)
	{
		int data_len = 0;
		int delim_len = 0;
		
		delim_len = strlen(opts.delimiter);
		do
		{
			buffer[data_len++] = getchar();
			if (data_len > delim_len)
			{
			//printf("comparing %s %s\n", opts.delimiter, &buffer[data_len - delim_len]);
			if (strncmp(opts.delimiter, &buffer[data_len - delim_len], delim_len) == 0)
				break;
			}
		} while (data_len < opts.maxdatalen);
				
		if (opts.verbose)
				printf("Publishing data of length %d\n", data_len);
		pub_opts.onSuccess = onPublish;
		pub_opts.onFailure = onPublishFailure;
		do
		{
			published = 0;
			rc = MQTTAsync_send(client, topic, data_len, buffer, opts.qos, opts.retained, &pub_opts);
			while (published == 0)
				#if defined(WIN32)
				Sleep(100);
				#else
				usleep(1000L);
				#endif
			if (published == -1)
				myconnect(&client);
			test6_socket_close(((MQTTAsyncs*)client)->c->net.socket);
		}
		while (published != 1);
	}
	
	printf("Stopping\n");
	
	free(buffer);

	disc_opts.onSuccess = onDisconnect;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
	    exit(-1);	
	}

	while	(!disconnected)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

 	MQTTAsync_destroy(&client);

	return 0;
}

void getopts(int argc, char** argv)
{
	int count = 2;
	
	while (count < argc)
	{
		if (strcmp(argv[count], "--retained") == 0)
			opts.retained = 1;
		if (strcmp(argv[count], "--verbose") == 0)
			opts.verbose = 1;
		else if (strcmp(argv[count], "--qos") == 0)
		{
			if (++count < argc)
			{
				if (strcmp(argv[count], "0") == 0)
					opts.qos = 0;
				else if (strcmp(argv[count], "1") == 0)
					opts.qos = 1;
				else if (strcmp(argv[count], "2") == 0)
					opts.qos = 2;
				else
					usage();
			}
			else
				usage();
		}
		else if (strcmp(argv[count], "--host") == 0)
		{
			if (++count < argc)
				opts.host = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--port") == 0)
		{
			if (++count < argc)
				opts.port = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--clientid") == 0)
		{
			if (++count < argc)
				opts.clientid = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--username") == 0)
		{
			if (++count < argc)
				opts.username = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--password") == 0)
		{
			if (++count < argc)
				opts.password = argv[count];
			else
				usage();
		}
		else if (strcmp(argv[count], "--maxdatalen") == 0)
		{
			if (++count < argc)
				opts.maxdatalen = atoi(argv[count]);
			else
				usage();
		}
		else if (strcmp(argv[count], "--delimiter") == 0)
		{
			if (++count < argc)
				opts.delimiter = argv[count];
			else
				usage();
		}
		count++;
	}
	
}

