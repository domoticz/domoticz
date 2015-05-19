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

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"

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


void myconnect(MQTTClient* client, MQTTClient_connectOptions* opts)
{
	printf("Connecting\n");
	if (MQTTClient_connect(*client, opts) != 0)
	{
		printf("Failed to connect\n");
		exit(-1);	
	}
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

int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* m)
{
	/* not expecting any messages */
	return 1;
}

int main(int argc, char** argv)
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
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

	rc = MQTTClient_create(&client, url, opts.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);

	rc = MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);

	conn_opts.keepAliveInterval = 10;
	conn_opts.reliable = 0;
	conn_opts.cleansession = 1;
	conn_opts.username = opts.username;
	conn_opts.password = opts.password;
	
	myconnect(&client, &conn_opts);

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
		rc = MQTTClient_publish(client, topic, data_len, buffer, opts.qos, opts.retained, NULL);
		if (rc != 0)
		{
			myconnect(&client, &conn_opts);
			rc = MQTTClient_publish(client, topic, data_len, buffer, opts.qos, opts.retained, NULL);
		}
		if (opts.qos > 0)
			MQTTClient_yield();
	}
	
	printf("Stopping\n");
	
	free(buffer);

	MQTTClient_disconnect(client, 0);

 	MQTTClient_destroy(&client);

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

