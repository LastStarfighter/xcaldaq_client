#ifndef __ClientGUI_h
#define __ClientGUI_h

// system includes
#include <iostream>                               // standard c++ in put and output stream functions
#include <string>                                 // string library
#include <pthread.h>                              // posix threading library

// local includes
#include "Client.h"
#include "version.h"

// fltk includes
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Input.H>
#include <Fl/Fl_Output.H>
#include <Fl/Fl_Dial.H>
#include <Fl/Fl_Progress.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Bar.H>

// local fltk includes
#include "SliderInput.h"
#include "plotWindow.h"
#include "PRDisplay.h"
#include "PulseAnalysisDisplay.h"

// local marcos
#define TIME_BETWEEN_EVENT_LOOP .2

class ClientGUI
{

    public:
        // constructor
        ClientGUI ();

        // destructor
        ~ClientGUI ();

        // public functions
        void initGUI ();                          // get the window ready for display
        void startGUI ();                         // display the windows on the screen
        void stopGUI ();                          // remove the windows from the screen

        // the client pointer
        xdaq::Client *client;

    private:

        void build_menu(int menu_width,int menu_height);

        //private functions
        void plotWindow_add ();
        void plotWindow_removeAll ();

        void connect (bool b);                    // gets the GUI ready for a connected client
        void stream  (bool b);                    // gets the GUI ready for a client that is receiving streaming data

                                                  // refresh the screen sometime in the future
        static void refreshTriggerControlUntilInitialized (void *);

        void startLoopRefresh ();                 // starts the recursive loop refresh
        static void loopRefresh (void *);         // the loop refresh
        void stopLoopRefresh ();                  // stops the recursive loop refresh

        // main window functions
        void        mainWindow_setup ();          // actually place the widgets into the window
                                                  // static wrapper to handle events from this window
        static void mainWindow_handleEvent(Fl_Widget *,void *);
                                                  // handle events from the window
        void        mainWindow_handleEvent(Fl_Widget *);
        void        mainWindow_userRefresh ();    // updates data on screen.  usually only executes when user interacts with widgets
        void        mainWindow_loopRefresh ();

        // Trigger Control Window functions
        void        triggerControlWindow_setup       ( void );
        static void triggerControlWindow_handleEvent ( Fl_Widget *,void *);
        void        triggerControlWindow_handleEvent ( Fl_Widget *);
        void        triggerControlWindow_userRefresh ( void );

        // Trigger rate functions
        void        triggerRateWindow_setup       ( void );
        static void triggerRateWindow_handleEvent ( Fl_Widget *, void * );
        void        triggerRateWindow_handleEvent ( Fl_Widget * );
        void        triggerRateWindow_userRefresh ( void );
        void        triggerRateWindow_loopRefresh ( void );

        // LJH Window functions
        void        LJHWindow_setup();
        static void LJHWindow_handleEvent(Fl_Widget *,void *);
        void        LJHWindow_handleEvent(Fl_Widget *);
        void        LJHWindow_userRefresh();

        // stream Channel functions
        void        streamChannelWindow_setup       ( void );
        static void streamChannelWindow_handleEvent ( Fl_Widget *, void * );
        void        streamChannelWindow_handleEvent ( Fl_Widget * );
        void        streamChannelWindow_userRefresh ( void );

        // file writer stream control functions
        void        fileWriterStreamControlWindow_setup       ( void );
        static void fileWriterStreamControlWindow_handleEvent ( Fl_Widget *, void * );
        void        fileWriterStreamControlWindow_handleEvent ( Fl_Widget * );
        void        fileWriterStreamControlWindow_userRefresh ( void );

        // group Trigger functions
        void        groupTriggerWindow_setup   ( void );
        static void groupTriggerWindow_handleEvent  ( Fl_Widget *, void * );
        void        groupTriggerWindow_handleEvent  ( Fl_Widget * );
        void        groupTriggerWindow_userRefresh ( void );

        // group Trigger functions
        void        mixControlWindow_setup   ( void );
        static void mixControlWindow_handleEvent  ( Fl_Widget *, void * );
        void        mixControlWindow_handleEvent  ( Fl_Widget * );
        void        mixControlWindow_userRefresh ( void );
        void        mixControlWindow_loopRefresh ( void );

        /* MAIN WINDOW WIDGETS */

        // main group window
        Fl_Double_Window *mainWindow;

        // server group widgets
        Fl_Group        *serverGroup;
        //	Fl_Choice       *chooseServerMenu;
        //        Fl_Button       *button_trigger_config_file_save;
        //     Fl_Button       *button_trigger_config_file_load;
        Fl_Button       *saveConfigFileButton;
        Fl_Input        *hostInput;
        Fl_Input        *portInput;
        Fl_Light_Button *connectButton;
        Fl_Progress     *bufferLevelIndicator;
        Fl_Progress     *dataRateIndicator;

