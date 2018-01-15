SET(ISO_IMAGE kiznix.iso)
SET(ISO_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/isodir)


if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    ADD_CUSTOM_TARGET(copy_kernel
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> "${ISO_TARGET_DIR}/boot/kiznix_x86.elf"
        DEPENDS kernel
    )
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    # Grub wants an elf32 binary, not an elf64 one
    ADD_CUSTOM_TARGET(copy_kernel
        COMMAND mkdir -p ${ISO_TARGET_DIR}/boot
        COMMAND x86_64-elf-objcopy $<TARGET_FILE:kernel> -O elf32-i386 "${ISO_TARGET_DIR}/boot/kiznix_x86_64.elf"
        DEPENDS kernel
    )
endif()


ADD_CUSTOM_TARGET(iso_layout
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/grub.cfg ${ISO_TARGET_DIR}/boot/grub/grub.cfg
    DEPENDS copy_kernel ${CMAKE_CURRENT_SOURCE_DIR}/grub.cfg
)


ADD_CUSTOM_TARGET(iso
    COMMAND grub-mkrescue -o ${ISO_IMAGE} ${ISO_TARGET_DIR} > /dev/null 2>&1
    DEPENDS iso_layout
)
