file(GLOB_RECURSE SRCS
    source/components/dispatcher/*.c
    source/components/events/*.c
    source/components/executer/*.c
    source/components/hardware/*.c
    source/components/namespace/*.c
    source/components/parser/*.c
    source/components/resources/*.c
    source/components/tables/*.c
    source/components/utilities/*.c
)


INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_SOURCE_DIR}/src/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/libc/include
    source/include
)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wstrict-aliasing=0")


ADD_LIBRARY(libacpica STATIC ${SRCS})
