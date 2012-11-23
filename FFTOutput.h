////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_FFTOutput_h
#define xdaq_FFTOutput_h

////////////////////////////////////////////////////////////////////////////////

// system included
#include <stdexcept>
#include <sstream>

// local includes
#include "XOUT.h"                                 // <--- this is the super class
#include "byteorder64.h"
#include <arpa/inet.h>                            // net to host and host to net functions

// get the namespace
namespace xdaq
{

    // the class definition
    class FFTOutput: public XOUT
    {
        public:

            /// constructor
            FFTOutput ( void );

            /// destructor
            virtual ~FFTOutput() {}

            /// have to over ride
                                                  ///< not used, but we have to override
            void     writeRecordXOUT ( const XPulseRec &PulseRec );

            /// the write record function that you actually use
            void     writeRecordXOUT ( unsigned int , double*, double*, unsigned int );

            /// override the record max function
            void         recordMax       ( int );

        private:
            void     writeHeaderXOUT ( void );    ///< the header

    };                                            // end of class def

}                                                 // end of name space


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
