/*
 *  plotWindow.cpp
 *  fltk
 *
 *  Created by Jonathan King on 1/19/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "plotWindow.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/////////////////////////////////////////////////////////////////////////////
// constructors
/////////////////////////////////////////////////////////////////////////////
plotWindow::plotWindow (int X,int Y,int W,int H,const char*L) : Fl_Overlay_Window(X,Y,W,H,L)
{

    // some initialization variables
    m_xAxisActive = false;
    m_xAuto = true;
    m_yAuto = false;
    m_xMax = false;
    m_yMax = true;
    m_xManual = false;
    m_yManual = false;
    m_xLog = false;
    m_yLog = false;
    m_connectedLineFlag = true;

    crossHairsFlag = false;
    deltaModeFlag = false;
    hold1Flag = false;
    hold2Flag = false;

    // init the ticks and units
    sprintf (m_xTickLabel,"xTick");
    sprintf (m_yTickLabel,"yTick");
    sprintf (m_xTickUnit,"xUnit");
    sprintf (m_yTickUnit,"yUnit");

    // setup the window for displaying
    setup();

    // refresh the inputs
    userRefresh();

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::init()
{

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::resize(int X,int Y,int W,int H)
{
    Fl_Overlay_Window::resize(X,Y,W,H);
    redraw();
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::draw_overlay()
{
    double xValue;
    double yValue;
    char buffer[128];
    static bool printedIt = false;

    if (deltaModeFlag)
    {
        if (hold1Flag)
        {
            // plot the hold1 lines
            fl_color(FL_BLUE);
            fl_line_style(FL_SOLID, 2);
                                                  // horizontal line
            fl_line(canvasLeft, yhold1, canvasRight, yhold1);
                                                  // vertical line
            fl_line(xhold1, canvasTop, xhold1, canvasBottom);

            if (hold2Flag)
            {
                // plot the hold2 lines, print the delta
                fl_color(FL_BLUE);
                fl_line_style(FL_SOLID, 2);
                                                  // horizontal line
                fl_line(canvasLeft, yhold2, canvasRight, yhold2);
                                                  // vertical line
                fl_line(xhold2, canvasTop, xhold2, canvasBottom);

                double xValue = fabs(x_axis->value(xhold1) - x_axis->value(xhold2));
                double yValue = fabs(y_axis->value(yhold1) - y_axis->value(yhold2));

                sprintf (buffer,"[%7.2f(%s), %7.2f(%s)]",xValue,m_xTickUnit,yValue,m_yTickUnit);
                if (!printedIt)
                {
                    printf ("Delta Mode: %s\n",buffer);
                    printedIt = true;
                }

                //fl_draw(buffer, xhold2 + 2, yhold2 - 6); // text
                if ((xhold2 + 150) > w())
                {
                    fl_draw(buffer, xhold2 -195, yhold2 - 25 , 200, 25, FL_ALIGN_LEFT);
                }
                else
                {
                    fl_draw(buffer, xhold2 +5, yhold2 - 25 , 200, 25, FL_ALIGN_RIGHT);
                }
            }
            else
            {
                // plot the mouse position, print the delta
                fl_color(FL_BLACK);
                fl_line_style(FL_SOLID, 0);
                if ((mouseY > canvasTop) & (mouseY < canvasBottom))
                {
                    if ((mouseX > canvasLeft) & (mouseX < canvasRight))
                    {
                                                  // horizontal line
                        fl_line(canvasLeft, mouseY, canvasRight, mouseY);
                                                  // vertical line
                        fl_line(mouseX, canvasTop, mouseX, canvasBottom);

                        double xValue = fabs(x_axis->value(xhold1) - x_axis->value(mouseX));
                        double yValue = fabs(y_axis->value(yhold1) - y_axis->value(mouseY));

                        sprintf (buffer,"[%7.2f(%s),%7.2f(%s)]",xValue,m_xTickUnit,yValue,m_yTickUnit);
                        printedIt = false;

                        if ((mouseX + 150) > w())
                        {
                            fl_draw(buffer, mouseX -195, mouseY - 25 , 200, 25, FL_ALIGN_LEFT);
                        }
                        else
                        {
                            fl_draw(buffer, mouseX +5, mouseY - 25 , 200, 25, FL_ALIGN_RIGHT);
                        }
                    }
                }
            }
        }
        else
        {
            // plot the mouse position
            fl_color(FL_BLACK);
            fl_line_style(FL_SOLID, 0);
            if ((mouseY > canvasTop) & (mouseY < canvasBottom))
            {
                if ((mouseX > canvasLeft) & (mouseX < canvasRight))
                {
                                                  // horizontal line
                    fl_line(canvasLeft, mouseY, canvasRight, mouseY);
                                                  // vertical line
                    fl_line(mouseX, canvasTop, mouseX, canvasBottom);

                    xValue = x_axis->value(mouseX);
                    yValue = y_axis->value(mouseY);

                    sprintf (buffer,"[%7.2f(%s),%7.2f(%s)]",xValue,m_xTickUnit,yValue,m_yTickUnit);
                    printedIt = false;

                    if ((mouseX + 150) > w())
                    {
                        fl_draw(buffer, mouseX -195, mouseY - 25 , 200, 25, FL_ALIGN_LEFT);
                    }
                    else
                    {
                        fl_draw(buffer, mouseX +5, mouseY - 25 , 200, 25, FL_ALIGN_RIGHT);
                    }

                }
            }

        }

    }
    else
    {
        // single mode
        if (hold1Flag)
        {
            // plot the hold1 lines
            fl_color(FL_BLUE);
            fl_line_style(FL_SOLID, 2);
                                                  // horizontal line
            fl_line(canvasLeft, yhold1, canvasRight, yhold1);
                                                  // vertical line
            fl_line(xhold1, canvasTop, xhold1, canvasBottom);

            xValue = x_axis->value(xhold1);
            yValue = y_axis->value(yhold1);
            sprintf (buffer,"[%7.2f(%s), %7.2f(%s)]",xValue,m_xTickUnit,yValue,m_yTickUnit);
            if (!printedIt)
            {
                printf ("Single Mode: %s\n",buffer);
                printedIt = true;
            }

            if ((xhold1 + 150) > w())
            {
                fl_draw(buffer, xhold1 -195, yhold1 - 25 , 200, 25, FL_ALIGN_LEFT);
            }
            else
            {
                fl_draw(buffer, xhold1 +5, yhold1 - 25 , 200, 25, FL_ALIGN_RIGHT);
            }
        }
        else
        {
            // plot the mouse position
            fl_color(FL_BLACK);
            fl_line_style(FL_SOLID, 0);
            if ((mouseY > canvasTop) & (mouseY < canvasBottom))
            {
                if ((mouseX > canvasLeft) & (mouseX < canvasRight))
                {
                                                  // horizontal line
                    fl_line(canvasLeft, mouseY, canvasRight, mouseY);
                                                  // vertical line
                    fl_line(mouseX, canvasTop, mouseX, canvasBottom);

                    double xValue = x_axis->value(mouseX);
                    double yValue = y_axis->value(mouseY);

                    char buffer[128];
                    sprintf (buffer,"[%7.2f(%s),%7.2f(%s)]",xValue,m_xTickUnit,yValue,m_yTickUnit);
                    printedIt = false;

                    //fl_draw(buffer, mouseX +2, mouseY - 6); // text
                    if ((mouseX + 150) > w())
                    {
                        fl_draw(buffer, mouseX -195, mouseY - 25 , 200, 25, FL_ALIGN_LEFT);
                    }
                    else
                    {
                        fl_draw(buffer, mouseX +5, mouseY - 25 , 200, 25, FL_ALIGN_RIGHT);
                    }

                }
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::setup()
{

    resizable(this);
    begin ();

    {                                             //place the widgets
        Fl_Widget* w;
        int xbuffer=5,ybuffer=5;
        int xsize[5];
        int ysize[5];
        int xGroupSize[5];
        int yGroupSize[5];
        int x[10];
        int y[20];

        // get some info from the container group
        m_winW = this->w();
        m_winH = this->h();
        m_offX = 0;
        m_offY = 0;

        xsize[0] = 150;                           // generic button size
        xsize[2] = 50;                            // y-axis width
                                                  // what is left is for the plot
        xsize[1] = m_winW - xbuffer - (xbuffer + xsize[0]) - (xbuffer + xsize[2]);
        xsize[3] = 30;                            // x-axis width
        xsize[4] = (xsize[0] - xbuffer)/2;

        ysize[0] = 25;                            // generic button height
        ysize[2] = 30;                            // x-axis height
                                                  // plot height
        ysize[1] = m_winH - ybuffer - (ybuffer + ysize[2]) - ysize[0];

        x[0] = m_offX + xbuffer;                  // button space on left
        x[1] = x[0] + xsize[0] + xbuffer;         // start of actual plot
        x[2] = x[1] + xsize[1];
        x[3] = x[0] + 2*xbuffer;                  // the x axis radial
        x[4] = x[3] + xsize[3] + 2*xbuffer;       // the y axis radial
        x[5] = x[0] + xsize[4] + xbuffer;         // delta buttons

        y[0] = m_offY + ybuffer;
        y[1] = 0;                                 //empty
        y[2] = y[0] + (ysize[0] + ybuffer);       // auto
        y[3] = y[2] + (ysize[0]);                 // maximum
        y[4] = y[3] + (ysize[0]);                 // manual
        y[14] = y[4] + (ysize[0]);                // scroll
        y[5] = y[14] + (ysize[0]) + 4*ybuffer;    // min
        y[6] = y[5] + (ysize[0]) + 4*ybuffer;     // max
        y[7] = y[6] + (ysize[0]) + 4*ybuffer;     // range
        y[8] = y[7] + ysize[0] + ybuffer;         // scroll
        y[9] = y[8] + ysize[0] + ybuffer;         // log & linear
        y[12] = y[9] + ysize[0] + 2*ybuffer;      // cross hairs
        y[13] = y[12] + ysize[0] + ybuffer;       // single or point

        y[10] = m_offY + ysize[0];                // top of the plot and top of the y axis
        y[11] = y[10] + ysize[1];                 // placement of the xaxis

        xGroupSize[0] = x[4] + xsize[3] - x[3];
        yGroupSize[0] = ysize[0];
        scaleRadioGroup = new Fl_Group (x[3],y[0],xGroupSize[0],yGroupSize[0]);
        scaleRadioGroup->begin();
        {
            w = xScaleButton = new Fl_Round_Button (x[3],y[0],xsize[3],ysize[0],"X");
            w->type(FL_RADIO_BUTTON);
            w->callback((Fl_Callback*) handleEvent, (void*) this);
            w = yScaleButton = new Fl_Round_Button (x[4],y[0],xsize[3],ysize[0],"Y");
            w->type(FL_RADIO_BUTTON);
            w->callback((Fl_Callback*) handleEvent, (void*) this);
        }
        scaleRadioGroup->end();

        xRangeGroup = new Fl_Group (x[0],y[0],xsize[0],m_winH);
        xRangeGroup->begin();
        {
            w = xAutoButton = new Fl_Round_Button   (x[0], y[2], ysize[0], ysize[0], "Auto");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xMaxButton = new Fl_Round_Button    (x[0], y[3], ysize[0], ysize[0], "Maximum");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xManualButton = new Fl_Round_Button (x[0], y[4], ysize[0], ysize[0], "Manual");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xScrollButton = new Fl_Round_Button (x[0], y[14], ysize[0], ysize[0], "Scroll");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xminInput = new Fl_Float_Input      (x[0], y[5], xsize[0], ysize[0], "X Minimum");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xmaxInput = new Fl_Float_Input      (x[0], y[6], xsize[0], ysize[0], "X Maximum");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = xrangeInput = new Fl_Float_Input    (x[0], y[7], xsize[0], ysize[0], "X Range");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            xLinLogGroup = new Fl_Group (x[0],y[8],xsize[0],m_winH);
            xLinLogGroup->begin();
            {
                w = xLinButton = new Fl_Light_Button    (x[0], y[8], xsize[0], ysize[0], "Linear");
                w->type(FL_RADIO_BUTTON);
                w->callback((Fl_Callback*) handleEvent, (void*) this);

                w = xLogButton = new Fl_Light_Button    (x[0], y[9], xsize[0], ysize[0], "Log10");
                w->type(FL_RADIO_BUTTON);
                w->callback((Fl_Callback*) handleEvent, (void*) this);

            }
            xLinLogGroup->end();
        }
        xRangeGroup->end();

        yRangeGroup = new Fl_Group (x[0],y[0],xsize[0],m_winH);
        yRangeGroup->begin();
        {
            w = yAutoButton = new Fl_Round_Button   (x[0], y[2], ysize[0], ysize[0], "Auto");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = yMaxButton = new Fl_Round_Button    (x[0], y[3], ysize[0], ysize[0], "Maximum");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = yManualButton = new Fl_Round_Button (x[0], y[4], ysize[0], ysize[0], "Manual");
            w->type(FL_RADIO_BUTTON);
            w->align(FL_ALIGN_RIGHT);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = yminInput = new Fl_Float_Input      (x[0], y[5], xsize[0], ysize[0], "Y Minimum");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = ymaxInput = new Fl_Float_Input      (x[0], y[6], xsize[0], ysize[0], "Y Maximum");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            w = yrangeInput = new Fl_Float_Input    (x[0], y[7], xsize[0], ysize[0], "Y Range");
            w->align(FL_ALIGN_TOP);
            w->callback((Fl_Callback*) handleEvent, (void*) this);

            yLinLogGroup = new Fl_Group (x[0],y[8],xsize[0],m_winH);
            yLinLogGroup->begin();
            {
                w = yLinButton = new Fl_Light_Button    (x[0], y[9], xsize[4], ysize[0], "Linear");
                w->type(FL_RADIO_BUTTON);
                w->callback((Fl_Callback*) handleEvent, (void*) this);

                w = yLogButton = new Fl_Light_Button    (x[5], y[9], xsize[4], ysize[0], "Log10");
                w->type(FL_RADIO_BUTTON);
                w->callback((Fl_Callback*) handleEvent, (void*) this);
            }
            yLinLogGroup->end();

        }
        yRangeGroup->end();

        // cross hair stuff
        w = crossHairButton = new Fl_Light_Button (x[0],y[12],xsize[0],ysize[0],"Crosshairs");
        w->callback((Fl_Callback*) handleEvent, (void*) this);

        crossHairGroup = new Fl_Group (x[0],y[13],xsize[0],m_winH);
        crossHairGroup->begin();
        {
            w = singleCrossHairButton = new Fl_Light_Button (x[0],y[13],xsize[4],ysize[0],"Single");
            w->callback((Fl_Callback*) handleEvent, (void*) this);
            w->type(FL_RADIO_BUTTON);

            w = deltaCrossHairButton = new Fl_Light_Button (x[5],y[13],xsize[4],ysize[0],"Delta");
            w->callback((Fl_Callback*) handleEvent, (void*) this);
            w->type(FL_RADIO_BUTTON);
        }

        crossHairGroup->end();

        // plot area
        // keep the data for later use in the cross hairs stuff!!!
        canvasTop = y[10];
        canvasBottom = y[10]+ysize[1];
        canvasLeft = x[1];
        canvasRight = x[1]+xsize[1];

        w = canvas = new Ca_Canvas (x[1],y[10],xsize[1],ysize[1],"");
        canvas->callback((Fl_Callback*) handleEvent, (void*) this);
        canvas->box(FL_DOWN_BOX);
        canvas->color(7);
        canvas->align(FL_ALIGN_TOP);
        canvas->border(15);

        // x axis
        w = x_axis = new Ca_X_Axis (x[1],y[11],xsize[1]+xsize[2],ysize[2]);
        x_axis->callback((Fl_Callback*) handleEvent, (void*) this);
        x_axis->align(FL_ALIGN_BOTTOM);
        x_axis->scale(CA_LIN);
        x_axis->minimum(1);
        x_axis->maximum(1024);
        x_axis->label_format("%g");
        x_axis->minor_grid_color(fl_gray_ramp(20));
        x_axis->major_grid_color(fl_gray_ramp(15));
        x_axis->label_grid_color(fl_gray_ramp(10));
        x_axis->grid_visible(CA_MINOR_GRID|CA_MAJOR_GRID|CA_LABEL_GRID);
        x_axis->major_step(10);
        x_axis->label_step(10);
        x_axis->axis_color(FL_BLACK);
        x_axis->axis_align(CA_BOTTOM|CA_LINE);

        // y axis
        w =y_axis = new Ca_Y_Axis (x[2],y[10],xsize[2],ysize[1]);
        y_axis->callback((Fl_Callback*) handleEvent, (void*) this);
        //y_axis->box(FL_DOWN_BOX);
        y_axis->scale(CA_LIN);
        y_axis->align(FL_ALIGN_TOP);
        y_axis->axis_align(CA_RIGHT);
        y_axis->grid_visible(CA_MINOR_TICK|CA_MAJOR_TICK|CA_LABEL_GRID|CA_ALWAYS_VISIBLE);
        //y_axis->minor_grid_style(FL_DOT);
        y_axis->minor_grid_color(FL_RED);
        y_axis->major_grid_color(FL_RED);
        y_axis->label_grid_color(FL_RED);
        y_axis->minimum(0.0);                     //setting beggining range
        y_axis->maximum(1024);
    }

    end();

}


/////////////////////////////////////////////////////////////////////////////
int plotWindow::handle(int event)
{

    switch (event)
    {
        case FL_MOVE:
        {
            if (crossHairsFlag)
            {
                mouseX = Fl::event_x() ;
                mouseY = Fl::event_y() ;

                redraw_overlay();
                return 1;
            }
        }

        case FL_RELEASE:
        {

            if (crossHairsFlag)
            {
                if ((mouseY > canvasTop) & (mouseY < canvasBottom))
                {
                    if ((mouseX > canvasLeft) & (mouseX < canvasRight))
                    {
                        if (!hold1Flag)
                        {
                            hold1Flag = true;
                            xhold1 = Fl::event_x();
                            yhold1 = Fl::event_y();
                        }
                        else
                        {
                            if (deltaModeFlag)
                            {
                                if (!hold2Flag)
                                {
                                    hold2Flag = true;
                                    xhold2 = Fl::event_x();
                                    yhold2 = Fl::event_y();
                                }
                                else
                                {
                                    hold1Flag = false;
                                    hold2Flag = false;
                                }
                            }
                            else
                            {
                                hold1Flag = false;
                                hold2Flag = false;
                            }                     // end of delta mode
                        }                         // end of hold1 flag
                    }                             // end of x position
                }                                 // end of y position
            }                                     // end of if crossHairsFlag
        }
        default:
            return Fl_Overlay_Window::handle(event);
    }
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::handleEvent(Fl_Widget *w,void *v)
{
    ((plotWindow *)v)->handleEvent(w);
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::handleEvent(Fl_Widget *w)
{

    // scale stuff
    if (w==xScaleButton)
    {
        m_xAxisActive = true;
        userRefresh();
        return;
    }

    if (w==xAutoButton)
    {
        m_xAuto = true;
        m_xMax = false;
        m_xManual = false;
        m_xScroll = false;
        userRefresh();
        return;
    }

    if (w==xMaxButton)
    {
        m_xAuto = false;
        m_xMax = true;
        m_xManual = false;
        m_xScroll = false;
        resetPlotXMax();
        userRefresh();
        return;
    }

    if (w==xManualButton)
    {
        m_xAuto = false;
        m_xMax = false;
        m_xManual = true;
        m_xScroll = false;
        userRefresh();
        return;
    }

    if (w==xScrollButton)
    {
        m_xAuto = false;
        m_xMax = false;
        m_xManual = false;
        m_xScroll = true;
        userRefresh();
        return;
    }

    if (w==xminInput)
    {
        float f;
        sscanf (xminInput->value(),"%f",&f);
        printf ("New plot xmin: %f\n",f);
        m_xminWin=f;
        m_xrangeWin = m_xmaxWin - m_xminWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==xmaxInput)
    {
        float f;
        sscanf (xmaxInput->value(),"%f",&f);
        printf ("New plot xmax: %f\n",f);
        m_xmaxWin=f;
        m_xrangeWin=m_xmaxWin - m_xminWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==xrangeInput)
    {
        float f;
        sscanf (xrangeInput->value(),"%f",&f);
        printf ("New plot xrange: %f\n",f);
        m_xrangeWin=f;
        m_xminWin=m_xmaxWin - m_xrangeWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==xLinButton)
    {
        m_xLog = false;
        x_axis->scale(CA_LIN);
        userRefresh();
        return;
    }

    if (w==xLogButton)
    {
        m_xLog = true;
        x_axis->scale(CA_LOG);
        userRefresh();
        return;
    }

    // y stuff
    if (w==yScaleButton)
    {
        m_xAxisActive = false;
        userRefresh();
        return;
    }

    if (w==yAutoButton)
    {
        m_yAuto = true;
        m_yMax = false;
        m_yManual = false;
        userRefresh();
        return;
    }

    if (w==yMaxButton)
    {
        m_yAuto = false;
        m_yMax = true;
        m_yManual = false;
        resetPlotYMax();
        userRefresh();
        return;
    }

    if (w==yManualButton)
    {
        m_yAuto = false;
        m_yMax = false;
        m_yManual = true;
        userRefresh();
        return;
    }

    if (w==yminInput)
    {
        float f;
        sscanf (yminInput->value(),"%f",&f);
        printf ("New plot ymin: %f\n",f);
        m_yminWin=f;
        m_yrangeWin = m_ymaxWin - m_yminWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==ymaxInput)
    {
        float f;
        sscanf (ymaxInput->value(),"%f",&f);
        printf ("New plot ymax: %f\n",f);
        m_ymaxWin=f;
        m_yrangeWin=m_ymaxWin - m_yminWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==yrangeInput)
    {
        float f;
        sscanf (yrangeInput->value(),"%f",&f);
        printf ("New plot yrange: %f\n",f);
        m_yrangeWin=f;
        m_yminWin=m_ymaxWin - m_yrangeWin;
        updatePlotRange();
        userRefresh();
        return;
    }

    if (w==yLinButton)
    {
        m_yLog = false;
        y_axis->scale(CA_LIN);
        userRefresh();
        return;
    }

    if (w==yLogButton)
    {
        m_yLog = true;
        y_axis->scale(CA_LOG);
        userRefresh();
        return;
    }

    if (w==crossHairButton)
    {
        crossHairsFlag = crossHairButton->value();
        hold1Flag = false;
        hold2Flag = false;
        userRefresh();
        return;
    }

    if (w==singleCrossHairButton)
    {
        deltaModeFlag = false;
        hold1Flag = false;
        hold2Flag = false;
        userRefresh ();
        return;
    }

    if (w==deltaCrossHairButton)
    {
        deltaModeFlag = true;
        hold1Flag = false;
        hold2Flag = false;
        userRefresh ();
        return;
    }

    std::cout << "Plot window got an event it couldn't handle" << std::endl;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::userRefresh()
{
    char output [128];

    if (crossHairsFlag)
    {
        crossHairButton->value(true);
        crossHairGroup->activate();
    }
    else
    {
        crossHairButton->value(false);
        crossHairGroup->deactivate();
    }

    if (deltaModeFlag)
    {
        deltaCrossHairButton->value(true);
        singleCrossHairButton->value(false);
    }
    else
    {
        deltaCrossHairButton->value(false);
        singleCrossHairButton->value(true);
    }

    if (m_xAxisActive)
    {
        yRangeGroup->hide();
        yScaleButton->value(false);
        xRangeGroup->show();
        xScaleButton->value(true);

        if (m_xAuto)
        {
            xAutoButton->value(true);
            xMaxButton->value(false);
            xManualButton->value(false);
            xScrollButton->value(false);
            xmaxInput->deactivate();
            xminInput->deactivate();
            xrangeInput->deactivate();
        }

        if (m_xMax)
        {
            xAutoButton->value(false);
            xMaxButton->value(true);
            xManualButton->value(false);
            xScrollButton->value(false);
            xmaxInput->deactivate();
            xminInput->deactivate();
            xrangeInput->deactivate();
        }

        if (m_xManual)
        {
            xAutoButton->value(false);
            xMaxButton->value(false);
            xManualButton->value(true);
            xScrollButton->value(false);
            xmaxInput->activate();
            xminInput->activate();
            xrangeInput->activate();
        }

        if (m_xScroll)
        {
            xAutoButton->value(false);
            xMaxButton->value(false);
            xManualButton->value(false);
            xScrollButton->value(true);
            xmaxInput->deactivate();
            xminInput->deactivate();
            xrangeInput->activate();
        }

        if (m_xLog)
        {
            xLinButton->value(false);
            xLogButton->value(true);
        }
        else
        {
            xLinButton->value(true);
            xLogButton->value(false);
        }

        // update the screen
        sprintf (output,"%.3f",m_xrangeWin);
        xrangeInput->value(output);

        sprintf (output,"%.3f",m_xminWin);
        xminInput->value(output);

        sprintf (output,"%.3f",m_xmaxWin);
        xmaxInput->value(output);

    }
    else
    {

        xRangeGroup->hide();
        xScaleButton->value(false);
        yRangeGroup->show();
        yScaleButton->value(true);

        if (m_yAuto)
        {
            yAutoButton->value(true);
            yMaxButton->value(false);
            yManualButton->value(false);
            ymaxInput->deactivate();
            yminInput->deactivate();
            yrangeInput->deactivate();
        }

        if (m_yMax)
        {
            yAutoButton->value(false);
            yMaxButton->value(true);
            yManualButton->value(false);
            ymaxInput->deactivate();
            yminInput->deactivate();
            yrangeInput->deactivate();
        }

        if (m_yManual)
        {
            yAutoButton->value(false);
            yMaxButton->value(false);
            yManualButton->value(true);
            ymaxInput->activate();
            yminInput->activate();
            yrangeInput->activate();
        }

        if (m_yLog)
        {
            yLinButton->value(false);
            yLogButton->value(true);
        }
        else
        {
            yLinButton->value(true);
            yLogButton->value(false);
        }

        sprintf (output,"%.3f",m_yrangeWin);
        yrangeInput->value(output);

        sprintf (output,"%.3f",m_yminWin);
        yminInput->value(output);

        sprintf (output,"%.3f",m_ymaxWin);
        ymaxInput->value(output);

    }

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::loopRefresh()
{
    char output [128];

    // clear the canvas
    canvas->clear();

    // test for no data
    if (traceVector.size() == 0)
    {
        return;
    }

    // set the windowing limits using the data that you have
    updatePlotRange ();

    // paint the traces to screen
    updateTraces();

    // if in auto or max mode, update the min & max & range
    if (m_xAxisActive)
    {
        if (!m_xManual)
        {

            sprintf (output,"%.3f",m_xminWin);
            xminInput->value(output);

            sprintf (output,"%.3f",m_xmaxWin);
            xmaxInput->value(output);

            if (!m_xScroll)
            {
                sprintf (output,"%.3f",m_xrangeWin);
                xrangeInput->value(output);
            }
        }
    }
    else
    {
        if (m_yAuto | m_yMax)
        {
            sprintf (output,"%.3f",m_yrangeWin);
            yrangeInput->value(output);

            sprintf (output,"%.3f",m_yminWin);
            yminInput->value(output);

            sprintf (output,"%.3f",m_ymaxWin);
            ymaxInput->value(output);
        }
    }

    // now clear out the trace data (this does not clear the canvas!!!)
    clearData();
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::clearData ()
{
    while (traceVector.size() > 0)
    {
        // get the last trace
        struct traceStructure *trace = traceVector.back();

        // delete the memory
        delete (trace->xData);
        delete (trace->yData);
        delete (trace);

        // pop it off the vector
        traceVector.pop_back();
    }
    traceVector.clear();
}


/////////////////////////////////////////////////////////////////////////////
int plotWindow::ingestAndKeepData (double* xx, double* yy, int n)
{
    // define a new trace
    struct traceStructure *trace = new struct traceStructure;

    if (n<=0)
    {
        printf ("i can't handle a zero length array\n");
        return -1;
    }

    // set some default
    trace->vertLineFlag = false;
    trace->horLineFlag  = false;
    trace->color        = FL_BLACK;

    // copy pointers into the trace
    trace->xData = xx;
    trace->yData = yy;

    // copy over the size
    trace->numSamples = n;

    //for (unsigned int i = 0;i<trace->numSamples;i++) {
    //printf ("%d [%f,%f] -> [%f,%f]\n",i,xx[i],yy[i],trace->xData[i],trace->yData[i]);
    //}

    // do some stat analysis on the trace
    analyzeTrace (trace);

    // add the trace to the vector of traces
    traceVector.push_back(trace);

    // returns the size of the vector, which, in effect, can be used by the calling function to refer to this trace
    return (traceVector.size()-1);
}


/////////////////////////////////////////////////////////////////////////////
int plotWindow::ingestAndCopyData (double* xx, double* yy, int n)
{
    if (n<=0)
    {
        printf ("i can't handle a zero length array\n");
        return -1;
    }

    // get some memory on the heap for the data
    double *x = new double[n];
    double *y = new double[n];

    // copy the data locally
    memcpy (x,xx,n*sizeof(double));
    memcpy (y,yy,n*sizeof(double));

    return ingestAndKeepData(x,y,n);
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::analyzeTrace (struct traceStructure *trace)
{
    // declare some variables that will hold stat info about the data
    double xMax = -99999999.;
    double yMax = -99999999.;
    double xMin = +99999999.;
    double yMin = +99999999.;

    // set the flags
    trace->xSafeForLogFlag = true;
    trace->ySafeForLogFlag = true;
    trace->xClipFlag = false;
    trace->yClipFlag = false;

    // some quick analysis
    for (unsigned int i = 0;i<trace->numSamples;i++)
    {
        // this is a good time to apply the scaling factor
        trace->xData[i] *= m_xScale;
        trace->yData[i] *= m_yScale;

        //		printf ("%d [%f,%f]\n",i,trace->xData[i],trace->yData[i]);

        if (trace->xData[i] > xMax) xMax = trace->xData[i];
        if (trace->xData[i] < xMin) xMin = trace->xData[i];
        if (trace->yData[i] > yMax) yMax = trace->yData[i];
        if (trace->yData[i] < yMin) yMin = trace->yData[i];

        // check x log safety
        if (trace->xData[i] <=0.)
        {
            if (m_xLog)
            {
                // if you are already in log mode, clip the point
                trace->xData[i] = 1;              // make this point = 0 on the log scale
                trace->xClipFlag = true;
            }
            else
            {
                // if not, make this trace as not safe for log
                trace->xSafeForLogFlag = false;
            }
        }

        // check y log safety
        if (trace->yData[i] <=0.)
        {
            if (m_yLog)
            {
                // if you are already in log mode, clip the point
                trace->yData[i] = 1;              // make this point = 0 on the log scale
                trace->yClipFlag = true;
            }
            else
            {
                // if not, make this trace as not safe for log
                trace->ySafeForLogFlag = false;
            }
        }

    }

    // put that data into the trace
    trace->xMin = xMin;
    trace->xMax = xMax;
    trace->yMin = yMin;
    trace->yMax = yMax;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::addHorLine(unsigned int traceNumber, double y)
{

    if ((traceNumber+1) > traceVector.size())
    {
        printf ("Invalid Trace ID Number!!\n");
        return;
    }

    //printf ("got %f * %f\n",y,m_yScale);

    traceVector[traceNumber]->horLine = y * m_yScale;
    traceVector[traceNumber]->horLineFlag = true;

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::addVertLine(unsigned int traceNumber, double x)
{

    if ((traceNumber+1) > traceVector.size())
    {
        printf ("Invalid Trace ID Number!!\n");
        return;
    }

    traceVector[traceNumber]->vertLine = x / m_xScale;
    traceVector[traceNumber]->vertLineFlag = true;

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::color(unsigned int traceNumber, Fl_Color color)
{

    if ((traceNumber+1) > traceVector.size())
    {
        printf ("Invalid Trace ID Number!!\n");
        return;
    }

    traceVector[traceNumber]->color = color;

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::updatePlotRange()
{

    if (m_xAuto)
    {
        // if the size is 0, just return
        if (traceVector.size() == 0) return;

        // get an iterator for the vector
        std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

        // get an instance from the iterator
        struct traceStructure *trace = (*traceVectorIterator);

        // use the first traces' values
        m_xminWin = trace->xMin;
        m_xmaxWin = trace->xMax;

        // iterate to the next trace
        traceVectorIterator++;

        // loop through all traces and see if there is any bigger/smaller
        for (unsigned int i=1; i<traceVector.size(); i++)
        {
            trace = (*traceVectorIterator);
            if (trace->xMin < m_xminWin) m_xminWin = trace->xMin;
            if (trace->xMax > m_xmaxWin) m_xmaxWin = trace->xMax;
            traceVectorIterator++;
        }

        // set the range
        m_xrangeWin = m_xmaxWin - m_xminWin;
    }

    if (m_xMax)
    {
        // if the size is 0, just return
        if (traceVector.size() == 0) return;

        // get an iterator for the vector
        std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

        // get an instance from the iterator
        struct traceStructure *trace = (*traceVectorIterator);

        // loop through all traces and see if there is any bigger/smaller than what you have
        for (unsigned int i=0; i<traceVector.size(); i++)
        {
            trace = (*traceVectorIterator);
            if (trace->xMin < m_xminWin) m_xminWin = trace->xMin;
            if (trace->xMax > m_xmaxWin) m_xmaxWin = trace->xMax;
            traceVectorIterator++;
        }

        // set the range
        m_xrangeWin = m_xmaxWin - m_xminWin;
    }

    if (m_xScroll)
    {
        // if the size is 0, just return
        if (traceVector.size() == 0) return;

        // get an iterator for the vector
        std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

        // get an instance from the iterator
        struct traceStructure *trace = (*traceVectorIterator);

        // loop through all traces and see if there is any bigger/smaller than what you have
        for (unsigned int i=0; i<traceVector.size(); i++, traceVectorIterator++)
        {
            trace = (*traceVectorIterator);
            if (trace->xMax > m_xmaxWin) m_xmaxWin = trace->xMax;
        }

        // set the min using the range
        m_xminWin = m_xmaxWin - m_xrangeWin;

    }

    if (m_yAuto)
    {
        // if the size is 0, just return
        if (traceVector.size() == 0) return;

        // get an iterator for the vector
        std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

        // get an instance from the iterator
        struct traceStructure *trace = (*traceVectorIterator);

        // use the first traces' values
        m_yminWin = trace->yMin;
        m_ymaxWin = trace->yMax;

        // iterate to the next trace
        traceVectorIterator++;

        // loop through all traces and see if there is any bigger/smaller
        for (unsigned int i=1; i<traceVector.size(); i++)
        {
            trace = (*traceVectorIterator);
            if (trace->yMin < m_yminWin)
            {
                m_yminWin = trace->yMin;
            }
            if (trace->yMax > m_ymaxWin)
            {
                m_ymaxWin = trace->yMax;
            }
            traceVectorIterator++;
        }

        m_yrangeWin = m_ymaxWin - m_yminWin;

    }

    if (m_yMax)
    {
        // if the size is 0, just return
        if (traceVector.size() == 0) return;

        // get an iterator for the vector
        std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

        // get an instance from the iterator
        struct traceStructure *trace = (*traceVectorIterator);

        // loop through all traces and see if there is any bigger/smaller than what you have
        for (unsigned int i=0; i<traceVector.size(); i++)
        {
            trace = (*traceVectorIterator);
            if (trace->yMin < m_yminWin)
            {
                m_yminWin = trace->yMin;
            }
            if (trace->yMax > m_ymaxWin)
            {
                m_ymaxWin = trace->yMax;
                m_yrangeWin = trace->yMax - trace->yMin;
            }
            traceVectorIterator++;
        }

        m_yrangeWin = m_ymaxWin - m_yminWin;

    }

    if (m_yManual)
    {
        // keep the same values
    }

    // make sure these values are okay for log
    if (m_xLog)
    {
        if (m_xminWin <= 0)
        {
            m_xminWin = 1;                        // clip it so log 10 is 0
        }
        if (m_xmaxWin <= 0)
        {
            m_xmaxWin = 1;                        // clip it so log 10 is 0
        }
    }

    if (m_yLog)
    {
        if (m_yminWin <= 0)
        {
            m_yminWin = 1;                        // clip it so log 10 is 0
        }
        if (m_ymaxWin <= 0)
        {
            m_ymaxWin = 1;                        // clip it so log 10 is 0
        }
    }

    // this applies to ALL ranging schemes
    if (m_xminWin == m_xmaxWin)
    {
        m_xmaxWin += .001;
        m_xminWin -= .001;
        // printf ("x range is 0.  editing\n");
    }

    if (m_yminWin == m_ymaxWin)
    {
        m_ymaxWin += .001;
        m_yminWin -= .001;
        // printf ("y range is 0.  editing\n");
    }

    // refresh the plot boundaries
    x_axis->minimum(m_xminWin);
    x_axis->maximum(m_xmaxWin);
    y_axis->minimum(m_yminWin);
    y_axis->maximum(m_ymaxWin);

    // printf ("x min %lf \n",m_xminWin);
    // printf ("x max %lf \n",m_xmaxWin);
    // printf ("y min %lf \n",m_yminWin);
    // printf ("y max %lf \n",m_ymaxWin);
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::updateTraces()
{

    // the vector iterator
    std::vector <struct traceStructure *>::iterator traceVectorIterator = traceVector.begin();

    // loop through all traces
    for (unsigned int i=0; i<traceVector.size() ; i++)
    {

        // get a pointer to the current trace in the vector
        struct traceStructure *trace = (*traceVectorIterator);

        if (m_xLog & !trace->xSafeForLogFlag)
        {
            printf ("This trace isn't safe to plot in the X axis\n");
            continue;
        }

        if (m_yLog & !trace->ySafeForLogFlag)
        {
            printf ("This trace isn't safe to plot in the Y axis\n");
            continue;
        }

        // get pointers to the data
        double       *x = trace->xData;
        double       *y = trace->yData;
        unsigned int  n = trace->numSamples;
        Fl_Color      color = trace->color;

        // make this canvas the current canvas
        canvas->current(canvas);

        // now loop through all points and add them to the plot
        Ca_LinePoint *old=0;                      // a dummy point
        double xx,yy;

        int lineWidth            = 1;
        int pointSize            = DEFAULT_POINT_SIZE;
        Ca_PointStyle pointStyle = CA_SIMPLE;
        Fl_Color borderColor     = FL_BLACK;
        int borderWidth          = 1;

        if (m_connectedLineFlag)
        {
            for (unsigned int j=0; j<n ; j++)
            {
                xx = x[j];
                yy = y[j];
                old = new Ca_LinePoint(old, xx,yy,lineWidth,color,pointStyle,pointSize,borderColor,borderWidth);
            }
        }
        else
        {
            pointSize = 3;
            pointStyle = CA_DOWN_TRIANGLE;
            lineWidth = 0;
            for (unsigned int j=0; j<n; j++)
            {
                xx = x[j];
                yy = y[j];

                // check if we should even both plotting it
                if (m_xScroll)
                {
                    if (xx < m_xminWin) continue;
                }
                new Ca_Point(xx, yy, color, pointStyle, pointSize, borderColor, borderWidth);
            }
        }

        // plot the hor line
        if (trace->horLineFlag)
        {
            //printf ("trying to plot line at %f\n",trace->horLine);
            old = new Ca_LinePoint ( NULL, -1. * fabs(2 * x[0]),trace->horLine,3,color,CA_SIMPLE,DEFAULT_POINT_SIZE,FL_BLACK,1 );
            old = new Ca_LinePoint ( old, fabs(2 * x[n-1]),trace->horLine,3,color,CA_SIMPLE,DEFAULT_POINT_SIZE,FL_BLACK,1 );
        }

        // plot the vert line
        if (trace->vertLineFlag)
        {
            //printf ("trying to plot line at %f\n",trace->vertLine);
            double range = fabs((x[n-1] - x[0])/2.);
            old = new Ca_LinePoint ( NULL, x[0]   - range,trace->vertLine,3,color,CA_SIMPLE,DEFAULT_POINT_SIZE,FL_BLACK,1 );
            old = new Ca_LinePoint ( old,  x[n-1] + range,trace->vertLine,3,color,CA_SIMPLE,DEFAULT_POINT_SIZE,FL_BLACK,1 );
        }

        // iterate the trace iterator
        traceVectorIterator++;
    }

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::resetPlotXMax()
{
    m_xminWin = +999999;
    m_xmaxWin = -999999;
    if (!m_xScroll)
    {
        m_xrangeWin = m_xmaxWin - m_xminWin;
    }
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::resetPlotYMax()
{
    m_yminWin = +999999;
    m_ymaxWin = -999999;
    m_yrangeWin = m_ymaxWin - m_yminWin;
}


/////////////////////////////////////////////////////////////////////////////
// query only
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// set only
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// full encapsulation
/////////////////////////////////////////////////////////////////////////////
void plotWindow::xLog(bool b)
{
    m_xLog = b;
    if (b)
    {
        if (traceVector.size() > 0)
        {
            if (!(traceVector.front())->xSafeForLogFlag)
            {
                printf ("not log x safe!\n");
            }
            else
            {
                printf ("log x safe!\n");
            }
        }

        if (x_axis->minimum() <= 0)
        {
            x_axis->minimum(1.);
            printf ("x axis min is too small.\n");
            fflush(0);
        }
        if (x_axis->maximum() <= 0)
        {
            x_axis->maximum(10.);
            printf ("x axis max is too small.\n");
            fflush(0);
        }

        x_axis->scale(CA_LOG);

    }
    else
    {
        x_axis->scale(CA_LIN);
    }

    userRefresh();                                // make sure to update buttons
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::xLog(void)
{
    return m_xLog;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yLog(bool b)
{
    m_yLog = b;

    if (b)
    {
        if (traceVector.size() > 0)
        {
            if (!(traceVector.front())->xSafeForLogFlag)
            {
                printf ("not log y safe!\n");
            }
            else
            {
                printf ("log y safe!\n");
            }
        }
        if (y_axis->minimum() <= 0)
        {
            y_axis->minimum(1.);
            printf ("y axis min is too small.\n");
            fflush(0);
        }
        if (y_axis->maximum() <= 0)
        {
            y_axis->maximum(10.);
            printf ("y axis max is too small.\n");
            fflush(0);
        }
        y_axis->scale(CA_LOG);
    } else
    y_axis->scale(CA_LIN);

    userRefresh();                                // make sure to update the buttons
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::yLog(void)
{
    return m_yLog;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::xLinLogVisibleFlag(bool b)
{
    m_xLinLogVisibleFlag = b;
    if (m_xLinLogVisibleFlag)
    {
        xLinLogGroup->activate();
    }
    else
    {
        xLinLogGroup->deactivate();
    }
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::xLinLogVisibleFlag(void)
{
    return m_xLinLogVisibleFlag;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yLinLogVisibleFlag(bool b)
{
    m_yLinLogVisibleFlag = b;
    if (m_yLinLogVisibleFlag)
    {
        yLinLogGroup->activate();
    }
    else
    {
        yLinLogGroup->deactivate();
    }
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::yLinLogVisibleFlag(void)
{
    return m_yLinLogVisibleFlag;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::xScale(float f)
{
    if (m_xScale != f)
    {

        m_xScale = f;

        // make sure that if you are in manual mode and you switch scales, you pop back into auto
        if (m_xManual == true)
        {
            m_xManual = false;
            m_xAuto = true;
        }

        //std::cout << "X Scale set to : " << m_xScale << std::endl;

        userRefresh();

    }

}


/////////////////////////////////////////////////////////////////////////////
float plotWindow::xScale(void)
{
    return m_xScale;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yScale(float f)
{
    if (m_yScale != f)
    {

        m_yScale = f;

        // make sure that if you are in manual mode and you switch scales, you pop back into auto
        if (m_yManual == true)
        {
            m_yManual = false;
            m_yAuto = true;
            userRefresh();
        }

        //std::cout << "Y Scale set to : " << m_yScale << std::endl;

    }

}


/////////////////////////////////////////////////////////////////////////////
float plotWindow::yScale(void)
{
    return m_yScale;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::xTickUnit(const char *c)
{
    if (c==NULL)
    {
        std::cout << "You sent a bad tick dummy" << std::endl;
    }
    if (sizeof(c) > 16)
    {
        std::cout << "You sent a unit that was too big" << std::endl;
    }

    sprintf (m_xTickUnit,"%s",c);

    char buf[256];
    sprintf (buf,"X Min (%s)",m_xTickUnit);
    xminInput->copy_label(buf);

    sprintf (buf,"X Max (%s)",m_xTickUnit);
    xmaxInput->copy_label(buf);

    sprintf (buf,"X Range (%s)",m_xTickUnit);
    xrangeInput->copy_label(buf);

    sprintf (buf,"%s (%s) vs %s (%s)",m_yTickLabel,m_yTickUnit,m_xTickLabel,m_xTickUnit);
    canvas->copy_label(buf);

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yTickUnit(const char *c)
{
    if (c==NULL)
    {
        std::cout << "You sent a bad tick dummy" << std::endl;
    }
    if (sizeof(c) > 8)
    {
        std::cout << "You sent a unit that was too big" << std::endl;
    }

    sprintf (m_yTickUnit,"%s",c);

    char buf[256];
    sprintf (buf,"Y Min (%s)",m_yTickUnit);
    yminInput->copy_label(buf);

    sprintf (buf,"Y Max (%s)",m_yTickUnit);
    ymaxInput->copy_label(buf);

    sprintf (buf,"Y Range (%s)",m_yTickUnit);
    yrangeInput->copy_label(buf);

    sprintf (buf,"%s (%s) vs %s (%s)",m_yTickLabel,m_yTickUnit,m_xTickLabel,m_xTickUnit);
    canvas->copy_label(buf);
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::xTickLabel(const char *c)
{
    if (c==NULL)
    {
        std::cout << "You sent a bad tick dummy" << std::endl;
    }
    if (sizeof(c) > 8)
    {
        std::cout << "You sent a label that was too big" << std::endl;
    }

    sprintf (m_xTickLabel,"%s",c);

    char buf[256];
    sprintf (buf,"%s (%s) vs %s (%s)",m_yTickLabel,m_yTickUnit,m_xTickLabel,m_xTickUnit);
    canvas->copy_label(buf);
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yTickLabel(const char *c)
{
    if (c==NULL)
    {
        std::cout << "You sent a bad tick dummy" << std::endl;
    }
    if (sizeof(c) > 8)
    {
        std::cout << "You sent a label that was too big" << std::endl;
    }

    sprintf (m_yTickLabel,"%s",c);

    char buf[256];
    sprintf (buf,"%s (%s) vs %s (%s)",m_yTickLabel,m_yTickUnit,m_xTickLabel,m_xTickUnit);
    canvas->copy_label(buf);

}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::connectedLineFlag(bool b)
{
    m_connectedLineFlag = b;
    if (m_connectedLineFlag)
    {
        //printf ("turning on connected line\n");
    }
    else
    {
        //printf ("turning off connected line\n");
    }
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::connectedLineFlag(void)
{
    return m_connectedLineFlag;
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::xDisplayMethod(int d)
{
    m_xAuto = false;
    m_xMax = false;
    m_xManual = false;
    m_xScroll = false;

    switch (d)
    {
        case 0:
            m_xAuto = true;
            break;
        case 1:
            m_xMax = true;
            break;
        case 2:
            m_xManual = true;
            break;
        case 3:
            m_xScroll = true;
            m_xrangeWin = 100;                    // might have to give this an init value
            break;
        default:
            printf ("bad input for x display\n");
            break;
    }
    userRefresh();
}


/////////////////////////////////////////////////////////////////////////////
void plotWindow::yDisplayMethod(int d)
{
    m_yAuto = false;
    m_yMax = false;
    m_yManual = false;

    switch (d)
    {
        case 0:
            m_yAuto = true;
            break;
        case 1:
            m_yMax = true;
            break;
        case 2:
            m_yManual = true;
            break;
        default:
            printf ("bad input for y display\n");
            break;
    }
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::yManual(void)
{
    return m_yManual;
}


/////////////////////////////////////////////////////////////////////////////
bool plotWindow::xManual(void)
{
    return m_xManual;
}


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
