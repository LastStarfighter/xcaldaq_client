////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_xout_h
#define xdaq_xout_h

////////////////////////////////////////////////////////////////////////////////

// system includes
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>

// local includes
#include "XPulseRec.h"
#include "XCALDAQLock.h"

// fltk includes
#include <FL/filename.H>                          // need this for the filename_name function, you should remove this so it is indepenedent from fl libs

// start namespace
namespace xdaq
{

    /**
     XOUT is the base class from which all data file writers inherit.  it contains
     basic functionality for opening and closing files as well as some encapsulated
     functions for checking the header and setting the max pulses to record.
     */
    class XOUT
    {
        public:
            // constructor
            XOUT            ( void );

            // destructor
            virtual     ~XOUT            ( void ){}

            // commands that have to be overridden by its subclasses
                                                  ///< take the pulse and process it
            virtual void writeRecordXOUT ( const XPulseRec &PulseRec) = 0;

            // common commands
                                                  ///< open the file for writing
            virtual void openXOUT        ( char *filename, char *mode );
            virtual void closeXOUT       ( void );///< close file
            virtual void readCFGFile     ( void ) ///< read the cfg file (not all writers use this so declare it to be empty)
            {
            };
            virtual void saveCFGFile     ( void ) ///< save the cfg file (not all writers use this so declare it to be empty)
            {
            };

            // note, the reason we don't declare virtual initXOUT here even though several writers use it is that all of the
            // inits take different variables.  we could overload, but it would be messy.  cast and call.

            // query only
            FILE*        dataWriterFD    ( void );

            // full encapsulation
            bool         writingFlag     ( void );
            void         writingFlag     ( bool );
            bool         headerFlag      ( void );
            void         headerFlag      ( bool );
            int          recordWritten   ( void );
            void         recordWritten   ( int );
            int          recordMax       ( void );
            virtual void recordMax       ( int );
            std::string  fileName        ( void );
            void         fileName        ( std::string );
            std::string  filePath        ( void );
            void         filePath        ( std::string );

            // globals have to do this for ljh
            char**       header;                  ///< the header (used by ljh)
            char**       title;                   ///< the title (used by ljh)

        protected:
            bool         m_writingFlag;           ///< flag for whether the writer is writing or not
            XCALDAQLock  m_lock;                  ///< Mutex object for XOUT file access
            FILE        *m_dataWriterFD;          ///< the file descriptor
            int          m_recordWritten;         ///< number of records written
            int          m_recordMax;             ///< maximum number of records to write
            char         m_mode[8];               // holds the file open mode

        private:
            std::string  m_fileName;              ///< the file name
            std::string  m_filePath;              ///< the path

            // flag
            bool         m_headerFlag;            ///< flag for whether the header has been written

    };                                            // end class defintion
}                                                 // end namespace


////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
