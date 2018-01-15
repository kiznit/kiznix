/*
    Copyright (c) 2015, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <acpi.h>
#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/mutex.h>
#include <kernel/pci.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <kernel/vmm.h>
#include <kernel/x86/io.h>

#include <assert.h>
#include <stdio.h>


#define _COMPONENT          ACPI_KIZNIX
        ACPI_MODULE_NAME    ("kiznix")


static ACPI_STATUS acpi_device_callback(ACPI_HANDLE handle, UINT32 level, void* context, void** returnValue)
{
    (void) level;
    (void) context;
    (void) returnValue;

    // http://wiki.osdev.org/User_talk:Cognition
    //printf("acpi_device_callback(%p, %d, %p, %p)\n", handle, level, context, returnValue);
    ACPI_DEVICE_INFO* info;

    ACPI_STATUS status = AcpiGetObjectInfo(handle, &info);
    if(status != AE_OK)
    {
        printf("Error retrieving device information - code: %u\n", status);
        return status;
    }

    if (info->Valid & ACPI_VALID_HID)
    {
        printf("  HID: %s  ", info->HardwareId.String);
    }

    if (info->Valid & ACPI_VALID_CID)
    {
        ACPI_PNP_DEVICE_ID* deviceId = info->CompatibleIdList.Ids;
        for(uint32_t i = 0; i != info->CompatibleIdList.Count; ++i, ++deviceId)
        {
            printf("  CID: %s  ", deviceId->String);
        }
    }

    return AE_OK;
}


void acpi_init()
{
    printf("acpi_init()\n");

    ACPI_STATUS status;

    // Initialize the ACPICA subsystem
    status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPICA"));
        return;
    }

    status = AcpiInitializeTables(NULL, 16, FALSE);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing Table Manager"));
        return;
    }

    // Create the ACPI namespace from ACPI tables
    status = AcpiLoadTables();
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While loading ACPI tables"));
        return;
    }

    // Initialize the ACPI hardware
    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While enabling ACPICA"));
        return;
    }

    // Complete the ACPI namespace object initialization
    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPICA objects"));
        return;
    }


    // Test that ACPICA is working properly
    printf("Enumerating ACPI devices:\n");
    AcpiGetDevices(NULL, acpi_device_callback, 0, NULL);
    printf("\n");
}



ACPI_STATUS AcpiOsInitialize()
{
    printf("AcpiOsInitialize()\n");

    return AE_OK;
}



ACPI_STATUS AcpiOsTerminate()
{
    printf("AcpiOsTerminate()\n");

    return AE_OK;
}



ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
    printf("AcpiOsGetRootPointer\n");

    ACPI_PHYSICAL_ADDRESS address;
    AcpiFindRootPointer(&address);

    return address;
}



ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES* predefinedObject, ACPI_STRING* newValue)
{
    (void) predefinedObject;

    // No override
    *newValue = NULL;

    return AE_OK;
}



ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER* existingTable, ACPI_TABLE_HEADER** newTable)
{
    (void) existingTable;

    // No override
    *newTable = NULL;

    return AE_OK;
}



void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS physicalAddress, ACPI_SIZE length)
{
    //printf("AcpiOsMapMemory(0x%x, 0x%x)\n", (int)physicalAddress, (int)length);

    return vmm_map(physicalAddress, length);
}



void AcpiOsUnmapMemory(void* memory, ACPI_SIZE length)
{
    //printf("AcpiOsUnmapMemory(%p, 0x%x)\n", memory, (int)length);

    vmm_unmap(memory, length);
}


/*
ACPI_STATUS AcpiOsGetPhysicalAddress(void* LogicalAddress, ACPI_PHYSICAL_ADDRESS* PhysicalAddress)
{
    PHYSICAL_ADDRESS PhysAddr;

    if (!LogicalAddress || !PhysicalAddress)
    {
        printf("Bad parameter\n");
        return AE_BAD_PARAMETER;
    }

    PhysAddr = MmGetPhysicalAddress(LogicalAddress);

    *PhysicalAddress = (ACPI_PHYSICAL_ADDRESS)PhysAddr.QuadPart;

    return AE_OK;
}
*/


void* AcpiOsAllocate(ACPI_SIZE size)
{
    //printf("AcpiOsAllocate(%d)\n", (int)size);

    void* memory = malloc(size);

    assert(memory != NULL);

    return memory;
}



