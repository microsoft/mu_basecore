/** @file
  Implements the Traditional MM policy protocol, providing services to publish and
  access system policy.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

/**
  Common Entry of the MM policy service module.

  @retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
PolicyMmCommonEntry (
  VOID
  );

/**
  Entry to the Traditional MM policy service module.

  @param[in] ImageHandle                The image handle.
  @param[in] SystemTable                The system table.

  @retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
PolicyTraditionalMmEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return PolicyMmCommonEntry ();
}
