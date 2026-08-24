/** @file
  ECIT capability reporting unit tests -- CMS content-digest op.

  Exercises GetCryptoOpCapability(gCryptoOpCmsContentDigestGuid): the sizing
  probe / fetch round-trip, the EFI_BUFFER_TOO_SMALL contract, presence of a
  provider digest OID (SHA-256), and the dispatcher error paths (unknown op
  GUID, NULL BufferSize). The content-digest op is accept-all, so it reports
  exactly the linked provider's message-digest set.

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestBaseCryptLib.h"
#include <Library/BaseLib.h>
#include <Guid/CryptoOpId.h>

#define OID_SHA256  "2.16.840.1.101.3.4.2.1"

/**
  Sizing probe then fetch: the CMS content-digest op reports a non-empty,
  NUL-terminated CSV that includes the SHA-256 OID.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCmsContentDigestReport (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR8       *Buffer;

  Size   = 0;
  Buffer = NULL;

  //
  // Sizing probe (Buffer == NULL): required size includes the trailing NUL,
  // so a populated set is > 1 byte.
  //
  Status = GetCryptoOpCapability (&gCryptoOpCmsContentDigestGuid, NULL, &Size);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_TRUE (Size > 1);

  Buffer = AllocatePool (Size);
  UT_ASSERT_NOT_NULL (Buffer);

  //
  // Exact-fit fetch.
  //
  Status = GetCryptoOpCapability (&gCryptoOpCmsContentDigestGuid, Buffer, &Size);
  UT_ASSERT_NOT_EFI_ERROR (Status);
  UT_ASSERT_EQUAL (Buffer[Size - 1], '\0');

  //
  // Any conformant OpenSSL build publishes SHA-256, so it must appear.
  //
  UT_ASSERT_NOT_NULL (AsciiStrStr (Buffer, OID_SHA256));

  FreePool (Buffer);
  return UNIT_TEST_PASSED;
}

/**
  A one-byte buffer cannot hold the payload: expect EFI_BUFFER_TOO_SMALL
  with the required size reported back.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestCmsContentDigestBufferTooSmall (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR8       Tiny[1];

  Size   = sizeof (Tiny);
  Status = GetCryptoOpCapability (&gCryptoOpCmsContentDigestGuid, Tiny, &Size);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BUFFER_TOO_SMALL);
  UT_ASSERT_TRUE (Size > sizeof (Tiny));

  return UNIT_TEST_PASSED;
}

/**
  An unrecognized op GUID is rejected with EFI_NOT_FOUND.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestGetCryptoOpCapabilityUnknownOp (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS      Status;
  UINTN           Size;
  CONST EFI_GUID  Unknown = {
    0x00000000, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa }
  };

  Size   = 0;
  Status = GetCryptoOpCapability (&Unknown, NULL, &Size);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);

  return UNIT_TEST_PASSED;
}

/**
  A NULL BufferSize is rejected with EFI_INVALID_PARAMETER.
**/
STATIC
UNIT_TEST_STATUS
EFIAPI
TestGetCryptoOpCapabilityNullSize (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  Status = GetCryptoOpCapability (&gCryptoOpCmsContentDigestGuid, NULL, NULL);
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  return UNIT_TEST_PASSED;
}

TEST_DESC  mCryptoOpCapabilityTest[] = {
  //
  // Description                                Class                            Function                             PreReq  CleanUp  Context
  //
  { "CMS content-digest reports SHA-256",       "CryptoPkg.BaseCryptLib.OpCap",  TestCmsContentDigestReport,          NULL,   NULL,    NULL },
  { "CMS content-digest honours too-small",     "CryptoPkg.BaseCryptLib.OpCap",  TestCmsContentDigestBufferTooSmall,  NULL,   NULL,    NULL },
  { "GetCryptoOpCapability unknown op",         "CryptoPkg.BaseCryptLib.OpCap",  TestGetCryptoOpCapabilityUnknownOp,  NULL,   NULL,    NULL },
  { "GetCryptoOpCapability NULL size",          "CryptoPkg.BaseCryptLib.OpCap",  TestGetCryptoOpCapabilityNullSize,   NULL,   NULL,    NULL },
};

UINTN  mCryptoOpCapabilityTestNum = ARRAY_SIZE (mCryptoOpCapabilityTest);
