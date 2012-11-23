////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "FFTChannel.h"
#include <numeric>                                // for accumulate algorithm

////////////////////////////////////////////////////////////////////////////////
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    FFTChannel::FFTChannel (int chanNum)
    {
        m_channel    = chanNum;                   // so the fft knows it's channel number

        // some default values
        fftLock              = XCALDAQLock();     // the local lock
        m_fftIn                = NULL;            // the "in" array pointer
        m_fftOut               = NULL;            // the "out" array pointer
        m_fftPlan              = NULL;            // will be initialized when the first data set comes in
        m_fftDataX             = NULL;            // the plotting x
        m_fftDataY             = NULL;            // the plotting y
        m_numRecsInFinalFFT    = 20;              // recs in each fft calculation
        m_fftLength            = 0;               // the plan length
        m_pulseLength          = 0;               // the length of the input array
        m_sampleRate           = 0.;              // default sample rate
        m_discriminatorCounter = -1;              // how many data records have been sent (set to -1 so the first record is the 0th)
        m_discriminator        = 10;              // how many data records to throw away for every 1 that is calculated
        m_fftFlag              = false;           // FFT calculation is OFF by default
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::ingestData ( double *data, unsigned int pulseLength, double sampleRate )
    {

        if (!m_fftFlag) return;

        // disregard every record except every n_th one.
        // we get lots of noise.  discriminator is an attempt to only use so much noise data
        // if discriminator is set to 10, 1 noise sample is calculated for every 10 noise records received.
        m_discriminatorCounter++;
        if (m_discriminatorCounter % m_discriminator != 0)
        {
            return;
        }

        // make sure 1) that the length of the incoming record hasn't changed
        //           2) that the sampleRate of the incoming record hasn't changed since
        if ((m_pulseLength != pulseLength) || (m_sampleRate != sampleRate))
        {
            replanFFT(pulseLength, sampleRate);
        }

        // get the lock for this instance
        fftLock.getLock();

        // reset the average
        double avgSamp = 0;

        // copy the data from the pulse to the input array
        for (unsigned int i=0; i<m_pulseLength; i++)
        {
            // dump that value into the fft array
            m_fftIn[i] = data[i];

            // add this value to the average
            avgSamp += m_fftIn[i];
        }

        // remember, that m_fftLength >= m_pulseLength, so pad out the rest of the array
        // if m_pulseLength = m_fftLength, this loop will not even run
        // from numerical recipes p505:  in fact, we categorically recommend that you _only_ use
        // FFTs with N a power of two.  If the length of your data set is not a power of two
        // pad it with zeros up to the next power of two.
        for (unsigned int i=m_pulseLength; i<m_fftLength; i++)
        {
            m_fftIn[i] = 0;                       // pad out the rest of the input array with 0's
            printf ("you shouldn't see this at all!! no padding! randy wanted this removed!!\n");
        }

        // get the average level
        avgSamp /=  m_pulseLength;

        /*
         note: because we are calculating the power spectral density, we should do
         some windowing to prevent excessive leakage.
         basically, multiply each number by some scaling factor that is ~0 at
         the edges and ~unity in the middle.  we do this b/c we see a snapshot
         of the entire data stream and there is considerable spectral leakage from
         one bin to the next.  because at the left and right, we have only the tailing
         or leading bins to look at, we do not get a complete picture of where they
         came from.  so, instead, we hardly look at those points, where the information
         is incomplete and look more at the middle points where we know both trailing
         and leading edges.  windowing methods accomplish this.  see numerical recipes p553
        */
        double windowingNormalizingFactor = 1.;
        if (m_windowingFlag)
        {
            // Do "SCDP-style" Hanning window on the data
            double scalingFactor = 2. * M_PI / m_pulseLength;
            for (unsigned int i=0; i<m_pulseLength; i++)
            {
                // calculate the apodize value
                double apodizeVal = (1 - cos( scalingFactor * i ))/2;

                // subtract the average from each value and multiply by the apodize value
                m_fftIn[i] = (m_fftIn[i] - avgSamp) * apodizeVal;
            }

            /*
             later note: got this from the wave metrics page
             When using a window function, the amount of power in the signal is reduced.
             A compensating multiplier of 1/average(window[i]^2) should be applied to
             the result to compensate for this. For a Hanning window this value is
             theoretically 0.375. Because the normalization factor is a denominator, you
             would divide N by 0.375 to compensate for the Hanning window:

             so, does this give us an extra factor of 1/.375?
             */

            windowingNormalizingFactor = 1 / .375;

            // hack attack
            // for no reason what-so-ever, i find that, using a particular test data
            // stream, the results i get with the
            // windowing vs no windowing are off by a factor of 1/root(2)
            // is this an artifact from the windowing scheme?  is this what hanning
            // is supposed to do?  should they be the same?  the data stream is mostly
            // noise, but how white is it?  who knows.  i am putting in the factor
            // of 1/root(2).  take it out if you want
            windowingNormalizingFactor /= sqrt(2.);

            /*
             //Do a split-cosine-bell tapering (alternate windowing alg)

             double cosProp = 0.10;
             int cosLength = (int) (m_fftLength * cosProp /2.0);

             for (int i=0;i<cosLength;i++) {

             m_fftIn[i] = m_fftIn[i] * (1.0 - cos((i-0.5)*M_PI/cosLength)) / 2.0
             + avgSamp * (1.0 + cos((i-0.5)*M_PI/cosLength)) / 2.0;
             }
            for (int i=pulseRec.length() - cosLength; i<pulseRec.length();i++) {
            m_fftIn[i] = m_fftIn[i] * (1.0 - cos((pulseRec.length() - i + 0.5)*M_PI/cosLength))/ 2.0
            + avgSamp * (1.0 + cos((pulseRec.length() - i + 0.5)*M_PI/cosLength))/ 2.0;
            }
            for (int i=pulseRec.length();i<m_fftLength;i++) {
            m_fftIn[i] = avgSamp;
            }

            */

        }

        /* note about normalization in fft
         found this in the fftw tutorial:
         Users should note that FFTW computes an unnormalized DFT.
         Thus, computing a forward followed by a backward transform
         (or vice versa) results in the original array scaled by n.
         it never says it, but i guess the normalizing factor is therefore 1/sqrt(n)?
         */
        double fftwNormalizingFactor = 1.0/sqrt(m_fftLength);

        /*
         note about the normalization for delta t
         fftw gives us volts per root sample.  we want volts per root hz, so we need
         to have a conversion factor that does this
         given randy's code, it seems this factor should be
         root (2*deltaT)
         */
        double deltaTNormalizingFactor = sqrt(2./m_sampleRate);

        // calculate the final normalizing factor
        double normalizingFactor = fftwNormalizingFactor * windowingNormalizingFactor * deltaTNormalizingFactor;
        // execute the plan (actually calculate the fft)
        fftw_execute(m_fftPlan);

        // now take the "out" array and use it to construct an magnitude array that is normalized
        // first construct a temperary value array
        std::valarray<double> magnitudeArray(m_fftLength/2);

        /*
         loop through the out buffer and generate the magnitude from the half
         complex (out) vector (see fftw's manual on half complex arrays)

         while we are at it, lets apply our normalization for both the window and the fftw

         note from randy:
         Be sure that you are averaging the SQUARED signals (V2/Hz) and then displaying
         the sqrt of that average.  If you, instead, just average the V/sqrt(Hz) signal,
         you will be off by an insidiously small gain factor, something like 8%,
         depending on the f binning!  This was a hard-won bug removal in some of my
        code ~3 years ago.

        result:
        take our normalized v/root(hz) function and square it (v^2/hz).  use the power spectrum to
        computer the average power. when it comes time to plot it (v/root(hz)) take the sqaure
        of the average.

        */
        for (unsigned int i=0; i<m_fftLength/2; i++)
        {
            // get the magnitude:  Mag^2 = R^2 + I^2
                                                  //<- original
            magnitudeArray[i]  = sqrt(m_fftOut[i]*m_fftOut[i] + m_fftOut[m_fftLength-i]*m_fftOut[m_fftLength-i]);

            // apply the fftw normalizing factor, windowing normalizing factor, and the
            // delta t normalizing factor
            magnitudeArray[i] *= normalizingFactor;

            // remember, this is VOLTS, but we want power (VOLTS^2) so we can average it
            magnitudeArray[i] = magnitudeArray[i]*magnitudeArray[i];
        }

        // drop the entire array you just calculated onto the back of the deque
        m_fftOutMagnitudeDeque.push_back(magnitudeArray);

        // drop items off the front of the queue until it is less than num_recs
        if (m_numRecsInFinalFFT > 0)
        {
            while ((int)m_fftOutMagnitudeDeque.size() > m_numRecsInFinalFFT)
            {
                m_fftOutMagnitudeDeque.pop_front();
            }
        }

        // now we need to take the n fft's in the deque and average them together
        std::valarray<double> tmp (0.0, m_fftLength/2);

        // add up all values at location i in each of the valarrays in the queue
        // this could be done more efficently by having a running total vector.
        // but, for the time being, this seems to be pretty fast, so i am going to keep
        tmp = accumulate(m_fftOutMagnitudeDeque.begin(), m_fftOutMagnitudeDeque.end(), tmp);

        // average every one of those values
        tmp = tmp / ((double)m_fftOutMagnitudeDeque.size());

        // assign that to the display buffer
        // remember, we computed the average of the power spectrum (V2/Hz)
        // but we want to plot in V/root(Hz)
        for (unsigned int i = 0; i<m_fftLength/2; i++)
        {
            //m_fftDataY[i] = tmp[i]; <- original
            m_fftDataY[i] = sqrt(tmp[i]);
        }

        // release the lock
        fftLock.releaseLock();
        return;
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::replanFFT(unsigned int pulseLength, double sampleRate)
    {
        m_pulseLength = pulseLength;              // reset and save the length
        m_sampleRate  = sampleRate;               // reset and save the sampleRate

        // first get the lock on this instance
        fftLock.getLock();

        // if there is a plan, remove it
        if (m_fftPlan)  fftw_destroy_plan(m_fftPlan);
        if (m_fftIn)    fftw_free(m_fftIn);
        if (m_fftOut)   fftw_free(m_fftOut);
        if (m_fftDataX) delete (m_fftDataX);
        if (m_fftDataY) delete (m_fftDataY);

        // find the smallest power of 2 which is > the length of the pulse array
        // this is the new "length" of the fft plan
        m_fftLength = 2;
        while ( m_fftLength < m_pulseLength )
        {
            m_fftLength *= 2;
        }

        // don't zero pad
        m_fftLength = m_pulseLength;

        std::cout << "Planning new FFT for channel " << m_channel << std::endl;
        std::cout << "\tPulse length: " <<  m_pulseLength << std::endl;
        std::cout << "\tFFT length: " <<  m_fftLength << std::endl;
        std::cout << "\tDisplay length: " <<  m_fftLength/2 << std::endl;
        std::cout << "\tSampleRate " << m_sampleRate << std::endl;

        // allocate space for the various arrays
        m_fftIn    = (double*) fftw_malloc(sizeof(double) * m_fftLength);
        m_fftOut   = (double*) fftw_malloc(sizeof(double) * m_fftLength);
        m_fftDataY = new double[m_fftLength/2];
        m_fftDataX = new double[m_fftLength/2];

        // create a new plan (these are functions from the fftw3 library
        m_fftPlan = fftw_plan_r2r_1d(m_fftLength, // the length of the plan
            m_fftIn,                              // the "in" array
            m_fftOut,                             // the "out" array
            FFTW_R2HC,                            // the "direction" of the transform, in this case Real to Half Complex
            FFTW_MEASURE);                        // how "optimal" you want the FFT routines (more inital overhead, less per calc overhead)
        // note about real numbers conversion
        // usually if you put in n real numbers into an fft, you get out n/2+1 complex numbers
        // note about half complex numbers
        // let's say we have n complex numbers where the kth number is composed of real imaginary parts R(k) + I(k)
        // instead of having an array of complex numbers with length n, we construct an array of length n in the following order:
        // R[0], R[1], R[2], ...,R[n/2], I[ ((n+1)/2) +1 ], ..., I[2], I[1]
        // note b/c of symmetry, the 0th elements and the nyquist element have no imiginary part and are therefore left out of the second half of the array
        // assuming of course that n is even.  if n is odd, the nyquest frequency lies b/w indicies and is included.
        // all of this is done so that the input and output arrays are the same lenght

        // redo the display's x-axis
        double scale = m_sampleRate / m_fftLength ;
        for (unsigned int i = 0; i<m_fftLength/2; i++)
        {
            m_fftDataX[i] = ((double)i) * scale;
        }

        // clear the queue
        m_fftOutMagnitudeDeque.clear();

        // release the lock
        fftLock.releaseLock();
    }

    /////////////////////////////////////////////////////////////////////////////
    double* FFTChannel::plotY(void)
    {
        return &m_fftDataY[1];                    // we always start at the first point but i don't know why we don't use it
        // i now know why.  the 0th element is the dc level which we don't care about
    }

    /////////////////////////////////////////////////////////////////////////////
    double* FFTChannel::plotX(void)
    {
        return &m_fftDataX[1];                    // we always start at the first point but i don't know why we don't use it
        // i now know why.  the 0th element is the dc level which we don't care about
    }

    /////////////////////////////////////////////////////////////////////////////
    int FFTChannel::plotN(void)
    {
        return m_fftLength/2 - 1;                 // since we never plot the [0] point
    }

    /////////////////////////////////////////////////////////////////////////////
    double* FFTChannel::dataY(void)
    {
        return &m_fftDataY[0];                    // we care about all levels here, so give the dc level i.e. the 0th element
    }

    /////////////////////////////////////////////////////////////////////////////
    double* FFTChannel::dataX(void)
    {
        return &m_fftDataX[0];                    // we care about all levels here, so give the dc level i.e. the 0th element
    }

    /////////////////////////////////////////////////////////////////////////////
    int FFTChannel::dataN(void)
    {
        return m_fftLength/2;                     // the full array
    }

    /////////////////////////////////////////////////////////////////////////////
    int FFTChannel::numRecords(void)
    {
        return m_numRecsInFinalFFT;
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::numRecords(int i)
    {
        if (i<=0)
        {
            printf ("Bad Input!!! \n");
            i=1;
        }
        m_numRecsInFinalFFT = i;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool FFTChannel::fftFlag(void)
    {
        return m_fftFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::fftFlag(bool b)
    {
        m_fftFlag = b;
        if (m_fftFlag) std::cout << "Computing FFTs ";
        else std::cout << "Not computing FFTs ";
        std::cout << "for channel " << m_channel << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool FFTChannel::windowingFlag(void)
    {
        return m_windowingFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::windowingFlag(bool b)
    {
        std::cout << "Windowing ";

        m_windowingFlag = b;
        if (m_windowingFlag) std::cout << "On ";
        else std::cout << " OFF ";
        std::cout << "for channel " << m_channel << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    int FFTChannel::discriminator(void)
    {
        return m_discriminator;
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTChannel::discriminator(int i)
    {
        if (i<=0)
        {
            printf ("Bad discriminator!  It is k%%n==0, so 1 throws no pulses away!\n");
            i=1;
        }
        m_discriminator = i;
    }

    /////////////////////////////////////////////////////////////////////////////
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
