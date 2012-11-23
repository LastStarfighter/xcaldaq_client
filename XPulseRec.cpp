
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "XPulseRec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
////////////////////////////////////////////////////////////////////////////////
namespace xdaq                                    // start name space
{

    /////////////////////////////////////////////////////////////////////////////
    // XPulseRec Class
    /////////////////////////////////////////////////////////////////////////////
    XPulseRec::XPulseRec ()
    {
        m_deltat            = 0.0;
        m_yoffset           = 0.0;
        m_yscale            = 1/16283;
        m_trigPoint         = 0;
        m_sampleCount       = 0;
        m_timecodeBase      = 0;
        m_timecode          = 0;
        m_pretrigPnts       = 0;
        m_channel           = 0;
        m_baseline          = 0.;
        m_pulseHeight       = 0.;
        m_clipped           = false;
        m_signedSamples     = false;
        m_calculatePhysFlag = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::calculatePhysSamples()
    {
        // turn on calculate phys so that any new samples added will be added to this deque as well
        m_calculatePhysFlag = true;

        // erase the raw samples deque
        erasePhysSamples();

        // resize the deque to be the same size as the raw short samples vector
        try
        {
            m_physSamplesVector.resize(m_rawShortSamplesVector.size());
        }
        catch  (const std::exception &x)
        {
            std::cout << x.what() << std::endl;
        }

        // set up the iterators
        std::deque<unsigned short int>::iterator rs = m_rawShortSamplesVector.begin();
        std::deque<double>::iterator ps = m_physSamplesVector.begin();

        // stick in the values
        for (unsigned int i=0; i < m_rawShortSamplesVector.size(); i++)
        {
            *ps = rawToPhysical(*rs);
            rs++;
            ps++;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::eraseRawShortSamples()
    {
        m_rawShortSamplesVector.erase(m_rawShortSamplesVector.begin(),m_rawShortSamplesVector.end());
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::erasePhysSamples()
    {
        m_physSamplesVector.erase(m_physSamplesVector.begin(),m_physSamplesVector.end());
    }

    /////////////////////////////////////////////////////////////////////////////
    int XPulseRec::eraseTo(unsigned long long startCount)
    {

        int nToErase = startCount - (m_sampleCount - length());

        if (nToErase < 0)
        {
            std::cout <<
                "channel " <<
                m_channel <<
                " tried to erase to: " <<
                startCount <<
                " but the lowest count available is: " <<
                m_sampleCount <<
                " - " <<
                length() <<
                "=" <<
                m_sampleCount - length() <<
                std::endl;
            //      throw std::runtime_error("eraseTo() called with invalid (too small) count.");
            return -1;
        }

        if ((unsigned int)nToErase > length())
        {
            std::cout <<
                "channel " <<
                m_channel <<
                " tried to erase to " <<
                startCount <<
                " but the largest count in this buffer is " <<
                m_sampleCount <<
                std::endl;

            //	throw std::runtime_error("eraseTo() called with invalid (too large) count.");
            return -1;
        }

        m_rawShortSamplesVector.erase(m_rawShortSamplesVector.begin(),m_rawShortSamplesVector.begin()+nToErase);
        if (m_calculatePhysFlag) m_physSamplesVector.erase(m_physSamplesVector.begin(),m_physSamplesVector.begin()+nToErase);
        return nToErase;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::appendRawShortSamples(unsigned short int raw)
    {
        // put this in the raw Short Vector
        m_rawShortSamplesVector.push_back(raw);

        if (m_calculatePhysFlag)
        {
            m_physSamplesVector.push_back(rawToPhysical(raw));
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::rawToPhysical(unsigned short raw)
    {
        double physical;
        if (m_signedSamples)
        {
            physical = (double)((signed short)raw * m_yscale + m_yoffset);
            //printf ("%d -> %f volts\n",(signed short)raw, physical);

        }
        else
        {
            physical = ((double)raw) * m_yscale + m_yoffset;
        }

        return physical;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned short XPulseRec::physicalToRaw(double physical)
    {
        unsigned short raw;
        if (m_signedSamples)
        {
            signed short raw2 = (signed short)((physical-m_yoffset)/m_yscale);
            raw = (unsigned short) raw2;
        }
        else
        {
            if ((physical-m_yoffset) < 0.)
            {
                std::cout << "You are asking to convert a negative number into a non-signed number. Clipping at 0" << std::endl;
                raw = 0;
            }
            else
            {
                raw = (unsigned short)((physical-m_yoffset)/m_yscale);
            }
        }
        //printf ("Got %f returning %u\n",physical, raw);
        return raw;
    }

    /////////////////////////////////////////////////////////////////////////////
    // statistical stuff
    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::rmsDC()
    {
        float rms=0;

        // set up the iterators
        std::deque<double>::iterator ps = m_physSamplesVector.begin();

        // find the total
        for (unsigned int i=0;i<m_physSamplesVector.size();i++)
        {
            rms += (*ps * *ps);
            ps++;
        }

        // get the average
        rms /= m_physSamplesVector.size();
        //std::cout<<"Mean of Squared: " << rms << std::endl;

        // get the sqrt
        rms = sqrt(rms);
        //std::cout << "DC Coupled RMS for channel " << m_channel << " is " << rms << std::endl;

        return rms;
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::rmsAC()
    {
        float rms=0;

        // set up the iterators
        std::deque<double>::iterator ps = m_physSamplesVector.begin();

        // find the total
        double samp=0;
        double avg = pulseAverage();
        for (unsigned int i=0;i<m_physSamplesVector.size();i++)
        {
            samp = *ps - avg;
            rms += (samp * samp);
            ps++;
        }

        // get the average
        rms /= m_physSamplesVector.size();
        //std::cout<<"Mean of Squared: " << rms << std::endl;

        // get the sqrt
        rms = sqrt(rms);
        //std::cout << "AC Coupled RMS for channel " << m_channel << " is " << rms << std::endl;

        return rms;
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::pulseAverage()
    {
        if (!m_calculatePhysFlag)
        {
            printf ("physical units have not been calculated yet!!!\n");
        }

        float avg=0;

        // set up the iterators
        std::deque<double>::iterator ps = m_physSamplesVector.begin();

        // find the total
        for (unsigned int i=0;i<m_physSamplesVector.size();i++)
        {
            avg += *ps;
            ps++;
        }

        // get the average
        avg /= m_physSamplesVector.size();
        //std::cout << "Average for channel " << m_channel << " is " << avg << std::endl;
        return avg;
    }

    /////////////////////////////////////////////////////////////////////////////
    // static functions
    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::average (double* array, int n)
    {
        double avg=0;

        for (int i=0; i<n; i++)
        {
            avg += array[i];
        }
        avg /= n;

        return avg;
    }

    double XPulseRec::standardDeviation (double *array, int n)
    {
        double std = 0;
        double avg = XPulseRec::average (array,n);
        for (int i=0; i<n; i++)
        {
            std += pow((array[i] - avg),2.);
        }
        std /= n;
        //printf ("variance: %e\n",std);
        std = sqrt (std);
        //printf ("std: %e\n",std);

        return std;
    }

    int XPulseRec::strip (double *array, int n, int deviations)
    {
        int m = 0;
        double avg = XPulseRec::average (array,n);
        double std = XPulseRec::standardDeviation (array, n);
        double upper = avg + deviations*std;
        double lower = avg - deviations*std;
        for (int i=0; i<n; i++)
        {
            if ((array[i] > upper) | (array[i] < lower))
            {
                //printf ("%d fail %d\n",i,m);;
            }
            else
            {
                array[m] = array[i];
                //printf ("%d pass %d\n",i,m);;
                m++;
            }
        }

        return m;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::analyzePulse()
    {

        if (!m_calculatePhysFlag)
        {
            printf ("physical units have not been calculated yet!!!\n");
        }

        // set up the array
        int n = m_physSamplesVector.size();
        double array[n];

        // copy over the data
                                                  // set up the iterators
        std::deque<double>::iterator ps = m_physSamplesVector.begin();
        for (int i=0; i<n; i++, ps++)
        {
            array[i] = *ps;
        }

        // find the heighest point in the array
        double max = array[0];
        for (int i=1; i<n; i++)
        {
            if (array[i] > max) max = array[i];
        }

        // get the average
        double avg = average (array,n);

        // strip out the outlying points three times
        for (int i=0; i<3; i++)
        {
            n = strip (array, n, 1);
        }

        // store the new average as the baseline
        m_baseline = average(array,n);

        // store the difference b/w the two as the pulse height
        m_pulseHeight = max - avg;

    }

    /////////////////////////////////////////////////////////////////////////////
    // request data copy
    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::requestDataCopy (double **x_pointer, double **y_pointer, unsigned int *n_pointer)
    {
        unsigned int n = m_rawShortSamplesVector.size();

        if (n == 0)
        {
            printf ("no data to copy!\n");
        }

        // calculate physics samples if necessary
        if (!m_calculatePhysFlag)
        {
            calculatePhysSamples();
        }

        // make some memory
        double *x = new double [n];
        double *y = new double [n];

        // copy the data
        std::deque<double>::iterator samp = m_physSamplesVector.begin();

        // set the scale
        double xscale = m_deltat;

        //if (m_decimateLevel != 0) xscale *= m_decimateLevel;

        // copy the scale over
        for (unsigned int i = 0; i < n; i++, samp++)
        {
            y[i] = *samp;
            x[i] = xscale * i;
            //printf ("%d [%f,%f]\n",i,x[i],y[i]);
        }

        // make sure the calling function can see it
        *n_pointer = n;
        *x_pointer = x;
        *y_pointer = y;

    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::requestDataDerivitiveCopy (double **x_pointer, double **y_pointer, unsigned int *n_pointer)
    {
        unsigned int n = m_rawShortSamplesVector.size();

        if (n == 0)
        {
            printf ("no data to copy!\n");
        }

        // calculate physics samples if necessary
        if (!m_calculatePhysFlag)
        {
            calculatePhysSamples();
        }

        // make some memory
        double *x = new double [n];
        double *y = new double [n];

        // copy the data
        std::deque<double>::iterator samp = m_physSamplesVector.begin();

        // set the scale
        double xscale = m_deltat;
        //if (m_decimateLevel != 0) xscale *= m_decimateLevel;

        // derivitive is calculated thus: ((Y3+Y4) - (Y1+Y2)) /  ((X3+X4) - (X1+X2))
        // but since the denominator never changes, calculate that externally
        double denominator = 4 * xscale;

        // because we are using 4 points to calculate the derivitive
        // our scale will go from 0 -> n-3;
        n -= 3;

        // copy the scale over
        for (unsigned int i = 0; i < n; i++, samp++)
        {
            y[i] = (*(samp + 3) + *(samp + 2) - *(samp + 1) - *(samp)) / denominator;;
            x[i] = xscale * i;
            //printf ("%d [%f,%f]\n",i,x[i],y[i]);
        }

        // make sure the calling function can see it
        *n_pointer = n;
        *x_pointer = x;
        *y_pointer = y;

    }

    /////////////////////////////////////////////////////////////////////////////
    // query only
    /////////////////////////////////////////////////////////////////////////////
    unsigned int XPulseRec::length() const
    {
        //		if (m_rawShortSamplesVector.size() != m_rawShortSamplesVector.size())
        //		std::cout << "something is very wrong with the phys samples..." <<std::endl;
        return m_rawShortSamplesVector.size();
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::baseline ( void )
    {
        return m_baseline;
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::pulseHeight ( void )
    {
        return m_pulseHeight;
    }

    /////////////////////////////////////////////////////////////////////////////
    std::deque<double>::const_iterator XPulseRec::physicalBegin  ( void ) const
    {
        std::deque<double>::const_iterator funk = m_physSamplesVector.begin();
        return funk;
    }

    /////////////////////////////////////////////////////////////////////////////
    std::deque<unsigned short>::const_iterator XPulseRec::rawSampleBegin  ( void ) const
    {
        return m_rawShortSamplesVector.begin();
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::physMax()
    {
        return m_rawMax * m_yscale + m_yoffset;
    }

    /////////////////////////////////////////////////////////////////////////////
    double XPulseRec::physMin()
    {
        return m_rawMin * m_yscale + m_yoffset;
    }

    /////////////////////////////////////////////////////////////////////////////
    // full encapsulation
    /////////////////////////////////////////////////////////////////////////////
    int XPulseRec::rawMax()
    {
        return m_rawMax;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::rawMax(int d)
    {
        m_rawMax = d;
        //std::cout << "Channel " << m_channel << " Physical Max set to: " << m_physicalMax << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XPulseRec::rawMin()
    {
        return m_rawMin;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XPulseRec::rawMin(int d)
    {
        m_rawMin = d;
        //std::cout << "Channel " << m_channel << " Physical Min set to: " << m_physicalMin << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool XPulseRec::signedSamples() const {return m_signedSamples;}
    void XPulseRec::signedSamples(bool val) {m_signedSamples = val;}

    bool XPulseRec::calculatePhysFlag() const {return m_calculatePhysFlag;}
    void XPulseRec::calculatePhysFlag(bool val)
    {
        m_calculatePhysFlag = val;
        calculatePhysSamples();
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned long long int XPulseRec::timecode() const {return m_timecode;}
    void XPulseRec::timecode(unsigned long long int l_timecode) {m_timecode = l_timecode;}

    unsigned long long int XPulseRec::timecodeBase() const {return m_timecodeBase;}
    void XPulseRec::timecodeBase (unsigned long long int l_timecodeBase) {m_timecodeBase = l_timecodeBase;}

    void XPulseRec::channel(int l_channel) {m_channel = l_channel;}
    int XPulseRec::channel() const {return m_channel;}

    void XPulseRec::pretrigPnts(unsigned int pretrig) {m_pretrigPnts= pretrig;}
    unsigned int XPulseRec::pretrigPnts() const {return m_pretrigPnts;}

    void XPulseRec::yoffset(double l_yoffset) {m_yoffset = l_yoffset;}
    double XPulseRec::yoffset() const {return m_yoffset;}

    void XPulseRec::yscale(double l_yscale) {m_yscale = l_yscale;}
    double XPulseRec::yscale() const {return m_yscale;}

    void XPulseRec::deltat(double deltat) {m_deltat = deltat;}
    double XPulseRec::deltat() const {return m_deltat;}

    void XPulseRec::triggerType (TriggerType tt) {m_triggerType = tt;}
    TriggerType XPulseRec::triggerType ( void ) const {return m_triggerType;}

    void XPulseRec::threshold(double threshold) {m_threshold = threshold;}
    double XPulseRec::threshold() const {return m_threshold;}

    void XPulseRec::clipped(bool clipped) {m_clipped = clipped;}
    bool XPulseRec::clipped() const {return m_clipped;}

    unsigned long long int XPulseRec::sampleCount() const {return m_sampleCount;}
    void XPulseRec::sampleCount(unsigned long long int count) {m_sampleCount = count;}

    unsigned long long int XPulseRec::timeCount() const {return m_timeCount;}
    void XPulseRec::timeCount(unsigned long long int count) {m_timeCount = count;}

}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
