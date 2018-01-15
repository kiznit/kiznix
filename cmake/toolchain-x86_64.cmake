SET(CMAKE_SYSTEM_NAME Kiznix)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)
SET(CMAKE_SYSTEM_VERSION 1)

INCLUDE(CMakeForceCompiler)
CMAKE_FORCE_C_COMPILER(x86_64-elf-gcc GNU)

SET(CMAKE_C_SIZEOF_DATA_PTR 8)

SET(CMAKE_C_FLAGS "-O2 -g -ffreestanding -fbuiltin -Wall -Wextra -std=gnu99 -mcmodel=kernel -mno-red-zone" CACHE STRING "" FORCE)
