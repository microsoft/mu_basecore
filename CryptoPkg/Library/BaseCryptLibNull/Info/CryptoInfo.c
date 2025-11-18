/** @file
  Cryptographic Library Information Implementation.
  This module provides version information for the underlying Crypto Provider.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "InternalCryptLib.h"

/**
  Gets the cryptographic provider version information.

  This function returns the version string of the cryptographic provider
  (e.g., OpenSSL, MbedTLS, SymCrypt) that was used to compile the library.

  @param[out]     Buffer       Pointer to the buffer to receive the version string.
                               If NULL, the required buffer size is returned in BufferSize.
  @param[in,out]  BufferSize   On input, the size of the buffer in bytes.
                               On output, the size of the data copied to the buffer (including null terminator).
                               If Buffer is NULL, returns the required buffer size.

  @retval  EFI_SUCCESS            The version string was successfully copied to the buffer.
  @retval  EFI_BUFFER_TOO_SMALL   The buffer is too small. BufferSize contains the required size.
  @retval  EFI_INVALID_PARAMETER  BufferSize is NULL.
  @retval  EFI_UNSUPPORTED        The function is not provided by the Crypto provider.
**/
EFI_STATUS
EFIAPI
GetCryptoProviderVersionString (
  OUT    CHAR8  *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  return EFI_UNSUPPORTED;
}
