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
 *    Ian Craggs - async client updates
 *******************************************************************************/

#if !defined(SOCKET_H)
#define SOCKET_H

#include <sys/types.h>

#if defined(WIN32) || defined(WIN64)
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#if !defined(SSLSOCKET_H)
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTCONN WSAENOTCONN
#define ECONNRESET WSAECONNRESET
#define ETIMEDOUT WAIT_TIMEOUT
#endif
#define ioctl ioctlsocket
#define socklen_t int
#else
#define INVALID_SOCKET SOCKET_ERROR
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#endif

/** socket operation completed successfully */
#define TCPSOCKET_COMPLETE 0
#if !defined(SOCKET_ERROR)
	/** error in socket operation */
	#define SOCKET_ERROR -1
#endif
/** must be the same as SOCKETBUFFER_INTERRUPTED */
#define TCPSOCKET_INTERRUPTED -22
#define SSL_FATAL -3

#if !defined(INET6_ADDRSTRLEN)
#define INET6_ADDRSTRLEN 46 /** only needed for gcc/cygwin on windows */
#endif


#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

#include "LinkedList.h"

/*BE
def FD_SET
{
   128 n8 "data"
}

def SOCKETS
{
	FD_SET "rset"
	FD_SET "rset_saved"
	n32 dec "maxfdp1"
	n32 ptr INTList "clientsds"
	n32 ptr INTItem "cur_clientsds"
	n32 ptr INTList "connect_pending"
	n32 ptr INTList "write_pending"
	FD_SET "pending_wset"
}
BE*/


/**
 * Structure to hold all socket data for the module
 */
typedef struct
{
	fd_set rset, /**< socket read set (see select doc) */
		rset_saved; /**< saved socket read set */
	int maxfdp1; /**< max descriptor used +1 (again see select doc) */
	List* clientsds; /**< list of client socket descriptors */
	ListElement* cur_clientsds; /**< current client socket descriptor (iterator) */
	List* connect_pending; /**< list of sockets for which a connect is pending */
	List* write_pending; /**< list of sockets for which a write is pending */
	fd_set pending_wset; /**< socket pending write set for select */
} Sockets;


void Socket_outInitialize(void);
void Socket_outTerminate(void);
int Socket_getReadySocket(int more_work, struct timeval *tp);
int Socket_getch(int socket, char* c);
char *Socket_getdata(int socket, int bytes, int* actual_len);
int Socket_putdatas(int socket, char* buf0, size_t buf0len, int count, char** buffers, size_t* buflens, int* frees);
void Socket_close(int socket);
int Socket_new(char* addr, int port, int* socket);

int Socket_noPendingWrites(int socket);
char* Socket_getpeer(int sock);

void Socket_addPendingWrite(int socket);
void Socket_clearPendingWrite(int socket);

typedef void Socket_writeComplete(int socket);
void Socket_setWriteCompleteCallback(Socket_writeComplete*);

#endif /* SOCKET_H */
