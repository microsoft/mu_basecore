/** @file
  Authenticode Portable Executable Signature Verification which does not provide
  real capabilities.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalCryptLib.h"

/**
  Verifies the validity of a PE/COFF Authenticode Signature as described in "Windows
  Authenticode Portable Executable Signature Format".

  Return FALSE to indicate this interface is not supported.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TrustedCert  Pointer to a trusted/root certificate encoded in DER, which
                           is used for certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[in]  ImageHash    Pointer to the original image file hash value. The procedure
                           for calculating the image hash value is described in Authenticode
                           specification.
  @param[in]  HashSize     Size of Image hash value in bytes.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
AuthenticodeVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TrustedCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *ImageHash,
  IN  UINTN        HashSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}

/**
  Compute the PE/COFF Authenticode-style image hash of an image as described
  in "Windows Authenticode Portable Executable Signature Format".

  The caller selects the digest algorithm by HashType (e.g.
  gEfiCertSha256Guid, gEfiCertSha384Guid).

  If the Data buffer is too small to hold the contents of the digest,
  the error EFI_BUFFER_TOO_SMALL is returned and DigestSize is set to
  the required buffer size to obtain the data.

  @param[in]   FileBuffer  Pointer to the in-memory PE/COFF image.
  @param[in]   FileSize    Size of FileBuffer in bytes.
  @param[in]   HashType    Signature-type GUID identifying the hash
                           algorithm to use.
  @param[out]  Digest      Caller-provided buffer that receives the
                           computed digest. Must be at least
                           SHA512_DIGEST_SIZE bytes.
  @param[out]  DigestSize  On success, receives the digest length in
                           bytes.

  @retval EFI_SUCCESS            Digest was computed successfully.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_BUFFER_TOO_SMALL   DigestSize is too small for the
                                 requested hash algorithm.
  @retval EFI_UNSUPPORTED        HashType is not a recognized image
                                 hash algorithm, or this interface is
                                 not supported by the underlying
                                 library instance.
**/
EFI_STATUS
EFIAPI
GetAuthenticodeHash (
  IN  VOID            *FileBuffer,
  IN  UINTN           FileSize,
  IN  CONST EFI_GUID  *HashType,
  OUT UINT8           *Digest,
  OUT UINTN           *DigestSize
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
