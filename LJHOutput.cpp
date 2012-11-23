////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "LJHOutput.h"
#include <string.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    LJHOutput::LJHOutput(): XOUT()
    {
        m_initialized = false;

        // get some memory
        int n = 40;
        header = new char* [n];
        title = new char* [n];
        for (int i=0;i<n;i++)
        {
            header[i] = new char [256];
            title[i] = new char [256];
        }

        cfgFD = NULL;
        LJHOutput::readCFGFile();

        // setup the title
        sprintf(&title[0][0],"Save File Format Version: ");
        sprintf(&title[1][0],"Software Version: ");
        sprintf(&title[2][0],"Software Driver Version: ");
        sprintf(&title[3][0],"Date: ");
        sprintf(&title[4][0],"Acquisition Mode: ");
        sprintf(&title[5][0],"Digitized Word Size in bytes: ");
        sprintf(&title[6][0],"Location: ");
        sprintf(&title[7][0],"Cryostat: ");
        sprintf(&title[8][0],"Thermometer: ");
        sprintf(&title[9][0],"Temperature (mK): ");
        sprintf(&title[10][0],"Bridge range: ");
        sprintf(&title[11][0],"Magnetic field (mGauss): ");
        sprintf(&title[12][0],"Detector: ");
        sprintf(&title[13][0],"Sample: ");
        sprintf(&title[14][0],"Excitation/Source: ");
        sprintf(&title[15][0],"Operator: ");
        sprintf(&title[16][0],"SYSTEM DESCRIPTION OF THIS FILE: ");
        sprintf(&title[17][0],"USER DESCRIPTION OF THIS FILE: ");
        sprintf(&title[18][0],"Number of Digitizers: ");
        sprintf(&title[19][0],"Number of Active Channels: ");
        sprintf(&title[20][0],"Timestamp offset (s): ");
        sprintf(&title[21][0],"Digitizer: ");
        sprintf(&title[22][0],"Description: ");
        sprintf(&title[23][0],"Master: ");
        sprintf(&title[24][0],"Bits: ");
        sprintf(&title[25][0],"Effective Bits: ");
        sprintf(&title[26][0],"Anti-alias low-pass cutoff frequency (Hz): ");
        sprintf(&title[27][0],"Timebase: ");
        sprintf(&title[28][0],"Number of samples per point: ");
        sprintf(&title[29][0],"Presamples: ");
        sprintf(&title[30][0],"Total Samples: ");
        sprintf(&title[31][0],"Trigger (V): ");
        sprintf(&title[32][0],"Tigger Hysteresis: ");
        sprintf(&title[33][0],"Trigger Slope: ");
        sprintf(&title[34][0],"Trigger Coupling: ");
        sprintf(&title[35][0],"Trigger Impedance: ");
        sprintf(&title[36][0],"Trigger Source: ");
        sprintf(&title[37][0],"Trigger Mode: ");
        sprintf(&title[38][0],"Trigger Time out: ");
        sprintf(&title[39][0],"Use discrimination: ");
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::openCFGFile()
    {
        if (cfgFD != NULL)
        {
            closeCFGFile();
        }

        printf ("Opening LJH configuration file");
        cfgFD = fopen("ljh.cfg","r+");
        if (cfgFD == NULL)
        {
            cfgFD = fopen("../../ljh.cfg","r+");
            if (cfgFD == NULL)
            {
                std::cout << "LJH Setup File does not exist. For right now, you are screwed" << std::endl;
                exit(-1);
            }
        }
        printf ("done!\n");

    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::closeCFGFile()
    {
        printf ("Closing LJH configuration file");

        if (cfgFD != NULL)
        {
            fclose (cfgFD);
            cfgFD = NULL;
        }
        printf ("done!\n");
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::readCFGFile()
    {

        if (cfgFD == NULL) openCFGFile();

        char temp;
        char line[256];
        //std::cout<< "Reading Setup File:" << std::endl;
        int i=0;
        int lineNum=0;
        while (fread(&temp, sizeof(char), 1, cfgFD) != 0)
        {
            if (temp != '\n')
            {
                line[i] = temp;
                i++;
            }
            else
            {
                line[i] = '\0';
                line[i+1] = '\0';
                i = 0;
                if (line[0] == '#')
                {
                    //printf ("\tComment: %s\n",&line[0]);
                    continue;
                }
                //	printf ("%s\n",line);
                char *pos = strstr(line,": ");
                if (pos == NULL)
                {
                    //std::cout<< "No ':', skipping line..." << std::endl;
                    if (lineNum < 40)
                    {
                        header[lineNum][0] = '\0';// make sure you num term the string
                    }
                }
                else
                {
                    pos += 2;                     // move it forward to past the ': '
                    //for (int i=0;i<(pos-line);i++) printf (" ");
                    //printf ("^\n");
                    strncpy (&header[lineNum][0],pos,255);
                    header[lineNum][255] = '\0';  // make sure you num term the string
                }
                lineNum++;
            }
        }

        for (int i=0; i<40;i++)
        {
            //printf ("Line %d:\t%s\n",i,header[i]);
        }

        closeCFGFile();
        //std::cout<< "Finished Reading Setup File:" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::saveCFGFile()
    {
        char buf[256];
        openCFGFile();

        for (int i=0;i<40;i++)
        {
            sprintf(buf,"%s%s\n",title[i],header[i]);
            fprintf(cfgFD,buf);
            std::cout << "Writing: " << buf;
        }

        closeCFGFile();

    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::initXOUT(unsigned int chan)
    {
        m_numChan = chan;

        // get some memory space
        m_chanHeader = new bool   [m_numChan];
        m_chanFD     = new FILE*  [m_numChan];
        m_range      = new double [m_numChan];
        m_offset     = new double [m_numChan];
        m_bits       = new int    [m_numChan];
        m_polarity   = new int    [m_numChan];

        // init some stuff
        for (unsigned int chan=0;chan<m_numChan;chan++)
        {
            m_chanHeader[chan] = false;
            m_chanFD[chan]     = NULL;
            m_range[chan]      = 0.;
            m_offset[chan]     = 0.;
            m_bits[chan]       = 16;
            m_polarity[chan]   = 1;
        }

        m_initialized = true;
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::openXOUT(char *filename,char *mode)
    {
        XOUT::openXOUT(filename,mode);

        fprintf (m_dataWriterFD,"This file is just a place holder.  Each channel has it's own file\n");

    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::closeXOUT()
    {

        // write your parting shot
        m_lock.getLock();
        if (m_dataWriterFD != NULL) fprintf (m_dataWriterFD,"Closing the file\n");

        // loop through all channel files and close them if necessary
        for (unsigned int chan=0; chan<m_numChan; chan++)
        {
            //printf ("chan %d m_numChan %d\n",chan,m_numChan); fflush(0);

            // get the file descriptor of this channel
            FILE* FD = m_chanFD[chan];

            // if it does not exist, keep looping
            if (FD == NULL) continue;

            // print to the place holder file
            //std::cout << "Closing channel "<< chan << "'s output file." << std::endl; fflush(0);
            fprintf (m_dataWriterFD,"Closing channel output file for channel: %d\n",chan);

            // actually close the file
            fclose(FD);

            // set the descriptor to zero
            m_chanFD[chan] = NULL;

            // set the header flag back to zero
            m_chanHeader[chan] = false;
        }

        // close the cfg file if it is still open
        closeCFGFile();

        // release lock
        m_lock.releaseLock();

        // close out the main file
        XOUT::closeXOUT();
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::writeHeaderXOUT(XPulseRec PR)
    {

        // if the writer isn't initialized, print an error text
        // probably should handle this at some point
        if (!m_initialized)
        {
            std::cout << "This writer isn't initialized yet!" << std::endl;
        }

        // check if we need to write a header for this channel
        if (m_chanHeader[PR.channel()])
        {
            //printf ("Already have a header for channel %d\n",PR.channel());
            return;
        }

        // first off, let's construct what the file will be called
        std::string tag("_chan");
        std::stringstream ss;
        std::string str;
        ss << PR.channel();
        ss >> str;
        tag += str;
        std::string filename = filePath();
        unsigned int i = filename.find(".",0);
        if (i != std::string::npos)
        {
            filename.insert(i,tag);
        }
        else
        {
            filename += tag + ".ljh";
        }

        // open the file for writing
        std::cout << "Channel "<< PR.channel() <<" doesn't have a open data file yet. " << std::endl;
        std::cout << "Opening: " << filename << " for writing" << std::endl;
        m_chanFD[PR.channel()] = fopen(filename.c_str(),m_mode);

        // now print the header
        printf ("printing a header for channel %d\n",PR.channel());

        // get the lock
        m_lock.getLock();

        // print a little note in the main file about this
        fprintf(m_dataWriterFD,"Channel %d opened a file for writing.\n",PR.channel());

        // drop the timecodebase from the PR record into the LJH header (line #4)
        unsigned long long sec  = PR.timecodeBase()/1000000LL;
        unsigned long long usec = PR.timecodeBase()%1000000LL;

        time_t tim = (time_t)sec;
        struct tm *ltime = gmtime(&tim);
        strftime(&header[3][0],255,"%d %b %Y, %H:%M:%S GMT",ltime);

        /*
        std::cout << "Using timecode " << PR.timecodeBase() << " to set date."<<std::endl;
        std::cout << "Setting date string to " << header[3] << std::endl;
        */

        // use the same information to setup the timestamp offset (line #21)
        sprintf (&header[20][0],"%llu.%06llu",sec,usec);

        //std::cout << "Setting timestamp offset to " << header[20] << std::endl;

        // get the timebase or the time in uS b/w samples
        sprintf(&header[27][0],"%e",PR.deltat());

        // fix this!!
        sprintf(&header[28][0],"%d",1);
        sprintf(&header[29][0],"%d",PR.pretrigPnts());
        sprintf(&header[30][0],"%d",PR.length());
        sprintf(&header[31][0],"%f",PR.threshold());
        //sprintf(&title[37][0],"Trigger Mode: ");
        //sprintf(&title[38][0],"Trigger Time out: ");

        /*

         // put the header info that you got from the config file into the channel file
                char endLine[4];
                endLine[0] = '\r';
                endLine[1] = '\n';
                endLine[2] = '';
                endLine[3] = '';

        for (int i=0;i<40;i++) {
        if (i==0) fprintf(FD,"#LJH Memorial File Format%s",endLine);
        if (i==18) fprintf(FD,"\n#End of description%s",endLine);
        fprintf(FD,"%s%s%s",title[i],header[i],endLine);
        }

        // get some info from the PR that will be used later
        m_range[PR.channel()] = PR.physMax()-PR.physMin();
        m_offset[PR.channel()] = PR.yoffset()*PR.yscale();
        m_bits[PR.channel()] = 14;
        m_polarity[PR.channel()] = 1;

        // write the particulars for the PR
        fprintf(FD,"Channel: %d.%d%s",1,0,endLine);
        fprintf(FD,"Description: %s%s","A (Voltage)",endLine);
        fprintf(FD,"Range: %f%s",m_range[PR.channel()],endLine);
        fprintf(FD,"Offset: %f%s",m_offset[PR.channel()],endLine); // this is VOLTS offset, we store raw offset in PR, so convert
        fprintf(FD,"Coupling: %s%s","DC",endLine);
        fprintf(FD,"Impedance: %s%s","1 Ohms",endLine);
        fprintf(FD,"Inverted: %s%s","No",endLine);
        fprintf(FD,"Preamp gain: %f%s","1",endLine);
        fprintf(FD,"Discrimination level (%%): %f%s",1,endLine);

        // write the EOH line
        fprintf(FD,"#End of Header%s",endLine);

        */

        for (int i=0;i<40;i++)
        {
            if (i==0) fprintf(m_chanFD[PR.channel()],"#LJH Memorial File Format\r\n");
            if (i==18) fprintf(m_chanFD[PR.channel()],"\n#End of description\r\n");
            fprintf(m_chanFD[PR.channel()],"%s%s\r\n",title[i],header[i]);
        }

        // get some info from the PR that will be used later
        // note, this is assuming a bi-polar adc.  so if we have a range of 0 volts to 1 volt,
        // ljh wants a range of .5v and an offset of .5v
        /// crazy huh?  so don't do it like this
        //    m_range[PR.channel()] = PR.physMax()-PR.physMin();
        //    m_offset[PR.channel()] = PR.yoffset()*PR.yscale();
        // do it like this:
        m_range[PR.channel()]  = ( PR.physMax() - PR.physMin() ) / 2.;
        m_offset[PR.channel()] = ( PR.physMax() + PR.physMin() ) / 2.;

        m_bits[PR.channel()] = 14;
        m_polarity[PR.channel()] = 1;

        // write the particulars for the PR
        fprintf(m_chanFD[PR.channel()],"Channel: %d.%d\r\n",1,0);
        fprintf(m_chanFD[PR.channel()],"Description: %s\r\n","A (Voltage)");
        fprintf(m_chanFD[PR.channel()],"Range: %f\r\n",m_range[PR.channel()]);
        fprintf(m_chanFD[PR.channel()],"Offset: %f\r\n",m_offset[PR.channel()]);
        fprintf(m_chanFD[PR.channel()],"Coupling: %s\r\n","DC");
        fprintf(m_chanFD[PR.channel()],"Impedance: %s\r\n","1 Ohms");
        fprintf(m_chanFD[PR.channel()],"Inverted: %s\r\n","No");
        fprintf(m_chanFD[PR.channel()],"Preamp gain: %f\r\n",1.0);
        fprintf(m_chanFD[PR.channel()],"Discrimination level (%%): %f\r\n",1.0);

        // write the EOH line
        fprintf(m_chanFD[PR.channel()],"#End of Header\r\n");

        // release the lock
        m_lock.releaseLock();

        // set the header flag for this channel to be true
        m_chanHeader[PR.channel()] = true;

        headerFlag(true);
    }

    /////////////////////////////////////////////////////////////////////////////
    void LJHOutput::writeRecordXOUT(const XPulseRec &PulseRec)
    {

        // make sure the writing flag is set
        if (m_writingFlag)
        {
            //std::cout << "For this record, timecode at base is:    " << PulseRec.timecodeBase() << std::endl;
            //std::cout << "For this record, timecode at trigger is: " << PulseRec.timecode() << std::endl;

            // send away to make sure file is open and header is there, ready for data
            writeHeaderXOUT(PulseRec);            // check if you have written a header

            unsigned int chan = PulseRec.channel();
            std::cout << "Writing Pulse For Channel: " << chan << std::endl;

            // error check
            if (PulseRec.length() == 0) throw std::runtime_error("No raw short samples in Pulse File!");

            // set the buffer
                                                  //samples + record header
            unsigned char m_buf [PulseRec.length()*2 + 6];

            // get the writer lock
            m_lock.getLock();

            // iterate the number of records
            m_recordWritten++;

            // write the first 2 bytes as required
            m_buf[0] = 0;                         // zero (raw data)
            //			m_buf[1] = c; // channel number, 255 is reserved for temp readout
            m_buf[1] = 0;                         // channel number, 255 is reserved for temp readout

            // get the millisecond counter of the trigger, defined as the # of milliseconds b/w the timecount base and
            // the trigger.  remember, in PR files, we have the number of uSecs from the epoch to the trigger time, so be sure to conver
            /// that into milliseconds
            unsigned long long int tick;          // want this limited to 32 bits b/c that is all that gets written to file

            // remember that timecodebase is uS from epoch
            //               timecode is the uS from the timecodebase
            // in LJH, we have a base (called timestamp offset, which is in the header, line 20), which is mS from epoch.
            // each record has a time stamp that is mS from the base time in line 20 i think.

            // first get the uS from timecodebase to timecode
            tick = (PulseRec.timecode());

            //std::cout << "Difference is " << tick << " uS. Should be this many seconds from the timecode base:" << header[3] << std::endl;

            // convert this tick to mS
            tick /= 1000;

            std::cout << "Time tick: " << tick;
            unsigned char x[4];
            memcpy (&x,&tick,4);
            printf ("\t%x %x %x %x\n",x[0],x[1],x[2],x[3]);

            //std::cout << "The trigger is " << tick << " mS from the timecode base to " << header[3] << std::endl;

            // we must guarantee that tick is written in LSB order on all systems, so convert it first to netorder (ie BIG ENDIAN)
            tick = htonl(tick);                   // tick is now BIG ENDIAN

            // back convert to LSB
            m_buf[2] = (0x000000FF & tick);
            m_buf[3] = (0x0000FF00 & tick) >> 8;
            m_buf[4] = (0x00FF0000 & tick) >> 16;
            m_buf[5] = (0xFF000000 & tick) >> 24;

            std::cout << "first six bytes: ";
            for (unsigned int y=0;y<6;y++)
            {
                printf ("\t%x",m_buf[y]);
            }
            printf ("\n");

            // setup a pointer to where the data will start in the buffer
            unsigned char *bptr = &m_buf[6];

            // setup to write the output
            // unsigned int scale = (unsigned int) pow(2,(m_bits[chan] - 1)) - 1;
            // unsigned int scale = (unsigned int) pow(2,(m_bits[chan])) - 1;
            // so for an 8 bit number, we get  2^7 - 1,  does this make sense?

            //std::cout << "Using Offset " << m_offset[chan] << std::endl;
            //std::cout << "Using Range " << m_range[chan] << std::endl;
            //std::cout << "Using Polarity " << m_polarity[chan] << std::endl;
            //double RxP = m_range[chan]*m_polarity[chan]; // for pulses, should be close to if not exactly one
            //std::cout << "Using Rxp of " << RxP << std::endl;
            //std::cout << "Using scale of " << scale << std::endl;

            // get the iterator
            std::deque<double>::const_iterator volt = PulseRec.physicalBegin();

            // loop through it
            unsigned char z[2];
            for (unsigned int i = 0;i<PulseRec.length();i++)
            {
                short samp;
                samp = (short) (
                    round(
                //											  ( (*volt - m_offset[chan]) / RxP) * scale
                    ( (*volt) / 1.0) * 16383      // don't forget this
                    )
                    );

                //std::cout << "Voltage: " << *volt << "\tSamp" << samp <<std::endl;

                // at this point, samp could be LSB or MSB, so convert it to something we know
                samp = htons(samp);

                // now we know it is big endian
                // write to file in little endian accordingly

                memcpy (&z,&samp,2);
                *bptr = z[1];
                bptr++;
                *bptr = z[0];
                bptr++;
                /*
                 *bptr = (unsigned char)((samp & 0x00FF) >> 8);
                bptr++;
                *bptr = (unsigned char)(samp & 0xFF00);
                bptr++;
                 */
                // printf ("Sample #%d: %d %x %x -> %x %x\n",i,samp,z[0],z[1],*(bptr-2),*(bptr-1));

                volt++;
            }

            int size = 6 + 2*PulseRec.length();   // header + 2 bytes per samp * # of samples

            //printf ("m_buf: %x size: %d fd: %x\n",m_buf,size,m_chanFD[PulseRec.channel()]);fflush(0);

            // actually write the data to the file
            fwrite(m_buf,1,size,m_chanFD[PulseRec.channel()]);

            // release the lock
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
