////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "PulseAnalysisDisplay.h"

////////////////////////////////////////////////////////////////////////////////
PulseAnalysisDisplay::PulseAnalysisDisplay (xdaq::Client* c)
{

    // copy over the client pointer
    client = c;

    // setup the window
    setup();

    // add the channels
    char buf[8];
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

    // push the plot selector button to refresh everything
    handleEvent(plotSelector);

    // hide the window
    window->hide();
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::init ()
{
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::show ()
{
    window->show();
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::hide ()
{
    window->hide();
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::stop ()
{
    hide();
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::setup ()
{
    Fl_Widget* w;
    int bigx = 1000;
    int bigy = 400;
    int xbuffer=5, ybuffer=5;
    int xsize0,xsize1,xsize2, xsize3;
    int ysize0,ysize1;
    int x[5];
    int y[INDEPENDENT_TRACES + 3];

    window = new Fl_Double_Window(bigx,bigy, "Pulse Analysis Window");
    // set the current user data (void *) argument to the instance pointer
    window->user_data((void*)(this));

    // position the window on screen
    window->resizable(window);

    // begin the main window group
    window->begin();
    {
        xsize0 = 50;                              // channel selector
        xsize1 = 100;                             // color selector
        xsize2 = xsize1 + xsize0 + xbuffer;       // plot selector
        ysize0 = 25;                              // standard size
        ysize1 = (bigy - 2*ybuffer);              // plotWindow height

        x[0] = xbuffer+xbuffer;                   // channel
        x[1] = x[0] + xsize0 + xbuffer;           // color
        x[2] = x[0];                              // plot type
        x[3] = x[1] + xsize1 + xbuffer;           // plot itself
                                                  // plotWindow width
        xsize3 = bigx - (x[1] + xsize1 + xbuffer) - xbuffer;

        y[0] = ybuffer;                           // plot selector
        y[1] = y[1] + ysize0 + ybuffer;           // analysis enable
        y[2] = y[1] + 2*ysize0 + ybuffer;         // label
        for (unsigned int i=0; i<INDEPENDENT_TRACES; i++)
        {
            y[3+i] = y[2+i] + ysize0 + ybuffer;   // top of top plot choice
        }

        // what kind of plot the user wants to see
        Fl_Menu_Item plotPopup[] =
        {
            {"Pulse Height", 0, (Fl_Callback*) handleEvent, (void*) this},
            {"Baseline",     0, (Fl_Callback*) handleEvent, (void*) this},
            {0}
        };
        w = plotSelector = new Fl_Choice (x[0],y[1],xsize2,ysize0,"");
        plotSelector->copy(plotPopup,NULL);
        w->align(FL_ALIGN_RIGHT);

        // enable analysis button
        w = performAnalysisFlagButton = new Fl_Light_Button(x[0],y[0],xsize2,ysize0,"Enable Analysis");
        w->callback((Fl_Callback*) handleEvent, (void*) this);

        // the channel selectors
        w  = new Fl_Box (x[0],y[2],xsize0,0,"Channel");
        w->align(FL_ALIGN_TOP);

        w  = new Fl_Box (x[1],y[2],xsize0,0,"Color");
        w->align(FL_ALIGN_TOP);

        Fl_Menu_Item topChanPopup[] =
        {
            {"-", 0,(Fl_Callback*) handleEvent, (void*) this},
            {0}
        };

        for (unsigned int i=0; i< INDEPENDENT_TRACES; i++)
        {
            w = channelSelector[i] = new Fl_Choice (x[0],y[i+2],xsize0,ysize0);
            channelSelector[i]->copy(topChanPopup,NULL);
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
            w = traceColorSelector[i] = new Fl_Choice (x[1],y[i+2],xsize1, ysize0);
            traceColorSelector[i]->copy(colorPopup,NULL);
            w->align(FL_ALIGN_TOP);
        }

        // the actual plotter widget
        plot = new plotWindow (x[3],y[0],xsize3,ysize1);

        // set some flags
        plot->connectedLineFlag(false);
        plot->xDisplayMethod (3);                 // set x to auto scroll
        plot->yDisplayMethod (1);                 // set y to maximum

    }   window->end();                            // end main window grouping

    // now that we are out of the group, init the plot
    plot->init();

}


/////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::handleEvent(Fl_Widget *w,void *v)
{
    ((PulseAnalysisDisplay *)v)->handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::handleEvent(Fl_Widget *w)
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

    if (w==plotSelector)
    {
        switch (plotSelector->value())
        {
            case 0:                               // regular plot
            {
                plot->xScale(1.);
                plot->yScale(1000.);
                plot->xTickLabel("Time");
                plot->yTickLabel("Pulse Height");
                plot->xTickUnit("S");
                plot->yTickUnit("mV");
                plot->xLog(false);
                plot->yLog(false);
                break;
            }
            case 1:                               // derivitive plot
            {
                plot->xScale(1.);
                plot->yScale(1000.);
                plot->xTickLabel("Time");
                plot->yTickLabel("Baseline");
                plot->xTickUnit("S");
                plot->yTickUnit("mV");
                plot->xLog(false);
                plot->xLog(false);
                break;
            }

            default:
            {
                std::cout << "Plot error... no plot type selected" << std::endl;
                break;
            }
        }

        plot->resetPlotXMax();
        plot->resetPlotYMax();

        return;
    }

    if (w==performAnalysisFlagButton)
    {
        bool b = performAnalysisFlagButton->value();
        for (unsigned int chan = 0; chan < client->nDatatStreams(); chan++)
        {
            client->performAnalysisFlag(chan,b);
        }
        return;
    }

    std::cout << "You didn't handle that very well, now did you?" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::userRefresh ()
{

}


////////////////////////////////////////////////////////////////////////////////
inline Fl_Color getColor (int value)
{
    Fl_Color color;
    switch (value)
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

    return color;
}


////////////////////////////////////////////////////////////////////////////////
void PulseAnalysisDisplay::loopRefresh ()
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
                case 0:                           // pulse height vs time
                {

                    // request a copy of the data
                    xdaq::StreamChannel *SD = client->streamData[channel];
                    if (SD != NULL)
                    {

                        // request some data
                        SD->requestPulseHeightCopy(&x,&y,&n);

                        // test for no data
                        if (n==0) break;

                        // pass that to the plotter and tell it to keep it
                        id = plot->ingestAndKeepData(x,y,n);

                        if (id == -1) break;

                        // add the color
                        Fl_Color color = getColor (traceColorSelector[i]->value());
                        plot->color(id,color);
                    }

                    // release the lock
                    client->latestTriggeredPulseLock[channel].releaseLock();
                    break;
                }

                case 1:                           // baseline vs time
                {
                    xdaq::StreamChannel *SD = client->streamData[channel];
                    if (SD != NULL)
                    {

                        // request some data
                        SD->requestBaselineCopy(&x,&y,&n);

                        // test for no data
                        if (n==0) break;

                        // pass that to the plotter and tell it to keep it
                        id = plot->ingestAndKeepData(x,y,n);

                        if (id == -1) break;

                        // add the color
                        Fl_Color color = getColor (traceColorSelector[i]->value());
                        plot->color(id,color);
                    }

                    client->latestTriggeredPulseLock[channel].releaseLock();
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
void PulseAnalysisDisplay::position (int x, int y)
{
    window->position(x,y);
}


////////////////////////////////////////////////////////////////////////////////
// boring encapsulation
////////////////////////////////////////////////////////////////////////////////
bool PulseAnalysisDisplay::visible ()             // only done every once in a while
{
    return window->visible();
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
