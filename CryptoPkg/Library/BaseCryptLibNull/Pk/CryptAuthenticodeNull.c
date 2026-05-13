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

/**
  Determine the image-hash algorithm declared by a PE/COFF Authenticode
  signature.

  The signature's PKCS#7 SignedData header carries the digest algorithm
  used to hash the image. This function parses that header and returns
  the matching signature-type GUID (for example gEfiCertSha256Guid).
  The returned GUID can be passed directly to GetAuthenticodeHash to
  compute the corresponding image hash.

  @param[in]   AuthData      Pointer to the Authenticode signature retrieved
                             from a signed PE/COFF image.
  @param[in]   AuthDataSize  Size of AuthData in bytes.
  @param[out]  HashType      On success, receives the signature-type GUID
                             identifying the digest algorithm declared by
                             the signature.

  @retval EFI_SUCCESS            HashType has been populated.
  @retval EFI_INVALID_PARAMETER  AuthData or HashType is NULL, or
                                 AuthDataSize is zero.
  @retval EFI_BAD_BUFFER_SIZE    AuthData is too small or not encoded in a
                                 supported ASN.1 form.
  @retval EFI_UNSUPPORTED        The signature's digest algorithm is not a
                                 recognized image hash algorithm, or this
                                 interface is not supported by the
                                 underlying library instance.
**/
EFI_STATUS
EFIAPI
GetAuthenticodeHashAlgorithm (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        AuthDataSize,
  OUT EFI_GUID     *HashType
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Compute the digest of the TBSCertificate of an X.509 certificate.

  Return EFI_UNSUPPORTED to indicate this interface is not supported.

  @retval EFI_UNSUPPORTED  This interface is not supported.

**/
EFI_STATUS
EFIAPI
X509GetTbsCertHash (
  IN  VOID            *Cert,
  IN  UINTN           CertSize,
  IN  CONST EFI_GUID  *HashType,
  OUT UINT8           *Digest,
  OUT UINTN           *DigestSize
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Locate, in a PKCS#7 SignedData blob, the X.509 certificate whose
  TBSCertificate digest matches a caller-supplied hash.

  Return EFI_UNSUPPORTED to indicate this interface is not supported.

  @retval EFI_UNSUPPORTED  This interface is not supported.

**/
EFI_STATUS
EFIAPI
GetTrustAnchorX509FromAuthData (
  IN OUT VOID      **CacheHandle  OPTIONAL,
  IN  CONST UINT8  *TbsCertHash,
  IN  UINTN        TbsCertHashSize,
  IN  CONST UINT8  *AuthData,
  IN  UINTN        AuthDataSize,
  OUT UINT8        **TrustAnchorX509,
  OUT UINTN        *TrustAnchorX509Size
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Release a trust-anchor cache. No-op for the Null instance.

**/
VOID
EFIAPI
FreeTrustAnchorX509Cache (
  IN  VOID  *CacheHandle  OPTIONAL
  )
{
  ASSERT (FALSE);
}
