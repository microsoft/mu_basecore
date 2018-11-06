/** @file -- CapsulePersistLib.h
A public library interface for persisting Capsules across reset.

Copyright (c) 2018, Microsoft Corporation

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

#ifndef _CAPSULE_PERSIST_LIB_H_
#define _CAPSULE_PERSIST_LIB_H_

/**
  Persist a Capsule across reset.

  @param[in]        CapsuleHeader     EFI_CAPSULE_HEADER pointing to Capsule Image to persist.

  @retval     EFI_SUCCESS             Capsule was successfully persisted.
  @retval     EFI_DEVICE_ERROR        Something went wrong while trying to persist the capsule.

**/
EFI_STATUS
EFIAPI
PersistCapsule (
  IN EFI_CAPSULE_HEADER *CapsuleHeader
);

/**
  Returns a pointer to a buffer of capsules.

  If no persisted capsules present, CapsuleArray is not modified, and CapsuleArraySize will be set to zero.

  Removes the persistent capsules from whatever the medium of persistence is.
  Note: if return is something other than EFI_SUCESS or EFI_BUFFER_TOO_SMALL, removal of all persistent
  capsules from persistence is not guaranteed.


  @param[out]       CapsuleArray      Pointer to a buffer to hold the capsules.
  @param[out]       CapsuleArraySize  On input, size of CapsuleArray allocation. 
                                      On output, size of actual buffer of capsules.

  @retval       EFI_SUCCESS           Capsules were de-perisisted, and ouptut data is valid.
  @retval       EFI_BUFFER_TOO_SMALL  CapsuleArray buffer is too small to hold all the data.
  @retval       EFI_DEVICE_ERROR      Something went wrong while trying to retrive the capsule.

**/
EFI_STATUS
EFIAPI
GetPersistedCapsules (
  OUT EFI_CAPSULE_HEADER *CapsuleArray,
  OUT UINTN              *CapsuleArraySize
);

#endif // _CAPSULE_PERSIST_LIB_H_
