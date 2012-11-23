////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "FFTOutput.h"

////////////////////////////////////////////////////////////////////////////////
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    FFTOutput::FFTOutput(): XOUT()
    {
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTOutput::writeHeaderXOUT( )
    {
        // make sure there is a file
        if (NULL == m_dataWriterFD) return;

        // get the lock
        m_lock.getLock();

        /* write header */
        printf ("wrote header for FFT output\n");

        // release the lock
        m_lock.releaseLock();

        // the header is now written
        headerFlag(true);

    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTOutput::writeRecordXOUT(const XPulseRec &PulseRec)
    {

        // don't use this

    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTOutput::writeRecordXOUT(unsigned int channel, double *freq, double *power, unsigned int n)
    {

        if (m_writingFlag)
        {

            if (!headerFlag()) writeHeaderXOUT();

            if (n==0)
            {
                printf ("FFT writer cannot write a 0 length array to file\n");
                return;
            }

            m_lock.getLock();

            // iterate the number of records written
            m_recordWritten++;

            // copy the actual data
            for (unsigned int i=0; i<n; i++)
            {
                fprintf (m_dataWriterFD,"%le\t%le\n", freq[i],power[i]);
            }

            m_lock.releaseLock();

            // see if we are done writing records
            if (m_recordMax && (m_recordWritten >= m_recordMax))
            {
                std::cout << "Done writing data!" << std::endl;
                closeXOUT();
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void FFTOutput::recordMax(int d)
    {
        if (d != 1)
        {
            printf ("FFT File writes can only record 1 data file at a time.\n");
            d=1;
        }
        m_recordMax = d;
        std::cout << "Max records set to " << d << std::endl;
    }
    /////////////////////////////////////////////////////////////////////////////

}                                                 // end of name space


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
