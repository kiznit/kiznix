SET(LIBC_SRCS
    stdio/printf.c
    stdio/putchar.c
    stdio/puts.c
    string/strlen.c
)


INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/include
)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -imacros libc-symbols.h")


ADD_LIBRARY(libc STATIC ${LIBC_SRCS})
