//Interface to a DAQ server.
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_daqpipe_h
#define xdaq_daqpipe_h

////////////////////////////////////////////////////////////////////////////////

// conditional includes
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include "uici.h"
#include <sys/stat.h>
#include <errno.h>
#endif

// system includes
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <fcntl.h>
#include <sys/time.h>                             // gettimeofday function
#include <sys/socket.h>                           // getsockopt function
#include <netinet/in.h>                           // IPPROTO_TCP
#include <netinet/tcp.h>                          // TCP_NODELAY

// local includes
#include "XCALDAQLock.h"

// macro definitions
#define DEFAULT_XCALDAQ_PORT 2011

// conditional global functions
#ifdef WIN32
void WinSockInit();
void WinSockCleanup();
#endif

namespace xdaq
{

    // class definition
    /**
     generic class for setting up a communications socket.
     simple functions for connecting, disconnecting, reading and writing.
     also has some functionality for indicating when data is ready to be read

     */
    class DAQPipe
    {
        public:
            // constructor
            DAQPipe();

            // init the socket
            void         connect    ( void );     ///< connect to port
            void         disconnect ( void );     ///< disconnect from port and close

            // read nBytes from socket
                                                  ///< if socket is non-blocking, this will read what it can and return
            unsigned int read       ( unsigned char *buff, int nBytes );
                                                  ///< Reads exactly nBytes from socket.  Doesn't return until it's got them (ie blocks)
            void         gread      ( unsigned char *buff, int nBytes );
                                                  ///< Reads exactly nBytes from socket.  fails after time mSeconds and returns false
            bool         gread      ( unsigned char *buff, int nBytes, int time );

            // Write nBytes to socket
                                                  ///< if socket is set to NON_block, this will write what it can and return
            unsigned int write      ( unsigned char *buff,int nBytes );
                                                  ///< Writes exactly nBytes from socket.  Doesn't return until it has wrote them (ie blocks)
            void         gwrite     ( unsigned char *buff,int nBytes );

            // blocks until the socket is ready for reading
            void         readReady  ( void );     ///< block until data is ready to be read on the socket
            int          readReady  (long, long );///< same as read ready, but with a timeout

            // simple encapsulation
            int          maxSeg     ( void );     ///< prints out the sockets max segment
            bool         valid      ( void ) {return m_fdValid;}
            void         valid      ( bool b ) {m_fdValid = b;}
            char*        hostid     ( void );
            void         hostid     ( const char* );
            unsigned int port       ( void );
            void         port       ( unsigned int );
            bool         nonBlock   ( void );
            void         nonBlock   ( bool );     ///< sets the socket's blocking flag
            bool         noDelay    ( void );
            void         noDelay    ( bool );
            int          socketID     ( void );
            void         socketID     ( int );    ///< if an external owner wants to sent an already open socket to the socket

            /// a mutex lock on the socket
            XCALDAQLock lock;

        private:

            // define the socket ID
        #ifdef WIN32
            SOCKET m_socketID;
        #else
            int m_socketID;                       ///< the socket's file descriptor
        #endif

            // variables
            unsigned int m_port;                  ///< the port number
            char m_hostid[256];                   ///< the host id

            // flags
            bool m_fdValid;                       ///< whether this socket is valid

    };                                            // end of class definition
}                                                 // end of namespace xdaq


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
