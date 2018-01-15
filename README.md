Kiznix
======

Thierry's own OS. Currently supports three targets: x86, x86 with PAE and x86_64.


License
=======

Kiznix is licensed under the BSD (Simplified) 2-Clause License.


Building Kiznix
===============

Required tools
--------------

To build Kiznix from your existing operating system, the following tools are
required (version numbers show what I am using and do not indicate strict
requirements):

    1) binutils 2.24.90 (i686-elf and x86_64-elf tools)
    2) gcc 4.8.3 (i686-elf and x86_64-elf cross-compilers)
    3) NASM 2.10.09
    4) CMake 2.8.12
    5) GNU Make 3.81

If you are unsure how to produce the required binutils and gcc cross-compilers,
take a look here: http://wiki.osdev.org/GCC_Cross-Compiler.

To generate an ISO image of Kiznix, you will also need:

    6) grub-mkrescue 2.02

And finally if you want to run your iso image under an emulator, I use qemu:

    7) qemu-system-x86_64 2.0.0


Makefile targets
----------------

    - make            --> build all kernel variants
    - make clean      --> cleanup
    - make iso        --> build an iso image with all kernel variants on it
    - make run-bochs  --> run the iso image under bochs
    - make run-qemu   --> run the iso image under qemu
