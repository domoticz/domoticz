#include "stdafx.h"
#include "csocket.h"
#include "../main/Logger.h"


#ifdef WIN32
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <unistd.h>
#endif

#undef INVALID_SOCKET
#define INVALID_SOCKET  (unsigned int)(~0)
#define BLOCK_RETRY_INTERVAL_MSECS 1
#define ERROR_INAPPROPRIATE_STATE -1

static bool s_bSocketsInitialized = false;;

#define SUCCESS     0
#define FAILURE     1

csocket::csocket() : m_socketState(CLOSED), 
                     m_remotePort(0)
{
    if ( ! s_bSocketsInitialized )
    {
#ifdef WIN32
        WORD socketVersion;  
        WSADATA socketData; 

        socketVersion = MAKEWORD(2, 0); 
		int ret = WSAStartup(socketVersion, &socketData);
		if (ret != 0)
		{
			ret = WSAGetLastError();

			if (ret == WSANOTINITIALISED)
			{
				_log.Log(LOG_ERROR, "Domoticz: Winsock could not be initialized!");
			}
		}
#endif
    }
     
    m_socket = 0;
}

void csocket::close()
{
	 if ( m_socketState != csocket::CLOSED || m_socketState != csocket::ERRORED)
    {
#ifdef WIN32
        closesocket(m_socket);
#else
        ::close(m_socket);
#endif
		m_socketState = CLOSED;
		m_socket = 0;
    }
}
csocket::~csocket()
{
   close();
}


int csocket::resolveHost(const std::string& szRemoteHostName, struct hostent** pHostEnt)
{
    if (szRemoteHostName.length() == 0) 
    {
        return FAILURE;
    }

    if ( ! s_bSocketsInitialized )
    {
#ifdef WIN32
        WORD wVersionRequested;  
        WSADATA wsaData; 
        wVersionRequested = MAKEWORD(2, 0); 
		int ret = WSAStartup(wVersionRequested, &wsaData);
		if (ret != 0)
		{
			ret = WSAGetLastError();

			if (ret == WSANOTINITIALISED)
			{
				_log.Log(LOG_ERROR, "Domoticz: Winsock could not be initialized!");
			}
		}
#endif
    }

    *pHostEnt = gethostbyname( szRemoteHostName.c_str() );

    if (*pHostEnt == NULL) 
    {
        unsigned long uhostname = inet_addr ( szRemoteHostName.c_str() );
        *pHostEnt = gethostbyaddr( reinterpret_cast<char *>(&uhostname), 
            sizeof(unsigned long),
            AF_INET);

        if (*pHostEnt == NULL)
        {
            return FAILURE;
        }
    }

    return SUCCESS;
}


int csocket::connect( const char* remoteHost, const unsigned int remotePort )
{
    if ( m_socketState != CLOSED ) 
    {
        return ERROR_INAPPROPRIATE_STATE;
    }

    struct hostent *pHostEnt = NULL;
    int status = resolveHost(remoteHost, &pHostEnt);
    if (status == FAILURE || !pHostEnt) 
    {
        return FAILURE;
    }

    m_strRemoteHost = remoteHost;
    m_remotePort = remotePort;

    m_remoteSocketAddr.sin_family = pHostEnt->h_addrtype;
    memcpy((char *) &m_remoteSocketAddr.sin_addr.s_addr,
        pHostEnt->h_addr_list[0], 
        pHostEnt->h_length);

    m_remoteSocketAddr.sin_port = htons(m_remotePort);


#ifdef WIN32
    m_socket = WSASocket(AF_INET, SOCK_STREAM, 
        IPPROTO_TCP, 0 , 0 , 0);

    if (m_socket == INVALID_SOCKET)
    {
        return FAILURE;
    }
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
#endif

	if (m_socket == INVALID_SOCKET)
    {
        return FAILURE;
    }

    m_localSocketAddr.sin_family = AF_INET;

    m_localSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int         iRecvTimeout = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, 
        (char*) &iRecvTimeout, sizeof(iRecvTimeout) );

    m_localSocketAddr.sin_port = htons(0);

    status = bind( m_socket, 
        (const sockaddr*)&(m_localSocketAddr),  
        sizeof(struct sockaddr) );

    if (status < 0) 
    {
        return FAILURE;
    }

