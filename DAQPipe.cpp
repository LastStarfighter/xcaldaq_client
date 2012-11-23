////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "DAQPipe.h"

////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    DAQPipe::DAQPipe()
    {
        // at the beginning, the fd is not valid
        m_fdValid = false;
        m_socketID = 0;

        // set the host
        sprintf (m_hostid,"%s","localhost");

        // set the default port
        m_port = DEFAULT_XCALDAQ_PORT;

    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::connect()
    {

        printf("Connecting to %s on port %d... ",m_hostid,m_port);

        #ifdef WIN32
        {
            struct sockaddr_in server;
            hostent *hp;
            SOCKET sock;
            int retval;

            if (! (hp = gethostbyname(m_hostid))
                || (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                std::cerr << "Error resolving XCALDAQ server spec: "<<WSAGetLastError() << std::endl;
                throw std::runtime_error("Failed to resolve XCALDAQ specification");
            }
            memcpy((char *)&server.sin_addr, hp->h_addr_list[0], hp->h_length);
            server.sin_port = htons((short)portnum);
            server.sin_family = AF_INET;

            do
            retval = connect(sock, (struct sockaddr *) &server, sizeof(server));
            while (retval == -1 && errno == EINTR);
            if (retval == -1)
            {
                closesocket(sock);
                throw std::runtime_error("Failed to connect to XCALDAQ server");
            }
            m_socketID = sock;
        }
        #else
        {
            m_socketID = -1;
            // try to get an adress once
            m_socketID = u_connect(m_port,m_hostid);

            if (m_socketID == -1)
            {
                // print an error
                std::cout << std::endl << "Can't connect to host "<< m_hostid << " on port " << m_port;

                // throw std::runtime_error("Failed to connect to XCALDAQ server");
                m_fdValid = false;

                std::cout << "failed!" << std::endl;

            }
            else
            {
                m_fdValid = true;
                std::cout << "done!" << std::endl;
            }

        }
        #endif

    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::disconnect()
    {
        if (m_socketID != 0)
        {
            close (m_socketID);
        }
        m_fdValid = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool DAQPipe::gread(unsigned char *buff,int nBytes, int time)
    {
        struct timeval currentTime;
        unsigned long long int failtime, now;
        gettimeofday(&currentTime,NULL);
        failtime = currentTime.tv_sec * 1000000LL + currentTime.tv_usec;
        failtime += (time * 1000);                // remember that time is in mS and failtime is in useconds

        int gotBytes = 0;
        bool success = true;
        while ((nBytes > 0) & (m_fdValid))
        {
            gettimeofday(&currentTime,NULL);
            now = currentTime.tv_sec * 1000000LL + currentTime.tv_usec;
            if (now > failtime)
            {
                success = false;
                break;
            }
            int nGot = DAQPipe::read(buff+gotBytes,nBytes);
            nBytes -= nGot;
            gotBytes += nGot;
        }

        return success;
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::gread(unsigned char *buff,int nBytes)
    {
        int gotBytes = 0;
        while ((nBytes > 0) & (m_fdValid))
        {
            int nGot = DAQPipe::read(buff+gotBytes,nBytes);
            nBytes -= nGot;
            gotBytes += nGot;

            if (!m_fdValid)
            {
                return;
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned int DAQPipe::read(unsigned char *buff,int nBytes)
    {
        if (!m_fdValid)
        {
            std::cout << "Read failed.  Connection is no longer valid." << std::endl; fflush(0);
            return 0;
        }

        #ifdef WIN32
        unsigned int retval;
        do
        retval = recv(m_socketID, (char *)buff, nBytes, 0);
        while (retval == -1 && errno == EINTR);
        if (retval == -1)
            throw std::runtime_error("Error reading from DAQ Pipe.");
        else
            return retval;
        #else
        int nGot = ::read(m_socketID,buff,nBytes);
        if (nGot < 0)
        {
            if (errno == EAGAIN)                  // this is the same thing as EWOULDBLOCK
            {
                return 0;                         // didn't write anything, but try again later
            }
            else
            {
                // this is a real error we need to be concerned abut
                std::cout << "Pipe read returned error "<< errno << std::endl; fflush(0);
                //throw std::runtime_error("Error reading from DAQ Pipe.");
                m_fdValid = false;
            }
        }
        return nGot;
        #endif
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::gwrite(unsigned char *buff,int nBytes)
    {
        int sentBytes = 0;
        while ((nBytes > 0) & (m_fdValid))
        {
            int nSent = DAQPipe::write(buff+sentBytes,nBytes);
            nBytes -= nSent;
            sentBytes += nSent;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned int DAQPipe::write(unsigned char *buff,int nBytes)
    {
        if (!m_fdValid)
        {
            std::cout << "Write failed.  Connection is no longer valid." << std::endl; fflush(0);
            return 0;
        }

        #ifdef WIN32
        DWORD nSent;
        if (!(nSent = send(m_socketID,(const char *)buff,nBytes,0)))
        {
            std::cerr << "Pipe write returned error "<<GetLastError() << std::endl;fflush(0);
            throw std::runtime_error("Error writing to DAQ Pipe.");
        }
        return (unsigned int) nSent;
        #else
        int nSent = ::write(m_socketID,buff,nBytes);

        if (nSent < 0)
        {
            if (errno == EAGAIN)
            {
                return 0;
            }
            else
            {
                std::cerr << "Pipe write returned error "<< errno << std::endl; fflush(0);
                m_fdValid = false;
                //throw std::runtime_error("Error writing to DAQ Pipe.");
            }
        }
        return nSent;
        #endif
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::readReady (void)
    {

        // register the fd set
        fd_set read_set;

        // initialize it to be zero
        FD_ZERO (&read_set);

        //  register the data communication socket with this set of fd's
        FD_SET (m_socketID, &read_set);

        // register the max fd variable (that is, max fd plus 1)
        int maxfdp1 = m_socketID + 1;

        // the select command will block until the fd has data availble for reading.
        // note, we don't want it to return for write ready, or timeout, so put in NULLs for those
        select(maxfdp1, &read_set, NULL, NULL, NULL);

        return;
    }

    /////////////////////////////////////////////////////////////////////////////
    int DAQPipe::readReady (long sec, long uSec)
    {
        // setup the timeout
        struct timeval timeout;
        timeout.tv_sec = sec;                     // set the timeout to 2 seconds
        timeout.tv_usec = uSec;                   // set the timeout to 2 seconds

        // register the fd set
        fd_set read_set;

        // initialize it to be zero
        FD_ZERO (&read_set);

        //  register the data communication socket with this set of fd's
        FD_SET (m_socketID, &read_set);

        // register the max fd variable (that is, max fd plus 1)
        int maxfdp1 = m_socketID + 1;

        // the select command will block until the fd has data availble for reading.
        // note, we don't want it to return for write ready, or timeout, so put in NULLs for those
        int result = select(maxfdp1, &read_set, NULL, NULL, &timeout);

        return result;
    }

    /////////////////////////////////////////////////////////////////////////////
    // encapsulation
    /////////////////////////////////////////////////////////////////////////////
    char* DAQPipe::hostid(void)
    {
        return m_hostid;
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::hostid(const char* host)
    {
        sprintf (m_hostid,"%s",host);
        std::cout << "Setting host name to " << m_hostid << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned int DAQPipe::port (void)
    {
        return m_port;
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::port (unsigned int p)
    {
        m_port=p;
        std::cout << "Setting port number to " << p << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    int DAQPipe::socketID (void)
    {
        return m_socketID;
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::socketID (int p)
    {
        m_socketID=p;
        // i am assuming that the user knows what he/she is doing, so lets hope this
        // pipe is actually open and valid
        m_fdValid = true;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool DAQPipe::nonBlock (void)
    {
        if (!m_fdValid)
        {
            std::cout << "Pipe is not valid!" << std::endl;
            return false;
        }

        // get the flags from the client FD
        int flgs = fcntl(m_socketID,F_GETFL);

        if (flgs & O_NONBLOCK)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::nonBlock (bool b)
    {
        if (!m_fdValid)
        {
            std::cout << "Pipe is not valid!" << std::endl;
            return;
        }

        // get the flags from the client FD
        int flags = fcntl(m_socketID,F_GETFL);

        if (b)
        {
            //std::cout << "Setting the pipe to NONBLOCK" << std::endl;
            flags = flags | O_NONBLOCK;
        }
        else
        {
            //std::cout << "Setting the pipe to BLOCK" << std::endl;
            flags = flags & ~O_NONBLOCK;
        }

        // send those flags back to the pipe
        fcntl(m_socketID,F_SETFL,flags|O_NONBLOCK);

    }

    /////////////////////////////////////////////////////////////////////////////
    int DAQPipe::maxSeg (void)
    {
        if (!m_fdValid)
        {
            std::cout << "Pipe is not valid!" << std::endl;
            return false;
        }

        int flag=0;
        int size=4;

        int result = getsockopt( m_socketID,      // socket affected
            IPPROTO_TCP,                          // set option at TCP level
            TCP_MAXSEG,                           // name of option
            (char *) &flag,                       // the cast is historical cruft
            (socklen_t*) &size
            );                                    // length of option value

        if (result < 0)
        {
            printf ("that didn't work\n");
            return -1;
        }

        printf ("Daq Pipe Max Seg is set to : %d\n",flag);
        return flag;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool DAQPipe::noDelay (void)
    {
        if (!m_fdValid)
        {
            std::cout << "Pipe is not valid!" << std::endl;
            return false;
        }

        // get the flags from the client FD
        int flgs = fcntl(m_socketID,F_GETFL);

        if (flgs & O_NONBLOCK)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void DAQPipe::noDelay (bool b)
    {
        if (!m_fdValid)
        {
            std::cout << "Pipe is not valid!" << std::endl;
            return;
        }

        int flag = 1;

        if (!b)
        {
            flag  = 0;
        }

        int result = setsockopt( m_socketID,      // socket affected
            IPPROTO_TCP,                          // set option at TCP level
            TCP_NODELAY,                          // name of option
            (char *) &flag,                       // the cast is historical cruft
            sizeof(int)
            );                                    // length of option value

        if (result < 0)
        {
            printf ("that didn't work\n");
        }

    }

    /////////////////////////////////////////////////////////////////////////////

}                                                 // end of namespace xdaq


#ifdef WIN32
////////////////////////////////////////////////////////////////////////////////
void WinSockInit()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 1, 0 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        throw std::runtime_error("Couldn't find usable WinSock DLL");
    }

    if ( LOBYTE( wsaData.wVersion ) != 1 ||
        HIBYTE( wsaData.wVersion ) != 0 )
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup( );
        return;
    }
}


////////////////////////////////////////////////////////////////////////////////
void WinSockCleanup()
{
    WSACleanup();
}
#endif

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
