#define CVS_VERSION_STRING "$Revision: 2.1 $"
#define VERSION_MAJOR 2
#define VERSION_MINOR 12
#define VERSION_REALLYMINOR 3

/*

 2.12.3 2009-06-06 -jsa Added missing includes for gcc 4.3.3 compliance

 2.12.2 2008-06-05 -jsa
 Seperate mixing configs from trigger configs. Added appropiate menus.

 2.11.1 2008-06-05 -jsa

 Added uploading of mix levels to server when trigger config file is
 loaded. The default is loaded before connection to the server, so the
server should not be changed on client startup. (Mix levels are saved
as part of the trigger config.

2.10.1 2008-04-17  -jsa
Changed trigger state to pass through method calls in StreamChannel,
this bypasses time issue on edge triggers (and give engineering units)

2.10.0 2008-04-17  -jsa
Added menus to support trigger status loads

2.9.9 2008-04-16  -jsa
Added dump of trigger status as additional file
Beautified Code
Added scons file

2.9.8 2008-03-12
documentation

2.9.7 2008-03-07
modified fft class to include 0th elements and fixed it so it returns the entire n/2 elements

2.9.6 2008-03-06
modified fft writer to only do 1 trace and write data as text

2.9.5 2008-03-05
added hanning window button
changed cross hairs to print data out to screen during user click

2.9.4 2008-03-05
added windowing flag to fft

2.9.3 2008-03-05
changed how the fft is normalized, removed zero padding, added error checking
in discriminator.  lots and lots and lots of comment changed.  formatted for
doxygen.

2.9.2 2008-03-03
fixed inversion in lowest std mix

2.9.1 2008-03-03
added lin log controls to plotter

2.9.0 2008-03-03
added ability to send different triggers and pulses to the file writer
added fft writer

2.8.2 2008-02-29
fixed another plotter error

2.8.1 2008-02-29
fixed plotter problem with manual mode and adding channels

2.8.0 2008-02-29
reworked pls writer and it's header

2.7.1 2008-02-25
added quick select plotter button

2.7.0 2008-02-22
added config file support

2.6.4 2008-02-22
added raw min and raw max to the header

2.6.3 2008-02-21
fixed a baseline plotting bug

2.6.2 2008-02-18
fixed stupid bug that disallowed edge triggering.  remember, don't compare two doubles with ==

2.6.1 2008-02-07
added rewind data button and suport

2.6.0 2008-02-07
added support for Rocket data stuff
fixed the edge trigger
removed any reference to yscale and yoffset as hard coded number.  they come from server. period.

2.5.3 2008-02-01
added some debugging stuff for NIST

2.5.2 2008-01-31
fixed the fft so that it is correctly normalized

2.5.1 2008-01-31
cleaned up ndfbserver
added a blocking select command to the command loop

2.5.0 2008-01-30
major clean up of the way the client and gui sense server death.  much more stable
when the sever crashes.  catches read and write errors much better and handles them correctly

2.4.6 2008-01-29
changed trigger class to trigger structure which allows it to be copied and passed around easier
fixed the progress meter for the optimize mix

2.4.5 2008-01-28
added a toggle switch so the user can turn pulse analysis on or off

2.4.4 2008-01-28
fixed a problem with pretrig and trig lengths
fixed small memory leak problem with Trigger structures

2.4.3 2008-01-25
rework how the client and streamData objects deal with decimation and mixing.
removed decimation from XPulseRec as it doesn't really belong there
added effective sampling rate function
added tilt variable so the GUI can sense when something has changed with the
server configuration
did a lot of code clean up

2.4.2 2008-01-18
massive clean up of the trigger generation code.  instead of passing triggers, pass a trigger
structure that keeps information about how the trigger was generated

2.4.1 2007-01-17
added a display for the sample rate

2.4.0 2007-01-17
adapted the optimal mixing algorithm to include coupled pairs and selectable algorithms

2.3.1 2007-01-14
fixed a bug with plotting wrt max and min resetting

2.3.0
added the really minor to the version string.  i know... a really minor change

2.3 2007-01-11
added the ability to choose which channel are sent to the data file
even if they trigger

2.2
added ability to change decimation mode

2.1
two types of couple trigger
shrunk the group trigger window

2.0
Jonathan's version of the xcaldaq_client

*/
