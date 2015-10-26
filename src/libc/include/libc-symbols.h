#ifndef _INCLUDED_LIBC_SYMBOLS_H
#define _INCLUDED_LIBC_SYMBOLS_H

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__MINGW32__)
#define WEAK_FUNCTION
#else
#define WEAK_FUNCTION __attribute__ ((weak))
#endif



#ifdef __cplusplus
}
#endif


#endif
