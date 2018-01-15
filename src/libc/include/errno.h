#ifndef _INCLUDED_ERRNO_H
#define _INCLUDED_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif



#define EINVAL 21
#define ENOMEM 23



//todo: this needs to be per-thread
extern int errno;


#ifdef __cplusplus
}
#endif

#endif
