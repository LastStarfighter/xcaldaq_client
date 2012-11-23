#ifndef xdaq_ndfbserver_h
#define xdaq_ndfbserver_h

//Reads from an AAServer FIFO (run the program "aaserver" which uses the FIFO AASFIFO in the current working directory)
//The server will eventually be "smarter" one way or another, right now it waits for an open on the other end of the
//FIFO and exits gracefully when the FIFO is closed

// conditional includes
#ifdef WIN32
#include <windows.h>
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// system includes
#include <iostream>
#include <deque>

// user includes
#include "XCALDAQCommands.h"                      // uses macros in
#include "XCALDAQRecordConstants.h"               // the form of the data packets
#include "DAQPipe.h"
#include "byteorder64.h"

#define TEMP_BUFFER_SIZE 0x800000                 ///< an 8 meg temp buffer

namespace xdaq
{
    /**
     a generic interface that lets the client talk to an arbitrary server.
     contains two pipes, one for communications and one for data.  very simple
     interface commands like connect and shutdown along with functions for setting
     and getting data from the server.

     also in charge of reading whatever it can from the server and chunking that
     into data packets to be processed by the client
     */
    class XCALDAQServer
    {

        public:

            // constructor
            XCALDAQServer();

            // public functions
            void connect               ( void );  ///< connect to server
            void shutdownServer        ( void );  ///< shutdown the server
                                                  ///< take the temp buffer and parse out the completed data packets
            unsigned int getDataPacket ( std::deque<unsigned char*>* deque, XCALDAQLock *lock);

            // server gets and sets
                                                  ///< set a bool in the server
            int          set           ( unsigned int secondary, unsigned int chan, bool b );
                                                  ///< set an int in the server
            int          set           ( unsigned int secondary, unsigned int chan, unsigned int i );
                                                  ///< set a float on ther server
            int          set           ( unsigned int secondary, unsigned int chan, float f );
                                                  ///< get a bool from the server
            void         get           ( unsigned int secondary, unsigned int chan, bool *b );
                                                  ///< get a short from the server
            void         get           ( unsigned int secondary, unsigned int chan, unsigned short int *s );
                                                  ///< get a long from the server
            void         get           ( unsigned int secondary, unsigned int chan, unsigned int *l );
                                                  ///< get a float from the server
            void         get           ( unsigned int secondary, unsigned int chan, float *f );
                                                  ///< the primary command (get, set, kill)
            unsigned int command       ( unsigned short int primary,
                unsigned short int secondary,     ///< the secondard command
                unsigned int channel,             ///< channel information if applicable
                unsigned int level                ///< what you are actually sending
                );                                ///< send a generic command packet to the server

            // encapsulation
            char*        host          ( void );
            void         host          ( char* );
            int          port          ( void );
            void         port          ( int );

            ///the communications pipe to the server
            DAQPipe *commPipe;

            ///the data pipe to the server
            DAQPipe *dataPipe;

        private:
            /// the temporary buffer where raw data from server is stored temporarily
            unsigned char  m_tempDataBuffer[TEMP_BUFFER_SIZE];

            /// where in the temp buffer new data should be written
            unsigned int   m_tempDataIndex;

    };                                            // end of class definition

}                                                 // end of namespace


////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
