/** @file
  Library for encoding EFI Crypto Indicator Table capability payloads.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/BaseCryptLib.h>

/**
  Encode an unordered set of algorithm OIDs as ECIT EntryData.

  The returned payload contains a NUL-terminated ASCII CSV followed by zero
  padding to an 8-byte boundary. EntryDataSize includes the padding. Duplicate
  OIDs are omitted while preserving the first occurrence.

  On success, EntryData points to a newly allocated buffer that the caller must
  release with FreePool(). An empty set is encoded as one NUL byte followed by
  zero padding.

  On error, EntryData is set to NULL and EntryDataSize is set to zero.

  @param[in]  Capabilities    Array of algorithm OID capabilities.
  @param[in]  CapabilityCount Number of elements in Capabilities.
  @param[out] EntryData       Allocated, encoded ECIT payload.
  @param[out] EntryDataSize   Size of EntryData, including NUL and padding.

  @retval EFI_SUCCESS            The capability set was encoded.
  @retval EFI_INVALID_PARAMETER  An argument or OID is invalid.
  @retval EFI_OUT_OF_RESOURCES   The encoded payload could not be allocated or
                                 exceeds the ECIT entry-size limit.
**/
EFI_STATUS
EFIAPI
EcitEncodeOidSet (
  IN  CONST BASE_CRYPT_OP_CAPABILITY  *Capabilities OPTIONAL,
  IN  UINTN                           CapabilityCount,
  OUT VOID                            **EntryData,
  OUT UINTN                           *EntryDataSize
  );
