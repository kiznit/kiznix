SET(CMAKE_SYSTEM_NAME EFI)
SET(CMAKE_SYSTEM_PROCESSOR x86)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)

SET(COMMON_FLAGS "-O2 -ffreestanding -fbuiltin -mno-stack-arg-probe -Wall -Wextra")
SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99" CACHE STRING "" FORCE)
SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -fno-exceptions -fno-rtti" CACHE STRING "" FORCE)

#-fno-stack-check
#-fno-stack-protector
#-mno-stack-arg-probe
