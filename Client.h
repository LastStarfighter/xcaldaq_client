////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __Client_h
#define __Client_h
////////////////////////////////////////////////////////////////////////////////

// system includes
#include <iostream>
#include <string>
#include <pthread.h>
#include <sys/time.h>                             // gettimeofday function
#include <cfloat>                                 // limits of floats ie DBL_MAX

// local includes
#include "XCALDAQServer.h"                        // the server class
#include "StreamChannel.h"                        // the stream channel class
#include "FFTChannel.h"                           // the fft channel class
#include "PLSOutput.h"                            // the PLS file writer class
#include "LJHOutput.h"                            // the LJH file writer class
#include "LANLOutput.h"                           // the LANL file writer class
#include "FFTOutput.h"                            // the FFT file writer class

// local macros for mix optimization
// the error on any mix optimization is given by (1/zoomFactor)^iteratios * 1/numDivs
// the bigger the numbers the better the fit however, if zoom factor is too big
// you may get a false positive.  everything else is just how much time you want to spend calculating
#define MIX_NUM_ITERATIONS            8           ///< number of iterations through a range
#define MIX_NUM_DIVISIONS_PER_RANGE 256           ///< number of divisions to divide up the range into
#define MIX_ZOOM_FACTOR               2.          ///< the factor by which  the range shrinks for every iteration
#define MIX_MAX_SEARCH_RANGE          1.          ///< the max ratio that will be considered
#define MIX_MIN_SEARCH_RANGE          0.          ///< the min ratio that will be considered

// pulse generation defines   should be set in a config file
#define PRE_TRIGGER_LENGTH 64                     ///< pretigger length
#define PULSE_LENGTH 2048                         //< length of the pulse

// config file stuff
#define CONFIG_FILE_NAME "xcaldaq_client.cfg"     ///< config file name

// enumeration of the server types and data streams
enum ServerType {UNDEFINED_SERVER, NDFB, IOTECH, ROCKET};
enum DataSource {UNDEFINED_DATA, LIVE, RECORDED};
enum XOUTType {UNDEFINED_XOUT, LJH, LANL, PLS, FFT};

namespace xdaq
{

    /// client is the main interface
    class Client
    {

        public:
            /// constructor
            Client ();

            std::string trigger_config_filename;
            std::string mixing_config_filename;

            // config file functions
                                                  ///< read server config information file
            void           readConfigFile              ( void );
                                                  ///< give keyword and value, put that data into the proper structure
            void           parseConfigFile             ( char*, char* );
                                                  ///< write current server config info to file
            void           saveConfigFile              ( void );
                                                  ///< create a data file from scratch
            void           createConfigFile            ( void );
                                                  ///< open the config file
            void           openConfigFile              ( void );

            // publically available functions used to control the client's behavior
                                                  ///< static wrapper for connect in case you want to call it in it's own thread
            static void  *connect                      ( void* );
                                                  ///< connect to the server, launches command loop
            void          connect                      ( void );
                                                  ///< static wrapper for disconnect in case you want to call it in it's own thread
            static void  *disconnect                   ( void* );
                                                  ///< disconnect, kills command loop
            void          disconnect                   ( void );
                                                  ///< start streaming and processing data, launches data loop
            void          startStreaming               ( void );
                                                  ///< stop streaming, kills data loop
            void          stopStreaming                ( void );
                                                  ///< shuts down the client
            void          shutdown                     ( void );
                                                  ///< ask the client to query the server for basic information about the setup
            void          getBasicServerInfo           ( void );
                                                  ///< ask the client to query the server and refresh local values to match this
            void          updateClientToServer         ( void );

            void push_stream_data_mixing_state_to_server_and_client(void);
                                                  ///< tells the client to take samples and calc optimal mix levels (returns false on error)
            bool          optimalMix_start             ( unsigned int );
                                                  ///< tells the client to calculate and update the trigger rates
            void          updateTriggerRate            ( void );
                                                  ///< tell the server to rewind the data file to the beginning
            void          rewindDataFile               ( void );

