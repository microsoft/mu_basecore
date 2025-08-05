/** @file
  PKCS7 Encryption implementation over OpenSSL

  Copyright (c) 2023, Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
*/

#include "InternalCryptLib.h"

/**
  Creates a DER-encoded PKCS#7 ContentInfo containing an envelopedData structure
  that wraps content encrypted for secure transmission to one or more recipients.

  If this interface is not supported, return FALSE.

  @param[in]  X509Stack        Pointer to a stack of X.509 certificates for the
                               intended recipients of this message, created using
                               X509ConstructCertificateStack or similar. Each
                               certificate must provide an RSA public key. Any of the
                               corresponding private keys will be able to decrypt the
                               content of the returned ContentInfo.
  @param[in]  InData           Pointer to the content to be encrypted.
  @param[in]  InDataSize       Size of the content to be encrypted in bytes.
  @param[in]  CipherNid        NID of the symmetric cipher to use for encryption.
                               Supported values are CRYPTO_NID_AES128CBC,
                               CRYPTO_NID_AES192CBC, and CRYPTO_NID_AES256CBC.
  @param[in]  Flags            Flags for the encryption operation. Currently only
                               CRYPTO_PKCS7_DEFAULT is supported, which indicates that
                               the input data is treated as binary data.
  @param[out] ContentInfo      Receives a pointer to the output, which is a PKCS#7
                               DER-encoded ContentInfo that wraps an envelopedData. The
                               caller must free the returned buffer with FreePool().
  @param[out] ContentInfoSize  Receives the size of the output in bytes.

  @retval     TRUE             PKCS#7 data encryption succeeded.
  @retval     FALSE            PKCS#7 data encryption failed.
  @retval     FALSE            This interface is not supported.
**/
BOOLEAN
EFIAPI
Pkcs7Encrypt (
  IN   UINT8   *X509Stack,
  IN   UINT8   *InData,
  IN   UINTN   InDataSize,
  IN   UINT32  CipherNid,
  IN   UINT32  Flags,
  OUT  UINT8   **ContentInfo,
  OUT  UINTN   *ContentInfoSize
  )
{
  ASSERT (FALSE);
  return FALSE;
}
