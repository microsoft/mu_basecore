/** @file
  BaseCryptDefs.h

  This file contains the definitions and constants used in the shared cryptographic library that
  are shared across different headers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BASE_CRYPT_DEFS_H_
#define BASE_CRYPT_DEFS_H_

#include <Uefi.h>

#define CRYPTO_NID_NULL  0x0000

// Hash
#define CRYPTO_NID_SHA256  0x0001
#define CRYPTO_NID_SHA384  0x0002
#define CRYPTO_NID_SHA512  0x0003

// Key Exchange
#define CRYPTO_NID_SECP256R1  0x0204
#define CRYPTO_NID_SECP384R1  0x0205
#define CRYPTO_NID_SECP521R1  0x0206

// Symmetric ciphers usable with Pkcs7Encrypt
#define CRYPTO_NID_AES128CBC  0x01A3
#define CRYPTO_NID_AES192CBC  0x01A7
#define CRYPTO_NID_AES256CBC  0x01AB

// Flags usable with Pkcs7Encrypt.
#define CRYPTO_PKCS7_DEFAULT  0x0 // Treat the input as binary data.

///
/// MD5 digest size in bytes
///
#define MD5_DIGEST_SIZE  16

///
/// SHA-1 digest size in bytes.
///
#define SHA1_DIGEST_SIZE  20

///
/// SHA-256 digest size in bytes
///
#define SHA256_DIGEST_SIZE  32

///
/// SHA-384 digest size in bytes
///
#define SHA384_DIGEST_SIZE  48

///
/// SHA-512 digest size in bytes
///
#define SHA512_DIGEST_SIZE  64

///
/// SM3 digest size in bytes
///
#define SM3_256_DIGEST_SIZE  32

///
/// TDES block size in bytes
///
#define TDES_BLOCK_SIZE  8

///
/// AES block size in bytes
///
#define AES_BLOCK_SIZE  16

///
/// RSA Key Tags Definition used in RsaSetKey() function for key component identification.
///
typedef enum {
  RsaKeyN,      ///< RSA public Modulus (N)
  RsaKeyE,      ///< RSA Public exponent (e)
  RsaKeyD,      ///< RSA Private exponent (d)
  RsaKeyP,      ///< RSA secret prime factor of Modulus (p)
  RsaKeyQ,      ///< RSA secret prime factor of Modules (q)
  RsaKeyDp,     ///< p's CRT exponent (== d mod (p - 1))
  RsaKeyDq,     ///< q's CRT exponent (== d mod (q - 1))
  RsaKeyQInv    ///< The CRT coefficient (== 1/q mod p)
} RSA_KEY_TAG;

/**
  The 3rd parameter of Pkcs7GetSigners will return all embedded
  X.509 certificate in one given PKCS7 signature. The format is:
  //
  // UINT8  CertNumber;
  // UINT32 Cert1Length;
  // UINT8  Cert1[];
  // UINT32 Cert2Length;
  // UINT8  Cert2[];
  // ...
  // UINT32 CertnLength;
  // UINT8  Certn[];
  //

  The two following C-structure are used for parsing CertStack more clearly.
**/
#pragma pack(1)

typedef struct {
  UINT32    CertDataLength;       // The length in bytes of X.509 certificate.
  UINT8     CertDataBuffer[0];    // The X.509 certificate content (DER).
} EFI_CERT_DATA;

typedef struct {
  UINT8    CertNumber;            // Number of X.509 certificate.
  // EFI_CERT_DATA   CertArray[];  // An array of X.509 certificate.
} EFI_CERT_STACK;

#pragma pack()

#endif // BASE_CRYPT_DEFS_H_
