////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_pulserec_h
#define xdaq_pulserec_h

////////////////////////////////////////////////////////////////////////////////

// system includes
#include <iostream>
#include <deque>
#include <stdexcept>
#include <math.h>

// local includes
#include "XCALDAQLock.h"

namespace xdaq                                    // start name space
{

    /// trigger enumeration - the different kinds of trigger
    enum TriggerType
    {
        AUTO,                                     ///< auto trigger 1 pulse every n mS
        LEVEL,                                    ///< level trigger
        EDGE,                                     ///< edge trigger
        NOISE                                     ///< noise trigger
    };

    /**
     XPulse Record is a generic class that represents a single triggered pulse
     it contains encapsulated funtions that allow you to set or query information
     about that pulse such as how it was triggered, when it was triggered, stream channel
     number, etc.  there are also sets of functions that allow you to add and remove samples
     from the class's internal data structure.
     */
    class XPulseRec
    {
        public:
            XPulseRec                 ( void );   ///< constructor
                                                  ///< update the internal physical samples vector to reflect the internal raw samples vector
            void           calculatePhysSamples      ( void );
                                                  ///< add a raw sample to the internal raw sample deque
            void           appendRawShortSamples     ( unsigned short int val );
                                                  ///< erase and reset the internal raw short sample deque
            void           eraseRawShortSamples      ( void );
                                                  ///< erase and reset the internal physical sample deque
            void           erasePhysSamples          ( void );
                                                  ///< convert raw unit to physical (volts)
            double         rawToPhysical             ( unsigned short );
                                                  ///< convert physical units to raw units
            unsigned short physicalToRaw             ( double );
                                                  ///< discard samples in the internal raw sample buffer up to count == startCount
            int            eraseTo                   ( unsigned long long startCount );
                                                  ///< do a quick analysis on the pulse (gets pulseHeight and baseline)
            void           analyzePulse              ( void );
                                                  ///< computes the mean of the record
            double         pulseAverage              ( void );
                                                  ///< rms of the signal - average
            double         rmsAC                     ( void );
                                                  ///< straight rms
            double         rmsDC                     ( void );
                                                  ///< request a copy of the internal PHYSICAL data
            void           requestDataCopy           ( double **, double **, unsigned int * );
                                                  ///< request a copy of the derivitive of the internal PHYSICAL data
            void           requestDataDerivitiveCopy ( double **, double **, unsigned int * );

            // static functions that are available to everyone
                                                  ///< computes the average of an array
            static double average                    ( double* array, int n );
                                                  ///< computes the standard deviation of the array
            static double standardDeviation          ( double *array, int n );
                                                  ///< removes points from an array that lay outside deviations deviations
            static int    strip                      ( double *array, int n, int deviations );

            // functions that query internal values only
                                                  ///< returns the pulse height
            double                                     pulseHeight    ( void );
                                                  ///< returns the baseline
            double                                     baseline       ( void );
                                                  ///< returns an iterator that starts at the beginning of the physical sample deque
            std::deque<double>::const_iterator         physicalBegin  ( void ) const;
                                                  ///< returns an iterator that starts at the beginning of the raw sample deque
            std::deque<unsigned short>::const_iterator rawSampleBegin ( void ) const;
                                                  ///< the maximum physical value this pulse could have
            double                                     physMax        ( void );
                                                  ///< the minimum physical value this pulse could have
            double                                     physMin        ( void );

