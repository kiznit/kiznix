CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(KIZNIX NONE)
ENABLE_LANGUAGE(C)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

SET(KIZNIX_PAE false CACHE BOOL "Enable PAE for 32 bits kernel")


# Architecture
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
SET(ARCH "x86")
SET(ARCH_BITS 32)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
SET(ARCH "x86")
SET(ARCH_BITS 64)
endif()

message("CMAKE_SYSTEM_PROCESSOR is ${CMAKE_SYSTEM_PROCESSOR}")
message("ARCH is ${ARCH}, ARCH_BITS is ${ARCH_BITS}")

# nasm support is broken in CMake, this will fix it
INCLUDE(CMakeASM-NASMCompiler)
ENABLE_LANGUAGE(ASM-NASM)


# This is KIZNIX
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__KIZNIX__")


# C Flags
if (KIZNIX_PAE)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DKIZNIX_PAE")
endif()


# Include projects
ADD_SUBDIRECTORY(src/acpica)
ADD_SUBDIRECTORY(src/kernel)
ADD_SUBDIRECTORY(src/libc)
ADD_SUBDIRECTORY(src/libx86emu)
ADD_SUBDIRECTORY(iso)


# Custom target to run the kernel under qemu

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    SET(QEMU qemu-system-i386)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET(QEMU qemu-system-x86_64)
endif()

ADD_CUSTOM_TARGET(run
    COMMAND ${QEMU} -m 8G -vga std -cdrom iso/kiznix.iso &
    DEPENDS iso
)