        // file writer widgets
        Fl_Group        *fileGroup;
        Fl_Choice       *chooseFileWriter;
        Fl_Input        *maxPulsesInput;
        Fl_Output       *numPulsesOutput;
        Fl_Button       *setFileNameButton;
        Fl_Button       *closeFileButton;
        Fl_Button       *fileWriterStreamControlWindowButton;
        Fl_Output       *fileNameOutput;
        Fl_Light_Button *writeDataButton;
        Fl_Progress     *fileCompletionIndicator;

        // we have an unfortunate race condition.  sometimes the
        // button is turned "on" before the callback is executed.  in
        // this case, a simple lock is not enough b/c we can't grab
        // the lock b/f the button turns on!  this extra flag removes
        // the button value from the logic and allows us to maintain a
        // tighter grip on when and if it changes (i.e. inside the
        // thread lock)
        bool writerButtonStatusFlag;
        XCALDAQLock writerButtonStatusLock;

        // command widgets
        Fl_Group         *commandGroup;
        Fl_Return_Button *quitButton;
        Fl_Return_Button *infoButton;
        Fl_Return_Button *hackButton;

        // trigger and stream buttons
        Fl_Group        *streamGroup;
        Fl_Light_Button *streamButton;
        Fl_Light_Button *decimateFlagButton;
        Fl_Choice       *decimateModeSelector;
        Fl_Choice       *decimateChannelSelector;
        Fl_Int_Input    *decimateLevelInput;
        Fl_Float_Input  *sampleRateDisplay;
        Fl_Input        *recordLengthInput;
        Fl_Input        *preTriggerLengthInput;
                                                  // launches the stream channel sub window
        Fl_Button       *showStreamChannelWindowButton;
                                                  // launches the trigger rate sub window
        Fl_Button       *showTriggerRateWindowButton;
                                                  // launches the mix control sub window
        Fl_Button       *showMixControlWindowButton;
                                                  // launches the Pulse Height sub window
        Fl_Button       *showPulseAnalysisWindowButton;
                                                  // launches the group trigger sub window
        Fl_Button       *showGroupTriggerWindowButton;
                                                  // launch the trigger control sub window
        Fl_Button       *showTriggerControlWindowButton;
        Fl_Button       *rewindDataFileButton;

        // mix and decimate buttons
        //Fl_Group        *decGroup;

        // plot windows
        Fl_Group *plotGroup;
        Fl_Button* morePlotsButton;
        Fl_Button* removeAllPlotWindowButton;

        /* TRIGGER RATE WIDGETS */
        Fl_Double_Window *triggerRateWindow;
        Fl_Box **triggerRateIndicatorBox;
        Fl_Float_Input *triggerIntegrationTimeInput;
        Fl_Float_Input *triggerRateIntensityInput;
        float triggerRateIntensity;
        Fl_Return_Button *triggerRateOkayButton;

        /* TRIGGER CONTROL WIDGETS */
        Fl_Double_Window *triggerControlWindow;
        Fl_Choice* channelChoice;
        Fl_Input* affectedChannelsInput;
        std::vector<unsigned int> affectedChannels;
        Fl_Group *triggerControlGroup;
        Fl_Light_Button  *autoTriggerFlagButton;
        SliderInput      *autoTriggerThresholdSlider;
        Fl_Light_Button  *levelTriggerFlagButton;
        FloatSliderInput *levelTriggerThresholdSlider;
        Fl_Light_Button  *levelInverseFlagButton;
        Fl_Light_Button  *edgeTriggerFlagButton;
        FloatSliderInput *edgeTriggerThresholdSlider;
        Fl_Light_Button  *edgeInverseFlagButton;
        Fl_Light_Button  *noiseTriggerFlagButton;
        FloatSliderInput *noiseTriggerThresholdSlider;
        Fl_Light_Button  *noiseInverseFlagButton;
        Fl_Light_Button  *computeFFTButton;
        Fl_Int_Input     *noiseRecordsPerFFTInput;
        Fl_Int_Input     *fftDiscriminationInput;
        Fl_Light_Button  *fftWindowingFlagButton;
        Fl_Return_Button *triggerControlOkayButton;

        /* GROUP TRIGGER WIDGETS */
        Fl_Double_Window *groupTriggerWindow;
        Fl_Check_Button *coupleOddToEvenButton;
        Fl_Check_Button *coupleEvenToOddButton;
        Fl_Check_Button **receiverButton;
        Fl_Check_Button **sourceButton;
        Fl_Return_Button *triggerOkayButton;