#ifdef WIN32 
    int set = 1;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,  (char*) &set, sizeof(set) );
#endif

    m_remoteSocketAddr.sin_family = pHostEnt->h_addrtype;

    memcpy(&(m_remoteSocketAddr.sin_addr), 
                pHostEnt->h_addr, 
                pHostEnt->h_length);

    m_remoteSocketAddr.sin_port = htons( m_remotePort );

    status = ::connect(m_socket, 
                    (const sockaddr*)&(m_remoteSocketAddr), 
                    sizeof(m_remoteSocketAddr)  );

    if (status < 0) 
    {
        return FAILURE;
    }

    m_socketState = CONNECTED;

    return SUCCESS;
}


int csocket::canRead( bool* readyToRead, float waitTime )
{
    if (m_socketState != CONNECTED )
    {
        return ERROR_INAPPROPRIATE_STATE;
    }

    fd_set  fds;
    FD_ZERO(&fds);
    FD_SET(m_socket, &fds);
    int     nfds = 0;

    timeval timeout;

    if ( waitTime <= 0.0f ) 
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    } 
    else 
    {
		timeout.tv_sec = static_cast<int>((waitTime));
		timeout.tv_usec = static_cast<int>((1000000.0f * (waitTime - (float)timeout.tv_sec)));
    }

    
//#ifdef WIN32
    nfds = m_socket+1;
//#endif

    int n = select(nfds, &fds, NULL, NULL, &timeout);
    if ( n < 0 ) 
    {
        m_socketState = ERRORED;
        return FAILURE;
    }

    if (n == 1) 
    {
        *readyToRead = true;
        return SUCCESS;
    }

    if (n == 0)
    {
        *readyToRead = false;
    }

    return SUCCESS;
}


int csocket::read( char* pDataBuffer, unsigned int numBytesToRead, bool bReadAll )
{
    if (m_socketState != CONNECTED ) 
    {
        return ERROR_INAPPROPRIATE_STATE;
    }

    int numBytesRemaining = numBytesToRead;

	do 
    {
#ifdef WIN32
        int numBytesRead = recv( m_socket, pDataBuffer, numBytesRemaining, 0 );
#else
        int numBytesRead = ::read( m_socket, pDataBuffer, numBytesRemaining);
#endif

        if (numBytesRead < 0)
        {
            if ( bReadAll ) 
            {
#ifdef WIN32
                Sleep(BLOCK_RETRY_INTERVAL_MSECS);
#else
                usleep(BLOCK_RETRY_INTERVAL_MSECS * 1000);
#endif
            } 
            else 
            {
                return numBytesRead;
            }
        } 
        
        if ( numBytesRead > 0 ) 
        {
            numBytesRemaining -= numBytesRead;
            pDataBuffer += numBytesRead;
        }

        if (numBytesRead == 0)
        {
            break;              
        }
        
    } while ((numBytesRemaining > 0) && (bReadAll) );

    return(numBytesToRead - numBytesRemaining);
}


int csocket::write( const char* pDataBuffer, unsigned int numBytesToWrite )
{
    if (m_socketState != CONNECTED ) 
    {
        return ERROR_INAPPROPRIATE_STATE;
    }

    int numBytesRemaining = numBytesToWrite;

    while (numBytesRemaining  > 0) 
    {
#ifdef WIN32
        int numBytesWritten = send ( m_socket, pDataBuffer, numBytesRemaining , 0 );
#else
        int numBytesWritten= ::write( m_socket, pDataBuffer, numBytesRemaining );
#endif

        if (numBytesWritten < 0) 
        {
            m_socketState = ERRORED;
            return numBytesWritten;      
        }

        numBytesRemaining  -= numBytesWritten;
        pDataBuffer += numBytesWritten;
    }

    return(numBytesToWrite - numBytesRemaining);
}

csocket::SocketState csocket::getState( void ) const 
{ 
    return m_socketState; 
}
