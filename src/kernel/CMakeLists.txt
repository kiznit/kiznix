SET(ARCH_SUFFIX "_${ARCH_BITS}")

if (KIZNIX_PAE)
    SET(BOOT_SUFFIX "_pae")
else()
    SET(BOOT_SUFFIX "_${ARCH_BITS}")
endif()


# KERNEL_VIRTUAL_BASE
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    SET(KERNEL_PHYSICAL_ADDRESS 0x00100000)
    SET(KERNEL_VIRTUAL_BASE 0xC0000000)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET(KERNEL_PHYSICAL_ADDRESS 0x00100000)
    SET(KERNEL_VIRTUAL_BASE 0xFFFFFFFFC0000000)
endif()




SET(ARCH_SRCS
    ${ARCH}/boot.asm
    ${ARCH}/boot${BOOT_SUFFIX}.asm
    ${ARCH}/cpu.c
    ${ARCH}/interrupt.c
    ${ARCH}/interrupt${ARCH_SUFFIX}.asm
    ${ARCH}/pci.c
    ${ARCH}/pmm.c
    ${ARCH}/thread${ARCH_SUFFIX}.asm
    ${ARCH}/timer.c
    ${ARCH}/vmm.c
)


if (ARCH STREQUAL "x86")
    LIST(APPEND ARCH_SRCS
        x86/bios.c
        x86/pic.c
    )
endif()


SET(SRCS
    acpi.c
    console.c
    kernel.c
    mutex.c
    semaphore.c
    spinlock.c
    thread.c
)


INCLUDE_DIRECTORIES(
    SYSTEM ${CMAKE_SOURCE_DIR}/src/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/acpica/source/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/libc/include
    SYSTEM ${CMAKE_SOURCE_DIR}/src/libx86emu/include
)

# Include path for NASM
SET(CMAKE_ASM-NASM_FLAGS "${CMAKE_ASM-NASM_FLAGS} -I${CMAKE_SOURCE_DIR}/src/include/")




SET(CMAKE_ASM-NASM_FLAGS "${CMAKE_ASM-NASM_FLAGS} -DKERNEL_VIRTUAL_BASE=${KERNEL_VIRTUAL_BASE}")


SET(LIBS gcc)

#SET(CRTI ${ARCH}/crti.asm)
#SET(CRTN ${ARCH}/crtn.asm)

# Find crtbegin.o
#EXECUTE_PROCESS(
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -print-file-name=crtbegin.o
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#    OUTPUT_VARIABLE CRTBEGIN)

# Find crtend.o
#EXECUTE_PROCESS(
#    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_C_FLAGS} -print-file-name=crtend.o
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#    OUTPUT_VARIABLE CRTEND)


# dummy.c is used to create a dependency on linker.ld
ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dummy.c
    COMMAND echo "/* This file is empty */" > ${CMAKE_CURRENT_BINARY_DIR}/dummy.c
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/linker${ARCH_SUFFIX}.ld
)


SET(CMAKE_EXE_LINKER_FLAGS "-Xlinker --defsym=KERNEL_PHYSICAL_ADDRESS=${KERNEL_PHYSICAL_ADDRESS} -Xlinker --defsym=KERNEL_VIRTUAL_BASE=${KERNEL_VIRTUAL_BASE} -T ${CMAKE_CURRENT_SOURCE_DIR}/${ARCH}/linker${ARCH_SUFFIX}.ld -O2 -ffreestanding -nostdlib")

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z max-page-size=0x1000")
endif()


# Linker order must be ${CRTI} ${CRTBEGIN} ${OBJS} ${CRTEND} ${CRTN} ${LIBS}

#TODO: can't link crtbegin.o and crtend.o with mcmodel=kernel (x86_64). For now, no CRT
#      to fix this, we would have to recompile crtstuff.c using mcmodel=kernel to produce crtbegin.o and crtend.o
#ADD_EXECUTABLE(kernel ${CMAKE_CURRENT_BINARY_DIR}/dummy.c ${CRTI} ${CRTBEGIN} ${ARCH_SRCS} ${SRCS} ${CRTEND} ${CRTN})

ADD_EXECUTABLE(kernel ${CMAKE_CURRENT_BINARY_DIR}/dummy.c ${ARCH_SRCS} ${SRCS})

TARGET_LINK_LIBRARIES(kernel libc libacpica libx86emu)
TARGET_LINK_LIBRARIES(kernel ${LIBS})
