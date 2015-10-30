# Copyright (c) 2015, Thierry Tremblay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(KIZNIX_MULTIBOOT)

# nasm support is broken in CMake, this will fix it
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../../cmake)
INCLUDE(CMakeASM-NASMCompiler)
ENABLE_LANGUAGE(ASM-NASM)


ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/../../libc "${CMAKE_CURRENT_BINARY_DIR}/libc")


SET(MULTIBOOT_SRCS
    console.cpp
    multiboot.asm
    multiboot.cpp
)


INCLUDE_DIRECTORIES(
    SYSTEM ../../include
    SYSTEM ${CMAKE_SOURCE_DIR}/../../libc/include
)



# Grub is compiled with -march=i386 by default. Let's do the same.
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i386")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i386")



# dummy.c is used to create a dependency on multiboot.ld
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dummy.c
    COMMAND echo "/* This file is empty */" > ${CMAKE_CURRENT_BINARY_DIR}/dummy.c
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/multiboot.ld
)


SET(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/multiboot.ld -nostdlib")

ADD_EXECUTABLE(multiboot ${CMAKE_CURRENT_BINARY_DIR}/dummy.c ${MULTIBOOT_SRCS})

TARGET_LINK_LIBRARIES(multiboot libc)
