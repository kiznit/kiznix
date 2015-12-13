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

#include "stdio.h"
#include "string.h"
#include <efi.h>
#include <efilib.h>


// _IPrint is an internal efilib internal print function
UINTN _IPrint(
    IN UINTN                            Column,
    IN UINTN                            Row,
    IN SIMPLE_TEXT_OUTPUT_INTERFACE     *Out,
    IN CHAR16                           *fmt,
    IN CHAR8                            *fmta,
    IN va_list                          args
);



int getchar()
{
    for (;;)
    {
        EFI_STATUS status;

        UINTN index;
        status = ST->BootServices->WaitForEvent(1, &ST->ConIn->WaitForKey, &index);
        if (EFI_ERROR(status))
        {
            return EOF;
        }

        EFI_INPUT_KEY key;
        status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key);
        if (EFI_ERROR(status))
        {
            if (status == EFI_NOT_READY)
                continue;

            return EOF;
        }

        return key.UnicodeChar;
    }
}



void* memcpy(void* destination, const void* source, size_t count)
{
    CopyMem(destination, source, count);
    return destination;
}



void* memset(void* memory, int value, size_t count)
{
    SetMem(memory, count, value);
    return memory;
}



int printf(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);

    return result;
}



int vprintf(const char* format, va_list args)
{
    return _IPrint ((UINTN) -1, (UINTN) -1, ST->ConOut, NULL, (CHAR8*)format, args);
}



int putchar(int c)
{
    CHAR16 s[2] = { c, 0 };
    return Print(s) > 0 ? s[0] : EOF;
}



int puts(const char* string)
{
    return printf("%a\n", string);
}