            // full encapsulation
            int                    rawMax            ( void );
            void                   rawMax            ( int );
            int                    rawMin            ( void );
            void                   rawMin            ( int );
            void                   signedSamples     ( bool val );
            bool                   signedSamples     ( void ) const;
            void                   timecode          ( unsigned long long int timecode );
            unsigned long long int timecode          ( void ) const;
            void                   timecodeBase      ( unsigned long long int timecodeBase );
            unsigned long long int timecodeBase      ( void ) const;
            void                   length            ( unsigned int length );
            unsigned int           length            ( void ) const;
            void                   channel           ( int channel );
            int                    channel           ( void ) const;
            void                   pretrigPnts       ( unsigned int pretrig );
            unsigned int           pretrigPnts       ( void ) const;
            void                   deltat            ( double deltat );
            double                 deltat            ( void ) const;
            void                   yscale            ( double yscale );
            double                 yscale            ( void ) const;
            void                   yoffset           ( double yoffset);
            double                 yoffset           ( void ) const;
            void                   physPulseHeight   ( double ph );
            double                 physPulseHeight   ( void ) const;
                                                  // how the pulse was triggered
            void                   triggerType       ( TriggerType );
            TriggerType            triggerType       ( void ) const;
                                                  // what the trigger threshold was
            void                   threshold         ( double );
            double                 threshold         ( void ) const;
            void                   clipped           ( bool );
            bool                   clipped           ( void ) const;
            void                   sampleCount       ( unsigned long long int );
            unsigned long long int sampleCount       ( void ) const;
            void                   timeCount         ( unsigned long long int );
            unsigned long long int timeCount         ( void ) const;
            void                   calculatePhysFlag ( bool );
            bool                   calculatePhysFlag ( void ) const;

            /// the OS independent thread lock class
            XCALDAQLock lock;

        private:
            // the internal data vectors
                                                  ///< the raw data storage
            std::deque<unsigned short> m_rawShortSamplesVector;
                                                  ///< the real data storage
            std::deque<double>         m_physSamplesVector;

            // variables
            int                        m_channel; ///< channel number (stream number would be a better identifier)

            //convert raw samples to volts
            // note about offset and scale:  remember physical = raw * m_yscale + m_yoffset;
            double                     m_yscale;  ///< is given as volts per tick
            double                     m_yoffset; ///< offset is in VOLTS.  the voltage that should be produced at raw = 0.  usually this is zero.

            // user enters these values
            int                        m_rawMin;  ///< the largest raw number this pulse can have
            int                        m_rawMax;  ///< the smallest raw number that this pulse can have

            // threshold stuff and trigger point
                                                  ///< how many pretrig points there are
            unsigned int               m_pretrigPnts;
                                                  ///< count value of the trigger point
            unsigned long long int     m_trigPoint;
                                                  ///< what kind of trigger produced this pulse
            TriggerType                m_triggerType;
                                                  ///< the threshold that this trigger was set to
            double                     m_threshold;
                                                  ///< whether the inverse flag on the trigger was set
            bool                       m_inverseFlag;

            // keep track of time
                                                  ///< milliseconds from the epoch to when the server started
            unsigned long long int     m_timecodeBase;
            unsigned long long int     m_timecode;///< milliseconds from the timecode_base to the trigger point
            double                     m_deltat;  ///< the time b/w two samples,  the inverse of the sampling freq
            double                     m_sampRate;///< the sample rate (1/deltaT)
                                                  ///< the sample rate multiplied by the decimation if it is on
            unsigned int               m_effectiveSampRate;

            /// the sample count of the last sample
            /**
             there are two counts that the server keeps track of and reports with each packet.
             one is the sample count of the last point and one is the time count of the last point.
             the time count is how many ticks of the clock have happened since the first sample.
             the time count is how many samples have been recorded since the first sample.
             so, if there is no decimation, the time count = the sample count for all points.
             since the time of the first sample is recorded in m_timecodeBase
             the time of any point can found using: time = timebase + (timeCount) * deltat
             */
            unsigned long long int     m_sampleCount;
                                                  ///< the time count of the last sample
            unsigned long long int     m_timeCount;

            // quick analysis
            double                     m_baseline;///< calculated baseline of the record
                                                  ///< calculated pulse height of the record
            double                     m_pulseHeight;

            // flags
            bool                       m_clipped; ///< a clipped flag (not used currently)
                                                  ///< whether or not the raw samples are signed or unsigned
            bool                       m_signedSamples;
                                                  ///< whether or not the record should automatically calculate the physical units
            bool                       m_calculatePhysFlag;
    };

}
#endif
