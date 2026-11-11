/** @file
  Unit tests for the DxeImageVerificationLib support helpers.

  Copyright (C) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockBaseCryptLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
  #include "../Support.h"
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class BuildImageAuthorityTest : public ::testing::Test {
protected:
  MockBaseCryptLib BaseCryptLibMock;
  SIGNATURE_VERIFICATION_INFO Verification = { };
};

TEST_F (BuildImageAuthorityTest, NullPayload_InvalidParameter) {
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, NULL, 4, &Authority, NULL, NULL, 0, &Verification), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST_F (BuildImageAuthorityTest, ZeroPayloadSize_InvalidParameter) {
  UINT8            Payload[4] = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, 0, &Authority, NULL, NULL, 0, &Verification), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST_F (BuildImageAuthorityTest, NullAuthority_InvalidParameter) {
  UINT8  Payload[4] = { 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), NULL, NULL, NULL, 0, &Verification), EFI_INVALID_PARAMETER);
}

//
// A payload large enough to overflow the EFI_SIGNATURE_DATA header addition is rejected before any
// allocation or read of the payload.
//
TEST_F (BuildImageAuthorityTest, OverflowingPayloadSize_InvalidParameter) {
  UINT8            Sentinel  = 0;
  IMAGE_AUTHORITY  Authority = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, &Sentinel, MAX_UINTN, &Authority, NULL, NULL, 0, &Verification), EFI_INVALID_PARAMETER);
  EXPECT_EQ (Authority.Data, nullptr);
}

//
// With no owner, the SignatureOwner is zeroed and the payload follows it. Size is the payload plus
// a SignatureOwner GUID.
//
TEST_F (BuildImageAuthorityTest, NoOwner_ZeroesOwnerAndCopiesPayload) {
  UINT8            Payload[5] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
  EFI_GUID         ZeroGuid   = { 0 };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority, NULL, NULL, 0, &Verification), EFI_SUCCESS);
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
TEST_F (BuildImageAuthorityTest, WithOwner_StoresOwnerAndPayload) {
  EFI_GUID         Owner = { 0x11223344, 0x5566, 0x7788, { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00 }
  };
  UINT8            Payload[3] = { 0xDE, 0xAD, 0xBE };
  IMAGE_AUTHORITY  Authority  = { NULL, 0 };

  EXPECT_EQ (BuildImageAuthority (&Owner, Payload, sizeof (Payload), &Authority, NULL, NULL, 0, &Verification), EFI_SUCCESS);
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
  MockBaseCryptLib             BaseCryptLibMock;
  SIGNATURE_VERIFICATION_INFO  Verification = { };
  UINT8                        Payload[4]   = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY              Authority    = { NULL, 0 };

  ASSERT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority, NULL, NULL, 0, &Verification), EFI_SUCCESS);
  FreeImageAuthority (&Authority);
  FreeImageAuthority (&Authority);
  EXPECT_EQ (Authority.Data, nullptr);
}

TEST_F (BuildImageAuthorityTest, FullCertificate_GeneratesSha256TbsThumbprint) {
  UINT8            Payload[4] = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY  Authority  = { };

  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (Payload, sizeof (Payload), _, _))
    .WillOnce (
       Invoke (
         [&] (CONST UINT8 *, UINTN, UINT8 **Tbs, UINTN *Size) -> BOOLEAN {
    *Tbs  = Payload + 1;
    *Size = sizeof (Payload) - 1;
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, Payload + 1, sizeof (Payload) - 1, _, _))
    .WillOnce (
       Invoke (
         [] (CONST EFI_GUID *Algorithm, CONST VOID *, UINTN, UINT8 *Digest, UINTN *Size) -> EFI_STATUS {
    EXPECT_TRUE (CompareGuid (Algorithm, &gEfiHashAlgorithmSha256Guid));
    SetMem (Digest, SHA256_DIGEST_SIZE, 0xA1);
    *Size = SHA256_DIGEST_SIZE;
    return EFI_SUCCESS;
  }
         )
       );

  ASSERT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority, NULL, NULL, 0, &Verification), EFI_SUCCESS);
  EXPECT_EQ (Verification.Status, (UINT32)EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY);
  EXPECT_TRUE (CompareGuid (&Verification.ThumbprintAlgorithm, &gEfiHashAlgorithmSha256Guid));
  EXPECT_EQ (Verification.ThumbprintSize, (UINTN)SHA256_DIGEST_SIZE);
  EXPECT_EQ (Verification.Thumbprint[0], 0xA1);
  FreeImageAuthority (&Authority);
}

TEST_F (BuildImageAuthorityTest, EnrolledTbsHash_CopiesWithoutRehashing) {
  UINT8  Payload[4] = { 1, 2, 3, 4 };
  UINT8  Thumbprint[SHA384_DIGEST_SIZE];

  SetMem (Thumbprint, sizeof (Thumbprint), 0xB2);
  IMAGE_AUTHORITY  Authority = { };

  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _)).Times (0);
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _)).Times (0);

  ASSERT_EQ (
    BuildImageAuthority (
      NULL,
      Payload,
      sizeof (Payload),
      &Authority,
      &gEfiHashAlgorithmSha384Guid,
      Thumbprint,
      sizeof (Thumbprint),
      &Verification
      ),
    EFI_SUCCESS
    );
  EXPECT_EQ (Verification.Status, (UINT32)EFI_SIGNATURE_VERIFICATION_TBS_HASH_AUTHORITY);
  EXPECT_TRUE (CompareGuid (&Verification.ThumbprintAlgorithm, &gEfiHashAlgorithmSha384Guid));
  EXPECT_EQ (Verification.ThumbprintSize, sizeof (Thumbprint));
  EXPECT_EQ (CompareMem (Verification.Thumbprint, Thumbprint, sizeof (Thumbprint)), 0);
  FreeImageAuthority (&Authority);
}

TEST_F (BuildImageAuthorityTest, ThumbprintHashFailure_PreservesMeasurementAuthority) {
  UINT8            Payload[4] = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY  Authority  = { };

  EXPECT_CALL (BaseCryptLibMock, X509GetTBSCert (_, _, _, _))
    .WillOnce (
       Invoke (
         [&] (CONST UINT8 *, UINTN, UINT8 **Tbs, UINTN *Size) -> BOOLEAN {
    *Tbs  = Payload;
    *Size = sizeof (Payload);
    return TRUE;
  }
         )
       );
  EXPECT_CALL (BaseCryptLibMock, HashAllByGuid (_, _, _, _, _))
    .WillOnce (Return (EFI_DEVICE_ERROR));

  ASSERT_EQ (BuildImageAuthority (NULL, Payload, sizeof (Payload), &Authority, NULL, NULL, 0, &Verification), EFI_SUCCESS);
  ASSERT_NE (Authority.Data, nullptr);
  EXPECT_EQ (Verification.Status, (UINT32)EFI_SIGNATURE_VERIFICATION_CERTIFICATE_AUTHORITY);
  EXPECT_EQ (Verification.ThumbprintSize, (UINTN)0);
  EXPECT_TRUE (IsZeroGuid (&Verification.ThumbprintAlgorithm));
  FreeImageAuthority (&Authority);
}

TEST_F (BuildImageAuthorityTest, MissingEnrolledHash_InvalidParameter) {
  UINT8            Payload[4] = { 1, 2, 3, 4 };
  IMAGE_AUTHORITY  Authority  = { };

  EXPECT_EQ (
    BuildImageAuthority (
      NULL,
      Payload,
      sizeof (Payload),
      &Authority,
      &gEfiHashAlgorithmSha256Guid,
      NULL,
      0,
      &Verification
      ),
    EFI_INVALID_PARAMETER
    );
  EXPECT_EQ (Authority.Data, nullptr);
}
