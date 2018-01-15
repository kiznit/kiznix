SET(SRCS
    math/modf.c
    posix/mman.c
    stdio/fcvtbuf.c
    stdio/printf.c
    stdio/putchar.c
    stdio/puts.c
    stdlib/abort.c
    stdlib/malloc.c
    assert.c
    errno.c
    string.c
    time.c
)


INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/include
)


ADD_LIBRARY(libc STATIC ${SRCS})
