////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __XCALDAQCommands_h
#define __XCALDAQCommands_h
////////////////////////////////////////////////////////////////////////////////

// 16 bytes

/*
    LDword	Dword		Sword		Byte		Width			description
    0			0			0			0-1		16 bits		command or acknowledge
                            1			2-3		16 bits		primary command (get, set, kill, ping)
                1			2			4-5		16 bits		secondary command
                            3			6			8 bis			error flag
                                        7			8 bits		error number
    1			2			4-5		8-11		32 bits		channel number
                3			6-7		12-15		32 bits		level

 */

// the channel number that mean  "apply to all channls"
#define XCD_ALLCHANNELS 0xFFFFFFFF                ///< signifies all channel

// command or acknowledge
#define XCD_COMMAND 0                             ///< command from a client
#define XCD_ACKNOWLEDGE 1                         ///< response from server

// primary commands
#define XCD_GET 0                                 ///< tell the server to send something back
#define XCD_SET 1                                 ///< tell the server to set something
#define XCD_KILLME 2                              ///< tell the server to kill the client
#define XCD_KEEPALIVE 3                           ///< keep client alive (not really used anymore)
#define XCD_TESTCOMM 4                            ///< tell the server to send the client some test data
#define XCD_TESTDATA 5                            ///< send this along with an int level, and the server will give level bytes of data back
#define XCD_REWIND_DATA_FILE 6                    ///< rewind the data file in the playback server

// secondary commands

// client specific bools (no channel information)
#define XCD_DATAFLAG 0                            ///< streaming data flag
#define XCD_INITIALIZED 1                         ///< server initialized flag

// client specific shorts (no channel information)

// client specific longs (no channel information)
#define XCD_VERSION 49                            ///< 4 bytes of version information
#define XCD_BUFFERLEVEL 50                        ///< how full the client buffer is (get only)
#define XCD_CHANNELS 51                           ///< number of channels
#define XCD_STARTTS 52                            ///< start time seconds part
#define XCD_STARTTUS 53                           ///< start time useconds part
#define XCD_SAMPLERATE 54                         ///< sample rate
#define XCD_BOARDS 55                             ///< number of boards (or mux columns)
#define XCD_STREAMS_PER_BOARD 58                  ///< number of streams per row (or mux rows)
#define XCD_SAMPLES 56                            ///< number of samples co-added to make final fb signal
#define XCD_SERVERTYPE 57                         ///< 4 bytes of server information

// channel specific bools (channel + bool)
#define XCD_ACTIVEFLAG 100                        ///< channel is actively streaming data to client flag
#define XCD_MIXFLAG 101                           ///< mix on or off flag
#define XCD_DECIMATEFLAG 102                      ///< decimate on or off flag
#define XCD_MIXINVERSIONFLAG 103                  ///< mix inversion on or off flag
#define XCD_DECIMATEAVGFLAG 104                   ///< whether to average decimation or just drop the data

// channel specific shorts (channel + short)
#define XCD_DECIMATELEVEL 150                     ///< set the decimation level

// channel specific longs (channel + long)
#define XCD_MIXLEVEL 200                          ///< how much the signal and error are mixed
#define XCD_GAINLEVEL 201                         ///< gain setting for this channel
#define XCD_RAW_MIN_LEVEL 202                     ///< signed int for the max raw number (for 12 bit unsigned, that would be 2^12-1)
#define XCD_RAW_MAX_LEVEL 203                     ///< signed int for the min raw number

// the error flag
#define XCD_ERRORFLAG 1                           ///< an error is present

// the error numbers
#define XCD_ERROR00 0                             ///< unspecified error
#define XCD_ERROR01 1                             ///< sending all channels with something that is channel dependent
#define XCD_ERROR02 2                             ///< trying to set something you can't set i.e. # of boards

////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
