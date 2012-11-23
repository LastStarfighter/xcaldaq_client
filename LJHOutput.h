////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_ljhoutput_h
#define xdaq_ljhoutput_h

////////////////////////////////////////////////////////////////////////////////

// system included
#include <stdexcept>
#include <sstream>
#include <math.h>                                 // round functions
#include <arpa/inet.h>                            // htonl functions

// local includes
#include "XOUT.h"                                 // <--- this is the super class

// get the namespace
namespace xdaq
{

    // the class definition
    class LJHOutput: public XOUT
    {
        public:

            // constructor
            LJHOutput();

            // destructor
            virtual ~LJHOutput() {}

            // over ride functions
            void initXOUT(unsigned int chan);     ///< tell the writer what to expect from the server
                                                  ///< over ride b/c you need to open lots of files
            void openXOUT(char *filename,char *mode);
            void closeXOUT();                     ///< override b/c you need to close lots of files
            void readCFGFile();                   ///< detail out cfg read
            void saveCFGFile();                   ///< detail out cfg save

            // have to override
                                                  // write the record to file
            void writeRecordXOUT(const XPulseRec &PulseRec);

        private:
            void openCFGFile();                   ///< open the cfg file
            void closeCFGFile();                  ///< close the cfg file
            void writeHeaderXOUT(XPulseRec PR);   ///< write the header

            unsigned int m_numChan;               ///< number of channels

            // information about the data that will be put into the header
            bool* m_chanHeader;                   ///< array of flags as to whether or not a header has been writen for a particular file
            FILE** m_chanFD;                      ///< array of file descriptor, one for each channel

            // info from the PR that will be used to write the samples to file
            double *m_range;                      ///< array of ranges
            double *m_offset;                     ///< array of offsets
            int *m_bits;                          ///< array of bits
            int *m_polarity;                      ///< array of polarities

            // flags
            bool m_readyToWriteHeader;            ///< flag indicating that the header can be written
            bool m_initialized;                   ///< flag indicating whether the writer is initialized

            // cfg file stuff
            FILE *cfgFD;                          ///< cfg file descriptor

    };                                            // end of class def

}                                                 // end of name space


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