            // query only functions
                                                  ///< returns the progress of the connection proces
            float         connectProgress              ( void );
                                                  ///< returns the progress of the optimization progress
            float         optimalMix_progress          ( void );
                                                  ///< returns the data rate
            float         dataRate                     ( void );
                                                  ///< return the version of the remote server
            char         *versionString                ( void );
                                                  ///< returns what type of server you are connected to
            ServerType    serverType                   ( void );
                                                  ///< returns what kind of data you are connected to
            DataSource    dataSource                   ( void );
            unsigned int  nDatatStreams                ( void );
            unsigned int  nMuxCols                     ( void );
            unsigned int  nMuxRows                     ( void );
            unsigned int  nMuxPixels                   ( void );
            XOUT         *XOUTWriter                   ( void );
            bool          connected                    ( void );
            bool          streaming                    ( void );
            bool          channelInit                  ( void );

            // full encapusulation
            void               triggerRateIntegrationTime ( double );
            double             triggerRateIntegrationTime ( void );
            unsigned int       optimizeMixMaxRecords      ( void );
            void               optimizeMixMaxRecords      ( unsigned int );
            void               writePulsesToFileFlag      ( unsigned int, bool );
            bool               writePulsesToFileFlag      ( unsigned int );
            bool               writeAutoToFileFlag        ( void );
            void               writeAutoToFileFlag        ( bool );
            bool               writeLevelToFileFlag       ( void );
            void               writeLevelToFileFlag       ( bool );
            bool               writeEdgeToFileFlag        ( void );
            void               writeEdgeToFileFlag        ( bool );
            bool               writeNoiseToFileFlag       ( void );
            void               writeNoiseToFileFlag       ( bool );
            void               performAnalysisFlag        ( unsigned int, bool );
            bool               performAnalysisFlag        ( unsigned int );
            void               streamDataFlag             ( unsigned int, bool );
            bool               streamDataFlag             ( unsigned int );
            void               coupleOddToEvenFlag        ( bool );
            bool               coupleOddToEvenFlag        ( void );
            void               coupleEvenToOddFlag        ( bool );
            bool               coupleEvenToOddFlag        ( void );
            XOUTType           getXOUTType                ( void );
            void               setXOUTType                ( XOUTType );
            void               noiseRecordsPerFFT         ( int );
            int                noiseRecordsPerFFT         ( void );
            void               fftDiscriminator           ( int );
            int                fftDiscriminator           ( void );
            void               optimizeMixCouplePairsFlag ( bool );
            bool               optimizeMixCouplePairsFlag ( void );
            void               optimizeMixRatioMethod     ( int );
            int                optimizeMixRatioMethod     ( void );
            bool               tiltFlag                   ( void );
            void               tiltFlag                   ( bool );

            // functions that give indirect access to variables on
            // the streamData structures.  this is provided so that common values (such as pretrigger length)
            // can be querried from the client without polling each streamData object.
            // in the case of mix, you should poll each one b/c each one will have a different
            // value.  also it is important that we have this extra layer of abstraction b/c
            // setting these values may trigger the need to communicate these values to the
            // remote server.  since the streamData structures do not have access to the server
            // structure, it must be done in the client object
            bool               mixFlag                    ( unsigned int );
            void               mixFlag                    ( unsigned int, bool );
            void               mixLevel                   ( unsigned int, float );
            float              mixLevel                   ( unsigned int );
            bool               mixInversionFlag           ( unsigned int );
            void               mixInversionFlag           ( unsigned int, bool );
            void               decimateLevel              ( unsigned int, unsigned short);
            unsigned short     decimateLevel              ( unsigned int);
            void               decimateFlag               ( unsigned int, bool b);
            bool               decimateFlag               ( unsigned int);
            void               decimateAvgFlag            ( unsigned int, bool b);
            bool               decimateAvgFlag            ( unsigned int);