void AcpiOsFree(void* memory)
{
    //printf("AcpiOsFree(%p)\n", memory);

    assert(memory != NULL);

    free(memory);
}



/*
BOOLEAN AcpiOsReadable(void* Memory, ACPI_SIZE Length)
{
    BOOLEAN Ret = FALSE;

    _SEH2_TRY
    {
        ProbeForRead(Memory, Length, sizeof(UCHAR));

        Ret = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}



BOOLEAN AcpiOsWritable(void* Memory, ACPI_SIZE Length)
{
    BOOLEAN Ret = FALSE;

    _SEH2_TRY
    {
        ProbeForWrite(Memory, Length, sizeof(UCHAR));

        Ret = TRUE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE;
    }
    _SEH2_END;

    return Ret;
}
*/



ACPI_THREAD_ID AcpiOsGetThreadId()
{
    // Thread ID must be non-zero
    return (uintptr_t)thread_current();
}



ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK function, void* context)
{
    printf("AcpiOsExecute()\n");

    (void) type;
    (void) function;
    (void) context;

    return AE_ERROR;

/*TODO
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;


    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  (PKSTART_ROUTINE)Function,
                                  Context);
    if (!NT_SUCCESS(Status))
        return AE_ERROR;

    ZwClose(ThreadHandle);

    return AE_OK;
*/
}



void AcpiOsSleep(UINT64 milliseconds)
{
    printf("AcpiOsSleep(%lu)\n", (unsigned long) milliseconds);
/* TODO
    KeStallExecutionProcessor(milliseconds*1000);
*/
}



void AcpiOsStall(UINT32 microseconds)
{
    printf("AcpiOsStall(%d)\n",microseconds);
/* TODO
    KeStallExecutionProcessor(microseconds);
*/
}


ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX* handle)
{
    //printf("AcpiOsCreateMutex()\n");

    if (!handle)
        return AE_BAD_PARAMETER;

    mutex_t* mutex = malloc(sizeof(mutex_t));

    if (!mutex)
        return AE_NO_MEMORY;

    mutex_init(mutex);

    *handle = mutex;

    return AE_OK;
}



void AcpiOsDeleteMutex(ACPI_MUTEX handle)
{
    //printf("AcpiOsDeleteMutex()\n");

    free(handle);
}



ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX handle, UINT16 timeout)
{
    //printf("AcpiOsAcquireMutex(%d)\n", timeout);

    if (timeout == ACPI_DO_NOT_WAIT)
    {
        if (mutex_try_lock(handle))
            return AE_OK;
        else
            return AE_TIME;
    }
    else if (timeout == ACPI_WAIT_FOREVER)
    {
        mutex_lock(handle);
        return AE_OK;
    }
    else
    {
        assert(0);
    }
}



void AcpiOsReleaseMutex(ACPI_MUTEX handle)
{
    //printf("AcpiOsReleaseMutex()\n");

    mutex_unlock(handle);
}



ACPI_STATUS AcpiOsCreateSemaphore(UINT32 maxCount, UINT32 initialCount, ACPI_SEMAPHORE* handle)
{
    (void) maxCount;

    //printf("AcpiOsCreateSemaphore()\n");

    if (!handle)
        return AE_BAD_PARAMETER;

    semaphore_t* semaphore = malloc(sizeof(semaphore_t));

    if (!semaphore)
        return AE_NO_MEMORY;

    semaphore_init(semaphore, initialCount);

    *handle = semaphore;

    return AE_OK;
}



ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle)
{
    //printf("AcpiOsDeleteSemaphore()\n");

    free(handle);

    return AE_OK;
}



ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 count, UINT16 timeout)
{
    //printf("AcpiOsWaitSemaphore()\n");

    assert(timeout == ACPI_WAIT_FOREVER);

    for (uint32_t i = 0; i != count; ++i)
    {
        semaphore_lock(handle);
    }

/* TODO
    KeAcquireSpinLock(&Sem->Lock, &OldIrql);

    // Make sure we can wait if we have fewer units than we need
    if ((Timeout == ACPI_DO_NOT_WAIT) && (Sem->CurrentUnits < Units))
    {
        // We can't so we must bail now
        KeReleaseSpinLock(&Sem->Lock, OldIrql);
        return AE_TIME;
    }

    // Time to block until we get enough units
    while (Sem->CurrentUnits < Units)
    {
        KeReleaseSpinLock(&Sem->Lock, OldIrql);
        KeWaitForSingleObject(&Sem->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        KeAcquireSpinLock(&Sem->Lock, &OldIrql);
    }
*/
    return AE_OK;
}



ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 count)
{
    //printf("AcpiOsSignalSemaphore()\n");

    for (uint32_t i = 0; i != count; ++i)
    {
        semaphore_unlock(handle);
    }

    return AE_OK;
}



ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* handle)
{
    //printf("AcpiOsCreateLock()\n");

    if (!handle)
        return AE_BAD_PARAMETER;

    spinlock_t* spinlock = malloc(sizeof(spinlock_t));

    if (!spinlock)
        return AE_NO_MEMORY;

    *spinlock = SPINLOCK_UNLOCKED;

    *handle = spinlock;

    return AE_OK;
}



