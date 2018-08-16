/** @file -- CapsulePersistLib.h
A public library interface for persisting Capsules across reset.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
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
  OUT EFI_CAPSULE_HEADER  *CapsuleArray,
  OUT UINTN               *CapsuleArraySize
  );

#endif // _CAPSULE_PERSIST_LIB_H_
