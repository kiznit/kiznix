// <assert.h> can be included multiple times, with and without NDEBUG defined.
// This is why there is no header guard and why we #undef assert.

#ifdef __cplusplus
extern "C" {
#endif

#undef assert


void __assert(const char* expression, const char* file, int line)  __attribute__ ((noreturn));

#if defined(NDEBUG)
#define assert(expression) ((void)(0))
#else
#define assert(expression) (__builtin_expect(!(expression), 0) ? __assert(#expression, __FILE__, __LINE__) : (void)0)
#endif



#ifdef __cplusplus
}
#endif
