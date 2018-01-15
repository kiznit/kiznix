SET(SRCS
    api.c
    decode.c
    mem.c
    ops.c
    ops2.c
    prim_ops.c
)



INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_SOURCE_DIR}/src/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/libc/include
    SYSTEM .
)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

#" -Wno-unused-parameter -Wstrict-aliasing=0")


ADD_LIBRARY(libx86emu STATIC ${SRCS})
