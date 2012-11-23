////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "LANLOutput.h"
#include <string.h>
////////////////////////////////////////////////////////////////////////////////
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    LANLOutput::LANLOutput():
    XOUT()
    {
    }

    /////////////////////////////////////////////////////////////////////////////
    void LANLOutput::writeHeaderXOUT(XPulseRec PR)
    {

        // make sure there is a file
        if (NULL == m_dataWriterFD) return;

        // get the lock
        m_lock.getLock();

        // quick note about the header: we assume that all of the important information
        // that is in the header will be the same for every channel.  this, of course,
        // doesn't have to be the case, but that is the weakness of the quick and dirty hack.
        // so we are going to grab everything from the first pulse and hope it is representative!

        // print the base time in uS from epoch to base
        fprintf( m_dataWriterFD, "BaseTime: %lld\n", PR.timecodeBase());
        printf( "BaseTime: %lld\n", PR.timecodeBase());

        // record length
        fprintf( m_dataWriterFD, "RecordLength: %d\n", PR.length());
        printf( "RecordLength: %d\n", PR.length());

        // pretrigger length
        fprintf( m_dataWriterFD, "PretriggerLength: %d\n", PR.pretrigPnts());
        printf( "PretriggerLength: %d\n", PR.pretrigPnts());

        // sample rate
        fprintf( m_dataWriterFD, "SampleRate: %e\n", (1/(double)PR.deltat()));
        printf( "SampleRate: %e\n", (1/(double)PR.deltat()));

        // yscale (volts per raw)
        // note: there may be some signed issues here depending on whether you are writing both feedback and error
        fprintf( m_dataWriterFD, "YScale: %e\n", PR.yscale());
        printf( "YScale: %e\n", PR.yscale());

        // print out some stuff about each channel

        // the header is now written
        headerFlag(true);

        // release the lock
        m_lock.releaseLock();

    }

    /////////////////////////////////////////////////////////////////////////////
    void LANLOutput::writeRecordXOUT(const XPulseRec &PulseRec)
    {

        // if you are not writing, just return
        if (!m_writingFlag) return;

        // check to make sure the header is written
        if (!headerFlag()) writeHeaderXOUT(PulseRec);

        // get the lock on the writer
        m_lock.getLock();

        // iterate the record number
        m_recordWritten++;

        // setup the write buffer
        int size = (PulseRec.length() * 2) + 8 + 4 + 1;
        //printf ("size %d\n",size);
        unsigned char *buf = new unsigned char [size];
        unsigned int bufPosition = 0;

        // time
        unsigned long long int tick = PulseRec.timecode();
        printf ("delta %lld\t",tick);
        tick = htonl(tick);
        memcpy(&buf[bufPosition], &tick, 8);
        bufPosition += 8;

        // channel
        unsigned int chan = PulseRec.channel();
        printf ("chan %d\t",chan);
        chan = htonl(chan);
        memcpy(&buf[bufPosition], &chan, 4);
        bufPosition += 4;

        // flag byte
        unsigned char flag = 0;
        if (PulseRec.signedSamples())   flag = flag | 0x01;
        if (false) flag = flag | 0x02;
        if (false) flag = flag | 0x04;
        if (false) flag = flag | 0x08;
        if (false) flag = flag | 0x10;
        if (false) flag = flag | 0x20;
        if (false) flag = flag | 0x40;
        if (false) flag = flag | 0x80;
        printf ("flag %d\n",flag);
        memcpy(&buf[bufPosition], &flag, 1);
        bufPosition += 1;

        // get the iterator
        std::deque<unsigned short int>::const_iterator raw = PulseRec.rawSampleBegin();

        // loop through the data and put it into the buffer
        for (unsigned int i = 0;i<PulseRec.length(); i++, raw++)
        {
            //printf ("\t%u",i);
            //printf ("\traw: %u",*raw);
            //printf ("\tunsigned sub: %u",*raw-32768);
            //printf ("\tsigned sub: %d",*raw-32768);
            //printf ("\tsigned short: %d\n",(signed short) (*raw-32768));

            // get the sample
            unsigned short samp = (*raw);
            samp = ntohs(samp);
            //printf ("[%d: %x]",bufPosition,samp);

            memcpy(&buf[bufPosition], &samp, 2);
            bufPosition += 2;
        }

        // write the data to file
        fwrite(buf,sizeof(unsigned char),bufPosition,m_dataWriterFD);

        // delete the memory
        delete (buf);

        // release the lock
        m_lock.releaseLock();

        // test to see if you are done writing
        if (m_recordMax && (m_recordWritten >= m_recordMax))
        {
            std::cout << "Done writing data!" << std::endl;
            closeXOUT();
        }

    }

    /////////////////////////////////////////////////////////////////////////////

}                                                 // end of name space


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
