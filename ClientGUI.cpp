////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "ClientGUI.h"

////////////////////////////////////////////////////////////////////////////////
ClientGUI::ClientGUI ()
{
    client = NULL;

    groupTriggerWindow            = NULL;
    triggerRateWindow             = NULL;
    triggerControlWindow          = NULL;
    streamChannelWindow           = NULL;
    mixControlWindow              = NULL;
    pulseAnaylsisWindow           = NULL;
    fileWriterStreamControlWindow = NULL;

    mainWindow_setup();

    stupidFLTKTimeoutHack = false;

    writerButtonStatusFlag = false;
}


////////////////////////////////////////////////////////////////////////////////
ClientGUI::~ClientGUI ()
{
    std::cout << "killing the gui...";
    stopGUI();
    std::cout << "done!"<<std::endl;

    client = NULL;
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::initGUI ()
{
    mainWindow_userRefresh();
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::startGUI ()
{
    mainWindow->show();
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::stopGUI ()
{

    // kill any children windows
    if (groupTriggerWindow            != NULL) groupTriggerWindow->hide();
    if (triggerRateWindow             != NULL) triggerRateWindow->hide();
    if (triggerControlWindow          != NULL) triggerControlWindow->hide();
    if (streamChannelWindow           != NULL) streamChannelWindow->hide();
    if (mixControlWindow              != NULL) mixControlWindow->hide();
    if (pulseAnaylsisWindow           != NULL) pulseAnaylsisWindow->hide();
    if (fileWriterStreamControlWindow != NULL) fileWriterStreamControlWindow->hide();

    plotWindow_removeAll();

    // stop the updating
    stopLoopRefresh();

    // kill this main window
    mainWindow->hide();

}


///////////////////////////////////////////////////////////////////////////////
void ClientGUI::startLoopRefresh ()
{
    //start the GUI eventloop
    Fl::add_timeout(0.0,loopRefresh, (void*)this);
    stupidFLTKTimeoutHack = true;
    loopCounter = 0;
}


///////////////////////////////////////////////////////////////////////////////
void ClientGUI::loopRefresh (void *v)
{
    ClientGUI *GUI;
    GUI = (ClientGUI *) v;

    // check if we are still valid
    if (!GUI->client->server->commPipe->valid())
    {
        std::cout << "GUI senses that you got booted from the server!!" << std::endl; fflush(0);

        // call the gui to be in disconnected state
        // note: the gui disconnect should kill the refresh loop so just keep on keeping on
        GUI->connect(false);
    }

    // call the main window's event loop
    GUI->mainWindow_loopRefresh ();

    // call the trigger rate window's event loop
    GUI->triggerRateWindow_loopRefresh ();

    // get the plotter window lock
    GUI->plotterWindowsLock.getLock();

    // call the plot windows event loop
    std::deque<PRDisplay*>::iterator prd = GUI->plotterWindows.begin();
    for (unsigned int i=0; i<GUI->plotterWindows.size(); i++, prd++)
    {
        (*prd)->loopRefresh();
    }

    // release the plotter window lock
    GUI->plotterWindowsLock.releaseLock();

    // call the pulse analysis window's event loop
    GUI->pulseAnaylsisWindow->loopRefresh();

    // call this function again
    if (GUI->stupidFLTKTimeoutHack)
    {
                                                  // make sure to pass along the instance pointer!
        Fl::repeat_timeout(TIME_BETWEEN_EVENT_LOOP,loopRefresh,GUI);
    }
    else
    {
        //std::cout << "You shouldn't ever see this b/c the event should have been removed."<<std::endl;
        //std::cout << "This is a bug with FLTK" << std::endl;
    }

    GUI->loopCounter++;
}


///////////////////////////////////////////////////////////////////////////////
void ClientGUI::stopLoopRefresh ()
{

    if (Fl::has_timeout(loopRefresh, (void*)this))
    {
        Fl::remove_timeout(loopRefresh, (void*)this);
    }
    stupidFLTKTimeoutHack = false;
}


///////////////////////////////////////////////////////////////////////////////
void ClientGUI::refreshTriggerControlUntilInitialized (void *v)
{
    ClientGUI *GUI;
    GUI = (ClientGUI *) v;

    // if it's not connected, don't bother
    if (!GUI->client->connected()) return;

    // if you're not streaming, don't bother
    if (!GUI->client->streaming()) return;

    // refresh the trigger control window
    GUI->triggerControlWindow_userRefresh();

    // check to see if you need to do this again
    bool again = false;
    for (unsigned int i=0;i<GUI->client->nDatatStreams();i++)
    {
        if (GUI->client->streamDataFlag(i))
        {
            if (!GUI->client->streamData[i]->init())
            {
                again = true;
            }
        }
    }
    if (again)
    {
                                                  // make sure to pass along the instance pointer!
        Fl::repeat_timeout(.5, refreshTriggerControlUntilInitialized,GUI);
    }
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::plotWindow_add ()
{
    PRDisplay *d1 = new PRDisplay(client);

    // get the plotter window lock
    plotterWindowsLock.getLock();

    // see how many plotters are visible
    int visible = 0;
    std::deque<PRDisplay*>::iterator prd = plotterWindows.begin();

    for (unsigned int i=0; i<plotterWindows.size(); i++, prd++)
    {
        if ((*prd)->visible()) visible++;
    }

    // position the new plotter
    d1->position(mainWindow->x()+15*visible,mainWindow->y()+mainWindow->h()+15*visible);
    d1->show();

    // add the plotter to the deque
    plotterWindows.push_back(d1);

    // try to get the main window to focus
    mainWindow->show();
    mainWindow->take_focus();
    mainWindow->show();

    // release the plotter window lock
    plotterWindowsLock.releaseLock();
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::plotWindow_removeAll ()
{
    // get the plotter window lock
    plotterWindowsLock.getLock();

    std::deque<PRDisplay*>::iterator prd = plotterWindows.begin();
    for (unsigned int i=0; i<plotterWindows.size(); i++)
    {
        (*prd)->stop();
        prd++;
    }
    plotterWindows.erase(plotterWindows.begin(),plotterWindows.end());

    // get the plotter window lock
    plotterWindowsLock.releaseLock();

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::connect (bool b)
{

    if (b)
    {
        connectButton->value(true);

        // quick and dirty progress bar
        char buf[128];
        int winH = 50;
        int winW = 300;
        int posY = 0;
        int posX = 0;
        Fl_Double_Window *win = new Fl_Double_Window(posX,posY,winW,winH,"Connecting to Server");
        Fl_Progress      *progressBar;
        win->begin();
        {
            progressBar = new Fl_Progress (5,5,winW-10,winH-10,"Connecting Progress");
            progressBar->minimum(0.);
            progressBar->maximum(1.);
            progressBar->align(FL_ALIGN_INSIDE);
        }
        win->end();
        win->show();
        float f;
        do
        {
            // get the progress
            f = client->connectProgress();
            //printf ("f %f\n",f);

            // update the progress bar
            progressBar->value(f);

            // update the title
            sprintf (buf,"Connecting to Servers - %%%5.1f",100.*f);
            progressBar->copy_label(buf);

            // tell fltk to paint it NOW
            Fl::check();

            // take a little nap
            usleep (1000);

        } while (f < 1.);                         // wait until done
        win->hide();

        // check the validity of the pipe
        if (!client->server->commPipe->valid())
        {
            connect(false);
            return;
        }

        // change the button
        sprintf (buf,"Connect to Server (%s)",client->versionString());
        connectButton->copy_label(buf);

        // prepare the file writer stream control subwindow
        fileWriterStreamControlWindow_setup();
        fileWriterStreamControlWindow_userRefresh();

        // prepare the trigger control subwindow
        triggerControlWindow_setup();
        triggerControlWindow_userRefresh();

        // prepare the stream channel chooser subwindow
        streamChannelWindow_setup();
        streamChannelWindow_userRefresh();

        // prepare the group trigger subwindow
        groupTriggerWindow_setup ();
        groupTriggerWindow_userRefresh();

        // prepare the mix control subwindow
        mixControlWindow_setup();
        mixControlWindow_userRefresh();

        // prepare the trigger rate control subwindow
        triggerRateWindow_setup ();
        triggerRateWindow_userRefresh();

        /*
         // add channel information to the these to the main window's GUI
         for (unsigned int j=0; j<client->nDatatStreams(); j++) {
         sprintf (buf,"Chan %d",j);
         decimateChannelSelector->add(buf,0,(Fl_Callback*) mainWindow_handleEvent, (void*) this);
         }
         */

        // setup the PRDisplays
        plotWindow_add();                         // one to start with

        // setup the pulse analysis display
        pulseAnaylsisWindow = new PulseAnalysisDisplay (client);
        pulseAnaylsisWindow->hide();

        // refresh the main window
        mainWindow_userRefresh();

        // activate the groups
        streamGroup->activate();
        plotGroup->activate();
        fileGroup->activate();

        // deactivate the buttons
        showGroupTriggerWindowButton->deactivate();
        showTriggerControlWindowButton->deactivate();
        showMixControlWindowButton->deactivate();
        showTriggerRateWindowButton->deactivate();
        showPulseAnalysisWindowButton->deactivate();
        rewindDataFileButton->deactivate();

    }
    else
    {

        // make sure stream false is set
        stream(false);

        connectButton->value(false);

        // change the button
        char buf[128];
        sprintf (buf,"Connect to Server");
        connectButton->copy_label(buf);

        // deactivate the groups
        streamGroup->deactivate();
        plotGroup->deactivate();
        fileGroup->deactivate();

        // hide windows
        if (triggerRateWindow != NULL) triggerRateWindow->hide();
        if (mixControlWindow != NULL) mixControlWindow->hide();
        if (triggerControlWindow != NULL) triggerControlWindow->hide();
        if (groupTriggerWindow != NULL) groupTriggerWindow->hide();

        // remove all plot windows
        plotWindow_removeAll();

    }
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::stream (bool b)
{
    if (b)
    {

        streamButton->value(true);

        // activate the buttons
        showGroupTriggerWindowButton->activate();
        showTriggerControlWindowButton->activate();
        if (client->serverType() == NDFB)
        {
            showMixControlWindowButton->activate();
        }
        showTriggerRateWindowButton->activate();
        showPulseAnalysisWindowButton->activate();

        if (client->dataSource() == RECORDED)
        {
            rewindDataFileButton->activate();
        }
        else
        {
            rewindDataFileButton->hide();
        }

        //start the recursive event loop
        startLoopRefresh();

    }
    else
    {

        // make sure the button is off
        streamButton->value(false);

        // deactivate the buttons
        showGroupTriggerWindowButton->deactivate();
        showTriggerControlWindowButton->deactivate();
        showMixControlWindowButton->deactivate();
        showPulseAnalysisWindowButton->deactivate();
        showTriggerRateWindowButton->deactivate();
        rewindDataFileButton->deactivate();

        // stop the recursive event loop
        stopLoopRefresh();
    }

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mainWindow_setup ()
{
    int menu_height=21;
    Fl_Widget* w;
    int bigx;
    int bigy = 205;
    int xbuffer=5, ybuffer=5;
    int yGroupSize[2];
    int xGroupSize[3];
    int X0,X1,X2;                                 // where the groups are
    int Y0,Y1;                                    // where the groups are

    int xsize[6];
    int ysize[6];
    int x[20];
    int y[20];
    int x0, x1, x2, x3, x4;
    int y0, y1, y2, y3;

    // yGroupSize
    yGroupSize[0] = (bigy - 3*ybuffer)/2;         // half the height
    yGroupSize[1] = bigy - 2*ybuffer;             // the full height

    //    yGroupSize[0] = yGroupSize[0]-menu_height;         // half the height
    // yGroupSize[1] = yGroupSize[1]-menu_height;             // the full height

    // x dimension stuff
    xGroupSize[0]=530;
    xGroupSize[1]=420;
    xGroupSize[2]=205;
    X0 = xbuffer;
    X1 = X0 + xGroupSize[0] + xbuffer;
    X2 = X1 + xGroupSize[1] + xbuffer;
    bigx = X2 + xGroupSize[2] + xbuffer;

    Y0 = ybuffer+menu_height;
    Y1 = Y0 + yGroupSize[0] + ybuffer;

    char buf[128];
    sprintf (buf,"Xcaldaq Client: Version %d.%d.%d",VERSION_MAJOR, VERSION_MINOR, VERSION_REALLYMINOR);
    mainWindow = new Fl_Double_Window(bigx,bigy+menu_height);
    mainWindow->copy_label(buf);

    // set the current user data (void *) argument to the instance pointer
    mainWindow->user_data((void*)(this));

    // position the window on screen
    mainWindow->position(0,2*ybuffer);

    // begin the main window group
    mainWindow->begin();
    {

        // Fl_Menu_Bar* m = new Fl_Menu_Bar(0, 0);
        //    build_menus(m,w);
        //fltk::MenuBar* m = new fltk::MenuBar(0, 0, 660, 21);
        //build_menus(m,w);

        int menu_width=bigx;

        build_menu(menu_width, menu_height);

        {                                         // server group
            serverGroup = new Fl_Group (X0,Y0,xGroupSize[0],yGroupSize[0]);
            serverGroup->box(FL_BORDER_BOX);
            serverGroup->begin();

            xsize[0] = 138;                       // port, hostname,
            xsize[1] = 50;                        // port
                                                  // connect button
            xsize[2] = 2*xsize[0]+xsize[1]+2*xbuffer;
                                                  // progress buttons
            xsize[3] = serverGroup->w() - (xsize[2] + xbuffer)-2*xbuffer;
            ysize[0] = 25;
                                                  // meters
            ysize[1] = (serverGroup->h() - 3*ybuffer)/2;

            x0 = serverGroup->x() + xbuffer;      // //server
            x1 = x0 + xsize[0] + xbuffer;         //host
            x2 = x1 + xsize[0] + xbuffer;         // port
            x3 = x2 + xsize[1] + xbuffer;         // buffer
            x4 = x3;                              // rate

            y0 = serverGroup->y() + ybuffer;
            y1 = y0 + ysize[0] + ybuffer;
            y2 = y1 + ysize[0] + ybuffer;
            y3 = y0 + ysize[1] + ybuffer;

            // 	    w = button_trigger_config_file_save = new Fl_Button (x0,y1,xsize[0],ysize[0],"Save TrigConfig");
            //             w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            // 	    w = button_trigger_config_file_load = new Fl_Button (x0,y1,xsize[0],ysize[0],"Load TrigConfig");
            //             w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = saveConfigFileButton = new Fl_Button (x0,y1,xsize[0],ysize[0],"Save NetConfig");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = hostInput = new Fl_Input (x1,y1,xsize[0],ysize[0],"Host Name");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = portInput = new Fl_Input (x2,y1,xsize[1],ysize[0],"Port");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // buffer level
            w = bufferLevelIndicator = new Fl_Progress (x3,y0,xsize[3],ysize[1],"Buffer Level");
            bufferLevelIndicator->minimum(0.);
            bufferLevelIndicator->maximum(1.);
            w->align(FL_ALIGN_INSIDE);

                                                  // rate
            w = dataRateIndicator = new Fl_Progress (x4,y3,xsize[3],ysize[1],"Data Rate");
            dataRateIndicator->minimum(0.);
            dataRateIndicator->maximum(10.);      // are we going to do better than 10M/s??? we shall see....
            dataRateIndicator->selection_color(FL_GREEN);
            w->align(FL_ALIGN_INSIDE);

            w = connectButton = new Fl_Light_Button (x0,y2,xsize[2],ysize[0]);
            connectButton->type(FL_TOGGLE_BUTTON);
            connectButton->copy_label("Connect to Server");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            serverGroup->end();
        }

        // the file writer
        {
            int ybuffer = 3;
            int xbuffer = 5;
            fileGroup = new Fl_Group (X0,Y1,xGroupSize[0],yGroupSize[0]);
            fileGroup->box(FL_BORDER_BOX);
            fileGroup->begin();

            xsize[0] = 80;                        // any of the small buttons
            xsize[1] = 2 * (xsize[0]) + xbuffer;  // formate selector
                                                  // filename
            xsize[2] = 3 * (xsize[0]) + 2 * xbuffer;
                                                  // filename ?
            xsize[3] = 4 * (xsize[0]) + 3 * xbuffer;
                                                  // go button and status
            xsize[4] = fileGroup->w() - xsize[3] - 3*xbuffer;

                                                  // button
            ysize[0] = (yGroupSize[0] - 4*ybuffer)/4;
                                                  // meters
            ysize[1] = (yGroupSize[0] - 3*ybuffer)/2;

            x[0] = X0 + xbuffer;                  // writer & set
            x[1] = x[0] + xsize[0] + xbuffer;     // max pulses
            x[2] = x[1] + xsize[0] + xbuffer;     // num pulses
            x[3] = x[2] + xsize[0] + xbuffer;     // Go
            x[4] = x[3] + xsize[0] + xbuffer;     // Go

            y[0] = fileGroup->y() + ybuffer;
            y[1] = y[0] + ysize[0] + ybuffer;
            y[2] = y[1] + ysize[0] + ybuffer;
            y[3] = y[2] + ysize[0] + ybuffer;
            y[4] = y[0] + ysize[1] + ybuffer;

            Fl_Menu_Item writerPopup[] =
            {
                {"PLS",        0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {"NIST (LJH)", 0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {"LANL",       0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {"FFT",        0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {0}
            };

            w = chooseFileWriter = new Fl_Choice (x[0],y[1],xsize[1],ysize[0],"File Writer");
            chooseFileWriter->copy(writerPopup,NULL);
            w->align(FL_ALIGN_TOP);

            w = maxPulsesInput = new Fl_Int_Input (x[2],y[1],xsize[0],ysize[0],"Max Pulses");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);
            w->align(FL_ALIGN_TOP);

            w = numPulsesOutput = new Fl_Output (x[3],y[1],xsize[0],ysize[0],"Rec Pulses");
            w->deactivate();
            w->align(FL_ALIGN_TOP);

            w = setFileNameButton = new Fl_Button (x[0],y[2],xsize[0],ysize[0],"Set File");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = closeFileButton = new Fl_Button (x[1],y[2],xsize[0],ysize[0],"Close File");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = fileWriterStreamControlWindowButton = new Fl_Button (x[2],y[2],xsize[1],ysize[0],"Stream Selector");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = fileNameOutput = new Fl_Output (x[1],y[3],xsize[2],ysize[0],"Filename");
            w->align(FL_ALIGN_LEFT);
            //w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = writeDataButton = new Fl_Light_Button (x[4],y[0],xsize[4],ysize[1],"Write Pulses to File!");
            w->align(FL_ALIGN_CENTER);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // rate
            w = fileCompletionIndicator = new Fl_Progress (x[4],y[4],xsize[4],ysize[1],"");
            fileCompletionIndicator->minimum(0.);
            fileCompletionIndicator->maximum(1.);
            fileCompletionIndicator->selection_color(FL_GREEN);
            w->align(FL_ALIGN_INSIDE);

            fileGroup->end();
            fileGroup->deactivate();
        }

        // commanding buttons
        {
            y[0] = Y0 + ybuffer;
            y[1] = y[0] + ysize[0] + ybuffer;
            y[2] = y[1] + ysize[0] + ybuffer;
            x0 = X2 + xbuffer;
            xsize[0] = xGroupSize[2] - 2*xbuffer;
            ysize[0] = 25;

            commandGroup = new Fl_Group (X2,Y0,xGroupSize[2],yGroupSize[0]);
            commandGroup->box(FL_BORDER_BOX);
            commandGroup->begin();

            w = quitButton = new Fl_Return_Button (x0,y[0],xsize[0],ysize[0],"QUIT");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = infoButton = new Fl_Return_Button (x0,y[1],xsize[0],ysize[0],"INFO");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = hackButton = new Fl_Return_Button (x0,y[2],xsize[0],ysize[0],"HACK");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            commandGroup->end();
        }

        // stream, record length and threshold
        {
            streamGroup = new Fl_Group (X1,Y0,xGroupSize[1],yGroupSize[1]);
            int xsize[3];
            int ysize[4];
            int x[5];
            int y[20];

            streamGroup->box(FL_BORDER_BOX);
            streamGroup->begin();
            xsize[1] = 125;                       // input
            xsize[2] = 160;                       // other buttons
                                                  // start stream button
            xsize[0] = streamGroup->w() - 4*xbuffer - xsize[1] - xsize[2];
            ysize[0] = (yGroupSize[1] - 5*ybuffer)/4;
            ysize[2] = 25;                        // slider space and button height
            ysize[1] = ysize[0] - ysize[2];       // text space

            x[0] = streamGroup->x() + xbuffer;    // stream
            x[1] = x[0] + xsize[0] + xbuffer;     // slider
            x[2] = x[1] + xsize[1] + xbuffer;     // other buttons col 1
            x[3] = x[2] + xsize[2] + xbuffer;     // other buttons col 2
            x[4] = x[0] + xsize[0] + xbuffer;     // the sample rate

            y[0] = streamGroup->y() + ybuffer;    // stream
            y[6] = y[0] + ysize[0] +ybuffer;      // stream select
            y[7] = y[6] + ysize[0] +ybuffer;      // decimate

            y[1] = y[0] + ysize[1];               // record length
                                                  // pre-trig
            y[2] = y[1] + ysize[2] + ybuffer + ysize[1];
                                                  // decimate
            y[10] = y[2] + ysize[2] + ybuffer + ysize[1];
                                                  // decimate
            y[11] = y[10] + ysize[2] + ybuffer + ysize[1];

            y[3] = y[0];                          // trigger rate
            //y[12] = y[0] + ysize[2] + ybuffer; // trigger rate
            y[4] = y[3] + ysize[2] + ybuffer;     // optimal mix
            y[5] = y[4] + ysize[2] + ybuffer;     // PHA
            y[8] = y[5] + ysize[2] + ybuffer;     // group trigger
            y[9] = y[8] + ysize[2] + ybuffer;     // trigger control
            y[12] = y[9] + ysize[2] + ybuffer;    // trigger control

                                                  // streamButton
            w = streamButton = new Fl_Light_Button (x[0],y[0],xsize[0],ysize[0],"Start\nStream");
            w->align(FL_ALIGN_CENTER);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // stream channel
            w = showStreamChannelWindowButton = new Fl_Button (x[0],y[6],xsize[0],ysize[0],"Data Stream\nSelect");
            w->align(FL_ALIGN_CENTER);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // decimate on
            w = decimateFlagButton = new Fl_Light_Button (x[0],y[7],xsize[0],ysize[0],"Decimate");
            w->align(FL_ALIGN_CENTER);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // rec length slider
            w = recordLengthInput = new Fl_Int_Input (x[1],y[1],xsize[1],ysize[2],"Record Length");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // pretrig length slider
            w = preTriggerLengthInput = new Fl_Int_Input (x[1],y[2],xsize[1],ysize[2],"Pre-Trigger Length");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            w = decimateLevelInput = new Fl_Int_Input (x[1],y[10],xsize[1],ysize[2],"Decimation Level");
            w->align(FL_ALIGN_TOP);
            decimateLevelInput->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            Fl_Menu_Item popup[] =
            {
                {"Average", 0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {"Truncate", 0,(Fl_Callback*) mainWindow_handleEvent, (void*) this},
                {0}
            };

            w = decimateModeSelector = new Fl_Choice (x[0],y[11],xsize[0],ysize[2],"Decimation Mode");
            decimateModeSelector->copy(popup,NULL);
            w->align(FL_ALIGN_TOP);

            w = sampleRateDisplay = new Fl_Float_Input (x[4],y[11],xsize[0],ysize[2],"Effective Sample Rate");
            w->deactivate();
            w->align(FL_ALIGN_TOP);

                                                  //
            w = showTriggerRateWindowButton = new Fl_Button (x[2],y[3],xsize[2],ysize[2],"Trigger Rate Window");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            // only shown if NDFB
                                                  //
            w = showMixControlWindowButton = new Fl_Button (x[2],y[4],xsize[2],ysize[2],"Optimal Mix Window");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // Pulse Height window
            w = showPulseAnalysisWindowButton = new Fl_Button (x[2],y[5],xsize[2],ysize[2],"Pulse Analysis Window");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // Group Trigger
            w = showGroupTriggerWindowButton = new Fl_Button (x[2],y[8],xsize[2],ysize[2],"Group Trigger Window");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // Master Threshold
            w = showTriggerControlWindowButton = new Fl_Button (x[2],y[9],xsize[2],ysize[2], "Trigger Control Window");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            // only show for play data
                                                  //
            w = rewindDataFileButton = new Fl_Button (x[2],y[12],xsize[2],ysize[2],"Rewind Data File");
            w->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            streamGroup->end();
            streamGroup->deactivate();
        }

        {                                         // plot window controller buttons
            plotGroup = new Fl_Group (X2,Y1,xGroupSize[2],yGroupSize[0]);

            plotGroup->box(FL_BORDER_BOX);
            plotGroup->begin();
            xsize[0] = plotGroup->w() - 2*xbuffer;// start plot button
            ysize[0] = 25;

            x0 = plotGroup->x() + xbuffer;        // plot

            y0 = plotGroup->y() + ybuffer;        // button1
            y1 = y0 + ysize[0] + ybuffer;         // button1

                                                  // plotButton
            w = morePlotsButton = new Fl_Button (x0,y0,xsize[0],ysize[0],"Open Another Plot Window");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

                                                  // plotButton
            w = removeAllPlotWindowButton = new Fl_Button (x0,y1,xsize[0],ysize[0],"Close All Plot Windows");
            w->callback((Fl_Callback*) mainWindow_handleEvent, (void*) this);

            plotGroup->end();
            plotGroup->deactivate();

        }

    }   mainWindow->end();                        // end main window grouping

}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::mainWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->mainWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::mainWindow_handleEvent(Fl_Widget *w)
{

    // server stuff
    if (w==connectButton)
    {
        if (connectButton->value() == true)
        {

            // init the client and get it ready to start streaming data
            // note, this will tell us stuff about the server(ie how many channels we can expect)
            // so we can setup the subwindows. we are running this in it's own
            // thread so we can do a status bar that does block
            pthread_t tempThread;
            pthread_create(&tempThread,NULL,client->connect,(void *)client);

            // tell the gui to set up for a connected client
            connect (true);
        }
        else
        {
            if (streamButton->value())
            {
                int resp = fl_choice("Are you sure you want to disconnect from the server?",
                    "Cancel","Yes, Disconnect", NULL);
                if (resp == 0)
                {
                    connectButton->value(true);   // turn the button back on!
                    return;
                }

                // tell the client to stop streaming data from the server
                client->stopStreaming();

                // connect(false) will call stream(false) so don't bother doing it here
            }
            // stop the client and disconnect from the server
            client->disconnect();                 // this blocks

            // tell the gui to get in the disconnected stat
            connect (false);
        }
        return;
    }

    if (w==portInput)
    {
        unsigned int port;
        port = atoi(portInput->value());
        client->server->port(port);
        return;
    }

    if (w==hostInput)
    {
        char *output = new char[256];             // get a new chunk of mem that must be deleted at some point
        sprintf (output,"%s",hostInput->value()); // copy the data
        std::cout << "New host: " << output << std::endl;
        client->server->host(output);
        return;
    }

    if (w==saveConfigFileButton)
    {
        client->saveConfigFile();
        return;
    }

    // file writer stuff
    if (w == chooseFileWriter)
    {
        XOUTType type = UNDEFINED_XOUT;
        switch(chooseFileWriter->value())
        {
            case 0:
            {
                type = PLS;
                break;
            }
            case 1:
            {
                type = LJH;
                break;
            }
            case 2:
            {
                type = LANL;
                break;
            }
            case 3:
            {
                type = FFT;
                break;
            }
        }

        // send the selection to the client
        client->setXOUTType(type);

        // if this is ljh, open the window
        if (client->getXOUTType() == LJH)
        {
            LJHWindow_setup();
            ljhWindow->show();
        }

        // user refresh
        mainWindow_userRefresh();

        return;
    }

    if (w == setFileNameButton)
    {
        char *filename = NULL;

        // display the file selector
        if (client->getXOUTType() == PLS) filename = fl_file_chooser("Output File","*.pls",NULL);
        if (client->getXOUTType() == LJH) filename = fl_file_chooser("Output File","*.ljh",NULL);
        if (client->getXOUTType() == LANL) filename = fl_file_chooser("Output File","*.lanl",NULL);
        if (client->getXOUTType() == FFT) filename = fl_file_chooser("Output File","*.fft",NULL);

        // test to see if the user canceled
        if (filename == NULL)
        {
            return;
        }

        // Turn off data writing
        client->XOUTWriter()->closeXOUT();        // this sets the name string to ""

        FILE *testFp = fopen(filename,"rb");
        if (testFp != NULL)
        {

            // File already exists
            fclose(testFp);
            int resp = fl_choice("File exists!","Abort","Overwrite","Append");
            switch (resp)
            {
                case 0:
                    //Aborted
                    std::cout << "User Aborted" << std::endl;
                    break;
                case 1:
                    //Overwrite
                    client->XOUTWriter()->openXOUT(filename,"wb");
                    std::cout << "Overwriting file: ";
                    break;
                case 2:
                    //Append
                    client->XOUTWriter()->openXOUT(filename,"ab");
                    std::cout << "Appending file: ";
                    break;
                default:
                    std::cout << "ACK!! fl_choice barfed." << std::endl;
            }                                     // end of switch

        }
        else
        {

            // file does not exist, so just open it
            client->XOUTWriter()->openXOUT(filename,"wb");
            std::cout << "Writing to file: ";

            // now we will choose to record a trigger file

            std::string trigger_filename=filename;

            const bool append_extension=true;
            client->save_trigger_state_to_file(trigger_filename,append_extension);
        }

        std::cout << client->XOUTWriter()->fileName() << std::endl;
        fileNameOutput->value((std::string("")+client->XOUTWriter()->fileName()).c_str());

        return;
    }

    if (w == writeDataButton)
    {
        printf ("%d\n",writeDataButton->value());
        if (writeDataButton->value())
        {

            // get the lock
            writerButtonStatusLock.getLock();

            if (client->XOUTWriter()->dataWriterFD() == NULL)
            {
                fl_message("Open file for writing first!");
                writeDataButton->value(false);
            }
            else
            {
                client->XOUTWriter()->writingFlag(true);
                writerButtonStatusFlag = true;
            }

            // release the lock
            writerButtonStatusLock.releaseLock();

        }
        else
        {

            // get the lock
            writerButtonStatusLock.getLock();

            client->XOUTWriter()->writingFlag(false);
            writerButtonStatusFlag = false;

            // release the lock
            writerButtonStatusLock.releaseLock();

        }
        return;
    }

    if (w == closeFileButton)
    {
        fileNameOutput->value("");
        client->XOUTWriter()->closeXOUT();

        fileCompletionIndicator->value(1.);
        fileCompletionIndicator->copy_label("Complete!");
        Fl::check();                              // paint it to screen NOW

        return;
    }

    if (w==fileWriterStreamControlWindowButton)
    {
        fileWriterStreamControlWindow->show();
        fileWriterStreamControlWindow_userRefresh();
        return;
    }

    if (w == maxPulsesInput)
    {
        int value;
        value = atoi(maxPulsesInput->value());
        std::cout << "New Max record value: " << value << std::endl;
        client->XOUTWriter()->recordMax(value);
        mainWindow_userRefresh();
        return;
    }

    // command stuff
    if (w==quitButton)
    {
        std::cout << "Quitting" << std::endl;
        stopGUI();
        return;
    }

    if (w==hackButton)
    {
        std::cout << "Hacking" << std::endl;
        //        std::cout << "Doing the alt mix on channel 3 and 2" << std::endl;

        //        client->optimalMix_start(XCD_ALLCHANNELS);

        std::cout << client->pretty_print_trigger_state();

        //this->read_trigger_state_dialog();

        //cb_read_trigger_state_dialog(0,this);

        return;
    }

    if (w==infoButton)
    {
        std::cout << "Info" << std::endl;
        mainWindow_userRefresh();
        return;
    }

    // stream and trigger stuff
    if (w==streamButton)
    {
        if (streamButton->value())
        {

            // tell the client to start streaming
            client->startStreaming();

            // update the gui
            stream (true);
        }
        else
        {
            // tell the client to stop streaming
            client->stopStreaming();

            // update the gui
            stream (false);
        }
        return;
    }

    if (w==recordLengthInput)
    {
        int i;
        sscanf(recordLengthInput->value(),"%u",&i);
        client->recLen(XCD_ALLCHANNELS,i);
        return;
    }

    if (w==preTriggerLengthInput)
    {
        int i;
        sscanf(preTriggerLengthInput->value(),"%u",&i);
        client->pretrig(XCD_ALLCHANNELS,i);
        return;
    }

    // this stuff is only active if the server is connected
    if (client->connected())
    {
        // decimate input
        if (w==decimateLevelInput)
        {
            unsigned int i;
            unsigned short int us;
            sscanf(decimateLevelInput->value(),"%u",&i);
            us = (unsigned short int) i;
            client->decimateLevel(XCD_ALLCHANNELS, us);
            return;
        }

        // decimate flag
        if (w==decimateFlagButton)
        {
            client->decimateFlag(XCD_ALLCHANNELS,decimateFlagButton->value());
            return;
        }

        // check the channel selector buttons
        if (w==decimateChannelSelector)
        {
            mainWindow_userRefresh();
            return;
        }

        if (w==decimateModeSelector)
        {
            if (decimateModeSelector->value() == 0)
            {
                client->decimateAvgFlag(XCD_ALLCHANNELS,true);
            }
            if (decimateModeSelector->value() == 1)
            {
                client->decimateAvgFlag(XCD_ALLCHANNELS,false);
            }
            return;
        }

        // launch the trigger control subwindow
        if (w==showMixControlWindowButton)
        {
            mixControlWindow->show();
            mixControlWindow_userRefresh();
            return;
        }

        if (w==rewindDataFileButton)
        {
            client->rewindDataFile();
            return;
        }

        // launch the trigger control subwindow
        if (w==showTriggerControlWindowButton)
        {
            triggerControlWindow->show();
            triggerControlWindow_userRefresh();
            return;
        }

        // launch the group trigger subwindow
        if (w==showGroupTriggerWindowButton)
        {
            groupTriggerWindow->show();
            mainWindow_userRefresh();
            return;
        }

        // launch the stream channel subwindow
        if (w==showStreamChannelWindowButton)
        {
            streamChannelWindow->show();
            streamChannelWindow_userRefresh();
            return;
        }

        // launch the trigger rate subwindow
        if (w==showTriggerRateWindowButton)
        {
            if (!triggerRateWindow->visible())
            {
                triggerRateWindow->show();
                triggerRateWindow_userRefresh();
            }
            else
            {
                triggerRateWindow->hide();
            }
            return;
        }

        // launch the pulse analysis subwindow
        if (w==showPulseAnalysisWindowButton)
        {
            if (!pulseAnaylsisWindow->visible())
            {
                pulseAnaylsisWindow->show();
            }
            else
            {
                pulseAnaylsisWindow->hide();
            }
            return;
        }

        // plot group widgets
        if (w==morePlotsButton)
        {
            plotWindow_add();
            return;
        }

        if (w==removeAllPlotWindowButton)
        {
            plotWindow_removeAll();
            return;
        }

    }

    std::cout << "Main Window received an even that it doesn't know how to handle?" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mainWindow_userRefresh ()
{
    char output[128];

    sprintf (output,"%d",client->server->port());
    portInput->value(output);

    sprintf (output,"%s",client->server->host());
    hostInput->value(output);

    sprintf (output,"%d",client->XOUTWriter()->recordMax());
    maxPulsesInput->value(output);

    if (client->connected())
    {

        // record length
        sprintf (output,"%d",client->recLen(0));
        recordLengthInput->value(output);

        // pretrigger length
        sprintf (output,"%d",client->pretrig(0));
        preTriggerLengthInput->value(output);

        streamButton->value(client->streaming());

        // in this version of the code, all decimation is the same for every channel.  pick the first one and display it
        unsigned short int level = client->decimateLevel(0);
        sprintf (output,"%u",level);
        decimateLevelInput->value(output);

        float f = client->effectiveSampleRate(0);
        sprintf (output,"%f Hz",f);
        sampleRateDisplay->value(output);

        bool flag = client->decimateFlag(0);
        decimateFlagButton->value(flag);

        if (client->decimateAvgFlag(0))
        {
            decimateModeSelector->value(0);
        }
        else
        {
            decimateModeSelector->value(1);
        }
        return;

    }                                             // end of connected

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mainWindow_loopRefresh ()
{
    char output[128];

    // get the lock
    writerButtonStatusLock.getLock();

    // handle any updates to the file writing widgets
    if (client->XOUTWriter()->writingFlag() == true)
    {
        // if necessary, update how many records have been taken
        sprintf (output,"%d",client->XOUTWriter()->recordWritten());
        numPulsesOutput->value(output);

        if (client->XOUTWriter()->recordMax() != 0)
        {
            float f = ((float)client->XOUTWriter()->recordWritten()) / ((float)client->XOUTWriter()->recordMax());
            sprintf (output,"%%Complete: %.1f",f*100);
            fileCompletionIndicator->value(f);
            fileCompletionIndicator->copy_label(output);
        }

        //writeDataButton->value(true);
        //printf ("writing true  button %d\n",writeDataButton->value());

    }
    else
    {

        // at this point, the writer is not writing
        // what we want to determine is whether this condition has been like this
        // (in which case we do nothing) or whether this just happened (in which case
        // we have to change some stuff on the screen.

        // check the status of the writerButtonStatusFlag
        if (writerButtonStatusFlag == true)
        {
            // this should only happen once per data capture

            // turn the off
            writeDataButton->value(false);

            // set the status bar to 1 and change the label
            fileCompletionIndicator->value(1.);
            fileCompletionIndicator->copy_label("Complete!");
            Fl::check();                          // paint it to screen NOW

            // set the final value of the records
            sprintf (output,"%d",client->XOUTWriter()->recordWritten());
            numPulsesOutput->value(output);

            // set the final value of the file name
            sprintf (output,"%s (Done!)",fileNameOutput->value());
            fileNameOutput->value(output);
            //fileNameOutput->value("");

            // finally, set this flag false
            writerButtonStatusFlag = false;
        }
    }

    // release the lock
    writerButtonStatusLock.releaseLock();

    // one out of every 5 get a buffer level
    if (loopCounter % 5 == 0)
    {
        // handle the data rate and buffer progress bars
        float level;                              // this should be done within client!!!
        client->server->get(XCD_BUFFERLEVEL,0,&level);

        // i would like the percentage left, not the percentage full as given
        level = 1-level;
        sprintf (output,"Buffer Level\n%.1f%%",level*100);
        bufferLevelIndicator->copy_label(output);

        Fl_Color oldColor, newColor;
        oldColor =  bufferLevelIndicator->selection_color();
        newColor = FL_GREEN;
        if (level < .40) newColor = FL_YELLOW;
        if (level < .20) newColor = FL_RED;
        if (level < .1) std::cout << "You are about to be booted.... " << std::endl;
        if (oldColor != newColor) bufferLevelIndicator->selection_color(newColor);
        bufferLevelIndicator->value(level);
    }

    // one out of every 2 get a data rate
    if (loopCounter % 2 == 0)
    {
        float level = client->dataRate() / 1e6;
        //std::cout << "data rate is " << level << " Mb per second " << std::endl;
        dataRateIndicator->value(level);
        sprintf (output,"Data Rate\n%.2f MB/s",level);
        dataRateIndicator->copy_label(output);
    }

    // check the tilt and refresh if necessary
    if(client->tiltFlag())
    {
        mainWindow_userRefresh ();                // update the screen
        client->tiltFlag (false);                 // reset the tilt
    }
}


////////////////////////////////////////////////////////////////////////////////
// Trigger Control Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerControlWindow_setup ()
{
    int xsize[10];
    int ysize[10];
    int x[20];
    int y[20];
    int xbuffer = 5;
    int ybuffer = 5;
    int yfactor;
    int winW;
    int winH;
    int posX = mainWindow->x();
    int posY = mainWindow->y();

    xsize[0]=150;                                 // channel chooser
    xsize[1]=500;                                 // affected channels  this size sets the window width
    winW = xsize[0] + xsize[1] + 3*xbuffer;

    xsize[2]=125;                                 // activate button
    xsize[3]=125;                                 // invert threshold
                                                  // threshold Slider
    xsize[4]=winW - (xsize[2] + xsize[3] + 4 * xbuffer);
    xsize[5]=125;                                 // compute FFT and windowing flag
    xsize[6]=(xsize[4] - xbuffer) / 2;            // noise records pers FFT
    xsize[7]=300;                                 // okay button

    ysize[0]=25;                                  // everything else
    ysize[1]=40;                                  // okay button

    x[0] = xbuffer;
    x[1] = x[0] + xsize[0] + xbuffer;
    x[2] = xbuffer;
    x[3] = x[2] + xsize[2] + xbuffer;

    x[4] = x[3] + xsize[3] + xbuffer;
    x[5] = xbuffer;
    x[6] = x[5] + xsize[5] + xbuffer;             // noise records per fft
    x[7] = xbuffer;
    x[8] = x[6] + xsize[5] + xbuffer;             // descriminator
    x[9] = x[8] + xsize[6] + xbuffer;             // windowing

    yfactor = 4;
    for (int i=0;i<7;i++)
    {
        y[i]=(ysize[0] + yfactor*ybuffer)*(i+1) - ysize[0];
    }
    winH=  y[6] + ysize[1] + ybuffer;

    triggerControlWindow = new Fl_Double_Window(posX,posY,winW,winH,"Trigger Control");
    triggerControlWindow->begin();
    {
        Fl_Widget *w;

        Fl_Menu_Item popup[] =
        {
            {"User Defined", 0, triggerControlWindow_handleEvent, (void*) this},
            {"All Channels", 0, triggerControlWindow_handleEvent, (void*) this},
            {"All FB Channels", 0, triggerControlWindow_handleEvent, (void*) this},
            {"All Error Channels", 0, triggerControlWindow_handleEvent, (void*) this},
            {0}
        };

        w = channelChoice = new Fl_Choice (x[0],y[0],xsize[0],ysize[0],"Channel Chooser");
        channelChoice->copy(popup,NULL);

        w->align(FL_ALIGN_TOP);
        w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

        w = affectedChannelsInput = new Fl_Input (x[1],y[0],xsize[1],ysize[0],"Targeted Data Streams");
        w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
        w->align(FL_ALIGN_TOP);

        triggerControlGroup = new Fl_Group (x[0],y[0],winW,winH);
        triggerControlGroup->begin();
        {

            // auto trigger
            w =                         new Fl_Box          (x[2],y[1],xsize[2],0,"Auto Trigger");
            w->align(FL_ALIGN_TOP);
            w = autoTriggerFlagButton = new Fl_Light_Button (x[2],y[1],xsize[2],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = autoTriggerThresholdSlider = new SliderInput (x[4],y[1],xsize[4],ysize[0],"Time B/W Triggers (mSecond)");
            autoTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            w->align(FL_ALIGN_TOP);

            // level trigger
            w =                          new Fl_Box          (x[2],y[2],xsize[2],0,"Level Trigger");
            w->align(FL_ALIGN_TOP);
            w = levelTriggerFlagButton = new Fl_Light_Button (x[2],y[2],xsize[2],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w =                          new Fl_Box          (x[3],y[2],xsize[3],0,"Invert Trigger");
            w->align(FL_ALIGN_TOP);
            w = levelInverseFlagButton = new Fl_Light_Button (x[3],y[2],xsize[3],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = levelTriggerThresholdSlider = new FloatSliderInput (x[4],y[2],xsize[4],ysize[0],"Threshold Slider (mVolts)");
            levelTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            w->align(FL_ALIGN_TOP);

            // edge trigger
            w =                         new Fl_Box          (x[2],y[3],xsize[2],0,"Edge Trigger");
            w->align(FL_ALIGN_TOP);
            w = edgeTriggerFlagButton = new Fl_Light_Button (x[2],y[3],xsize[2],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w =                         new Fl_Box          (x[3],y[3],xsize[3],0,"Invert Trigger");
            w->align(FL_ALIGN_TOP);
            w = edgeInverseFlagButton = new Fl_Light_Button (x[3],y[3],xsize[3],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = edgeTriggerThresholdSlider = new FloatSliderInput (x[4],y[3],xsize[4],ysize[0],"Threshold Slider (Volts / mSecond)");
            edgeTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            w->align(FL_ALIGN_TOP);

            // noise trigger
            w =                          new Fl_Box          (x[2],y[4],xsize[2],0,"Noise Trigger");
            w->align(FL_ALIGN_TOP);
            w = noiseTriggerFlagButton = new Fl_Light_Button (x[2],y[4],xsize[2],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w =                          new Fl_Box          (x[3],y[4],xsize[3],0,"Invert Trigger");
            w->align(FL_ALIGN_TOP);
            w = noiseInverseFlagButton = new Fl_Light_Button (x[3],y[4],xsize[3], ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = noiseTriggerThresholdSlider = new FloatSliderInput (x[4],y[4],xsize[4],ysize[0],"Threshold Slider (mVolts)");
            noiseTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            w->align(FL_ALIGN_TOP);

            // fft stuff
            w =                    new Fl_Box          (x[5],y[5],xsize[5],0,"Compute FFT");
            w->align(FL_ALIGN_TOP);
            w = computeFFTButton = new Fl_Light_Button (x[5],y[5],xsize[5],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w                          = new Fl_Box          (x[6],y[5],xsize[5],0,"Use Windowing");
            w->align(FL_ALIGN_TOP);
            w = fftWindowingFlagButton = new Fl_Light_Button (x[6],y[5],xsize[5],ysize[0],"Active");
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = noiseRecordsPerFFTInput = new Fl_Int_Input (x[8],y[5],xsize[6],ysize[0],"Noise Records Per FFT");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            w = fftDiscriminationInput = new Fl_Int_Input (x[9],y[5],xsize[6],ysize[0],"FFT Discriminator");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

            // okay button
            w = triggerControlOkayButton = new Fl_Return_Button (x[7],y[6],xsize[7],ysize[1],"OKAY");
            w->align(FL_ALIGN_INSIDE);
            w->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);

        }
        triggerControlGroup->end();

    }
    triggerControlWindow->end();
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerControlWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->triggerControlWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerControlWindow_handleEvent(Fl_Widget *w)
{
    if (w==channelChoice)
    {
        switch (channelChoice->value())
        {
            case 0:                               // user defined
            {
                break;
            }
            case 1:                               // all streams
            {
                affectedChannels.clear();
                for (unsigned int i=0;i<client->nDatatStreams(); i++)
                {
                    affectedChannels.push_back(i);
                }
                break;
            }
            case 2:                               // all fb streams
            {
                affectedChannels.clear();
                for (unsigned int i=1;i<client->nDatatStreams(); i+=2)
                {
                    affectedChannels.push_back(i);
                }
                break;
            }
            case 3:                               // all error streams
            {
                affectedChannels.clear();
                for (unsigned int i=0;i<client->nDatatStreams(); i+=2)
                {
                    affectedChannels.push_back(i);
                }
                break;
            }
        }

        triggerControlWindow_userRefresh();
        return;
    }

    if (w==affectedChannelsInput)
    {
        affectedChannels.clear();
        channelChoice->value(0);                  // set the selector to "User Selected"
        char input[1028];
        sprintf(input,"%s",affectedChannelsInput->value());
        if (strlen(input) != 0)
        {
            int i = 0;
            int ret;
            while (i<1024)
            {
                int j;
                ret = sscanf(&input[i],"%d",&j);

                if (ret != 0)
                {
                    //printf ("Adding channel: %d\n",j);
                    if (j<0)
                    {
                        printf ("\tinvalid channel!!!\n");

                    }
                    else
                    {
                        if ((unsigned int)j >= client->nDatatStreams())
                        {
                            printf ("\tinvalid channel!!!\n");
                        }
                        else
                        {
                            affectedChannels.push_back(j);
                        }
                    }
                }

                char *pos = strstr(&input[i],",");
                if (pos == NULL)
                {
                    //printf ("NULL!!\n");
                    break;
                }
                else
                {
                    //				printf ("%d-%d=%d\n",pos,&input[0],i);
                    i = pos - &input[0]+1;
                }
            }
        }

        triggerControlWindow_userRefresh();
        return;

    }

    std::vector<unsigned int>::iterator streamIterator;
    streamIterator = affectedChannels.begin();

    // the activate trigger buttons
    if (w==autoTriggerFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            std::cout << "No channels selected." << std::endl;
            autoTriggerFlagButton->value(false);
        }
        else
        {
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->autoTriggerFlag(autoTriggerFlagButton->value());
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==levelTriggerFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            std::cout << "No channels selected." << std::endl;
            levelTriggerFlagButton->value(false);
        }
        else
        {
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->levelTriggerFlag(levelTriggerFlagButton->value());
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==edgeTriggerFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            std::cout << "No channels selected." << std::endl;
            edgeTriggerFlagButton->value(false);
        }
        else
        {
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->edgeTriggerFlag(edgeTriggerFlagButton->value());
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==noiseTriggerFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            std::cout << "No channels selected." << std::endl;
            noiseTriggerFlagButton->value(false);
        }
        else
        {
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->noiseTriggerFlag(noiseTriggerFlagButton->value());
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    // threshold sliders
    if (w==autoTriggerThresholdSlider)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    double value = autoTriggerThresholdSlider->value();
                    client->streamData[*streamIterator]->autoThreshold((int)value);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==levelTriggerThresholdSlider)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            double value = levelTriggerThresholdSlider->value();
            value /= 1000.;                       // the slider is in mV, the function expects volts, so convert it
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->levelThreshold(value);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==edgeTriggerThresholdSlider)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            double value = edgeTriggerThresholdSlider->value();
            // silder is in v/mS, convert this to volts / s because that is what client expects

            printf ("user selects %f\n",edgeTriggerThresholdSlider->value());

            value *= 1000.;
            for (unsigned int i=0; i<affectedChannels.size(); i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->edgeThreshold(value);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==noiseTriggerThresholdSlider)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            double value = noiseTriggerThresholdSlider->value();
            value /= 1000.;                       // the slider is in mV, the function expects volts, so convert it
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->noiseThreshold(value);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        triggerControlWindow_userRefresh();
        return;
    }

    // all the inverse buttons
    if (w==levelInverseFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            bool b = levelInverseFlagButton->value();
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->levelInverseFlag(b);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        return;
    }

    if (w==edgeInverseFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            bool b = edgeInverseFlagButton->value();
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->edgeInverseFlag(b);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        return;
    }

    if (w==noiseInverseFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            bool b = noiseInverseFlagButton->value();
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->streamData[*streamIterator]->noiseInverseFlag(b);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        return;
    }

    // fft stuff
    if (w==noiseRecordsPerFFTInput)
    {
        unsigned int i;
        sscanf (noiseRecordsPerFFTInput->value(),"%u",&i);
        client->noiseRecordsPerFFT(i);
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==fftDiscriminationInput)
    {
        unsigned int i;
        sscanf (fftDiscriminationInput->value(),"%u",&i);
        client->fftDiscriminator(i);
        triggerControlWindow_userRefresh();
        return;
    }

    if (w==fftWindowingFlagButton)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            bool b = fftWindowingFlagButton->value();
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                if (client->streamDataFlag(*streamIterator))
                {
                    client->noiseRecordFFT[*streamIterator]->windowingFlag(b);
                }
                else
                {
                    //std::cout << "Channel " << *streamIterator << " is not enabled.  Skipping" << std::endl;
                }
                streamIterator++;
            }
        }
        return;
    }

    // fft stuff
    if (w==computeFFTButton)
    {
        if (affectedChannels.size() == 0)
        {
            //std::cout << "No channels selected." << std::endl;
        }
        else
        {
            bool b = computeFFTButton->value();
            for (unsigned int i=0;i<affectedChannels.size();i++)
            {
                client->noiseRecordFFT[*streamIterator]->fftFlag(b);
                streamIterator++;
            }
            triggerControlWindow_userRefresh();
        }

        return;
    }

    if (w==triggerControlOkayButton)
    {
        // to keep the stupid sliders from issuing yet another call back, disable them for the time being
                                                  // turn the callback off
        autoTriggerThresholdSlider->callback  ( NULL, NULL );
                                                  // turn the callback off
        levelTriggerThresholdSlider->callback ( NULL, NULL );
                                                  // turn the callback off
        edgeTriggerThresholdSlider->callback  ( NULL, NULL );
                                                  // turn the callback off
        noiseTriggerThresholdSlider->callback ( NULL, NULL );

        triggerControlWindow->hide();

        // now turn the dang things back on again
                                                  // turn the callback off
        autoTriggerThresholdSlider->callback  ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );
                                                  // turn the callback off
        levelTriggerThresholdSlider->callback ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );
                                                  // turn the callback off
        edgeTriggerThresholdSlider->callback  ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );
                                                  // turn the callback off
        noiseTriggerThresholdSlider->callback ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );

        return;
    }

    std::cout << "You didn't handle that very well, now did you?" << std::endl;
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerControlWindow_userRefresh()
{
    char output[1024];

    std::vector<unsigned int>::iterator streamIterator;
    bool everythingTheSame;
    bool tempFlag;
    double temp;
    double high,low;

    if (affectedChannels.size() > 0)
    {
        triggerControlGroup->activate();

        ////////////////////////////
        // create the targeted data stream output string
        streamIterator = affectedChannels.begin();// reset the iterator
        output[0] = '\0';                         // clear the output string
        for (unsigned int i=0;i<affectedChannels.size();i++)
        {
                                                  // build up the string
            sprintf(output,"%s%d,",output,*streamIterator);
            streamIterator++;
            fflush(0);
        }
        output[strlen(output)-1] = '\0';          // remove the last comma
        affectedChannelsInput->value(output);     // paint it to screen

        ////////////////////////////
        // the auto trigger flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->autoTriggerFlag();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->autoTriggerFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            autoTriggerFlagButton->value(tempFlag);
            autoTriggerFlagButton->copy_label("Active");
        }
        else
        {
            autoTriggerFlagButton->value(true);   // if even 1 is on, turn the button true
            autoTriggerFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the auto threshold slider
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        temp = client->streamData[*streamIterator]->autoThreshold();
        low  = client->streamData[*streamIterator]->minLevelThreshold();
        high = client->streamData[*streamIterator]->maxLevelThreshold();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (temp != client->streamData[*streamIterator]->autoThreshold ())
            {
                everythingTheSame = false;
                break;
            }
        }
        low = 1;
        high = 3600000;                           // 3600 second
        autoTriggerThresholdSlider->bounds((int)low,(int)high);
        if (everythingTheSame)
        {
            // hack attack!  remember, we with stupid sliders, everytime you set a value it kicks a call back!! annoying...
            // so turn off the callback, set the value, and turn it back on
                                                  // turn the callback off
            autoTriggerThresholdSlider->callback(NULL,NULL);
            autoTriggerThresholdSlider->value((int)temp);
                                                  // turn the callback back on
            autoTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            autoTriggerThresholdSlider->copy_label("Threshold Slider (mSeconds)");
        }
        else
        {
            // just leave the auto trigger level alone
            autoTriggerThresholdSlider->copy_label("Threshold Slider (mSeconds) (Various)");
        }

        ////////////////////////////
        // the level trigger flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->levelTriggerFlag();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->levelTriggerFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            levelTriggerFlagButton->value(tempFlag);
            levelTriggerFlagButton->copy_label("Active");
        }
        else
        {
            levelTriggerFlagButton->value(true);  // if even 1 is on, turn the button true
            levelTriggerFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the level inverse flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->levelInverseFlag ( );
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->levelInverseFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            levelInverseFlagButton->value(tempFlag);
            levelInverseFlagButton->copy_label("Active");
        }
        else
        {
            levelInverseFlagButton->value(true);  // if even 1 is on, turn the button true
            levelInverseFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the level threshold slider
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        temp = client->streamData[*streamIterator]->levelThreshold();
        low = client->streamData[*streamIterator]->minLevelThreshold();
        high = client->streamData[*streamIterator]->maxLevelThreshold();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (temp != client->streamData[*streamIterator]->levelThreshold ())
            {
                everythingTheSame = false;
                break;
            }
            if (client->streamData[*streamIterator]->maxLevelThreshold() > high)
            {
                high = client->streamData[*streamIterator]->maxLevelThreshold();
            }
            if (client->streamData[*streamIterator]->minLevelThreshold() < low)
            {
                low = client->streamData[*streamIterator]->minLevelThreshold();
            }
        }
        low  *= 1000.;                            // we want the units in mV, but the function supplies them in V, so convert
        high *= 1000.;
        temp *= 1000.;
        levelTriggerThresholdSlider->bounds(low,high);
        if (everythingTheSame)
        {
            // hack attack!  remember, we with stupid sliders, everytime you set a value it kicks a call back!! annoying...
            // so turn off the callback, set the value, and turn it back on
                                                  // turn the callback off
            levelTriggerThresholdSlider->callback(NULL,NULL);
            levelTriggerThresholdSlider->value(temp);
                                                  // turn the callback back on
            levelTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            levelTriggerThresholdSlider->copy_label("Threshold Slider (mVolts)");
        }
        else
        {
                                                  // turn the callback off
            levelTriggerThresholdSlider->callback(NULL,NULL);
            levelTriggerThresholdSlider->value(0);// if they are different, just make it 0
                                                  // turn the callback back on
            levelTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            levelTriggerThresholdSlider->copy_label("Threshold Slider (mVolts) (Various)");
        }

        ////////////////////////////
        // the edge trigger flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->edgeTriggerFlag();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->edgeTriggerFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            edgeTriggerFlagButton->value(tempFlag);
            edgeTriggerFlagButton->copy_label("Active");
        }
        else
        {
            edgeTriggerFlagButton->value(true);   // if even 1 is on, turn the button true
            edgeTriggerFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the edge inverse flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->edgeInverseFlag ( );
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->edgeInverseFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            edgeInverseFlagButton->value(tempFlag);
            edgeInverseFlagButton->copy_label("Active");
        }
        else
        {
            edgeInverseFlagButton->value(true);   // if even 1 is on, turn the button true
            edgeInverseFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the edge threshold slider
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        temp = client->streamData[*streamIterator]->edgeThreshold ();
        low  = client->streamData[*streamIterator]->minEdgeThreshold ();
        high = client->streamData[*streamIterator]->maxEdgeThreshold ();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            printf ("channel %d has the threshold set to %f\n",i, client->streamData[*streamIterator]->edgeThreshold ());
            //if (temp != client->streamData[*streamIterator]->edgeThreshold ()) {
            if ((1 - (client->streamData[*streamIterator]->edgeThreshold ()/temp)) > .001)
            {

                printf ("aparently we think that %f does not equal this %f\n",temp, client->streamData[*streamIterator]->edgeThreshold());
                everythingTheSame = false;
                break;
            }
            if (client->streamData[*streamIterator]->maxEdgeThreshold() > high)
            {
                high = client->streamData[*streamIterator]->maxEdgeThreshold();
            }
            if (client->streamData[*streamIterator]->minEdgeThreshold() < low)
            {
                low = client->streamData[*streamIterator]->minEdgeThreshold();
            }
        }
        low  /= 1000.;                            // makes units easier for the user
        high /= 1000.;
        temp /= 1000.;
        edgeTriggerThresholdSlider->bounds(low,high);
        printf ("setting GUI slider to %f high: %f low: %f\n",temp,low,high);

        if (everythingTheSame)
        {
            // hack attack!  remember, we with stupid sliders, everytime you set a value it kicks a call back!! annoying...
            // so turn off the callback, set the value, and turn it back on
                                                  // turn the callback off
            edgeTriggerThresholdSlider->callback   ( NULL, NULL );
            edgeTriggerThresholdSlider->value      ( temp );
                                                  // turn the callback back on
            edgeTriggerThresholdSlider->callback   ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );
            edgeTriggerThresholdSlider->copy_label ( "Threshold Slider (Volts / mSecond)" );
        }
        else
        {
                                                  // turn the callback off
            edgeTriggerThresholdSlider->callback   ( NULL, NULL );
                                                  // if they are different, just make it 0
            edgeTriggerThresholdSlider->value      ( 0 );
                                                  // turn the callback back on
            edgeTriggerThresholdSlider->callback   ( (Fl_Callback*) triggerControlWindow_handleEvent, (void*) this );
            edgeTriggerThresholdSlider->copy_label ( "Threshold Slider (Volts / mSecond) (Various)" );
        }

        ////////////////////////////
        // the noise trigger flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->noiseTriggerFlag();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->noiseTriggerFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            noiseTriggerFlagButton->value(tempFlag);
            noiseTriggerFlagButton->copy_label("Active");
        }
        else
        {
            noiseTriggerFlagButton->value(true);  // if even 1 is on, turn the button true
            noiseTriggerFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the noise inverse flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->streamData[*streamIterator]->noiseInverseFlag ( );
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (tempFlag != client->streamData[*streamIterator]->noiseInverseFlag ())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            noiseInverseFlagButton->value(tempFlag);
            noiseInverseFlagButton->copy_label("Active");
        }
        else
        {
            noiseInverseFlagButton->value(true);  // if even 1 is on, turn the button true
            noiseInverseFlagButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the noise threshold slider
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        temp = client->streamData[*streamIterator]->noiseThreshold();
        low = client->streamData[*streamIterator]->minLevelThreshold();
        high = client->streamData[*streamIterator]->maxLevelThreshold();
                                                  // test if everything is the same
        for (unsigned int i=0;i<affectedChannels.size();i++, streamIterator++)
        {
            if (temp != client->streamData[*streamIterator]->noiseThreshold ())
            {
                everythingTheSame = false;
                break;
            }
            if (client->streamData[*streamIterator]->maxLevelThreshold() > high)
            {
                high = client->streamData[*streamIterator]->maxLevelThreshold();
            }
            if (client->streamData[*streamIterator]->minLevelThreshold() < low)
            {
                low = client->streamData[*streamIterator]->minLevelThreshold();
            }
        }
        low  *= 1000.;                            // we want the units in mV, but the function supplies them in V, so convert
        high *= 1000.;
        temp *= 1000.;
        noiseTriggerThresholdSlider->bounds(low,high);
        if (everythingTheSame)
        {
            // hack attack!  remember, we with stupid sliders, everytime you set a value it kicks a call back!! annoying...
            // so turn off the callback, set the value, and turn it back on
                                                  // turn the callback off
            noiseTriggerThresholdSlider->callback(NULL,NULL);
            noiseTriggerThresholdSlider->value(temp);
                                                  // turn the callback back on
            noiseTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            noiseTriggerThresholdSlider->copy_label("Threshold Slider (mVolts)");
        }
        else
        {
                                                  // turn the callback off
            noiseTriggerThresholdSlider->callback(NULL,NULL);
            noiseTriggerThresholdSlider->value(0);// if they are different, just make it 0
                                                  // turn the callback back on
            noiseTriggerThresholdSlider->callback((Fl_Callback*) triggerControlWindow_handleEvent, (void*) this);
            noiseTriggerThresholdSlider->copy_label("Threshold Slider (mVolts) (Various)");
        }

        ////////////////////////////
        // the compute fft flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->noiseRecordFFT[*streamIterator]->fftFlag();
                                                  // test if everything is the same
        for (unsigned int i=0; i<affectedChannels.size(); i++, streamIterator++)
        {
            if (tempFlag != client->noiseRecordFFT[*streamIterator]->fftFlag())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            computeFFTButton->value(tempFlag);
            computeFFTButton->copy_label("Active");
        }
        else
        {
            computeFFTButton->value(true);        // if even 1 is on, turn the button true
            computeFFTButton->copy_label("Active (Various)");
        }

        ////////////////////////////
        // the fft windowing flag button
        everythingTheSame = true;
        streamIterator = affectedChannels.begin();// reset the iterator
                                                  // get an initial value
        tempFlag = client->noiseRecordFFT[*streamIterator]->windowingFlag();
                                                  // test if everything is the same
        for (unsigned int i=0; i<affectedChannels.size(); i++, streamIterator++)
        {
            if (tempFlag != client->noiseRecordFFT[*streamIterator]->windowingFlag())
            {
                everythingTheSame = false;
                break;
            }
        }
        if (everythingTheSame)
        {
            fftWindowingFlagButton->value(tempFlag);
            fftWindowingFlagButton->copy_label("Active");
        }
        else
        {
            fftWindowingFlagButton->value(true);  // if even 1 is on, turn the button true
            fftWindowingFlagButton->copy_label("Active (Various)");
        }

        // fft stuff (same for all channels, so we don't care about looping through channels)
        sprintf (output,"%u",client->noiseRecordsPerFFT());
        noiseRecordsPerFFTInput->value(output);

        sprintf (output,"%u",client->fftDiscriminator());
        fftDiscriminationInput->value(output);

    }
    else
    {

        triggerControlGroup->deactivate();
        //levelTriggerFlagButton->copy_label("Active");

        return;
    }

}


////////////////////////////////////////////////////////////////////////////////
// Stream Channel Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::streamChannelWindow_setup ()
{
    int n = client->nDatatStreams();
    int xsize[2];                                 // slider
    int ysize[2];
    int x[6];
    int xbuffer = 5;
    int ybuffer = 5;
    int winW;
    int winH;
    int y;
    int posX = mainWindow->x() + 400;
    int posY = mainWindow->y() + 300;
    xsize[0]=150;
    xsize[1]=xsize[0]*2+xbuffer;
    ysize[0]=20;
    y = ybuffer;

    x[0] = xbuffer;
    x[1] = x[0] + xsize[0] + xbuffer;

    winH=(n/2+2)*(ysize[0]+ybuffer)+ybuffer;
    winW = x[1] + xsize[0] + xbuffer;

    // declare the arrays now that you have the size
    streamChannelButton = new Fl_Light_Button* [n];

    streamChannelWindow = new Fl_Double_Window(posX,posY,winW,winH,"Choose Which Channel To Stream");
    streamChannelWindow->begin();
    {
        Fl_Widget *w;
        char buf[128];

        sprintf (buf,"All Even Channels");
        w = allEvenButton = new Fl_Light_Button (x[0],y,xsize[0],ysize[0]);
        w->copy_label(buf);
        w->callback((Fl_Callback*) streamChannelWindow_handleEvent, (void*) this);

        sprintf (buf,"All Odd Channels");
        w = allOddButton = new Fl_Light_Button (x[1],y,xsize[0],ysize[0]);
        w->copy_label(buf);
        w->callback((Fl_Callback*) streamChannelWindow_handleEvent, (void*) this);

        // move down
        y+= (ysize[0]+ybuffer);

        for (int i = 0; i<n; i+=2)
        {

            // streamChannel button
            sprintf (buf,"Channel %d",i);
            w = streamChannelButton[i] = new Fl_Light_Button (x[0],y,xsize[0],ysize[0]);
            w->copy_label(buf);
            w->callback((Fl_Callback*) streamChannelWindow_handleEvent, (void*) this);

            sprintf (buf,"Channel %d",i+1);
            w = streamChannelButton[i+1] = new Fl_Light_Button (x[1],y,xsize[0],ysize[0]);
            w->copy_label(buf);
            w->callback((Fl_Callback*) streamChannelWindow_handleEvent, (void*) this);

            // move down
            y+= (ysize[0]+ybuffer);
        }

        // the okay button
        w = streamChannelOkayButton = new Fl_Return_Button (xbuffer,y,xsize[1],ysize[0],"Okay");
        w->callback((Fl_Callback*) streamChannelWindow_handleEvent, (void*) this);

    }
    streamChannelWindow->end();
    streamChannelWindow->hide();

}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::streamChannelWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->streamChannelWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::streamChannelWindow_handleEvent(Fl_Widget *w)
{

    // okay button
    if (w==streamChannelOkayButton)
    {
        streamChannelWindow->hide();
        return;
    }

    // all even button
    if (w==allEvenButton)
    {
        for (unsigned int i=0;i<client->nDatatStreams();i+=2)
        {
            client->streamDataFlag(i,allEvenButton->value());
        }
        streamChannelWindow_userRefresh();
        return;
    }

    // all odd button
    if (w==allOddButton)
    {
        for (unsigned int i=1;i<client->nDatatStreams();i+=2)
        {
            client->streamDataFlag(i,allOddButton->value());
        }
        streamChannelWindow_userRefresh();
        return;
    }

    // loop though all channels and check for click in the selector boxes
    for (unsigned int i=0;i<client->nDatatStreams();i++)
    {

        if (w==streamChannelButton[i])
        {
            client->streamDataFlag(i,streamChannelButton[i]->value());

            //make sure to redo the gui so the bounds are set correctly on the slider
            Fl::add_timeout(.1,refreshTriggerControlUntilInitialized, (void*)this);

            // don't know which one of these is necessary
            streamChannelWindow_userRefresh();
            return;
        }
    }

    std::cout << "Stream Channel got an event it couldn't handle" << std::endl;

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::streamChannelWindow_userRefresh ()
{

    // stream channel window refreshed
    if (streamChannelWindow->visible())
    {
        bool even=true;
        bool odd=true;
        for (unsigned int i=0;i<client->nDatatStreams();i++)
        {
            bool b = client->streamDataFlag(i);
            if((i%2)==0)
            {
                if (!b) even = false;
            }
            else
            {
                if (!b) odd = false;
            }
            streamChannelButton[i]->value(b);
        }
        allOddButton->value(odd);
        allEvenButton->value(even);
    }
}


////////////////////////////////////////////////////////////////////////////////
// File Writer Stream Controller Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::fileWriterStreamControlWindow_setup ()
{
    int xsize[2];                                 // slider
    int ysize[2];
    int x[20];
    int y[20];
    int xbuffer = 5;
    int ybuffer = 5;
    int winW;
    int winH;
    int posX = mainWindow->x() + 400;
    int posY = mainWindow->y() + 300;

    xsize[0] = 175;
    xsize[1] = xsize[0]*2+xbuffer;
    x[0] = xbuffer;
    x[1] = x[0] + xsize[0] + xbuffer;

    ysize[0] = 25;
    y[0] = ybuffer;
    y[1] = y[0] + ysize[0] + ybuffer;
    y[2] = y[1] + ysize[0] + ybuffer;
    y[3] = y[2] + ysize[0] + ybuffer;
    y[4] = y[3] + ysize[0] + ybuffer;

    winH = y[4] + ysize[0] + ybuffer;
    winW = x[1] + xsize[0] + xbuffer;

    fileWriterStreamControlWindow = new Fl_Double_Window(posX,posY,winW,winH,"Choose Which Channel to Write To File");
    fileWriterStreamControlWindow->begin();
    {
        Fl_Widget *w;
        char buf[128];

        sprintf (buf,"All Even Channels");
        w = writeEvenButton = new Fl_Light_Button (x[0],y[0],xsize[0],ysize[0]);
        w->copy_label(buf);
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        sprintf (buf,"All Odd Channels");
        w = writeOddButton = new Fl_Light_Button (x[1],y[0],xsize[0],ysize[0]);
        w->copy_label(buf);
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        w = writeAutoToFileFlagButton = new Fl_Light_Button (x[0],y[1],xsize[0],ysize[0],"Write Auto Triggers");
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        w = writeLevelToFileFlagButton = new Fl_Light_Button (x[1],y[1],xsize[0],ysize[0],"Write Level Triggers");
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        w = writeEdgeToFileFlagButton = new Fl_Light_Button (x[0],y[2],xsize[0],ysize[0],"Write Edge Triggers");
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        w = writeNoiseToFileFlagButton = new Fl_Light_Button (x[1],y[2],xsize[0],ysize[0],"Write Noise Triggers");
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);

        // the done button
        w = fileWriterStreamControlDoneButton = new Fl_Return_Button (x[0],y[4],xsize[1],ysize[0],"Done");
        w->callback((Fl_Callback*) fileWriterStreamControlWindow_handleEvent, (void*) this);
    }

    fileWriterStreamControlWindow->end();
    fileWriterStreamControlWindow->hide();
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::fileWriterStreamControlWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->fileWriterStreamControlWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::fileWriterStreamControlWindow_handleEvent(Fl_Widget *w)
{

    // done button
    if (w==fileWriterStreamControlDoneButton)
    {
        fileWriterStreamControlWindow->hide();
        return;
    }

    // all even button
    if (w==writeEvenButton)
    {
        for (unsigned int i=0;i<client->nDatatStreams();i+=2)
        {
            client->writePulsesToFileFlag(i,writeEvenButton->value());
        }
        fileWriterStreamControlWindow_userRefresh();
        return;
    }

    // all odd button
    if (w==writeOddButton)
    {
        for (unsigned int i=1;i<client->nDatatStreams();i+=2)
        {
            client->writePulsesToFileFlag(i,writeOddButton->value());
        }
        fileWriterStreamControlWindow_userRefresh();
        return;
    }

    // file writer buttons
    if (w==writeAutoToFileFlagButton)
    {
        client->writeAutoToFileFlag(writeAutoToFileFlagButton->value());
        return;
    }
    if (w==writeLevelToFileFlagButton)
    {
        client->writeLevelToFileFlag(writeLevelToFileFlagButton->value());
        return;
    }
    if (w==writeEdgeToFileFlagButton)
    {
        client->writeEdgeToFileFlag(writeEdgeToFileFlagButton->value());
        return;
    }
    if (w==writeNoiseToFileFlagButton)
    {
        client->writeNoiseToFileFlag(writeNoiseToFileFlagButton->value());
        return;
    }

    std::cout << "File Writer Stream Control Window got an event it couldn't handle" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::fileWriterStreamControlWindow_userRefresh ()
{

    // stream channel window refreshed
    if (fileWriterStreamControlWindow->visible())
    {
        bool even = true;
        bool odd  = true;
        for (unsigned int i=0;i<client->nDatatStreams();i++)
        {
            bool b = client->writePulsesToFileFlag(i);
            if((i%2)==0)
            {
                if (!b) even = false;
            }
            else
            {
                if (!b) odd = false;
            }
        }

        writeOddButton->value(odd);
        writeEvenButton->value(even);

        // the trigger write flags
        writeAutoToFileFlagButton->value(client->writeAutoToFileFlag());
        writeLevelToFileFlagButton->value(client->writeLevelToFileFlag());
        writeEdgeToFileFlagButton->value(client->writeEdgeToFileFlag());
        writeNoiseToFileFlagButton->value(client->writeNoiseToFileFlag());

    }

}


////////////////////////////////////////////////////////////////////////////////
// Group Trigger Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::groupTriggerWindow_setup ()
{
    int n = client->nDatatStreams();
    int xsize[3];                                 // slider
    int ysize[0];
    int x[20];
    int xbuffer = 5;
    int ybuffer = 1;
    int winW;
    int winH;
    int y;
    int posX = mainWindow->x() + 400;
    int posY = mainWindow->y() + 300;

    // group trigger window
    xsize[0]=65;                                  // channel
    xsize[1]=65;                                  // receiver / source
    xsize[2]=xsize[0] + xbuffer + xsize[1];
    ysize[0]=20;
    winH=(n+4)*(ysize[0]+ybuffer)+4*ybuffer;
    y = ybuffer;

    x[0] = xbuffer;                               // far left
    x[1] = x[0] + xsize[0] + xbuffer;             // label
    x[2] = x[1] + xsize[1] + xbuffer;             // label
    x[10] = x[0] + xsize[0]/2 - ysize[0]/2 + xbuffer;
    x[11] = x[1] + xsize[1]/2 - ysize[0]/2 + xbuffer;
    x[12] = x[2] + xsize[1]/2 - ysize[0]/2 + xbuffer;

    winW = x[2] + xsize[1] + xbuffer;

    // declare the arrays now that you have the size
    receiverButton = new Fl_Check_Button* [n];
    sourceButton = new Fl_Check_Button* [n];

    groupTriggerWindow = new Fl_Double_Window(posX,posY,winW,winH,"Group Trigger");
    groupTriggerWindow->begin();
    {
        Fl_Widget *w;
        char buf[128];

        // couple trigger odd to even button
        w = coupleOddToEvenButton = new Fl_Check_Button (x[0],y,ysize[0],ysize[0],"Couple FB -> Error");
        w->callback((Fl_Callback*) groupTriggerWindow_handleEvent, (void*) this);
        w->align(FL_ALIGN_RIGHT);
        y+= (ysize[0]+ybuffer);

        // couple trigger even to odd button
        w = coupleEvenToOddButton = new Fl_Check_Button (x[0],y,ysize[0],ysize[0],"Couple Error -> FB");
        w->callback((Fl_Callback*) groupTriggerWindow_handleEvent, (void*) this);
        w->align(FL_ALIGN_RIGHT);
        y+= (ysize[0]+ybuffer);

        // just a line
        w = new Fl_Button (x[0],y,winW-2*xbuffer,1);
        y+= 2*ybuffer;

        // titles
        w = new Fl_Output (x[0],y,xsize[0],0,"Channel");
        w->align(FL_ALIGN_BOTTOM);

        w = new Fl_Output (x[1],y,xsize[1],0,"Source");
        w->align(FL_ALIGN_BOTTOM);

        w = new Fl_Output (x[2],y,xsize[1],0,"Receiver");
        w->align(FL_ALIGN_BOTTOM);

        y+= (ysize[0]);
        for (int i = 0; i<n; i++)
        {

            // channel number
            sprintf (buf,"%d",i);
            w = new Fl_Output (x[10],y,0,ysize[0]);
            w->copy_label(buf);
            w->align(FL_ALIGN_RIGHT);

            // source
            w = sourceButton[i] = new Fl_Check_Button (x[11],y,xsize[1],ysize[0],"");
            w->callback((Fl_Callback*) groupTriggerWindow_handleEvent, (void*) this);

            // receiver
            w = receiverButton[i] = new Fl_Check_Button (x[12],y,xsize[1],ysize[0],"");
            w->callback((Fl_Callback*) groupTriggerWindow_handleEvent, (void*) this);

            // move down
            y+= (ysize[0]+ybuffer);
        }

        // the okay button
        y += ybuffer;
        w = triggerOkayButton = new Fl_Return_Button (xbuffer,y,xsize[2],ysize[0],"Okay");
        w->callback((Fl_Callback*) groupTriggerWindow_handleEvent, (void*) this);

    }
    groupTriggerWindow->end();
    groupTriggerWindow->hide();

}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::groupTriggerWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->groupTriggerWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::groupTriggerWindow_handleEvent(Fl_Widget *w)
{

    // the okay button
    if (w==triggerOkayButton)
    {
        groupTriggerWindow->hide();
        groupTriggerWindow_userRefresh();
        return;
    }

    // the couple trigger button
    if (w==coupleOddToEvenButton)
    {
        client->coupleOddToEvenFlag ( coupleOddToEvenButton->value() );
        groupTriggerWindow_userRefresh();
        return;
    }

    // the couple trigger button
    if (w==coupleEvenToOddButton)
    {
        client->coupleEvenToOddFlag ( coupleEvenToOddButton->value() );
        groupTriggerWindow_userRefresh();
        return;
    }

    // the is the subwindow stuff... loop though all channels
    for (unsigned int i=0;i<client->nDatatStreams();i++)
    {
        // group trigger stuff
        if (w==sourceButton[i])
        {
            client->streamData[i]->groupTrigSource(sourceButton[i]->value());
            return;
        }
        if (w==receiverButton[i])
        {
            client->streamData[i]->groupTrigReceiver(receiverButton[i]->value());
            return;
        }

    }

    std::cout << "Group Trigger got an event it couldn't handle" << std::endl;

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::groupTriggerWindow_userRefresh ()
{

    // update the couple trigger
    coupleOddToEvenButton->value(client->coupleOddToEvenFlag());

    // update the couple trigger
    coupleEvenToOddButton->value(client->coupleEvenToOddFlag());

    // don't we want to update the source / receiver buttens?

}


////////////////////////////////////////////////////////////////////////////////
// Mix Control Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mixControlWindow_setup ()
{
    Fl_Widget *w;
    int xsize[10];
    int ysize[10];
    int x[20];
    int y[20];
    int xbuffer = 5;
    int ybuffer = 5;
    int winW;
    int winH;
    int posX = mainWindow->x() + 400;
    int posY = mainWindow->y() + 300;

    // the y guides
    ysize[0] = 25;
    y[0]     = ysize[0] + ybuffer;
    y[1]     = y[0] + ysize[0] + ybuffer;
    y[2]     = y[1] + ysize[0] + ybuffer;
    y[3]     = y[2] + ysize[0] + ybuffer;
    y[4]     = y[3] + ysize[0] + ybuffer;
    y[5]     = y[4] + ysize[0] + ybuffer;
    y[6]     = y[5] + ysize[0] + ybuffer;
    winH     = y[6] + ysize[0] + ybuffer;

    // the x guides
    xsize[0] = 200;                               // default button size
    xsize[1] = 2*(xsize[0])+xbuffer;              // the entire length of the window minus two buffers

    x[0]     = xbuffer;
    x[1]     = x[0] + xsize[0] + xbuffer;
    winW     = x[1] + xsize[0] + xbuffer;

    // setup the window
    mixControlWindow = new Fl_Double_Window(posX,posY,winW,winH,"Mix Control Window");
    mixControlWindow->begin();
    {
        char buf[128];

        // a blank menu item
        Fl_Menu_Item channelPopup[] =
        {
            {"ALL", 0,(Fl_Callback*) mixControlWindow_handleEvent, (void*) this},
            {0}
        };

        // declare the channel selector and add that blank item to it
                                                  // dec chan
        w = mixChannelSelector = new Fl_Choice (x[0],y[0],xsize[0],ysize[0],"Channel");
        mixChannelSelector->copy(channelPopup,NULL);
        w->align(FL_ALIGN_TOP);

        // now add all of the other channels (don't put the even ones in b/c it doesn't make sense to "mix" a error signal)
        for (unsigned int j=1; j<client->nDatatStreams(); j+=2)
        {
            sprintf (buf,"Chan %d",j);
            mixChannelSelector->add(buf,0,(Fl_Callback*) mixControlWindow_handleEvent, (void*) this);
        }

        // mix level
                                                  // dec slider
        w = mixLevelInput = new Fl_Float_Input (x[1],y[0],xsize[0],ysize[0],"Percentage of Error In F/B");
        w->align(FL_ALIGN_TOP);
        mixLevelInput->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // mix flag
                                                  // mix flag
        w = mixFlagButton = new Fl_Light_Button (x[0],y[1],xsize[0],ysize[0],"Mix");
        w->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // mix inversion
                                                  // mix flag
        w = mixInversionFlagButton = new Fl_Light_Button (x[1],y[1],xsize[0],ysize[0],"Invert Error in Mix");
        w->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // couple pairs
                                                  // mix flag
        w = mixCouplePairsFlagButton = new Fl_Light_Button (x[0],y[3],xsize[0],ysize[0],"Couple Pairs");
        w->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // ratio method
        Fl_Menu_Item methodPopup[] =
        {
            {"lowest STD", 0,(Fl_Callback*) mixControlWindow_handleEvent, (void*) this},
            {"RMS Ratio", 0,(Fl_Callback*) mixControlWindow_handleEvent, (void*) this},
            {0}
        };
                                                  // dec chan
        w = mixRatioMethodSelector = new Fl_Choice (x[1],y[3],xsize[0],ysize[0],"Channel");
        mixRatioMethodSelector->copy(methodPopup,NULL);
        w->align(FL_ALIGN_TOP);

        // mix level
        w = mixRecordsInput = new Fl_Int_Input (x[0],y[5],xsize[0],ysize[0],"Records Used In Optimization");
        w->align(FL_ALIGN_TOP);
        mixRecordsInput->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // calculate optimal mix
        w = optimalMix_calculateMedianMixButton = new Fl_Return_Button (x[1],y[5],xsize[0],ysize[0],"Calculate Optimal Mix");
        w->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

        // the okay button
        w = mixControlOkayButton = new Fl_Return_Button (x[1],y[6],xsize[0],ysize[0],"Okay");
        w->callback((Fl_Callback*) mixControlWindow_handleEvent, (void*) this);

    }

    mixControlWindow->end();
    mixControlWindow->hide();

    // the progress bar
    winH = 1 * (ybuffer + ysize[0]) + ybuffer;
    posY += y[1] + ybuffer;
    posX += xbuffer;
    mixControlProgressWindow = new Fl_Double_Window(posX,posY,winW,winH,"Optimizing Mix Level");
    mixControlProgressWindow->begin();
    {
        mixControlProgressBar = new Fl_Progress (x[0],ybuffer,xsize[1],ysize[0],"Optimizing Progress");
        mixControlProgressBar->minimum(0.);
        mixControlProgressBar->maximum(1.);
        w->align(FL_ALIGN_INSIDE);
    }
    mixControlProgressWindow->end();
    mixControlProgressWindow->hide();

}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::mixControlWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->mixControlWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::mixControlWindow_handleEvent(Fl_Widget *w)
{

    // channel selector
    if (w==mixChannelSelector)
    {
        mixControlWindow_userRefresh();
        return;
    }

    // mix level
    if (w==mixLevelInput)
    {
        float f;
        sscanf(mixLevelInput->value(),"%f",&f);
        f /= 100;
        if (mixChannelSelector->value() == 0)
        {
            bool everyoneIsTheSame = true;
            float masterLevel = client->mixLevel(0);

            for (unsigned int chan=0;chan<client->nDatatStreams(); chan++)
            {
                if (masterLevel != client->mixLevel(chan))
                {
                    std::cout << "different mix levels" <<std::endl;
                    everyoneIsTheSame = false;
                }
            }

            if (everyoneIsTheSame)
            {
                client->mixLevel(XCD_ALLCHANNELS,f);
            }
            else
            {
                // the user entered something and the values of the various channel differ
                // confirm with user before sending command
                int resp = fl_choice("Channels currently have non-uniform mix levels.  Are you sure you want to update all channels with this mix level?",
                    "Cancel","Yes, Update all channels", NULL);
                if (resp == 1)
                {
                    client->mixLevel(XCD_ALLCHANNELS,f);
                }
            }

        }
        else
        {
                                                  // only odd values are shown
            unsigned int chan = (2*mixChannelSelector->value()) - 1;
            client->mixLevel(chan,f);
        }
        mixControlWindow_userRefresh();
        return;
    }

    //mix flag
    if (w==mixFlagButton)
    {
        if (mixChannelSelector->value() == 0)
        {
            bool everyoneIsTheSame = true;
            bool masterMixFlag = client->mixFlag(0);

            for (unsigned int chan=0;chan<client->nDatatStreams(); chan++)
            {
                if (masterMixFlag != client->mixFlag(chan))
                {
                    std::cout << "different mix enabled" <<std::endl;
                    everyoneIsTheSame = false;
                }
            }

            if (everyoneIsTheSame)
            {
                client->mixFlag(XCD_ALLCHANNELS,mixFlagButton->value());
            }
            else
            {
                // the user entered something and the values of the various channel differ
                // confirm with user before sending command
                int resp;
                if (mixFlagButton->value())
                {
                    resp = fl_choice("Some Channels are already enabled. Are you sure you want to enable mixing for all channels?",
                        "Cancel","Yes, Enable Mixing for All Channel", NULL);
                }
                else
                {
                    //if there is a mix of on / off i hardcoded the button to be TRUE.  therefore you should only see the following
                    resp = fl_choice("Some Channels are still enabled. Are you sure you want to disable mixing for all channels?",
                        "Cancel","Yes, Disable Mixing for All Channel", NULL);
                }

                if (resp == 1)
                {
                    client->mixFlag(XCD_ALLCHANNELS,mixFlagButton->value());
                }
            }

        }
        else
        {
                                                  // only odd values are shown
            unsigned int chan = (2*mixChannelSelector->value()) - 1;
            client->mixFlag(chan,mixFlagButton->value());
        }
        mixControlWindow_userRefresh();
        return;
    }

    // mix inversion
    if (w==mixInversionFlagButton)
    {
        if (mixChannelSelector->value() == 0)
        {
            bool everyoneIsTheSame = true;
            bool masterInvertFlag = client->mixInversionFlag(0);

            for (unsigned int chan=0;chan<client->nDatatStreams(); chan++)
            {
                if (masterInvertFlag != client->mixInversionFlag(chan))
                {
                    std::cout << "different inversion enabled" <<std::endl;
                    everyoneIsTheSame = false;
                }
            }

            if (everyoneIsTheSame)
            {
                client->mixInversionFlag(XCD_ALLCHANNELS,mixInversionFlagButton->value());
            }
            else
            {
                // the user entered something and the values of the various channel differ
                // confirm with user before sending command
                int resp;
                if (mixInversionFlagButton->value())
                {
                    resp = fl_choice("Some Channels are already inverted. Are you sure you want to enable inversion for all channels?",
                        "Cancel","Yes, Invert the Mix for All Channel", NULL);
                }
                else
                {
                    //if there is a mix of on / off, i hardcoded the button to be TRUE.  therefore you should only see the following
                    resp = fl_choice("Some Channels are currently inverted. Are you sure you want to turn off inversion for all channels?",
                        "Cancel","Yes, Turn Off Inversion for All Channel", NULL);
                }

                if (resp == 1)
                {
                    client->mixInversionFlag(XCD_ALLCHANNELS,mixInversionFlagButton->value());
                }
            }
        }
        else
        {
                                                  // only odd values are shown
            unsigned int chan = (2*mixChannelSelector->value()) - 1;
            client->mixInversionFlag(chan,mixInversionFlagButton->value());
        }
        mixControlWindow_userRefresh();
        return;
    }

    // method selector
    if (w==mixRatioMethodSelector)
    {
        client->optimizeMixRatioMethod(mixRatioMethodSelector->value());
        return;
    }

    // pair couple button
    if (w==mixCouplePairsFlagButton)
    {
        if (mixCouplePairsFlagButton->value() == 0)
        {
            client->optimizeMixCouplePairsFlag(false);
        }
        else
        {
            client->optimizeMixCouplePairsFlag(true);
        }
        return;
    }

    // mix Records input
    if (w==mixRecordsInput)
    {
        int d;
        sscanf(mixRecordsInput->value(),"%d",&d);
        client->optimizeMixMaxRecords(d);
        mixControlWindow_userRefresh();
        return;
    }

    // calculate optimal mix button
    if (w==optimalMix_calculateMedianMixButton)
    {
        unsigned int channel;
        if (mixChannelSelector->value() == 0)
        {
            channel = XCD_ALLCHANNELS;
        }
        else
        {
                                                  // only odd values are shown
            channel = (2*mixChannelSelector->value()) - 1;
        }

        if (!client->optimalMix_start(channel))
        {
            int resp = fl_choice("You are trying to optimize a channel that has the error already mixed in! What do you want me to do?",
                "Cancel","Turn them off for me and try again", NULL);
            if (resp == 1)
            {
                // turn off the mix
                client->mixFlag(channel,false);

                // recall this button push
                mixControlWindow_handleEvent(w);
            }
        }

        // deactive the window
        mixControlWindow->deactivate();

        // show the progress bar
        mixControlProgressWindow->show();

        float f;
        char buf[128];
        do
        {
            // get the progress (number of frames collected / total number of frames required)
            f = client->optimalMix_progress();

            // update the progress bar
            mixControlProgressBar->value(f);

            // update the title
            if (channel == XCD_ALLCHANNELS)
            {
                sprintf (buf,"Optimizing All Channels - %%%5.1f",100.*f);
            }
            else
            {
                sprintf (buf,"Optimizing Channel %d - %%%5.1f",channel,100.*f);
            }
            mixControlProgressBar->copy_label(buf);

            // tell fltk to paint it NOW
            Fl::check();

            // take a little nap
            usleep (1000);

        } while (f < 1.);                         // wait until done

        // pause a little to let it calculate the mix from the collected frames
        // that way when you paint the user refresh, it paints the new value to screen
        usleep (500000);                          // sometimes if it takes a long time to calculate, it refreshes b/f it is done calculating

        // hide the progress bar
        mixControlProgressWindow->hide();

        // activate the control window
        mixControlWindow->activate();

        // refresh the screen
        mixControlWindow_userRefresh();

        return;
    }

    // the okay button
    if (w==mixControlOkayButton)
    {
        mixControlWindow->hide();
        return;
    }

    std::cout << "Mix Control got an event it couldn't handle" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mixControlWindow_userRefresh ()
{
    char output[256];

    // mix button
    if (mixChannelSelector->value() == 0)
    {
        // go through all the channels and make sure they are all the same

        // test for level
        bool everyoneIsTheSame = true;
        float masterLevel  = client->mixLevel(0);
        for (unsigned int chan=0; chan<client->nDatatStreams(); chan++)
        {
            if (masterLevel != client->mixLevel(chan))
            {
                everyoneIsTheSame = false;
            }
        }

        // update the level
        if (everyoneIsTheSame)
        {
            sprintf(output,"%f",masterLevel*100.);
            mixLevelInput->value(output);
        }
        else
        {
            sprintf(output,"(various)");
            mixLevelInput->value(output);
        }

        // test for enable flag
        everyoneIsTheSame = true;
        bool  masterFlag = client->mixFlag(0);
        for (unsigned int chan=0; chan<client->nDatatStreams(); chan++)
        {
            if (masterFlag != client->mixFlag(chan))
            {
                everyoneIsTheSame = false;
            }
        }

        // update the enable flag
        if (everyoneIsTheSame)
        {
            mixFlagButton->copy_label("Mix Enabled");
            mixFlagButton->value(masterFlag);
        }
        else
        {
            mixFlagButton->copy_label("Mix Enabled (various)");
            mixFlagButton->value(true);
        }

        // test for inversion flag
        everyoneIsTheSame = true;
        bool  masterInvert = client->mixInversionFlag(0);
        for (unsigned int chan=0; chan<client->nDatatStreams(); chan++)
        {
            if (masterInvert != client->mixInversionFlag(chan))
            {
                everyoneIsTheSame = false;
            }
        }

        // update the invert
        if (everyoneIsTheSame)
        {
            mixInversionFlagButton->copy_label("Inversion Enabled");
            mixInversionFlagButton->value(masterInvert);
        }
        else
        {
            mixInversionFlagButton->copy_label("Inversion Enabled (various)");
            mixInversionFlagButton->value(true);
        }

    }
    else
    {

        // user has one channel selected
        unsigned int chan = (2 * mixChannelSelector->value()) - 1;

        // send that channel's mix flag to the mix flag button
        mixFlagButton->value(client->mixFlag(chan));
        mixFlagButton->copy_label("Mix Enabled");

        // send that channel's inversion flag to the inversion flag button
        mixInversionFlagButton->value(client->mixInversionFlag(chan));
        mixInversionFlagButton->copy_label("Inversion Enabled");

        // get that channels mix level
        sprintf(output,"%f",client->mixLevel(chan)*100.);

        // output that level to the mix level input
        mixLevelInput->value(output);

    }

    // set the couple pairs button
    mixCouplePairsFlagButton->value (client->optimizeMixCouplePairsFlag());

    // set the method
    mixRatioMethodSelector->value (client->optimizeMixRatioMethod());

    // get the max records value
    sprintf(output,"%d",client->optimizeMixMaxRecords());

    // output that value to the  input
    mixRecordsInput->value(output);

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::mixControlWindow_loopRefresh ()
{

}


////////////////////////////////////////////////////////////////////////////////
// Trigger Rate Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerRateWindow_setup ()
{
    // probably belongs in an init function but whatever
    triggerRateIntensity = 10;

    int n          = client->nDatatStreams();
    int numColumns = client->nMuxCols();
    int numRows    = client->nMuxRows();

    int xbuffer = 15;
    int ybuffer = 15;
    int xsize[2];
    int ysize[2];
    int x;
    int y;
    int winW;
    int winH;
    int posX = mainWindow->x() + 000;
    int posY = mainWindow->y() + 300;
    xsize[1]=85;
    ysize[1]=25;

    xsize[0]=28;
    ysize[0]=xsize[0];

    y = ybuffer + ysize[0] + ybuffer;

    winH  = ybuffer + ((numRows) * (ysize[0] + ybuffer));
    if (winH < (3*ysize[1] + 6*ybuffer))
    {
        winH = (3*ysize[1] + 6*ybuffer);
    }
    winW  = (2 * numColumns) * (xsize[0] + xbuffer);
    winW += (xsize[1] + xbuffer) + xbuffer;

    // declare the arrays now that you have the size
    triggerRateIndicatorBox = new Fl_Box* [n];

    triggerRateWindow = new Fl_Double_Window(posX,posY,winW,winH,"Trigger Rates");
    triggerRateWindow->begin();
    {
        Fl_Widget *w;
        char buf[128];

        x = xbuffer;
        y = xbuffer;
        // loop through and paint the rate indicators
        for (int i = 0; i<n; i+=1)
        {
            x = xbuffer + (2*(i / (2*numRows)) + i % 2) * (xsize[0] + xbuffer);
            y = ybuffer + (i / 2 - (numRows)*(i / (2*numRows))) * (ysize[0] + ybuffer);
            //printf ("%d %d %d %d\n",i / (2*numRows),(numRows)*(i / (2*numRows)),i/2,i / 2 - (numRows)*(i / (2*numRows)));
            //printf ("[%d,%d] \n",x,y);

            // the box
            w = triggerRateIndicatorBox[i] = new Fl_Box (x,y,xsize[0],ysize[0]);
            w->box(FL_BORDER_BOX);
            w->labelsize(10);

            // the static top label
            w = new Fl_Box (x,y,xsize[0],0);
            w->align(FL_ALIGN_TOP | FL_ALIGN_CENTER);
            if (i % 2 == 0)
            {
                sprintf (buf,"%d ER",i/2);
            }
            else
            {
                sprintf (buf,"%d FB",i/2);
            }
            w->copy_label(buf);
            w->labelsize(9);

            // the static right label
            w = new Fl_Box (x+xsize[0]-2,y,0,ysize[0]);
            w->align(FL_ALIGN_RIGHT);
            sprintf (buf,"%d",i);
            w->copy_label(buf);
            w->labelsize(9);

        }

        x += (xbuffer +xsize[0]) + xbuffer/2;
        y  = ybuffer + ybuffer;
        w  = triggerIntegrationTimeInput = new Fl_Float_Input (x,y,xsize[1],ysize[1],"Integration Time");
        w->align(FL_ALIGN_TOP);
        w->callback((Fl_Callback*) triggerRateWindow_handleEvent, (void*) this);

        y += ysize[1] + ybuffer + ybuffer;
        w = triggerRateIntensityInput = new Fl_Float_Input (x,y,xsize[1],ysize[1],"Trigger Intensity");
        w->align(FL_ALIGN_TOP);
        w->callback((Fl_Callback*) triggerRateWindow_handleEvent, (void*) this);

        y += ysize[1] + ybuffer;
        w = triggerRateOkayButton = new Fl_Return_Button (x,y,xsize[1],ysize[1],"OKAY");
        w->callback((Fl_Callback*) triggerRateWindow_handleEvent, (void*) this);

    }

    triggerRateWindow->end();
    triggerRateWindow->show();
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerRateWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->triggerRateWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerRateWindow_handleEvent(Fl_Widget *w)
{
    if (w == triggerIntegrationTimeInput)
    {
        float f;
        sscanf(triggerIntegrationTimeInput->value(),"%f",&f);
        client->triggerRateIntegrationTime((double)f);
        triggerRateWindow_userRefresh();
        return;
    }

    if (w == triggerRateIntensityInput)
    {
        float f;
        sscanf(triggerRateIntensityInput->value(),"%f",&f);
        triggerRateIntensity = f;
        printf ("Setting Trigger Rate Intensity to %f\n",triggerRateIntensity);
        triggerRateWindow_userRefresh();
        return;
    }

    if (w == triggerRateOkayButton)
    {
        triggerRateWindow->hide();
        return;
    }

    std::cout << "Trigger Rate got an event it couldn't handle" << std::endl;

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerRateWindow_userRefresh ()
{
    char buf[128];

    // trigger rate window
    sprintf(buf,"%f", client->triggerRateIntegrationTime());
    triggerIntegrationTimeInput->value(buf);

    // trigger rate intensity
    sprintf(buf,"%f", triggerRateIntensity);
    triggerRateIntensityInput->value(buf);

}


////////////////////////////////////////////////////////////////////////////////
void ClientGUI::triggerRateWindow_loopRefresh ()
{
    // one out of every 5 get a buffer level
    if (loopCounter % 3 != 0) return;

    double rate;
    double r1,r2,r;
    double g1,g2,g;
    double b1,b2,b;
    char buf[128];

    // yellow
    r1 = 255;
    g1 = 255;
    b1 = 0;

    // dark red
    r2 = 225;
    g2 = 0;
    b2 = 0;

    for (unsigned int chan=0; chan<client->nDatatStreams(); chan++)
    {
        rate = client->streamData[chan]->triggerRate();

        if (rate > triggerRateIntensity)
        {
            r = r2;
            b = b2;
            g = g2;
        }
        else
        {
            if (rate == 0)
            {
                b = 255;
                g = 255;
                r = 255;
            }
            else
            {
                b = b1 + (rate / triggerRateIntensity) * (b2-b1);
                g = g1 + (rate / triggerRateIntensity) * (g2-g1);
                r = r1 + (rate / triggerRateIntensity) * (r2-r1);
            }
        }

        r = round(r);
        g = round(g);
        b = round(b);
        //printf ("[%f,%f,%f]\n",r,g,b);

        sprintf (buf,"%3.1f",rate);
        triggerRateIndicatorBox[chan]->color(fl_rgb_color((uchar)r,(uchar)g,(uchar)b));
        triggerRateIndicatorBox[chan]->copy_label(buf);
    }

    Fl::check();

}


////////////////////////////////////////////////////////////////////////////////
// LJH Sub Window Functions
////////////////////////////////////////////////////////////////////////////////
void ClientGUI::LJHWindow_setup ()
{
    int n = 40;
    int xsize[2];
    int ysize[0];
    int x[2];
    int xbuffer = 3;
    int ybuffer = 3;
    int winW;
    int winH;
    int y;
    int posX = mainWindow->x();
    int posY = mainWindow->y();

    {
        xsize[0]=250;
        xsize[1]=250;
        ysize[0]=20;
        y = ybuffer;

        x[0] = xbuffer;
        x[1] = x[0] + xsize[0] + xbuffer;

        winH=(n+1)*(ysize[0]+ybuffer)+ybuffer;
        winW = x[1] + xsize[1] + xbuffer;

        ljhWindow = new Fl_Double_Window(posX,posY,winW,winH,"Config for LJH");
        ljhWindow->begin();
        {
            Fl_Widget *w;

            for (int i = 0; i<n; i++)
            {
                // key output
                w = keyOutput[i] = new Fl_Output (x[0],y,xsize[0],ysize[0]);
                keyOutput[i]->value(client->XOUTWriter()->title[i]);
                w->callback((Fl_Callback*) LJHWindow_handleEvent, (void*) this);
                w->deactivate();

                // value input
                w = valueInput[i] = new Fl_Input(x[1],y,xsize[1],ysize[0]);
                valueInput[i]->value(client->XOUTWriter()->header[i]);
                //if ((i==5)
                w->callback((Fl_Callback*) LJHWindow_handleEvent, (void*) this);

                // skip some that the user should mess with
                if (i==0) continue;
                if (i==1) continue;
                if (i==2) continue;
                if (i==23) continue;              //skip 'em all!
                if (i==24) continue;              //skip 'em all!
                if (i==25) continue;              //skip 'em all!
                if (i==26) continue;              //skip 'em all!
                if (i==27) continue;              //skip 'em all!

                // move down
                y+= (ysize[0]+ybuffer);
            }

            w = ljhOkayButton = new Fl_Return_Button (x[0],y,xsize[0],ysize[0],"Okay");
            w->callback((Fl_Callback*) LJHWindow_handleEvent, (void*) this);

            w = ljhSaveButton = new Fl_Return_Button (x[1],y,xsize[0],ysize[0],"Save Config File");
            w->callback((Fl_Callback*) LJHWindow_handleEvent, (void*) this);
        }
        ljhWindow->end();

    }
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::LJHWindow_handleEvent(Fl_Widget *w,void *v)
{
    ((ClientGUI *)v)->LJHWindow_handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::LJHWindow_handleEvent(Fl_Widget *w)
{
    if (w==ljhOkayButton)
    {
        ljhWindow->hide();
        return;
    }

    if (w==ljhSaveButton)
    {
        client->XOUTWriter()->saveCFGFile();
        return;
    }

    for (int i=0;i<40;i++)
    {
        if (w==valueInput[i])
        {
            std::cout << "New value for line " << i << ": " << valueInput[i]->value() << std::endl;
            sprintf(client->XOUTWriter()->header[i],"%s",valueInput[i]->value());
            LJHWindow_userRefresh();
            return;
        }
    }

    std::cout << "LJH got an event it couldn't handle" << std::endl;
}


/////////////////////////////////////////////////////////////////////////////
void ClientGUI::LJHWindow_userRefresh()
{
    for (int i=0;i<40;i++)
    {
        keyOutput[i]->value(client->XOUTWriter()->title[i]);
        valueInput[i]->value(client->XOUTWriter()->header[i]);
    }
}


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
