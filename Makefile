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

ROOTDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

SRCDIR = $(ROOTDIR)/src
BUILDDIR = build
BINDIR = bin

CMAKE = cmake
QEMUFLAGS = -m 8G


.PHONY: all
all: efi-image grub-image


.PHONY: clean
clean:
	rm -rf $(BINDIR)
	rm -rf $(BUILDDIR)


###############################################################################
# EFI
###############################################################################

.PHONY: efi-image
efi-image: efi-boot
	rm -rf $(BUILDDIR)/efi-image
	mkdir -p $(BUILDDIR)/efi-image/efi/boot
	cp $(BUILDDIR)/x86/efi/bootia32.efi $(BUILDDIR)/efi-image/efi/boot
	cp $(BUILDDIR)/x86_64/efi/bootx64.efi $(BUILDDIR)/efi-image/efi/boot

.PHONY: efi-boot
efi-boot: $(BUILDDIR)/x86/efi/Makefile $(BUILDDIR)/x86_64/efi/Makefile
	$(MAKE) -C $(BUILDDIR)/x86/efi
	$(MAKE) -C $(BUILDDIR)/x86_64/efi

$(BUILDDIR)/x86/efi/Makefile:
	mkdir -p $(dir $@)
	cd $(dir $@); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOTDIR)/cmake/toolchain-x86-efi.cmake $(SRCDIR)/boot/efi

$(BUILDDIR)/x86_64/efi/Makefile:
	mkdir -p $(dir $@)
	cd $(dir $@); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOTDIR)/cmake/toolchain-x86_64-efi.cmake $(SRCDIR)/boot/efi


###############################################################################
# Multiboot
###############################################################################

.PHONY: grub-image
grub-image: multiboot
	rm -rf $(BUILDDIR)/grub-image
	mkdir -p $(BUILDDIR)/grub-image/boot/grub
	cp $(BUILDDIR)/x86/multiboot/multiboot $(BUILDDIR)/grub-image/boot/kiznix_multiboot.elf
	cp $(SRCDIR)/iso/grub.cfg $(BUILDDIR)/grub-image/boot/grub/grub.cfg
	mkdir -p $(BINDIR)
	grub-mkrescue -o $(BINDIR)/kiznix-grub-image.iso $(BUILDDIR)/grub-image

.PHONY: multiboot
multiboot: $(BUILDDIR)/x86/multiboot/Makefile
	$(MAKE) -C $(BUILDDIR)/x86/multiboot

$(BUILDDIR)/x86/multiboot/Makefile:
	mkdir -p $(dir $@)
	cd $(dir $@); cmake -DCMAKE_TOOLCHAIN_FILE=$(ROOTDIR)/cmake/toolchain-x86-kiznix.cmake $(SRCDIR)/boot/multiboot


###############################################################################
# Run targets
###############################################################################

.PHONY: run-qemu-32
run-qemu-32: grub-image
	qemu-system-i386 $(QEMU_FLAGS) -cdrom $(BINDIR)/kiznix-grub-image.iso

.PHONY: run-qemu-64
run-qemu-64: grub-image
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom $(BINDIR)/kiznix-grub-image.iso

.PHONY: run-efi-32
run-efi-32: efi-image
	qemu-system-i386 $(QEMUFLAGS) -bios OVMF-ia32.fd -hda fat:$(BUILDDIR)/efi-image

.PHONY: run-efi-64
run-efi-64: efi-image
	qemu-system-x86_64 $(QEMUFLAGS) -bios OVMF-x64.fd -hda fat:$(BUILDDIR)/efi-image

.PHONY: run-bochs
run-bochs: efi-image
	bochs -q

.PHONY: run
run: run-qemu-64
