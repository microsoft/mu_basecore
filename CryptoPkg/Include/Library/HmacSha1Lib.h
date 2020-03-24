/** @file
  Defines base cryptographic library APIs for HmacSha1.
  The Base Cryptographic Library provides implementations of basic cryptography
  primitives (Hash Serials, HMAC, RSA, Diffie-Hellman, etc) for UEFI security
  functionality enabling.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

Largely copied from BaseCryptLib from release/202002

**/

#ifndef __HMAC_SHA_1_LIB_H__
#define __HMAC_SHA_1_LIB_H__

#include <Uefi/UefiBaseType.h>

/**
  Set user-supplied key for subsequent use. It must be done before any
  calling to HmacSha1Update().

  If HmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha1Context    Pointer to HMAC-Sha1 context.
  @param[in]   Key                Pointer to the user-supplied key.
  @param[in]   KeySize            Key size in bytes.

  @retval TRUE   The Key is set successfully.
  @retval FALSE  The Key is set unsuccessfully.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1SetKey (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

/**
  Allocates and initializes one HMAC_CTX context for subsequent HMAC-SHA1 use.

  If this interface is not supported, then return NULL.

  @return  Pointer to the HMAC_CTX context that has been initialized.
           If the allocations fails, HmacSha1New() returns NULL.
  @return  NULL   This interface is not supported.

**/
VOID *
EFIAPI
HmacSha1New (
  VOID
  );

/**
  Release the specified HMAC_CTX context.

  If this interface is not supported, then do nothing.

  @param[in]  HmacSha1Ctx  Pointer to the HMAC_CTX context to be released.

**/
VOID
EFIAPI
HmacSha1Free (
  IN  VOID  *HmacSha1Ctx
  );

/**
  Initializes user-supplied memory pointed by HmacSha1Context as HMAC-SHA1 context for
  subsequent use.

  If HmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HmacSha1Context  Pointer to HMAC-SHA1 context being initialized.
  @param[in]   Key              Pointer to the user-supplied key.
  @param[in]   KeySize          Key size in bytes.

  @retval TRUE   HMAC-SHA1 context initialization succeeded.
  @retval FALSE  HMAC-SHA1 context initialization failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Init (
  OUT  VOID         *HmacSha1Context,
  IN   CONST UINT8  *Key,
  IN   UINTN        KeySize
  );

/**
  Makes a copy of an existing HMAC-SHA1 context.

  If HmacSha1Context is NULL, then return FALSE.
  If NewHmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in]  HmacSha1Context     Pointer to HMAC-SHA1 context being copied.
  @param[out] NewHmacSha1Context  Pointer to new HMAC-SHA1 context.

  @retval TRUE   HMAC-SHA1 context copy succeeded.
  @retval FALSE  HMAC-SHA1 context copy failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Duplicate (
  IN   CONST VOID  *HmacSha1Context,
  OUT  VOID        *NewHmacSha1Context
  );

/**
  Digests the input data and updates HMAC-SHA1 context.

  This function performs HMAC-SHA1 digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should not
  be finalized by HmacSha1Final(). Behavior with invalid context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha1Context Pointer to the HMAC-SHA1 context.
  @param[in]       Data            Pointer to the buffer containing the data to be digested.
  @param[in]       DataSize        Size of Data buffer in bytes.

  @retval TRUE   HMAC-SHA1 data digest succeeded.
  @retval FALSE  HMAC-SHA1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Update (
  IN OUT  VOID        *HmacSha1Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the HMAC-SHA1 digest value.

  This function completes HMAC-SHA1 hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the HMAC-SHA1 context cannot
  be used again.
  HMAC-SHA1 context should be already correctly initialized by HmacSha1Init(), and should
  not be finalized by HmacSha1Final(). Behavior with invalid HMAC-SHA1 context is undefined.

  If HmacSha1Context is NULL, then return FALSE.
  If HmacValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HmacSha1Context  Pointer to the HMAC-SHA1 context.
  @param[out]      HmacValue        Pointer to a buffer that receives the HMAC-SHA1 digest
                                    value (20 bytes).

  @retval TRUE   HMAC-SHA1 digest computation succeeded.
  @retval FALSE  HMAC-SHA1 digest computation failed.
  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
HmacSha1Final (
  IN OUT  VOID   *HmacSha1Context,
  OUT     UINT8  *HmacValue
  );

#endif
