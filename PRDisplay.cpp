//plot->ingestTrace(client->latestTriggeredPulse[channel]);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "PRDisplay.h"

////////////////////////////////////////////////////////////////////////////////
PRDisplay::PRDisplay (xdaq::Client* c)
{

    // setup the initial channel display

    // copy over the client pointer
    client = c;

    // setup the window
    setup();

    // char buffer
    char buf[256];

    // some client info
    // note, we use streams per column instead of MuxRows b/c ndfb has two streams
    // per row and rocket server has 1 stream per row
    int columns = client->nMuxCols();
    int streamsPerColumn = client->nDatatStreams() / columns;

    // add the channels
    for (unsigned int i=0; i<INDEPENDENT_TRACES; i++)
    {

        channelSelector[i]->value(0);
        traceColorSelector[i]->value(i%8);

        for (unsigned int j=0; j<client->nDatatStreams(); j++)
        {
            sprintf (buf,"%d",j);
            channelSelector[i]->add(buf,0,(Fl_Callback*) handleEvent, (void*) this);
        }
    }

    // add quick select by even or feedback
    if (client->serverType() == NDFB)
    {
        int stream;
        stream=0;
        while (stream < streamsPerColumn * columns)
        {
            quickStreamVector.push_back(stream);
            stream += 2;

            if (quickStreamVector.size() == INDEPENDENT_TRACES)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }

            if (stream >= streamsPerColumn * columns)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }
        }

        stream=1;
        while (stream < streamsPerColumn * columns)
        {
            quickStreamVector.push_back(stream);
            stream += 2;

            if (quickStreamVector.size() == INDEPENDENT_TRACES)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }

            if (stream >= streamsPerColumn * columns)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }
        }

    }

    // add the quick select by column
    for (int i=0; i<columns; i++)
    {
        int stream;

        // straight streams
        stream=0;

        while (stream < streamsPerColumn)
        {
            quickStreamVector.push_back(stream + (i*streamsPerColumn));
            stream++;

            if (quickStreamVector.size() == INDEPENDENT_TRACES)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }

            if (stream >= streamsPerColumn)
            {
                buildStreamString();
                quickStreamVector.clear();
                continue;
            }
        }

        // ndfb error and feedback
        if (client->serverType() == NDFB)
        {
            // error
            stream=0;
            while (stream < streamsPerColumn)
            {
                quickStreamVector.push_back(stream + (i*streamsPerColumn));
                stream += 2;

                if (quickStreamVector.size() == INDEPENDENT_TRACES)
                {
                    buildStreamString();
                    quickStreamVector.clear();
                    continue;
                }

                if (stream >= streamsPerColumn)
                {
                    buildStreamString();
                    quickStreamVector.clear();
                    continue;
                }
            }

            // feedback
            stream=1;
            while (stream < streamsPerColumn)
            {
                quickStreamVector.push_back(stream + (i*streamsPerColumn));
                stream += 2;

                if (quickStreamVector.size() == INDEPENDENT_TRACES)
                {
                    buildStreamString();
                    quickStreamVector.clear();
                    continue;
                }

                if (stream >= streamsPerColumn)
                {
                    buildStreamString();
                    quickStreamVector.clear();
                    continue;
                }
            }
        }

    }

    // push the plot selector button to refresh everything
    plotSelector->value(0);                       // set the default value to the 0th element (normal)
    handleEvent(plotSelector);

    // hide the window
    window->hide();
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::buildStreamString (void)
{
    char output[256];
    output[0] = '\0';
    std::vector<unsigned int>::iterator streamIterator = quickStreamVector.begin();
    for (unsigned int i=0; i<quickStreamVector.size(); i++, streamIterator++)
    {
                                                  // build up the string
        sprintf(output,"%s%d,",output,*streamIterator);
        fflush(0);
    }
    output[strlen(output)-1] = '\0';              // remove the last comma

    quickSelector->add(output,0,(Fl_Callback*) handleEvent, (void*) this);
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::init ()
{
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::show ()
{
    window->show();
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::stop ()
{
    // kill this main window
    window->hide();
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::setup ()
{
    Fl_Widget* w;
    int bigx = 1000;
    int bigy = 400;
    int xbuffer=5, ybuffer=5;
    int xsize0,xsize1,xsize2, xsize3, xsize4;
    int ysize0,ysize1;
    int x[10];
    int y[INDEPENDENT_TRACES + 3];

    window = new Fl_Double_Window(bigx,bigy, "Plot Window");
    // set the current user data (void *) argument to the instance pointer
    window->user_data((void*)(this));

    // position the window on screen
    window->resizable(window);

    // begin the main window group
    window->begin();
    {
        xsize0 = 50;                              // channel selector
        xsize1 = 100;                             // color selector
        xsize2 = 100;                             // plot selector
        xsize4 = xsize1 + xsize0 + xbuffer;
        ysize0 = 25;                              // standard size
        ysize1 = (bigy - 2*ybuffer);              // plotWindow height

        x[0] = xbuffer+xbuffer;                   // channel
        x[1] = x[0] + xsize0 + xbuffer;           // color
        x[2] = x[0];                              // plot type
        x[3] = x[1] + xsize2 + xbuffer;           // plot itself
                                                  // plotWindow width
        xsize3 = bigx - (x[1] + xsize1 + xbuffer) - xbuffer;

        y[0] = ybuffer;                           // top
        y[1] = y[0] + 2*ysize0 + ybuffer;
        y[2] = bigy - ysize0-ybuffer;

        for (unsigned int i=0; i<INDEPENDENT_TRACES; i++)
        {
                                                  // top of top plot choice
            y[i+3] = y[1] + (i)*(ysize0 + ybuffer);
        }

        // what kind of plot the user wants to see
        Fl_Menu_Item plotPopup[] =
        {
            {"Standard",   0, (Fl_Callback*) handleEvent, (void*) this},
            {"Derivitive", 0, (Fl_Callback*) handleEvent, (void*) this},
            {"FFT",        0, (Fl_Callback*) handleEvent, (void*) this},
            {"Noise",      0, (Fl_Callback*) handleEvent, (void*) this},
            {0}
        };
        w = plotSelector = new Fl_Choice (x[0],y[0],xsize2,ysize0,"Plot");
        plotSelector->copy(plotPopup,NULL);
        w->align(FL_ALIGN_RIGHT);

        // the channel selectors
        w  = new Fl_Box (x[0],y[1],xsize0,1,"Channel");
        w->align(FL_ALIGN_TOP);

        Fl_Menu_Item dashPopup[] =
        {
            {"-", 0,(Fl_Callback*) handleEvent, (void*) this},
            {0}
        };

        for (unsigned int i=0; i< INDEPENDENT_TRACES; i++)
        {
            w = channelSelector[i] = new Fl_Choice (x[0],y[i+3],xsize0,ysize0);
            channelSelector[i]->copy(dashPopup,NULL);
            w->align(FL_ALIGN_TOP);
        }

        // color selector
        Fl_Menu_Item colorPopup[] =
        {
            {"Black",      0, NULL, (void*) this},
            {"Red",        0, NULL, (void*) this},
            {"Blue",       0, NULL, (void*) this},
            {"Green",      0, NULL, (void*) this},
            {"Dark Red",   0, NULL, (void*) this},
            {"Dark Blue",  0, NULL, (void*) this},
            {"Dark Green", 0, NULL, (void*) this},
            {"Dark Cyan",  0, NULL, (void*) this},
            {0}
        };
        for (unsigned int i=0; i< INDEPENDENT_TRACES; i++)
        {
            w = traceColorSelector[i] = new Fl_Choice (x[1],y[i+3],xsize1, ysize0);
            traceColorSelector[i]->copy(colorPopup,NULL);
            w->align(FL_ALIGN_TOP);
        }

        // quick choice
        w = quickSelector = new Fl_Choice (x[0],y[2],xsize4,ysize0,"Quick Select");
        quickSelector->copy(dashPopup,NULL);
        w->align(FL_ALIGN_TOP);

        // the actual plotter widget
        plot = new plotWindow (x[3],y[0]-ybuffer,xsize3,ysize1);

    }   window->end();                            // end main window grouping

    // now that we are out of the group, init the plot
    plot->init();

}


/////////////////////////////////////////////////////////////////////////////
void PRDisplay::handleEvent(Fl_Widget *w,void *v)
{
    ((PRDisplay *)v)->handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void PRDisplay::handleEvent(Fl_Widget *w)
{
    // plot widgets
    for (unsigned int i=0; i< INDEPENDENT_TRACES; i++)
    {
        if (w==channelSelector[i])
        {
            //	channel[i] = channelSelector[i]->value() - 1; // remember that selection 1 is the '-'
            std::cout << "setting channel selector to: " << channelSelector[i]->value() - 1 << std::endl;
            if (!plot->xManual()) plot->resetPlotXMax();
            if (!plot->yManual()) plot->resetPlotYMax();
            return;
        }
    }

    if (w==quickSelector)
    {
        // reset everything
        for (unsigned int i=0; i<INDEPENDENT_TRACES; i++)
        {
            channelSelector[i]->value(0);
            handleEvent(channelSelector[i]);
        }

        char input[1028];
        sprintf(input,"%s",quickSelector->text());
        if (strlen(input) != 0)
        {
            int i = 0;
            int ret;
            int plotChannel=0;                    // which channel selector to put it in
            while (i<1024)
            {
                int j;
                ret = sscanf(&input[i],"%d",&j);

                if (ret != 0)
                {
                    if (j<0)
                    {
                        printf ("\tinvalid channel!!!\n");
                    }
                    else
                    {
                        if (plotChannel >= INDEPENDENT_TRACES)
                        {
                            printf ("you are trying to change the %dth channel selector, but you only have %d independent channels plotted\n",plotChannel+1,INDEPENDENT_TRACES);
                        }
                        channelSelector[plotChannel]->value(j+1);
                        handleEvent(channelSelector[plotChannel]);
                        plotChannel++;

                    }
                }

                char *pos = strstr(&input[i],",");
                if (pos == NULL)
                {
                    break;
                }
                else
                {
                    i = pos - &input[0]+1;
                }
            }
        }
        return;
    }

    if (w==plotSelector)
    {
        switch (plotSelector->value())
        {
            case 0:                               // regular plot
            {
                plot->xScale(1000000.);
                plot->yScale(1000);
                plot->xTickLabel("Time");
                plot->yTickLabel("Voltage");
                plot->xTickUnit("uS");
                plot->yTickUnit("mV");
                plot->xLog(false);
                plot->yLog(false);
                plot->xLinLogVisibleFlag(false);
                plot->yLinLogVisibleFlag(false);
                break;
            }
            case 1:                               // derivitive plot
            {
                plot->xScale(1000000.);
                plot->yScale(1./1000.);
                plot->xTickLabel("Time");
                plot->yTickLabel("d_Volts/d_Time");
                plot->xTickUnit("uS");
                plot->yTickUnit("V/mS");
                plot->xLog(false);
                plot->xLog(false);
                plot->xLinLogVisibleFlag(false);
                plot->yLinLogVisibleFlag(false);
                break;
            }
            case 2:                               // fft plot
            {
                plot->xScale(1);
                plot->yScale(1e6);
                plot->yTickLabel("Magnitude");
                plot->xTickLabel("Frequency");
                plot->yTickUnit("uV/Hz^\xbd");
                plot->xTickUnit("Hz");
                plot->xLog(true);
                plot->yLog(true);
                plot->xLinLogVisibleFlag(true);
                plot->yLinLogVisibleFlag(true);
                break;
            }
            case 3:                               // noise plot
            {
                plot->xScale(1000000);
                plot->yScale(1000);
                plot->xTickLabel("Time");
                plot->yTickLabel("Voltage");
                plot->xTickUnit("uS");
                plot->yTickUnit("mV");
                plot->xLog(false);
                plot->yLog(false);
                plot->xLinLogVisibleFlag(false);
                plot->yLinLogVisibleFlag(false);
                break;
            }

            default:
            {
                std::cout << "Plot error... no plot type selected" << std::endl;
                break;
            }
        }

        // reset the plot maxes for the two axis
        plot->resetPlotXMax();
        plot->resetPlotYMax();

        return;
    }

    std::cout << "You didn't handle that very well, now did you?" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::userRefresh ()
{

}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::loopRefresh ()
{
    if (!window->visible()) return;

    // some pointers
    double *x=NULL;
    double *y=NULL;
    unsigned int n = 0;
    int id;

    // loop through all traces
    for (unsigned int i=0; i<INDEPENDENT_TRACES; i++)
    {

                                                  // remember that '0" is the '-' channel
        int channel = channelSelector[i]->value() - 1;

        // this is where you pick what gets splashed on the plot canvas
        if (channel != -1)
        {
            switch (plotSelector->value())
            {
                case 0:                           // regular plot
                {
                    client->latestTriggeredPulseLock[channel].getLock();

                    // request a copy of the data
                    xdaq::XPulseRec *PR = client->latestTriggeredPulse[channel];
                    if (PR != NULL)
                    {

                        // request some data
                        PR->requestDataCopy(&x,&y,&n);

                        // pass that to the plotter and tell it to keep it
                        id = plot->ingestAndKeepData(x,y,n);

                        if (id == -1) break;

                        // add the trigger threshold line
                        if (PR->triggerType() == xdaq::LEVEL)
                        {
                            plot->addHorLine(id,PR->threshold());
                        }

                        // add the color
                        Fl_Color color;
                        switch (traceColorSelector[i]->value())
                        {
                            case 0:
                                color = FL_BLACK;
                                break;
                            case 1:
                                color = FL_RED;
                                break;
                            case 2:
                                color = FL_BLUE;
                                break;
                            case 3:
                                color = FL_GREEN;
                                break;
                            case 4:
                                color = FL_DARK_RED;
                                break;
                            case 5:
                                color = FL_DARK_BLUE;
                                break;
                            case 6:
                                color = FL_DARK_GREEN;
                                break;
                            case 7:
                                color = FL_DARK_CYAN;
                                break;
                            default:
                                color = FL_BLACK;
                                break;
                        }
                        plot->color(id,color);
                    }

                    // release the lock
                    client->latestTriggeredPulseLock[channel].releaseLock();
                    break;
                }

                case 1:                           // derivitive plot
                {
                    client->latestTriggeredPulseLock[channel].getLock();

                    // request a copy of the data
                    xdaq::XPulseRec *PR = client->latestTriggeredPulse[channel];
                    if (PR != NULL)
                    {

                        // request some data
                        PR->requestDataDerivitiveCopy(&x,&y,&n);

                        // pass that to the plotter and tell it to keep it
                        id = plot->ingestAndKeepData(x,y,n);

                        if (id == -1) break;

                        // add the trigger threshold line
                        if (PR->triggerType() == xdaq::EDGE)
                        {
                            plot->addHorLine(id,PR->threshold());
                        }

                        // add the color
                        Fl_Color color;
                        switch (traceColorSelector[i]->value())
                        {
                            case 0:
                                color = FL_BLACK;
                                break;
                            case 1:
                                color = FL_RED;
                                break;
                            case 2:
                                color = FL_BLUE;
                                break;
                            case 3:
                                color = FL_GREEN;
                                break;
                            case 4:
                                color = FL_DARK_RED;
                                break;
                            case 5:
                                color = FL_DARK_BLUE;
                                break;
                            case 6:
                                color = FL_DARK_GREEN;
                                break;
                            case 7:
                                color = FL_DARK_CYAN;
                                break;
                            default:
                                color = FL_BLACK;
                                break;
                        }
                        plot->color(id,color);

                    }

                    client->latestTriggeredPulseLock[channel].releaseLock();
                    break;
                }

                case 2:                           // fft plot
                {
                    double* x = client->noiseRecordFFT[channel]->plotX();
                    double* y = client->noiseRecordFFT[channel]->plotY();
                    int     n = client->noiseRecordFFT[channel]->plotN();
                    id = plot->ingestAndCopyData(x,y,n);

                    if (id == -1) break;

                    // add the color
                    Fl_Color color;
                    switch (traceColorSelector[i]->value())
                    {
                        case 0:
                            color = FL_BLACK;
                            break;
                        case 1:
                            color = FL_RED;
                            break;
                        case 2:
                            color = FL_BLUE;
                            break;
                        case 3:
                            color = FL_GREEN;
                            break;
                        case 4:
                            color = FL_DARK_RED;
                            break;
                        case 5:
                            color = FL_DARK_BLUE;
                            break;
                        case 6:
                            color = FL_DARK_GREEN;
                            break;
                        case 7:
                            color = FL_DARK_CYAN;
                            break;
                        default:
                            color = FL_BLACK;
                            break;
                    }
                    plot->color(id,color);

                    break;
                }

                case 3:                           // noise plot
                {
                    client->latestNoisePulseLock[channel].getLock();
                    // request a copy of the data
                    xdaq::XPulseRec *PR = client->latestNoisePulse[channel];
                    if (PR != NULL)
                    {
                        // request the data
                        PR->requestDataCopy(&x,&y,&n);

                        // pass that to the plotter and tell it to keep it
                        id = plot->ingestAndKeepData(x,y,n);

                        if (id == -1) break;

                        // add the trigger threshold line
                        if (PR->triggerType() == xdaq::NOISE)
                        {
                            plot->addHorLine(id,PR->threshold());
                        }

                        // add the color
                        Fl_Color color;
                        switch (traceColorSelector[i]->value())
                        {
                            case 0:
                                color = FL_BLACK;
                                break;
                            case 1:
                                color = FL_RED;
                                break;
                            case 2:
                                color = FL_BLUE;
                                break;
                            case 3:
                                color = FL_GREEN;
                                break;
                            case 4:
                                color = FL_DARK_RED;
                                break;
                            case 5:
                                color = FL_DARK_BLUE;
                                break;
                            case 6:
                                color = FL_DARK_GREEN;
                                break;
                            case 7:
                                color = FL_DARK_CYAN;
                                break;
                            default:
                                color = FL_BLACK;
                                break;
                        }
                        plot->color(id,color);

                    }

                    // release the lock
                    client->latestNoisePulseLock[channel].releaseLock();
                    break;
                }

                default:
                {
                    std::cout << "Plot error... no plot type selected" << std::endl;
                    break;
                }
            }
        }

    }

    // now that you have sent all the data to the plots, stick it to the screen
    plot->loopRefresh();

}


////////////////////////////////////////////////////////////////////////////////
void PRDisplay::position (int x, int y)
{
    window->position(x,y);
}


////////////////////////////////////////////////////////////////////////////////
// boring encapsulation
////////////////////////////////////////////////////////////////////////////////
bool PRDisplay::visible ()                        // only done every once in a while
{
    if (window == NULL)
    {
        printf ("there isn't a window variable dummy\n"); fflush(0);
    }
    return window->visible();
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
