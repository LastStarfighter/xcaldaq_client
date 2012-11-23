////////////////////////////////////////////////////////////////////////////////
/*
 description:
    Lays out the structure of the packets passed back and forth between
    the client and the server

    VERSION 1:
    An XCALDAQRecord, when packed looks like:
    LDword	Dword	Sword		Byte		Width   Description
    0			0     0-1		0-3		32bits  Channel Number
                1     2-3		4-7		32bits  Total bytes in this record
    1			2     4-5		8-11		32bits  Bytes in Header
3     6-7		12-15		32bits  Number of Samples in this record
2			4     8			16			8bits   Bits per Sample
17			8bits   Version
9			18-19		16bits  Flags
5     10-11		20-23		32bits  Empty  (something may go here eventually)
3			6-7   12-15		24-31		64bits  "Count" of the last sample in this record (used for channel synchronization and timekeeping)
4			8-9   16-19		32-39		64bits  UT (in microseconds since 1970)  (or zero if UTValidFlag bit isn't set in flags)
then a buffer of net ordered data

VERSION 2:
An XCALDAQRecord, when packed looks like:
LDword	Dword	Sword		Byte		Width		Description
0			0     0-1		0-3		32bits	Channel Number
1		2-3		4-7		32bits	Total bytes in this record
1			2     4-5		8-11		32bits	Bytes in Header
3		6-7		12-15		32bits	Number of Samples in this record
2			4     8			16			8bits		Bits per Sample
17			8bits		Version
9			18-19		16bits	Flags
5     10			20-21		16bits	Decimation Level: Short Int that represents the amount the data was decimated
11			22-23		16bits	Empty (something may eventually go here)
3			6-7   12-15		24-31		64bits	"Count" of the last sample in this record (used for channel synchronization and timekeeping)
4			8-9	16-19		32-39		64bits	UT (in microseconds since 1970)  (or zero if UTValidFlag bit isn't set in flags)
5			10		20-21		40-43		32bits	Mixing ratio: Float b/w 0-1 that represents amount of error in signal
11		22-23		44-47		32bits	Empty (for something else or in case mixing param needs to be 8 bytes)
then a buffer of net ordered data

VERSION 3:
An XCALDAQRecord, when packed looks like:
LDword	Dword	Sword		Byte		Width		Description
0			0     0-1		0-3		32bits	Channel Number
1		2-3		4-7		32bits	Total bytes in this record
1			2     4-5		8-11		32bits	Bytes in Header
3		6-7		12-15		32bits	Number of Samples in this record
2			4     8			16			8bits		Bits per Sample
17			8bits		Version
9			18-19		16bits	Flags
5     10			20-21		16bits	Decimation Level: Short Int that represents the amount the data was decimated
11			22-23		16bits	Empty (something may eventually go here)
3			6-7   12-15		24-31		64bits	"Count" of the last sample in this record (used for channel synchronization and timekeeping)
4			8-9	16-19		32-39		64bits	UT (in microseconds since 1970)  (or zero if UTValidFlag bit isn't set in flags)
5			10		20-21		40-43		32bits	Mixing ratio: Float b/w 0-1 that represents amount of error in signal
11		22-23		44-47		32bits	Empty (for something else or in case mixing param needs to be 8 bytes)
6			12		24-25		48-51		32bits	Sampling Rate Numerator
13		26-27		52-55		32bits	Sampling Rate Denominator
7			14		28-29		56-59		32bits	Offset (remember: volts = offset + int * scale)
15		30-31		60-63		32bits	Scale
then a buffer of net ordered data

VERSION 4:
Same as version 3 but with xcfmixInversionFlag now one of the flags

VERSION 5:
Added a 64 bit number that is the time count in addition to the sample count. Added 6/22/07

An XCALDAQRecord, when packed looks like:
LDword	Dword	Sword		Byte		Width		Description
0			0     0-1		0-3		32bits	Channel Number
1		2-3		4-7		32bits	Total bytes in this record
1		 	2     4-5		8-11		32bits	Bytes in Header
3		6-7		12-15		32bits	Number of Samples in this record
2			4     8			16			8bits		Bits per Sample
17			8bits		Version
9			18-19		16bits	Flags
5     10			20-21		16bits	Decimation Level: Short Int that represents the amount the data was decimated
11			22-23		16bits	EMPTY (something may eventually go here)
3			6-7   12-15		24-31		64bits	The Sample Count of the last sample in this record.  Used for group triggering
4			8-9	16-19		32-39		64bits	UT (in microseconds since 1970)  (or zero if UTValidFlag bit isn't set in flags)
5			10		20-21		40-43		32bits	Mixing ratio: Float b/w 0-1 that represents amount of error in signal
11		22-23		44-47		32bits	EMPTY (for something else or in case mixing param needs to be 8 bytes)
6			12		24-25		48-51		32bits	Sampling Rate Numerator
13		26-27		52-55		32bits	Sampling Rate Denominator
7			14		28-29		56-59		32bits	Volt Offset (remember: volts = offset + raw * scale)
15		30-31		60-63		32bits	Volt Scale
8			16-17	32-35		64-73		64bits	The time count of the last sample in the record.  Used for time keeping
then a buffer of net ordered data

VERSION 7:
Added 2 x 32 bit numbers that are the raw max / min number that corresponds to the physical limits of the ADC

An XCALDAQRecord, when packed looks like:
LDword	Dword	Sword		Byte		Width		Description
0			0     0-1		0-3		32bits	Channel Number
1		2-3		4-7		32bits	Total bytes in this record
1		 	2     4-5		8-11		32bits	Bytes in Header
3		6-7		12-15		32bits	Number of Samples in this record
2			4     8			16			8bits		Bits per Sample
17			8bits		Version
9			18-19		16bits	Flags
5     10			20-21		16bits	Decimation Level: Short Int that represents the amount the data was decimated
11			22-23		16bits	EMPTY (something may eventually go here)
3			6-7   12-15		24-31		64bits	The Sample Count of the last sample in this record.  Used for group triggering
4			8-9	16-19		32-39		64bits	UT (in microseconds since 1970)  (or zero if UTValidFlag bit isn't set in flags)
5			10		20-21		40-43		32bits	Mixing ratio: Float b/w 0-1 that represents amount of error in signal
11		22-23		44-47		32bits	EMPTY (for something else or in case mixing param needs to be 8 bytes)
6			12		24-25		48-51		32bits	Sampling Rate Numerator
13		26-27		52-55		32bits	Sampling Rate Denominator
7			14		28-29		56-59		32bits	Volt Offset (remember: volts = offset + raw * scale)
15		30-31		60-63		32bits	Volt Scale
8			16-17	32-35		64-71		64bits	The time count of the last sample in the record.  Used for time keeping
9       18    36-37    72-75    32bits   The max raw number that the samples will be
19    38-39    76-79    32bits   The min raw number that the samples will be
then a buffer of net ordered data

NOTES:
- All integers are in network byte order.
- All integers but the samples are unsigned.
- The Samples may be signed or unsigned depending on the SignedSamples flag.
- The indexes are always _with respect to their appropriate data type_

revision history:
2008-02-21 - version 7 - raw min and max
2007-03-13 - header now includes samplerate, yoffset, and yscale
2007-02-23 - updated to version 2 which includes mixing and decimation information
2007-02-02 - submitted to cvs
2007-02-01 - first compiled version

*/

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __XCALDAQRecordConstants_h
#define __XCALDAQRecordConstants_h

