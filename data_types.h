#ifndef _jsa_typedefs_
#define _jsa_typedefs_

/*! \file data_types.h
\brief typedefs to assist in portability

*/

//defined in the C99 <inttypes.h>
//int8_t   int16_t   int32_t   int64_t
//uint8_t   uint16_t   uint32_t   uint64_t

//#if defined(_WIN32)
// this is here so I can easliy find this define
//#endif

#ifndef GLIB_MAJOR_VERSION
typedef unsigned char guint8;
typedef signed char gint8;
typedef char        gchar8;                       //!< 8 bit char
typedef signed char     gschar8;                  //!< 8 bit signed char
typedef unsigned char   guchar8;                  //!< 8 bit unsigned char
typedef signed short    gint16;                   //!< 16 bit signed int
typedef unsigned short  guint16;                  //!< 16 bit unsigned int
typedef signed int      gint32;                   //!< 32 bit signed int
typedef unsigned int    guint32;                  //!< 32 bit signed int

typedef float       gfloat32;                     //!< 32 bit float
typedef double      gdouble64;                    //!< 64 bit double

#ifndef __APPLE__
typedef long double     gdouble96;
#endif

typedef gchar8          gchar;                    //!< 8 bit char
typedef guchar8          guchar;                  //!< 8 bit unsigned char
typedef gfloat32        gfloat;                   //!< 32 bit float
typedef gdouble64       gdouble;                  //!< 64 bit double

// 64 bit quantities are different for different compilers

//......................................begin gcc
#if (defined __GNUG__)
typedef long long       gint64;                   //!< 64 bit signed int
typedef unsigned long long  guint64;              //!< 64 bit unsigned int

//......................................begin icc (linux)
#elif (defined __INTEL_COMPILER)

//......................................begin visual studio 6
#elif (defined _MSC_VER) && _MSC_VER < 1300
typedef __int64  gint64;                          //!< 64 bit signed int
typedef __int64 guint64;                          //!< 64 bit unsigned int
// I don't think there is a __uint64 for msvc

//......................................begin visual studio .NET
// Visual C++.Net Family is _MSC_VER >= 1300
//
// .Net 2002 is 1300?
// .Net 2003 is 1310?
#elif defined(_MSC_VER) && _MSC_VER >= 1300
typedef __int64  gint64;                          //!< 64 bit signed int
typedef __int64 guint64;                          //!< 64 bit unsigned int

#else
//......................................begin unknown compiler
#endif                                            // end of if compiler
#endif                                            // end of if n GLIB
#endif                                            // end of sentinel
