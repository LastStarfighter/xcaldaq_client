////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "XCALDAQServer.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
namespace xdaq
{

    /////////////////////////////////////////////////////////////////////////////
    XCALDAQServer::XCALDAQServer()
    {

        // set up the communications pipe
        commPipe = new DAQPipe();

        // set up the data pipe
        dataPipe = new DAQPipe();

        // temp data buffer stuff
        m_tempDataIndex = 0;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::connect()
    {
        std::cout << "Connecting to Server." << std::endl;

        // connect the comm pipe
        std::cout << "\tEstablishing communications pipe with NDFB Server....\n\t";
        commPipe->connect();
        if (commPipe->valid())
        {
            std::cout << "done!" << std::endl;
        }
        else
        {
            std::cout << "failed!" << std::endl;
            return;
        }

        // make the comm pipe non-blocking
        if (!commPipe->nonBlock())
        {
            commPipe->nonBlock(true);
        }

        // connect the data pipe
        std::cout << "\tEstablishing data pipe with NDFB Server....\n\t";
        dataPipe->connect();
        if (dataPipe->valid())
        {
            std::cout << "done!" << std::endl;
        }
        else
        {
            std::cout << "failed!" << std::endl;
            return;
        }

        // make the data pipe non-blocking
        if (!dataPipe->nonBlock())
        {
            dataPipe->nonBlock(true);
        }

        // test the comm pipe
        std::cout << "\tTesting comm pipe....";
        unsigned int res;
        res=command(XCD_TESTCOMM,0,0,0);
        // error check?
        std::cout << "done!" << std::endl;

        // test the data pipe
        std::cout << "\tTesting data pipe....";
        res=command(XCD_TESTDATA,0,0,64);
        unsigned char buf[64];
        dataPipe->gread(buf,64);
        // error check?
        std::cout << "done!" << std::endl;

        // all good
        std::cout << "Server Connected" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::shutdownServer()
    {
        std::cout << "Shutting down the server interface" << std::endl;
        commPipe->disconnect();
        dataPipe->disconnect();
        std::cout << "Server interface shut down" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::get(unsigned int secondary, unsigned int chan, unsigned int* l)
    {
        *l = command(XCD_GET,secondary,chan,0);
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::get(unsigned int secondary, unsigned int chan, unsigned short int *s)
    {
        unsigned int l=0;
        l = command(XCD_GET,secondary,chan,0);
        *s = (unsigned short int)l;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::get(unsigned int secondary, unsigned int chan, float *f)
    {
        unsigned int l=0;
        l = command(XCD_GET,secondary,chan,0);
        memcpy(f,&l,4);
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::get(unsigned int secondary, unsigned int chan, bool *b)
    {
        unsigned int i=0;
        i = command(XCD_GET,secondary,chan,0);
        if (i==1) *b=true;
        else *b = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XCALDAQServer::set(unsigned int secondary, unsigned int chan, bool b)
    {
        unsigned int i=0;
        if (b) i=1;
        int watchdog=0;
        while (0 > command(XCD_SET,secondary,chan,i))
        {
            watchdog++;
            if (watchdog > 5)
            {
                std::cout << "Command failed to send!"<<std::endl;
                return -1;
            }
        }

        return 0;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XCALDAQServer::set(unsigned int secondary, unsigned int chan, unsigned int i)
    {
        int watchdog=0;
        while (0 > command(XCD_SET,secondary,chan,i))
        {
            watchdog++;
            if (watchdog > 5)
            {
                std::cout << "Command failed to send!"<<std::endl;
                return -1;
            }
        }

        return 0;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XCALDAQServer::set(unsigned int secondary, unsigned int chan, float f)
    {
        unsigned int i;
        memcpy(&i,&f,4);
        int watchdog=0;
        while (0 > command(XCD_SET,secondary,chan,i))
        {
            watchdog++;
            if (watchdog > 5)
            {
                std::cout << "Command failed to send!"<<std::endl;
                return -1;
            }
        }
        return 0;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned int XCALDAQServer::command(unsigned short int primary,
        unsigned short int secondary,
        unsigned int channel,
        unsigned int level)
    {

        unsigned short int command;
        float levelf;
        memcpy (&levelf,&level,4);

        command = XCD_COMMAND;

        /*
         std::cout << "Sending (host): " << std::endl;
         std::cout << "\tcommand: " << command << std::endl;
         std::cout << "\tprimary: " << primary << std::endl;
         std::cout << "\tsecondary: " << secondary << std::endl;
         std::cout << "\tchannel: " << channel << std::endl;
         std::cout << "\tlevel: " << level << " " << levelf << std::endl;
         */

        // declare buffer
        unsigned char       buf[16];

        //cast the buffer into different pointer types
        unsigned short int *bufs = (unsigned short int*) &buf;
        unsigned int       *bufi = (unsigned int*)       &buf;

        // reset buffer to zero
        for (int i=0;i<16;i++) buf[i]=0;

        /*
         std::cout << "Sending (host): " << std::endl;
         std::cout << "\tcommand: " << rcommand << std::endl;
         std::cout << "\tprimary: " << rprimary << std::endl;
         std::cout << "\tsecondary: " << rsecondary << std::endl;
         std::cout << "\tchannel: " << rchannel << std::endl;
         std::cout << "\tlevel: " << rlevel << std::endl;
         */

        // set command buffer
        bufs[0] = htons(command);
        bufs[1] = htons(primary);
        bufs[2] = htons(secondary);
        bufi[2] = htonl(channel);
        bufi[3] = htonl(level);

        /*
         std::cout << "Sending (net): " << std::endl;
         std::cout << "\tcommand: " << bufs[0] << std::endl;
         std::cout << "\tprimary: " << bufs[1] << std::endl;
         std::cout << "\tsecondary: " << bufs[2] << std::endl;
         std::cout << "\tchannel: " << bufi[2] << std::endl;
         std::cout << "\tlevel: " << bufi[3] << " " << levelf << std::endl;

         std::cout << "Sending chars (hton): " << std::endl;
         for (int i=0;i<16;i++) std::cout << "\t" << i << ": " << (int) buf[i] << std::endl;
         */

        // get the lock on the pipe:
        commPipe->lock.getLock();

        // send buffer
        commPipe->gwrite (buf, 16);               //  we set non blocking, so make sure to use gwrite

        // wait for responce
        unsigned char rbuf[16];
        commPipe->gread (&rbuf[0],16);

        // release the lock
        commPipe->lock.releaseLock();

        unsigned short int *rbufs      = (unsigned short int*) &rbuf;
        unsigned int       *rbufl      = (unsigned int*) &rbuf;

        unsigned short int  rcommand   = rbufs[0];
        unsigned short int  rprimary   = rbufs[1];
        unsigned short int  rsecondary = rbufs[2];
        unsigned int        rchannel   = rbufl[2];
        unsigned int        rlevel     = rbufl[3];

        /*
         std::cout << "received (net): " << std::endl;
         std::cout << "\tcommand: " << rcommand << std::endl;
         std::cout << "\tprimary: " << rprimary << std::endl;
         std::cout << "\tsecondary: " << rsecondary << std::endl;
         std::cout << "\tchannel: " << rchannel << std::endl;
         std::cout << "\tlevel: " << rlevel << std::endl;
         */

        rcommand   = ntohs (rcommand);
        rprimary   = ntohs (rprimary);
        rsecondary = ntohs (rsecondary);
        rchannel   = ntohl (rchannel);
        rlevel     = ntohl (rlevel);

        /*
         std::cout << "received (host): " << std::endl;
         std::cout << "\tcommand: " << rcommand << std::endl;
         std::cout << "\tprimary: " << rprimary << std::endl;
         std::cout << "\tsecondary: " << rsecondary << std::endl;
         std::cout << "\tchannel: " << rchannel << std::endl;
         std::cout << "\tlevel: " << rlevel << std::endl;
         */

        // parse the responce command
        if(rcommand != XCD_ACKNOWLEDGE)
        {
            std::cout << "Error 1" << std::endl;
            return (unsigned int) -1;
        }
        if(rprimary != primary)
        {
            std::cout << "Error 2" << std::endl;
            return (unsigned int) -2;
        }
        if(rsecondary != secondary)
        {
            std::cout << "Error 3" << std::endl;
            return (unsigned int) -3;
        }
        if(rchannel != channel)
        {
            std::cout << "Error 4" << std::endl;
            return (unsigned int) -4;
        }

        if (primary != XCD_GET)
        {
            if(rlevel != level) std::cout << "Error 5" << std::endl;
            return 0;
        }
        else
        {
            return rlevel;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    /**
     data comes from the server in chunks, but the chunks aren't necessarily
     organized in packets.  so, the server is writing whatever it can and the
     reader is reading whatever it can and storing it in order in the temp data buffer.
     meanwhile, another thread is looking at that temp data buffer and seeing if
     it can chunk out any full packets.  if it can, it removes the packet from the
     temp data buffer and puts it into the packet deque.  once there are no more
     full packets left in the temp data buffer, the partial packet at the end is
     copied over the beginning and it waits for more data.
     */
    unsigned int XCALDAQServer::getDataPacket(std::deque<unsigned char*> *deque, XCALDAQLock *lock)
    {
        static unsigned int blockSize = 0x200000; // 2 megs block size

        // all of this information comes from the XCALDAQRecordConstants files!!!

        // get what you can out of the pipe
        if (m_tempDataIndex + blockSize > TEMP_BUFFER_SIZE)
        {
            printf ("real problem with array sizes.  stop being lazy and fix them\n");
        }

        //		printf ("starting out tempDataIndex: %d\n",m_tempDataIndex);

        // wait until data is ready for reading on dataPipe
        dataPipe->readReady();

        // get the read socket lock
        dataPipe->lock.getLock();

        // read the data from the pipe
        int n = dataPipe->read(&m_tempDataBuffer[m_tempDataIndex],blockSize);
        if (n == -1)
        {
            printf ("error in the data pipe, kill this client!\n"); fflush(0);
            dataPipe->valid(false);
            commPipe->valid(false);

            // release the read socket lock
            dataPipe->lock.releaseLock();

            // return
            return 0;
        }

        //printf ("read %d bytes from the socket\n",n);fflush(0);

        // release the read socket lock
        dataPipe->lock.releaseLock();

        //printf ("Read %d bytes\n",n);
        m_tempDataIndex += n;
        if (m_tempDataIndex < 8)
        {
            return n;                             // wait for more data
        }

        /* 
         now that we have more than 8 bytes of data lets see if there are any
         full packets in our raw data stream.  if there are, chunk them out and put
         them in the raw data packet deque
         */

        // okay, we have some data, so let's see if we have any full packets
        unsigned int index = 0;                   // the starting position of the raw data packet in the stream
        unsigned int bytesInPacket;               // bytes in the current packet

        // keep track of how many packets we have received
        static int count = 0;

        for (;;)
        {

            // do we have enough data to look at the buffer size?
            if (m_tempDataIndex < (index + 8))    //  first 4 bytes are chan, second 4 bytes are # of bytes in packet
            {
                //printf ("not enough data to read size\n"); fflush(0);
                break;
            }

            // get the number of bytes in the current packet
            memcpy (&bytesInPacket,&m_tempDataBuffer[index+4],4);
            bytesInPacket = ntohl(bytesInPacket);

            // do we have enough data for the whole packet?
            if (m_tempDataIndex < (index + bytesInPacket))
            {
                break;
            }

            // init a new buffer that is long enough to store the entire data packet
            // remember, at some point, you will have to delete this buffer from memory!
            unsigned char *buf = new unsigned char [bytesInPacket];

            // copy the complete packet over to it's shiny new buffer
            memcpy (&buf[0],&m_tempDataBuffer[index], bytesInPacket);
            count++;

            // grab the deque lock
            lock->getLock();

            // add this buffer address to the end of the deque
            deque->push_back(buf);

            // release the deque lock
            lock->releaseLock();

            // update the index
            index += bytesInPacket;
        }

        // clean up the temp data buffer
        memcpy (&m_tempDataBuffer[0],&m_tempDataBuffer[index],(m_tempDataIndex-index));

        // update the temp data index
        m_tempDataIndex -= index;

        // return the number of bytes read
        return n;
    }

    /////////////////////////////////////////////////////////////////////////////
    // encapsulation
    /////////////////////////////////////////////////////////////////////////////
    char* XCALDAQServer::host (void)
    {
        return commPipe->hostid();
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::host (char* l_host)
    {
        commPipe->hostid (l_host);
        dataPipe->hostid (l_host);
    }

    /////////////////////////////////////////////////////////////////////////////
    int XCALDAQServer::port (void)
    {
        return commPipe->port();
    }

    /////////////////////////////////////////////////////////////////////////////
    void XCALDAQServer::port (int l_port)
    {
        commPipe->port(l_port);
        dataPipe->port(l_port);
    }

}                                                 // end nameclass xdaq


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
