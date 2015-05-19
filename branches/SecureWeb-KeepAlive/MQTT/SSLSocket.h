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
 *    Ian Craggs, Allan Stockdill-Mander - initial implementation 
 *******************************************************************************/
#if !defined(SSLSOCKET_H)
#define SSLSOCKET_H

#if defined(WIN32) || defined(WIN64)
	#define ssl_mutex_type HANDLE
#else
	#include <pthread.h>
	#include <semaphore.h>
	#define ssl_mutex_type pthread_mutex_t
#endif

#include <openssl/ssl.h>
#include "SocketBuffer.h"
#include "Clients.h"

#define URI_SSL "ssl://"

int SSLSocket_initialize();
void SSLSocket_terminate();
int SSLSocket_setSocketForSSL(networkHandles* net, MQTTClient_SSLOptions* opts);
int SSLSocket_getch(SSL* ssl, int socket, char* c);
char *SSLSocket_getdata(SSL* ssl, int socket, int bytes, int* actual_len);

int SSLSocket_close(networkHandles* net);
int SSLSocket_putdatas(SSL* ssl, int socket, char* buf0, size_t buf0len, int count, char** buffers, size_t* buflens, int* frees);
int SSLSocket_connect(SSL* ssl, int socket);

int SSLSocket_getPendingRead();
int SSLSocket_continueWrite(pending_writes* pw);

#endif