            // note, we seperate these particular functions from above b/c they do not
            // require the client to communicate with the server.  they only provide
            // a nice interface for the client gui
            void               pretrig                    ( unsigned int, int );
            int                pretrig                    ( unsigned int );
            void               recLen                     ( unsigned int, int );
            int                recLen                     ( unsigned int );
            float              effectiveSampleRate        ( unsigned int );

            //! Give a formatted list of all trigger for each stream
            std::string pretty_print_trigger_state(void) const;

            void save_trigger_state_to_file(const std::string& basename,bool append_extension=false) const;
            void load_trigger_state_from_file(const std::string& filename);

            std::string pretty_print_mixing_state(void) const;

            void save_mixing_state_to_file(const std::string& basename,bool append_extension=false) const;
            void load_mixing_state_from_file(const std::string& filename);

            void load_generic_config_from_file_into_stream_data(const std::string& filename);

            /// the pointer to the server interface
            XCALDAQServer   *server;

            /// an array of data streams, one for every data stream that you will get from the server
            StreamChannel  **streamData;

            /// an array of fft channels, one fft for every channel
            FFTChannel     **noiseRecordFFT;

            /// publically available triggered pulses
            XPulseRec      **latestTriggeredPulse;// an array of PR's.  1 for each data stream

            /// the lock for the triggered pulses
                                                  // an array of locks,  1 for each data stream
            XCALDAQLock     *latestTriggeredPulseLock;

            /// publically available noise pulses
            XPulseRec      **latestNoisePulse;    // an array of noise PR's.  1 for each data stream

            /// the lock for the noise pulses
            XCALDAQLock     *latestNoisePulseLock;// an array of locks, 1 for each data stream

        private:
            /* INDEPENDENT THREAD FUNCTIONS AND VARIABLES */

            // each thread executes in their specific static function
            // each static function continually calls the instance functions until told to die
                                                  ///< the analyze data thread
            pthread_t    analyzeDataThread          ;
                                                  ///< indicated whether the analyze data loop is running
            bool         m_analyzeDataLoopAliveFlag ;
                                                  ///< indicate that this thread should die
            bool         m_analyzeDataLoopDieFlag   ;
                                                  ///< static wrapper
            static void *runAnalyzeDataLoop         ( void* );
                                                  ///< one quanta of running the analyze date loop
            void         analyzeDataLoop            ( void );

                                                  ///< the collect data thread
            pthread_t    collectDataThread          ;
                                                  ///< indicated whether the collect data loop is running
            bool         m_collectDataLoopAliveFlag ;
                                                  ///< indicate that this thread should die
            bool         m_collectDataLoopDieFlag   ;
                                                  ///< static wrapper
            static void *runCollectDataLoop         ( void* );
                                                  ///< one quanta of running the collect data loop
            void         collectDataLoop            ( void );

                                                  ///< the command loop flag
            pthread_t    commandThread              ;
                                                  ///< indicated whether the command loop is running
            bool         m_commandLoopAliveFlag     ;
                                                  ///< indicated whether the command loop should die
            bool         m_commandLoopDieFlag       ;
                                                  ///< static wrapper
            static void *runCommandLoop             ( void* );
                                                  ///< one quanta of running the command loop
            void         commandLoop                ( void );

            /* ANALYZE DATA FUNCTIONS */
                                                  ///< take data from pipe and put it into the streamchannel
            bool  fillStreams                      ( void );
                                                  ///< makes sure all streams have some data
            void  checkChannelInit                 ( void );
                                                  ///< tells each channel to scan for triggers
            void  scanStreamForTriggers            ( void );
                                                  ///< sends scanned triggers from sources to receivers
            void  distributeTriggers               ( void );
                                                  ///< use the pending triggers to generate triggered pulse records
            void  generateTriggerRecords           ( void );
                                                  ///< sends any triggered records that may have been generated to other classes (i.e. fft, file writers)
            void  sendTriggerRecords               ( void );
                                                  ///< discards old data from the stream channels
            void  trimStreams                      ( void );

