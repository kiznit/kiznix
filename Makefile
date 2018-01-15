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

ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BUILD_DIR ?= build
BUILD_DIR_X86 ?= $(BUILD_DIR)/x86
BUILD_DIR_X86_PAE ?= $(BUILD_DIR)/x86_pae
BUILD_DIR_X86_64 ?= $(BUILD_DIR)/x86_64
BUILD_DIR_ISO ?= $(BUILD_DIR)/iso

.PHONY: all
all: iso


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)


.PHONY: kernel_x86
kernel_x86: $(BUILD_DIR_X86)/Makefile
	$(MAKE) -s -C $(BUILD_DIR_X86)

.PHONY: kernel_x86_pae
kernel_x86_pae: $(BUILD_DIR_X86_PAE)/Makefile
	$(MAKE) -s -C $(BUILD_DIR_X86_PAE)

.PHONY: kernel_x86_64
kernel_x86_64: $(BUILD_DIR_X86_64)/Makefile
	$(MAKE) -s -C $(BUILD_DIR_X86_64)


$(BUILD_DIR_X86)/Makefile:
	mkdir -p $(BUILD_DIR_X86)
	cd $(BUILD_DIR_X86); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOT_DIR)/cmake/toolchain-x86.cmake $(ROOT_DIR)

$(BUILD_DIR_X86_PAE)/Makefile:
	mkdir -p $(BUILD_DIR_X86_PAE)
	cd $(BUILD_DIR_X86_PAE); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOT_DIR)/cmake/toolchain-x86.cmake -DKIZNIX_PAE=true $(ROOT_DIR)

$(BUILD_DIR_X86_64)/Makefile:
	mkdir -p $(BUILD_DIR_X86_64)
	cd $(BUILD_DIR_X86_64); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOT_DIR)/cmake/toolchain-x86_64.cmake $(ROOT_DIR)


.PHONY: kernels
kernels: kernel_x86 kernel_x86_pae kernel_x86_64


.PHONY: iso
iso: kernels $(BUILD_DIR_ISO)/kiznix.iso


$(BUILD_DIR_ISO)/kiznix.iso: $(ROOT_DIR)/iso/grub.cfg $(BUILD_DIR_X86)/src/kernel/kernel $(BUILD_DIR_X86_PAE)/src/kernel/kernel $(BUILD_DIR_X86_64)/src/kernel/kernel
	rm -rf $(BUILD_DIR_ISO)
	mkdir -p $(BUILD_DIR_ISO)/layout/boot/grub
	cp $(BUILD_DIR_X86)/src/kernel/kernel $(BUILD_DIR_ISO)/layout/boot/kiznix-x86.elf
	cp $(BUILD_DIR_X86_PAE)/src/kernel/kernel $(BUILD_DIR_ISO)/layout/boot/kiznix-x86-pae.elf
	x86_64-elf-objcopy $(BUILD_DIR_X86_64)/src/kernel/kernel -O elf32-i386 "$(BUILD_DIR_ISO)/layout/boot/kiznix-x86_64.elf"
	cp $(ROOT_DIR)/iso/grub.cfg $(BUILD_DIR_ISO)/layout/boot/grub/grub.cfg
	grub-mkrescue -o $(BUILD_DIR_ISO)/kiznix.iso $(BUILD_DIR_ISO)/layout


.PHONY: run-bochs
run-bochs: iso
	bochs -q


.PHONY: run-qemu
run-qemu: iso
	qemu-system-x86_64 -m 8G -vga std -cdrom $(BUILD_DIR_ISO)/kiznix.iso


.PHONY: run
run: run-qemu
