////////////////////////////////////////////////////////////////////////////////
/*
    This object ingests the data stream from a single channel of data and
    implements triggering to form "records" thus it can "plug-in" to such
    continuous input DAQ objects as the SCDP, the MUX, etc. It's does the sort
    of thing the Qboard software does.
 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_streamchan_h
#define xdaq_streamchan_h

////////////////////////////////////////////////////////////////////////////////

// system includes
#include <deque>
#include <vector>
#include <sys/time.h>                             // gettimeofday function
#include <arpa/inet.h>                            // net to host and host to net functions

// local includes
#include "XCALDAQRecordConstants.h"
#include "byteorder64.h"                          //ntohll
#include "XPulseRec.h"

#include "string_utils.h"

// macro definitions

// The short answer is to use ULLONG_MAX, which is covered by the standard.
// ULONG_LONG_MAX is nonstandard and will not appear in <limits.h> if GCC is
// invoked in compliant mode.

#include <limits.h>
#ifndef ULLONG_MAX
#define ULLONG_MAX ULONG_LONG_MAX
#endif

#define MAX_PULSE_ANALYSIS_POINTS 10000

// start namespace
namespace xdaq
{

    ////////////////////////////////////////////////////////////////////////////////
    /**
     Trigger structure gives some general information about what the conditions were
     that generated the trigger.  this allows us to ask how and from where the pulse
     was generated
    */
    struct Trigger
    {

        unsigned long long int sampleCount;       // where
        unsigned long long int timeCount;
        TriggerType            triggerType;       // what kind of trigger (enumerated in XPulseRec.h)
        double                 threshold;
        bool                   inverseFlag;

    };                                            // end of Trigger structure

    ////////////////////////////////////////////////////////////////////////////////
    /**
     StreamChannel is a general class that represents a non-trigger stream of data
     from the server.  there is one stream channel instance for every data stream.
     it should probably be called dataStream.cpp  i might change this to avoid confusion.
     basically, it includes functions for controlling how the data passes through the system.
     the main functions are  ingesting data, looking for triggers, and generating the pulse
     records from those triggers.  there are also a bunch of functions for setting
     the thresholds for the triggers and converting the various ticks and times back and forth.

     each stream channel contains 1 xpulse record where it stores the raw data stream.
     when you request a trigger, it returns a pulse record that contains 1 pulse.  the pulse
    record used for storing the raw data is not triggered, but just happened to be an
    easy way to handle the large amounts of data instead of replicating all of the
    functionality that is already in xpulserec.cpp

    */
    class StreamChannel
    {
        public:
            /// constructor
            StreamChannel (int chanNum,int pretrig,int recLen);

            // stream channel commands
                                                  ///< take the streamchannel down nicely... doesn't do anything right now
            void                   stop                        ( void );
                                                  ///< this takes a xcaldaqRecord packet, parses it for errors and ingests it into the data stream xPulseRecord
            bool                   ingestDataPacket            ( const unsigned char* );
                                                  ///< scans for triggers and stores their count number in the scanned trigger deque
            void                   scanStreamForTriggers       ( void );
                                                  ///< returns the first trigger in the scanned trigger deque, or zero if none are left
            Trigger                getNextTrig                 ( void );
                                                  ///< puts a trigger point (i.e. a sample count number) in the pending trigger deque
            void                   addPendingTrig              ( Trigger );
                                                  ///< clears the pending trig deque
            void                   eraseAllPendingTrigs        ( void );
                                                  ///< use the pending trigger deque to generate a PR and return it to the caller
            XPulseRec*             getNextTriggeredPulseRecord ( void );
                                                  ///< use the noise trigger deque to generate a PR and return it to the caller
            XPulseRec*             getNextNoisePulseRecord     ( void );
                                                  ///< given a time count, this function returns a complete pulse record
            XPulseRec*             createTriggeredPulseRecord  ( Trigger );
                                                  ///< trim stream to the default length
            void                   trimStream                  ( void );
                                                  ///< trim to a specific point
            void                   trimStream                  ( unsigned long long int);
                                                  ///< return the oldest point that you would want to trim to
            unsigned long long int oldestTrimPoint             ( void );
                                                  ///< store a pulse height and trigger time into the PH vs Time vectors
            void                   addPulseData                ( double height, double baseline, double time);

            // query only (i.e. the owner can see them but not set them)
                                                  ///< request that the stream data instance give you what information it has about pulse height from pulses it has generated
            void                   requestPulseHeightCopy      ( double **x_pointer, double **y_pointer, unsigned int *n_pointer);
                                                  ///< request that the stream data instance give you what information it has about the baseline from pulses it has generated
            void                   requestBaselineCopy         ( double **x_pointer, double **y_pointer, unsigned int *n_pointer);
                                                  ///< returns a pointer to the data stream PR
            XPulseRec             *pulseRec                    ( void );
                                                  ///< the max level threshold that makes sense for this stream data instance
            double                 maxLevelThreshold           ( void );
                                                  ///< the min level threshold that makes sense for this stream data instance
            double                 minLevelThreshold           ( void );
                                                  ///< the max edge threshold that makes sense for this stream data instance
            double                 maxEdgeThreshold            ( void );
                                                  ///< the min edge threshold that makes sense for this stream data instance
            double                 minEdgeThreshold            ( void );
                                                  ///< check to see if this channel has received any data from the stream
            bool                   init                        ( void );
                                                  ///< convert a sample to epoch time
            unsigned long long int sampleCountToEpochTime      ( unsigned long long int sampleCount );
                                                  ///< how many triggers per second this data stream is producing
            double                 triggerRate                 ( void );

            // complete encapsulation
            bool               signedRawSamples           ( void );
            void               signedRawSamples           ( bool );
            int                recLen                     ( void );
            void               recLen                     ( int );
            int                pretrig                    ( void );
            void               pretrig                    ( int );
            int                chanNum                    ( void );
            void               chanNum                    ( int );
            void               triggerRateIntegrationTime ( double );
            double             triggerRateIntegrationTime ( void );
            void               autoThreshold              ( double );
            double             autoThreshold              ( void );
            void               levelThreshold             ( double );
            double             levelThreshold             ( void );
            void               edgeThreshold              ( double );
            double             edgeThreshold              ( void );
            void               noiseThreshold             ( double );
            double             noiseThreshold             ( void );
            void               autoTriggerFlag            ( bool );
            bool               autoTriggerFlag            ( void );
            void               levelTriggerFlag           ( bool );
            bool               levelTriggerFlag           ( void );
            void               edgeTriggerFlag            ( bool );
            bool               edgeTriggerFlag            ( void );
            void               noiseTriggerFlag           ( bool );
            bool               noiseTriggerFlag           ( void );
            void               levelInverseFlag           ( bool );
            bool               levelInverseFlag           ( void );
            void               edgeInverseFlag            ( bool );
            bool               edgeInverseFlag            ( void );
            void               noiseInverseFlag           ( bool );
            bool               noiseInverseFlag           ( void );
            bool               mixFlag                    ( void );
            void               mixFlag                    ( bool );
            float              mixLevel                   ( void );
            void               mixLevel                   ( float );
            bool               mixInversionFlag           ( void );
            void               mixInversionFlag           ( bool );
            void               decimateLevel              ( unsigned int d );
            unsigned int       decimateLevel              ( void ) const;
            bool               decimateFlag               ( void );
            void               decimateFlag               ( bool );
            bool               decimateAvgFlag            ( void);
            void               decimateAvgFlag            ( bool );
            void               decimateLevel              ( unsigned short int );
            unsigned short int decimateLevel              ( void);
            double             sampRate                   ( void );
            double             effectiveSampRate          ( void );

            // i don't see why the stream channel needs to know this.  perhaps this should be something maintained in client object
            bool               groupTrigReceiver          ( void );
            void               groupTrigReceiver          ( bool );
            bool               groupTrigSource            ( void );
            void               groupTrigSource            ( bool );

        private:
            /* private utility functions */
                                                  ///< convert a sample tick count to a time tick count
            unsigned long long int sampleCountToTimeCount ( unsigned long long int );
                                                  ///< convert an absolute sample tick count to its place in the stream buffer
            unsigned long long int indexToCount           ( int );
                                                  ///< convert a position in the stream buffer to its absolute sample tick count
            int                    countToIndex           ( unsigned long long int );
                                                  ///< use delta t to convert seconds to ticks
            int                    mSecondToTicks         ( double );

            /* variables */
            /// flag if the data is initialized (set internally)
            bool                                m_init;

            /// group trigger receiver flags (set externally by the user)
            /**
             NOTE: does the stream channel really need to know it is a group trigger source /  receiver? isn't this more
             of a top level thing that belongs in client?
             */
                                                  ///< whether this is a group trig receiver
            bool                                m_groupTrigReceiver;

            /**
             NOTE: does the stream channel really need to know it is a group trigger source /  receiver? isn't this more
             of a top level thing that belongs in client?
             */
                                                  ///< whether this is a group trig source
            bool                                m_groupTrigSource;

            // the variables that control when and how the stream generates a trigger (set externally)
                                                  ///< auto trigger
            bool                                m_autoTriggerFlag;
                                                  ///< level trigger
            bool                                m_levelTriggerFlag;
                                                  ///< level trigger inverse flag
            bool                                m_levelInverseFlag;
                                                  ///< edge trigger
            bool                                m_edgeTriggerFlag;
                                                  ///< edge trigger inverse flag
            bool                                m_edgeInverseFlag;
                                                  ///< noise (level) trigger
            bool                                m_noiseTriggerFlag;
                                                  ///< noise trigger inverse flag
            bool                                m_noiseInverseFlag;

            // the threshold values for the various triggers
                                                  ///< mSeconds b/w triggers
            double                              m_autoThreshold;
                                                  ///< time of the last auto trigger
            unsigned long long int              m_nextAutoTrigger;
                                                  ///< threshold for the level trigger
            double                              m_levelThreshold;
                                                  ///< raw threshold for the level trigger
            int                                 m_levelThresholdRaw;
                                                  ///< treshold for edge trigger (actually the stored value is the trigger * 2 * deltat to make trigger sensing easier)
            double                              m_edgeThreshold;
                                                  ///< raw treshold for edge trigger (actually the stored value is the trigger * 2 * deltat to make trigger sensing easier)
            int                                 m_edgeThresholdRaw;
                                                  ///< threshold for the noise level (used to scan for noise)
            double                              m_noiseThreshold;
                                                  ///< raw threshold for the noise level (used to scan for noise)
            int                                 m_noiseThresholdRaw;
                                                  ///< the number of points below the noise threshold in a row
            unsigned int                        m_continuousNoisePoints;

            // where the trigger scanner should start scanning (set internally)
                                                  ///< where you started to scan for triggers in count space
            unsigned long long                  m_startScanCount;
            ///<int                                 m_maxSamples; ///< the maximum number allowable in the rec length
                                                  ///< the number of triggers produced by this channel
            int                                 m_numTriggers;

            // how big the trigger record should be (set externally)
                                                  ///< length of the triggered pulse record
            unsigned int                        m_recLen;
                                                  ///< pretrigger level
            int                                 m_pretrig;

            // mixing and decimation variables
            // these values are essentially local copies of variables on the server
            // we keep them local so we don't have to ask the server for them everytime we
            // want to put them on screen
            float              m_mixLevel;        ///< the mixing level for the sig and error
            bool               m_mixFlag;         ///< mixing flag
            bool               m_mixInversionFlag;///< mix inversion flag
            unsigned short int m_decimateLevel;   ///< decimation level
            bool               m_decimateFlag;    ///< decimation flag
            bool               m_decimateAvgFlag; ///< decimation mode (true = avg, false = truncate)

            /// contains auto, level, and edge trigs
            /**
             the deques that contain the various trigger points in count space
             they are generated _internally_ in the scanStreamForTriggers function
             */
            std::deque <Trigger>               m_scannedTriggers;

            /// contains noise trigs (used for FFT)
            /**
             the deques that contain the various trigger points in count space
             they are generated _internally_ in the scanStreamForTriggers function
             */
            std::deque <Trigger>               m_noiseTriggers;

            /**
             a deque of trigger points in count space that are waiting to be used to
             generate triggered pulse records.  triggers are placed here _externally_
             so as to allow the use of group trigger.  it is up to the user to move
             scanned triggers out of the scanned trigger queue and into this one
             note: should use a priority queue for pending with greater than to
             order them.  this would guarantee that the oldest trigger would be done first.
             probably not important, but it is the right way to do it
             */
                                                  ///< a deque of trigger points in count space
            std::deque <Trigger>                m_pendingTriggers;
                                                  ///< the trigger id number
            std::deque <int>                    m_pendingTriggersNumber;

            // these variables are used to keep track of the pulse rate
                                                  ///< holds the time tick of a pulse
            std::deque <unsigned long long int> m_triggerRateDeque;
                                                  ///< how much time you want to integrate
            int                                 m_triggerRateIntegrationTime;
                                                  ///< lock on the deque
            XCALDAQLock                         m_triggerRateLock;

            // these varibales are used to keep track of pulse height and trigger time from triggered pulses
                                                  ///< pulse time
            std::deque <double>                 m_pulseTimeFloat;
                                                  ///< pulse height
            std::deque <double>                 m_pulseHeight;
                                                  ///< base line
            std::deque <double>                 m_baseline;
                                                  ///< analysis lock
            XCALDAQLock                         m_pulseAnalysisLock;

            /**
             a local pulse record constructed from data packets
             this is where the raw stream data goes
             */
            XPulseRec                           m_pulseRec;

        public:
            //            std::string pretty_print_trigger_state(void) const;

            std::string pretty_print_trigger_state(const std::string& seperator="\n\t", const std::string& prefix="", const std::string& postfix="");

            std::string pretty_print_mixing_state(const std::string& seperator="\n\t", const std::string& prefix="", const std::string& postfix="");

            int set_parameter(const std::string& parameter_name, const std::string& value);

            // this nastiness is to convert the given string into the
            // correct data type for the method pointer
            template <class T>
                void set_parameter_via_function_pointer(const std::string& value,
                void (StreamChannel::*f_ptr)(T))

            {
                T x;
                gsd::str::string_to_val(value,x);

                (*this.*f_ptr)(x);

            }

    };                                            // end of class definition

}                                                 // end of name space


////////////////////////////////////////////////////////////////////////////////
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
