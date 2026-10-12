/** @file
  Unit tests for the PEI ECIT registration library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHobLib.h>

#include <vector>

extern "C" {
  #include <Uefi.h>
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/EcitCapabilityHob.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/CryptoIndicatorRegistrationLib.h>
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

STATIC CONST EFI_GUID  mFeature        = EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID;
STATIC CONST UINTN     mMaxPayloadSize =
  0xFFF8U - sizeof (EFI_HOB_GUID_TYPE) - sizeof (EDKII_ECIT_CAPABILITY_HOB);

TEST (CryptoIndicatorRegistrationLibPeiTest, RejectsInvalidParameters) {
  StrictMock<MockHobLib>  HobLib;
  EFI_GUID                ZeroGuid;
  UINT8                   Payload;

  ZeroMem (&ZeroGuid, sizeof (ZeroGuid));
  Payload = 1;

  EXPECT_EQ (EcitRegisterCryptoCapability (nullptr, &Payload, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (EcitRegisterCryptoCapability (&ZeroGuid, &Payload, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, nullptr, sizeof (Payload)), EFI_INVALID_PARAMETER);
  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, &Payload, mMaxPayloadSize + 1), EFI_INVALID_PARAMETER);
}

TEST (CryptoIndicatorRegistrationLibPeiTest, ReturnsOutOfResources) {
  StrictMock<MockHobLib>  HobLib;

  EXPECT_CALL (HobLib, BuildGuidHob (_, sizeof (EDKII_ECIT_CAPABILITY_HOB)))
    .WillOnce (Return (nullptr));

  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, nullptr, 0), EFI_OUT_OF_RESOURCES);
}

TEST (CryptoIndicatorRegistrationLibPeiTest, AcceptsMaximumGuidHobPayload) {
  StrictMock<MockHobLib>  HobLib;
  CONST UINTN             HobDataSize = sizeof (EDKII_ECIT_CAPABILITY_HOB) + mMaxPayloadSize;
  std::vector<UINT8>      Payload (mMaxPayloadSize, 0x5A);
  std::vector<UINT8>      HobData (HobDataSize);

  EXPECT_CALL (HobLib, BuildGuidHob (_, HobDataSize))
    .WillOnce (Return (HobData.data ()));

  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, Payload.data (), Payload.size ()), EFI_SUCCESS);

  EDKII_ECIT_CAPABILITY_HOB  *CapabilityHob;

  CapabilityHob = (EDKII_ECIT_CAPABILITY_HOB *)HobData.data ();
  EXPECT_EQ (CapabilityHob->EntryDataSize, mMaxPayloadSize);
  EXPECT_EQ (CompareMem (CapabilityHob->EntryData, Payload.data (), Payload.size ()), 0);
}

TEST (CryptoIndicatorRegistrationLibPeiTest, BuildsAlignedCapabilityHob) {
  StrictMock<MockHobLib>  HobLib;
  STATIC CONST UINT8      Payload[]   = { 1, 2, 3, 4, 5 };
  CONST UINTN             HobDataSize = ALIGN_VALUE (sizeof (EDKII_ECIT_CAPABILITY_HOB) + sizeof (Payload), 8);
  std::vector<UINT8>      HobData (HobDataSize, 0xA5);

  EXPECT_CALL (HobLib, BuildGuidHob (_, HobDataSize))
    .WillOnce (
       Invoke (
         [&HobData] (
                     CONST EFI_GUID  *Guid,
                     UINTN           DataLength
         ) -> VOID * {
    EXPECT_TRUE (CompareGuid (Guid, &gEdkiiEcitCapabilityHobGuid));
    return HobData.data ();
  }
         )
       );

  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, Payload, sizeof (Payload)), EFI_SUCCESS);

  EDKII_ECIT_CAPABILITY_HOB  *CapabilityHob;

  CapabilityHob = (EDKII_ECIT_CAPABILITY_HOB *)HobData.data ();
  EXPECT_EQ (CapabilityHob->Revision, EDKII_ECIT_CAPABILITY_HOB_REVISION);
  EXPECT_EQ (CapabilityHob->HeaderSize, sizeof (*CapabilityHob));
  EXPECT_EQ (CapabilityHob->EntryDataSize, sizeof (Payload));
  EXPECT_TRUE (CompareGuid (&CapabilityHob->FeatureIdentifier, &mFeature));
  EXPECT_EQ (CompareMem (CapabilityHob->EntryData, Payload, sizeof (Payload)), 0);

  for (UINTN Index = sizeof (*CapabilityHob) + sizeof (Payload); Index < HobDataSize; Index++) {
    EXPECT_EQ (HobData[Index], 0);
  }
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