////////////////////////////////////////////////////////////////////////////////

// macros
// version 1 Flags
#define xcfUTValid         0x0001
#define xcfSignedSamples   0x0002
#define xcfMixFlag         0x0004
#define xcfDecimateFlag    0x0008

// version 1 stuff
#define xcdChanIdx 0                              // 0th Byte
#define xcdBytesInRecIdx 1                        // 1 DWord
#define xcdBytesInHeaderIdx 2                     // second DWord over
#define xcdNumSampsIdx 3                          // third DWord over
#define xcdFlagsIdx 9                             // 9th SWord over
#define xcdBitsPerSampleIdx 16                    //16th byte over
#define xcdVersionIdx 17                          // the byte the version is on
#define xcdLastSampCountIdx  3                    // third LDWord over
#define xcdUTIdx 4                                // 4th LDWord over
#define xcdHeaderBytes 40                         // length of the header in bytes

// version 2 overrides (make sure you redefine the header size!)
#define xcdDecimate 10                            // 10th Sword over
#define xcdMix 10                                 // 10th DWord Over
#undef xcdHeaderBytes
#define xcdHeaderBytes 48

// version 3 stuff ( make sure you redefine the header size!)
#define xcdSampleRateN 12
#define xcdSampleRateD 13
#define xcdOffset 14
#define xcdScale 15
#undef xcdHeaderBytes
#define xcdHeaderBytes 64

// version 4 stuff (no need to redefine header size b/c this goes in the flag word)
#define xcfMixInversionFlag 0x0010

// version 5 stuff ( make sure you redefine the header size!)
#define xcdVersion 5                              // don't know why the version hasn't been in this header before this
#define xcdTimeCountIdx 8
#undef xcdHeaderBytes
#define xcdHeaderBytes 72                         // length of the header in bytes

// version 6 stuff some bit flags (no need to redefine header size b/c this goes in the flag word)
#define xcfDecimateAvgFlag 0x0020

// version 7 stuff
#undef xcdVersion
#define xcdVersion 6
#define xcdRawMax 18
#define xcdRawMin 19
#undef xcdHeaderBytes
#define xcdHeaderBytes 80                         // length of the header in bytes

////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
