UEFI Firmware Bugs
==================

1) AllocatePool() and AllocatePages() using an application defined memory type.
-------------------------------------------------------------------------------

According to the UEFI specification, memory types with value 0x80000000
and above are reserved to the application. Using a memoty type in that
range will make the next call to GetMemoryMap() hang some computers. This
is happening on my main development machine (Hero Hero Maximus VI,
firmware build 1603 2014/09/19).

The workaround is to not use application defined memory types and use
EfiBootServicesData instead. It does mean that we can't identify our own
allocations when walking the memory map, which is rather unfortunate.


2) SetVirtualMemoryMap() calling into EFI Boot Services.
--------------------------------------------------------

Once you call ExitBootServices(), you should still be able to call the
EFI Runtime Services functions, such as SetVirtualMemoryMap(). Multiple
firmware are buggy and will crash when you do so. The only "workaround"
is to call SetVirtualMemoryMap() to remap the EFI Runtime Services
memory before exiting the EFI Boot Services.


3) Writing UEFI variables will brick some machines.
---------------------------------------------------

This has been reported for some Samsung laptops. Details can be found
here: http://mjg59.dreamwidth.org/22855.html.

Linux seems to work around this specific issue by making sure less than
half of the available variable memory is used at any given time.
