#include "Client.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include "misc_utils.h"
#include "string_utils.h"
#include "StreamChannel.h"

namespace xdaq
{
    //////////////////////////////////////////////////////////////////////////
    Client::Client () :   trigger_config_filename("default.trigger_config"),mixing_config_filename("default.mixing_config")
    {
        std::cout << "Creating new Client Instance" << std::endl;

        // make sure any pointers are set NULL
        latestNoisePulse = NULL;
        m_optimizeMixPRArray = NULL;
        m_optimizeMixChannelFlag = NULL;
        server=NULL;
        m_XOUTWriter = NULL;

        // init variables
        m_totalBytes = 0;
        m_nDataStreams=0;
        m_connectProgressD = 1;
        m_connectProgressN = 0;
        sprintf (m_versionString,"Version: Unknown");

        // reset any flags
        m_optimizeMixCalculateMedianMixFlag = false;
        m_connected = false;
        m_streaming = false;
        m_channelInit = false;
        m_coupleOddToEvenFlag = false;

        // auto tune stuff
        optimizeMixRatioMethod (1);
        optimizeMixCouplePairsFlag (false);
        m_optimizeMixMaxRecords = 10;
        m_optimizeMixMaxRecords = 10;

        // each client gets its own data writer
        setXOUTType(PLS);                         // default to PLS
        writeAutoToFileFlag(true);
        writeLevelToFileFlag(true);
        writeEdgeToFileFlag(true);
        writeNoiseToFileFlag(false);

        // create the server interface
        server = new XCALDAQServer();

        std::cout << "done!" << std::endl;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // config file stuff
    ////////////////////////////////////////////////////////////////////////////////
    void Client::readConfigFile ()
    {
        openConfigFile ();

        char temp;
        char line[256];
        char keyword[256];
        char value [256];

        std::cout<< "Reading Network Config File..." << std::endl;

        int i=0;
        int lineNum=0;
        while (fread(&temp, sizeof(char), 1, g_configFD) != 0)
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

                //printf ("%s\n",line);
                //fflush(0);

                if (line[0] == '#')
                {
                    //printf ("\tComment: %s\n",&line[0]);
                    i = 0;
                    continue;
                }
                if ((line[0] == '/') & (line[1] == '/'))
                {
                    //printf ("\tComment: %s\n",&line[0]);
                    i = 0;
                    continue;
                }

                char *pos = strstr(line,": ");
                if (pos == NULL)
                {
                    //std::cout<< "No ':', skipping line..." << std::endl;
                }
                else
                {
                    strncpy (&keyword[0],&line[0], (pos - &line[0]));
                    keyword[(pos - &line[0])] = '\0';
                    strncpy (&value[0],&line[(pos - &line[0]) + 2],i-(pos - &line[0])-2);
                    value[i-(pos - &line[0])-2] = '\0';
                    //std::cout << "Keyword: " << keyword << "\tValue: " << value << std::endl;

                    std::cout<< "\t";
                    fflush(0);

                    parseConfigFile (keyword,value);
                }
                i = 0;
                lineNum++;
            }
        }

        std::cout<< "done!" << std::endl;

