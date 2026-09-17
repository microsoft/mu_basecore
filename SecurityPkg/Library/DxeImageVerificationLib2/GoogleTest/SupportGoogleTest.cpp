/** @file
  Unit tests for the DxeImageVerificationLib support helpers.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Support.h"
}

TEST (BuildImageAuthorityTest, NullPayload_InvalidParameter) {
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, NULL, 4, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (BuildImageAuthorityTest, ZeroPayloadSize_InvalidParameter) {
  UINT8            Payload[4] = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, 0, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST (BuildImageAuthorityTest, NullAuthority_InvalidParameter) {
  UINT8  Payload[4] = { 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), NULL), EFI_INVALID_PARAMETER);
}

//
// A payload large enough to overflow the EFI_SIGNATURE_DATA header addition is rejected before any
// allocation or read of the payload.
//
TEST (BuildImageAuthorityTest, OverflowingPayloadSize_InvalidParameter) {
  UINT8            Sentinel  = 0;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, &Sentinel, MAX_UINTN, &Authority), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// With no owner, the SignatureOwner is zeroed and the payload follows it. Size is the payload plus
// a SignatureOwner GUID.
//
TEST (BuildImageAuthorityTest, NoOwner_ZeroesOwnerAndCopiesPayload) {
  UINT8            Payload[5] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
  EFI_GUID         ZeroGuid   = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  ASSERT_NE (Authority.Data, nullptr);
  EXPECT_EQ (Authority.Size, (UINTN)(sizeof (EFI_GUID) + sizeof (Payload)));
  EXPECT_TRUE (CompareGuid (&Authority.Data->SignatureOwner, &ZeroGuid));
  EXPECT_EQ (CompareMem (Authority.Data->SignatureData, Payload, sizeof (Payload)), 0);

  FreeImageAuthority (&Authority);
  EXPECT_EQ (Authority.Data, nullptr);
  EXPECT_EQ (Authority.Size, (UINTN)0);
}

//
// A supplied owner GUID is stored verbatim ahead of the payload.
//
TEST (BuildImageAuthorityTest, WithOwner_StoresOwnerAndPayload) {
  EFI_GUID         Owner = { 0x11223344, 0x5566, 0x7788, { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 }
  };
  UINT8            Payload[3] = { 0xDE, 0xAD, 0xBE };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (&Owner, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  ASSERT_NE (Authority.Data, nullptr);
  EXPECT_TRUE (CompareGuid (&Authority.Data->SignatureOwner, &Owner));
  EXPECT_EQ (CompareMem (Authority.Data->SignatureData, Payload, sizeof (Payload)), 0);

  FreeImageAuthority (&Authority);
}

//
// FreeImageAuthority tolerates a NULL pointer and an already-empty authority.
//
TEST (FreeImageAuthorityTest, NullAndEmpty_NoOp) {
  IMAGE_AUTHORITY  Empty = { NULL, 0 };

  FreeImageAuthority (NULL);
  FreeImageAuthority (&Empty);
  EXPECT_EQ (Empty.Data, nullptr);
  EXPECT_EQ (Empty.Size, (UINTN)0);
}

//
// FreeImageAuthority clears the record it frees, so a second call is a safe no-op (no double free).
//
TEST (FreeImageAuthorityTest, DoubleFree_Safe) {
  UINT8            Payload[4] = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  ASSERT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority), EFI_SUCCESS);
  FreeImageAuthority (&Authority);
  FreeImageAuthority (&Authority);
  EXPECT_EQ (Authority.Data, nullptr);
}
