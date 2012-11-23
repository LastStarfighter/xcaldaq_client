/*
 *  plotWindow.h
 *  xcaldaq_client
 *
 *  Created by Jonathan King on 9/24/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

/*
 *  plotWindow.h
 *  fltk
 *
 *  Created by Jonathan King on 1/19/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef _plotWindow_h
#define _plotWindow_h

// local includes
#include "XPulseRec.h"

// system includes
#include <cmath>
#include <vector>

// fltk includes
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Output.H>
#include <Fl/Fl_Pack.H>
#include <Fl/Fl_Box.H>
#include <Fl/Fl_Button.H>
#include <Fl/Fl_Overlay_Window.H>
#include <Fl/fl_draw.H>

// local fltk includes
#include "Cartesian.h"

// macros
#define MAX_PLOT_POINTS 5000

///////////////////////////////////////////////////////////////////////////////

class plotWindow : public Fl_Overlay_Window
{

    public:
        // constructor
        plotWindow (int X,int Y,int W,int H,const char*L=0);

        // command functions
        void init              ( void );          // initializes the class
        void position          ( int, int );      // move the window
        void loopRefresh       ( void );
                                                  // copy the data internally
        int  ingestAndCopyData ( double*, double*, int );
                                                  // keep the data and dispose of it internally
        int  ingestAndKeepData ( double*, double*, int );
        void addHorLine        ( unsigned int traceNumber, double y);
        void addVertLine       ( unsigned int traceNumber, double x);
        void clearData         ( void );
        void resetPlotXMax     ( void );
        void resetPlotYMax     ( void );
        void color             ( unsigned int traceNumber, Fl_Color color);

        // the stuff that you need to flush out to inherit from fl_overlay_window (an abstract class)
        void draw_overlay ( void );
        void resize       ( int X,int Y,int W,int H );
        int  handle       ( int );

        // set only
        void xTickLabel ( const char * );
        void yTickLabel ( const char * );
        void xTickUnit  ( const char * );
        void yTickUnit  ( const char * );

        // the x and y ranging
        void xDisplayMethod ( int d );
        void yDisplayMethod ( int d );

        // encapsulation
        bool  xLog              ( void );
        void  xLog              ( bool );
        bool  yLog              ( void );
        void  yLog              ( bool );
        float xScale            ( void );
        void  xScale            ( float );
        float yScale            ( void );
        void  yScale            ( float );
        void  connectedLineFlag ( bool b );
        bool  connectedLineFlag ( void );
        bool  xManual           ( void );
        bool  yManual           ( void );
        void  yLinLogVisibleFlag ( bool );
        bool  yLinLogVisibleFlag ( void );
        void  xLinLogVisibleFlag ( bool );
        bool  xLinLogVisibleFlag ( void );

    private:
        void        setup             ( void );
        static void handleEvent       ( Fl_Widget *, void * );
        void        handleEvent       ( Fl_Widget * );
        void        userRefresh       ( void );
        void        updateTraces      ( void );
        void        updatePlotRange   ( void );
                                                  // get some stat data
        void        analyzeTrace      ( struct traceStructure *trace );

        // a vector of traces
        std::vector <struct traceStructure *> traceVector;

        // windowing widgets
        int m_posX, m_posY;                       // position on the screen of the window
        int m_winW, m_winH;                       // width and height of the window
        int m_offX, m_offY;                       // offset of the group inside it's container

        // flags
        bool m_xAxisActive;                       // x if true, y if false
        bool m_xAuto,   m_yAuto;
        bool m_xMax,    m_yMax;
        bool m_xManual, m_yManual;
        bool m_xLog,    m_yLog;
        bool m_xScroll;
        bool m_connectedLineFlag;

        // the range variables
                                                  // x information about the data
        float m_xminData,   m_xmaxData,   m_xrangeData;
                                                  // y information about the data
        float m_yminData,   m_ymaxData,   m_yrangeData;
                                                  // x information about the window
        float m_xminWin,    m_xmaxWin,    m_xrangeWin;
                                                  // y information about the window
        float m_yminWin,    m_ymaxWin,    m_yrangeWin;

        // scale variables
        float m_xScale;
        float m_yScale;

        // tick labels
        char m_xTickLabel [ 32 ];
        char m_yTickLabel [ 32 ];
        char m_xTickUnit  [ 32 ];
        char m_yTickUnit  [ 32 ];

        // mouse variables
        int mouseX;
        int mouseY;
        int xhold1,xhold2;
        int yhold1,yhold2;
        bool hold1Flag;
        bool hold2Flag;
        bool crossHairsFlag;
        bool deltaModeFlag;
        int canvasTop, canvasBottom, canvasLeft,canvasRight;

        // visibilty variables
        bool m_xLinLogVisibleFlag;
        bool m_yLinLogVisibleFlag;

        // scale widgets
        Fl_Group        *scaleRadioGroup;
        Fl_Round_Button *xScaleButton;
        Fl_Round_Button *yScaleButton;

        // xscale widgets
        Fl_Group        *xRangeGroup;
        Fl_Round_Button *xAutoButton;
        Fl_Round_Button *xMaxButton;
        Fl_Round_Button *xManualButton;
        Fl_Round_Button *xScrollButton;
        Fl_Float_Input  *xminInput, *xmaxInput, *xrangeInput;
        Fl_Group        *xLinLogGroup;
        Fl_Light_Button *xLinButton;
        Fl_Light_Button *xLogButton;

        // yscale widgets
        Fl_Group        *yRangeGroup;
        Fl_Round_Button *yAutoButton;
        Fl_Round_Button *yMaxButton;
        Fl_Round_Button *yManualButton;
        Fl_Float_Input  *yminInput, *ymaxInput, *yrangeInput;
        Fl_Group        *yLinLogGroup;
        Fl_Light_Button *yLinButton;
        Fl_Light_Button *yLogButton;

        // crosshairs widget
        Fl_Group         *crossHairGroup;
        Fl_Light_Button  *crossHairButton;
        Fl_Light_Button  *singleCrossHairButton;
        Fl_Light_Button  *deltaCrossHairButton;

        // canvas widgets
        Ca_Canvas *canvas;
        Ca_X_Axis *x_axis;
        Ca_Y_Axis *y_axis;
        Fl_Input  *mousePositionInput;
};

struct traceStructure
{
    double *xData;
    double *yData;
    unsigned int numSamples;
    double xMin;
    double xMax;
    double yMin;
    double yMax;
    bool xSafeForLogFlag;
    bool ySafeForLogFlag;
    bool xClipFlag;
    bool yClipFlag;
    double vertLine;
    double horLine;
    bool horLineFlag;
    bool vertLineFlag;
    Fl_Color color;

};

///////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
