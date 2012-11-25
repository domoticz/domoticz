
#if !defined( sock_port_h )
#define sock_port_h

// There are differences between Linux/Berkeley sockets and WinSock2
// This file wraps some of the more common ones (as used by this library).

#include <stdarg.h>

#if defined( WIN32 )

 // Windows features go here
 #include <Winsock2.h>
 #include <stdio.h>
 
 #if !defined( NEED_GETLASTERROR )
  #define NEED_GETLASTERROR 1
 #endif

 typedef int socklen_t;
 #define MSG_NOSIGNAL 0
 #define MAKE_SOCKET_NONBLOCKING(x,r) \
   do { u_long _x = 1; (r) = ioctlsocket( (x), FIONBIO, &_x ); } while(0)
 #define NONBLOCK_MSG_SEND 0
 #define INIT_SOCKET_LIBRARY() \
   do { WSADATA wsaData; WSAStartup( MAKEWORD(2,2), &wsaData ); } while(0)

 #pragma warning( disable: 4250 )

 #define SIN_ADDR_UINT(x) \
   ((uint&)(x).S_un.S_addr)
 #define BAD_SOCKET_FD 0xffffffffU
 
#else

 // Linux features go here
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <sys/time.h>
 #include <sys/poll.h>

 #if !defined( NEED_IOCTLSOCKET )
  #define NEED_IOCTLSOCKET 1
 #endif
 #if !defined( NEED_ERRNO )
  #define NEED_ERRNO 1
 #endif

 extern int h_errno;
 // hack -- I don't make it non-blocking; instead, I pass 
 // NONBLOCK_MSG_SEND for each call to sendto()
 #define MAKE_SOCKET_NONBLOCKING(x,r) do { (r) = 0; } while(0)
 #define NONBLOCK_MSG_SEND MSG_DONTWAIT
 #define INIT_SOCKET_LIBRARY() do {} while(0)

 #if !defined( SOCKET )
  #define SOCKET int
 #endif
 #define SIN_ADDR_UINT(x) \
   ((uint&)(x).s_addr)
 #define BAD_SOCKET_FD -1

#endif

#if NEED_GETLASTERROR
 #define SOCKET_ERRNO WSAGetLastError()
 inline bool SOCKET_WOULDBLOCK_ERROR( int e ) { return e == WSAEWOULDBLOCK; }
 inline bool SOCKET_NEED_REOPEN( int e ) { return e == WSAECONNRESET; }
#endif

#if NEED_ERRNO
 #include <errno.h>
 #define SOCKET_ERRNO errno
 inline bool SOCKET_WOULDBLOCK_ERROR( int e ) { return e == EWOULDBLOCK; }
 inline bool SOCKET_NEED_REOPEN( int e ) { return false; }
#endif


#endif  //  sock_port_h