            /* OPTIMIZE MIX FUNCTIONS */
                                                  ///< grabs that data used to analyze the mix
            void  optimalMix_collectData           ( void );
                                                  ///< adjusts the mix so that the error signal is minimized
            void  optimalMix_analyzeData           ( void );
                                                  ///< utility function that returns the best mixing parameter based on two pulse records
            float optimalMix_calculateMedianMix    ( std::deque<XPulseRec *>, std::deque<XPulseRec *> );
                                                  ///< a method for determining optimal mix by finding the lowest standard deviation
            float optimalMix_lowestSTDMix          ( XPulseRec *even, XPulseRec *odd );
                                                  ///< a method for determining optimal mix by scaling  the RMS
            float optimalMix_RMSRatioMix           ( XPulseRec *even, XPulseRec *odd );

            /// a random math function
            float median                           ( float *x, int n);

            /* VARIABLES */
                                                  ///< string that holds the version tag
            char                       m_versionString[128];
            FILE                      *g_configFD;///< the config file descriptor

            // get these from the server once you make a connection
                                                  ///< this the the type of server.  you get this by asking the server what it is
            ServerType                 m_serverType;
                                                  ///< the type of server you are getting the data from
            DataSource                 m_dataSource;
                                                  ///< the number of channels
            unsigned int               m_nDataStreams;
                                                  ///< number of mux pixels (there are 2 data streams (channels) for every pixel!!!)
            unsigned int               m_nMuxPixels;
            unsigned int               m_nMuxCols;///< number of mux rows
            unsigned int               m_nMuxRows;///< number of mux colums

            // progress of the connect to server
                                                  ///< the numerator in the connect progress
            int                        m_connectProgressN;
                                                  ///< the denominator in the connect progress
            int                        m_connectProgressD;

            // status flags (set internally)
                                                  ///< whether or not you are actually connected to the server
            bool                       m_connected;
                                                  ///< whether or not you are actually streaming data from the server
            bool                       m_streaming;
                                                  ///< whether or not you have data from all channels
            bool                       m_channelInit;
            bool                       m_tiltFlag;///< indicates that the display should be updated

            // data flow structure
                                                  ///< stats on how many bytes got transfered and at what time
            struct                    dataRateStruct
            {
                unsigned long long int t;         ///< the time this was taken
                unsigned int bytes;               ///< the number of bytes recorded
            };

            // data flow statistics
                                                  ///< how full the output buffer is (actually 1-fill_level)
            float                       m_bufferLevel;
                                                  ///< bytes per seconds (m_totalBytes/deltat)
            float                       m_dataRate;
                                                  ///< total bytes transfered over n ticks
            unsigned int                m_totalBytes;
                                                  ///< queue that holds the bytes and time stamps
            std::deque<dataRateStruct>  m_dataRateDeque;

            // variables (set by user through encapsulated functions)
                                                  ///< flags whether a trigger on the signal channel should also trigger the corresponding error channel
            bool                        m_coupleOddToEvenFlag;
                                                  ///< flags whether a trigger on the signal channel should also trigger the corresponding error channel
            bool                        m_coupleEvenToOddFlag;
                                                  ///< whether to couple trigger from f/b -> error or f/b -> error
            bool                        m_fbCoupleTrigger;
                                                  ///< whether or not this channel's data should be streamed to the client
            bool                       *m_streamDataFlag;
                                                  ///< whether or not this channel's triggered pulses should be anaylzed
            bool                       *m_performAnalysisFlag;
                                                  ///< whether or not this channel's triggered pulses should be sent to the file writer
            bool                       *m_writePulsesToFileFlag;
                                                  ///< write auto triggers to file
            bool                        m_writeAutoToFileFlag;
                                                  ///< write level triggers to file
            bool                        m_writeLevelToFileFlag;
                                                  ///< write edge triggers to file
            bool                        m_writeEdgeToFileFlag;
                                                  ///< write noise triggers to file
            bool                        m_writeNoiseToFileFlag;

