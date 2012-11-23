////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "PLSOutput.h"

////////////////////////////////////////////////////////////////////////////////
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    PLSOutput::PLSOutput(): XOUT()
    {
    }

    /////////////////////////////////////////////////////////////////////////////
    PLSOutput::PLSOutput(bool evenChannelSignedFlag, bool oddChannelSignedFlag, unsigned int nStream): XOUT()
    {
        init (evenChannelSignedFlag, oddChannelSignedFlag, nStream);
    }

    /////////////////////////////////////////////////////////////////////////////
    void PLSOutput::init(bool evenChannelSignedFlag, bool oddChannelSignedFlag, unsigned int nStream)
    {
        m_evenChannelSignedFlag = evenChannelSignedFlag;
        m_oddChannelSignedFlag = oddChannelSignedFlag;
        m_nStream = nStream;
    }

    /////////////////////////////////////////////////////////////////////////////
    void PLSOutput::writeHeaderXOUT(XPulseRec PR)
    {

        // make sure there is a file
        if (NULL == m_dataWriterFD) return;

        // get the lock
        m_lock.getLock();

        // header
        char *PLSWriterVersion = "V 2005 08 18 PLSOutput writer code";
        fprintf(m_dataWriterFD,"%-80s",PLSWriterVersion);

        /* get current time and write to the output file */
        struct tm *tmnow;
        time_t timenow;
        time( &timenow);
        tmnow = localtime( &timenow);
        fprintf( m_dataWriterFD, "O %.2d %.2d %.2d", tmnow->tm_mon + 1, tmnow->tm_mday,
            tmnow->tm_year - 100);

        /* write record length to output file */
        /* All our record lengths are equal, so use the first channel*/
        fprintf( m_dataWriterFD, "S %.5d", PR.length());

        /* Extra bytes for high-res timestamp*/
        fprintf( m_dataWriterFD, "E 00004");

        fprintf( m_dataWriterFD, "%-80s","C Written by PLSOutput Module.");

        /* 
         some notes about the header in PLS
         PLS is supposed to be record driven, so you can have a little header info
         a little data info and then some more header.  steve's reader can't handle this.
         larry's solution is to write all of the header information first, and then to
         write all data after that.  he makes some assumptions about the incoming data
         that was correct for NDFB data.  even channels are signed, odd channels are unsigned.
         of coure, for rocket data this isn't true.

         i propose this:  write out a header as if you are expecting 128 channels using
         larry's assumptions.  i don't think the pls readers will complain about this.
        then, keep track of what the header _should_ be while you are writing the actual
        binary data.  then, when it is time to close the file, rewind the file and write the
        new header over the old one. as long as the header is the same length, it shouldn't
        matter.  we shall see....

        */

        /*
         note about scale and offset:
         in pls, volts = (raw - offset) * scale
            in our system, volts = raw * scale + offset
         we will need to convert our offset (double) into their offset (int)
         */

        /* note about LEB's conversion from signed and unsigned etc in pls

         the way that LB had it set up, for signed channels, there is no additional
         offset noted in the header and no offset when writing the binary data
         for unsigned channels there was an additional offset was -32768 given in the header
         and an offset of -32768 when writing the binary data

         let's try a few examples to convince ourselves that this is okay
         unsigned samples:
               volt = 0 -> raw = 0 -> pls raw = -32768, (pls_raw - offset) = 0 okay
               volt = 1 -> raw = 16383 -> pls_raw = -16385, (pls_raw - offset) = 16383, okay
        signed samples:
        volt = +0.5 -> raw = (2^13-1) = 8191 -> pls_raw = 8189, (pls_raw - offset) = 8191, okay
        volt = -0.5 -> raw = -(2^13) = -8192 in signed, in unsigned = 57344 -> pls_raw = 57344, (pls_raw - offset) = 57344 = -8192

        we now have flags that tell us how to do the header

        */

        // make a header for 128 channels just to be sure
        for (int chan = 0; chan < 128; chan++)
        {
            char nbuf[128]= {' '};
            // note:  even if you have 3 digit channel numbers, it should still be fine b/c
            //        even though the white space padding will be wrong, you only write 47
            //        bytes to the actual header file

            if ((chan % 2) == 0)                  // even
            {
                if (m_evenChannelSignedFlag)
                {
                    // even & signed: no additional offset necessary
                    sprintf(nbuf,
                        "N %.2d %d %12.5e %d %12.5e                                      ",
                        chan + 1,                 // one based channel number, be careful if you get have 3 digit channel pixel numbers!!!
                        (int) 0,                  // always zero but i don't know why
                        PR.deltat(),              // time b/w samples (1 over sample rate).  deltat is in seconds already, so no need to convert it i think
                                                  // this is supposed to convert to signed short, but i don't know why that should work... jmk
                        (signed short)(PR.physicalToRaw(PR.yoffset())),
                        PR.yscale()               // convert raw to volts
                        );
                }
                else
                {
                    // even & unsigned:  give an additional offset of -32768.  make sure the raw numbers have this factor included
                    sprintf(nbuf,
                        "N %.2d %d %12.5e %d %12.5e                                      ",
                        chan + 1,                 // one based channel number, be careful if you have 3 digit channel pixel numbers!!!
                        (int) 0,                  // always zero but i don't know why
                        PR.deltat(),              // time b/w samples (1 over sample rate).  deltat is in seconds already, so no need to convert it i think
                                                  // this is supposed to convert to signed short, but i don't know why that should work... jmk
                        (signed short)(PR.physicalToRaw(PR.yoffset()) - 32768),
                        PR.yscale()               // convert raw to volts
                        );
                }
            }
            else
            {
                if (m_evenChannelSignedFlag)
                {
                    // odd & signed: no additional offset necessary
                    sprintf(nbuf,
                        "N %.2d %d %12.5e %d %12.5e                                      ",
                        chan + 1,                 // one based channel number, be careful if you get have 3 digit channel pixel numbers!!!
                        (int) 0,                  // always zero but i don't know why
                        PR.deltat(),              // time b/w samples (1 over sample rate).  deltat is in seconds already, so no need to convert it i think
                                                  // this is supposed to convert to signed short, but i don't know why that should work... jmk
                        (signed short)(PR.physicalToRaw(PR.yoffset())),
                        PR.yscale()               // convert raw to volts
                        );
                }
                else
                {
                    // odd & unsigned:  give an additional offset of -32768.  make sure the raw numbers have this factor included
                    sprintf(nbuf,
                        "N %.2d %d %12.5e %d %12.5e                                      ",
                        chan + 1,                 // one based channel number, be careful if you have 3 digit channel pixel numbers!!!
                        (int) 0,                  // always zero but i don't know why
                        PR.deltat(),              // time b/w samples (1 over sample rate).  deltat is in seconds already, so no need to convert it i think
                                                  // this is supposed to convert to signed short, but i don't know why that should work... jmk
                        (signed short)(PR.physicalToRaw(PR.yoffset()) - 32768),
                        PR.yscale()               // convert raw to volts
                        );
                }
            }
            fwrite(nbuf,1,47,m_dataWriterFD);

        }

        // release the lock
        m_lock.releaseLock();

        // the header is now written
        headerFlag(true);
    }

    /////////////////////////////////////////////////////////////////////////////
    void PLSOutput::writeRecordXOUT(const XPulseRec &PulseRec)
    {

        if (m_writingFlag)
        {

            if (!headerFlag()) writeHeaderXOUT(PulseRec);

            m_lock.getLock();

            m_recordWritten++;

            // remember, xpulserecords have two different time values
            // the fist, called the time code base, is the time in uS from the epoch to the beginning
            // of the data stream.
            // the second is, just called the time code, is the time, in uS from the
            // timecodebase to the trigger point.
            // pls records have 4 bytes worth of time data at the beginning and another
            // 4 bytes of time data at the end of the
            unsigned int tick;
            tick = (PulseRec.timecode() + PulseRec.timecodeBase())/1000000LL;

            unsigned int usec;
            usec = (PulseRec.timecode() + PulseRec.timecodeBase())%1000000LL;

                                                  //samples + record header + "extra bytes" for hi-res timestamp
            unsigned int bufferSize = PulseRec.length()*2 + 11 + 4;
            unsigned char buffer [bufferSize];

            buffer[0] = 'D';
            buffer[1] = ' ';
                                                  //PLS file channels are 1's based
            buffer[2] =  (unsigned char)PulseRec.channel()+1;
            buffer[3] =  (0x000000FF & tick);
            buffer[4] =  (0x0000FF00 & tick) >> 8;
            buffer[5] =  (0x00FF0000 & tick) >> 16;
            buffer[6] =  (0xFF000000 & tick) >> 24;
            buffer[7] =  (0x000000FF & PulseRec.sampleCount());
            buffer[8] =  (0x0000FF00 & PulseRec.sampleCount()) >> 8;
            buffer[9] =  (0x00FF0000 & PulseRec.sampleCount()) >> 16;
            buffer[10] = (0xFF000000 & PulseRec.sampleCount()) >> 24;

            // setup a pointer to where the data will start in the buffer
            unsigned char *bptr = &buffer[11];

            // get the iterator
            //			std::deque<unsigned short int>::const_iterator raw = PulseRec.m_rawShortSamplesVector.begin();
            std::deque<unsigned short int>::const_iterator raw = PulseRec.rawSampleBegin();

            // loop through the data and put it into the buffer
            for (unsigned int i = 0;i<PulseRec.length();i++)
            {
                //printf ("\t%u",i);
                //printf ("\traw: %u",*raw);
                //printf ("\tunsigned sub: %u",*raw-32768);
                //printf ("\tsigned sub: %d",*raw-32768);
                //printf ("\tsigned short: %d\n",(signed short) (*raw-32768));

                short samp;
                if (PulseRec.signedSamples())
                {
                    // a signed error stream.  no need to add an offset b/c it is already signed. see note above
                    samp =(signed short) (*raw);  // convert to signed?
                }
                else
                {
                    // an unsigned stream.  convert this to signed.
                    // accomplished by subtracting 32768 and adding it back in the offset.  see note above
                                                  // convert to signed
                    samp =(signed short) (*raw - 32768);
                }
                raw++;
                *bptr++ = (unsigned char)((samp & 0xFF00) >> 8);
                *bptr++ = (unsigned char)(samp & 0x00FF);
            }

            *bptr++ = (0x000000FF & usec);
            *bptr++ = (0x0000FF00 & usec) >> 8;
            *bptr++ = (0x00FF0000 & usec) >> 16;
            *bptr++ = (0xFF000000 & usec) >> 24;

            fwrite(buffer,1,11+2*PulseRec.length()+4,m_dataWriterFD);
            m_lock.releaseLock();

            if (m_recordMax && (m_recordWritten >= m_recordMax))
            {
                std::cout << "Done writing data!" << std::endl;
                closeXOUT();
            }
        }
    }

}                                                 // end of name space


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
