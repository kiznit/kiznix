#define HAVE_MORECORE 0
#define LACKS_SCHED_H 1
#define LACKS_SYS_PARAM_H 1
#define MMAP_CLEARS 0
#define NO_MALLOC_STATS 1

// todo: use our own spinlocks (or at least provide sched_yield())
#define USE_LOCKS 1


#include "dlmalloc.inc"
