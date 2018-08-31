/** @file -- HeapGuardTestApp.c

This protocol provides debug access to Heap Guard memory protections to allow
validation of the memory protections.

Copyright (c) 2017, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef __HEAP_GUARD_DEBUG_PROTOCOL__
#define __HEAP_GUARD_DEBUG_PROTOCOL__

#define HEAP_GUARD_DEBUG_PROTOCOL_GUID \
  { \
    0xe8150630, 0x6366, 0x4294, { 0xa3, 0x47, 0x89, 0x2c, 0x3a, 0x7d, 0xe4, 0xaf } \
  }

typedef
BOOLEAN
(EFIAPI *IS_GUARD_PAGE)(
  EFI_PHYSICAL_ADDRESS    Address
  );

typedef struct {
  IS_GUARD_PAGE IsGuardPage;
} _HEAP_GUARD_DEBUG_PROTOCOL;

typedef _HEAP_GUARD_DEBUG_PROTOCOL HEAP_GUARD_DEBUG_PROTOCOL;

extern EFI_GUID gHeapGuardDebugProtocolGuid;

#endif
