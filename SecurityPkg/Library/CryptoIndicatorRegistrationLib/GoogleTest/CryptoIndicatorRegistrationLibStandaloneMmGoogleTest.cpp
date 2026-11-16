/** @file
  Unit tests for Standalone MM ECIT registration and aligned entry limits.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockMmServicesTableLib.h>

#include <vector>

extern "C" {
  #include <Guid/CryptoIndicatorTable.h>
  #include <Guid/CryptoIndicatorMmBridge.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/CryptoIndicatorRegistrationLib.h>
}

using ::testing::_;
using ::testing::Invoke;
using ::testing::StrictMock;

static CONST EFI_GUID  mFeature = EFI_ECIT_FEATURE_SB_IMAGE_VERIFICATION_GUID;

/** Reject records whose aligned length cannot be represented. **/
TEST (CryptoIndicatorRegistrationLibStandaloneMmTest, RejectsUnrepresentableAlignedEntries) {
  StrictMock<MockMmServicesTableLib>  MmServices;
  UINT8                               Payload = 0;

  for (UINTN Size = 65505; Size <= 65511; Size++) {
    EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, &Payload, Size), EFI_INVALID_PARAMETER);
  }

  EXPECT_EQ (EcitRegisterCryptoCapability (&mFeature, &Payload, MAX_UINTN), EFI_INVALID_PARAMETER);
}

/** Verify ownership and serialization at the MM record size limit. **/
TEST (CryptoIndicatorRegistrationLibStandaloneMmTest, CopiesAndSerializesLargestAlignedEntry) {
  StrictMock<MockMmServicesTableLib>  MmServices;
  EFI_MM_HANDLER_ENTRY_POINT          Handler = nullptr;
  std::vector<UINT8>                  Payload (65504, 0x5A);

  EXPECT_CALL (MmServices, gMmst_MmInterruptRegister (_, _, _))
    .WillOnce (
       Invoke (
         [&Handler] (EFI_MM_HANDLER_ENTRY_POINT Callback, CONST EFI_GUID *Guid, EFI_HANDLE *Handle) {
    EXPECT_TRUE (CompareGuid (Guid, &gEcitMmBridgeHandlerGuid));
    Handler = Callback;
    *Handle = nullptr;
    return EFI_SUCCESS;
  }
         )
       );

  ASSERT_EQ (EcitRegisterCryptoCapability (&mFeature, Payload.data (), Payload.size ()), EFI_SUCCESS);
  ASSERT_NE (Handler, nullptr);
  SetMem (Payload.data (), Payload.size (), 0xFF);

  std::vector<UINT8>  Buffer (sizeof (ECIT_MM_BRIDGE_COMM) + sizeof (ECIT_MM_BRIDGE_ENTRY) + Payload.size (), 0);
  auto                Comm = (ECIT_MM_BRIDGE_COMM *)Buffer.data ();
  UINTN               Size = Buffer.size ();

  Comm->Signature = ECIT_MM_BRIDGE_COMM_SIGNATURE;
  Comm->Revision  = ECIT_MM_BRIDGE_COMM_REVISION;
  ASSERT_EQ (Handler (nullptr, nullptr, Comm, &Size), EFI_SUCCESS);
  ASSERT_EQ (Comm->ReturnStatus, EFI_SUCCESS);
  ASSERT_EQ (Comm->EntryCount, 1U);
  ASSERT_EQ (Comm->EntriesSize, sizeof (ECIT_MM_BRIDGE_ENTRY) + Payload.size ());

  auto  Entry = (ECIT_MM_BRIDGE_ENTRY *)(Comm + 1);
  auto  Data  = (UINT8 *)(Entry + 1);

  EXPECT_TRUE (CompareGuid (&Entry->FeatureIdentifier, &mFeature));
  ASSERT_EQ (Entry->DataSize, Payload.size ());
  for (UINTN Index = 0; Index < Payload.size (); Index++) {
    ASSERT_EQ (Data[Index], 0x5A);
  }
}

/** Run the registration tests with the supplied GoogleTest arguments. **/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
