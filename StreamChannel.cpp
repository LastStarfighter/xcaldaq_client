////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "StreamChannel.h"
////////////////////////////////////////////////////////////////////////////////

#include "string_utils.h"
#include <sstream>
#include <string.h>
// being name space
namespace xdaq
{
    /////////////////////////////////////////////////////////////////////////////
    // StreamChannel Class
    /////////////////////////////////////////////////////////////////////////////
    StreamChannel::StreamChannel(int chanNum,
        int pretrig,
        int recLen)
    {
        // put the inputs into the local vars
        m_pretrig                    = pretrig;
        m_recLen                     = recLen;
        m_pulseRec.channel(chanNum);

        m_numTriggers                = 0;
        m_autoThreshold              = 1000;      // send it 1000 ms
        m_levelThreshold             = 2;
        m_edgeThreshold              = 10;
        m_noiseThreshold             = 0;
        m_nextAutoTrigger            = 0;
        m_triggerRateIntegrationTime = 5000000;   // 1 second window to calculate pulse rate

        m_decimateFlag = false;
        m_decimateAvgFlag = false;
        m_decimateLevel = 0;
        m_mixFlag = false;
        m_mixInversionFlag = false;
        m_mixLevel = 0.;

        // init some values
        m_groupTrigReceiver = false;
        m_groupTrigSource   = false;
        m_autoTriggerFlag   = false;
        m_levelTriggerFlag  = false;
        m_edgeTriggerFlag   = false;
        m_noiseTriggerFlag  = false;
        m_levelInverseFlag  = false;
        m_edgeInverseFlag   = false;
        m_noiseInverseFlag  = false;

        m_init = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::stop ()
    {
        std::cout << "This channel is shutting down nicely" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::ingestDataPacket(const unsigned char *buf)
    {
        unsigned int i;
        bool tiltFlag = false;
        // set up the various pointer casts to this buffer
        unsigned long long *llptr = (unsigned long long *) buf;
        unsigned long      *lptr  = (unsigned long *) buf;
        unsigned short     *sptr  = (unsigned short *) buf;

        // strip the informaiton out of the buffer, this requires knowledge of the xcd header info!
        // we will check these local versions against the stream channels versions
        // to determin if we need to resync the channel

        // the size of the header
        unsigned long          bytesInHeader = ntohl(lptr[xcdBytesInHeaderIdx]);

        // the number of samples it will contain
        unsigned long          numSamps      = ntohl(lptr[xcdNumSampsIdx]);

        // how many bits per sample
        unsigned char          bitsPerSample = buf[xcdBitsPerSampleIdx];

        // the samples count number (i.e. the sample tick of the last data point)
        unsigned long long int sampleCount   = ntohll(llptr[xcdLastSampCountIdx]);

        // the samples count number (i.e. the sample tick of the last data point)
        unsigned long long int timeCount     = ntohll(llptr[xcdTimeCountIdx]);

        // the time code base
        unsigned long long     timecodeBase  = ntohll(llptr[xcdUTIdx]);

        // get the mix level
        float mixLevel;
        i = ntohl(lptr[xcdMix]);                  // get the unsigned int from the buffer
        memcpy (&mixLevel,&i,4);                  // drop it into a float

        // the decimate level
        unsigned short int decimateLevel = ntohs(sptr[xcdDecimate]);

        // delta t
        unsigned int sampleRateN = ntohl(lptr[xcdSampleRateN]);
        unsigned int sampleRateD = ntohl(lptr[xcdSampleRateD]);
                                                  // remember that deltat = 1/sampleRate
        double deltat = (double) ((double) sampleRateD / (double) sampleRateN);

        //printf ("got a sample rate of %e\n",1./deltat);

        // the yoffset
        float yoffset;
        i = ntohl(lptr[xcdOffset]);
        memcpy(&yoffset,&i,4);

        // the yscale
        float yscale;
        i = ntohl(lptr[xcdScale]);
        memcpy(&yscale,&i,4);

        // the raw min and max
        int rawMin = ntohl(lptr[xcdRawMin]);
        int rawMax = ntohl(lptr[xcdRawMax]);

        // the flags sent with this particular header
        unsigned short flags = ntohs(sptr[xcdFlagsIdx]);

        // test for the UT flag
        bool UTValid = false;
        if (flags & xcfUTValid)
        {
            UTValid = true;
        }

        // test for the signed flag
        bool signedSamples = false;
        if (flags & xcfSignedSamples)
        {
            signedSamples = true;
        }

        // test for the mix flag
        bool mixFlag = false;
        if (flags & xcfMixFlag)
        {
            mixFlag = true;
        }

        // test for the mix inverse flag
        bool mixInversionFlag = false;
        if (flags & xcfMixInversionFlag)
        {
            mixInversionFlag = true;
        }

        // test for the decimateAvg flag
        bool decimateAvgFlag;
        if (flags & xcfDecimateAvgFlag)
        {
            decimateAvgFlag = true;
        }

        // test for the decimate flag
        bool decimateFlag = false;
        if (flags & xcfDecimateFlag)
        {
            decimateFlag = true;
        }

        // quick error checks on the bits per sample rate
        if (bitsPerSample != 16 )
        {
            throw std::runtime_error("Only 16-bit samples are currently supported by XPulseRec.");
        }

        /*
         okay this is a little tricky, before we are initialized, we query the server for
         all sorts of information regarding the status regarding the data stream.  local variables
         in the _client object_ are initialized to match these remote values.  because these variables are
         used to make the buttons match up to the remote server, lets call these the gui local variables.
         however, each packet that comes in
         also carries these status variables and this is where the local variables in streamData get their values.
         note, the values in the header which may or may not match these local variables.
         sometimes if there is a mis-match, we want to resync the channel and change the local variables
         to match the remote ones.  other times if there is a mismatch it isn't a big deal.
         example 1: local gui local and remote have mix inverse as false.  local user changes local gui value to true
        and requests that the same thing happens at the server.  however, because it may
        take a few seconds for the change to actually be applied, the client will continue to receive
        some packets that still have the mix inverse flag as false. the local mix inverse flag
        stays false until eventually packets will arive that have the new value, but how much time later is uncertain.
        in this case, because nothing major changed, a mismatch does not require a resync of the channel
        example 2:  local gui, local and remote server both have the decimation as false.  local user changes the
        local gui value to true and requests that the same thing happens at the server.  server applies change
        but continues to send the rest of its non-decimated data first.  when the first packet arrives
        that is decimated, the local variable in streamData is updated.  because this requires and update
        in the effective sampling rate, the edge trigger, and other things, this requires a resync
        */

        // error check to make sure the header of this PR matches the header for the stream PR
        bool resync=false;

        if (m_pulseRec.timecodeBase() != timecodeBase)
        {
            resync = true;
            if (m_pulseRec.timecodeBase() == 0)
            {
                std::cout << "Timecode Base initialized to: " << timecodeBase << std::endl;
            }
            else
            {
                std::cout << "Timecode Base changed: " << m_pulseRec.timecodeBase() << " != " << timecodeBase << std::endl;
            }
        }

        if (deltat != m_pulseRec.deltat())
        {
            resync = true;
            if (m_pulseRec.deltat() == 0)
            {
                std::cout << "DeltaT initialized to: " << deltat << std::endl;
            }
            else
            {
                std::cout << "DeltaT changed: " << m_pulseRec.deltat() << " != " << deltat << std::endl;
            }
        }

        if (decimateLevel != m_decimateLevel)
        {
            // don't issue a refresh when decimation changes unless the decimation is actually enabled
            if (decimateFlag)
            {
                resync = true;
                if (m_decimateLevel == 0)
                {
                    std::cout << "Decimate Level initialized to: " << decimateLevel << std::endl;
                }
                else
                {
                    std::cout << "Decimate Level changed: " << m_decimateLevel << " (old) " <<  " != " << decimateLevel << " (new) " << std::endl;
                }
            }
        }

        if (decimateFlag != m_decimateFlag)
        {
            resync = true;
            std::cout << "Decimate Flag changed: " << m_decimateFlag << " (old) " <<    " != " << decimateFlag << " (new) " << std::endl;
        }

        if ( sampleCount - m_pulseRec.sampleCount() != numSamps)
        {
            resync = true;
            if (m_pulseRec.sampleCount() == 0)
            {
                std::cout << "Sample Count initialized to: " << sampleCount << std::endl;
            }
            else
            {
                std::cout << "Sample Count mismatch: " << sampleCount << " - " <<  m_pulseRec.sampleCount() << " = " << sampleCount - m_pulseRec.sampleCount() << " != " << numSamps << std::endl;
            }
        }

        if ( yscale != m_pulseRec.yscale())
        {
            resync = true;
            if (m_pulseRec.sampleCount() == 0)
            {
                std::cout << "Y Scale initialized to: " << yscale << std::endl;
            }
            else
            {
                std::cout << "Y Scale mismatch: " << yscale << " != " << m_pulseRec.yscale() << std::endl;
            }
        }

        if ( yoffset != m_pulseRec.yoffset())
        {
            resync = true;
            if (m_pulseRec.sampleCount() == 0)
            {
                std::cout << "Y Offset initialized to: " << yoffset << std::endl;
            }
            else
            {
                std::cout << "Y Offset mismatch: " << yoffset << " != " << m_pulseRec.yoffset() << std::endl;
            }
        }

        if ( rawMin != m_pulseRec.rawMin())
        {
            resync = true;
            if (m_pulseRec.sampleCount() == 0)
            {
                std::cout << "Raw Min initialized to: " << rawMin << std::endl;
            }
            else
            {
                std::cout << "Raw Min mismatch: " << rawMin << " != " << m_pulseRec.rawMin() << std::endl;
            }
        }

        if ( rawMax != m_pulseRec.rawMax())
        {
            resync = true;
            if (m_pulseRec.sampleCount() == 0)
            {
                std::cout << "Raw Max initialized to: " << rawMax << std::endl;
            }
            else
            {
                std::cout << "Raw Max mismatch: " << rawMax << " != " << m_pulseRec.rawMax() << std::endl;
            }
        }

        if (resync)
        {
            /*
             note: unlike other trigger values, the edge trigger is
             actually dependent on the delta t.  every time we change
             the delta t (or the decimation for that matter) we should
             also refresh the value of the raw edge trigger.  before
             we change delta t, store the user defined value of the
             edge trigger.  of course, if this is the first time
             through and we don't have a delta_t yet, then we should
             just init the threshold at some inane value
             */
            double edgeTrigger;
            if (m_init)
            {
                edgeTrigger = edgeThreshold();
            }
            else
            {
                edgeTrigger = 1000;               // remember this is V/S, the display is in V/ms
                m_init = true;
            }

            // something about the stream has changed so make sure to wipe the internal PR
            // shouldn't we be erasing all of the pending trigs as well?
            m_pulseRec.eraseRawShortSamples ();
            m_pulseRec.erasePhysSamples ();

            /* set the flags */
            // UTValid is not stored locally, so ignore it

            // reset signed samples
            m_pulseRec.signedSamples(signedSamples);

            // reset the timecode base
            m_pulseRec.timecodeBase(timecodeBase);

            // reset the yscale
            m_pulseRec.yscale(yscale);

            // reset the yoffset
            m_pulseRec.yoffset(yoffset);

            // reset the deltat
            m_pulseRec.deltat(deltat);

            // raw min
            m_pulseRec.rawMin(rawMin);
            m_pulseRec.rawMax(rawMax);

            // reset the local mix and decimate variables
            m_mixFlag = mixFlag;
            m_mixInversionFlag = mixInversionFlag;
            m_mixLevel = mixLevel;
            m_decimateAvgFlag = decimateAvgFlag;
            m_decimateFlag = decimateFlag;
            m_decimateLevel = decimateLevel;

            // now that we have reset deltaT and decimation level and
            // flag, reset the edge trigger
            edgeThreshold(edgeTrigger);           // see above

            // outside of the first initialization, this is the most
            // important step in resyncing all of the other values are
            // pretty much the same except perhaps the decimation,
            // mixing, and flags reset the current trigger scan point
            // to the beginning plus pretrigger
            m_startScanCount = sampleCount - numSamps + m_pretrig;

            // set the tilt flag
            tiltFlag = true;
        }

        // update the sample count
        m_pulseRec.sampleCount(sampleCount);

        // update the time count
        m_pulseRec.timeCount(timeCount);

        // cast the data buffer to a short int array
        unsigned short int *buf_us = (unsigned short int*) &buf[bytesInHeader];

        // add the data from the remote PR file into the local PR file
        // of the streamchannel remember that the raw data packet
        // still has data in net order!!
        for (unsigned int i=0;i<numSamps;i++)
        {
            m_pulseRec.appendRawShortSamples(ntohs(buf_us[i]));
        }

        // return the tilt
        return tiltFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::scanStreamForTriggers()
    {
        if (!m_init) return;

        // stop scanning as far out as possible
        unsigned long long int maxScanCount = m_pulseRec.sampleCount() - m_recLen ;

        // if the trimming is keeping up, there should be little to scan and this will be the case most of the time
        if (m_startScanCount == maxScanCount) return;

        // set up a sample iterator, picking up where you left off
        //std::deque<double>::const_iterator samp = m_pulseRec.m_physSamplesVector.begin() + countToIndex(m_startScanCount);
        std::deque<unsigned short>::const_iterator rawIterator = m_pulseRec.rawSampleBegin() + countToIndex(m_startScanCount);

        // begin the scan
        //std::cout << "channel: " << m_pulseRec.channel() ;
        //std::cout << " scanning " << "(" << m_startScanCount << ") " << " up to but not including " << "(" << maxScanCount << ") ";
        //std::cout << " threshold " << m_levelThreshold<< std::endl;

        unsigned short raw;
        bool triggered;
        int deriv;
        while (m_startScanCount < maxScanCount)
        {
            // the raw sampple
            raw = *rawIterator;

            // check for trips
            triggered = false;

            // auto trigger
            if (m_autoTriggerFlag)
            {

                // check if it is time to take another auto trig
                if (m_startScanCount >= m_nextAutoTrigger)
                {
                    triggered = true;

                    Trigger trig;
                    trig.sampleCount = m_startScanCount;
                    trig.triggerType = AUTO;
                    m_scannedTriggers.push_back(trig);
                    //std::cout << "auto triggered at " << m_startScanCount << " next trig at " << m_nextAutoTrigger << std::endl;

                    // set the next auto trigger point (don't forget to factor in any possible decimation)
                    // note, this could also be done using the timeCount factor, but this is easier for now

                    if (m_decimateFlag)
                    {
                        m_nextAutoTrigger = m_startScanCount + (mSecondToTicks(m_autoThreshold) / m_decimateLevel) ;
                    }
                    else
                    {
                        m_nextAutoTrigger = m_startScanCount + mSecondToTicks(m_autoThreshold) ;
                    }
                    //m_nextAutoTrigger = m_startScanCount + (m_autoThreshold) ;

                    /*
                    printf ("nextTrig %lld\t startScanCount %lld\t autoThreshold %llu\t decimate: %lld\n",
                              m_nextAutoTrigger,
                              m_startScanCount,
                              m_autoThreshold,
                              m_decimateLevel);
                     */

                    // get the trigger rate lock
                    m_triggerRateLock.getLock();

                    // add this trigger to the trigger rate deque
                    m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                    m_triggerRateLock.releaseLock();

                }

            }

            // level trigger
            if (m_levelTriggerFlag)
            {
                if (m_pulseRec.signedSamples())
                {
                    if (m_levelInverseFlag)
                    {
                                                  // negative level
                        if ((signed short)raw < (signed short)(m_levelThresholdRaw))
                        {
                            triggered = true;

                            Trigger trig;
                            trig.sampleCount = m_startScanCount;
                            trig.triggerType = LEVEL;
                            trig.threshold = levelThreshold();
                            trig.inverseFlag = m_levelInverseFlag;
                            m_scannedTriggers.push_back(trig);

                            // add this trigger to the trigger rate deque
                            m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                        }
                    }
                    else
                    {
                                                  // positive level
                        if ((signed short)raw > (signed short)m_levelThresholdRaw)
                        {
                            triggered = true;
                            Trigger trig;
                            trig.sampleCount = m_startScanCount;
                            trig.triggerType = LEVEL;
                            trig.threshold = levelThreshold();
                            trig.inverseFlag = m_levelInverseFlag;
                            m_scannedTriggers.push_back(trig);

                            // add this trigger to the trigger rate deque
                            m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                        }
                    }
                }
                else
                {
                    if (m_levelInverseFlag)
                    {
                                                  // negative level
                        if (raw < m_levelThresholdRaw)
                        {
                            triggered = true;
                            Trigger trig;
                            trig.sampleCount = m_startScanCount;
                            trig.triggerType = LEVEL;
                            trig.threshold = levelThreshold();
                            trig.inverseFlag = m_levelInverseFlag;
                            m_scannedTriggers.push_back(trig);

                            // add this trigger to the trigger rate deque
                            m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));
                        }
                    }
                    else
                    {
                                                  // positive level
                        if (raw > m_levelThresholdRaw)
                        {
                            triggered = true;
                            Trigger trig;
                            trig.sampleCount = m_startScanCount;
                            trig.triggerType = LEVEL;
                            trig.threshold = levelThreshold();
                            trig.inverseFlag = m_levelInverseFlag;
                            m_scannedTriggers.push_back(trig);
                            // add this trigger to the trigger rate deque
                            m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                        }
                    }
                }
            }

            // edge trigger
            if (m_edgeTriggerFlag)
            {
                // we should always have at least 3 points in the
                // future to look at b/c we never get closer to the
                // edge of the data stream then recLen.  also, if this
                // triggers, the trigger point will be the first point
                // used in the derivitive. i don't know if this is
                // necessarily correct.  in general, the deriv =
                // ((y4+y3)/2 - (y2+y1)/2) / (2*deltat); but remember!
                // we already factored in the 4 * delta t when we
                // encapsulated the edge threshold so derivitive is
                // simply y4 + y3 - y2 - y1 note, any decimation
                // should be already factored into the
                // m_edgeThresholdRaw so we shouldn't have to worry
                // about it here
                deriv=0;

                if (m_pulseRec.signedSamples())
                {

                    deriv  = (signed short) *(rawIterator+3);
                    deriv += (signed short) *(rawIterator+2);
                    deriv -= (signed short) *(rawIterator+1);
                    deriv -= (signed short) *(rawIterator);

                }
                else
                {
                    deriv = *(rawIterator+3) + *(rawIterator+2) - *(rawIterator+1) - *(rawIterator);
                }

                if (m_edgeInverseFlag)
                {
                                                  // negative level
                    if (deriv < (-1 * m_edgeThresholdRaw))
                    {
                        triggered = true;
                        Trigger trig;
                        trig.sampleCount = m_startScanCount;
                        trig.triggerType = EDGE;
                        trig.threshold   = edgeThreshold();
                        trig.inverseFlag = m_edgeInverseFlag;
                        m_scannedTriggers.push_back(trig);

                        // add this trigger to the trigger rate deque
                        m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                    }
                }
                else
                {
                                                  // positive level
                    if (deriv > m_edgeThresholdRaw)
                    {
                        triggered = true;
                        Trigger trig;
                        trig.sampleCount = m_startScanCount;
                        trig.triggerType = EDGE;
                        trig.threshold   = edgeThreshold();
                        trig.inverseFlag = m_edgeInverseFlag;
                        m_scannedTriggers.push_back(trig);

                        // add this trigger to the trigger rate deque
                        m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount));

                    }
                }

            }

            // noise trigger
            if (m_noiseTriggerFlag)
            {

                if (m_noiseInverseFlag)
                {
                    if (raw > m_noiseThresholdRaw)// we want to discard anything that falls beloe the threshold, so the sign is opposite what you might think
                    {
                        m_continuousNoisePoints++;// another one in a row
                    }
                    else
                    {
                                                  // reset
                        m_continuousNoisePoints = 0;
                    }
                }
                else
                {
                    if (raw < m_noiseThresholdRaw)// we want to discard anything that excedes the threshold, so the sign is opposite what you might think
                    {
                        m_continuousNoisePoints++;// another one in a row
                        if (m_continuousNoisePoints % 500 == 0)
                        {
                            //printf ("passed: %d up until %llu\n",m_continuousNoisePoints,m_startScanCount);
                        }
                    }
                    else
                    {
                        //printf ("failed: %d up until %llu\n",m_continuousNoisePoints,m_startScanCount);
                                                  // reset
                        m_continuousNoisePoints = 0;
                    }
                }

                if (m_continuousNoisePoints > m_recLen)
                {

                    // note this point might have already been discarded.  if that is the case, the trigger
                    // will just be discarded.  this isn't really a problem b/c noise is cheap
                    Trigger trig;
                    trig.sampleCount = m_startScanCount - (m_recLen - m_pretrig);
                    trig.triggerType = NOISE;
                    trig.threshold = noiseThreshold();
                    trig.inverseFlag = m_noiseInverseFlag;
                    m_noiseTriggers.push_back(trig);

                    // add this trigger to the trigger rate deque
                    m_triggerRateDeque.push_back(sampleCountToEpochTime(m_startScanCount  - (m_recLen - m_pretrig)) );

                    //printf ("noise trigger: %d logged at %llu trigger at %llu\n",m_noiseTriggers.size(),m_startScanCount,m_startScanCount  - (m_recLen - m_pretrig));

                    // triggered = true;	 don't sent this true b/c looking for noise shouldn't interupt the scan for real triggers
                    m_continuousNoisePoints = 0;  // make sure to reset the counter!!

                }

            }

            // set what point you will look at next
            if (triggered)
            {

                /*
                std::cout <<
                   "Channel "<<m_pulseRec.channel() << ": Trigger happened at TimeBase +  " <<
                    (unsigned long long int)((double)m_startScanCount * m_pulseRec.deltat()) <<
                    " Seconds" << std::endl;
                */

                // setup the start scaning point, so that if the next scanned point
                //   triggers, the pretrig will not overlap the end of this record
                //   actually they do overlap by one point
                //   if recLen = 100 and pretrig is 25 and you trigger at 50
                //   pretrig is 24-49, trig at 50, rec goes from 24 - 124
                //   start scanning again at 150, if you trigger on that point
                //   pretrig is 124-149, trig at 150 and rec goes from 124 to 224
                //   point 124 is repeated.  do you care?
                rawIterator += m_recLen;
                m_startScanCount += m_recLen;
                m_continuousNoisePoints = 0;      // we are not continuous anymore so reset this
            }
            else
            {
                rawIterator++;
                m_startScanCount++;
            }
        }
        //std::cout <<"next point to be scanned " << "(" << m_startScanCount << ") " << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    Trigger StreamChannel::getNextTrig()
    {
        Trigger trig;
        trig.sampleCount=0;

        if (m_scannedTriggers.size() > 0)
        {
            trig = m_scannedTriggers.front();
            m_scannedTriggers.pop_front();
        }

        // calling function can test if the scannedTrigger deque is empty
        // by testing if trig.sampleCount == zero.
        return trig;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::eraseAllPendingTrigs(void)
    {
        while (m_pendingTriggers.size() != 0)
        {
            Trigger trig = m_pendingTriggers.front();
            m_pendingTriggersNumber.pop_front();
            m_pendingTriggers.pop_front();
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::addPendingTrig(Trigger trig)
    {
        Trigger l_trig = trig;

        m_pendingTriggers.push_back(l_trig);
        m_numTriggers++;
        m_pendingTriggersNumber.push_back(m_numTriggers);

        /*
         std::cout << "Channel " << m_pulseRec.channel() << " received a pending trigger #" << m_numTriggers << " at " << trig << std::endl;
         */

    }

    /////////////////////////////////////////////////////////////////////////////
    XPulseRec* StreamChannel::getNextTriggeredPulseRecord()
    {
        Trigger trig;
        int num;

        if (m_pendingTriggers.size() == 0)
        {
            // no more pending
            return NULL;
        }
        else
        {
            trig = m_pendingTriggers.front();
            num = m_pendingTriggersNumber.front();
        }

        // catch any invalid times
        if (trig.sampleCount < m_pulseRec.sampleCount() - m_pulseRec.length() + m_pretrig)
        {
            std::cout << "There is not enough pretrigger information in the channel " << m_pulseRec.channel() << " stream to create a record around trigger #" <<
                num << " at " << trig.sampleCount << ". Removing trigger from pending menu." << std::endl;
            m_pendingTriggers.pop_front();
            m_pendingTriggersNumber.pop_front();
            return NULL;
        }

        static bool annoyingErrorFlag = true;

        if (trig.sampleCount > m_pulseRec.sampleCount() - m_recLen + m_pretrig)
        {
            if (annoyingErrorFlag)
            {
                std::cout << "Channel " << m_pulseRec.channel() <<
                    " needs more post trigger data for trigger #" << num << " at " << trig.sampleCount << std::endl;
            }
            annoyingErrorFlag = false;
            return NULL;
        }

        annoyingErrorFlag = true;

        //std::cout << "Channel " << m_pulseRec.channel() << " is generating a TRIGGERED pulse record for trigger #" << num << " at " << trig << std::endl;

        XPulseRec *PR = createTriggeredPulseRecord(trig);
        if (PR == NULL)
        {
            // this trigger will have to wait for more data
            return NULL;
        }
        else
        {
            // take that trig off the pending list and delete the trig
            m_pendingTriggers.pop_front();
            m_pendingTriggersNumber.pop_front();
            return PR;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    XPulseRec* StreamChannel::getNextNoisePulseRecord()
    {
        Trigger trig;

        if (m_noiseTriggers.size() == 0)
        {
            return NULL;                          // no more noise
        }
        else
        {
            trig = m_noiseTriggers.front();
        }

        // catch any invalid times <- as opposed to getNextPendingPR this is going to happen
        // frequently so we don't care if we drop them and don't need to print a error to screen
        if (trig.sampleCount < m_pulseRec.sampleCount() - m_pulseRec.length() + m_pretrig)
        {
            m_noiseTriggers.pop_front();
            std::cout << "Channel " << m_pulseRec.channel() <<
                " needs more data in order to create a NOISE record for trigger " << trig.sampleCount << std::endl;
            return NULL;
        }

        // you shouldn't ever see this b/c it can't scan past the end and can't
        // receiver triggers from other sources
        if (trig.sampleCount > m_pulseRec.sampleCount() - m_recLen + m_pretrig)
        {
            std::cout << "Channel " << m_pulseRec.channel() <<
                " needs more data in order to create a NOISE record for trigger " << trig.sampleCount << std::endl;
            return NULL;
        }

        //std::cout << "Channel " << m_pulseRec.channel() << " is generating a NOISE pulse record for trigger " << trig << std::endl;
        XPulseRec *PR = createTriggeredPulseRecord(trig);
        if (PR == NULL)
        {
            m_noiseTriggers.pop_front();          // regardless of what happens to this trig, pop it to avoid loop
            return NULL;
        }
        else
        {
            // take that trig off the noise list
            m_noiseTriggers.pop_front();
            return PR;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    XPulseRec* StreamChannel::createTriggeredPulseRecord(Trigger trig)
    {
        unsigned long long int triggerPoint = trig.sampleCount;
        // note that trigger point is in sample count unit, not time count units!!!

        // catch zero's and ignore
        if (triggerPoint == 0)
        {
            return NULL;
        }

        // declare some bounds for the trigger in count space
        unsigned long long int maxCount;
        unsigned long long int minCount;
                                                  // trig can be no closer to end then this
        maxCount = m_pulseRec.sampleCount() - m_recLen + m_pretrig;
                                                  // trig can be no closer to beginning than this
        minCount = m_pulseRec.sampleCount() - m_pulseRec.length() + m_pretrig;

        // catch any invalid times
        if (triggerPoint < minCount)
        {
            std::cout << "There is not enough information in the stream to create a record around trigger " << triggerPoint << std::endl;
            return NULL;
        }

        if (triggerPoint > maxCount)
        {
            std::cout << "Stream needs more data in order to create a record around trigger " << triggerPoint << std::endl;
            return NULL;
        }

        //std::cout << "Channel " << m_pulseRec.channel() << " created a triggered pulse record at SAMPLE count "  << triggerPoint <<  std::endl;

        /*
         std::cout << "Creating a trigger for channel " <<
         m_pulseRec.channel() <<
         " triggered at "  << triggerPoint <<
         " starting at "   << triggerPoint - m_pretrig <<
         " and ending at " << triggerPoint + (m_recLen - m_pretrig) - 1 << std::endl;
         */

        /*
         std::cout << "Creating a trigger for channel " <<
         m_pulseRec.channel() <<
         " trig point "  << triggerPoint <<
         " timebase  "   << m_pulseRec.timecodeBase() <<
         " delta time  " << (unsigned long long int)(sampleCountToTimeCount(triggerPoint) * m_pulseRec.deltat() * 1000000LL) <<
         std::endl;
         */

        // create a new triggered pulse record.  remember, you have to delete this somewhere!
        XPulseRec *PR = new XPulseRec ();

        // store the pertinent stream data into the pulse record (an XPulseRec)
        PR->channel       ( m_pulseRec.channel() );
        PR->pretrigPnts   ( m_pretrig );
        PR->yscale        ( m_pulseRec.yscale() );
        PR->yoffset       ( m_pulseRec.yoffset() );
        PR->timecodeBase  ( m_pulseRec.timecodeBase() );
        PR->signedSamples ( m_pulseRec.signedSamples() );
        PR->rawMax        ( m_pulseRec.rawMax() );
        PR->rawMin        ( m_pulseRec.rawMin() );

        // remember that the xpulserec doesn't need to know anything about decimation, only the effective deltaT
        if (m_decimateFlag)
        {
            PR->deltat((m_pulseRec.deltat() * m_decimateLevel));
        }
        else
        {
            PR->deltat(m_pulseRec.deltat());
        }

        // don't forget to use the time count # and not the sample count #.  this should give the PR the proper time stamp
        // this is important b/c when we decimate, timeCount != sampleCount
        PR->timecodeBase ( m_pulseRec.timecodeBase() );
        PR->timecode     ( (unsigned long long int)(sampleCountToTimeCount(triggerPoint) * m_pulseRec.deltat() * 1000000LL) );

        //std::cout << "Channel " << m_pulseRec.channel() << " created a triggered pulse record at TIMECODE count "  << PR->timecode() <<  std::endl;

        // the trigger method
        PR->triggerType(trig.triggerType);

        // the trigger threshold
        PR->threshold(trig.threshold);

        // where should we start in array space
        unsigned int startIdx = countToIndex(triggerPoint - m_pretrig);

        // setup the iterators
        std::deque<unsigned short int>::const_iterator rawSamp = m_pulseRec.rawSampleBegin() + startIdx;

        // copy a segment of samples from the stream's internal PR's to the new PR
        for (unsigned int sampNum = 0; sampNum < m_recLen; sampNum++, rawSamp++)
        {
            PR->appendRawShortSamples(*rawSamp);
        }

        // return the PR
        return PR;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned long long int StreamChannel::oldestTrimPoint(void)
    {
        if (!m_init) return 0;

        unsigned long long int oldestPoint;

        // this is the newest possible "oldestpoint" that you would ever want to throw out
        // we will apply various tests to see if we want to keep older data than this
        //oldestPoint = m_pulseRec.sampleCount() - m_recLen - m_pretrig - 1;
        oldestPoint = m_pulseRec.sampleCount() - (2 * m_recLen) - 1;

        // this is auto mode, so don't attempt to throw away more data then you actually have
        // this can happen when streaming has just started and recLen ~ length()
        if (oldestPoint < (m_pulseRec.sampleCount() - m_pulseRec.length())) oldestPoint = m_pulseRec.sampleCount() - m_pulseRec.length();

        // see if there are any pending triggers that are older than this point
        std::deque<Trigger>::iterator trig = m_pendingTriggers.begin();

        // get the oldest point you still need to make trigger records
        for (unsigned int i=0;i<m_pendingTriggers.size();i++)
        {
            unsigned long long int point = (*trig).sampleCount - m_pretrig - 1;
            if (point < oldestPoint) oldestPoint = point;
        }

        return oldestPoint;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::addPulseData (double height, double baseline, double time)
    {
        // get lock
        m_pulseAnalysisLock.getLock();

        // add the data
        m_pulseHeight.push_back    ( height );
        m_baseline.push_back       ( baseline );
        m_pulseTimeFloat.push_back ( time );

        while (m_pulseHeight.size() > MAX_PULSE_ANALYSIS_POINTS)
        {
            m_pulseHeight.pop_front();
            m_baseline.pop_front();
            m_pulseTimeFloat.pop_front();
        }

        // release the lock
        m_pulseAnalysisLock.releaseLock();

        //printf ("height %f baseline %f time %llu\n",height,baseline, time);
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::trimStream(void)
    {
        if (!m_init) return;

        // this is auto mode, so give it a really high number to trim to.
        // the calculated oldest point will def be lower than that, which is what we want anyways.
        trimStream(m_pulseRec.sampleCount());
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::trimStream(unsigned long long int requestedPoint)
    {
        if (!m_init) return;

        // check if we don't have an older point that we want to keep
        unsigned long long int calculatedOldestPoint = oldestTrimPoint();

        //std::cout << "Channel: " <<m_pulseRec.channel() << " was asked to erase up to " << requestedPoint;

        // compare the requested point with the calculated oldest point and make sure that you haven't requested
        // that too much data be thrown away
        if (calculatedOldestPoint < requestedPoint) requestedPoint = calculatedOldestPoint;
        //std::cout << " but is going to erase up to " << requestedPoint << "b/c calcualted oldest point is "<< calculatedOldestPoint << std::endl;

        // now compare the requested point to make sure that that point actually exists in the data stream
        if (requestedPoint < m_pulseRec.sampleCount() - m_pulseRec.length())
        {
            /*
             change this statement so that it fails quietly
             std::cout << "channel " << m_pulseRec.channel() <<
             " tried to erase to " << requestedPoint <<
             " but oldest point in stream is: " << m_pulseRec.sampleCount() - m_pulseRec.length() <<
             std::endl;
             */
            return;
        }

        // error check: don't erase more than this point ever, you can't possibly have scanned it
        if ((m_pulseRec.sampleCount() - m_recLen) < requestedPoint)
        {
            requestedPoint = m_pulseRec.sampleCount() - m_recLen;
            return;
        }

        // you are erasing data we haven't scanned
        if (requestedPoint > m_startScanCount)
        {
            std::cout << "channel " << m_pulseRec.channel() << " trying to erase data it hasn't scanned yet! " << std::endl;
            return;
        }

        // we are already fully trimed
        if (requestedPoint == m_pulseRec.sampleCount() - m_pulseRec.length())
        {
            //std::cout << "channel " << m_pulseRec.channel() << " fully trimmed" << std::endl;
            return;
        }

        /*
        std::cout << "channel " << m_pulseRec.channel() << " oldest point is now " << requestedPoint <<
            " newest point is " << m_pulseRec.sampleCount() << std::endl;
         */
        m_pulseRec.eraseTo(requestedPoint);
    }

    /////////////////////////////////////////////////////////////////////////////
    // conversions
    /////////////////////////////////////////////////////////////////////////////
    unsigned long long int StreamChannel::sampleCountToTimeCount(unsigned long long int sampleCount)
    {
        // this is tricky.  here is an example:
        // trigger is at sample #1000, the last point in the record is at sample #2000 and time #20000 with decimation of 10
        // the _time_ of the trigger is given by 20000 - ((2000 - 1000) * 10) = 10,000
        // if decimation is, and has always been, = 1, then t = s;
        // this assumes that the # of ticks b/w the end of the record and the trigger is << time since last decimation change
        // in our example, if the decimation changed from 2 to 10 at sample 1500, we would get the wrong time
        // the moral of the story is, turn on decimation a few seconds b/f you want to record data
        // and don't change the decimation level whilest recording.   you have been warned
        unsigned long long int t = m_pulseRec.timeCount();
        if (m_decimateFlag)
        {
            t -=    ((m_pulseRec.sampleCount() - sampleCount) * m_decimateLevel);
        }
        else
        {
            t -=    (m_pulseRec.sampleCount() - sampleCount);
        }
        return t;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned long long int StreamChannel::sampleCountToEpochTime(unsigned long long int sampleCount)
    {

        // first, convert the sample count into a time count (remember, that this is only accurate if there
        // has been sufficient time since the last decimation change!!!).  now multiply the time count by
        // the deltat to give you the time (in uS) since the time base.  convert to u-seconds and add in the
        // time code base and bob's you uncle.  no seriously, he was your dad's brother
        return ( m_pulseRec.timecodeBase() +
            ( unsigned long long int) ( sampleCountToTimeCount(sampleCount) *
            m_pulseRec.deltat() *
            1000000LL )
            );
    }

    /////////////////////////////////////////////////////////////////////////////
    int StreamChannel::countToIndex(unsigned long long int c)
    {
        return c - (m_pulseRec.sampleCount() - m_pulseRec.length());
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned long long int StreamChannel::indexToCount(int i)
    {
        // this is very simplistic and does not look at changing delta t's
        return i + (m_pulseRec.sampleCount() - m_pulseRec.length());
    }

    /////////////////////////////////////////////////////////////////////////////
    int StreamChannel::mSecondToTicks (double mSeconds)
    {
        if (m_pulseRec.deltat() == 0)
        {
            return 0;
        }
        else
        {
            return (int) round((mSeconds / (1000. * m_pulseRec.deltat())));
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // boring query stuff
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::requestPulseHeightCopy (double **x_pointer, double **y_pointer, unsigned int *n_pointer)
    {
        // get lock
        m_pulseAnalysisLock.getLock();

        unsigned int n = m_pulseHeight.size();

        if (n == 0)
        {
            //printf ("no pulse height data to copy!\n");

            // set the n value.  this will indicate to the calling function that
            // there is nothing to copy
            *n_pointer = n;

            // release the lock
            m_pulseAnalysisLock.releaseLock();

            return;
        }

        // make some memory
        double *x = new double [n];
        double *y = new double [n];

        // setup the iterators
        std::deque<double>::iterator PH = m_pulseHeight.begin();
        std::deque<double>::iterator T  = m_pulseTimeFloat.begin();

        // copy the data over
        for (unsigned int i = 0; i < n; i++, PH++, T++)
        {
            y[i] = *PH;
            x[i] = *T;
            //printf ("%d [%f,%f]\n",i,x[i],y[i]);
        }

        // make sure the calling function can see it
        *n_pointer = n;
        *x_pointer = x;
        *y_pointer = y;

        // release the lock
        m_pulseAnalysisLock.releaseLock();
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::requestBaselineCopy (double **x_pointer, double **y_pointer, unsigned int *n_pointer)
    {
        // get lock
        m_pulseAnalysisLock.getLock();

        unsigned int n = m_baseline.size();

        if (n == 0)
        {
            //printf ("no baseline data to copy!\n");

            // set the n value.  this will indicate to the calling function that
            // there is nothing to copy
            *n_pointer = n;

            // release the lock
            m_pulseAnalysisLock.releaseLock();

            return;
        }

        // make some memory
        double *x = new double [n];
        double *y = new double [n];

        // setup the iterators
        std::deque<double>::iterator BL = m_baseline.begin();
        std::deque<double>::iterator T  = m_pulseTimeFloat.begin();

        // copy the data over
        for (unsigned int i = 0; i < n; i++, BL++, T++)
        {
            y[i] = *BL;
            x[i] = *T;
            //printf ("%d [%f,%f]\n",i,x[i],y[i]);
        }

        // make sure the calling function can see it
        *n_pointer = n;
        *x_pointer = x;
        *y_pointer = y;

        // release the lock
        m_pulseAnalysisLock.releaseLock();
    }

    /////////////////////////////////////////////////////////////////////////////
    XPulseRec* StreamChannel::pulseRec(void)
    {
        return &m_pulseRec;
    }
    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::maxLevelThreshold(void)
    {
        return m_pulseRec.physMax();
    }
    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::minLevelThreshold(void)
    {
        return m_pulseRec.physMin();
    }
    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::maxEdgeThreshold(void)
    {
        if (m_pulseRec.deltat() == 0.0)
        {
            printf ("delta_t hasn't been initialized yet, so we can't give a max edge threshold\n");
        }

        return (m_pulseRec.physMax() - m_pulseRec.physMin()) / m_pulseRec.deltat();
    }
    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::minEdgeThreshold(void)
    {
        return 0.;                                // if you want to trigger for less than that, use the negative switch
    }
    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::init(void)
    {
        return m_init;
    }
    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::triggerRate(void)
    {

        // get the trigger rate lock
        m_triggerRateLock.getLock();

        // get the current tick time (make sure to use data time, not machine local time!!!)
        unsigned long long int currentTime = sampleCountToEpochTime(m_pulseRec.sampleCount());

        // calculate the minimum tick that you want to look at (i.e. the window size)
        unsigned long long int window = currentTime - m_triggerRateIntegrationTime;

        //printf ("chan %d current: %llu window: %u cutoff time: %llu\n",m_pulseRec.channel(),currentTime, m_triggerRateIntegrationTime, window);

        // get the trigger size
        int trigger = m_triggerRateDeque.size();

        // loop
        for (;;)
        {

            // see if there are any triggers left
            if (trigger == 0)
            {
                m_triggerRateLock.releaseLock();
                return 0.;
            }

            //printf ("sample %llu \t\twindow %llu... ", m_triggerRateDeque.front(), window);

            // pop off everything that is outside the window
            if (m_triggerRateDeque.front() < window)
            {
                m_triggerRateDeque.pop_front();
                trigger--;
            }
            else
            {
                break;
            }
        }

        // calculate the rate.  remember to divide by 1e6 to convert from (trigger / uS) to (triggers / S)
        // there are two ways to do this, depending upon how many triggers there are in the window
        double events;
        double time;
        if (trigger == 1)
        {
            events = 1.;
            time = (double) m_triggerRateIntegrationTime / 1e6;
        }
        else
        {
            events = trigger - 1;
            time = (double) m_triggerRateDeque.back() - (double) m_triggerRateDeque.front();
            time /= 1e6;
        }

        double rate;
        rate = events / time;

        //printf ("channel %d events %lf time %lf rate %lf\n",m_pulseRec.channel(),events, time, rate);

        // release the lock
        m_triggerRateLock.releaseLock();

        return rate;
    }

    /////////////////////////////////////////////////////////////////////////////
    // boring encapsulation stuff
    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::groupTrigReceiver (void)
    {
        return m_groupTrigReceiver;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::groupTrigReceiver (bool b)
    {
        m_groupTrigReceiver = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::groupTrigSource(void)
    {
        return m_groupTrigSource;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::groupTrigSource(bool b)
    {
        m_groupTrigSource = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    int StreamChannel::pretrig(void)
    {
        return m_pretrig;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::pretrig(int val)
    {
        m_pretrig = val;
    }

    /////////////////////////////////////////////////////////////////////////////
    int StreamChannel::recLen(void)
    {
        return m_recLen;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::recLen(int val)
    {
        m_recLen = val;
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::triggerRateIntegrationTime(void)
    {
                                                  // convert back to S;
        return ((double) m_triggerRateIntegrationTime)  / 1e6;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::triggerRateIntegrationTime(double d)
    {
                                                  // convert to mS
        m_triggerRateIntegrationTime = (unsigned long long int) (d * 1e6);
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::levelThreshold(void)
    {
        return m_levelThreshold;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::levelThreshold(double d)
    {
        m_levelThreshold=d;
        m_levelThresholdRaw = m_pulseRec.physicalToRaw(d);
        std::cout << "Threshold for channel "<< m_pulseRec.channel() << " Level Mode set to " << m_levelThreshold << " - " << m_levelThresholdRaw << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::edgeThreshold(void)
    {
        if (m_pulseRec.deltat() == 0.0)
        {
            printf ("delta_t hasn't been initialized yet, so edge threshold isn't going to make sense\n");
        }

        // remember to convert back to standard V/s units by removing the ants (delta t)

        double threshold  = m_edgeThreshold / (2. * 2. * m_pulseRec.deltat());

        // don't forget about decimation (remember, decimate level can non 1 but if it is not enabled it doesn't matter)
        if (m_decimateFlag)
        {
            threshold /= m_decimateLevel;
        }

        return threshold;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::edgeThreshold(double d)
    {
        // d is coming in as volts per S which is easier to use internally.

        std::cout << "Threshold for channel "<< m_pulseRec.channel() << " Edge Mode set to " << d << " Volts / Second" << std::endl;

        // the slope of the raw data stream is ((y4 +y3)/2 - (y2
        // +y1)/2) / (2*deltat) which is in volts per _second_ we
        // compare this number vs what the user has inputed as the
        // threshold (also in volts per second) but that isn't very
        // computationally efficent b/c we are always dividing and
        // multiplying by constants instead, let's put all of the
        // constants into the edge threshold.  this makes the math
        // easier when we check to see if the raw data stream has
        // exceded the threshold i.e. instead of checking of ((y4
        // +y3)/2 - (y2 +y1)/2) / (2*deltat) > threshold let's check
        // if (y4 +y3) - (y2 +y1) > threshold * 2 *2 * deltat =
        // threshold_prime

        m_edgeThreshold = d * (2. * 2. * m_pulseRec.deltat());

        // don't forget about decimation
        if (m_decimateFlag)
        {
            m_edgeThreshold *= m_decimateLevel;
        }

        // this is interesting, we cannot simply convert the physical
        // to raw signal to get a derivitive in raw units the reason
        // behind this is that the physical units we have are delta V
        // and the physical to raw actually converts V doesn't sound
        // important right?  but the physical -> raw actually takes
        // the y offset into consideration we should really take raw =
        // physicalToRaw(m_edgeThreshold) - physicalToRaw(0);

        // the edge is always positive regardless of the signedness of the signal
        m_edgeThresholdRaw = m_pulseRec.physicalToRaw(m_edgeThreshold) - m_pulseRec.physicalToRaw(0);

        printf ("desire: %f\tdelta: %f\tedge: %f\n",d,m_pulseRec.deltat(),m_edgeThreshold);
        printf ("raw signed: %d\traw unsigned: %u\n",m_edgeThresholdRaw,m_edgeThresholdRaw);
        printf ("raw signed: %d\traw unsigned: %u\n",(signed short)m_edgeThresholdRaw,(signed short)m_edgeThresholdRaw);
        std::cout << "Raw Threshold for channel "<< m_pulseRec.channel() << " Edge Mode set to " << m_edgeThresholdRaw << " raw counts/tick" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::autoThreshold(void)
    {
        return m_autoThreshold;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::autoThreshold (double d)  // d is in mSeconds!!
    {
        m_autoThreshold = d;

        std::cout << "Auto trigger threshold for channel "
            << m_pulseRec.channel() << " is set to " <<  m_autoThreshold << " mSecond between triggers or "
            << mSecondToTicks(m_autoThreshold) << " undecimated ticks of " << m_pulseRec.deltat() << " seconds" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::noiseThreshold(void)
    {
        return m_noiseThreshold;
    }
    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::noiseThreshold(double d)
    {
        m_noiseThreshold=d;
        m_noiseThresholdRaw = m_pulseRec.physicalToRaw(d);
        std::cout << "Threshold for channel "<< m_pulseRec.channel() << " Noise Mode set to " << m_noiseThreshold << " - " << m_noiseThresholdRaw << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::autoTriggerFlag(bool b)
    {
        std::cout << "Auto triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "on" << std::endl;
        else std::cout << "off" << std::endl;
        m_autoTriggerFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::autoTriggerFlag(void)
    {
        return m_autoTriggerFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::levelTriggerFlag(bool b)
    {
        std::cout << "Level triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "on" << std::endl;
        else std::cout << "off" << std::endl;
        m_levelTriggerFlag = b;
    }

    ///////////////////////////////////////////////////////////////////////
    bool StreamChannel::levelTriggerFlag(void)
    {
        return m_levelTriggerFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::edgeTriggerFlag(bool b)
    {
        std::cout << "Edge triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "on" << std::endl;
        else std::cout << "off" << std::endl;
        m_edgeTriggerFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::edgeTriggerFlag(void)
    {
        return m_edgeTriggerFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::noiseTriggerFlag(bool b)
    {
        std::cout << "noise triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b)
        {
            std::cout << "on" << std::endl;
            m_continuousNoisePoints = 0;          // reset the count
        }
        else std::cout << "off" << std::endl;
        m_noiseTriggerFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::noiseTriggerFlag(void)
    {
        return m_noiseTriggerFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::levelInverseFlag(bool b)
    {
        std::cout << "Level triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "inverted" << std::endl;
        else std::cout << "normal" << std::endl;
        m_levelInverseFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::levelInverseFlag(void)
    {
        return m_levelInverseFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::edgeInverseFlag(bool b)
    {
        std::cout << "Edge triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "inverted" << std::endl;
        else std::cout << "normal" << std::endl;
        m_edgeInverseFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::edgeInverseFlag(void)
    {
        return m_edgeInverseFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::noiseInverseFlag(bool b)
    {
        std::cout << "Noise triggering for channel "<< m_pulseRec.channel() << " is ";
        if (b) std::cout << "inverted" << std::endl;
        else std::cout << "normal" << std::endl;
        m_noiseInverseFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::noiseInverseFlag(void)
    {
        return m_noiseInverseFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::decimateLevel(unsigned short int val)
    {
        m_decimateLevel = val;
    }

    /////////////////////////////////////////////////////////////////////////////
    unsigned short int StreamChannel::decimateLevel(void)
    {
        return m_decimateLevel;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::decimateFlag(void)
    {
        return m_decimateFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::decimateFlag( bool b)
    {
        //if (b) std::cout << "Enabling";
        //else std::cout << "Disabling";
        //std::cout << " decimation for channel "<<m_pulseRec.channel()<< std::endl;
        m_decimateFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::decimateAvgFlag(void)
    {
        return m_decimateAvgFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::decimateAvgFlag(bool b)
    {
        //if (b) std::cout << "Enabling";
        //else std::cout << "Disabling";
        //std::cout << " decimation averaging for channel "<<m_pulseRec.channel()<< std::endl;

        m_decimateAvgFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    double StreamChannel::effectiveSampRate ( void )
    {
        if (!m_init) return 0.;

        double sampleRate = 1./((double) m_pulseRec.deltat());
        if (m_decimateFlag) sampleRate /= ((double) m_decimateLevel);

        return sampleRate;
    }

    /////////////////////////////////////////////////////////////////////////////
    float StreamChannel::mixLevel(void)
    {
        return m_mixLevel;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::mixLevel(float f)
    {
        m_mixLevel = f;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::mixFlag(void)
    {
        return m_mixFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::mixFlag( bool b)
    {
        m_mixFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool StreamChannel::mixInversionFlag(void)
    {
        return m_mixInversionFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void StreamChannel::mixInversionFlag(bool b)
    {
        //if (b) std::cout << "Mix Polarity set to Negative for ";
        //else std::cout << "Mix Polarity set to Positive for ";
        //std::cout << " channel "<< m_pulseRec.channel() << std::endl;
        m_mixInversionFlag = b;
    }

    std::string StreamChannel::pretty_print_mixing_state(const std::string& seperator, const std::string& prefix, const std::string& postfix)
    {
        std::stringstream a;

        //        const std::string prefix="\n\t";
        a << prefix << "decimateAvgFlag: " << decimateAvgFlag() << postfix << seperator;
        a << prefix << "decimateFlag: " << decimateFlag() << postfix << seperator;
        a << prefix << "mixFlag: " << mixFlag() << postfix << seperator;
        a << prefix << "mixInversionFlag: " << mixInversionFlag() << postfix << seperator;
        a << prefix << "mixLevel: " << mixLevel() << postfix << seperator;

        return a.str();

    }
    std::string StreamChannel::pretty_print_trigger_state(const std::string& seperator, const std::string& prefix, const std::string& postfix)
    {
        std::stringstream a;

        //        const std::string prefix="\n\t";

        a << prefix << "autoThreshold: " << autoThreshold() << postfix << seperator;
        a << prefix << "autoTriggerFlag: " << autoTriggerFlag() << postfix << seperator;
        a << prefix << "decimateAvgFlag: " << decimateAvgFlag() << postfix << seperator;
        a << prefix << "decimateFlag: " << decimateFlag() << postfix << seperator;
        a << prefix << "edgeInverseFlag: " << edgeInverseFlag() << postfix << seperator;
        a << prefix << "edgeThreshold: " << edgeThreshold() << postfix << seperator;
        //        a << prefix << "edgeThresholdRaw: " << edgeThresholdRaw() << postfix << seperator;
        a << prefix << "edgeTriggerFlag: " << edgeTriggerFlag() << postfix << seperator;
        a << prefix << "groupTrigReceiver: " << groupTrigReceiver() << postfix << seperator;
        a << prefix << "groupTrigSource: " << groupTrigSource() << postfix << seperator;
        //        a << prefix << "init: " << init() << postfix << seperator;
        a << prefix << "levelInverseFlag: " << levelInverseFlag() << postfix << seperator;
        a << prefix << "levelThreshold: " << levelThreshold() << postfix << seperator;
        // a << prefix << "levelThresholdRaw: " << levelThresholdRaw() << postfix << seperator;
        a << prefix << "levelTriggerFlag: " << levelTriggerFlag() << postfix << seperator;
        //        a << prefix << "maxSamples: " << maxSamples() << postfix << seperator;
        //        a << prefix << "mixFlag: " << mixFlag() << postfix << seperator;
        //        a << prefix << "mixInversionFlag: " << mixInversionFlag() << postfix << seperator;
        //        a << prefix << "mixLevel: " << mixLevel() << postfix << seperator;
        a << prefix << "noiseInverseFlag: " << noiseInverseFlag() << postfix << seperator;
        a << prefix << "noiseThreshold: " << noiseThreshold() << postfix << seperator;
        //        a << prefix << "noiseThresholdRaw: " << noiseThresholdRaw() << postfix << seperator;
        a << prefix << "noiseTriggerFlag: " << noiseTriggerFlag() << postfix << seperator;
        //        a << prefix << "numTriggers: " << numTriggers() << postfix << seperator;
        a << prefix << "pretrig: " << pretrig() << postfix << seperator;
        //        a << prefix << "pulseAnalysisLock: " << pulseAnalysisLock() << postfix << seperator;
        //        a << prefix << "pulseRec: " << _pulseRec() << postfix << seperator;
        a << prefix << "triggerRateIntegrationTime: " << triggerRateIntegrationTime() << postfix << seperator;
        // a << prefix << "triggerRateLock: " << triggerRateLock() << postfix << seperator;

        return a.str();

    }

    int StreamChannel::set_parameter(const std::string& parameter, const std::string& value)
    {

        //        if (parameter== "autoThreshold") gsd::str::string_to_val(value,m_autoThreshold);

        if (parameter== "autoThreshold") set_parameter_via_function_pointer(value,&StreamChannel::autoThreshold);
        else if (parameter== "autoTriggerFlag") set_parameter_via_function_pointer(value,&StreamChannel::autoTriggerFlag);
        else if (parameter== "decimateAvgFlag") set_parameter_via_function_pointer(value,&StreamChannel::decimateAvgFlag);
        else if (parameter== "decimateFlag") set_parameter_via_function_pointer(value,&StreamChannel::decimateFlag);
        else if (parameter== "edgeInverseFlag") set_parameter_via_function_pointer(value,&StreamChannel::edgeInverseFlag);
        else if (parameter== "edgeThreshold") set_parameter_via_function_pointer(value,&StreamChannel::edgeThreshold);
        //        else if (parameter== "edgeThresholdRaw") set_parameter_via_function_pointer(value,&StreamChannel::edgeThresholdRaw);
        else if (parameter== "edgeTriggerFlag") set_parameter_via_function_pointer(value,&StreamChannel::edgeTriggerFlag);
        else if (parameter== "groupTrigReceiver") set_parameter_via_function_pointer(value,&StreamChannel::groupTrigReceiver);
        else if (parameter== "groupTrigSource") set_parameter_via_function_pointer(value,&StreamChannel::groupTrigSource);
        //  else if (parameter== "init") set_parameter_via_function_pointer(value,&StreamChannel::init);
        else if (parameter== "levelInverseFlag") set_parameter_via_function_pointer(value,&StreamChannel::levelInverseFlag);
        else if (parameter== "levelThreshold") set_parameter_via_function_pointer(value,&StreamChannel::levelThreshold);
        //        else if (parameter== "levelThresholdRaw") set_parameter_via_function_pointer(value,&StreamChannel::levelThresholdRaw);
        else if (parameter== "levelTriggerFlag") set_parameter_via_function_pointer(value,&StreamChannel::levelTriggerFlag);
        //     else if (parameter== "maxSamples") set_parameter_via_function_pointer(value,&StreamChannel::maxSamples);
        else if (parameter== "mixFlag") set_parameter_via_function_pointer(value,&StreamChannel::mixFlag);
        else if (parameter== "mixInversionFlag") set_parameter_via_function_pointer(value,&StreamChannel::mixInversionFlag);
        else if (parameter== "mixLevel") set_parameter_via_function_pointer(value,&StreamChannel::mixLevel);
        else if (parameter== "noiseInverseFlag") set_parameter_via_function_pointer(value,&StreamChannel::noiseInverseFlag);
        else if (parameter== "noiseThreshold") set_parameter_via_function_pointer(value,&StreamChannel::noiseThreshold);
        //        else if (parameter== "noiseThresholdRaw") set_parameter_via_function_pointer(value,&StreamChannel::noiseThresholdRaw);
        else if (parameter== "noiseTriggerFlag") set_parameter_via_function_pointer(value,&StreamChannel::noiseTriggerFlag);
        //        else if (parameter== "numTriggers") set_parameter_via_function_pointer(value,&StreamChannel::numTriggers);
        else if (parameter== "pretrig") set_parameter_via_function_pointer(value,&StreamChannel::pretrig);
        //        else if (parameter== "pulseAnalysisLock") set_parameter_via_function_pointer(value,&StreamChannel::pulseAnalysisLock);
        //        else if (parameter== "pulseRec")  _pulseRec);
        else if (parameter== "triggerRateIntegrationTime") set_parameter_via_function_pointer(value,&StreamChannel::triggerRateIntegrationTime);
        // else if (parameter== "triggerRateLock") set_parameter_via_function_pointer(value,&StreamChannel::triggerRateLock);

        else
        {

            std::cout << "\n Warning, unknown parameter" << parameter;
            std::cout << "in StreamChannel::set_parameter";
            std::cout.flush();
            return -1;
        }

        return 0;
    }
}                                                 // end of namespace


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
