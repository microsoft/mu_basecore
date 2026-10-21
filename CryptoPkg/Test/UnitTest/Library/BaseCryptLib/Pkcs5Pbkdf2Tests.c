/** @file
  Application for PKCS#5 PBKDF2 Function Validation.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// MU_CHANGE [BEGIN]
// Common PBKDF2 input password and salt shared by all test vectors below.
// (From the RFC 6070 "password"/"salt" test inputs.)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Password = "password"; // Input Password
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        PassLen   = 8;          // Length of Input Password
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8  *Salt     = "salt";     // Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED UINTN        SaltLen   = 4;          // Length of Input Salt
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINTN  Count     = 2;          // Iteration Count
// MU_CHANGE [END]

// MU_CHANGE [BEGIN] Add SHA-384/512 support and parameterized test vectors
//
// Description of a single PKCS#5 PBKDF2 known-answer test vector.
//
typedef struct {
  UINTN          DigestSize;    // Digest size selecting the hash algorithm.
  UINTN          KeyLen;        // Length of the derived key in bytes.
  CONST UINT8    *DerivedKey;   // Expected derived key output.
} PKCS5_TEST_VECTOR;
// MU_CHANGE [END]

//
// PBKDF2 HMAC-SHA1 derived key ("password"/"salt", c=2). (From IETF RFC 6070)
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKeySha1[20] = {
  0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c, 0xcd, 0x1e, 0xd9, 0x2a, 0xce, 0x1d, 0x41, 0xf0,
  0xd8, 0xde, 0x89, 0x57
};

// MU_CHANGE [BEGIN] Add SHA-256/384/512 derived key vectors
//
// PBKDF2 HMAC-SHA256 derived key ("password"/"salt", c=2).
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKeySha256[32] = {
  0xae, 0x4d, 0x0c, 0x95, 0xaf, 0x6b, 0x46, 0xd3, 0x2d, 0x0a, 0xdf, 0xf9, 0x28, 0xf0, 0x6d, 0xd0,
  0x2a, 0x30, 0x3f, 0x8e, 0xf3, 0xc2, 0x51, 0xdf, 0xd6, 0xe2, 0xd8, 0x5a, 0x95, 0x47, 0x4c, 0x43
};

//
// PBKDF2 HMAC-SHA384 derived key ("password"/"salt", c=2).
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKeySha384[48] = {
  0x54, 0xf7, 0x75, 0xc6, 0xd7, 0x90, 0xf2, 0x19, 0x30, 0x45, 0x91, 0x62, 0xfc, 0x53, 0x5d, 0xbf,
  0x04, 0xa9, 0x39, 0x18, 0x51, 0x27, 0x01, 0x6a, 0x04, 0x17, 0x6a, 0x07, 0x30, 0xc6, 0xf1, 0xf4,
  0xfb, 0x48, 0x83, 0x2a, 0xd1, 0x26, 0x1b, 0xaa, 0xdd, 0x2c, 0xed, 0xd5, 0x08, 0x14, 0xb1, 0xc8
};

//
// PBKDF2 HMAC-SHA512 derived key ("password"/"salt", c=2).
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8  DerivedKeySha512[64] = {
  0xe1, 0xd9, 0xc1, 0x6a, 0xa6, 0x81, 0x70, 0x8a, 0x45, 0xf5, 0xc7, 0xc4, 0xe2, 0x15, 0xce, 0xb6,
  0x6e, 0x01, 0x1a, 0x2e, 0x9f, 0x00, 0x40, 0x71, 0x3f, 0x18, 0xae, 0xfd, 0xb8, 0x66, 0xd5, 0x3c,
  0xf7, 0x6c, 0xab, 0x28, 0x68, 0xa3, 0x9b, 0x9f, 0x78, 0x40, 0xed, 0xce, 0x4f, 0xef, 0x5a, 0x82,
  0xbe, 0x67, 0x33, 0x5c, 0x77, 0xa6, 0x06, 0x8e, 0x04, 0x11, 0x27, 0x54, 0xf2, 0x7c, 0xcf, 0x4e
};
// MU_CHANGE [END]

// MU_CHANGE [BEGIN] Refactor to parameterized test with multiple digest sizes
GLOBAL_REMOVE_IF_UNREFERENCED PKCS5_TEST_VECTOR  mPkcs5Sha1Vector = {
  SHA1_DIGEST_SIZE, sizeof (DerivedKeySha1), DerivedKeySha1
};

GLOBAL_REMOVE_IF_UNREFERENCED PKCS5_TEST_VECTOR  mPkcs5Sha256Vector = {
  SHA256_DIGEST_SIZE, sizeof (DerivedKeySha256), DerivedKeySha256
};

GLOBAL_REMOVE_IF_UNREFERENCED PKCS5_TEST_VECTOR  mPkcs5Sha384Vector = {
  SHA384_DIGEST_SIZE, sizeof (DerivedKeySha384), DerivedKeySha384
};

GLOBAL_REMOVE_IF_UNREFERENCED PKCS5_TEST_VECTOR  mPkcs5Sha512Vector = {
  SHA512_DIGEST_SIZE, sizeof (DerivedKeySha512), DerivedKeySha512
};

UNIT_TEST_STATUS
EFIAPI
TestVerifyPkcs5Pbkdf2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BOOLEAN                  Status;
  UINT8                    *OutKey;
  CONST PKCS5_TEST_VECTOR  *Vector;

  Vector = (CONST PKCS5_TEST_VECTOR *)Context;

  OutKey = AllocatePool (Vector->KeyLen);
  if (OutKey == NULL) {
    UT_LOG_ERROR ("Failed to allocate memory for OutKey.\n");
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  //
  // Verify PKCS#5 PBKDF2 Key Derivation Function
  //
  Status = Pkcs5HashPassword (
             PassLen,
             Password,
             SaltLen,
             (CONST UINT8 *)Salt,
             Count,
             Vector->DigestSize,
             Vector->KeyLen,
             OutKey
             );
  UT_ASSERT_TRUE (Status);

  //
  // Check the output key with the expected key result
  //
  UT_ASSERT_MEM_EQUAL (OutKey, Vector->DerivedKey, Vector->KeyLen);

  //
  // Release Resources
  //
  FreePool (OutKey);

  return EFI_SUCCESS;
}

TEST_DESC  mPkcs5Test[] = {
  //
  // -----Description------------------------------------Class----------------------Function-----------------Pre---Post--Context
  //
  { "TestVerifyPkcs5Pbkdf2(SHA1)",   "CryptoPkg.BaseCryptLib.Pkcs5", TestVerifyPkcs5Pbkdf2, NULL, NULL, &mPkcs5Sha1Vector   },
  { "TestVerifyPkcs5Pbkdf2(SHA256)", "CryptoPkg.BaseCryptLib.Pkcs5", TestVerifyPkcs5Pbkdf2, NULL, NULL, &mPkcs5Sha256Vector },
  { "TestVerifyPkcs5Pbkdf2(SHA384)", "CryptoPkg.BaseCryptLib.Pkcs5", TestVerifyPkcs5Pbkdf2, NULL, NULL, &mPkcs5Sha384Vector },
  { "TestVerifyPkcs5Pbkdf2(SHA512)", "CryptoPkg.BaseCryptLib.Pkcs5", TestVerifyPkcs5Pbkdf2, NULL, NULL, &mPkcs5Sha512Vector },
};

UINTN  mPkcs5TestNum = ARRAY_SIZE (mPkcs5Test);
// MU_CHANGE [END]
