
#if defined(__APPLE__)
#if defined( __BIG_ENDIAN__)
#define ntohll(x) (x)
#define htonll(x) (x)
#else                                             /* Little Endian */
#include <machine/byte_order.h>
#define ntohll(x) NXSwapLongLong(x)
#define htonll(x) NXSwapLongLong(x)
#endif                                            /* __BIG_ENDIAN__ */
#endif                                            /* __APPLE__ */

#ifdef __linux__

#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) (x)
#define htonll(x) (x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) bswap_64(x)
#define htonll(x) bswap_64(x)
#else
#error Uknown Byte Ordering
#endif
#endif                                            //__linux__

#ifdef WIN32
#define ntohll(x) (((unsigned long long)(ntohl((int)((x << 32) >> 32))) << 32) | (unsigned int)ntohl(((int)(x >> 32))))
#define htonll(x) ntohll(x)
#endif                                            //WIN32
