////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __PulseHeightDisplay_h
#define __PulseHeightDisplay_h
////////////////////////////////////////////////////////////////////////////////

// system includes
#include <iostream>                               // standard c++ in put and output stream functions
#include <string>                                 // string library

// local includes
#include "Client.h"
#include "XPulseRec.h"

// fltk includes
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Item.H>
#include <Fl/Fl_Output.H>
#include <FL/Fl_Return_Button.H>

// local fltk includes
#include "plotWindow.h"

// local macros
#define INDEPENDENT_TRACES 8

class PulseAnalysisDisplay
{

    public:
        // constructor
        PulseAnalysisDisplay (xdaq::Client* );

        // public functions
        void init        ( void );                // get the window ready for display
        void show        ( void );                // paints window on screen
        void hide        ( void );                // paints window on screen
        void stop        ( void );                // remove the windows from the screen
        void loopRefresh ( void );                // what gets updated in a loop (must be public so others can call it!)
        void position    ( int x, int y );

        // encapsulation
        bool visible();

    private:
        //private functions
        void              setup       ( void );   // actually place the widgets into the window
        void              userRefresh ( void );   // reprints user data on the screen
        static void       handleEvent ( Fl_Widget *, void * );
        void              handleEvent ( Fl_Widget * );

        // private variables
        xdaq::Client     *client;                 // link to the client from which it will get it's plotting info

        // windowing widgets
        Fl_Double_Window *window;                 // the container window
        plotWindow       *plot;                   // the plot window

        Fl_Choice        *channelSelector[INDEPENDENT_TRACES];
        Fl_Choice        *plotSelector;
        Fl_Choice        *traceColorSelector[INDEPENDENT_TRACES];
        Fl_Light_Button  *performAnalysisFlagButton;
};

////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
