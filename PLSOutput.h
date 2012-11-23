////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_plsoutput_h
#define xdaq_plsoutput_h

////////////////////////////////////////////////////////////////////////////////

// system included
#include <stdexcept>
#include <sstream>

// local includes
#include "XOUT.h"                                 // <--- this is the super class

// get the namespace
namespace xdaq
{

    // the class definition
    class PLSOutput: public XOUT
    {
        public:

            // constructor
            PLSOutput ( void );                   ///< standard constructor
                                                  ///< constructor that has flags, and nStreams
            PLSOutput ( bool, bool, unsigned int );

            // destructor
            virtual ~PLSOutput() {}

            // can override
            void     init              ( bool, bool, unsigned int );

            /// have to over ride
            void     writeRecordXOUT ( const XPulseRec &PulseRec );

        private:
            void     writeHeaderXOUT ( XPulseRec PulseRec );

            bool     m_evenChannelSignedFlag;     ///< whether even channels are signed
            bool     m_oddChannelSignedFlag;      ///< whether odd channels are signed
            unsigned int m_nStream;               ///< the total number of streams

    };                                            // end of class def

}                                                 // end of name space


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