            // client level versions of the mix and decimation status.  this set represents what the USER sees on the GUI
            // not necessarily what the status of the stream channel actually is.  variables with the same name are in the
            // each streamData object and represent what the data headers are actually indicating
                                                  ///< the mixing level for the sig and error
            float                      *m_mixLevel;
            bool                       *m_mixFlag;///< mix enabled at server
                                                  ///< inversion enabled at server
            bool                       *m_mixInversionFlag;
                                                  ///< decimation level at server
            unsigned short int         *m_decimateLevel;
                                                  ///< decimation enabled at server
            bool                       *m_decimateFlag;
                                                  ///< average decimation enabled at server (other wise it truncates)
            bool                       *m_decimateAvgFlag;

            // optimize mix stuff
                                                  ///< whether or not the run thread should run the autotune function
            bool                        m_optimizeMixCalculateMedianMixFlag;
                                                  ///< whether corresponding pairs of feedback and error PR should have the same trigger point
            bool                        m_optimizeMixCouplePairsFlag;
                                                  ///< method for getting the optimal ratio given two PRs
            int                         m_optimizeMixRatioMethod;
                                                  ///< the number of pulses to consider when auto tuning
            unsigned int                m_optimizeMixMaxRecords;
                                                  ///< an array of PR deques.  each channel get's its own vector of PR's
            std::deque<XPulseRec *>    *m_optimizeMixPRArray;
                                                  ///< just like above except each feedback PR has a coupled PR in the error channe. ie if the third pulse in channel 1's deque triggered at time 1234, then the third pulse in channel 0's deque also triggered at that time
            std::deque<XPulseRec *>    *m_optimizeMixCoupledPRArray;
                                                  ///< array of flags as to whether the channel should be optimized
            bool                       *m_optimizeMixChannelFlag;
                                                  ///< store the old value of the auto trigger flag
            bool                       *m_oldAutoFlag;
                                                  ///< store the old value of the auto trigger threshold
            double                     *m_oldAutoValue;
                                                  ///< store the old value of the level trigger flag
            bool                       *m_oldLevelFlag;
                                                  ///< store the old value of the level trigger threshold
            bool                       *m_oldEdgeFlag;
                                                  ///< store the old value of the auto trigger flag
            bool                       *m_oldNoiseFlag;
                                                  ///< store the old value of the even to odd flag
            bool                        m_oldCoupleEvenToOddFlag;
                                                  ///< store the old value of the odd to even flag
            bool                        m_oldCoupleOddToEvenFlag;
                                                  ///< indicates when the server is done collecting data
            bool                        m_doneCollectingOptimalMixData;
                                                  ///< numerator of the progress
            unsigned int                m_optimizeMixProgressN;
                                                  ///< denominator of the progress
            unsigned int                m_optimizeMixProgressD;

            /// a deque of raw data packets (received from the data pipe) and made available to the analyze data thread
            std::deque<unsigned char *> m_rawDataPacketDeque;

            /// lock on the raw data packet deque
            XCALDAQLock                 m_rawDataPacketDequeLock;

            /// a vector of triggered pulse records
            std::vector<XPulseRec *>    m_trigPR;

            /// a vector of noise pulse records
            std::vector<XPulseRec *>    m_noisePR;

            /// output file writer class
            xdaq::XOUT                 *m_XOUTWriter;

            /// what kind of writer the file writer is (PLS, LJH, FFT, etc.)
                                                  // 0 = PLS, 1 = LJH
            XOUTType                    m_XOUTType;
    };

}                                                 // end of namespace xdaq


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
