SET(LIBC_SRCS
    stdio/printf.c
    stdio/putchar.c
    stdio/puts.c
    string/strlen.c
)


INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include
)



ADD_LIBRARY(libc STATIC ${LIBC_SRCS})