        /* STREAM CHANNEL WIDGETS */
        Fl_Double_Window* streamChannelWindow;
        Fl_Light_Button** streamChannelButton;
        Fl_Light_Button* allOddButton;
        Fl_Light_Button* allEvenButton;
        Fl_Return_Button* streamChannelOkayButton;

        /* FILE WRITER STREAM CONTROL */
        Fl_Double_Window *fileWriterStreamControlWindow;
        Fl_Light_Button  *writeOddButton;
        Fl_Light_Button  *writeEvenButton;
        Fl_Light_Button  *writeAutoToFileFlagButton;
        Fl_Light_Button  *writeLevelToFileFlagButton;
        Fl_Light_Button  *writeEdgeToFileFlagButton;
        Fl_Light_Button  *writeNoiseToFileFlagButton;

        Fl_Return_Button *fileWriterStreamControlDoneButton;

        /* LJH WIDGETS WIDGETS */
        Fl_Double_Window *ljhWindow;
        Fl_Output* keyOutput[40];
        Fl_Input* valueInput[40];
        Fl_Return_Button* ljhOkayButton;
        Fl_Return_Button* ljhSaveButton;

        /* MIX CONTROL WIDGETS */
        Fl_Double_Window *mixControlWindow;
        Fl_Choice        *mixChannelSelector;
        Fl_Light_Button  *mixFlagButton;
        Fl_Light_Button  *mixInversionFlagButton;
        Fl_Float_Input   *mixLevelInput;
        Fl_Int_Input     *mixRecordsInput;
        Fl_Button        *optimalMix_calculateMedianMixButton;
        Fl_Button        *mixControlWarningInput;
        Fl_Return_Button *mixControlOkayButton;
        Fl_Double_Window *mixControlProgressWindow;
        Fl_Progress      *mixControlProgressBar;
        Fl_Choice        *mixRatioMethodSelector;
        Fl_Button        *mixCouplePairsFlagButton;

        // COMMON VARIABLES

        // the Pulse Record display windows
        std::deque<PRDisplay *> plotterWindows;
        XCALDAQLock plotterWindowsLock;

        // the pulse height vs time display windows
        PulseAnalysisDisplay *pulseAnaylsisWindow;

        // a stupid hack b/c FLTK sucks
        bool stupidFLTKTimeoutHack;

        // counts how many loops have been called, used to meter the various event loops
        unsigned long long int loopCounter;

    public:
        void trigger_state_load_dialog(void);

        void trigger_state_save_dialog(void);

        void mixing_state_load_dialog(void);

        void mixing_state_save_dialog(void);

};

// callback functions for the menus

void cb_quit( Fl_Widget *, void * );
void cb_trigger_state_load_dialog( Fl_Widget *, void * );
void cb_trigger_state_save_dialog( Fl_Widget *, void * );

void cb_trigger_state_load_default ( Fl_Widget * w, void * v);
void cb_trigger_state_save_default ( Fl_Widget * w, void * v);

void cb_trigger_state_load_tconf1 ( Fl_Widget * w, void * v);
void cb_trigger_state_save_tconf1 ( Fl_Widget * w, void * v);
void cb_trigger_state_load_tconf2 ( Fl_Widget * w, void * v);
void cb_trigger_state_save_tconf2 ( Fl_Widget * w, void * v);
void cb_trigger_state_load_tconf3 ( Fl_Widget * w, void * v);
void cb_trigger_state_save_tconf3 ( Fl_Widget * w, void * v);
void cb_trigger_state_load_tconf4 ( Fl_Widget * w, void * v);
void cb_trigger_state_save_tconf4 ( Fl_Widget * w, void * v);

void cb_mixing_state_load_dialog( Fl_Widget *, void * );
void cb_mixing_state_save_dialog( Fl_Widget *, void * );

void cb_mixing_state_load_default ( Fl_Widget * w, void * v);
void cb_mixing_state_save_default ( Fl_Widget * w, void * v);

void cb_mixing_state_load_mconf1 ( Fl_Widget * w, void * v);
void cb_mixing_state_save_mconf1 ( Fl_Widget * w, void * v);
void cb_mixing_state_load_mconf2 ( Fl_Widget * w, void * v);
void cb_mixing_state_save_mconf2 ( Fl_Widget * w, void * v);
void cb_mixing_state_load_mconf3 ( Fl_Widget * w, void * v);
void cb_mixing_state_save_mconf3 ( Fl_Widget * w, void * v);
void cb_mixing_state_load_mconf4 ( Fl_Widget * w, void * v);
void cb_mixing_state_save_mconf4 ( Fl_Widget * w, void * v);
#endif
