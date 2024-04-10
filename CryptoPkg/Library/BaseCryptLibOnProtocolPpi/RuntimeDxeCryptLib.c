/** @file
  Implements the GetCryptoServices() API that makes services from the EDK II
  Crypto Protocol available to Runtime DXE drivers.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Guid/EventGroup.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/Crypto.h>

STATIC  EDKII_CRYPTO_PROTOCOL  *mCryptoProtocol           = NULL;
STATIC  EFI_EVENT              mVirtualAddressChangeEvent = NULL;

/**
  Internal worker function that returns the pointer to an EDK II Crypto
  Protocol/PPI. The layout of the PPI, DXE Protocol, and SMM Protocol are
  identical which allows the implementation of the BaseCryptLib functions that
  call through a Protocol/PPI to be shared for the PEI, DXE, and SMM
  implementations.

  This DXE implementation returns the pointer to the EDK II Crypto Protocol
  that was found in the library constructor DxeCryptLibConstructor().

  @return A pointer to the EDK II Crypto Protocol found in the constructor.
**/
VOID *
GetCryptoServices (
  VOID
  )
{
  return (VOID *)mCryptoProtocol;
}

/**
  Library instance EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE notification function.

  Converts global pointers to their new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.
**/
VOID
EFIAPI
DxeCryptLibAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (mCryptoProtocol != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mCryptoProtocol);
  }
}

/**
  Locate the EDK II Crypto Protocol.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.
  @retval EFI_NOT_FOUND     An instance of the EDK II Crypto protocol was not found.
  @retval EFI_ABORTED       Failed to register for the address change event.
  @retval EFI_UNSUPPORTED   The EDK II Crypto protocol version is less than the expected version.
**/
EFI_STATUS
EFIAPI
RuntimeDxeCryptLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Version;

  Status = gBS->LocateProtocol (
                  &gEdkiiCryptoProtocolGuid,
                  NULL,
                  (VOID **)&mCryptoProtocol
                  );

  if (EFI_ERROR (Status) || (mCryptoProtocol == NULL)) {
    DEBUG ((DEBUG_ERROR, "[%a] Failed to locate Crypto Protocol. Status = %r.\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
    ASSERT (mCryptoProtocol != NULL);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DxeCryptLibAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  if (EFI_ERROR (Status)) {
    // If virtual address change registration fails, the pointer will be invalid at runtime.
    // Consider the pointer invalid in that case by setting it to null.
    DEBUG ((DEBUG_ERROR, "[%a] Failed to register for address change notification.\n", __func__));
    ASSERT_EFI_ERROR (Status);
    mCryptoProtocol = NULL;
    return EFI_ABORTED;
  }

  Version = mCryptoProtocol->GetVersion ();
  if (Version < EDKII_CRYPTO_VERSION) {
    DEBUG ((DEBUG_ERROR, "[%a] Crypto Protocol unsupported version %u.\n", __func__, Version));
    ASSERT (Version >= EDKII_CRYPTO_VERSION);
    mCryptoProtocol = NULL;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
