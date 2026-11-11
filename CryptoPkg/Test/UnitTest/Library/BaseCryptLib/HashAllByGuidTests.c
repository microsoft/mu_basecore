/** @file
  Tests for generic hash-algorithm GUID dispatch.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Protocol/Hash.h>
#include <Guid/ImageAuthentication.h>

STATIC
UNIT_TEST_STATUS
EFIAPI
TestHashAllByGuidAlgorithms (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST struct {
    EFI_GUID    Guid;
    UINTN       DigestSize;
    BOOLEAN (EFIAPI *HashAll)(IN CONST VOID *Data, IN UINTN DataSize, OUT UINT8 *HashValue);
  } Algorithms[] = {
    { EFI_HASH_ALGORITHM_SHA1_GUID,   SHA1_DIGEST_SIZE,   Sha1HashAll   },
    { EFI_HASH_ALGORITHM_SHA256_GUID, SHA256_DIGEST_SIZE, Sha256HashAll },
    { EFI_HASH_ALGORITHM_SHA384_GUID, SHA384_DIGEST_SIZE, Sha384HashAll },
    { EFI_HASH_ALGORITHM_SHA512_GUID, SHA512_DIGEST_SIZE, Sha512HashAll }
  };
  STATIC CONST UINT8  Data[] = { 'a', 'b', 'c' };
  UINT8               Expected[SHA512_DIGEST_SIZE];
  UINT8               Digest[SHA512_DIGEST_SIZE];
  UINTN               DigestSize;
  UINTN               Index;
  UINTN               DataSize;

  for (Index = 0; Index < ARRAY_SIZE (Algorithms); Index++) {
    for (DataSize = 0; DataSize <= sizeof (Data); DataSize += sizeof (Data)) {
      UT_ASSERT_TRUE (Algorithms[Index].HashAll (Data, DataSize, Expected));
      SetMem (Digest, sizeof (Digest), 0xA5);
      DigestSize = 0;
      UT_ASSERT_STATUS_EQUAL (
        HashAllByGuid (&Algorithms[Index].Guid, Data, DataSize, Digest, &DigestSize),
        EFI_SUCCESS
        );
      UT_ASSERT_EQUAL (DigestSize, Algorithms[Index].DigestSize);
      UT_ASSERT_MEM_EQUAL (Digest, Expected, DigestSize);
      if (DigestSize < sizeof (Digest)) {
        UT_ASSERT_EQUAL (Digest[DigestSize], 0xA5);
      }
    }
  }

  return UNIT_TEST_PASSED;
}

STATIC
UNIT_TEST_STATUS
EFIAPI
TestHashAllByGuidInvalidInputs (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  STATIC CONST EFI_GUID  HashGuid        = EFI_HASH_ALGORITHM_SHA256_GUID;
  STATIC CONST EFI_GUID  CertificateGuid = EFI_CERT_SHA256_GUID;
  STATIC CONST EFI_GUID  UnknownGuid     = { 0 };
  STATIC CONST UINT8     Data[]          = { 'a', 'b', 'c' };
  UINT8                  Digest[SHA512_DIGEST_SIZE];
  UINTN                  DigestSize;

  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (NULL, Data, sizeof (Data), Digest, &DigestSize), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&HashGuid, NULL, sizeof (Data), Digest, &DigestSize), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&HashGuid, Data, sizeof (Data), NULL, &DigestSize), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&HashGuid, Data, sizeof (Data), Digest, NULL), EFI_INVALID_PARAMETER);
  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&UnknownGuid, Data, sizeof (Data), Digest, &DigestSize), EFI_UNSUPPORTED);
  UT_ASSERT_STATUS_EQUAL (HashAllByGuid (&CertificateGuid, Data, sizeof (Data), Digest, &DigestSize), EFI_UNSUPPORTED);
  return UNIT_TEST_PASSED;
}

TEST_DESC  mHashAllByGuidTest[] = {
  { "Generic GUIDs select the expected digest", "CryptoPkg.BaseCryptLib.HashAllByGuid", TestHashAllByGuidAlgorithms,    NULL, NULL, NULL },
  { "Invalid inputs and non-hash GUIDs fail",   "CryptoPkg.BaseCryptLib.HashAllByGuid", TestHashAllByGuidInvalidInputs, NULL, NULL, NULL }
};

UINTN  mHashAllByGuidTestNum = ARRAY_SIZE (mHashAllByGuidTest);
