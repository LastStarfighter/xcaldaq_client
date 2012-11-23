////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef xdaq_fftchannel_h
#define xdaq_fftchannel_h

////////////////////////////////////////////////////////////////////////////////
// system includes
#include <iostream>                               // standard c++ in put and output stream functions
#include <fftw3.h>
#include <valarray>
#include <deque>

// local includes
#include "XCALDAQLock.h"

// name space
namespace xdaq
{

    // class definition
    class FFTChannel
    {
        public:

            /// constructor with channel information (could get this info out of the pulse i guess)
            FFTChannel(int chanNum);

            /// take and analyze some data
            void ingestData (
                double *data,                     ///< array of doubles
                unsigned int N,                   ///< the number in the array
                double sampleRate                 ///< the sample rate
                );

            /// initialize the FFT plan
            void replanFFT (
                unsigned int N,                   ///< the number in the array
                double sampleRate                 ///< the sample rate
                );

            // query only
            double* plotX ( void );               ///< return an array that has frequency data for plotting
            double* plotY ( void );               ///< return an array that has power density data for plotting
            int     plotN ( void );               ///< return the number of points in the plot for plotting
            double* dataX ( void );               ///< return an array that has frequency data
            double* dataY ( void );               ///< return an array that has power density data
            int     dataN ( void );               ///< return the number of points in the plot

            // encapsulation
            bool   fftFlag       ( void );
            void   fftFlag       ( bool );
            bool   windowingFlag ( void );
            void   windowingFlag ( bool );
            int    numRecords    ( void );
            void   numRecords    ( int );
            double threshold     ( void );
            void   threshold     ( double );
            void   discriminator ( int );
            int    discriminator ( void );

            // public available globals
            XCALDAQLock   fftLock;                // thread locking mechanisim

        private:
            bool          m_fftFlag;              ///< flag for whether to compute fft
            bool          m_windowingFlag;        ///< whether to use windowing or not

            unsigned int  m_channel;              ///< the channel of the pulse
            unsigned int  m_pulseLength;          // length of the data pulse array
            unsigned int  m_discriminatorCounter; // record count
            unsigned int  m_discriminator;        // records discriminator
            double        m_sampleRate;           // sample rate of the data

            double       *m_fftIn;                // pointer to the input buffer that you want "transformed".
            double       *m_fftOut;               // pointer to the output buffer that gets the transform of in.
            double       *m_fftDataY;             // the y axis display buffer
            double       *m_fftDataX;             // the y axis display buffer

            int           m_numRecsInFinalFFT;    // number of "outs" that are summed together to produce the final fft (held in the fft_queue)
            unsigned int  m_fftLength;            // length of the fft
            fftw_plan     m_fftPlan;              // the "plan" on how to calculate the fft.  read the fftw manual

                                                  // a deque of "outs"
            std::deque<std::valarray<double> > m_fftOutMagnitudeDeque;

    };

}


////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