void AcpiOsDeleteLock(ACPI_SPINLOCK handle)
{
    //printf("AcpiOsDeleteLock()\n");

    free(handle);
}



ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle)
{
    //printf("AcpiOsAcquireLock()\n");

    spin_lock(handle);

    return 0;
}



void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags)
{
    (void) flags;

    //printf("AcpiOsReleaseLock()\n");

    spin_unlock(handle);
}






static ACPI_OSD_HANDLER AcpiInterruptHandler;
static void* AcpiInterruptContext;



static int acpi_interrupt_handler(interrupt_context_t* context)
{
    (void) context;

    INT32 status = (*AcpiInterruptHandler)(AcpiInterruptContext);

    if (status == ACPI_INTERRUPT_HANDLED)
        return TRUE;
    else
        return FALSE;
}




UINT32 AcpiOsInstallInterruptHandler(UINT32 interruptNumber, ACPI_OSD_HANDLER serviceRoutine, void* context)
{
    if (interruptNumber > 255)
        return AE_BAD_PARAMETER;

    if (!serviceRoutine)
        return AE_BAD_PARAMETER;

    if (AcpiInterruptHandler)
        return AE_ALREADY_EXISTS;

    AcpiInterruptHandler = serviceRoutine;
    AcpiInterruptContext = context;

    if (interrupt_register(interruptNumber, acpi_interrupt_handler) == 0)
    {
        AcpiInterruptHandler = NULL;
        AcpiInterruptContext = NULL;
        return AE_ALREADY_EXISTS;
    }

    printf("ACPI using interrupt %d\n", interruptNumber);

    return AE_OK;
}



ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 interruptNumber, ACPI_OSD_HANDLER serviceRoutine)
{
    printf("AcpiOsRemoveInterruptHandler(%d, %p)\n", interruptNumber, serviceRoutine);

    if (!AcpiInterruptHandler)
        return AE_NOT_EXIST;

    AcpiInterruptHandler = NULL;
    AcpiInterruptContext = NULL;

    return AE_OK;
}



ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address, UINT64* value, UINT32 width)
{
    printf("AcpiOsReadMemory()\n");

    (void) address;
    (void) value;
    (void) width;
    return AE_BAD_PARAMETER;


/*
    printf("AcpiOsReadMemory(%p, %d)\n", (void*)address, width);

    switch (width)
    {
    case 8:
        *value = *(uint8_t*)address;
        break;
    case 16:
        *value = *(uint16_t*)address;
        break;
    case 32:
        *value = *(uint32_t*)address;
        break;
    case 64:
        *value = *(uint64_t*)address;
        break;
    default:
        printf("AcpiOsReadMemory got bad width: %d\n", width);
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
*/
}



ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 value, UINT32 width)
{
    printf("AcpiOsWriteMemory()\n");

    (void) address;
    (void) value;
    (void) width;

    return AE_BAD_PARAMETER;
/*
    printf("AcpiOsWriteMemory(%p, %d)\n", (void*)address, width);

    switch (width)
    {
    case 8:
        *(uint8_t*)address = value;
        break;
    case 16:
        *(uint16_t*)address = value;
        break;
    case 32:
        *(uint32_t*)address = value;
        break;
    case 64:
        *(uint64_t*)address = value;
        break;
    default:
        printf("AcpiOsWriteMemory got bad width: %d\n", width);
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
*/
}



ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS address, UINT32* value, UINT32 width)
{
    //printf("AcpiOsReadPort %x, width %d\n", (unsigned) address, width);

    switch (width)
    {
    case 8:
        *value = io_in_8(address);
        break;

    case 16:
        *value = io_in_16(address);
        break;

    case 32:
        *value = io_in_32(address);
        break;
    default:
        printf("AcpiOsReadPort got bad width: %d\n", width);
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}



ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS address, UINT32 value, UINT32 width)
{
    //printf("AcpiOsWritePort %x, width %d\n", (unsigned) address, width);
    switch (width)
    {
    case 8:
        io_out_8(address, value);
        break;

    case 16:
        io_out_16(address, value);
        break;

    case 32:
        io_out_32(address, value);
        break;

    default:
        printf("AcpiOsWritePort got bad width: %d\n", width);
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}



ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64* result, UINT32 width)
{
    //printf("AcpiOsReadPciConfiguration(%p, %d, %p, %d)\n", pciId, reg, value, width);

    switch (width)
    {
    case 8:
        *result = pci_read_config_8(pciId->Bus, pciId->Device, pciId->Function, reg);
        break;

    case 16:
        *result = pci_read_config_16(pciId->Bus, pciId->Device, pciId->Function, reg);
        break;

    case 32:
        *result = pci_read_config_32(pciId->Bus, pciId->Device, pciId->Function, reg);
        break;

    default:
        assert(0);
        return AE_BAD_PARAMETER;
    }

    return AE_OK;
}



ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64 value, UINT32 width)
{
    printf("AcpiOsWritePciConfiguration(%p, %d, %d, %d)\n", pciId, reg, (int)value, width);

    return AE_OK;

/*TODO
    ULONG buf = Value;
    PCI_SLOT_NUMBER slot;

    slot.u.AsULONG = 0;
    slot.u.bits.DeviceNumber = PciId->Device;
    slot.u.bits.FunctionNumber = PciId->Function;

    printf("AcpiOsWritePciConfiguration, slot=0x%x\n", slot.u.AsULONG);
    if (!OslIsPciDevicePresent(PciId->Bus, slot.u.AsULONG))
        return AE_NOT_FOUND;

    // Width is in BITS
    HalSetBusDataByOffset(PCIConfiguration,
        PciId->Bus,
        slot.u.AsULONG,
        &buf,
        Reg,
        (Width >> 3));

    return AE_OK;
*/
}



void AcpiOsPrintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}



void AcpiOsVprintf(const char* format, va_list args)
{
    vprintf(format, args);
}



/*
void AcpiOsRedirectOutput(void* Destination)
{
    printf("Output redirection not supported\n");
}
*/


UINT64 AcpiOsGetTimer()
{
    printf("AcpiOsGetTimer()()\n");

    return 0;

/* TODO
    LARGE_INTEGER CurrentTime;

    KeQuerySystemTime(&CurrentTime);

    return CurrentTime.QuadPart;
*/
}



ACPI_STATUS AcpiOsSignal(UINT32 function, void* info)
{
    printf("AcpiOsSignal(%d, %p)\n", function, info);


/*TODO
    ACPI_SIGNAL_FATAL_INFO *FatalInfo = Info;

    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:
        if (Info)
            printf ("AcpiOsBreakpoint: %d %d %d ****\n", FatalInfo->Type, FatalInfo->Code, FatalInfo->Argument);
        else
            printf ("AcpiOsBreakpoint ****\n");
        break;
    case ACPI_SIGNAL_BREAKPOINT:
        if (Info)
            printf ("AcpiOsBreakpoint: %s ****\n", Info);
        else
            printf ("AcpiOsBreakpoint ****\n");
        break;
    }

    ASSERT(FALSE);
*/
    return AE_OK;
}



/*
ACPI_STATUS AcpiOsGetLine(char* Buffer, UINT32 BufferLength, UINT32* BytesRead)
{
    printf("File reading not supported\n");
    return AE_ERROR;
}
*/



void AcpiOsWaitEventsComplete()
{
    printf("AcpiOsWaitEventsComplete()\n");
}



ACPI_STATUS AcpiOsPhysicalTableOverride (ACPI_TABLE_HEADER* existingTable, ACPI_PHYSICAL_ADDRESS* newAddress, UINT32* newTableLength)
{
    (void) existingTable;

    // No override
    *newAddress = 0;
    *newTableLength = 0;

    return AE_OK;
}