        // close the file
        std::cout << "Closing network configuration file." << std::endl << std::endl;
        fclose (g_configFD);
    }

    ////////////////////////////////////////////////////////////////////////////////
    void Client::parseConfigFile (char* keyword, char* value)
    {

        if (strstr (keyword, "port") != 0)
        {
            int i;
            sscanf(value,"%d",&i);
            server->commPipe->port(i);
            server->dataPipe->port(i);
            return;
        }

        if (strstr (keyword, "host") != 0)
        {
            // send it over
            server->commPipe->hostid(value);
            server->dataPipe->hostid(value);
            return;
        }

        std::cout << "Config file failed to parse keyword : " << keyword << " value " << value << std::endl;
    }

    ////////////////////////////////////////////////////////////////////////////////
    void Client::saveConfigFile ()
    {
        openConfigFile();

        std::cout << "Saving configuration file... ";

        fprintf (g_configFD, "// ndfb_server configuaration file\n");
        fprintf (g_configFD, "host: %s\n",server->host());
        fprintf (g_configFD, "port: %d\n",server->port());
        std::cout << "done!" << std::endl;

        // close the file
        std::cout << "Closing configuration file." << std::endl << std::endl;
        fclose (g_configFD);

    }

    ////////////////////////////////////////////////////////////////////////////////
    void Client::createConfigFile ()
    {
        std::cout << "Creating new configuration file... " << std::endl;

        g_configFD = fopen (CONFIG_FILE_NAME,"w");

        server->port(DEFAULT_XCALDAQ_PORT);
        server->host("localhost");

        saveConfigFile();

        std::cout << "Done creating new configuration file." << std::endl << std::endl;
    }

    ////////////////////////////////////////////////////////////////////////////////
    void Client::openConfigFile ()
    {
        char path[256];

        sprintf(path,"./%s",CONFIG_FILE_NAME);
        std::cout << "Opening configuration file " << path;
        g_configFD = fopen (path,"r+");
        if (g_configFD == NULL)
        {
            std::cout << " failed!" << std::endl;
            sprintf(path,"./../../%s",CONFIG_FILE_NAME);
            std::cout << "Opening configuration file " << path;
            g_configFD = fopen (path,"r+");
            if (g_configFD == NULL)
            {
                std::cout << " failed.  No configuration file found!" << std::endl;
                createConfigFile();
                openConfigFile();
                return;
            }
            else
            {
                std::cout << " success!" << std::endl;
            }
        }
        else
        {
            std::cout << " success!" << std::endl;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::shutdown ()
    {
        std::cout << "Shutting down the client" << std::endl;

        // put the client down nice an easy
        if (m_connected)
        {
            disconnect();
        }

        std::cout << "done!" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    void *Client::connect (void* v)
    {
        Client* client = (Client*)v;
        client->connect();
        pthread_exit(NULL);
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::connect ()
    {
        // make sure to initialize this flag as soon as this gets called otherwise
        // you might be waiting for a thread to die that never lived!
        m_commandLoopAliveFlag = false;

        // first of all, initialize the client
        server->connect();
        if (server->commPipe->valid())
        {
            m_connected=true;                     // really should error check
        }
        else
        {
            m_connectProgressN = m_connectProgressD = 1;
            return;
        }

        // get basic info from the server, including the number of data streams we can expect
        getBasicServerInfo();

        // now that you have the number of channels, lets set up some arrays

        // the stream channel vector
        streamData = new StreamChannel *[m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            streamData[chan] = new StreamChannel(chan,PRE_TRIGGER_LENGTH,PULSE_LENGTH);
        }

        // the local (GUI) copies of the status flags
        m_mixFlag          = new bool  [m_nDataStreams];
        m_mixInversionFlag = new bool  [m_nDataStreams];
        m_mixLevel         = new float [m_nDataStreams];
        m_decimateFlag     = new bool  [m_nDataStreams];
        m_decimateAvgFlag  = new bool  [m_nDataStreams];
        m_decimateLevel    = new unsigned short int [m_nDataStreams];

        // the FFT channels
        noiseRecordFFT = new FFTChannel *[m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            noiseRecordFFT[chan] = new FFTChannel (chan);
        }

        // the streamDataFlag
        m_streamDataFlag = new bool [m_nDataStreams];

        // the write pulese to file array
        m_writePulsesToFileFlag = new bool [m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            // all channels send triggered pulses to the file writer by default
            m_writePulsesToFileFlag[chan] = true;
        }

        // preform analysis flag array
        m_performAnalysisFlag = new bool [m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            m_performAnalysisFlag[chan] = false;  //off by default
        }

        // publically available PR array
        latestTriggeredPulse = new XPulseRec *[m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            latestTriggeredPulse[chan] = NULL;
        }
        latestTriggeredPulseLock = new XCALDAQLock[m_nDataStreams];

        // publically available Noise array
        latestNoisePulse = new XPulseRec *[m_nDataStreams];
        for (unsigned int chan=0;chan<m_nDataStreams;chan++)
        {
            latestNoisePulse[chan] = NULL;
        }
        latestNoisePulseLock = new XCALDAQLock[m_nDataStreams];

        // if the writer needs information and you haven't sent it, now
        // is the time to do it.
        if (m_XOUTType == LJH)
        {
            ((xdaq::LJHOutput*) m_XOUTWriter)->initXOUT(m_nDataStreams);
        }
        if (m_XOUTType == PLS)
        {
            bool even = false;                    // default is even channel are unsigned
            bool odd = false;                     // default is odd channel is unsigned
            if (m_serverType == NDFB)
            {
                // in ndfb, even channels are signed, odd are unsigned
                even = true;
                odd = false;
            }
            if (m_serverType == ROCKET)
            {
                //in rocket, all data is signed;
                even = true;
                odd = true;
            }
            ((xdaq::PLSOutput*) m_XOUTWriter)->init(even,odd,m_nDataStreams);
        }

        // now poll the server for specifics and schtick it into the arrays
        updateClientToServer ();

        //  at this point,  launch the command thread
        m_commandLoopDieFlag = false;
        pthread_create(&commandThread,NULL,runCommandLoop,(void *)this);

        // iterate the progress counter
        m_connectProgressN++;

        // load up the default trigger state
        load_trigger_state_from_file(trigger_config_filename);

        // NB: we do *not* load a default mixing state at connect as
        // that would change the state of the server
    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::connectProgress(void)
    {
        // we should check to make sure the process hasn't stalled

        if (m_connectProgressD == 0) return 0;
                                                  // make sure to return exactly 1.00000000000
        if (m_connectProgressN == m_connectProgressD) return 1.;
        return ((float) m_connectProgressN) / ((float) m_connectProgressD);
    }

    /////////////////////////////////////////////////////////////////////////////
    void *Client::disconnect (void* v)
    {
        Client* client = (Client*)v;
        client->disconnect();
        pthread_exit(NULL);
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::disconnect ()
    {

        // if it is still streaming, tell it to stop
        if (m_streaming)
        {
            stopStreaming();                      // blocks until streaming is dead
        }

        // stop all server interfaces
        std::cout << "Disconnecting from server"; fflush(0);
        if (server!=NULL) server->shutdownServer();
        std::cout << " done!" << std::endl;

        // make sure the run data loop is dead
        std::cout << "Stopping the Command Thread" << std::endl; fflush(0);
        m_commandLoopDieFlag = true;
        while (m_commandLoopAliveFlag)
        {
            //std::cout << "."; fflush(0);
            usleep (100000);
        }
        std::cout << "The Command Thread is dead." << std::endl; fflush(0);

        // indicate that we are not connected
        m_connected = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::startStreaming ()
    {
        std::cout << "Starting the data stream";
        server->set(XCD_DATAFLAG,XCD_ALLCHANNELS,true);
        m_streaming = true;
        std::cout << " done!" << std::endl;

        // launch the run data loop
        m_collectDataLoopDieFlag = false;
        pthread_create(&collectDataThread,NULL,runCollectDataLoop,(void *)this);
        m_analyzeDataLoopDieFlag = false;
        pthread_create(&analyzeDataThread,NULL,runAnalyzeDataLoop,(void *)this);

    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::stopStreaming ()
    {
        if (server->commPipe->valid())
        {
            std::cout << "Stopping the data stream";
            server->set(XCD_DATAFLAG,XCD_ALLCHANNELS,false);
            std::cout << " done!" << std::endl; fflush(0);
        }
        else
        {
            std::cout << "Server communication pipe is invalid.  Not sending \"Stop Stream\" command to server." << std::endl; fflush(0);
        }
        m_streaming = false;

        // set the die flags and wait for analyze data and collect datat loops to die
        std::cout << "Stopping the Collect Data Thread and Analyze Data Thread." << std::endl; fflush(0);
        m_analyzeDataLoopDieFlag = true;
        m_collectDataLoopDieFlag = true;
        while (m_analyzeDataLoopAliveFlag & m_collectDataLoopAliveFlag)
        {
            //std::cout << "."; fflush(0);
            usleep (100000);
        }
        std::cout << "Collect Data Thread and Analyze Data Thread are Dead!" << std::endl; fflush(0);

        // indicate that we have stopped streaming
        m_streaming = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::getBasicServerInfo(void)
    {

        std::cout << "Updating Client to Server" << std::endl;

        // get the server type and data source
        char name[5];
        name[4] = '\0';
        unsigned int *name_i = (unsigned int*) &name[0];
        server->get(XCD_SERVERTYPE, 0, name_i);

        /*
         printf ("responce: %s\n",name);
         int j;
         for (j=0;j<5;j++) {
         printf ("[%d] [%d] [%c]\n",j,(int)name[j],name[j]);
         }
        */

        m_dataSource = UNDEFINED_DATA;
        if (name[0] == 'R')
        {
            printf ("Connected to A Recorded Data Stream\n");
            m_dataSource = RECORDED;
        }
        else
        {
            printf ("Connected to A Live Data Stream\n");
            m_dataSource = LIVE;
        }
        if (m_dataSource == UNDEFINED_DATA)
        {
            printf ("Error in Data Type!  Exiting\n");
            exit (0);
        }

        m_serverType = UNDEFINED_SERVER;
        if (strcmp(&name[1],"RCK") == 0)
        {
            printf ("Connected to Rocket Server\n");
            m_serverType = ROCKET;
        }
        if (strcmp(&name[1],"NFB") == 0)
        {
            printf ("Connected to NDFB Server\n");
            m_serverType = NDFB;
        }
        if (strcmp(&name[1],"ITC") == 0)
        {
            printf ("Connected to IOTECH Server\n");
            m_serverType = IOTECH;
        }
        if (m_serverType == UNDEFINED_SERVER)
        {
            printf ("Error in Server Type!  Exiting\n");
            exit (0);
        }

        // get version
        unsigned int d;
        char *a;
        a = (char*) &d;
        server->get(XCD_VERSION,0,&d);
        char major;
        char minor;
        char r_minor;
        major=a[0];
        minor=a[1];
        r_minor=a[2];
        sprintf (m_versionString,"%d.%d.%d",major,minor,r_minor);

        // get the number of channels (data streams)
                                                  // the number of data streams
        server->get(XCD_CHANNELS,0,&m_nDataStreams);

        // calculate the number of pixels (i guess we could query for this)
        if (m_serverType == NDFB)
        {
            m_nMuxPixels = m_nDataStreams / 2;    // the number of actual pixels
        }
        else
        {
            m_nMuxPixels = m_nDataStreams;        // the number of actual pixels
        }

        // this is how we measure the progress of updating the client to the server
        // do this right away so that we can measure our progress
        m_connectProgressN = 0;
        m_connectProgressD = (7 * m_nDataStreams) + 6 ;

        // get the number of mux columns (or boards if you want to think of it that way)
        server->get(XCD_BOARDS, 0, &m_nMuxCols); m_connectProgressN++;

        // get the number of mux rows (or streams per board)
        server->get(XCD_STREAMS_PER_BOARD, 0, &m_nMuxRows); m_connectProgressN++;

        // calculate the number of mux rows
        if (m_nMuxPixels != (m_nMuxRows * m_nMuxCols))
        {
            printf ("the number of pixels expected given the number of data streams does not match the rows * cols\n");
            exit(1);
        }

        // get the number of samples (i.e. the number of samples that are co-added to form each sample)
        unsigned int nSamps;
        server->get(XCD_SAMPLES,0,&nSamps); m_connectProgressN++;

        // get the sample rate of the mux
        unsigned int sampRate;
        server->get(XCD_SAMPLERATE,0,&sampRate); m_connectProgressN++;

        // get the timebase from the server
        unsigned int timecodeBase_sec;            // the seconds part of the timecode base
        unsigned int timecodeBase_usec;           // the useconds part of the timecode base
        server->get(XCD_STARTTS,0,&timecodeBase_sec);   m_connectProgressN++;
        server->get(XCD_STARTTUS,0,&timecodeBase_usec); m_connectProgressN++;
        unsigned long long int timecodeBase =  1000000LL * timecodeBase_sec + timecodeBase_usec;

        // convert that timecode base into formatted text
        char ftime[256];
        time_t tim = (time_t)timecodeBase_sec;
        struct tm *ltime = gmtime(&tim);
        strftime(&ftime[0],255,"%d %b %Y, %H:%M:%S GMT",ltime);

        // display data
        std::cout << "Server information:" << std::endl;
        std::cout << "\tServer Version: " << m_versionString << std::endl;
        std::cout << "\tNumber of Mux Columns: " << m_nMuxCols << std::endl;
        std::cout << "\tNumber of Mux Rows: " << m_nMuxRows << std::endl;
        std::cout << "\tNumber of Mux Pixels: " << m_nMuxPixels << std::endl;
        std::cout << "\tNumber of Data Channels: " << m_nDataStreams << std::endl;
        std::cout << "\tCo-added Samples: " << nSamps << std::endl;
        std::cout << "\tMaster Sample Rate: " << sampRate << std::endl;
        std::cout << "\tMaster Timecode Base: " << timecodeBase << std::endl;
        std::cout << "\tMaster Timecode Base (formatted): "<< ftime << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////

    void Client::updateClientToServer(void)
    {
        printf ("\n");

        // get the decimation levels
        for (unsigned int i=0; i<m_nDataStreams; i++)
        {
            bool b;
            unsigned short int s;
            float f;

            // header
            std::cout << "Channel " << i << " Information: " << std::endl;

            // decimate flag
            server->get(XCD_DECIMATEFLAG,i,&b); m_connectProgressN++;
            m_decimateFlag[i] = b;
            std::cout << "\tDecimation Enabled: ";
            if (b) std::cout << "TRUE"; else std::cout << "FALSE";
            std::cout << std::endl;

            // decimate average flag
            server->get(XCD_DECIMATEAVGFLAG,i,&b);  m_connectProgressN++;
            m_decimateAvgFlag[i] = b;
            std::cout << "\tDecimation Averaging Enabled: ";
            if (b) std::cout << "TRUE"; else std::cout << "FALSE";
            std::cout << std::endl;

            // decimage level
            server->get(XCD_DECIMATELEVEL,i,&s); m_connectProgressN++;
            m_decimateLevel[i] = s;
            std::cout << "\tDecimation Level: " << s << std::endl;

            // mix flag
            server->get(XCD_MIXFLAG,i,&b);  m_connectProgressN++;
            m_mixFlag[i] = b;
            std::cout << "\tMixing Enabled: ";
            if (b) std::cout << "TRUE"; else std::cout << "FALSE";
            std::cout << std::endl;

            // min inversion
            server->get(XCD_MIXINVERSIONFLAG,i,&b); m_connectProgressN++;
            m_mixInversionFlag[i] = b;
            std::cout << "\tMixing Inversion Enabled: ";
            if (b) std::cout << "TRUE"; else std::cout << "FALSE";
            std::cout << std::endl;

            // mix level
            server->get(XCD_MIXLEVEL,i,&f); m_connectProgressN++;
            m_mixLevel[i] = f;
            std::cout << "\tMixing Level: " << f << std::endl;

            // get the active channels (if you just opened this client, these should all be "on" or streaming)
            server->get(XCD_ACTIVEFLAG,i,&b);   m_connectProgressN++;
            m_streamDataFlag[i]=b;
            std::cout << "\tData Stream Enabled at the Server: ";
            if (b) std::cout << "TRUE"; else std::cout << "FALSE";
            std::cout << std::endl;

            printf ("\n");

        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void* Client::runCommandLoop (void *v)
    {
        std::cout << "Running the command loop!" << std::endl;
        Client* client = (Client*) v;
        client->commandLoop();
        pthread_exit(NULL);
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::commandLoop(void)
    {

        try
        {
            m_commandLoopAliveFlag = true;
            while (!m_commandLoopDieFlag)
            {

                if (!m_connected)
                {
                    printf ("you can't get commands when you are not connected!!! \n");
                    continue;
                }

                // look at the validity of the connetion
                if (!server->commPipe->valid())
                {
                    printf ("The Command Thread has sensed a bad communciation pipe and is commanding a disconnect\n"); fflush(0);
                    // note, we are inside a thread loop, so we can't call a disconnect
                    // from inside this thread as disconnect blocks until all independent
                    // threads are dead.  since this thread is waiting for disconect and
                    // disconnect is waiting for this thread, it never dies.
                    // create a new thread and ask it to call the disconnect function
                    pthread_t temp;
                    pthread_create(&temp,NULL,disconnect,(void *)this);
                }

                /* register a select command that blocks until a client sends a command */
                // block until there is something to read on the comm pipe
                // note:  other threads can send commands as well which may or may not
                // get a responce from the server.  it is important that that thread get
                // the responce and not this thread so make sure to surround the following
                // read with comm lock.  the other thread will get the lock, send a command
                // and get a repsonce.  this reponce will trigger unblock the readReady
                // function.  however, before this thread can read the responce, it will
                // have to try to get the lock which the other thread already has.  the other
                // thread will read the responce and then release the lock.  this thread will
                // then grab the lock, and read the pipe (nothing will be there) and go
                // back to the readReady function.
                                                  // two second timeout
                int result = server->commPipe->readReady(1,0);

                if (result == 0)
                {
                    //printf ("command loop select timed out... \n");
                    continue;
                }

                // okay, the comm channel has some data on it (or had at least)
                // so reserver some space on the heap
                unsigned char rbuf[16];

                // reset the array
                for (int i=0;i<16;i++) rbuf[i]=0;

                // get the comm pipe lock
                server->commPipe->lock.getLock();

                // look for command packet from client
                int nGot = server->commPipe->read(&rbuf[0],16);

                if (nGot > 0)
                {
                                                  // make sure to get the whole packet
                    if (nGot < 16) server->commPipe->gread (&rbuf[nGot-1],16-nGot);
                    printf ("Got: _%s_\n",rbuf);
                }
                else
                {
                    //printf ("Got: nuffim:\n");
                }

                // release the lock
                server->commPipe->lock.releaseLock();

            }

            // this thread is dead so indicate it
            m_commandLoopAliveFlag = false;

        }
        catch  (const std::exception &x)
        {
            std::cout << x.what() << std::endl;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void* Client::runCollectDataLoop (void *v)
    {
        std::cout << "Running the Collect Data Loop!" << std::endl;
        Client* client = (Client*)v;
        client->collectDataLoop();
        pthread_exit(NULL);
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::collectDataLoop()
    {
        try
        {

            m_collectDataLoopAliveFlag = true;
            unsigned int bytes;
            struct timeval newDataTime;

            while (!m_collectDataLoopDieFlag)
            {

                if (!m_streaming)
                {
                    printf ("You can't collect data if you aren't streaming!\n");
                    continue;
                }

                // request data from the server be put into a deque
                bytes = server->getDataPacket(&m_rawDataPacketDeque, &m_rawDataPacketDequeLock);

                // get the time right after you return from gettting the data. this is used for data transfer rate measurement
                // note, take time b/f you ingest the record, b/c that throws off the measurement
                gettimeofday(&newDataTime,NULL);

                // update the data rate
                dataRateStruct dr;
                dr.t = newDataTime.tv_sec*1000000 + newDataTime.tv_usec;
                dr.bytes = bytes;
                m_dataRateDeque.push_back(dr);
                m_totalBytes += bytes;            // add that number to the running total

                // trim the data rate packet deque
                while ((int)m_dataRateDeque.size() > 100)
                {
                                                  // remove that number from the running total
                    m_totalBytes -= m_dataRateDeque.front().bytes;
                    m_dataRateDeque.pop_front();
                }

                // get delta b/w data transfers (note this has nothing to do with the deltat of the XPulseRec!!!)
                unsigned long long delta_t = m_dataRateDeque.back().t - m_dataRateDeque.front().t;
                if (delta_t == 0) continue;

                // calculate the data rate
                m_dataRate = ((float)m_totalBytes) / (((float)delta_t / 1000000.));

                //printf ("delta_Time: %llu Bytes: %u Rate: %f\n",delta_t,m_totalBytes,m_dataRate);

                // do a validity check on the pipes and disconnect if the pipe is invalid
                if (server->commPipe->valid() == false)
                {
                    server->dataPipe->valid(false);

                    printf ("The Collect Data Thread has sensed a bad pipe and is commanding a disconnect\n"); fflush(0);
                    // note, we are inside a thread loop, so we can't call a disconnect
                    // from inside this thread as disconnect blocks until all independent
                    // threads are dead.  since this thread is waiting for disconect and
                    // disconnect is waiting for this thread, it never dies.
                    // create a new thread and ask it to call the disconnect function
                    pthread_t temp;
                    pthread_create(&temp,NULL,disconnect,(void *)this);
                }
            }

            // this thread is dead
            m_collectDataLoopAliveFlag = false;

        }
        catch  (const std::exception &x)
        {
            std::cout << x.what() << std::endl;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void* Client::runAnalyzeDataLoop (void *v)
    {
        std::cout << "Running the Analyze Data Loop!" << std::endl;
        Client* client = (Client*)v;
        client->analyzeDataLoop();
        pthread_exit(NULL);
        return NULL;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::analyzeDataLoop()
    {
        try
        {
            m_analyzeDataLoopAliveFlag = true;

            // note, b/c the analyze data loop never sees the server, it doesn't
            // need to do the validity tests that the pipes that the other threads do.
            // those other threads will kill this thread if need be
            while (!m_analyzeDataLoopDieFlag)
            {

                if (!m_streaming)
                {
                    printf ("you can't analyze data if you aren't streaming!\n");
                    continue;
                }

                // populate the streams with raw data packets collected from the server pipe
                                                  // this can block
                if (fillStreams()) m_tiltFlag = true;

                // make sure the channel are all initialized
                checkChannelInit ();

                if (m_channelInit)
                {

                    // test for optimizeMixLeve mode
                    if (m_optimizeMixCalculateMedianMixFlag)
                    {
                        if (!m_doneCollectingOptimalMixData)
                        {
                            optimalMix_collectData();
                        }
                        else
                        {
                            optimalMix_analyzeData ();
                        }
                        trimStreams();
                        usleep (0);
                        continue;
                    }

                    // this is default behavior
                    scanStreamForTriggers();
                    distributeTriggers();
                    generateTriggerRecords();
                    sendTriggerRecords();
                    trimStreams();
                    usleep (0);
                }
                else
                {
                    fflush(0);
                    usleep (1000);
                }
            }

            // indicate this thread is dead
            m_analyzeDataLoopAliveFlag = false;

        }
        catch  (const std::exception &x)
        {
            std::cout << x.what() << std::endl;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::fillStreams()
    {

        // get the lock
        m_rawDataPacketDequeLock.getLock();

        // define the tilt flag
        bool tiltFlag = false;

        while (m_rawDataPacketDeque.size() != 0)
        {

            // get the pointer to the first raw data packet
            unsigned char* buf = m_rawDataPacketDeque.front();

            // pop the pointer off the deque.  buf is now the only pointer to the raw data packet
            m_rawDataPacketDeque.pop_front();

            // get the channel that this packet goes to
            unsigned int chan;
            memcpy (&chan,&buf[0],4);

            // convert channel from net to host
            chan = ntohl(chan);

            //printf ("processing address %x to channel %d\n", buf, chan);fflush(0);

            // send the buffer to its correct streamData member
            // note: the ingestDataPacket function will delete 'buf' from memory
            bool tilt = streamData[chan]->ingestDataPacket(buf);

            // set the tilt flag if necessary
            if (tilt) tiltFlag = true;

            // remember when i said we had to erase the raw data packet buffer? well, this is when we do it
            // printf ("deleting array located at address %x\n",buf);
            delete[] buf;
        }

        // release the lock
        m_rawDataPacketDequeLock.releaseLock();

        // return the tilt
        return tiltFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::checkChannelInit()
    {

        // check that all streams have some data
        bool result = true;
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++)
        {
            if (!m_streamDataFlag[chan]) continue;
            if (!streamData[chan]->init()) result = false;
        }

        if (result)
        {
            // set the channel flag
            m_channelInit = true;
        }

    }

    /////////////////////////////////////////////////////////////////////////////
    // start of calculate optimial mix stuff
    /////////////////////////////////////////////////////////////////////////////
    bool Client::optimalMix_start (unsigned int chan)
    {

        // quick error check
        if ((chan % 2) == 0)
        {
            printf ("you can't request this function to get a mix on an error.  odd numbers only!\n");
        }

        // print some dialog to the screen
        printf ("Initiating a mix optimization for ");
        if (chan == XCD_ALLCHANNELS)
        {
            printf ("all channels\n"); fflush (0);
        }
        else
        {
            printf ("channel %u\n",chan); fflush (0);
        }

        // if you have run the auto tune mix before, just wipe the variables
        if (m_optimizeMixPRArray != NULL)
        {

            printf ("Not running for the first time\n"); fflush(0);

            // loop through all channels
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                printf ("Deleting non coupled PR from channel %d deque\n",i);

                // delete all PR's that are in the deque
                std::deque<XPulseRec *>::iterator pr = m_optimizeMixPRArray[i].begin();
                for (unsigned int j=0; j<m_optimizeMixPRArray[i].size(); j++)
                {
                    delete (*pr);
                    pr++;
                }

                // clear the vector itself
                m_optimizeMixPRArray[i].clear();

                //printf ("\tshould be zero: %d\n",m_optimizeMixPRArray[i].size());
            }

            // check that the couple pr array is also empty
            if (m_optimizeMixCoupledPRArray != NULL)
            {

                // loop through all channels
                for (unsigned int i=0; i<m_nDataStreams; i++)
                {
                    printf ("Deleting coupled PR from channel %d deque\n",i);

                    // delete all PR's that are in the deque
                    std::deque<XPulseRec *>::iterator pr = m_optimizeMixCoupledPRArray[i].begin();
                    for (unsigned int j=0; j<m_optimizeMixCoupledPRArray[i].size(); j++)
                    {
                        delete (*pr);
                        pr++;
                    }

                    // clear the vector itself
                    m_optimizeMixCoupledPRArray[i].clear();
                }
            }

        }
        else
        {
            // set up the variables for the first time
            m_optimizeMixPRArray        = new std::deque <XPulseRec*>  [m_nDataStreams];
            m_optimizeMixCoupledPRArray = new std::deque <XPulseRec*>  [m_nDataStreams];
            m_optimizeMixChannelFlag    = new bool                     [m_nDataStreams];
            m_oldAutoFlag               = new bool                     [m_nDataStreams];
            m_oldLevelFlag              = new bool                     [m_nDataStreams];
            m_oldEdgeFlag               = new bool                     [m_nDataStreams];
            m_oldNoiseFlag              = new bool                     [m_nDataStreams];
            m_oldAutoValue              = new double                   [m_nDataStreams];
        }

        // loop through all channels and set the channel flag appropriately
        // this tells the algorithms which channels to optimize
        for (unsigned int i=1; i<m_nDataStreams; i+=2)
        {

            if ((chan == XCD_ALLCHANNELS) | (chan == i))
            {
                                                  // feedback channel you want to optimize
                m_optimizeMixChannelFlag[i]   = true;
                                                  // corresponding error signal
                m_optimizeMixChannelFlag[i-1] = true;
            }
            else
            {
                // user does not want to tune this feedback channel (or its error)
                m_optimizeMixChannelFlag[i]   = false;
                m_optimizeMixChannelFlag[i-1] = false;
            }
        }

        // run through all the channels and make sure that any channel
        // you are trying to optimize doesn't have the error already mixed in!
        for (unsigned int i=1; i<m_nDataStreams; i+=2)
        {
            if (m_optimizeMixChannelFlag[i])
            {
                if (streamData[i]->mixFlag())
                {
                    printf ("you are trying to optimize the mix with a feed back that already has the error mixed in!  Bad User.  Bad. \n");
                    return false;
                }
            }
        }

        // loop through all data streams and set the scan for trigger flags
        for (unsigned int chan = 0; chan < m_nDataStreams; chan ++ )
        {

            // ignore any channel that the user doesn't want to look at
            if (!m_optimizeMixChannelFlag[chan]) continue;

            // store the old trigger flags
            m_oldAutoFlag[chan]  = streamData[chan]->autoTriggerFlag();
            m_oldLevelFlag[chan] = streamData[chan]->levelTriggerFlag();
            m_oldEdgeFlag[chan]  = streamData[chan]->edgeTriggerFlag();
            m_oldNoiseFlag[chan] = streamData[chan]->noiseTriggerFlag();

            // store the old auto trigger value
            m_oldAutoValue[chan] = streamData[chan]->autoThreshold();

            // now turn off all other trigger flags
            streamData[chan]->levelTriggerFlag(false);
            streamData[chan]->edgeTriggerFlag (false);
            streamData[chan]->noiseTriggerFlag(false);

            // only for the feedback channels
            if (chan %2==1)
            {
                // turn on auto triggering for all feedback channels
                streamData[chan]->autoTriggerFlag(true);
                // make the time b/w auto triggers very low
                streamData[chan]->autoThreshold(1);
            }
            else
            {
                // what we  do with the even channels depends on whether we want them
                // coupled to the feedback signals or not
                if (m_optimizeMixCouplePairsFlag)
                {
                    // nothing to do here, all triggers are taken care of by the coupled feedback channels
                    streamData[chan]->autoTriggerFlag(false);
                }
                else
                {
                    // turn on auto triggering for all feedback channels
                    streamData[chan]->autoTriggerFlag(true);
                    // make the time b/w auto triggers very low
                    streamData[chan]->autoThreshold(1);
                }
            }
        }

        // remember the old couple trigger flags;
        m_oldCoupleOddToEvenFlag = coupleOddToEvenFlag();
        m_oldCoupleEvenToOddFlag = coupleEvenToOddFlag();

        // set the coupling flags (this depends on if the user wants the pairs coupled!)
        if (m_optimizeMixCouplePairsFlag)
        {
            // set the couple trigger (remember to turn this off!)
            coupleOddToEvenFlag(true);
        }
        else
        {
            coupleOddToEvenFlag(false);
            coupleEvenToOddFlag(false);
        }

        // set up the progress counters
        m_optimizeMixProgressN = 0;               // reset this
        unsigned int activeChannels = 0;
        for (unsigned int i=1; i<m_nDataStreams; i++)
        {
            if (m_optimizeMixChannelFlag[i]) activeChannels++;
        }
        m_optimizeMixProgressD = m_optimizeMixMaxRecords * activeChannels;
        if (m_optimizeMixRatioMethod == 1)
        {
            m_optimizeMixProgressD += (m_optimizeMixMaxRecords * activeChannels)/2;
            m_optimizeMixProgressD ++;            // add one extra step that comes after the median is done
        }
        else
        {
            m_optimizeMixProgressD += (m_optimizeMixMaxRecords * activeChannels)*MIX_NUM_ITERATIONS*MIX_NUM_DIVISIONS_PER_RANGE/2;
            m_optimizeMixProgressD ++;            // add one extra step that comes after the median is done
        }

        // set this flag false
        m_doneCollectingOptimalMixData = false;

        // this has to be last otherwise the analyze loop (which runs in another thread!) might
        // start banging on data b/f you have a chance to initialize it!
        m_optimizeMixCalculateMedianMixFlag = true;

        // print status to screen
        std::cout << "Collecting auto tune records...." << std::endl; fflush(0);

        // let the calling thread know that things went okay
        return true;
    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::optimalMix_progress(void)
    {
        //printf ("%u/%u\n",m_optimizeMixProgressN,m_optimizeMixProgressD);
                                                  // make sure to return exactly 1.00000000000
        if (m_optimizeMixProgressN>=m_optimizeMixProgressD) return 1.;
        return ((float) m_optimizeMixProgressN) / ((float) m_optimizeMixProgressD);
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::optimalMix_collectData ()
    {
        // collect trigger points from the feedback (odd) streams
        // note: this changes depending on whether the user wants to couple the streams!!!
        for (unsigned int chan = 1; chan < m_nDataStreams; chan += 2 )
        {
            Trigger trig;

            //printf ("looking at chan %d\n",chan); fflush(0);
            if (!m_streamDataFlag[chan]) continue;// ignore any channel that isn't set to stream

            // clear the pending just to be sure.... noise is cheap
            streamData[chan]->eraseAllPendingTrigs();

            // scan for a triggers
            streamData[chan]->scanStreamForTriggers();

            // move acquired trigs to the pending trigger deque
            trig = streamData[chan]->getNextTrig();
            while (trig.sampleCount != 0)
            {
                streamData[chan]->addPendingTrig(trig);

                // if necessary, also add this trigger to the error stream
                if (m_optimizeMixCouplePairsFlag)
                {
                                                  // ignore any channel that isn't set to stream
                    if (!m_streamDataFlag[chan-1]) continue;
                                                  // send that same trig to the odd channel
                    streamData[chan-1]->addPendingTrig(trig);
                }

                // get the next trigger in the queue
                trig = streamData[chan]->getNextTrig();
            }
        }

        // collect trigger points from the error (even) streams
        // note: this changes depending on whether the user wants to couple the streams!!!
        for (unsigned int chan = 0; chan < m_nDataStreams; chan += 2 )
        {
            Trigger trig;

            //	printf ("looking at chan %d\n",chan); fflush(0);
            if (!m_streamDataFlag[chan]) continue;// ignore any channel that isn't set to stream

            if (!m_optimizeMixCouplePairsFlag)
            {
                // clear the pending just to be sure.... noise is cheap
                streamData[chan]->eraseAllPendingTrigs();
            }

            // always scan for a triggers.  you have to do this even if you are couple trigger
            // so that data streams can be trimed of old data
            streamData[chan]->scanStreamForTriggers();

            if (!m_optimizeMixCouplePairsFlag)
            {
                // move acquired trigs to the pending trigger deque
                trig = streamData[chan]->getNextTrig();
                while (trig.sampleCount != 0)
                {
                    streamData[chan]->addPendingTrig(trig);

                    // get the next trigger in the queue
                    trig = streamData[chan]->getNextTrig();
                }
            }
        }

        // now that we have trigger points, lets generate some pulse records
        for (unsigned int chan = 0; chan < m_nDataStreams; chan ++ )
        {
            XPulseRec *PR;
            PR = streamData[chan]->getNextTriggeredPulseRecord();
            while (PR != NULL)
            {

                // note, you are going use the physical units to calc the mix, so make sure that that data is available!
                PR->calculatePhysSamples();

                // put this into the array
                m_optimizeMixPRArray[chan].push_back(PR);

                // iterate the progress counter if necessary
                if (!m_optimizeMixCouplePairsFlag)
                {
                    m_optimizeMixProgressN++;
                }

                // get the next trig
                PR = streamData[chan]->getNextTriggeredPulseRecord();
            }
        }

        // if necessary, make matching pairs b/w the odd and even channels and store them in the couple deque
        if (m_optimizeMixCouplePairsFlag)
        {
            for (unsigned int chan = 0; chan < m_nDataStreams; chan +=2 )
            {
                for (;;)
                {
                    XPulseRec *PReven = NULL;
                    XPulseRec *PRodd = NULL;
                    if (m_optimizeMixPRArray[chan].size() > 0)
                    {
                        PReven = m_optimizeMixPRArray[chan].front();
                        if (PReven == NULL)
                        {
                            printf ("something is wrong here: %d\n",chan);
                        }

                        if (m_optimizeMixPRArray[chan+1].size() > 0)
                        {
                            PRodd = m_optimizeMixPRArray[chan+1].front();
                            if (PRodd == NULL)
                            {
                                printf ("something is wrong here: %d\n", chan+1);
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }

                    unsigned long long int timecodeodd = PRodd->timecode();
                    unsigned long long int timecodeeven = PReven->timecode();

                    //printf ("#%d: %llu\t #%d: %llu\t\t",chan, timecodeeven, chan+1, timecodeodd);

                    if (timecodeeven == timecodeodd)
                    {
                        m_optimizeMixPRArray[chan].pop_front();
                        m_optimizeMixPRArray[chan+1].pop_front();

                        m_optimizeMixCoupledPRArray[chan].push_back(PReven);
                        m_optimizeMixCoupledPRArray[chan+1].push_back(PRodd);

                        // iterate the progress counter
                        m_optimizeMixProgressN += 2;

                        printf ("channel %d & %d received pair #%d triggered at %llu\n",chan, chan+1, m_optimizeMixCoupledPRArray[chan].size(), timecodeodd);

                    }
                    else
                    {
                        if (timecodeeven < timecodeodd)
                        {
                            m_optimizeMixPRArray[chan].pop_front();
                            delete (PReven);
                        }
                        else
                        {
                            m_optimizeMixPRArray[chan+1].pop_front();
                            delete (PRodd);
                        }
                    }
                }
            }
        }

        // now check to see if we are have enough pairs to do analysis
        bool done = true;
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++)
        {
            // ignore any channel that isn't set to stream
            if (!m_streamDataFlag[chan]) continue;

            // ignore any channel that the user doesn't want to look at
            if (!m_optimizeMixChannelFlag[chan]) continue;

            // check to see if you have enough records (depends on couple flag)
            if (m_optimizeMixCouplePairsFlag)
            {
                if (m_optimizeMixCoupledPRArray[chan].size() < m_optimizeMixMaxRecords)
                {
                    //std::cout << "couple failed chan "<<chan <<" size: "<<m_optimizeMixCoupledPRArray[chan].size()<< std::endl;
                    done = false;
                }
            }
            else
            {
                if (m_optimizeMixPRArray[chan].size() < m_optimizeMixMaxRecords)
                {
                    //std::cout << "non coupled failed chan "<<chan <<" size: "<<m_optimizeMixPRArray[chan].size()<< std::endl;
                    done = false;
                }
            }
        }

        // set the flag if necessary
        if (done)
        {
            std::cout << "...done collecting auto tune info" << std::endl; fflush(0);
            m_doneCollectingOptimalMixData = true;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::optimalMix_analyzeData ()
    {

        // now calc the best mix ratio for each pair
        for (unsigned int chan = 1; chan < m_nDataStreams; chan+=2)
        {
            unsigned int chan_feedback = chan;
            unsigned int chan_error = chan - 1;

            // ignore this if the feedback channel isn't set to stream
            if (!m_streamDataFlag[chan_feedback])
            {
                std::cout << "Cannot calculate mix level b/c channel " << chan_feedback << " is not set to stream!" << std::endl;
                continue;
            }

            // ignore this if the error channel isn't set to stream
            if (!m_streamDataFlag[chan_error])
            {
                std::cout << "Cannot calculate mix level b/c channel " << chan_error
                    << " is not set to stream!" << std::endl;
                continue;
            }

            // ignore any channel that the user doesn't want to look at
            if (!m_optimizeMixChannelFlag[chan]) continue;

            // calculate the best mix
            float f;
            if (m_optimizeMixCouplePairsFlag)
            {
                f = optimalMix_calculateMedianMix (m_optimizeMixCoupledPRArray[chan_error],m_optimizeMixCoupledPRArray[chan_feedback]);
            }
            else
            {
                f = optimalMix_calculateMedianMix (m_optimizeMixPRArray[chan_error],m_optimizeMixPRArray[chan_feedback]);
            }

            std::cout << "Best mix for error (channel "<<chan_error<<") and signal (channel "<<chan_feedback<<") is: "<< f << std::endl;
            mixLevel(chan_feedback,f);
            m_optimizeMixProgressN++;             // this is the last step

        }

        // should probably delete any PR's that are left over in the deques

        // erase any left over triggers that may be in the pending trig queue
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++)
        {

            // ignore any channel that the user doesn't want to look at
            if (!m_optimizeMixChannelFlag[chan]) continue;

            streamData[chan]->eraseAllPendingTrigs();
        }

        // all done, so set things back to how they were
        m_optimizeMixCalculateMedianMixFlag = false;
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++)
        {

            // ignore any channel that the user doesn't want to look at
            if (!m_optimizeMixChannelFlag[chan]) continue;

            // set the trigger back the way they were
            streamData[chan]->autoThreshold    ( m_oldAutoValue[chan] );
            streamData[chan]->autoTriggerFlag  ( m_oldAutoFlag[chan]  );
            streamData[chan]->levelTriggerFlag ( m_oldLevelFlag[chan] );
            streamData[chan]->edgeTriggerFlag  ( m_oldEdgeFlag[chan]  );
            streamData[chan]->noiseTriggerFlag ( m_oldNoiseFlag[chan] );
        }

        coupleOddToEvenFlag (m_oldCoupleOddToEvenFlag);
        coupleEvenToOddFlag (m_oldCoupleEvenToOddFlag);

    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::optimalMix_calculateMedianMix (std::deque<XPulseRec *> error, std::deque<XPulseRec *> feedback)
    {

        float ratio[m_optimizeMixMaxRecords];

        // setup the iterators
        std::deque<XPulseRec*>::iterator PR_feedback = feedback.begin();
        std::deque<XPulseRec*>::iterator PR_error = error.begin();

        // get the best ratio for each pair
        for (unsigned int i=0; i<m_optimizeMixMaxRecords; i++, PR_feedback++, PR_error++)
        {
            if (m_optimizeMixRatioMethod == 0)
            {
                ratio[i] = optimalMix_lowestSTDMix(*PR_error, *PR_feedback);
            }
            if (m_optimizeMixRatioMethod == 1)
            {
                ratio[i] = optimalMix_RMSRatioMix (*PR_error, *PR_feedback);
            }
        }

        // take a median filter on this data i guess
        float ratio_median=0;
        ratio_median = median (ratio, m_optimizeMixMaxRecords);
        std::cout << "Median ratio is: " << ratio_median << std::endl;

        // return the median ratio
        return ratio_median;
    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::optimalMix_RMSRatioMix(XPulseRec *even, XPulseRec *odd)
    {

        // get the rms values for each
        double errRMS = even->rmsAC();
        double sigRMS = odd->rmsAC();

        // check for a 0 in the denominator
        if (errRMS == 0)
        {
            std::cout << "Error RMS is zero.  Mix calculation fails for this pair" << std::endl;
            return 0.;
        }

        // iterate the progress counter
        m_optimizeMixProgressN ++;

        double ratio  = sigRMS / errRMS;

        printf ("\tBest pair ratio is %lf\n",ratio);
        return ratio;

    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::optimalMix_lowestSTDMix(XPulseRec *PReven, XPulseRec *PRodd)
    {

        double       *x0 = NULL;
        double       *y0 = NULL;
        unsigned int  n0 = 0;
        double       *x1 = NULL;
        double       *y1 = NULL;
        unsigned int  n1 = 0;

        // request some data
        PReven->requestDataCopy(&x0,&y0,&n0);
        PRodd->requestDataCopy(&x1,&y1,&n1);

        // check for a mismatch
        if (n0 != n1)
        {
            printf ("mix channel mis match!!\n");
            return 0.;
        }

        printf ("working on %d %d %llu %llu\n",PReven->channel(),PRodd->channel(), PReven->timecode(),PRodd->timecode());

        int    n             = n0;                // number of samples in the record
        double y[n]          ;                    // temp array that will store the mixed record
        double minStd        = DBL_MAX;           // the minimum std ratio
        double minStdRatio   = 0;                 // the ratio that corresponds to this minumum std

                                                  // beginning of the seach range
        double rmin          = MIX_MIN_SEARCH_RANGE;
                                                  // end of the search range
        double rmax          = MIX_MAX_SEARCH_RANGE;
        double divSize       ;                    // size of each division
        double newRange      ;                    // the size of the new range (the old range / zoom factor)

        if (m_mixInversionFlag[PRodd->channel()])
        {
            printf ("subtracting error\n");
        }
        else
        {
            printf ("adding error\n");
        }

        // loop through iterations
        for (int i = 0; i < MIX_NUM_ITERATIONS; i++)
        {

            divSize = ((1.) / ((double) MIX_NUM_DIVISIONS_PER_RANGE)) * (rmax - rmin);
                                                  // once a min is found the new range is set to be the min +/- the error
            newRange = (rmax - rmin) / (MIX_ZOOM_FACTOR);

            //printf ("seeking %e b/w [%e,%e] in %e chunks\tnew range%e\n",(rmax-rmin),rmin, rmax, divSize,zoomFactor);

            printf ("\tChannel: %d+%d",PReven->channel(), PRodd->channel());
            printf ("\tSeek min: %e",rmin);
            printf ("\tSeek max: %e",rmax);
            printf ("\tSeek range: %e",rmax-rmin);
            printf ("\tDiv Size: %e",divSize);
            printf ("\tNew Seek range: %e",newRange);

            // always reset the min
            minStd        = DBL_MAX;

            for (int j = 0; j <= MIX_NUM_DIVISIONS_PER_RANGE; j++)
            {
                double r = rmin + j * divSize;

                // contruct the new array
                if (m_mixInversionFlag[PRodd->channel()])
                {
                    for (int k = 0; k < n; k++) { y[k] = y1[k] - r*y0[k];   }
                }
                else
                {
                    for (int k = 0; k < n; k++) { y[k] = y1[k] + r*y0[k];   }
                }

                // get the standard deviation of this new array
                double std = XPulseRec::standardDeviation(y,n);

                //printf ("Channel: %d&%d Factor: %e\tStd: %e\n",PReven->channel(), PRodd->channel(), r, std);fflush(0);

                if (std < minStd)
                {
                    minStd = std;
                    minStdRatio = r;
                    rmin = r - newRange/2.;
                    rmax = r + newRange/2.;
                    if (rmin < MIX_MIN_SEARCH_RANGE) rmin = MIX_MIN_SEARCH_RANGE;
                    if (rmax > MIX_MAX_SEARCH_RANGE) rmax = MIX_MAX_SEARCH_RANGE;
                }

                // iterate the progress counter
                m_optimizeMixProgressN ++;

            }
            printf ("\tMin std: %e",minStd);
            printf ("\tMin std ratio: %e",minStdRatio);
            printf ("\n");

            //printf ("\tChannel: %d&%d Factor: %e\tStd: %e\tError +/-%e\n",PReven->channel(), PRodd->channel(), minStdRatio, minStd,divSize);
        }

        printf ("Channel: %d&%d Factor: %e\tStd: %e\tError +/-%e\n",PReven->channel(), PRodd->channel(), minStdRatio, minStd,divSize);

        // clean up the memory
        delete (x0);
        delete (x1);
        delete (y0);
        delete (y1);

        // return the best ratio for this pair
        return minStdRatio;
    }

    /////////////////////////////////////////////////////////////////////////////
    // end of NIST mix stuff
    /////////////////////////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////////////////////////
    // start of standard running mode
    /////////////////////////////////////////////////////////////////////////////
    void Client::scanStreamForTriggers()
    {
        //   give each stream a chance to get scan for trigs
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
        {
            if (!m_streamDataFlag[chan]) continue;
            streamData[chan]->scanStreamForTriggers();
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::distributeTriggers()
    {
        // find all sources

        for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
        {
            if (!m_streamDataFlag[chan]) continue;
            Trigger trig;

            // get the first trigger
            trig = streamData[chan]->getNextTrig();
            while (trig.sampleCount != 0)
            {

                // first of all, make sure you pend it in your own deque!
                streamData[chan]->addPendingTrig(trig);

                // if you happen to be a group source...
                if (streamData[chan]->groupTrigSource())
                {

                    // distribute to all receivers
                    for (unsigned int receiver = 0; receiver < m_nDataStreams; receiver++ )
                    {
                                                  // always ignore yourself, you already have this trig
                        if (receiver == chan) continue;
                        if (streamData[receiver]->groupTrigReceiver())
                        {
                            std::cout << "Group Trigger Sending " << trig.sampleCount << " from " << chan << " to " << receiver << std::endl;
                            streamData[receiver]->addPendingTrig(trig);
                        }
                    }
                }

                // if you are a signal and the error couple triggering is enabled,
                // distribute scanned trigs to corresponding error streams
                if (m_coupleOddToEvenFlag)
                {
                    if ((chan % 2) == 1)
                    {
                        //std::cout << "Couple Trigger Sending " << trig.sampleCount << " from " << chan << " to " << chan - 1 << std::endl;
                        streamData[chan - 1]->addPendingTrig(trig);
                    }
                }

                // if you are a error and the signal couple triggering is enabled,
                // distribute scanned trigs to corresponding error streams
                if (m_coupleEvenToOddFlag)
                {
                    if ((chan % 2) == 0)
                    {
                        //std::cout << "Couple Trigger Sending " << trig.sampleCount << " from " << chan << " to " << chan + 1 << std::endl;
                        streamData[chan + 1]->addPendingTrig(trig);
                    }
                }

                // get more triggers point until they are all gone
                trig = streamData[chan]->getNextTrig();

            }
        }                                         // end of looping through channels

    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::generateTriggerRecords()
    {
        XPulseRec *PR=NULL;

        // loop through all streams and get regular triggers
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
        {
            if (!m_streamDataFlag[chan]) continue;

            // get all pending data PR's from the channel
            PR = streamData[chan]->getNextTriggeredPulseRecord();
            while (PR!=NULL)
            {
                m_trigPR.push_back(PR);
                PR = streamData[chan]->getNextTriggeredPulseRecord();
            }
        }

        // loop through all streams and get noise triggers
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
        {
            if (!m_streamDataFlag[chan]) continue;
            // get all pending data PR's from the channel
            PR = streamData[chan]->getNextNoisePulseRecord();
            while (PR!=NULL)
            {
                m_noisePR.push_back(PR);
                PR = streamData[chan]->getNextNoisePulseRecord();
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::sendTriggerRecords()
    {

        /* DISTRIBUTE PULSE RECORDS */

        // get an iterator to the triggered pulse records
        std::vector<XPulseRec *>::iterator PRt = m_trigPR.begin();

        // loop through distribute regular triggers
        for(unsigned int i=0;i < m_trigPR.size();i++)
        {
            XPulseRec *PR = *PRt;

            // these pulses don't necessarily have the physical quantites calculated already
            if (!PR->calculatePhysFlag())
            {
                PR->calculatePhysSamples();
            }

            // send this rec off to any file writers
            if (m_XOUTWriter->writingFlag())
            {

                // make sure this channel wants to send to the writer
                if (writePulsesToFileFlag(PR->channel()))
                {

                    // auto trigger
                    if ( (PR->triggerType() == AUTO) & (m_writeAutoToFileFlag) )
                    {
                        m_XOUTWriter->writeRecordXOUT(*PR);
                    }

                    // level trigger
                    if ( (PR->triggerType() == LEVEL) & (m_writeLevelToFileFlag) )
                    {
                        m_XOUTWriter->writeRecordXOUT(*PR);
                    }

                    // edge trigger
                    if ( (PR->triggerType() == EDGE) & (m_writeEdgeToFileFlag) )
                    {
                        m_XOUTWriter->writeRecordXOUT(*PR);
                    }

                    // noise trigger is taken care of below
                }
            }

            // if the user wants, perfrom pulse analysis
            if (m_performAnalysisFlag[PR->channel()])
            {
                // analysize the pulse for height and trigger time
                PR->analyzePulse();

                // get that info
                double pulseHeight               = PR->pulseHeight();
                double pulseBaseline             = PR->baseline();

                // a note on the time keeping.  there are two times the timecodebase and the timecode
                // the base is uS from epoch to beginning of when server is started
                // the time code is uS from base time to trigger time
                // since this data is for display purposes only, i see no reason why just
                // the delta time isn't sufficient.
                                                  // just the delta
                unsigned long long int pulseTime      = PR->timecode();
                                                  // convert back to seconds
                double                 pulseTimeFloat = ((double) pulseTime) / 1000000.;

                // send this information to the correct stream channel object
                // we will use this at some later point to plot the pulse height vs time
                streamData[PR->channel()]->addPulseData (pulseHeight, pulseBaseline, pulseTimeFloat);

            }

            // try to get the lock for this slot in the latest PR array
            if (latestTriggeredPulseLock[PR->channel()].tryLock()==0)
            {

                // if there is an old pulse at this location, delete it b/c it is no longer the latest PR for that channel
                if (latestTriggeredPulse[PR->channel()] != NULL)
                {
                    delete (latestTriggeredPulse[PR->channel()]);
                }

                // assign the new PR in it's place
                                                  // this PR will be deleted from memory when it is replaced
                latestTriggeredPulse[PR->channel()] = PR;

                // release the lock
                latestTriggeredPulseLock[PR->channel()].releaseLock();

            }
            else
            {
                printf ("failed to get the PR lock for channel %u\n",PR->channel()); fflush(0);
                // we can't stick this PR into the holder for "latest PR".  let's just kill it right now
                delete (PR);
            }

            // iterate the pr
            PRt++;
        }                                         // end of triggered pulse loop

        // remove these PR's from the m_trigPR vector
        // note, this does not erase the PR's from memory!  that needs to be done to each PR individually!!!
        // this only removes the pointers from the trigger PR deque
        m_trigPR.erase(m_trigPR.begin(),m_trigPR.end());

        /* DISTRIBUTE NOISE RECORDS */
        // get an iterator to the noise pulse records
        std::vector<XPulseRec *>::iterator PRn = m_noisePR.begin();

        // loop through and distribute noise pulses
        for(unsigned int i=0;i < m_noisePR.size();i++, PRn++)
        {
            XPulseRec *PR = *PRn;

            if (noiseRecordFFT[PR->channel()]->fftFlag())
            {

                // make sure that every one of these pulses has the physical information calculated
                if (!PR->calculatePhysFlag())
                {
                    PR->calculatePhysSamples();
                }

                // send this rec off to any file writers
                if (m_XOUTWriter->writingFlag())
                {

                    // make sure this channel wants to send to the writer
                    if (writePulsesToFileFlag(PR->channel()))
                    {

                        // noise trigger
                        if ( (PR->triggerType() == NOISE) & (m_writeNoiseToFileFlag) )
                        {
                            m_XOUTWriter->writeRecordXOUT(*PR);
                        }
                    }
                }

                // request some data
                double *x=NULL;
                double *y=NULL;
                unsigned int n = 0;
                PR->requestDataCopy(&x,&y,&n);
                double sampleRate = 1/PR->deltat();

                // send this rec to the proper FFT channel
                noiseRecordFFT[PR->channel()]->ingestData(y,n,sampleRate);

                // remember, we requested the arrays in heap memory, so make sure to erase them!
                delete (x);
                delete (y);

                // write FFT data to file
                if (m_XOUTType == FFT)
                {
                    // lock the fft
                    noiseRecordFFT[PR->channel()]->fftLock.getLock();

                    // get the data from the fft
                    double* freq = noiseRecordFFT[PR->channel()]->dataX();
                    double* power = noiseRecordFFT[PR->channel()]->dataY();
                    unsigned int number = noiseRecordFFT[PR->channel()]->dataN();

                    // send it to the file writer
                    ((xdaq::FFTOutput*)m_XOUTWriter)->writeRecordXOUT(PR->channel(),freq,power,number);

                    // unlock the fft
                    noiseRecordFFT[PR->channel()]->fftLock.releaseLock();

                }
            }

            // try to get the vector lock
            if (latestNoisePulseLock[PR->channel()].tryLock()==0 )
            {
                //printf ("overwriting the latest noise PR for channel %u\n",PR->channel()); fflush(0);

                // if there is an old pulse at this location
                if (latestNoisePulse[PR->channel()] != NULL)
                {
                    // delete the old PR instance as it is no longer the latest PR for that channel
                    delete (latestNoisePulse[PR->channel()]);
                }

                // assign the new PR in it's place
                latestNoisePulse[PR->channel()] = PR;

                // release the lock
                latestNoisePulseLock[PR->channel()].releaseLock();

            }
            else
            {
                printf ("failed to get the noise PR lock for channel %u\n",PR->channel()); fflush(0);
            }

        }                                         // end of noise records loop

        // remove these PR's from the m_noisePR vector
        m_noisePR.erase(m_noisePR.begin(),m_noisePR.end());

    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::trimStreams()
    {
        // there are three different trim algorithms, once for standard, one for
        // coupling the feeback with the error, and one for coupling the error with
        // the feeback

        // #1 couple from odd to even i.e. triggers on an odd stream will trigger on the corresponding even channel
        if (m_coupleOddToEvenFlag)
        {

            /* 
             here we deal with the case where all triggers present on even streams
             will be passed to their corresponding odd streams.  we will also have to
             deal with the fact that any of these streams can be a group trigger source
             or reciever.

             there are 4 cases to consider
             1) even numbered streams that are not group trigger receivers
             2) odd numbered streams that are not group trigger receivers
             3) even numbered streams that are group trigger receivers
             4) odd numbered streams that are group trigger receivers

            to deal with #3 and #4 we have to scan all group trigger SOURCES and determine
            the proper trim point from them.  we cannot throw away data in a receiver
            if we might still get a trigger from a source.

            another important thing to note:  consider the following
            stream #0 and stream #1 are couple triggered from even to odd
            so a trigger on #0 produces a trigger on #1

            let us now consider the case where stream #1 is a also group trigger source
            and stream #99 is a group trigger receiver
            so a trigger on #1 produces a trigger on #99

            b/c of the couple trigger, a trigger on #0 will still produce a trigger on #1
            but it will _not_, in turn, produce a trigger on #99
            as this could lead to recursion.

            in other words, triggers are only passed on one time and not more.
            it is important that trimming of such streams reflect this fact
            */

            // first, loop through all streams that are group trigger SOURCES and find the earliest group trigger point
            unsigned long long int oldestGroupTriggerTrimPoint = ULLONG_MAX;
            for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
            {
                if (streamData[chan]->groupTrigSource())
                {
                    if (m_streamDataFlag[chan])
                    {
                        // get the oldest point we should erase to
                        unsigned long long int oldestIndividualPoint;
                        if (streamData[chan]->groupTrigReceiver())
                        {
                            // this source is a receiver of group tiggers.  play it safe and use it's oldest point
                            oldestIndividualPoint = streamData[chan]->oldestTrimPoint();
                        }
                        else
                        {
                            // this source is NOT a receiver of group triggers.  use a more recent point
                            oldestIndividualPoint = streamData[chan]->pulseRec()->sampleCount() - streamData[chan]->pulseRec()->length();
                        }

                        // keep track of which one is the oldest point of all the group trigger sources
                        if (oldestIndividualPoint < oldestGroupTriggerTrimPoint) oldestGroupTriggerTrimPoint = oldestIndividualPoint;
                    }
                    else
                    {
                        printf ("You have channel %d as a group trigger source, but it is not set to stream\n",chan);
                    }
                }
            }

            // give each ODD stream that is not a RECEIVER a chance to get rid of old data
            for (unsigned int chan = 1; chan < m_nDataStreams; chan += 2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // use default trimming
                    streamData[chan]->trimStream();
                }
            }

            // give each EVEN stream that is not a RECEIVER a chance to get rid of old data
            // Note, this EVEN stream get's its trim point from the ODD channel!!!
            for (unsigned int chan = 0; chan < m_nDataStreams; chan += 2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // check that the coupled stream is getting data
                    if (!m_streamDataFlag[chan+1])
                    {
                        printf ("You are telling channel %d to trigger on channel %d's triggers, but channel %d is not streaming\n",chan,chan+1,chan+1);
                                                  // use default trimming
                        streamData[chan]->trimStream();
                        continue;
                    }

                    // use the coupled stream (i.e. the odd stream) to calculate the trim point for this channel
                    unsigned long long int oldestCoupleTrimPoint = streamData[chan+1]->pulseRec()->sampleCount() - streamData[chan+1]->pulseRec()->length();
                    streamData[chan]->trimStream(oldestCoupleTrimPoint);
                }
            }

            // give each ODD stream that is a RECEIVER a chance to get rid of old data
            // note, we must know the the oldest group trigger point calculated above
            for (unsigned int chan = 1; chan < m_nDataStreams; chan+=2 )
            {
                if (streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (m_streamDataFlag[chan])
                    {

                        // trim using thge oldest group trigger trim point
                        streamData[chan]->trimStream(oldestGroupTriggerTrimPoint);
                    }

                }
            }

            // give each EVEN stream that is a RECEIVER a chance to get rid of old data
            // note, we must know the the oldest group trigger trim point calculated above and
            // the oldest coupled trim point calculated from its coupled stream
            for (unsigned int chan = 0; chan < m_nDataStreams; chan+=2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // declare the couple trim point
                    unsigned long long int oldestCoupleTrimPoint = ULLONG_MAX;

                    // check that the coupled stream is getting data
                    if (!m_streamDataFlag[chan+1])
                    {
                        printf ("You are telling channel %d to trigger on channel %d's triggers, but channel %d is not streaming\n",chan,chan+1,chan+1);
                    }
                    else
                    {
                        // calculate the oldest trim point from it's coupled channel
                        oldestCoupleTrimPoint = streamData[chan+1]->pulseRec()->sampleCount() - streamData[chan+1]->pulseRec()->length();
                    }

                    // use the older of either the coupled trim point or the group trim point
                    if (oldestCoupleTrimPoint < oldestGroupTriggerTrimPoint)
                    {
                        streamData[chan]->trimStream(oldestCoupleTrimPoint);
                    }
                    else
                    {
                        streamData[chan]->trimStream(oldestGroupTriggerTrimPoint);
                    }
                }
            }

            return;
        }                                         // end of couple Odd to Even

        // #2 - the couple from even to odd flag
        if (m_coupleEvenToOddFlag)
        {

            /* 
             here we deal with the case where all triggers present on even streams
             will be passed to their corresponding odd streams.  we will also have to
             deal with the fact that any of these streams can be a group trigger source
             or reciever.

             there are 4 cases to consider
             1) even numbered streams that are not group trigger receivers
             2) odd numbered streams that are not group trigger receivers
             3) even numbered streams that are group trigger receivers
             4) odd numbered streams that are group trigger receivers

            to deal with #3 and #4 we have to scan all group trigger SOURCES and determine
            the proper trim point from them.  we cannot throw away data in a receiver
            if we might still get a trigger from a source.

            another important thing to note:  consider the following
            stream #0 and stream #1 are couple triggered from even to odd
            so a trigger on #0 produces a trigger on #1

            let us now consider the case where stream #1 is a also group trigger source
            and stream #99 is a group trigger receiver
            so a trigger on #1 produces a trigger on #99

            b/c of the couple trigger, a trigger on #0 will still produce a trigger on #1
            but it will _not_, in turn, produce a trigger on #99
            as this could lead to recursion.

            in other words, triggers are only passed on one time and not more.
            it is important that trimming of such streams reflect this fact
            */

            // first, loop through all streams that are group trigger SOURCES and find the earliest group trigger point
            unsigned long long int oldestGroupTriggerTrimPoint = ULLONG_MAX;
            for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
            {
                if (streamData[chan]->groupTrigSource())
                {
                    if (m_streamDataFlag[chan])
                    {
                        // get the oldest point we should erase to
                        unsigned long long int oldestIndividualPoint;
                        if (streamData[chan]->groupTrigReceiver())
                        {
                            // this source is a receiver of group tiggers.  play it safe and use it's oldest point
                            oldestIndividualPoint = streamData[chan]->oldestTrimPoint();
                        }
                        else
                        {
                            // this source is NOT a receiver of group triggers.  use a more recent point
                            oldestIndividualPoint = streamData[chan]->pulseRec()->sampleCount() - streamData[chan]->pulseRec()->length();
                        }

                        // keep track of which one is the oldest point of all the group trigger sources
                        if (oldestIndividualPoint < oldestGroupTriggerTrimPoint) oldestGroupTriggerTrimPoint = oldestIndividualPoint;
                    }
                    else
                    {
                        printf ("You have channel %d as a group trigger source, but it is not set to stream\n",chan);
                    }
                }
            }

            // give each EVEN stream that is not a RECEIVER a chance to get rid of old data
            for (unsigned int chan = 0; chan < m_nDataStreams; chan += 2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // use default trimming
                    streamData[chan]->trimStream();
                }
            }

            // give each ODD stream that is not a RECEIVER a chance to get rid of old data
            // Note, this ODD stream get's its trim point from the EVEN channel!!!
            for (unsigned int chan = 1; chan < m_nDataStreams; chan += 2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // check that the coupled stream is getting data
                    if (!m_streamDataFlag[chan-1])
                    {
                        printf ("You are telling channel %d to trigger on channel %d's triggers, but channel %d is not streaming\n",chan ,chan-1,chan-1);
                                                  // use default trimming
                        streamData[chan]->trimStream();
                        continue;
                    }

                    // use the coupled stream (i.e. the even stream) to calculate the trim point for this channel
                    unsigned long long int oldestCoupleTrimPoint = streamData[chan-1]->pulseRec()->sampleCount() - streamData[chan-1]->pulseRec()->length();
                    streamData[chan]->trimStream(oldestCoupleTrimPoint);
                }
            }

            // give each EVEN stream that is a RECEIVER a chance to get rid of old data
            // note, we must know the the oldest group trigger point calculated above
            for (unsigned int chan = 0; chan < m_nDataStreams; chan+=2 )
            {
                if (streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (m_streamDataFlag[chan])
                    {

                        // trim using thge oldest group trigger trim point
                        streamData[chan]->trimStream(oldestGroupTriggerTrimPoint);
                    }

                }
            }

            // give each ODD stream that is a RECEIVER a chance to get rid of old data
            // note, we must know the the oldest group trigger trim point calculated above and
            // the oldest coupled trim point calculated from its coupled stream
            for (unsigned int chan = 1; chan < m_nDataStreams; chan+=2 )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {

                    // check that the stream is getting data
                    if (!m_streamDataFlag[chan]) continue;

                    // declare the couple trim point
                    unsigned long long int oldestCoupleTrimPoint = ULLONG_MAX;

                    // check that the coupled stream is getting data
                    if (!m_streamDataFlag[chan-1])
                    {
                        printf ("You are telling channel %d to trigger on channel %d's triggers, but channel %d is not streaming\n",chan,chan-1,chan-1);
                    }
                    else
                    {
                        // calculate the oldest trim point from it's coupled channel
                        oldestCoupleTrimPoint = streamData[chan-1]->pulseRec()->sampleCount() - streamData[chan-1]->pulseRec()->length();
                    }

                    // use the older of either the coupled trim point or the group trim point
                    if (oldestCoupleTrimPoint < oldestGroupTriggerTrimPoint)
                    {
                        streamData[chan]->trimStream(oldestCoupleTrimPoint);
                    }
                    else
                    {
                        streamData[chan]->trimStream(oldestGroupTriggerTrimPoint);
                    }
                }
            }

            return;
        }                                         // end of couple Even to Odd

        // none of the coupling flags are on, so just use the standard trimming
        {
            // loop through all streams that are SOURCES and find the earliest group trigger point
            unsigned long long int oldestGroupTriggerPoint = ULLONG_MAX;
            for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
            {
                if (streamData[chan]->groupTrigSource())
                {
                    if (!m_streamDataFlag[chan]) continue;
                    unsigned long long int oldestIndividualPoint;

                    // define oldestIndividualPoint depending upon whether or not a source is also a receiver
                    if (streamData[chan]->groupTrigReceiver())
                    {
                        oldestIndividualPoint = streamData[chan]->oldestTrimPoint();
                    }
                    else
                    {
                        oldestIndividualPoint = streamData[chan]->pulseRec()->sampleCount() - streamData[chan]->pulseRec()->length();
                    }

                    // store the oldest point of all of the source streams
                    if (oldestIndividualPoint < oldestGroupTriggerPoint) oldestGroupTriggerPoint = oldestIndividualPoint;
                }
            }

            // loop through all streams that are NOT receivers, and rid of old data using default levels
            for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
            {
                if (!streamData[chan]->groupTrigReceiver())
                {
                    if (!m_streamDataFlag[chan]) continue;
                    streamData[chan]->trimStream();
                }
            }

            // loop through all streams that are RECEIVER a chance to get rid of old data knowing the oldest group trigger  point
            for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
            {
                if (streamData[chan]->groupTrigReceiver())
                {
                    if (!m_streamDataFlag[chan])
                    {
                        printf ("Channel %d is set to be a group trigger receiver, but it is not set to stream!\n",chan);
                        continue;
                    }
                    streamData[chan]->trimStream(oldestGroupTriggerPoint);
                }
            }
        }                                         // end of standard trimming
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::updateTriggerRate()
    {

        // loop through all streams and get regular triggers
        for (unsigned int chan = 0; chan < m_nDataStreams; chan++ )
        {
            if (!m_streamDataFlag[chan]) continue;

            // get all pending data PR's from the channel
            streamData[chan]->triggerRate();
        }

    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::rewindDataFile()
    {
        //server->set(XCD_REWIND_DATA_FILE,XCD_ALLCHANNELS,0);

        printf ("Sending data rewind command\n");
        server->command(XCD_REWIND_DATA_FILE,0,XCD_ALLCHANNELS,0);

    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::median (float* arr, int n)
    {
        #define ELEM_SWAP(a,b) { register float t=(a);(a)=(b);(b)=t; }
        int i;
        int low, high;
        int median;
        int middle, ll, hh;
        float *buffer;

        // copy data into a new array
        buffer = (float *) malloc (n*sizeof(float));
        for (i=0;i<n;i++) buffer[i] = arr[i];

        low = 0 ; high = n-1 ; median = (low + high) / 2;
        for (;;)
        {
            if (high <= low)
            {
                /* One element only */
                float m = buffer[median];
                free (buffer);
                return m ;
            }

            if (high == low + 1)                  /* Two elements only */
            {
                if (buffer[low] > buffer[high])
                    ELEM_SWAP(buffer[low], buffer[high]) ;
                return buffer[median] ;
            }

            /* Find median of low, middle and high items; swap into position low */
            middle = (low + high) / 2;
            if (buffer[middle] > buffer[high])    ELEM_SWAP(buffer[middle], buffer[high]) ;
            if (buffer[low] > buffer[high])       ELEM_SWAP(buffer[low], buffer[high]) ;
            if (buffer[middle] > buffer[low])     ELEM_SWAP(buffer[middle], buffer[low]) ;

            /* Swap low item (now in position middle) into position (low+1) */
            ELEM_SWAP(buffer[middle], buffer[low+1]) ;

            /* Nibble from each end towards middle, swapping items when stuck */
            ll = low + 1;
            hh = high;
            for (;;)
            {
                do ll++; while (buffer[low] > buffer[ll]) ;
                do hh--; while (buffer[hh]  > buffer[low]) ;

                if (hh < ll)
                    break;

                ELEM_SWAP(buffer[ll], buffer[hh]) ;
            }

            /* Swap middle item (in position low) back into correct position */
            ELEM_SWAP(buffer[low], buffer[hh]) ;

            /* Re-set active partition */
            if (hh <= median)
                low = ll;
            if (hh >= median)
                high = hh - 1;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // query only
    /////////////////////////////////////////////////////////////////////////////
    bool         Client::connected     ( void ) { return m_connected; }
    bool         Client::streaming     ( void ) { return m_streaming; }
    unsigned int Client::nDatatStreams ( void ) { return m_nDataStreams; }
    unsigned int Client::nMuxPixels    ( void ) { return m_nMuxPixels; }
    unsigned int Client::nMuxRows      ( void ) { return m_nMuxRows; }
    unsigned int Client::nMuxCols      ( void ) { return m_nMuxCols; }
    XOUT*        Client::XOUTWriter    ( void ) { return m_XOUTWriter; }
    float        Client::dataRate      ( void ) { return m_dataRate; }
    bool         Client::channelInit   ( void ) { return m_channelInit; }
    char*        Client::versionString ( void ) { return m_versionString; }
    ServerType   Client::serverType    ( void ) { return m_serverType; }
    DataSource   Client::dataSource    ( void ) { return m_dataSource; }

    /////////////////////////////////////////////////////////////////////////////
    // encapsulation
    /////////////////////////////////////////////////////////////////////////////
    void Client::triggerRateIntegrationTime(double val)
    {
        std::cout << "Trigger Rate Integration Time " << val << std::endl;
        for (unsigned int i=0; i<m_nDataStreams; i++)
        {
            streamData[i]->triggerRateIntegrationTime(val);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    double Client::triggerRateIntegrationTime( void )
    {
        return streamData[0]->triggerRateIntegrationTime();
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned int Client::optimizeMixMaxRecords(void)
    {
        return m_optimizeMixMaxRecords;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::optimizeMixMaxRecords(unsigned int d)
    {
        m_optimizeMixMaxRecords = d;
        printf ("New Max Records value for Optimize Mix: %d\n",m_optimizeMixMaxRecords);
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::mixLevel(unsigned int chan, float val)
    {
        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << "Setting mix level for all channels to " << val << std::endl;
            server->set(XCD_MIXLEVEL,XCD_ALLCHANNELS,val);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                streamData[i]->mixLevel(val);
            }
        }
        else
        {
            std::cout << "Setting mix level for channel "<<chan<< " to " << val << std::endl;
            server->set(XCD_MIXLEVEL,chan,val);
            streamData[chan]->mixLevel(val);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::mixLevel(unsigned int chan)
    {
        return streamData[chan]->mixLevel();
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::streamDataFlag(unsigned int chan, bool b)
    {
        if (b) std::cout << "Activating ";
        else std::cout << "Deactivating ";
        std::cout << "channel "<< chan <<" data stream." << std::endl;
        server->set(XCD_ACTIVEFLAG, chan,b);
        m_streamDataFlag[chan] = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::streamDataFlag(unsigned int chan)
    {
        return m_streamDataFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::writePulsesToFileFlag(unsigned int chan, bool b)
    {
        std::cout << "Triggered pulses from channel "<< chan << " will ";
        if (b) std::cout << "be sent";
        else std::cout << "not be sent";
        std::cout <<  " to the data writer." << std::endl;
        m_writePulsesToFileFlag[chan] = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::writePulsesToFileFlag(unsigned int chan)
    {
        return m_writePulsesToFileFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::writeAutoToFileFlag(bool b)
    {
        std::cout << "Auto Triggered pulses will ";
        if (b) std::cout << "be";
        else std::cout << "not be";
        std::cout <<  " sent to the file writer" << std::endl;
        m_writeAutoToFileFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::writeAutoToFileFlag()
    {
        return m_writeAutoToFileFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::writeLevelToFileFlag(bool b)
    {
        std::cout << "Level Triggered pulses will ";
        if (b) std::cout << "be";
        else std::cout << "not be";
        std::cout <<  " sent to the file writer" << std::endl;
        m_writeLevelToFileFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::writeLevelToFileFlag()
    {
        return m_writeLevelToFileFlag;
    }
    /////////////////////////////////////////////////////////////////////////////
    void Client::writeEdgeToFileFlag(bool b)
    {
        std::cout << "Edge Triggered pulses will ";
        if (b) std::cout << "be";
        else std::cout << "not be";
        std::cout <<  " sent to the file writer" << std::endl;
        m_writeEdgeToFileFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::writeEdgeToFileFlag()
    {
        return m_writeEdgeToFileFlag;
    }
    /////////////////////////////////////////////////////////////////////////////
    void Client::writeNoiseToFileFlag(bool b)
    {
        std::cout << "Noise Triggered pulses will ";
        if (b) std::cout << "be";
        else std::cout << "not be";
        std::cout <<  " sent to the file writer" << std::endl;
        m_writeNoiseToFileFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::writeNoiseToFileFlag()
    {
        return m_writeNoiseToFileFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::performAnalysisFlag(unsigned int chan, bool b)
    {
        std::cout << "Triggered pulses from channel "<< chan << " will ";
        if (b) std::cout << "be";
        else std::cout << "not be";
        std::cout <<  " analyzed" << std::endl;
        m_performAnalysisFlag[chan] = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::performAnalysisFlag(unsigned int chan)
    {
        return m_performAnalysisFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::coupleEvenToOddFlag()
    {
        return m_coupleEvenToOddFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::coupleEvenToOddFlag(bool b)
    {
        if (b) std::cout << "Coupling Error to Feedback" << std::endl;
        else std::cout << "De-Coupling Error to Feedback" << std::endl;

        // turn off off to even if necessary
        if (m_coupleOddToEvenFlag & b)
        {
            coupleOddToEvenFlag(false);
        }

        m_coupleEvenToOddFlag=b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::coupleOddToEvenFlag()
    {
        return m_coupleOddToEvenFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::coupleOddToEvenFlag(bool b)
    {
        if (b) std::cout << "Coupling Feedback to Error" << std::endl;
        else std::cout << "De-Coupling Feedback to Error" << std::endl;

        // turn off even to off if necessary
        if (m_coupleEvenToOddFlag & b)
        {
            coupleEvenToOddFlag(false);
        }

        m_coupleOddToEvenFlag=b;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::setXOUTType(XOUTType val)
    {
        std::cout << "Setting the XOUT Type to : ";
        if (m_XOUTWriter != NULL)
        {
            m_XOUTWriter->closeXOUT();
            delete (m_XOUTWriter);
        }

        switch (val)
        {
            case PLS:
            {
                std::cout << "PLS" << std::endl;
                m_XOUTWriter = new PLSOutput();
                if (m_connected)
                {
                    // you are already running, so make sure to init!
                    bool even = false;            // default is even channel are unsigned
                    bool odd = false;             // default is odd channel is unsigned
                    if (m_serverType == NDFB)
                    {
                        // in ndfb, even channels are signed, odd are unsigned
                        even = true;
                        odd = false;
                    }
                    if (m_serverType == ROCKET)
                    {
                        //in rocket, all data is signed;
                        even = true;
                        odd = true;
                    }

                    ((xdaq::PLSOutput*) m_XOUTWriter)->init(even,odd,m_nDataStreams);

                }
                break;
            }
            case LJH:
            {
                std::cout << "NIST (LJH)" << std::endl;
                m_XOUTWriter = new LJHOutput();

                if (m_connected)
                {
                    // you are already running, so make sure to init!
                    ((xdaq::LJHOutput*) m_XOUTWriter)->initXOUT(m_nDataStreams);
                }
                break;
            }
            case LANL:
            {
                std::cout << "LANL" << std::endl;
                m_XOUTWriter = new LANLOutput();
                break;
            }
            case FFT:
            {
                std::cout << "FFT" << std::endl;
                m_XOUTWriter = new FFTOutput();
                m_XOUTWriter->recordMax(1);
                break;
            }

            default:
            {
                std::cout << "Unknown selection" << std::endl;
                return;
            }
        }

        m_XOUTType = val;
    }

    /////////////////////////////////////////////////////////////////////////////
    XOUTType Client::getXOUTType()
    {
        return m_XOUTType;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::noiseRecordsPerFFT(int val)
    {
        std::cout << "Setting fft record number to : "<< val << std::endl;

        for (unsigned int chan = 0;chan < m_nDataStreams;chan++)
        {
            noiseRecordFFT[chan]->numRecords(val);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    int Client::noiseRecordsPerFFT(void)
    {
        return noiseRecordFFT[0]->numRecords();
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::fftDiscriminator(int val)
    {
        std::cout << "Keeping 1 noise record for every "<< val << "  noise records taken." << std::endl;

        for (unsigned int chan = 0;chan < m_nDataStreams;chan++)
        {
            noiseRecordFFT[chan]->discriminator(val);
        }

    }

    /////////////////////////////////////////////////////////////////////////////
    int Client::fftDiscriminator(void)
    {
        return noiseRecordFFT[0]->discriminator();
    }

    /////////////////////////////////////////////////////////////////////////////
    int Client::optimizeMixRatioMethod ( void )
    {
        return m_optimizeMixRatioMethod;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::optimizeMixRatioMethod ( int i )
    {
        printf ("Using ");
        m_optimizeMixRatioMethod = i;
        if (m_optimizeMixRatioMethod == 0)
        {
            printf ("lowest STD seek.");
        }
        if (m_optimizeMixRatioMethod == 1)
        {
            printf ("RMS ratio");
        }

        printf (" method for the optimized mix.\n");
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::optimizeMixCouplePairsFlag ( void )
    {
        return m_optimizeMixCouplePairsFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::optimizeMixCouplePairsFlag ( bool b )
    {
        printf ("Using ");
        m_optimizeMixCouplePairsFlag = b;
        if (m_optimizeMixCouplePairsFlag)
        {
            printf ("coupled pairs");
        }
        else
        {
            printf ("uncoupled pairs");
        }
        printf (" for the optimized mix\n");
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned short int Client::decimateLevel(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in decimate flag request" << std::endl;
        }
        return m_decimateLevel[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::decimateLevel(unsigned int chan, unsigned short int val)
    {
        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << "Setting decimation level for all channels to " <<val<< std::endl;
            server->set(XCD_DECIMATELEVEL,XCD_ALLCHANNELS,(unsigned int)val);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                m_decimateLevel[i] = val;
            }
        }
        else
        {
            std::cout << "Setting decimation level for channel " <<chan<< " to " << val<<std::endl;
            server->set(XCD_DECIMATELEVEL,chan,(unsigned int)val);
            m_decimateLevel[chan] = val;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::decimateFlag(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in decimate flag request" << std::endl;
        }
        return m_decimateFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::decimateFlag(unsigned int chan, bool b)
    {
        if (b) std::cout << "Enabled"; else std::cout << "Disabled";

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << " decimation for all channels" <<std::endl;
            server->set(XCD_DECIMATEFLAG,XCD_ALLCHANNELS,b);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                m_decimateFlag[i] = b;
            }
        }
        else
        {
            std::cout << "decimation for channel "<<chan<< std::endl;
            server->set(XCD_DECIMATEFLAG,chan,b);
            m_decimateFlag[chan] = b;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::decimateAvgFlag(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in decimate flag request" << std::endl;
        }
        return m_decimateAvgFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::decimateAvgFlag(unsigned int chan, bool b)
    {
        if (b) std::cout << "Enabling";
        else std::cout << "Disabling";

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << " decimation averaging for all channels " << std::endl;
            server->set(XCD_DECIMATEAVGFLAG,XCD_ALLCHANNELS,b);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                m_decimateAvgFlag[i] = b;
            }
        }
        else
        {
            std::cout << " decimation averaging for channel "<<chan<< std::endl;
            server->set(XCD_DECIMATEAVGFLAG,chan,b);
            m_decimateAvgFlag[chan] = b;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::mixFlag(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in mix flag request" << std::endl;
        }
        return m_mixFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::mixFlag(unsigned int chan, bool b)
    {
        if (b) std::cout << "Enabling";
        else std::cout << "Disabling";

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << " mixing for all channels " << std::endl;
            server->set(XCD_MIXFLAG,XCD_ALLCHANNELS,b);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                m_mixFlag[i] = b;
            }
        }
        else
        {
            std::cout << " mixing for channel "<<chan<< std::endl;
            server->set(XCD_MIXFLAG,chan,b);
            m_mixFlag[chan] = b;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::mixInversionFlag(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in mix flag request" << std::endl;
            return false;
        }
        return m_mixInversionFlag[chan];
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::mixInversionFlag(unsigned int chan, bool b)
    {
        if (b) std::cout << "Mix Polarity set to Negative for ";
        else std::cout << "Mix Polarity set to positive for ";

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << "all channels" << std::endl;
            server->set(XCD_MIXINVERSIONFLAG,XCD_ALLCHANNELS,b);
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                m_mixInversionFlag[i] = b;
            }
        }
        else
        {
            std::cout << "channel "<<chan<< std::endl;
            server->set(XCD_MIXINVERSIONFLAG,chan,b);
            m_mixInversionFlag[chan] = b;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    int Client::pretrig(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in pretrigger length set " << std::endl;
            return false;
        }

        return streamData[chan]->pretrig();
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::pretrig(unsigned int chan, int val)
    {

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << "Setting pre-trigger length for all channels to " << val << std::endl;
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                streamData[i]->pretrig(val);
            }
        }
        else
        {
            std::cout << "Setting pre-trigger length for channel "<<chan<< " to " << val << std::endl;
            streamData[chan]->pretrig(val);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    int Client::recLen(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in record length set " << std::endl;
            return 0;
        }

        return streamData[chan]->recLen();
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::recLen(unsigned int chan, int val)
    {

        if (chan == XCD_ALLCHANNELS)
        {
            std::cout << "Setting record length for all channels to " << val << std::endl;
            for (unsigned int i=0; i<m_nDataStreams; i++)
            {
                streamData[i]->recLen(val);
            }
        }
        else
        {
            if (chan > m_nDataStreams)
            {
                std::cout << "Bad channel in record length set " << std::endl;
                return;
            }

            std::cout << "Setting record length for channel "<< chan << " to " << val << std::endl;
            streamData[chan]->recLen(val);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    float Client::effectiveSampleRate(unsigned int chan)
    {
        if (chan > m_nDataStreams)
        {
            std::cout << "Bad channel in effective sample rate " << std::endl;
            return 0;
        }

        return streamData[chan]->effectiveSampRate();
    }

    /////////////////////////////////////////////////////////////////////////////
    bool Client::tiltFlag(void)
    {
        return m_tiltFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void Client::tiltFlag(bool b)
    {
        m_tiltFlag = b;
        if (b) printf ("turning off tiltflag\n");
    }

    //     std::string Client::pretty_print_trigger_state(void) const
    //     {
    //         std::stringstream a;

    //         for (unsigned int chan=0;chan<m_nDataStreams;chan++)
    //         {
    //             std::stringstream chan_name;
    //             chan_name << "CH" << chan << "_";
    //             a << streamData[chan]->pretty_print_trigger_state("\n",chan_name.str());
    //         }
    //         return a.str();
    //     }

    //     void Client::write_trigger_state_to_file(const std::string& basename) const
    //     {

    //         const std::string ext="trigger_config";
    //         std::string filename=basename + "." + ext;

    //         // make an simple ascii file
    //         std::ofstream ofs(filename.c_str());

    //         ofs << pretty_print_trigger_state();
    //         ofs << "\n";

    //     }

    //     void Client::read_trigger_state_from_file(const std::string& filename) const
    //     {

    //       //        const std::string ext="trigger_config";
    //       //   std::string filename=basename + "." + ext;
    //       std::deque<std::string>lines;

    //       lines=util::slurp_file(filename);

    //       for(unsigned int i=0; i < lines.size(); i++)
    // 	{

    // 	  std::cout << lines[i];
    // }

    //     }
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
